# Lane i-slice3: result (powers, hulls, division; the rest of WP 2.1, 2.3, 2.4)

Worktree at edfa658; `refs/src` linked to the main checkout. Work 2026-09-30 01:23 to 02:39 (clock).

## What was built

A user raises unit cosets, ideles and classes to integer powers (the default enclosure and the smallest coset),
maps an idele to an adele (two hulls) and back, and divides an adele by an idele, in C and from Julia. With this
slice milestone 2 is complete except for the text and dump forms of the three types.

- `include/adelefeld/idpow.h`, `src/idpow.c` (new): `adf_ucoset_pow`, `adf_ucoset_pow_tight` (void),
  `adf_idele_pow`, `adf_idele_pow_tight`, `adf_idclass_pow`, `adf_idclass_pow_tight` (`OK`, `NOT_DETERMINED`,
  `LIMIT`), `ADF_IDELE_POW_BITS_MAX = 2^26`. Exponent `slong`, `WORD_MIN` included; `k = 0` gives the exact
  unit / idele / class 1. The tight modulus `M_k` of `ideles.md` P13 is built from the factorisation of `abs(k)`
  alone (`n_factor`, `n_is_prime`), never of `N`. The real part of a power: binary powering of the E1 ends with
  directed rounding after every product, then kernel B of slice 1 (sign `sign(X)^k`).
- `include/adelefeld/idmap.h`, `src/idmap.c` (new): `adf_adele_set_idele` (smallest hull
  `r c' + r lcm(N, 2) Zhat`, `c'` odd; exact `r e` for an exact unit), `adf_adele_set_idele_simple`
  (`r c + r N Zhat` of the normal form), `adf_idele_set_adele` (`OK`, `UNIT_NOT_CERTIFIED`, `NOT_UNIT`, by the
  maximum), `adf_adele_div_idele` (finite part: the product rule of `fball.h` with the smallest hull of the inverse coset,
  then the exact division by `r`, which is the smallest ball of P19; real part `arb_div`; `OK`,
  `NOT_DETERMINED`, `LIMIT`). Division by an exact rational is `adf_adele_div_rat` of milestone 1, now tested
  against P18. No division by an adele (SPEC 4.5).
- `docs/api-2.md` section 3: functions, examples, Statements J (the two powers of a unit coset, with the proof of
  the d-ideles "statement E3"), K (the real kernel of a power), L (powers of ideles and classes, the limit), M (the
  hulls), N (adele to idele), O (the division), each with a stepwise proof; what was taken from d-ideles;
  decisions i3-1 to i3-12 with alternatives.
- `proto/ideles_checks.py` part 4 (`check_api_powers`, `check_api_hulls`, `check_api_division`; `part4` runs it
  alone); `lanes/i-slice3/gen_vectors.py` -> `tests/ref/vectors/i-slice3/idpow.jsonl` (1573 lines, 553596 bytes),
  `idmap.jsonl` (774 lines, 147115 bytes): 701 KB in all, below 1 MB; regenerated identically (sha256).
- Tests `tests/test_idpow.c` (6 tests), `tests/test_idmap.c` (6 tests); Julia `tests/julia/idmap.jl`, run by a new
  block 2d of `tests/test_julia.sh`; two includes and two comment lines in `include/adelefeld.h`.

## Checks run (commands and results)

