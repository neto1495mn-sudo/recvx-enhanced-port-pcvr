import numpy as np,math
from village_validate import axes
from materials import category

def visible(mesh,cameras,size=480):
 seen=set()
 for cam in cameras:
  forward,right,up=map(np.array,axes(cam));origin=np.array([cam['px'],cam['py'],cam['pz']]);t=math.tan(cam['pers']*math.pi/65536)
  width=size;height=size*3//4;depth=np.full((height,width),np.inf);ids=np.full((height,width),-1,dtype=int)
  for i,(cl,ps) in enumerate(mesh):
   if category(cl) in ('opacity','foliage'):continue # cutout cards cannot act as solid occluders
   rel=np.array(ps)-origin;z=rel@forward
   if min(z)<=.1:continue
   x=((rel@right)/z/t+1)*width/2;y=(1-(rel@up)/z/(t*.75))*height/2
   x0=max(0,int(min(x)));x1=min(width-1,math.ceil(max(x)));y0=max(0,int(min(y)));y1=min(height-1,math.ceil(max(y)))
   if x0>x1 or y0>y1:continue
   den=(y[1]-y[2])*(x[0]-x[2])+(x[2]-x[1])*(y[0]-y[2])
   if abs(den)<1e-9:continue
   xx,yy=np.meshgrid(np.arange(x0,x1+1)+.5,np.arange(y0,y1+1)+.5)
   a=((y[1]-y[2])*(xx-x[2])+(x[2]-x[1])*(yy-y[2]))/den;b=((y[2]-y[0])*(xx-x[2])+(x[0]-x[2])*(yy-y[2]))/den;c=1-a-b
   zz=1/np.maximum(a/z[0]+b/z[1]+c/z[2],1e-9);old=depth[y0:y1+1,x0:x1+1];mask=(a>=0)&(b>=0)&(c>=0)&(zz<old)
   old[mask]=zz[mask];ids[y0:y1+1,x0:x1+1][mask]=i
  seen.update(int(x) for x in np.unique(ids) if x>=0)
 return [(cl,p) for i,(cl,p) in enumerate(mesh) if i in seen or category(cl) in ('opacity','foliage','roof')]
