#!/usr/bin/env python3
"""Headless G5 integration and malformed-statement checks."""
import argparse
import os
import pathlib
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('--exe', required=True)
parser.add_argument('--case', required=True, choices=('valid', 'invalid'))
args = parser.parse_args()
root = pathlib.Path(__file__).resolve().parent
env = dict(os.environ, CLINTER_GRAPHICS_HEADLESS='1')

def run(path):
    return subprocess.run([args.exe, str(path)], input='\n', text=True,
                          capture_output=True, timeout=20, env=env)

if args.case == 'valid':
    result = run(root / 'auto' / 'GraphicsPhaseG5.bas')
    assert result.returncode == 0, (result.stdout, result.stderr)
    assert 'GRAPHICS G5 PASS' in result.stdout, result.stdout
    assert 'FAIL ' not in result.stdout and 'ERROR:' not in result.stdout, result.stdout
else:
    cases = (
        'PSET (1,1),16',
        'PRESET (1,1),-1',
        'LINE (1,1)-(10,10),2,Q',
        'CIRCLE (10,10),-1',
        'PAINT (1,1),16',
        'DRAW "Z10"',
        'COPY (10,10)-(1,1) TO (20,20)',
        'SCREEN 5,,,4,0',
    )
    for statement in cases:
        with tempfile.TemporaryDirectory() as tmp:
            source = pathlib.Path(tmp) / 'invalid.bas'
            source.write_text('10 SCREEN 5\n20 ' + statement +
                              '\n30 PRINT "UNEXPECTED CONTINUATION"\n')
            result = run(source)
            assert 'ERROR:' in result.stdout, (statement, result.stdout, result.stderr)
            assert 'UNEXPECTED CONTINUATION' not in result.stdout, statement
print('PASS: G5 ' + args.case)
