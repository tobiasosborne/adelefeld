# Lane f-slice1: result (milestone 1F.3, first slice: `adf_lball`)

Date 2026-09-29. Worktree at f0a23fc plus my files. I ran no git command that changes state and no `bd`.

## What was built (28 functions, all exported)

`include/adelefeld/lball.h` (new, included from `include/adelefeld.h`), `src/lball.c` (new). The struct is the one
of `docs/conventions.md` 5.8 (48 bytes, alignment 8: p at 0, u = fmpq at 8, v at 24, N at 32, exact at 40).

- Life cycle: `init clear set swap is_canonical identical`; layout `adf_sizeof_lball`, `adf_alignof_lball`.
- Constructors: `set_rat` (exact), `set_rat_ball` (`c + p^N Z_p`), `set_fball` (projection of a finite ball to a prime,
  global or local backend).
- Accessors: `place is_exact contains_zero get_prec get_center`.
- Predicates: `equal_set overlaps contains` (no power of p is formed: they compare valuations of differences).
- Arithmetic: `neg add sub mul inv div`, each the SMALLEST ball containing the set of results; two exact operands
  give the exact result. `div` is not in the brief; it is `x (1/y)`, tight by L4 and L3.
- `valuation(v, is_inf, x)`, `abs(a, x)` (exact rational `p^-v`), `decompose(m, unit, x)` (`x = p^m unit`, the unit
  is a ball of relative precision `N - v`, or the exact unit).

Every arithmetic function computes into a temporary and swaps on `ADF_OK`, so aliasing is free and every non-OK
status leaves the outputs untouched. Statuses: `DOMAIN` (two primes, archimedean place, decomposition of the exact
0), `NOT_UNIT` (inverse of the exact 0), `UNIT_NOT_CERTIFIED` (inverse of a ball containing 0), `NOT_DETERMINED`
(valuation, abs, decompose of a ball containing 0), `LIMIT` (see decision 3).

The statements the header cites are proved stepwise in `docs/api-1f.md`, section "Statements to add to
functions.md": L0 (reduction to the canonical centre), L1 (projection of a finite ball), L2 (sum), L3 (product;
exponent `K = min(v(c) + M, v(d) + N, N + M)`, the infinite terms left out), L4 (inverse: `1/c + p^(N - 2 v(c))`),
L5 (negation), L6 (decomposition of a ball), L7 (valuation, absolute value), L8 (predicates). The same file lists
the functions and the decisions. Its prose is reflowed to 116 columns; table rows are longer.

## Files written (all under "You own")

`include/adelefeld/lball.h`, `src/lball.c`, `tests/test_lball.c`, `tests/julia/lball.jl`, `docs/api-1f.md`,
`proto/functions_checks.py` (a section at the end, existing checks untouched; I changed the last two lines of the
`if __name__` guard to call `lball_main()` after `main()`), `tests/ref/vectors/f-slice1/` (4 files, 23695 lines,
4.8 MB), `include/adelefeld.h` (2 lines: the include and its entry in the comment), `tests/test_julia.sh` (a call of
`lball.jl` after `smoke.jl` on both success paths), and in `lanes/f-slice1/`: `gen_vectors.py`, `bite.py`,
`bite.log`, `redgreen.log`, `progress.md`, `check-all.log`, `check-san.log`, `check-clang.log`, this file.

## Decisions (alternatives)

1. Orchestrator's two, followed: arithmetic on `fmpz`/`fmpq`, not FLINT `padic`; smallest ball, proved (L2 to L5) and
   tested by enumeration.
2. Statuses (alternative for the inverse of a ball around 0: `NOT_DETERMINED`; I took `UNIT_NOT_CERTIFIED` because
   conventions 3.1 and SPEC 4.5 use it for "the enclosure does not prove invertibility"). The exact 0: `NOT_UNIT`
   for the inverse, `DOMAIN` for the decomposition (functions.md Proposition 4 "Zero has no such decomposition"), and
   the valuation of the exact 0 is reported as infinity through `*is_inf` (SPEC 9.3.6 "an integer or infinity");
   alternative: a status for it.
