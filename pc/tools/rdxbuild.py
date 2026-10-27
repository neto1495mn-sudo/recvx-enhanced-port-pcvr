#!/usr/bin/env python3
"""RDX authoring compiler. JSON records + OBJ/PNG, with lossless retail sidecars.

No game data is bundled. The rdx_demo.py source resolves its zombie dependency from the player's disc.
See docs/modding/BUILDING_ROOMS.md for coordinates, budgets, commands and limitations.
"""
import argparse
import copy
import json
import math
from pathlib import Path
import statistics
import struct
from collections import Counter
import rdx

# Authoring budgets (docs/modding/BUILDING_ROOMS.md "Budgets", docs/modding/ROOM_FORMAT.md "Limits and budgets"). "retail" (the default) keeps a
# room within the PS2's memory, as the retail rooms are; "pc" (room.json "budget": "pc") uses the
# PC build's raised limits (pc_prelude.h): such a room needs that build.
BUDGETS = {
    'retail': dict(expanded=9 << 20, loaded=None, textures=192, texture_bytes=6 << 20,
                   group_triangles=666, groups=512),
    # loaded: packed + expanded file, both in the room arena while it loads (~74.6 MB free on
    # arrival). group_triangles: one vertex chunk per node, at most 8189 vertices (u16 size in
    # words) and the 8192-entry vertex work buffer. Nodes 512+ cannot be hidden by camera masks.
    'pc': dict(expanded=44 << 20, loaded=64 << 20, textures=768, texture_bytes=40 << 20,
               group_triangles=2729, groups=65535),
}
# Engine limits for both budgets (not raised on the PC): runtime arrays and byte-wide indices in
# the save, scripts and records. Walls: BH_PWORK.psh_idx is a short; scripts address 0..255.
RECORD_LIMITS = {'enemies': 128, 'objects': 32, 'items': 32, 'triggers': 256, 'floors': 256,
                 'positions': 128, 'routes': 255, 'effects': 256, 'lights': 256,
                 'event_lights': 128, 'walls': 32767}


# ROM_WORK.flg bit the PC build reads (pc_prelude.h PC_ROOM_SLOPES): the player stands on type-6
# slope walls. Set in every "budget": "pc" room; retail rooms use bits 0..1 only.
ROOM_SLOPES = 0x100
# Type-6 slope walls (docs/modding/BUILDING_ROOMS.md "Slopes"): wall `id` = the direction the surface rises.
SLOPE_RISE = {'+x': 0, '-z': 1, '+z': 2, '-x': 3}
FLAT = 0.001   # h of a flat patch (h 0 would mean the room's default wall height)


def slope_wall(x, z, w, d, low, high, rise, flr_no=0):
    """Type-6 record: box x..x+w, z..z+d whose surface rises from `low` to `high` toward `rise`."""
    if w <= 0 or d <= 0 or high < low:
        raise ValueError('slope needs w, d > 0 and high >= low')
    return zero(rdx.ATR, flg=1, type=6, id=SLOPE_RISE[rise], flr_no=flr_no, px=float(x), py=float(low),
                pz=float(z), w=float(w), h=float(max(high - low, FLAT)), d=float(d))


def slope_height(rec, x, z):
    """Surface y of a slope record at x/z as the PC engine computes it (hitchk.c pc_slope_ground);
    None outside its box (min edges inside, max edges outside)."""
    u, v = x - rec['px'], z - rec['pz']
    if not (0 <= u < rec['w'] and 0 <= v < rec['d']):
        return None
    s = (u / rec['w'], 1 - v / rec['d'], v / rec['d'], 1 - u / rec['w'])[rec['id']]
    return rec['py'] + s * rec['h']


def heightfield_slopes(heights, x0, z0, cell, flr_no=0):
    """One slope per grid cell from corner heights heights[row z][col x] (cell units apart, the
    corner [0][0] at x0/z0): the cell's mean gradient along its steeper axis, its mean height
    across the other. Neighbours meet without a step where the terrain does not twist."""
    out = []
    for r in range(len(heights) - 1):
        for c in range(len(heights[0]) - 1):
            a, b, e, f = heights[r][c], heights[r][c + 1], heights[r + 1][c], heights[r + 1][c + 1]
            gx, gz = (b - a + f - e) / 2, (e - a + f - b) / 2
            if abs(gx) >= abs(gz):
                lo, rise = ((a + e) / 2, '+x') if gx >= 0 else ((b + f) / 2, '-x')
                g = abs(gx)
            else:
                lo, rise = ((a + b) / 2, '+z') if gz >= 0 else ((e + f) / 2, '-z')
                g = abs(gz)
            out.append(slope_wall(x0 + c * cell, z0 + r * cell, cell, cell, lo, lo + g, rise, flr_no))
    return out


def budget_of(name):
    if name not in BUDGETS:
        raise ValueError('budget must be one of ' + ', '.join(BUDGETS))
    return BUDGETS[name]


def json_safe(value):
    if isinstance(value,float) and not math.isfinite(value):
        return {'$float': str(value)}
    if isinstance(value,dict): return {k:json_safe(v) for k,v in value.items()}
    if isinstance(value,list): return [json_safe(v) for v in value]
    return value


def json_hook(value):
    if set(value)=={'$float'}:
        if value['$float'] not in ('nan','inf','-inf'): raise ValueError('invalid special float')
        return float(value['$float'])
    return value


def zero(st, **values):
    r = rdx.unpack(st, bytes(st[0].size), 0)
    r.update(values)
    return r


