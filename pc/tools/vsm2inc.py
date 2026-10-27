#!/usr/bin/env python3
"""Convert the decomp's VU microcode listings (vsm/ps2_vu0.vsm, vsm/ps2_vu1.vsm)
into decoded instruction tables for pc/src/ps2/vu.c.

    vsm2inc.py <in.vsm> <out.inc> <prefix>     e.g. ... ps2_vu1.vsm vu1_microcode.inc vu1

The table is indexed by VU micro-memory address (MPG load addresses are honoured).
Labels become <PREFIX>_ADDR_<label> defines. Encodings in the listing are used to
cross-check label immediates and LOI constants."""
import re, sys, struct, os

src, dst, prefix = sys.argv[1], sys.argv[2], sys.argv[3]
PFX = prefix.upper()

slots = {}       # address -> (hex, upper, lower)
labels = {}
addr = 0
pending_labels = []
for line in open(src, encoding='latin-1'):
    m = re.search(r'\*/\s+MPG\s+(0x[0-9A-Fa-f]+|\d+)\s*,', line)
    if m:
        addr = int(m.group(1), 0)
        continue
    m = re.match(r'\s*([A-Za-z_0-9]+):\s*$', line)
    if m:
        labels[m.group(1)] = addr
        continue
    m = re.match(r'\s*/\* [0-9A-F]{4} ([0-9A-F]{16}) \*/\s*(.*?)\s*/\* \| \*/\s*(.*?)\s*$', line)
    if m and 'PADDING' not in line:
        slots[addr] = (m.group(1), m.group(2), m.group(3))
        addr += 1

FIELD = {'x': 8, 'y': 4, 'z': 2, 'w': 1}
COMP = {'x': 0, 'y': 1, 'z': 2, 'w': 3}
def mask(s): return sum(FIELD[c] for c in s)
def vf(s): return int(re.match(r'vf(\d+)', s.strip()).group(1))
def vi(s): return int(re.match(r'\(?vi(\d+)', s.strip()).group(1))

def upper(txt):
    e = '[E]' in txt; txt = txt.replace('[E]', '')
    d = dict(op='U_NOP', mask=0, fd=0, fs=0, ft=0, bc='BC_NONE', e=e)
    if txt.strip() == 'NOP':
        return d
    m = re.match(r'(FTOI0|FTOI4|FTOI12|FTOI15|ITOF0|ITOF4|ITOF12|ITOF15|ABS)\.([xyzw]+)\s+(.*)', txt)
    if m:
        op, fields, args = m.groups(); args = [a.strip() for a in args.split(',')]
        d.update(op='U_' + op, mask=mask(fields), fd=vf(args[0]), fs=vf(args[1]))
        return d
    m = re.match(r'([A-Z]+?)(A?)([xyzwqi]?)\.([xyzw]+)\s+(.*)', txt)
    base, acc, bc, fields, args = m.groups()
    args = [a.strip() for a in args.split(',')]
    d['mask'] = mask(fields)
    if bc in ('x', 'y', 'z', 'w'): d['bc'] = 'BC_' + bc.upper()
    elif bc == 'q': d['bc'] = 'BC_Q'
    elif bc == 'i': d['bc'] = 'BC_I'
    if base == 'CLIP':
        d.update(op='U_CLIP', fs=vf(args[0]), ft=vf(args[1]))
        return d
    if acc: base += 'A'
    d['op'] = 'U_' + base
    d['fd'] = 0xFF if args[0] == 'ACC' else vf(args[0])
    d['fs'] = vf(args[1])
    if d['bc'] not in ('BC_Q', 'BC_I'): d['ft'] = vf(args[2])
    return d

