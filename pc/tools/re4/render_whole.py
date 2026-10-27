"""Render an overview and selected compiled/candidate cameras without the game."""
import argparse,json,math,numpy as np
from pathlib import Path
from PIL import Image,ImageDraw
from build_rooms import output_path
import rdx,rdxbuild as b
from village_validate import axes
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('directory',type=Path);p.add_argument('--candidate-cameras',action='store_true');p.add_argument('--cuts',type=int,nargs='+',default=[0,2,4]);a=p.parse_args()
out=output_path(a.directory/'previews')
room=rdx.Room((a.directory/'room.rdx.raw').read_bytes())
j=json.loads((a.directory/'room.json').read_text())
mesh=[(ni,tid,face) for ni,g in enumerate(b.read_obj(a.directory/j.get('mesh','room.obj'))) for tid,face in g]
textures=[np.array(Image.frombytes('RGBA',(t['w'],t['h']),rdx.tim2_rgba(room.d,t))) for _,ts in rdx.texture_blocks(room) for t in ts]
def render(cam,width=800,height=600):
 forward,right,up=map(np.array,axes(cam));origin=np.array([cam['px'],cam['py'],cam['pz']]);t=math.tan(cam['pers']*math.pi/65536)
 depth=np.full((height,width),np.inf);rgb=np.zeros((height,width,3),dtype=np.uint8);rgb[:]=[116,132,145]
 for node,tid,face in mesh:
  if node<512 and cam.get('hidobj',[0]*16)[node//32] & (0x80000000>>(node%32)):continue
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

def annotated(cam,width=800,height=600):
 im,project=render(cam,width,height);draw=ImageDraw.Draw(im)
 # Placement markers are intentionally schematic, not retail actor renders.
 for kind,color in (('positions','cyan'),('items','yellow'),('enemies','red')):
  for i,rec in enumerate(room.records(kind)):
   p=np.array([rec['px'],rec['py']+1,rec['pz']]);origin=np.array([cam[k] for k in ('px','py','pz')])
   if (p-origin)@np.array(axes(cam)[0])<=0:continue
   x,y=project(p)
   if 0<=x<width and 0<=y<height:
    draw.ellipse((x-3,y-3,x+3,y+3),fill=color);draw.text((x+4,y),f'{kind[0]}{i}',fill=color)
 return im

cuts=json.loads((a.directory/'cameras/cuts.json').read_text()) if a.candidate_cameras else room.cuts()
label='terrain' if any(w['type']==6 for w in room.records('walls')) else 'inspection'
for ci in a.cuts:
 if ci>=len(cuts):continue
 annotated(cuts[ci]['cam'][0]).save(out/('candidate' if a.candidate_cameras else label).__add__(f'-camera{ci}.png'))
 print('Rendered camera',ci,flush=True)
over={'px':j['room']['w']/2,'py':max(j['room']['w'],j['room']['d'])*1.1,'pz':j['room']['d']/2,'ax':16384,'ay':0,'pers':12000}
annotated(over,1200,900).save(out/'overview.png')
print('Rendered overview',flush=True)