def encoded(st, rec):
    b = bytearray(st[0].size)
    rdx.pack_into(st, b, 0, rec)
    return b


def append(out, data, alignment=16):
    out.extend(bytes((-len(out)) % alignment))
    off = len(out)
    out.extend(data)
    return off


def table_blob(entries, counted=False, alignment=16):
    b = bytearray(4 * (len(entries) + int(counted)))
    if counted:
        struct.pack_into('<I', b, 0, len(entries))
    for i, entry in enumerate(entries):
        if entry:
            off = append(b, entry, alignment)
            struct.pack_into('<I', b, 4 * (i + int(counted)), off)
    return b


def split_table(room, kind):
    """Keep shared entries shared in the preserved-section path; asset extraction copies blobs."""
    idx = {'model': 1, 'motion': 2, 'texture': 4}[kind]
    offsets = getattr(room, kind + '_table')()
    end = room.hdr['hed'][idx + 1] if idx < 4 else len(room.d)
    bounds = sorted(set(x for x in offsets if x) | {end})
    return [bytes(room.d[x:bounds[bounds.index(x) + 1]]) if x else b'' for x in offsets]


def relayout(room, edit, replacements=None, budget='retail'):
    """Rebuild records, relocate outer tables, preserve opaque relative-offset asset sections.

    Unknown message/event-camera bytes are retained, not converted through display strings.
    replacements maps section index 1..4 to a new section with section-relative tables.
    """
    replacements = replacements or {}
    original = room.to_json()
    j = copy.deepcopy(original)
    j.update(edit)
    rom = copy.deepcopy(room.rom)
    rom.update(j.get('room', {}))
    rom['ptr'] = [0] * 16
    rom['cnt'] = [0] * 14
    out = bytearray(room.d[:128])
    romoff = append(out, bytes(rdx.ROMH[0].size))
    cuts = j['cuts']
    if len(cuts) > 255:
        raise ValueError('at most 255 cuts (script and wall operands are bytes)')
    if cuts:
        off = append(out, bytes(len(cuts) * rdx.CUT_SIZE))
        rom['ptr'][0], rom['cnt'][0] = off, len(cuts)
        for i, cut in enumerate(cuts):
            c = copy.deepcopy(cut)
            if len(c['areas']) > 255:
                raise ValueError('at most 255 areas per cut')
            c['ctab_n'] = len(c['areas'])
            c['cuttp'] = append(out, b''.join(encoded(rdx.CTAB, a) for a in c['areas']), 4)
            p = off + i * rdx.CUT_SIZE
            rdx.pack_into(rdx.CUT, out, p, c)
            for k in range(2):
                rdx.pack_into(rdx.CAM, out, p + 32 + k * rdx.CAM[0].size, c['cam'][k])
            exd = bytes.fromhex(c['exd'])
            if len(exd) != 256:
                raise ValueError('cut exd must contain 256 bytes')
            out[p + 0x1a8:p + 0x2a8] = exd
    for key, pi, ci, st in rdx.SECTIONS:
        recs = j[key]
        if recs:
            chunks=[]
            for i,r in enumerate(recs):
                if i<len(original[key]) and json.dumps(r,sort_keys=True)==json.dumps(original[key][i],sort_keys=True):
                    p=room.rom['ptr'][pi]+i*st[0].size
                    chunks.append(room.d[p:p+st[0].size])
                else:
                    chunks.append(encoded(st,r))
            rom['ptr'][pi] = append(out, b''.join(chunks), 4)
        rom['cnt'][ci] = len(recs)
    for k, n in RECORD_LIMITS.items():
        if len(j[k]) > n:
            raise ValueError('%s exceeds %d records' % (k, n))
    n = len(j['routes'])
    if n:
        matrix = j.get('route_next', [])
        if len(matrix) != n or any(len(row) != n for row in matrix):
            raise ValueError('route_next must be a square routes x routes byte matrix')
        rom['ptr'][11] = append(out, bytes(x for row in matrix for x in row), 4)
    # These contain data not fully represented in rdx.to_json; preserve every original byte.
    for pi, ci in ((13, 11), (14, 12)):
        key = 'event_cameras' if pi == 13 else 'messages'
        if json.dumps(j[key], sort_keys=True) != json.dumps(original[key], sort_keys=True):
            raise ValueError('%s edits are not supported; preserve or supply a new room' % key)
        start = room.rom['ptr'][pi]
        if start:
            end = min([p for p in room.rom['ptr'] if p > start] + [room.hdr['hed'][1]])
            rom['ptr'][pi] = append(out, room.d[start:end], 4)
        rom['cnt'][ci] = room.rom['cnt'][ci]
    # Retail carries a stale editor pointer here, overwritten from hed03 at load.
    rom['ptr'][12] = room.ptr['evtp']
    old_h = room.hdr['hed']
    new_h = [romoff]
    for sec in range(1, 5):
        fresh = sec in replacements
        data = bytearray(replacements[sec] if fresh else
                         room.d[old_h[sec]:old_h[sec + 1] if sec < 4 else len(room.d)])
        off = append(out, data, 64)
        new_h.append(off)
        if sec in (1, 2, 4):
            count = (1 + sum(bool(e['flg'] & 1) for e in j['enemies']) +
                     max(0, len(j['objects']) - 4) + len(j['items'])) if sec == 1 else (
                         len(j['enemies']) + 3 if sec == 2 else rdx.u32(data, 0))
            old_count = len(room.model_table()) if sec == 1 else len(room.motion_table()) if sec == 2 else count
            if not fresh and count != old_count:
                raise ValueError('changed asset record counts require replacement model/motion tables')
            for k in range(count):
                p = off + 4 * (k + int(sec == 4))
                v = rdx.u32(out, p)
                if v:
                    struct.pack_into('<I', out, p, v + off - (0 if fresh else old_h[sec]))
    hdr = copy.deepcopy(room.hdr)
    hdr['hed'][:5] = new_h
    rdx.pack_into(rdx.RDT, out, 0, hdr)
    rdx.pack_into(rdx.ROMH, out, romoff, rom)
    if len(out) > budget_of(budget)['expanded']:
        raise ValueError('expanded file exceeds the %d MiB %s authoring budget' % (budget_of(budget)['expanded'] >> 20, budget))
    return bytes(out)


