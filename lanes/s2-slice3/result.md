# Lane s2-slice3: real roots (milestone S, S.2, slice 3)

## What was done

- `adf_roots_real(L, f, prec)`, Algorithm RR of `docs/proofs/solvers.md` 3.10. Steps: `f = 0` gives DOMAIN;
  `prec` above `ADF_ROOTS_REAL_PREC_MAX` (2^21) gives LIMIT before any allocation; a `prec` below 2 becomes 2;
  `g = g*`, the normalised polynomial; count = `fmpz_poly_num_real_roots(g)` (trusted, S-D11); the candidates are
  the enclosures of `arb_fmpz_poly_complex_roots` whose imaginary part is exactly zero (not trusted); a candidate
  that fails the exact test of P3.8 is widened once (radius doubled, or `2^-prec`); then, on the stored balls:
  the test of P3.8, `hi_i < lo_(i+1)`, number = count, and `arb_rel_accuracy_bits >= prec` or exact (S-D19).
  Any failure gives NOT_DETERMINED with `L` untouched. A candidate that is not of admissible size gives LIMIT.
- `adf_rootlist_get_arb`, and the real place of `adf_rootlist_is_canonical`, `adf_rootlist_verify_entries`
  (P3.13(4)) and `adf_rootlist_verify_complete` (P3.13(5)). The complete verifier recounts from `f`
  (`normalise`, then `fmpz_poly_num_real_roots`). It never reads the count from the list. The three TEMPORARY
  places of the header and of `src/roots.c` are replaced.
- Every sign that decides anything is the sign of an integer. The end points come from
  `arb_get_interval_fmpz_2exp`. The sign of `g` at `m 2^e` is computed by a homogeneous Horner rule in `fmpz`.
  Two end points are compared with `arf_cmp` on exact `arf_set_fmpz_2exp` values. A ball is "of admissible
  size" (header, "Real balls"): finite, a midpoint mantissa of at most 2^24 bits, and midpoint and radius 0 or
  strictly between `2^-2^24` and `2^2^24`. Only then are its end points formed, which answers FLINT's warning
  about memory (`arb.rst:468` to `477`).
- One hidden function (not exported, `ADF_ROOTS_HIDDEN`): `adf_roots_real_finish`, steps 5 to 7 on candidates
  that the caller gives. It is the only way to reach the widening (P3.10(4)) and a disagreement of the counts,
  because FLINT produces neither.
- The code of the primes is unchanged. `verify_complete` now tests the real place before `depth < 0`; at a
  prime the order of the tests is the same as before.

## Files written

- `include/adelefeld/roots.h`: additions ("Real balls", `ADF_ROOTS_REAL_PREC_MAX`, the real place of the
  predicate and of both verifiers, `adf_roots_real`, `adf_rootlist_get_arb`); the TEMPORARY texts replaced.
- `src/roots.c`: the section "slice 3" at the end; `is_canonical_real` and the two real branches of the
  verifiers.
- `tests/test_roots_real.c` (new, 8 tests); `tests/ref/vectors/s2-slice3/real.jsonl` (new, 163 lines);
  `tests/julia/roots.jl` (a third testset); `tests/fuzz/diff_roots_real.py` (new).
- `lanes/s2-slice3/`: `gen_vectors.py`, `bite.py`, `redgreen.log`, `temporary-checks.patch`, `fuzz180.out`,
  `julia.log`, `check-all-tree.log`, `check-all.log`, `check-san.log`, `check-clang.log`, `check-headers.log`,
  and this file.

## Checks (command, result)

