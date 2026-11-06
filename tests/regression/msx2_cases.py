"""Day 5 continuation: graphics text and MML regression, no audio/display needed."""
import argparse
import os
import subprocess
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('--exe', required=True)
p.add_argument('--case', choices=('valid', 'invalid'), required=True)
a = p.parse_args()
fixture = 'GraphicsMSX2Auto.bas' if a.case == 'valid' else 'GraphicsMSX2Invalid.bas'
env = dict(os.environ, CLINTER_GRAPHICS_HEADLESS='1')
r = subprocess.run([a.exe, str(Path(__file__).parent / 'auto' / fixture)],
                   capture_output=True, text=True, env=env, timeout=15)
output = r.stdout + r.stderr
if a.case == 'valid':
    assert r.returncode == 0 and 'MSX2 TEXT PLAY PASS' in output and 'FAIL' not in output, repr(output)
else:
    assert 'ERROR: PLAY octave out of range' in output and 'INVALID PLAY ACCEPTED' not in output, repr(output)
print('PASS: MSX2 ' + a.case)
