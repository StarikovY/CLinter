#!/usr/bin/env python3
"""Run unchanged BAS1.bas under a real pseudo-terminal, inject X, enforce timeout."""
import os
import pty
import select
import subprocess
import sys
import time
from pathlib import Path

exe, program = sys.argv[1:3]
master, slave = pty.openpty()
try:
    proc = subprocess.Popen([exe, program], stdin=slave, stdout=slave, stderr=slave,
                            start_new_session=True)
finally:
    os.close(slave)
output = bytearray()
deadline = time.monotonic() + 8
last_send = 0.0
try:
    while time.monotonic() < deadline:
        now = time.monotonic()
        if now - last_send >= 0.15:
            os.write(master, b'x')
            last_send = now
        ready, _, _ = select.select([master], [], [], 0.05)
        if ready:
            try:
                chunk = os.read(master, 8192)
            except OSError:
                chunk = b''
            if chunk:
                output.extend(chunk)
        if proc.poll() is not None:
            break
    else:
        proc.kill()
        print('FAIL BAS1: timeout')
        sys.exit(1)
    out = output.decode('utf-8', errors='replace')
    if proc.returncode != 0 or 'Finish' not in out or 'ERROR:' in out:
        print('FAIL BAS1:', proc.returncode, repr(out[-1000:]))
        sys.exit(1)
    print('PASS BAS1 interactive INKEY$')
finally:
    if proc.poll() is None:
        proc.kill()
    proc.wait()
    os.close(master)
