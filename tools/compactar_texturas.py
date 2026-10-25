"""Compact an HD texture pack for the VR mod (vr120).

The packs made for PCSX2 come as BC7 DDS without mipmaps, many at 2048 or 4096 px. The game uses at most
tex_hd_max (1024) and makes the mipmaps VR needs while you play (a stutter on entering a room). This makes a
copy of the pack with every image at most MAX px (aspect kept) and all its mipmaps, still BC7, with the same
file names (the PCSX2 names the game looks for) - nothing to convert in the game, and much smaller.

Uses Microsoft's texconv (DirectXTex). The original folder is not touched.

  python tools/compactar_texturas.py <pack folder> <new folder> [max px, default 1024] [path to texconv.exe]

Several texconv run at once (half the CPU cores, at most 8): the source's BC7 decoding and the resizing are CPU
work, only the BC7 encoding is the GPU's - one at a time left both idle. A file already in the new folder at its
full size is kept (a stopped run goes on where it was; a half-written one is made again).
"""
import os
import struct
import subprocess
import sys
import tempfile
import time
from collections import defaultdict
from concurrent.futures import ThreadPoolExecutor

src, dst = sys.argv[1], sys.argv[2]
cap = int(sys.argv[3]) if len(sys.argv) > 3 else 1024
texconv = sys.argv[4] if len(sys.argv) > 4 else r"C:\Users\Neto\source\tools\texconv\texconv.exe"
os.makedirs(dst, exist_ok=True)

def bc7_size(w, h):
    """a BC7 DDS with the DX10 header and all mipmaps"""
    n = 148
    while True:
        n += max(1, (w + 3) // 4) * max(1, (h + 3) // 4) * 16
        if w == 1 and h == 1:
            return n
        w, h = max(1, w // 2), max(1, h // 2)


groups = defaultdict(list)   # (w, h) -> files
names = set()
skipped = 0
for dp, _, fs in os.walk(src):
    for f in fs:
        p = os.path.join(dp, f)
        if not f.lower().endswith(".dds"):
            continue
        if f.lower() in names:   # (the same name twice: the first one found wins in the game too)
            skipped += 1
            continue
        names.add(f.lower())
        with open(p, "rb") as fh:
            h = fh.read(20)
        if len(h) < 20 or h[:4] != b"DDS ":
            skipped += 1
            continue
        hh, ww = struct.unpack_from("<II", h, 12)
        k = max(ww, hh)
        s = min(1.0, cap / k) if k else 1.0
        tw, th = max(1, round(ww * s)), max(1, round(hh * s))
        out = os.path.join(dst, f)
        if os.path.exists(out) and os.path.getsize(out) == bc7_size(tw, th):   # (done by an earlier run)
            continue
        groups[(tw, th)].append(p)

total = sum(len(v) for v in groups.values())
print(f"{total} textures in {len(groups)} sizes, at most {cap} px ({skipped} skipped)", flush=True)
done, t0 = 0, time.time()
jobs = [((w, h), files[i:i + 40]) for (w, h), files in groups.items() for i in range(0, len(files), 40)]


def run(job):
    (w, h), part = job
    with tempfile.NamedTemporaryFile("w", suffix=".txt", delete=False, encoding="utf-8") as fl:
        fl.write("\n".join(part))
        lst = fl.name
    r = subprocess.run([texconv, "-nologo", "-y", "-f", "BC7_UNORM", "-m", "0", "-w", str(w), "-h", str(h),
                        "-o", dst, "-flist", lst], capture_output=True, text=True)
    os.unlink(lst)
    bad = [l for l in r.stdout.splitlines() if "FAILED" in l]
    return (w, h), len(part), (bad[:3] or [r.stdout[-300:]]) if (r.returncode != 0 or bad) else None


workers = max(2, min(8, (os.cpu_count() or 4) // 2))
print(f"{workers} at once", flush=True)
with ThreadPoolExecutor(workers) as ex:
    for (w, h), n, bad in ex.map(run, jobs):
        done += n
        if bad:
            print(f"  texconv problems ({w}x{h}): {bad}", flush=True)
        if done % 400 < n or done == total:
            el = time.time() - t0
            print(f"  {done}/{total} - {el / 60:.1f} min, about {el / max(done, 1) * (total - done) / 60:.0f} min left", flush=True)

size = sum(os.path.getsize(os.path.join(dst, f)) for f in os.listdir(dst))
print(f"done: {len(os.listdir(dst))} files, {size / 2**30:.2f} GB in {(time.time() - t0) / 60:.1f} min", flush=True)
