"""Stage full terrain, collision, cuts and contents; publish only after offline gates.

All source-specific placements, asset bytes and reports stay in clone-local
scratch/mods. Existing fallback rooms are preserved. A failed terrain movement
check cannot retarget the installed graveyard to an unfinished room.
"""
import argparse
import copy
import hashlib
import json
import math
from pathlib import Path
import sys
import struct
from types import SimpleNamespace
import numpy as np
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
import rdx
import rdxbuild as b
from build_rooms import output_path
from build_area import atr
from terrain_pc import Ground, movement, record_patch, coverage
from collision_whole import boundary_audit
from validate_outline import blocked_boxes
from survey_whole import terrain_flood
from refine_terrain import edge_audit
from cameras_whole import framed
from publish_regions import install


def available_flags(kind,retail):
    occupied=set(retail['it_flg' if kind=='items' else 'ed_flg'])
    for path in Path('mods').glob('RM_*.RDX'):
        room=rdx.Room(rdx.expand(path.read_bytes()));seen=[]
        if not rdx.disasm(room,seen)[1]:raise ValueError('unclean installed script '+str(path))
        for off in set(seen):
            if rdx.insn_len(room.d,off)<4:continue
            op=room.d[off];bit=struct.unpack_from('<H',room.d,off+2)[0]&1023
            if op in ((14,35) if kind=='items' else (13,34)):occupied.add(bit)
            if op in (4,5) and room.d[off+1]==(7 if kind=='items' else 3):occupied.add(bit)
            if kind=='enemies' and op==33:occupied.update(range(bit,bit+room.d[off+1]))
    return [bit for bit in range(511,-1,-1) if bit not in occupied]


def spread(points,n,excluded):
    selected=[];anchors=np.asarray(excluded,float)
    for _ in range(n):
        distance=np.min(np.sum((points[:,None,[0,2]]-anchors[None,:,[0,2]])**2,axis=2),axis=1)
        i=int(np.argmax(distance));selected.append(points[i].tolist());anchors=np.vstack((anchors,points[i]))
    return selected


def mesh_budget(room):
    model=rdx.model_objects(room);maximum_vertices=maximum_strip=triangles=0
    for obj in model['objects']:
        vl,pl,_,_=rdx.cnk_model(room.d,model['top'],obj['model'])
        vertices,_=rdx.parse_vlist(room.d,model['top']+vl)
        maximum_vertices=max(maximum_vertices,len(vertices))
        groups,_=rdx.parse_plist(room.d,model['top']+pl)
        triangles+=sum(max(0,len(strip)-2) for _,strips in groups for _,strip in strips)
        off=model['top']+pl
        while room.d[off]!=255:
            kind=room.d[off]
            if kind<8:off+=2
            elif kind<16:off+=4
            else:
                size=struct.unpack_from('<H',room.d,off+2)[0]
                if kind>=64:maximum_strip=max(maximum_strip,size)
                off+=4+2*size
    return dict(nodes=len(model['objects']),triangles=triangles,maximum_node_vertices=maximum_vertices,maximum_strip_shorts=maximum_strip)


