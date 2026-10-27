"""Height-aware whole-map terrain, collision and slope-fit evidence.

No missing-support cells become invisible walls. Candidate records are explicitly
not installable until slope projection, seams and floor-level transitions pass.
"""
import argparse
import collections
import json
import math
from pathlib import Path
import numpy as np
from PIL import Image
from boundary_heights import height_field
from build_rooms import output_path
from navigation import read_obj, raster
from materials import resolve, category
from outline import rectangles
from fit_transitions import fit_axis_slope


def dilate(mask, radius):
    result=mask.copy();n=math.ceil(radius)
    for dx in range(-n,n+1):
        for dz in range(-n,n+1):
            # Square clearance matches the existing strict-box runtime audit.
            if max(abs(dx),abs(dz))>radius:continue
            a=(slice(max(0,dx),min(mask.shape[0],mask.shape[0]+dx)),slice(max(0,dz),min(mask.shape[1],mask.shape[1]+dz)))
            b=(slice(max(0,-dx),min(mask.shape[0],mask.shape[0]-dx)),slice(max(0,-dz),min(mask.shape[1],mask.shape[1]-dz)))
            result[a]|=mask[b]
    return result


def terrain_flood(allowed, height, start, cell, gradient):
    reached=np.zeros(allowed.shape,bool)
    if not allowed[start]:raise ValueError('entry has no clear reviewed terrain')
    reached[start]=True;queue=collections.deque([start]);edges=collections.Counter()
    while queue:
        x,z=queue.popleft()
        for q in ((x-1,z),(x+1,z),(x,z-1),(x,z+1)):
            if not(0<=q[0]<allowed.shape[0] and 0<=q[1]<allowed.shape[1]):edges['survey_edge']+=1
            elif not allowed[q]:edges['blocked_or_unsupported']+=1
            elif abs(height[q]-height[x,z])>cell*gradient:edges['steep_or_discontinuous']+=1
            elif not reached[q]:reached[q]=True;queue.append(q)
    return reached,dict(edges)


