#!/usr/bin/env python3
import argparse, os, pathlib, subprocess, sys, tempfile
p=argparse.ArgumentParser();p.add_argument('--exe',required=True);p.add_argument('--case',choices=['valid','invalid'],required=True);a=p.parse_args()
env=dict(os.environ,CLINTER_GRAPHICS_HEADLESS='1')
if a.case=='valid':
    source=pathlib.Path(__file__).parent/'auto'/'GraphicsPhaseG1.bas'
    r=subprocess.run([a.exe,str(source)],input='\n',text=True,capture_output=True,timeout=15,env=env)
    if r.returncode or 'GRAPHICS G1 PASS' not in r.stdout or 'FAIL ' in r.stdout or 'ERROR:' in r.stdout:sys.exit(repr(r.stdout+r.stderr))
else:
    for command in ['DRAW "Z10"','PAINT (1,1),4,16','CIRCLE (10,10),5,2,,,0','LINE (1,1)-STEP(3,3),16']:
        with tempfile.TemporaryDirectory() as d:
            f=pathlib.Path(d)/'bad.bas';f.write_text('10 SCREEN 5\n20 '+command+'\n30 PRINT "UNEXPECTED CONTINUATION"\n')
            r=subprocess.run([a.exe,str(f)],input='\n',text=True,capture_output=True,timeout=15,env=env)
            if 'ERROR: GRAPHICS' not in r.stdout or 'UNEXPECTED CONTINUATION' in r.stdout:sys.exit(command+': '+repr(r.stdout+r.stderr))
print('PASS: G1 '+a.case)
