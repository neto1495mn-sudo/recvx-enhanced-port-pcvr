"""Build the square prototype from external assets; generated files stay in scratch."""
import os,sys,json,math,copy,collections
from pathlib import Path
from types import SimpleNamespace
sys.path.insert(0,'pc/tools')
import analyze_obj_surfaces as a,rdx,rdxbuild as b,kitbash
from build_area import atr
from build_village import camera
from village_validate import in_view
from materials import resolve, category
ROOT=Path.cwd();out=ROOT/'scratch/re4/square';out.mkdir(exist_ok=True)
args=SimpleNamespace(iso=os.environ['RECVX_ISO'])
transform=json.load(open('scratch/re4/transform.json'));scale=transform['scale'];origin=transform['source_origin']
def shot(x,z,tx,tz):
 c=camera(x,120,z,tx,tz);c['pers']=16000;return c
def convert(p):return tuple((p[k]-origin[k])*scale for k in range(3))
mp=json.load(open('pc/tools/area/re4_materials.json'))
# Exact source group names belong in an ignored, user-editable local override file.
import argparse
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--group-overrides',type=Path)
options=parser.parse_args()
override_path=options.group_overrides or Path('scratch/re4/group-overrides.json')
if options.group_overrides or override_path.exists():
 local=json.loads(override_path.read_text())
 for group,overrides in local.items():mp.setdefault('group_overrides',{}).setdefault(group,{}).update(overrides)
faces=a.read_obj(os.environ['RE4_VILLAGE_OBJ'])
selected=[];excluded=collections.Counter();uses=collections.Counter()
for g,m,ps in faces:
 c=[sum(p[k] for p in ps)/3 for k in range(3)]
 # 000 sky shell, 076 distant detail; 001/002/006 are single map-wide alpha overlay layers (terrain
 # decals): their sloped parts rendered as fence cutouts on the ground in game.
 if not (-190<c[0]<250 and -160<c[2]<190) or m in ('ROOM_MATERIAL_000','ROOM_MATERIAL_076','ROOM_MATERIAL_001','ROOM_MATERIAL_002','ROOM_MATERIAL_006'):continue
 p=tuple(convert(v) for v in ps)
 u=[p[1][k]-p[0][k] for k in range(3)];v=[p[2][k]-p[0][k] for k in range(3)]
 n=(u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]);length=math.sqrt(sum(x*x for x in n))
 if length<1e-8:continue
 ny=abs(n[1])/length;cl=mp['materials'][m]['class']
 if cl=='unknown':cl='roof' if ny<.9 and ny>.2 and c[1]>30 else 'wood'
 surface,decision=resolve(mp,g,m,cl)
 if surface is None:excluded['explicit_group_exclusion']+=1;continue
 if surface.category in ('opacity','foliage') and ny>.75:excluded['horizontal_opacity_decals']+=1;continue
 uses[(g,m,surface,decision.get('provisional_fallback'))]+=1;selected.append((surface,p))
# Global vertex grid shares positions across material seams. Keep original Y=0 plane.
def simplify(cell):
 result=[];seen=set()
 for cl,ps in selected:
  qs=tuple(tuple(round(v/cell)*cell for v in p) for p in ps)
  if len(set(qs))<3:continue
  u=[qs[1][k]-qs[0][k] for k in range(3)];v=[qs[2][k]-qs[0][k] for k in range(3)]
  n=(u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0])
  if sum(x*x for x in n)<1e-8:continue
  key=(cl,tuple(sorted(qs)))
  if key in seen:continue
  seen.add(key);result.append((cl,qs))
 return result
