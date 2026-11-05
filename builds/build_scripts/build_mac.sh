#!/usr/bin/env bash
set -euo pipefail
HERE="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd -- "$HERE/../.." && pwd)"
CONFIG="${1:-Debug}"
case "$CONFIG" in Debug|Release) ;; *) echo "Usage: ./build_mac.sh [Debug|Release]" >&2; exit 2;; esac
[[ "$(uname -s)" == Darwin && "$(uname -m)" == arm64 ]] || { echo 'Requires macOS ARM64' >&2; exit 2; }
DROP="$ROOT/builds/drops/macos-arm64"
BUILD="$DROP/cmake/$CONFIG"
cmake -S "$ROOT/builds/cmake_config" -B "$BUILD" -G 'Unix Makefiles' \
  -DCMAKE_BUILD_TYPE="$CONFIG" -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCLINTER_DROP_ROOT="$DROP"
cmake --build "$BUILD" --parallel 8
EXE="$DROP/bin/$CONFIG/CLinter"
[[ -x "$EXE" ]] || { echo "Missing executable: $EXE" >&2; exit 1; }
printf 'Executable: %s\n' "$EXE"
