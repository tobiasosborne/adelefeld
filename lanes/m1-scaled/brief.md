# Lane m1-scaled: the scaled-residue policy (work package 1.7, second part)

Read `lanes/COMMON-C.md`. Then `include/adelefeld/common.h`, `status.h`, `rat.h`, `fball.h`, `modctx.h`,
`scaled.h` completely, `docs/api-m1.md`; `docs/proofs/policies.md` sections 0 to 2 (Definition 4 to
Corollary 12) with proofs; `docs/proofs/precision.md` Propositions 5 and 6; `docs/conventions.md` 3.2, 4.1,
4.3, 4.6 (shared-pointer rule, closure edit E1), 5.4; `docs/SPEC.md` 4.4; `docs/reviews/m0-gate/review.md`
finding G1 and `closure.md` E1, C5; `tests/ref/adfref/policies.py`; `tests/ref/vectors/policies.jsonl`.

**You own:** `src/scaled.c`, `tests/test_scaled.c`, `tests/test_scaled_vectors.c`,
`tests/ref/vectors/m1-scaled/`, `bench/bench_scaled.c`.

Implement every function of `scaled.h` from `adf_scaled_init` to `adf_scaled_add_rat`. The five cap
functions at the end of the header are in `src/cap.c` and not yours. Contexts come from `src/modctx.c`
(on master): use only the public functions of `modctx.h` (`adf_modctx_get_modulus` and the constructors);
the layout of a context is not visible to you. `adf_scaled_get_str`, `adf_scaled_dump_str` and the
loaders are declared in `dump.h` and belong to another lane.

Tests, besides the rules of COMMON-C: the scaled rows of the vector file completely; the context rule:
two inputs with different context pointers give `ADF_DOMAIN` with the output untouched, also when the two
contexts have the same modulus, also when an input is exact, also when the output aliases an input, and the
old context of the output is never compared; the loss cases of the default product (factor
`h = gcd(u, v, K)`), where `adf_scaled_mul_tight` must be strictly finer and both must contain the tight
`adf_fball_mul` of the converted operands (`adf_scaled_get_fball`, `adf_fball_contains`); exact values keep
the tag through every operation; `set_fball` and `set_context` report `lost` exactly when the set changes;
conversion of both operands into the context `lcm(K, K')` is lossless and the ordinary operation then works
(policies Corollary 12); the same random expression of 30 operations in the tight policy and in the scaled
policy, containment after every step; canonical data (`s`, `u`) equal for equal sets from different
inputs; moduli of one word and of 4096 bits (context without blocks).

Benchmark rows of PLAN row 1.7 (add, mul), provisional. Mutation run:
`make mutate FILES=src/scaled.c JOBS=2 LIMIT=300` (the tool was repaired today and has the kinds `status`,
`drop_call`, `call_swap`, `prec`; read `lanes/tools-mutate/report.md`). Read `lanes/m1-modctx-b/report.md`.
