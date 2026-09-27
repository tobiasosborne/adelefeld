# Report of lane m0-seams (work package 0.6)

Date: 2026-09-27. Author: Claude opus subagent. Written to disk by the orchestrator from the agent's final
message (the harness did not let the subagent write this file); shortened, the facts unchanged.

## What was done
The public types and concepts of version 1 were tested on paper against K = Q(sqrt(-5)) (class number 2,
P = (2, 1+w) not principal) and F = F_3(T) with its place at infinity. Result: `docs/seams.md`, a table of 38 rows
(15 types, 23 concepts), each with one verdict. Types: 0 unchanged, 12 generalise, 3 special to Q (`adf_cadele`,
`adf_idclass`, `adf_rfun`). Concepts: 4 unchanged, 14 generalise, 5 special to Q (radius as a positive rational,
place as an integer prime, the `prec` argument, idele = `r u`, the functions of SPEC 9.3). Recommendations R1 to
R9 in `docs/seams.md` section 5.

## Files written
`docs/seams.md`, `proto/seams_checks.py` (standard library and python-flint 0.8.0).

## Checks run
`python3 proto/seams_checks.py`: exit 0; 38 PASS, 0 failures; 15.4 s on one core. Among them: P^2 = (2) and
x^2 + 5 y^2 = 2 has no solution; sum rule 400 cases and product rule `a J + b I + I J` 400 cases, enclosure and
tightness, 0 failures, the mutant without `I J` fails in 118 of 400; predicates against enumeration 120 of 120;
trace dual of O is (2w)^-1 O, N(D) = 20; 3P is not principal and its minimal principal enclosures are pairwise
incomparable; over F_3(T): affine product rule 300 cases, the mutant without `N M` fails in 142; residue theorem
300 cases, the mutant with the wrong sign at infinity fails in 72.

## Not done
No source is quoted (`refs/` did not exist). Class number 2 is proved only modulo the Minkowski bound. Hecke
characters, self-dual measures and the standard function at a complex place are named, not worked out. The
uniqueness bound for partial reconstruction in K is not derived. For unit cosets in K only the containment of the
product is proved, not equality. q = 2 and higher genus are not tested. Whether FLINT has rational functions
over F_q is not checked.

## Sources pending
Twelve items, listed in `docs/seams.md` section 7.

## Findings against the specification
1. SPEC 3, row A_F: "a polynomial for the affine places" is too narrow: a monic rational function (a fractional
   ideal of F_q[T]).
2. SPEC 3, row A_K: "vector over the embeddings" should be "over the infinite places".
3. SPEC 3, row A_K: "the character uses the trace" omits the different.
No statement about Q in SPEC was found wrong.

## Notes
The draft `docs/conventions.md` CV-18 has `typedef ulong adf_place_t` with 0 for the archimedean place;
recommendation R1 accepts this only if the integer value is not part of the contract.