w,d=640*scale,490*scale
# Traversal bounded to surveyed flat centre; surrounding buildings remain visible.
x0,_,z0=convert((-60,0,-50));x1,_,z1=convert((130,0,50))
walls=[atr(px=0,pz=0,w=x0,d=d,h=200),atr(px=x1,pz=0,w=w-x1,d=d,h=200),atr(px=x0,pz=0,w=x1-x0,d=z0,h=200),atr(px=x0,pz=z1,w=x1-x0,d=d-z1,h=200)]
# Conservative footprints from connected solid components intersecting traversal (raised terrain
# such as the 9-unit bonfire mound included; flat ground is skipped by its height).
components=json.load(open('scratch/re4/components.json'));obstacles=[]
for c in components:
 lo,hi=[convert(p) for p in c['bounds']];m=c['material'];cl=mp['materials'][m]['class']
 if cl in ('opacity','foliage') or hi[1]<2 or lo[1]>15 or len(c['triangles'])<4:continue
 if hi[0]<=x0 or lo[0]>=x1 or hi[2]<=z0 or lo[2]>=z1:continue
 if hi[0]-lo[0]>45 or hi[2]-lo[2]>45:continue
 lo=[max(lo[0],x0),0,max(lo[2],z0)];hi=[min(hi[0],x1),max(hi[1],18),min(hi[2],z1)]
 if min(hi[0]-lo[0],hi[2]-lo[2])<.3:continue
 box=(lo[0],lo[1],lo[2],hi[0]-lo[0],hi[1]-lo[1],hi[2]-lo[2]);obstacles.append(box)
# Union nearby overlapping footprint boxes to avoid excessive collider records.
changed=True
while changed:
 changed=False
 for i,a0 in enumerate(obstacles):
  for k in range(i+1,len(obstacles)):
   a1=obstacles[k]
   if a0[0]<=a1[0]+a1[3]+1 and a1[0]<=a0[0]+a0[3]+1 and a0[2]<=a1[2]+a1[5]+1 and a1[2]<=a0[2]+a0[5]+1:
    lo=[min(a0[t],a1[t]) for t in range(3)];hi=[max(a0[t]+a0[t+3],a1[t]+a1[t+3]) for t in range(3)]
    obstacles[i]=tuple(lo+[hi[t]-lo[t] for t in range(3)]);obstacles.pop(k);changed=True;break
  if changed:break
for x,y,z,ww,h,dd in obstacles:walls.append(atr(px=x,py=y,pz=z,w=ww,h=h,d=dd))
# Six fixed cuts (3 columns x 2 rows). Each row is shot from the opposite side of the square,
# looking across it, from the lowest/closest camera that frames every walkable sample of its tile
# (feet and head), is not under any geometry and sees each head past the footprints and the
# opaque mesh. Cameras 120 high (first prototype) left Claire ~10 px tall in a 480-line frame.
import numpy as np
from village_validate import walkable
from area_validate import segment_hits_box
walk=walkable(walls,math.ceil(w),math.ceil(d))
opaque=np.array([ps for cl,ps in selected if category(cl) not in ('opacity','foliage')],dtype=float)
def covered(x,z):
 """Highest opaque triangle point above (x,z), or -inf."""
 A,B,C=opaque[:,0],opaque[:,1],opaque[:,2]
 den=(B[:,2]-C[:,2])*(A[:,0]-C[:,0])+(C[:,0]-B[:,0])*(A[:,2]-C[:,2]);ok=np.abs(den)>1e-9;den=np.where(ok,den,1)
 u=((B[:,2]-C[:,2])*(x-C[:,0])+(C[:,0]-B[:,0])*(z-C[:,2]))/den;v=((C[:,2]-A[:,2])*(x-C[:,0])+(A[:,0]-C[:,0])*(z-C[:,2]))/den
 inside=ok&(u>=0)&(v>=0)&(u+v<=1)
 return float(np.max((u*A[:,1]+v*B[:,1]+(1-u-v)*C[:,1])[inside])) if inside.any() else -math.inf