- Red: `timeout 300 ./build/test_roots_real` against stubs: 8 tests, 2843 checks, 1510 failed, 8 failed tests.
- Green: `timeout 300 ./build/test_roots_real`: 8 tests, 10666 checks, 0 failed; 30.6 s, of which 28.9 s is one
  vector line (see the FLINT findings). What the tests check:
  - REAL_CASES: 72 runs (the 18 cases at prec 2, 20, 53, 200), 72 lists true by the test's oracle. The oracle
    is a squarefree part over Q with `fmpq_poly`, a Sturm chain over Q built in the test, and signs by
    `fmpz_poly_evaluate_fmpq`; FLINT's count is not used. A run fails when any of these fails: n = count =
    the Sturm count; the test of P3.8 at the exact end points; exactly one root in each closed ball;
    `hi_i < lo_(i+1)`; no root in a gap or on the two outer rays; the accuracy; the predicate and both
    verifiers accept.
  - Planted roots at prec 2, 20, 53: repeated roots, 10^30 and -1/10^20, two roots 2^-40 apart, `X - 10^400`,
    Wilkinson 20, and 0 (double), 1/2, -3/4, 5/8 (double), 2^-30, 1/3. Each planted root must lie in exactly
    one ball, and each ball must hold exactly one planted root. Only the ball of 0 was exact (1 of 6), at each
    prec.
  - Precision: 6 polynomials at prec 2, 10, 53, 200, 2000. For prec 1, 0, -5 and `WORD_MIN` the balls are
    `arb_equal` to those at prec 2. There were 108 nested pairs of balls at growing prec. `X^2 - 2` works at
    `ADF_ROOTS_REAL_PREC_MAX`.
  - Verifiers: all 18 results are accepted, at depth 0, -1 and `WORD_MAX`. Changed lists, each checked
    against the oracle (entries may accept only what it proves, complete never accepts a changed list):

    | kind of change | lists | refused | refused by entries |
    |---|---|---|---|
    | ball removed | 51 | 51 | 0 |
    | two balls merged | 24 | 24 | 24 |
    | ball moved off its root | 15 | 15 | 15 |
    | ball widened over two roots | 12 | 12 | 12 |
    | false count | 33 | 33 | 33 |
    | false n | 15 | 15 | 15 |
    | g replaced by f | 4 | 4 | 4 |
    | one root twice | 12 | 12 | 12 |

    `[0, 4]` for `(X-1)(X-2)(X-3)` passes entries and fails complete (also with count 3). `[1, 1 + 6/1024]`
    and `[1 - 6/1024, 1]` for `X - 1` are refused (a root at an end point). The exact ball `[1, 1]` is
    accepted.
  - Predicate: shape changes, balls exchanged, balls that touch, infinite and NaN balls, and balls outside the
    admissible size are refused, with no abort.
  - Finish: the P3.10(4) example (`X - 1`, prec 8, `1 + 3/1024 +- 3/1024`) gives NOT_DETERMINED with L
    untouched, and OK at prec 7 with the widened ball stored. The widening from `2^-12` to `2^-11` gives OK.
    Exact candidates widened by `2^-prec` give NOT_DETERMINED (accuracy 7) or OK (accuracy 8). Wrong count,
    dropped ball, exchanged balls and a doubled ball give NOT_DETERMINED; an inadmissible candidate gives LIMIT.
  - Statuses: DOMAIN and LIMIT leave L untouched (`memcmp` of the struct plus deep contents). Constants give
    the empty list. Aliasing `f = L->g` works, also from a list at 7. `get_arb` covers aliasing, out of range
    and the prime place.
  - Vectors: 163 lines (161 OK with 385 roots, 2 DOMAIN). A line fails unless g, reduced and n = count agree,
    C ball i meets reference enclosure j exactly when i = j (the enclosures are at 100 bits), the accuracy
    holds, and the verifiers accept.
- Prime tests, unchanged (in the tree): `test_roots_forged` 2 tests, 26 checks, 0 failed. `test_roots_seed`
  5347209 checks, 1 failed (line 400). `test_roots_padic` 55131 checks, 1 failed (line 1237). These two failures
  were expected: see finding 1.
- Item 5 (`python3 lanes/s2-slice3/bite.py <i>`, three calls, each under 3 min): 6 of 6 changes caught.
  - The squarefree part not taken: FLINT aborts, "non-squarefree polynomial in _fmpz_poly_num_real_roots".
  - The sign test accepting a zero: 9 failed checks.
  - The disjointness test removed: 30 failed checks.
  - The comparison of the counts removed: 5 failed checks.
  - The accuracy measured before the widening: 3 failed checks.
  - `verify_complete` reading the count from the list: 54 failed checks.
  - The first run of change 3 did not build (-Werror, unused parameter); its second run is counted.
- `timeout 200 python3 tests/fuzz/diff_roots_real.py --seconds 180 --seed 1`: 2211 calls (2160 OK, 51 DOMAIN),
  5691 roots, 440 exact balls, 1137 reduced inputs, 0 disagreements. This is a smoke test of 180 s, not a long
  run.
- `python3 tools/memcheck/check_uninit.py src/roots.c tests/test_roots_real.c`: 0, 0, 0 findings.
- `julia tests/julia/roots.jl build/libadelefeld.so` (with `LD_PRELOAD` of the system libgmp): 10, 20 and 13
  checks pass (log `julia.log`). The new testset: `X^3 - 2X` to 100 bits; midpoint and radius read through
  `arb_get_mid_arb` and `arb_get_rad_arb`; the signs checked in BigInt.
- Item 6, in the tree as it is: `make clean && make check-all` gives "check FAILED", exit 2
  (`check-all-tree.log`). The only failed checks are `test_roots_seed.c:400` and `test_roots_padic.c:1237`;
  every other program of the 55 passes.
