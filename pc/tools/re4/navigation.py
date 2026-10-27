"""Measure flat-ground navigation from an external OBJ; write evidence only to scratch.

Solid cells conservatively intersect opaque polygons clipped to the actor-height band.
This is a survey, not a collider author: open exits require reviewed room transitions.
"""
import argparse
import collections
import json
import math
from pathlib import Path
import sys

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from analyze_obj_surfaces import read_obj


def clip_height(poly, height, above):
    result = []
    for p, q in zip(poly, poly[1:] + poly[:1]):
        pin = (p[1] >= height) if above else (p[1] <= height)
        qin = (q[1] >= height) if above else (q[1] <= height)
        if pin:
            result.append(p)
        if pin != qin:
            t = (height - p[1]) / (q[1] - p[1])
            result.append(tuple(p[k] + t * (q[k] - p[k]) for k in range(3)))
    return result


def raster(poly, bounds, cell, shape, centers=False):
    """Convex polygon/cell SAT, including edge-on wall segments; or ground centers."""
    points = np.asarray([(p[0], p[2]) for p in poly])
    low = np.floor((points.min(axis=0) - bounds[:2]) / cell).astype(int)
    high = np.floor((points.max(axis=0) - bounds[:2]) / cell).astype(int)
    low = np.maximum(low, 0)
    high = np.minimum(high, np.array(shape) - 1)
    if np.any(low > high):
        return np.array([], dtype=int), np.array([], dtype=int)
    xx, zz = np.meshgrid(np.arange(low[0], high[0] + 1),
                         np.arange(low[1], high[1] + 1), indexing='ij')
    x = bounds[0] + (xx + .5) * cell
    z = bounds[1] + (zz + .5) * cell
    keep = np.ones(xx.shape, dtype=bool)
    axes = [(1., 0.), (0., 1.)]
    axes += [(-(q-p)[1], (q-p)[0]) for p, q in zip(points, np.roll(points, -1, axis=0))]
    for ax, az in axes:
        if abs(ax) + abs(az) < 1e-10:
            continue
        projection = points[:, 0] * ax + points[:, 1] * az
        radius = 0 if centers else .5 * cell * (abs(ax) + abs(az))
        sample = x * ax + z * az
        keep &= (sample + radius >= projection.min()-1e-8) & (sample-radius <= projection.max()+1e-8)
    return xx[keep], zz[keep]


