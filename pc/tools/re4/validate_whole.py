"""Audit a compiled whole-map inspection artifact without claiming full traversal."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import numpy as np
from build_rooms import output_path, flood
from cameras_whole import ray_nodes
from navigation import raster
from village_validate import in_view
import rdx
import rdxbuild as b


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('recipe',type=Path);a=p.parse_args();spec=json.loads(a.recipe.read_text());out=output_path(spec['output'])
    raw=(out/'room.rdx.raw').read_bytes();room=rdx.Room(raw);b.validate_authored(room)
    j=json.loads((out/'room.json').read_text());w,d=math.ceil(j['room']['w']),math.ceil(j['room']['d'])
    xx=np.arange(w+1)[:,None];zz=np.arange(d+1)[None,:]
    mask=(xx>=5.5)&(xx<=w-5.5)&(zz>=5.5)&(zz<=d-5.5)
    for wall in room.records('walls'):
        radius=5.5 if wall['h']>12 else 3.5
        mask&=~((xx>wall['px']-radius)&(xx<wall['px']+wall['w']+radius)&(zz>wall['pz']-radius)&(zz<wall['pz']+wall['d']+radius))
    entry=room.records('positions')[0];spawn=(round(entry['px']),round(entry['pz']))
    # Flood directly over the compiled full-size mask, including space outside
    # the historical room dimensions, to detect an enclosure escape.
    import collections
    reached={spawn};queue=collections.deque([spawn])
    if not mask[spawn]:raise ValueError('compiled entry blocked')
    while queue:
        x,z=queue.popleft()
        for nx,nz in ((x-1,z),(x+1,z),(x,z-1),(x,z+1)):
            if 0<=nx<=w and 0<=nz<=d and mask[nx,nz] and (nx,nz) not in reached:reached.add((nx,nz));queue.append((nx,nz))
    groups=b.read_obj(out/'room.obj');tri=[];uvs=[];tids=[];nodes=[]
    for ni,g in enumerate(groups):
        for tid,face in g:tri.append([p for p,_ in face]);uvs.append([uv for _,uv in face]);tids.append(tid);nodes.append(ni)
    tri=np.array(tri);uvs=np.array(uvs);tids=np.array(tids);nodes=np.array(nodes)
    textures=[np.frombuffer(rdx.tim2_rgba(room.d,t),np.uint8).reshape(t['h'],t['w'],4) for _,ts in rdx.texture_blocks(room) for t in ts]
    low=np.min(list(reached),axis=0);high=np.max(list(reached),axis=0);shape=tuple(high-low+1);bounds=[low[0]-.5,low[1]-.5,high[0]+.5,high[1]+.5];support=np.zeros(shape,bool)
    # Actual compiled mesh, integer standing positions, no assumed flat rectangle.
    for face,tid in zip(tri,tids):
        if np.max(abs(face[:,1]))>1e-4 or np.any(textures[tid][:,:,3]<=127):continue
        n=np.cross(face[1]-face[0],face[2]-face[0])
        if np.linalg.norm(n)<1e-8 or abs(n[1])<.9*np.linalg.norm(n):continue
        support[raster(face.tolist(),bounds,1,shape,centers=True)]=True
    unsupported=sum(not support[x-low[0],z-low[1]] for x,z in reached)
    metadata=json.loads((out/'nodes.json').read_text())
    alpha_bounds=[]
    for cut in room.cuts():
        for cam in cut['cam']:
            alpha_bounds.append(sum(n.get('alpha_triangles',0) for n in metadata if n['index']>=512 or not (cam['hidobj'][n['index']//32]&(0x80000000>>(n['index']%32)))))
    if max(alpha_bounds)>32768-4096:raise ValueError('compiled alpha budget reserve exhausted')
    rays=0;occlusions=[];frustum=0;uncovered=0
    for x,z in reached:
        active=[c for c in room.cuts() if any(a['minx']<=x<=a['maxx'] and a['minz']<=z<=a['maxz'] for a in c['areas'])]
        if not active:uncovered+=1
        if any(not in_view(c['cam'][0],(x,y,z),1) for c in active for y in (0,17.5)):frustum+=1
    for ci,c in enumerate(room.cuts()):
        cam=c['cam'][0];targets=np.array([(x,17.5,z) for x,z in sorted(reached) if x%3==0 and z%3==0 and any(a['minx']<=x<=a['maxx'] and a['minz']<=z<=a['maxz'] for a in c['areas'])])
        if not len(targets):continue
        visible=np.array([not(cam['hidobj'][int(n)//32]&(0x80000000>>(int(n)%32))) for n in nodes]);origin=np.array([cam[k] for k in ('px','py','pz')])
        hits=ray_nodes(tri[visible],uvs[visible],tids[visible],nodes[visible],textures,origin,targets);rays+=len(targets)
        if hits:occlusions.append(dict(cut=ci,occluding_nodes=sorted(hits)))
    before=json.loads(Path('scratch/re4/whole-before.json').read_text());changed=[p for p,h in before.items() if not Path(p).exists() or hashlib.sha256(Path(p).read_bytes()).hexdigest()!=h]
    name=f'RM_{spec["id"]:04d}.RDX';packed=(Path('mods')/name).read_bytes();packed_ok=rdx.expand(packed)==raw
    report=dict(maximum_compiled_unhidden_alpha_triangles=max(alpha_bounds),sorted_primitive_reserve=4096,inspection_reachable_samples=len(reached),unsupported_compiled_mesh_samples=unsupported,uncovered_samples=uncovered,frustum_failures=frustum,head_rays=rays,occluded_cuts=occlusions,
                old_mods_changed=changed,installed_roundtrip=packed_ok,installed_sha256=hashlib.sha256(packed).hexdigest(),whole_map_navigation_pass=False,runtime_tested=False)
    (out/'offline-audit.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
    if changed or not packed_ok or unsupported or uncovered or frustum or occlusions:raise ValueError('inspection audit failed')

if __name__=='__main__':main()