def fit_patches(height, mask, cell, tolerance, tile=32):
    """Subdivide until all center samples in a rectangle fit one axis incline."""
    rows=[];failed=[]
    def fit(x,z,r,t):
        active=mask[x:r,z:t]
        if not active.any():return
        h=height[x:r,z:t]
        complete=np.isfinite(h).all()
        if complete:
            xx,zz=np.meshgrid((np.arange(x,r)+.5)*cell,(np.arange(z,t)+.5)*cell,indexing='ij')
            points=np.column_stack((xx.ravel(),h.ravel(),zz.ravel()))
            result=fit_axis_slope(points) if len(points)>1 else None
            if result and result['maximum_error']<=tolerance:
                rows.append(dict(bounds=[x*cell,z*cell,r*cell,t*cell],fit=result,samples=len(points),height_range=[float(h.min()),float(h.max())]));return
        if r-x<=1 and t-z<=1:
            failed.append([x,z]);return
        if r-x>=t-z and r-x>1:
            mid=(x+r)//2;fit(x,z,mid,t);fit(mid,z,r,t)
        else:
            mid=(z+t)//2;fit(x,z,r,mid);fit(x,mid,r,t)
    for x in range(0,height.shape[0],tile):
        for z in range(0,height.shape[1],tile):fit(x,z,min(x+tile,height.shape[0]),min(z+tile,height.shape[1]))
    return rows,failed


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('recipe',type=Path);p.add_argument('--cell',type=float,default=2.);a=p.parse_args()
    a.recipe.resolve().relative_to(Path('scratch').resolve())
    spec=json.loads(a.recipe.read_text());out=output_path(Path(spec['output'])/'terrain')
    transform=json.loads((Path(spec['output'])/'transform.json').read_text());bounds=transform['bounds'];cell=a.cell
    if cell<=0:raise ValueError('positive cell required')
    mapping=json.loads(Path(spec['mapping']).read_text())
    if spec.get('overrides'):mapping['group_overrides']=json.loads(Path(spec['overrides']).read_text())
    mapping['navigation_excluded_materials']=spec['exclude_materials']
    faces=read_obj(spec['obj']);shape=tuple(math.ceil(bounds[k+2]/cell) for k in range(2))
    print('Surveying heights',shape,flush=True)
    opaque,terrain=height_field(faces,mapping,transform,bounds,cell,shape)
    solid=np.zeros(shape,bool);hits=collections.Counter()
    # Compare solids with the local terrain, not a global Y=0 actor band.
    for g,m,ps in faces:
        if m in spec['exclude_materials']:continue
        surface,info=resolve(mapping,g,m,'wood')
        if surface is None or (category(surface) in ('opacity','foliage') and not info.get('navigation_solid')):continue
        ps=(np.asarray(ps)-transform['source_origin'])*transform['scale']
        normal=np.cross(ps[1]-ps[0],ps[2]-ps[0]);length=np.linalg.norm(normal)
        if length<1e-9:continue
        if category(surface)=='ground' and abs(normal[1])/length>.2:continue
        ix,iz=raster(ps.tolist(),bounds,cell,shape)
        floor=terrain[ix,iz]
        # Conservative triangle vertical range; report overblocking explicitly.
        keep=np.isfinite(floor)&(ps[:,1].max()>floor+.5)&(ps[:,1].min()<floor+17.5)
        solid[ix[keep],iz[keep]]=True;hits[category(surface)]+=int(keep.sum())
    supported=np.isfinite(terrain)
    blocked=dilate(solid,math.ceil(5.5/cell))
    unsupported=dilate(~supported,math.ceil(5.5/cell))
    allowed=supported&~blocked&~unsupported
    entry=json.loads((Path(spec['output'])/'room.json').read_text())['positions'][0]
    start=(int(entry['px']/cell),int(entry['pz']/cell))
    gradient=spec.get('navigation_gradient',.65)
    reached,edges=terrain_flood(allowed,terrain,start,cell,gradient)
    boxes=rectangles(solid)
    patches,failed=fit_patches(terrain,reached,cell,.25)
    np.savez_compressed(out/'grid.npz',terrain=terrain,opaque=opaque,solid=solid,blocked=blocked,supported=supported,allowed=allowed,reachable=reached)
    # Keep explicit boundary evidence; no automatic wall authoring at these cells.
    openings=[]
    for x,z in zip(*np.where(reached)):
        for nx,nz in ((x-1,z),(x+1,z),(x,z-1),(x,z+1)):
            if 0<=nx<shape[0] and 0<=nz<shape[1] and not reached[nx,nz] and not blocked[nx,nz]:
                openings.append(dict(inside=[int(x),int(z)],outside=[int(nx),int(nz)],inside_y=float(terrain[x,z]),outside_y=float(terrain[nx,nz]) if supported[nx,nz] else None,reason='support_edge' if unsupported[nx,nz] else 'steep_or_discontinuous'))
    (out/'slope-candidates.json').write_text(json.dumps(dict(patches=patches,unfitted_cells=failed,installable=False),indent=2)+'\n')
    (out/'outline-candidates.json').write_text(json.dumps(dict(boxes=[[v*cell for v in box] for box in boxes],open_boundaries=openings,installable=False),indent=2)+'\n')
    rgb=np.full((*shape,3),(30,30,38),np.uint8);rgb[supported]=(95,90,65);rgb[allowed]=(70,100,65);rgb[reached]=(70,190,100);rgb[solid]=(180,145,110)
    Image.fromarray(rgb.transpose(1,0,2)).save(out/'overview.png')
    report=dict(maximum_navigation_gradient=gradient,cell_size=cell,actor_radius=5.5,conservative_grid_padding=math.ceil(5.5/cell)*cell,bounds=bounds,
                reviewed_terrain_samples=int(supported.sum()),reachable_samples=int(reached.sum()),reachable_area=float(reached.sum()*cell**2),
                terrain_height_range=[float(np.nanmin(terrain)),float(np.nanmax(terrain))],reachable_height_range=[float(terrain[reached].min()),float(terrain[reached].max())],
                solid_boxes=len(boxes),boundary_counts=edges,open_boundary_edges=len(openings),slope_candidates=len(patches),unfitted_cells=len(failed),
                fitted_maximum_center_error=max(p['fit']['maximum_error'] for p in patches),installable=False,
                limitations=['Nearest-to-zero reviewed terrain layer; bridges and overlapping floors need review',
                             'Conservative triangle height interval may overblock props',
                             'Center fits do not certify rectangle corners or adjacent seams',
                             'Slope projection, landing transitions and grand[] assignment not validated',
                             'No missing support converted into walls; open boundary evidence remains unresolved'])
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2),flush=True)

if __name__=='__main__':main()
