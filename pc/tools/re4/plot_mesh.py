import os,sys,colorsys
sys.path.insert(0,'pc/tools')
import analyze_obj_surfaces as a
from PIL import Image,ImageDraw
faces=a.read_obj(os.environ['RE4_VILLAGE_OBJ'])
im=Image.new('RGB',(1600,1250),'#222222');d=ImageDraw.Draw(im)
def point(p):return (int((p[0]+1100)*.75),int((p[2]+750)*.75))
for g,m,p in sorted(faces,key=lambda f:sum(v[1] for v in f[2])):
 h=sum(v[1] for v in p)/3
 if max(v[1] for v in p)>160:continue
 c=tuple(int(x*255) for x in colorsys.hsv_to_rgb(.33-h/240,.5,.75))
 d.polygon([point(v) for v in p],fill=c)
for x in range(-1000,1000,100):
 for z in range(-700,701,100):
  u,v=point((x,0,z));d.line((u-3,v,u+3,v),fill='white');d.text((u,v),f'{x},{z}',fill='white')
im.save('scratch/re4/top.png')
