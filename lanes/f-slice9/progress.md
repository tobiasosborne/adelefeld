# f-slice9 progress (running notes; result.md is written at the end)

- Read: brief, CLAUDE.md, COMMON.md, COMMON-C.md, workflow.md, SPEC 9.3.1-9.3.4 and 15.4, functions.md
  Lemma 9, Props 11, 13, 15, 17, 18, api-1f4 (F1-F7), api-1f5 (R1-R8), lball.h, lfunc.h, lroot.h, rfunc.h,
  src/lroot.c, pow_si in src/lball.c, rfunc.c root_seed_at, adf.c root_at, README, review-lroot, oracle.py.
- Design (before code):
  - adf_lball_powrat(y, x, e, n, seed, N): reduce e/n; n'=1 -> pow_si exactly (seed, N ignored, as degree 1 of
    lroot.h ignores them); n'>=2 -> root_seed (identifier of the n'-th root of x) then pow_si, at a root precision
    chosen so that the result is the ball at K=min(N,E'), E'=e'j+(M-m)-v(n')+v(e').
  - adf_lball_powunit(y, u, s, N): Prop 18 R=min(A+beta,B+alpha,A+B), K=min(N,R); at 2 the sign w^(s mod 2);
    sign or parity not fixed (A=1 or B=0 with w=-1): the hull 1+2Z_2 (proved smallest), or the exact image when
    the parity is even / w=1.
- 22:45 header lpow.h, oracle proto/lpow_checks.py (vectors 909 KB), tests/test_lpow.c (9 tests), src/lpow.c: green.
  Next: _at forms (rfunc.c), driver, Julia, api-1f6.md, faults, mutation, final checks.
- 23:05 _at forms (red 70 failed checks with stubs, then green), driver commands (red: all PARSE; green: 45 lines),
  README section, Julia example (7/7), test_julia.sh lines. All 35 driver cases pass on the lane build.
  Next: docs/api-1f6.md, planted faults, mutation, sanitizer, final checks.
- 23:20 docs/api-1f6.md written (decisions 1-11, P1-P8).
- faults: F1 15780+ failed checks (timeout 300 s, killed), F2 117, F3 1104, F4 609, F5 30, F6 13552 (test_lpow); test_rfunc_prime 0 for all
- 23:00 mutation (src/lpow.c, 60 of N mutants, seed 1, 2 jobs, --san, timeout 1300): run 1 all 60 NOT COMPILED
  (tool flaw: --san does not set SAN=1 when the make command contains the string "SAN", here in ASAN_OPTIONS; the
  tool then reported "passed"); run 2 with SAN=1 in the command: 40 killed, 14 survived (all equivalent, listed in
  result.md), 6 not compiled, 472.6 s.
- 23:01 final check-all started in build/.
- 23:10 final checks done; result.md written.
