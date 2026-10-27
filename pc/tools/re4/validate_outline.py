"""Audit a candidate outline against box clearance and source support; never publish it."""
import argparse
from collections import deque
import json
import math
from pathlib import Path

import numpy as np

from outline import boundary_points, clusters, expand


def flood(allowed, start):
    reached = np.zeros_like(allowed)
    if not all(0 <= start[k] < allowed.shape[k] for k in range(2)) or not allowed[start]:
        return reached
    reached[start] = True
    queue = deque([start])
    while queue:
        x, z = queue.popleft()
        for nx, nz in ((x-1,z), (x+1,z), (x,z-1), (x,z+1)):
            if (0 <= nx < allowed.shape[0] and 0 <= nz < allowed.shape[1]
                    and allowed[nx,nz] and not reached[nx,nz]):
                reached[nx,nz] = True
                queue.append((nx,nz))
    return reached


def blocked_boxes(boxes, bounds, cell, shape, radius):
    """Exact strict box inequalities at survey cell centers, expanded once.

    Matches village_validate's box rule at these sample positions. It is not a
    substitute for in-game collision or integer-grid publication checks.
    """
    blocked = np.zeros(shape, bool)
    for x,y,z,w,h,d in boxes:
        if y > 17.5 or y+h <= 0:
            continue
        lo = [(x-radius-bounds[0])/cell-.5, (z-radius-bounds[1])/cell-.5]
        hi = [(x+w+radius-bounds[0])/cell-.5, (z+d+radius-bounds[1])/cell-.5]
        a = [max(0, math.floor(v)+1) for v in lo]
        b = [min(shape[k], math.ceil(hi[k])) for k in range(2)]
        if a[0] < b[0] and a[1] < b[1]:
            blocked[a[0]:b[0],a[1]:b[1]] = True
    return blocked


def audit(report, solid, ground, outline):
    cell, radius, bounds = report['cell_size'], report['actor_radius'], report['bounds']
    start = tuple(math.floor((report['entry'][k]-bounds[k])/cell) for k in range(2))
    # Solid cells are unit boxes; strict inequalities exclude equality at the edge.
    padding = max(0, math.ceil(radius/cell+.5)-1)
    baseline_blocked = expand(solid, padding)
    baseline = flood(ground & ~baseline_blocked, start)
    blocked = blocked_boxes(outline['boxes'], bounds, cell, ground.shape, radius)
    reached = flood(ground & ~blocked, start)
    boundaries = boundary_points(reached, blocked, ground)
    disconnected = baseline & ~blocked & ~reached
    exits = []
    for kind in ('unsupported_ground', 'survey_edge'):
        for group in clusters(boundaries[kind]):
            exits.append(dict(kind=kind, samples=len(group), points=[
                [bounds[0]+(x+.5)*cell, bounds[1]+(z+.5)*cell] for x,z in group]))
    result = dict(wall_records=len(outline['boxes']), within_wall_budget=len(outline['boxes']) <= 168,
                  entry_clear=bool(reached.any()), baseline_reachable_cells=int(baseline.sum()),
                  candidate_reachable_cells=int(reached.sum()),
                  baseline_cells_blocked=int((baseline & blocked).sum()),
                  surviving_baseline_cells_disconnected=int(disconnected.sum()),
                  open_boundary_clusters=len(exits), open_boundaries=exits,
                  publishable=False,
                  limitations=['Cell-center box model; compiled integer-grid checks still required',
                               'Ground masks limit this audit, but do not limit engine movement',
                               'Open boundaries require real floor/room transitions and camera coverage'])
    return result, baseline, reached, blocked


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('survey', type=Path)
    p.add_argument('outline', type=Path)
    p.add_argument('--output', type=Path, required=True)
    args = p.parse_args()
    if not args.output.resolve().is_relative_to((Path.cwd()/'scratch').resolve()):
        p.error('output must be inside scratch/')
    report = json.loads((args.survey/'report.json').read_text())
    with np.load(args.survey/'grid.npz') as grid:
        result, baseline, reached, blocked = audit(report, grid['solid'], grid['ground'],
                                                  json.loads(args.outline.read_text()))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2)+'\n')
    from PIL import Image
    rgb = np.full((*baseline.shape,3), (35,35,40), dtype=np.uint8)
    rgb[blocked] = (180,150,120)
    rgb[baseline & ~reached] = (235,70,70)
    rgb[reached] = (90,175,105)
    Image.fromarray(rgb.transpose(1,0,2)).save(args.output.with_suffix('.png'))
    print(json.dumps({k:v for k,v in result.items() if k != 'open_boundaries'}, indent=2))


if __name__ == '__main__':
    main()
