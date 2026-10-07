# Lane f4-slice6: slice 4g, Poisson summation with certified tails (result)

Claude Opus subagent, about 95 minutes. Worktree `/home/tobias/Projects/adelefeld-wt/f4-slice6`, branch
`lane/f4-slice6`. No git command that changes state, no `bd`. Statements: `docs/api-4c.md` "Slice 4g" (G1-G6).
Red-green log: `lanes/f4-slice6/redgreen.md`.

## Files

New: `src/poisson.c` (575 lines; a new file, since `src/tensor.c` at 684 lines would pass about 1000),
`tests/test_poisson.c`, `tests/ref/vectors/f4-slice6/poisson.jsonl` (51 records, 182 KB),
`tests/driver/poisson.cmd` and `.out`, `tests/julia/poisson.jl`, and in the lane directory `gen_vectors.py`,
`plant_faults.py`, `variants.sh`, `redgreen.md`, logs (all below 10 KB).
Appended: the declaration and comment block in `include/adelefeld/tensor.h`; the command `poisson` in
`tools/adf/adf.c` (enum value after `ADF_DRV_TENSOR_NORM2`, table row, dispatch, `adf_drv_poisson`) and its section
in `tools/adf/README.md`; the block of `poisson.jl` in `tests/test_julia.sh`; "Slice 4g" in `docs/api-4c.md`.
`tests/test_tensor.c` is unchanged.

## What is done, per step

A. `timeout 900 python3 -B lanes/f4-slice6/gen_vectors.py` (32 s): 50 tensors, `(D, M)` in (1,1), (2,3), (3,2),
   (4,1), (6,6) times 7 functions (Gaussian; `x`, `x^2` times it; shifted `B = 7/10`; complex `A = 6/5 + i/5`
   with complex `B`, `C` and coefficients; two terms; degree 4), two value families at (2,3) and (6,6), `delta_1`
   at (2,3); each with both sides from the oracle's `poisson_sides_ball` at 600 bits with tails below `2^-460`
   (60-digit balls), `choose_cutoffs` for bits 20, 53, 80, 128, the Lemma 6 bound `lattice_tail` per lattice and
   term at `N` = 0, 1, 2, 4, 8, 16, and `max_k |g_k|`; one record with `sum_n exp(-pi n^2)` at 60 digits.
   The theta witness (`c` in `[1, 2]`) is built in the test (its value interval is `[theta, 2 theta]`).
B. `tests/test_poisson.c`, 47835 checks (plain), 47850 (INV): every record at every bits with `prec = bits + 64`
   (200 calls): both sides contain the oracle balls, every coordinate diameter `<= 2^-bits`, the sides overlap,
   `NL` and `NR` equal to `choose_cutoffs` (all 200), 1651 comparisons with the planted bounds (each earlier `N`
   not below the goal, the returned one not above it), 351 radius windows `[E, E (1 + 2^-40) + 2^-(bits+20)]` on
   both coordinates; `direct`: each side against a partial sum formed in the test from `adf_ffun_fourier`,
   `adf_rfun_fourier` and `adf_rfun_eval` (50 records); theta at bits 0, 20, 53, 80, 128 (prec 144), retries
   from prec 20 to bits 53 and 100 and from prec 2 to bits 30; the witness: NOT_DETERMINED at bits 0, 10, 53
   (prec 144) and 20 (prec 20), sentinel bytes of both balls and both cutoffs unchanged, under 1 s of processor
   time; `c` in `[1, 1 + 2^-40]`: OK with both `theta` and `(1 + 2^-40) theta` inside; `c` in `[1, 1 + 2^-10]`:
   NOT_DETERMINED; the width rule after the tail (diameter `2^-13 - E/2` before the tails, `2^-13 + 3E/2` after:
   NOT_DETERMINED; half the input width: OK); three precisions (80/30, 144/70, 212/110: diameters decrease);
   bits -1, 2^21 + 1, `WORD_MIN`: DOMAIN; prec cap + 1: LIMIT first; bits `2^21` with the zero function: OK, exact
   zeros; `f = 0`: exact zeros and cutoffs 0; `Re(A) = 2^-60` and `10^-8`: LIMIT, both in under 0.25 s (measured
   0.000 s); `Re(A) = 10^-3`: OK; `f = [1, -1]` at (1, 2) (`g[0] = 0`); a zero polynomial term changes nothing
   (`acb_equal`); `2^16` terms of `2^-20 exp(-pi x^2)`: OK at the caps' boundary, `theta/16`; `2^16 + 1` terms:
   LIMIT, also with bits -1; `L = 1025`: LIMIT, also with bits -1; INV: 6 aborts (`left == right`, `NL == NR`,
   `NULL` cutoffs, `Re(A) < 0`, a nonfinite `f` entry), prec cap before INV.
