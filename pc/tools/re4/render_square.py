import sys,json,math,numpy as np
from pathlib import Path
from PIL import Image
sys.path.insert(0,'pc/tools')
import rdx,rdxbuild as b
from village_validate import axes
p=Path(sys.argv[1] if len(sys.argv)>1 else 'scratch/re4/square')
p.resolve().relative_to(Path('scratch').resolve())
room=rdx.Room((p/'room.rdx.raw').read_bytes());mesh=[f for g in b.read_obj(p/'room.obj') for f in g]
textures=[np.array(Image.frombytes('RGBA',(t['w'],t['h']),rdx.tim2_rgba(room.d,t))) for _,ts in rdx.texture_blocks(room) for t in ts]
def render(cam,width=800,height=600):
 forward,right,up=map(np.array,axes(cam));origin=np.array([cam['px'],cam['py'],cam['pz']]);t=math.tan(cam['pers']*math.pi/65536)
 depth=np.full((height,width),np.inf);rgb=np.zeros((height,width,3),dtype=np.uint8);rgb[:]=[116,132,145]
 for tid,face in mesh:
  ps,uv=zip(*face);rel=np.array(ps)-origin;z=rel@forward
  if min(z)<=.1:continue
  x=((rel@right)/z/t+1)*width/2;y=(1-(rel@up)/z/(t*.75))*height/2
  x0=max(0,int(min(x)));x1=min(width-1,math.ceil(max(x)));y0=max(0,int(min(y)));y1=min(height-1,math.ceil(max(y)))
  if x0>x1 or y0>y1:continue
  den=(y[1]-y[2])*(x[0]-x[2])+(x[2]-x[1])*(y[0]-y[2])
  if abs(den)<1e-9:continue
  xx,yy=np.meshgrid(np.arange(x0,x1+1)+.5,np.arange(y0,y1+1)+.5)
  a=((y[1]-y[2])*(xx-x[2])+(x[2]-x[1])*(yy-y[2]))/den;b0=((y[2]-y[0])*(xx-x[2])+(x[0]-x[2])*(yy-y[2]))/den;c=1-a-b0
  iz=np.maximum(a/z[0]+b0/z[1]+c/z[2],1e-9);zz=1/iz;uv=np.array(uv);u=(a*uv[0,0]/z[0]+b0*uv[1,0]/z[1]+c*uv[2,0]/z[2])/iz;v=(a*uv[0,1]/z[0]+b0*uv[1,1]/z[1]+c*uv[2,1]/z[2])/iz
  tex=textures[tid];col=tex[np.clip((v*(len(tex)-1)).astype(int),0,len(tex)-1),np.clip((u*(tex.shape[1]-1)).astype(int),0,tex.shape[1]-1)]
  old=depth[y0:y1+1,x0:x1+1];mask=(a>=0)&(b0>=0)&(c>=0)&(zz<old)&(col[:,:,3]>127)
  old[mask]=zz[mask];rgb[y0:y1+1,x0:x1+1][mask]=col[:,:,:3][mask]
 return Image.fromarray(rgb),lambda q:(((np.array(q)-origin)@right/((np.array(q)-origin)@forward)/t+1)*width/2,(1-(np.array(q)-origin)@up/((np.array(q)-origin)@forward)/(t*.75))*height/2)
cuts=room.cuts()
for ci,cut in enumerate(cuts):render(cut['cam'][0])[0].save(p/('camera%d.png'%ci))
if not (p/'build-report.json').exists():sys.exit(0)
# Top-down overview: straight down from above the room centre, north (-Z) up; walk bounds yellow,
# footprints red, cameras as numbered dots with their view direction.
from PIL import ImageDraw
j=json.loads((p/'room.json').read_text());rep=json.loads((p/'build-report.json').read_text())
over={'px':j['room']['w']/2,'py':420.,'pz':j['room']['d']/2,'ax':16384,'ay':0,'pers':11000}
im,proj=render(over,1200,900);d=ImageDraw.Draw(im)
x0,z0,x1,z1=rep['play_bounds'];d.polygon([proj((x,0,z)) for x,z in ((x0,z0),(x1,z0),(x1,z1),(x0,z1))],outline='yellow')
for x,y,z,w,h,dd in rep['footprints']:d.polygon([proj((a,h,c)) for a,c in ((x,z),(x+w,z),(x+w,z+dd),(x,z+dd))],outline='red')
for ci,cut in enumerate(cuts):
 c=cut['cam'][0];f=axes(c)[0];a=proj((c['px'],0,c['pz']));b=proj((c['px']+f[0]*25,0,c['pz']+f[2]*25))
 d.line((a,b),fill='cyan',width=3);d.ellipse((a[0]-6,a[1]-6,a[0]+6,a[1]+6),fill='cyan');d.text((a[0]+8,a[1]-8),str(ci),fill='white')
im.save(p/'overview.png')