| Command | Result |
|---|---|
| `timeout 300 python3 proto/ideles_checks.py` | 19 checks pass (parts 1 to 4), 2 min 8 s. Part 4: 8996 (coset, k) against the table `M_k` of part 1 (N <= 60, 52 exponents up to 196), 26988 centres by random `chat`; 224 cases against enumeration (smallest coset, residue, default contains, repeated multiplication equals default); 3000 real powers against exact ends; 1680 (coset, content) hulls; 2500 quotients against enumeration |
| `timeout 120 python3 lanes/i-slice3/ref_faults.py` | three faults of the reference (tight exponent `e_p` for `1 + e_p`, hull modulus `N` for `lcm(N, 2)`, division by the coset for its inverse): each caught |
| `./build/test_idpow` | 6 tests, 10122 checks, 0 failed. Enumeration: 296 (coset, k) cases (N <= 12, k in -4..4 and 6), 118 levels above 400000 skipped (those are in the vectors); vectors 1573 lines (642 unit cosets, 509 ideles, 422 classes), 322 `NOT_DETERMINED`, 15 `LIMIT`, 114 exact results, 594 with exact ends |
| `./build/test_idmap` | 6 tests, 54197 checks, 0 failed. Hulls: 1680 (coset, content) cases, every unit at level `210 lcm(N, 2)` inside, small radius `= r D` (tightness), simple strictly coarser in 1392; division: 4736 cases against enumeration, 600 by exact units (= `adf_adele_div_rat`), 64 with `a = M = 0`; vectors 774 lines (47 refusals of adele to idele, 2 `LIMIT`) |
| `make clean && make -j2 check-all` (02:23 to 02:29) | `check-all passed: make check, driver, exports, julia, mutate-selftest, memcheck-selftest`; `check passed: all 68 test programs`; `test_exports: passed: 395 of 395 declared functions are exported, ... no exported name is undeclared, no variadic function`; Julia testset of this slice 35 of 35 |
| `make clean && make -j2 check SAN=1` (02:29 to 02:33) | `check passed: all 68 test programs`; 0 lines with `runtime error`, `AddressSanitizer` or `LeakSanitizer` |
| `make clean && make -j2 check CC=clang` (02:33 to 02:35) | `check passed: all 68 test programs`; 0 warnings |
| `make clean && make -j2 check INV=1` (02:35 to 02:38) | `check passed: all 68 test programs` |
| `sh lanes/m1-headers/check_headers.sh` | `check_headers: passed` (72 single-header compilations, 38 inline functions exported, 6 declarations in `idpow.h`, 4 in `idmap.h`) |
| INV probe (a scratch program from the session scratchpad, built into `build/`) | a non-canonical input aborts with `adf_ucoset_pow_tight: argument x is not a canonical adf_ucoset`, `adf_idele_pow: argument x ...`, `adf_adele_div_idele: argument y ...` |
| cost probe of `adf_ucoset_pow_tight` on `[1 mod 1]` | `k = 897612484786617600` (103680 divisors): `M_k` of 556490 bits in 0.2 s; `k = 963761198400`: 31468 bits; `k = WORD_MIN`: 97 bits |

The four `make` suites, the header check, the INV probe and the cost probe ran after the last edit of any source,
header or test (the last code edit, 02:22, moved the invariant checks of the idele and class powers from the
static helpers into the public functions, so that the abort message names the public function; the four suites
had passed before it as well and were run again; the table gives the second runs). The Python checks (01:58) and
the fault runs (01:57) are older; they do not depend on that edit (the Python not at all, the faults are built
without the flag, where the macros are empty). The divisor counts 103680 and 6720 were checked with
`sympy.divisor_count`. Logs: `lanes/i-slice3/check-all.log`, `check-san.log`, `check-clang.log`,
`check-inv.log`, `check-headers.log`, `proto.log`, `julia.log`, `faults.log`, `ref_faults.log`,
`gen_vectors.log`.

What would have made a case fail: a tight coset whose modulus differs from the canonical hull modulus of the
enumerated powers, or from the table `M_k` of P13 (through the vectors), or whose residue misses a power; a
default coset other than the product of `abs(k)` copies; a power outside a result; a real result that misses the
exact ends `sign * L`, `sign * H` of `{xi^k}` or the rounded ends of the reference, contains 0, has the wrong sign,
or is not exact when the reference ends coincide; a status other than the reference's (a function of correctly
rounded ends, K.3); a content other than `r^k`; a hull that misses a unit of the coset at the level, a smallest
hull whose radius is not `r` times the gcd of the differences, a simple hull that is not `r N'`, two hulls of one
set that differ; a status of adele to idele other than the reference's; a finite quotient outside the result, a
radius other than the enumerated hull, a result other than the product with the hull of `adf_idele_inv`, or than
`adf_adele_div_rat` for an exact unit; a real quotient of the end points outside the real part; a touched output
on `NOT_DETERMINED`, `LIMIT`, `UNIT_NOT_CERTIFIED`, `NOT_UNIT` (also aliased); an aliased call that differs from
the plain one; a local output or input that leaves the result local.

Red and green (`lanes/i-slice3/redgreen.log`): test_idpow: link errors; then a stub (each power returns its
input): 5582 failed checks in all 6 tests; green after one defect of my own test (it expected `OK` for
`(-3 +- 1/4)^3` at prec 2, where the ends 8 and 64 are 3 binades apart and B1 gives `NOT_DETERMINED`; replaced by
the property the header states, not weakened). test_idmap: link errors; then a stub: 25574 failed checks in 5 of 6
tests (the sixth tests `adf_adele_div_rat` of milestone 1 against P18 and passes on the stub, as it must); green at
the first run. A first stub run aborted in my test's reader: the generator had written `k = 6 (2^61 - 1)`, above
`2^63`; the exponent list was corrected and the vectors regenerated.