C. `src/poisson.c`: `pn_series` (Lemma 6 with the certified ratio, prefix charged, a lower-bound preflight),
   `pn_lattice` (Taylor shift `acb_poly_taylor_shift`, `q_j = Q_j h^j`, `alpha`, `beta`, `gamma` outward),
   `pn_tail_left`, `pn_tail_right`, `pn_search` (0, 1, 2, 4, ...; D1 checked before each step), `pn_attempt` (both
   transforms, both searches, both sums, tails on both coordinates), `adf_tensor_poisson` (retries by doubling,
   the stall rule, atomic commit). Sources cited at the top of the file and at each function.
D. `poisson R with F with BITS` prints `LEFT | RIGHT | NL=n NR=n`. Fixture `tests/driver/poisson`: 11 lines derived
   by hand (theta at bits 0, 20, 53, 80; the retry from prec 20; the witness NOT_DETERMINED; four DOMAIN; LIMIT
   for `Re(A) = 10^-8`), all 11 equal on the first run. `tests/julia/poisson.jl`: the design's ccall, 13 of 13.
E. `docs/api-4c.md` "Slice 4g": statuses and order, G1 (the two sums, Prop. 7), G2 (the tail bound and the
   certified ratio, with the monotonicity argument for parameter balls), G3 (the search, the certificate and why
   the tail on both radii encloses the full sum; the promise on the cutoffs), G4 (width rule, retries, the stall
   rule), G5 (independence), G6 (the theta witness); decisions; cost; "Check:" lines.
F. Faults and mutation testing, below.

## Checks run at the end

- `sh lanes/f4-slice6/variants.sh` (2 jobs, build directories under the lane directory, removed afterwards):
  plain: test_poisson exit 0, 47835 checks; test_tensor exit 0, 149857 checks. SAN=1 with
  `ASAN_OPTIONS=detect_leaks=1`: 47835 and 149857, exit 0. INV=1: 47850 and 149891, exit 0. CC=clang: 47835 and
  149857, exit 0.
- `sh tests/test_driver.sh`: 91 cases, 101578 expected lines, all equal.
- `sh tests/test_exports.sh`: 599 of 599 declared functions exported. `sh tests/test_julia.sh`: passed (with the
  libgmp `LD_PRELOAD` fallback), `tensor poisson` 13 of 13.
- Not run: `check-all` (as the brief says).

## Fault table (`plant_faults.py`; `faults.log` for 1-20 and 22 (numbered 21 there), `faults-survivor.log` for 21)

| # | Fault | Caught by |
| --- | --- | --- |
| 1 | faults_45: tail bound without the factor 2 of Lemma 6 | vectors, left contains the oracle ball |
| 2 | faults_45: reverse only the real transform sign (`phihat(-n/M)`) | vectors, right contains the oracle ball |
| 3 | faults_45: `q = 0` (`n = 0`) omitted on both sides | vectors, left contains |
| 4 | faults_45: right spacing `1/D` | vectors, right contains |
| 5 | faults_45: omitted `n > N` bounded by `S(N + 1)` | vectors, left contains |
| 6 | faults_45: `beta` dropped from the shifted tail bound | vectors, left contains |
| 7 | the ratio `rho` not certified (geometric bound at `K = N + 1`) | vectors, status OK |
| 8 | the tail added to the real coordinate only | vectors, left contains |
| 9 | right derived from left | vectors, right contains |
| 10 | the width checked before the tail is added | witness, width after the tail |
| 11 | `NL` reported as the number of terms | vectors, `NL` equals the oracle |
| 12 | the retry not raising the precision beyond 64 | theta at bits 100 from prec 20 (added test) |
| 13 | outputs written on NOT_DETERMINED | sentinel bytes |
| 14 | left lattice `j/D + D n` | vectors, left contains |
| 15 | bits cap one short | bits `2^21` OK |
| 16 | right tail with `|g[0]|` for `max_k |g_k|` | `f = [1, -1]` at (1, 2) (added test) |
| 17 | left tail with `j = 0` only | vectors, left contains |
| 18 | Taylor shift dropped | the radius window |
| 19 | search from `N = 1` | vectors, `NL` equals the oracle |
| 20 | goal `epsilon/4` | vectors, `NL` equals the oracle |
| 21 | terms cap not in the preflight (mutation survivor) | `2^16 + 1` terms with bits -1: LIMIT (added test) |
| 22 | stall rule without the halving | witness under 1 s (added test) |

22 of 22 caught (16 and 22 only after the added tests, 21 is a mutation survivor turned into a test).

## Mutation testing

`python3 tools/mutate/mutate.py --files src/poisson.c --limit 60 --seed 20261008 --jobs 2 --san --timeout 300
--scratch <scratchpad> --make "make -s -j2 INV=1 SAN=1 BUILD=lanes/f4-slice6/mbuild
lanes/f4-slice6/mbuild/test_poisson lanes/f4-slice6/mbuild/test_tensor && ASAN_OPTIONS=detect_leaks=1 timeout 300
./lanes/f4-slice6/mbuild/test_poisson && ASAN_OPTIONS=detect_leaks=1 timeout 300
./lanes/f4-slice6/mbuild/test_tensor" --copy Makefile include src tests lanes/f4-slice6/mbuild`.

