#!/usr/bin/env python3
"""G10/M3: collision dispatch, PLAY completion and PSG validation."""
import os, pathlib, subprocess, sys, tempfile
exe=sys.argv[1]
env=dict(os.environ,CLINTER_GRAPHICS_HEADLESS='1')
def run(lines):
    with tempfile.TemporaryDirectory() as d:
        f=pathlib.Path(d)/'test.bas'
        f.write_text('\n'.join(f'{(i+1)*10} {s}' for i,s in enumerate(lines))+'\n')
        return subprocess.run([exe,str(f)],env=env,text=True,capture_output=True,timeout=15)
# Handler after END: event dispatch must return to the correct line.
r=run(['SCREEN 5','SPRITE$(0)="XXXXXXXX"','PUT SPRITE 0,(20,20),3,0',
       'PUT SPRITE 1,(20,20),4,0','ON SPRITE GOSUB 100','SPRITE ON',
       'PRINT "AFTER"','END','REM reserved','PRINT "COLLISION EVENT"','SPRITE OFF','RETURN'])
assert r.returncode==0 and r.stdout.count('COLLISION EVENT')==1 and 'AFTER' in r.stdout and 'ERROR:' not in r.stdout,(r.stdout,r.stderr)
r=run(['ON PLAY GOSUB 70','PLAY ON','PLAY "T200 O4 L16 C E G", "O3 C", "O2 C"',
       'PRINT "AFTER PLAY"','END','REM reserved','PRINT "PLAY EVENT"','PLAY OFF','RETURN'])
assert r.returncode==0 and r.stdout.count('PLAY EVENT')==1 and 'AFTER PLAY' in r.stdout and 'ERROR:' not in r.stdout,(r.stdout,r.stderr)
r=run(['SOUND 0,100','SOUND 1,1','SOUND 7,62','SOUND 8,12','BEEP','PRINT "PSG PASS"','END'])
assert r.returncode==0 and 'PSG PASS' in r.stdout and 'ERROR:' not in r.stdout,(r.stdout,r.stderr)
for stmt in ['SOUND 16,0','SOUND 1,16','SOUND 8,32','SOUND 0,256','BEEP 3','ON SPRITE GOSUB -1']:
    r=run([stmt,'PRINT "INVALID ACCEPTED"','END'])
    assert 'ERROR:' in r.stdout+r.stderr and 'INVALID ACCEPTED' not in r.stdout,(stmt,r.stdout,r.stderr)
print('G10 M3 PASS')