3. Limits (conventions 5.8 is silent; the struct admits any `slong`): `ADF_LBALL_EXP_MAX = 2^60` on `|v|`, `|N|` of
   inputs and results, so no exponent sum (largest `N - 2 v`, `3 * 2^60`) overflows; `ADF_LBALL_BITS_MAX = 2^26` on a
   power of `p` that is formed. Beyond: `ADF_LIMIT` before any allocation that grows with the request. Alternative:
   no bound and abort on out of memory. A ball such as `1 + O(5^(2^60))` is still stored (u = 1, decided by bit length,
   no power formed); `-1 + O(5^(2^40))` is `LIMIT` because its centre needs the residue.
4. Two operands at different primes: `DOMAIN` (not a silent 0); predicates return 0.
5. `is_canonical` compares `u < p^(N - v)` by bit lengths and forms the power only when it is smaller than `u^2`.
6. Sum and product avoid `p^v`: the sum drops an operand of valuation at least `K` (proved in L2) and scales the
   others by `p^(v - m)`, the product multiplies unit parts and adds valuations.
7. `div` added (see above). The `w u` split of Proposition 4 (Teichmueller factor and principal unit) is NOT in
   `decompose`; only `p^m` times the unit.
8. Debug build: `src/lball.c` has its own entry check macro under `ADF_CHECK_INVARIANTS` (I do not own
   `src/invariants.h`); `make build/test_lball INV=1` runs green (20 tests, 3938748 checks).

## Red and green (`lanes/f-slice1/redgreen.log`)

- RED 1: `make -j2 build/test_lball` without `src/lball.c`: link error, `undefined reference` for all 28 functions.
  (Two `-Werror` errors in the test file itself came first and were repaired.)
- RED 2: a stub `src/lball.c` (init and clear do nothing, predicates 0, arithmetic returns `ADF_UNSUPPORTED`):
  `20 tests, 947996 checks, 103721 failed checks, 20 failed tests`; every test fails by assertion.
- GREEN: the real `src/lball.c`. One failing check on the first run was a bug of the test (a forged `u = 2^201 + 1`
  is divisible by 3). After the repair: `20 tests, 3937477 checks, 0 failed checks, 0 failed tests`. Later I added
  the checks of the dropped operand (below) and made the sampling order deterministic: final
  `20 tests, 3938748 checks, 0 failed checks, 0 failed tests`, the same under gcc, clang and the sanitizers.

## What the tests check (and what would make a case fail)

`tests/test_lball.c`, 20 tests, about 3.9 million checks, 0.3 s.
- Vectors (`tests/ref/vectors/f-slice1/`, made by `lanes/f-slice1/gen_vectors.py` from the Python reference at the
  end of `proto/functions_checks.py`; a second run reproduces the files byte for byte): 2185 lines of constructors
  (`set_rat`, `set_rat_ball`, `set_fball`; the field-by-field result), 4520 binary (add, sub, mul, div: status,
  result, outputs untouched on a status), 6187 unary (neg, inv, valuation, abs, decompose), 10803 predicates.
  Primes 2, 3, 5, 7, 11 and `2^64 - 59`; valuations -3 to 3, relative precision 1 to 4; all pairs of a set of special
  values (exact 0, balls around 0, exact powers of p) at p = 2, 3; centres of thousands of bits (p = 5 at relative
  precision 900 to 1500, p = 3 at 1500 to 2200, p = 2 at 3000 to 4000, `2^64 - 59` at 40 to 60). A wrong exponent
  rule or centre reduction fails the line that has it; a status vector fails when the status or the untouched output
  is wrong.
