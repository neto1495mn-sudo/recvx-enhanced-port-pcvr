"""Adaptive source-sampled terrain candidates; never installs unverified floors.

Nine probes per rectangle include corners and edge midpoints. Shared edges are
checked at every subdivision endpoint and midpoint. These discrete checks do not
certify polygon coverage between probes or engine movement through the wall loop.
"""
import argparse
from collections import defaultdict, Counter
import hashlib
import json
from pathlib import Path

import numpy as np

from navigation import read_obj
from fit_transitions import fit_axis_slope
from audit_terrain_seams import patch_height
from build_rooms import output_path


class TerrainSampler:
    """Spatially indexed reviewed triangles, matching the survey's layer policy."""
    def __init__(self, triangles, bin_size=2.):
        self.bin_size = bin_size
        self.bins = defaultdict(list)
        self.cache = {}
        for triangle in triangles:
            p = np.asarray(triangle, float)
            normal = np.cross(p[1]-p[0], p[2]-p[0])
            if np.linalg.norm(normal) < 1e-9 or abs(normal[1])/np.linalg.norm(normal) <= .2:
                continue
            lo = np.floor(p[:, [0, 2]].min(axis=0)/bin_size).astype(int)
            hi = np.floor(p[:, [0, 2]].max(axis=0)/bin_size).astype(int)
            # Include the bin on the other side of an exact boundary as well.
            for x in range(lo[0]-1, hi[0]+1):
                for z in range(lo[1]-1, hi[1]+1):
                    self.bins[x, z].append(p)
        self.bins = {k: np.asarray(v) for k, v in self.bins.items()}

    def __call__(self, x, z):
        key = (float(x), float(z))
        if key in self.cache:
            return self.cache[key]
        p = self.bins.get((int(np.floor(x/self.bin_size)), int(np.floor(z/self.bin_size))))
        value = None
        if p is not None:
            a, b = p[:, 1]-p[:, 0], p[:, 2]-p[:, 0]
            dx, dz = x-p[:, 0, 0], z-p[:, 0, 2]
            det = a[:, 0]*b[:, 2]-a[:, 2]*b[:, 0]
            u = (dx*b[:, 2]-dz*b[:, 0])/det
            v = (a[:, 0]*dz-a[:, 2]*dx)/det
            keep = (u >= -1e-8) & (v >= -1e-8) & (u+v <= 1+1e-8)
            heights = (p[:, 0, 1]+u*a[:, 1]+v*b[:, 1])[keep]
            if len(heights):
                value = float(heights[np.argmin(abs(heights))])
        self.cache[key] = value
        return value


def reviewed_triangles(faces, mapping, transform, excluded):
    for group, material, vertices in faces:
        info = dict(mapping['materials'].get(material, {}))
        overrides = mapping.get('group_overrides', {}).get(group, {})
        for key in ('*', material):
            info.update(overrides.get(key, {}))
        if material in excluded or info.get('exclude') or info.get('class') != 'ground':
            continue
        p = (np.asarray(vertices)-transform['source_origin'])*transform['scale']
        if transform.get('reflect_z'):
            p[:, 2] *= -1
        yield p


def refine(bounds, sampler, tolerance=.12, minimum=.125, maximum_gradient=.65, parent=None):
    x, z, r, t = bounds
    points = [(px, sampler(px, pz), pz) for px in (x, (x+r)/2, r) for pz in (z, (z+t)/2, t)]
    complete = all(p[1] is not None for p in points)
    fit = fit_axis_slope(points) if complete else None
    reason = 'missing_support' if not complete else ('source_fit' if fit['maximum_error'] > tolerance else
              ('steep' if abs(fit['gradient']) > maximum_gradient else 'pass'))
    # Steep fits cannot be cured by subdivision of a planar incline. Keep them
    # explicit for a stepped-route design instead of inventing a flat floor.
    if reason in ('source_fit', 'missing_support') and min(r-x, t-z) > minimum+1e-8:
        mx, mz = (x+r)/2, (z+t)/2
        return [p for b in ((x,z,mx,mz),(mx,z,r,mz),(x,mz,mx,t),(mx,mz,r,t))
                for p in refine(b, sampler, tolerance, minimum, maximum_gradient, parent)]
    return [dict(bounds=list(bounds), fit=fit, source_check=reason, parent=parent,
                 supported_probes=sum(p[1] is not None for p in points))]