Faults (`sh lanes/i-slice3/run_faults.sh`, scratch copies under `build/faults3/`, log `faults.log`; run at 01:57,
before the move of the invariant macros, which are empty in this build):

| Fault | Caught by |
|---|---|
| H1 tight modulus: `b_p = e_p` for odd `p` with `(p - 1) | k` | test_idpow: 731 failed checks in 3 tests |
| H2 the sign `sign(X)` also for even `k` | test_idpow: 214 in 3 tests |
| H3 `k < 0`: `lo = RD_p(1/lo)` (the wrong end) | test_idpow: 699 in 4 tests |
| H4 smallest hull without the factor 2 of `lcm(N, 2)` | test_idmap: 4235 in 4 tests |
| H5 division by the hull of the coset instead of its inverse | test_idmap: 750 in 2 tests |
| H6 the content limit `>=` instead of `>` | test_idpow: 2 (the hand case at the edge `4 abs(k) = 2^26`) |

No mutation run and no fuzz target (brief item 5).

## Decisions taken (the table of `docs/api-2.md` 3.5 has the alternatives)

| Id | Taken | Main alternative |
|---|---|---|
| i3-1 | no set predicates of ideles and classes: the conventions ask them of finite balls (SPEC 4.2, conventions 2.1), `adf_adele` has none, no plan item asks | offer them |
| i3-2 | exponent `slong`, every value | `fmpz` |
| i3-3 | `LIMIT` when `r != 1` and `abs(k) (bits(n) + bits(d)) > 2^26`, before `r^k` is formed | `2^24`; a limit after the power |
| i3-4 | simple hull of the normal form (a function of the set) | of the stored pair |
| i3-5 | real part of a power by binary powering of the ends, directed rounding, kernel B | correctly rounded `RD_p(l^n)`; `arb_pow_ui` and a sign test |
| i3-6 | `adf_ucoset_pow_tight` void (size of `M_k` bounded by `k`, J.4) | `LIMIT` above a size |
| i3-7 | real part of the division `arb_div`, `NOT_DETERMINED` if not finite (CV-08) | kernel-B inverse then `arb_mul` |
| i3-8 | finite part of the division by the product rule with the smallest hull of the inverse, then `/ r` | the formula of P19 directly (kept as the oracle of the reference) |
| i3-9 | finite parts of results always global | keep a local result where `fball.h` would |
| i3-10 | adele to idele, real ball with 0 (not the exact 0): `UNIT_NOT_CERTIFIED` | `NOT_DETERMINED` |
| i3-11 | division by an exact rational: `adf_adele_div_rat`, tested against P18 | a new function |
| i3-12 | `adf_adele_set_idele` is the smallest hull, `_simple` the other | the reverse |

From the unreviewed d-ideles reference I took, and proved again (api-2.md 3.3, 3.4): the tight modulus as a part at
the primes of `N'` times a part elsewhere with the centre `c'^k` mod the first and 1 mod the second (its "E3"; now
Statement J.2, the first part from the factorisation of `k`); binary powering of the ends; the value `2^26` of the
content limit; the statuses of adele to idele; the smallest hull with an odd representative; the simple hull of the
normal form; the P19 formula as the oracle. Not taken: its Bernoulli-denominator check (no source on disk).

## What is proved and what is not

- Proved in `docs/api-2.md` 3.3, stepwise, not reviewed: Statements J (including that the construction gives
  `M_k` of P13 and an admissible `chat` of P13.2, the completeness of the prime search, the size bound J.4), K
  (enclosure, sign, status a function of the inputs, exactness K.4), L (points, `k = 0`, classes, the limit), M, N,
  O (the division equals P19 through P19.6 and P18). They rest on `ideles.md` P3, P10, P13, P15, P16, P17, P18, P19
  and `precision.md` Proposition 2, which are reviewed (review record of `ideles.md`), and on FLINT's documented
  behaviour (`ulong_extras.rst:833-840, 1203`, `arf.rst:24-39`, `arb.rst:9-11, 856-876`, `fmpz.rst:923-929,
  1154-1160, 1292-1302`, `fmpq.rst:480`).
- Not proved: a bound on how much wider the ends of K are than the correctly rounded `l^n`, `h^n` (stated as not
  proved; nothing depends on it). No numerical maximum of `tau(abs(k))` for 64-bit `k` is claimed (J.4 is stated in
  terms of `tau`; the probe measured 103680 divisors and 556490 bits).