- Enumeration written in C, independent of the reference: for every pair of a universe and every choice of one point
  in each operand (`c + p^N a`, `a` in `[0, p)`), the exact result lies in the result ball (`contains`), and the p
  classes of the results modulo `p^(K + 1)` are all present (no ball of exponent `K + 1` or more contains them:
  tight). p = 2: 57 balls, all 12996 binary cases and 114 unary; p = 3: 38 balls, all 5776 and 76; p = 5: 86 balls, 16000
  sampled pairs and 172 unary; p = 7: 158 balls, 8000 sampled pairs and 316 unary. The universe has valuations -2..2
  (p = 2) or -1..1, relative precision up to 3 (p = 2) or 2, balls around 0, exact 0, exact `+-p^v`, an exact non-integer
  unit.
- `is_canonical`: 7 good values and about 30 forged ones, one clause each (composite p, p = 0, 1, exact not 0 or 1, 2/4,
  exact with N != 0, p dividing numerator or denominator, ball around 0 with v != 0, u not an integer, v = N, v > N,
  p | u, u < 0, u = p^k + 1, u >= p^k, extreme `slong` exponents, a 200-bit u against `3^100` and `3^207`).
- Statuses with the state of every output (`statuses_and_untouched_outputs`); aliasing of every binary function in
  the three combinations `(x, x, y)`, `(y, x, y)`, `(x, x, x)` at OK and at a status, and of neg, inv, decompose
  (1102 cases); decomposition round trip; the prime `2^64 - 59`; a ball of relative precision 3000 (about 7000
  bits): `x * (1/x)` is `1 + O(5^3000)`; limits and extreme exponents (`1 + O(5^(2^60))` stored, `2^60 + 1` and the
  bit bound give `LIMIT`, an exact `5^(2^40)` in a sum with an operand of valuation at least `K` is dropped and
  needs no power); the projection of a global and of a local fball (contexts of blocks 4, 9, 5) at 5 primes.
- `tests/julia/lball.jl` (30 tests, run by `tests/test_julia.sh`, which now runs it after `smoke.jl`): `1/3` at p = 5
  to precision 10 (checked with BigInt: `3 u = 1 mod 5^10`), its square, its inverse (3), `x * (1/x) = 1 + O(5^10)`,
  a sum at the coarser precision 3, valuation 2 and unit part of `50/3 + O(5^7)`, `|x|_5 = 1/25`, the statuses
  `UNIT_NOT_CERTIFIED`, `NOT_UNIT`, `NOT_DETERMINED`, `DOMAIN` (two primes, archimedean place), `LIMIT`, the prime
  `2^64 - 59`.
- Python (`python3 -B proto/functions_checks.py`, 9 s, exit 0; the 20 old checks pass, 28 of 28 planted wrong rules
  still rejected): `check_lball_reduction` 3000, `check_lball_projection` 480, `check_lball_enumeration`
  (add 6929, sub 6929, mul 6929, div 6929, neg 325, inv 325 by the same points-modulo-`p^(K+1)` method, plus 800 pairs
  repeated with two digits of `a` to check the remark that one digit suffices), `check_lball_decompose` 185. I planted
  two faults in the reference (add with max, product exponent + 1): both rejected. At `2^64 - 59` the generator can
  only check a sample of points for enclosure, not tightness; the tightness at that prime rests on L2 to L5.

## The tests bite (`lanes/f-slice1/bite.py`, `bite.log`)

13 faults planted one at a time in a scratch copy of `src/lball.c` under `build/bite/`, each against the full test:
mul exponent `min(N, N')` without the valuations (caught by `vectors_binary`, `enumeration_p2` ...); mul without the
`N + N'` term (5 tests); add exponent max instead of min; inverse exponent `N - v` instead of `N - 2 v`; sub as add;
add never drops the second operand (caught only after I added the limit check with the exact `5^(2^40)`: first run
12 of 13, then 13 of 13); `contains` without `N >= N'`; `is_canonical` without `p | u`; valuation of a ball around 0
answered; inverse of a ball around 0 as `NOT_UNIT`; the integer shortcut for a negative centre; decompose with
precision `N`; mul writing its output on a status. Result: `13 faults, 13 caught, survivors: []`. This is not a
mutation run (the brief excludes it).

## Checks of the brief (last line of each, all run after the last change to a file of the tree)

