#!/usr/bin/env python3
"""Measure an external OBJ without copying geometry into tracked files.

Reports and generated mapping drafts must stay under this clone's scratch/.
Y is provisionally up; units are deliberately not converted. No third-party packages.
"""
import argparse
from collections import defaultdict
import json
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def scratch_path(value):
    path = Path(value).resolve()
    if not path.is_relative_to(ROOT / 'scratch'):
        raise ValueError('generated output must be inside this clone\'s scratch/')
    path.parent.mkdir(parents=True, exist_ok=True)
    return path


def read_obj(path):
    vertices, faces = [], []
    group, material = 'default', 'default'
    with open(path) as stream:
        for line in stream:
            parts = line.split('#', 1)[0].split() if not line.startswith('g ') else line.split()
            if not parts:
                continue
            if parts[0] == 'v':
                vertices.append(tuple(map(float, parts[1:4])))
            elif parts[0] in ('g', 'o'):
                group = ' '.join(parts[1:]) or 'default'
            elif parts[0] == 'usemtl':
                material = ' '.join(parts[1:])
            elif parts[0] == 'f':
                indices = [int(p.split('/')[0]) for p in parts[1:]]
                if any(i == 0 or not -len(vertices) <= i <= len(vertices) for i in indices):
                    raise ValueError('invalid OBJ vertex index')
                points = [vertices[i-1 if i > 0 else len(vertices)+i] for i in indices]
                for i in range(1, len(points)-1):
                    faces.append((group, material, (points[0], points[i], points[i+1])))
    return faces


def opacity_materials(path):
    result, current = set(), None
    if path is None:
        return result
    for line in Path(path).read_text().splitlines():
        p = line.split()
        if not p:
            continue
        if p[0] == 'newmtl':
            current = ' '.join(p[1:])
        elif p[0] == 'map_d' or (p[0] == 'd' and float(p[-1]) < 1) or (p[0] == 'Tr' and float(p[-1]) > 0):
            result.add(current)
    return result


def weighted_quantile(samples, fraction):
    target = sum(weight for _, weight in samples) * fraction
    cumulative = 0
    for value, weight in sorted(samples):
        cumulative += weight
        if cumulative >= target:
            return value
    return samples[-1][0]


def measure(faces, grid_size):
    bounds, rows, floors = {}, [], defaultdict(list)
    degenerate = 0
    for group, material, points in faces:
        low, high = bounds.setdefault(group, ([math.inf]*3, [-math.inf]*3))
        for p in points:
            for axis in range(3):
                low[axis], high[axis] = min(low[axis], p[axis]), max(high[axis], p[axis])
        a, b, c = points
        u, v = [b[i]-a[i] for i in range(3)], [c[i]-a[i] for i in range(3)]
        n = (u[1]*v[2]-u[2]*v[1], u[2]*v[0]-u[0]*v[2], u[0]*v[1]-u[1]*v[0])
        length = math.sqrt(sum(x*x for x in n))
        if length <= 1e-12:
            degenerate += 1
            continue
        area = length / 2
        center = tuple(sum(p[i] for p in points)/3 for i in range(3))
        ny = abs(n[1])/length
        orientation = 'horizontal' if ny >= .9 else 'vertical' if ny <= .2 else 'sloped'
        cell = (math.floor(center[0]/grid_size), math.floor(center[2]/grid_size))
        if orientation == 'horizontal':
            floors[cell].append((center[1], area))
        rows.append((group, material, area, center, orientation, cell))
    # Lower horizontal surfaces are only a floor hypothesis; roofs/undersides can fool it.
    anchors = {cell: weighted_quantile(samples, .1) for cell, samples in floors.items()}
    grouped = defaultdict(list)
    for group, material, area, center, orientation, cell in rows:
        low, high = bounds[group]
        span = high[1]-low[1]
        relative = (center[1]-low[1])/span if span else 0
        floor = anchors.get(cell)
        near = floor is not None and abs(center[1]-floor) <= grid_size*.1
        role = ('ground' if orientation == 'horizontal' and near else
                'wall' if orientation == 'vertical' and span > 0 else
                'roof' if orientation == 'sloped' and relative >= .65 else 'unknown')
        grouped[group, material].append((area, center[1], orientation, relative,
                                         None if floor is None else center[1]-floor, role))
    def summarize(values):
        total = sum(r[0] for r in values)
        def shares(index, names):
            return {name: sum(r[0] for r in values if r[index] == name)/total for name in names}
        local = [(r[4], r[0]) for r in values if r[4] is not None]
        return {'triangles': len(values), 'area': total,
                'orientation_area_fraction': shares(2, ('horizontal', 'vertical', 'sloped')),
                'role_area_fraction': shares(5, ('ground', 'wall', 'roof', 'unknown')),
                'height_quantiles': [weighted_quantile([(r[1], r[0]) for r in values], q) for q in (.1, .5, .9)],
                'group_relative_height_mean': sum(r[0]*r[3] for r in values)/total,
                'local_horizontal_anchor_coverage': sum(w for _, w in local)/total,
                'local_height_quantiles': [weighted_quantile(local, q) for q in (.1, .5, .9)] if local else None}
    groups = []
    materials = defaultdict(list)
    for (group, material), values in sorted(grouped.items()):
        groups.append(dict(group=group, material=material, bounds=bounds[group], **summarize(values)))
        materials[material].extend(values)
    return {'axis_assumption': 'Y up; unverified', 'scale': None, 'grid_size_source_units': grid_size,
            'degenerate_triangles': degenerate, 'groups': groups,
            'materials': {name: summarize(values) for name, values in sorted(materials.items())}}


