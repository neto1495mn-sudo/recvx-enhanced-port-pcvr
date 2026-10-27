"""Convert adaptive source fits into PC slopes and audit the actual eligibility band.

Source recipes/data stay in scratch. Flats are explicit slopes, grand has one
floor, and both directions across seams are tested before publication.
"""
import argparse
import collections
import hashlib
import json
import math
from pathlib import Path
import sys
import numpy as np
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
import rdxbuild as b
from build_rooms import output_path
from refine_terrain import edge_audit


def record(patch,padding=0):
    x,z,r,t=patch['bounds'];f=patch['fit'];g=f['gradient']
    if padding:
        if f['axis']=='x':x-=padding;r+=padding
        else:z-=padding;t+=padding
    lo,hi=(x,r) if f['axis']=='x' else (z,t)
    ys=[g*(v-f['coordinate_origin'])+f['height_at_origin'] for v in (lo,hi)]
    rise=('+' if g>=0 else '-')+f['axis']
    # Exercise the heightfield helper for square planar cells; adaptive strips
    # use the equivalent rectangle helper without resampling their fitted plane.
    if abs(r-x-(t-z))<1e-8:
        ys4=[[g*((px if f['axis']=='x' else pz)-f['coordinate_origin'])+f['height_at_origin'] for px in (x,r)] for pz in (z,t)]
        return b.heightfield_slopes(ys4,x,z,r-x)[0]
    return b.slope_wall(x,z,r-x,t-z,min(ys),max(ys),rise)


class Ground:
    def __init__(self,records,cell=4):
        self.records=records;self.cell=cell;self.bins=collections.defaultdict(list)
        for i,r in enumerate(records):
            for x in range(math.floor(r['px']/cell),math.ceil((r['px']+r['w'])/cell)):
                for z in range(math.floor(r['pz']/cell),math.ceil((r['pz']+r['d'])/cell)):self.bins[x,z].append(i)

    def candidates(self,x,z):
        return [(i,b.slope_height(self.records[i],x,z)) for i in self.bins.get((math.floor(x/self.cell),math.floor(z/self.cell)),[]) if b.slope_height(self.records[i],x,z) is not None]

    def height(self,x,z,y=None,ah=17.5):
        cs=self.candidates(x,z)
        if y is not None:cs=[(i,h) for i,h in cs if y+ah>=self.records[i]['py'] and y-abs(ah)<=self.records[i]['py']+self.records[i]['h']]
        return min(cs,key=lambda p:abs(p[1]-(0 if y is None else y)))[1] if cs else None


def record_patch(r):
    axis='x' if r['id'] in (0,3) else 'z'
    width=r['w'] if axis=='x' else r['d']
    center=(r['px'] if axis=='x' else r['pz'])+width/2
    return dict(bounds=[r['px'],r['pz'],r['px']+r['w'],r['pz']+r['d']],source_check='pass',
                fit=dict(axis=axis,gradient=r['h']/width*(1 if r['id'] in (0,2) else -1),
                         coordinate_origin=center,height_at_origin=r['py']+r['h']/2))


def movement(ground,mask,terrain,cell,grand):
    from slope_engine import EngineProbe
    engine=EngineProbe(ground.records)
    failures=[];steps=0;max_error=0;uncovered=0
    for x,z in zip(*np.where(mask)):
        px,pz=(x+.5)*cell,(z+.5)*cell
        h=ground.height(px,pz)
        if h is None:uncovered+=1;continue
        for dx,dz in ((1,0),(0,1)):
            nx,nz=x+dx,z+dz
            if nx>=mask.shape[0] or nz>=mask.shape[1] or not mask[nx,nz]:continue
            qx,qz=(nx+.5)*cell,(nz+.5)*cell;qh=ground.height(qx,qz)
            if qh is None:continue
            for start,end in (([px,h,pz],[qx,qh,qz]),([qx,qh,qz],[px,h,pz])):
                y=start[1];bad=[]
                for k in range(1,9):
                    u=k/8;xx=start[0]*(1-u)+end[0]*u;zz=start[2]*(1-u)+end[2]*u
                    indices=ground.bins.get((math.floor(xx/ground.cell),math.floor(zz/ground.cell)),[])
                    new=engine.height(indices,xx,zz,y,17.5);steps+=1
                    if new is None:
                        bad.append(dict(x=xx,z=zz,previous_y=y,unconditional_surface=ground.height(xx,zz)))
                        y=grand
                    else:y=new
                error=abs(y-end[1]);max_error=max(max_error,error)
                if bad or error>.5:failures.append(dict(start=start,end=end,ineligible_steps=bad,endpoint_error=error))
    return dict(steps=steps,failed_trajectories=len(failures),uncovered_centers=uncovered,maximum_endpoint_error=max_error,engine=engine.evidence),failures


def coverage(ground,mask,cell):
    # Include exact lattice seams, not just original cell centers.
    gaps=[];samples=0
    for x,z in zip(*np.where(mask)):
        for dx in np.arange(0,cell,.25):
            for dz in np.arange(0,cell,.25):
                xx=x*cell+dx;zz=z*cell+dz;samples+=1
                if not ground.candidates(xx,zz):gaps.append([xx,zz])
    return samples,gaps


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('recipe',type=Path);a=p.parse_args()
    a.recipe.resolve().relative_to(Path('scratch').resolve());spec=json.loads(a.recipe.read_text());root=output_path(spec['output']);out=output_path(root/'terrain')
    source=out/'corner-patches.json';j=json.loads(source.read_text())
    if j['failed']:raise ValueError('unfitted source patches')
    patches=j['patches'];records=[record(q) for q in patches]
    for q in patches:q['source_check']='pass'
    seams=edge_audit([record_patch(r) for r in records],1.)
    with np.load(out/'grid.npz') as grid:mask=grid['reachable'];terrain=grid['terrain']
    cell=json.loads((out/'report.json').read_text())['cell_size'];ground=Ground(records)
    grand=[0.]*32;grand[2]=math.floor(min(r['py'] for r in records))-1
    audit,failures=movement(ground,mask,terrain,cell,grand[2])
    samples,gaps=coverage(ground,mask,cell)
    report=dict(slope_records=len(records),walkable_cells=int(mask.sum()),coverage_samples=samples,uncovered_subcells=len(gaps),
                maximum_seam_step=seams['maximum_gap'],seam_failures=seams['failed_edges'],movement=audit,
                one_floor=True,grand=grand,source_lattice_tolerance=j['report']['tolerance'],
                installable=not gaps and not seams['failed_edges'] and not audit['failed_trajectories'] and not audit['uncovered_centers'],
                input_sha256={str(f):hashlib.sha256(f.read_bytes()).hexdigest() for f in (source,out/'grid.npz',out/'report.json')})
    (out/'pc-slopes.json').write_text(json.dumps(dict(report=report,walls=records,grand=grand,coverage_gaps=gaps,movement_failures=failures),indent=2)+'\n')
    print(json.dumps(report,indent=2),flush=True)

if __name__=='__main__':main()