def blocked(o,targets):
 """Any segment o->target crosses an opaque triangle (Moller-Trumbore)."""
 A=opaque[:,0];E=opaque[:,1]-A;F=opaque[:,2]-A;O=np.array(o)
 for t in targets:
  D=np.array(t)-O;P=np.cross(D,F);det=np.sum(E*P,axis=1);ok=np.abs(det)>1e-9;inv=np.where(ok,1/np.where(ok,det,1),0)
  T=O-A;U=np.sum(T*P,axis=1)*inv;Q=np.cross(T,E);V=np.sum(D*Q,axis=1)*inv;dist=np.sum(F*Q,axis=1)*inv
  if np.any(ok&(U>=0)&(V>=0)&(U+V<=1)&(dist>.001)&(dist<.99)):return True
 return False
def choose(tile,south):
 # Aim past the tile at the facades beyond it when a camera can still frame the tile from there.
 for beyond in (40,20,0):
  try:return choose_target(tile,south,beyond)
  except ValueError:pass
 raise ValueError('no camera frames tile %r'%(tile,))
def choose_target(tile,south,beyond):
 ax0,az0,ax1,az1=tile;tx,tz=(ax0+ax1)/2,(az0-beyond if south else az1+beyond)
 pts=[(x,z) for x,z in walk if ax0<=x<=ax1 and az0<=z<=az1]
 cands=[]
 for h in range(30,65,5):
  for dx in (0,-15,15,-30,30):
   for dist in range(10,95,5):
    for pers in (13000,15000):
     cx=tx+dx;cz=az1+dist if south else az0-dist
     if not (2<cx<w-2 and 2<cz<d-2):continue
     cam=camera(cx,h,cz,tx,tz);cam['pers']=pers
     cands.append((math.dist((cx,h,cz),((ax0+ax1)/2,0,(az0+az1)/2))*(1+(pers-13000)/20000),cam))
 cands.sort(key=lambda c:c[0])
 for score,cam in cands:
  o=(cam['px'],cam['py'],cam['pz'])
  if not all(in_view(cam,(x,y,z),.92) for x,z in pts for y in (0,17.5)):continue
  if covered(o[0],o[2])>o[1]-2:continue
  if any(segment_hits_box(o,(x,17.5,z),bx) for x,z in pts for bx in obstacles):continue
  if blocked(o,[(x,17.5,z) for x,z in pts if x%4==0 and z%4==0]):continue
  if score>75:break
  return cam
 raise ValueError('no camera within 75 units frames tile %r'%(tile,))
zm=(z0+z1)/2;cuts=[]
for row,(az0,az1) in enumerate(((z0,zm),(zm,z1))):
 for i in range(3):
  tile=(x0+i*(x1-x0)/3,az0,x0+(i+1)*(x1-x0)/3,az1);cam=choose(tile,row==0)
  cam.update(fog_col=0xff918474,fog_nr=-200.,fog_fr=-700.);cuts.append((tile,cam))
  print('cut',len(cuts)-1,'tile',[round(v) for v in tile],'camera',[round(cam[k]) for k in ('px','py','pz')],'pers',cam['pers'],flush=True)
# Cull opaque faces hidden in all fixed cuts before clustering; regenerated with the cameras.
from visibility import visible
selected=visible(simplify(.5),[c for _,c in cuts],800)
print('visible input',len(selected),flush=True)
def build_mesh(mesh):
 kit=kitbash.Kit(args);group=[]
# UV projection normalized per triangle for this first structure prototype. No UV outside 0..1.
 for cl,ps in mesh:
  tid=kit.texture(cl.room,cl.texture,cl.texture_list)
  u=[ps[1][k]-ps[0][k] for k in range(3)];v=[ps[2][k]-ps[0][k] for k in range(3)]
  n=(u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]);drop=max(range(3),key=lambda k:abs(n[k]));axes=[k for k in range(3) if k!=drop]
  lo=[min(p[k] for p in ps) for k in axes];hi=[max(p[k] for p in ps) for k in axes]
  uv=[tuple((p[k]-lo[i])/max(hi[i]-lo[i],1e-6) for i,k in enumerate(axes)) for p in ps]
  if category(cl) in ('opacity','foliage'):
   group.append((tid,[(p,cl.map_uv(st)) for p,st in zip(ps,uv)]))
  else:
   from tile import tiled
   group.extend((tid,[(p,cl.map_uv(st)) for p,st in face]) for face in tiled(ps,axes,128 if cl.category=='roof' else 64))
 return kit,group
