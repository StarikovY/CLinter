#!/usr/bin/env python3
"""M2: numeric note N0..N96, Q1..Q8 articulation, independent voices."""
import os, pathlib, subprocess, sys, tempfile
exe=sys.argv[1]
env=dict(os.environ,CLINTER_GRAPHICS_HEADLESS='1')
def run(lines):
    with tempfile.TemporaryDirectory() as tmp:
        p=pathlib.Path(tmp)/'m2.bas'
        p.write_text('\n'.join(f'{10*(i+1)} {line}' for i,line in enumerate(lines))+'\n')
        return subprocess.run([exe,str(p)],capture_output=True,text=True,env=env,timeout=15)
valid=['PLAY "T160 O4 L8 Q2 C D Q8 E F", "T160 N46 N0 N58", "Q1 N1 Q8 N96"',
       'PLAY "Q8 N46", "Q1 N46", "Q4 R4"',
       'PRINT "M2 MUSIC PASS"','END']
r=run(valid)
assert r.returncode==0 and 'M2 MUSIC PASS' in r.stdout and 'ERROR:' not in (r.stdout+r.stderr),(r.stdout,r.stderr)
for statement in ['PLAY "Q0 C"','PLAY "Q9 C"','PLAY "N97"','PLAY "N-1"','PLAY "N"','PLAY "Q"','PLAY "Q2 N999"']:
    r=run([statement,'PRINT "INVALID ACCEPTED"','END'])
    assert 'ERROR: PLAY' in (r.stdout+r.stderr) and 'INVALID ACCEPTED' not in r.stdout,(statement,r.stdout,r.stderr)
print('M2 NUMERIC NOTES AND ARTICULATION PASS')
