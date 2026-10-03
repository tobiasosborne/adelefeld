# f-slice10 progress (running notes; the report is result.md)

- 22:55 started; worktree at 86d9be4; refs/src symlinked. Read brief, draft, COMMON, COMMON-C, SPEC 9.3, 15.4,
  functions.md P12, P14, P16, P22, conventions 3.1-3.3, PLAN 1F.8, the headers.
- Slice A header written: include/adelefeld/gfunc.h (adf_rat_root, adf_adele_root, adf_idele_root), all with
  `where` (conventions 2.2 order: outputs, reports, inputs, prec).
- 23:01-23:06 slice A tests red (link, then stub: 130053 failed checks in 7 tests) and green (7 tests, 1004258
  checks, 0 failed, 0.27 s). Oracle proto/gfunc_checks.py; vectors tests/ref/vectors/f-slice10/rat_root.jsonl
  (5814 rows) and real_root.jsonl (558 rows), 725843 bytes.
- Finding (rfunc, not gfunc): adf_real_root of 2^1000 +- 2^960 at prec 2, n = 3, is [4.48e102 +- 8.15e102], a ball
  that contains 0 although the input is positive; the tight bound of test_rfunc fails at prec 2 for 2^+-1000.
  Consequence: adf_idele_root of (2^999 +- 2^960, 2^999, [1]) at prec 2, n = 3, is NOT_DETERMINED (tested).
- 23:09 driver `root X with N [with SIGN]` (arity 2 or 3; new arity code in the table), golden
  tests/driver/gfunc-root.cmd/.out (43 lines, red on the HEAD driver: 43 x PARSE; green). README section
  "Roots at all places (1F.8)" and a table row.
- 23:10 Julia tests/julia/gfunc.jl (7 of 7; red without gfunc.c); added to tests/test_julia.sh (two lines).
- docs/api-1f8.md: decisions 1-7, G1 to G4 with proofs.
- 23:15 planted faults A1-A5 all detected (lanes/f-slice10/faults-A.log): A1 1117 failed checks, A2 18909, A3 abort
  (FLINT exception, negative degree), A4 11334 + 1906 failed checks then abort, A5 2 failed checks.
- SLICE A FINISHED (library, tests, driver, README, Julia, statements, faults). Mutation testing is run once at the
  end over the whole src/gfunc.c (60 mutants), after slice B.
- 23:17 slice B header (five declarations, one comment block) in gfunc.h; oracle extended (series_real.jsonl 327
  rows, series_place.jsonl 988 rows; all four vector files 933874 bytes).
- 23:22-23:24 slice B red (link; stubs: 11347 failed checks in 3 tests) and green (10 tests, 1088693 checks).
- 23:27 driver commands exp, sin, sinh, cos, cosh (arity 1; rational or adele); golden gfunc-series.cmd/.out (23
  lines; red on HEAD: 23 x PARSE). README section "The series at all places (1F.8)".
- 23:29 Julia slice B testset 4 of 4 (red against the slice A library). api-1f8.md decisions 8-11, G5, G6, Scope.
- 23:29 planted faults B1-B5 all detected (lanes/f-slice10/faults-B.log): 77, 10599, 2205, 615, 359 failed checks.
- 23:30 mutation testing of src/gfunc.c started (60 mutants, seed 1, 2 jobs, SAN=1 by --san, prebuilt
  lanes/f-slice10/build/san copied; log lanes/f-slice10/mutate.log).
- 23:31 mutation done: 60 mutants in 106.9 s: 48 killed, 3 survived, 9 not compiled (so 51 compiled), 0 timed out.
  Survivors: :54 `s < 0` -> `s < 1` (s is +-1 there); :101 `>` -> `>=` in the maximum (ties give the same value);
  :268 n_nextprime(p, 1) -> (p, 0) (proved vs probable prime below 2^64; no test can tell).
- 23:32 final check-all started in build/.
- 23:38 check-all passed (exit 0); sanitizer run of test_gfunc clean; vectors reproduced byte for byte. result.md
  written.
