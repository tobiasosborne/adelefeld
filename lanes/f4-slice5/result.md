# Lane f4-slice5: result (slice 4f of docs/api-4.md: evaluation and additive integrals of test functions)

All work was done in the worktree /home/tobias/Projects/adelefeld-wt/f4-slice5 (branch lane/f4-slice5).
- No git command that changes state was run, and no bd.
- Builds used `make -j2`, and every program ran under `timeout`.
- The build trees are removed: `lanes/f4-slice5/{build,b-*,mbuild}` and the root `build/` (the driver,
  `libadelefeld.so`, the mutation scratch). The fault copies were in the session scratchpad and are removed.
- The lane directory is 88 KB, with no file above 4 KB except the logs (largest 3.2 KB).

## Files

- New:
  - `include/adelefeld/tensor.h`: all seven declarations. A comment says why the three `adf_ffun_*` are not in
    `ffun.h`.
  - `src/tensor.c`
  - `tests/test_tensor.c`
  - `tests/ref/vectors/f4-slice5/{eval,local,sball,tensor,integral}.jsonl` (184 KB)
  - `tests/driver/tensor-eval.cmd` and `.out`
  - `tests/julia/tensor.jl`
  - `docs/api-4c.md`
  - `lanes/f4-slice5/{gen_vectors.py, plant_faults.py, variants.sh, redgreen.md, result.md}` and logs
- Changed:
  - `include/adelefeld.h`: one line.
  - `tools/adf/adf.c`: 7 enum values, 7 table rows, 1 dispatch line and the function `adf_drv_tensor`.
  - `tools/adf/README.md`: a section at the end.
  - `tests/test_julia.sh`: one block.
- Not touched: `src/ffun.c`, `ffun.h`, `src/rfun.c`, `rfun.h`.

## A. Vectors

Command: `timeout 300 python3 -B lanes/f4-slice5/gen_vectors.py`. It is deterministic: a rerun gives identical
md5 sums for all 5 files.

- `eval.jsonl`, 1603 records:
  - `(D, M)` in `(1,1), (2,3), (3,2), (4,1), (1,4), (6,6), (12,5)`;
  - finite balls with `d` in `{1, 2, 3, 4, 5, 12}`, `H` in `{0, 1, 2, 3, 6, 12}` and `A` in
    `{-7, -1, 0, 1, 5, 11}`;
  - `A = ±(2^1999 + 123456789)`;
  - 5 raw or fractional-radius triples.
  - Each record holds the oracle's `evaluate` mask and outside flag.
  - The generator asserts the oracle against rational samples of the ball (as functions4_checks.py:430-435) and
    against the canonical triple.
- `local.jsonl`, 56 records: 8 local balls `c + R Zhat` of the context with blocks 8, 9, 5, among them canonical
  `d = 1` and `d = 2`.
- `sball.jsonl`, 140 records:
  - 91 tuples at the places `{2, 3}`, `{5}` and `{}` (balls and exact points), masks from the oracle's
    `evaluate_partial`. The brief expected that the oracle might not cover sballs; it does
    (functions4_checks.py:191-200). The generator also asserts its own transcription of the E1 step 4 rule.
  - 49 adele projections to `{2, 3}`. They agree with the adele index set in 46 of 49; the 3 others are at
    `(12, 5)`, where the prime 5 of `M` is not supplied.
- `tensor.jsonl`, 25 records: 5 rfuns at 4 points (`rvalue_ball` at 400 bits); their integrals (the transform at 0)
  and norms (ordered pairs of `rterm_product` with the conjugate).
- `integral.jsonl`, 14 records: exact integrals and norms of two families per `(D, M)`.
- `timeout 120 python3 -B proto/functions4_checks.py`: TOTAL 2670 checks.

## B and C. Tests, then code

- RED A: test and header written in full; the build was a link failure, 20 undefined references (`red-a.log`).
- The code for all seven functions was then written in one step. Per function, the red evidence is the fault
  table of F.
