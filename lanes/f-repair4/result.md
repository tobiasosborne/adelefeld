# f-repair4: N-D14 and the findings F1, F2, F4 of review f-review6 (local roots, 1F.5)

All four items are done, red then green. Every check passes: `make check` (73 programs, 58092354 checks, 0
failed), the driver and Julia scripts, the sanitizer build, and the reviewer's attacks against an N-D14 copy of the
reviewer's own oracle. Wherever N-D14 gives the same K as before, results are byte-identical to the old library:
111487 harness cases were run through both builds, and every differing line is a ball with requested N < E.
Mutation run: 60 mutants, 46 killed, 9 survived (one was a gap in the tests and now has a test), 5 did not compile.

## What changed (files)

- `src/lroot.c` (rewritten around a `shared_t` per call). In it:
  - `principal()` computes z0 = exp(Log(a/p^m)/n) at most once per call.
  - `rational_branches()` finds the rational roots once.
  - `early_status()` decides LIMIT before the listing.
  - `dth_root()` and `identifiers()` list the branches from a primitive root.
  - `nmod_poly` is no longer used.
- `include/adelefeld/lroot.h`: the comment block only. That covers the exponent rule, the LIMIT sentences, the
  comment of `ADF_LROOT_BRANCH_MAX` and R1-R8. The declarations and the macro value are unchanged.
- `docs/api-1f5.md`: decisions 3, 5 and 6; R2 (statement, new step 6); R4 (statement, steps 4 and 5); R5 step 4;
  R6 steps 2, 3 and 6; R7 steps 2 and 3 (F4); new R8.
- `include/adelefeld/rfunc.h`: three comments said "exponent E" and now say `min(N,E)`. No code changed.
  `src/rfunc.c` is unchanged: none of its comments states the rule.
- `tools/adf/README.md`: the sentence "independent of `prec`" is replaced by the `min(prec, E)` rule with an example.
  "coefficient-count" becomes "branch-count".
- `tests/test_lroot.c`, `tests/test_rfunc_prime.c` (root parts), `tests/julia/lroot.jl`.
- `tests/driver/root-values.cmd` and `.out`: 8 command lines and 4 expected lines are appended. No existing golden
  line changed. Reason: brief step 1(d), the driver inherits N-D14.
- Lane files: `progress.md`, `redgreen.log`, `oracle14.py`, the attack copies and the classifier, `timing.c`, and
  the logs. `build/`, `build-before/` and `build-san/` under the lane are build output and are git-ignored.

## The repairs

1. **N-D14.** For a ball input, K = min(N, E) with E = j + (M-m) - s (`shared_init`).
   - N >= E gives the exact image, exactly as before.
   - N < E gives the ball at N whose centre is b mod p^(N-j).
   - N <= j gives the zero ball at N (the existing R4 step 5).
   - |K| > 2^60 is LIMIT. That includes N = LONG_MIN for a ball, which used to return OK.
   - Exact inputs, the exact zero and degree 1 are unchanged.
   - Proof: R2 step 6. The image b + p^E Z_p lies inside b + p^N Z_p because E > N. That ball is the only ball of
     exponent N containing the image, and it is p^N Z_p when N <= j.
   - The `_at` forms, the driver and Julia inherit the rule. The driver passes N = the `prec` value (`st->prec`,
     default 64, `tools/adf/adf.c:97`). It does so at `adf.c:1897`, which reaches `adf_sball_roots_at` at
     `adf.c:1770` and `adf_sball_root_seed_at` at `adf.c:1775`.
