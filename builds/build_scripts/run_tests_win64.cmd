#!/usr/bin/env bash
set -euo pipefail
HERE="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd -- "$HERE/../.." && pwd)"
CONFIG="${1:-Debug}"
case "$CONFIG" in Debug|Release) ;; *) echo "Usage: ./run_tests_mac.sh [Debug|Release]" >&2; exit 2;; esac
"$HERE/build_mac.sh" "$CONFIG"
BUILD="$ROOT/builds/drops/macos-arm64/cmake/$CONFIG"
EXE="$ROOT/builds/drops/macos-arm64/bin/$CONFIG/CLinter"
ctest --test-dir "$BUILD" --output-on-failure
printf '\nMAT direct regression:\n'
"$EXE" "$ROOT/Tests/regression/auto/MatTest.bas"