# Finest clustering whose tiled triangle count stays below the measured retail maximum (13639
# triangles; scratch/re4/retail-budget.json). The 6-unit clustering of the first prototype tore
# roofs and trees into spikes in game.
for cell in (.5,1,1.5,2,3,4,6):
 mesh=simplify(cell);kit,group=build_mesh(mesh)
 if len(group)<=12000:break
print('cluster cell',cell,'triangles',len(mesh),'tiled',len(group),collections.Counter(c for c,p in mesh),flush=True)
if len(group)>13639:raise ValueError('tiled geometry exceeds the retail maximum triangle count')
kit.groups=[group];kit.write(out/'room.obj')
j={'format':'recvx-room-1','mesh':'room.obj','retail_textures':kit.refs,'room':{'w':w,'d':d,'h':200,'amb_r':[.75]*4,'amb_g':[.7]*4,'amb_b':[.65]*4,'bak_col':0xff918474,'fog_col':0xff918474,'fog_nr':-200.,'fog_fr':-700.},'walls':walls,'floors':[atr(type=1,px=x0,pz=z0,w=x1-x0,d=z1-z0),atr(type=2,px=x0,pz=z0,w=x1-x0,d=z1-z0,prm0=1)],'routes':[atr(px=x0,pz=z0,w=x1-x0,d=z1-z0)],'route_next':[[0]],'objects':[b.zero(rdx.ETTY) for _ in range(4)],'cuts':[],'positions':[],'scripts':['3700b6000000','0000']}
for tile,cam in cuts:
 left,az0,right,az1=tile
 area=b.zero(rdx.CTAB,minx=left-1,minz=az0-1,maxx=right+1,maxz=az1+1)
 j['cuts'].append(dict(b.zero(rdx.CUT,flg=1,cx=left,cz=az0,cw=right-left,cd=az1-az0),cam=[cam,copy.deepcopy(cam)],areas=[area],exd='00'*256))
entry=convert((-35,0,0));j['positions']=[b.zero(rdx.POS,px=entry[0],py=0,pz=entry[2],ay=0)]
(out/'room.json').write_text(json.dumps(j,indent=2)+'\n')
report={'source_selected_triangles':len(selected),'cluster_size_game_units':cell,'triangles':len(group),'vertices':3*len(group),'excluded':dict(excluded),'provisional_material_uses':[{'group':g,'material':m,'class':c.category,'retail_texture':{'room':c.room,'list':c.texture_list,'texture':c.texture,'uv_rect':c.uv_rect},'fallback':fallback,'triangles':n} for (g,m,c,fallback),n in uses.items()],'play_bounds':[x0,z0,x1,z1],'footprints':obstacles,'transform':transform,'limitations':['provisional materials','cutout UVs need review','traversal restricted to flat central rectangle','sky uses room clear colour','silhouette grid error requires visual acceptance']}
(out/'build-report.json').write_text(json.dumps(report,indent=2)+'\n')
print('colliders',len(walls),'footprints',len(obstacles),'textures',len(kit.refs),flush=True)
raw=b.compile_source(out/'room.json',args);(out/'room.rdx.raw').write_bytes(raw)
print('compiled',len(raw),flush=True)
# Opaque classes must not get a cutout donor: 0050:0:73 (a 56 % transparent grate) chosen as
# roof tiles showed the sky through the roofs in game.
compiled=rdx.Room(raw);decoded=[t for _,ts in rdx.texture_blocks(compiled) for t in ts]
for tid in {kit.texture(c.room,c.texture,c.texture_list) for c,_ in mesh if category(c) not in ('opacity','foliage')}:
 if any(px<=127 for px in rdx.tim2_rgba(compiled.d,decoded[tid])[3::4]):
  raise ValueError(f'opaque surfaces use donor {kit.refs[tid]} with transparent texels')
