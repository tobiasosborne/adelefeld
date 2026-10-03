# Lane f-repair5: result

Worktree: `/home/tobias/Projects/adelefeld/.claude/worktrees/agent-ad395d4412577a4a3`, based on `8f57a51`. Nothing
is committed. Running notes: `lanes/f-repair5/progress.md`. Red-green log: `lanes/f-repair5/redgreen.log`.

## The four repairs

1. **Stale cost sentence (finding 1).** In `include/adelefeld/lpow.h` (comment of `adf_lball_powunit`), "one Log at
   min(A, N - B) for alpha" is now "one exact subtraction for alpha (P6 (a), decision 10 of api-1f6.md; only when
   the exponent ball is not exact)". This is what `principal()` in `src/lpow.c` does. The integer-exponent cost
   of item 2 is added after it.
2. **LIMIT for a small result (finding 2): decision (a), proved.** In `src/lpow.c`, `integer_exponent()` and
   `via_pow_si()` take a ball `u` with an exact integer exponent `k` that fits a `slong` (after the domain checks
   and the exact 1). The result is `pow_si(u, k)` when `N >= R`, and `pow_si(u, k) + p^N Z_p` when `N < R`. If this
   way returns `LIMIT`, the general way of Proposition 18 runs as before. The proof is P9 in `docs/api-1f6.md`,
   with decision 12, and P8 step 2 is amended. The `lpow.h` block states the new rule and the example; `s = -1`
   and `s = 1/2` stay `LIMIT`.
3. **False order sentence (finding 3).** `early_status()` in `src/lroot.c` now decides the two cases before the
   listing. The new rule is: LIMIT if `general > one` and `p^L` is beyond the bound. Here `one` (0 or 1) counts the
   general branch whose `z = z0*omega(t)` is the exact 1, that is `z0 = +-1` exactly and `t` the seed with
   `omega(t) = z0`. The old rule counted the seeds 1 and p-1 both as free of powers, but the branch p-1 needs the
   centre `p^(K-j) - 1`.
   - Proof: R6 step 6 of `docs/api-1f5.md` is rewritten as a, b, c. The early status is the status of the whole
     evaluation.
   - Corrected sentences: the `lroot.h` comment of `roots`, R6 steps 4 and 6, and one erratum paragraph appended
     to `lanes/f-repair4/result.md` (dated 2026-10-03, naming f-review7 finding 3).
   - `api-1f5.md` line 31 is about the capacity `LIMIT`, which was always early. It is not refuted and was left as
     it is.
4. **One Teichmueller lift per list.** `adf_lball_roots` now evaluates the branches in the chain order
   `t0 zeta^i` of R8. It finds each slot by a binary search in the sorted ids (`position()`).
   - At odd p, the first general branch whose seed is not +-1 lifts `omega(seed)` and `omega(zeta)` once. After
     that, `y = c0 * omega(seed_i) mod p^(K-j)`, with `c0` the unit of `z0`, is updated by one `fmpz_mod_mul` per
     step. `y` is the centre that `branch()` would compute.
   - Seeds +-1, rational seeds and everything at p = 2 still go through `branch()`.
   - `adf_lball_root_seed` is unchanged and keeps its one lift.
   - Proof: R9 in `docs/api-1f5.md`, with decision 11. (a) Teichmueller representatives are multiplicative, by
     the uniqueness of Lemma 3 item 2. (b) `y_i` equals `unit_mod(z0*t, K-j)`, so every branch is identical in all
     fields to the one-lift code. (c) No new status.

## Item 2: the proof (P9), in short