def lower(txt, hexw):
    d = dict(op='L_NOP', mask=0, a=0, b=0, c=0, imm=0, fsf=0, ftf=0, fimm=None)
    t = txt.strip()
    if t == 'NOP':
        return d
    m = re.match(r'([A-Z0-9]+)(?:\.([xyzw]+))?\s*(.*)', t)
    op, fields, args = m.groups()
    args = [a.strip() for a in args.split(',')] if args else []
    d['op'] = 'L_' + op
    if fields: d['mask'] = mask(fields)
    lab = lambda s: labels[s] if s in labels else int(s, 0)
    if op in ('SQ', 'LQ', 'ILW', 'ISW'):
        mm = re.match(r'(-?\d+)\((vi\d+)\)', args[1])
        d['imm'] = int(mm.group(1)); d['b'] = vi(mm.group(2))
        d['a'] = vf(args[0]) if op in ('SQ', 'LQ') else vi(args[0])
    elif op in ('LQI', 'SQI', 'LQD', 'SQD'):
        d['a'] = vf(args[0]); d['b'] = vi(re.search(r'vi\d+', args[1]).group(0))
    elif op in ('ILWR', 'ISWR'):
        d['a'] = vi(args[0]); d['b'] = vi(re.search(r'vi\d+', args[1]).group(0))
    elif op in ('IADDIU', 'ISUBIU', 'IADDI'):
        d['a'] = vi(args[0]); d['b'] = vi(args[1]); d['imm'] = lab(args[2])
    elif op in ('IADD', 'IAND', 'IOR', 'ISUB'):
        d['a'], d['b'], d['c'] = vi(args[0]), vi(args[1]), vi(args[2])
    elif op in ('MOVE', 'MR32'):
        d['a'], d['b'] = vf(args[0]), vf(args[1])
    elif op in ('MFIR',):
        d['a'], d['b'] = vf(args[0]), vi(args[1])
    elif op in ('MTIR',):
        d['a'] = vi(args[0]); d['b'] = vf(args[1]); d['fsf'] = COMP[args[1][-1]]
    elif op in ('DIV', 'RSQRT'):
        d['b'] = vf(args[1]); d['fsf'] = COMP[args[1][-1]]
        d['c'] = vf(args[2]); d['ftf'] = COMP[args[2][-1]]
    elif op == 'SQRT':
        d['c'] = vf(args[1]); d['ftf'] = COMP[args[1][-1]]
    elif op == 'FMAND':
        d['a'], d['b'] = vi(args[0]), vi(args[1])
    elif op in ('FCGET', 'XTOP', 'XITOP'):
        d['a'] = vi(args[0])
    elif op in ('FCOR', 'FCAND', 'FCEQ', 'FCSET'):
        if op == 'FCSET':
            d['imm'] = int(args[0], 0)
        else:
            d['a'] = vi(args[0]); d['imm'] = int(args[1], 0)
    elif op in ('IBEQ', 'IBNE'):
        d['a'], d['b'] = vi(args[0]), vi(args[1]); d['imm'] = labels[args[2]]
    elif op in ('IBLEZ', 'IBGTZ', 'IBLTZ', 'IBGEZ'):
        d['a'] = vi(args[0]); d['imm'] = labels[args[1]]
    elif op == 'B':
        d['imm'] = labels[args[0]]
    elif op == 'BAL':
        d['a'] = vi(args[0]); d['imm'] = labels[args[1]]
    elif op == 'JR':
        d['b'] = vi(args[0])
    elif op == 'JALR':
        d['a'], d['b'] = vi(args[0]), vi(args[1])
    elif op == 'XGKICK':
        d['b'] = vi(args[0])
    elif op in ('WAITQ', 'WAITP'):
        pass
    elif op == 'LOI':
        d['fimm'] = struct.unpack('<f', bytes.fromhex(hexw[:8]))[0]
        assert d['fimm'] == float(args[0]), (txt, d['fimm'])
    else:
        raise SystemExit('unhandled lower op: ' + txt)
    return d

# sanity: IADDIU label immediates must match the encoding
for a, (h, u, l) in slots.items():
    m = re.match(r'IADDIU\s+vi\d+,\s*vi\d+,\s*([A-Za-z_]\w*)', l)
    if m:
        lo = int.from_bytes(bytes.fromhex(h[:8]), 'little')
        assert (lo & 0x7FF) | ((lo >> 10) & 0x7800) == labels[m.group(1)], (l, hex(lo))

size = max(slots) + 1
out = ['/* Generated by pc/tools/vsm2inc.py from %s - do not edit. */' % os.path.basename(src)]
out.append('#define %s_MICRO_COUNT %d' % (PFX, size))
out.append('static const VuMicroInstr %s_micro_code[%s_MICRO_COUNT] = {' % (prefix, PFX))
for a in range(size):
    if a not in slots:
        out.append('  /* %4d */ { U_NOP, 0, 0, 0, 0, BC_NONE, 1, L_NOP, 0, 0, 0, 0, 0, 0, 0, 0.0f }, /* (not loaded) */' % a)
        continue
    h, u, l = slots[a]
    U, L = upper(u), lower(l, h)
    fimm = repr(L['fimm']) + 'f' if L['fimm'] is not None else '0.0f'
    out.append("  /* %4d */ { %s, %d, %d, %d, %d, %s, %d, %s, %d, %d, %d, %d, %d, %d, %d, %s }, /* %s | %s */" % (
        a, U['op'], U['mask'], U['fd'], U['fs'], U['ft'], U['bc'], int(U['e']),
        L['op'], L['mask'], L['a'], L['b'], L['c'], L['imm'], L['fsf'], L['ftf'], fimm, u, l))
out.append('};')
open(dst, 'w').write('\n'.join(out) + '\n')

lab = ['/* Generated by pc/tools/vsm2inc.py from %s - microprogram entry addresses. */' % os.path.basename(src), '#pragma once']
for name, a in sorted(labels.items(), key=lambda x: x[1]):
    lab.append('#define %s_ADDR_%s %d' % (PFX, name, a))
open(os.path.join(os.path.dirname(dst), '%s_labels.h' % prefix), 'w').write('\n'.join(lab) + '\n')
print('%s: %d slots, %d labels' % (prefix, len(slots), len(labels)))
