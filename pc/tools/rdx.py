#!/usr/bin/env python3
"""Room files (RM_SRRC.RDX) of Resident Evil Code: Veronica X (PS2, SLUS-20184).

Reads the player's own disc image at run time; nothing derived from it lives in the repo.
Format reference: docs/modding/ROOM_FORMAT.md. Commands (run with -h for options):

  list                         room files in RDX_LNK.AFS (index, name, packed size)
  dump ROOM [-o x.json]        all sections of a room as JSON (ROOM = RM_1000, 1000 or a file)
  build ROOM edit.json -o OUT  write the JSON's records (same counts) and "patches" (raw bytes at
                               expanded-file offsets) back into the room, compress -> OUT
  extract ROOM -o x.bin        expanded room file (raw, for hex editors / own tools)
  pack x.bin -o RM_xxxx.RDX    compress an expanded room file
  script ROOM                  disassemble the room's event scripts
  tex ROOM -o DIR              export the room's TIM2 textures (.tm2 + .png)
  teximport ROOM L K x.png -o OUT   write a PNG into texture K of list L in place
                               (same size, nearest colours of its palette)
  obj ROOM -o x.obj            export the room model (positions, strips, UVs) as Wavefront OBJ
  census                       statistics over all rooms (used for docs/modding/ROOM_FORMAT.md)
  selftest                     LZ round trip + JSON round trip on a few rooms

ISO path: --iso, $RECVX_ISO, or ISO/SLUS-20184 ROM.iso in the repository.
Modded rooms go to <mods dir>/RM_xxxx.RDX (the port loads them instead of the disc's, docs/modding/README.md "The mods folder").
"""
import argparse, json, os, re, struct, sys, zlib

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SECTOR = 2048

