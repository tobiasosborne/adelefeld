# f-repair3 result: the two minors of review f-review5 (sin, cos, sinh, cosh at a prime)

## The two repairs

R1 (text). `docs/api-1f4.md` F13 proof step 1: the sentence "LONG_MIN ... returns LIMIT unless an exact-zero
result ignores it" is replaced by two sentences: LONG_MIN is allowed as an argument; in the order of F13 it
returns LIMIT only for an input that passes the input limits and the domain test and is not exact zero; at
p=2 exact 2 gives DOMAIN and 2 Z_2 gives NOT_DETERMINED. Nothing else in F13 changed.

R2 (code). `src/lfunc.c` `parity_centre` now steps by two over the retained parity:
`for (k = L; k >= 2; k -= 2) { F = k(k-1)F mod p^W (fmpz_mul2_uiui); A = x^2 A + epsilon_(k-2) F mod p^W }`,
then for odd L one final `A = x A mod p^W`. x^2 mod p^W is formed once. `parity_coefficient` became dead in its
parity test (every degree reached has the retained parity); it is replaced by `parity_sign(j, alternating)`,
which returns only the sign. Count, working precision, domain checks, exponent rule, statuses, final
division: unchanged (no line outside the loop, the sign function and the declarations of x2 changed).
The proof is a new statement F15 at the end of `docs/api-1f4.md` (5 steps, own words; it would have exceeded
ten comment lines); F11 got one sentence pointing to F15; the code comment at the loop cites F15 and
`refs/src/flint-3.0.1/fmpz.rst:758-760` (fmpz_mul2_uiui) and 880-883 (fmpz_mod, nonnegative remainder).
After the orchestrator's message refs/src is on disk; both FLINT lines were read there.

Files changed: `src/lfunc.c` (parity evaluator only), `docs/api-1f4.md` (F11 one sentence, F13 step 1, new F15),
`tests/test_lfunc_trig.c` (one new test). Lane files: `lanes/f-repair3/` (ref_lfunc.c, compare.c, gen_pins.c,
pins.txt, check_pins.py, loop_mutants.py, logs, progress.md, redgreen.log, this file).

## Checks run (all from the worktree root; numbers)

