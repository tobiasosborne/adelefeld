# f-repair4 progress (running notes)

- 21:18 read brief, CLAUDE.md, COMMON, COMMON-C, workflow, N-D13, N-D14, review, lroot.h, lroot.c, api-1f5.md,
  F6 of api-1f4.md and its use in lfunc.c (lines 636-643, 769), oracle.py, attack1.py, h.c, the tests.
- 21:18 baseline build in lanes/f-repair4/build-before (exit 0); timings before in timings-before.txt:
  polyroots 65537/65536: 5.656 s (generator 0.57 ms); 2^64-59/6028: 5.700 s (generator 0.44 ms);
  perf: roots 27.2 ms, one branch 1.92 ms; perf irr: 26.3 ms, 2.10 ms; h `A 65537 ... 65536 N=2^40`: LIMIT after
  6.02 s; nd13 100000: root_seed at N=10 50.3 s, exponent 99999.
- Observation: src/lroot.c:263 already decides LIMIT for capacity < d and d > ADF_LROOT_BRANCH_MAX BEFORE
  identifiers(); the 5.9 s LIMIT of the review is the precision LIMIT (N = 2^40, Teichmueller power) found
  after the enumeration. The brief's F2(b) wording ("after the enumeration") is true of that LIMIT, not of the
  capacity LIMIT. The capacity test of brief step 1(c) is therefore green on the old code.
- 21:27 tests written (test_lroot.c, test_rfunc_prime.c, julia/lroot.jl, driver root-values) and run RED:
  see redgreen.log.
- 21:30 src/lroot.c rewritten (shared_t: K=min(N,E), z0 once, rational branches once; principal() without the
  power pre-check; early_status(); dth_root() and identifiers() by a primitive root). GREEN at 21:30-21:32.
- 21:31-21:33 timings after: timings-roots.txt, timings-after.txt. N=2^40 LIMIT 6.02 s -> 0.017 s (h, wall incl.
  process start); N=0 listing 65537/65536 5.81 s -> 0.014 s; 2^64-59/6028 5.98 s -> 0.0017 s; d=299756: 0.084 s at
  N=0, 44.6 s at N=20 (Teichmueller lift per branch, 149 us each: an avoidable cost, see result.md);
  perf 27.2 -> 5.4 ms, perf irr 26.3 -> 5.1 ms; nd13 100000 at N=10: 50.3 s -> < 1 ms (exponent 10).
- 21:33-21:36 documents: lroot.h comment block (exponent rule, LIMIT sentences, macro comment, R1-R8), api-1f5.md
  (decisions 3, 5, 6; R2 statement and step 6; R4 statement, steps 4-5; R5 step 4; R6 steps 2, 3, 6; R7 steps
  2-3 (F4); new R8), rfunc.h (three comments stating exponent E), tools/adf/README.md (the "independent of prec"
  sentence; "coefficient-count" -> "branch-count").
- 21:36 sanitizer build (SAN=1, build-san): test_lroot 0 failed of 1308396; test_rfunc_prime 0 of 521989; with
  leak detection also 0 leaks reported.
- 21:37-21:39 reviewer's attacks with lanes/f-repair4/oracle14.py (N-D14 check at Nreq < E, reviewer's check at
  Nreq >= E): attack1 seeds 1-12: 12 x 27316 cases, 0 failures; attack1s seed 21 on an ASan/UBSan h: 27316, 0;
  attack2: 1092, 0; attack3: 1131, 0. With the reviewer's N-D13 oracle unchanged: attack1 seeds 1-6 7033 failures,
  attack2 64, attack3 72, every one an OK ball result with Nreq < E and exponent Nreq (classify13.py).
- 21:41 mutation run restarted (first start stopped after 2 mutants because a header edit forced full rebuilds).
- 21:41-21:46 mutation, src/lroot.c, 60 of 448 mutants, seed 1, 2 jobs, --san: 46 killed, 9 survived, 5 not
  compiled, 279 s. One survivor (line 361, n==1 -> n==0) was a gap: test added, red on the mutant, green.
- 21:48 differential old/new h on attack1 seeds 1-4, attack2, attack3 (111487 cases): every differing line is a
  ball with Nreq < E (4720 + 64 + 72); all others byte-identical.
- 21:50-21:55 make check, test_driver.sh, test_julia.sh: all pass.
