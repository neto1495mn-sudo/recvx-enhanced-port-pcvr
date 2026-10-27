"""Certify rectangle coverage and height error against source terrain polygons.

Unlike a lattice check, convex subtraction detects holes between probes. Height
error is bounded at every clipped triangle vertex (the difference of two planes
is affine). Overlapping layers are deliberately rejected unless every layer fits;
this conservative certificate never silently chooses a bridge or fills a void.
"""
import argparse
import hashlib
import json
from pathlib import Path
import numpy as np
from terrain_source import TerrainSource
from terrain_patches import fit_rectangle
from navigation import read_obj
from build_rooms import output_path
from refine_terrain import edge_audit


def clip_half(poly, axis, value, greater):
    if not len(poly): return np.empty((0, 3))
    result=[]
    for a,b in zip(poly,np.roll(poly,-1,axis=0)):
        da=(a[axis]-value)*(1 if greater else -1)
        db=(b[axis]-value)*(1 if greater else -1)
        if da>=0: result.append(a)
        if (da<0)!=(db<0): result.append(a+(b-a)*da/(da-db))
    return np.array(result).reshape((-1,3))


def clipped(triangle,bounds):
    x,z,r,t=bounds;poly=triangle
    for axis,value,greater in ((0,x,True),(0,r,False),(2,z,True),(2,t,False)):
        poly=clip_half(poly,axis,value,greater)
        if not len(poly):break
    return poly


def area(poly):
    if len(poly)<3:return 0.
    # Translate before the shoelace sum to avoid cancellation far from origin.
    p=poly-poly[0]
    return abs(float(np.sum(p[:,0]*np.roll(p[:,1],-1)-p[:,1]*np.roll(p[:,0],-1))))/2


def cut_line(poly,a,b,inside):
    result=[];v=b-a
    for p,q in zip(poly,np.roll(poly,-1,axis=0)):
        dp=(v[0]*(p[1]-a[1])-v[1]*(p[0]-a[0]))*(1 if inside else -1)
        dq=(v[0]*(q[1]-a[1])-v[1]*(q[0]-a[0]))*(1 if inside else -1)
        if dp>=0:result.append(p)
        if (dp<0)!=(dq<0):result.append(p+(q-p)*dp/(dp-dq))
    return np.array(result).reshape((-1,2))


def subtract(poly,cover):
    """Disjoint convex pieces of poly minus a convex cover polygon."""
    signed=np.sum(cover[:,0]*np.roll(cover[:,1],-1)-cover[:,1]*np.roll(cover[:,0],-1))
    if signed<0:cover=cover[::-1]
    result=[];remaining=poly
    for a,b in zip(cover,np.roll(cover,-1,axis=0)):
        outside=cut_line(remaining,a,b,False)
        if area(outside)>1e-12:result.append(outside)
        remaining=cut_line(remaining,a,b,True)
        if area(remaining)<=1e-12:break
    return result


def certify(bounds,fit,triangles,tolerance=.12):
    x,z,r,t=bounds;uncovered=[np.array([[x,z],[r,z],[r,t],[x,t]],float)]
    maximum=0.;polygons=0
    for triangle in triangles:
        poly=clipped(triangle,bounds)
        if area(poly[:,[0,2]])<=1e-12:continue
        polygons+=1;axis=0 if fit['axis']=='x' else 2
        predicted=fit['gradient']*(poly[:,axis]-fit['coordinate_origin'])+fit['height_at_origin']
        maximum=max(maximum,float(abs(predicted-poly[:,1]).max()))
        if uncovered:
            uncovered=[piece for old in uncovered for piece in subtract(old,poly[:,[0,2]])]
    missing=sum(area(p) for p in uncovered)
    return dict(intersected_polygons=polygons,uncovered_area=missing,maximum_error=maximum,
                passed=bool(polygons and missing<=1e-8 and maximum<=tolerance+1e-8))


def nearby(source,bounds):
    x,z,r,t=bounds;step=source.bin_size;ids=set()
    for i in range(int(np.floor(x/step)),int(np.floor(r/step))+1):
        for j in range(int(np.floor(z/step)),int(np.floor(t/step))+1):ids.update(source.bins.get((i,j),[]))
    return source.triangles[sorted(ids)]


