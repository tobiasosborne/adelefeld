# f-slice9 result: rational powers and principal-unit powers at a prime (1F.6), 2026-10-02

Worktree `/home/user/adelefeld-wt/f-slice9`, branch `lane/f-slice9`. No git command that changes state was run, and
no `bd`. All work was built in `lanes/f-slice9/build`; the final checks ran in `build/` and `build/san`.
Times are from `date` (UTC): start 22:13, check-all 23:00 to 23:06, sanitizer run 23:07.

## Functions built

| Level | Function | What it computes |
|---|---|---|
| library (`include/adelefeld/lpow.h`, `src/lpow.c`) | `adf_lball_powrat(y, x, e, n, seed, N)` | `x^(e/n)`: the fraction is reduced to `e'/n'`. For `n' = 1` it is `adf_lball_pow_si(y, x, e')` exactly. For `n' >= 2` it takes the root of degree `n'` of `x` on the branch `seed` (`lroot.h` identifier), then the power `e'`. Ball result: exponent `min(N, E')` with `E' = e' j + (M - m) - v_p(n') + v_p(e')`. Exact input: exact when the root is rational, else a ball at `N` |
| library | `adf_lball_powunit(y, u, s, N)` | `u^s = exp(s log u)`; at 2 `w^(s mod 2) exp(s log u')`. `u` and `s` are local balls at one prime. Result exponent `min(N, R)` with `R = min(A + beta, B + alpha, A + B)` (Proposition 18). Where the 2-adic sign or parity is not fixed, the result is the hull `1 + 2 Z_2` |
| `_at` (`rfunc.h`, `src/rfunc.c`) | `adf_sball_powrat_at`, `adf_sball_powunit_at` | the same on the component at a named prime; built on the pattern of `adf_sball_root_seed_at` |
| driver (`tools/adf/adf.c`, `README.md`) | `powrat_at X with PRIME with E/N with SEED`, `powunit_at X with PRIME with S` | one line each; `E/N` is the value text of an exact rational; `S` is a value projected to the prime |
| Julia | `tests/julia/lpow.jl` (added to `tests/test_julia.sh`) | `9^(3/2)` at 5 on both branches (27 and -27). `(1 + 5)^(1/2)` gives `6520516 + O(5^10)`. `(1 + 5)^(1/2 + O(5^3))` gives `516 + O(5^4)`. `2^(1/2)` at 2 is `DOMAIN`. `(1 + 4 Z_2)^(1/2)` is `NOT_DETERMINED`. 7 of 7 pass |

Files written: `include/adelefeld/lpow.h` (new), `src/lpow.c` (new), `include/adelefeld/rfunc.h` and `src/rfunc.c`
(the two `_at` forms only), `include/adelefeld.h` (one include line, one comment line), `tests/test_lpow.c` (new),
`tests/test_rfunc_prime.c` (one test added), `tests/julia/lpow.jl` (new), `tests/test_julia.sh` (two lines),
`proto/lpow_checks.py` (new oracle), `tests/ref/vectors/f-slice9/` (3 files, 909424 bytes), `docs/api-1f6.md` (new),
`tools/adf/adf.c`, `tools/adf/README.md`, `tests/driver/pow-values.cmd/.out`, `tests/driver/pow-status.cmd/.out`,
`lanes/f-slice9/` (progress.md, redgreen.log, drv.sh, faults.sh, mutate logs, check-all.log, this file).

## Decisions and alternatives (for SPEC 15.4; they are also in `docs/api-1f6.md`, "Interface and decisions")

1. **Names.** `powrat` and `powunit` are used at every level. Rejected alternatives: `pow_q` and `pow_padic`.
2. **Exponent type.** The rational exponent is a pair: `slong e`, `ulong n`. Rejected alternative: an `fmpq`, which
   would reduce the fraction silently.
3. **Order of root and power.** The root comes first, then the power. `seed` is the `lroot.h` identifier of the
   root of degree `n'` of `x`. Alternative: the power first, with the identifier of the root of `x^e'`. For a reduced
   fraction both orders give the same set of values (P1).
4. **Degree `n' = 1`.** It is `pow_si` exactly: seed and N are ignored, and `pow_si`'s statuses are kept. So a ball
   containing 0 with `e' < 0` gives `UNIT_NOT_CERTIFIED`, and a ball result is not capped at `N`. Alternative:
   `min(N, E)` and `NOT_DETERMINED`. That would break the agreement with `pow_si` that the brief requires (finding 1).
5. **Exact results of `powrat`.** An exact `x` gives an exact result exactly when the root is rational. P1(b)
   proves there is no other exact case for a reduced fraction.
