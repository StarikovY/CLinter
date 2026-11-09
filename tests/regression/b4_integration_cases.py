#!/usr/bin/env python3
"""B4: deterministic cross-subsystem MSX2 BASIC compatibility smoke tests."""
import os
import pathlib
import subprocess
import sys
import tempfile

exe = sys.argv[1]
env = dict(os.environ, CLINTER_GRAPHICS_HEADLESS='1', SDL_AUDIODRIVER='dummy')

def run(lines):
    with tempfile.TemporaryDirectory() as d:
        p = pathlib.Path(d) / 'b4.bas'
        p.write_text('\n'.join(f'{(i + 1) * 10} {line}' for i, line in enumerate(lines)) + '\n')
        return subprocess.run([exe, str(p)], input='\n', capture_output=True,
                              text=True, env=env, timeout=20)

def check(name, lines, marker):
    r = run(lines)
    assert r.returncode == 0 and marker in r.stdout and 'ERROR:' not in r.stdout, \
        (name, r.returncode, r.stdout, r.stderr)
    print(f'B4 {name} PASS')

check('GRAPHICS_MUSIC', [
    'SCREEN 5', 'COLOR 15,0', 'CLS', 'LINE (10,10)-(70,60),4,BF',
    'IF POINT(30,30)<>4 THEN PRINT "FAIL COLOR":END',
    'CIRCLE (100,60),12,7', 'PLAY BACKGROUND "T180 O4 L16 C E G C"',
    'PLAY PROGRESS P', 'IF P<0 OR P>100 THEN PRINT "FAIL PROGRESS":END',
    'PLAY STOP', 'PLAY PROGRESS P',
    'IF P<>100 THEN PRINT "FAIL STOP":END',
    'PRINT "B4 GRAPHICS MUSIC PASS"', 'END'], 'B4 GRAPHICS MUSIC PASS')

check('LANGUAGE_GRAPHICS', [
    'DEF FNSQ(X)=X*X', 'A=0', 'WHILE A<3', 'A=A+1', 'WEND',
    'IF FNSQ(A)<>9 THEN PRINT "FAIL FN":END',
    'A$="HELLO"', 'MID$(A$,2,3)="XYZ"',
    'IF A$<>"HXYZO" THEN PRINT "FAIL MID":END',
    'SCREEN 5', 'PSET (20,20),A',
    'IF POINT(20,20)<>3 THEN PRINT "FAIL POINT":END',
    'PRINT "B4 LANGUAGE GRAPHICS PASS"', 'END'], 'B4 LANGUAGE GRAPHICS PASS')

check('SPRITE_SOUND', [
    'SCREEN 5', 'SPRITE$(0)="XXXXXXXX"',
    'PUT SPRITE 0,(20,20),3,0', 'SOUND 0,100', 'SOUND 1,1',
    'BEEP', 'PRINT "B4 SPRITE SOUND PASS"', 'END'], 'B4 SPRITE SOUND PASS')

for statement in ['SCREEN 8,,,9,0', 'SOUND 16,0', 'PLAY PROGRESS -1']:
    r = run([statement, 'PRINT "INVALID CONTINUATION"', 'END'])
    assert 'ERROR:' in r.stdout + r.stderr and 'INVALID CONTINUATION' not in r.stdout, \
        (statement, r.stdout, r.stderr)
print('B4 INVALID INPUT PASS')
