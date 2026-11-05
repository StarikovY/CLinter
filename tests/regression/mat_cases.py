#!/usr/bin/env python3
"""Strict MAT I/O and rejection checks. Runs CLinter in a temporary directory."""
import argparse
import pathlib
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parent / 'auto'
CASES = {
    'read': ('MatReadTest.bas', '', 'MAT READ PASS', False),
    'input': ('MatInputTest.bas', '11 12 21 22 31 32\n', 'MAT INPUT PASS', False),
    'singular': ('MatSingularTest.bas', '', 'ERROR: MAT singular matrix', True),
    'dimension_add': ('MatDimensionAddTest.bas', '', 'ERROR: MAT addition/subtraction dimension mismatch', True),
    'dimension_multiply': ('MatDimensionMultiplyTest.bas', '', 'ERROR: MAT multiplication dimension mismatch', True),
    'dimension_transpose': ('MatDimensionTransposeTest.bas', '', 'ERROR: MAT transpose dimension mismatch', True),
    'dimension_inverse': ('MatDimensionInverseTest.bas', '', 'ERROR: MAT INV requires square matrix', True),
    'input_invalid': ('MatInputInvalidTest.bas', 'oops\n', 'ERROR: MAT numeric INPUT expected', True),
}

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--exe', required=True)
    ap.add_argument('--case', required=True, choices=sorted(CASES))
    args = ap.parse_args()
    filename, stdin, expected, negative = CASES[args.case]
    with tempfile.TemporaryDirectory(prefix='clinter_mat_') as tmp:
        try:
            result = subprocess.run([str(pathlib.Path(args.exe).resolve()), str(ROOT / filename)],
                                    input=stdin, text=True, capture_output=True, cwd=tmp, timeout=10)
        except subprocess.TimeoutExpired:
            print('FAIL MAT ' + args.case + ': timed out')
            return 1
    output = result.stdout + result.stderr
    # CLinter currently reports runtime errors in stdout and may exit with status 0.
    # For negative cases, require the exact diagnostic and no continuation marker.
    ok = expected in output and 'FAIL MAT' not in output
    if not negative:
        ok = ok and result.returncode == 0 and 'ERROR:' not in output
    else:
        ok = ok and output.count('ERROR:') == 1
    if not ok:
        print('FAIL MAT ' + args.case + ': unexpected result (exit=' + str(result.returncode) + ')')
        print(output)
        return 1
    print('PASS MAT ' + args.case)
    return 0

if __name__ == '__main__':
    sys.exit(main())
