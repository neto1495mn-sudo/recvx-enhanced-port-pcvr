#!/usr/bin/env python3
"""
asm2c.py - translate the decomp's inline EE/VU0 asm blocks into portable C.

Each asm statement in a source file is rewritten as

    #ifndef TARGET_PC
    <original asm, untouched>
    #else
    { <generated C> }
    #endif

so the decomp keeps building for PS2 and the PC build gets C with identical
semantics.  The generated code runs on the VU0 model in pc/src/ps2/vu0.[ch]
and the EE register helpers in pc/src/ps2/ee_asm.h.

Usage:
    asm2c.py [--check] file.c [file.c ...]     rewrite files in place
    asm2c.py --dump file.c                     print translations only

Blocks the translator cannot handle (calls via jal, ABI registers that are
not bound to C operands, COP0 waits, ...) abort with a message; those are
translated by hand.
"""
import re, sys

# ---------------------------------------------------------------- registers
GPR_NAMES = ['zero', 'at', 'v0', 'v1', 'a0', 'a1', 'a2', 'a3',
             't0', 't1', 't2', 't3', 't4', 't5', 't6', 't7',
             's0', 's1', 's2', 's3', 's4', 's5', 's6', 's7',
             't8', 't9', 'k0', 'k1', 'gp', 'sp', 'fp', 'ra']
GPR = {n: i for i, n in enumerate(GPR_NAMES)}
GPR.update({'a4': 8, 'a5': 9, 'a6': 10, 'a7': 11, 's8': 30})
for i in range(32):
    GPR[f'${i}'] = i
    GPR['r%d' % i] = GPR.get('r%d' % i, i)
ABI_ONLY = {'a0', 'a1', 'a2', 'a3', 'v0', 'v1', 'sp', 'ra', 'gp', 'fp', 's0', 's1', 's2', 's3', 's4', 's5', 's6', 's7', 's8'}

class TranslateError(Exception):
    pass

def strip_dollar(s):
    return s[1:] if s.startswith('$') else s

# ---------------------------------------------------------------- block discovery
def find_blocks(src):
    """Yield (start, end, kind, body, operands) for each asm statement."""
    i = 0
    pat = re.compile(r'\b(__asm__|asm)\b(\s+(volatile|__volatile__))?\s*([({])')
    while True:
        m = pat.search(src, i)
        if not m:
            return
        # skip matches inside comments
        line_start = src.rfind('\n', 0, m.start()) + 1
        if '//' in src[line_start:m.start()]:
            i = m.end(); continue
        cstart = src.rfind('/*', 0, m.start()); cend = src.rfind('*/', 0, m.start())
        if cstart > cend:
            i = m.end(); continue
        opener = m.group(4)
        j = m.end()
        if opener == '{':
            depth = 1
            while depth:
                if src[j] == '{': depth += 1
                elif src[j] == '}': depth -= 1
                j += 1
            body = src[m.end():j - 1]
            end = j
            # optional trailing ';'
            k = end
            while k < len(src) and src[k] in ' \t': k += 1
            if k < len(src) and src[k] == ';': end = k + 1
            yield (m.start(), end, 'brace', body, None)
        else:
            # gcc style: ( "template" : out : in : clobber ) ;
            depth = 1; in_str = False; parts = ['']
            while depth:
                ch = src[j]
                if in_str:
                    if ch == '"': in_str = False
                    parts[-1] += ch
                elif ch == '"':
                    in_str = True; parts[-1] += ch
                elif ch == '(':
                    depth += 1; parts[-1] += ch
                elif ch == ')':
                    depth -= 1
                    if depth: parts[-1] += ch
                elif ch == ':' and depth == 1:
                    parts.append('')
                else:
                    parts[-1] += ch
                j += 1
            k = j
            while k < len(src) and src[k] in ' \t\r\n': k += 1
            end = k + 1 if k < len(src) and src[k] == ';' else j
            template = ''.join(re.findall(r'"([^"]*)"', parts[0], re.S))
            ops = parts[1:]
            yield (m.start(), end, 'gcc', template, ops)
        i = end

def parse_operands(ops):
    """Return list of dicts {constraint, expr, out}; outputs first, as numbered by gcc."""
    res = []
    for sec_idx, sec in enumerate(ops[:2]):
        sec = sec.strip()
        if not sec:
            continue
        # split top-level commas
        depth = 0; cur = ''; items = []
        for ch in sec:
            if ch == '(':
                depth += 1
            elif ch == ')':
                depth -= 1
            if ch == ',' and depth == 0:
                items.append(cur); cur = ''
            else:
                cur += ch
        if cur.strip(): items.append(cur)
        for it in items:
            m = re.match(r'\s*"([^"]*)"\s*\((.*)\)\s*$', it, re.S)
            if not m:
                raise TranslateError('bad operand: ' + it)
            c, e = m.group(1), m.group(2).strip()
            res.append(dict(constraint=c, expr=e, out=(sec_idx == 0)))
    return res

# ---------------------------------------------------------------- instruction parsing
def split_args(s):
    out = []; depth = 0; cur = ''
    for ch in s:
        if ch == '(':
            depth += 1
        elif ch == ')':
            depth -= 1
        if ch == ',' and depth == 0:
            out.append(cur.strip()); cur = ''
        else:
            cur += ch
    if cur.strip(): out.append(cur.strip())
    return out