6. **Exponent of `powunit`.** `s` is a local ball at the same prime; the `_at` form takes `s` as a partial ball.
   Alternative: an exact exponent only. It could not carry the `B + alpha` term of Proposition 18.
7. **2-adic sign or parity not fixed.** The result is the smallest ball containing the union, `1 + 2 Z_2` (P5(e)).
   Where the union is one coset, the result is the exact image (P5(d)). Alternatives: `NOT_DETERMINED` or
   `NEEDS_SPLIT`. Both rejected: the domain is certain, and `pow_si` and `Log` already return a hull in the same
   situation.
8. **Exact `u` and exact `s`.** The result is the ball at `N`, also for an integer `s`. The exceptions are the exact
   1, and the exact `+-1` for `u = -1` at 2. Alternative: the exact `u^k` for an integer `s = k` (finding 4).
9. **Status order of `powunit`.** Different primes give DOMAIN, then input LIMIT, then DOMAIN if either coordinate
   is outside its domain, then NOT_DETERMINED.
10. **How `alpha` is computed.** `alpha = v(w0 u0 - 1)`, by the isometry of log, not by `Log`. The first version used
    `Log` to `min(A, N - B)` digits and returned LIMIT for an exact base at `N = 2^60`.
11. **Driver exponent.** `E/N` is the value text of an exact rational, so the fraction is already reduced. The seed
    is always required, as in `root_at`, and `-1` is accepted at 2. Alternative: the pair `E with N`.

## What is proved and what is not

`docs/api-1f6.md` proves these statements, stepwise, from `functions.md` Lemma 3, Proposition 4, Lemma 9,
Propositions 11, 13, 15, 17, 18 and 22 (cited by line), `api-1f.md` L3, L8, L12, `api-1f4.md` F1, F6 and
`api-1f5.md` R1 to R6:

- **P1:** the bijection between roots and powers for a reduced fraction; the counterexample for an unreduced one
  (4 at 5 with 2/2); rationality; degree 1 equals `pow_si`; the zero cases.
- **P2:** the exact image exponent `E'`, by R2 composed with L12, with a second derivation by exp and log.
- **P3:** the root precision `max(1, K - e' j - v_p(e'))` is enough.
- **P4:** the product domain and its statuses.
- **P5:** the values in all cases at 2. This includes the hull `1 + 2 Z_2`, which equals the image when `A = 1` and
  is strictly larger when `w0 = -1`, `B = 0` (the class `5 + 8 Z_2` is missed).
- **P6:** `alpha` by the isometry, and the working precisions.
- **P7:** what the oracle proves: dominance, enumeration with a finite exponent depth `H' = max(0, k - B)`,
  `k = max(H - 1, 1)`, scaling, and the existence test of a branch.
- **P8:** limits, transactions, aliasing, and the `_at` forms.

Not proved here and not done:
- the real place of either operation (UNSUPPORTED);
- the all-places forms;
- the quasi-character.

Sources pending: none new. The pending names in `functions.md` (Teichmueller representative, Iwasawa `Log`) stay
pending.

## Tests: numbers and what would have made a case fail

All counts are from the final run in `build/`. Red-green log: `lanes/f-slice9/redgreen.log`. In the red runs:

- `test_lpow`: first a link error (new file), then stubs gave 861185 failed checks.
- `_at` forms: 70 failed checks.
- driver: every line `PARSE`.
- First implementation: 3085 failed checks, from two real defects of the code and one wrong hand value in a test
  (the value is now derived in the comment).

