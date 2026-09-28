# Lane m1-fball: finite balls, tight arithmetic, global backend (work package 1.2)

Read `lanes/COMMON-C.md`. Then `include/adelefeld/common.h`, `rat.h` (you use the type `adf_rat` through its
field, an `fmpq`; the functions of `rat.h` are implemented by another lane at the same time, so do not call
them: use FLINT's `fmpq` functions on the field), `fball.h`, `docs/api-m1.md`; `docs/proofs/precision.md`
completely; `docs/proofs/policies.md` statements on the hull and on canonical form; `docs/conventions.md`
sections 2 to 5.2; `docs/SPEC.md` sections 4.1 to 4.3 and 4.5; `tests/ref/adfref/fball.py`, `membership.py`.

**You own:** `src/fball.c`, `tests/test_fball.c`, `tests/test_fball_vectors.c`, `tests/ref/vectors/m1-fball/`.

Implement every function of `fball.h` for the global backend (`ADF_GLOBAL`). A function that receives a value in
the local backend returns what the header says for that case; if the header is silent, `ADF_UNSUPPORTED` with
the outputs untouched, and a `HEADER-FINDING`.

Tests must cover, besides the rules of COMMON-C: every row of the tables of SPEC 4.2 and 4.3 literally; the
vector files `add`, `sub`, `neg`, `mul`, `scale`, `canonical`, `predicates`, `compare`, `membership` completely;
enclosure by enumeration in C for small radii (all members in a window, the sum and product lie in the result);
tightness (the four witness pairs of `precision.md`, and the valuation formula
`min(v(a)+v(M), v(b)+v(N), v(N)+v(M))` at each prime of a random product); canonical form (the same set from
different inputs gives identical fields, compared field by field); operands of 4096 bits.

Add the benchmark rows that PLAN row 1.2 asks for (tight add and mul, chain and batch, word-sized and 4096-bit
operands) as `bench/bench_fball.c`, using `bench/harness.h` as `bench/bench_word.c` does (read `bench/README.md`
and `docs/PERF.md` section 7); you also own `bench/bench_fball.c` and may add its name to the list of programs in
`bench/Makefile`. Run it once, short, and report the numbers as provisional (`quiet_machine: no`).
