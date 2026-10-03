# f-slice10 result: the first all-places forms (1F.8): the root at all places; the five series on a finite part 0

Worktree `/home/tobias/Projects/adelefeld/.claude/worktrees/agent-af025eba4375b1a69`, at commit 86d9be4. No git
command that changes state was run, and no `bd`. `refs/src` was created as a symlink to the main checkout's
`refs/src`. Work was built in `lanes/f-slice10/build`; the final checks ran in `build/` and `build/san`. Times are
from `date` (CEST): start 22:55, slice A finished 23:15, slice B green 23:24, mutation 23:30-23:31, check-all
23:32-23:38.

Both slices are complete: library, tests (red, then green), driver, README, Julia, statements, planted faults. The
mutation run covers the whole of `src/gfunc.c`.

## Functions built

| Level | Function | What it computes |
|---|---|---|
| library (`include/adelefeld/gfunc.h`, `src/gfunc.c`) | `adf_rat_root(y, where, a, n, sign)` | the rational `n`-th root of an exact rational on the branch `sign`, found by two integer root tests; `DOMAIN` when there is none |
| library | `adf_adele_root(y, where, x, n, sign, prec)` | `(J ; rho)`: `J` = `adf_real_root` of the real ball (negated for the branch -1), `rho` = the rational root of the exact finite part |
| library | `adf_idele_root(y, where, x, n, sign, prec)` | exact unit `[c]`: the rational contract on `c r`, result `(J, |rho|, [sign rho])`; unit of finite precision: `NOT_DETERMINED` |
| library | `adf_adele_exp`, `_sin`, `_sinh`, `_cos`, `_cosh` `(y, where, x, prec)` | finite part exactly 0: `(f of the real ball ; exact f(0))`; exact `q != 0`: `DOMAIN` at the first prime outside `p^c Z_p`; a ball: `NOT_DETERMINED` |
| driver (`tools/adf/adf.c`, README) | `root X with N [with SIGN]` | X a rational, adele or idele; the first command of the driver with two or three operands (new arity code) |
| driver | `exp X`, `sin X`, `sinh X`, `cos X`, `cosh X` | X a rational (as `(q ; q)`) or an adele |
| Julia | `tests/julia/gfunc.jl` (two lines in `tests/test_julia.sh`) | 8 has the cube root 2; 2 has no square root (no place named); 1 has the square roots 1 and -1; -4: DOMAIN at the real place; `(4 ; 4 * [1])` has the root `(2 ; 2 * [1])`; `(4 ; 4 * [1 mod 8])` is NOT_DETERMINED; `exp (1/2 ; 0)` = `(1.648721271 +/- 3e-10 ; 1)` (checked against BigFloat `exp(0.5)`); `cos (0 ; 0)` = `(1 ; 1)`; `exp (1 ; 1)` DOMAIN at 2. 11 of 11 pass |

Files written: `include/adelefeld/gfunc.h`, `src/gfunc.c`, `tests/test_gfunc.c`, `tests/julia/gfunc.jl`,
`proto/gfunc_checks.py`, `tests/ref/vectors/f-slice10/` (4 files, 933874 bytes), `docs/api-1f8.md`,
`tools/adf/adf.c`, `tools/adf/README.md`, `tests/driver/gfunc-root.cmd/.out`, `tests/driver/gfunc-series.cmd/.out`,
two lines (one include, one comment) in `include/adelefeld.h`, two lines in `tests/test_julia.sh`; in the lane
directory `progress.md`, `redgreen.log`, `faults.py`, `faults-A.log`, `faults-B.log`, `julia.sh`, `mutate.log`,
`check-all.log`, `san.log`, this file. The names `gfunc.*` were free.

## Decisions as implemented (proposed N-D16; also in `docs/api-1f8.md`, "Interface and decisions")

1. **Types.** `adf_rat`, `adf_adele` (exact finite part, any real ball), `adf_idele`. All functions take a report
   `where` (conventions 2.2 order: outputs, report, inputs, prec; `where` may be NULL). Alternative: the rational
   alone (no adele form); the brief chose all three.
