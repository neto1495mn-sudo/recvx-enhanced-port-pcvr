import os,sys,json,math,hashlib,numpy as np
from pathlib import Path
from types import SimpleNamespace
sys.path.insert(0,'pc/tools')
import rdx,rdxbuild as b,analyze_obj_surfaces as a
from village_validate import walkable
p=Path('scratch/re4/square');raw=(p/'room.rdx.raw').read_bytes();room=rdx.Room(raw);j=json.load(open(p/'room.json'));r=json.load(open(p/'validation.json'));build=json.load(open(p/'build-report.json'));rays=json.load(open(p/'ray-check.json'))
assert r['entry_clear'] and all(r[k]==0 for k in ('disconnected_samples','camera_frustum_failures','uncovered_samples','footprint_occlusion_samples'))
assert rays['head_occlusions']==0
assert build['triangles']<=13639 # measured retail maximum; runtime headroom is measured in game (RECVX_SPAWN_SURVEY)
b.validate_authored(room)
# Verify every traversable integer sample has source ground on the selected floor plane.
fs=a.read_obj(os.environ['RE4_VILLAGE_OBJ'])
f=np.array([ps for g,m,ps in fs if all(abs(v[1])<1e-5 for v in ps)])[:,:,::2]
A,B,C=f[:,0],f[:,1],f[:,2];den=(B[:,1]-C[:,1])*(A[:,0]-C[:,0])+(C[:,0]-B[:,0])*(A[:,1]-C[:,1]);good=abs(den)>1e-8
A,B,C,den=A[good],B[good],C[good],den[good];tr=build['transform'];missing=[]
for x,z in walkable(room.records('walls'),math.ceil(j['room']['w']),math.ceil(j['room']['d'])):
 sx=x/tr['scale']+tr['source_origin'][0];sz=z/tr['scale']+tr['source_origin'][2]
 u=((B[:,1]-C[:,1])*(sx-C[:,0])+(C[:,0]-B[:,0])*(sz-C[:,1]))/den;v=((C[:,1]-A[:,1])*(sx-C[:,0])+(A[:,0]-C[:,0])*(sz-C[:,1]))/den
 if not np.any((u>=-1e-5)&(v>=-1e-5)&(u+v<=1+1e-5)):missing.append((x,z))
assert not missing,('ground support missing',missing[:5])
args=SimpleNamespace(iso=os.environ['RECVX_ISO'])
for ref,t in zip(j['retail_textures'],rdx.texture_blocks(room)[0][1]):
 donor=rdx.load_room(args,ref['room']);d=rdx.texture_blocks(donor)[ref['list']][1][ref['index']];payload=bytes(room.d[t['ofs']:t['ofs']+t['size']]);assert payload==bytes(donor.d[d['ofs']:d['ofs']+d['size']]);assert payload[5]==1
for cut in room.cuts():
 for c in cut['cam']:
  assert 0<c['px']<j['room']['w'] and 0<c['pz']<j['room']['d']
# Verify actual compiled geometry counts, including tiling, against the chosen budgets.
mo=rdx.model_objects(room);nv=nt=0
for ob in mo['objects']:
 vl,pl,*_=rdx.cnk_model(room.d,mo['top'],ob['model'])
 nv+=len(rdx.parse_vlist(room.d,mo['top']+vl)[0])
 for _,ss in rdx.parse_plist(room.d,mo['top']+pl)[0]:nt+=sum(max(0,len(s)-2) for _,s in ss)
assert (nv,nt)==(build['vertices'],build['triangles']);assert len(mo['objects'])<=120
packed=rdx.compress(raw);assert rdx.expand(packed)==raw
assert len(raw)+len(packed)+3*1024*1024<12845056
name='RM_1260.RDX';assert name not in rdx.room_names()
mods=Path('mods');mods.mkdir(exist_ok=True);dest=mods/name
if dest.exists(): # keep the previous revision for comparison instead of refusing
 import time;arch=Path('scratch/re4/archive');arch.mkdir(parents=True,exist_ok=True);dest.rename(arch/('RM_1260.%d.RDX'%time.time()))
r.update(room_id=name,teleport='1:1:26:0',packed_bytes=len(packed),triangles=nt,vertices=nv,nodes=len(mo['objects']),source_ground_missing_samples=len(missing),native_TIM2_payloads_preserved=True,format_id_1=True,compression_roundtrip=True,mesh_ray_checks=rays,sha256=hashlib.sha256(packed).hexdigest(),runtime_tested=False)
# All checks above must pass before publication.
tmp=dest.with_suffix('.tmp');tmp.write_bytes(packed);tmp.replace(dest)
(mods/'RM_1260.SPQ.key').write_text('60\n');(mods/'re4-square-validation.json').write_text(json.dumps(r,indent=2)+'\n');(p/'final-validation.json').write_text(json.dumps(r,indent=2)+'\n');print(json.dumps(r,indent=2))