1. Reference comparison, `lanes/f-repair3/compare.c` against `lanes/f-repair3/ref_lfunc.c` (= src/lfunc.c
   before the change, sha256 5951d2231a03...6edc0, its 7 external names renamed ref_lball_*; exp/log/Log had
   to be renamed too, or the archive's lfunc.o would define them twice).
   Inputs: 3300 random (p = 2, 3, 5, 7, 13, 65537, 2^64-59: 450, 900, 450, 450, 450, 300, 300 draws;
   1317 exact nonzero, 971 noncentred balls, 690 balls about 0, 322 exact zero; centres up to 3000-bit
   numerator and denominator; v from c-2 to c+5; N from -2 up to 2000 at p=3 and 300 elsewhere) plus 14 fixed
   edge inputs (N = LONG_MIN, LONG_MAX, the W-only LIMIT). 13256 distinct calls, 12084 aliased calls.
   Failure condition: a different status, or a result (or untouched output) not adf_lball_identical.
   Before the change: `timeout 600 lanes/f-repair3/build/compare` -> differences 0 (trivially).
   Detection: with a planted fault in the reference (old loop `k >= 2`): differences 11714.
   After the change: differences 0, exit 0. Sums actually formed (the parity_centre path, replicated in the
   program from parity_apply and F10's count): sin/sinh 1687 each, cos/cosh 1397 each; term count C even/odd
   812/875 for sin/sinh and 588/809 for cos/cosh, so both branches of `if (L % 2 != odd) L--` occur for each
   function; top degree L odd (sin/sinh, L=1 417 times, no pair) and even (cos/cosh, L=2 250 times); max L
   3993/3994; sums per prime 808, 1682, 864, 884, 930, 542, 458. Statuses per function: OK 2539, DOMAIN 652,
   NOT_DETERMINED 114, LIMIT 9.
   The same comparison with library and reference built with SAN (address, undefined): differences 0,
   0 sanitizer lines.
2. Pinned residues, new test `pinned_residues_odd_and_even_top_degree`: 20 residues (5 cases x 4 functions:
   p=3 x=3/2 N=20 C=39; p=3 x=9/2 N=21 C=14; p=5 x=35/3 N=12 C=16; p=2 x=12/5 N=30 C=29; p=2^64-59 x=3p N=4
   C=5; L = 37,13,15,27,3 for sin/sinh and 38,12,14,28,4 for cos/cosh), printed by gen_pins.c from the code
   before the change. `timeout 120 python3 lanes/f-repair3/check_pins.py`: 20 of 20 equal to the Fraction
   oracle `proto/lfunc_trig_checks.py` point(). Green before the change; red against an archive whose
   lfunc.o had the planted loop fault: 20 of 20 pinned checks failed (whole program 8584 failed checks).
3. `timeout 300 make -j2 BUILD=lanes/f-repair3/build .../test_lfunc_trig .../test_lfunc .../test_rfunc_prime`:
   exit 0. test_lfunc_trig 7 tests, 2174090 checks, 0 failed; test_lfunc 13 tests, 473031 checks, 0 failed;
   test_rfunc_prime 14 tests, 521939 checks, 0 failed (rerun at the end: same numbers).
4. `timeout 300 python3 proto/lfunc_trig_checks.py` (check mode, no `--generate`): 3 independent truncations,
   462 identity checks, 0 failures. No fixture regenerated.
5. `make -j2 SAN=1 BUILD=lanes/f-repair3/build-san .../test_lfunc_trig` and its run: 7 tests, 2174090 checks,
   0 failed; 0 sanitizer lines.
6. Timing, review's case p=3, N=2000, x=3/2 exact, one run, one call each, CPU ms (a measurement, not a bound),
   `timeout 120 lanes/f-repair3/build/compare --time`: before (reference) / after (library):
   sin 51.659 / 25.031, cos 49.930 / 24.586, sinh 49.469 / 24.809, cosh 49.730 / 26.308; all 4 identical, N=2000.
7. Mutation testing, `ASAN_OPTIONS=hard_rss_limit_mb=3000 timeout 1300 python3 tools/mutate/mutate.py
   --files src/lfunc.c --limit 60 --seed 1 --jobs 2 --san --timeout 150 --scratch <scratchpad>/mutate
   --make "make -s -j1 INV=1 build/test_lfunc_trig build/test_lfunc && ./build/test_lfunc_trig &&
   ./build/test_lfunc"`: 545 mutants, 60 run in 1193.3 s: 41 killed, 14 survived, 4 not compiled,
   1 timed out (392 `%` -> `/` in log_sum: an endless loop, i.e. detected). test_rfunc_prime was left
   out of the per-mutant command to stay inside 20 minutes (one SAN build of the library is about 37 s).
   A first start with `ulimit -v` was stopped at once: ASan's virtual reservation exceeds any
   such limit, so ASan's RSS limit was used instead.
8. Because only 1 of those 60 (line 739) fell on the changed loop, every tool mutant of lines 690-739
   (parity_sign and parity_centre through the odd step) was run by `lanes/f-repair3/loop_mutants.py` (no sanitizer;
   test_lfunc_trig, then the comparison program): 51 mutants, 37 killed by the test, 1 killed by the
   comparison only (a timeout), 13 survived.

## Mutation survivors (one line each)

Tool run (14):
- 739 drop `fmpz_mod(A, A, P)` after `A = x A`: equivalent; A x and its residue differ by a multiple of p^W,
  absorbed by the exact division by p^D and the reduction modulo p^K (W - D = K).
- 722 `fmpz_mul(x, x, tmp)` -> `(x, tmp, x)`: equivalent, a commutative product (line unchanged by this lane).
- 270, 358, 437 the same argument swaps of a commutative mul/add in exp_centre, log_sum_word, log_split.
- 130 `fmpz_init_set_ui(a, p - 1)` -> `p - 0` in count_exp: more terms, same residues; it moves only the
  W-only LIMIT boundary, which a test reaches only with a modulus of about 2^26 bits (not affordable).
- 137 `fmpz_cmp_si(a, 1) < 0` -> `(a, 0)`: equivalent; callers guarantee K > w >= c, so the count is >= 1.
- 110 `hi < 1` -> `hi <= 1` in count_log: equivalent (sets hi = 1 when it is 1).
- 570 `Tc > 0 ? Tc : 1` -> `Tc > 1 ? ...`: equivalent (Tc = 1 gives 1 either way).
- 643 `N < E ? N : E` -> `N <= E`: equivalent (equal when N = E).
- 382, 594 drop `fmpz_zero(S)`: equivalent in the present callers (S is freshly initialised to 0).
- 388 drop `fmpz_divexact(Q, Q, tmp)` in log_sum: the shrinking modulus of F8 stays larger; same residues,
  cost only.
- 510 `return a - b` -> `a + b` in val_fmpq: equivalent at its one caller (z = a - 1, a p-integral, so b = 0).
Targeted loop run (13): 706 `count_exp(...) - 1` -> `+ 1` and -> `- 0` (more terms, same residues, as 130);
722, 732, 733 (add), 739 commutative argument swaps; 723 `fmpz_mul(x2, x, x)` -> identical text (a no-op
mutant, see below); 733 `k - 2` -> `k + 2` (floor((k+2)/2) and floor((k-2)/2) differ by 2: same sign);
733 `> 0` -> `>= 0` (parity_sign is never 0); 722, 723, 731, 739 drop of `fmpz_mod` on x, x2, F, A (all later
operations reduce modulo p^W, and F, A are divided exactly by p^D: same residues).
Killed by the comparison only: 734 drop `fmpz_mod(A, A, P)` in the loop (output-equivalent by the same
argument; unbounded growth of A makes N = 2000 time out). None of the survivors is a gap of the tests that an
affordable test could close, so no further test was added; no entry was written to equivalent.txt.

## What is not done

- No fuzzing (none asked; the comparison program is a differential run of about 5 s, a smoke test in the
  sense of docs/workflow.md rule 5, with 0 differences).
- test_rfunc_prime was not part of the per-mutant command of the tool run; it was run on the final code.
- `make check` over the whole tree and `make check INV=1` were not run (the INV=1 build was exercised only
  inside the mutation run, whose baseline passed in 40.4 s).
- The timing is a single run on a shared machine.

## Sources pending

None. Cited: docs/proofs/functions.md (Definition 1, Lemma 5, Propositions 6-10, as before),
refs/src/flint-3.0.1/fmpz.rst:752-760 (fmpz_mul, fmpz_mul_ui, fmpz_mul2_uiui) and 880-883 (fmpz_mod), read on
disk after refs/src was linked into the worktree.

## Findings against the specification or the review

- None against docs/SPEC.md. The review's two findings were confirmed: R1 by reading parity_apply
  (order: input limits, domain, exact zero, |K|; LONG_MIN at exact 2 or 2 Z_2 was not rerun by this lane);
  R2: the paired loop gives identical residues on 25340 calls and halves the
  time of the review's case (about 50 ms -> 25 ms here; the review measured 76 -> 36 ms).
- Mutation tool (not repaired, reported): `swap_args` generates a mutant whose two exchanged arguments are
  the same expression (`fmpz_mul(x2, x, x)` -> `fmpz_mul(x2, x, x)`); it always survives and costs a run.
- Avoidable cost noted (lanes/COMMON-C.md rule 7): count_exp's max(1, ...) branch is unreachable from both
  callers (mutant 137).
