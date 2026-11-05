#!/usr/bin/env python3
"""Noninteractive screen regression: verify exact ANSI operations from ColorScreen1.bas."""
import argparse
import pathlib
import re
import subprocess
import sys
import tempfile

p = argparse.ArgumentParser()
p.add_argument('--exe', required=True)
p.add_argument('--case', choices=('colorscreen1','invalid'), required=True)
p.add_argument('--file')
a = p.parse_args()
if a.case == 'colorscreen1':
    program = pathlib.Path(a.file)
else:
    program = None
if a.case == 'invalid':
    samples = [('SCREEN 1', 'ERROR: SCREEN unsupported mode'),
               ('COLOR 16,0', 'ERROR: SCREEN COLOR index out of range'),
               ('LOCATE 80,0', 'ERROR: SCREEN LOCATE coordinates out of range'),
               ('LOCATE 0,25', 'ERROR: SCREEN LOCATE coordinates out of range')]
    for statement, expected in samples:
        with tempfile.TemporaryDirectory() as tmp:
            path = pathlib.Path(tmp) / 'negative.bas'
            path.write_text('10 '+statement+'\n20 PRINT \"UNEXPECTED CONTINUATION\"\n')
            result = subprocess.run([a.exe, str(path)], capture_output=True, timeout=15)
            output = result.stdout.decode(errors='replace')
            if expected not in output or 'UNEXPECTED CONTINUATION' in output:
                sys.exit('FAIL: '+statement+' => '+repr(output))
            print('PASS: '+statement)
    sys.exit(0)
r = subprocess.run([a.exe, str(program)], capture_output=True, timeout=15)
s = r.stdout.decode('utf-8', errors='replace')
if r.returncode != 0:
    sys.exit('Unexpected process failure: '+r.stderr.decode(errors='replace'))
if a.case == 'colorscreen1':
    checks = {
        'no interpreter errors': 'ERROR:' not in s,
        'clear/home ANSI': '\x1b[2J\x1b[H' in s,
        '64 printed X characters': s.count('X\n') == 64,
        '64 ANSI color pairs': len(re.findall(r'\x1b\[\d+;\d+m', s)) == 65,
        '64 grid cursor locations': all(f'\x1b[{y+11};{x+11}H' in s for y in range(8) for x in range(8)),
        'last line compatibility': '\x1b[25;2H' in s,
        'final colors': '\x1b[37;40mDone!' in s,
    }
else:
    checks = {
        'unsupported mode': 'ERROR: SCREEN unsupported mode' in s,
        'invalid color': 'ERROR: SCREEN COLOR index out of range' in s,
        'invalid cursor': 'ERROR: SCREEN LOCATE coordinates out of range' in s,
        'valid subsequent command': 'SCREEN NEGATIVE PASS' in s,
    }
for label, ok in checks.items():
    print(('PASS' if ok else 'FAIL') + ': ' + label)
if not all(checks.values()):
    print(repr(s[-1200:]))
    sys.exit(1)
