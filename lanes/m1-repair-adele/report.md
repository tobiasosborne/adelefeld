# Lane m1-repair-adele: findings R3, R4 and part of R6 of reviewer `arith`

## What was done

1. New test `tests/test_adele_lowprec.c` (534 lines). For `prec` in 1, 0, -5, 2, 3, 53 it runs
   `add_rat`, `mul_rat`, `div_rat`, `set_rat`, `add`, `sub`, `mul` of `adf_adele` and `adf_cadele`,
   with `q` in 1/3, -1/3, 3, 7/1024 and (2^4096 + 1)/(2^4095 + 1). Each result is checked with
   `adf_adele_is_canonical` / `adf_cadele_is_canonical`, with `arb_contains_fmpq` /
   `acb_contains_fmpq` against the exact `fmpq` image, and, below 2, for identity with the result
   at `prec = 2`. The rational operations alias the output with the input.
2. `src/adele.c`: a static helper `adele_prec` clamps `prec` below 2 to 2; every function that takes
   a `prec` calls it. `mul_rat` is `arb_mul_fmpz` by `n` then `arb_div_fmpz` by `d`; `div_rat` is
   `arb_mul_fmpz` by `d` then `arb_div_fmpz` by `n` (the sign of `n` is carried by the integer;
   `n = 0` is `ADF_NOT_UNIT` before). The `acb` counterparts use `acb_mul_fmpz` / `acb_div_fmpz`.
   `add_rat` still converts `q` to a ball at the clamped `prec` and adds it; there is no exact sum of
   a ball and a rational in FLINT 3.0.1. The file comment was changed from `HEADER-FINDING` to the
   reading of decision M1-D4.
3. Comments, finding R6: `src/rat.c:15` and `:225` now cite `fmpq.h:212` for `fmpq_sub` (was 209);
   `src/adele.c` cites `arb.rst:9-11` and `arb.rst:25-26` (was 11-14 and 23-24);
   `src/fball.c` keeps no `#pragma weak` and no comments about a missing place backend, and
   `adf_fball_prec_at` has no null checks for the always-linked place functions.
4. One record of `tests/ref/vectors/m1-adele/set_rat.jsonl` changed: `3/4` at `prec = 1` now has
   `"exact": true` (was `false`). No other record changed.

## Files written or changed

- `src/adele.c` (repaired; the only arithmetic change).
- `tests/test_adele_lowprec.c` (new).
- `src/rat.c`, `src/fball.c` (comments only, except the removal of the dead `#pragma weak` and its
  two guards in `adf_fball_prec_at`; no statement of those two files changes otherwise).
- `tests/ref/vectors/m1-adele/set_rat.jsonl` (one record).
- `lanes/m1-repair-adele/` (logs, checks, this report).

## Red and green runs

- Red (`lanes/m1-repair-adele/checks/lowprec_red.out`): the pre-repair `src/adele.c` (commit
  16feaa0) was put back, `make -j2 check` was run. It failed at
  `tests/test_adele_lowprec.c:194`: `adf_adele_add_rat at prec 1: the result differs from the result
  at prec 2`. The old code then aborted with `Unable to allocate memory (2305843009213693936)` at
  `prec` 0 or -5, because the old code passed the unclamped `prec` to `arb_set_fmpq`. This abort is
  the memory incident that stopped the first attempt. The repaired tree was restored and compared
  with `git diff HEAD -- src/adele.c` (empty).
- Green (`lanes/m1-repair-adele/checks/lowprec_green.out`): `make -j2 check` passed, 31 test
  programs; `build/test_adele_lowprec` itself reports 14 tests, 1426 checks, 0 failed.

## Checks that were run

- `make -j2 check` in the repaired tree: `check passed: all 31 test programs`.
- `make clean && make -j2 check SAN=1` (`checks/san_check.out`): `check passed: all 31 test
  programs`; `grep -c "runtime error\|ERROR:"` gives 0.
- `make clean && make -j2 check CC=clang` (`checks/clang_check.out`): `check passed: all 31 test
  programs`.
