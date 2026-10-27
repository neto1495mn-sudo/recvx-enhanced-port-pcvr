"""Exercise candidate type-6 terrain records through bidirectional movement.

This is a slope-only wall-loop simulation for standing actors. Static collision,
combat transitions and engine runtime acceptance are separate gates.
"""
import argparse
import collections
import json
import math
from pathlib import Path
import numpy as np
from build_rooms import output_path
from audit_terrain_seams import patch_height


def records_for_patch(patch,padding=0.):
    x,z,r,t=patch['bounds'];f=patch['fit'];g=f['gradient'];axis=f['axis']
    # Flat zero is the actual floor-0 grand level; it needs no inclined wall.
    if abs(g)<1e-7 and abs(f['height_at_origin'])<.005:return []
    if axis=='x':x-=padding;r+=padding
    else:z-=padding;t+=padding
    low,high=(x,r) if axis=='x' else (z,t)
    cuts=[low,high]
    if abs(g)>1e-7:
        zero=f['coordinate_origin']-f['height_at_origin']/g
        if low+1e-6<zero<high-1e-6:cuts.insert(1,zero)
    result=[]
    for lo,hi in zip(cuts,cuts[1:]):
        bounds=[lo,z,hi,t] if axis=='x' else [x,lo,r,hi]
        a,b=[g*(v-f['coordinate_origin'])+f['height_at_origin'] for v in (lo,hi)]
        bottom=min(a,b);height=abs(b-a)
        if height<.02:bottom=(a+b)/2-.01;height=.02
        # Quantization is performed by hitchk before applying the line.
        bottom=math.floor(bottom*100+.5)/100
        direction=(0 if g>=0 else 3) if axis=='x' else (2 if g>=0 else 1)
        bx,bz,br,bt=bounds
        result.append(dict(flg=1,type=6,id=direction,flr_no=-1 if (a+b)/2<-.005 else 0,attr=0,
                           px=bx,py=bottom,pz=bz,w=br-bx,h=height,d=bt-bz,prm0=0,prm1=0,prm2=0,prm3=0))
    return result


class SlopeLoop:
    def __init__(self,records,grand,bin_size=4.):
        self.records=records;self.grand=grand;self.bin_size=bin_size;self.bins=collections.defaultdict(list)
        for i,r in enumerate(records):
            for x in range(math.floor(r['px']/bin_size),math.floor((r['px']+r['w'])/bin_size)+1):
                for z in range(math.floor(r['pz']/bin_size),math.floor((r['pz']+r['d'])/bin_size)+1):self.bins[x,z].append(i)

    def move(self,position,floor):
        x,y,z=position;y=math.trunc(100*(.001+y))/100;hit=False;last=-1
        # The engine scans records in order. Re-query after a projection moves
        # X/Z, but never visit earlier records again within this wall-loop call.
        while True:
            candidates=self.bins.get((math.floor(x/self.bin_size),math.floor(z/self.bin_size)),())
            changed=False
            for i in candidates:
                if i<=last:continue
                last=i;r=self.records[i];py=math.trunc(100*(.001+r['py']))/100;h=r['h']
                if not(r['px']<=x<r['px']+r['w'] and r['pz']<=z<r['pz']+r['d'] and y+17.5>=py and y<=py+h):continue
                hit=True;d=r['id']
                if d==0:origin=r['px'];v=r['w'];coordinate=x
                elif d==3:origin=r['px']+r['w'];v=-r['w'];coordinate=x
                elif d==2:origin=r['pz'];v=r['d'];coordinate=z
                else:origin=r['pz']+r['d'];v=-r['d'];coordinate=z
                k=((coordinate-origin)*v+(y-py)*h)/(v*v+h*h);new_coordinate=origin+k*v;y=py+k*h
                for j,level in enumerate(self.grand[:31]):
                    if (level!=0 or j==2) and y>=level:floor=j-2
                if floor!=r['flr_no']:
                    if y<self.grand[floor+2]:coordinate=new_coordinate
                    else:y=self.grand[floor+2]
                else:coordinate=new_coordinate;y=max(y,self.grand[floor+2])
                old=(math.floor(x/self.bin_size),math.floor(z/self.bin_size))
                if d in (0,3):x=coordinate
                else:z=coordinate
                if old!=(math.floor(x/self.bin_size),math.floor(z/self.bin_size)):changed=True;break
            if not changed:break
        if not hit:y=self.grand[floor+2]
        return [x,y,z],floor,hit


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('recipe',type=Path);p.add_argument('--padding',type=float,default=0.);p.add_argument('--stride',type=int,default=1);a=p.parse_args()
    spec=json.loads(a.recipe.read_text());root=output_path(spec['output']);out=output_path(root/'movement')
    patches=json.loads((root/'terrain/corner-patches.json').read_text());
    if patches['failed']:raise ValueError('unfitted terrain')
    records=[r for patch in patches['patches'] for r in records_for_patch(patch,a.padding)]
    grand=[0.]*32;grand[1]=-2.;loop=SlopeLoop(records,grand)
    with np.load(root/'terrain/grid.npz') as grid:mask=grid['reachable'];h=grid['terrain']
    cell=json.loads((root/'terrain/report.json').read_text())['cell_size']
    starts=[];ends=[]
    for x,z in zip(*np.where(mask)):
        if (x+z)%a.stride:continue
        for dx,dz in ((1,0),(0,1)):
            nx,nz=x+dx,z+dz
            if nx>=mask.shape[0] or nz>=mask.shape[1] or not mask[nx,nz]:continue
            p0=[(x+.5)*cell,float(h[x,z]),(z+.5)*cell];p1=[(nx+.5)*cell,float(h[nx,nz]),(nz+.5)*cell]
            starts.extend((p0,p1));ends.extend((p1,p0))
    positions=[];failures=[];hits=0
    for index,(start,end) in enumerate(zip(starts,ends)):
        position=start.copy();floor=-1 if start[1]<0 else 0;step=(np.array(end)[[0,2]]-np.array(start)[[0,2]])/8
        for _ in range(8):
            position[0]+=step[0];position[2]+=step[1];position,floor,hit=loop.move(position,floor);hits+=hit
        positions.append(position)
        if index and index%20000==0:print('trajectories',index,flush=True)
    from terrain_source import TerrainSource
    from navigation import read_obj
    mapping=json.loads(Path(spec['mapping']).read_text());mapping['navigation_excluded_materials']=spec['exclude_materials']
    if spec.get('overrides'):mapping['group_overrides']=json.loads(Path(spec['overrides']).read_text())
    source=TerrainSource(read_obj(spec['obj']),mapping,json.loads((root/'transform.json').read_text()))
    ps=np.array(positions);ys=source.heights(ps[:,[0,2]]);error=abs(ys-ps[:,1])
    for i in np.where(~np.isfinite(error)|(error>.5))[0]:failures.append(dict(start=starts[i],target=ends[i],actual=positions[i],source_y=float(ys[i]) if np.isfinite(ys[i]) else None))
    report=dict(slope_records=len(records),trajectories=len(starts),steps=len(starts)*8,slope_hits=hits,failed_endpoints=len(failures),maximum_height_error=float(np.nanmax(error)),padding=a.padding,installable=False)
    (out/'terrain-walls.json').write_text(json.dumps(dict(walls=records,grand=grand),indent=2)+'\n')
    (out/'report.json').write_text(json.dumps(dict(report=report,failures=failures),indent=2)+'\n');print(json.dumps(report,indent=2))

if __name__=='__main__':main()
