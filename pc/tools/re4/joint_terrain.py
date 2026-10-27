"""Joint axis-slope fits with shared-edge constraints and explicit error gates.

Uses matrix-free conjugate gradients (NumPy only). Source-derived coefficients
and reports stay in scratch. Passing seam checks alone never authorizes install.
"""
import argparse
import copy
import json
import hashlib
from pathlib import Path
import numpy as np
from build_rooms import output_path


def shared_edges(patches, mask, cell, terrain=None):
    owner=np.full(mask.shape,-1,int)
    for i,p in enumerate(patches):
        x,z,r,t=[round(v/cell) for v in p['bounds']]
        if np.any(owner[x:r,z:t]>=0):raise ValueError('overlapping patches')
        owner[x:r,z:t]=i
    edges=[]
    for x,z in zip(*np.where(mask)):
        for nx,nz in ((x+1,z),(x,z+1)):
            if nx>=mask.shape[0] or nz>=mask.shape[1] or not mask[nx,nz]:continue
            i,j=int(owner[x,z]),int(owner[nx,nz])
            if min(i,j)<0:raise ValueError('reachable sample without patch')
            if i==j:continue
            # A flood can reach both sides of a cliff via a different path.
            # Such adjacency is a drop/transition review, never a smooth seam.
            if terrain is not None and abs(terrain[x,z]-terrain[nx,nz])>cell*.65:continue
            endpoints=[((x+1)*cell,z*cell),((x+1)*cell,(z+1)*cell)] if nx!=x else [(x*cell,(z+1)*cell),((x+1)*cell,(z+1)*cell)]
            edges.extend((i,j,px,pz) for px,pz in endpoints)
    return edges