- Corrections before green (`redgreen.md`): one in the code (the hull, see finding 1) and four in the test.

What `test_tensor` covers, every vector file run in full:

- **ffun_eval:** 1603 balls times 3 families (integer, complex, ball values). For each:
  - every selected `f[j]` lies in `z`, and 0 does when the ball leaves the support;
  - tightness: `z` lies in the exact hull widened by `2^-120 (1 + |h|) + 2^-26 rad(h)`;
  - exclusion: an unselected value outside the hull is outside `z` (49644 such checks; at least 1000 are
    required);
  - the empty set gives exactly 0.
- **Local backend:** 56 balls; the result equals the global result, also through `tensor_eval`.
- **E1 step 3, exactly:**
  - `Zhat` gives `3 +/- 2`;
  - `(1/4) Zhat` gives `3 +/- 3`;
  - `1/5` gives exactly 0;
  - point balls give `f[j]` exactly, ball values included;
  - the jump of PLAN 4.4 contains 10 and 20 and excludes 9 and 21.
- **tensor_eval:**
  - every phi, at 4 points and on 4 balls, overlaps the oracle product and lies in its widened product;
  - it equals the product of the two component calls;
  - a real ball of radius `2^-10` contains the values at both ends.
- **tensor_eval_sball:**
  - 91 tuples and 49 projections, times 3 families;
  - the adele result lies in the sball result;
  - the oracle example `{2, 5}` with 0;
  - the exact point `1/4` at 2 gives exactly 0;
  - NONE: the Gaussian gives `R = 1`, and 8 rfuns are checked against 1000 sampled points each (among them
    `exp(-pi x^2 + 4 x)`);
  - COMPLEX: `DOMAIN` with `z` untouched (sentinel bytes).
- **Integrals and norms:**
  - exact rationals; the result is exact for `M` in `{1, 4}`;
  - Parseval against `adf_ffun_fourier`;
  - the faults_44 values 7 and 91/3;
  - a value `0 +/- 1` gives a nonnegative norm;
  - tensor products against the oracle.
- **Statuses:**
  - prec 2, 53, 0, -5 and the cap; cap + 1 and `WORD_MAX` give `LIMIT` for all seven, `z` untouched;
  - `C = 10^300` gives `NOT_DETERMINED` for the four tensor calls, `z` untouched;
  - a nonfinite raw real gives `DOMAIN` (non-INV only);
  - the zero rfun gives exactly 0.
- **Caps:**
  - `2^20 + 1` entries (forged) give `LIMIT` for all seven; `2^20` entries pass;
  - work: `2^20` entries times 1 prime pass, times 2 do not; `2^19` times 2 pass, times 3 do not;
  - bits: `2^20 - 128` bits pass and one more does not (`A`, `d`, an lball centre);
  - `2^16 + 1` rfun terms give `LIMIT`, and `2^16` pass.
- **Storage guards:** an entry beyond `L` is never read; hulls at prec 1024 use allocated arf endpoints.
- **INV:** 14 forked children abort. The prec cap is decided before the entry predicates.

Results:

| Build | Checks |
| --- | --- |
| plain | 149857 |
| `SAN=1` (`ASAN_OPTIONS=detect_leaks=1`) | 149857 |
| `INV=1` | 149891 |
| `CC=clang` | 149857 |

All four pass. Command: `sh lanes/f4-slice5/variants.sh`, which runs
`make -s -j2 BUILD=lanes/f4-slice5/b-<v> ... lanes/f4-slice5/b-<v>/test_tensor`.

## D. Driver and Julia

