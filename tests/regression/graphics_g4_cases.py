#!/usr/bin/env python3
import argparse,os,pathlib,subprocess,tempfile
p=argparse.ArgumentParser();p.add_argument('--exe',required=True);p.add_argument('--case',choices=['valid','invalid'],required=True);a=p.parse_args()
env=dict(os.environ,CLINTER_GRAPHICS_HEADLESS='1')
if a.case=='valid':
 f=pathlib.Path(__file__).parent/'auto'/'GraphicsPhaseG4.bas'
 r=subprocess.run([a.exe,str(f)],input='\n',text=True,capture_output=True,timeout=15,env=env)
 assert r.returncode==0 and 'GRAPHICS G4 PASS' in r.stdout and 'FAIL ' not in r.stdout and 'ERROR:' not in r.stdout,repr(r.stdout+r.stderr)
else:
 for cmd in ['COLOR=(16,1,2,3)','COLOR=(2,8,0,0)','COLOR=(2,1,2)','COLOR=(2,1,2,3,4)']:
  with tempfile.TemporaryDirectory() as d:
   f=pathlib.Path(d)/'invalid.bas';f.write_text('10 SCREEN 5\n20 '+cmd+'\n30 PRINT "UNEXPECTED CONTINUATION"\n')
   r=subprocess.run([a.exe,str(f)],input='\n',text=True,capture_output=True,timeout=15,env=env)
   assert 'ERROR:' in r.stdout and 'UNEXPECTED CONTINUATION' not in r.stdout,(cmd,r.stdout)
print('PASS: G4 '+a.case)
