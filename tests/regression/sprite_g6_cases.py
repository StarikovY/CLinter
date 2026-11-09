#!/usr/bin/env python3
"""G6 headless sprite integration and rejection tests."""
import os
import subprocess
import sys
import tempfile
from pathlib import Path

exe = sys.argv[1]
cases = [
    ('normal', '''10 SCREEN 5,1
20 SPRITE$(0)="ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"
30 PUT SPRITE 0,(15,25),3,0
40 PUT SPRITE 0,(30,45),2,0
50 PUT SPRITE 0
60 PRINT "SPRITE G6 PASS"
70 END
''', 'SPRITE G6 PASS', False),
    ('undefined', '10 SCREEN 5\n20 PUT SPRITE 0,(5,5),2,0\n', 'ERROR: SPRITE undefined pattern', True),
    ('range', '10 SCREEN 5\n20 PUT SPRITE 32,(5,5),2,0\n', 'ERROR: SPRITE slot range', True),
    ('length', '10 SCREEN 5\n20 SPRITE$(0)="SHORT"\n', 'ERROR: SPRITE pattern length', True),
    ('size', '10 SCREEN 5,4\n', 'ERROR: SCREEN sprite size', True),
]
for name, code, expected, negative in cases:
    with tempfile.TemporaryDirectory() as d:
        path = Path(d) / (name + '.bas')
        path.write_text(code)
        env = dict(os.environ, CLINTER_GRAPHICS_HEADLESS='1')
        result = subprocess.run([exe, str(path)], input='\n', text=True,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                env=env, timeout=15)
        if expected not in result.stdout or (not negative and 'ERROR:' in result.stdout):
            print(f'FAIL {name}: {result.stdout}')
            sys.exit(1)
print('SPRITE G6 REGRESSION PASS')
