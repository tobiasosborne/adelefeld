# f-review6 progress (running notes)

- Read brief, COMMON, lroot.h, lroot.c, api-1f5.md, functions.md Prop 13/15/15r/16, lfunc.h, lball.h parts.
- Building archive: make -j2 BUILD=lanes/f-review6/build lanes/f-review6/build/libadelefeld.a
- Harness lanes/f-review6/h.c (raw lball fields per line; sentinel outputs to detect writes on failure).
- Oracle lanes/f-review6/oracle.py: digit lifting (O2), a-posteriori exact-image check (O3), integer roots.
- attack1.py seeds 1,2,3: p=2,3,5,7,13; n=1..12,p,p^2,p-1,2(p-1),3(p-1); balls (rootable/random,
  j in -2..2), exact inputs; every seed 0..p+1; roots and count. 3 x 27316 cases, 0 failures.
- attack2.py (p=65537, 2^64-59; n up to 2^64-1, n=p, n=p-1, 3(p-1), 2^63): 1092 cases, 0 failures (114 s).
- limits.c (ASan/UBSan): 1+2^(2^27) Z_2, n=2 -> LIMIT although result 1+2^(2^27-1) Z_2 has u=1 (Log, pow_si
  of the same sizes return OK). Candidate MINOR finding F1. Other limit/alias/capacity cases consistent.
- perf.c: p=65537 n=16 N=200: roots 21.7 ms (exact 3^16), 33.0 ms (irrational); one branch 1.47 ms;
  Log 0.35 ms + exp 0.85 ms per branch recomputed.
- drv1.cmd, drv2.cmd: driver; no inconsistency found except '+1' seed is PARSE (README lists 1/-1 only).
- at.c (ASan/UBSan): _at forms; where/arch/len/aliasing consistent.
- attack1 seeds 4..11 (attack1-long.log) and seed 21 under ASan/UBSan (instrumented lroot.c, rfunc.c):
  9 x 27316 more cases, 0 failures. Total attack1: 12 x 27316 = 327792 cases.
- attack3.py (r up to 3000 digits, 1500-bit^n exact, word-size n): 1131 cases, 0 failures.
- oracle_selftest.py: lift() against brute force mod p^(L+s+c): 6672 cases, 0 mismatches.
- polyroots.c: FLINT Rabin root finding of T^65536-1 at 65537: 5.88 s; generator listing 0.5 ms.
  roots at p=65537, n=65536, exact 1, N=2^40: LIMIT after 5.9 s; seeded: LIMIT in 9 ms.
  p=2^64-59, d=6028: 5.86 s; d=299756: not finished in 100 s.
- nd13.c: ball 2-adic square, r=10^4: 0.20 s, r=10^5: 51.8 s at requested N=10; r=4*10^5 > 110 s.
  expcost.c: the time is exp (p=2, K=5*10^4: 3.9 s; Log 0.03 s).
- R7 step 2 "E<=M on a guarded rootable ball" false for j<0 (scaled rows); MINOR proof text.
- result.md written.
