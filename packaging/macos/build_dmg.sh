#!/usr/bin/env bash
set -euo pipefail

# Run from any directory: bash packaging/macos/build_dmg.sh [--allow-missing]
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
VERSION="${CLINTER_VERSION:-1.0.0}"
ARCH="macos-arm64"
EXECUTABLE="${CLINTER_EXECUTABLE:-$ROOT/builds/drops/$ARCH/bin/Release/CLinter}"
OUT="${CLINTER_PACKAGE_OUT:-$ROOT/builds/packages}"
STAGE="$OUT/staging/CLinter-$VERSION-$ARCH"
APP="$STAGE/CLinter.app"
ALLOW_MISSING=0/Users/yst/Devs/CLinter/packaging/macos/clinter-launcher.sh
[[ "${1:-}" == "--allow-missing" ]] && ALLOW_MISSING=1

if [[ "$(uname -s)" != Darwin ]]; then echo 'ERROR: DMG creation requires macOS.' >&2; exit 1; fi
if [[ ! -x "$EXECUTABLE" ]]; then echo "ERROR: Release executable missing: $EXECUTABLE" >&2; exit 1; fi
command -v iconutil >/dev/null || { echo "ERROR: iconutil not available" >&2; exit 1; }
command -v hdiutil >/dev/null || { echo 'ERROR: hdiutil not available' >&2; exit 1; }
mkdir -p "$OUT"; rm -rf "$STAGE"; mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources" "$STAGE/Examples/AI" "$STAGE/Examples/Games" "$STAGE/Examples/Graphics" "$STAGE/Examples/BASIC" "$STAGE/Documentation"
cp "$EXECUTABLE" "$APP/Contents/MacOS/CLinter"
chmod 755 "$APP/Contents/MacOS/CLinter"
# Build native Finder/Dock icon from the checked-in PNG iconset.
iconutil -c icns "$SCRIPT_DIR/CLinter.iconset" -o "$APP/Contents/Resources/CLinter.icns"
cp "$SCRIPT_DIR/clinter-launcher.sh" "$APP/Contents/MacOS/clinter-launcher"
chmod 755 "$APP/Contents/MacOS/clinter-launcher"
python3 - "$APP/Contents/Info.plist" "$VERSION" <<'PY'
import plistlib, sys
with open(sys.argv[1], 'wb') as f:
    plistlib.dump({
        'CFBundleName': 'CLinter', 'CFBundleDisplayName': 'CLinter',
        'CFBundleIdentifier': 'org.clinter.interpreter',
        'CFBundleExecutable': 'clinter-launcher', 'CFBundleIconFile': 'CLinter.icns',
        'CFBundlePackageType': 'APPL', 'CFBundleVersion': sys.argv[2],
        'CFBundleShortVersionString': sys.argv[2],
        'LSMinimumSystemVersion': '11.0',
        'NSHighResolutionCapable': True,
    }, f)
PY

missing=0
# Accept a specific source override, otherwise find the first matching filename.
copy_sample() {
  local label="$1" destination="$2" override="$3"; shift 3
  local source="${!override:-}" candidate
  if [[ -z "$source" ]]; then
    for candidate in "$@"; do
      if [[ -f "$ROOT/$candidate" ]]; then source="$ROOT/$candidate"; break; fi
    done
  fi
  if [[ -n "$source" && -f "$source" ]]; then cp "$source" "$destination"; echo "Included: $label <- $source"
  else echo "MISSING: $label (set $override to its source path)" >&2; missing=$((missing+1)); fi
}
copy_sample 'Eliza.bas' "$STAGE/Examples/AI/Eliza.bas" CLINTER_ELIZA \
  packaging/examples/AI/Eliza.bas
copy_sample 'Nebula.bas' "$STAGE/Examples/Graphics/Nebula.bas" CLINTER_NEBULA \
  packaging/examples/Graphics/Nebula.bas
copy_sample 'DuckHunt720p.bas' "$STAGE/Examples/Games/DuckHunt720p.bas" CLINTER_DUCK720 \
  tests/regression/auto/DuckHunt720p.bas tests/DuckHunt720p.bas
copy_sample 'DuckHuntD1.bas' "$STAGE/Examples/Games/DuckHuntD1.bas" CLINTER_DUCKD1 \
  tests/regression/auto/DuckHuntD1.bas tests/regression/auto/DuckHunt.bas
copy_sample 'DuckHuntText.bas' "$STAGE/Examples/Games/DuckHuntText.bas" CLINTER_DUCKTEXT \
  tests/regression/auto/DuckHuntText.bas tests/regression/legacy/Tests/utka.bas
copy_sample 'RegTest.bas' "$STAGE/Examples/BASIC/RegTest.bas" CLINTER_REGTEST \
  tests/regression/legacy/Tests/regression.bas
copy_sample 'English manual' "$STAGE/Documentation/CLinter_BASIC_Manual_EN.pdf" CLINTER_MANUAL_EN \
  docs/CLinter_BASIC_Manual_EN.pdf

if [[ -f "$ROOT/LICENSE" ]]; then cp "$ROOT/LICENSE" "$STAGE/LICENSE"; else echo 'MISSING: LICENSE' >&2; missing=$((missing+1)); fi
cp "$SCRIPT_DIR/README.md" "$STAGE/README.md"
ln -s /Applications "$STAGE/Applications"
if (( missing > 0 && ALLOW_MISSING == 0 )); then
  echo "ERROR: $missing required files missing. Resolve paths or use --allow-missing for a non-release smoke test." >&2
  exit 2
fi
DMG="$OUT/CLinter-$VERSION-$ARCH.dmg"
rm -f "$DMG"
hdiutil create -volname "CLinter $VERSION" -srcfolder "$STAGE" -ov -format UDZO "$DMG"
shasum -a 256 "$DMG" > "$DMG.sha256"
echo "Created: $DMG"
if (( missing > 0 )); then echo "WARNING: INCOMPLETE TEST PACKAGE ($missing missing files); do not publish." >&2; fi
