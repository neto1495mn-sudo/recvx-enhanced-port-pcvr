"""Compile an uncropped PC-budget mesh with a preserved inspection enclosure.

Recipes and every source-derived artifact must be under scratch/. This is a
geometry milestone, not permission to publish unvalidated terrain traversal.
"""
import argparse
import collections
import copy
import hashlib
import json
import math
from pathlib import Path
from types import SimpleNamespace
import numpy as np
from build_rooms import output_path, cluster, flood
import analyze_obj_surfaces as obj
import rdx
import rdxbuild as b
import kitbash
from materials import resolve, category
from tile import tiled
from village_validate import walkable, in_view


def write_mesh(path, groups, alpha_tids=()):
    """Keep source shells separate and split only at the PC vertex limit."""
    nodes=[];n=0
    with path.open('w') as stream:
        for label,faces in groups:
            for start in range(0,len(faces),2729):
                part=faces[start:start+2729]
                stream.write(f'o node{len(nodes)}\n')
                for tid,face in part:
                    stream.write(f'usemtl tex{tid}\n')
                    for p,uv in face:
                        stream.write('v %.6f %.6f %.6f\n'%tuple(p))
                        stream.write('vt %.6f %.6f\n'%(uv[0],1-uv[1]))
                    stream.write('f '+' '.join(f'{n+k}/{n+k}' for k in (1,2,3))+'\n');n+=3
                points=np.array([p for _,f in part for p,_ in f])
                nodes.append(dict(index=len(nodes),source_group=label,triangles=len(part),
                                  bounds=[points.min(axis=0).tolist(),points.max(axis=0).tolist()],
                                  alpha_triangles=sum(t in alpha_tids for t,_ in part)))
    return nodes


