import os,sys,json,collections
sys.path.insert(0,'pc/tools')
import analyze_obj_surfaces as a
faces=a.read_obj(os.environ['RE4_VILLAGE_OBJ'])
by=collections.defaultdict(list)
for g,m,p in faces:by[g,m].append(p)
out=[]
for (g,m),fs in by.items():
 parent=list(range(len(fs)));seen={}
 def root(x):
  while parent[x]!=x:parent[x]=parent[parent[x]];x=parent[x]
  return x
 for i,f in enumerate(fs):
  for p in f:
   p=tuple(round(v,3) for v in p)
   if p in seen:parent[root(i)]=root(seen[p])
   seen[p]=i
 groups=collections.defaultdict(list)
 for i,f in enumerate(fs):groups[root(i)].append(f)
 for ff in groups.values():
  pts=[p for f in ff for p in f];lo=[min(p[k] for p in pts) for k in range(3)];hi=[max(p[k] for p in pts) for k in range(3)]
  out.append(dict(group=g,material=m,bounds=[lo,hi],triangles=ff))
json.dump(out,open('scratch/re4/components.json','w'))
for i,q in enumerate(out):
 lo,hi=q['bounds'];sx,sy,sz=[hi[k]-lo[k] for k in range(3)]
 if 15<sy<32 and min(sx,sz)<1 and 7<max(sx,sz)<18:print(i,q['material'],len(q['triangles']),q['bounds'])