# ---------------------------------------------------------------- disc image
class Iso:
    def __init__(self, path):
        self.f = open(path, 'rb')
        pvd = self.read(16, 1)
        if pvd[1:6] != b'CD001':
            sys.exit('%s: not an ISO9660 image' % path)
        self.root = struct.unpack_from('<I', pvd, 156 + 2)[0], struct.unpack_from('<I', pvd, 156 + 10)[0]

    def read(self, lsn, count):
        self.f.seek(lsn * SECTOR)
        return self.f.read(count * SECTOR)

    def find(self, name):
        """root-directory file -> (lsn, size)"""
        lsn, size = self.root
        d = self.read(lsn, (size + SECTOR - 1) // SECTOR)
        p = 0
        while p < len(d):
            n = d[p]
            if n == 0:
                p = (p // SECTOR + 1) * SECTOR
                continue
            fn = d[p + 33:p + 33 + d[p + 32]].split(b';')[0].decode('ascii', 'replace')
            if fn.upper() == name.upper():
                return struct.unpack_from('<I', d, p + 2)[0], struct.unpack_from('<I', d, p + 10)[0]
            p += n
        sys.exit('%s not found on the disc' % name)

class Afs:
    """AFS archive: 'AFS\\0', u32 count, count x (u32 offset, u32 size); offsets from the archive start."""
    def __init__(self, iso, name):
        self.iso = iso
        self.lsn, _ = iso.find(name)
        h = iso.read(self.lsn, 1)
        if h[:3] != b'AFS':
            sys.exit('%s is not an AFS archive' % name)
        n = struct.unpack_from('<I', h, 4)[0]
        toc = iso.read(self.lsn, (8 + 8 * n + SECTOR - 1) // SECTOR)
        self.toc = [struct.unpack_from('<II', toc, 8 + 8 * i) for i in range(n)]

    def get(self, i):
        o, s = self.toc[i]
        self.iso.f.seek(self.lsn * SECTOR + o)
        return self.iso.f.read(s)

def room_names():
    """RM_SRRC.RDX names in AFS order, from the decomp's file table (ps2_dvd_image.c rdx_files[])."""
    src = os.path.join(REPO, 'decomp/src/ps2/veronica/prog/ps2_dvd_image.c')
    return re.findall(r'"(RM_\d{4}\.RDX)"', open(src).read())

# ---------------------------------------------------------------- LZ (expand.c Expand)
def expand(s):
    """Decompress. 8-bit descriptor fields, LSB first, fetched when the previous one is used up
    (the first one before any data). 1 = literal byte; 00 = short copy: 2 bits -> length 2..5,
    1 byte offset -256..-1; 01 = long copy: u16 LE w, offset (w >> 3) - 8192, length (w & 7) + 2
    or, if w & 7 == 0, next byte + 1; w == 0 ends the stream."""
    out = bytearray()
    p = 1
    field, left = s[0], 9

    def bit():
        nonlocal field, left, p
        left -= 1
        if left == 0:
            field, left = s[p], 8
            p += 1
        b = field & 1
        field >>= 1
        return b
    while True:
        if bit():
            out.append(s[p]); p += 1
            continue
        if not bit():
            n = 1 + 2 * bit()
            n += bit()
            off = s[p] - 0x100; p += 1
        else:
            w = s[p] | s[p + 1] << 8; p += 2
            if w == 0:
                break
            off = (w >> 3) - 0x2000
            n = w & 7
            if n:
                n += 1
            else:
                n = s[p]; p += 1
        n += 1
        start = len(out) + off
        if -off >= n:
            out += out[start:start + n]
        else:
            for k in range(n):
                out.append(out[start + k])
    return bytes(out)

def compress(d):
    """Greedy LZ in the format expand() reads (hash of 3 bytes, window 8191, length <= 256)."""
    # The decoder fetches a descriptor byte when it needs its first bit, so a field is written
    # (followed by the data bytes of the tokens read while it was current) only when a ninth
    # bit is requested.
    out = bytearray()
    bits = []          # descriptor bits of the current field
    pend = bytearray() # data bytes read while the current field is current

    def put(*bs):
        for b in bs:
            if len(bits) == 8:
                flush()
            bits.append(b)

    def flush():
        v = 0
        for k, b in enumerate(bits):
            v |= b << k
        out.append(v)
        out.extend(pend)
        bits.clear(); pend.clear()

    def common(i, j, lim):
        lo, hi = 3, lim          # d[i:i+lo] == d[j:j+lo] holds
        while lo < hi:
            mid = (lo + hi + 1) // 2
            if d[i + lo:i + mid] == d[j + lo:j + mid]:
                lo = mid
            else:
                hi = mid - 1
        return lo

    head = {}
    n = len(d)
    i = 0
    while i < n:
        best_len = best_off = 0
        if i + 3 <= n:
            cands = head.get(d[i:i + 3])
            if cands:
                lim = min(256, n - i)
                for j in reversed(cands):
                    if i - j > 8191:
                        break
                    m = common(i, j, lim)
                    if m > best_len:
                        best_len, best_off = m, i - j
                        if m == lim:
                            break
        if best_len >= 3 or (best_len == 2 and best_off <= 256):
            if best_len <= 5 and best_off <= 256:
                ln = best_len - 2
                put(0, 0, ln >> 1, ln & 1)
                pend.append(0x100 - best_off)
            elif best_len <= 9:
                w = ((0x2000 - best_off) << 3) | (best_len - 2)
                put(0, 1)
                pend += bytes((w & 0xFF, w >> 8))
            else:
                w = (0x2000 - best_off) << 3
                put(0, 1)
                pend += bytes((w & 0xFF, w >> 8, best_len - 1))
            step = best_len
        else:
            put(1)
            pend.append(d[i])
            step = 1
        for k in range(i, min(i + step, n - 2)):
            l = head.setdefault(d[k:k + 3], [])
            l.append(k)
            if len(l) > 32:      # ponytail: 16 newest candidates per key, enough for ~45% ratio
                del l[:16]
        i += step
    put(0, 1)
    pend += b'\0\0'
    flush()
    return bytes(out)

# ---------------------------------------------------------------- structures
# (name, struct code); arrays as 'name[n]' with one code. Little endian, packed.
def S(*fields):
    fmt, names = '<', []
    for name, code in fields:
        m = re.match(r'(\w+)\[(\d+)\]', name)
        if m:
            fmt += m.group(2) + code
            names.append((m.group(1), int(m.group(2))))
        else:
            fmt += code
            names.append((name, 0))
    return struct.Struct(fmt), names

def unpack(st, buf, off):
    s, names = st
    v = s.unpack_from(buf, off)
    r, k = {}, 0
    for name, n in names:
        if n:
            r[name] = list(v[k:k + n]); k += n
        else:
            r[name] = v[k]; k += 1
    return r

def pack_into(st, buf, off, rec):
    s, names = st
    vals = []
    for name, n in names:
        x = rec[name]
        vals += list(x) if n else [x]
    s.pack_into(buf, off, *vals)

RDT = S(('ver', 'f'), ('flg', 'I'), ('dat0', 'I'), ('dat1', 'I'), ('hed[20]', 'I'), ('usrname', '32s'))
# ETTY_WORK / EGG_WORK (enemies, objects, items)
ETTY = S(('flg', 'I'), ('id', 'H'), ('type', 'H'), ('flr_no', 'b'), ('mdlver', 'B'), ('wrk_no', 'B'), ('prm1', 'b'),
         ('px', 'f'), ('py', 'f'), ('pz', 'f'), ('ax', 'h'), ('az', 'h'), ('ay', 'h'), ('aspd', 'h'), ('hide[4]', 'b'))
# ATR_WORK (walls, triggers "etc", floors, routes)
ATR = S(('flg', 'B'), ('type', 'B'), ('id', 'B'), ('flr_no', 'b'), ('attr', 'I'), ('px', 'f'), ('py', 'f'), ('pz', 'f'),
        ('w', 'f'), ('h', 'f'), ('d', 'f'), ('prm0', 'B'), ('prm1', 'B'), ('prm2', 'B'), ('prm3', 'B'))
POS = S(('px', 'f'), ('py', 'f'), ('pz', 'f'), ('ay', 'i'))
EFF = S(('flg', 'I'), ('id', 'H'), ('type', 'H'), ('flr_no', 'h'), ('mdlver', 'H'), ('px', 'f'), ('py', 'f'), ('pz', 'f'),
        ('sx', 'f'), ('sy', 'f'), ('sz', 'f'), ('ay', 'h'), ('ax', 'h'), ('lkflg', 'i'), ('lkno', 'i'), ('lkono', 'i'),
        ('lx', 'f'), ('ly', 'f'), ('lz', 'f'), ('param', 'i'))
LGT = S(('flg', 'I'), ('type', 'I'), ('aspd', 'i'), ('lkflg', 'i'), ('lkno', 'i'), ('lkono', 'i'), ('lsrc', 'i'),
        ('px', 'f'), ('py', 'f'), ('pz', 'f'), ('lx', 'f'), ('ly', 'f'), ('lz', 'f'), ('vx', 'f'), ('vy', 'f'), ('vz', 'f'),
        ('spc', 'f'), ('dif', 'f'), ('amb', 'f'), ('r', 'f'), ('g', 'f'), ('b', 'f'), ('nr', 'f'), ('fr', 'f'),
        ('iang', 'i'), ('oang', 'i'), ('ax', 'i'), ('ay', 'i'), ('az', 'i'), ('runtime[27]', 'I'))
CAM = S(('flg', 'H'), ('lgtclip', 'B'), ('spd', 'b'), ('px', 'f'), ('py', 'f'), ('pz', 'f'), ('ln', 'f'), ('w', 'f'),
        ('h', 'f'), ('d', 'f'), ('y0', 'f'), ('y1', 'f'), ('y2', 'f'), ('y3', 'f'), ('am_spd', 'f'), ('ax', 'i'),
        ('ay', 'i'), ('az', 'i'), ('lax', 'i'), ('lay', 'i'), ('laz0', 'h'), ('laz1', 'h'), ('laz2', 'h'), ('laz3', 'h'),
        ('aa_spd', 'b'), ('fil_no', 'b'), ('fil_rt', 'b'), ('reserve', 'b'), ('pers', 'i'), ('hidobj[16]', 'I'),
        ('hidlgt[8]', 'I'), ('fog_col', 'I'), ('fog_nr', 'f'), ('fog_fr', 'f'))
CUT = S(('flg', 'B'), ('type', 'B'), ('flr_no', 'b'), ('ctab_n', 'B'), ('cuttp', 'I'), ('cx', 'f'), ('cy', 'f'),
        ('cz', 'f'), ('cw', 'f'), ('ch', 'f'), ('cd', 'f'))          # + cam[2] at 0x20, exd[256] at 0x1A8
CUT_SIZE = 0x2A8
CTAB = S(('attr', 'I'), ('flr_no', 'b'), ('reserve0', 'b'), ('reserve1', 'b'), ('atr_tp', 'b'), ('h', 'f'),
         ('dmy', 'f'), ('minx', 'f'), ('minz', 'f'), ('maxx', 'f'), ('maxz', 'f'))
KEYF = S(('flg', 'H'), ('frame', 'h'), ('px', 'f'), ('py', 'f'), ('pz', 'f'), ('ax', 'h'), ('ay', 'h'), ('az', 'h'),
         ('pers', 'h'), ('hidobj[8]', 'I'), ('hidlgt[4]', 'I'), ('fog_col', 'I'), ('fog_nr', 'f'), ('fog_fr', 'f'),
         ('lkflg', 'h'), ('lkno', 'h'), ('lkono', 'h'), ('nxt_no', 'h'), ('lx', 'f'), ('ly', 'f'), ('lz', 'f'),
         ('prm[5]', 'f'), ('recp', 'I'))
ROMH = S(('ptr[16]', 'I'), ('dmp[16]', 'I'), ('cnt[14]', 'i'), ('dmy[18]', 'i'), ('flg', 'I'), ('bak_col', 'I'),
         ('fog_col', 'I'), ('fog_nr', 'f'), ('fog_fr', 'f'), ('w', 'f'), ('h', 'f'), ('d', 'f'), ('grand[32]', 'f'),
         ('mdl[6]', 'I'), ('fog[128]', 'f'), ('amb_rom', 'b'), ('amb_chr', 'b'), ('amb_obj', 'b'), ('amb_itm', 'b'),
         ('amb_r[4]', 'f'), ('amb_g[4]', 'f'), ('amb_b[4]', 'f'))
ROM_PTRS = 'cutp lgtp enep objp itmp effp walp etcp flrp posp rutp ruttp evtp evcp mesp evlp'.split()
ROM_CNTS = 'cut lgt ene obj itm eff wal etc flr pos rut evc mes evl'.split()
# record sections: json key, pointer index, count index, struct
SECTIONS = [('lights', 1, 1, LGT), ('enemies', 2, 2, ETTY), ('objects', 3, 3, ETTY), ('items', 4, 4, ETTY),
            ('effects', 5, 5, EFF), ('walls', 6, 6, ATR), ('triggers', 7, 7, ATR), ('floors', 8, 8, ATR),
            ('positions', 9, 9, POS), ('routes', 10, 10, ATR), ('event_lights', 15, 13, LGT)]

def u32(d, o): return struct.unpack_from('<I', d, o)[0]

class Room:
    def __init__(self, data, name='?'):
        self.d = bytearray(data)
        self.name = name
        self.hdr = unpack(RDT, self.d, 0)
        self.rom_ofs = self.hdr['hed'][0]
        self.rom = unpack(ROMH, self.d, self.rom_ofs)
        self.ptr = dict(zip(ROM_PTRS, self.rom['ptr']))
        self.cnt = dict(zip(ROM_CNTS, self.rom['cnt']))

    # tables behind the header (offsets into the expanded file)
    def model_table(self):
        """hed01: u32 offsets: room model, then one per enemy record with flg&1, per object 4..,
        per item (room.c bhSetRoom order)"""
        o = self.hdr['hed'][1]
        n = 1 + sum(1 for e in self.records('enemies') if e['flg'] & 1) + max(0, self.cnt['obj'] - 4) + self.cnt['itm']
        return [u32(self.d, o + 4 * i) for i in range(n)]

    def motion_table(self):
        """hed02: one u32 per enemy record (0 = reuse the type's set), room motions, mouth/lip data"""
        o = self.hdr['hed'][2]
        return [u32(self.d, o + 4 * i) for i in range(self.cnt['ene'] + 3)]

    def texture_table(self):
        """hed04: u32 count, then count offsets (room model, owning enemy models, objects, items, effects)"""
        o = self.hdr['hed'][4]
        n = u32(self.d, o)
        return [u32(self.d, o + 4 + 4 * i) for i in range(n)]

    def records(self, key):
        for k, pi, ci, st in SECTIONS:
            if k == key:
                base, n = self.rom['ptr'][pi], self.rom['cnt'][ci]
                return [unpack(st, self.d, base + i * st[0].size) for i in range(n)] if base else []

    def cuts(self):
        res = []
        for i in range(self.cnt['cut']):
            o = self.ptr['cutp'] + i * CUT_SIZE
            c = unpack(CUT, self.d, o)
            c['cam'] = [unpack(CAM, self.d, o + 0x20 + k * CAM[0].size) for k in range(2)]
            c['exd'] = self.d[o + 0x1A8:o + 0x2A8].hex()
            c['areas'] = [unpack(CTAB, self.d, c['cuttp'] + j * 0x20) for j in range(c['ctab_n'])]
            res.append(c)
        return res

    def event_cameras(self):
        res = []
        for i in range(self.cnt['evc']):
            o = self.ptr['evcp'] + i * 0x808
            flg, typ, nxt, kn = struct.unpack_from('<HHhh', self.d, o)
            res.append({'flg': flg, 'type': typ, 'nxt_no': nxt, 'keyf_n': kn,
                        'keyf': [unpack(KEYF, self.d, o + 8 + k * 0x80) for k in range(max(0, min(kn, 16)))]})
        return res

    def messages(self):
        """mesp: u32 table (entry 0 = ?, entry i+1 = offset of message i from mesp), u16 glyphs"""
        if not self.ptr['mesp']:
            return []
        base = self.ptr['mesp']
        out = []
        for i in range(self.cnt['mes']):
            o = base + u32(self.d, base + 4 * (i + 1))
            out.append(decode_text(self.d, o))
        return out

    def script_header(self):
        """hed03 = EVT block: u32 offsets relative to its start: scd0 (on room entry), scd1 (every
        frame), then one per event (started by op 20 bhEvtOn)."""
        base = self.hdr['hed'][3]
        first = u32(self.d, base)
        offs = []
        for i in range(0, 4096):
            v = u32(self.d, base + 4 * i)
            if i * 4 >= first:
                break
            offs.append(v)
            first = min(first, v) if v else first
        return base, offs

    def to_json(self):
        j = {'name': self.name, 'size': len(self.d),
             'header': {'ver': self.hdr['ver'], 'flg': self.hdr['flg'], 'hed': self.hdr['hed'][:5],
                        'usrname': self.hdr['usrname'].split(b'\0')[0].decode('ascii', 'replace')},
             'room': {k: self.rom[k] for k in ('flg', 'bak_col', 'fog_col', 'fog_nr', 'fog_fr', 'w', 'h', 'd',
                                                'grand', 'fog', 'amb_rom', 'amb_chr', 'amb_obj', 'amb_itm',
                                                'amb_r', 'amb_g', 'amb_b')},
             'counts': self.cnt,
             'offsets': {k: v for k, v in self.ptr.items()}}
        j['cuts'] = self.cuts()
        for k, *_ in SECTIONS:
            j[k] = self.records(k)
        j['event_cameras'] = self.event_cameras()
        j['messages'] = self.messages()
        j['tables'] = {'models': self.model_table(), 'motions': self.motion_table(),
                       'textures': self.texture_table(), 'script': self.script_header()[1]}
        if self.cnt['rut'] and self.ptr['ruttp']:
            n = self.cnt['rut']
            j['route_next'] = [list(self.d[self.ptr['ruttp'] + r * n:self.ptr['ruttp'] + (r + 1) * n]) for r in range(n)]
        return j

    def apply_json(self, j):
        """write records from JSON back (same counts; fields keep their binary layout)"""
        for k, pi, ci, st in SECTIONS:
            recs = j.get(k)
            if recs is None:
                continue
            if len(recs) != self.rom['cnt'][ci]:
                sys.exit('%s: %d records, room has %d (record counts are fixed, see docs/modding/BUILDING_ROOMS.md "Editing a retail room")' % (k, len(recs), self.rom['cnt'][ci]))
            for i, r in enumerate(recs):
                pack_into(st, self.d, self.rom['ptr'][pi] + i * st[0].size, r)
        for i, c in enumerate(j.get('cuts', [])):
            o = self.ptr['cutp'] + i * CUT_SIZE
            if c['ctab_n'] != self.cuts()[i]['ctab_n']:
                sys.exit('cut %d: area count is fixed' % i)
            pack_into(CUT, self.d, o, c)
            for kk in range(2):
                pack_into(CAM, self.d, o + 0x20 + kk * CAM[0].size, c['cam'][kk])
            for jj, a in enumerate(c['areas']):
                pack_into(CTAB, self.d, c['cuttp'] + jj * 0x20, a)
        if 'room' in j:
            r = dict(self.rom)
            r.update(j['room'])
            pack_into(ROMH, self.d, self.rom_ofs, r)
        # raw byte patches, e.g. script operands: {"offset": "0x84550", "hex": "01 04 00 16"}
        # (offsets as printed by the `script` command = offsets in the expanded file)
        for p in j.get('patches', []):
            o, b = int(str(p['offset']), 0), bytes.fromhex(p['hex'])
            if 'expect' in p and self.d[o:o + len(b)] != bytes.fromhex(p['expect']):
                sys.exit('patch at %#x: found %s, expected %s' % (o, self.d[o:o + len(b)].hex(' '), p['expect']))
            self.d[o:o + len(b)] = b
        return bytes(self.d)

def decode_text(d, o, limit=2048):
    """u16 glyphs: ASCII - 32 below 0x60 (as cheat_glue.c reads item names); 0xFFxx control codes
    in {}; ends at 0xFFFF"""
    s = []
    for k in range(limit):
        c = struct.unpack_from('<H', d, o + 2 * k)[0]
        if c == 0xFFFF:
            break
        if c == 0xFF01:
            s.append(' ')
        elif c < 95:
            s.append(chr(c + 32))
        else:
            s.append('{%04X}' % c)
    return ''.join(s)

# ---------------------------------------------------------------- room access
def open_disc(args):
    iso = args.iso or os.environ.get('RECVX_ISO') or os.path.join(REPO, 'ISO/SLUS-20184 ROM.iso')
    if not os.path.exists(iso):
        sys.exit('disc image not found: pass --iso or set RECVX_ISO')
    return Afs(Iso(iso), 'RDX_LNK.AFS')

def room_name(s):
    m = re.fullmatch(r'(?:RM_)?(\d{4})(?:\.RDX)?', os.path.basename(s), re.I)
    return 'RM_%s.RDX' % m.group(1) if m else None

def load_room(args, spec):
    """ROOM spec: an existing file (.RDX packed or expanded .bin), or a room name/number from the disc"""
    if os.path.exists(spec):
        b = open(spec, 'rb').read()
        if struct.unpack_from('<f', b, 0)[0] != 10.0:   # expanded files start with version 10.0
            b = expand(b)
        return Room(b, os.path.basename(spec))
    name = room_name(spec)
    names = room_names()
    if not name or name not in names:
        sys.exit('unknown room %s' % spec)
    return Room(expand(open_disc(args).get(names.index(name))), name)

# ---------------------------------------------------------------- event scripts (event.c)
# OP_NAMES: bhScenarioJmpT[256] without the "bh" prefix. OP_LEN: bytes per instruction incl.
# the opcode, derived from the handlers (path analysis of the bhScePtr increments; flow ops by
# hand: While = 2 + the condition instruction that follows, Sleep = 4 incl. the Sleeping byte it
# rewinds to); 0 = unused (dm0). Checked: a linear sweep ends cleanly in all 205 rooms.
OP_NAMES = """
End IfelCk ElseCk Endif Ck Set CmpB CmpW Sv SvW WalAtariSet EtcAtariSet FlrAtariSet DieCk ItmCk
UseItemClear UseItemCheck PlItemCheck CineSet CamSet EvtOn BgmOn BgmOff SeOn SeOff VoiceOn VoiceOff
AdxCk BGSeOn BGSeOff AdxTimeCk MessageSet SetDispObj DieEventCk EneSetCk ItmSetCk InitModelSet
EtcAtariSet2 ArmsItemCheck ArmsItemChange SubStatus CamPauseSet CamSet2 MotionPauseSet EffectSet
InitMotionPause MotionPauseSetPly InitSetKage InitMotionPauseEx PlItemLost ObjLinkSet SetDoorCall
ObjLinkSetPly LightSet FadeSet RoomCaseNo FrameCheck CamInfoSet MutekiSetPl DefModelSet MaskSet
LipSet MaskStart LipStart LookGsetPlStart LookGsetPlStop ItmAspdSet EffDispSet EffAmbSet DelObjSe
SetNextRoomBgm SetNextRoomBgSe FootSeCall WeaponSeCall YakkyouSet LightTypeSet FogColorSet
PlItemBlockCk EffBloodSet CyoutenHenkeiSet SetObjMotion ObjLinkSetObjEne ObjLinkSetObjItem
ObjLinkSetEneItem ObjLinkSetEneEne CyoutenHenkeiStart EffBloodPoolSet FixEventCamPly
EffBloodPoolSet2 ObjLinkSetObjObj CamYureSet InitCamSet MesDispEndSet PadCheck MovieStart MovieStop
TFrameCheck EventTimerClr CamCheck RandamSet PlCtr LoadWork ObjCtr SubCtr LoadWork2 CommonCtr
EventSkipSet DelYakkyou ObjAlphaSet CyodanSet HEffectSet ObjLinkSetObjPly EffPush EffPop
AreaSearchObj LightParameterCSet LightParameterStart InitMidiSlotSet 3dSoundFlagSet SoundVolumeSet
LightParameterSet EneSeOn EneSeOff WalAtariSet2 FlrAtariSet2 MotionPosSetEnePly KageSwSet
SoundPanSet InitPonySet SubMapBusyCk SetDebugLoopEx SoundFadeOut CyoutenHenkeiSetEX
CyoutenHenkeiStartEX EasySESet SoundFlagReSet EffUVSet PlayerChangeSet PlayerPoisonCk AddObjSe
RandTest EvtComSet ZombieUpDieCk FacePauseSet FaceReSet EffModeSet BGSeOff2 BgmOff2 BGSeOn2 BgmOn2
EffectSensyaSet EffectKokuenSet EffectSandSet EnemyHpUp FaceRep MovieCk SetItmMotion ObjAspdSet
PuruPuruFlagSet PuruPuruStart MapSystemOn TrapDamageSet EvtLighterFireSet ObjLinkSetPlyItem
PlayerKaidanMotion EneRenderSet BgmOnEx BgmOn2Ex FogParameterCSet FogParameterStart EffUVSet2
BGColorSet MovieTimeCk EffTypeSet PlayerPoison2Cr PlyHandChange HEffectSet2 ObjDposCk ItemGetGet
EtcAtariEnePosSet EtcAtariEvtPosSet LoadWorkEx RoomSoundCase ItemPlToSBox ItemSBoxToIBox GrdPosSet
GrdPosMoveCSet GrdPosMoveStart EvtKill ReTryPointSet PlyDposCk PlItemLostEx CyodanSetEx ArmsItemSet
ItemGetGetEx EffectSandSetMatsumoto VoiceWait VoiceStart GameOverSet PlItemChangeM EffBakuDrmSet
PlItemTamaSet EffClearEvt EvtTimerSet EneLookFlgSet ReturnTitleEvt SyukanModeSet ExGameItemInit
EneLifeSetM EffSSizeSet EffLinkOffsetSet RankingCall CallSysSe dm0 dm0 dm0 dm0 dm0 dm0 dm0 dm0 dm0
dm0 dm0 dm0 dm0 dm0 dm0 dm0 dm0 dm0 dm0 dm0 dm0 dm0 dm0 dm0 dm0 dm0 dm0 dm0 dm0 dm0 Ewhile2 ComNext
dm0 dm0 dm0 Sleep Sleeping For Next While Ewhile EvtNext EvtEnd
""".split()
OP_LEN = [
    2, 2, 2, 2, 6, 6, 4, 6, 4, 4, 4, 4, 4, 4, 6, 2, 2, 2, 2, 4, 4, 4, 2, 8, 2, 8, 2, 6, 6, 4, 6, 4,
    4, 6, 4, 6, 4, 10, 2, 2, 2, 2, 4, 2, 4, 2, 2, 4, 4, 2, 12, 8, 12, 4, 6, 2, 6, 4, 2, 6, 4, 4, 4, 4,
    8, 2, 4, 4, 6, 2, 8, 10, 6, 10, 8, 4, 6, 4, 14, 6, 2, 12, 12, 12, 12, 2, 6, 2, 14, 12, 4, 2, 2, 6, 4, 2,
    6, 2, 6, 2, 1, 4, 1, 1, 6, 1, 2, 2, 8, 22, 22, 12, 2, 2, 14, 22, 4, 4, 4, 6, 14, 6, 2, 10, 10, 4, 4, 6,
    4, 2, 4, 2, 8, 4, 16, 2, 10, 2, 2, 12, 6, 2, 4, 4, 2, 4, 2, 2, 4, 4, 2, 8, 12, 4, 4, 6, 2, 4, 2, 2,
    6, 2, 4, 12, 4, 4, 4, 4, 10, 2, 10, 6, 6, 4, 2, 4, 22, 6, 4, 8, 4, 4, 2, 2, 2, 10, 16, 2, 2, 2, 6, 4,
    28, 2, 6, 4, 8, 2, 2, 4, 8, 4, 2, 2, 4, 2, 2, 2, 4, 10, 10, 2, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 4, 3, 4, 2, 2, 1, 1, 2,
]
# Ops 100/102/103/105 are followed by a sub-command. Controller sub-commands (event.c Player_controll / Obj_controll / Sub_controll / Common_controll):
# length incl. the sub-command byte, after the 1-byte controller op.
CTRL_LEN = {
    100: {150: 3, 129: 11, 130: 1, 141: 5, 142: 1, 131: 3, 140: 7, 12: 5, 13: 5, 132: 5, 143: 3, 145: 5,
          137: 3, 128: 1, 139: 1, 7: 7, 52: 1, 0: 1},
    102: {20: 3, 1: 1},
    103: {128: 1, 139: 1, 144: 1, 143: 5, 146: 7, 147: 7, 148: 3, 149: 3, 0: 1},
    105: {53: 3, 51: 5, 52: 3, 50: 3, 47: 1, 43: 4, 44: 4, 45: 4, 42: 3, 41: 3, 46: 3, 40: 3, 28: 2, 30: 2,
          31: 2, 29: 2, 32: 2, 33: 2, 26: 9, 27: 7, 48: 3, 49: 3, 25: 7, 35: 7, 24: 7, 34: 9, 23: 3, 5: 5,
          6: 5, 12: 5, 13: 5, 7: 7, 11: 5, 8: 7, 9: 5, 2: 2, 3: 2, 4: 2, 17: 2, 18: 2, 19: 2, 22: 5, 10: 5,
          132: 5, 140: 7, 141: 5, 14: 3, 21: 3, 15: 5, 16: 5, 36: 1, 38: 9, 0: 1},
}
FLAG_SETS = {1: 'ev_flg', 2: 'ky_flg', 3: 'ed_flg', 4: 'rm_flg', 5: 'st_flg', 6: 'sp_flg', 7: 'it_flg', 8: 'mp_flg',
             9: 'ic_flg', 10: 'cb_flg', 11: 'gm_flg', 12: 'ts_flg', 13: 'plp->flg', 14: 'plp->stflg', 15: 'plp->flg2',
             16: 'ssd_flg'}

def insn_len(d, o):
    op = d[o]
    n = OP_LEN[op]
    if op in CTRL_LEN:
        # an unknown sub-command is not consumed (the controllers' switch has no default), so
        # the game runs its byte as the next opcode -- RM_3020 event 6 relies on that
        n += CTRL_LEN[op].get(d[o + 1], 0)
    return n

def comment(d, o):
    """readable operands for the ops a map maker needs most"""
    op = d[o]
    nm = OP_NAMES[op]
    if nm in ('Ck', 'Set'):
        fs = FLAG_SETS.get(d[o + 1], '?%d' % d[o + 1])
        bit = struct.unpack_from('<H', d, o + 2)[0]     # bit n = word n>>5, mask 0x80000000 >> (n&31)
        if nm == 'Ck':                                  # continues if (bit set) != byte 5
            if d[o + 1] == 10 and d[o + 2] in (22, 23):
                return 'player %s %s %d' % ('is at' if d[o + 5] == 0 else 'not at',
                                            'trigger (etc)' if d[o + 2] == 23 else 'floor', d[o + 4])
            return '%s bit %d is %s' % (fs, bit & 0x3FF, 'set' if d[o + 5] == 0 else 'clear')
        return '%s bit %d %s' % (fs, bit & 0x3FF, ['set', 'clear', 'toggle', 'set from', 'clear from', 'toggle from'][d[o + 5]] if d[o + 5] < 6 else '?')
    if nm == 'SetDoorCall':
        a = struct.unpack_from('<H', d, o + 2)[0]
        return 'door attr %#x -> stage %d room %d pos %d door type %d' % (a, d[o + 4], d[o + 5], d[o + 6], d[o + 7])
    if nm in ('EtcAtariSet', 'WalAtariSet', 'FlrAtariSet'):
        return '%s %d %s' % (nm[:3].lower(), d[o + 1], 'off' if d[o + 2] else 'on')
    if nm in ('EtcAtariSet2', 'WalAtariSet2', 'FlrAtariSet2'):
        return '%s %d := type %d attr %#x prm %d %d %d %d' % (nm[:3].lower(), d[o + 1], d[o + 8],
                                                            struct.unpack_from('<H', d, o + 2)[0], *d[o + 4:o + 8])
    if nm == 'InitModelSet':
        return '%s %d %s' % (['player', 'enemy record', 'object', 'item'][d[o + 1]] if d[o + 1] < 4 else '?', d[o + 2],
                             'hidden' if d[o + 3] == 0 else 'shown')
    if nm == 'EvtOn':
        v = struct.unpack_from('<H', d, o + 2)[0]
        return 'start event %d at task level %d' % (v >> 8, v & 0xFF)
    if nm == 'MessageSet':
        return 'message %d' % d[o + 2]
    if nm == 'ItmCk':
        return 'item trigger etc %d, it_flg bit %d' % (d[o + 4], struct.unpack_from('<H', d, o + 2)[0] & 0x3FF)
    if nm == 'ItmSetCk':
        return 'item slot %d, etc %d, it_flg bit %d' % (d[o + 1], d[o + 4], struct.unpack_from('<H', d, o + 2)[0] & 0x1FF)
    if nm in ('DieCk', 'EneSetCk'):   # EneSetCk tests only the low 9 bits (event.c bhEneSetCk)
        return 'enemy record %d, ed_flg bit %d' % (d[o + 1], struct.unpack_from('<H', d, o + 2)[0] & (0x3FF if nm == 'DieCk' else 0x1FF))
    if nm == 'CmpB':
        what = {0: 'stg_no', 1: 'rom_no', 2: 'cut', 15: 'pos_no', 8: 'sb_id', 17: 'wpnl_no', 21: 'rcase', 23: 'gm_mode',
                24: 'ply_id', 25: 'costume'}.get(d[o + 1], '?%d' % d[o + 1])
        return '%s %s %d' % (what, ['==', '>', '>=', '<', '<=', '!='][d[o + 2]] if d[o + 2] < 6 else '?', d[o + 3])
    if nm == 'UseItemCheck':
        return 'used item == %d' % d[o + 1]
    if nm in ('IfelCk', 'ElseCk'):
        return 'skip %d' % d[o + 1]
    return ''

def disasm_block(d, start, end_hint, is_event, seen=None):
    """linear sweep; returns (lines, ok); instruction offsets are appended to seen"""
    # IfelCk skip -> offset (from the IfelCk) of its Endif or ElseCk; ElseCk skip -> offset (from
    # the ElseCk) of the first instruction after the else part (no Endif then).
    out, o, ends = [], start, []
    while o < len(d):
        while ends and ends[-1] <= o:
            ends.pop()
        op = d[o]
        n = insn_len(d, o)
        if n <= 0:
            out.append('%06x  %02x  ??? %s  (unknown opcode or sub-command: sweep stops)' % (o, op, OP_NAMES[op]))
            return out, False
        nm = OP_NAMES[op]
        if op in CTRL_LEN:
            nm += '.%d' % d[o + 1] if d[o + 1] in CTRL_LEN[op] else '(no sub-command)'
        out.append('%06x  %s%-34s %-24s %s' % (o, '  ' * len(ends), d[o:o + n].hex(' '), nm, comment(d, o)))
        if seen is not None:
            seen.append(o)
        if nm == 'IfelCk':
            ends.append(o + d[o + 1] + 1)        # the Endif / ElseCk itself closes the block
        elif nm == 'ElseCk':
            if ends:
                ends.pop()
            ends.append(o + d[o + 1])
        elif nm == 'Endif' and ends:
            ends.pop()
        o += n
        if not ends and (op == 0 or (is_event and op == 255)):
            return out, True
        if end_hint and o > end_hint:
            out.append('        (ran past the next block)')
            return out, False
    return out, False

def disasm(room, seen=None):
    base, offs = room.script_header()
    starts = sorted(set(offs))
    lines, ok_all = [], True
    for i, off in enumerate(offs):
        nxt = min([s for s in starts if s > off], default=0)
        title = 'scd0 (room entry)' if i == 0 else 'scd1 (every frame)' if i == 1 else 'event %d' % (i - 2)
        lines.append('; %s at hed03+%#x' % (title, off))
        l, ok = disasm_block(room.d, base + off, base + nxt if nxt else 0, i >= 2, seen)
        ok_all &= ok
        lines += l
    return lines, ok_all

# ---------------------------------------------------------------- textures (PLI/TIM2 lists)
def texture_blocks(room):
    """texture table entries -> list of (entry, [TIM2 headers])"""
    res = []
    for k, off in enumerate(room.texture_table()):
        tims, p = [], off
        while p + 32 <= len(room.d):     # the last list may end with the file, without -1
            code, sz = struct.unpack_from('<iI', room.d, p)
            if code == -1:
                break
            if room.d[p:p + 4] == b'TIM2':
                h = room.d[p + 32:p + 32 + 0xD0]
                tims.append({'ofs': p + 32, 'size': sz, 'gindex': struct.unpack_from('<I', h, 8)[0],
                             'clut_size': struct.unpack_from('<I', h, 0x84)[0],
                             'image_size': struct.unpack_from('<I', h, 0x88)[0],
                             'clut_colors': struct.unpack_from('<H', h, 0x8E)[0],
                             'type': h[0x93], 'w': struct.unpack_from('<H', h, 0x94)[0],
                             'h': struct.unpack_from('<H', h, 0x96)[0]})
            p += 32 + sz
        res.append((k, tims))
    return res

def tim2_rgba(d, t):
    """decode an indexed TIM2 picture (type 4 = 4 bit, 5 = 8 bit) to RGBA bytes, first CLUT.
    Layout (TIM2_PICTUREHEADER_EX): 0x80 bytes file header + palette info, the standard TIM2
    picture header at 0x80, pixels at 0x80 + HeaderSize, CLUT(s) (32-bit RGBA) after the pixels."""
    hdr = t['ofs'] + 0x80
    hsize = struct.unpack_from('<H', d, hdr + 0x0C)[0]
    img = hdr + hsize
    clut = img + t['image_size']
    w, h = t['w'], t['h']
    ncol = t['clut_colors']
    pal = [d[clut + 4 * i:clut + 4 * i + 4] for i in range(ncol)]
    if t['type'] == 5 and ncol == 256:              # PS2 CSM1 order: swap entries 8..15 / 16..23 per 32
        pal = [pal[(i & ~0x18) | ((i & 8) << 1) | ((i & 16) >> 1)] for i in range(256)]
    out = bytearray()
    for y in range(h):
        for x in range(w):
            if t['type'] == 5:
                ix = d[img + y * w + x]
            else:
                b = d[img + (y * w + x) // 2]
                ix = b & 15 if x % 2 == 0 else b >> 4
            c = pal[ix] if ix < len(pal) else b'\0\0\0\xff'
            out += bytes((c[0], c[1], c[2], min(255, c[3] * 2)))
    return bytes(out)

def read_png(path):
    """8-bit RGB/RGBA, non-interlaced PNG -> (w, h, rgba bytes)"""
    with open(path, 'rb') as f:
        b = f.read()
    p, idat, w = 8, b'', 0
    while p < len(b):
        n, t = struct.unpack_from('>I4s', b, p)
        c = b[p + 8:p + 8 + n]
        if t == b'IHDR':
            w, h, depth, ctype, _, _, inter = struct.unpack('>IIBBBBB', c)
            if depth != 8 or ctype not in (2, 6) or inter:
                sys.exit('%s: need an 8-bit RGB or RGBA PNG without interlacing' % path)
            bpp = 4 if ctype == 6 else 3
        elif t == b'IDAT':
            idat += c
        p += 12 + n
    raw, out, prev, stride = zlib.decompress(idat), bytearray(), bytearray(w * bpp), w * bpp
    for y in range(h):
        f, line = raw[y * (stride + 1)], bytearray(raw[y * (stride + 1) + 1:(y + 1) * (stride + 1)])
        for i in range(stride):
            a = line[i - bpp] if i >= bpp else 0
            c = prev[i - bpp] if i >= bpp else 0
            up = prev[i]
            if f == 1: line[i] = (line[i] + a) & 255
            elif f == 2: line[i] = (line[i] + up) & 255
            elif f == 3: line[i] = (line[i] + (a + up) // 2) & 255
            elif f == 4:
                pa, pb, pc = abs(up - c), abs(a - c), abs(a + up - 2 * c)
                line[i] = (line[i] + (a if pa <= pb and pa <= pc else up if pb <= pc else c)) & 255
        prev = line
        for x in range(w):
            px = line[x * bpp:x * bpp + bpp]
            out += px if bpp == 4 else px + b'\xff'
    return w, h, bytes(out)

def tim2_import(d, t, rgba):
    """write RGBA pixels into an indexed TIM2 in place: nearest colour of its first CLUT
    (same size and format; the palette itself is kept)"""
    hdr = t['ofs'] + 0x80
    img = hdr + struct.unpack_from('<H', d, hdr + 0x0C)[0]
    clut = img + t['image_size']
    cols = []                     # CLUT in pixel-value order (as tim2_rgba decodes it)
    for i in range(t['clut_colors']):
        j = (i & ~0x18) | ((i & 8) << 1) | ((i & 16) >> 1) if t['clut_colors'] == 256 else i
        c = d[clut + 4 * j:clut + 4 * j + 4]
        cols.append((c[0], c[1], c[2], min(255, c[3] * 2)))
    cache = {}
    w, h = t['w'], t['h']
    for y in range(h):
        for x in range(w):
            px = tuple(rgba[(y * w + x) * 4:(y * w + x) * 4 + 4])
            ix = cache.get(px)
            if ix is None:
                ix = min(range(len(cols)), key=lambda k: sum((cols[k][q] - px[q]) ** 2 for q in range(4)))
                cache[px] = ix
            if t['type'] == 5:
                d[img + y * w + x] = ix
            else:
                o = img + (y * w + x) // 2
                d[o] = (d[o] & 0xF0) | ix if x % 2 == 0 else (d[o] & 0x0F) | (ix << 4)

def write_png(path, w, h, rgba):
    raw = b''.join(b'\0' + rgba[y * w * 4:(y + 1) * w * 4] for y in range(h))
    def chunk(t, b):
        return struct.pack('>I', len(b)) + t + b + struct.pack('>I', zlib.crc32(t + b) & 0xFFFFFFFF)
    with open(path, 'wb') as f:
        f.write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 6, 0, 0, 0)) +
                chunk(b'IDAT', zlib.compress(raw, 9)) + chunk(b'IEND', b''))

# ---------------------------------------------------------------- room model (Ninja chunk)
def model_objects(room):
    """the room model (first entry of the model table): MLB header, flat NJS_CNK_OBJECT array"""
    base = room.model_table()[0]
    d = room.d
    status = d[base + 3]
    hsize, nobj = struct.unpack_from('<HH', d, base + 4)[0], struct.unpack_from('<H', d, base + 6)[0]
    tex_off, obj_off = struct.unpack_from('<ii', d, base + 8)
    top = base + hsize
    objs = []
    for i in range(nobj):
        o = top + obj_off + i * 0x34
        ev, model = struct.unpack_from('<Ii', d, o)
        pos = struct.unpack_from('<3f', d, o + 8)
        ang = struct.unpack_from('<3i', d, o + 20)
        scl = struct.unpack_from('<3f', d, o + 32)
        child, sib = struct.unpack_from('<ii', d, o + 44)
        objs.append({'evalflags': ev, 'model': model, 'pos': pos, 'ang': ang, 'scl': scl, 'child': child, 'sibling': sib})
    return {'base': base, 'status': status, 'top': top, 'tex_off': tex_off, 'obj_off': obj_off, 'objects': objs}

def cnk_model(d, top, moff):
    """NJS_CNK_MODEL at top+moff: vlist, plist (offsets from top, -1 = none), center, r"""
    vl, pl = struct.unpack_from('<ii', d, top + moff)
    c = struct.unpack_from('<3f', d, top + moff + 8)
    r = struct.unpack_from('<f', d, top + moff + 20)[0]
    return vl, pl, c, r

def parse_vlist(d, p):
    """vertex chunks -> {index: (x,y,z)}; types: 41 CV_VN (pos + normal), 51 CV_VN for the PS2 VU
    path (same element layout as far as positions go, see docs/modding/ROOM_FORMAT.md "MLB models and the room model")"""
    verts, types = {}, []
    while True:
        t = d[p]
        if t == 255:
            break
        flag = d[p + 1]
        size = struct.unpack_from('<H', d, p + 2)[0]       # in 4-byte words after this header
        idx_off, n = struct.unpack_from('<HH', d, p + 4)
        types.append(t)
        # 51 (ps2_NinjaCnk.c njCnkCvVnPs2): 56 unused bytes, then x y z 1 nx ny nz 1 per vertex
        first, stride = {41: (8, 6), 51: (64, 8)}.get(t, (0, 0))
        for k in range(n if stride else 0):
            verts[idx_off + k] = struct.unpack_from('<3f', d, p + first + 4 * stride * k)
        p += 4 + 4 * size
    return verts, types

def parse_plist(d, p):
    """poly chunks -> list of (texture id, [strips: [(index, u, v), ...]]); handles CT_TID (8),
    materials (17..23), bits (1..3), strips 64 (no UV), 65 (UV 0..255), 66 (UV 0..1023)"""
    out, types, tid = [], [], 0
    while True:
        t = d[p]
        types.append(t)
        if t == 255:
            break
        if t == 0:
            p += 2
        elif t < 8:                                          # bits chunks: 2 bytes
            p += 2
        elif t < 16:                                         # tiny: texture id
            tid = struct.unpack_from('<H', d, p + 2)[0] & 0x1FFF
            p += 4
        elif t < 64:                                         # material / volume: u16 size in words
            size = struct.unpack_from('<H', d, p + 2)[0]
            p += 4 + 2 * size
        else:
            size = struct.unpack_from('<H', d, p + 2)[0]     # in 2-byte units after this header
            hdr = struct.unpack_from('<H', d, p + 4)[0]
            nstrip, user = hdr & 0x3FFF, hdr >> 14
            q = p + 6
            uvs = t in (65, 66, 68, 69)
            scale = 256.0 if t in (65, 68) else 1024.0
            strips = []
            for s in range(nstrip):
                cnt = struct.unpack_from('<h', d, q)[0]
                q += 2
                st = []
                for k in range(abs(cnt)):
                    ix = struct.unpack_from('<H', d, q)[0]; q += 2
                    u = v = 0.0
                    if uvs:
                        u, v = struct.unpack_from('<hh', d, q); q += 4
                        u, v = u / scale, v / scale
                    q += 2 * user if k >= 2 else 0
                    st.append((ix, u, v))
                strips.append((cnt < 0, st))
            out.append((tid, strips))
            p += 4 + 2 * size
    return out, types

def euler_matrix(ang):
    """Ninja XYZ rotation (bhCalcTree: rotateXYZ), angles in 65536 units"""
    import math
    m = [[1, 0, 0], [0, 1, 0], [0, 0, 1]]
    def mul(a, b):
        return [[sum(a[i][k] * b[k][j] for k in range(3)) for j in range(3)] for i in range(3)]
    for axis, a in zip('xyz', ang):
        if not a:
            continue
        c, s = math.cos(a * 2 * math.pi / 65536), math.sin(a * 2 * math.pi / 65536)
        r = {'x': [[1, 0, 0], [0, c, s], [0, -s, c]], 'y': [[c, 0, -s], [0, 1, 0], [s, 0, c]],
             'z': [[c, s, 0], [-s, c, 0], [0, 0, 1]]}[axis]
        m = mul(r, m)
    return m

def export_obj(room, path):
    """room model -> OBJ: one group per object, world transforms applied (flat hierarchy walk as
    MdlPut.c bhCalcTree), strips -> triangles, texture id -> material tex<id>"""
    mo = model_objects(room)
    d, top = room.d, mo['top']
    stack, cur = [], ([[1, 0, 0], [0, 1, 0], [0, 0, 1]], (0.0, 0.0, 0.0))
    lines, vt, nv, nt = ['mtllib none'], [], 0, 0
    objs = mo['objects']
    for i, ob in enumerate(objs):
        if ob['sibling'] != -1:
            stack.append(cur)
        rot, tr = cur
        # world = parent * translate(pos) * rotate(ang): point p -> parent(pos + R p)
        R = euler_matrix(ob['ang'])
        wrot = [[sum(rot[r][k] * R[k][c] for k in range(3)) for c in range(3)] for r in range(3)]
        wtr = tuple(tr[r] + sum(rot[r][k] * ob['pos'][k] for k in range(3)) for r in range(3))
        cur = (wrot, wtr)
        if ob['model'] != -1:
            vl, pl, c, rad = cnk_model(d, top, ob['model'])
            verts, _ = parse_vlist(d, top + vl) if vl != -1 else ({}, [])
            polys, _ = parse_plist(d, top + pl) if pl != -1 else ([], [])
            lines.append('o obj%03d' % i)
            vmap = {}
            for ix, p in sorted(verts.items()):
                w = [wtr[r] + sum(wrot[r][k] * p[k] for k in range(3)) for r in range(3)]
                nv += 1
                vmap[ix] = nv
                lines.append('v %.4f %.4f %.4f' % (w[0], w[1], w[2]))
            for tid, strips in polys:
                lines.append('usemtl tex%d' % tid)
                for rev, st in strips:
                    idx = []
                    for ix, u, v in st:
                        nt += 1
                        lines.append('vt %.5f %.5f' % (u, 1 - v))
                        idx.append((vmap.get(ix), nt))
                    for k in range(len(idx) - 2):
                        a, b, cc = idx[k], idx[k + 1], idx[k + 2]
                        if None in (a[0], b[0], cc[0]):
                            continue
                        if (k % 2 == 1) ^ rev:
                            a, b = b, a
                        lines.append('f %d/%d %d/%d %d/%d' % (a[0], a[1], b[0], b[1], cc[0], cc[1]))
        if ob['child'] == -1:
            cur = stack.pop() if stack else cur
    with open(path, 'w') as f:
        f.write('\n'.join(lines) + '\n')
    return sum(1 for o in objs if o['model'] != -1)

# ---------------------------------------------------------------- commands
def cmd_list(args):
    afs = open_disc(args)
    for i, n in enumerate(room_names()):
        print('%3d %s %9d' % (i, n, afs.toc[i][1]))

def cmd_dump(args):
    r = load_room(args, args.room)
    s = json.dumps(r.to_json(), indent=1)
    if args.o:
        open(args.o, 'w').write(s)
    else:
        print(s)

def cmd_build(args):
    r = load_room(args, args.room)
    b = r.apply_json(json.load(open(args.json)))
    write_room(args, b)

def write_room(args, b):
    if args.raw:
        open(args.o, 'wb').write(b)
    else:
        c = compress(b)
        open(args.o, 'wb').write(c)
        print('%s: %d bytes expanded, %d packed' % (args.o, len(b), len(c)))

def cmd_extract(args):
    open(args.o, 'wb').write(bytes(load_room(args, args.room).d))

def cmd_pack(args):
    args.raw = False
    write_room(args, open(args.file, 'rb').read())

def cmd_script(args):
    lines, ok = disasm(load_room(args, args.room))
    print('\n'.join(lines))
    if not ok:
        print('; warning: a sweep did not end cleanly (see docs/modding/ROOM_FORMAT.md "Event scripts")')

def cmd_tex(args):
    r = load_room(args, args.room)
    os.makedirs(args.o, exist_ok=True)
    n = 0
    for k, tims in texture_blocks(r):
        for j, t in enumerate(tims):
            stem = os.path.join(args.o, 'tex%02d_%02d_g%08x' % (k, j, t['gindex']))
            open(stem + '.tm2', 'wb').write(r.d[t['ofs']:t['ofs'] + t['size']])
            write_png(stem + '.png', t['w'], t['h'], tim2_rgba(r.d, t))
            n += 1
    print('%d textures -> %s' % (n, args.o))

def cmd_teximport(args):
    r = load_room(args, args.room)
    t = texture_blocks(r)[args.block][1][args.index]
    w, h, rgba = read_png(args.png)
    if (w, h) != (t['w'], t['h']):
        sys.exit('PNG is %dx%d, the texture is %dx%d (in-place import keeps the size)' % (w, h, t['w'], t['h']))
    tim2_import(r.d, t, rgba)
    write_room(args, bytes(r.d))

def cmd_obj(args):
    r = load_room(args, args.room)
    print('%d objects with geometry -> %s' % (export_obj(r, args.o), args.o))

def cmd_census(args):
    afs = open_disc(args)
    from collections import Counter
    vt, pt, cnt, bad, mx, flags = Counter(), Counter(), Counter(), [], Counter(), {}
    sizes = []
    for i, n in enumerate(room_names()):
        r = Room(expand(afs.get(i)), n)
        sizes.append((len(r.d), n))
        for k, v in r.cnt.items():
            mx[k] = max(mx[k], v)
        mo = model_objects(r)
        cnt['objects'] = max(cnt['objects'], len(mo['objects']))
        for ob in mo['objects']:
            if ob['model'] == -1:
                continue
            vl, pl, c, rad = cnk_model(r.d, mo['top'], ob['model'])
            if vl != -1:
                vt.update(parse_vlist(r.d, mo['top'] + vl)[1])
            if pl != -1:
                pt.update(parse_plist(r.d, mo['top'] + pl)[1])
        seen = []
        lines, ok = disasm(r, seen)
        if not ok:
            bad.append(n)
        for o in seen:
            nm, b = OP_NAMES[r.d[o]], r.d[o:o + 8]
            if nm in ('Ck', 'Set') and b[1] in (1, 2, 3, 7, 8):
                flags.setdefault(FLAG_SETS[b[1]], set()).add(struct.unpack_from('<H', b, 2)[0] & 0x3FF)
            elif nm in ('ItmCk', 'ItmSetCk'):
                flags.setdefault('it_flg', set()).add(struct.unpack_from('<H', b, 2)[0] & 0x1FF)
            elif nm in ('DieCk', 'EneSetCk'):
                flags.setdefault('ed_flg', set()).add(struct.unpack_from('<H', b, 2)[0] & 0x3FF)
        for t in r.records('triggers'):
            cnt['trigger type %d' % t['type']] += 1
        for w in r.records('walls'):
            cnt['wall type %d' % w['type']] += 1
        for f in r.records('floors'):
            cnt['floor type %d' % f['type']] += 1
        for c in r.cuts():
            cnt['cut type %d' % c['type']] += 1
    print('largest expanded:', sorted(sizes)[-3:])
    print('max counts:', dict(mx))
    print('room model vertex chunk types:', dict(vt))
    print('room model poly chunk types:', dict(pt))
    print('records:', dict(sorted(cnt.items())))
    print('script sweeps not clean (%d):' % len(bad), bad)
    size = {'ev_flg': 1024, 'ky_flg': 512, 'ed_flg': 1024, 'it_flg': 512, 'mp_flg': 256}
    for k, used in sorted(flags.items()):
        free, run = [], None
        for i in range(size[k]):
            if i in used:
                run = None
            elif run is None:
                run = [i, i]; free.append(run)
            else:
                run[1] = i
        print('%s: %d bits referenced by room scripts; never referenced: %s' % (k, len(used),
              ' '.join('%d' % a if a == b else '%d-%d' % (a, b) for a, b in free)))

def cmd_selftest(args):
    afs = open_disc(args)
    names = room_names()
    for n in ('RM_0130.RDX', 'RM_0100.RDX', 'RM_1110.RDX'):
        d = expand(afs.get(names.index(n)))
        r = Room(d, n)
        assert r.apply_json(json.loads(json.dumps(r.to_json()))) == d, n + ' json round trip'
        lines, ok = disasm(r)
        assert ok, n + ' script sweep'
        if n == 'RM_0130.RDX':
            assert expand(compress(d)) == d, n + ' LZ round trip'
        print(n, 'ok')

def cmd_grants(args):
    """items the room scripts hand over: Sv sb_id N with Set cb_flg bit 27 (0x10: the status screen's
    item get) - 'given' when bit 17 (0x4000: no question) is set too, else 'asked'; per script part,
    with the conditions of the blocks that start that event (docs/COOP.md "Items from events")"""
    afs = open_disc(args)
    for i, n in enumerate(room_names()):
        r = Room(expand(afs.get(i)), n)
        base, offs = r.script_header()
        starts, parts, starters = sorted(set(offs)), [], {}
        for k, off in enumerate(offs):
            seen, nxt = [], min([s for s in starts if s > off], default=0)
            disasm_block(r.d, base + off, base + nxt if nxt else 0, k >= 2, seen)
            parts.append(seen)
        for k, seen in enumerate(parts):
            stack = []
            for o in seen:
                while stack and stack[-1][0] <= o:
                    stack.pop()
                nm = OP_NAMES[r.d[o]]
                if nm == 'IfelCk':
                    stack.append([o + r.d[o + 1] + 1, []])
                elif nm == 'ElseCk' and stack:
                    stack[-1] = [o + r.d[o + 1], ['else']]
                elif nm in ('Ck', 'CmpB', 'UseItemCheck') and stack:
                    stack[-1][1].append(comment(r.d, o))
                elif nm == 'EvtOn':
                    where = 'scd0' if k == 0 else 'scd1' if k == 1 else 'event %d' % (k - 2)
                    starters.setdefault(struct.unpack_from('<H', r.d, o + 2)[0] >> 8, []).append(
                        '%s [%s]' % (where, '; '.join(c for s in stack for c in s[1])))
        for k, seen in enumerate(parts):
            for j, o in enumerate(seen):
                if OP_NAMES[r.d[o]] != 'Sv' or r.d[o + 1] != 8:
                    continue
                bits = {struct.unpack_from('<H', r.d, q + 2)[0] for q in seen[max(0, j - 3):j + 5]
                        if OP_NAMES[r.d[q]] == 'Set' and r.d[q + 1] == 10 and r.d[q + 5] == 0}
                if 27 not in bits and 17 not in bits:
                    continue
                where = 'scd0' if k == 0 else 'scd1' if k == 1 else 'event %d' % (k - 2)
                print('%s %-8s %06x item %3d %s%s  <- %s' % (n, where, o, r.d[o + 2], 'given' if 17 in bits else 'asked',
                      ' (equipped)' if 16 in bits else '', ' | '.join(starters.get(k - 2, [])) if k >= 2 else ''))

def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--iso')
    sp = ap.add_subparsers(dest='cmd', required=True)
    sp.add_parser('list').set_defaults(f=cmd_list)
    p = sp.add_parser('dump'); p.add_argument('room'); p.add_argument('-o'); p.set_defaults(f=cmd_dump)
    p = sp.add_parser('build'); p.add_argument('room'); p.add_argument('json'); p.add_argument('-o', required=True)
    p.add_argument('--raw', action='store_true', help='write expanded, do not compress'); p.set_defaults(f=cmd_build)
    p = sp.add_parser('extract'); p.add_argument('room'); p.add_argument('-o', required=True); p.set_defaults(f=cmd_extract)
    p = sp.add_parser('pack'); p.add_argument('file'); p.add_argument('-o', required=True); p.set_defaults(f=cmd_pack)
    p = sp.add_parser('script'); p.add_argument('room'); p.set_defaults(f=cmd_script)
    p = sp.add_parser('tex'); p.add_argument('room'); p.add_argument('-o', required=True); p.set_defaults(f=cmd_tex)
    p = sp.add_parser('obj'); p.add_argument('room'); p.add_argument('-o', required=True); p.set_defaults(f=cmd_obj)
    p = sp.add_parser('teximport'); p.add_argument('room'); p.add_argument('block', type=int); p.add_argument('index', type=int)
    p.add_argument('png'); p.add_argument('-o', required=True); p.add_argument('--raw', action='store_true')
    p.set_defaults(f=cmd_teximport)
    sp.add_parser('census').set_defaults(f=cmd_census)
    sp.add_parser('grants').set_defaults(f=cmd_grants)
    sp.add_parser('selftest').set_defaults(f=cmd_selftest)
    args = ap.parse_args()
    args.f(args)

if __name__ == '__main__':
    main()
