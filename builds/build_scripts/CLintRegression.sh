#!/bin/bash
# Install in builds/build_scripts.
set -u
ROOT=$(cd -- "$(dirname -- "$0")/../.." && pwd) || exit 1
EXE="$ROOT/builds/drops/macos-arm64/bin/Release/CLinter"
if [[ ! -x "$EXE" ]]; then
    echo "ERROR: Release executable not found or not executable: $EXE" >&2
    exit 1
fi
cd "$ROOT" || exit 1
passed=0
fail() {
    echo "FAIL: $*" >&2
    echo "REGRESSION FAILED. Passed $passed of 13. No further tests run." >&2
    exit 1
}
run() {
    local id="$1" name="$2" file answer rc
    local -a matches=()
    while IFS= read -r -d '' file; do
        matches+=("$file")
    done < <(find "$ROOT/tests" -type f -iname "$name" -print0)
    [[ ${#matches[@]} -eq 1 ]] || fail "$id: $name - found ${#matches[@]} matching files."
    printf '\n[%s] %s\n' "$id" "${matches[0]}"
    "$EXE" "${matches[0]}"
    rc=$?
    [[ $rc -eq 0 ]] || fail "$id: exit code $rc"
    read -r -p 'Confirm output and behavior: [P] PASS / [F] FAIL: ' answer || fail "$id: no confirmation"
    [[ "$answer" == P || "$answer" == p ]] || fail "$id: operator rejected test"
    passed=$((passed + 1))
    echo "PASS $id: $name"
}
echo 'CLinter R1 - macos-arm64 Release'
run B01 GenericRegTest.bas
run B02 regression.bas
run B03 FloatIntegration.bas
run B04 ColorScreen1.bas
run B05 Screen20Test.bas
run B06 GraphicsDay5.bas
run B07 SpritePhaseG6.bas
run B08 utka.bas
run B09 DuckHuntD1.bas
run B10 DuckHunt720p.bas
run B11 Life720p.bas
run B12 spiral.bas
run B13 StairwayCloudsDemo.bas
echo 'ALL 13 BASIC TESTS PASSED.'
