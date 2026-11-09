#!/usr/bin/env python3
"""M1: three concurrent MML voices, string variables and validation."""
import os
import pathlib
import subprocess
import sys
import tempfile

exe = sys.argv[1]
env = dict(os.environ, CLINTER_GRAPHICS_HEADLESS='1')
def run(lines):
    with tempfile.TemporaryDirectory() as tmp:
        path = pathlib.Path(tmp) / 'm1.bas'
        path.write_text('\n'.join(f'{(i+1)*10} {line}' for i,line in enumerate(lines))+'\n')
        return subprocess.run([exe, str(path)], capture_output=True, text=True, env=env, timeout=15)
valid = [
    'PLAY "T160 O4 L8 C D E F", "T160 O3 L4 C G", "T160 O2 L2 C"',
    'A$="O4 L4 C R E"',
    'PLAY A$, "O3 C", "O2 G"',
    'PLAY "T255 V0 R16", "T32 V15 C16", "O5 C# D- E."',
    'PRINT "M1 MUSIC PASS"', 'END',
]
r = run(valid)
assert r.returncode == 0 and 'M1 MUSIC PASS' in r.stdout and 'ERROR:' not in (r.stdout+r.stderr), (r.stdout,r.stderr)
invalid = [
    'PLAY "O9 C", "O4 C"',
    'PLAY "O4 C", "O3 C", "O2 C", "O1 C"',
    'PLAY "O4 C",',
    'PLAY "O4 C", "Q"',
    'PLAY "O4 C", "O4 C", "T0 C"',
]
for statement in invalid:
    r = run([statement,'PRINT "INVALID ACCEPTED"','END'])
    assert 'ERROR: PLAY' in (r.stdout+r.stderr) and 'INVALID ACCEPTED' not in r.stdout, (statement,r.stdout,r.stderr)
print('M1 THREE-VOICE PLAY PASS')
