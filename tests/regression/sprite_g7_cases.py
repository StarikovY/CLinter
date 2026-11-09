#!/usr/bin/env python3
"""G7: size modes, sprite collision state, and literal LOAD paths."""
import os, subprocess, sys, tempfile
from pathlib import Path
exe=sys.argv[1]
env=dict(os.environ,CLINTER_GRAPHICS_HEADLESS='1')
with tempfile.TemporaryDirectory() as tmp:
    base=Path(tmp)
    cases={
      'size_8_double': '10 SCREEN 5,2\n20 SPRITE$(0)="ABCDEFGH"\n30 PUT SPRITE 0,(10,10),3,0\n40 PRINT "G7 SIZE PASS"\n50 END\n',
      'size_16_double': '10 SCREEN 5,3\n20 SPRITE$(0)="ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"\n30 PUT SPRITE 0,(10,10),3,0\n40 PRINT "G7 LARGE PASS"\n50 END\n',
      'collision_controls': '10 SCREEN 5,1\n20 SPRITE$(0)="ABCDEFGHIJKLMNOPABCDEFGHIJKLMNOP"\n30 PUT SPRITE 0,(10,10),3,0\n40 PUT SPRITE 1,(10,10),4,0\n50 SPRITE ON\n60 SPRITE STOP\n70 SPRITE OFF\n80 PRINT "G7 EVENT PASS"\n90 END\n',
    }
    for name,body in cases.items():
        path=base/(name+'.bas');path.write_text(body)
        p=subprocess.run([exe,str(path)],input='\n',text=True,errors='replace',env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=15)
        if 'ERROR:' in p.stdout or 'PASS' not in p.stdout:raise SystemExit(f'{name}: {p.stdout}')
    target=base/'sample.bas';target.write_text('10 PRINT "G7 LOAD PASS"\n20 END\n')
    p=subprocess.run([exe],input=f'LOAD {target}\nRUN\nBYE\n',text=True,errors='replace',env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=15)
    if 'G7 LOAD PASS' not in p.stdout:raise SystemExit('unquoted LOAD failed: '+p.stdout)
print('G7 REGRESSION PASS')
