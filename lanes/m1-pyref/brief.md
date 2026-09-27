# Lane m1-pyref: work package 1.1 (Python reference for the ring)

Read `docs/SPEC.md` sections 2, 4 and 9.2 (first bullet), `docs/proofs/precision.md` completely,
`proto/precision_rules.py`, `docs/PLAN.md` rows 1.1 to 1.3, 1.6, 1.7 and section 7.

**You own:** `tests/ref/` (all of it).

The reference is the oracle against which the C code will be tested. It is written **from the proofs**, for
clarity, not speed. Use `fractions.Fraction` and Python integers only.

1. `tests/ref/adfref/rat.py`, `fball.py`: exact rationals; finite balls `(A + H Zhat)/d` in canonical form (SPEC
   4.1), with radius 0; tight sum, difference, negation, product, scaling by exact rationals (SPEC 4.3); the three
   set predicates and the three-valued comparison of points (SPEC 4.2).
2. `policies.py`: the scaled-residue policy and the absolute cap (SPEC 4.4), with conversion from tight balls that
   reports loss.
3. `recon.py`: rational reconstruction from a full adelic ball and a real interval with rational endpoints (SPEC
   9.2): results one, none, several.
4. `membership.py`: an independent definition of "the rational x lies in the ball" that does not use the ball
   arithmetic (directly from the definition: `(x - a)/N` has no prime in its denominator ... write down carefully
   what membership of a rational in `a + N Zhat` means and cite the line of the proof file).
5. Tests with `unittest` under `tests/ref/tests/`, red-green (write the test, see it fail, then the code; record
   this in the report):
   - every row of the tables of SPEC 4.2, 4.3, 4.4 literally;
   - enclosure by enumeration: for small radii, all pairs of members from a window of rationals, the sum and
     product lie in the result;
   - tightness: the result radius is the gcd of the differences actually attained (witnesses as in the proof);
   - canonical form: the same set from different inputs gives the identical triple;
   - the cases zero radius, zero centre, negative centre, fractional radius, shared factors between centre and
     radius;
   - policies: the same expression in all three policies from the same inputs, containment of the tight result in
     the others after every operation; the wrong policy "force a fixed radius" must be shown to fail on
     `(1 mod 2) * (1/2)`.
   A test counts only if a wrong implementation would fail it (PLAN section 7): for each rule, also run the tests
   against a deliberately wrong variant (a mutant, for example `lcm` for `gcd`, or dropping the term `N M`) and
   show that the tests catch it. Provide `tests/ref/mutants.py` that does this automatically and prints the number
   of mutants killed and surviving.
6. `tests/ref/gen_vectors.py`: writes deterministic test vectors (seeded) as JSON lines to
   `tests/ref/vectors/*.jsonl`: inputs and expected outputs for every operation, a few hundred each, including the
   edge cases. The C tests will read these. Document the format in `tests/ref/README.md`.

Run with `python3 -m unittest discover -s tests/ref/tests`.
