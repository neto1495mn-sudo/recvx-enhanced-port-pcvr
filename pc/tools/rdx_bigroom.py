#!/usr/bin/env python3
"""Synthetic stress room for the PC budget (docs/modding/BUILDING_ROOMS.md "Budgets"), no disc data.

    rdx_bigroom.py OUT [--grid 224] [--size 1200] [--textures 300]
    rdxbuild.py build OUT/room.json -o mods/RM_1290.RDX

A size x size floor of grid x grid quads (2 triangles each, one 16x16-quad tile per node), a
pillar every 48 units (a 4x4 wall record and a 10-triangle box each), the four outer walls,
4x4 fixed cameras and entries at both corners. Defaults: 106,602 triangles, 629 walls,
300 textures (44 of them 256x256), 16 cuts. Pillar centres are at 48*i + 24.
"""
import argparse
import copy
import json
from pathlib import Path
import rdx
from rdxbuild import zero


def generate(dest, grid=224, size=1200., textures=300):
    dest.mkdir(parents=True, exist_ok=True)
    lines, groups, nv = ['# synthetic stress room (rdx_bigroom.py)'], [], 0
    lines += ['vt 0 0', 'vt 1 0', 'vt 1 1', 'vt 0 1']
    q = size / grid
    tile = 16
    for ty in range(0, grid, tile):
        for tx in range(0, grid, tile):
            lines += ['o floor_%d_%d' % (tx, ty), 'usemtl tex%d' % (len(groups) % textures)]
            groups.append(1)
            for z in range(ty, min(grid, ty + tile)):
                for x in range(tx, min(grid, tx + tile)):
                    lines += ['v %g 0 %g' % (x*q, z*q), 'v %g 0 %g' % ((x+1)*q, z*q),
                              'v %g 0 %g' % ((x+1)*q, (z+1)*q), 'v %g 0 %g' % (x*q, (z+1)*q)]
                    a = nv + 1
                    lines += ['f %d/1 %d/3 %d/2' % (a, a+2, a+1), 'f %d/1 %d/4 %d/3' % (a, a+3, a+2)]
                    nv += 4
    centres = [48*i + 24 for i in range(int(size // 48))]
    walls = [zero(rdx.ATR, flg=1, px=cx-2., pz=cz-2., w=4., h=20., d=4.) for cz in centres for cx in centres]
    for k in range(0, len(walls), 240):   # 2,400 triangles per node (PC budget: 2,729)
        lines += ['o pillars_%d' % k, 'usemtl tex%d' % (len(groups) % textures)]
        groups.append(1)
        for w in walls[k:k+240]:
            x, z, x1, z1 = w['px'], w['pz'], w['px'] + 4, w['pz'] + 4
            p = [(x, 0, z), (x1, 0, z), (x1, 20, z), (x, 20, z), (x, 0, z1), (x1, 0, z1), (x1, 20, z1), (x, 20, z1)]
            lines += ['v %g %g %g' % v for v in p]
            for a, b, c, d in [(0, 3, 2, 1), (5, 6, 7, 4), (4, 7, 3, 0), (1, 2, 6, 5), (3, 7, 6, 2)]:
                lines += ['f %d/1 %d/4 %d/3' % (nv+a+1, nv+b+1, nv+c+1), 'f %d/1 %d/3 %d/2' % (nv+a+1, nv+c+1, nv+d+1)]
            nv += 8
    (dest/'room.obj').write_text('\n'.join(lines) + '\n')
    walls += [zero(rdx.ATR, flg=1, px=-2., pz=-2., w=2., h=30., d=size+4), zero(rdx.ATR, flg=1, px=size, pz=-2., w=2., h=30., d=size+4),
              zero(rdx.ATR, flg=1, px=0., pz=-2., w=size, h=30., d=2.), zero(rdx.ATR, flg=1, px=0., pz=size, w=size, h=30., d=2.)]
    names = []
    for i in range(textures):
        n = 256 if i < 44 else 64
        a, b = (40 + i*37 % 200, 60 + i*53 % 180, 80 + i*71 % 160), (200 - i*29 % 150, 190 - i*41 % 140, 170 - i*13 % 120)
        pix = bytes(v for y in range(n) for x in range(n) for v in (*((a, b)[((x*8//n) + (y*8//n)) % 2]), 255))
        names.append('t%03d.png' % i)
        rdx.write_png(str(dest/names[-1]), n, n, pix)
    j = {'format': 'recvx-room-1', 'budget': 'pc', 'mesh': 'room.obj', 'textures': names, 'gindex': 0x4D100000,
         'room': {'w': size, 'h': 30., 'd': size, 'amb_r': [.8]*4, 'amb_g': [.8]*4, 'amb_b': [.8]*4},
         'objects': [zero(rdx.ETTY) for _ in range(4)], 'items': [], 'enemies': [], 'triggers': [], 'routes': [],
         'route_next': [], 'walls': walls, 'floors': [zero(rdx.ATR, flg=1, type=1, px=0., pz=0., w=size, d=size)],
         'positions': [zero(rdx.POS, px=12., pz=12., ay=32768), zero(rdx.POS, px=size-12, pz=size-12, ay=0)],
         'scripts': ['0000', '0000'], 'cuts': []}
    c = size / 4
    for k in range(16):
        x0, z0 = k % 4 * c, k // 4 * c
        cam = zero(rdx.CAM, flg=257, px=x0 + c/2, py=90., pz=z0 - 20, ax=9000, ay=32768, pers=11000,
                   h=12., am_spd=.3, aa_spd=30, fog_nr=0., fog_fr=-3000.)
        j['cuts'].append(dict(zero(rdx.CUT, flg=1, cx=x0, cz=z0, cw=c, cd=c), cam=[cam, copy.deepcopy(cam)],
                              areas=[zero(rdx.CTAB, minx=x0 - (k % 4 == 0), minz=z0 - (k < 4), maxx=x0 + c + 1, maxz=z0 + c + 1)],
                              exd='00'*256))
    (dest/'room.mtl').write_text(''.join('newmtl tex%d\nmap_Kd %s\n' % (i, t) for i, t in enumerate(names)))
    (dest/'room.json').write_text(json.dumps(j, indent=1) + '\n')
    return len(groups)


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('out'); p.add_argument('--grid', type=int, default=224)
    p.add_argument('--size', type=float, default=1200.); p.add_argument('--textures', type=int, default=300)
    a = p.parse_args()
    print(generate(Path(a.out), a.grid, a.size, a.textures), 'nodes')
