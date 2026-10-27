"""Search terrain-relative fixed-camera candidates over a whole-map survey.

Writes candidate cuts and mesh-visibility evidence; does not install unvalidated
terrain navigation. Hide masks are restricted to detail nodes below index 512.
"""
import argparse
import copy
import hashlib
import json
import math
from pathlib import Path
import numpy as np
from build_rooms import output_path
from build_whole import limit_sorted_nodes
from build_village import camera
from village_validate import axes
import rdxbuild as b
import rdx


def framed(cam, points, margin=.94):
    f,r,u=map(np.array,axes(cam));rel=points-np.array([cam[k] for k in ('px','py','pz')]);depth=rel@f;t=math.tan(cam['pers']*math.pi/65536)*margin
    return bool(np.all((depth>0)&(abs(rel@r)<depth*t)&(abs(rel@u)<depth*t*.75)))


def ray_nodes(tri, uvs, tids, nodes, textures, origin, targets):
    """Alpha-tested finite segments; broadphase before triangle intersections."""
    low=np.minimum(np.min(targets,axis=0),origin)-.01;high=np.maximum(np.max(targets,axis=0),origin)+.01
    keep=np.all(tri.max(axis=1)>=low,axis=1)&np.all(tri.min(axis=1)<=high,axis=1)
    tri=tri[keep];uvs=uvs[keep];tids=tids[keep];nodes=nodes[keep]
    a=tri[:,0];e=tri[:,1]-a;f=tri[:,2]-a;t=origin-a;q=np.cross(t,e);hits=set()
    for target in targets:
        d=target-origin;p=np.cross(d,f);det=np.sum(e*p,axis=1);ok=abs(det)>1e-8;inv=1/np.where(ok,det,1)
        u=np.sum(t*p,axis=1)*inv;v=np.sum(d*q,axis=1)*inv;dist=np.sum(f*q,axis=1)*inv
        for k in np.where(ok&(u>=0)&(v>=0)&(u+v<=1)&(dist>.002)&(dist<.995))[0]:
            st=(1-u[k]-v[k])*uvs[k,0]+u[k]*uvs[k,1]+v[k]*uvs[k,2];tex=textures[tids[k]]
            if tex[int(np.clip(st[1]*(len(tex)-1),0,len(tex)-1)),int(np.clip(st[0]*(tex.shape[1]-1),0,tex.shape[1]-1)),3]>127:hits.add(int(nodes[k]))
    return hits


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('directory',type=Path);p.add_argument('--tile',type=int,default=48);p.add_argument('--dense-rays',action='store_true');a=p.parse_args();out=output_path(a.directory/'cameras')
    j=json.loads((a.directory/'room.json').read_text());survey=json.loads((a.directory/'terrain/report.json').read_text());cell=survey['cell_size']
    with np.load(a.directory/'terrain/grid.npz') as grid:terrain=grid['terrain'];reached=grid['reachable'];allowed=grid['allowed']
    ix,iz=np.where(reached);points=np.column_stack(((ix+.5)*cell,terrain[ix,iz],(iz+.5)*cell))
    groups=b.read_obj(a.directory/'room.obj');tri=[];uvs=[];tids=[];nodes=[]
    for ni,g in enumerate(groups):
        for tid,face in g:
            tri.append([p for p,_ in face]);uvs.append([uv for _,uv in face]);tids.append(tid);nodes.append(ni)
    tri=np.array(tri);uvs=np.array(uvs);tids=np.array(tids);nodes=np.array(nodes)
    metadata=json.loads((a.directory/'nodes.json').read_text());unhideable={n['index'] for n in metadata if n['source_group'][1]=='terrain' or n['index']>=512}
    room=rdx.Room((a.directory/'room.rdx.raw').read_bytes());textures=[np.frombuffer(rdx.tim2_rgba(room.d,t),np.uint8).reshape(t['h'],t['w'],4) for _,ts in rdx.texture_blocks(room) for t in ts]
    tile=a.tile
    while len(set(map(tuple,np.floor(points[:,[0,2]]/tile).astype(int))))>120:tile+=4
    tiles=sorted(set(map(tuple,np.floor(points[:,[0,2]]/tile).astype(int))));cuts=[];fail=[];witnesses=[]
    for tx,tz in tiles:
        x,z=int(tx)*tile,int(tz)*tile;r,t=x+tile,z+tile
        ps=points[(points[:,0]>=x-cell/2)&(points[:,0]<=r+cell/2)&(points[:,2]>=z-cell/2)&(points[:,2]<=t+cell/2)]
        target=ps.mean(axis=0);body=np.concatenate((ps,ps+[0,17.5,0]));sample=ps[np.linspace(0,len(ps)-1,min(25,len(ps))).astype(int)]+[0,17.5,0]
        if a.dense_rays:sample=np.concatenate((ps+[0,.5,0],ps+[0,8.,0],ps+[0,17.5,0]))
        candidates=[]
        for height in (24,30,38,46):
            for dist in (32,48,64,88,112,144):
                for angle in range(0,360,30):
                    rad=math.radians(angle);cx=target[0]+math.cos(rad)*dist;cz=target[2]+math.sin(rad)*dist
                    ci,cj=int(cx/cell),int(cz/cell)
                    if not(0<=ci<terrain.shape[0] and 0<=cj<terrain.shape[1] and allowed[ci,cj]):continue
                    cy=float(terrain[ci,cj])+height
                    for pers in (13000,15500):
                        cam=camera(cx,cy,cz,target[0],target[2]);cam['pers']=pers;cam['ax']=round(math.atan2(cy-target[1]-8,dist)*65536/math.tau)
                        if framed(cam,body):candidates.append((dist+height*2+(pers-13000)/200,cam))
        chosen=None
        for _,cam in sorted(candidates,key=lambda c:c[0]):
            origin=np.array([cam[k] for k in ('px','py','pz')]);hide=ray_nodes(tri,uvs,tids,nodes,textures,origin,sample)
            if hide&unhideable:continue
            chosen=cam;break
        if chosen is None:fail.append([int(tx),int(tz)]);continue
        masks=[0]*16
        for node in hide:masks[node//32]|=0x80000000>>(node%32)
        chosen.update(hidobj=masks,fog_nr=-1500.,fog_fr=-4000.)
        alpha_bound=limit_sorted_nodes(chosen,metadata)
        cuts.append(dict(b.zero(rdx.CUT,flg=1,cx=x,cz=z,cw=tile,cd=tile),cam=[chosen,copy.deepcopy(chosen)],areas=[b.zero(rdx.CTAB,minx=x-.01,minz=z-.01,maxx=r+.01,maxz=t+.01)],exd='00'*256))
        witnesses.append(dict(cut=len(cuts)-1,tile=[int(tx),int(tz)],survey_samples=len(ps),visibility_rays=len(sample),hidden_nodes=[n['index'] for n in metadata if chosen['hidobj'][n['index']//32] & (0x80000000>>(n['index']%32))],unhidden_alpha_bound=alpha_bound,camera_height_over_local_terrain=chosen['py']-float(terrain[int(chosen['px']/cell),int(chosen['pz']/cell)])))
        print('camera',len(cuts),'tile',int(tx),int(tz),'hides',len(hide),flush=True)
    uncovered=0;frustum=0
    for point in points:
        active=[c for c in cuts if c['areas'][0]['minx']<=point[0]<=c['areas'][0]['maxx'] and c['areas'][0]['minz']<=point[2]<=c['areas'][0]['maxz']]
        if not active:uncovered+=1
        elif any(not framed(c['cam'][0],np.array([point,point+[0,17.5,0]]),1.) for c in active):frustum+=1
    report=dict(input_sha256={str(p.relative_to(a.directory)):hashlib.sha256(p.read_bytes()).hexdigest() for p in (a.directory/'room.obj',a.directory/'nodes.json',a.directory/'terrain/grid.npz')},dense_rays=a.dense_rays,cameras=len(cuts),tile_size=tile,survey_samples=len(points),failed_tiles=fail,uncovered_samples=uncovered,frustum_failures=frustum,witnesses=witnesses,installable=False,
                limitations=['Covers candidate terrain flood, not a validated engine navigation mesh','Visibility tested at discrete survey positions; hide masks can remove complete props/building shells','Continuous cut boundaries and feet occlusion require final compiled checks'])
    (out/'cuts.json').write_text(json.dumps(cuts,indent=2)+'\n');(out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k!='witnesses'},indent=2))

if __name__=='__main__':main()
