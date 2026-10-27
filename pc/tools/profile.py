#!/usr/bin/env python3
"""Summarise a RECVX_PROFILE sample file by function.
usage: profile.py <recvx binary> <samples file> [top N] [thread: r render, g game]"""
import bisect, collections, subprocess, sys

exe, prof = sys.argv[1], sys.argv[2]
top = int(sys.argv[3]) if len(sys.argv) > 3 else 25
thread = sys.argv[4] if len(sys.argv) > 4 else None
syms = []
for line in subprocess.run(['nm', '-n', '--defined-only', exe], capture_output=True, text=True).stdout.splitlines():
    parts = line.split()
    if len(parts) == 3 and parts[1] in 'tTwW' and not parts[2].startswith('.'):
        syms.append((int(parts[0], 16), parts[2]))
addrs = [a for a, _ in syms]
maps = []                           # <samples>.maps (/proc/self/maps at exit): library per address
try:
    for line in open(prof + '.maps'):
        p = line.split()
        if len(p) >= 6:
            lo, hi = (int(x, 16) for x in p[0].split('-'))
            maps.append((lo, hi, p[5].rsplit('/', 1)[-1]))
except OSError:
    pass
libs = collections.Counter()
hits, total, inside = collections.Counter(), 0, 0
for line in open(prof):
    p = line.split()
    if thread and p[1:] != [thread]:
        continue
    a = int(p[0], 16)
    total += 1
    if not addrs or a < addrs[0] or a >= 0x40000000:
        libs[next((n for lo, hi, n in maps if lo <= a < hi), '?')] += 1
        continue                    # shared libraries (GL driver, libc), kernel
    inside += 1
    hits[syms[bisect.bisect_right(addrs, a) - 1][1]] += 1
print(f'{total} samples, {100 * inside / max(total, 1):.1f}% in the executable')
for name, n in hits.most_common(top):
    print(f'{100 * n / max(inside, 1):5.1f}%  {name}')
if libs:
    print('outside the executable:')
    for name, n in libs.most_common(8):
        print(f'{100 * n / max(total, 1):5.1f}%  {name}')