- Reviewer reproducer `docs/reviews/m1/arith/checks/adele_prec1.c`, built with
  `sh docs/reviews/m1/arith/checks/build.sh adele_prec1` and run; the full lines are in
  `checks/adele_prec1_after.out`. The values of the three lines:

      prec 1: arb_set_fmpq(1/3) = 0.2500000000 +/- 0.25000
              adele div_rat OK, inf = 3.000000000 +/- 0, is_canonical = 1
              cadele OK, is_canonical = 1
      prec 2: arb_set_fmpq(1/3) = 0.2500000000 +/- 0.12500
              adele div_rat OK, inf = 3.000000000 +/- 0, is_canonical = 1
              cadele OK, is_canonical = 1
      prec 3: arb_set_fmpq(1/3) = 0.3125000000 +/- 0.062500
              adele div_rat OK, inf = 3.000000000 +/- 0, is_canonical = 1
              cadele OK, is_canonical = 1

  At `prec = 1` the result is now `3 +/- 0` (the exact ball of `q` at `prec = 2` is exact for
  `3`), and the real part is finite. The old output is `checks/adele_prec1_before.out` (committed
  by the first attempt).
- Radius comparison, 1000 random inputs at `prec = 53` (`checks/radius_compare.c`,
  `checks/radius_compare.out`):

      mul_rat: 527 smaller, 164 equal, 309 larger, 0 old non-finite
      div_rat: 568 smaller, 224 equal, 208 larger, 0 old non-finite

  The new `mul_rat` and `div_rat` are not uniformly tighter; they are often smaller and sometimes
  larger than the old composition (which is the same value up to the ball chosen). Both enclose, so
  this is not a correctness difference.
- `make mutate FILES=src/adele.c JOBS=2 LIMIT=300` (`checks/mutate_adele_full.log`):
  169 mutants in 2337.2 s, 147 killed, 13 survived, 9 not compiled, 0 timed out, 0 excused.
  This is the full mutant set of the file, so `LIMIT=300` covers all of them.
- Citations were checked against the files on disk with `grep -n` and the reviewer output
  `docs/reviews/m1/arith/checks/citations.out`.

## Mutation survivors

12 are `swap_args`; 1 is `cmp`. All 13 are listed here because
`tools/mutate/equivalent.txt` may not be edited.

1. `src/adele.c:53:17 cmp: '<' -> '<='` in `adele_prec`. This one is equivalent in the strict
   sense: for `prec = 2` both comparisons return 2, and for `prec != 2` they agree. No test can kill
   it; it computes the same clamp for every input.
2. `src/adele.c:218:5 swap_args` `arb_add(z->inf, x->inf, y->inf, prec)`.
3. `src/adele.c:219:5 swap_args` `adf_fball_add(&z->fin, &x->fin, &y->fin)`.
4. `src/adele.c:242:5 swap_args` `arb_mul(z->inf, x->inf, y->inf, prec)`.
5. `src/adele.c:243:5 swap_args` `adf_fball_mul(&z->fin, &x->fin, &y->fin)`.
6. `src/adele.c:279:5 swap_args` `arb_add(z->inf, x->inf, t, prec)`.
7. `src/adele.c:280:5 swap_args` `adf_fball_add(&z->fin, &x->fin, fq)`.
8. `src/adele.c:470:5 swap_args` `acb_add(z->inf, x->inf, y->inf, prec)`.
9. `src/adele.c:471:5 swap_args` `adf_fball_add(&z->fin, &x->fin, &y->fin)`.
10. `src/adele.c:486:5 swap_args` `acb_mul(z->inf, x->inf, y->inf, prec)`.
11. `src/adele.c:487:5 swap_args` `adf_fball_mul(&z->fin, &x->fin, &y->fin)`.
12. `src/adele.c:515:5 swap_args` `acb_add(z->inf, x->inf, t, prec)`.
13. `src/adele.c:516:5 swap_args` `adf_fball_add(&z->fin, &x->fin, fq)`.

