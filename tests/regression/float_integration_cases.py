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

# Real terminal Enter is CR, unlike piped regression input (LF).
if os.name == 'posix':
    import pty
    import select
    import termios
    import time
    with tempfile.TemporaryDirectory() as tmp:
        p = pathlib.Path(tmp) / 'terminal.bas'
        p.write_text('10 INPUT "NUMBER: "; N\n20 INPUT "NAME: "; A$\n30 IF N=42 AND A$="ADA" THEN PRINT "TERMINAL PASS"\n40 END\n')
        for damaged in (False, True):
            master, slave = pty.openpty()
            proc = None
            try:
                mode = termios.tcgetattr(slave)
                if damaged:
                    mode[0] &= ~termios.ICRNL
                    mode[3] &= ~termios.ISIG
                    mode[6][termios.VINTR] = b"\x00"
                else:
                    mode[0] |= termios.ICRNL
                termios.tcsetattr(slave, termios.TCSANOW, mode)
                proc = subprocess.Popen([exe, str(p)], stdin=slave, stdout=slave, stderr=slave, env=env)
                output = bytearray()
                def expect(token):
                    deadline = time.monotonic() + 5
                    while token not in output and time.monotonic() < deadline:
                        if select.select([master], [], [], 0.1)[0]:
                            output.extend(os.read(master, 4096))
                    assert token in output, (damaged, bytes(output))
                expect(b'NUMBER: ')
                os.write(master, b'42\r')
                expect(b'NAME: ')
                os.write(master, b'ADA\r')
                expect(b'TERMINAL PASS')
                assert proc.wait(timeout=5) == 0 and b'^M' not in output, bytes(output)
                assert termios.tcgetattr(slave)[0] & termios.ICRNL
            finally:
                if proc is not None and proc.poll() is None:
                    proc.kill(); proc.wait()
                os.close(master); os.close(slave)

    # Use a controlling terminal so Ctrl+C generates a real SIGINT.
    import signal
    with tempfile.TemporaryDirectory() as tmp:
        p = pathlib.Path(tmp) / 'interrupt.bas'
        p.write_text('10 INPUT "WAIT: "; N\n20 PRINT "BAD CONTINUATION"\n30 END\n')
        pid, master = pty.fork()
        if pid == 0:
            mode = termios.tcgetattr(0)
            mode[0] &= ~termios.ICRNL
            mode[3] &= ~termios.ISIG
            mode[6][termios.VINTR] = b"\x00"
            termios.tcsetattr(0, termios.TCSANOW, mode)
            os.execve(exe, [exe, str(p)], env)
        output = bytearray()
        try:
            def wait_for(token):
                deadline = time.monotonic() + 5
                while token not in output and time.monotonic() < deadline:
                    if select.select([master], [], [], 0.1)[0]:
                        try:
                            output.extend(os.read(master, 4096))
                        except OSError:
                            break
                assert token in output, bytes(output)
            wait_for(b'WAIT: ')
            os.write(master, b'\x03')
            wait_for(b'BREAK at line 10')
            assert b'BAD CONTINUATION' not in output, bytes(output)
            deadline = time.monotonic() + 5
            while time.monotonic() < deadline:
                done, status = os.waitpid(pid, os.WNOHANG)
                if done:
                    pid = 0
                    assert os.WIFEXITED(status) and os.WEXITSTATUS(status) == 0
                    break
                time.sleep(0.05)
            assert pid == 0, 'Ctrl+C did not terminate batch execution'
        finally:
            if pid:
                os.kill(pid, signal.SIGKILL)
                os.waitpid(pid, 0)
            os.close(master)