- `adf_drv_tensor` implements the seven commands. The finite ball of `ffun_eval` can also be a rational.
- `tests/driver/tensor-eval`: 21 expected lines, derived by hand before the run.
  - Among them are E1 step 3 (`(3 +/- 2)`, `(3 +/- 3)`, `(0)`), the jump `(15 +/- 5)`, `(2.4652 +/- 2.6e-5)`,
    exactly 0 at the 2-adic point `1/4`, `30.333 +/- 0.00034` and `21.449 +/- 9.5e-5`, and the statuses.
  - One line, the NONE box, had also been seen in an exploratory run before the fixture was written.
  - The first run agreed on all 21 lines.
- `sh tests/test_driver.sh`: 88 cases, 101543 expected lines, all equal.
- `tests/julia/tensor.jl` (the ccall of design section 9), registered in `tests/test_julia.sh`: 10 of 10.
  - Run directly as `LD_PRELOAD=libgmp julia tests/julia/tensor.jl build/libadelefeld.so`, with the preload
    that the script applies itself.
  - The `.so` came from `sh tests/test_exports.sh`: 591 of 591 declared functions are exported.
  - `tests/test_julia.sh` was not run as a whole (not check-all).

## E. Statements

`docs/api-4c.md` holds:
- the order of statuses;
- statements 1 to 5 with proofs: the gcd criterion, the support test, the image and the hull, step 4 with local
  independence (and that 0 is attained), the valuation without `p^v`, step 5's bound, and P4;
- "Check:" lines;
- 6 decisions;
- the cost.

## F. Faults

Command: `timeout 3000 python3 -B lanes/f4-slice5/plant_faults.py <scratch>`. Every planted fault makes the test
fail. Logs: `faults.log`, `faults-8.log`, `faults-survivors.log`.

