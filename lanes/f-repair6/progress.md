# f-repair6 running notes

2026-10-04. Read the lane rules, review F1-F10, fault builders, f-repair5 result, current APIs,
headers, power/root tests, fixture generators and the two integer oracles. Source files are read-only.

Plan: record baseline CPU time; add integer-certified fixtures and exhaustive large-list centre checks;
run F6/F7/F8 faults before the unchanged library; repair the sentences; rerun all 16 faults;
run normal, sanitizer and one final check-all. Every build and runner is bounded by timeout.

F9 is being investigated with the largest admissible word-prime branch count. A timing assertion must
have a wide margin and a listing cost at least 100 times its guard, or R6 will state the unasserted cost property.

Baseline: test_lpow 3066274 checks, CPU 6.41 s; test_lroot 1323173 checks, CPU 1.11 s;
test_rfunc_prime 522217 checks, CPU 0.09 s. Every failed-check count was 0.

Tests added: 30 powunit rows (18 exhaustive small-prime sets, 12 large-prime witness pairs),
144 independently lifted powrat centres, 12 seed-42 root rows, all-centre checks on all six large lists,
and independent wrapper values. Before sentence edits: F6 failed 8/0/4 checks, F7 288/0/144,
F8 72/6/36 (lpow/lroot/rfunc). The unchanged library failed 0/0/0.
The first generator attempt found a tie in its second A+beta case; B=3 was corrected to B=4.
No C library call failed against a new expected value.

F9: exact x=1 at p=2^64-59, n=d=824329, N=2^60+1 is refused in 0.000279 s CPU.
Valid listing at N=40 cost 4.131082 s CPU, 206.55 times the 0.02 s guard.
Always-OK early_status took 0.217694 s for the refused call. It also now fails older value tests,
because R9 relies on early_status to initialize z0 before starting the chain.
A second fault delays early_status until after identifiers while preserving that initialization.
This isolates the cost property; its red and green checks are being recorded.

R9 moved the list write out of branch() for most seeds. The review's seed-42 fault is still caught
by the seeded fixtures. A supplemental fault shifts the centre at the fused write site instead:
10 failed lroot checks, including branch 41 of the 65536-branch list. The whole-list check bites.

Sentence edits: P1's roots and exact injectivity criterion, its negative-power nonzero witness;
header's P3/P6 bounds; P7 H=1 at 2; P8 LONG_MIN endpoint (also P9); R8 R=1 convention and full digit cost;
R6 and lroot.h state the F9 cost guard. Each was re-read against current source.
Own integer proof checks: 20800 cyclic-group, 6048 zero-ball, 4096 H=1 and 1443 R=1 comparisons, 0 failures.

Final 16-fault table: 16 killed, 48 executions, 0 timeouts. Two rfunc runs ended with signal 11
after 13 and 11 failed checks (root_K_N and seed_mod_p_minus1); every other run reached its summary.
Pure early-status deferral: 0/1/0 failed checks, with the one failure solely the 0.02 s guard.
Final normal counts: 3074149 / 2361423 / 526806 checks, 0 failures. CPU: 5.58 / 1.54 / 0.10 s,
total 7.22 s against baseline 7.61 s (single runs; frequency/load variation is not a speedup claim).
All six large-list centre verifications together took 0.409 s CPU, maximum 0.222 s for one list.

SAN=1 build completed. Each test ran with ASAN_OPTIONS=detect_leaks=1 and reached its summary
with 0 failed checks, then exited 1: LeakSanitizer reports its fatal ptrace restriction.
This is a sandbox limitation, not a returned-value mismatch. Leak checking is not verified.
The same binaries are being run with detect_leaks=0 to verify address and undefined-behaviour checks.

check-all ran once, exit 0: 75 C programs with 0 failed checks; driver 51 cases / 100982 expected lines;
exports 432/432; Julia 48/48; mutate and memcheck self-tests passed. Its last line is
`check-all passed: make check, driver, exports, julia, mutate-selftest, memcheck-selftest`.
ASAN/UBSAN with detect_leaks=0: all three programs reached the same 0-failure summaries, exit 0.

Supplemental Valgrind 3.22.0 check: raw test_lpow timed out at 180 s (exit 124; no test summary).
Its shutdown log is not a completed leak check. Raw test_lroot exits 99, with 0 definite/indirect loss
and possible loss in FLINT allocations. Root has 2 cost-guard failures under instrumentation:
0.722 s against R9's 0.45 s, and 9.454 s against the new centre verifier's 5 s.
No assertion is relaxed. refs/src/flint-3.0.1/memory.rst:29-44 documents FLINT's global caches
and the cleanup before exit. A lane-only wrapper now performs flint_cleanup_master after the full tests,
with their assertions and guards unchanged, to distinguish retained caches from leaks.

After cache cleanup: full root and rfunc programs have 0 bytes in use at exit and 0 Valgrind errors.
Root exits 1 only for the same two instrumented timing guards (1.308 s / 13.653 s).
Rfunc exits 0, 526806 checks, 0 failures. Full lpow again exceeds its bound (170 s, exit 124).
Its 2 new tests were then checked separately, unchanged: 7875 checks, 0 failures, 7807 allocs/frees,
0 bytes in use at exit, 0 Valgrind errors, exit 0. LeakSanitizer itself remains unavailable under ptrace.

Additional prose finding at final inspection: R9's existing test comment and timing sentence said
x=3^n+p^30 Z_p, but the code sets its centre to w=3^n mod p. Both owned sentences now name the actual ball.
For p=2^64-59, n=24068, w=5016686989854155913, while 3^n mod p^2 is
101245085488030368830923462363895328769. Difference/p mod p=5488507081980157208 is nonzero.
Thus 3^n is outside the actual ball already modulo p^2. f-repair5/result.md repeats the incorrect input
description; it is read-only here. No executable statement changed after the check-all run.