def parse_lines(text):
    items = []
    for raw in text.replace('\\n', '\n').split('\n'):
        line = re.sub(r'//.*', '', raw)
        line = re.sub(r'/\*.*?\*/', '', line)
        line = re.sub(r'#.*', '', line).strip().rstrip(';').strip()
        while line:
            m = re.match(r'^([A-Za-z_.$][\w.$]*)\s*:\s*(.*)$', line)
            if m and not m.group(1).startswith('.'):
                items.append(('label', m.group(1)))
                line = m.group(2).strip()
                continue
            break
        if not line:
            continue
        if line.startswith('.'):
            items.append(('dir', line))
            continue
        m = re.match(r'^([a-z][a-z0-9.]*)\s*(.*)$', line)
        if not m:
            raise TranslateError('cannot parse line: ' + raw)
        items.append(('insn', m.group(1), split_args(m.group(2))))
    return items

# ---------------------------------------------------------------- translator
FIELDS = {'x': 8, 'y': 4, 'z': 2, 'w': 1}
COMP = {'x': 0, 'y': 1, 'z': 2, 'w': 3}

BRANCHES = {'beq', 'bne', 'beqz', 'bnez', 'bgtz', 'bltz', 'bgez', 'blez', 'ble', 'bgt', 'bge', 'blt', 'b', 'j',
            'beql', 'bnel', 'bc0t', 'bc0f', 'jr', 'jal'}

