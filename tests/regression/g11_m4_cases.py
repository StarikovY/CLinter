#!/usr/bin/env python3
"""G11/M4 headless regression: variable sprite patterns and PLAY ACTIVE."""
import os
import pathlib
import subprocess
import sys
import tempfile

exe = sys.argv[1]
env = dict(os.environ, CLINTER_GRAPHICS_HEADLESS='1')

def run(lines):
    with tempfile.TemporaryDirectory() as directory:
        program = pathlib.Path(directory) / 'g11m4.bas'
        program.write_text('\n'.join(f'{(i + 1) * 10} {line}' for i, line in enumerate(lines)) + '\n')
        return subprocess.run([exe, str(program)], env=env, text=True,
                              capture_output=True, timeout=15)

r = run(['SCREEN 5', 'P$="XXXXXXXX"', 'SPRITE$(0)=P$',
         'PUT SPRITE 0,(20,20),3,0', 'PRINT "G11 PASS"', 'END'])
assert r.returncode == 0 and 'G11 PASS' in r.stdout and 'ERROR:' not in r.stdout + r.stderr, (r.stdout, r.stderr)

r = run(['PLAY BACKGROUND "T180 O4 L16 C E G"', 'PLAY ACTIVE A',
         'PLAY PROGRESS P', 'PRINT "ACTIVE=";A', 'PRINT "PROGRESS=";P',
         'PLAY STOP', 'PLAY ACTIVE A', 'PRINT "STOPPED=";A', 'END'])
assert r.returncode == 0 and 'ACTIVE=0' in r.stdout and 'STOPPED=0' in r.stdout and 'ERROR:' not in r.stdout + r.stderr, (r.stdout, r.stderr)

for statement in ['SPRITE$(0)=A$', 'SPRITE$(0)=4', 'PLAY ACTIVE A$', 'PLAY ACTIVE']:
    r = run(['SCREEN 5', statement, 'PRINT "INVALID ACCEPTED"', 'END'])
    assert 'ERROR:' in r.stdout + r.stderr and 'INVALID ACCEPTED' not in r.stdout, (statement, r.stdout, r.stderr)
print('G11 M4 PASS')
