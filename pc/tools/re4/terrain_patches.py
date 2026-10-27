"""Fit axis ramps to source-sampled shared corners with adaptive rectangles.

Unlike center-only fits, every accepted patch is checked at a common lattice of
source heights. This bounds edge disagreement by twice the source-fit tolerance
on continuous surfaces; discontinuities are recorded separately, never flattened.
"""
import argparse
import json
import math
from pathlib import Path
import numpy as np
from PIL import Image
from boundary_heights import height_field
from navigation import read_obj
from build_rooms import output_path
from fit_transitions import fit_axis_slope


def fit_rectangle(h, x, z, r, t, step):
    values=h[x:r+1,z:t+1]
    if not np.isfinite(values).all():return None
    xx,zz=np.meshgrid(np.arange(x,r+1)*step,np.arange(z,t+1)*step,indexing='ij')
    points=np.column_stack((xx.ravel(),values.ravel(),zz.ravel()))
    # Minimax vertical offset for each axis; average slope from LS is retained.
    fits=[]
    for axis in ('x','z'):
        coordinate=points[:,0 if axis=='x' else 2];origin=float(coordinate.mean())
        a,_=np.linalg.lstsq(np.column_stack((coordinate-origin,np.ones(len(points)))),points[:,1],rcond=None)[0]
        offsets=points[:,1]-a*(coordinate-origin);b=(offsets.max()+offsets.min())/2
        error=abs(a*(coordinate-origin)+b-points[:,1])
        fits.append(dict(axis=axis,coordinate_origin=origin,gradient=float(a),height_at_origin=float(b),maximum_error=float(error.max()),rms_error=float(np.sqrt(np.mean(error**2))),direction_id=(0 if a>=0 else 3) if axis=='x' else (2 if a>=0 else 1)))
    return min(fits,key=lambda f:f['maximum_error'])


def adaptive_patches(h, mask, step, tolerance=.1, tile=64):
    patches=[];failed=[]
    def fit(x,z,r,t):
        if not mask[x:r,z:t].any():return
        f=fit_rectangle(h,x,z,r,t,step)
        if f is not None and f['maximum_error']<=tolerance:
            patches.append(dict(bounds=[x*step,z*step,r*step,t*step],fit=f));return
        if r-x<=1 and t-z<=1:
            failed.append(dict(bounds=[x*step,z*step,r*step,t*step],fit=f));return
        # Prefer the split that decreases the worst child error most. This
        # creates thin strips across the secondary gradient on diagonal planes.
        candidates=[]
        if r-x>1:
            m=(x+r)//2;candidates.append([(x,z,m,t),(m,z,r,t)])
        if t-z>1:
            m=(z+t)//2;candidates.append([(x,z,r,m),(x,m,r,t)])
        scores=[]
        for children in candidates:
            fs=[fit_rectangle(h,*b,step) for b in children if mask[b[0]:b[2],b[1]:b[3]].any()]
            scores.append(max((q['maximum_error'] if q else float('inf') for q in fs),default=0))
        choice=min(range(len(candidates)),key=lambda i:(scores[i],-max(candidates[i][0][2]-x,candidates[i][0][3]-z)))
        for bounds in candidates[choice]:fit(*bounds)
    for x in range(0,mask.shape[0],tile):
        for z in range(0,mask.shape[1],tile):fit(x,z,min(x+tile,mask.shape[0]),min(z+tile,mask.shape[1]))
    return patches,failed


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('recipe',type=Path);p.add_argument('--step',type=float,default=.5);p.add_argument('--tolerance',type=float,default=.12);p.add_argument('--reuse-samples',action='store_true');p.add_argument('--refine',type=float);a=p.parse_args()
    spec=json.loads(a.recipe.read_text());out=output_path(Path(spec['output'])/'terrain');transform=json.loads((Path(spec['output'])/'transform.json').read_text());step=a.step
    w,d=transform['bounds'][2:];shape=(math.ceil(w/step)+1,math.ceil(d/step)+1)
    cache=out/f'source-corners-{step:g}.npz'
    if a.reuse_samples:
        with np.load(cache) as g:h=g['terrain']
    else:
        mapping=json.loads(Path(spec['mapping']).read_text());mapping['navigation_excluded_materials']=spec['exclude_materials']
        if spec.get('overrides'):mapping['group_overrides']=json.loads(Path(spec['overrides']).read_text())
        print('Sampling source corners',shape,flush=True)
        _,h=height_field(read_obj(spec['obj']),mapping,transform,[-step/2,-step/2,w+step/2,d+step/2],step,shape)
        np.savez_compressed(cache,terrain=h)
    with np.load(out/'grid.npz') as g:base=g['reachable']
    cell=json.loads((out/'report.json').read_text())['cell_size'];ratio=round(cell/step)
    if abs(ratio*step-cell)>1e-6:raise ValueError('step must divide survey cell')
    mask=np.repeat(np.repeat(base,ratio,axis=0),ratio,axis=1)[:shape[0]-1,:shape[1]-1]
    print('Fitting shared source lattice',int(mask.sum()),flush=True)
    patches,failed=adaptive_patches(h,mask,step,a.tolerance)
    if a.refine:
        from terrain_source import TerrainSource
        mapping=json.loads(Path(spec['mapping']).read_text());mapping['navigation_excluded_materials']=spec['exclude_materials']
        if spec.get('overrides'):mapping['group_overrides']=json.loads(Path(spec['overrides']).read_text())
        source=TerrainSource(read_obj(spec['obj']),mapping,transform)
        unresolved=[];initial_failures=len(failed)
        for index,patch in enumerate(failed):
            bounds=patch['bounds'];local=source.grid(bounds,a.refine)
            children,bad=adaptive_patches(local,np.ones((local.shape[0]-1,local.shape[1]-1),bool),a.refine,a.tolerance)
            for child in children+bad:
                x,z,r,t=child['bounds'];child['bounds']=[x+bounds[0],z+bounds[1],r+bounds[0],t+bounds[1]]
                if child['fit']:child['fit']['coordinate_origin']+=bounds[0 if child['fit']['axis']=='x' else 1]
            patches.extend(children);unresolved.extend(bad)
        failed=unresolved
        print('Refined',initial_failures,'source patches; unresolved',len(failed),flush=True)
    report=dict(patches=len(patches),failed_smallest_patches=len(failed),step=step,tolerance=a.tolerance,maximum_source_lattice_error=max((p['fit']['maximum_error'] for p in patches),default=0),installable=False)
    (out/'corner-patches.json').write_text(json.dumps(dict(patches=patches,failed=failed,report=report),indent=2)+'\n')
    rgb=np.full((*mask.shape,3),32,np.uint8)
    for ps,color in ((patches,(60,170,95)),(failed,(230,60,40))):
        for patch in ps:
            x,z,r,t=[round(v/step) for v in patch['bounds']];rgb[x:r,z:t]=color
    Image.fromarray(rgb.transpose(1,0,2)).save(out/'corner-fit-checks.png')
    print(json.dumps(report,indent=2))

if __name__=='__main__':main()