def joint_fit(patches, terrain, mask, cell, seam_weight=10000., tolerance=None, max_iterations=5000):
    if not np.isfinite(seam_weight) or seam_weight<=0:raise ValueError('positive finite seam weight required')
    if tolerance is not None and (not np.isfinite(tolerance) or tolerance<0):raise ValueError('finite nonnegative tolerance required')
    if not patches:raise ValueError('patches required')
    result=copy.deepcopy(patches);count=len(patches);indices=[];values=[];targets=[]
    # Normalize slope coordinates to patch width for better conditioning.
    centers=[];spans=[]
    for p in patches:
        x,z,r,t=p['bounds'];axis=p['fit']['axis'];centers.append((x+r)/2 if axis=='x' else (z+t)/2);spans.append((r-x) if axis=='x' else (t-z))
    def coord(i,x,z):return ((x if patches[i]['fit']['axis']=='x' else z)-centers[i])/spans[i]
    samples=[]
    for i,p in enumerate(patches):
        x,z,r,t=[round(v/cell) for v in p['bounds']]
        xx,zz=np.meshgrid((np.arange(x,r)+.5)*cell,(np.arange(z,t)+.5)*cell,indexing='ij')
        hh=terrain[x:r,z:t]
        if not np.isfinite(hh).all():raise ValueError('patch includes unsupported terrain')
        cc=coord(i,xx,zz).ravel();samples.append((cc,hh.ravel()))
        for c,h in zip(cc,hh.ravel()):indices.append([2*i,2*i+1,2*i,2*i]);values.append([1,c,0,0]);targets.append(h)
    edges=shared_edges(patches,mask,cell,terrain);weight=seam_weight**.5
    for i,j,x,z in edges:
        indices.append([2*i,2*i+1,2*j,2*j+1]);values.append([weight,weight*coord(i,x,z),-weight,-weight*coord(j,x,z)]);targets.append(0.)
    ix=np.array(indices);vv=np.array(values);rhs=np.array(targets)
    def transpose(v):return np.bincount(ix.ravel(),weights=(vv*v[:,None]).ravel(),minlength=2*count)
    def mat(v):return transpose(np.sum(vv*v[ix],axis=1))+1e-10*v
    diagonal=np.bincount(ix.ravel(),weights=(vv*vv).ravel(),minlength=2*count)+1e-10
    solution=np.zeros(2*count);residual=transpose(rhs);pre=residual/diagonal;direction=pre.copy();rz=residual@pre;initial=max(np.linalg.norm(residual),1.)
    iterations=0
    for iterations in range(1,1 if tolerance is not None else 10001):
        product=mat(direction);denom=direction@product
        if denom<=0:break
        alpha=rz/denom;solution+=alpha*direction;residual-=alpha*product
        if np.linalg.norm(residual)<initial*1e-10:break
        pre=residual/diagonal;new=residual@pre;direction=pre+(new/rz)*direction;rz=new
    if tolerance is not None:
        # Feasibility rather than forcing every seam toward exact equality:
        # each source sample and shared endpoint has the same error allowance.
        scale=np.where(np.arange(len(rhs))<sum(len(h) for _,h in samples),1.,weight)
        vv=vv/scale[:,None];rhs=rhs/scale
        bound=np.bincount(ix.ravel(),weights=(abs(vv)*np.sum(abs(vv),axis=1)[:,None]).ravel(),minlength=2*count)+1e-10
        momentum=solution.copy();accel=1.
        for iterations in range(1,max_iterations+1):
            residual=np.sum(vv*momentum[ix],axis=1)-rhs
            excess=np.sign(residual)*np.maximum(abs(residual)-tolerance,0.)
            next_solution=momentum-transpose(excess)/bound
            if np.max(abs(excess))<1e-5:
                solution=next_solution;break
            next_accel=(1+np.sqrt(1+4*accel*accel))/2
            momentum=next_solution+(accel-1)/next_accel*(next_solution-solution)
            solution=next_solution;accel=next_accel
        residual=np.sum(vv*solution[ix],axis=1)-rhs
    errors=[]
    for i,p in enumerate(result):
        cc,hh=samples[i];error=float(np.max(abs(solution[2*i]+solution[2*i+1]*cc-hh)));errors.append(error)
        gradient=float(solution[2*i+1]/spans[i])
        p['fit'].update(coordinate_origin=centers[i],height_at_origin=float(solution[2*i]),gradient=gradient,maximum_error=error,
                        rms_error=float(np.sqrt(np.mean((solution[2*i]+solution[2*i+1]*cc-hh)**2))),
                        direction_id=(0 if gradient>=0 else 3) if p['fit']['axis']=='x' else (2 if gradient>=0 else 1))
    gaps=[abs(solution[2*i]+solution[2*i+1]*coord(i,x,z)-solution[2*j]-solution[2*j+1]*coord(j,x,z)) for i,j,x,z in edges]
    rejected={i for i,e in enumerate(errors) if e>.25}
    for (i,j,_,_),gap in zip(edges,gaps):
        if gap>.25:rejected.update((i,j))
    for i,p in enumerate(result):p['center_and_seam_checks_pass']=i not in rejected
    return result,dict(steep_adjacency_endpoints_requiring_review=len(shared_edges(patches,mask,cell))-len(edges),patches_passing_local_checks=count-len(rejected),iterations=iterations,maximum_constraint_excess=float(max(0.,max(max(errors,default=0.),max(gaps,default=0.))-(tolerance if tolerance is not None else 0.))),maximum_center_error=max(errors,default=0.),maximum_seam_gap=float(max(gaps,default=0.)),center_failures=sum(e>.25 for e in errors),seam_failures=int(sum(g>.25 for g in gaps)),shared_edge_endpoints=len(edges),seam_weight=seam_weight)


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('directory',type=Path);p.add_argument('--seam-weight',type=float,default=10000.);p.add_argument('--tolerance',type=float);p.add_argument('--tile',type=int);a=p.parse_args();a.directory.resolve().relative_to(Path('scratch').resolve());out=output_path(a.directory)
    if a.tile is not None and a.tile<1:p.error('tile must be positive')
    patches=json.loads((out/'slope-candidates.json').read_text())['patches'];cell=json.loads((out/'report.json').read_text())['cell_size']
    with np.load(out/'grid.npz') as g:
        if a.tile==1:
            patches=[]
            h=g['terrain'];mask=g['reachable']
            for x,z in zip(*np.where(mask)):
                slopes=[]
                for dx,dz in ((1,0),(0,1)):
                    neighbors=[(xx,zz) for xx,zz in ((x-dx,z-dz),(x+dx,z+dz)) if 0<=xx<h.shape[0] and 0<=zz<h.shape[1] and np.isfinite(h[xx,zz])]
                    slopes.append(max((abs(h[xx,zz]-h[x,z]) for xx,zz in neighbors),default=0.))
                patches.append(dict(bounds=[int(x)*cell,int(z)*cell,int(x+1)*cell,int(z+1)*cell],fit=dict(axis='x' if slopes[0]>=slopes[1] else 'z')))
        elif a.tile:
            from survey_whole import fit_patches
            patches,failed=fit_patches(g['terrain'],g['reachable'],cell,.25,tile=a.tile)
            if failed:raise ValueError('unfitted cells')
        result,report=joint_fit(patches,g['terrain'],g['reachable'],cell,a.seam_weight,a.tolerance)
    report.update(patches=len(patches),feasibility_tolerance=a.tolerance,steep_adjacencies_excluded_from_smooth_seams=True,input_sha256={name:hashlib.sha256((out/name).read_bytes()).hexdigest() for name in ('grid.npz','report.json','slope-candidates.json')})
    from PIL import Image
    with np.load(out/'grid.npz') as g:shape=g['terrain'].shape
    rgb=np.full((*shape,3),32,np.uint8)
    for patch in result:
        x,z,r,t=[round(v/cell) for v in patch['bounds']]
        rgb[x:r,z:t]=(65,170,95) if patch['center_and_seam_checks_pass'] else (220,65,45)
    Image.fromarray(rgb.transpose(1,0,2)).save(out/'joint-fit-checks.png')
    report.update(installable=False,remaining_checks=['Source rectangle corners and edge support','Engine floor levels and bidirectional landing projection','Open boundary drop/transition evidence'])
    (out/'joint-slope-candidates.json').write_text(json.dumps(dict(patches=result,report=report),indent=2)+'\n');print(json.dumps(report,indent=2))

if __name__=='__main__':main()
