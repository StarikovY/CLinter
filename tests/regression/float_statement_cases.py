import pathlib, subprocess, sys, tempfile
exe=str(pathlib.Path(sys.argv[1]).resolve())
auto=pathlib.Path(__file__).resolve().parent/'auto'
with tempfile.TemporaryDirectory() as d:
    def run(args, data=''):
        return subprocess.run([exe,*args],input=data,capture_output=True,text=True,errors="replace",cwd=d,timeout=10)
    for name, marker, data in [('FloatAssignment.bas','FLOAT ASSIGNMENT PASS','6.9\n'),('FloatStatements.bas','FLOAT STATEMENTS PASS','')]:
        r=run([str(auto/name)],data)
        assert r.returncode==0 and marker in r.stdout and 'ERROR:' not in r.stdout,(r.stdout,r.stderr)
    r=run([],f'LOAD "{auto / "FloatRenum.bas"}"\nRENUM 1000 10\nLIST\nRUN\nQUIT\n')
    assert '\n> FLOAT RENUM PASS\n' in r.stdout and '3.141592653589793' in r.stdout and 'ERROR:' not in r.stdout,(r.stdout,r.stderr)
    assert '\n> FLOAT RENUM FAIL\n' not in r.stdout,(r.stdout,r.stderr)
    p=pathlib.Path(d)/'bad.bas'
    for stmt in ['GOTO 1.5','GOSUB 1E20','PRINT VAL("1E999")','A%=1E20','PRINT TAB(1E20)','OPEN "x" FOR OUTPUT AS #1E20']:
        p.write_text('10 '+stmt+'\n20 END\n')
        r=run([str(p)])
        assert 'ERROR:' in r.stdout,(stmt,r.stdout,r.stderr)
    p.write_text('10 INPUT A\n20 END\n')
    r=run([str(p)],'NaN\n')
    assert 'ERROR:' in r.stdout,(r.stdout,r.stderr)
print('P6 FLOAT STATEMENTS PASS')
