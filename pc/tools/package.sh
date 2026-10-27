#!/bin/sh
# Release archives from finished builds (docs/BUILDING.md "Release packages"). No disc image, no game data:
# only the binaries named here and the repository's docs.
#   pc/tools/package.sh OUT_DIR [--linux BUILD_DIR] [--windows BUILD_DIR] [--server BUILD_DIR]
# (no build: only the notes and the checksums, e.g. over archives downloaded into OUT_DIR)
#     --linux    build/  (cmake/linux-i686.cmake)  -> recvx-enhanced-recomp-VER-linux-i686.tar.gz (+ -debug)
#     --windows  build-win/ (cmake/mingw-i686.cmake) -> recvx-enhanced-recomp-VER-windows-x86.zip (+ -debug)
#     --server   build-server/ (cmake -S pc/server)  -> recvx-server-VER-linux-x86_64.tar.gz
# Also writes OUT_DIR/RELEASE_NOTES.md and OUT_DIR/SHA256SUMS (over every archive in OUT_DIR).
# The -debug archives hold the unstripped binaries, for addr2line on crash-log addresses.
# Tools: strip (STRIP), i686-w64-mingw32-strip (WIN_STRIP; or $MINGW_PREFIX/bin), tar, zip, sha256sum.
set -eu
root=$(cd "$(dirname "$0")/../.." && pwd)
[ $# -ge 1 ] || { sed -n 2,11p "$0"; exit 2; }
out=$(mkdir -p "$1" && cd "$1" && pwd); shift
ver=$(sed -n 's/^project(recvx_pc VERSION \([0-9.]*\).*/\1/p' "$root/CMakeLists.txt")
[ -n "$ver" ] || { echo "no version in CMakeLists.txt" >&2; exit 1; }
stage=$(mktemp -d); trap 'rm -rf "$stage"' EXIT
docs="README.md LICENSE THIRD_PARTY_NOTICES.md CHANGELOG.md docs/USER_GUIDE.md"

copy_docs() {   # copy_docs DIR FILES...
    d=$1; shift
    for f in "$@"; do mkdir -p "$d/$(dirname "$f")"; cp "$root/$f" "$d/$f"; done
}
tgz() {   # tgz NAME: stage/NAME -> out/NAME.tar.gz, stable owners and order
    tar -C "$stage" --owner=0 --group=0 --numeric-owner --sort=name -czf "$out/$1.tar.gz" "$1"
}
need() { [ -f "$1" ] || { echo "missing $1 (build it first)" >&2; exit 1; }; }

while [ $# -gt 0 ]; do
    case $1 in
    --linux)
        b=$2/pc/recvx; need "$b"
        n=recvx-enhanced-recomp-$ver-linux-i686; d=$stage/$n; mkdir -p "$d"
        ${STRIP:-strip} -o "$d/recvx" "$b"
        chmod 755 "$d/recvx"
        copy_docs "$d" $docs
        tgz "$n"
        mkdir -p "$stage/$n-debug"; cp "$b" "$stage/$n-debug/recvx"; tgz "$n-debug"
        ;;
    --windows)
        b=$2/pc/recvx.exe; need "$b"
        ws=${WIN_STRIP:-${MINGW_PREFIX:+$MINGW_PREFIX/bin/}i686-w64-mingw32-strip}
        n=recvx-enhanced-recomp-$ver-windows-x86; d=$stage/$n; mkdir -p "$d"
        "$ws" -o "$d/recvx.exe" "$b"
        copy_docs "$d" $docs
        (cd "$stage" && rm -f "$out/$n.zip" && zip -qrX "$out/$n.zip" "$n")
        mkdir -p "$stage/$n-debug"; cp "$b" "$stage/$n-debug/recvx.exe"
        (cd "$stage" && rm -f "$out/$n-debug.zip" && zip -qrX "$out/$n-debug.zip" "$n-debug")
        ;;
    --server)
        need "$2/recvx-server"; need "$2/recvx-client"
        n=recvx-server-$ver-linux-x86_64; d=$stage/$n; mkdir -p "$d"
        strip -o "$d/recvx-server" "$2/recvx-server"
        strip -o "$d/recvx-client" "$2/recvx-client"
        cp "$root/pc/server/recvx-server.conf.example" "$root/pc/server/recvx-server.service" "$d/"
        copy_docs "$d" README.md LICENSE THIRD_PARTY_NOTICES.md CHANGELOG.md docs/SERVER.md
        tgz "$n"
        ;;
    *) echo "unknown argument $1" >&2; exit 2 ;;
    esac
    shift 2
done

# release notes: the changelog section of this version, first start, the AI note of the README
{
    awk -v v="$ver" '/^## /{p = index($0, v) > 0; next} p' "$root/CHANGELOG.md"
    echo
    cat <<EOF
## Requirements / first start

- Your own disc image of *Resident Evil Code: Veronica X*, PS2 NTSC-U, **SLUS-20184** (\`.iso\`).
  No game data is included in these downloads.
- **Windows** (\`recvx-enhanced-recomp-$ver-windows-x86.zip\`): Windows 10 or later, OpenGL 3.3.
  Unpack and run \`recvx.exe\`.
- **Linux** (\`recvx-enhanced-recomp-$ver-linux-i686.tar.gz\`): glibc 2.34 or newer and the 32-bit
  (multilib) OpenGL, X11/Wayland and audio libraries. Unpack and run \`./recvx\`.
- On the first start a file selector asks for the disc image; the choice is remembered. Settings
  and saves go to \`save/\` next to the game. F1 opens the settings. See \`docs/USER_GUIDE.md\`.
- Co-op server (\`recvx-server-$ver-linux-x86_64.tar.gz\`): static Linux binary, see \`docs/SERVER.md\`.
- \`-debug\` archives: unstripped binaries for resolving crash-log addresses; players do not need them.
- Check downloads with \`sha256sum -c SHA256SUMS\`.

EOF
    awk '/^> /{print; q = 1; next} q {exit}' "$root/README.md"
} > "$out/RELEASE_NOTES.md"

(cd "$out" && find . -maxdepth 1 \( -name '*.tar.gz' -o -name '*.zip' \) | sed 's|^\./||' | sort | xargs -r sha256sum > SHA256SUMS)
for f in "$out"/*.tar.gz; do [ -e "$f" ] && { echo "== $(basename "$f")"; tar -tzvf "$f"; }; done
for f in "$out"/*.zip; do [ -e "$f" ] && { echo "== $(basename "$f")"; unzip -l "$f"; }; done
cat "$out/SHA256SUMS"