def read_obj(path):
    """OBJ y-up, same axes as the game (both right-handed); texN materials. Triangulate first."""
    vertices, uv, groups = [], [], []
    group = None
    tid = 0
    for line in Path(path).read_text().splitlines():
        p = line.split('#')[0].split()
        if not p:
            continue
        if p[0] == 'v':
            vertices.append((float(p[1]), float(p[2]), float(p[3])))
        elif p[0] == 'vt':
            uv.append((float(p[1]), 1 - float(p[2])))
        elif p[0] in ('o', 'g'):
            group = []
            groups.append(group)
        elif p[0] == 'usemtl':
            if not p[1].startswith('tex') or not p[1][3:].isdigit():
                raise ValueError('material must be named tex0, tex1, ...')
            tid = int(p[1][3:])
        elif p[0] == 'f':
            if len(p) != 4:
                raise ValueError('OBJ must be triangulated before import')
            if group is None:
                group = []
                groups.append(group)
            face = []
            for token in p[1:]:
                indices = token.split('/')
                vi = int(indices[0]); ti = int(indices[1]) if len(indices) > 1 and indices[1] else None
                if not vi or not -len(vertices) <= vi <= len(vertices):
                    raise ValueError('OBJ vertex index out of range')
                if ti is not None and (not ti or not -len(uv) <= ti <= len(uv)):
                    raise ValueError('OBJ UV index out of range')
                face.append((vertices[vi - 1 if vi > 0 else vi],
                             uv[ti - 1 if ti > 0 else ti] if ti is not None else (0., 0.)))
            group.append((tid, face))
    if not all(math.isfinite(x) for v in vertices + uv for x in v):
        raise ValueError('OBJ positions and UVs must be finite')
    return [g for g in groups if g]


def mesh_mlb(groups, texture_count, budget='retail'):
    limit = budget_of(budget)
    if not groups or len(groups) > limit['groups']:
        raise ValueError('model requires 1..%d mesh groups' % limit['groups'])
    # All offsets after the 16-byte MLB header. Flat siblings, baked object transforms.
    b = bytearray(52 * len(groups))
    tex = append(b, struct.pack('<II', 0, texture_count) + bytes(12 * texture_count))
    struct.pack_into('<I', b, tex, tex + 8)
    for i, triangles in enumerate(groups):
        if len(triangles) > limit['group_triangles']:
            raise ValueError('split mesh groups into at most %d triangles (vertex work buffer, %s budget)' % (limit['group_triangles'], budget))
        # Triangles share a vertex where position and normal are the same (UVs live in the
        # strips): the game transforms and lights every vertex of a drawn node (VU0 CALCPOINT),
        # so each one saved is per-frame work saved; the picture is the same.
        verts, vindex, poly = [], {}, bytearray()
        # One material and one strip chunk (3-vertex strips) per texture: the game sets up a
        # draw per chunk, so a chunk per triangle cost 4,500 draws a frame in the village.
        for tid in sorted({t for t, _ in triangles}):
            run = [face for t, face in triangles if t == tid]
            if not 0 <= tid < texture_count:
                raise ValueError('material texture index out of range')
            # CM_DA: diffuse and ambient BGRA, opaque, no proprietary material bytes.
            poly += struct.pack('<BBHBBHII', 8, 0, tid, 19, 0, 4, 0xffffffff, 0xffffffff)
            poly += struct.pack('<BBHH', 66, 0, 1 + 10*len(run), len(run))
            for face in run:
                a, c, d = [v[0] for v in face]
                e, f = [c[k] - a[k] for k in range(3)], [d[k] - a[k] for k in range(3)]
                normal = (e[1]*f[2]-e[2]*f[1], e[2]*f[0]-e[0]*f[2], e[0]*f[1]-e[1]*f[0])
                length = math.sqrt(sum(x*x for x in normal)) or 1
                normal = tuple(x / length + 0. for x in normal)   # + 0.: no -0, which would split shared vertices
                poly += struct.pack('<h', 3)
                for pos, st in face:
                    # Retail room UVs all lie in 0..1 and the game clamps textures (tiny flags
                    # 0x34, CLAMP); UVs beyond 1 smear the edge texel instead of repeating.
                    if any(not -1e-6 <= v <= 1 + 1e-6 for v in st):
                        raise ValueError('UV outside 0..1: textures clamp, split faces to tile')
                    v = struct.pack('<8f', *pos, 1., *normal, 1.)   # as stored (float32)
                    if v not in vindex:
                        vindex[v] = len(verts)
                        verts.append(struct.unpack('<8f', v))
                    poly += struct.pack('<Hhh', vindex[v], round(st[0]*1024), round(st[1]*1024))
        poly += b'\xff\x00'
        vdata = struct.pack('<BBHHH', 51, 0, 15 + 8*len(verts), 0, len(verts)) + bytes(56)
        vdata += b''.join(struct.pack('<8f', *v) for v in verts) + b'\xff\x00\x00\x00'
        vl = append(b, vdata)
        pl = append(b, poly)
        center = tuple(sum(v[k] for v in verts)/len(verts) for k in range(3))
        radius = max(math.dist(v[:3], center) for v in verts)
        model = append(b, struct.pack('<ii4f', vl, pl, *center, -max(radius, .001)))
        struct.pack_into('<Ii3f3i3fii', b, i*52, 0, model, 0, 0, 0, 0, 0, 0,
                         1, 1, 1, -1, (i+1)*52 if i+1 < len(groups) else -1)
    return struct.pack('<3sBHHii', b'MDL', 128, 16, len(groups), tex, 0) + b


