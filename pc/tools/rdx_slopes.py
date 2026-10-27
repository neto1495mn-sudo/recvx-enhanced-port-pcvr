#!/usr/bin/env python3
"""Slope test room (docs/modding/BUILDING_ROOMS.md "Slopes"), no disc data: the player on type-6 slopes.

    rdx_slopes.py OUT
    rdxbuild.py build OUT/room.json -o mods/RM_1280.RDX

A 160x180 ground at y 0 with a ramp (x 20..60 rising +x to 10 along z 20..40), a plateau at 10
(a flat patch, x 60..80) and a ramp back down (x 80..120); and a 15-unit hill (x 40..120, z 90..170)
from rdxbuild.heightfield_slopes. The visible mesh is the slope records' own surfaces. Entry 0 at
(10, 30) facing +x, entry 1 on the hill top facing -z; two overview cameras (z < 80, z >= 80).
"""
import copy
import json
from pathlib import Path
import sys
import rdx
from rdxbuild import zero, slope_wall, heightfield_slopes, FLAT

W, D = 160., 180.


def corners(s):
    """The slope's surface corners (x, y, z), in the order the box corners go round."""
    x0, z0, x1, z1 = s['px'], s['pz'], s['px'] + s['w'], s['pz'] + s['d']
    lo, hi = s['py'], s['py'] + (0 if s['h'] <= FLAT else s['h'])
    y = {0: (lo, hi, hi, lo), 1: (hi, hi, lo, lo), 2: (lo, lo, hi, hi), 3: (hi, lo, lo, hi)}[s['id']]
    return [(x0, y[0], z0), (x1, y[1], z0), (x1, y[2], z1), (x0, y[3], z1)]


def generate(dest):
    dest.mkdir(parents=True, exist_ok=True)
    slopes = [slope_wall(20, 20, 40, 20, 0, 10, '+x'), slope_wall(60, 20, 20, 20, 10, 10, '+x'),
              slope_wall(80, 20, 40, 20, 0, 10, '-x')]
    hill = [[max(0., 15 * (1 - ((40 + 10 * c - 80) ** 2 + (90 + 10 * r - 130) ** 2) / 35 ** 2)) for c in range(9)] for r in range(9)]
    slopes += heightfield_slopes(hill, 40, 90, 10)
    lines, nv, last = ['# slope test room (rdx_slopes.py)', 'mtllib room.mtl', 'vt 0 0', 'vt 1 0', 'vt 1 1', 'vt 0 1'], 0, None

    def quad(p, group, tex):
        nonlocal nv, last
        if group != last:
            lines.extend(['o ' + group, 'usemtl tex%d' % tex])
            last = group
        lines.extend(['v %g %g %g' % v for v in p] +
                     ['f %d/1 %d/3 %d/2' % (nv + 1, nv + 3, nv + 2), 'f %d/1 %d/4 %d/3' % (nv + 1, nv + 4, nv + 3)])
        nv += 4
    covered = lambda x, z: any(s['px'] <= x < s['px'] + s['w'] and s['pz'] <= z < s['pz'] + s['d'] for s in slopes)
    for k in range(0, int(D), 10):   # ground cells not under a slope, one node per row
        for i in range(0, int(W), 10):
            if not covered(i + 5, k + 5):
                quad([(i, 0, k), (i + 10, 0, k), (i + 10, 0, k + 10), (i, 0, k + 10)], 'ground%d' % k, 0)
    for n, s in enumerate(slopes):
        quad(corners(s), 'slope%d' % (n // 16), 1)
    (dest/'room.obj').write_text('\n'.join(lines) + '\n')
    atr = lambda **kw: zero(rdx.ATR, flg=1, **kw)
    walls = [atr(px=-2., pz=-2., w=2., h=30., d=D + 4), atr(px=W, pz=-2., w=2., h=30., d=D + 4),
             atr(px=0., pz=-2., w=W, h=30., d=2.), atr(px=0., pz=D, w=W, h=30., d=2.)] + slopes
    j = {'format': 'recvx-room-1', 'budget': 'pc', 'mesh': 'room.obj', 'textures': ['ground.png', 'slope.png'],
         'gindex': 0x4D200000, 'room': {'w': W, 'h': 30., 'd': D, 'amb_r': [.8] * 4, 'amb_g': [.8] * 4, 'amb_b': [.8] * 4},
         'objects': [zero(rdx.ETTY) for _ in range(4)], 'items': [], 'enemies': [], 'triggers': [], 'routes': [],
         'route_next': [], 'walls': walls, 'floors': [atr(type=1, px=0., pz=0., w=W, d=D)],
         'positions': [zero(rdx.POS, px=10., pz=30., ay=49152), zero(rdx.POS, px=80., py=14., pz=130., ay=0)], 'scripts': ['0000', '0000'], 'cuts': []}
    for z0, z1 in ((0., 80.), (80., D)):
        cam = zero(rdx.CAM, flg=257, px=W / 2, py=90., pz=z0 - 40, ax=8000, ay=32768, pers=12000,
                   h=12., am_spd=.3, aa_spd=30, fog_nr=0., fog_fr=-3000.)
        j['cuts'].append(dict(zero(rdx.CUT, flg=1, cx=0., cz=z0, cw=W, cd=z1 - z0), cam=[cam, copy.deepcopy(cam)],
                              areas=[zero(rdx.CTAB, minx=-1., minz=z0 - (z0 == 0), maxx=W + 1, maxz=z1 + (z1 == D))], exd='00' * 256))
    for name, (a, b) in (('ground.png', ((60, 90, 60), (90, 120, 80))), ('slope.png', ((170, 120, 60), (120, 80, 40)))):
        rdx.write_png(str(dest/name), 64, 64, bytes(v for y in range(64) for x in range(64) for v in (*((a, b)[(x // 8 + y // 8) % 2]), 255)))
    (dest/'room.mtl').write_text('newmtl tex0\nmap_Kd ground.png\nnewmtl tex1\nmap_Kd slope.png\n')
    (dest/'room.json').write_text(json.dumps(j, indent=1) + '\n')
    return slopes


if __name__ == '__main__':
    print(len(generate(Path(sys.argv[1]))), 'slopes')