2. **The sign.** `int sign`, `+1` / `-1` as in the brief. For odd `n` the value `-1` is `DOMAIN` (an invalid
   selector, as `lroot.h` treats a seed that names no branch: "An invalid seed is DOMAIN as an invalid selector,
   even when other branches exist"); any other value is `DOMAIN` for `n >= 2`. **Mine where the brief is silent:**
   the exact 0 of the type (the rational 0; the adele whose real ball is exactly 0 and finite part exactly 0)
   accepts either sign for any `n` and returns 0 (the brief: "Exact 0: the root 0 for either sign"); an idele is
   never 0, so odd `n` with `-1` is always `DOMAIN` for it. Degree 1 ignores `sign` entirely, as `lroot.h` ignores
   the seed for degree 1, so `adf_rat_root(a, 1, 7)` is `OK`. Alternative: degree 1 also requires a valid sign.
   Order of checks: `LIMIT` (prec), `n = 0`, `n = 1`, the sign, then the values.
3. **No rational root: `DOMAIN`.** `where` = the real place when the real coordinate fails (even `n` and a negative
   rational, or a real ball certified negative); untouched when only the finite part fails. Alternatives:
   `NO_SOLUTION`; a search for the first failing prime (needs a factorisation in the worst case).
4. **Finite part not exactly a rational** (positive radius or local backend), `n >= 2`: `NOT_DETERMINED`, `where`
   untouched, no prime of the modulus examined (header example: `2 + 4 Zhat`, `n = 2`, where a look at 2 would prove
   `DOMAIN`). `n = 1` is the identity for every input (an exact copy; the real ball is not rounded to `prec`). `n =
   0`: `DOMAIN`, `where` untouched (as `adf_real_root`, `lroot.h`).
5. **Ideles.** Unit of modulus `N >= 1` and `n >= 2`: `NOT_DETERMINED`. Exact unit: the rational contract (G4). This
   is not false nor ugly with the representation: the finite part of `(X, r, [c])` is the single rational `c r`. The
   real root must exclude 0 for the result to be an idele; if arb's ball does not, `NOT_DETERMINED` at the real
   place.
6. **Combined status**: conventions 3.3, the maximum; the real place is reported when the real status is the
   maximum. Consequences: `(I ; q)` with `I` negative, even `n`, `q` a perfect power: `DOMAIN` at the real place; an
   idele with a negative real ball and even `n` is `DOMAIN` at the real place even when its unit is inexact.
7. **Series, finite part**: the exact constant 1 (`exp`, `cos`, `cosh`) or 0 (`sin`, `sinh`), as N-D12 at a prime.
8. **Series, exact `q != 0`**: `DOMAIN` with `where` = the first prime `p` with `v_p(q) < c`: 2 when 4 does not
   divide the numerator, else the first odd prime that does not divide the numerator (G6: at most `floor(log_3
   |num|) + 1` odd primes are tried). Series on a ball or the local backend: `NOT_DETERMINED`, `where` untouched.
9. **Series, real part (mine)**: through `adf_sball_exp_at` ... at the real place for all five (one path; `rfunc.h`
   has no `adf_real_sinh`, `adf_real_cosh`). A real `NOT_DETERMINED` (exp of `2^1000`, an infinite ball) is reported
   at the real place unless the finite part is `DOMAIN` (the larger status, at its prime).
10. **Driver (mine)**: `root` takes the sign as an optional third operand, an integer that fits an `int` (passed on;
    the library answers `DOMAIN` for values other than +1, -1); the degree is 0 .. 2^64 - 1, else `DOMAIN`. Other
    types of X (finite ball, unit coset, class, complex adele): `DOMAIN`. The series commands read a rational `q` as
    `(q ; q)`, as the commands at places do. The place that the library reports is not printed (the driver prints
    the status only). Alternatives: a separate command for the negative root; printing the place.

## What is proved and what is not

`docs/api-1f8.md` proves, stepwise, from `functions.md` Propositions 4, 6, 12, 13, 14, 16, 22 (cited by line),
`api-1f5.md` R3 and `fmpz.rst:983-988`:

- **G1**: the rational root by two integer roots of numerator and denominator; `n >= bits(q)` excludes a root, so no
  degree above the bit length (or above `WORD_MAX`) reaches `fmpz_root`.
- **G2**: a non-zero rational has an `n`-th root at every FINITE place exactly when it has a rational one (the
  all-places version is Proposition 16; the finite version, needed because the adele's coordinates are independent,
  adds: `-r^n` with even `n` has no root in `Q_3`, since -1 is not a square modulo 3). So "no rational root" is a
  proved `DOMAIN` for the adele and the idele.
- **G3**: the adele root is a root of every point, all coordinates on one branch; statuses and the combination.
- **G4**: ideles: inexact unit `NOT_DETERMINED` (Proposition 16 steps 1-2); exact unit, the rational contract.
- **G5**: the five series on `(I ; 0)`, `DOMAIN` for an exact `q != 0`, `NOT_DETERMINED` never a false proof.
- **G6**: the first failing prime and the bound `floor(log_3 |A|) + 1` of the search.

Not proved here: the enclosure and radius of the real coordinate (that is `rfunc.h`'s, and arb's). Not done: `Log`
on ideles at all places (the next lane), `x^(e/n)` of an exact rational at all places, the per-place variant of SPEC
9.3.1. No source is pending.

## Tests: numbers and what would have made a case fail

Final run (`build/test_gfunc`): **10 tests, 1088693 checks, 0 failed checks, 0 failed tests**, 0.3 s. Red-green:
`lanes/f-slice10/redgreen.log`. Slice A: red 1 link error, red 2 stubs (130053 failed checks in 7 tests); the first
implementation had 19 failed checks, all from my test's assumptions (the exactness of `arb_root_ui(8, 3)`, which is
`1.99999999999999999989 +- 1.2e-19`; and the tight-radius bound at prec 2, finding 3), plus one test defect (big
integers are JSON strings). Slice B: red 1 link, red 2 stubs (11347 failed checks in 3 tests), green at the first
implementation.

