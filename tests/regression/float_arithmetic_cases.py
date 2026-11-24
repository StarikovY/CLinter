import pathlib, subprocess, sys, tempfile
exe=sys.argv[1]
def run(p):
    return subprocess.run([exe,str(p)],capture_output=True,text=True,timeout=10)
r=run(pathlib.Path(__file__).parent/'auto'/'FloatArithmetic.bas')
assert r.returncode==0 and 'FLOAT ARITHMETIC PASS' in r.stdout and 'ERROR:' not in r.stdout, (r.stdout,r.stderr)
with tempfile.TemporaryDirectory() as d:
    p=pathlib.Path(d)/'bad.bas'
    for stmt in ['PRINT 1.2.3','PRINT 1E999','PRINT EXP(1000)','PRINT SQR(-1)','PRINT LOG(0)','PRINT 1E308*10','PRINT MOD(1E30,3)','PRINT ROUND(1,1E20)','FOR X=0 TO 1 STEP 0','FOR X=1E20 TO 2E20 STEP 1']:
        p.write_text('10 '+stmt+'\n20 PRINT "BAD CONTINUATION"\n30 END\n')
        r=run(p)
        assert 'ERROR:' in r.stdout and 'BAD CONTINUATION' not in r.stdout,(stmt,r.stdout,r.stderr)
print('P4 FLOAT ARITHMETIC PASS')
