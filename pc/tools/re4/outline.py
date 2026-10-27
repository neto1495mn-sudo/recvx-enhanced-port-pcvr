"""Compact surveyed solid cells without filling courtyards or inventing exit walls.

This exports candidate collision boxes, not a publishable room. Unsupported and survey
edges remain explicit transition requirements. All measured output stays in scratch.
"""
import argparse
import json
import heapq
from collections import deque
from pathlib import Path
import numpy as np


def rectangles(mask):
    """Exact disjoint cover; merge identical horizontal runs on successive rows."""
    active = {}
    result = []
    for z in range(mask.shape[1] + 1):
        row = mask[:, z] if z < mask.shape[1] else np.zeros(mask.shape[0], bool)
        edges = np.diff(np.r_[False, row, False].astype(int))
        runs = set(zip(np.where(edges == 1)[0], np.where(edges == -1)[0]))
        for run in list(active):
            if run not in runs:
                result.append((int(run[0]), active.pop(run), int(run[1]), z))
        for run in runs:
            active.setdefault(run, z)
    return sorted(result)


def expand(mask, cells):
    """Square clearance used by the existing CVX axis-aligned wall validator."""
    result = np.zeros_like(mask)
    # Integral image avoids quadratic per-cell box queries.
    pad = np.pad(mask.astype(np.int64), cells)
    sums = np.pad(pad, ((1, 0), (1, 0))).cumsum(0).cumsum(1)
    n = 2 * cells + 1
    result[:] = (sums[n:, n:] - sums[:-n, n:] - sums[n:, :-n] + sums[:-n, :-n]) > 0
    return result


def reduce_rectangles(rects, forbidden):
    """Merge boxes only when their bounding union touches no protected cell.

    Protect all clear supported ground, including disconnected courtyards, with
    actor clearance before calling this. This may fill inaccessible building
    interiors but cannot bridge an actor-sized passage or block a protected cell.
    No source solid is discarded. The result is still a review candidate.
    """
    sums = np.pad(forbidden.astype(np.int64), ((1, 0), (1, 0))).cumsum(0).cumsum(1)
    active = dict(enumerate(rects))
    queue = []

    def offer(i, j):
        a, b = active[i], active[j]
        box = (min(a[0], b[0]), min(a[1], b[1]), max(a[2], b[2]), max(a[3], b[3]))
        x0, z0, x1, z1 = box
        if sums[x1,z1]-sums[x0,z1]-sums[x1,z0]+sums[x0,z0]:
            return
        area = (x1-x0)*(z1-z0)
        extra = area-(a[2]-a[0])*(a[3]-a[1])-(b[2]-b[0])*(b[3]-b[1])
        heapq.heappush(queue, (extra, area, i, j, box))

    for i in active:
        for j in range(i):
            offer(i, j)
    next_id = len(active)
    while queue:
        _, _, i, j, box = heapq.heappop(queue)
        if i not in active or j not in active:
            continue
        del active[i], active[j]
        active[next_id] = box
        for other in list(active):
            if other != next_id:
                offer(next_id, other)
        next_id += 1
    return sorted(active.values())


def connect_regions(core, allowed):
    """Protect shortest cell paths joining core islands in each allowed component.

    Multi-source waves join with a spanning forest; disconnected courtyards are
    never connected through a solid. Protecting the resulting paths during box
    merging preserves narrow passages between the larger clear regions.
    """
    labels = np.full(core.shape, -1, dtype=np.int32)
    parents = {}
    count = 0
    def neighbors(x,z):
        for nx,nz in ((x-1,z),(x+1,z),(x,z-1),(x,z+1)):
            if 0<=nx<core.shape[0] and 0<=nz<core.shape[1]:
                yield nx,nz
    for x,z in zip(*np.where(core)):
        if labels[x,z]>=0:continue
        labels[x,z]=count;queue=deque([(x,z)])
        while queue:
            for nx,nz in neighbors(*queue.popleft()):
                if core[nx,nz] and labels[nx,nz]<0:
                    labels[nx,nz]=count;queue.append((nx,nz))
        count+=1
    roots=list(range(count))
    def root(i):
        while roots[i]!=i:
            roots[i]=roots[roots[i]];i=roots[i]
        return i
    result=core.copy()
    queue=deque(zip(*np.where(core)))
    def trace(point):
        while not result[point]:
            result[point]=True
            point=parents[point]
    while queue:
        x,z=queue.popleft()
        for nx,nz in neighbors(x,z):
            if not allowed[nx,nz]:continue
            if labels[nx,nz]<0:
                labels[nx,nz]=labels[x,z];parents[nx,nz]=(x,z);queue.append((nx,nz))
            else:
                a,b=root(labels[x,z]),root(labels[nx,nz])
                if a!=b:
                    roots[a]=b;trace((x,z));trace((nx,nz))
    return result


def boundary_points(reached, blocked, ground):
    result = {'solid': set(), 'unsupported_ground': set(), 'survey_edge': set()}
    for x, z in zip(*np.where(reached)):
        for nx, nz in ((x-1,z),(x+1,z),(x,z-1),(x,z+1)):
            if not (0 <= nx < reached.shape[0] and 0 <= nz < reached.shape[1]):
                kind = 'survey_edge'
            elif reached[nx, nz]:
                continue
            elif blocked[nx, nz]:
                kind = 'solid'
            elif not ground[nx, nz]:
                kind = 'unsupported_ground'
            else:
                continue
            result[kind].add((int(x), int(z)))
    return result