The oracle `proto/gfunc_checks.py` uses no C code, FLINT or arb; a second run reproduces the four vector files byte
for byte (md5 compared).

| Test | Cases | A case fails on |
|---|---|---|
| `rat_root_vectors` | 5814 rows x 2 signs, 34884 calls; 1234 rows with `root^n = a` rechecked with `fmpq_pow_si` | a status, place or exact root other than the oracle's (Newton and the bisection of `lpow_checks.py:70` must agree; for `|A|, B <= 2^20` also the valuation criterion of Proposition 16); y written on a status; `where` written on OK or wrong; aliasing `y = a`. Rows: `n` 1..12, 15, 31, 64, degrees at and around the bit lengths, `WORD_MAX`, `WORD_MAX + 1`, `UWORD_MAX - 1`, `UWORD_MAX`; perfect powers and perfect powers +- 1 of bases up to 1000 bits (12 rows over 3000 bits) |
| `rat_root_named_cases` | 50 cases x 3 calls (the call, the aliased call, `where = NULL`) | the values of PLAN 1F.8 (1: `1`, `-1`; 8: `2`; 2: DOMAIN untouched), -4/2 DOMAIN real, -8/3 = -2, 0, `n = 0, 1`, invalid signs, `2^63` with `n = 63, 64`, `2^64` with 64, `1/2^63` with 63 |
| `adele_root_real_vectors` | 558 real rows x 5 finite parts x 2 signs, 11160 calls, 1062 OK results checked | the combined status or place (real status from the mpmath row, finite status by hand); the real part not containing the image `[lo, hi]` (800-bit mpmath intervals, exact integer roots); not identical to `adf_real_root`'s ball (negated for -1); radius above `4 (hi - lo) + 2^(5-p) max` on tight rows from 24 bits; finite part not the exact root; `n = 1` not an identical copy |
| `adele_root_rat_vectors` | 5814 rows x 2 forms x 2 signs, 46512 calls | `(q ; q)` disagreeing with `adf_rat_root` (status, place, root); the real ball missing the rational root; `(|q| + 1 ; q)` naming a place |
| `adele_root_statuses` | 28 cases x 2 calls (with aliasing) + 1 | LIMIT first (also before `n = 0`), prec -5 and 0, invalid signs, the exact zero adele, the header's examples, `2 + 4 Zhat` and `0 + 4 Zhat` NOT_DETERMINED untouched, local backend NOT_DETERMINED, `(-8 ; 8)` with `n = 3` |
| `idele_root_rat_vectors` | every non-zero rational row as its idele, 23208 calls | the row's status and place; content `|root|`, exact unit `[sign root]`, real ball identical to `adf_real_root`'s, containing the root and excluding 0; canonical result |
| `idele_root_statuses` | 25 cases x 2 calls (with aliasing) + 5 | inexact unit `n = 2, 3` NOT_DETERMINED; negative real ball with inexact unit DOMAIN at real for even `n`; exact units, `(-8, 8/27, [1])`; `(2^999 +- 2^960, 2^999, [1])` NOT_DETERMINED at real at prec 2 and OK at 64 |
| `series_real_vectors` | 327 rows (5 functions x 23 balls x 3 precisions, huge arguments for sin and cos only), 1962 calls | `(I ; 0)` not OK, real part not containing the mpmath image, not identical to the real-place function's ball, radius above `2 lip rad(I) + 2^(6-p) max(|lo|,|hi|,1)` from 24 bits; finite part not the exact constant; `(I ; 1)` not DOMAIN at 2; `(I ; 0 + 4 Zhat)` not NOT_DETERMINED untouched |
| `series_place_vectors` | 988 rationals x 5 functions, 9880 calls | another status or another prime than the oracle's (exact valuations, primes to 2000); numerators `4 * 3 * 5 * ... * p_k` up to `k = 59` |
| `series_statuses` | 5 functions x 21 cases + 4 hand values | the brief's finite parts 0, 1, 4, 12, 1/3, -9/2 (places 2, 3, 5, 2, 2), 60 (7), 4/9 (3); `0 + 4 Zhat`, `3 + 9 Zhat`, `1 + 4 Zhat`, local backend NOT_DETERMINED; LIMIT; prec -7, 0; `2^1000` real part (status taken from `rfunc.h`: NOT_DETERMINED for exp, sinh, cosh) combined with 0, 1, `0 + 4 Zhat`; `exp (1/2 ; 0)` containing 1.6487212707001281468 |
| driver `gfunc-root`, `gfunc-series` | 43 + 23 lines | any line different. Red on the HEAD driver: every line PARSE. 61 lines written by hand before the run; the 5 inexact series lines taken from the output and checked by hand against the constants (comment in the .cmd) |
| Julia `gfunc.jl` | 11 | the values listed above |

