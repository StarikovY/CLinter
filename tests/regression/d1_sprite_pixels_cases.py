#!/usr/bin/env python3
"""Exact sprite bit patterns and D1 demo regression."""
import os,pathlib,subprocess,sys,tempfile
exe=sys.argv[1]
demo=pathlib.Path(__file__).parent/'auto'/'DuckHuntD1.bas'
lines=demo.read_text().splitlines()
assert any('SPRITE$(0)=HEX$(' in x for x in lines)
assert any('SPRITE$(1)=HEX$(' in x for x in lines)
assert any('BX=248' in x for x in lines)
setup=[next(x.split(' ',1)[1] for x in lines if x.startswith(f'{n} ')) for n in (30,40,50,61,70)]
def run(commands):
    with tempfile.TemporaryDirectory() as d:
        p=pathlib.Path(d)/'test.bas'
        p.write_text('\n'.join(f'{10*(i+1)} {s}' for i,s in enumerate(commands))+'\n')
        env=dict(os.environ,CLINTER_GRAPHICS_HEADLESS='1')
        return subprocess.run([exe,str(p)],env=env,text=True,capture_output=True,timeout=15)
r=run(setup+['PUT SPRITE 0,(20,110),10,0','PUT SPRITE 1,(248,169),15,1','PRINT "D1 PIXELS PASS"','END'])
assert r.returncode==0 and 'D1 PIXELS PASS' in r.stdout and 'ERROR:' not in r.stdout+r.stderr,(r.stdout,r.stderr)
for bad in ['HEX$("00")','HEX$("'+'GG'*32+'")']:
    r=run(['SCREEN 7,1',f'SPRITE$(0)={bad}','END'])
    assert 'ERROR: SPRITE hex pattern' in r.stdout,(bad,r.stdout,r.stderr)
print('D1 PIXELS PASS')
