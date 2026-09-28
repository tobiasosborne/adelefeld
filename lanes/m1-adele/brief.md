# Lane m1-adele: adeles and complex adeles (work package 1.3)

Read `lanes/COMMON-C.md`. Then `include/adelefeld/common.h`, `status.h`, `place.h`, `rat.h`, `fball.h`,
`adele.h`, `docs/api-m1.md`; `docs/conventions.md` sections 2 to 4.4 and 5.5; `docs/SPEC.md` 4.1, 4.3, 4.5;
`docs/seams.md` section 5 (R3, R4); `docs/proofs/precision.md` Propositions 1, 2 and 6. For every `arb_*` and
`acb_*` function you call, read its documentation under `refs/src/flint-3.0.1/doc/source/` (`arb.rst`,
`acb.rst`) and cite file and line at the call. The functions of `rat.h`, `place.h` and `fball.h` (global
backend) are implemented on master: call them.

**You own:** `src/adele.c`, `tests/test_adele.c`, `tests/test_cadele.c`, `tests/ref/adfref/adele_ref.py`,
`tests/ref/vectors/m1-adele/`, `bench/bench_adele.c` (you may add its name to the list of programs in
`bench/Makefile`).

Implement every function that `adele.h` declares, for `adf_adele` and for `adf_cadele`, except the two inline
layout queries, which exist. The finite coordinate is handled only through the functions of `fball.h`; a
local finite part is passed to them unchanged (they decide).

Reference first: write `tests/ref/adfref/adele_ref.py` from the header and the proofs, not from your C. The
real coordinate in the reference is an exact closed interval with rational end points
(`fractions.Fraction`); the finite coordinate is `adfref.fball`. Generate vectors with it
(`tests/ref/vectors/m1-adele/*.jsonl`): operands, the exact result set of the finite part, and exact rational
witnesses (points of the input sets and the image point) that the C result must contain.

Tests must cover, besides the rules of COMMON-C:
- containment after conversion of a rational at precisions 2, 10, 53, 64, 4096, including `1/3`, `-1/3`,
  `0`, negative and huge rationals (4096 bits), and exactness for dyadic numbers that fit;
- enclosure of add, sub, mul, neg, add_rat, mul_rat, div_rat: for random inputs, random rational points of the
  input sets are combined exactly and must lie in the output, in each coordinate; the finite coordinate must
  be identical to the result of the `fball.h` function alone and must not depend on `prec`;
- every status with the outputs untouched: `ADF_DOMAIN` for non-finite `arb` or `acb` (infinite midpoint, NaN,
  infinite radius), `ADF_DOMAIN` of `adf_adele_get_arb_at` at a finite place, `ADF_NOT_UNIT` for `q = 0`;
- aliasing of every permitted combination; `identical` against differing midpoint, radius, finite part;
- `adf_cadele`: `(i ; 0)` squared is `(-1 ; 0)` (SPEC 4.1); `set_adele` is exact; imaginary part exact 0
  after `set_rat`.

Benchmark rows that PLAN row 1.3 asks for (add, mul of `adf_adele` at 53 and 4096 bits, word-sized and
4096-bit finite parts), with `bench/harness.h` as `bench/bench_fball.c` does. Run once, short; report as
provisional (`quiet_machine: no`).

Mutation run: `make mutate FILES=src/adele.c JOBS=2 LIMIT=150`.