2. **F1.** The pre-check `power_ok(p, L+s)` is removed. Log, div, exp, teichmuller, mul and unit_mod test the
   powers they form.
   - A product equal to the exact 1 gives the centre 1 without `unit_mod`, so no p^(K-j) is formed.
   - `1 + 2^(2^27) Z_2`, n = 2, seed 1, N = LONG_MAX: OK, `1 + O(2^(2^27-1))`. At `2^60`: OK at exponent 2^60-1.
   - `1 + 5^(2^60) Z_5`, n = 3: OK, the input ball itself.
   - Still LIMIT where the power is really needed:
     - the -1 branch of `1 + 2^(2^27) Z_2` at N = LONG_MAX (its centre is 2^K-1);
     - the ball with centre 17 and radius 2^(2^27) Z_2 (its Log needs the power).
   - Both of those are OK at N = 100.
3. **F2.**
   - (a) Identifiers are t0 zeta^i with zeta = g^((p-1)/d) and g = `n_primitive_root_prime(p)`
     (refs/src/flint-3.0.1/ulong_extras.rst:1410-1413).
     - t0 is one root of T^d = w^e. It is built from CRT idempotents of p-1: the part prime to d is solved by
       inverting d, and each Sylow q-part by a Pohlig-Hellman digit search over q <= d.
     - t0^d = A is checked before anything is listed.
     - Proved as R8 in `docs/api-1f5.md`.
   - (b) Capacity and `ADF_LROOT_BRANCH_MAX`: in the old code this LIMIT was already decided before enumeration
     (old `src/lroot.c:263`); see finding 1. What was decided late was the precision LIMIT. `early_status()` now
     returns, before the listing, every LIMIT that some branch would return: of K, of z0, or of a Teichmueller power
     p^L that a non-rational branch with seed outside {1, p-1} needs (R6 step 6).
   - `ADF_LROOT_BRANCH_MAX` is kept. The driver uses it (`tools/adf/adf.c:1765`) and bindings may. It now bounds the
     branch count d, that is, the temporary id and lball arrays of `roots` and q in the digit search. No coefficient
     vector exists any more. Strictly it is not needed for memory, because the caller already supplies d slots.
   - (c) z0 is computed once per call and multiplied by each torsion factor (R2 step 1). The rational branches are
     also found once.
4. **F4.** R7 step 2 is now limited to j >= 0, with the counterexample (p = 3, n = 2, x = 3^-2(1+3 Z_3), E = 0 >
   M = -1). The rows scaled by shift = +-1 rely on the scaling instead: x -> p^(n*shift) x has roots p^shift times
   the unscaled roots, so E = shift + E0.

## Checks (commands from the worktree root; B = lanes/f-repair4/build, S = lanes/f-repair4/build-san)

**Red, on the unchanged library** (`timeout 400 $B/test_lroot`): exit 1, with 21325 failed checks in 6 of 10 tests.
- enumeration rows at N = E-1 and E-3: 11100 failed;
- seeded sweep over N from j-2 to E+1: 10171;
- boundary case: 4;
- nd14 test: 24;
- F1 test: 21;
- F2 tests, 3 failed. Against a 2 s CPU guard, the times were:
  - LIMIT at N = 2^40: 5.76 s;
  - listing at 65537 with d = 65536 and N = 0: 5.46 s;
  - listing at 2^64-59 with d = 6028 and N = 0: 5.78 s.

Other red runs on the unchanged library:
- `test_rfunc_prime`: 6 failed checks.
- Driver: the new prec-3 lines printed `3 + O(5^6)`.
- Julia: 8 of 9 passed.

Already green on the old code:
- the capacity-1 check, for the reason in finding 1;
- `f2_identifiers_against_residue_search`: 61324 lists against a search over all residues, for every odd p < 110.
- Log: `redgreen.log`, `red-lroot.summary`.

**Green:**

