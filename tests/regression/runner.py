#!/usr/bin/env python3
"""CLinter regression runner; stdlib only; isolates file side effects."""
import argparse
import pathlib
import re
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parent
CASES = {
    'core_flow': ('auto/core_flow.bas', ['CORE-BEGIN','PASS:precedence','PASS:gosub','LOOP=','CORE-END'], False),
    'data_read': ('auto/data_read.bas', ['PASS:data','DATA-END'], False),
    'string_ops': ('auto/string_ops.bas', ['STRING=','PASS:string'], False),
    'math': ('legacy/RegrTest/mathtest.bas', ['MATH TESTS'], True),
    'strings': ('legacy/RegrTest/strtest.bas', [], True),
    'if': ('legacy/RegrTest/iftest.bas', [], True),
    'arrays': ('legacy/RegrTest/arraytest.bas', [], True),
    'arrays_extended': ('legacy/RegrTest/array_test.bas', [], True),
    'basic_1': ('legacy/Tests/BAS1.bas', [], True),
    'factorial': ('legacy/Tests/factorial.bas', [], True),
    'star': ('legacy/Tests/star.bas', [], True),
    'regression': ('legacy/Tests/regression.bas', [], True),
    'eof': ('legacy/Tests/eoftest.bas', [], True),
}
# Games and interactive programs are preserved but not run unattended:
# eliza, bagels, lunar, utka, utka2, pause, ColorScreen1, Beer.

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--exe', required=True)
    ap.add_argument('--case', choices=sorted(CASES), required=True)
    ap.add_argument('--timeout', type=int, default=12)
    args = ap.parse_args()
    exe = pathlib.Path(args.exe).resolve()
    name, required, legacy = CASES[args.case]
    src = ROOT / name
    with tempfile.TemporaryDirectory(prefix='clinter_reg_') as tmp:
        try:
            cp = subprocess.run([str(exe), str(src)], cwd=tmp, input='', capture_output=True,
                                text=True, errors='replace', timeout=args.timeout)
        except subprocess.TimeoutExpired:
            print(f'FAIL {args.case}: timeout (possibly interactive)', file=sys.stderr)
            return 1
    out = cp.stdout + '\n' + cp.stderr
    errors = []
    if cp.returncode: errors.append(f'exit status {cp.returncode}')
    for needle in required:
        if needle not in out: errors.append(f'missing {needle!r}')
    if re.search(r'(?im)^\s*(?:\*+\s*)?(?:FAIL(?::|\b)|ERROR(?::|\b)|SYNTAX ERROR\b)', out):
        errors.append('error/failure marker in output')
    if not out.strip(): errors.append('no output')
    if errors:
        print(f'FAIL {args.case}: {"; ".join(errors)}\n{out[-5000:]}')
        return 1
    print(f'PASS {args.case}'+ (' (legacy smoke; not full assertions)' if legacy else ''))
    return 0

if __name__ == '__main__': sys.exit(main())
