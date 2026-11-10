#!/usr/bin/env python3
"""D2 demo validation: flying silhouette, randomized speed, hunter exit."""
import os
import pathlib
import re
import subprocess
import sys
import tempfile

exe = sys.argv[1]
src = (pathlib.Path(__file__).parent / 'auto' / 'DuckHuntD1.bas').read_text()
lines = {int(n): statement for n, statement in re.findall(r'^([0-9]+) (.*)$', src, re.M)}
assert '9010 V=3+INT(RND(1)*7)' in src
assert '6670 X=20: Y=112: GOSUB 9000: T=0' in src
assert '7005 PRINT "IT IS TOO COLD! HUNTER GOES HOME..."' in src
assert '7008 FOR HX=256 TO 480 STEP 4' in src
assert '7019 LINE (HX+14,179)-(HX+19,204),15' in src
assert '7022 NEXT HX' in src
assert '7010 LINE (HX-24,139)-(HX+23,211),4,BF' in src
hexpat = re.search(r'^61 SPRITE\$\(0\)=HEX\$\("([0-9A-F]{64})"\)$', src, re.M)
assert hexpat
pattern = bytes.fromhex(hexpat.group(1))
rows = [((pattern[y] << 8) | pattern[16+y]) for y in range(16)]
assert max(row.bit_count() for row in rows) >= 12, 'duck needs horizontally extended wings'
assert rows[0] == 0 and rows[-1] == 0
with tempfile.TemporaryDirectory() as tmp:
    path = pathlib.Path(tmp) / 'd2_speed.bas'
    program = ['10 RANDOMIZE', '20 FOR I=1 TO 20', '30 GOSUB 9000',
               '40 IF V<3 OR V>9 THEN PRINT "D2 FAIL": END',
               '50 NEXT I', '60 PRINT "D2 PASS"', '70 END']
    program += [f'{number} {lines[number]}' for number in (9000, 9010, 9020, 9030)]
    path.write_text('\n'.join(program) + '\n')
    env = dict(os.environ, CLINTER_GRAPHICS_HEADLESS='1')
    result = subprocess.run([exe, str(path)], env=env, capture_output=True,
                            text=True, timeout=15)
    assert result.returncode == 0 and 'D2 PASS' in result.stdout and 'ERROR:' not in result.stdout + result.stderr, (result.stdout, result.stderr)
print('D2 DUCK PASS')
