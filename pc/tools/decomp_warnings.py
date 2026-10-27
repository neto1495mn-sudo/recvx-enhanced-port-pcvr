#!/usr/bin/env python3
"""Regenerate docs/dev/missing-returns.txt and docs/dev/uninitialized-locals.txt.

    python3 pc/tools/decomp_warnings.py [BUILD_DIR]   (default: build)

Compiles every decomp source of the configured build tree again (its own command line, output
to /dev/null) with -Wreturn-type -Wreturn-mismatch -Wmaybe-uninitialized instead of -w, and
lists the warnings by file and function (no line numbers, so the lists only change when the
code does). The build does not change: the decomp keeps compiling with -w (pc/CMakeLists.txt).
"""
import os
import re
import shlex
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..'))
PROG = 'decomp/src/ps2/veronica/prog/'
FLAGS = ['-Wreturn-type', '-Wreturn-mismatch', '-Wmaybe-uninitialized', '-Wuninitialized']

HEADERS = {
    'missing-returns.txt': (
        '# Non-void decomp functions that can fall off their end or return no value (GCC -Wreturn-type /\n'
        '# -Wreturn-mismatch), by file and function; pc/tools/decomp_warnings.py writes this file.\n'
        '# MWCC returned whatever was in v0. Fixed against retail (TARGET_PC, no longer listed): en01.c\n'
        '# bhEne01_SetLinkEnemy, adxwrap.c GetAdxStatus, en03.c bhEne03_GetWallDir, en17.c\n'
        '# bhEne17_PlayerDGCheck, en22.c bhEne22_GetDengekiColorAddr (returns NULL; its two stores check it).\n'
        '# The rest: value unused by callers, or only reached for invalid inputs (retail returns a stale register).\n'),
    'uninitialized-locals.txt': (
        '# Possible uses of uninitialized locals in game code (GCC -Wmaybe-uninitialized), by file and\n'
        '# function; pc/tools/decomp_warnings.py writes this file. The PC build zero-initializes all locals\n'
        '# (-ftrivial-auto-var-init=zero); on the PS2 these read stale register/stack values. Verify the\n'
        '# ones that matter against the retail ELF. Fixed: ps2_sg_sd.c Volume_Control (dvol of a fade up,\n'
        '# docs/ARCHITECTURE.md "Fidelity notes").\n'),
}


def commands(build):
    out = subprocess.run(['ninja', '-C', build, '-t', 'commands', 'pc/recvx'], check=True,
                         capture_output=True, text=True).stdout
    for line in out.splitlines():
        args = shlex.split(line)
        if '-c' not in args or PROG not in args[-1] or not args[-1].endswith('.c'):
            continue
        keep, skip = [], 0
        for a in args:
            if skip:
                skip -= 1
            elif a in ('-MT', '-MF', '-o'):
                skip = 1
            elif a in ('-w', '-MD', '-ftrivial-auto-var-init=zero') or a.startswith('-Werror'):
                pass
            else:
                keep.append(a)
        yield args[-1], keep[:-2] + FLAGS + ['-o', os.devnull, '-c', args[-1]]


def warnings(job):
    src, cmd = job
    err = subprocess.run(cmd, cwd=BUILD, capture_output=True, text=True).stderr
    func, found = None, []
    for line in err.splitlines():
        m = re.match(r'(.+?): In function [‘\'](.+?)[’\']:', line)
        if m:
            func = m.group(2) if os.path.basename(m.group(1)) == os.path.basename(src) else None
            continue
        m = re.match(r'(.+?):\d+:\d+: warning: (.*) \[(-W[a-z-]+)\]', line)
        if m and os.path.basename(m.group(1)) == os.path.basename(src) and func:
            msg = re.sub(r'[‘’]', "'", m.group(2))
            found.append((os.path.basename(src), func, m.group(3), msg))
    return found


if __name__ == '__main__':
    BUILD = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, 'build'))
    jobs = list(commands(BUILD))
    if not jobs:
        sys.exit('no decomp compile commands in %s (configure it first)' % BUILD)
    with ThreadPoolExecutor(os.cpu_count()) as ex:
        found = sorted(set(w for ws in ex.map(warnings, jobs) for w in ws))
    lists = {'missing-returns.txt': [w for w in found if w[2] in ('-Wreturn-type', '-Wreturn-mismatch')],
             'uninitialized-locals.txt': [w for w in found if 'uninitialized' in w[2]]}
    for name, ws in lists.items():
        with open(os.path.join(ROOT, 'docs', 'dev', name), 'w') as f:
            f.write(HEADERS[name])
            for src, func, _, msg in ws:
                f.write('%s %s: %s\n' % (src, func, msg))
        print('docs/dev/%s: %d entries' % (name, len(ws)))