def edge_audit(patches, tolerance=.25):
    """Compare all adjacent quadtree leaves, including T junctions."""
    sides = defaultdict(lambda: ([], []))
    for i, p in enumerate(patches):
        x,z,r,t = p['bounds']
        sides['x',x][1].append((z,t,i)); sides['x',r][0].append((z,t,i))
        sides['z',z][1].append((x,r,i)); sides['z',t][0].append((x,r,i))
    failures=[]; tested=0; maximum=0.; graph=defaultdict(set)
    for (axis, coordinate), (left, right) in sides.items():
        left.sort(); right.sort(); j=0
        for lo,hi,i in left:
            while j<len(right) and right[j][1]<=lo+1e-9: j+=1
            k=j
            while k<len(right) and right[k][0]<hi-1e-9:
                low,high,n=right[k]; k+=1
                low,high=max(lo,low),min(hi,high)
                if high-low<=1e-9: continue
                if patches[i]['fit'] is None or patches[n]['fit'] is None:
                    failures.append(dict(patches=[i,n],reason='missing_support')); continue
                points=[(coordinate,v) if axis=='x' else (v,coordinate) for v in (low,(low+high)/2,high)]
                gap=max(abs(patch_height(patches[i],*p)-patch_height(patches[n],*p)) for p in points)
                tested+=1; maximum=max(maximum,gap)
                if gap>tolerance:
                    failures.append(dict(patches=[i,n],reason='height_gap',maximum_gap=gap,edge=[list(points[0]),list(points[-1])]))
                elif patches[i]['source_check']=='pass' and patches[n]['source_check']=='pass':
                    graph[i].add(n); graph[n].add(i)
    unseen={i for i,p in enumerate(patches) if p['source_check']=='pass'}; components=[]
    while unseen:
        queue=[unseen.pop()]; count=0
        while queue:
            i=queue.pop(); count+=1
            for j in graph[i]:
                if j in unseen: unseen.remove(j); queue.append(j)
        components.append(count)
    return dict(tested_edges=tested,maximum_gap=maximum,failed_edges=len(failures),
                passing_components=sorted(components,reverse=True),failures=failures)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('recipe',type=Path)
    parser.add_argument('--tolerance',type=float,default=.12)
    parser.add_argument('--minimum',type=float,default=.125)
    args=parser.parse_args()
    args.recipe.resolve().relative_to(Path('scratch').resolve())
    if not np.isfinite(args.tolerance) or args.tolerance<=0 or not np.isfinite(args.minimum) or args.minimum<=0:
        parser.error('tolerance and minimum must be finite and positive')
    spec=json.loads(args.recipe.read_text()); root=output_path(Path(spec['output'])); out=output_path(root/'terrain')
    transform=json.loads((root/'transform.json').read_text()); mapping=json.loads(Path(spec['mapping']).read_text())
    if spec.get('overrides'): mapping['group_overrides']=json.loads(Path(spec['overrides']).read_text())
    print('Indexing reviewed source triangles',flush=True)
    sampler=TerrainSampler(reviewed_triangles(read_obj(spec['obj']),mapping,transform,spec['exclude_materials']))
    report=json.loads((out/'report.json').read_text()); cell=report['cell_size']
    old=json.loads((out/'joint-slope-candidates.json').read_text())
    with np.load(out/'grid.npz') as g: mask=g['reachable']
    patches=[]; parents=[]
    for i,(x,z) in enumerate(zip(*np.where(mask))):
        bounds=[float(x*cell),float(z*cell),float((x+1)*cell),float((z+1)*cell)]
        rows=refine(bounds,sampler,args.tolerance,args.minimum,parent=i)
        parents.append(dict(bounds=bounds,all_source_checks_pass=all(p['source_check']=='pass' for p in rows)))
        patches.extend(rows)
        if (i+1)%4000==0: print(f'{i+1} cells, {len(patches)} leaves',flush=True)
    old_failed={tuple(p['bounds']) for p in old['patches'] if not p['center_and_seam_checks_pass']}
    audit=edge_audit(patches)
    counts=Counter(p['source_check'] for p in patches)
    summary=dict(parent_cells=len(parents),patches=len(patches),source_checks=dict(counts),
                 formerly_failed_cells=len(old_failed),formerly_failed_cells_source_refitted=sum(tuple(p['bounds']) in old_failed and p['all_source_checks_pass'] for p in parents),
                 source_tolerance=args.tolerance,minimum_requested=args.minimum,source_probes=len(sampler.cache),
                 seam_summary={k:v for k,v in audit.items() if k!='failures'},installable=False,
                 remaining_checks=['Continuous source polygon coverage between probes','Stepped routes or retaining walls at rejected terrain',
                                   'Floor levels and complete engine wall-loop movement','Boundary closure and actor-radius connectivity'],
                 input_sha256={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in [args.recipe,Path(spec['obj']),Path(spec['mapping']),root/'transform.json',out/'grid.npz',out/'joint-slope-candidates.json']+([Path(spec['overrides'])] if spec.get('overrides') else [])})
    (out/'refined-slope-candidates.json').write_text(json.dumps(dict(report=summary,patches=patches,seams=audit),indent=2,allow_nan=False)+'\n')
    from PIL import Image
    rgb=np.full((*mask.shape,3),32,np.uint8)
    for p in parents:
        x,z,_,_=p['bounds'];rgb[round(x/cell),round(z/cell)]=(65,170,95) if p['all_source_checks_pass'] else (220,65,45)
    Image.fromarray(rgb.transpose(1,0,2)).save(out/'refined-fit-checks.png')
    print(json.dumps(summary,indent=2))


if __name__=='__main__': main()
