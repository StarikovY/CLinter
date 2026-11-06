#!/usr/bin/env python3
"""Day 5 MSX2 graphics regression; no SDL display required."""
import argparse
import os
from pathlib import Path
import subprocess
import sys
import tempfile

p = argparse.ArgumentParser()
p.add_argument('--exe', required=True)
p.add_argument('--case', choices=('primitives', 'invalid'), required=True)
a = p.parse_args()
env = dict(os.environ, CLINTER_GRAPHICS_HEADLESS='1')
if a.case == 'primitives':
    source = Path(__file__).parent / 'auto' / 'GraphicsDay5.bas'
    r = subprocess.run([a.exe, str(source)], capture_output=True, text=True, timeout=15, env=env)
    if r.returncode or 'GRAPHICS DAY5 PASS' not in r.stdout or 'FAIL ' in r.stdout or 'ERROR:' in r.stdout:
        sys.exit('FAIL graphics primitives: ' + repr(r.stdout + r.stderr))
    print('PASS: MSX2 graphics primitives')
else:
    cases = [('PSET (1,1),3', 'SCREEN 5 or SCREEN 7 required'),
             ('SCREEN 5\n20 PSET (1,1),16', 'pixel syntax/color'),
             ('SCREEN 5\n20 CIRCLE (1,1),-1', 'CIRCLE radius/color')]
    for body, expected in cases:
        with tempfile.TemporaryDirectory() as d:
            lines = '10 ' + body
            path = Path(d) / 'invalid.bas'
            path.write_text(lines + '\n30 PRINT "UNEXPECTED CONTINUATION"\n')
            r = subprocess.run([a.exe, str(path)], capture_output=True, text=True, timeout=15, env=env)
            if expected not in r.stdout or 'UNEXPECTED CONTINUATION' in r.stdout:
                sys.exit('FAIL: ' + repr(r.stdout + r.stderr))
    print('PASS: graphics invalid inputs')
