# Report: lane m0-repair-catalogue

## What was done

Applied the whole review `docs/reviews/m0-proofs/catalogue-review.md` to `docs/proofs/catalogue.md` and applied
the red-green strengthening of `proto/catalogue_checks.py`.

1. All repairs of the review are in the proof file. Statements 9 and 13 were changed to agree with
   `docs/SPEC.md` 9.3.7, not the other way round. Statement 7 now states the local unit part of an idele
   includes the scale cofactor. Statement 13 now states the CRT centre and the canonical output modulus.
   Statement 15 now has the missing converse. Each repaired proof was read again and remains a proof.
2. The checks in `proto/catalogue_checks.py` now plant nine wrong formulas and demand rejection. Before
   strengthening, all nine survived; after strengthening, all nine are rejected. See the red and green output
   below.
3. A section "Review record" was appended to `docs/proofs/catalogue.md`.

## Files written

- `docs/proofs/catalogue.md` (repairs R3, R7, R9, R13, R15 and the Review record)
- `proto/catalogue_checks.py` (mutation self-test and strengthened checks)
- `lanes/m0-repair-catalogue/report.md` (this file)

## Repairs, one line each

- R3 (Proposition 3): appended the meaning of `(x/b)` for `x` in `Zhat` via a representative modulo `K`, and
  the range `c U(N)` for a unit coset.
- R7 (Proposition 7): stated the per-place precision bound, added the counterexample `1+4 Z_2` against 2 at
  relative precision 2, and inserted the idele component `r u_p`, its unit part `r' u_p` with
  `r' = r p^(-v_p(r))`, the permitted square classes, and the example `(3,3)_2 = -1`.
- R9 (Proposition 9): the statement returns only a pole status for an input ball containing a pole, never an
  unbounded or finite ball, matching `docs/SPEC.md` 9.3.7.
- R13 (Proposition 13): added the CRT centre `r = c^e` at `p|N` and `r = 1` at the other primes of the finest
  modulus, the example `F=120`, `r=49`, the returned modulus `canon(F)`, the missing converse in step (2), the
  value 1 at primes outside `N` in step (3), and renamed the step (6) cross-check.
- R15 (Proposition 15): added the missing converse of step (3), with the unit `u`, the `q = 2`, `j = 0` case
  `u_q = 3`, and the non-canonical failure `U(3)` inside `U(6)`.

## Checks run

### Red run (before strengthening), command and result

    python3 proto/catalogue_checks.py        exit 1

The nine planted errors all survived the original checks:

    planted error '(a/2) = (-1)^eps(a) instead of (-1)^om(a)' vs check_symbols: SURVIVES
    planted error 'Jacobi ignores prime multiplicity' vs check_symbols: SURVIVES
    planted error '(0/-1) = -1 instead of 1' vs check_symbols: SURVIVES
    planted error 'centre c^e for the finest modulus' vs check_power: SURVIVES
    planted error 'idele symbol without the scale cofactor' vs check_hilbert: SURVIVES
    planted error 'Hilbert precision bound 2 instead of 3 at p=2' vs check_hilbert: SURVIVES
    planted error 'missing pi^(-s/2) in the real factor' vs check_zeta: SURVIVES
    planted error 'wrong finite pole set' vs check_zeta: SURVIVES
    planted error 'wrong real pole set' vs check_zeta: SURVIVES
    AssertionError: check(s) blind to planted errors: [all nine]