| # | Fault | Caught by |
|---|---|---|
| 1 | faults_44: a jump sampled at one point (first coset met) | exact_cases `exact_ball(z, 3, 2)` |
| 2 | faults_44: outside test without `d \| D H` | `(1/4) Zhat` contains 0 |
| 3 | faults_44: empty set (1/5) not exactly 0 | `acb_is_zero` |
| 4 | faults_44: partial evaluator without the zero | sball_vectors contains 0 |
| 5 | faults_44 / brief: `1/D` for `1/M` in the integral | tensor integral vs oracle |
| 6 | faults_44: abs(f) for abs(f)^2 in the norm | tensor norm2 vs oracle |
| 7 | gcd criterion with lcm | exact_cases |
| 8 | support test dropped (zero never included) | `(1/4) Zhat` contains 0 (after the first copy did not build) |
| 9 | jump sampled at the midpoint only (centre's coset) | exact_cases |
| 10 | step 4 with max instead of min | sball_vectors containment |
| 11 | step 4 exact point with `min(0, v_p(M))` | sball_vectors tightness |
| 12 | step 5 without `exp(beta^2/(2 alpha))` | none_bounds sample of `exp(-pi x^2 + 4x)` |
| 13 | norm not clipped | `arb_is_nonnegative` |
| 14 | `z` written before the last check | statuses, sentinel |
| 15 | bits cap one bit short | caps |
| 16 | work cap off by one | caps |
| 17 | COMPLEX accepted | statuses DOMAIN |
| 18 | hull radius always rounded up | `exact_ball(z, 3, 2)` |
| 19 | (survivor) integral loop to `j <= L` | crash / guard_cases |
| 20 | (survivor) `arf_clear(b->hi - k)`, SAN | ASan abort |
| 22 | (survivor) tensor_eval without ffun entry check, INV | INV child 10 |

Fault 21 (tensor_eval without the rfun entry check) is not caught: `adf_rfun_eval` checks phi under INV itself.

Mutation testing: `python3 tools/mutate/mutate.py --files src/tensor.c --limit 60 --seed 20261008 --jobs 2 --san
--timeout 300 --make "make -s -j2 INV=1 SAN=1 BUILD=lanes/f4-slice5/mbuild lanes/f4-slice5/mbuild/test_tensor &&
ASAN_OPTIONS=detect_leaks=1 timeout 300 ./lanes/f4-slice5/mbuild/test_tensor" --copy Makefile include src tests
lanes/f4-slice5/mbuild`.

| Run | Time | Killed | Survived | Not compiled |
| --- | --- | --- | --- | --- |
| 1 (`mutate-run1.log`) | 193 s | 50 | 7 | 3 |
| 2 (`mutate-run2.log`, after the new tests) | 213 s | 53 | 4 | 3 |

Survivors of run 2, one line each:
- `tensor.c:346 l->v >= 0 -> > 0`: equivalent; at `v = 0` the factor is `p^0 = 1` on either side.
- `tensor.c:432 drop ok = 0`: unreachable; a canonical `Re A` is certified positive, so `pi lower(Re A) > 0`.
- `tensor.c:490 x->len > 1 -> > 0`: equivalent; with one prime, `L > 2^20 / 1` is excluded by the items cap.
- `tensor.c:284 drop TN_INV_RFUN(phi)` in tensor_eval: equivalent under INV; `adf_rfun_eval` checks phi.

No entries were added to `tools/mutate/equivalent.txt`; it is not mine.

Tool behaviour: mutant `tensor.c:178 if(inexact) -> if(!inexact)` is reported "not compiled". It built; at run time
FLINT aborted with `error: ulp error not defined for special value!`, and the tool's `COMPILE_ERROR` pattern took
that line for a compiler error. It is in effect killed. Not repaired.

## Findings

1. **Design, hull by `acb_union`.**
   - In the installed FLINT 3.0.1, `arb_set_interval_arf(1, 5)` gives `3 +/- (2 + 2^-28.4)`: the radius is rounded
     up by one mag ulp even when it is exact.
   - Chained `acb_union` compounds this: `{2 +/- 1} ∪ {5}` gives `(3 - 2^-30) +/- (2 + 2^-27)`.
   - The code forms the same rectangle over all balls at once and builds the ball itself (`tn_set_interval`,
     proof in api-4c statement 1), with an exact radius when it fits 30 bits. So `[1, 5]` prints `3 +/- 2`.
   - A single ball is copied exactly, so a point ball gives `f[j]` exactly.
   - The test bound states the remaining `2^-26 rad` term: every arb product rounds a radius up, the product with
     the exact 1 included, so the driver prints `3 +/- 3.1` for `1 · [0, 6]`.
2. **Design, E1 step 4, strengthening (no defect).** "Always include zero" is exact, not only safe: some prime is
   absent, and `t_q = q^(-v_q(D) - 1)` gives a completion outside support. The hull is therefore the hull of the
   exact image.
3. **Oracle:** `evaluate_partial` covers sballs (the brief expected that it might not); it agrees with an
   independent transcription of step 4 on all 91 tuples. No defect found in the oracle or the goldens.
4. **Design statuses:**
   - `ffun_eval`'s "raw real input must be finite, otherwise DOMAIN" (api-4.md:274) concerns only the real calls.
   - `NOT_DETERMINED` of `ffun_eval`, `ffun_integral` and `ffun_norm2` is unreachable on canonical input (arb
     exponents are unbounded), as is the `alpha <= 0` branch of step 5.
5. **No HEADER-FINDING.** All seven functions are implemented as written.

## Sources pending

- `[source pending: Fubini/Tonelli for the product of Lebesgue and Haar measure]`: `tensor_integral` and
  `tensor_norm2` use the product formula; this is the analytic import listed at docs/proofs/analysis.md:30-66,
  inherited from the design.

## Not done

- The dump forms and the transform certificate (4c), Poisson (4g) and the finite algebra (4b) are out of scope.
- `tests/test_julia.sh` and `check-all` were not run as a whole.
- No fuzzing.
- Avoidable cost (api-4c Cost): `j d` is recomputed per index; `v_p(M)` and `v_p(D)` are recomputed per index in
  `tn_keep`.
