#!/bin/sh
# pc/tools/re4/play_square.sh NAME SECONDS [teleport] -- one headless room_test run with an explicit vanilla 4:3 ini.
cd "$(dirname "$0")/../../.." || exit 1   # clone root; output in ignored scratch/re4/play/NAME
out=scratch/re4/play/$1; rm -rf "$out"; mkdir -p "$out/save"
printf 'aspect = 0\ninterpolate = 0\nframe_blend = 0\nnative_3d = 1\nnative_tex = 1\nskip_doors = 0\nmodel_cull = 1\ntps = 0\nenemy_extra = 0\nenemy_place = 0\nenemy_random = 0\nshow_fps = 0\n' > "$out/save/recvx.ini"
export RECVX_ISO="${RECVX_ISO:?set RECVX_ISO to your SLUS-20184 disc image}" RECVX_MODS="$PWD/mods"
export RECVX_NOCLIP=${RECVX_NOCLIP-1:0} RECVX_INF_HEALTH=1 RECVX_TRACE_SND=1
SIZE=${SIZE:-640x480} pc/tools/room_test.sh "$out" "${3:-1:26:0}" "$2"
