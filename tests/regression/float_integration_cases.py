"""P7 integration and event target conversion regression."""
import os
import pathlib
import subprocess
import sys
import tempfile

exe = str(pathlib.Path(sys.argv[1]).resolve())
auto = pathlib.Path(__file__).resolve().parent / 'auto'
env = dict(os.environ, CLINTER_GRAPHICS_HEADLESS='1')
with tempfile.TemporaryDirectory() as tmp:
    def run(path):
        return subprocess.run([exe, str(path)], input='\n', text=True,
                              capture_output=True, timeout=15, cwd=tmp, env=env)
    r = run(auto / 'FloatIntegration.bas')
    assert r.returncode == 0 and 'FLOAT INTEGRATION PASS' in r.stdout, (r.stdout, r.stderr)
    assert 'ERROR:' not in r.stdout and 'FLOAT INTEGRATION FAIL' not in r.stdout, r.stdout
    p = pathlib.Path(tmp) / 'event.bas'
    for event in ('SPRITE', 'PLAY'):
        for target in ('1.5', '2147483648', '1E20', '65536', '0', '1E999'):
            p.write_text(f'10 ON {event} GOSUB {target}\n20 PRINT "BAD CONTINUATION"\n30 END\n')
            r = run(p)
            assert 'ERROR:' in r.stdout and 'BAD CONTINUATION' not in r.stdout, (event, target, r.stdout, r.stderr)
        p.write_text(f'10 ON {event} GOSUB 100\n20 PRINT "EVENT PASS"\n30 END\n100 RETURN\n')
        r = run(p)
        assert r.returncode == 0 and 'EVENT PASS' in r.stdout and 'ERROR:' not in r.stdout, (event, r.stdout, r.stderr)
print('P7 FLOAT INTEGRATION PASS')
