#!/bin/sh
# R9.6: run from anywhere: sh ErrHandTest.sh /path/to/CLinter
set -eu
BIN="${1:-CLinter}"
TMP=$(mktemp -d "${TMPDIR:-/tmp}/clinter-r96.XXXXXX")
trap 'rm -rf "$TMP"' EXIT HUP INT TERM
printf 'THIS IS NOT A NUMBERED BASIC LINE\n' > "$TMP/bad.bas"
cat > "$TMP/commands" <<EOF2
10 PRINT "ORIGINAL PROGRAM"
LOAD "$TMP/missing.bas"
LIST
LOAD "$TMP/bad.bas"
LIST
SAVE "$TMP/no-such-directory/output.bas"
LIST
SAVE "$TMP/good.bas"
NEW
LOAD "$TMP/good.bas"
LIST
BYE
EOF2
"$BIN" < "$TMP/commands" > "$TMP/output" 2>&1
# Both failed LOAD operations and failed SAVE must preserve the stored program.
[ "$(grep -c '10 PRINT "ORIGINAL PROGRAM"' "$TMP/output")" -eq 4 ] || { cat "$TMP/output"; echo 'FAIL: stored program changed'; exit 1; }
grep -q 'ERROR: cannot open file' "$TMP/output" || { cat "$TMP/output"; exit 1; }
grep -q 'ERROR: invalid or unreadable program file' "$TMP/output" || { cat "$TMP/output"; exit 1; }
grep -q 'ERROR: cannot write file' "$TMP/output" || { cat "$TMP/output"; exit 1; }
grep -q 'Saved to ' "$TMP/output" || { cat "$TMP/output"; exit 1; }
grep -q 'Loaded ' "$TMP/output" || { cat "$TMP/output"; exit 1; }
echo 'PASS: R9.6 SAVE/LOAD error handling and program preservation'
