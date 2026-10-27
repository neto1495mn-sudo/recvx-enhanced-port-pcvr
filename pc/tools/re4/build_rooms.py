"""Build bounded RE4 room regions from an ignored, source-specific JSON recipe.

Source coordinates, partitions and asset bytes stay in scratch/mods. This tool does
not replace the square's paused outline survey. Each room has an explicitly chosen
simple walking region, conservative source collision, and searched fixed cameras.
"""
import argparse
import collections
import copy
import hashlib
import json
import math
from pathlib import Path
import sys
from types import SimpleNamespace
import numpy as np
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import analyze_obj_surfaces as obj
import rdx
import rdxbuild as b
import kitbash
from build_area import atr
from build_village import camera
from village_validate import walkable, in_view
from navigation import survey, clip_height, raster
from outline import rectangles
from materials import resolve, category
from tile import tiled
from visibility import visible


def output_path(path):
    path=Path(path).resolve()
    if not any(path.is_relative_to(Path(base).resolve()) for base in ('scratch','mods')):
        raise ValueError('generated output must be in scratch/ or mods/')
    path.mkdir(parents=True,exist_ok=True)
    return path


def flood(walk, start):
    reached={start}; queue=collections.deque([start])
    while queue:
        x,z=queue.popleft()
        for q in ((x-1,z),(x+1,z),(x,z-1),(x,z+1)):
            if q in walk and q not in reached:reached.add(q);queue.append(q)
    return reached


def blocked(opaque, origin, targets):
    if not len(opaque):return False
    a=opaque[:,0];e=opaque[:,1]-a;f=opaque[:,2]-a;t=np.asarray(origin)-a
    q=np.cross(t,e)
    for target in targets:
        d=np.asarray(target)-origin;p=np.cross(d,f);det=np.sum(e*p,axis=1)
        ok=abs(det)>1e-8;inv=1/np.where(ok,det,1)
        u=np.sum(t*p,axis=1)*inv;v=np.sum(d*q,axis=1)*inv;dist=np.sum(f*q,axis=1)*inv
        if np.any(ok&(u>=0)&(v>=0)&(u+v<=1)&(dist>.002)&(dist<.985)):return True
    return False


def cluster(selected, cell):
    result=[];seen=set()
    for surface,ps in selected:
        qs=tuple(tuple(round(v/cell)*cell for v in p) for p in ps)
        if len(set(qs))<3 or np.linalg.norm(np.cross(np.subtract(qs[1],qs[0]),np.subtract(qs[2],qs[0])))<1e-8:continue
        key=(surface,tuple(sorted(qs)))
        if key not in seen:seen.add(key);result.append((surface,qs))
    return result


def floor_cells(ps, tolerance, shape):
    """Raster only the portion of a nearly level triangle close to floor zero."""
    if not 0 <= tolerance <= .15:
        raise ValueError('flat-floor tolerance must be between 0 and .15')
    n=np.cross(np.subtract(ps[1],ps[0]),np.subtract(ps[2],ps[0]))
    length=np.linalg.norm(n)
    if length<1e-8 or abs(n[1]) < .9*length:
        return np.array([],dtype=int),np.array([],dtype=int)
    poly=clip_height(clip_height(list(ps),-tolerance,True),tolerance,False)
    return raster(poly,[0,0,*shape],1,shape,centers=True) if poly else (np.array([],dtype=int),np.array([],dtype=int))


def tile_samples(walk, ix, iz, size):
    return sorted((x,z) for x,z in walk if ix*size<=x<=(ix+1)*size and iz*size<=z<=(iz+1)*size)