For a ball `u` in `1 + p Z_p` and `k != 0`:
- **(a)** `pow_si(u, k)` is the set `{t^k}` and a ball (L12).
- **(b)** By Proposition 17 ("agrees with every integer power", functions.md:585) the image in P5 is the same set.
  Its exponent R agrees with L12's `R'` in every case:
  - odd p: `A + v_p(k)`;
  - p = 2, A >= 2: `A + v_2(k)`. The sign factor is included: `u0^k = w0^(k mod 2) exp(k log(w0 u0))` is P5's
    centre.
  - p = 2, A = 1, k even: `2 + v_2(k) = 1 + v_2(k) + 1`;
  - p = 2, A = 1, k odd: 1.
  - The case `w0 = -1, B = 0` cannot occur, because `s` is exact.
- **(c)** For `N < R` the ball at `N` is `I + p^N Z_p` (L2, P2 step 5).
- **(d)** A canonical value is determined by its set, so whenever both ways return OK the fields are identical.
  `pow_si` of a ball without 0, and `add`, return only OK or LIMIT.
- **(e)** So no OK result changes. The domain statuses come first and are unchanged. A LIMIT becomes OK exactly
  when the route succeeds.
- **Not routed:** an exact `u` (decision 8: the ball at `N`, not the exact power, which is a different value),
  `s = 0`, a ball `s`, a non-integer `s`, and `|s|` beyond a `slong`.

The proof did not fail anywhere.

## Checks (commands, numbers, what would have failed)

- **Red** (`redgreen.log`), on the code at 8f57a51:
  - `test_lpow`: 21 failed checks, which are 7 cases with their aliased calls, all `LIMIT` where `OK` was expected:
    - the three inputs of `limits.in` lines 2 to 4: `(6 + 5^(2^40) Z_5)^1` at N = 2^40 and 2^60, `^2` at 2^40;
    - `^1` at N = 2^40 - 1;
    - `(3 + 2^(2^30) Z_2)^3`, `^2` at 2^60, and `^2` at 2^30.
  - `test_lroot`: the guard failed, 2.307 s CPU against 0.5 s.
  - The item 3 test `review7_f3_branch_limit_after_a_good_branch` was green on the old code. The status was already
    right. The order is not observable through the API, because the only branch computed before the late LIMIT
    is the seed 1, which needs no power. So this test guards the status and the untouched outputs, not the order.
- **Green**, `BUILD=lanes/f-repair5/build`:
  - `test_lpow`: 10 tests, 3066274 checks, 0 failed. The new `powunit_integer_exponent_is_pow_si` has 41723
    calls. A call fails on another status, or on any field that differs from the coarse ball of `pow_si`, with
    every aliasing of `check_powunit` checked. Its grid (p = 2, 3, 5, 7, 65537; every centre of `1 + p Z_p` modulo
    `p^A`, A <= 4; k = -7..7; nine N) already passed on the old code. That is the "byte-identical before and after"
    demand for every case that was OK. The old test `powunit_identities` (`u^k` contains `pow_si(u, k)`) uses
    exact `u`, which is not routed.
  - `test_lroot`: 12 tests, 1323173 checks, 0 failed.
  - `test_rfunc_prime`: 15 tests, 522217 checks, 0 failed.
- **Sanitizers**: `SAN=1`, `ASAN_OPTIONS=detect_leaks=1`, `BUILD=lanes/f-repair5/build-san`. The same three
  tests passed with the same counts and no sanitizer report.
- **Item 4 guard** (`r9_one_teichmuller_lift_per_list`): p = 2^64-59, x = 3^n + p^30 Z_p, n = d = 24068, N = 20,
  CPU seconds as in the file's other guards.
  - Guard `GUARD_R9` = 0.45 s.
  - Old code (the same test file linked to the old archive): 2.307 s with little load, then 3.937, 5.317 and
    7.770 s at load average 7 to 9. It misses the guard by a factor of 5.1 or more.
  - New code: 0.039 to 0.089 s over 13 runs at load average 5 to 9, and 0.110 s under SAN. That is a margin of
    5.1 or more without SAN.
  - The test also compares every branch at d = 1094 and d = 6028 (exact x = 1) and every 97th at d = 24068 with
    `root_seed`: 7371 branches with identical fields.
- **Item 4 timings** (wall, `lanes/f-repair5/tbranch.c`, p = 2^64-59, N = 20):

  | d | before | after |
  |---|---|---|
  | 6028 | 0.64 s (105 us per branch) | 0.02 s |
  | 24068 | 2.73 s | 0.04 to 0.08 s |
  | 74939 | 8.58 s | 0.22 to 0.47 s |
  | 299756 | 44.6 s (lane f-repair4) | 1.19 s (exact x = 1), 1.44 s (ball) |

- **`tbranch` with `CHECK=all`**: every listed branch against `root_seed` at the same seed, in 7 settings (d =
  24068, 12034, 3014, 6017; exact and ball; z0 exact and not; N = 7, 20, 25, 200; one with K = 12 < N). 78232
  branches, 0 differences.
- **Differential, old library against new**, through f-review7's `h.c`:
  - `diff_powunit.py 10000 3`: 9876 lines identical, 124 `LIMIT` -> `OK`, 0 failures. Each of the 124 was checked
    equal to the coarse ball of `pow_si`. A failure would be any other change: a different OK value, a different
    status, or an aliasing difference. A smoke run of 3000 cases with seed 2 gave 2964 / 36 / 0.
  - `diff_roots.py 6000 2`: 7821 lists and seeded calls, 0 differences in any byte (statuses, ids, fields,
    TOUCHED, ALIAS). 186107 branches compared. Statuses: A OK 1719, A LIMIT 2555, A DOMAIN 1641, A NOT_DETERMINED
    85; S OK 461, S LIMIT 99, S DOMAIN 1239, S NOT_DETERMINED 22.
- **Reviewer's attacks** (copies in `lanes/f-repair5/attacks/`, harness built against this lane's archive with
  `-std=gnu11`):

  | Attack | Command | Result |
  |---|---|---|
  | B1 | `attack_roots.py 1 exact` | 4758 cases, 14344 branches, 0 failures |
  | B1 | `attack_roots.py 1 ball` | 4758 cases, 14582 branches, 0 failures |
  | B1 | `attack_roots.py 2 exact` | 4758 cases, 14154 branches, 0 failures |
  | B2 | `attack_roots_big.py 1` | 68 lists at p = 65537 and 2^64-59, d up to 824329, N = 3 (the chain path), 0 failures; d = 824329 takes 1.0 to 1.7 s |
  | B3 | `attack_shared.py 1 3000` | 8594 branches identical to `root_seed`, 0 differences |
  | B4 | `attack_early.py` | 1200 lists, 2590 seeded calls, 0 differences (311 OK, 444 LIMIT, 445 DOMAIN) |
  | A2 | `attack_powunit.py 4000 s`, s = 1..4 | 16000 cases, 0 failures |

  Each attack fails on: wrong ids, order or count against a residue search; a certified root outside its ball; a
  list entry different from `root_seed`; a list status different from the seeded statuses; an image or
  tightness mismatch modulo `p^H`.
- **`check-all`**: `timeout 900 make -j2 check-all` was run once at the end, in `build/`. It covered 74 test
  programs (all "0 failed"), the driver (49 cases, 100916 expected lines, all equal), exports (424 of 424), Julia,
  the mutate self-test and the memcheck self-test. Last line: `check-all passed: make check, driver, exports,
  julia, mutate-selftest, memcheck-selftest`.

## Mutation testing (changed lines only, `--seed 1`, 2 jobs, `--san`, 1290 s in all)

The tool has no line filter. `lanes/f-repair5/mutate_lines.py` loads it and keeps the mutants on the new-side
lines of `git diff -U0` (`lpow.diff`, `lroot.diff`).
- The root was a scratch copy with a prebuilt SAN build (`BUILD=mb`).
- `--make "make -s BUILD=mb mb/test_lroot mb/test_rfunc_prime mb/test_lpow && ./mb/test_lroot && ./mb/test_rfunc_prime
  && ./mb/test_lpow"`. The string does not contain "SAN", so `--san` was in effect.
