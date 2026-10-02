# f-slice7 running notes

Read CLAUDE.md and the lane rules. No state-changing git command or tracker command is used.
Builds and scratch faults will use lanes/f-slice7/build. Every test and script runs under timeout.

Decision: cos and cosh use E = 2M - v_p(2) for a centred input ball, E = M otherwise.
The alternative is E = M everywhere. The chosen rule exposes Proposition 10's proved centred hull.
All four use K = N for exact input, min(N,E) for a ball. Only exact zero returns an exact value.
Proposition 7 already contains the four parity counts. F10 onward will restate and prove the implementation.

Ownership issue found before implementation: old tests still expect UNSUPPORTED for sin/cos at a prime.
Read-only affected paths: tests/test_rfunc.c, tests/julia/sball.jl, tests/julia/f_at.jl,
tests/driver/f-places-hostile.cmd and .out. A narrow authorization question is pending.

Implemented all four local functions by an added parity Horner evaluator. No existing evaluator was changed.
F10-F14 added to docs/api-1f4.md: count, common denominator, working precision, ball enclosure,
constant shortcuts, limits, partial-ball wrapper. F12 also proves odd-series surjectivity on domain balls.
Oracle: 4064 case rows, 503 point rows, 8 Hensel roots; 592240 bytes total.
Local suite: 6 tests, 2173970 checks, 0 failures. Enumeration: 1296 requests, 18108 enclosures,
480 hull witnesses. Existing lfunc regression: 12 tests, 454429 checks, 0 failures.
At-prime suite: 12 tests, 279015 checks, 0 failures (all new fixture rows, both output modes).
Driver: 3 new golden files, 96 lines matched. Red and green details are in redgreen.log.
Julia example written; five faults and final checks remain.

Five deliberate scratch faults are rejected by unmodified local tests. Failed checks:
short parity count 857; input exponent without cap/gain 2412; uncertain zero exact 9186;
alternating sign removed 1680; one guard digit lost 910. No mutation or fuzz run was used.
The first direct Julia attempt hit the documented bundled-GMP loader error before running tests.
A retry uses LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10, as the existing Julia harness does.

Julia example now passes 25/25 checks after correcting a Julia syntax typo (k div 2 -> div(k,2)).
The legacy-test ownership patch is prepared but NOT applied: lanes/f-slice7/legacy-tests.patch.
Scratch copies with that patch pass: C rfunc 10 tests/59700 checks, Julia sball 36/36,
Julia f_at 23/23, old hostile driver output unchanged. Four files change; the .out need not change.
The tests keep their original purpose: local-domain failures replace obsolete UNSUPPORTED assertions;
sin(5/3) additionally checks an independent 12-digit rational sum; tan_at remains an unknown command.
The ownership question remains pending. Final acceptance runs will therefore expose obsolete assertions.

Final commands, each run once:
- timeout 180 sh -c 'make clean && make -j2 check-all': exit 124 during test_resid_rest.
  47 test-program summaries completed without a failure before timeout. Last line:
  make[1]: *** [Makefile:102: check] Terminated
- Sanitizer binaries built once by timeout 180 python3 -B lanes/f-slice7/build_san.py: exit 0.
- ASAN_OPTIONS=detect_leaks=0 timeout 180 make -j2 check SAN=1 BUILD=build/san:
  exit 124, also during test_resid_rest, after 47 zero-failure summaries. Last line:
  make: *** [Makefile:102: check] Terminated
Both runs include local trig: 6 tests, 2173970 checks, 0 failures; old local functions:
12 tests, 454429 checks, 0 failures. Exports in the normal run: 413/413 functions, 0 missing.
No ASan/UBSan diagnostic occurs in the sanitizer log.
The new wrapper test lay after the timeout, so ran separately:
ASAN_OPTIONS=detect_leaks=0 timeout 60 build/san/test_rfunc_prime: exit 0,
12 tests, 279015 checks, 0 failures. No second full acceptance or sanitizer run was made.

Tracked legacy C test run: timeout 60 build/test_rfunc: exit 1, 6 failed checks of 59700.
Tracked legacy hostile driver: one differing output, the now-valid sin_at command.
No permission to change those paths has arrived; the tested four-file patch remains unapplied.
The last header change only corrected the stated cost to include both unit inverses; no C token changed.
Implementation and the lane's numerical checks are complete. Full integration remains incomplete:
two 180-second acceptance timeouts, and the legacy-test ownership patch awaits the orchestrator.