def build(spec, recipe, args, faces, mapping):
    out=output_path(Path(recipe['output'])/str(spec['id']))
    scale=recipe['scale'];origin=spec['origin']
    def cv(p):return tuple((p[k]-origin[k])*scale for k in range(3))
    w,d=[math.ceil(v*scale) for v in spec['size']]
    transform={'scale':scale,'source_origin':origin}
    entry=cv(spec['entry']); selected=[]
    x0,z0,x1,z1=spec['crop']
    for g,m,ps in faces:
        c=np.mean(ps,axis=0)
        if not(x0<c[0]<x1 and z0<c[2]<z1):continue
        if m in recipe['exclude_materials']:continue
        normal=np.cross(np.subtract(ps[1],ps[0]),np.subtract(ps[2],ps[0]));length=np.linalg.norm(normal)
        if length<1e-8:continue
        ny=abs(normal[1])/length
        fallback='roof' if .2<ny<.9 and c[1]>origin[1]+30 else 'wood'
        surface,_=resolve(mapping,g,m,fallback)
        if surface is None or (category(surface) in ('opacity','foliage') and ny>.75):continue
        selected.append((surface,tuple(cv(p) for p in ps)))
    # One-unit source raster, only for this bounded prototype. Explicit corridor
    # limits are allowed here; unsupported source ground is never silently filled.
    relevant=[f for f in faces if min(p[0] for p in f[2])<origin[0]+spec['size'][0] and max(p[0] for p in f[2])>origin[0] and min(p[2] for p in f[2])<origin[2]+spec['size'][1] and max(p[2] for p in f[2])>origin[2]]
    navmap=copy.deepcopy(mapping)
    navmap['navigation_excluded_materials']=list(set(navmap.get('navigation_excluded_materials',[])+recipe['exclude_materials']))
    _,solid,ground,_=survey(relevant,navmap,transform,[0,0,w,d],1,[entry[0],entry[2]],0)
    # Source terrain has sub-unit seams. Accept only actual near-horizontal
    # surface portions within the declared flat-floor tolerance, never a void.
    tolerance=spec.get('floor_tolerance', .15)
    if not 0 <= tolerance <= .15:
        raise ValueError('flat-floor tolerance must be between 0 and .15')
    for g,m,ps in relevant:
        if m in recipe['exclude_materials']:continue
        surface,_=resolve(mapping,g,m,'wood')
        if surface is None or category(surface) in ('opacity','foliage','roof'):continue
        ps=[cv(p) for p in ps]
        ground[floor_cells(ps,tolerance,(w,d))]=True
    xx,zz=np.meshgrid(np.arange(w)+.5,np.arange(d)+.5,indexing='ij');allowed=np.zeros((w,d),bool)
    for bounds in spec['walk_rects']:
        a,_,c=cv((bounds[0],origin[1],bounds[1]));e,_,f=cv((bounds[2],origin[1],bounds[3]))
        allowed|=(xx>=a)&(xx<=e)&(zz>=c)&(zz<=f)
    mask=solid|~allowed
    np.savez_compressed(out/'navigation.npz', solid=solid, ground=ground, allowed=allowed)
    boxes=rectangles(mask)
    walls=[atr(px=x,pz=z,w=r-x,d=t-z,h=100) for x,z,r,t in boxes]
    if len(walls)>168:raise ValueError(f'{spec["id"]}: {len(walls)} walls exceed budget')
    walk=walkable(walls,w,d);spawn=(round(entry[0]),round(entry[2]))
    # Persist a plan before camera search, including failed later builds.
    from PIL import Image,ImageDraw
    im=Image.new('RGB',(w,d),'#282828');pix=im.load()
    for x,z in zip(*np.where(ground)):pix[int(x),int(z)]=(80,80,50)
    for x,z in zip(*np.where(mask)):pix[int(x),int(z)]=(120,85,65)
    for x,z in walk:pix[x,z]=(70,180,100)
    im.resize((w*3,d*3)).save(out/'walking.png')
    if spawn not in walk:raise ValueError(f'blocked entry {spawn}')
    reached=flood(walk,spawn)
    if reached!=walk:raise ValueError(f'{len(walk-reached)} disconnected walk samples')
    missing=[(x,z) for x,z in walk if not ground[min(x,w-1),min(z,d-1)]]
    if missing:raise ValueError(f'unsupported floor: {len(missing)} {missing[:10]}')
    opaque=np.array([ps for s,ps in selected if category(s) not in ('opacity','foliage')])
    cuts=[];tiles=collections.defaultdict(list);tile_size=spec.get('camera_tile',24)
    for x,z in sorted(walk):tiles[(x//tile_size,z//tile_size)].append((x,z))
    for (ix,iz),pts in sorted(tiles.items()):
        # Both cuts own their shared integer edge after compilation. Include
        # those points in each search instead of checking only the bucket.
        pts=tile_samples(walk,ix,iz,tile_size)
        tx,tz=np.mean(pts,axis=0);candidates=[]
        for h in spec.get('camera_heights',[24,30,38,46]):
            for dist in (24,32,44,58):
                for angle in range(0,360,30):
                    rad=math.radians(angle);cx=tx+math.cos(rad)*dist;cz=tz+math.sin(rad)*dist
                    if not(1<cx<w-1 and 1<cz<d-1):continue
                    for pers in (13000,15500):
                        cam=camera(cx,h,cz,tx,tz);cam['pers']=pers
                        cam['ax']=round(math.atan2(h-8,dist)*65536/math.tau)
                        candidates.append((math.hypot(h,dist)*(pers/13000),cam))
        chosen=None
        for _,cam in sorted(candidates,key=lambda q:q[0]):
            if not all(in_view(cam,(x,y,z),.92) for x,z in pts for y in (0,17.5)):continue
            o=np.array([cam[k] for k in ('px','py','pz')])
            # Camera must have a clear vertical route from the floor: never above
            # an interior ceiling or inside source geometry.
            if blocked(opaque,o,[(o[0],.1,o[2])]):continue
            targets=[(x,17.5,z) for x,z in pts if x%3==0 and z%3==0]
            targets += [(x,17.5,z) for x,z in (pts[0],pts[-1])]
            if blocked(opaque,o,targets):continue
            chosen=cam;break
        if chosen is None:raise ValueError(f'no unobstructed camera for tile {(ix,iz)} ({tx},{tz})')
        chosen.update(fog_col=0xff918474,fog_nr=-200.,fog_fr=-700.)
        bounds=(ix*tile_size-.01,iz*tile_size-.01,(ix+1)*tile_size+.01,(iz+1)*tile_size+.01)
        cuts.append((bounds,chosen))
        print(spec['id'],'camera',len(cuts),flush=True)
    selected=visible(cluster(selected,.5),[cam for _,cam in cuts],600)
    for cell in (.5,1,1.5,2,3,4):
        mesh=cluster(selected,cell);kit=kitbash.Kit(args);group=[]
        for s,ps in mesh:
            tid=kit.texture(s.room,s.texture,s.texture_list)
            normal=np.cross(np.subtract(ps[1],ps[0]),np.subtract(ps[2],ps[0]));drop=int(np.argmax(abs(normal)));axes=[k for k in range(3) if k!=drop]
            if category(s) in ('opacity','foliage'):
                lo=[min(p[k] for p in ps) for k in axes];hi=[max(p[k] for p in ps) for k in axes]
                group.append((tid,[(p,s.map_uv(tuple((p[k]-lo[i])/max(hi[i]-lo[i],1e-6) for i,k in enumerate(axes)))) for p in ps]))
            else:group.extend((tid,[(p,s.map_uv(uv)) for p,uv in face]) for face in tiled(ps,axes,128 if s.category=='roof' else 64))
        if len(group)<=12000:break
    if len(group)>13639:raise ValueError('geometry budget exceeded')
    kit.groups=[group];kit.write(out/'room.obj')
    j={'format':'recvx-room-1','mesh':'room.obj','retail_textures':kit.refs,
       'room':{'w':w,'d':d,'h':200,'amb_r':[.75]*4,'amb_g':[.7]*4,'amb_b':[.65]*4,'bak_col':0xff918474,'fog_col':0xff918474,'fog_nr':-200.,'fog_fr':-700.},
       'walls':walls,'floors':[atr(type=1,w=w,d=d),atr(type=2,w=w,d=d,prm0=1)],
       'routes':[],'route_next':[],'objects':[b.zero(rdx.ETTY) for _ in range(4)],
       'positions':[b.zero(rdx.POS,px=entry[0],py=0,pz=entry[2],ay=spec.get('entry_angle',0))],
       'cuts':[],'scripts':['3700b6000000','0000']}
    for bounds,cam in cuts:
        x,z,r,t=bounds
        j['cuts'].append(dict(b.zero(rdx.CUT,flg=1,cx=x,cz=z,cw=r-x,cd=t-z),cam=[cam,copy.deepcopy(cam)],areas=[b.zero(rdx.CTAB,minx=x,minz=z,maxx=r,maxz=t)],exd='00'*256))
    # Compact route regions derived from the clear grid; no route through a blocker.
    route_mask=np.zeros((w,d),bool)
    for x,z in walk:route_mask[x,z]=True
    rr=rectangles(route_mask)
    if len(rr)<=128:
        j['routes']=[atr(px=x,pz=z,w=r-x,d=t-z) for x,z,r,t in rr]
        j['route_next']=[[int(not(a[2]<c[0] or c[2]<a[0] or a[3]<c[1] or c[3]<a[1])) for c in rr] for a in rr]
    (out/'room.json').write_text(json.dumps(j,indent=2)+'\n')
    raw=b.compile_source(out/'room.json',args);room=rdx.Room(raw);b.validate_authored(room)
    textures=[t for _,ts in rdx.texture_blocks(room) for t in ts]
    for tid in {kit.texture(s.room,s.texture,s.texture_list) for s,_ in mesh if category(s) not in ('opacity','foliage')}:
        if any(v<=127 for v in rdx.tim2_rgba(room.d,textures[tid])[3::4]):raise ValueError('transparent donor on opaque class')
    for ref,t in zip(kit.refs,textures):
        donor=kit.room(ref['room']);dt=rdx.texture_blocks(donor)[ref['list']][1][ref['index']]
        if bytes(room.d[t['ofs']:t['ofs']+t['size']])!=bytes(donor.d[dt['ofs']:dt['ofs']+dt['size']]):raise ValueError('native texture changed')
    cw=walkable(room.records('walls'),w,d)
    if cw!=walk:raise ValueError('compiled collision changed')
    failures=0
    for x,z in cw:
        active=[c for c in room.cuts() if any(a['minx']<=x<=a['maxx'] and a['minz']<=z<=a['maxz'] for a in c['areas'])]
        if not active or any(not in_view(c['cam'][0],(x,y,z),1) for c in active for y in (0,17.5)):failures+=1
    if failures:raise ValueError(f'{failures} compiled camera failures')
    packed=rdx.compress(raw)
    if rdx.expand(packed)!=raw:raise ValueError('compression round trip failed')
    if len(raw)+len(packed)+3*1024*1024>=12845056:raise ValueError('arena budget')
    name=f'RM_{spec["id"]:04d}.RDX'
    if name in rdx.room_names():raise ValueError('retail room id collision')
    (out/'room.rdx.raw').write_bytes(raw)
    report={'id':name,'name':spec['name'],'teleport':f'1:{spec["id"]//1000}:{(spec["id"]//10)%100}:0','walkable_samples':len(cw),'walls':len(walls),'cameras':len(cuts),'triangles':len(group),'textures':len(textures),'cluster_cell':cell,'expanded_bytes':len(raw),'packed_bytes':len(packed),'source_support_failures':len(missing),'floor_tolerance':tolerance,'compiled_camera_failures':failures,'compression_roundtrip':True,'native_textures_preserved':True,'sha256':hashlib.sha256(packed).hexdigest(),'runtime_tested':False,'limitations':['bounded prototype corridors','sampled source mesh visibility','material substitutions','integer-center ground support; footprint seams need game acceptance']}
    (out/'validation.json').write_text(json.dumps(report,indent=2)+'\n')
    if args.publish:
        dest=output_path('mods')/name
        if dest.exists():
            archive=output_path('scratch/re4/archive');(archive/(name+'.'+hashlib.sha256(dest.read_bytes()).hexdigest()[:12])).write_bytes(dest.read_bytes())
        tmp=dest.with_suffix('.tmp');tmp.write_bytes(packed);tmp.replace(dest)
        dest.with_suffix('.SPQ.key').write_text('60\n')
    print(json.dumps(report),flush=True)


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('recipe',type=Path);p.add_argument('--room',type=int);p.add_argument('--publish',action='store_true');args=p.parse_args()
    args.recipe.resolve().relative_to(Path('scratch').resolve())
    recipe=json.loads(args.recipe.read_text());args.iso=recipe['iso'];mapping=json.loads(Path(recipe['mapping']).read_text())
    if recipe.get('overrides'):
        local=json.loads(Path(recipe['overrides']).read_text())
        for group,overrides in local.items():mapping.setdefault('group_overrides',{}).setdefault(group,{}).update(overrides)
    faces=list(obj.read_obj(recipe['obj']))
    for spec in recipe['rooms']:
        if args.room is None or args.room==spec['id']:build(spec,recipe,args,faces,mapping)

if __name__=='__main__':main()