class Block:
    def __init__(self, kind, body, operands, cvars, func, clabels=(), noreorder_default=False, float_params=()):
        self.kind = kind
        self.items = parse_lines(body)
        self.operands = operands or []
        self.cvars = cvars           # names of C locals/params visible (brace style)
        self.func = func
        self.gprs = set()            # GPR numbers used
        self.fprs = set()
        self.bound = {}              # name -> ('op', idx) | ('cvar', name)
        self.bound_kind = {}         # key -> 'int' | 'float'
        self.written = set()
        self.labels = {it[1] for it in self.items if it[0] == 'label'} | set(clabels)
        self.noreorder_default = noreorder_default
        self.float_params = list(float_params)
        self.implicit_return = None   # set by the driver: return type if the function ends with this block
        self.out = []
        self.scratch_ok, self.fscratch_ok = self.scratch_analysis()
        self.ind = '    '

    # ---- operand resolution
    def reg(self, tok, write=False, want='int'):
        """Return C lvalue/expression for a GPR-ish operand (ee_reg)."""
        t = strip_dollar(tok.strip())
        m = re.match(r'^%(\d+)$', t)
        if m:
            idx = int(m.group(1))
            if idx >= len(self.operands):
                raise TranslateError('operand %' + m.group(1) + ' out of range')
            key = 'op%d' % idx
            self.bound[key] = ('op', idx)
            if 'f' in self.operands[idx]['constraint'].replace('=', ''):
                self.bound_kind[key] = 'float'
            else:
                self.bound_kind.setdefault(key, 'int')
            if write: self.written.add(key)
            return key
        if self.kind == 'brace' and t in self.cvars and re.match(r'^[A-Za-z_]\w*$', t):
            key = 'cv_' + t
            self.bound[key] = ('cvar', t)
            self.bound_kind.setdefault(key, 'int')
            if write: self.written.add(key)
            return key
        if t in GPR:
            n = GPR[t]
            if n == 0:
                return 'ee_zero' if not write else 'ee_dummy'
            if t in ABI_ONLY and n not in self.scratch_ok:
                raise TranslateError('ABI register %s read before written (%s block)' % (t, self.kind))
            self.gprs.add(n)
            return 'R%d' % n
        if re.match(r'^f\d+$', t):
            raise TranslateError('FPR %s used as GPR' % t)
        if self.kind == 'brace' and re.match(r'^[A-Za-z_]\w*$', t):
            key = 'cv_' + t
            self.bound[key] = ('cvar', t)
            self.bound_kind.setdefault(key, 'int')
            if write: self.written.add(key)
            return key
        raise TranslateError('unknown register ' + tok)

    def freg(self, tok, write=False):
        t = strip_dollar(tok.strip())
        m = re.match(r'^%(\d+)$', t)
        if m:
            idx = int(m.group(1)); key = 'op%d' % idx
            self.bound[key] = ('op', idx); self.bound_kind[key] = 'float'
            if write: self.written.add(key)
            return key + '.f[0]'
        m = re.match(r'^f(\d+)$', t)
        if m:
            n = int(m.group(1))
            if n >= 12 and n <= 19 and ('F%d' % n) not in self.fscratch_ok:
                if n - 12 < len(self.float_params) and not write:
                    return '(float)(%s)' % self.float_params[n - 12]
                raise TranslateError('FPR argument register f%d read before written' % n)
            if n == 0 and write:
                raise TranslateError('writes FPR return register f0')
            self.fprs.add(n)
            return 'F%d' % n
        if self.kind == 'brace' and re.match(r'^[A-Za-z_]\w*$', t):
            key = 'cv_' + t
            self.bound[key] = ('cvar', t); self.bound_kind[key] = 'float'
            if write: self.written.add(key)
            return key + '.f[0]'
        raise TranslateError('unknown FPR ' + tok)

    def scratch_analysis(self):
        """ABI registers / FPR arg registers whose first use (in text order) is a write."""
        first = {}
        for it in self.items:
            if it[0] != 'insn':
                continue
            op, args = it[1], it[2]
            toks = [strip_dollar(x.strip()) for x in args]
            # destination is the first operand except for stores/branches/moves-to-coprocessor
            dst_first = op not in ('sw', 'sh', 'sb', 'sd', 'sq', 'sdl', 'sdr', 'swc1', 'sqc2', 'qmtc2', 'qmtc2.ni', 'qmtc2.i',
                                   'ctc2', 'ctc2.ni', 'ctc2.i', 'mtc1') and op not in BRANCHES
            for k, tk in enumerate(toks):
                regs = re.findall(r'\(?\$?([a-z]+\d*)\)?', tk)
                for r in regs:
                    key = None
                    if r in GPR and r in ABI_ONLY:
                        key = GPR[r]
                    elif re.match(r'^f\d+$', r):
                        key = 'F' + r[1:]
                    if key is None or key in first:
                        continue
                    is_write = (k == 0 and dst_first and '(' not in tk)
                    if op == 'mtc1' and k == 1: is_write = True
                    first[key] = is_write
        return ({k for k, w in first.items() if w and isinstance(k, int)},
                {k for k, w in first.items() if w and isinstance(k, str)})

    def vf(self, tok):
        t = strip_dollar(tok.strip())
        m = re.match(r'^vf(\d+)[xyzw]*$', t)
        if not m:
            raise TranslateError('bad vf ' + tok)
        return int(m.group(1))

    def vi(self, tok):
        t = strip_dollar(tok.strip())
        m = re.match(r'^vi(\d+)$', t)
        if not m:
            raise TranslateError('bad vi ' + tok)
        return int(m.group(1))

    def imm(self, tok):
        t = tok.strip()
        m = re.match(r'^([A-Za-z_]\w*)\.([A-Za-z_][\w.\[\]]*)$', t)
        if m:
            return 'offsetof(%s, %s)' % (m.group(1), m.group(2))
        try:
            return str(int(t, 0))
        except ValueError:
            pass
        m = re.match(r'^-\s*(0x[0-9a-fA-F]+|\d+)$', t)
        if m:
            return str(-int(m.group(1), 0))
        if re.match(r'^[\w\s()+\-*/<>|&~^]+$', t) and not self.is_reg(t):
            return '(%s)' % t      # C constant expression (sizeof, shifts, ...)
        raise TranslateError('bad immediate ' + tok)

    def mem(self, tok):
        m = re.match(r'^(.*)\((.+)\)$', tok.strip())
        if not m:
            t = tok.strip()
            if re.match(r'^[A-Za-z_]\w*$', t) and t not in GPR:
                return '(uint32_t)(uintptr_t)&%s' % t      # absolute address of a global
            raise TranslateError('bad memory operand ' + tok)
        off = m.group(1).strip() or '0'
        base = self.reg(m.group(2))
        return '(%s.w[0] + (uint32_t)(%s))' % (base, self.imm(off))

    def emit(self, s):
        self.out.append(self.ind + s)

    # ---- translation
    def translate(self):
        mode_noreorder = self.noreorder_default
        items = self.items
        i = 0
        body = []
        self.out = body
        while i < len(items):
            it = items[i]
            if it[0] == 'label':
                body.append('%s:;' % it[1])
                i += 1; continue
            if it[0] == 'dir':
                d = it[1].replace(' ', '')
                if d == '.setnoreorder': mode_noreorder = True
                elif d == '.setreorder': mode_noreorder = False
                elif d.startswith('.set') or d.startswith('.align'): pass
                else: raise TranslateError('directive ' + it[1])
                i += 1; continue
            op, args = it[1], it[2]
            if op in BRANCHES:
                # find delay slot (next insn) when noreorder
                delay = None
                if mode_noreorder:
                    k = i + 1
                    while k < len(items) and items[k][0] == 'dir': k += 1
                    if k < len(items) and items[k][0] == 'insn':
                        delay = items[k]
                        if delay[1] in BRANCHES:
                            raise TranslateError('branch in delay slot')
                        i = k
                    elif k < len(items) and items[k][0] == 'label':
                        raise TranslateError('label in delay slot after ' + op)
                self.branch(op, args, delay)
                i += 1; continue
            self.insn(op, args)
            i += 1
        return body

    def branch(self, op, args, delay):
        def cond_expr():
            if op in ('b', 'j'):
                return None
            if op in ('beqz', 'bnez', 'bgtz', 'bltz', 'bgez', 'blez'):
                r = self.reg(args[0]); tgt = args[1]
                c = {'beqz': '==', 'bnez': '!=', 'bgtz': '>', 'bltz': '<', 'bgez': '>=', 'blez': '<='}[op]
                return '(%s.sd[0] %s 0)' % (r, c), tgt
            if op in ('beq', 'bne', 'ble', 'bgt', 'bge', 'blt'):
                a = self.reg(args[0])
                bt = strip_dollar(args[1])
                if re.match(r'^-?(0x)?[0-9a-fA-F]+$', bt):
                    b = '(int64_t)(%s)' % self.imm(bt)
                else:
                    b = self.reg(args[1]) + '.sd[0]'
                c = {'beq': '==', 'bne': '!=', 'ble': '<=', 'bgt': '>', 'bge': '>=', 'blt': '<'}[op]
                return '(%s.sd[0] %s %s)' % (a, c, b), args[2]
            raise TranslateError('unsupported branch ' + op)
        if op in ('bc0t', 'bc0f'):
            # COP0 condition = "DMA idle"; DMA completes synchronously on PC
            if delay: self.insn(delay[1], delay[2])
            if op == 'bc0t':
                self.emit('goto %s;' % args[0])
            return
        if op in ('jr', 'jal', 'beql', 'bnel'):
            raise TranslateError('unsupported control flow ' + op)
        if op in ('b', 'j'):
            tgt = args[0]
            if tgt not in self.labels:
                raise TranslateError('branch to unknown label ' + tgt)
            if delay: self.insn(delay[1], delay[2])
            self.emit('goto %s;' % tgt)
            return
        cond, tgt = cond_expr()
        if tgt not in self.labels:
            raise TranslateError('branch to unknown label ' + tgt)
        if delay:
            self.emit('ee_cond = %s;' % cond)
            self.insn(delay[1], delay[2])
            self.emit('if (ee_cond) goto %s;' % tgt)
        else:
            self.emit('if %s goto %s;' % (cond, tgt))

    def set32(self, rd, expr):
        self.emit('%s.sd[0] = (int32_t)(%s);' % (rd, expr))

    def insn(self, op, a):
        R = lambda k, w=False: self.reg(a[k], write=w)
        base = op.split('.')[0]
        suffix = op[len(base):]

        # ---------- COP2 / VU0 macro ----------
        if op.startswith('v') and op not in ('vnop',):
            return self.vu(op, a)
        if op in ('vnop', 'nop', 'sync', 'sync.l', 'sync.p'):
            return
        if base == 'qmtc2':
            return self.emit('vu_load_vf(%d, &%s);' % (self.vf(a[1]), R(0)))
        if base == 'qmfc2':
            return self.emit('vu_store_vf(&%s, %d);' % (R(0, True), self.vf(a[1])))
        if base == 'cfc2':
            return self.set32(R(0, True), 'vu_cfc2(%d)' % self.vi_ctl(a[1]))
        if base == 'ctc2':
            return self.emit('vu_ctc2(%d, %s.w[0]);' % (self.vi_ctl(a[1]), R(0)))
        if op == 'lqc2':
            return self.emit('vu_load_vf(%d, EE_PTR(%s));' % (self.vf(a[0]), self.mem(a[1])))
        if op == 'sqc2':
            return self.emit('vu_store_vf(EE_PTR(%s), %d);' % (self.mem(a[1]), self.vf(a[0])))

        # ---------- FPU ----------
        if op == 'mfc1':
            return self.set32(R(0, True), 'ee_f2u(%s)' % self.freg(a[1]))
        if op == 'mtc1':
            return self.emit('%s = ee_u2f(%s.w[0]);' % (self.freg(a[1], True), R(0)))
        if op == 'lwc1':
            return self.emit('%s = EE_LWF(%s);' % (self.freg(a[0], True), self.mem(a[1])))
        if op == 'swc1':
            return self.emit('EE_SWF(%s, %s);' % (self.mem(a[1]), self.freg(a[0])))
        if op in ('add.s', 'sub.s', 'mul.s', 'div.s'):
            o = {'add.s': '+', 'sub.s': '-', 'mul.s': '*', 'div.s': '/'}[op]
            return self.emit('%s = vu_clampf(%s %s %s);' % (self.freg(a[0], True), self.freg(a[1]), o, self.freg(a[2])))
        if op == 'neg.s':
            return self.emit('%s = -%s;' % (self.freg(a[0], True), self.freg(a[1])))
        if op == 'abs.s':
            return self.emit('%s = fabsf(%s);' % (self.freg(a[0], True), self.freg(a[1])))
        if op == 'mov.s':
            return self.emit('%s = %s;' % (self.freg(a[0], True), self.freg(a[1])))
        if op == 'cvt.w.s':
            return self.emit('%s = ee_u2f((uint32_t)(int32_t)%s);' % (self.freg(a[0], True), self.freg(a[1])))
        if op == 'cvt.s.w':
            return self.emit('%s = (float)(int32_t)ee_f2u(%s);' % (self.freg(a[0], True), self.freg(a[1])))

        # ---------- integer ----------
        if op in ('add', 'addu', 'addi', 'addiu', 'sub', 'subu'):
            rd = R(0, True); rs = R(1)
            if op in ('addi', 'addiu') or not self.is_reg(a[2]):
                v = '(uint32_t)(%s)' % self.imm(a[2])
                if op.startswith('sub'): v = '-' + v
            else:
                v = R(2) + '.w[0]'
                if op.startswith('sub'): v = '-' + v
            return self.set32(rd, '%s.w[0] + %s' % (rs, v))
        if op in ('dadd', 'daddu', 'daddi', 'daddiu', 'dsub', 'dsubu'):
            rd = R(0, True); rs = R(1)
            v = '(int64_t)(%s)' % self.imm(a[2]) if (op in ('daddi', 'daddiu') or not self.is_reg(a[2])) else R(2) + '.sd[0]'
            o = '-' if 'sub' in op else '+'
            return self.emit('%s.sd[0] = %s.sd[0] %s %s;' % (rd, rs, o, v))
        if op in ('and', 'or', 'xor', 'nor', 'andi', 'ori', 'xori'):
            rd = R(0, True); rs = R(1)
            if op.endswith('i') or not self.is_reg(a[2]):
                v = '(uint64_t)(uint16_t)(%s)' % self.imm(a[2])
            else:
                v = R(2) + '.d[0]'
            o = {'and': '&', 'or': '|', 'xor': '^', 'nor': '|'}[op.rstrip('i') if op != 'nor' else 'nor']
            e = '%s.d[0] %s %s' % (rs, o, v)
            if op == 'nor': e = '~(%s)' % e
            return self.emit('%s.d[0] = %s;' % (rd, e))
        if op in ('sll', 'srl', 'sra'):
            rd = R(0, True); rt = R(1); sa = self.imm(a[2])
            e = {'sll': '%s.w[0] << %s', 'srl': '%s.w[0] >> %s', 'sra': '(uint32_t)(%s.sw[0] >> %s)'}[op] % (rt, sa)
            return self.set32(rd, e)
        if op in ('sllv', 'srlv', 'srav'):
            rd = R(0, True); rt = R(1); rs = R(2)
            e = {'sllv': '%s.w[0] << (%s.w[0] & 31)', 'srlv': '%s.w[0] >> (%s.w[0] & 31)', 'srav': '(uint32_t)(%s.sw[0] >> (%s.w[0] & 31))'}[op] % (rt, rs)
            return self.set32(rd, e)
        if op in ('dsll', 'dsrl', 'dsra', 'dsll32', 'dsrl32', 'dsra32'):
            rd = R(0, True); rt = R(1); sa = int(self.imm(a[2])) + (32 if op.endswith('32') else 0)
            e = {'dsll': '%s.d[0] << %d', 'dsrl': '%s.d[0] >> %d', 'dsra': '(uint64_t)(%s.sd[0] >> %d)'}[op.rstrip('32')] % (rt, sa)
            return self.emit('%s.d[0] = %s;' % (rd, e))
        if op in ('slt', 'sltu', 'slti', 'sltiu'):
            rd = R(0, True); rs = R(1)
            if op.endswith('i') or op.endswith('iu') and not self.is_reg(a[2]):
                b = '(int64_t)(%s)' % self.imm(a[2])
            else:
                b = R(2) + '.sd[0]'
            if 'u' in op[3:]:
                return self.emit('%s.d[0] = %s.d[0] < (uint64_t)%s;' % (rd, rs, b))
            return self.emit('%s.d[0] = %s.sd[0] < %s;' % (rd, rs, b))
        if op == 'lui':
            return self.set32(R(0, True), '(uint32_t)(%s) << 16' % self.imm(a[1]))
        if op == 'li':
            return self.emit('%s.sd[0] = (int64_t)(int32_t)(%s);' % (R(0, True), self.imm(a[1])))
        if op == 'move':
            return self.emit('%s.d[0] = %s.d[0];' % (R(0, True), R(1)))
        if op == 'neg':
            return self.set32(R(0, True), '0u - %s.w[0]' % R(1))
        if op == 'muli':
            return self.set32(R(0, True), '%s.w[0] * (uint32_t)(%s)' % (R(1), self.imm(a[2])))
        if op in ('mult', 'multu'):
            if len(a) == 3:
                rd, rs, rt = R(0, True), R(1), R(2)
            else:
                rd, rs, rt = None, R(0), R(1)
            t = 'int64_t' if op == 'mult' else 'uint64_t'
            c = 'int32_t' if op == 'mult' else 'uint32_t'
            self.emit('ee_prod = (uint64_t)((%s)(%s)%s.w[0] * (%s)(%s)%s.w[0]);' % (t, c, rs, t, c, rt))
            self.emit('ee_lo.sd[0] = (int32_t)(uint32_t)ee_prod; ee_hi.sd[0] = (int32_t)(uint32_t)(ee_prod >> 32);')
            if rd: self.emit('%s.sd[0] = ee_lo.sd[0];' % rd)
            return
        if op == 'mflo':
            return self.emit('%s.d[0] = ee_lo.d[0];' % R(0, True))
        if op == 'mfhi':
            return self.emit('%s.d[0] = ee_hi.d[0];' % R(0, True))
        if op in ('movz', 'movn'):
            c = '==' if op == 'movz' else '!='
            return self.emit('if (%s.d[0] %s 0) %s.d[0] = %s.d[0];' % (R(2), c, R(0, True), R(1)))
        # loads / stores
        LD = {'lb': 'EE_LB', 'lbu': 'EE_LBU', 'lh': 'EE_LH', 'lhu': 'EE_LHU', 'lw': 'EE_LW', 'lwu': 'EE_LWU'}
        if op in LD:
            return self.emit('%s.sd[0] = %s(%s);' % (R(0, True), LD[op], self.mem(a[1])))
        if op == 'ld':
            return self.emit('%s.d[0] = EE_LD(%s);' % (R(0, True), self.mem(a[1])))
        if op == 'lq':
            return self.emit('ee_lq(&%s, %s);' % (R(0, True), self.mem(a[1])))
        if op in ('ldl', 'ldr'):
            return self.emit('%s.d[0] = ee_%s(%s.d[0], %s);' % (R(0, True), op, R(0), self.mem(a[1])))
        ST = {'sb': 'EE_SB', 'sh': 'EE_SH', 'sw': 'EE_SW'}
        if op in ST:
            return self.emit('%s(%s, %s.d[0]);' % (ST[op], self.mem(a[1]), R(0)))
        if op == 'sd':
            return self.emit('EE_SD(%s, %s.d[0]);' % (self.mem(a[1]), R(0)))
        if op == 'sq':
            return self.emit('ee_sq(%s, &%s);' % (self.mem(a[1]), R(0)))
        if op in ('sdl', 'sdr'):
            return self.emit('ee_%s(%s, %s.d[0]);' % (op, self.mem(a[1]), R(0)))
        # MMI
        if op in ('pcpyld', 'pcpyud', 'pextlw', 'pextuw', 'pextlh', 'paddub', 'paddw', 'psubw', 'pand', 'por', 'pxor',
                  'pcpyh', 'pexew', 'ppacw', 'pextlb', 'pextub', 'pmaxw', 'pminw'):
            rd = R(0, True); rs = R(1); rt = R(2)
            return self.emit('ee_%s(&%s, &%s, &%s);' % (op, rd, rs, rt))
        if op in ('prot3w', 'pcpyh', 'plzcw', 'pexcw', 'pexch', 'prevh', 'pabsw'):
            return self.emit('ee_%s(&%s, &%s);' % (op, R(0, True), R(1)))
        raise TranslateError('unsupported instruction %s %s' % (op, ', '.join(a)))

    def is_reg(self, tok):
        t = strip_dollar(tok.strip())
        if t in GPR or re.match(r'^%\d+$', t):
            return True
        if self.kind == 'brace' and re.match(r'^[A-Za-z_]\w*$', t) and not re.match(r'^[A-Z_][A-Z0-9_]*$', t):
            return True
        return False

    def vi_ctl(self, tok):
        t = strip_dollar(tok.strip())
        m = re.match(r'^(vi|\$)?(\d+)$', t)
        if not m:
            raise TranslateError('bad control reg ' + tok)
        return int(m.group(2))

    # ---------- VU0 macro instructions
    def vu(self, op, a):
        base = op.split('.')[0]
        fields = op.split('.')[1] if '.' in op else 'xyzw'
        mask = sum(FIELDS[c] for c in fields) if fields and all(c in FIELDS for c in fields) else 15
        if base == 'vcallms':
            return self.emit('vu0_callms(VU0_ADDR_%s);' % a[0].strip())
        if base == 'vcallmsr':
            return self.emit('vu0_callms(vu_cfc2(27));')
        if base in ('vwaitq',):
            return
        if base == 'vmove':
            return self.emit('vu_move(%d, %d, %d);' % (mask, self.vf(a[0]), self.vf(a[1])))
        if base == 'vmr32':
            return self.emit('vu_mr32(%d, %d, %d);' % (mask, self.vf(a[0]), self.vf(a[1])))
        if base in ('vdiv', 'vrsqrt'):
            fs, ft = strip_dollar(a[1]), strip_dollar(a[2])
            fn = 'vu_div' if base == 'vdiv' else 'vu_rsqrt'
            return self.emit('%s(%d, %d, %d, %d);' % (fn, self.vf(fs), COMP.get(fs[-1], 0), self.vf(ft), COMP.get(ft[-1], 0)))
        if base == 'vsqrt':
            ft = strip_dollar(a[1])
            return self.emit('vu_sqrt(%d, %d);' % (self.vf(ft), COMP.get(ft[-1], 0)))
        if base == 'vclipw':
            return self.emit('vu_clipw(%d, %d);' % (self.vf(a[0]), self.vf(a[1])))
        if base == 'vmtir':
            fs = strip_dollar(a[1])
            return self.emit('vu_mtir(%d, %d, %d);' % (self.vi(a[0]), self.vf(fs), COMP[fs[-1]] if fs[-1] in COMP else 0))
        if base == 'vmfir':
            return self.emit('vu_mfir(%d, %d, %d);' % (mask, self.vf(a[0]), self.vi(a[1])))
        if base in ('viaddi', 'viadd', 'viand', 'vior', 'visub'):
            d = self.vi(a[0]); s = self.vi(a[1])
            if base == 'viaddi':
                e = 'g_vu0.vi[%d] + (%s)' % (s, self.imm(a[2]))
            else:
                o = {'viadd': '+', 'viand': '&', 'vior': '|', 'visub': '-'}[base]
                e = 'g_vu0.vi[%d] %s g_vu0.vi[%d]' % (s, o, self.vi(a[2]))
            if d == 0:
                return
            return self.emit('g_vu0.vi[%d] = (uint16_t)(%s);' % (d, e))
        m = re.match(r'^v(ftoi|itof)(0|4|12|15)$', base)
        if m:
            u = 'U_%s%s' % (m.group(1).upper(), m.group(2))
            return self.emit('vu_upper(%s, %d, %d, %d, 0, BC_NONE);' % (u, mask, self.vf(a[0]), self.vf(a[1])))
        if base == 'vabs':
            return self.emit('vu_upper(U_ABS, %d, %d, %d, 0, BC_NONE);' % (mask, self.vf(a[0]), self.vf(a[1])))
        if base in ('vopmula', 'vopmsub'):
            u = 'U_OPMULA' if base == 'vopmula' else 'U_OPMSUB'
            fd = 0 if base == 'vopmula' else self.vf(a[0])
            return self.emit('vu_upper(%s, 14, %d, %d, %d, BC_NONE);' % (u, fd, self.vf(a[1]), self.vf(a[2])))
        m = re.match(r'^v(madd|msub|mul|add|sub|max|mini)(a?)([xyzwqi]?)$', base)
        if m:
            b, acc, bc = m.groups()
            if b in ('max', 'mini') and acc:
                raise TranslateError('bad op ' + op)
            u = 'U_' + b.upper() + ('A' if acc else '')
            bcs = {'': 'BC_NONE', 'x': 'BC_X', 'y': 'BC_Y', 'z': 'BC_Z', 'w': 'BC_W', 'q': 'BC_Q', 'i': 'BC_I'}[bc]
            if acc:
                fd = 0
                if strip_dollar(a[0]).upper() != 'ACC':
                    raise TranslateError('expected ACC in ' + op)
            else:
                fd = self.vf(a[0])
            fs = self.vf(a[1])
            ft = 0 if bc in ('q', 'i') else self.vf(a[2])
            return self.emit('vu_upper(%s, %d, %d, %d, %d, %s);' % (u, mask, fd, fs, ft, bcs))
        raise TranslateError('unsupported VU op ' + op)

    # ---------- wrap up
    def render(self):
        body = self.translate()
        decl = []
        regs = sorted(self.gprs)
        if regs:
            decl.append('ee_reg ' + ', '.join('R%d = {{0}}' % r for r in regs) + ';')
        if self.fprs:
            decl.append('float ' + ', '.join('F%d = 0.0f' % f for f in sorted(self.fprs)) + ';')
        joined = '\n'.join(body)
        if 'ee_cond' in joined: decl.append('int ee_cond;')
        if 'ee_prod' in joined: decl.append('uint64_t ee_prod;')
        if 'ee_lo' in joined or 'ee_hi' in joined: decl.append('ee_reg ee_lo = {{0}}, ee_hi = {{0}};')
        if 'ee_dummy' in joined: decl.append('ee_reg ee_dummy;')
        if 'ee_zero' in joined: decl.append('const ee_reg ee_zero = {{0}};')
        pre, post = [], []
        for key, (kind, ref) in sorted(self.bound.items()):
            fk = self.bound_kind.get(key) == 'float'
            if kind == 'op':
                o = self.operands[ref]
                expr = o['expr']
                c = o['constraint']
                if 'm' in c and '=' not in c:
                    # mwcc hands "m" operands to the asm as a register holding the
                    # object's value (used as 'add t2, %2, t1' on a pointer global)
                    decl.append('ee_reg %s = {{0}};' % key)
                    pre.append('%s.sd[0] = (int32_t)(uintptr_t)(%s);' % (key, expr))
                    continue
                decl.append('ee_reg %s = {{0}};' % key)
                is_lvalue = re.match(r'^[A-Za-z_][\w.\->\[\]]*$', expr) is not None
                if fk:
                    if not o['out'] or is_lvalue:
                        pre.append('%s.f[0] = (float)(%s);' % (key, expr))
                else:
                    if not o['out'] or is_lvalue:
                        pre.append('%s.sd[0] = (int32_t)(uintptr_t)(%s);' % (key, expr))
                if o['out']:
                    if fk:
                        post.append('%s = %s.f[0];' % (expr, key))
                    else:
                        post.append('%s = (__typeof__(%s))(uintptr_t)%s.w[0];' % (expr, expr, key))
            else:
                decl.append('ee_reg %s = {{0}};' % key)
                if fk:
                    pre.append('%s.f[0] = %s;' % (key, ref))
                    if key in self.written:
                        post.append('%s = %s.f[0];' % (ref, key))
                else:
                    pre.append('%s.sd[0] = (int32_t)(uintptr_t)(%s);' % (key, ref))
                    if key in self.written:
                        post.append('%s = (__typeof__(%s))(uintptr_t)%s.w[0];' % (ref, ref, key))
        lines = ['{ /* asm2c */']
        lines += ['    ' + d for d in decl]
        lines += ['    ' + p for p in pre]
        lines += body
        # trailing label needs a statement
        lines += ['    ' + p for p in post]
        if self.implicit_return and 2 in self.gprs:
            lines.append('    return (%s)R2.w[0]; /* the asm returned in v0 */' % self.implicit_return)
        lines.append('}')
        return '\n'.join(lines)