**Fuzzing:** none (no long differential run). Not done.

## Planted faults (`lanes/f-slice10/faults.py`, scratch copies in `lanes/f-slice10/build/faults/`)

| Fault | test_gfunc |
|---|---|
| A1 the sign of the even root taken for odd `n` | 1117 failed checks, 6 tests |
| A2 the denominator not tested | 18909 failed checks, 6 tests |
| A3 `n` above the bit length passed to `fmpz_root` | abort (FLINT exception, the degree cast to a negative `slong`), exit -6 |
| A4 the real part computed from the finite part | 11334 + 1906 failed checks, then abort (the fault's `arb_set_fmpq` at prec -5) |
| A5 the inexact idele of degree 2 returned as OK | 2 failed checks, 1 test (`idele_root_statuses`) |
| B1 the constant of `sin` exchanged with that of `cos` (1 instead of 0) | 77 failed checks, 3 tests |
| B2 the exact non-zero finite part accepted | 10599 failed checks, 3 tests |
| B3 the place of DOMAIN off by one prime | 2205 failed checks, 2 tests |
| B4 the 2-adic domain taken as `2 Z_2` | 615 failed checks, 1 test |
| B5 a finite ball read as its centre | 359 failed checks, 2 tests |

All ten are detected. A5 is detected by two checks only (the idele vectors are all exact units).

## Mutation testing (`src/gfunc.c`; `lanes/f-slice10/mutate.log`)

```
timeout 1300 python3 tools/mutate/mutate.py --files src/gfunc.c --limit 60 --seed 1 --jobs 2 --san --timeout 200
  --scratch <scratchpad>/mutate --copy Makefile include src tests lanes/f-slice10/build/san
  --make "make -s -j2 BUILD=lanes/f-slice10/build/san lanes/f-slice10/build/san/test_gfunc
          && lanes/f-slice10/build/san/test_gfunc"
```

The `--make` string does not contain "SAN", so `--san` put `SAN=1` into the environment (the log says "every mutant
is built and run with SAN=1"); the copied `build/san` objects are sanitized. Result: 242 mutants, 60 run in 106.9 s:
**48 killed, 3 survived, 9 not compiled** (the tool lists the three apart: 51 mutants compiled and ran), 0 timed
out. The 9 not compiled: 4 removed assignments that leave a variable uninitialised, 4 `*` -> `+` or `/` on a pointer
declaration, and `prec` -> `2` at :350 (an unused parameter then; all under `-Werror`).

Survivors (none is a gap of the tests):
- `:54` `s < 0` -> `s < 1` in `rat_root_core`: `s = fmpq_sgn(q)` is -1 or 1 there (0 returned before).
- `:101` `>` -> `>=` in `combine`: with equal statuses both branches give the same value.
- `:268` `n_nextprime(p, 1)` -> `n_nextprime(p, 0)`: a proved against a probable prime below 2^64; no test can tell
  them apart (not provably equivalent from the FLINT text, `ulong_extras.rst:688-692`).

Nothing was added to `tools/mutate/equivalent.txt`. Tool defects, as reported by f-slice9 (not repaired): the
`--san` / "SAN" substring rule; here avoided.

## Final checks

| Check | Last line |
|---|---|
| `timeout 900 make -j2 check-all` in `build/` (23:32-23:38, exit 0) | `check-all passed: make check, driver, exports, julia, mutate-selftest, memcheck-selftest` |
| `make SAN=1 BUILD=build/san build/san/test_gfunc`, run with `ASAN_OPTIONS=detect_leaks=1` | `10 tests, 1088693 checks, 0 failed checks, 0 failed tests` (no sanitizer report) |

Inside check-all: `check passed: all 75 test programs`; `test_driver: 51 cases, 100982 expected lines, all equal`;
`test_exports: passed: 432 of 432 declared functions are exported`; `test_julia: passed (with
LD_PRELOAD=...libgmp)`, `gfunc.jl` 4 of 4 and 7 of 7.

## Findings against the specification or the brief

1. **Conventions 3.3 rule 2 cannot be met for the finite part of the root.** "The reported place is the first place,
   in the canonical order of places, whose status equals the combined status." For `2` with `n = 2` it is 2; for 17
   it is 3; in general it needs the primes of numerator and denominator. `where` is left untouched (brief decision);
   the conventions (3.1 "Reported with the place, where there is one", 3.2 row "DOMAIN (with place)") do not name a
   value for "the finite part as a whole". Counterexample to "every DOMAIN of an all-places function carries a
   place": `adf_rat_root(2, 2)`.
2. **SPEC 9.3.3 line 637 and PLAN 1F.8 say "NOT_DETERMINED for ideles" without qualification.** For an idele with an
   exact unit (`(4 ; 4 * [1])`, the idele of the rational 4) the root `(2 ; 2 * [1])` is certified (G4), and for `(4
   ; 4 * [-1])` the absence is proved (DOMAIN). Also, with the maximum of conventions 3.3, an idele of finite
   precision with a negative real ball and even `n` is DOMAIN at the real place, not NOT_DETERMINED (`(-4 ; 4 * [1
   mod 8])`, `n = 2`). The text should say "an idele of finite precision" and "unless a coordinate is proved outside
   the domain".
3. **`rfunc.h` (not this lane's code): the real root at low precision.** `adf_real_root(2^1000 +- 2^960, 3, prec 2)`
   is `[4.48e102 +- 8.15e102]`, a ball that contains 0 although the input is positive (probe
   `lanes/f-slice10/build/probe.c`); at prec 4 still `[3.94e100 +- 4.38e100]`. The tight bound of
   `tests/test_rfunc.c` (`2 rad <= 4 (hi - lo) + 2^(5 - p) max`) fails for `2^1000 +- 2^960` and `2^-1000 +-
   2^-1040` at prec 2 with `n = 3, 7, 12` (16 checks of my first run); `rfunc.h` promises no radius, so this is no
   contract violation, but a caller at small `prec` gets a useless ball. My test asserts that bound from 24 bits on.
   Consequence: `adf_idele_root` of `(2^999 +- 2^960, 2^999, [1])`, `n = 3`, at prec 2 is NOT_DETERMINED (tested).
4. **Proposition 16 covers all places at once; the adele form needs the finite places alone.** An adele `(I ; q)`
   has independent coordinates, so "no rational root" must imply "no root in `A_f`" without the real place. That is
   true (G2: `-r^n`, even `n`, has no root in `Q_3`), but it is not what Proposition 16 states; G2 adds the step.
5. **The selector conventions differ between `lroot.h` and `gfunc.h` for the zero.** `lroot.h` requires the
   identifier 0 for the exact 0 and refuses other seeds; the brief asks that the exact 0 take either sign here. A
   user moving between the two will meet it.
6. **SPEC 9.3.1 line 573** ("an exact input does not give an exactly representable output"): the series at all
   places return the exact constants 1, 0, as N-D12 already does at a prime. The sentence should name the exception.

## Avoidable costs (COMMON-C rule 7; not optimised)

- `adf_adele_root` computes the real root even when the finite part already fails (needed only to decide `where`).
- The series build two partial balls and copy the real ball twice to reach the real-place function.
- `first_failing_prime` divides the whole numerator by each candidate prime; a numerator of `10^6` bits that is a
  multiple of the first ~56000 odd primes would cost about `56000` multi-word divisions.
- `rat_root_core` copies `|num|` into a temporary.

## For the next lane (`Log` on ideles; draft questions 8 and 9)

- `src/gfunc.c` has the pattern: a real part through `adf_sball_*_at` at the real place, a finite part decided
  apart, `combine` by the maximum with the real place first, and a finite failure that names a prime only when one
  can be found without factorisation. `Log` at all places can reuse it: the real coordinate is `adf_sball_Log_at` at
  the real place (domain `t > 0`; `log_abs` is the other function), the finite part the enclosure `4 Zhat`
  (Proposition 12 step 4).
- Question 8 (the local component of an idele at a prime `p`): there is still no function that gives it as an
  `adf_lball`; the hull of `idmap.h:53` is wrong for `p` not dividing `N` (it contains non-units). A new internal
  helper `r c + p^(v_p(r) + v_p(N)) Z_p`, and for `v_p(N) = 0` the image `p^c Z_p` of `Log` without a call, is the
  natural design; it needs a statement (the image of `Log` over a unit coset at a prime).
- Question 9 (the result type): an `adf_adele` `(log X ; 0 + 4 Zhat)` for the plain form is consistent with this
  lane (`adf_adele` outputs, `where` reports); the refined form at named primes is an `adf_sball`.
- The driver now has free names `Log`, `log`; the table accepts an arity code for two or three operands
  (`ADF_DRV_ARITY_2_OR_3`).
