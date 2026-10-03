# f-repair5 progress (running notes)

- 2026-10-03 22:47: worktree at 8f57a51, brief present; refs/src symlinked. Read COMMON, COMMON-C, workflow,
  f-review7 notes, lpow.h/.c, lroot.h/.c, api-1f5/1f6, lball.h (teichmuller, unit_mod, pow_si).
- 23:00 RED: see redgreen.log (test_lpow 21 failed checks = 7 LIMIT cases; test_lroot guard 2.307 s CPU vs 0.5).
- 23:02 item 2 coded (src/lpow.c integer_exponent, via_pow_si): test_lpow green (10 tests, 3066274 checks).
  Differential old vs new powunit (diff_powunit.py, harness f-review7/h.c): seed 3, 10000 cases: 9876 identical,
  124 LIMIT -> OK (each = the coarse ball of pow_si), 0 failures. (seed 2, 3000 cases: 2964 / 36 / 0.)
- 23:10 items 3, 4 coded in src/lroot.c (early_status rule general>one && !power_ok(L); all_branches with the
  chain y = c0 omega(t0 zeta^i) mod p^(K-j)). Versions tried: lball_mul chain (0.16 s at d=24068), fmpz chain +
  branch() (0.17 s), fused centre (0.07 s), single fused chain (0.04-0.06 s CPU). Guard set to 0.45 s CPU.
  Old code against the same test: 2.307 s (unloaded), 3.9 / 5.3 / 7.8 s (loaded, load average 7-9).
- 23:35 docs written (lpow.h, lroot.h comment blocks; api-1f6 decision 12, P8 step 2, P9; api-1f5 decision 11,
  R6 steps 4 and 6, R9; f-repair4 erratum). Green: test_lpow 3066274 checks, test_lroot 1323173, test_rfunc_prime
  522217; the same under SAN=1 + detect_leaks=1 in build-san: 0 failures, no sanitizer report.
- diff_roots.py 6000 2 (old vs new, lists and seeded calls): 7821 lines, 0 differences, 186107 branches.
- Attacks (copies in lanes/f-repair5/attacks, harness h.c built against this lane's archive): B1 exact seed 1:
  4758 cases, 14344 branches, 0 failures; B1 ball seed 1: 4758 / 14582 / 0; B1 exact seed 2: 4758 / 14154 / 0;
  B2 seed 1: 68 lists (p = 65537, 2^64-59, d up to 824329, N = 3), 0 failures; B3 1 3000: 8594 branches, 0
  differences; B4: 1200 lists, 2590 seeded calls, 0 differences; A2 4000 x seeds 1..4: 0 failures.
- tbranch CHECK=all (every branch against root_seed, p = 2^64-59): 78232 branches, 0 differences.
- 23:20-23:55 mutation (mutate_lines.py wrapper, changed lines only, --seed 1 --jobs 2 --san, root = scratch copy
  with a prebuilt SAN build): lpow.c 33 mutants, bound 700 s hit (2 runs in progress killed); printed: 7 NOT
  COMPILED (-Werror), 4 SURVIVED (all equivalent). lroot.c 40 of 106 mutants, bound 590 s hit; printed: 3 NOT
  COMPILED, 1 TIMED OUT, 3 SURVIVED (equivalent or unreachable). The tool prints killed mutants only in its
  summary, which the bound cut off.
- 23:56 check-all started in build/.
