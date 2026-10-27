"""Measure axis-aligned slope fit feasibility; emit review candidates, never room records."""
import argparse
import json
from pathlib import Path

import numpy as np


def fit_axis_slope(points):
    """Least-squares fit y=a*x+b or y=a*z+b; choose lower maximum error.

    These are source fit diagnostics, not engine movement simulation. Type-6
    collision projects in both horizontal and vertical directions, and grand[]
    can clamp height. A good fit alone is insufficient for publication.
    """
    p = np.asarray(points, dtype=float)
    if p.ndim != 2 or p.shape[1] != 3 or len(p) < 2 or not np.isfinite(p).all():
        raise ValueError('need at least two finite x/y/z samples')
    candidates = []
    for axis in (0,2):
        coordinate = p[:,axis]
        origin = float(coordinate.mean())
        a, b = np.linalg.lstsq(np.column_stack((coordinate-origin, np.ones(len(p)))),p[:,1],rcond=None)[0]
        errors = abs(a*(coordinate-origin)+b-p[:,1])
        candidates.append(dict(axis='x' if axis == 0 else 'z', gradient=float(a),
                               coordinate_origin=origin, height_at_origin=float(b),
                               maximum_error=float(errors.max()), rms_error=float(np.sqrt(np.mean(errors**2))),
                               direction_id=(0 if a >= 0 else 3) if axis == 0 else (2 if a >= 0 else 1)))
    return min(candidates, key=lambda c:c['maximum_error'])


def assess(evidence, tolerance):
    rows = []
    for boundary in evidence['boundaries']:
        samples = boundary['samples']
        points = [[s['x'],s['terrain_y'],s['z']] for s in samples if s['terrain_y'] is not None]
        fit = fit_axis_slope(points) if len(points) >= 2 else None
        complete = len(points) == len(samples) and bool(points)
        rows.append(dict(index=boundary['index'], terrain_complete=complete, fit=fit,
                         single_slope_height_fit=bool(complete and fit and fit['maximum_error'] <= tolerance),
                         single_slope_fit_rejected=bool(complete and fit and fit['maximum_error'] > tolerance)))
    return dict(tolerance=tolerance, boundaries=rows, publishable=False,
                limitations=['Fits outward probes only, not full rectangular slope footprints',
                             'Least-squares rejection is not proof no minimax fit can pass',
                             'Fit tolerance is a review threshold, not an accepted runtime tolerance',
                             'Multiple slopes need seam, floor-number and projection simulation',
                             'A successful fit neither closes a boundary nor establishes a safe landing'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('evidence',type=Path)
    parser.add_argument('--tolerance',type=float,default=.25)
    parser.add_argument('--output',type=Path,required=True)
    args = parser.parse_args()
    if not args.output.resolve().is_relative_to((Path.cwd()/'scratch').resolve()):
        parser.error('output must be inside scratch/')
    if not np.isfinite(args.tolerance) or args.tolerance < 0:
        parser.error('tolerance must be finite and nonnegative')
    result = assess(json.loads(args.evidence.read_text()), args.tolerance)
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(result,indent=2,allow_nan=False)+'\n')
    print(json.dumps(result,indent=2))


if __name__ == '__main__':
    main()
