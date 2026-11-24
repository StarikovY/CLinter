#!/usr/bin/env python3
"""P3 graphics rounding and exact discrete parameters."""
import os
import pathlib
import subprocess
import sys
import tempfile

exe = sys.argv[1]
root = pathlib.Path(__file__).resolve().parent
env = dict(os.environ, CLINTER_GRAPHICS_HEADLESS='1')
def run(source):
    return subprocess.run([exe, str(source)], input='\n', text=True,
                          capture_output=True, timeout=15, env=env)
r = run(root / 'auto' / 'FloatGraphics.bas')
assert r.returncode == 0 and 'FLOAT GRAPHICS PASS' in r.stdout, (r.stdout, r.stderr)
assert 'ERROR:' not in r.stdout and 'FLOAT GRAPHICS FAIL' not in r.stdout, r.stdout
invalid = ['PSET (1,2),3.5', 'SCREEN 5.5', 'COLOR 2.5,0',
           'COPY (1,1)-(2,2),0.5 TO (3,3)', 'CIRCLE (10,10),-0.1',
           'PSET (100001,0),3', 'LOCATE 200,0',
           'PUT SPRITE 0.5,(1,2),3,0', 'PUT SPRITE 0,(1,2),3.5,0',
           'PUT SPRITE 0,(1,2),3,0.5', 'SPRITE$(0.5)="XXXXXXXX"']
with tempfile.TemporaryDirectory() as tmp:
    f = pathlib.Path(tmp) / 'bad.bas'
    for statement in invalid:
        f.write_text('10 SCREEN 20\n20 SPRITE$(0)="XXXXXXXX"\n30 '+statement+'\n40 PRINT "BAD CONTINUATION"\n50 END\n')
        r = run(f)
        assert 'ERROR:' in r.stdout and 'BAD CONTINUATION' not in r.stdout, (statement, r.stdout, r.stderr)
print('P3 FLOAT GRAPHICS PASS')
