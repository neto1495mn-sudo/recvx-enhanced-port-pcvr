import sys,json,math,collections
from pathlib import Path
sys.path.insert(0,'pc/tools')
import rdx,rdxbuild as b
from village_validate import walkable,in_view,axes
from area_validate import segment_hits_box
p=Path(sys.argv[1] if len(sys.argv)>1 else 'scratch/re4/square');j=json.loads((p/'room.json').read_text());report=json.loads((p/'build-report.json').read_text());room=rdx.Room((p/'room.rdx.raw').read_bytes());b.validate_authored(room)
walk=walkable(room.records('walls'),math.ceil(j['room']['w']),math.ceil(j['room']['d']))
entry=room.records('positions')[0];spawn=(round(entry['px']),round(entry['pz']));print('spawn',spawn,'clear',spawn in walk,'walkable',len(walk))
reached={spawn};q=collections.deque([spawn])
while q:
 x,z=q.popleft()
 for n in [(x-1,z),(x+1,z),(x,z-1),(x,z+1)]:
  if n in walk and n not in reached:reached.add(n);q.append(n)
misses=[];uncovered=[];occluded=[]
for x,z in walk:
 cuts=[c for c in room.cuts() if any(a['minx']<=x<=a['maxx'] and a['minz']<=z<=a['maxz'] for a in c['areas'])]
 if not cuts:uncovered.append((x,z))
 for c in cuts:
  cam=c['cam'][0]
  if not all(in_view(cam,(x,y,z),1.) for y in (0,17.5)):misses.append((x,z))
  if any(segment_hits_box((cam['px'],cam['py'],cam['pz']),(x,17.5,z),bx) for bx in report['footprints']):occluded.append((x,z))
result={'entry_clear':spawn in walk,'walkable_samples':len(walk),'disconnected_samples':len(walk-reached),'camera_frustum_failures':len(set(misses)),'uncovered_samples':len(uncovered),'footprint_occlusion_samples':len(set(occluded)),'expanded_bytes':len(room.d),'textures':len([t for _,ts in rdx.texture_blocks(room) for t in ts])}
print(json.dumps(result,indent=2));print('miss examples',misses[:5],'occluded',occluded[:5]);(p/'validation.json').write_text(json.dumps(result,indent=2)+'\n')
