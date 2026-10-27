"""Author exact whole-survey solid footprints and audit actor clearance.

Static boxes enclose the surveyed terrain band. Explicit support-edge walls
confine actor centers to authored terrain without adding terrain.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import numpy as np
from PIL import Image
from outline import rectangles
from survey_whole import terrain_flood
from build_rooms import output_path
from validate_outline import blocked_boxes


def build_candidate(solid, terrain, cell, radius):
    boxes=rectangles(solid)
    cover=np.zeros_like(solid)
    for x,z,r,t in boxes: cover[x:r,z:t]=True
    if not np.array_equal(cover,solid): raise ValueError('outline changed source footprints')
    finite=terrain[np.isfinite(terrain)]
    if not len(finite): raise ValueError('no reviewed heights')
    bottom=math.floor(float(finite.min()))-1
    top=math.ceil(float(finite.max()))+18
    records=[dict(flg=1,type=0,id=0,flr_no=0,attr=0,px=x*cell,py=bottom,pz=z*cell,
                  w=(r-x)*cell,h=top-bottom,d=(t-z)*cell,prm0=0,prm1=0,prm2=0,prm3=0) for x,z,r,t in boxes]
    # The vertical band encloses all source terrain; evaluate horizontal clearance
    # independently of Y, with the same strict inequalities used by box validation.
    audit_boxes=[[r['px'],0,r['pz'],r['w'],200,r['d']] for r in records]
    blocked=blocked_boxes(audit_boxes,[0,0,0,0],cell,solid.shape,radius)
    return records,blocked


def close_boundaries(openings, cell, radius, bottom, top):
    """Offset box faces by actor radius so clearance ends just inside the edge.

    Tangential spans cover the whole edge, including its endpoints. These are
    invisible collision barriers, not additional source terrain or geometry.
    """
    records=[]
    thickness=.125
    inset=.03125
    for edge in openings:
        x,z=edge['inside'];nx,nz=edge['outside'];dx,dz=nx-x,nz-z
        if abs(dx)+abs(dz)!=1:raise ValueError('non-cardinal boundary')
        if dx:
            face=(x+(dx>0))*cell+dx*(radius-inset)
            px=face if dx>0 else face-thickness
            pz=z*cell;w=thickness;d=cell
        else:
            face=(z+(dz>0))*cell+dz*(radius-inset)
            pz=face if dz>0 else face-thickness
            px=x*cell;w=cell;d=thickness
        records.append(dict(flg=1,type=0,id=0,flr_no=0,attr=0,px=px,py=bottom,pz=pz,
                            w=w,h=top-bottom,d=d,prm0=0,prm1=0,prm2=0,prm3=0))
    return records


def boundary_audit(openings, records, cell, radius):
    """Check each expanded wall's inner face and complete tangential coverage."""
    failures=[]
    if len(openings)!=len(records):raise ValueError('boundary wall count differs')
    for i,(edge,r) in enumerate(zip(openings,records)):
        x,z=edge['inside'];nx,nz=edge['outside'];dx,dz=nx-x,nz-z
        if dx:
            face=r['px']-radius if dx>0 else r['px']+r['w']+radius
            target=(x+(dx>0))*cell-dx*.03125
            covered=r['pz']-radius<=z*cell and r['pz']+r['d']+radius>=(z+1)*cell
        else:
            face=r['pz']-radius if dz>0 else r['pz']+r['d']+radius
            target=(z+(dz>0))*cell-dz*.03125
            covered=r['px']-radius<=x*cell and r['px']+r['w']+radius>=(x+1)*cell
        if abs(face-target)>1e-5 or not covered:failures.append(i)
    return dict(tested_edges=len(openings),closed_edges=len(openings)-len(failures),failed_edges=failures)


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('recipe',type=Path);a=p.parse_args()
    a.recipe.resolve().relative_to(Path('scratch').resolve())
    spec=json.loads(a.recipe.read_text());root=output_path(Path(spec['output']));out=output_path(root/'terrain')
    report=json.loads((out/'report.json').read_text());cell=report['cell_size'];radius=report['actor_radius']
    with np.load(out/'grid.npz') as g:
        solid=g['solid'];terrain=g['terrain'];baseline=g['reachable'];allowed=g['allowed'];survey_blocked=g['blocked']
    records,blocked=build_candidate(solid,terrain,cell,radius)
    footprint_blocked=blocked.copy()
    footprint_count=len(records)
    openings=json.loads((out/'outline-candidates.json').read_text())['open_boundaries']
    if len(openings)!=report['open_boundary_edges']:raise ValueError('stale boundary evidence')
    boundary_walls=close_boundaries(openings,cell,radius,records[0]['py'],records[0]['py']+records[0]['h'])
    closure=boundary_audit(openings,boundary_walls,cell,radius)
    records+=boundary_walls
    blocked=blocked_boxes([[r['px'],0,r['pz'],r['w'],200,r['d']] for r in records],[0,0,0,0],cell,solid.shape,radius)
    entry=json.loads((root/'room.json').read_text())['positions'][0];start=(int(entry['px']/cell),int(entry['pz']/cell))
    reached,_=terrain_flood(allowed&~blocked,terrain,start,cell,report['maximum_navigation_gradient'])
    base_reached,_=terrain_flood(allowed&~footprint_blocked,terrain,start,cell,report['maximum_navigation_gradient'])
    lost=int((baseline&~base_reached).sum());added=int((footprint_blocked&~survey_blocked).sum())
    if lost or added:raise ValueError(f'footprints lose {lost} baseline samples; {added} excess blocked samples')
    perimeter_blocked=int((baseline&blocked).sum())
    disconnected=int((baseline&~blocked&~reached).sum())
    if not reached[start]:raise ValueError('boundary wall blocks entry')
    np.savez_compressed(out/'collision-navigation.npz',reachable=reached,blocked=blocked)
    summary=dict(wall_records=len(records),script_toggled_records=0,exact_solid_cover=True,
                 baseline_reachable=int(baseline.sum()),candidate_reachable=int(reached.sum()),lost_baseline_samples=lost,
                 extra_blocked_samples=added,perimeter_restricted_samples=perimeter_blocked,disconnected_surviving_samples=disconnected,open_boundary_edges=len(closure['failed_edges']),source_open_boundary_edges=len(openings),
                 footprint_walls=footprint_count,boundary_walls=len(boundary_walls),boundary_closure=closure,
                 installable=not closure['failed_edges'],limitations=['Conservative full terrain-height band may overblock other floors',
                 'Invisible support-edge barriers inset centers by .03125; add no terrain','Survey-center clearance only; continuous engine motion pending'],
                 input_sha256={name:hashlib.sha256((out/name).read_bytes()).hexdigest() for name in ('grid.npz','report.json','outline-candidates.json')})
    (out/'static-collision-candidates.json').write_text(json.dumps(dict(report=summary,walls=records),indent=2)+'\n')
    rgb=np.full((*solid.shape,3),32,np.uint8);rgb[blocked]=(125,90,65);rgb[solid]=(205,160,110);rgb[reached]=(65,170,95)
    Image.fromarray(rgb.transpose(1,0,2)).save(out/'static-collision-checks.png')
    print(json.dumps(summary,indent=2))


if __name__=='__main__':main()
