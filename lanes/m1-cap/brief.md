# Lane m1-cap: the absolute cap on finite balls (work package 1.7, first part)

Read `lanes/COMMON-C.md`. Then `include/adelefeld/common.h`, `status.h`, `rat.h`, `fball.h`, `scaled.h`
(the whole header for context; yours are the five functions `adf_fball_cap`, `adf_fball_add_cap`,
`adf_fball_sub_cap`, `adf_fball_mul_cap`, `adf_fball_mul_rat_cap` and their comments), `docs/api-m1.md`;
`docs/proofs/policies.md` sections 0, 1 and 3 (Definition 13, Propositions 14 and 15) with proofs;
`docs/conventions.md` 5.4 (the part on the cap), 3.2, 4.1, 4.3; `docs/SPEC.md` 4.4;
`tests/ref/adfref/policies.py` (the cap functions); `tests/ref/vectors/policies.jsonl` (the rows on the cap).

**You own:** `src/cap.c`, `tests/test_cap.c`, `tests/test_cap_vectors.c`, `tests/ref/vectors/m1-cap/`.

Implement the five cap functions for the global backend, on top of the functions of `fball.h` (implemented
on master). The scaled type `adf_scaled` is not yours; another lane writes `src/scaled.c` later. A value in
the local backend: what the header says; if it is silent, `ADF_UNSUPPORTED`, outputs untouched, and a
`HEADER-FINDING`.

Tests, besides the rules of COMMON-C: the cap rows of the vector file completely; every status of the
header with the outputs untouched (the cap `C` not positive, and what else the header names); exact values
are untouched by the cap (M0-D2); the result contains the tight result of the same operation (policies
Theorem 3): by the predicate `adf_fball_contains` and by enumeration of members for small radii; the radius
of the result is `gcd(R, C)` with the rational gcd; the invariant of Proposition 15 along a chain of 50
random capped operations; the same expression evaluated tight and capped, containment after every step;
operands of 4096 bits.

Mutation run: `make mutate FILES=src/cap.c JOBS=2`.
