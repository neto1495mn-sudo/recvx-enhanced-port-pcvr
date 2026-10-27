"""Reject discontinuous slope candidates and model engine type-6 projection.

This models the documented branch of bhCheckWallEx, not the complete player or
zombie controller. Evidence and candidate patch IDs remain ignored artifacts.
"""
import argparse
import json
import math
from pathlib import Path
import numpy as np
from build_rooms import output_path


def patch_height(patch, x, z):
    f=patch['fit'];coordinate=x if f['axis']=='x' else z
    return f['gradient']*(coordinate-f['coordinate_origin'])+f['height_at_origin']


def project_slope(position, record, grand, floor, room_height=200., actor_height=17.5):
    """Single active type-6 record, slope-eligible actor; returns (xyz,floor,hit).

    Engine quantizes Y by C integer truncation. A zero record height means the
    room height, NOT a horizontal floor. Do not use flat fit patches as slopes.
    """
    pos=np.array(position,dtype=float);pos[1]=math.trunc(100*(.001+pos[1]))/100
    x,z,w,d=record['px'],record['pz'],record['w'],record['d']
    y=math.trunc(100*(.001+record['py']))/100;h=record['h'] or room_height
    if not(x<=pos[0]<x+w and z<=pos[2]<z+d and pos[1]+actor_height>=y and pos[1]<=y+h):return pos.tolist(),floor,False
    direction=record['id']
    if direction==0:origin=np.array([x,y,pos[2]]);vector=np.array([w,h,0.])
    elif direction==1:origin=np.array([pos[0],y,z+d]);vector=np.array([0.,h,-d])
    elif direction==2:origin=np.array([pos[0],y,z]);vector=np.array([0.,h,d])
    elif direction==3:origin=np.array([x+w,y,pos[2]]);vector=np.array([-w,h,0.])
    else:raise ValueError('invalid slope direction')
    projected=origin+vector*np.dot(pos-origin,vector)/np.dot(vector,vector)
    pos[1]=projected[1]
    for j in range(31):
        if (grand[j]!=0 or j==2) and pos[1]>=grand[j]:floor=j-2
    if floor!=record['flr_no']:
        if pos[1]<grand[floor+2]:pos[[0,2]]=projected[[0,2]]
        else:pos[1]=grand[floor+2]
    else:
        pos[[0,2]]=projected[[0,2]]
        pos[1]=max(pos[1],grand[floor+2])
    return pos.tolist(),floor,True


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('directory',type=Path);p.add_argument('--tolerance',type=float,default=.25);a=p.parse_args();out=output_path(a.directory)
    patches=json.loads((out/'slope-candidates.json').read_text())['patches'];report=json.loads((out/'report.json').read_text());cell=report['cell_size']
    with np.load(out/'grid.npz') as g:mask=g['reachable'];terrain=g['terrain']
    owner=np.full(mask.shape,-1,int)
    for i,patch in enumerate(patches):
        x,z,r,t=[round(v/cell) for v in patch['bounds']];owner[x:r,z:t]=i
    failures=[];tested=0;max_error=0.;pair_count=set()
    for x,z in zip(*np.where(mask)):
        for nx,nz in ((x+1,z),(x,z+1)):
            if nx>=mask.shape[0] or nz>=mask.shape[1] or not mask[nx,nz]:continue
            left,right=owner[x,z],owner[nx,nz]
            if left<0 or right<0 or left==right:continue
            # Test the shared cell-edge endpoints, not just its center.
            edge=[((x+1)*cell,z*cell),((x+1)*cell,(z+1)*cell)] if nx!=x else [(x*cell,(z+1)*cell),((x+1)*cell,(z+1)*cell)]
            error=max(abs(patch_height(patches[left],px,pz)-patch_height(patches[right],px,pz)) for px,pz in edge)
            tested+=1;max_error=max(max_error,error);pair_count.add((int(left),int(right)))
            if error>a.tolerance:failures.append(dict(patches=[int(left),int(right)],edge=edge,maximum_gap=error))
    flat=[p for p in patches if abs(p['fit']['gradient'])<1e-6]
    levels=sorted({round(p['fit']['height_at_origin'],3) for p in flat})
    result=dict(tested_seam_edges=tested,adjacent_patch_pairs=len(pair_count),seam_failures=len(failures),maximum_gap=max_error,tolerance=a.tolerance,flat_patches=len(flat),candidate_flat_heights=levels,
                floor_level_capacity=31,flat_patches_must_not_use_zero_height_type6=True,installable=False,failures=failures,
                next_steps=['Jointly fit shared edges; independently fitted slopes have incompatible boundaries','Validate all rectangle corners against the source mesh','Assign plateau grand[] levels and simulate both travel directions through each landing','Audit the whole actor movement loop, including non-slope hits and zombie eligibility'])
    (out/'seam-audit.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k!='failures'},indent=2))

if __name__=='__main__':main()
