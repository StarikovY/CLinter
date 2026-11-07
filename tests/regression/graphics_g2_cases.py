#!/usr/bin/env python3
import argparse, os, pathlib, subprocess, sys, tempfile
p=argparse.ArgumentParser();p.add_argument('--exe',required=True);p.add_argument('--case',choices=['valid','invalid'],required=True);a=p.parse_args()
env=dict(os.environ,CLINTER_GRAPHICS_HEADLESS='1')
if a.case=='valid':
    source=pathlib.Path(__file__).parent/'auto'/'GraphicsPhaseG2.bas'
    r=subprocess.run([a.exe,str(source)],input='\n',text=True,capture_output=True,timeout=15,env=env)
    if r.returncode or 'GRAPHICS G2 PASS' not in r.stdout or 'FAIL ' in r.stdout or 'ERROR:' in r.stdout:sys.exit(repr(r.stdout+r.stderr))
else:
    for command in ['SCREEN 5,,,4,0','SCREEN 7,,,0,2','COPY (0,0)-(10,10),4 TO (20,20)','COPY (10,10)-(1,1) TO (20,20)','COPY (0,0)-(10,10) TO (20,20),4']:
        with tempfile.TemporaryDirectory() as d:
            f=pathlib.Path(d)/'bad.bas';f.write_text('10 SCREEN 5\n20 '+command+'\n30 PRINT "UNEXPECTED CONTINUATION"\n')
            r=subprocess.run([a.exe,str(f)],input='\n',text=True,capture_output=True,timeout=15,env=env)
            if 'ERROR:' not in r.stdout or 'UNEXPECTED CONTINUATION' in r.stdout:sys.exit(command+': '+repr(r.stdout+r.stderr))
print('PASS: G2 '+a.case)
