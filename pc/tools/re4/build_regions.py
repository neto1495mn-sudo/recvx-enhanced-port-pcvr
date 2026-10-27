#!/usr/bin/env python3
"""Build bounded RE4 regions from an ignored JSON recipe and external OBJ/retail ISO.

No geometry or source-specific region coordinates belong in the tracked recipe. Outputs
are confined to scratch/; publish_regions.py validates and installs the resulting rooms.
"""
import argparse
import collections
import copy
import json
import math
import sys
from pathlib import Path

import numpy as np
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import analyze_obj_surfaces as obj
import kitbash
import rdx
import rdxbuild as b
from build_area import atr
from build_village import camera
from materials import resolve, category
from tile import tiled
from village_validate import in_view, walkable
from visibility import visible


def output_path(path):
    path = Path(path).resolve()
    if not path.is_relative_to(Path('scratch').resolve()):
        raise ValueError('generated region files must stay in scratch/')
    path.mkdir(parents=True, exist_ok=True)
    return path


def simplify(selected, cell):
    result, seen = [], set()
    for surface, points in selected:
        points = tuple(tuple(round(v/cell)*cell for v in p) for p in points)
        if len(set(points)) < 3:
            continue
        if np.linalg.norm(np.cross(np.subtract(points[1], points[0]), np.subtract(points[2], points[0]))) < 1e-8:
            continue
        key = surface, tuple(sorted(points))
        if key not in seen:
            seen.add(key)
            result.append((surface, points))
    return result


def ray_blocked(triangles, origin, targets):
    if not len(triangles):
        return False
    a = triangles[:, 0]
    e, f = triangles[:, 1]-a, triangles[:, 2]-a
    t = np.array(origin)-a
    q = np.cross(t, e)
    for target in targets:
        d = np.array(target)-origin
        p = np.cross(d, f)
        det = np.sum(e*p, axis=1)
        good = np.abs(det)>1e-9
        inv = np.where(good, 1/np.where(good, det, 1), 0)
        u, v = np.sum(t*p, axis=1)*inv, np.sum(d*q, axis=1)*inv
        distance = np.sum(f*q, axis=1)*inv
        if np.any(good & (u>=0) & (v>=0) & (u+v<=1) & (distance>.001) & (distance<.99)):
            return True
    return False


def support_missing(triangles, points, tolerance=.15):
    """Require an actual nearly horizontal source surface at the rebased floor."""
    triangles = np.array([p for p in triangles if max(abs(v[1]) for v in p) <= tolerance])
    if not len(triangles):
        return list(points)
    a, bb, c = triangles[:, 0], triangles[:, 1], triangles[:, 2]
    den = (bb[:,2]-c[:,2])*(a[:,0]-c[:,0])+(c[:,0]-bb[:,0])*(a[:,2]-c[:,2])
    good = abs(den)>1e-9
    den = np.where(good, den, 1)
    missing=[]
    for x,z in points:
        u=((bb[:,2]-c[:,2])*(x-c[:,0])+(c[:,0]-bb[:,0])*(z-c[:,2]))/den
        v=((c[:,2]-a[:,2])*(x-c[:,0])+(a[:,0]-c[:,0])*(z-c[:,2]))/den
        if not np.any(good & (u>=-1e-5) & (v>=-1e-5) & (u+v<=1+1e-5)):
            missing.append((x,z))
    return missing


def auto_camera(tile, walk, opaque, size, interior):
    x0,z0,x1,z1=tile
    points=[(x,z) for x,z in walk if x0-1<=x<=x1+1 and z0-1<=z<=z1+1]
    if not points:
        raise ValueError('empty camera tile')
    tx,tz=(x0+x1)/2,(z0+z1)/2
    targets=[(x,17.5,z) for x,z in points if x%3==0 and z%3==0]
    candidates=[]
    for height in ((21,24,27) if interior else (25,30,35,40,45)):
        for angle in range(0,360,45):
            for dist in range(15,86,5):
                cx=tx+math.sin(math.radians(angle))*dist
                cz=tz+math.cos(math.radians(angle))*dist
                if not (1<cx<size[0]-1 and 1<cz<size[1]-1):
                    continue
                for pers in (13000,15000,17000):
                    cam=camera(cx,height,cz,tx,tz)
                    cam['ax']=round(math.atan2(height-8,dist)*65536/math.tau)
                    cam['pers']=pers
                    candidates.append((math.hypot(height,dist)*(1+(pers-13000)/22000),cam))
    candidates.sort(key=lambda c:c[0])
    for _,cam in candidates:
        if not all(in_view(cam,(x,y,z),.94) for x,z in points for y in (0,17.5)):
            continue
        origin=np.array([cam[k] for k in ('px','py','pz')])
        if ray_blocked(opaque,origin,targets):
            continue
        return cam
    raise ValueError(f'no unobstructed camera for {tile}; subdivide the tile or review bounds')


