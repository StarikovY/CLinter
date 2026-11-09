#!/usr/bin/env python3
"""G8 extended SCREEN 6/7/8 page, dimensions and color validation."""
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
                          capture_output=True, timeout=20, env=env)
r = run(root/'auto'/'GraphicsPhaseG8.bas')
assert r.returncode == 0 and 'GRAPHICS G8 PASS' in r.stdout, (r.stdout, r.stderr)
assert 'FAIL G8' not in r.stdout and 'ERROR:' not in r.stdout, r.stdout
invalid = ['SCREEN 8,,,1,0', 'SCREEN 8,,,2,0', 'SCREEN 7,,,2,0', 'SCREEN 6,,,4,0',
           'SCREEN 6\n30 COLOR 4,0', 'SCREEN 8\n30 COLOR 256,0']
for statement in invalid:
    with tempfile.TemporaryDirectory() as tmp:
        f = pathlib.Path(tmp)/'bad.bas'
        f.write_text('10 SCREEN 5\n20 '+statement+'\n40 PRINT "BAD CONTINUATION"\n50 END\n')
        r = run(f)
        assert 'ERROR:' in r.stdout and 'BAD CONTINUATION' not in r.stdout, (statement,r.stdout,r.stderr)
print('G8 EXTENDED GRAPHICS PASS')
