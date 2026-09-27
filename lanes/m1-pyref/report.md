# Lane m1-pyref: work package 1.1 (Python reference for the ring)

## What was done

A Python reference for the additive ring of finite adeles of Q, written from the proofs in
`docs/proofs/precision.md` and the tables of `docs/SPEC.md` sections 2, 4 and 9.2 (first bullet).
Only `fractions.Fraction` and Python integers are used. The reference covers:

- exact rational helpers: `qgcd`, `qlcm`, `is_integer`, `v_p`;
- the finite ball `(A + H Zhat)/d` in the canonical form of SPEC 4.1 (radius 0 is a point);
- the tight rules of SPEC 4.3: sum, difference, negation, product, scaling by an exact rational;
- the set predicates and the three-valued comparison of SPEC 4.2;
- the scaled-residue policy and the absolute cap of SPEC 4.4, with a conversion from a tight ball
  that reports loss;
- rational reconstruction from a full ball and a closed real interval (SPEC 9.2, first bullet);
- an independent membership test for a rational in a ball, from the definition;
- a red-green `unittest` suite, a mutation harness, and deterministic test vectors as JSON lines.

The code was written after the tests (see the checks below). It is the oracle for the C code and
is deliberately written for clarity, not speed.

## Files written (all under `tests/ref/`)

- `adfref/__init__.py`
- `adfref/rat.py`: `qgcd`, `qlcm`, `is_integer`, `v_p`
- `adfref/fball.py`: `Fball`, `canon`, `add`, `sub`, `neg`, `mul`, `scale`, `contains_rational`,
  `equal_set`, `overlaps`, `contains`, `compare`
- `adfref/policies.py`: `ScaledBall`, `Exact`, `convert_from_tight`, `scaled_add`,
  `scaled_sub`, `scaled_neg`, `scaled_mul`, `scaled_scale`, `absolute_cap`
- `adfref/recon.py`: `reconstruct`, `Result`, statuses `NONE`, `ONE`, `SEVERAL`
- `adfref/membership.py`: `rational_in_fball`
- `tests/test_tables.py`: every row of the tables of SPEC 4.2, 4.3, 4.4 literally
- `tests/test_arithmetic.py`: enclosure by enumeration, tightness witnesses, negation, compare
- `tests/test_canonical.py`: canonical invariants, uniqueness from different inputs
- `tests/test_policies.py`: conversion and loss, cap, policy comparison, the fixed-radius failure
- `tests/test_recon.py`: one, none, several; endpoints; exact ball
- `tests/test_membership.py`: the definition, the `1/5` versus `5 mod 6` example
- `tests/test_mutants.py`: the mutation harness must kill every mutant
- `mutants.py`, `gen_vectors.py`, `README.md`
- `vectors/*.jsonl` (11 files, 6128 lines)

## Checks that were run

All commands from the repository root `/home/tobias/Projects/adelefeld`.

1. Red, before `adfref/` existed:

       python3 -m unittest discover -s tests/ref/tests

   Result: `Ran 6 tests in 0.001s` and `FAILED (errors=6)`; each of the six test modules failed to
   import with `ModuleNotFoundError: No module named 'adfref'`.

2. First run after the first implementation attempt:

       python3 -m unittest discover -s tests/ref/tests

   Result: `Ran 54 tests in 0.286s`, `FAILED (failures=2, errors=2)`. The failures were
   `test_contains_row` (a wrong expected value in the test itself: `0 mod 2` is inside `0 mod 1`)
   and `test_invariants_random` (the raw centre differs from the canonical centre by an integer
   multiple of the radius); the errors were two missing `import random` lines in `test_recon.py`.
   All four were test defects and were fixed without weakening a statement.

3. Final suite:

       python3 -m unittest discover -s tests/ref/tests

   Result: `Ran 59 tests in 5.731s` and `OK`.