def texture_list(images, gindex=0x4D000000, budget='retail'):
    """Deterministic RGB332 palette quantization, own CLUT, linear 8-bit pixels."""
    limit = budget_of(budget)
    if not 1 <= len(images) <= limit['textures']:
        raise ValueError('1..%d textures (keep runtime entries free for actors/effects)' % limit['textures'])
    out = bytearray()
    for i, (w, h, rgba) in enumerate(images):
        if w not in (8,16,32,64,128,256,512) or h not in (8,16,32,64,128,256,512):
            raise ValueError('texture dimensions must be powers of two, 8..512')
        if len(rgba) != w*h*4 or any(a != 255 for a in rgba[3::4]):
            raise ValueError('only opaque RGBA images supported by RGB332 quantizer')
        pixels = bytes(((rgba[p]*7+127)//255)*32 + ((rgba[p+1]*7+127)//255)*4 + ((rgba[p+2]*3+127)//255)
                       for p in range(0, len(rgba), 4))
        palette = bytearray(1024)
        for c in range(256):
            slot = (c & ~0x18) | ((c & 8) << 1) | ((c & 16) >> 1)
            # Ps2CheckTextureAlpha makes exact-black CLUT entries transparent: keep black opaque.
            palette[slot*4:slot*4+4] = bytes((round((c>>5)*255/7), round(((c>>2)&7)*255/7), (c&3)*85 or (c == 0), 128))
        # TIM2_PICTUREHEADER_EX as retail: FormatId 1 (picture header at 0x80, read there by
        # Send_Tim2_dataEx), OrgColorType 1 (RGB565, opaque), pixels at 256.
        hdr = bytearray(256)
        hdr[:8] = b'TIM2\x04\x01\x01\x00'
        struct.pack_into('<IBB', hdr, 8, gindex+i, 1, 3)
        struct.pack_into('<IIIHHBBBBHH', hdr, 128, 128+len(pixels)+1024, 1024, len(pixels),
                         128, 256, 3, 1, 3, 5, w, h)
        tex0 = (max(1, w//64) << 14) | (19 << 20) | ((w.bit_length()-1) << 26) | ((h.bit_length()-1) << 30) | (1 << 34)
        struct.pack_into('<QQ', hdr, 152, tex0, 0x260)   # GsTex1 as retail: bilinear
        payload = hdr + pixels + palette
        out += struct.pack('<4sI24x', b'TIM2', len(payload)) + payload
    out += struct.pack('<i28x', -1)
    if len(out) > limit['texture_bytes']:
        raise ValueError('texture payload exceeds the %d MiB %s room budget' % (limit['texture_bytes'] >> 20, budget))
    return out


def blank_room():
    b = bytearray(128 + rdx.ROMH[0].size)
    hed = [128]
    for data in (table_blob([b'']), table_blob([b'',b'',b'']), struct.pack('<II4x',8,10), table_blob([], True)):
        hed.append(append(b,data,64))
    rdx.pack_into(rdx.RDT,b,0,zero(rdx.RDT,ver=10.,hed=hed+[0]*15,usrname=b'rdxbuild'))
    rdx.pack_into(rdx.ROMH,b,128,zero(rdx.ROMH,h=30.,w=40.,d=100.,amb_r=[.6]*4,amb_g=[.6]*4,amb_b=[.6]*4))
    return rdx.Room(b,'authored')


def semantic(room):
    j = room.to_json()
    for k in ('name','size','offsets','tables','header'):
        j.pop(k,None)
    for c in j['cuts']:
        c.pop('cuttp',None)
    return j


def decompile(room, dest):
    dest.mkdir(parents=True, exist_ok=True)
    (dest/'preserved.bin').write_bytes(room.d)
    j = room.to_json()
    j['format'] = 'recvx-room-1'
    j['preserved'] = 'preserved.bin'
    (dest/'room.json').write_text(json.dumps(json_safe(j),indent=2,allow_nan=False)+'\n')
    rdx.export_obj(room,str(dest/'room.obj'))
    ts = []
    for _, ts in rdx.texture_blocks(room)[:1]:
        for i,t in enumerate(ts):
            rdx.write_png(str(dest/('tex%d.png'%i)),t['w'],t['h'],rdx.tim2_rgba(room.d,t))
    (dest/'room.mtl').write_text(''.join('newmtl tex%d\nmap_Kd tex%d.png\n'%(i,i) for i in range(len(ts))))
    p=dest/'room.obj'; p.write_text(p.read_text().replace('mtllib none','mtllib room.mtl'))


def split_section_first(data, counted=False):
    off=rdx.u32(data,4 if counted else 0)
    return bytes(data[off:])


def enemy_textured_parts(asset):
    """Replay bhSetEneMdl: the texture key counts textured MLBs, not SKIN/MASKs."""
    pos = 0
    count = 0
    while True:
        if pos + 4 > len(asset): raise ValueError('unterminated enemy model list')
        size = struct.unpack_from('<i', asset, pos)[0]
        pos += 4
        if size == -1: return count
        if size < 0 or pos + size > len(asset): raise ValueError('invalid enemy chunk size')
        if size and asset[pos:pos+4] not in (b'MASK', b'SKIN'):
            if size < 16: raise ValueError('short enemy MLB')
            if struct.unpack_from('<i', asset, pos+8)[0] != -1: count += 1
        pos += size


def enemy_dependency(donor, record):
    """Return a model, its type's motions and textures, even for a non-owning donor.

    Texture lists follow first ownership of (id, mdlver, textured-part ordinal),
    exactly as room.c, rather than the record's index in the model table.
    """
    records = donor.records('enemies')
    if not 0 <= record < len(records): raise ValueError('donor record outside enemy array')
    models = split_table(donor, 'model')
    textures = split_table(donor, 'texture')
    owners = {}
    model_index, texture_index = 1, 1
    selected = None
    for k, egg in enumerate(records):
        if not egg['flg'] & 1: continue
        model = models[model_index]
        model_index += 1
        keys = [(egg['id'], egg['mdlver'], n) for n in range(enemy_textured_parts(model))]
        for key in keys:
            if key not in owners:
                if texture_index >= len(textures): raise ValueError('missing donor texture owner')
                owners[key] = textures[texture_index]
                texture_index += 1
        if k == record: selected = (model, [owners[key] for key in keys])
    if selected is None: raise ValueError('donor enemy must own a model (flg & 1)')
    motion = next((b for b, e in zip(split_table(donor, 'motion'), records)
                   if b and e['id'] == records[record]['id']), b'')
    if not motion: raise ValueError('donor has no motion set for this enemy')
    return selected[0], motion, selected[1]


def validate_enemy_mix(records):
    # Limit authored encounters to independently usable, documented combat types.
    helpers = {1, 2, 3, 5, 6, 13, 17, 23}
    types = {e['id'] for e in records}
    if len(types & helpers) > 1:
        raise ValueError('enemy helper-code slot conflict')
    if any(not 0 <= e['mdlver'] < 16 for e in records):
        raise ValueError('enemy mdlver outside runtime table (et_lp alias)')
    # Conservative allowances, including late children; unfamiliar types are refused.
    slots = {1:11, 4:4, 5:4, 7:4, 9:4, 26:4}
    if types - slots.keys(): raise ValueError('unsupported authored enemy type; needs encounter-specific validation')
    if sum(slots[e['id']] for e in records) > 112:
        raise ValueError('enemy slots exceed 112 including child reserve')
    if any(e['type'] != 0 for e in records):
        raise ValueError('authored enemies require variant 0; other variants need scripted setup')


def retail_texture_list(refs, args):
    """Copy original TIM2 including all palettes; never quantize through a preview PNG."""
    donors, out = {}, bytearray()
    for ref in refs:
        name=ref['room']
        if name not in donors: donors[name]=rdx.load_room(args,name)
        room=donors[name]
        t=rdx.texture_blocks(room)[ref.get('list',0)][1][ref['index']]
        out.extend(struct.pack('<4sI24x',b'TIM2',t['size']))
        out.extend(room.d[t['ofs']:t['ofs']+t['size']])
    out.extend(struct.pack('<i28x',-1))
    return out


def compile_source(path, args=None):
    j = json.loads(path.read_text(), object_hook=json_hook)
    if j.get('patches'): raise ValueError('absolute patches: apply with rdx.py build before decompiling')
    if j.get('format') != 'recvx-room-1':
        raise ValueError('expected format recvx-room-1')
    base = rdx.Room((path.parent/j['preserved']).read_bytes()) if 'preserved' in j else blank_room()
    if 'preserved' not in j and 'mesh' not in j:
        raise ValueError('an authored room requires a mesh')
    budget = j.get('budget', 'retail')
    budget_of(budget)
    if budget == 'pc':   # the PC build lets the player stand on type-6 slopes in this room
        j['room'] = dict(j.get('room', {}), flg=j.get('room', {}).get('flg', base.rom['flg']) | ROOM_SLOPES)
    repl = {}
    if 'mesh' in j:
        if 'preserved' in j:
            j.setdefault('cuts',base.cuts())
            for cut in j['cuts']:
                for camera in cut['cam']: camera['hidobj']=[0]*16
        images = [rdx.read_png(str(path.parent/p)) for p in j.get('textures', [])]
        retail = j.get('retail_textures', [])
        if retail and images: raise ValueError('choose retail_textures or textures')
        texture_count = len(retail) if retail else len(images)
        meshes = split_table(base,'model')
        meshes[0] = mesh_mlb(read_obj(path.parent/j['mesh']),texture_count,budget)
        textures = split_table(base,'texture')
        authored = retail_texture_list(retail, args) if retail else texture_list(images,j.get('gindex',0x4D000000),budget)
        if textures: textures[0] = authored
        else: textures = [authored]
        repl[1],repl[4] = table_blob(meshes),table_blob(textures,True)
    if any(k in j for k in ('enemy_assets','item_meshes','object_meshes','item_assets','object_assets')):
        if 'preserved' in j:
            raise ValueError('asset declarations currently require an authored room, not preserved assets')
        models = [split_section_first(repl[1])]
        texlists = [split_section_first(repl[4], True)]
        motions = []
        declarations = j.get('enemy_assets', [])
        if len(declarations) != len(j.get('enemies', [])):
            raise ValueError('one enemy_assets dependency per enemy required')
        validate_enemy_mix(j.get('enemies', []))
        donors, texture_owners, motion_owners = {}, {}, {}
        for dep, rec in zip(declarations, j.get('enemies', [])):
            if dep['room'] not in donors: donors[dep['room']] = rdx.load_room(args, dep['room'])
            donor = donors[dep['room']]
            asset, motion, lists = enemy_dependency(donor, dep['record'])
            egg = donor.records('enemies')[dep['record']]
            if not rec['flg'] & 1: raise ValueError('authored enemy must own a model (flg & 1)')
            if (rec['id'], rec['mdlver']) != (egg['id'], egg['mdlver']):
                raise ValueError('enemy id/mdlver must match donor record')
            models.append(asset)
            if rec['id'] in motion_owners and motion_owners[rec['id']] != motion:
                raise ValueError('conflicting donor motions for same enemy type')
            motions.append(motion if rec['id'] not in motion_owners else b'')
            motion_owners[rec['id']] = motion
            for k, texture in enumerate(lists):
                key = (rec['id'], rec['mdlver'], k)
                if key in texture_owners:
                    if texture_owners[key] != texture:
                        raise ValueError('conflicting donor textures for same enemy model')
                else:
                    texture_owners[key] = texture
                    texlists.append(texture)
        for kind in ('objects','items'):
            import assets
            records = j.get(kind,[])[4:] if kind=='objects' else j.get(kind,[])
            prefix = 'object' if kind=='objects' else 'item'
            native = prefix+'_assets' in j
            deps = j.get(prefix+('_assets' if native else '_meshes'), [])
            if len(deps) != len(records): raise ValueError(prefix+' asset count must match records')
            owners = {}
            for dep, rec in zip(deps, records):
                if native and 'door' in dep:
                    if kind != 'objects': raise ValueError('door asset must be an object')
                    _, parts = assets.door_pack(rdx.open_disc(args).iso, dep['door'])
                    model, texture = parts[:2]
                elif native:
                    if kind=='items' and 'room' not in dep and 'inventory_item' not in dep:
                        dep=assets.find_item(args,rec['id'])
                    if 'inventory_item' in dep:
                        if kind!='items' or dep['inventory_item']!=rec['id']:
                            raise ValueError('inventory display id must match item')
                        model,texture,_=assets.inventory_model(rdx.open_disc(args).iso,rec['id'])
                    else:
                        if dep['room'] not in donors: donors[dep['room']] = rdx.load_room(args,dep['room'])
                        donor = donors[dep['room']]
                        if kind == 'items' and rec['id'] != donor.records(kind)[dep['record']]['id']:
                            raise ValueError('item id must match its retail display model')
                        model, texture = assets.dependency(donor,kind,dep['record'])
                else:
                    images=[rdx.read_png(str(path.parent/t)) for t in dep['textures']]
                    model=mesh_mlb(read_obj(path.parent/dep['mesh']),len(images),budget)
                    texture=texture_list(images,dep['gindex'],budget)
                models.append(model)
                if texture is not None:
                    if rec['id'] in owners:
                        if owners[rec['id']] != texture: raise ValueError('conflicting textures for shared '+prefix+' id')
                    else:
                        owners[rec['id']] = texture
                        texlists.append(texture)
        repl[1]=table_blob(models)
        repl[2]=table_blob(motions+[b'',b'',b''])
        repl[4]=table_blob(texlists,True)
    if 'scripts' in j:
        blocks = [bytes.fromhex(s) for s in j['scripts']]
        if len(blocks)<2 or any(len(block)<2 or len(block)%2 for block in blocks):
            raise ValueError('scripts requires nonempty, even-sized entry and frame blocks')
        repl[3] = table_blob(blocks, alignment=2)
    data = relayout(base,j,repl,budget)
    validate_authored(rdx.Room(data), 'preserved' not in j, budget)
    return data


def validate_authored(room, authored=True, budget='retail'):
    if not authored:
        return
    limit = budget_of(budget)
    ts = [t for _,lst in rdx.texture_blocks(room) for t in lst]
    if len(ts)>limit['textures'] or sum(t['size'] for t in ts)>limit['texture_bytes']:
        raise ValueError('combined assets exceed %d textures / %d MiB %s room texture budget' % (limit['textures'], limit['texture_bytes'] >> 20, budget))
    seen={}
    for t in ts:
        payload=bytes(room.d[t['ofs']:t['ofs']+t['size']])
        if t['gindex'] in seen and seen[t['gindex']] != payload:
            raise ValueError('different textures share a Gindex')
        seen[t['gindex']]=payload
    for key,limit in [('enemies',100),('objects',1300),('items',200)]:
        if any(not 0<=r['id']<limit for r in room.records(key)):
            raise ValueError(key+' id outside runtime table')
    if any(r['id']>=1213 for r in room.records('objects')):
        raise ValueError('object id exceeds bhJumpObject2 dispatch table')
    if any(not 0<=r['mdlver']<16 for r in room.records('enemies')):
        raise ValueError('enemy mdlver outside runtime table')
    # EneSetCk/DieCk address ene[enep[record].wrk_no]; all 759 retail records hold their own
    # index there. A copied 0 made every kill flag test the first enemy (in-game finding).
    if any(r['wrk_no']!=k for k,r in enumerate(room.records('enemies'))):
        raise ValueError('enemy wrk_no must equal its record index')
    seen=[]
    if not rdx.disasm(room,seen)[1]: raise ValueError('authored scripts do not disassemble cleanly')
    for o in seen:
        # it_flg has 512 bits and ItmSetCk/EneSetCk test only the low 9 bits of their flag
        # (event.c), so a higher bit is set by ItmCk/DieCk but never seen on re-entry.
        if rdx.OP_NAMES[room.d[o]] in ('ItmCk','ItmSetCk','DieCk','EneSetCk') and struct.unpack_from('<H',room.d,o+2)[0]>=512:
            raise ValueError('item/enemy persistence flags must be below 512')
    for key in ('enemies','objects','items','walls','triggers','floors'):
        for rec in room.records(key):
            if not -2 <= rec['flr_no'] <= 29:
                raise ValueError(key+' floor outside grand[]')
    for rec in room.records('walls'):
        if rec['type'] == 6 and not (rec['id'] <= 3 and rec['w'] > 0 and rec['d'] > 0 and rec['h'] > 0):
            raise ValueError('slope walls need id 0..3 and w, h, d > 0 (h 0 is the room height): use slope_wall')
    if not room.cuts() or not room.records('positions'):
        raise ValueError('authored room needs a camera and an entrance')
    for r in room.records('triggers'):
        if r['type']==4 and not r['attr'] & 2 and r['prm0']>=room.cnt['itm']:
            raise ValueError('pickup references missing item')


def link_door(room, trigger, target):
    if len(target)!=3 or not (0<=target[0]<=9 and 0<=target[1]<=99 and 0<=target[2]<=255):
        raise ValueError('target is stage 0..9:room 0..99:entry 0..255')
    records=room.records('triggers')
    t=records[trigger]
    if t['type']!=0: raise ValueError('select a type-0 door trigger')
    previous=bytes(t['prm%d'%i] for i in range(3))
    for i,v in enumerate(target): t['prm%d'%i]=v
    rdx.pack_into(rdx.ATR,room.d,room.ptr['etcp']+trigger*rdx.ATR[0].size,t)
    seen=[]
    _,ok=rdx.disasm(room,seen)
    if not ok: raise ValueError('script sweep is not clean; refusing partial door patch')
    hits=0
    for off in set(seen):
        op=room.d[off]
        if ((op==37 and room.d[off+1]==trigger and room.d[off+8]==0) or
            (op==51 and room.d[off+4:off+7]==previous)):
            room.d[off+4:off+7]=bytes(target); hits+=1
    print('retargeted record and',hits,'script instructions; room-case logic is preserved')
    return bytes(room.d)


def player_speeds(args):
    disc=rdx.open_disc(args).iso
    afs=rdx.Afs(disc,'SYSTEM.AFS')
    result={}
    for index,name in [(10,'Claire'),(11,'Chris')]:
        d=afs.get(index);p=0
        for _ in range(2): p+=4+rdx.u32(d,p)
        end=p+4+rdx.u32(d,p);p+=4;sets=[]
        while p<end:
            size=rdx.u32(d,p);p+=4
            if size==0xffffffff: break
            vals=struct.unpack_from('<%df'%(size//4),d,p);p+=size
            sets.append(vals)
        result[name]={}
        for i,label in [(0,'healthy_walk'),(2,'injured_walk'),(3,'healthy_run'),(5,'injured_run'),(6,'healthy_backstep')]:
            values=sets[i][1:]
            result[name][label]={'samples':len(values),'min':min(values),'mean':statistics.mean(values),'max':max(values)}
    return result


def census(args):
    afs=rdx.open_disc(args); rows=[]; measurements={k:[] for k in ('fixed_fov_degrees','camera_height','wall_width','wall_depth','door_width','door_depth')}; hist={k:Counter() for k in ('wall_type','wall_attr','floor_type','cut_flags','camera_flags','texture_dimensions','enemy_type','item_id')}
    for i,name in enumerate(rdx.room_names()):
        r=rdx.Room(rdx.expand(afs.get(i)),name)
        mo=rdx.model_objects(r); triangles=vertices=0
        for ob in mo['objects']:
            if ob['model']==-1: continue
            vl,pl,*_=rdx.cnk_model(r.d,mo['top'],ob['model'])
            if vl!=-1: vertices+=len(rdx.parse_vlist(r.d,mo['top']+vl)[0])
            if pl!=-1:
                for _,strips in rdx.parse_plist(r.d,mo['top']+pl)[0]:
                    triangles+=sum(max(0,len(s)-2) for _,s in strips)
        ts=[t for _,tlist in rdx.texture_blocks(r) for t in tlist]
        roomts=rdx.texture_blocks(r)[0][1]
        areas=[a for c in r.cuts() for a in c['areas']]
        width=max((a['maxx'] for a in areas),default=0)-min((a['minx'] for a in areas),default=0)
        depth=max((a['maxz'] for a in areas),default=0)-min((a['minz'] for a in areas),default=0)
        for cut in r.cuts():
            cam=cut['cam'][0]
            if not cam['flg'] & 32 and 0 < cam['pers'] < 32768:
                measurements['fixed_fov_degrees'].append(cam['pers']*360/65536)
            fl=cut['flr_no']
            if -2<=fl<=29: measurements['camera_height'].append(cam['py']-r.rom['grand'][fl+2])
        for rec in r.records('walls'):
            measurements['wall_width'].append(rec['w']); measurements['wall_depth'].append(rec['d'])
        for rec in r.records('triggers'):
            if rec['type']==0:
                measurements['door_width'].append(rec['w']); measurements['door_depth'].append(rec['d'])
        rectangles=[(i,a) for i,c in enumerate(r.cuts()) if c['flg']&1 for a in c['areas'] if a['atr_tp']==0]
        overlapping=set()
        for ii,(i,a) in enumerate(rectangles):
            for k,c in rectangles[ii+1:]:
                if i!=k and a['flr_no']==c['flr_no'] and max(a['minx'],c['minx'])<min(a['maxx'],c['maxx']) and max(a['minz'],c['minz'])<min(a['maxz'],c['maxz']):
                    overlapping.add(tuple(sorted((i,k))))
        rows.append(dict(rect_overlap_cut_pairs=len(overlapping),name=name,expanded=len(r.d),**r.cnt,area_width=width,area_depth=depth,
                         triangles=triangles,vertices=vertices,nodes=len(mo['objects']),textures=len(ts),
                         room_textures=len(roomts),texture_pixels=sum(t['w']*t['h'] for t in ts),
                         texture_bytes=sum(t['image_size']+t['clut_size'] for t in ts),areas=len(areas)))
        for k,recs,field in [('wall_type',r.records('walls'),'type'),('wall_attr',r.records('walls'),'attr'),
                              ('floor_type',r.records('floors'),'type'),('cut_flags',r.cuts(),'flg'),
                              ('enemy_type',r.records('enemies'),'id'),('item_id',r.records('items'),'id')]:
            hist[k].update(str(x[field]) for x in recs)
        hist['camera_flags'].update(str(c['cam'][0]['flg']) for c in r.cuts())
        hist['texture_dimensions'].update('%dx%d'%(t['w'],t['h']) for t in ts)
    summary={}
    for k in rows[0]:
        if k=='name': continue
        vals=sorted(r[k] for r in rows)
        summary[k]={'min':vals[0],'median':statistics.median(vals),'p90':vals[math.ceil(.9*len(vals))-1],'max':vals[-1],'total':sum(vals)}
    for k,values in measurements.items():
        values.sort();summary[k]={'min':values[0],'median':statistics.median(values),'p90':values[math.ceil(.9*len(values))-1],'max':values[-1],'count':len(values)}
    result={'player_motion_deltas':player_speeds(args),'rooms':len(rows),'summary':summary,'histograms':{k:dict(v.most_common()) for k,v in hist.items()},'per_room':rows}
    Path(args.output).write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(summary,indent=2))


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--iso')
    sub=p.add_subparsers(dest='cmd',required=True)
    d=sub.add_parser('decompile');d.add_argument('room');d.add_argument('-o','--output',required=True)
    d=sub.add_parser('build');d.add_argument('source');d.add_argument('-o','--output',required=True)
    d=sub.add_parser('link');d.add_argument('room');d.add_argument('trigger',type=int);d.add_argument('--to',required=True);d.add_argument('-o','--output',required=True)
    d=sub.add_parser('census');d.add_argument('-o','--output',required=True)
    d=sub.add_parser('roundtrip');d.add_argument('--all',action='store_true');d.add_argument('--lz',action='store_true',help='also recompress every room (slow)');d.add_argument('room',nargs='?',default='1110')
    args=p.parse_args()
    if args.cmd=='decompile':decompile(rdx.load_room(args,args.room),Path(args.output))
    elif args.cmd=='build':
        b=compile_source(Path(args.source),args);packed=rdx.compress(b)
        loaded=budget_of(json.loads(Path(args.source).read_text()).get('budget','retail'))['loaded']
        if loaded and len(b)+len(packed)>loaded:
            raise ValueError('packed + expanded file exceed the %d MiB the room arena holds while loading' % (loaded >> 20))
        Path(args.output).write_bytes(packed);print('expanded',len(b),'bytes, packed',len(packed))
    elif args.cmd=='link':
        Path(args.output).write_bytes(rdx.compress(link_door(rdx.load_room(args,args.room),args.trigger,[int(x) for x in args.to.split(':')])))
    elif args.cmd=='census':census(args)
    elif args.cmd=='roundtrip':
        afs=rdx.open_disc(args)
        names=rdx.room_names() if args.all else [rdx.room_name(args.room)]
        for name in names:
            a=rdx.Room(rdx.expand(afs.get(rdx.room_names().index(name))),name)
            b=rdx.Room(relayout(a,{}),name)
            assert json.dumps(semantic(a),sort_keys=True)==json.dumps(semantic(b),sort_keys=True),name+' records differ'
            for key,pi,ci,st in rdx.SECTIONS:
                n=a.rom['cnt'][ci]*st[0].size
                assert a.d[a.rom['ptr'][pi]:a.rom['ptr'][pi]+n]==b.d[b.rom['ptr'][pi]:b.rom['ptr'][pi]+n],name+' '+key+' bytes'
            for kind in ('model','motion','texture'):
                aa,bb=split_table(a,kind),split_table(b,kind)
                # Section alignment padding may grow at the end; meaningful bytes must match.
                assert len(aa)==len(bb) and all(y.startswith(x) and not any(y[len(x):]) for x,y in zip(aa,bb)),name+' '+kind
            start,end=a.hdr['hed'][3:5]; start2,end2=b.hdr['hed'][3:5]
            assert bytes(b.d[start2:end2]).startswith(a.d[start:end]),name+' scripts'
            if args.lz or not args.all:
                assert rdx.expand(rdx.compress(bytes(b.d)))==b.d,name+' LZ'
            print(name,'OK',len(a.d),'->',len(b.d),flush=True)
        print(len(names),'rooms passed')

if __name__=='__main__':
    main()
