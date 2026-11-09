import pathlib, subprocess, sys, tempfile
program='''10 DEFINT A-C
20 A=3.9
30 PRINT "INT=";A
40 DEFSTR S
50 S="HELLO"
60 PRINT "STR=";S
70 ON ERROR GOTO 150
80 GOTO 9999
90 PRINT "RESUMED"
100 END
150 PRINT "TRAPPED"
160 RESUME 90
'''
with tempfile.TemporaryDirectory() as d:
    path=pathlib.Path(d)/'b3.bas';path.write_text(program)
    r=subprocess.run([sys.argv[1],str(path)],capture_output=True,text=True,timeout=15)
    out=r.stdout+r.stderr
    for marker in ('INT=3','STR=HELLO','TRAPPED','RESUMED'):
        if marker not in out:raise SystemExit(f'Missing {marker}:\n{out}')
    if r.returncode:raise SystemExit(f'Exit {r.returncode}:\n{out}')
print('B3 PASS')