4. Mutation harness:

       python3 tests/ref/mutants.py

   Result:

       baseline: 58 tests, failures 0, errors 0
       mutants killed: 18
       mutants survived: 0

   The 18 mutants are: sum radius uses lcm; product drops `N M`; canonical form skips `A mod H`;
   canonical form skips `gcd(A,H,d)`; `qgcd` returns the largest argument; `contains` drops `N/M`;
   `contains` drops `(a-b)/M`; `overlaps` uses lcm; `equal_set` tests only the radii; negation
   forgets the sign; scaling forgets the radius; scaled sum adds scales and residues; scaled
   product adds residues; scaled product adds scales; cap uses `min(R,C)`; membership tests only
   the centre; reconstruction uses `floor` for the lower bound; reconstruction ignores the lower
   bound.

5. Vector generator:

       python3 tests/ref/gen_vectors.py

   Result (lines per file): add 444, canonical 212, compare 444, membership 580, mul 444,
   neg 212, policies 1260, predicates 1332, recon 360, scale 396, sub 444; total 6128.

6. Determinism of the vectors:

       sha256sum tests/ref/vectors/*.jsonl | sha256sum

   Result: `0cc9e48c13f69c69398dbeeb336c224c93dd7a72a6c0779e1889f8482a8c041d` on two consecutive
   regenerations. All 6128 lines parse as JSON with an `op` field.

7. Cross-check of `qgcd` and the product radius against `proto/precision_rules.py`:

   - 5000 random rational tuples: 0 mismatches for `qgcd`;
   - 5000 random pairs of balls: 0 mismatches for the tight product radius
     `gcd(a M, b N, N M)`.

8. Canonical uniqueness, wider offline check: 646 distinct canonical triples with
   `-12 <= A <= 12`, `0 <= H <= 12`, `1 <= d <= 8`; 0 pairs with the same set.

9. Style and syntax: no Python line in `tests/ref/` is longer than 116 characters;
   `python3 -m py_compile` succeeds for every module.

## What is not done

- The exact-scalar rules for addition in the scaled policy, and conversion between two scaled
  contexts, are not implemented. SPEC 4.4 defers these to work package 0.3. The policy comparison
  in `tests/test_policies.py` therefore uses only positive-radius scaled inputs; where an exact
  value meets a scaled value, `policies.scaled_add` falls back through a tight ball and the
  conversion, which is a valid enclosure.
- The types `adf_rat`, `adf_adele`, `adf_cadele` (work packages 1.2 and 1.3) and the local backend
  (1.8) are outside this lane.
- Reconstruction from partial data (SPEC 9.2, second bullet) is a separate function on a separate
  type and is not in this lane's brief.
- No fuzzing was done; the brief asks for mutation testing for the arithmetic rules.
- The C tests that will read `vectors/*.jsonl` are not written (they belong to the C lanes).

## Sources pending

None. Every mathematical statement used is either proved in `docs/proofs/precision.md` (on disk)
or is my own proof written out in the code. The membership test cites `precision.md` Lemma 1,
lines 13-18. The code cites the propositions of `precision.md` by number.

## Findings against the specification

1. **Absolute cap at radius 0.** SPEC 4.4 says "after a tight operation the radius `R` is replaced
   by `gcd(R, C)`". With the definition of the rational gcd used in the same document,
   `gcd(0, C) = C`, so a literal reading turns the exact point `a` into the ball `a + C Zhat`. This
   is an enclosure and is coarser, so it does not contradict any stated invariant, but it destroys
   exactness. It is not stated whether this is intended. The reference keeps an exact value exact
   (`absolute_cap` returns it unchanged, tested in
   `test_cap_keeps_an_exact_value_exact`). The C code must share whichever convention is chosen;
   the specification should say which.

2. **Endpoints of the real interval in reconstruction.** SPEC 9.2 says "Intersect it with the real
   interval" and does not say whether the interval is open or closed. The reference treats it as
   the closed interval `[lo, hi]` and includes endpoints (`test_endpoint_included`). This should
   be fixed in the conventions of work package 0.4.

3. **Exact values in the scaled policy.** SPEC 4.4 says that exact zero, exact scalars and
   conversion between contexts "have their own rules, to be proved in work package 0.3". This is a
   deferral, not an error. The reference therefore represents an exact value in a policy as a
   separate `Exact` tag and only proves the positive-radius conversion of Proposition 6(1).
