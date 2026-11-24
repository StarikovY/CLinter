import pathlib, subprocess, sys, tempfile
exe=sys.argv[1]
def run(p):
    return subprocess.run([exe,str(p)],capture_output=True,text=True,timeout=10)
for name, marker in [('FloatSubscript.bas','FLOAT SUBSCRIPT PASS'),('FloatMatrix.bas','FLOAT MATRIX PASS')]:
    r=run(pathlib.Path(__file__).parent/'auto'/name)
    assert r.returncode==0 and marker in r.stdout and 'ERROR:' not in r.stdout,(name,r.stdout,r.stderr)
with tempfile.TemporaryDirectory() as d:
    p=pathlib.Path(d)/'bad.bas'
    for stmt in ['A(3.5)=1','DIM B(2.5)','A(-0.5)=1','A(1E30)=1','A(4)=1','DIM B(50000,50000)']:
        p.write_text('10 DIM A(3)\n20 '+stmt+'\n30 END\n')
        r=run(p)
        assert 'ERROR:' in r.stdout,(stmt,r.stdout,r.stderr)
print('P5 FLOAT ARRAY PASS')
