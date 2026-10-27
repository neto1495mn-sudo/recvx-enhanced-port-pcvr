import sys,json,numpy as np,math
from pathlib import Path
sys.path.insert(0,'pc/tools');import rdx,rdxbuild as b
from village_validate import walkable
p=Path(sys.argv[1] if len(sys.argv)>1 else 'scratch/re4/square');j=json.load(open(p/'room.json'));room=rdx.Room((p/'room.rdx.raw').read_bytes())
mesh=[f for g in b.read_obj(p/'room.obj') for f in g];tri=np.array([[v[0] for v in face] for tid,face in mesh]);uv=np.array([[v[1] for v in face] for tid,face in mesh]);tids=np.array([tid for tid,face in mesh]);A=tri[:,0];E=tri[:,1]-A;F=tri[:,2]-A
textures=[np.frombuffer(rdx.tim2_rgba(room.d,t),dtype=np.uint8).reshape(t['h'],t['w'],4) for _,ts in rdx.texture_blocks(room) for t in ts]
walk=walkable(room.records('walls'),math.ceil(j['room']['w']),math.ceil(j['room']['d']));fail=[];tested=0
for x,z in sorted(walk):
 if x%3 or z%3:continue
 for ci,cut in enumerate(room.cuts()):
  if not any(a['minx']<=x<=a['maxx'] and a['minz']<=z<=a['maxz'] for a in cut['areas']):continue
  cam=cut['cam'][0];O=np.array([cam['px'],cam['py'],cam['pz']]);D=np.array([x,17.5,z])-O;P=np.cross(D,F);det=np.sum(E*P,axis=1);valid=np.abs(det)>1e-8;inv=np.zeros(len(det));inv[valid]=1/det[valid];T=O-A;U=np.sum(T*P,axis=1)*inv;Q=np.cross(T,E);V=np.sum(D*Q,axis=1)*inv;dist=np.sum(F*Q,axis=1)*inv
  hits=np.where(valid&(U>=0)&(V>=0)&(U+V<=1)&(dist>.001)&(dist<.995))[0];blocked=False
  for ix in hits:
   t=textures[tids[ix]];st=(1-U[ix]-V[ix])*uv[ix,0]+U[ix]*uv[ix,1]+V[ix]*uv[ix,2];alpha=t[int(np.clip(st[1]*(len(t)-1),0,len(t)-1)),int(np.clip(st[0]*(t.shape[1]-1),0,t.shape[1]-1)),3]
   if alpha>127:blocked=True;break
  tested+=1
  if blocked:fail.append([ci,x,z])
r=dict(samples=tested,head_occlusions=len(fail),examples=fail[:20]);(p/'ray-check.json').write_text(json.dumps(r,indent=2)+'\n');print(r)