def limit_sorted_nodes(cam, nodes, reserve=4096):
    """Conservative all-unhidden alpha bound, even without frustum clipping."""
    masks=cam.setdefault('hidobj',[0]*16)
    def hidden(n):return n['index']<512 and masks[n['index']//32] & (0x80000000>>(n['index']%32))
    remaining=sum(n.get('alpha_triangles',0) for n in nodes if not hidden(n))
    def distance(n):
        lo,hi=n['bounds']
        return math.hypot((lo[0]+hi[0])/2-cam['px'],(lo[2]+hi[2])/2-cam['pz'])
    for n in sorted(nodes,key=distance,reverse=True):
        if n['index']>=512 or hidden(n) or n.get('alpha_triangles',0)!=n['triangles']:continue
        if remaining<=32768-reserve and distance(n)<=350:continue
        masks[n['index']//32]|=0x80000000>>(n['index']%32);remaining-=n['alpha_triangles']
    if remaining>32768-reserve:raise ValueError('sorted primitive reserve exhausted')
    return remaining


def translated_enclosure(base, offset):
    j=copy.deepcopy(base)
    # Do not carry obsolete links, actors or scripts into the inspection room.
    for key in ('triggers','items','enemies','item_assets','enemy_assets','object_assets','routes','route_next'):
        j.pop(key,None)
    j['objects']=[b.zero(rdx.ETTY) for _ in range(4)]
    j['positions']=j['positions'][:1]
    j['scripts']=['3700b6000000','0000']
    for key in ('walls','floors','positions'):
        for rec in j.get(key,[]):
            for axis,v in zip(('px','py','pz'),offset):
                if axis in rec:rec[axis]+=v
    for cut in j['cuts']:
        cut['cx']+=offset[0];cut['cz']+=offset[2]
        for a in cut['areas']:
            for k,v in (('minx',offset[0]),('maxx',offset[0]),('minz',offset[2]),('maxz',offset[2])):a[k]+=v
        for cam in cut['cam']:
            for k,v in zip(('px','py','pz'),offset):cam[k]+=v
            cam['hidobj']=[0]*16
            cam.update(fog_nr=-1500.,fog_fr=-4000.)
    return j


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('recipe',type=Path);p.add_argument('--publish-inspection',action='store_true');args=p.parse_args()
    args.recipe.resolve().relative_to(Path('scratch').resolve())
    spec=json.loads(args.recipe.read_text());out=output_path(spec['output']);args.iso=spec['iso']
    mapping=json.loads(Path(spec['mapping']).read_text())
    if spec.get('overrides'):mapping['group_overrides']=json.loads(Path(spec['overrides']).read_text())
    faces=obj.read_obj(spec['obj']);points=np.array([p for _,_,ps in faces for p in ps]);scale=spec['scale']
    # Integral translation preserves integer collision/camera sample alignments.
    old_origin=np.array(spec['inspection_origin']);low=(points.min(axis=0)-old_origin)*scale
    offset=np.array([math.ceil(-low[0])+16,0,math.ceil(-low[2])+16])
    origin=old_origin-offset/scale;maximum=((points.max(axis=0)-origin)*scale)
    size=[math.ceil(maximum[0])+16,math.ceil(maximum[2])+16]
    (out/'transform.json').write_text(json.dumps(dict(scale=scale,source_origin=origin.tolist(),offset=offset.tolist(),bounds=[0,0,*size]),indent=2)+'\n')
    selected=collections.defaultdict(list);excluded=collections.Counter();fallback=collections.Counter()
    for g,m,ps in faces:
        if m in spec.get('exclude_materials',[]):excluded[m]+=1;continue
        ps=(np.asarray(ps)-origin)*scale;n=np.cross(ps[1]-ps[0],ps[2]-ps[0]);length=np.linalg.norm(n)
        if length<1e-8:excluded['degenerate']+=1;continue
        ny=abs(n[1])/length
        surface,info=resolve(mapping,g,m,'roof' if .2<ny<.9 and ps[:,1].mean()>30 else 'wood')
        if surface is None:excluded['mapping_exclusion']+=1;continue
        if info.get('provisional_fallback'):fallback[m]+=1
        selected[(g,'terrain' if surface.category=='ground' else 'detail')].append((surface,tuple(map(tuple,ps))))
    kit=kitbash.Kit(args);groups=[];counts=collections.Counter()
    # No decimation unless explicitly requested by the ignored recipe.
    for g,mesh in selected.items():
        if spec.get('cluster_cell',0) and g[1]!='terrain':mesh=cluster(mesh,spec['cluster_cell'])
        group=[]
        for s,ps in mesh:
            tid=kit.texture(s.room,s.texture,s.texture_list)
            normal=np.cross(np.subtract(ps[1],ps[0]),np.subtract(ps[2],ps[0]));axes=[k for k in range(3) if k!=int(np.argmax(abs(normal)))]
            if category(s) in ('opacity','foliage'):
                lo=[min(p[k] for p in ps) for k in axes];hi=[max(p[k] for p in ps) for k in axes]
                part=[[(p,s.map_uv(tuple((p[k]-lo[i])/max(hi[i]-lo[i],1e-6) for i,k in enumerate(axes)))) for p in ps]]
            else:
                part=[[(p,s.map_uv(uv)) for p,uv in face] for face in tiled(ps,axes,spec.get('texture_tile',64))]
            group.extend((tid,f) for f in part);counts[s.category]+=len(part)
        groups.append((g,group))
    # Opaque shells first; pure vegetation last. No distant/background culling.
    alpha_tids={kit.texture(s.room,s.texture,s.texture_list) for mesh in selected.values() for s,_ in mesh if category(s) in ('opacity','foliage')}
    groups.sort(key=lambda row:(row[0][1]=='terrain',all(t in alpha_tids for t,_ in row[1])))
    nodes=write_mesh(out/'room.obj',groups,alpha_tids)
    (out/'nodes.json').write_text(json.dumps(nodes,indent=2)+'\n')
    if len(nodes)>512:raise ValueError('hideable source groups exceed 512; partition by role before publication')
    base=json.loads(Path(spec['inspection_room']).read_text());j=translated_enclosure(base,offset.tolist())
    j.update(budget='pc',mesh='room.obj',retail_textures=kit.refs)
    sorted_bounds=[]
    for cut in j['cuts']:
        for cam in cut['cam']:sorted_bounds.append(limit_sorted_nodes(cam,nodes))
    j['room'].update(w=size[0],d=size[1],h=math.ceil(maximum[1])+32,fog_nr=-1500.,fog_fr=-4000.)
    (out/'room.json').write_text(json.dumps(j,indent=2)+'\n')
    print('Compiling',sum(n['triangles'] for n in nodes),'triangles in',len(nodes),'nodes',flush=True)
    raw=b.compile_source(out/'room.json',args);room=rdx.Room(raw);b.validate_authored(room)
    (out/'room.rdx.raw').write_bytes(raw)
    textures=[t for _,ts in rdx.texture_blocks(room) for t in ts]
    for ref,t in zip(kit.refs,textures):
        donor=kit.room(ref['room']);dt=rdx.texture_blocks(donor)[ref['list']][1][ref['index']]
        if bytes(room.d[t['ofs']:t['ofs']+t['size']])!=bytes(donor.d[dt['ofs']:dt['ofs']+dt['size']]):raise ValueError('native donor mismatch')
    # Only certify the preserved enclosure; never label the exterior navigable.
    walk=walkable(base['walls'],math.ceil(base['room']['w']),math.ceil(base['room']['d']))
    walk={(int(x+offset[0]),int(z+offset[2])) for x,z in walk}
    entry=room.records('positions')[0];spawn=(round(entry['px']),round(entry['pz']))
    if spawn not in walk or flood(walk,spawn)!=walk:raise ValueError('inspection enclosure disconnected')
    failures=0
    for x,z in walk:
        cuts=[c for c in room.cuts() if any(a['minx']<=x<=a['maxx'] and a['minz']<=z<=a['maxz'] for a in c['areas'])]
        if not cuts or any(not in_view(c['cam'][0],(x,y,z),1) for c in cuts for y in (0,17.5)):failures+=1
    if failures:raise ValueError('inspection camera coverage failed')
    print('Compressing and round-tripping',len(raw),'bytes',flush=True)
    packed=rdx.compress(raw)
    if rdx.expand(packed)!=raw:raise ValueError('compression roundtrip')
    if len(raw)+len(packed)>64*1024**2:raise ValueError('PC load budget')
    name=f'RM_{spec["id"]:04d}.RDX'
    if name in rdx.room_names():raise ValueError('retail id conflict')
    report=dict(room=name,teleport=f'1:{spec["id"]//1000}:{spec["id"]//10%100}:0',budget='pc',
                source_triangles=len(faces),triangles=sum(n['triangles'] for n in nodes),nodes=len(nodes),
                max_node_vertices=max(n['triangles']*3 for n in nodes),max_strip_shorts=1+10*max(n['triangles'] for n in nodes),
                class_triangles=dict(counts),maximum_unhidden_alpha_triangles=max(sorted_bounds),textures=len(textures),expanded_bytes=len(raw),packed_bytes=len(packed),
                load_bytes=len(raw)+len(packed),sha256=hashlib.sha256(packed).hexdigest(),
                native_textures_preserved=True,compression_roundtrip=True,cluster_cell=spec.get('cluster_cell',0),
                source_bounds=[points.min(axis=0).tolist(),points.max(axis=0).tolist()],excluded=dict(excluded),fallback_materials=dict(fallback),
                inspection_walk_samples=len(walk),inspection_camera_failures=failures,
                whole_village_navigation_complete=False,runtime_tested=False,
                limitations=['Inspection enclosure only; exterior terrain traversal not installed',
                             'Existing camera framing checked; full uncropped mesh occlusion needs recheck',
                             'No new items, enemies or links; old rooms and graveyard unchanged',
                             'Material fallbacks and alpha-card UVs remain provisional'])
    (out/'validation.json').write_text(json.dumps(report,indent=2)+'\n')
    (out/name).write_bytes(packed)
    if args.publish_inspection:
        dest=output_path('mods')/name
        if dest.exists():
            archive=output_path('scratch/re4/archive');(archive/(name+'.'+hashlib.sha256(dest.read_bytes()).hexdigest()[:12])).write_bytes(dest.read_bytes())
        dest.write_bytes(packed);dest.with_suffix('.SPQ.key').write_text('60\n')
        (dest.parent/'re4-whole-validation.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2),flush=True)

if __name__=='__main__':main()