Reason for items 2 to 13 (finding R4, the true one): the exchanged operands give another ball that
is also an enclosure of the exact result; `arb_mul` and `acb_mul` are not symmetric and were seen
to give a different radius on random pairs (`docs/reviews/m1/arith/checks/swap_equiv.out`:
`arb_mul differs 38216`, `acb_mul differs 55563` of 200000). The library promises an enclosure, not
a particular ball, and the tests check enclosure and canonicality, not a pinned ball. These mutants
are therefore not killed. `tools/mutate/equivalent.txt` still gives the old line numbers and the
false "same value" reason for them; that file was not touched.

## Line citations used (checked on disk)

- `fmpq_sub` is `/usr/include/flint/fmpq.h:212`; `fmpq_add` is `:200`, `fmpq_mul` `:230`,
  `fmpq_div` `:248`. The installed FLINT header was read directly; it is not under `refs/`.
- `refs/src/flint-3.0.1/arb.rst`: the enclosure sentence is at `:9-11`; the exact-input sentence at
  `:25-26`; `arb_set_fmpq` `:167`; `arb_add` `:767`; `arb_mul` `:798`; `arb_mul_fmpz` `:806`;
  `arb_div` `:856`; `arb_div_fmpz` `:864`.
- `refs/src/flint-3.0.1/acb.rst`: `acb_set_fmpq` `:128`; `acb_add` `:429`; `acb_mul_fmpz` `:457`;
  `acb_mul` `:463`; `acb_div_fmpz` `:517`; `acb_div` `:521`.

## Why the `set_rat.jsonl` record is right

`3/4 = 3 * 2^-2`. The odd mantissa 3 has 2 bits. Decision M1-D4 takes `prec = 1` as 2, and at
`prec = 2` the binary expansion of 3 fits exactly, so `adf_adele_set_rat(3/4, 1)` gives an exact
real ball. The test `adele_set_rat_vectors` compares `arb_is_exact(x->inf)` with the `exact` field
(`tests/test_adele.c:419-420`), so the field must be `true`. The Python generator wrote the
unclamped `prec = 1` answer, `false`; the change makes the one vector agree with the M1-D4
decision. No other record was changed.

## Not done

- The review's R1 (BLOCKER, `adf_adele_reconstruct` with huge exponents), R2 (memory abort in
  `adf_adele_reconstruct`) and R5 (stale excuses of `src/fball.c` in `equivalent.txt`) belong to
  other lanes and were not repaired here. R5 needs `tools/mutate/equivalent.txt`, which this lane
  may not edit.
- The complex-coordinate fuzz of `adf_cadele` beyond `prec = 1` (the review's "Not examined") was
  not added. `tests/test_adele_lowprec.c` covers `adf_cadele` `add`, `sub`, `mul`, `set_rat` and
  the three rational operations at the listed precisions, with aliasing of the output with the
  input.
- The lowprec test checks the output aliased with the input (first input for the binary
  operations). Output aliased with both inputs at normal precision is covered by
  `tests/test_adele.c` `aliasing_of_add_sub_mul` (line 636).

## Findings against the specification

- The Python oracle `tests/ref/vectors/m1-adele/set_rat.jsonl` is generated with the raw `prec`;
  decision M1-D4 clamps `prec` below 2 in the C library. For `prec = 1` the two disagree on
  exactness, which is why one vector was edited by hand. The generator in
  `lanes/m1-adele/gen_adele_vectors.py` is not in this lane and was not changed. Any other
  `prec < 2` set_rat vector would need the same treatment; the file has no other such record.
- `tools/mutate/equivalent.txt` lists the four commutative swaps of `src/adele.c` with the reason
  "the same value is written" (`:54`, `:58`, `:61`, `:65`). As the reviewer's R4 shows and the full
  mutant run confirms, `arb_mul` and `acb_mul` are not symmetric, so that reason is false. The
  entries also name line numbers that have moved after the M1-D4 repair. The file was not touched
  because the brief forbids it; this report is the record.

## Sources pending

None: every cited line was read from a file on disk (`refs/src/flint-3.0.1/*.rst`,
`/usr/include/flint/fmpq.h`).
