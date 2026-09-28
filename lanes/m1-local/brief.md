# Lane m1-local: the local backend of finite balls (work package 1.8, second part)

Read `lanes/COMMON-C.md`. Then `include/adelefeld/common.h`, `status.h`, `rat.h`, `fball.h` completely
(every sentence on local values), `modctx.h` (the section on conversion into and out of the local
backend), `docs/api-m1.md`; `docs/proofs/policies.md` section 4 completely (Definition 16 to Summary 26)
with proofs and the review record on Propositions 24 and 25; `docs/conventions.md` 4.6, 5.2, 5.3, 5.14;
`docs/SPEC.md` 4.1; `docs/PLAN.md` row 1.8; `docs/reviews/m0-gate/review.md` finding G11;
`lanes/m1-fball/report.md` (HEADER-FINDING 1); `lanes/m1-modctx/report.md`; `src/fball.c`, `src/modctx.c`.

**You own:** `src/fball.c` (you extend the file of lane m1-fball; its global behaviour must not change),
`src/fball_local.c`, `tests/test_fball_local.c`, `tests/test_fball_local_vectors.c`,
`tests/ref/adfref/local_ref.py`, `tests/ref/vectors/m1-local/`, `bench/bench_local.c` (you may add its
name to `bench/Makefile`). `src/modctx.c` is read-only for you: if you need an internal accessor of the
context that `modctx.h` does not offer, do not add it; use the public read access and list the cost in the
report.

1. Reference first: `tests/ref/adfref/local_ref.py`, written from the proofs: a local value as
   `(d; res_1..res_k)` at a context, conversion both ways, negation, sum, product, product with an exact
   scalar, the canonical triple, and the rules of conventions 5.3 on when a result stays local. Generate
   vectors from it.
2. `adf_fball_set_local`, `adf_fball_set_local_enclose`, `adf_fball_set_global`, `adf_fball_is_local`,
   `adf_fball_context` in `src/fball_local.c`.
3. In `src/fball.c`: every function of `fball.h` accepts local inputs as the header says (life cycle with
   the residue array, `set`, `swap`, `identical`, `is_canonical` with predicate L, the predicates and
   comparisons through the canonical triple, arithmetic that stays local at a shared context pointer and
   is global otherwise). Remove the `HEADER-FINDING 1` marks where the behaviour now exists.

Tests, besides the rules of COMMON-C: every existing test of `tests/test_fball.c` and
`tests/test_fball_vectors.c` passes unchanged (you do not own them); the set is unchanged by conversion
in both directions, denominators included; the case `A = d = 2` with block 4 (a denominator sharing a
factor with a block, Proposition 25); the case `(2; 2)` in context `(4)` (cancellation keeps the set, not
the canonical triple, Proposition 24); the canonical `H` changing after a sum; equality and `identical`
through the rules of the header; two different context pointers with equal moduli give a global result;
every local operation against the same operation on the global forms (`adf_fball_equal_set`); 1, 2, 64
and 128 blocks.

Benchmark rows of PLAN row 1.8 (conversions; batch add and mul), next to the global rows, provisional.
Mutation runs: `make mutate FILES=src/fball_local.c JOBS=2 LIMIT=150` and
`make mutate FILES=src/fball.c JOBS=2 LIMIT=150`.