| Test | Cases | A case fails on |
|---|---|---|
| `powrat_grid_balls` | 6930 oracle rows, 19152 OK branch rows in 4 scalings, 260496 calls | a status other than the oracle's, for any of the seeds 0..p; an exponent other than `min(N, E')`; a centre other than the oracle's gamma modulo `p^(K - e'j)`; a written output on a status; an aliased `y = x` result that differs. The oracle checks each image as a set modulo `p^(Erel+1)` (4788 witnesses) |
| `powrat_exact_rows` | 3024 rows; 1096 exact results, 1160 balls; 18090 calls | a ball for a rational branch or the reverse; a centre other than `root^e' mod p^(N - e'j)` (roots found by exhaustive enumeration); `r^n' != x^e'` for an exact result |
| `degree_one_is_pow_si_and_numerator_one_is_root` | 35360 calls | any field or status that differs from `pow_si` (`n' = 1`; zero, zero balls, `LONG_MIN`, `LONG_MAX`, unreduced fractions) or from `root_seed` (`1/n`, `2/2n`) |
| `powrat_zero_and_hand_values` | 27 calls + 4 | e.g. 9^(3/2) = 27 / -27 at 5 and at 2; 8^(2/3) = 4; `0^(-3/2)` NOT_UNIT; the seed checked before the sign |
| `powrat_large_prime_and_big_inputs` | 204 | at `p = 2^64 - 59` and `N` = 50 and 200: lost enclosure (`y^n'` must contain `x^e'`), or a unit residue other than `seed^e'`. 3000-bit inputs at 2 and 5 |
| `powrat_limits` | 13 | LIMIT where a value is due (`1 + 5^(2^60) Z_5` to the 1/2; `p^5 Z_7` for `v = 3*2^58`), or a value where LIMIT is due |
| `powunit_rows` | 4352 rows (305 exact, 276 exact-input balls, 3748 images, 23 hulls), 28721 calls | an exponent other than `min(N, R)`; a centre other than the enumerated set's centre; exact versus ball; a hull other than `1 + 2 Z_2`; aliasing `y = u`, `y = s` |
| `powunit_statuses_and_limits` | 31 + 3 | each domain case with its reason; DOMAIN takes precedence over NOT_DETERMINED; the prime check comes before LIMIT; no power for centre 1; `u = s` the same object |
| `powunit_identities` | 964 groups at p = 2, 3, 5, 7, 13, `2^64 - 59` | `u^(s+t) != u^s u^t` or `(uv)^s != u^s v^s` as sets at N = 30 (200); `u^k` not containing `pow_si(u, k)`, k = -5..5; `(u^n)^(1/n)` not containing `u`; at 3000 bits, `u^s u^(1-s)` not containing `u` at N = 40 |
| `powers_at_prime` (`test_rfunc_prime.c`) | 1 test | other components or fields; `where` not `v` on a failure, or written on OK; a written `y`; aliasing `y = x`, `y = s`, `x = s` |
| driver `pow-values`, `pow-status` | 25 + 20 lines | any line different from a value computed by hand or by integer enumeration (`6520516`, `379` = cube root of 3 mod 2^10) |
| Julia `lpow.jl` | 7 | the values above |

**Fuzzing:** none. No long differential run was made; this is not done.

## Planted faults (scratch copies in `lanes/f-slice9/build/faults`, `lanes/f-slice9/faults.sh`)

| Fault | test_lpow | test_rfunc_prime |
|---|---|---|
| F1: the fraction is not reduced (`g = 1`) | red; at least 15780 failed checks before the 300 s timeout (exit 124) | 0 failed |
| F2: the `A + B` term of `R` is missing | 117 failed checks | 0 |
| F3: the sign factor at 2 is ignored for odd `s` | 1104 | 0 |
| F4: `beta` is taken from the ball (`v = 0` for a centre-0 ball) instead of the centre | 609 | 0 |
| F5: the 2-adic hull has exponent 2 instead of 1 | 30 | 0 |
| F6: `v_p(e')` is missing from `E'` | 13552 | 0 |

All six faults are detected by `test_lpow`. `test_rfunc_prime` tests only the wrapper and detects none of them.

## Mutation testing (`src/lpow.c`; `lanes/f-slice9/mutate.log`)

Command:

```
python3 tools/mutate/mutate.py --files src/lpow.c --limit 60 --seed 1 --jobs 2 --san --timeout 150
    --copy Makefile include src tests lanes/f-slice9/build/san
    --make "make -s -j2 SAN=1 ... test_lpow test_rfunc_prime && run both"
```

The whole command was run under `timeout 1300`.

Result: 60 mutants in 472.6 s. 40 killed, 14 survived, 6 not compiled (unused-variable and parentheses warnings
under `-Werror`), 0 timed out.

No survivor is a gap in the tests. Each one has no effect on any value:

- `:37` clamp `a < -INF` -> `<=`: at `a = -INF` both return `-INF`.
- `:40` `min2` `<` -> `<=`: equal arguments give the same value.
- `:45` `e < 0` -> `e < 1`: for `e = 0`, `-(ulong) 0 = 0`.
- `:77:23` `j = 0` -> `1`: `j` is read only after it is assigned (divisible case).
- `:77:45` `Nr = 0` -> `1`: used only when `n'` does not divide `m`, where `root_seed` fails at any `N`.
- `:90` `x->v < 0` -> `< 1`: for `v = 0`, `-(ulong) 0 = 0`.
- `:144` `n_gcd(ae, n)` -> `n_gcd(n, ae)`: gcd is symmetric.
- `:146` `e < 0` -> `e < 1`: for `e = 0`, `-(slong) 0 = 0`.
- `:190` `sc->v` 0 -> 1 for a zero centre: `sc` is never read when `s0 = 0` (the shortcut at `:211`).
- `:191` `A >= INF` -> `>`: the else branch then gives `R = INF` and `K = Nc`, the same value.
- `:196` `A < INF` -> `<=`: with `A = INF` the term is `INF` and does not change the minimum.
- `:211` `K <= c` -> `< c`: at `K = c` the computed centre is also 1 modulo `p^c` (P6(b)); the shortcut only saves
  work.
- `:220` `sign < 0` -> `<= 0`: `sign` is always 1 or -1.
- `:259` the `K` argument of `signed_one` 0 -> 1: `K` is not read for an exact result.

Nothing was added to `tools/mutate/equivalent.txt`; that file is not mine.

**Defects of the mutation tool (not repaired):**

1. `--san` sets `SAN=1` only if the string "SAN" is absent from `--make`. "ASAN_OPTIONS" contains it, so the first
   run built a sanitized archive and linked without sanitizers. All 60 mutants were NOT COMPILED, and the tool
   still printed "passed: every mutant was killed" with exit 0 (`lanes/f-slice9/mutate-run1-notcompiled.log`).
2. Not-compiled mutants do not make a run fail.

## Final checks

| Check | Last line |
|---|---|
| `timeout 900 make -j2 check-all` in `build/` (23:00 to 23:06, exit 0) | `check-all passed: make check, driver, exports, julia, mutate-selftest, memcheck-selftest` |
| Sanitizer build `make SAN=1 BUILD=build/san build/san/test_lpow build/san/test_rfunc_prime`, run with `ASAN_OPTIONS=detect_leaks=1` | `9 tests, 2760383 checks, 0 failed checks, 0 failed tests` and `15 tests, 522217 checks, 0 failed checks, 0 failed tests` (no sanitizer report) |
| Extra: `INV=1` build of the same two tests in `lanes/f-slice9/build/inv` | the same two lines |
| Extra: `lpow.h` compiled alone (included twice) in C11 and in C++17 | compiles |

Inside check-all:
- make check: 74 test programs.
- test_driver: 49 cases, 100916 expected lines, all equal.
- test_exports: 424 of 424 declared functions exported.
- test_julia: passed (with `LD_PRELOAD` of the system libgmp, as the script does); `lpow.jl` 7 of 7.

## Findings against the specification and the brief

1. **The brief conflicts with itself for `n' = 1`.** It asks for `NOT_DETERMINED` "for a ball containing 0 with
   `e < 0`", and also for exact agreement with `adf_lball_pow_si` when `n = 1`. For `n' = 1` and `e' < 0`, `pow_si`
   returns `UNIT_NOT_CERTIFIED`. I chose `pow_si` (decision 4). `NOT_DETERMINED` applies for `n' >= 2`.
2. **Degree 1 does not follow `K = min(N, E)`.** Under N-D13/N-D14 the root of degree 1 ignores `N`, and so does
   `pow_si`. `x^(e/1)` of a ball is therefore the exact image whatever `N` is, unlike every other ball result of
   this family. This is not an error, but the cost of `n' = 1` is not bounded by `N`.
3. **SPEC 9.3.4 item 3 and Proposition 18 step 5** say "take the union of the applicable sign/parity images". That
   union is not always a ball: for `w0 = -1`, `B = 0` it misses `5 + 8 Z_2`. A ball result can only be the hull
   `1 + 2 Z_2` (P5(e)). The specification does not say which result is returned; I chose the hull (decision 7).
4. **SPEC 9.3.1** says an exact input "does not give an exactly representable output". `powunit` with an exact `u`
   and an exact integer `s` has a rational value (`6^2 = 36`) and returns it as a ball, as the brief asks. Whether
   integer exponents should be exact here is open (decision 8).
5. **Integer powers vs `powunit`.** The `pow` of SPEC 9.3.4 item 1 is `pow_si` and needs no domain, while `powunit`
   restricts `u` to `1 + p Z_p`. So `powunit(2, 1)` at 5 is DOMAIN although `2^1` exists. This is as Proposition 17
   says, but users should be told.

## Avoidable costs (COMMON-C rule 7; not optimised)

- `powrat` calls `root_seed` even when `K <= e' j`, only for the status and the rational test.
- `root_seed` repeats the rational-root test on each call.
- `powunit` copies both centres into temporaries.
- `powunit` adds the ball `p^K Z_p` to round an exact 1. A direct construction would do.
- `test_lpow` makes about 2.7 million checks in 8.6 s (12.9 s under ASan).

## Proposed next slice

**1F.8, the all-places forms.** This slice gives the local rational power. The rational-root contract of SPEC 9.3.3
(Proposition 16), and its extension to `x^(e/n)` of an exact rational at all places (the rational branch, by integer
root tests), need only this slice and `lroot.h`. The same holds for `NOT_DETERMINED` for ideles of degree >= 2.
1F.9 (catalogue) is wider and does not build on these powers, except the profinite power, which is a different
operation.