- `make clean && make check-all` (single job; 4 min 48 s): `check-all passed: make check, driver, exports, julia,
  mutate-selftest, memcheck-selftest`; inside it `check passed: all 58 test programs`, `test_exports: passed: 290 of 290
  declared functions are exported ... no variadic function`, `test_julia: passed (with LD_PRELOAD=...)`.
- `make clean && make -j2 check SAN=1` (5 min 9 s): `check passed: all 58 test programs`; no `runtime error`,
  `AddressSanitizer` or `LeakSanitizer` line in the log.
- `make clean && make -j2 check CC=clang` (2 min 50 s): `check passed: all 58 test programs`.
- `sh lanes/m1-headers/check_headers.sh`: `check_headers: passed` (48 single-header compilations, lball.h among them,
  0 failures; g++ C++17 compiles it; `lball.h` declares 28 functions, total 290).
- Note: a first `make -j2 check-all` failed at its last but one step, `python3 tools/mutate/selftest.py` ("34 mutants
  survived"); the same script run alone passed (exit 0, 43 killed, 0 survived), and `make check-all` without `-j2`
  passed twice. I did not find the cause (the run overlapped my editing of docs; a load or timing effect on the
  self-test is my guess, not established). The brief's `make check-all` is single job.
- Times exceed the 3 minute guidance because the whole suite of 58 programs and the self-tests is that long; my own
  tests take 0.3 s and every command of mine ran under `timeout`.

## Not done

- No mutation run and no fuzz target (brief item 5). The 13 planted faults are not a substitute.
- `w u` split of Proposition 4, the fractional part `{x}_p`, `exp`, `log`, roots, `adf_sball`, text and dump forms:
  outside the slice.
- No test of the local backend beyond one context (blocks 4, 9, 5) in `set_fball`.
- No benchmark. Avoidable costs seen: `lb_make` takes `fmpz_remove` twice and forms `p^k` and one modular inverse for
  every non-integer result; the sum forms `p^(v - m)`; `n_is_prime` on every `is_canonical` and `adf_lball_place`;
  `mul` reduces a product of two `k`-digit units although only `k` digits are needed (a truncated multiplication);
  `div` computes the inverse and the product, two reductions.
- The vector files are 4.8 MB; a smaller sample would do for the repository.

## Findings against the specification and the conventions

1. `conventions.md` 5.8 gives no bound on `v` and `N`; the struct admits 2^63 digits. The limits above are my
   decision (please confirm or replace; they are named in the header).
2. `conventions.md` 3.2 has no row for `adf_lball`. I used the row "Adele to idele; inversion of an adele-like value"
   for `inv`/`div` (`OK`, `UNIT_NOT_CERTIFIED`, `NOT_UNIT`, `NOT_DETERMINED`), "Functions at places" for valuation,
   abs, decomposition, and `DOMAIN` for two different primes (3.1: a stated compatibility requirement). A row should be
   added.
3. `functions.md` has no statement for the sum, product, inverse of balls, the projection, the decomposition of a ball
   (only of a point); written as L0 to L8 in `docs/api-1f.md`, to be merged into `functions.md` by the orchestrator.
4. SPEC 9.3.6 says the valuation is "an integer or infinity" and Proposition 4 says zero has no decomposition: not a
   contradiction, but the two functions answer the exact 0 differently (infinity through `*is_inf`; `DOMAIN`).
5. The brief calls `proto/functions_checks.py` a file with an existing part; it exists (955 lines, cited by
   `functions.md`). As the d-functions notes said, I kept it and appended; part 2 of the d-functions worktree was not
   used.

## Next slice proposed

1F.3-b: the `w u` split of `decompose` (Teichmueller factor at odd p, sign at 2, principal unit; determined exactly
when the relative precision is at least `c`, `functions.md` Proposition 4) and the p-primary fractional part
`{x}_p` and the unit part accessors, with the same enumeration method (they complete WP 1F.3 and are needed by
Hilbert symbols, 1F.9). After it, `adf_sball` (1F.1) on top of `adf_lball`, since `adf_rootlist` already gives roots at
a prime whose third accessor (S-D14) is this type.
