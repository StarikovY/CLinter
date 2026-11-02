# CLinter RT-11-style BASIC regression suite

This package combines both supplied archives unchanged under `legacy/`, plus
three new noninteractive assertion-based BASIC programs under `auto/`.
It does **not** claim complete historical RT-11 BASIC language conformance:
CLinter implements its own dialect, and several legacy programs currently
use syntax that this build rejects.

## Run

From the repository root:

    bash builds/build_scripts/regression_mac.sh Debug
    bash builds/build_scripts/regression_linux.sh Debug

On Windows ARM64 (with CMake, Python 3, CTest, VS build tools):

    builds\build_scripts\regression_winarm64.cmd Debug

Or after building:

    ctest --test-dir builds/drops/macos-arm64/cmake/Debug --output-on-failure

The default suite contains 9 enabled checks, including legacy smoke runs.
The smoke tests assert clean process exit, nonempty output and absence of
ERROR/FAIL markers, **not** exact numerical outputs for every statement.

## Known incompatibilities (disabled CTest cases)

- `factorial.bas`: INPUT prompt syntax fails at line 30; requires interactive input.
- `star.bas`: unsupported statement syntax at line 70.
- `regression.bas`: fails at OPEN file syntax (`ERROR: OPEN needs FOR`).
- `eoftest.bas`: fails at OPEN file syntax (`ERROR: OPEN needs FOR`).

The `RESTORE` statement also failed in an exploratory DATA/READ test
(`ERROR: OUT OF DATA`); the enabled DATA test checks READ only.

Other supplied interactive programs are archived but not run unattended:
`eliza.bas`, `bagels.bas`, `lunar.bas`, `utka.bas`, `utka2.bas`,
`pause.bas`, `ColorScreen1.bas`, and `Beer.bas`.

## Next coverage milestones

Add deterministic expected-output tests for numeric/string functions, array
bounds, relational operators, nested subroutines, FOR STEP, DATA/RESTORE,
SAVE/LOAD, file I/O, error handling, and interactive INPUT. Test cases should
only be enabled once their expected behavior is verified against CLinter's
intended dialect.