# ---------------------------------------------------------------- driver
# Per-block fixups, keyed by (file, function, index of asm block in function).
OVERRIDES = {
    # (none: bhObj005 block 1 was checked against the retail ELF - reorder mode is right)
}

def function_float_params(src, pos):
    """Names of the float/Float parameters of the enclosing function, in order (EABI: f12..f19)."""
    starts = [m.start() for m in re.finditer(r'^\{', src[:pos], re.M)]
    if not starts:
        return []
    head_start = max(src.rfind(';', 0, starts[-1]), src.rfind('}', 0, starts[-1]))
    head = src[head_start + 1:starts[-1]]
    m = re.search(r'\((.*)\)', head, re.S)
    if not m:
        return []
    out = []
    for prm in m.group(1).split(','):
        mm = re.match(r'\s*(?:const\s+)?(?:float|Float)\s+(\w+)\s*$', prm.strip())
        if mm:
            out.append(mm.group(1))
    return out

def implicit_return_type(src, start, end):
    """Return type if the enclosing function is non-void, the asm block is its last
    statement and no C 'return' follows (mwcc returned the value in v0)."""
    starts = [m.start() for m in re.finditer(r'^\{', src[:start], re.M)]
    if not starts:
        return None
    head_start = max(src.rfind(';', 0, starts[-1]), src.rfind('}', 0, starts[-1]))
    head = src[head_start + 1:starts[-1]].strip()
    m = re.match(r'^(?:static\s+)?([A-Za-z_][\w \*]*?)\s*\b[A-Za-z_]\w*\s*\(', head)
    if not m or m.group(1).strip() in ('void', ''):
        return None
    rest = src[end:src.find('\n}', end)]
    if re.search(r'\breturn\b', rest) or re.search(r'[A-Za-z_]', re.sub(r'#\w+.*', '', rest)):
        return None
    return m.group(1).strip()