def refine_patch(patch,source,tolerance,minimum):
    bounds=patch['bounds'];check=certify(bounds,patch['fit'],nearby(source,bounds),tolerance)
    if check['passed'] or min(bounds[2]-bounds[0],bounds[3]-bounds[1])<=minimum+1e-8:
        return [dict(patch,polygon_check=check,source_check='pass' if check['passed'] else 'polygon_failure')]
    x,z,r,t=bounds;mx=(x+r)/2;mz=(z+t)/2;result=[]
    for box in ((x,z,mx,mz),(mx,z,r,mz),(x,mz,mx,t),(mx,mz,r,t)):
        # Fit nine exact source queries. The certificate below remains the final test.
        xx,zz=np.meshgrid(np.linspace(box[0],box[2],3),np.linspace(box[1],box[3],3),indexing='ij')
        heights=source.heights(np.column_stack((xx.ravel(),zz.ravel()))).reshape((3,3))
        from fit_transitions import fit_axis_slope
        if np.isfinite(heights).all():
            fit=fit_axis_slope(np.column_stack((xx.ravel(),heights.ravel(),zz.ravel())))
            result.extend(refine_patch(dict(bounds=list(box),fit=fit),source,tolerance,minimum))
        else:result.append(dict(bounds=list(box),fit=None,source_check='missing_support',polygon_check=dict(passed=False)))
    return result


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('recipe',type=Path)
    p.add_argument('--minimum',type=float,default=.0625);p.add_argument('--tolerance',type=float,default=.12);a=p.parse_args()
    if not np.isfinite(a.minimum) or a.minimum<=0 or not np.isfinite(a.tolerance) or a.tolerance<=0:p.error('positive finite limits required')
    a.recipe.resolve().relative_to(Path('scratch').resolve())
    spec=json.loads(a.recipe.read_text());root=output_path(spec['output']);out=output_path(root/'terrain')
    mapping=json.loads(Path(spec['mapping']).read_text());mapping['navigation_excluded_materials']=spec['exclude_materials']
    if spec.get('overrides'):mapping['group_overrides']=json.loads(Path(spec['overrides']).read_text())
    source=TerrainSource(read_obj(spec['obj']),mapping,json.loads((root/'transform.json').read_text()))
    candidate=out/'corner-patches.json';old=json.loads(candidate.read_text());patches=[]
    for i,patch in enumerate(old['patches']):
        patches.extend(refine_patch(patch,source,a.tolerance,a.minimum))
        if (i+1)%1000==0:print('Certified',i+1,'input patches;',len(patches),'leaves',flush=True)
    failures=[i for i,p in enumerate(patches) if p['source_check']!='pass']
    seams=edge_audit(patches,.25)
    files=[a.recipe,Path(spec['obj']),Path(spec['mapping']),root/'transform.json',candidate]
    if spec.get('overrides'):files.append(Path(spec['overrides']))
    report=dict(input_patches=len(old['patches']),input_unresolved=len(old['failed']),patches=len(patches),failed_polygons=len(failures),
                maximum_error=max((p['polygon_check'].get('maximum_error',0) for p in patches),default=0),
                maximum_uncovered_area=max((p['polygon_check'].get('uncovered_area',0) for p in patches),default=0),
                seams={k:v for k,v in seams.items() if k!='failures'},tolerance=a.tolerance,minimum=a.minimum,
                installable=False,player_type6_supported=True,
                limitations=['PC_ROOM_SLOPES supports type 6; source certificates alone do not test the eligibility band during movement',
                             'Continuous terrain needs full slope coverage on one grand[] floor and separate collision',
                             'All overlapping reviewed terrain layers must pass the conservative certificate'],
                input_sha256={str(f):hashlib.sha256(f.read_bytes()).hexdigest() for f in files})
    (out/'continuous-patches.json').write_text(json.dumps(dict(report=report,patches=patches,failed=failures,seams=seams),indent=2)+'\n')
    print(json.dumps(report,indent=2),flush=True)

if __name__=='__main__':main()
