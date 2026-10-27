import os,sys,json
sys.path.insert(0,'pc/tools')
import rdx,assets,kitbash
from PIL import Image,ImageDraw
iso=rdx.Iso(os.environ['RECVX_ISO'])
_,parts=assets.door_pack(iso,22)
try:
 print('retail door 22 bounds',kitbash.bounds(kitbash.mesh_nodes(parts[0])))
except Exception as e:print(type(e).__name__,str(e))
cs=json.load(open('scratch/re4/components.json'));sel=cs[6794]
im=Image.new('RGB',(1000,800),'#202020');d=ImageDraw.Draw(im)
for q in cs:
 for f in q['triangles']:
  if all(198<p[0]<208 and 105<p[2]<155 and 5<p[1]<60 for p in f):
   d.polygon([(100+(p[2]-105)*16,760-(p[1]-5)*13) for p in f],outline=('#ffb030' if q is sel else '#999999'))
for y in range(10,61,10):d.text((2,760-(y-5)*13),str(y),fill='white')
im.save('scratch/re4/door-reference.png')
