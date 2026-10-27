import sys,json,math,statistics
from pathlib import Path
sys.path.insert(0,'pc/tools')
import rdx
cat=json.load(open('scratch/assets/catalogue.json'));rows=[]
for r in cat['rooms']:
 blob=Path('scratch/assets',r['room'][:7],'model0.bin').read_bytes()
 class Model:
  d=blob
  def model_table(self):return [0]
 mo=rdx.model_objects(Model());tri=verts=0
 for ob in mo['objects']:
  if ob['model']==-1:continue
  vl,pl,*_=rdx.cnk_model(blob,mo['top'],ob['model'])
  if vl!=-1:verts+=len(rdx.parse_vlist(blob,mo['top']+vl)[0])
  if pl!=-1:
   for _,ss in rdx.parse_plist(blob,mo['top']+pl)[0]:tri+=sum(max(0,len(s)-2) for _,s in ss)
 rows.append(dict(room=r['room'],triangles=tri,vertices=verts,nodes=len(mo['objects']),textures=len(r['textures'])))
summary={}
for k in ('triangles','vertices','nodes','textures'):
 vs=sorted(r[k] for r in rows);summary[k]=dict(median=statistics.median(vs),p90=vs[math.ceil(.9*len(vs))-1],maximum=vs[-1])
Path('scratch/re4/retail-budget.json').write_text(json.dumps(dict(rooms=len(rows),summary=summary,per_room=rows),indent=2)+'\n');print(summary)
