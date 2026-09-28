# Lane m1-recon: rational reconstruction from a full ball (work package 1.6)

Read `lanes/COMMON-C.md`. Then `include/adelefeld/common.h`, `status.h`, `rat.h`, `fball.h`, `adele.h`
(types only), `recon.h`, `docs/api-m1.md`; `docs/proofs/quotient.md` Proposition 11 with its proof;
`docs/conventions.md` 3.2, 4.3, 6.8; `docs/SPEC.md` 9.2; `tests/ref/adfref/recon.py`;
`tests/ref/vectors/recon.jsonl`.

**You own:** `src/recon.c`, `tests/test_recon.c`, `tests/test_recon_vectors.c`,
`tests/ref/vectors/m1-recon/`, `bench/bench_recon.c` (you may add its name to `bench/Makefile`).

Implement the two functions of `recon.h`. `adele.h` is implemented by another lane at the same time: do not
call its functions; read the fields `x->inf` and `&x->fin` of the adele, and in tests initialise and clear
an adele field by field (`arb_init`, `adf_fball_init`). The end points of the `arb` are exact dyadic
numbers: obtain them exactly (`arb_get_interval_fmpz_2exp` or `arf_get_fmpq` with `mag`; read the FLINT
documentation under `refs/src/flint-3.0.1/doc/source/arb.rst`, `arf.rst`, `mag.rst` and cite file and
line). A finite ball in the local backend: what the header says; the local backend does not exist yet, so
such an input cannot be built in your tests; write the code so that it uses only functions of `fball.h`
that are documented to accept both backends, and say in the report which ones you relied on.

Tests, besides the rules of COMMON-C: the vector file completely; none, one and several candidates; both
end points of the closed interval included (a candidate equal to `lo`, equal to `hi`; M0-D3); `lo > hi`;
`lo = hi`; `N = 0` (exact finite ball) inside, at the end points, outside; radius with a denominator
(`N = 1/6`); negative centres; interval of width exactly `N` and just below `N`; an `arb` of radius 0;
operands of 4096 bits; enumeration: for small `N` and small windows, all candidates listed by brute force
agree with the count the function implies (`ADF_OK` iff exactly one). Output untouched on both failures.

One benchmark row (PLAN row 1.6), provisional. Mutation run: `make mutate FILES=src/recon.c JOBS=2`.
