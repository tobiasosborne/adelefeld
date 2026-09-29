# Lane s2-slice3: real roots

Slices 1 and 2 of S.2 are on master (`include/adelefeld/roots.h`, `src/roots.c`): roots at a prime. This
lane adds the real place. A review of the code at the primes runs at the same time: do not change it;
`tests/test_roots_seed.c`, `tests/test_roots_padic.c`, `tests/test_roots_forged.c` must pass unchanged.

Read first: `lanes/COMMON.md` (rule 3: every test under `timeout`), `lanes/COMMON-C.md` (rule 6 does not
hold for `roots.h`; rule 5 is replaced by item 5 below); `docs/proofs/solvers.md` section 3: Lemma 3.1,
Propositions 3.8, 3.9, Algorithm RR and Proposition 3.10, 3.13; `docs/api-s.md` section 4; decisions
S-D11 (FLINT's count of real roots is trusted, its isolation is not), S-D13, S-D17, S-D19 in
`docs/SPEC.md` 15.3; `docs/sources.md` table 3 (the rows of FLINT for real roots); the FLINT documentation
under `refs/src/flint-3.0.1/` for every function you call (`fmpz_poly.rst`, `arb_fmpz_poly.rst`,
`arb.rst`, `arb_poly.rst`, `arb_calc.rst`); `proto/solvers_checks.py` (`REAL_CASES`, the reference of
Algorithm RR, `check_s2_real*`).

**You own:** `include/adelefeld/roots.h` and `src/roots.c` (additions; the three places that the header
marks TEMPORARY for the real place are replaced), `tests/test_roots_real.c` (new),
`tests/ref/vectors/s2-slice3/` (new), `tests/julia/roots.jl` (additions),
`tests/fuzz/diff_roots_real.py` (new), `lanes/s2-slice3/`. Everything else is read-only.

## What is built

1. `adf_roots_real(L, f, prec)`, `adf_rootlist_get_arb`; at the real place `adf_rootlist_is_canonical`,
   `adf_rootlist_verify_entries` and `adf_rootlist_verify_complete`, as `docs/api-s.md` section 4 and
   Propositions 3.8 to 3.10, 3.13 state them. The accuracy of decision S-D19 is measured on the balls that
   are stored, after any widening; if it fails, `NOT_DETERMINED`, `L` untouched.
2. Every test of a sign is exact (rational end points, integer arithmetic). No floating-point number
   decides anything. The count of the real roots is recomputed in `verify_complete`, never read from the
   list.
3. Tests first, red then green (`lanes/s2-slice3/redgreen.log`), in `tests/test_roots_real.c`:
   - The 18 cases of `REAL_CASES` (close roots, multiple roots, rational and dyadic roots, no real root,
     degree 0 and 1) against a count by a Sturm chain in exact rational arithmetic written in the test
     (not FLINT's count): the number of balls equals the count; each ball passes the exact test of
     Proposition 3.8 computed by the test itself; the balls are disjoint and increasing; each gap and
     the two outer rays hold no root (by the Sturm chain of the test).
   - Planted roots: repeated roots, roots of size `10^30`, two roots `2^-40` apart, `X - 10^400`,
     Wilkinson's polynomial of degree 20, a polynomial with a root at 0 and at dyadic points (exact balls).
   - Precision: `prec` in 2, 10, 53, 200, 2000 and below 2; `arb_rel_accuracy_bits` of every output ball
     at least `max(prec, 2)` or the ball exact; balls nested for growing `prec`.
   - Verifiers: every result accepted; lists with a ball removed, two balls merged, a ball moved off its
     root, a ball widened over two roots, a false `count`, a false `n`, `g` replaced by `f` when they
     differ are refused by the entries verifier or, if the list is only short, by the complete verifier;
     the interval `[0, 4]` for `(X-1)(X-2)(X-3)` passes the first and fails the second. Print the number
     refused for each kind of change; fail if one kind has none.
   - Statuses: `DOMAIN` for `f = 0`; constants (no root, `OK`, the empty list); `L` untouched on every
     status other than `OK`; `f` a field of `L`.
   - Vectors from the reference (`lanes/s2-slice3/gen_vectors.py`, import, do not copy): the counts and
     rational enclosures of the roots; the C balls must overlap the enclosures one to one.
4. `tests/julia/roots.jl`: the real roots of `X^3 - 2 X` to 100 bits, read through `get_arb` (midpoint
   and radius through the accessors of FLINT). `tests/fuzz/diff_roots_real.py --seconds N --seed S`:
   random polynomials (degree up to 12, coefficients up to 200 bits, planted rational roots, repeated
   factors) to the C function and to the reference: equal count, enclosures that overlap one to one, the C
   verifiers accept. Run it for 180 seconds under `timeout`; that run is a smoke test.
5. Show that the tests bite, in a scratch copy under `build/`, one change at a time, recorded in
   `redgreen.log`: the squarefree part not taken (`f` used for `g`); the sign test at one end point
   accepting a zero; the disjointness test removed; the comparison of the two counts removed; the
   accuracy measured before the widening instead of after; `verify_complete` reading the count from the
   list. No `make mutate`.
6. `make clean && make check-all`, `make clean && make -j2 check SAN=1`, `make clean && make -j2 check
   CC=clang`, `sh lanes/m1-headers/check_headers.sh` pass. Give the last line of each.

Not in this lane: the route of P3.7(2) for larger primes, `set`, `swap`, `identical`, benchmarks.

If a statement of `docs/proofs/solvers.md` section 3 or a row of `docs/sources.md` about FLINT is wrong,
give the counterexample or the quotation in the report.
