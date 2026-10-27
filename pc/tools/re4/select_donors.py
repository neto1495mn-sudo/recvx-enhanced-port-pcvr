import json
from pathlib import Path
p=Path('pc/tools/area/re4_materials.json');m=json.loads(p.read_text())
refs={'plaster':('0030',72),'roof':('0030',18),'foliage':('1000',29),'water':('1040',11),'metal':('5810',1),'opacity':('1000',22)}
for k,(r,t) in refs.items():m['classes'][k]={'room':r,'list':0,'texture':t}
m['class_notes']={
 'plaster':'Previewed weathered grey render; provisional wall finish.',
 'roof':'Previewed dark roof surface; verify UV density and seams in game.',
 'foliage':'Previewed palm-frond cutout, binary alpha confirmed. Botanical mismatch with village trees remains for review.',
 'water':'Previewed blue mottled surface, opaque static surrogate only; native use and water rendering unverified. Not approved for transparent water.',
 'metal':'Previewed dark metal panel. Opaque donor; not a fence cutout.',
 'opacity':'Previewed decorative iron cutout with binary alpha. Provisional fence donor, not suitable for every opacity material. Use foliage after group review.'}
r=json.load(open('scratch/re4/surfaces.json'))['materials'];r2=json.load(open('scratch/re4/surfaces-grid200.json'))['materials']
changes={}
for n,v in m['materials'].items():
 reasons=v['review_reasons'];kind=v['class'];new=kind;why=None
 if any('opacity' in s for s in reasons):new='opacity';why='cutout_semantics_unresolved_fence_foliage_or_decal'
 elif kind=='unknown':
  q=r[n];q2=r2[n]
  if all(x['orientation_area_fraction']['sloped']>.6 and x['group_relative_height_mean']>.65 and x['local_height_quantiles'][1]>20 for x in (q,q2)):
   new='roof';why='upper_sloped_surface_context_provisional'
  elif all(x['orientation_area_fraction']['horizontal']>.9 and x['group_relative_height_mean']<.25 and x['local_height_quantiles'][1]<3 for x in (q,q2)):
   new='ground';why='lower_horizontal_surface_context_provisional'
 if new!=kind:
  v['class']=new;changes[n]={'from':kind,'to':new};reasons.append(why)
 if m['classes'].get(v['class']) is not None:
  v['retail_texture']=m['classes'][v['class']].copy()
  v['review_reasons']=[s for s in reasons if s!='retail_donor_unresolved']
 v['review_required']=True
m['description']='Previewed retail donor candidates with provisional geometry-based material classes. All source uses require review. Opacity is unresolved cutout semantics, not approval to replace foliage with ironwork. Null donors block conversion.'
p.write_text(json.dumps(m,indent=2)+'\n')
Path('scratch/re4/donor-classification-changes.json').write_text(json.dumps(changes,indent=2)+'\n')
from collections import Counter
print(Counter(v['class'] for v in m['materials'].values()));print(changes)
