<!-- Second attempt at the brief of lane m1-modctx, run in parallel on another model because the first is slow. The orchestrator takes one of the two results. -->
# Lane m1-modctx: modulus contexts (first part of work package 1.8)

Read `lanes/COMMON-C.md`. Then `include/adelefeld/common.h`, `modctx.h`, `docs/api-m1.md`;
`docs/conventions.md` section 4.6 (context life cycle: constructors, free, ownership, inspection with capacity
and descriptor, as decided by the gate review and its closure check: `docs/reviews/m0-gate/review.md` G2,
`closure.md` E2, C2) and sections 5.3, 12; `docs/proofs/policies.md` definitions and statements on the local
backend (contexts as lists of pairwise coprime word-sized blocks; derived contexts); `docs/SPEC.md` 4.1 and
section 15 (modulus families); `docs/PERF.md` section 4 (the rows on conversion between the global integer and
residues).

**You own:** `src/modctx.c`, `tests/test_modctx.c`, `tests/ref/vectors/m1-modctx/`,
`tests/ref/adfref/modctx_ref.py` (a reference you write first, from the proofs, for the constructors and for
conversion of an integer to residues and back).

Implement every function of `modctx.h`: the constructors of every modulus family (arbitrary integer, list of
pairwise coprime blocks, list of prime powers, factorial, power of a primorial), with complete validation of
their arguments and the statuses of the header; free; inspection; reduction of an integer to residues and
recombination (use FLINT's `fmpz_multi_mod` / `fmpz_multi_CRT` or `fmpz_comb` after reading their documentation
under `refs/src/flint-3.0.1/` and their headers; cite file and line). A context is immutable after
construction and may be shared between threads: no mutable field, no lazy initialisation.

Tests: every family against the Python reference; failure of every validation (blocks not coprime, a block of 0
or 1, a block above one word, exponent 0, too many blocks), with `*out` untouched and no leak (run under
`SAN=1`); round trip integer to residues to integer for random integers up to the modulus, for 1, 2, 64 and 128
blocks; a test that two threads can read one context at the same time (pthreads; run it under
`-fsanitize=thread` from a script in your lane directory, since the Makefile has no such target, and report the
output).

Add the benchmark rows for the two conversions (`n = 4096` bits, `k = 128` blocks) as `bench/bench_modctx.c`
with `bench/harness.h` (read `bench/README.md`, `docs/PERF.md` sections 4 and 7); you own that file and may add
its name to `bench/Makefile`. Report the numbers as provisional, next to the I/O floor of PERF section 4.

Mutation run: `make mutate FILES=src/modctx.c JOBS=2 LIMIT=150`. `adf_modctx_new_from_dump` needs the grammar of the dump form (conventions 10.1, body "modctx"): write the validation of that one body in `src/modctx.c`, by hand over `(s, len)`, with no reliance on a NUL terminator and no raw input passed to a FLINT string function. The conversions of `adf_fball` into and out of the local backend at the end of `modctx.h` are not yours (lane m1-local).
