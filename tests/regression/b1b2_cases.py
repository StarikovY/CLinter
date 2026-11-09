import subprocess, sys, tempfile, pathlib
program = '''10 DEF FNSQ(X)=X*X
20 PRINT "FN=";FNSQ(4)
30 A=0
40 WHILE A<3
50 A=A+1
60 WEND
70 PRINT "LOOP=";A
80 X=5:Y=9
90 SWAP X,Y
100 PRINT "SWAP=";X;",";Y
110 A$="HELLO"
120 MID$(A$,2,3)="XYZ"
130 PRINT "MID=";A$
140 DIM B(3)
150 ERASE B
160 CLEAR
170 PRINT "CLEAR=";X
180 END
'''
with tempfile.TemporaryDirectory() as d:
    p=pathlib.Path(d)/'b1b2.bas';p.write_text(program)
    r=subprocess.run([sys.argv[1],str(p)],capture_output=True,text=True,timeout=15)
    out=r.stdout+r.stderr
    for token in ('FN=16','LOOP=3','SWAP=9,5','MID=HXYZO','CLEAR=0'):
        if token not in out:
            raise SystemExit(f'Missing {token!r} in output:\n{out}')
    if r.returncode != 0 or 'ERROR:' in out:
        raise SystemExit(out)
print('B1+B2 PASS')
