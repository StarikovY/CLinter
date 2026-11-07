#!/usr/bin/env python3
import argparse, os, pathlib, subprocess, sys, tempfile, signal, time
p=argparse.ArgumentParser();p.add_argument('--exe',required=True);p.add_argument('--case',choices=['valid','invalid','interrupt'],required=True);a=p.parse_args()
env=dict(os.environ,CLINTER_GRAPHICS_HEADLESS='1')
if a.case=='valid':
    f=pathlib.Path(__file__).parent/'auto'/'GraphicsPhaseG3.bas'
    r=subprocess.run([a.exe,str(f)],input='\n',text=True,capture_output=True,timeout=15,env=env)
    assert r.returncode==0 and 'GRAPHICS G3 PASS' in r.stdout and 'FAIL ' not in r.stdout and 'ERROR:' not in r.stdout,repr(r.stdout+r.stderr)
elif a.case=='invalid':
    for mode,command in [(6,'PSET (2,2),4'),(8,'PSET (2,2),256'),(8,'SCREEN 8,,,1,0')]:
        with tempfile.TemporaryDirectory() as d:
            f=pathlib.Path(d)/'bad.bas';f.write_text(f'10 SCREEN {mode}\n20 {command}\n30 PRINT "UNEXPECTED CONTINUATION"\n')
            r=subprocess.run([a.exe,str(f)],input='\n',text=True,capture_output=True,timeout=15,env=env)
            assert 'ERROR:' in r.stdout and 'UNEXPECTED CONTINUATION' not in r.stdout,(command,r.stdout)
else:
    if sys.platform=='win32':
        print('SKIP: POSIX SIGINT test');sys.exit(0)
    with tempfile.TemporaryDirectory() as d:
        f=pathlib.Path(d)/'loop.bas';f.write_text('10 SCREEN 5\n20 GOTO 20\n')
        proc=subprocess.Popen([a.exe,str(f)],stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,env=env)
        try:
            time.sleep(.25);proc.send_signal(signal.SIGINT)
            out,err=proc.communicate(timeout=5)
            assert proc.returncode==0 and 'BREAK at line 20' in out,(proc.returncode,out,err)
        finally:
            if proc.poll() is None:proc.kill();proc.communicate()
print('PASS: G3 '+a.case)
