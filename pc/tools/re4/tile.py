import math

def tiled(ps,axes,size):
 def clip(poly,axis,bound,lower):
  out=[]
  for p,q in zip(poly,poly[1:]+poly[:1]):
   ip=p[axis]>=bound-1e-9 if lower else p[axis]<=bound+1e-9
   iq=q[axis]>=bound-1e-9 if lower else q[axis]<=bound+1e-9
   if ip:out.append(p)
   if ip!=iq:
    t=(bound-p[axis])/(q[axis]-p[axis]);out.append(tuple(p[k]+t*(q[k]-p[k]) for k in range(3)))
  return out
 aa,bb=axes
 for a in range(math.floor(min(p[aa] for p in ps)/size),math.floor(max(p[aa] for p in ps)/size)+1):
  for b in range(math.floor(min(p[bb] for p in ps)/size),math.floor(max(p[bb] for p in ps)/size)+1):
   poly=list(ps)
   for ax,lo in [(aa,a*size),(bb,b*size)]:
    poly=clip(poly,ax,lo,True);poly=clip(poly,ax,lo+size,False)
   for i in range(1,len(poly)-1):
    pp=[poly[0],poly[i],poly[i+1]]
    cross=(pp[1][aa]-pp[0][aa])*(pp[2][bb]-pp[0][bb])-(pp[1][bb]-pp[0][bb])*(pp[2][aa]-pp[0][aa])
    if abs(cross)>1e-8:yield [(p,(max(0,min(1,p[aa]/size-a)),max(0,min(1,p[bb]/size-b)))) for p in pp]