def clusters(points):
    remaining = set(points)
    result = []
    while remaining:
        todo = [remaining.pop()]
        group = []
        while todo:
            x, z = todo.pop()
            group.append((x, z))
            for dx in (-1,0,1):
                for dz in (-1,0,1):
                    p = (x+dx,z+dz)
                    if p in remaining:
                        remaining.remove(p)
                        todo.append(p)
        result.append(sorted(group))
    return sorted(result, key=lambda g: (-len(g),g[0]))


def candidate(report, solid, ground, reached, reduce=False, tolerance=0, transitions=None):
    cell = report['cell_size']
    bounds = report['bounds']
    # Only solids close enough to affect this component. Never dilate the boxes themselves:
    # the player collision applies the radius once more at runtime.
    near = expand(reached, int(np.ceil(report['actor_radius']/cell))+2)
    relevant = solid & near
    rects = rectangles(relevant)
    exact_count = len(rects)
    # Strict wall inequalities sampled at cell centers: equality remains clear.
    clearance = max(0,int(np.ceil(report['actor_radius']/cell+.5))-1)
    runtime_clear = ground & ~expand(solid, clearance)
    protected_clear = ground & ~expand(solid, clearance+int(np.ceil(tolerance/cell)))
    if tolerance:
        if 'entry' in report:
            entry=tuple(int(np.floor((report['entry'][k]-bounds[k])/cell)) for k in range(2))
            if all(0<=entry[k]<ground.shape[k] for k in range(2)) and runtime_clear[entry]:
                protected_clear[entry]=True
        protected_clear=connect_regions(protected_clear,runtime_clear)
    protected = expand(protected_clear, clearance)
    if transitions is not None:
        # Apply the same bounded simplification to transition footprints. Raw
        # slope triangles can overlap buildings: protect their clear portions,
        # not every projected terrain pixel underneath an existing solid.
        transition_clear = transitions & ~expand(solid, clearance+int(np.ceil(tolerance/cell)))
        protected |= expand(transition_clear, clearance)
    if reduce:
        rects = reduce_rectangles(rects, protected)
    cover = np.zeros_like(solid)
    for x0,z0,x1,z1 in rects:
        cover[x0:x1,z0:z1] = True
    lost_clear = runtime_clear & expand(cover, clearance)
    assert not np.any(relevant & ~cover), 'reduction discarded source solids'
    assert not np.any(protected_clear & expand(cover, clearance)), 'reduction exceeded clearance tolerance'
    boxes = [[bounds[0]+x0*cell, 0, bounds[1]+z0*cell,
              (x1-x0)*cell, 18, (z1-z0)*cell] for x0,z0,x1,z1 in rects]
    # Preserve survey boundary classification (its original circular dilation).
    blocked = solid.copy()
    r = int(np.ceil(report['actor_radius']/cell))
    for dx in range(-r,r+1):
        for dz in range(-r,r+1):
            if np.hypot(dx,dz)*cell > report['actor_radius']:
                continue
            dst=(slice(max(0,dx),min(solid.shape[0],solid.shape[0]+dx)),slice(max(0,dz),min(solid.shape[1],solid.shape[1]+dz)))
            src=(slice(max(0,-dx),min(solid.shape[0],solid.shape[0]-dx)),slice(max(0,-dz),min(solid.shape[1],solid.shape[1]-dz)))
            blocked[dst] |= solid[src]
    boundaries = boundary_points(reached, blocked, ground)
    exits = []
    for kind in ('unsupported_ground','survey_edge'):
        for group in clusters(boundaries[kind]):
            pts = [[bounds[0]+(x+.5)*cell,bounds[1]+(z+.5)*cell] for x,z in group]
            exits.append({'kind':kind,'samples':len(pts),'points':pts,
                          'review_required':True,'action':'survey height or author a real opening transition'})
    return {'format':'recvx-outline-candidate-1','boxes':boxes,'wall_records':len(boxes),
            'exact_wall_records':exact_count,'retail_wall_budget':168,
            'within_wall_budget':len(boxes)<=168,
            'runtime_clear_cells_lost':int(lost_clear.sum()),
            'clearance_tolerance':int(np.ceil(tolerance/cell))*cell,
            'added_collision_cells':int((cover & ~solid).sum()),
            'solid_cells_preserved':int(relevant.sum()),'open_boundaries':exits,
            'publishable':False,
            'limitations':['Candidate only: compiled runtime-clearance flood and camera checks required',
                           'No perimeter walls generated from unsupported-ground or survey boundaries',
                           'Reduction uses strict box clearance at cell centers; survey uses circular clearance',
                           'Tolerance may remove wall-adjacent walking cells; inspect the audit and preview']}


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('survey',type=Path)
    p.add_argument('--output',type=Path)
    p.add_argument('--reduce',action='store_true',help='merge inside actor-inaccessible regions')
    p.add_argument('--tolerance',type=float,default=0,help='additional clearance loss allowed, in game units')
    args=p.parse_args()
    if not np.isfinite(args.tolerance) or args.tolerance<0:
        p.error('tolerance must be finite and nonnegative')
    output=args.output or args.survey/'outline.json'
    if not output.resolve().is_relative_to((Path.cwd()/'scratch').resolve()):
        p.error('output must be inside scratch/')
    report=json.loads((args.survey/'report.json').read_text())
    with np.load(args.survey/'grid.npz') as grid:
        result=candidate(report,grid['solid'],grid['ground'],grid['reachable'],args.reduce,args.tolerance,
                         grid['transitions'] if 'transitions' in grid else None)
    output.parent.mkdir(parents=True,exist_ok=True)
    output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k not in ('boxes','open_boundaries')},indent=2))
    print('Open boundary clusters:',len(result['open_boundaries']))


if __name__=='__main__':
    main()
