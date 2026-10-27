#!/bin/bash
# compare_native.sh BINARY OUTDIR [ISO] [FIRST LAST]
# Runs $VAR=0 ("orig") and $VAR=1 ("native") in parallel (VAR default RECVX_NATIVE3D),
# headless, with the same scripted input, FAST_DEMO and SKIP_MOVIES, dumping
# every 30th frame; then prints per-frame mean abs diff and writes
# OUTDIR/montage.png (original | native | 4x diff). Builds nothing.
# FIRST/LAST restrict the diffed frame window (default: all common frames).
# Runtime: RUN_SECS (default 150s) wall clock + a few seconds of diffing.
# Env: RUN_SECS, PY (python with opencv+numpy, default python3), RECVX_INPUT, VAR.
# Never commit OUTDIR: frames are derived from the disc.
set -u
BIN=$(realpath "$1"); OUT=$(realpath -m "$2")
ISO=${3:-${RECVX_ISO:-$(dirname "$(realpath "$0")")/../../ISO/SLUS-20184 ROM.iso}}
FIRST=${4:-0}; LAST=${5:-1000000}
RUN_SECS=${RUN_SECS:-150}
PY=${PY:-python3}
VAR=${VAR:-RECVX_NATIVE3D}
HERE=$(dirname "$(realpath "$0")")
rm -rf "$OUT"; mkdir -p "$OUT"/{orig,native,save0,save1}
run() { # $1 = $VAR value, $2 = name
  SDL_AUDIO_DRIVER=dummy SDL_VIDEO_DRIVER=offscreen env "$VAR=$1" \
  RECVX_FAST_DEMO=1 RECVX_SKIP_MOVIES=1 RECVX_INPUT="${RECVX_INPUT:-200:start,206:-}" \
  RECVX_DUMP_FRAMES="$OUT/$2" timeout "$RUN_SECS" "$BIN" --size 320x240 --vanilla \
  --quiet --save-dir "$OUT/save$1" "$ISO" >"$OUT/$2.log" 2>&1
}
run 0 orig & run 1 native & wait
rm -f "$OUT"/orig/rt*.bmp "$OUT"/native/rt*.bmp
rm -rf "$OUT/save0" "$OUT/save1"
"$PY" "$HERE/compare_frames.py" "$OUT/orig" "$OUT/native" "$OUT/montage.png" "$FIRST" "$LAST"