def function_labels(src, pos):
    starts = [m.start() for m in re.finditer(r'^\{', src[:pos], re.M)]
    if not starts:
        return set()
    body = src[starts[-1]:]
    end = body.find('\n}')
    body = body[:end if end >= 0 else len(body)]
    return set(re.findall(r'^\s*([A-Za-z_]\w*)\s*:(?!:)\s*$', body, re.M)) - {'default'}

def enclosing_function(src, pos):
    """Crude: name and parameter/local identifiers of the function containing pos."""
    # find the last top-level '{' before pos (column 0)
    starts = [m.start() for m in re.finditer(r'^\{', src[:pos], re.M)]
    if not starts:
        return '', set()
    fstart = starts[-1]
    head_start = src.rfind(';', 0, fstart)
    head_start = max(head_start, src.rfind('}', 0, fstart))
    head = src[head_start + 1:fstart]
    body = src[fstart:]
    # identifiers declared/used by C code only (asm text removed)
    end = body.find('\n}')
    body = body[:end if end >= 0 else len(body)]
    body = re.sub(r'\basm\b\s*(volatile)?\s*\{.*?\n\s*\}', '', body, flags=re.S)
    body = re.sub(r'\basm\b\s*(volatile)?\s*\(.*?\);', '', body, flags=re.S)
    body = re.sub(r'//.*', '', body)
    ids = set(re.findall(r'[A-Za-z_]\w*', head + body))
    m = re.search(r'([A-Za-z_]\w*)\s*\(', head)
    return (m.group(1) if m else ''), ids