- Both runs hit their bounds: 700 s for `lpow.c` and 590 s for `lroot.c`, each with 2 runs in progress. The tool
  prints killed mutants only in its final summary, which the bound cut off, so the number killed is not
  recorded.
- **Not-compiled mutants are reported as NOT COMPILED, not as killed, in this run.** All of them are -Werror cases
  (`&&` inside `||` without parentheses, a dropped assignment leaving a variable uninitialised, `*k` -> `+k`, an
  unused static function).

**`src/lpow.c`**: 33 mutants on changed lines. Printed: 7 NOT COMPILED, 4 SURVIVED. The last but one mutant of the
shuffled list was judged, so at least 31 were judged and at least 20 killed. Survivors, all equivalent:
- `:238 s->v >= 64` -> `> 64`: for v = 64, `|s| >= p^64 > 2^63` and `fmpz_fits_si` is false anyway.
- `:262 P->N <= Nc` -> `<`: for `P->N == Nc`, `P + p^Nc Z_p` is `P` itself.
- `:267 add(res, P, zb)` -> `add(res, zb, P)`: the sum is commutative.
- `:267 add` -> `sub`: `P - p^N Z_p` and `P + p^N Z_p` are the same set.
- Not judged before the stop (2 runs): among them probably `:309 st = ADF_OK;` dropped. That mutant is equivalent,
  because every path after it assigns `st`.

