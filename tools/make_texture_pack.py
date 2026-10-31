#!/usr/bin/env python3
"""Joins a folder of replacement textures (.dds, .png) into one .pack, the files as they are:

    python tools/make_texture_pack.py "D:/textures" "D:/textures.pack"

Format (little endian, read by pc/src/gs/texreplace.c load_pack):
    "RVXPACK1", u32 count, u64 index offset, then the files, then per file: u64 offset, u64 length, u16 name
    length, the name (UTF-8, no folder). Put the .pack in a folder of textures; loose files there win over it.
"""
import hashlib, os, random, struct, sys

MAGIC = b"RVXPACK1"

def main(src, out):
    files = []
    for root, _, names in os.walk(src):
        for n in names:
            if n.lower().endswith((".dds", ".png")):
                files.append(os.path.join(root, n))
    files.sort(key=lambda p: os.path.basename(p).lower())
    index, seen = [], set()
    with open(out + ".tmp", "wb") as f:
        f.write(MAGIC + struct.pack("<IQ", 0, 0))
        for i, p in enumerate(files):
            name = os.path.basename(p)
            if name.lower() in seen:   # the first one wins, as in the folder scan
                continue
            seen.add(name.lower())
            off = f.tell()
            with open(p, "rb") as s:
                n = 0
                while b := s.read(1 << 24):
                    f.write(b); n += len(b)
            index.append((off, n, name, p))
            if i % 500 == 0:
                print(f"{i}/{len(files)}", flush=True)
        at = f.tell()
        for off, n, name, _ in index:
            b = name.encode("utf-8")
            f.write(struct.pack("<QQH", off, n, len(b)) + b)
        f.seek(len(MAGIC))
        f.write(struct.pack("<IQ", len(index), at))
    os.replace(out + ".tmp", out)
    # check: the index read back, a sample of the files against their originals
    with open(out, "rb") as f:
        assert f.read(8) == MAGIC
        count, at = struct.unpack("<IQ", f.read(12))
        assert count == len(index)
        for off, n, _, p in random.sample(index, min(300, count)) + index[-1:]:
            f.seek(off)
            with open(p, "rb") as s:
                assert hashlib.sha1(f.read(n)).digest() == hashlib.sha1(s.read()).digest(), p
    print(f"{count} textures, {os.path.getsize(out) / 2**30:.2f} GiB: {out}")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    main(sys.argv[1], sys.argv[2])
