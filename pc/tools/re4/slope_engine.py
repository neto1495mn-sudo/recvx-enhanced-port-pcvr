"""Compile the current PC slope function for authoring movement checks.

Only the engine function is extracted; the stand-in structs expose the fields it
uses. Spatial bins prune impossible X/Z hits while preserving wall array order.
Generated C/shared libraries and evidence stay in ignored scratch.
"""
import ctypes
import hashlib
import json
from pathlib import Path
import subprocess

ENGINE = Path('decomp/src/ps2/veronica/prog/hitchk.c')


def engine_function():
    source = ENGINE.read_text()
    start = source.index('int pc_slope_ground(')
    brace = source.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


class Wall(ctypes.Structure):
    _fields_ = [('flg', ctypes.c_uint), ('type', ctypes.c_int), ('id', ctypes.c_int)] + [
        (name, ctypes.c_float) for name in ('px', 'py', 'pz', 'w', 'h', 'd')]


class EngineProbe:
    def __init__(self, records):
        function = engine_function()
        digest = hashlib.sha256(function.encode()).hexdigest()
        out = Path('scratch/re4/slope-engine').resolve()
        out.relative_to(Path('scratch').resolve())
        out.mkdir(parents=True, exist_ok=True)
        c = out / (digest + '.c')
        so = c.with_suffix('.so')
        source = '''#include <math.h>
#include <stdlib.h>
typedef struct { float x,y,z; } NJS_POINT3;
typedef struct { unsigned int flg; int type,id; float px,py,pz,w,h,d; } ATR_WORK;
struct BH_PWORK { int flr_no,flr_nob; };
struct { int wal_n; ATR_WORK *walp; float h; } room_value, *rom=&room_value;
struct { int mwal_n; ATR_WORK *mwalp; } system_value, *sys=&system_value;
int bhCheckFloorNum(float y) { return 0; }
''' + function + '''
int probe(ATR_WORK *walls, int *indices, int n, float x, float y, float z,
          float ah, float *result) {
    ATR_WORK subset[n ? n : 1];
    for (int i=0; i<n; i++) subset[i]=walls[indices[i]];
    rom->wal_n=n; rom->walp=subset; rom->h=0;
    struct BH_PWORK actor={0,0}; NJS_POINT3 p={x,y,z};
    int hit=pc_slope_ground(&actor,&p,ah); *result=p.y; return hit;
}
'''
        c.write_text(source)
        if not so.exists():
            subprocess.run(['cc', '-shared', '-fPIC', '-O2', '-std=c99', str(c), '-lm', '-o', str(so)], check=True)
        self.lib = ctypes.CDLL(str(so))
        self.lib.probe.argtypes = [ctypes.POINTER(Wall), ctypes.POINTER(ctypes.c_int), ctypes.c_int] + [ctypes.c_float]*4 + [ctypes.POINTER(ctypes.c_float)]
        self.lib.probe.restype = ctypes.c_int
        self.walls = (Wall * len(records))(*[Wall(**{name: r[name] for name, _ in Wall._fields_}) for r in records])
        self.evidence = dict(engine_path=str(ENGINE), function_sha256=digest, compiled_function=True,
                             float_precision='C float', spatial_pruning='XZ bins, original record order')
        (out/'function.json').write_text(json.dumps(self.evidence, indent=2)+'\n')

    def height(self, indices, x, z, y, ah):
        ids = (ctypes.c_int * len(indices))(*indices)
        result = ctypes.c_float()
        hit = self.lib.probe(self.walls, ids, len(indices), x, y, z, ah, ctypes.byref(result))
        return result.value if hit else None