**`src/lroot.c`**: 106 mutants on changed lines, 40 run. Printed: 3 NOT COMPILED, 1 TIMED OUT (`:363 hi-lo>1` ->
`>=`, an endless loop, so in effect killed), 3 SURVIVED:
- `:423 fmpz_mod_mul(y, y, c0)` -> `(y, c0, y)`: the product is commutative. Equivalent.
- `:464 p-1` -> `p+1`, the seed when `z0` is exactly -1: unreachable. `z0` is `exp` of something, and `exp` is
  exact only for the exact 0, giving 1 (lfunc.h line 20). The code keeps the general form so that the rule does
  not rest on that fact.
- `:442 (seed==1 || seed==3)` -> `&&`, the 2-adic branch test for even n in `general_seed`: equivalent. At 2 with n
  even both branches are general or both rational, so `general` is 2 or 0, and `general > one` comes out the
  same for `one` = 0 or 1.

No entry was added to `tools/mutate/equivalent.txt`.

## What is not done

- The killed counts of the two mutation runs are not known (see above). The remaining 66 mutants of `lroot.c` on
  changed lines were not run (limit 40).
- No fuzzing. The differential runs above are random runs of seconds to minutes; they are not the long
  differential fuzz of `docs/workflow.md`.
- `ADF_LBALL`-level speed-ups beyond the brief were not attempted, for example a Montgomery product or avoiding
  the copy into the caller's slots. After the change, the listing at N = 20 is dominated by one 20-limb modular
  product per branch.

## Findings against the specification or the review

1. **The brief's description of the tool's defect did not show here.** Mutants that did not compile were reported
   as NOT COMPILED (7 and 3), not as killed. The real defect is a different one: killed mutants are printed only in
   the final summary, so a run stopped by its bound loses the killed count.
2. **The item 3 test cannot be red.** The defect was the order, and it is not observable through the API: the
   branches computed before the late LIMIT are at most the seed 1, which forms no power. A test can only guard the
   status and the untouched outputs.
3. **The non-fused fallback of R9 is unreachable.** R9 tests whether `z0` has relative precision >= K - j and,
   if not, lifts per branch. By R4 that never happens. It is kept for robustness and costs nothing.
4. **Remaining LIMITs for a small result.** P9 does not remove every LIMIT for a small result. An integer exponent
   whose `pow_si` needs the power (`k < 0`, or a large centre) still falls back, for example `(6 + 5^(2^40)
   Z_5)^(-1)`. So does a non-integer exponent such as `^(1/2)`. `lpow.h` states this.