| Run | Time | Killed | Survived | Not compiled |
| --- | --- | --- | --- | --- |
| 1 (`mutate-run1.log`) | 517 s | 40 | 17 | 3 |
| 2 (`mutate-run2.log`, after the new tests) | 494 s | 45 | 12 | 3 |

After run 2 the survivor 95 got a test (planted fault 21, caught); no third run (time). The other 11, one line
each (line numbers of `src/poisson.c` as in the logs):
- 356 `arb_add(E, B, E)`: equivalent, addition commutes.
- 249 `arb_mul(hp, h, hp)`: equivalent, multiplication commutes.
- 359 `mag_zero(arb_radref(a))` dropped: equivalent, `a` was initialised to the exact 0 and never widened.
- 386 `st = ADF_LIMIT` dropped, 384 `2N + 0`, 384 `W - w` to `W + w` (the evaluation preflight of a search step):
  the status is the same, since the charges of the evaluation give LIMIT later; only wasted work differs.
- 417 `nnz = 1`: the evaluation preflight counts one lattice more; differs only at the exact work boundary.
- 76 one unit for a zero-length term in the transform charge: differs only at the exact work boundary.
- 132 and 134 (the prefix preflight's lower bound loosened by 1/2 or by `N + 1`, so it refuses slightly earlier):
  differ only when the first certified `K` lies within `N + 2` of the remaining budget; no cheap OK case exists
  there (the next search steps exceed the budget anyway).
- 561 `ADF_REAL_PREC_MAX * 2`: the last doubling could pass the cap (then LIMIT from the callee instead of
  NOT_DETERMINED); reaching it needs a width that halves at every doubling up to `2^20` bits, a computation of
  minutes; not tested.
Not compiled: 3 (`&&` to `||` inside a mixed condition, `-Werror=parentheses`). No entry was added to
`tools/mutate/equivalent.txt`.

## Findings against the design, the oracle and the proofs

1. Design P1 step 2 (api-4.md:310-312): the start `2 alpha K >= j + beta + log 2` is an upper bound of the prefix
   work; used as the refusal it would refuse calls that finish within the budget (it exceeds the first certified
   `K` by up to `j/(2 alpha)`). The code refuses on a lower bound (`alpha (2K + 1) >= beta + log 2`) and charges
   every iteration; the design's sentence "not permission to omit the prefix" holds.
2. Design P1 step 4 (api-4.md:318-320) says to raise the precision "geometrically from p to the cap"; with input
   radii (step 5, the theta witness) that means about 14 doublings to `2^21` bits per call (planted fault 22 shows
   the cost). The design is silent on telling the input radii from rounding; the stall rule of G4 is a decision.
   It only turns a later NOT_DETERMINED (or, at the border, a later OK) into an earlier NOT_DETERMINED.
3. Oracle `poisson_sides_ball` (proto/functions4_checks.py:295-317): correct, but its balls carry its own tail
   (6.12e-111 at its cutoff for 2^-230) in the imaginary part, wider than the code's certified tail at bits 128
   (6.117e-111), so containment of the oracle ball failed although both enclose the value. Not a defect of the
   oracle; the vectors use 2^-460 tails. Its `choose_cutoffs` agrees with the code in all 200 calls.
4. Analysis Lemma 6 and Proposition 7: no counterexample. The test checks the code's tail against the oracle's
   `lattice_bound` within `1 + 2^-40` in 351 windows, so the two transcriptions of Lemma 6 agree.
5. The design's sentence on lists (api-4.md:333-335) needs a caller-owned list of pairs that no header declares
   (D2); `adf_tensor_poisson_list` is not added.
6. Header (mine): `left == right`, `NL == NR` and `NULL` cutoffs are INV preconditions, not statuses; the INV
   message reads "argument right is not a canonical output distinct from left" (the format of `adf_inv_fail`).

## Sources pending

None new. Lemma 6, Proposition 7 (docs/proofs/analysis.md:213-294), acb_poly.rst:391-395, arb.rst:6-12 and
:435-443, acb.rst:142-149 and :290-292 are on disk and cited. The analytic imports of analysis Definition 1
(analysis.md:30-66) remain pending as before.

## Not done

- `adf_tensor_poisson_list` (finding 5).
- A third full mutation run after the last test; survivor 561 (a megabit computation) untested.
- Avoidable cost (noted in api-4c.md): each search step recomputes the Taylor shifts and `alpha`, `beta`, `gamma`
  of every lattice, which do not depend on `N`; a retry recomputes the left tails.
