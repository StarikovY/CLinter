#!/bin/bash
# Package frozen CLinter V1.0; modify only staged copies.
set -euo pipefail
fail() { echo "ERROR: $*" >&2; exit 1; }
[[ $# -eq 4 ]] || fail 'Usage: Pack_macosarm64.sh "English.pdf" "Russian.pdf" "LICENSE.txt" "README.md"'
[[ $(uname -s) == Darwin && $(uname -m) == arm64 ]] || fail 'Run on native macOS ARM64.'
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
BIN="$ROOT/builds/drops/macos-arm64/bin/Release/CLinter"
SAMPLES="$ROOT/tests/regression/auto"
ELIZA="$ROOT/packaging/examples/AI/Eliza.bas"
[[ -f "$ELIZA" ]] || ELIZA="$ROOT/Eliza.bas"
[[ -f "$ELIZA" ]] || fail "Eliza.bas not found in packaging/examples/AI or repository root."
OUT="$ROOT/builds/drops/macos-arm64/distribution"
NAME=CLinter-1.0-macos-arm64
STAGE="$OUT/$NAME"
ZIP="$OUT/$NAME.zip"
for f in "$BIN" "$@"; do [[ -f "$f" ]] || fail "Missing $f"; done
for f in regression.bas utka.bas DuckHuntD1.bas DuckHunt720p.bas Nebula.bas spiral.bas StairwayCloudsDemo.bas; do
  [[ -f "$SAMPLES/$f" ]] || fail "Missing $SAMPLES/$f"
done
for f in "$STAGE" "$ZIP" "$ZIP.sha256"; do [[ ! -e "$f" ]] || fail "Output already exists: $f"; done
lipo -verify_arch arm64 "$BIN" || fail 'Executable lacks ARM64 architecture'
deps() { otool -L "$1" | tail -n +2 | sed -E 's/^[[:space:]]+//; s/ \(compatibility version.*$//'; }
# Resolve the SDL2 install name; allow explicit override for nonstandard installations.
SDL_REF=$(deps "$BIN" | grep -iE '(^|/)libSDL2[^/]*\.dylib$|/SDL2\.framework/.*/SDL2$' || true)
[[ -n "$SDL_REF" && "$SDL_REF" != *$'\n'* ]] || fail 'Expected exactly one dynamic SDL2 dependency; inspect otool -L.'
SDL_SRC=${CLINTER_SDL2_DYLIB:-}
if [[ -z "$SDL_SRC" ]]; then
  case "$SDL_REF" in
    /*) SDL_SRC="$SDL_REF" ;;
    @loader_path/*) SDL_SRC="$(dirname "$BIN")/${SDL_REF#@loader_path/}" ;;
    @executable_path/*) SDL_SRC="$(dirname "$BIN")/${SDL_REF#@executable_path/}" ;;
    @rpath/*)
      while IFS= read -r rp; do
        rp=${rp//@loader_path/$(dirname "$BIN")}; rp=${rp//@executable_path/$(dirname "$BIN")}
        candidate="$rp/${SDL_REF#@rpath/}"
        if [[ -f "$candidate" ]]; then SDL_SRC="$candidate"; break; fi
      done < <(otool -l "$BIN" | awk '/cmd LC_RPATH/{r=1;next} r && /path /{sub(/^.*path /, "");sub(/ \(offset.*$/, "");print;r=0}')
      if [[ -z "$SDL_SRC" ]] && command -v brew >/dev/null; then SDL_SRC="$(brew --prefix sdl2)/lib/${SDL_REF##*/}"; fi ;;
  esac
fi
[[ -f "$SDL_SRC" ]] || fail 'SDL2 not found; set CLINTER_SDL2_DYLIB to its full path.'
[[ "$SDL_REF" != *SDL2.framework* ]] || fail 'Framework SDL2 requires separate framework packaging; this script supports dylibs only.'
lipo -verify_arch arm64 "$SDL_SRC" || fail 'SDL2 lacks ARM64 architecture'
# Fail closed on additional non-system dependencies rather than shipping a broken ZIP.
check_deps() {
  local file="$1" allowed="$2" d
  while IFS= read -r d; do
    case "$d" in /usr/lib/*|/System/Library/*) continue ;; esac
    [[ "$d" == "$allowed" ]] || fail "Unbundled dependency in $file: $d"
  done < <(deps "$file")
}
check_deps "$BIN" "$SDL_REF"
SDL_ID=$(otool -D "$SDL_SRC" | sed -n '2p')
[[ -n "$SDL_ID" ]] || fail 'SDL2 install ID missing'
check_deps "$SDL_SRC" "$SDL_ID"
# sdl2-compat loads SDL3 via dlopen, so otool alone cannot detect it.
SDL3_PREFIX=$(brew --prefix sdl3 2>/dev/null) || fail 'SDL3 Homebrew prefix not found'
SDL3_SRC=${CLINTER_SDL3_DYLIB:-$SDL3_PREFIX/lib/libSDL3.0.dylib}
[[ -f "$SDL3_SRC" ]] || fail "Missing SDL3: $SDL3_SRC"
lipo -verify_arch arm64 "$SDL3_SRC" || fail 'SDL3 lacks ARM64 architecture'
SDL3_ID=$(otool -D "$SDL3_SRC" | sed -n '2p')
[[ -n "$SDL3_ID" ]] || fail 'SDL3 install ID missing'
check_deps "$SDL3_SRC" "$SDL3_ID"
SDL3_LICENSE=${CLINTER_SDL3_LICENSE:-}
if [[ -z "$SDL3_LICENSE" ]]; then
  for f in "$SDL3_PREFIX/LICENSE.txt" "$SDL3_PREFIX/LICENSE" "$SDL3_PREFIX/share/licenses/sdl3/LICENSE.txt"; do
    if [[ -f "$f" ]]; then SDL3_LICENSE="$f"; break; fi
  done
fi
[[ -f "$SDL3_LICENSE" ]] || fail 'Set CLINTER_SDL3_LICENSE to the SDL3 license file.'
mkdir -p "$STAGE/docs" "$STAGE/samples" "$STAGE/lib"
cp "$BIN" "$STAGE/CLinter"
cp -L "$SDL_SRC" "$STAGE/lib/libSDL2.dylib"
chmod u+w "$STAGE/CLinter" "$STAGE/lib/libSDL2.dylib"
chmod +x "$STAGE/CLinter"
install_name_tool -id '@rpath/libSDL2.dylib' "$STAGE/lib/libSDL2.dylib"
install_name_tool -change "$SDL_REF" '@executable_path/lib/libSDL2.dylib' "$STAGE/CLinter"
cp -L "$SDL3_SRC" "$STAGE/lib/libSDL3.0.dylib"
chmod u+w "$STAGE/lib/libSDL3.0.dylib"
install_name_tool -id '@rpath/libSDL3.0.dylib' "$STAGE/lib/libSDL3.0.dylib"
ln -s libSDL3.0.dylib "$STAGE/lib/libSDL3.dylib"
codesign --force --sign - "$STAGE/lib/libSDL3.0.dylib"
codesign --verify --strict "$STAGE/lib/libSDL3.0.dylib"
check_deps "$STAGE/lib/libSDL3.0.dylib" '@rpath/libSDL3.0.dylib'
cp "$SDL3_LICENSE" "$STAGE/lib/SDL3_LICENSE.txt"
# Native dyld search paths for SDL2-compat's dynamically loaded SDL3.
# Include the calling dylib's directory as well as the executable's lib directory.
add_rpath() {
  local file="$1" path="$2"
  if ! otool -l "$file" | awk '/cmd LC_RPATH/{r=1;next} r && /path /{sub(/^.*path /, "");sub(/ \(offset.*$/, "");print;r=0}' | grep -Fx "$path" >/dev/null; then
    install_name_tool -add_rpath "$path" "$file"
  fi
}
add_rpath "$STAGE/CLinter" '@executable_path/lib'
add_rpath "$STAGE/lib/libSDL2.dylib" '@loader_path'
codesign --force --sign - "$STAGE/lib/libSDL2.dylib"
codesign --force --sign - "$STAGE/CLinter"
codesign --verify --strict "$STAGE/lib/libSDL2.dylib"
codesign --verify --strict "$STAGE/CLinter"
check_deps "$STAGE/CLinter" '@executable_path/lib/libSDL2.dylib'
check_deps "$STAGE/lib/libSDL2.dylib" '@rpath/libSDL2.dylib'
cp "$1" "$STAGE/docs/CLinter_BASIC_Manual_EN.pdf"
cp "$2" "$STAGE/docs/CLinter_BASIC_Manual_RU.pdf"
cp "$3" "$STAGE/LICENSE.txt"
cp "$4" "$STAGE/README.md"
cp "$SAMPLES/regression.bas" "$STAGE/samples/RegTest.bas"
cp "$SAMPLES/utka.bas" "$STAGE/samples/DuckHuntText.bas"
cp "$ELIZA" "$STAGE/samples/Eliza.bas"
cp "$SAMPLES/spiral.bas" "$STAGE/samples/Spiral.bas"
for f in DuckHuntD1.bas DuckHunt720p.bas Nebula.bas StairwayCloudsDemo.bas; do cp "$SAMPLES/$f" "$STAGE/samples/$f"; done
# Include SDL2 license from Homebrew or an explicitly supplied license path.
SDL_LICENSE=${CLINTER_SDL2_LICENSE:-}
if [[ -z "$SDL_LICENSE" ]] && command -v brew >/dev/null; then
  prefix=$(brew --prefix sdl2-compat 2>/dev/null || true)
  for f in "$prefix/LICENSE.txt" "$prefix/LICENSE" "$prefix/share/licenses/sdl2/LICENSE.txt"; do
    if [[ -n "$prefix" && -f "$f" ]]; then SDL_LICENSE="$f"; break; fi
  done
fi
[[ -f "$SDL_LICENSE" ]] || fail 'Set CLINTER_SDL2_LICENSE to the SDL2 license file; remove partial staging before retrying.'
cp "$SDL_LICENSE" "$STAGE/lib/SDL2_LICENSE.txt"
cat > "$STAGE/README_FIRST.txt" <<'README'
CLinter V1.0 - macOS ARM64

Extract the entire ZIP before running CLinter.
Open Terminal in the extracted directory.
Interactive mode: ./CLinter
Run a sample: ./CLinter samples/DuckHunt720p.bas
See docs for English and Russian BASIC manuals.
Source: https://github.com/StarikovY/CLinter
License: LICENSE.txt; SDL2 compatibility license: lib/SDL2_LICENSE.txt
SDL3 license: lib/SDL3_LICENSE.txt
See README.md for project information.
Samples include Nebula.bas, Spiral.bas, DuckHuntText.bas,
DuckHuntD1.bas, DuckHunt720p.bas, StairwayCloudsDemo.bas, Eliza.bas and RegTest.bas.
This package is ad-hoc signed, not Developer ID signed or notarized.
macOS Gatekeeper may require approval before running downloaded software.
README
# Verify the real native executable actually loads our SDL3, not Homebrew's.
# Run from a different working directory, without the old launcher's environment.
SMOKE=$(mktemp -d "${TMPDIR:-/tmp}/clinter-package.XXXXXXXX")
trap 'rm -rf "$SMOKE"' EXIT
printf '10 PRINT "CLINTER PACKAGE PASS"\n20 END\n' > "$SMOKE/smoke.bas"
if ! (cd "$SMOKE" && env -u DYLD_LIBRARY_PATH -u DYLD_FALLBACK_LIBRARY_PATH \
  -u DYLD_INSERT_LIBRARIES -u SDL3_DYNAMIC_API -u CLINTER_GRAPHICS_HEADLESS \
  DYLD_PRINT_LIBRARIES=1 "$STAGE/CLinter" "$SMOKE/smoke.bas" </dev/null >stdout 2>dyld.log); then
  cat "$SMOKE/stdout" "$SMOKE/dyld.log" >&2
  fail 'Native package smoke test failed; ZIP not created.'
fi
grep -F 'CLINTER PACKAGE PASS' "$SMOKE/stdout" >/dev/null || fail 'BASIC smoke test did not finish.'
# Resolve the physical staging path (dyld may report a canonical path).
LOCAL_LIB=$(cd "$STAGE/lib" && pwd -P)
if ! grep -F "$LOCAL_LIB/libSDL3" "$SMOKE/dyld.log" >/dev/null; then
  cat "$SMOKE/dyld.log" >&2
  fail 'Bundled SDL3 load not verified; ZIP not created.'
fi
if grep -Ei '/(opt/homebrew|usr/local|opt/local)/.*libSDL[23]' "$SMOKE/dyld.log" | grep -Fv "$LOCAL_LIB/" >/dev/null; then
  fail 'External SDL runtime detected; ZIP not created.'
fi
echo 'PASS: native CLinter loaded bundled SDL3 without launcher environment.'
for f in RegTest.bas DuckHuntText.bas DuckHuntD1.bas DuckHunt720p.bas Nebula.bas Spiral.bas StairwayCloudsDemo.bas Eliza.bas; do
  [[ -s "$STAGE/samples/$f" ]] || fail "Missing or empty staged sample: $f"
done
(cd "$OUT" && /usr/bin/zip -q -y -r "$NAME.zip" "$NAME" && /usr/bin/unzip -tq "$NAME.zip" && shasum -a 256 "$NAME.zip" > "$NAME.zip.sha256")
echo "SUCCESS: $ZIP"
echo "SHA256: $ZIP.sha256"
