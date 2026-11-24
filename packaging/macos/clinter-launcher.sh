#!/bin/bash
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
BIN="$HERE/CLinter"
# Launch in Terminal so BASIC INPUT and keyboard controls work normally.
# AppleScript handles quoting the executable path safely.
/usr/bin/osascript - "$BIN" <<'APPLESCRIPT'
on run argv
  set executablePath to item 1 of argv
  set commandText to "exec " & quoted form of executablePath
  tell application "Terminal"
    activate
    do script commandText
  end tell
end run
APPLESCRIPT