- `NOT_DETERMINED` of `adf_adele_div_idele` (a non-finite `arb_div`) is reached by no test and by no known input: a
  probe with `Y = [1 + 2^-100 +- 1]` gives a finite ball at every precision from 2 to 256. It is kept because
  conventions 4.4 (CV-08) requires it.
- Tests: the tight power is checked against enumeration only for `N <= 12`, `k` in `-4..4` and 6 (296 cases); for
  large `k` (up to `WORD_MIN`, `WORD_MAX`, `720720`, `3^39`, `2^61 - 1` and six random 63-bit exponents) the C is
  compared with the reference, whose modulus is checked against the table `M_k` of part 1 for `abs(k) <= 196` and
  whose table part 1 checks against enumeration (`check_power`, `check_power_local`, `abs(k) <= 30`). The claim for
  large `k` rests on the proof of J.2, not on enumeration.

## Findings against the specification

1. `PLAN.md` 2.1 (tests column): "`k = 1, -1` give `M_k = N`" contradicts `ideles.md` P13.4 (`M_k = Nbar`). Found
   by d-ideles and i-slice1; it stands. The code follows P13.4 and conventions 5.6 ("both coincide for `k = 1` and
   `k = -1`"); `[5 mod 6]^1` is `[2 mod 3]` in both functions (tests and Julia).
2. Conventions 3.2, row "Unit coset, idele, idele class arithmetic", lists `OK`, `NOT_DETERMINED`, `NOT_UNIT`. The
   powers add `LIMIT` for the size of `r^k` besides the `prec` limit of i-slice2 (its finding 2). The row should
   name `LIMIT`.
3. Conventions 5.6 names `adf_ucoset_pow(y, x, k)` without the type of `k`, and 5.7 the simple ball
   `r c + r N Zhat` without saying whether `(c, N)` is the stored pair or the normal form (for the stored pair the
   simple ball is not a function of the set). Taken: `slong` (i3-2) and the normal form (i3-4).
4. Conventions 3.2, row "Adele to idele; inversion of an adele-like value", admits `NOT_DETERMINED`; no case of
   `adf_idele_set_adele` returns it (i3-10). Not a contradiction, noted.

No source is pending: every formula used is cited from `docs/proofs/ideles.md`, `docs/proofs/precision.md` or
the FLINT documentation under `refs/src/flint-3.0.1/`.

## Avoidable costs (COMMON-C rule 7)

`adf_ucoset_pow_tight` recomputes each divisor from the exponent vector (a product per divisor) and multiplies
`B` by one prime power at a time (quadratic in the number of primes; a product tree would do; 0.2 s at 103680
divisors); both powers copy and normalise the input (a temporary); `hull_fin` copies the unit; the division calls
`adf_fball_set_global` on a value that is global unless its input was local, forms the ucoset inverse and the hull
as two values, and divides by `r` after the product where one scaling of the hull would do; `adf_idele_set_adele`
initialises a rational it may not use; `ends_pow` multiplies by 1 at the first set bit.

## Not done

- Text and dump forms of unit cosets, ideles and classes (the last part of milestone 2).
- Set predicates of ideles and classes (i3-1), characters of the class group (milestone 3).
- `docs/api-2.md` section 3 is not reviewed; the orchestrator records the decisions in `docs/SPEC.md` 15 if wanted.
- No mutation run, no fuzz target (brief item 5).

## Files

New: `include/adelefeld/idpow.h`, `include/adelefeld/idmap.h`, `src/idpow.c`, `src/idmap.c`, `tests/test_idpow.c`,
`tests/test_idmap.c`, `tests/julia/idmap.jl`, `tests/ref/vectors/i-slice3/idpow.jsonl`,
`tests/ref/vectors/i-slice3/idmap.jsonl`, `lanes/i-slice3/{gen_vectors.py, run_faults.sh, ref_faults.py,
redgreen.log, progress.md, result.md}` and the logs `check-all.log`, `check-san.log`, `check-clang.log`,
`check-inv.log`, `check-headers.log`, `proto.log`, `julia.log`, `faults.log`, `ref_faults.log`, `gen_vectors.log`.
Changed: `docs/api-2.md` (new section 3), `proto/ideles_checks.py` (part 4 and `part4` in `__main__`),
`include/adelefeld.h` (two includes, two comment lines), `tests/test_julia.sh` (block 2d). `src/idele_internal.h`,
`ucoset.h`, `idele.h`, `idclass.h` and their sources are unchanged (the kernel of slice 1 is used through
`idele_internal.h` as it is).
