#!/usr/bin/env bash
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
CONFIG="${1:-Debug}"
"$HERE/build_linux.sh" "$CONFIG"
ctest --test-dir "$ROOT/builds/drops/linux-x64/cmake/$CONFIG" --output-on-failure