def survey(faces, mapping, transform, bounds, cell, entry, radius, transitions=None):
    shape = tuple(math.ceil((bounds[k+2]-bounds[k])/cell) for k in range(2))
    solid = np.zeros(shape, dtype=bool)
    ground = np.zeros(shape, dtype=bool)
    counts = collections.Counter()
    transition_counts = collections.Counter()
    if transitions is None:
        transitions = np.zeros(shape, dtype=bool)
    for group, material, vertices in faces:
        info = dict(mapping['materials'].get(material, {}))
        overrides = mapping.get('group_overrides', {}).get(group, {})
        for key in ('*', material):
            info.update(overrides.get(key, {}))
        if info.get('exclude', False):
            continue
        if material in mapping.get('navigation_excluded_materials', []) or info.get('class') in ('opacity', 'foliage'):
            continue
        ps = [tuple((p[k]-transform['source_origin'][k])*transform['scale'] for k in range(3)) for p in vertices]
        if transform.get('reflect_z'):
            ps = [(x,y,-z) for x,y,z in ps]
        if all(abs(p[1]) < 1e-4 for p in ps):
            ground[raster(ps, bounds, cell, shape, centers=True)] = True
        # Reviewed terrain with a nonvertical normal is a floor-height transition,
        # never a tall wall. Keep its location for later stair/step authoring.
        normal = np.cross(np.subtract(ps[1], ps[0]), np.subtract(ps[2], ps[0]))
        length = np.linalg.norm(normal)
        if info.get('class') == 'ground' and length > 1e-9 and abs(normal[1])/length > .2:
            if any(abs(p[1]) > 1e-4 for p in ps):
                terrain = clip_height(clip_height(ps, -.5, True), 17.5, False)
                if terrain:
                    cells = raster(terrain, bounds, cell, shape)
                    transitions[cells] = True
                    transition_counts[material] += 1
            continue
        poly = clip_height(clip_height(ps, .5, True), 17.5, False)
        if poly:
            cells = raster(poly, bounds, cell, shape)
            solid[cells] = True
            counts[material] += len(cells[0])
    blocked = solid.copy()
    padding = math.ceil(radius / cell)
    for dx in range(-padding, padding+1):
        for dz in range(-padding, padding+1):
            if math.hypot(dx, dz)*cell > radius:
                continue
            dst = (slice(max(0,dx),min(shape[0],shape[0]+dx)), slice(max(0,dz),min(shape[1],shape[1]+dz)))
            src = (slice(max(0,-dx),min(shape[0],shape[0]-dx)), slice(max(0,-dz),min(shape[1],shape[1]-dz)))
            blocked[dst] |= solid[src]
    allowed = ground & ~blocked
    start = tuple(math.floor((entry[k]-bounds[k])/cell) for k in range(2))
    if not all(0 <= start[k] < shape[k] for k in range(2)) or not allowed[start]:
        raise ValueError('entry has no clear supported cell')
    reached = np.zeros(shape, dtype=bool)
    reached[start] = True
    queue = collections.deque([start])
    boundary = collections.Counter()
    while queue:
        x,z = queue.popleft()
        for nx,nz in ((x-1,z),(x+1,z),(x,z-1),(x,z+1)):
            if not (0 <= nx < shape[0] and 0 <= nz < shape[1]):
                boundary['survey_edge'] += 1
            elif allowed[nx,nz]:
                if not reached[nx,nz]:
                    reached[nx,nz] = True
                    queue.append((nx,nz))
            elif blocked[nx,nz]:
                boundary['solid'] += 1
            else:
                boundary['unsupported_ground'] += 1
    report = dict(cell_size=cell, bounds=bounds, actor_radius=radius, entry=entry,
                  solid_cells=int(solid.sum()), supported_cells=int(ground.sum()),
                  reachable_cells=int(reached.sum()), boundary_edges=dict(boundary),
                  solid_material_cell_hits=dict(counts),
                  terrain_transition_cells=int(transitions.sum()),
                  terrain_transition_triangles=dict(transition_counts),
                  ready_for_colliders=not (boundary['survey_edge'] or boundary['unsupported_ground']),
                  limitations=['Unknown opaque materials treated as solid',
                               'Flat floor only; ground sampled at cell centers',
                               'Open boundaries need surveyed door/path transitions, never automatic walls'])
    return report, solid, ground, reached


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('obj', type=Path)
    p.add_argument('--mapping', type=Path, required=True)
    p.add_argument('--transform', type=Path, required=True)
    p.add_argument('--bounds', type=float, nargs=4, required=True, metavar=('X0','Z0','X1','Z1'))
    p.add_argument('--entry', type=float, nargs=2, required=True)
    p.add_argument('--cell', type=float, default=1.)
    p.add_argument('--radius', type=float, default=5.5)
    p.add_argument('--output', type=Path, default=Path('scratch/re4/navigation'))
    args = p.parse_args()
    if not args.output.resolve().is_relative_to((Path.cwd()/'scratch').resolve()):
        p.error('output must be inside scratch/')
    if args.cell <= 0 or args.radius < 0 or any(args.bounds[k] >= args.bounds[k+2] for k in range(2)):
        p.error('invalid cell, radius or bounds')
    transitions = np.zeros(tuple(math.ceil((args.bounds[k+2]-args.bounds[k])/args.cell) for k in range(2)), dtype=bool)
    report, solid, ground, reached = survey(read_obj(args.obj), json.loads(args.mapping.read_text()),
        json.loads(args.transform.read_text()), args.bounds, args.cell, args.entry, args.radius, transitions)
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    np.savez_compressed(args.output/'grid.npz', solid=solid, ground=ground, reachable=reached, transitions=transitions)
    from PIL import Image
    rgb = np.full((*solid.shape,3), (35,35,40), dtype=np.uint8)
    rgb[ground] = (115,105,80)
    rgb[reached] = (90,175,105)
    rgb[solid] = (220,190,165)
    rgb[transitions & ~solid] = (200,120,200)
    Image.fromarray(rgb.transpose(1,0,2)).save(args.output/'overview.png')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
