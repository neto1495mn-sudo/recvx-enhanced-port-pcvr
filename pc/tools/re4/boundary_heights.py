"""Survey source support beyond outline openings; never infer a wall from missing data."""
import argparse
import json
from pathlib import Path

import numpy as np

from navigation import read_obj, raster


def height_field(faces, mapping, transform, bounds, cell, shape):
    """Nearest-to-zero opaque surface, plus reviewed terrain, at cell centers.

    Separate fields prevent a roof or unknown prop from certifying walking support.
    Missing samples remain NaN, not zero-height floor or evidence of a real drop.
    """
    opaque = np.full(shape, np.nan)
    terrain = np.full(shape, np.nan)
    for group, material, vertices in faces:
        info = dict(mapping['materials'].get(material, {}))
        overrides = mapping.get('group_overrides', {}).get(group, {})
        for key in ('*', material):
            info.update(overrides.get(key, {}))
        if (info.get('exclude') or material in mapping.get('navigation_excluded_materials', [])
                or info.get('class') in ('opacity', 'foliage')):
            continue
        ps = (np.asarray(vertices)-transform['source_origin'])*transform['scale']
        if transform.get('reflect_z'):
            ps[:, 2] *= -1
        normal = np.cross(ps[1]-ps[0], ps[2]-ps[0])
        length = np.linalg.norm(normal)
        if length < 1e-9 or abs(normal[1])/length <= .2:
            continue
        ix, iz = raster(ps.tolist(), bounds, cell, shape, centers=True)
        x, z = bounds[0]+(ix+.5)*cell, bounds[1]+(iz+.5)*cell
        y = ps[0,1]-(normal[0]*(x-ps[0,0])+normal[2]*(z-ps[0,2]))/normal[1]
        fields = [opaque, terrain] if info.get('class') == 'ground' else [opaque]
        for field in fields:
            old = field[ix, iz]
            keep = ~np.isfinite(old) | (abs(y) < abs(old))
            field[ix[keep], iz[keep]] = y[keep]
    return opaque, terrain


def summarize(values):
    valid = values[np.isfinite(values)]
    return dict(samples=int(values.size), supported=int(valid.size),
                height_range=[float(valid.min()), float(valid.max())] if valid.size else None)


def inspect_boundaries(audit, report, ground, blocked, opaque, terrain, distance):
    result = []
    bounds, cell = report['bounds'], report['cell_size']
    for index, boundary in enumerate(audit['open_boundaries']):
        outside = set()
        for x, z in boundary['points']:
            ix, iz = int(round((x-bounds[0])/cell-.5)), int(round((z-bounds[1])/cell-.5))
            for dx, dz in ((-1,0),(1,0),(0,-1),(0,1)):
                nx, nz = ix+dx, iz+dz
                if not (0 <= nx < ground.shape[0] and 0 <= nz < ground.shape[1]):
                    continue
                if ground[nx,nz] or blocked[nx,nz]:
                    continue
                for step in range(1, distance+1):
                    nx, nz = ix+dx*step, iz+dz*step
                    if not (0 <= nx < ground.shape[0] and 0 <= nz < ground.shape[1]) or blocked[nx,nz]:
                        break
                    outside.add((nx,nz))
        cells = sorted(outside)
        indices = tuple(np.array(cells, dtype=int).T) if cells else (np.array([],int),np.array([],int))
        th, oh = terrain[indices], opaque[indices]
        # This is an evidence classification, deliberately not a closure decision.
        if not cells:
            proposal = 'manual_review_no_outward_samples'
        elif np.isfinite(th).all():
            proposal = 'terrain_transition_review'
        elif np.isfinite(oh).all():
            proposal = 'surface_material_review'
        else:
            proposal = 'mixed_or_missing_support_review'
        result.append(dict(index=index, kind=boundary['kind'], boundary_samples=boundary['samples'],
                           proposal=proposal, reviewed_terrain=summarize(th), opaque_surface=summarize(oh),
                           samples=[dict(x=bounds[0]+(ix+.5)*cell,z=bounds[1]+(iz+.5)*cell,
                                         terrain_y=float(terrain[ix,iz]) if np.isfinite(terrain[ix,iz]) else None,
                                         opaque_y=float(opaque[ix,iz]) if np.isfinite(opaque[ix,iz]) else None)
                                    for ix,iz in cells], closed=False))
    return dict(boundaries=result, closed_boundaries=0, publishable=False,
                limitations=['Center samples are not actor-footprint support',
                             'Closest-to-zero surface is evidence, not a chosen walking layer',
                             'Missing mesh does not prove a drop; no automatic closing walls',
                             'Type-6 slope boxes consume the same wall budget as solid boxes'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('obj', type=Path)
    parser.add_argument('survey', type=Path)
    parser.add_argument('outline', type=Path)
    parser.add_argument('--mapping', type=Path, required=True)
    parser.add_argument('--transform', type=Path, required=True)
    parser.add_argument('--group-overrides', type=Path)
    parser.add_argument('--distance', type=int, default=12, help='outward probe distance in cells')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if not args.output.resolve().is_relative_to((Path.cwd()/'scratch').resolve()):
        parser.error('output must be inside scratch/')
    if args.distance < 1:
        parser.error('distance must be positive')
    from validate_outline import audit
    report = json.loads((args.survey/'report.json').read_text())
    mapping = json.loads(args.mapping.read_text())
    if args.group_overrides:
        mapping['group_overrides'] = json.loads(args.group_overrides.read_text())
    with np.load(args.survey/'grid.npz') as grid:
        ground = grid['ground']
        check, _, _, blocked = audit(report, grid['solid'], ground, json.loads(args.outline.read_text()))
    opaque, terrain = height_field(read_obj(args.obj), mapping, json.loads(args.transform.read_text()),
                                   report['bounds'], report['cell_size'], ground.shape)
    result = inspect_boundaries(check, report, ground, blocked, opaque, terrain, args.distance)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2, allow_nan=False)+'\n')
    np.savez_compressed(args.output.with_suffix('.npz'), opaque=opaque, terrain=terrain)
    for b in result['boundaries']:
        print(b['index'], b['proposal'], b['reviewed_terrain'], b['opaque_surface'])


if __name__ == '__main__':
    main()