| Check | Result |
|---|---|
| `timeout 600 make -j2 BUILD=$B $B/test_lroot $B/test_rfunc_prime` | exit 0 |
| `timeout 300 $B/test_lroot` | 10 tests, 1308401 checks, 0 failed |
| `timeout 300 $B/test_rfunc_prime` | 14 tests, 521989 checks, 0 failed |
| Same two in `$S` with `SAN=1`, `ASAN_OPTIONS=detect_leaks=0` | 0 failed each, no sanitizer report |
| One extra run of `$S/test_lroot` with leak detection on | 0 leaks |
| `timeout 600 make -j2 check` (build/) | exit 0; 73 programs, 788 tests, 58092354 checks, 0 failed |
| `sh tests/test_driver.sh` | exit 0; 47 cases, 100871 lines equal |
| `sh tests/test_julia.sh` | exit 0; lroot.jl 9/9 |

The fixtures are now compared at N = `ADF_LBALL_EXP_MAX` (exact image), and also at N = E, E-1 and E-3.

**Reviewer's attacks.** The copies are `lanes/f-repair4/attack*_nd14.py`; they run harness `h` on `$B`. They use
`oracle14.py`: below E it checks N-D14 with exact integers, at N >= E it applies the reviewer's own check unchanged.
A case fails on a wrong status, exponent, valuation, centre (compared with the reviewer's digit lifting), id list or
touched output.

| Attack | Cases | Failures |
|---|---|---|
| attack1, seeds 1-12 | 12 x 27316 | 0 |
| attack1s, seed 21, on an ASan/UBSan `h` over `$S` | 27316 | 0 |
| attack2 | 1092 | 0 (h 3.7 s; the reviewer measured 114 s) |
| attack3 | 1131 | 0 |

The same cases were also run against the reviewer's unchanged N-D13 oracle (`attack*_nd13.py | classify13.py`):

| Attack | Cases that differ from N-D13 | Unexplained |
|---|---|---|
| attack1, seeds 1-6 | 7033 | 0 |
| attack2 | 64 | 0 |
| attack3 | 72 | 0 |

Every one of those differences is an OK ball result with requested N < E, returned at exponent N. That is right by
R2 step 6, and oracle14 checks each one's centre.

**Old against new library** (`gen{1,2,3}.py` + `diff_old_new.py`, the same `h` built over `build-before` and `$B`):
- 111487 cases in total;
- 106631 lines byte-identical;
- 4720 + 64 + 72 differ, all of them balls with N < E;
- 0 old-LIMIT-to-new-OK cases occurred in these sets (the F1 inputs are in the unit tests);
- 0 other differences.

## Timings (wall, once each; `timings-before.txt`, `timings-roots.txt`, `timings-after.txt`)

| Case | Before | After |
|---|---|---|
| `roots`, exact 1, p = 65537, n = 65536, N = 2^40 (LIMIT) | 6.02 s | 0.0000 s (`h`, with process start: 0.017 s) |
| same, N = 0 (list only) | 5.81 s | 0.014 s |
| same, N = 20 | 7.24 s | 1.08 s |
| `roots`, exact 1, p = 2^64-59, n = 6028, N = 0 | 5.98 s | 0.0017 s |
| same, N = 20 | 6.52 s | 0.89 s |
| `roots`, p = 2^64-59, d = 299756, N = 0 | over 100 s (review) | 0.084 s |
| same, N = 20 | not run | 44.6 s (see finding 3) |
| `perf` (exact 3^16 at 65537, n = 16, N = 200) | 27.2 ms (one branch 1.92 ms) | 5.4 ms (1.63 ms) |
| `perf irr` | 26.3 ms (2.10 ms) | 5.1 ms (2.64 ms) |
| `nd13 100000`: root_seed at N = 10 | 50.3 s, exponent 99999 | under 1 ms, exponent 10 |

`polyroots` measures only FLINT's Rabin method against a generator listing, and nothing in this lane changed it. Before
this lane it took 5.66 s (generator listing 0.57 ms) and 5.70 s (0.44 ms); the "after" figures are the `roots` rows
above.

## Mutation testing

Command: `tools/mutate/mutate.py --files src/lroot.c --limit 60 --seed 1 --jobs 2 --san --timeout 300` with
`--copy Makefile include src tests $S`. The make step built and ran `$S/test_lroot` and `$S/test_rfunc_prime`, all
under `timeout 1300`.

Result: 448 mutants, 60 run in 279 s. 46 killed, 9 survived, 5 did not compile, 0 timed out. A first start was
stopped after 2 mutants, because a header edit had forced a full rebuild for every mutant. Survivors:

- **Gap, now tested.** `:361 n==1 -> n==0` (`early_status`): `roots` of degree 1 at N = LONG_MIN became LIMIT. A test
  was added; it is red on the mutant in a scratch copy (4 failed checks) and green on the code.
- **No effect on any value:**
  - `:64 *s=0 -> 1` (exact zero in `criterion`): s is never read for the exact zero; `branch` returns x first.
  - `:26 k>=0 -> k>0` (`power_ok`): the only call has k = L >= c >= 1.
  - `:68 x->v<0 -> x->v<1` and `<` -> `<=`: they differ only at v = 0, where -0 = 0.
  - `:153 sh->L=0 -> 1`: `principal()` sets L before every use.
- **The early LIMIT does not change statuses:**
  - `:363` (negate `K<=j`), `:373` (`ADF_LIMIT -> ADF_OK`), `:370` (rational scan starts at index 1).
  - With these mutants the early check is weakened, but every branch failure is LIMIT and the per-branch evaluation
    returns it. Removing the early LIMIT costs only one listing, which is now O(d): 14 ms at d = 65536 and 84 ms at
    d = 299756. A test could kill these only with a timing guard that tight.

No entry was added to `tools/mutate/equivalent.txt`. Nothing to report about the tool itself.

## Not done

- No long fuzzing run. The attacks above take seconds each, so they count as smoke tests plus exhaustive grids.
- `d = ADF_LROOT_BRANCH_MAX` exactly is not exercised; the review says the same.
- The Teichmueller lift per branch is not shared (finding 3). COMMON-C rule 7 says not to optimise, and F2(c) named
  only Log/exp.
- `docs/SPEC.md` 15.4: the N-D13 row still describes the old rule. That file is not mine; N-D14 supersedes the row.

## Findings against the specification or the review

1. **The brief's F2(b) premise is half wrong.** The old code already decided the capacity and
   `ADF_LROOT_BRANCH_MAX` LIMITs before the enumeration (old `src/lroot.c:263`). The 5.9 s LIMIT in the review was
   the precision LIMIT (the Teichmueller power at N = 2^40), and that is the one now decided early. The brief's
   capacity-1 test was green on the old code.
2. **The early decision is not needed for correctness.** With the generator listing, F2(b) only saves the O(d)
   listing; the mutants at `:363` and `:373` survive for this reason.
3. **An avoidable cost remains.** Each non-rational branch runs a Teichmueller Newton lift at L: 149 us per branch
   at p = 2^64-59, N = 20, which makes 44.6 s for d = 299756. Teichmueller representatives are multiplicative, so
   omega(t0 zeta^i) = omega(t0) omega(zeta)^i. Two lifts and d multiplications at L would suffice. Not done.
4. **Behaviour changes that N-D14 implies** (the tests now state them):
   - A ball with N <= j gives a zero ball. Previously that could not happen for a ball.
   - A ball with |min(N,E)| > 2^60, N = LONG_MIN included, gives LIMIT where N-D13 gave OK.
   - x = 3^-(2^60)(1 + 3^(2^61) Z_3), n = 2, N = 20 now gives:
     - seed 1: OK, v = -2^59, exponent 20, centre 1;
     - seed 2: LIMIT, because its centre is 3^(2^59+20) - 1.
     Under N-D13 both were LIMIT. Relative precision K - j can be large when j is very negative, so the
     `lroot.h` text says that N bounds the work through the relative precision max(K-j, c).
5. No false statement was found in R1-R6 besides F4. The review's oracle facts O1 to O3 were used as given, through
   `oracle.py`.
