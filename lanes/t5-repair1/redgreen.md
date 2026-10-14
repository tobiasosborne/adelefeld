# Red and green runs of lane t5-repair1

The tests are written against code that exists, so each new test was green on the merged code at its first run;
its red is the replanted mutant (`lanes/t5-repair1/replant.py`, the exact text change of tools/mutate, the mutated
object linked before the plain archive of `lanes/t5-repair1/build`).

1. `test_tate_continue jbound` (20 records of `tests/ref/vectors/t5-repair1/jbound.jsonl`): green, 2141 checks.
   Red: m247 (src/tate.c:247 `arb_mul(u, bb, R0, TT_TP);` removed) exit 1 at test_tate_continue.c:693 (N, R);
   m551 (src/tate.c:623 `arb_get_ubound_arf(r2, ...)` removed) exit 1 at :707 (the radius bound).
   Before the new section: m247 passes the sections `cutoffs` and `values` (exit 0); m551 is already killed by
   `cutoffs` (:407) and `values` (:248) of slice 5d, it survived only the 5c program test_tate.
2. `test_tate work_count`: green, 22909 checks. Red: m161 (src/tate.c:161 `tt_charge(w, 1)` -> `tt_charge(w, 0)`)
   exit 1 at test_tate.c:675 (`work == 43`); the same mutant also fails `test_tate_continue jbound` at :699.
3. `test_localfactor real_derivative_bound_boxes` (8 boxes of `gamma_bound.jsonl`): green, 14 tests, 309581 checks.
   Red, test_localfactor before and after the new test (exit codes), see replant-lf.log and result.md.
   Final replant of every mutant: replant-final.log (4 of 4 in src/tate.c and 12 of 16 in src/localfactor.c fail).
4. test_tate_gamma under valgrind: red (exit 9, 18 errors from 6 contexts, the test's own byte comparison);
   after re-initialising s at the end of the loop body: green (exit 0, 0 errors), plain/SAN/INV/clang green.
5. tests/julia/tate_gamma.jl: a scratch copy with -0.7 for -0.75 fails (exit 1, 35 pass, 1 fail at its line 56,
   jl/red.log); the original passes 36 of 36 (jl/green.log); the scratch copy is deleted.
