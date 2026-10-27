#!/usr/bin/env python3
"""compare_frames.py ORIG_DIR NATIVE_DIR OUT_PNG [first last]
Per-frame mean abs diff for frame_NNNNN.bmp present in both dirs; writes a
montage (original | native | 4x diff) of the frames to OUT_PNG."""
import sys, os, re, cv2, numpy as np

a, b, out = sys.argv[1:4]
lo = int(sys.argv[4]) if len(sys.argv) > 4 else 0
hi = int(sys.argv[5]) if len(sys.argv) > 5 else 10**9
names = lambda d: {f for f in os.listdir(d) if re.fullmatch(r"frame_\d+\.bmp", f)}
rows = []
for f in sorted(names(a) & names(b)):
    n = int(f[6:11])
    if not lo <= n <= hi:
        continue
    x, y = cv2.imread(f"{a}/{f}"), cv2.imread(f"{b}/{f}")
    if x is None or y is None or x.shape != y.shape:
        print(f"{n}: unreadable or size mismatch")
        continue
    d = cv2.absdiff(x, y)
    print(f"frame {n}: mean abs diff {d.mean():.2f}")
    rows.append(np.hstack([x, y, np.clip(d.astype(int) * 4, 0, 255).astype(np.uint8)]))
if rows:
    cv2.imwrite(out, np.vstack(rows))
    print("montage:", out)
else:
    print("no common frames")
