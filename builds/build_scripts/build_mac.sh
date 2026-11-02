#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd -- "$SCRIPT_DIR/../.." && pwd)"
CMAKE_SOURCE="$REPO_ROOT/builds/cmake_config"
CONFIG="${1:-Debug}"
case "$CONFIG" in Debug|Release) ;; *) echo "Usage: $0 [Debug|Release]" >&2; exit 2;; esac
[[ "$(uname -s)" == Darwin && "$(uname -m)" == arm64 ]] || { echo 'Requires native macOS ARM64' >&2; exit 2; }
DROP="$REPO_ROOT/builds/drops/macos-arm64"
BUILD_DIR="$DROP/cmake/$CONFIG"
cmake -S "$CMAKE_SOURCE" -B "$BUILD_DIR" -G "Unix Makefiles" \
  -DCMAKE_BUILD_TYPE="$CONFIG" -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCLINTER_DROP_ROOT="$DROP"
cmake --build "$BUILD_DIR" --parallel 8
printf 'Executable: %s/bin/%s/CLinter\n' "$DROP" "$CONFIG"