### Green run, command and result

    python3 proto/catalogue_checks.py        exit 0, 1.8 s

    symbols: 2279 residue and coset cases
    hilbert solvability: 96 class pairs
    hilbert rational product: 690 rational pairs
    zeta: 38 series-tail and real-integral cases
    power: 2560 target-modulus cases, 1558 unrestricted-modulus cases; D=1, finest=24
    binomial: 980 finite-difference cases
    content and volume: 827 quotient-count and valuation cases
    cyclotomic: 1604 canonical-containment and uniformiser cases
    planted error '(a/2) = (-1)^eps(a) instead of (-1)^om(a)' vs check_symbols: rejected
    planted error 'Jacobi ignores prime multiplicity' vs check_symbols: rejected
    planted error '(0/-1) = -1 instead of 1' vs check_symbols: rejected
    planted error 'centre c^e for the finest modulus' vs check_power: rejected
    planted error 'idele symbol without the scale cofactor' vs check_hilbert: rejected
    planted error 'Hilbert precision bound 2 instead of 3 at p=2' vs check_hilbert: rejected
    planted error 'missing pi^(-s/2) in the real factor' vs check_zeta: rejected
    planted error 'wrong finite pole set' vs check_zeta: rejected
    planted error 'wrong real pole set' vs check_zeta: rejected
    repair self-test: all 9 planted errors rejected

Strengthening that made the difference, per planted error:

- symbols: explicit values `(5/2) = -1`, `(7/2) = 1`, `(2/9) = 1`, `(2/3) = -1`, `(2/27) = -1`,
  `(0/-1) = 1`, plus a comparison of `kronecker_exact` with `sympy.kronecker_symbol` on
  `a, b` in `[-60, 60]` (14641 pairs).
- Hilbert precision: the random probe now uses `required_precision(p)` and the check asserts
  `required_precision(p) == minimal_sufficient_precision(p)` for p in {2, 3, 5, 7}; the scan shows 3 at 2 and
  1 at the odd primes, and `1 + 4 Z_2` against 2 still gives both signs.
- Hilbert ideles: `idele_local_symbol(3, 3, 2, 1, 1)` must equal `hilbert_formula(3, 3, 2) = -1`, so the
  scale cofactor is not dropped.
- finest modulus: for each brute-force case the centre and canonical modulus are recomputed as
  `r = c^e` at `p|N` and `r = 1` at the other primes, and compared with `b0^e mod canon(F)` for a unit
  `b0 = c mod N`; the exact-exponent row (`M = 0`) and negative `e` are now included (1558 cases, up from 738).
- zeta: the Gaussian integral is evaluated with `mpmath` against `real_local_factor`, the finite poles
  `2 pi i k / log p` are tested by `1 - p^(-s) = 0` and derivative `log p`, and `Gamma(s/2)` is tested finite at
  `-1, -3, -5` and blowing up near `0, -2, -4`.

### Independent review checks, command and result

    python3 docs/reviews/m0-proofs/catalogue_review_checks.py        exit 0, 48.8 s

    39 [ok] lines, 0 failed.  The review's five mutants of the author's checks are now all killed (the three
    symbol mutants were survivors before this lane).

### Line length

    awk 'length > 116' docs/proofs/catalogue.md proto/catalogue_checks.py      no output

## What is not done

- No source under `refs/` exists, so the external theorems remain unchecked against a source. The review did
  not check them either.
- The review's own `[source pending]` items are unchanged; see below.
- The review's note that Definition 1 defines only integer numerators was repaired in Proposition 3 (R3).
- Nothing else from the brief is outstanding.

## Sources pending

All are pre-existing markers in `docs/proofs/catalogue.md`, none newly introduced:

- `[source pending: the Kronecker conventions of Definition 1]`
- `[source pending: a local-fields text stating simple Hensel's lemma]`
- `[source pending: a local-fields text stating the strong form of Hensel's lemma]`
- `[source pending: a local copy of a number-theory text stating quadratic reciprocity and its supplements]`
- `[source pending: a complex-analysis text stating Gamma's continuation and pole set]`
- `[source pending: a Haar-measure reference stating local scaling]`
- `[source pending: a local-fields text stating the p-adic logarithm theorem]`
- `[source pending: a finite-unit-group reference stating the unit-group exponents]`
- `[source pending: local copies of reciprocity references fixing the arithmetic and geometric names]`

## Findings against the specification

None. The review gave VALID 11, MINOR 4, INVALID 0. The only choice about `docs/SPEC.md` was R9 (whether an
unbounded enclosure may be returned); it was resolved by changing the statement to match `docs/SPEC.md` 9.3.7,
so no specification change is requested. No counterexample to a specification statement was found.
