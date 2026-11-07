#!/usr/bin/env python3
"""Exercise original MSX blocks1.bas without changing the supplied fixture."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

p = argparse.ArgumentParser()
p.add_argument('--exe', required=True)
p.add_argument('--file', required=True)
a = p.parse_args()
original = Path(a.file).read_bytes()
assert b'260 GOTO 260' in original, 'Original blocks1.bas changed unexpectedly'
checks = (b'260 IF POINT(32,0)<>14 THEN PRINT "FAIL BLOCK 1": END\n'
          b'261 IF POINT(48,0)<>6 THEN PRINT "FAIL BLOCK 2": END\n'
          b'262 IF POINT(64,0)<>6 THEN PRINT "FAIL DIAGONAL": END\n'
          b'263 PRINT "BLOCKS1 PASS"\n264 END')
with tempfile.TemporaryDirectory() as d:
    f = Path(d) / 'blocks1_auto.bas'
    f.write_bytes(original.replace(b'260 GOTO 260', checks))
    env = dict(os.environ, CLINTER_GRAPHICS_HEADLESS='1')
    r = subprocess.run([a.exe, str(f)], capture_output=True, text=True,
                       env=env, timeout=15)
    if r.returncode or 'BLOCKS1 PASS' not in r.stdout or 'FAIL ' in r.stdout or 'ERROR:' in r.stdout:
        raise SystemExit(r.stdout + r.stderr)
print('PASS: original MSX SCREEN 2 blocks1.bas drawing and pixels')
