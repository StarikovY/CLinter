#!/usr/bin/env python3
"""D1 integration smoke test (headless, bounded; interactive game tested manually)."""
import os, pathlib, re, subprocess, sys, tempfile
exe=sys.argv[1]
demo=pathlib.Path(__file__).parent/'auto'/'DuckHuntD1.bas'
source=demo.read_text()
lines={int(n):s for n,s in re.findall(r'^([0-9]+) (.*)$',source,re.M)}
assert len(lines)>=90
for target in re.findall(r'\b(?:GOTO|GOSUB)\s+(\d+)',source,re.I):
    assert int(target) in lines, f'missing target {target}'
assert 'PLAY BACKGROUND' in source and 'SPRITE$(0)' in source
assert 'PRESS ENTER TO EXIT' in source
# SPRITE positions must be integral: fractional falling-duck X caused runtime failure.
assert 'FX=INT(FX+DX/6)' in source
assert re.search(r'^61 SPRITE\$\(0\)=HEX\$\("[0-9A-Fa-f]{64}"\)$', source, re.M)
assert re.search(r'^70 SPRITE\$\(1\)=HEX\$\("[0-9A-Fa-f]{64}"\)$', source, re.M)
assert 'SPRITE$(0)=P$' not in source
with tempfile.TemporaryDirectory() as d:
    test=pathlib.Path(d)/'d1_smoke.bas'
    statements=[lines[k] for k in (30,40,50,60,61,70,80,90)] + [
        'LINE (0,142)-(511,211),4,BF',
        'PUT SPRITE 0,(20,110),10,0',
        'PUT SPRITE 1,(256,184),15,1',
        'PLAY BACKGROUND "T240 O4 L16 C E G"',
        'PLAY ACTIVE A', 'PLAY STOP',
        'PLAY ACTIVE A', 'IF A<>0 THEN PRINT "D1 FAIL": END',
        'PRINT "D1 PASS"', 'END']
    test.write_text('\n'.join(f'{10*i} {s}' for i,s in enumerate(statements,1))+'\n')
    env=dict(os.environ,CLINTER_GRAPHICS_HEADLESS='1')
    result=subprocess.run([exe,str(test)],env=env,text=True,capture_output=True,timeout=15)
    assert result.returncode==0 and 'D1 PASS' in result.stdout and 'ERROR:' not in result.stdout+result.stderr,(result.stdout,result.stderr)
print('D1 DUCK PASS')