def draft_mapping(report, opacity):
    classes = {
        'ground': {'room': '5810', 'list': 0, 'texture': 20},
        'wood': {'room': '0120', 'list': 0, 'texture': 26},
        'stone': {'room': '7150', 'list': 0, 'texture': 29},
        'plaster': None, 'roof': None, 'foliage': None, 'water': None, 'metal': None,
        'unknown': None,
    }
    materials = {}
    for name, stats in report['materials'].items():
        shares = stats['role_area_fraction']
        role = max(shares, key=shares.get)
        mixed = shares[role] < .7
        candidate = 'unknown' if mixed or role == 'unknown' else 'stone' if role == 'wall' else role
        reasons = ['axis_scale_and_local_floor_unverified', 'visual_confirmation_required']
        if mixed:
            reasons.append('mixed_surface_roles_use_group_overrides')
        if role == 'wall':
            reasons.append('wall_shell_or_prop_ambiguous_stone_wood_plaster_unresolved')
        if name in opacity:
            reasons.append('opacity_map_or_transparency_requires_review')
            candidate = 'unknown'
        if classes[candidate] is None:
            reasons.append('retail_donor_unresolved')
        materials[name] = {'class': candidate, 'retail_texture': classes[candidate],
                           'review_required': True, 'review_reasons': reasons}
    return {'format': 'recvx-material-map-1',
            'description': 'Provisional geometry-based classes. All entries need visual review. Null donors block conversion; never silently substitute.',
            'texture_reference_fields': 'room, list, texture (zero-based)',
            'classes': classes, 'materials': materials, 'group_overrides': {}}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('obj', type=Path)
    p.add_argument('--mtl', type=Path)
    p.add_argument('--grid-size', type=float, required=True, help='local floor sampling cell in source units; not a scale assertion')
    p.add_argument('--report', default='scratch/re4/surfaces.json')
    p.add_argument('--mapping', default='scratch/re4/material_mapping.draft.json')
    args = p.parse_args()
    if not math.isfinite(args.grid_size) or args.grid_size <= 0:
        p.error('--grid-size must be finite and positive')
    report_path, mapping_path = scratch_path(args.report), scratch_path(args.mapping)
    if report_path == mapping_path or any(x in (args.obj.resolve(), args.mtl.resolve() if args.mtl else None) for x in (report_path, mapping_path)):
        p.error('outputs must differ from each other and inputs')
    report = measure(read_obj(args.obj), args.grid_size)
    mapping = draft_mapping(report, opacity_materials(args.mtl))
    report_path.write_text(json.dumps(report, indent=2)+'\n')
    mapping_path.write_text(json.dumps(mapping, indent=2)+'\n')
    print(f'Wrote {report_path} and {mapping_path}; all classifications require review.')


if __name__ == '__main__':
    main()