- Item 6, in a scratch copy of the tree with only those two lines changed as `temporary-checks.patch` proposes:
  - `make clean && make check-all`: "check-all passed: make check, driver, exports, julia, mutate-selftest,
    memcheck-selftest".
  - `make clean && make -j2 check SAN=1`: "check passed: all 55 test programs" (test_roots_real 10666 checks,
    0 failed).
  - `make clean && make -j2 check CC=clang`: "check passed: all 55 test programs".
  - `sh lanes/m1-headers/check_headers.sh`: "check_headers: passed" (roots.h: 21 declared functions).
- `sh tests/test_exports.sh`: 260 of 260 declared functions exported; `adf_roots_real_finish` stays hidden.

## Not done

- `set`, `swap` and `identical`, the route of P3.7(2), and benchmarks (not in this lane).
- No mutation run (replaced by item 5).
- The two checks of finding 1 are not changed: the files are not mine.
- I did not count how often a FLINT ball is widened in the fuzz run.

## Header findings (HEADER-FINDING, decisions of this lane in roots.h)

1. `ADF_LIMIT` for `adf_roots_real`: `docs/api-s.md` 4 lists only OK, NOT_DETERMINED and DOMAIN. Without a bound,
   a `prec` of `WORD_MAX` would make FLINT allocate without end. The bound is `ADF_ROOTS_REAL_PREC_MAX = 2^21`
   bits, which is a policy. LIMIT is also returned for a FLINT candidate that is not of admissible size; no
   test input produced one.
2. The admissible size of a real ball (bound 2^24, a policy like S-D18). The predicate and the verifiers give 0
   for any other ball.
3. `depth` is not used by `verify_complete` at the real place. api-s is silent on this. Before this lane,
   `depth < 0` gave 0 at every place.
4. At the real place the entries verifier checks that the shape is consistent (PARTITION, complete = 1,
   `n = count`), as it does at a prime. It does not check that `count` is true. So a false count is refused by
   entries, and a list that is only short is refused by complete.

## Findings against the specification, solvers.md, sources.md

1. Conflict in the brief. `tests/test_roots_seed.c:400` asserts `verify_entries(init list, f = 1) == 0`
   ("the real place: 0 in this slice"). `tests/test_roots_padic.c:1237` asserts
   `verify_complete(init list, f = 1, 3) == 0` ("TEMPORARY"). The brief says these files must pass unchanged,
   and also that the TEMPORARY places are replaced. For `f = 1` the init list (empty, `g = 1`, count 0) is the
   true complete list, so the new header requires 1 for both. I did not weaken the code. The two-line change is
   in `lanes/s2-slice3/temporary-checks.patch`. With it, every check of item 6 passes.
2. FLINT cost. `arb_fmpz_poly_complex_roots` takes about 28 s on `(X - 10^400)(X - 10^400 - 1)` at prec 6 and at
   prec 53, even though the root separation needs only about 1330 bits. The balls have midpoints of 2^21 bits
   and accuracy 1456. Higher precisions were not measured: the timing program hit its 110 s timeout before it got
   to prec 3000. This is correct but slow, and it
   is in line with `arb_fmpz_poly.rst:103` to `105` ("not competitive"). The probe of P3.9 has `X - 10^400` but
   not this pair. For inputs with close roots of large size, the cost of `adf_roots_real` is not bounded by
   anything in the header.
3. P3.9(1) and the row at `docs/sources.md:336` are confirmed. With `f` in place of `g`, FLINT aborts on the
   first non-squarefree case of the test ("non-squarefree polynomial in _fmpz_poly_num_real_roots"). That is,
   by the order of the cases, `(x-1)^2(x+3)`: a cubic whose rest has discriminant 0.
4. Dyadic roots. FLINT returned an exact ball only for the root 0 (it removes the factor X,
   `complex_roots.c:71` to `73`, `:158` to `159`). The roots 1/2, -3/4, 5/8 and 2^-30 got inexact balls. The
   brief's "(exact balls)" holds for 0 only. The test requires at least 1 exact ball, which is the root 0.
5. Nesting for growing `prec` is not promised by api-s.md or by FLINT. The test checks it because the brief asks
   (108 pairs held). A failure there would not be a violation of the contract.
6. Notation. In P3.13(5) "the n of the list" is the field `count` of the C struct; the struct's `n` is the
   number of balls. The verifier checks both against the recount.
7. Avoidable costs (not optimised):
   - `real_finish` tests each unwidened ball twice (before and after the widening step).
   - `verify_complete` at the real place normalises `f` twice (once in entries, once for the recount).

No `[source pending]` marks. The one statement without a source on disk is Sturm's theorem, as P3.9 already says.
