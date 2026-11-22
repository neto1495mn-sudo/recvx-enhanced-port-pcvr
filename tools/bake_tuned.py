"""Passes what is tuned on the headset into the code's defaults (the release carries no recvx.ini).

    python tools/bake_tuned.py            one pass: pull recvx.ini from the Quest, write the values into pc/glue/vr_glue.c
    python tools/bake_tuned.py --watch    the same every 15 s, printing a line whenever the code changed

What it bakes: the ammo guides (g_vr_gd, ini vr_gd<item>_x/y/z/yaw), the pistol slide (g_vr_sl, g_vr_sl_pull) and the holsters (g_vr_hol, ini vr_hol*_down/out/side/rot).
A value is written only when the ini has it set (a guide of 0 0 0 is "not set": the code's default stays).
The Quest: adb at 192.168.137.35:5555 (ADB env var: the adb.exe; SERIAL: the device).
"""
import os
import re
import subprocess
import sys
import tempfile
import time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
SRC = os.path.join(ROOT, 'pc', 'glue', 'vr_glue.c')
ADB = os.environ.get('ADB', 'C:/Users/Neto/AppData/Local/Android/Sdk/platform-tools/adb.exe')
SERIAL = os.environ.get('SERIAL', '192.168.137.35:5555')
INI = '/sdcard/Android/data/org.recvx.vr/files/recvx.ini'


def pull():
    tmp = os.path.join(tempfile.gettempdir(), 'recvx_bake.ini')
    env = dict(os.environ, MSYS_NO_PATHCONV='1')
    r = subprocess.run([ADB, '-s', SERIAL, 'pull', INI, tmp], capture_output=True, text=True, env=env)
    if r.returncode:
        subprocess.run([ADB, 'connect', SERIAL], capture_output=True, text=True, env=env)
        r = subprocess.run([ADB, '-s', SERIAL, 'pull', INI, tmp], capture_output=True, text=True, env=env)
    if r.returncode:
        return None
    return open(tmp, encoding='utf-8', errors='replace').read()


def parse(ini):
    d = {}
    for m in re.finditer(r'(?m)^\s*([A-Za-z0-9_]+)\s*=\s*(-?\d+)\s*$', ini):
        d[m.group(1)] = int(m.group(2))
    return d


def bake(ini):
    kv = parse(ini)
    s = open(SRC, encoding='utf-8', newline='').read()
    nl = '\r\n' if '\r\n' in s else '\n'
    changed = []
    # --- the ammo guides: the initializer of g_vr_gd
    a = s.index('int g_vr_gd[40][4] = {')
    b = s.index('};', a) + 2
    block = s[a:b]
    cur = {int(m.group(1)): [int(m.group(2)), int(m.group(3)), int(m.group(4)), int(m.group(5))]
           for m in re.finditer(r'\[(\d+)\] = \{ (-?\d+), (-?\d+), (-?\d+), (-?\d+) \}', block)}
    for it in range(1, 40):
        v = [kv.get('vr_gd%d_%s' % (it, k)) for k in ('x', 'y', 'z', 'yaw')]
        if None in v or not (v[0] or v[1] or v[2]):
            continue
        if cur.get(it) != v:
            changed.append('guide of item %d: %s -> %s' % (it, cur.get(it), v))
            cur[it] = v
    head = block[:block.index('= {') + 3]
    first_line_end = block.index('\n')
    comment = block[block.index('/*'):first_line_end] if '/*' in block[:first_line_end] else ''
    entries = ', '.join('[%d] = { %d, %d, %d, %d }' % (k, *cur[k]) for k in sorted(cur))
    new_block = 'int g_vr_gd[40][4] = {   ' + comment + nl + '    ' + entries + ' };'
    if new_block != block.replace('\r\n', nl):
        s = s[:a] + new_block + s[b:]
    # --- the holsters: g_vr_hol[standing, seated][single ammo, several kinds][down, forward, side, turn]
    keys = [['vr_hol', 'vr_hol2'], ['vr_hol_s', 'vr_hol2_s']]
    suff = ['_down', '_out', '_side', '_rot']
    a = s.index('int g_vr_hol[2][2][4] = ')
    b = s.index('};', a) + 2
    m = re.search(r'\{ \{ \{ (.*?) \} \} \}', s[a:b], re.S)
    nums = [int(x) for x in re.findall(r'-?\d+', s[a:b].split('=', 1)[1])]
    if len(nums) == 16:
        new = list(nums)
        for p in range(2):
            for k in range(2):
                for j in range(4):
                    key = keys[p][k] + suff[j]
                    if key in kv:
                        i = p * 8 + k * 4 + j
                        if new[i] != kv[key]:
                            changed.append('holster %s: %d -> %d' % (key, new[i], kv[key]))
                            new[i] = kv[key]
        if new != nums:
            it = iter(new)
            body = '{ ' + ', '.join('{ ' + ', '.join('{ %d, %d, %d, %d }' % tuple(next(it) for _ in range(4)) for _ in range(2)) + ' }' for _ in range(2)) + ' };'
            s = s[:a] + 'int g_vr_hol[2][2][4] = ' + body + s[b:]
    # --- the pistols' magazine guide's pitch (g_vr_gp)
    m = re.search(r'int g_vr_gp\[40\] = \{([^}]*)\};', s)
    if m:
        pit = {int(x): int(y) for x, y in re.findall(r'\[(\d+)\] = (-?\d+)', m.group(1))}
        for it in (5, 9):
            v = kv.get('vr_gd%d_pitch' % it)
            if v is not None and pit.get(it) != v:
                changed.append('guide pitch of item %d: %s -> %d' % (it, pit.get(it), v))
                pit[it] = v
        body = ', '.join('[%d] = %d' % (k, pit[k]) for k in sorted(pit))
        s = s.replace(m.group(0), 'int g_vr_gp[40] = { %s };' % body)
    # --- the pistol's slide: where the left grip takes it and how far it is pulled (g_vr_sl, g_vr_sl_pull)
    sl = [kv.get('vr_slide_x'), kv.get('vr_slide_y'), kv.get('vr_slide_z')]
    pl = kv.get('vr_slide_pull')
    m = re.search(r'int g_vr_sl\[3\] = \{ (-?\d+), (-?\d+), (-?\d+) \};', s)
    m2 = re.search(r'int g_vr_sl_pull = (-?\d+);', s)
    if m and m2 and None not in sl and pl is not None:
        old = [int(m.group(1)), int(m.group(2)), int(m.group(3))]
        if old != sl or int(m2.group(1)) != pl:
            changed.append('slide: grip %s pull %s -> grip %s pull %d' % (old, m2.group(1), sl, pl))
            s = s.replace(m.group(0), 'int g_vr_sl[3] = { %d, %d, %d };' % tuple(sl))
            s = s.replace(m2.group(0), 'int g_vr_sl_pull = %d;' % pl)
    if changed:
        open(SRC, 'w', encoding='utf-8', newline='').write(s)
    return changed


def once():
    ini = pull()
    if ini is None:
        print('bake: the Quest did not answer (adb)', flush=True)
        return None
    ch = bake(ini)
    print('bake: %s' % ('; '.join(ch) if ch else 'nothing new'), flush=True)
    return ch


if __name__ == '__main__':
    if '--watch' in sys.argv:
        while True:
            once()
            time.sleep(15)
    else:
        once()