def build(spec, recipe, args, faces, mapping):
    out=output_path(Path(recipe['output'])/str(spec['id']))
    scale=recipe['scale']; origin=spec['origin']
    def convert(p):
        return tuple((p[k]-origin[k])*scale for k in range(3))
    bounds=spec['region']; selected=[]; support=[]; uses=collections.Counter()
    excluded=set(recipe.get('exclude_materials',[]))
    for group,material,points in faces:
        center=np.mean(points,axis=0)
        if not(bounds[0]<center[0]<bounds[2] and bounds[1]<center[2]<bounds[3]) or material in excluded:
            continue
        p=tuple(convert(v) for v in points)
        normal=np.cross(np.subtract(p[1],p[0]),np.subtract(p[2],p[0])); length=np.linalg.norm(normal)
        if length<1e-8:
            continue
        ny=abs(normal[1])/length
        cl=mapping['materials'][material]['class']
        fallback='roof' if .2<ny<.9 and center[1]-origin[1]>30 else 'wood'
        surface,decision=resolve(mapping,group,material,fallback if cl=='unknown' else cl)
        if surface is None or (surface.category in ('opacity','foliage') and ny>.75):
            continue
        selected.append((surface,p));uses[(material,surface.category)]+=1
        if surface.category not in ('opacity','foliage','roof'):
            support.append(p)
    w,d=spec['size']; x0,z0,x1,z1=spec['walk']
    walls=[atr(px=0,pz=0,w=x0,d=d,h=200),atr(px=x1,pz=0,w=w-x1,d=d,h=200),
           atr(px=x0,pz=0,w=x1-x0,d=z0,h=200),atr(px=x0,pz=z1,w=x1-x0,d=d-z1,h=200)]
    for x,z,ww,dd in spec.get('obstacles',[]):
        walls.append(atr(px=x,pz=z,w=ww,d=dd,h=30))
    walk=walkable(walls,math.ceil(w),math.ceil(d))
    missing=support_missing(support,walk)
    if missing:
        raise ValueError(f"{spec['name']}: {len(missing)}/{len(walk)} unsupported floor samples; first {missing[:5]}")
    opaque=np.array([p for s,p in selected if category(s) not in ('opacity','foliage')])
    # Reject any source solid crossing the allowed standing volume. Thin wall triangles
    # are sampled via segment/triangle rays horizontally at several actor heights below.
    bad=[]
    for x,z in walk:
        if ray_blocked(opaque,np.array((x,.3,z)),[(x,17.5,z)]):
            bad.append((x,z))
    if bad:
        (out/'standing-obstructions.json').write_text(json.dumps(bad))
        raise ValueError(f"{spec['name']}: geometry intersects {len(bad)} standing samples: {bad[:5]}")
    cuts=[]
    for tile in spec['tiles']:
        cam=auto_camera(tile,walk,opaque,(w,d),spec.get('interior',False))
        cam.update(fog_col=0xff918474,fog_nr=-200.,fog_fr=-700.)
        cuts.append((tile,cam));print(spec['name'],'camera',len(cuts),[round(cam[k],1) for k in ('px','py','pz')],flush=True)
    selected=visible(simplify(selected,.25),[cam for _,cam in cuts],800)
    for cell in (.25,.5,1,1.5,2,3):
        mesh=simplify(selected,cell);kit=kitbash.Kit(args);group=[]
        for surface,points in mesh:
            tid=kit.texture(surface.room,surface.texture,surface.texture_list)
            normal=np.cross(np.subtract(points[1],points[0]),np.subtract(points[2],points[0]))
            axes=[k for k in range(3) if k!=int(np.argmax(abs(normal)))]
            if surface.category in ('opacity','foliage'):
                lo=[min(p[k] for p in points) for k in axes];hi=[max(p[k] for p in points) for k in axes]
                face=[(p,surface.map_uv(tuple((p[k]-lo[i])/max(hi[i]-lo[i],1e-6) for i,k in enumerate(axes)))) for p in points]
                group.append((tid,face))
            else:
                group.extend((tid,[(p,surface.map_uv(uv)) for p,uv in face]) for face in tiled(points,axes,128 if surface.category=='roof' else 64))
        if len(group)<=12000:
            break
    if len(group)>13639:
        raise ValueError('geometry budget exceeded')
    kit.groups=[group];kit.write(out/'room.obj')
    j={'format':'recvx-room-1','mesh':'room.obj','retail_textures':kit.refs,
       'room':{'w':w,'d':d,'h':200,'amb_r':[.75]*4,'amb_g':[.7]*4,'amb_b':[.65]*4,'bak_col':0xff918474,'fog_col':0xff918474,'fog_nr':-200.,'fog_fr':-700.},
       'walls':walls,'floors':[atr(type=1,px=x0,pz=z0,w=x1-x0,d=z1-z0),atr(type=2,px=x0,pz=z0,w=x1-x0,d=z1-z0,prm0=1)],
       'routes':[atr(px=x0,pz=z0,w=x1-x0,d=z1-z0)],'route_next':[[0]],'objects':[b.zero(rdx.ETTY) for _ in range(4)],
       'cuts':[],'positions':[b.zero(rdx.POS,px=spec['entry'][0],pz=spec['entry'][1],ay=0)],'scripts':['3700b6000000','0000']}
    for tile,cam in cuts:
        left,top,right,bottom=tile
        j['cuts'].append(dict(b.zero(rdx.CUT,flg=1,cx=left,cz=top,cw=right-left,cd=bottom-top),
                             cam=[cam,copy.deepcopy(cam)],areas=[b.zero(rdx.CTAB,minx=left-1,minz=top-1,maxx=right+1,maxz=bottom+1)],exd='00'*256))
    (out/'room.json').write_text(json.dumps(j,indent=2)+'\n')
    raw=b.compile_source(out/'room.json',args);room=rdx.Room(raw);b.validate_authored(room)
    textures=rdx.texture_blocks(room)[0][1]
    for tid in {kit.texture(s.room,s.texture,s.texture_list) for s,_ in mesh if s.category not in ('opacity','foliage')}:
        if any(alpha<=127 for alpha in rdx.tim2_rgba(room.d,textures[tid])[3::4]):
            raise ValueError(f'opaque surfaces use transparent donor {kit.refs[tid]}')
    (out/'room.rdx.raw').write_bytes(raw)
    report={'id':spec['id'],'name':spec['name'],'triangles':len(group),'vertices':3*len(group),'cluster_size_game_units':cell,
            'transform':{'scale':scale,'source_origin':origin},'play_bounds':spec['walk'],'footprints':[(x,0,z,ww,30,dd) for x,z,ww,dd in spec.get('obstacles',[])],
            'source_ground_missing_samples':len(missing),'walkable_samples':len(walk),'material_uses':[{'material':m,'class':c,'triangles':n} for (m,c),n in uses.items()],
            'limitations':['bounded prototype traversal','substitute retail materials','runtime acceptance pending']}
    (out/'build-report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(spec['name'],len(group),'triangles',len(raw),'bytes',flush=True)


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('recipe',type=Path);p.add_argument('--only',type=int);p.add_argument('--iso')
    args=p.parse_args();recipe=json.loads(args.recipe.read_text())
    args.iso=args.iso or recipe.get('iso')
    mapping=json.loads(Path('pc/tools/area/re4_materials.json').read_text())
    overrides=Path(recipe.get('group_overrides','scratch/re4/group-overrides.json'))
    if overrides.exists():
        for group,rows in json.loads(overrides.read_text()).items():
            mapping.setdefault('group_overrides',{}).setdefault(group,{}).update(rows)
    faces=obj.read_obj(recipe['source'])
    for spec in recipe['rooms']:
        if args.only is None or spec['id']==args.only:
            build(spec,recipe,args,faces,mapping)


if __name__=='__main__':
    main()