def retarget_graveyard(target_entry,rid):
    path=Path('mods/RM_0020.RDX');old=rdx.Room(rdx.expand(path.read_bytes()));j=old.to_json()
    matches=[i for i,t in enumerate(j['triggers']) if t['type']==0 and (t['prm0'],t['prm1'])==(1,26)]
    if len(matches)!=1:raise ValueError('expected exactly one installed RE4 graveyard door')
    i=matches[0];j['triggers'][i].update(prm0=rid//1000,prm1=rid//10%100,prm2=target_entry)
    raw=b.relayout(old,j,{});new=rdx.Room(raw)
    # Only the three destination bytes of that trigger may change.
    expected=bytearray(old.d);rec=old.records('triggers')[i];rec.update(prm0=rid//1000,prm1=rid//10%100,prm2=target_entry)
    rdx.pack_into(rdx.ATR,expected,old.ptr['etcp']+i*rdx.ATR[0].size,rec)
    if bytes(expected)!=raw:raise ValueError('graveyard changed beyond target trigger')
    if not any(t['type']==0 and (t['prm0'],t['prm1'])==(1,22) for t in new.records('triggers')):raise ValueError('CVX door lost')
    return raw,dict(trigger=i,target_entry=target_entry,cvx_door_preserved=True,only_destination_changed=True)


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('recipe',type=Path);p.add_argument('--publish',action='store_true');a=p.parse_args()
    a.recipe.resolve().relative_to(Path('scratch').resolve());spec=json.loads(a.recipe.read_text());root=output_path(spec['output']);out=output_path(root/'final')
    if f'RM_{spec["id"]:04d}.RDX' in rdx.room_names():raise ValueError('whole-map ID collides with retail room')
    before_path=root/'final-before.json'
    if not before_path.exists():
        before_path.write_text(json.dumps({str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in Path('mods').glob('*') if p.is_file()},indent=2)+'\n')
    terrain=json.loads((root/'terrain/pc-slopes.json').read_text());collision=json.loads((root/'terrain/static-collision-candidates.json').read_text())
    cameras=json.loads((root/'cameras/report.json').read_text())
    for report in (terrain['report'],collision['report']):
        for name,digest in report['input_sha256'].items():
            path=Path(name) if Path(name).is_absolute() else root/'terrain'/name
            if hashlib.sha256(path.read_bytes()).hexdigest()!=digest:raise ValueError('stale terrain/collision '+str(path))
    for name,digest in cameras['input_sha256'].items():
        if hashlib.sha256((root/name).read_bytes()).hexdigest()!=digest:raise ValueError('stale cameras '+name)
    j=json.loads((root/'room.json').read_text());j['mesh']=str((root/'room.obj').resolve());j['budget']='pc'
    j['room']['grand']=terrain['grand'];j['walls']=collision['walls']+terrain['walls'];j['floors']=[]
    j['cuts']=json.loads((root/'cameras/cuts.json').read_text());j['triggers']=[]
    ground=Ground(terrain['walls']);entry=j['positions'][0];entry.update(py=ground.height(entry['px'],entry['pz']),flr_no=0)
    if entry['py'] is None:raise ValueError('entry has no slope')
    with np.load(root/'terrain/grid.npz') as grid:mask=grid['reachable'];height=grid['terrain']
    survey=json.loads((root/'terrain/report.json').read_text());cell=survey['cell_size']
    source_mask=mask.copy()
    static_boxes=[[r['px'],0,r['pz'],r['w'],200,r['d']] for r in collision['walls']]
    blocked=blocked_boxes(static_boxes,[0,0,0,0],cell,mask.shape,survey['actor_radius'])
    start=(int(entry['px']/cell),int(entry['pz']/cell))
    mask,_=terrain_flood(mask&~blocked,height,start,cell,survey['maximum_navigation_gradient'])
    if not mask[start]:raise ValueError('entry blocked by final collision')
    ix,iz=np.where(mask)
    points=np.column_stack(((ix+.5)*cell,height[ix,iz],(iz+.5)*cell))
    # Return through the historical graveyard landing, now in whole-map coordinates.
    connections=json.loads(Path(spec.get('connections','scratch/re4/connections.json')).read_text());offset=json.loads((root/'transform.json').read_text())['offset']
    gx,gz,angle=connections['graveyard']['square_landing'];gx+=offset[0];gz+=offset[2]
    closest=points[np.argmin((points[:,0]-gx)**2+(points[:,2]-gz)**2)];gx,_,gz=closest
    target_entry=len(j['positions']);j['positions'].append(b.zero(rdx.POS,px=float(gx),py=ground.height(gx,gz),pz=float(gz),ay=angle))
    tx,tz,tw,td=connections['graveyard']['square_trigger'];tx+=offset[0];tz+=offset[2]
    grave=rdx.Room(rdx.expand(Path('mods/RM_0020.RDX').read_bytes()))
    if len(grave.records('positions'))<=3:raise ValueError('graveyard return entry missing')
    j['triggers'].append(atr(type=0,px=tx,pz=tz,w=tw,d=td,prm0=0,prm1=2,prm2=3,prm3=255))
    args=SimpleNamespace(iso=spec['iso']);donor=rdx.load_room(args,'0060');retail=json.loads(Path(connections['retail_flags']).read_text())
    starts=[[e['px'],e['py'],e['pz']] for e in j['positions']];places=spread(points,11,starts)
    entry_script=bytearray.fromhex('3700b600');frame_script=bytearray();flags={}
    for kind,count in (('items',6),('enemies',5)):
        flags[kind]=available_flags(kind,retail)[:count]
        if len(flags[kind])<count:raise ValueError('not enough persistence flags')
        j[kind]=[];assets='item_assets' if kind=='items' else 'enemy_assets';j[assets]=[]
        for slot,bit in enumerate(flags[kind]):
            x,y,z=places.pop(0);y=ground.height(x,z)
            if kind=='items':
                record=0 if slot%2==0 else 2;rec=copy.deepcopy(donor.records('items')[record]);rec.update(px=x,py=y,pz=z,ay=0,flr_no=0,hide=[-1]*4)
                j[kind].append(rec);j[assets].append(dict(room='0060',record=record))
                trigger=len(j['triggers']);j['triggers'].append(atr(type=4,px=x-3,py=y,pz=z-3,w=6,d=6,prm0=slot))
                entry_script.extend(struct.pack('<BBHBB',35,slot,bit,trigger,0));frame_script.extend(struct.pack('<BBHBB',14,0,bit,trigger,0))
            else:
                j[kind].append(b.zero(rdx.ETTY,flg=1,id=1,mdlver=1,wrk_no=slot,px=x,py=y,pz=z,hide=[-1]*4))
                j[assets].append(dict(room='0060',record=0));entry_script.extend(struct.pack('<BBH',34,slot,bit));frame_script.extend(struct.pack('<BBH',13,slot,bit))
    j['scripts']=[(entry_script+bytes(2)).hex(),(frame_script+bytes(2)).hex()]
    (out/'room.json').write_text(json.dumps(j,indent=2)+'\n')
    print('Compiling final candidate',flush=True);raw=b.compile_source(out/'room.json',args);room=rdx.Room(raw);b.validate_authored(room,budget='pc')
    (out/'room.rdx.raw').write_bytes(raw);packed=rdx.compress(raw)
    if rdx.expand(packed)!=raw:raise ValueError('compression roundtrip')
    (out/f'RM_{spec["id"]:04d}.RDX').write_bytes(packed)
    slopes=[w for w in room.records('walls') if w['type']==6];compiled_ground=Ground(slopes)
    movement_report,movement_failures=movement(compiled_ground,source_mask,height,cell,terrain['grand'][2])
    coverage_samples,coverage_gaps=coverage(compiled_ground,source_mask,cell)
    compiled_static=[w for w in room.records('walls') if w['type']==0]
    openings=json.loads((root/'terrain/outline-candidates.json').read_text())['open_boundaries']
    closure=boundary_audit(openings,compiled_static[collision['report']['footprint_walls']:],cell,survey['actor_radius'])
    compiled_blocked=blocked_boxes([[r['px'],0,r['pz'],r['w'],200,r['d']] for r in compiled_static],[0,0,0,0],cell,mask.shape,survey['actor_radius'])
    if not np.array_equal(blocked,compiled_blocked):raise ValueError('compiled wall clearance changed')
    uncovered=frustum=0;maximum_source_center_error=0
    for x,y,z in points:
        sy=compiled_ground.height(x,z,y)
        if sy is None:sy=compiled_ground.height(x,z)
        if sy is None:raise ValueError('compiled terrain coverage gap')
        maximum_source_center_error=max(maximum_source_center_error,abs(sy-y))
        active=[c for c in room.cuts() if any(q['minx']<=x<=q['maxx'] and q['minz']<=z<=q['maxz'] for q in c['areas'])]
        if not active:uncovered+=1
        if any(not framed(c['cam'][0],np.array([[x,sy,z],[x,sy+17.5,z]]),1.) for c in active):frustum+=1
    grave_raw,grave_report=retarget_graveyard(target_entry,spec['id']);(out/'RM_0020.rdx.raw').write_bytes(grave_raw)
    grave_packed=rdx.compress(grave_raw)
    if rdx.expand(grave_packed)!=grave_raw:raise ValueError('graveyard roundtrip')
    (out/'RM_0020.RDX').write_bytes(grave_packed)
    blockers=[]
    if movement_report['failed_trajectories']:blockers.append('PC slope eligibility fails bidirectional movement')
    compiled_seams=edge_audit([record_patch(r) for r in slopes],1.)
    if compiled_seams['failed_edges']:blockers.append('compiled slope seam step')
    if maximum_source_center_error>terrain['report']['source_lattice_tolerance']+.01:blockers.append('compiled source center fit')
    if coverage_gaps or terrain['report']['uncovered_subcells']:blockers.append('terrain coverage gaps')
    if uncovered or frustum:blockers.append('compiled camera coverage/framing')
    if collision['report']['lost_baseline_samples']:blockers.append('collision loses navigation')
    if len(raw)+len(packed)+3*1024**2>64*1024**2:blockers.append('PC arena budget')
    if len(room.cuts())>=127 or len(j['walls'])>32767:blockers.append('record budget')
    budgets=mesh_budget(room)
    if budgets['maximum_node_vertices']>8189 or budgets['maximum_strip_shorts']>32767:blockers.append('compiled geometry buffer budget')
    inspection=rdx.Room((root/'room.rdx.raw').read_bytes())
    # The last table entry includes final 32-byte alignment padding; adding
    # actor entries makes model0 non-final and changes only that zero padding.
    geometry_preserved=b.split_table(room,'model')[0].rstrip(b'\0')==b.split_table(inspection,'model')[0].rstrip(b'\0')
    native_textures_preserved=b.split_table(room,'texture')[0]==b.split_table(inspection,'texture')[0]
    if not geometry_preserved or not native_textures_preserved:blockers.append('geometry or native room textures changed during assembly')
    boundary_count=len(closure['failed_edges'])
    if boundary_count:blockers.append('compiled support-edge walls fail closure')
    certificate_path=root/'terrain/continuous-patches.json'
    certificate=json.loads(certificate_path.read_text())['report'] if certificate_path.exists() else None
    # User acceptance replaces this optional certificate with compiled rectangle
    # coverage and exact engine movement; keep the old evidence for reference.
    metadata=json.loads((root/'nodes.json').read_text());alpha_bound=max(sum(n['alpha_triangles'] for n in metadata if not(c['cam'][0]['hidobj'][n['index']//32]&(0x80000000>>(n['index']%32)))) for c in room.cuts())
    if alpha_bound>32768-4096:blockers.append('sorted alpha budget')
    before=json.loads((root/'final-before.json').read_text())
    changed=[name for name,digest in before.items() if not Path(name).exists() or hashlib.sha256(Path(name).read_bytes()).hexdigest()!=digest]
    if changed:blockers.append('installed fallback files changed during staging')
    placement_errors=[]
    for kind in ('positions','items','enemies'):
        for i,rec in enumerate(room.records(kind)):
            y=compiled_ground.height(rec['px'],rec['pz'],rec['py'])
            if y is None or abs(y-rec['py'])>.01:placement_errors.append([kind,i,'surface'])
            ix,iz=int(rec['px']/cell),int(rec['pz']/cell)
            if not mask[ix,iz] or any(r['px']-survey['actor_radius']<rec['px']<r['px']+r['w']+survey['actor_radius'] and r['pz']-survey['actor_radius']<rec['pz']<r['pz']+r['d']+survey['actor_radius'] for r in compiled_static):placement_errors.append([kind,i,'collision'])
    if placement_errors:blockers.append('entry or actor surface height')
    report=dict(teleport='1:1:31:0',expanded_bytes=len(raw),packed_bytes=len(packed),sha256=hashlib.sha256(packed).hexdigest(),
                slopes=len(slopes),static_walls=len(collision['walls']),walkable_cells=len(points),cameras=len(room.cuts()),items=6,zombies=5,
                enemy_slots_reserved=45,persistence_flags=flags,maximum_seam_step=compiled_seams['maximum_gap'],maximum_source_center_error=maximum_source_center_error,
                maximum_unhidden_alpha=alpha_bound,sorted_primitive_reserve=4096,open_support_boundary_edges=boundary_count,
                continuous_source_certificate_passed=bool(certificate and not certificate['failed_polygons']),
                continuous_source_certificate_superseded=True,certificate_acceptance='Compiled quarter-unit rectangle coverage plus current engine movement',
                compiled_coverage_samples=coverage_samples,compiled_coverage_gaps=len(coverage_gaps),
                boundary_closure=closure,perimeter_restricted_samples=collision['report']['perimeter_restricted_samples'],
                perimeter_isolated_samples=collision['report']['disconnected_surviving_samples'],
                original_installed_files_changed=changed,placement_errors=placement_errors,
                geometry_nodes=len(metadata),geometry_triangles=sum(n['triangles'] for n in metadata),
                maximum_node_vertices=max(n['triangles']*3 for n in metadata),
                compiled_geometry_budget=budgets,geometry_preserved=geometry_preserved,native_room_textures_preserved=native_textures_preserved,
                textures=sum(len(ts) for _,ts in rdx.texture_blocks(room)),texture_bytes=sum(t['size'] for _,ts in rdx.texture_blocks(room) for t in ts),
                input_sha256={str(f):hashlib.sha256(f.read_bytes()).hexdigest() for f in (a.recipe,root/'room.obj',root/'terrain/pc-slopes.json',root/'terrain/static-collision-candidates.json',root/'cameras/cuts.json')},
                movement=movement_report,uncovered_camera_samples=uncovered,frustum_failures=frustum,graveyard=grave_report,
                compression_roundtrip=True,pc_room_slopes=bool(room.rom['flg']&b.ROOM_SLOPES),runtime_tested=False,
                publishable=not blockers,blockers=blockers,published=False)
    (out/'movement-failures.json').write_text(json.dumps(movement_failures,indent=2)+'\n')
    if a.publish and not blockers:
        install(spec['id'],packed);install(20,grave_packed);report['published']=True
        (output_path('mods')/'re4-whole-validation.json').write_text(json.dumps(report,indent=2)+'\n')
    (out/'validation.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2),flush=True)

if __name__=='__main__':main()
