# CLinter BASIC compatibility audit — baseline v0.1
Date: 2026-11-01

## Scope and evidence
Source reviewed: `CLinter_snapshot_small(2).zip` and the subsequent CLI/history patch, as assembled in the local working tree. This is a **source inventory and test-gap assessment**, not a certification of DEC RT-11 BASIC-11 conformance. Historical compatibility claims require an identified DEC manual/version and test cases. No interpreter behavior was modified in this audit.

## Status key
- **Present**: a source implementation was located; not necessarily covered by assertions.
- **Partial / investigate**: implemented but compatibility, syntax, or reported failure needs examination.
- **Not found**: no implementation identified in this source review; not definitive proof of absence.
- **Extension**: CLinter functionality not assumed to be part of historical DEC BASIC.

## Source-level matrix
| Area | Syntax / feature | Status | Evidence / follow-up |
|---|---|---|---|
| Program control | NEW, RUN, LIST, SAVE, LOAD, RENUM | Present | `main.cpp`, `help.cpp`; add exact-output roundtrips |
| Flow | GOTO, GOSUB/RETURN, FOR/NEXT/STEP, IF/THEN, STOP/END | Present | `wxecut.cpp`, `help.cpp`; nested/boundary tests |
| Computed branch | ON expression GOTO / ON expression GOSUB | Present | `wxecut.cpp` near line 478; 64-target fixed array; test out-of-range and nested GOSUB |
| Data | DATA, READ, RESTORE, RESTORE line | Partial / investigate | `wxecut.cpp` near lines 913–979; archived README reports OUT OF DATA in exploratory RESTORE test |
| Arrays | DIM, numeric/string multidimensional arrays | Present | `help.cpp`, parser/runtime; check default lower bounds and bounds errors |
| Console input | INPUT, INPUT "prompt"; variable | Partial / investigate | `wxecut.cpp` near line 605; archived legacy `factorial.bas` prompt incompatibility |
| File input/output | OPEN ... FOR INPUT/OUTPUT/APPEND AS #n, CLOSE, PRINT #, INPUT #, LINE INPUT #, EOF | Partial / investigate | `wxecut.cpp` near line 655; archived `regression.bas` and `eoftest.bas` fail on OPEN syntax |
| Expressions | + - * / ^, comparisons, AND OR XOR NOT | Present | `help.cpp`, `Parse.cpp`; precedence and numeric error tests needed |
| Math | ABS ATN COS SIN TAN EXP LOG LOG10 SQR POW INT SGN RND PI | Present | `help.cpp`, parser; verify semantics against chosen dialect |
| Strings | LEN ASC VAL CHR$ STR$ SEG$ TRM$ | Present | `help.cpp`; also inspect LEFT$/RIGHT$/MID$ implementation |
| User functions | DEF FN | Not found | Proposed compatibility addition, contingent on historical dialect reference |
| Program resume | CONT after STOP | Not found | Proposed addition; define continuation semantics |
| Random seeding | RANDOMIZE | Not found | RND exists; decide seed behavior |
| Array base | OPTION BASE | Not found | Define before implementing to avoid breaking existing programs |
| Matrix operations | MAT | Not found | Defer until historical dialect and teaching priority established |
| Error handling | ON ERROR, ERR, ERL | Not found | Defer; confirm historical availability |
| Program chaining | CHAIN | Not found | Defer |
| REPL | Cursor editing, HISTORY [n|CLEAR], !n | Extension | `basic_repl.cpp`, `main.cpp`; test on actual macOS terminal and piped input |
| Graphics | SCREEN, CLS, PSET, LINE, CIRCLE, PAINT | Not found | Future MSX-inspired extension; not RT-11 baseline |

## Current regression evidence
The archive's `Tests/regression/README.md` describes nine enabled checks and disabled legacy cases. The later CMake configuration enumerates 13 tests, including `star`, `regression`, and `eof`. Previous local Linux execution reported 10/13 passed and these three failed. Treat that as **prior local evidence**, not a fresh macOS result. The regression README explicitly warns that smoke checks do not verify exact numerical output.

## Prioritized audit actions
1. Establish the exact historical reference: DEC BASIC-11 under RT-11, manual edition and syntax, rather than mixing BASIC-PLUS or later Microsoft BASIC features.
2. Add deterministic tests for RESTORE and RESTORE line, including DATA across multiple program lines.
3. Reproduce and classify `INPUT` prompt and `OPEN` dialect differences using archived BASIC programs.
4. Add asserted tests for ON GOTO/GOSUB, nested FOR/NEXT, array bounds, expressions and string functions.
5. Verify noninteractive REPL behavior after the history patch; interactive arrow keys require a real terminal.
6. Produce a historical-reference crosswalk with citations before labeling any item 'RT-11 conformant'.

## Project policy
Preserve existing CLinter extensions and keep compatibility additions additive. CMake is the build authority; do not modify or package `.vcxproj` / `.sln` files. New source changes should be isolated, tested, and delivered as repository-relative ZIP patches.

RegTest_2.bas — INPUT and string handling.



RegTest_3.bas — FOR/NEXT, GOSUB/RETURN, computed branching.



RegTest_4.bas — file I/O.



RegTest_5.bas — arrays and mathematical functions.