def process(path, dump=False):
    src = open(path, encoding='latin-1').read()
    pieces = []; last = 0; ok = 0; bad = 0; per_func = {}
    for start, end, kind, body, ops in find_blocks(src):
        func, ids = enclosing_function(src, start)
        # skip blocks already inside a '#ifndef TARGET_PC' region
        pre = src[:start]
        if pre.rfind('#ifndef TARGET_PC') > pre.rfind('#endif'):
            continue
        try:
            operands = parse_operands(ops) if kind == 'gcc' else []
            idx = per_func[func] = per_func.get(func, -1) + 1
            ov = OVERRIDES.get((path.split('/')[-1], func, idx), {})
            if kind == 'gcc' and not body.strip().replace('nop', '').strip():
                c = '{ /* asm2c */ }'
            else:
                b = Block(kind, body, operands, ids, func, function_labels(src, start), ov.get('noreorder', False), function_float_params(src, start))
                b.implicit_return = implicit_return_type(src, start, end)
                c = b.render()
            ok += 1
        except (TranslateError, KeyError, IndexError, ValueError) as e:
            bad += 1
            line = src.count('\n', 0, start) + 1
            print('%s:%d: [%s] cannot translate: %s' % (path, line, func, e), file=sys.stderr)
            continue
        if dump:
            print('// %s:%d %s' % (path, src.count('\n', 0, start) + 1, func)); print(c); print()
            continue
        # indentation of the asm statement
        ls = src.rfind('\n', 0, start) + 1
        indent = re.match(r'[ \t]*', src[ls:start]).group(0)
        cl = '\n'.join(indent + l if l else l for l in c.split('\n'))
        pieces.append(src[last:ls])
        pieces.append('#ifndef TARGET_PC\n')
        pieces.append(src[ls:end])
        pieces.append('\n#else\n' + cl + '\n#endif')
        last = end
    if not dump and ok:
        pieces.append(src[last:])
        out = ''.join(pieces)
        if not re.search(r'#include\s+"ee_asm.h"', out):
            out = '#ifdef TARGET_PC\n#include "ee_asm.h"\n#endif\n' + out
        open(path, 'w', encoding='latin-1').write(out)
    return ok, bad

if __name__ == '__main__':
    args = sys.argv[1:]
    dump = '--dump' in args
    args = [x for x in args if not x.startswith('--')]
    tot_ok = tot_bad = 0
    for p in args:
        ok, bad = process(p, dump)
        tot_ok += ok; tot_bad += bad
        print('%s: %d translated, %d failed' % (p, ok, bad), file=sys.stderr)
    print('total: %d translated, %d failed' % (tot_ok, tot_bad), file=sys.stderr)
