# Lane m1-recon: rational reconstruction from a full ball (work package 1.6)

## What was done

`adf_fball_reconstruct` and `adf_adele_reconstruct` of `include/adelefeld/recon.h` are implemented in
`src/recon.c`, with the tests of the rules of `lanes/COMMON-C.md` in `tests/test_recon.c` and
`tests/test_recon_vectors.c`, the extra vectors of the lane in `tests/ref/vectors/m1-recon/` written by
`lanes/m1-recon/gen_vectors.py` from the Python reference, and one benchmark row (PLAN row 1.6, provisional)
in `bench/bench_recon.c`, registered in `bench/Makefile`.

The algorithm is the one of `docs/proofs/quotient.md` Proposition 11 (line 257) and nothing else. For
`N > 0` the candidates are `a + N k` for `ceil((lo - a)/N) <= k <= floor((hi - a)/N)`; the two bounds are
computed with the integer divisions `fmpz_cdiv_q` (towards `+infinity`) and `fmpz_fdiv_q` (towards
`-infinity`) of the fraction in lowest terms with a positive denominator, compared as integers, and the
candidate `a + N kmin` is formed when they are equal. For `N = 0` the ball is the point `a` and is tested
against the closed interval. For the adele the closed interval is the exact dyadic interval of the `arb`,
read with `arb_get_interval_fmpz_2exp` (`refs/src/flint-3.0.1/arb.rst:461-466`, its abort condition at
`arb.rst:468-470`). Nothing is enumerated, so the number of candidates is never computed as a count and
never overflows; `ADF_LIMIT` is never returned and the class of conventions 3.2 allows the reconstruction
only to return `OK`, `NO_SOLUTION` or `NOT_UNIQUE`, which is what it does.

## Files written

| File | Content |
|---|---|
| `src/recon.c` | the two functions and two static helpers (222 lines) |
| `tests/test_recon.c` | 19 tests, 52740 checks (unit tests and the enumeration against the code) |
| `tests/test_recon_vectors.c` | 3 tests, 21194 checks: `recon.jsonl` completely, and the two files of the lane |
| `tests/ref/vectors/m1-recon/recon_edges.jsonl` | 239 lines, the format of `recon.jsonl` |
| `tests/ref/vectors/m1-recon/recon_adele.jsonl` | 78 lines, a real ball as `mid_num`, `mid_exp`, `rad_exp` |
| `lanes/m1-recon/gen_vectors.py` | writes the two vector files from `adfref/recon.py` (deterministic) |
| `lanes/m1-recon/log.txt` | the red and green runs |
| `bench/bench_recon.c` | 8 rows (chain and batch, word and 4096 bit, both functions) |
| `bench/Makefile` | one added target `bench_recon`; the brief allows this change |
| `bench/results/2026-09-28T083803Z_recon.txt` | the result quoted below, written by the harness |

## Checks that were run, with their results

Red-green. `lanes/m1-recon/log.txt` has the runs in full; the short version:

- red: `make -j2 build/test_recon build/test_recon_vectors` after the two test files were written and
  before `src/recon.c` existed: both files compile clean and fail at the link with
  `undefined reference to adf_fball_reconstruct` and `undefined reference to adf_adele_reconstruct`.
- green: `make -j2 && make check` after `src/recon.c`: `check passed: all 11 test programs`, with
  `build/test_recon` at `19 tests, 52740 checks, 0 failed checks` and `build/test_recon_vectors` at
  `3 tests, 21194 checks, 0 failed checks`.
- green again after the ball was changed to be read through `adf_fball_get_fmpz3` (see "The local
  backend" below) and after the removal of a dead store: the same numbers.
- `make clean && make -j2 check SAN=1`: `check passed: all 11 test programs`, no leak and no
  undefined-behaviour report.
- `make clean && make -j2 check CC=clang`: `check passed: all 11 test programs`.
- `python3 lanes/m1-recon/gen_vectors.py`: `recon_edges.jsonl: 239 lines`,
  `recon_adele.jsonl: 78 lines`.
- runtime of the two test programs: `build/test_recon` 0.48 s, `build/test_recon_vectors` 0.03 s.
- mutation: `make mutate FILES=src/recon.c JOBS=2`, 35 mutants, 148.5 s, `30 killed, 2 survived,
  3 not compiled, 0 timed out, 0 excused`. See "Mutation" below.

The three mutants that do not compile are `drop_assign` of `status = adf_rat_set_fmpz2(a, A, d);`
(then `status` may be used uninitialised), `drop_assign` of `status = adf_fball_reconstruct(...)` in
`adf_adele_reconstruct` (then `q` is unused) and `op` `&` to `|` on `&x->fin`; they are compile errors
under `-Werror`, not results.

## What the tests cover

The oracle is written in the test files and never calls the code under test. There are two oracles, and
they are different in kind: an enumeration of the candidates `a + N k` compared one by one with the two
end points, and the formula of Proposition 11 computed with `fmpq` and integer floor and ceiling
division. The committed vector file is the third oracle.

- the whole of `tests/ref/vectors/recon.jsonl` (360 lines), and the whole of the two files of the lane;
- none, one and several candidates, and the state of the output on both failure statuses
  (conventions 4.3): every case in `tests/test_recon.c` goes through `run_fball` or `run_adele`, which
  put a marker `-99/7` in the output before the call and require it to be bitwise unchanged unless the
  status is `ADF_OK`; the vector tests do the same for every one of the 679 lines;
- both end points of the closed interval, M0-D3: a candidate equal to `lo` and a candidate equal to `hi`,
  for the finite-ball call (`the_end_points_of_the_closed_interval_are_candidates`, and the pairs in
  `one_candidate_of_a_ball_of_radius_one_sixth`, `an_exact_finite_ball_gives_its_centre`,
  `negative_centres`) and for the adele call (`the_end_points_of_the_real_interval_are_candidates`);
- `lo > hi` (the empty interval), for `N > 0` and for `N = 0`; `lo = hi` throughout;
- `N = 0`: inside, at the left end point, at the right end point, at both, and outside on each side, for
  the points `0`, `2`, `3/7` and `-5/6`;
- a radius with a denominator: `(0, 1, 6)` (radius `1/6`), `(3, 5, 6)` (`5/6`), `(1, 7, 3)` (`7/3`),
  `(1, 4, 3)` (`4/3`);
- negative centres: `(1, 2, 3)` is the set `1/3 + (2/3) Zhat` and also `-1/3 + (2/3) Zhat`, `(1, 3, 2)` is
  `1/2 + (3/2) Zhat`, `(0, 1, 4)` is `(1/4) Zhat`; the negative candidates `-5/3`, `-1`, `-1/4` are tested;
- an interval of width exactly the radius (`[1/2, 3/2]`, `[1/3, 4/3]`, `[0, 2/3]` against `N = 2/3`) and
  just below it (`[1/2, 5/4]`, `[1/4, 1]`, `[1/12, 5/12]`, `[0, 7/12]`);
- an `arb` of radius 0: `an_adele_with_a_real_ball_of_radius_zero` builds the real ball with
  `arb_set_fmpq` of a dyadic number at 64 bits, requires `arb_is_exact`, and then the interval of the
  adele is the single point;
- operands of 4096 bits: `operands_of_4096_bits` (the finite ball with `A`, `H`, `d` of 4096 bits and
  seven intervals, one per status and two per side of the centre) and
  `an_adele_with_operands_of_4096_bits` (the finite ball with `A` of 4096 bits, `H = 2^3100`,
  `d = 2^4000`, and a real ball of a 4096-bit midpoint, in three widths);
- enumeration: `the_enumeration_of_candidates_agrees_with_the_function` runs the 7 * 5 * 3 raw triples
  `A` in `-3..3`, `H` in `0..4`, `d` in `1..3` against the 12 * 12 pairs of the end-point grid, and for
  every pair requires `ADF_OK` exactly when the enumeration finds one candidate, and checks the value
  when it does. That is 105 * 144 = 15120 cases inside 52740 checks. The grid bounds (`|lo|, |hi| <= 4`,
  every radius at least `1/12`) are stated in the test and are what makes the enumeration `k` in
  `[-64, 64]` complete; the vector tests instead count by sweeping `k` outwards from `0` and `-1` until a
  candidate is outside the interval, which needs no bound at all and copes with the 201 candidates of
  `recon.jsonl`;
- aliasing: `the_output_may_be_one_of_the_interval_end_points` runs the call with `q` equal to `lo`, with
  `q` equal to `hi`, with `lo` and `hi` the same object and `q` that object, and checks the value on
  `ADF_OK` and the untouched marker on both failures. The finite ball is of another type than the output
  and cannot alias it, and for `adf_adele_reconstruct` the only output is an `adf_rat`, so there is no
  other permitted combination;
- the two oracles agree with the enumeration: for every vector line of status `one` the candidate is
  compared with the recorded solution and with the one the enumeration found, and for a line of status
  `several` the recorded list is pinned without repeating the formula: every recorded solution lies in
  the closed interval and in the ball, consecutive ones differ by the radius, the neighbours just
  outside the list are outside the interval, and the length of the list is the count of the enumeration.

Two things are worth stating because they are easy to get wrong and the tests pin them: a closed interval
of width exactly `N` can hold one candidate or two (`[1/2, 3/2]` holds `1`, `[0, 1]` holds `0` and `1`),
and the stored centre of a canonical ball lies in `[0, N)`, so the ball `1/2 + (1/6) Zhat` is stored as
`(0, 1, 6)` and its candidates are the multiples of `1/6`; both are in the tests with a comment.

## The closed interval of the `arb`, and how the tests check it

`src/recon.c` reads the end points with `arb_get_interval_fmpz_2exp`, whose documented meaning is the exact
interval of the ball, `x = [A, B] * 2^exp` (`refs/src/flint-3.0.1/arb.rst:461-466`); the two integers are
then shifted into a rational by the power `exp`. The tests do not trust this: every real ball that a test
builds is read back through a different route, `arb_get_interval_arf` at 256, 4096 or 8192 bits followed by
`arf_get_fmpq` on the two end points, and compared with the interval the test asked for. That check caught
three wrong expectations of mine during the work (a real ball of radius `2^-4100` around a midpoint of
4192 bits is a point, because the radius is below the unit in the last place of the midpoint, and the
intervals I had chosen were not the ones I meant), so the two routes are independent in the test as well.

The real balls of the tests are built with `arb_set_fmpz_2exp` for the midpoint (exact, radius 0,
`/usr/include/flint/arb.h:187`) and `mag_set_fmpz_2exp_fmpz` for a radius that is a power of two
(`/usr/include/flint/mag.h:479`, exact for a one-limb mantissa), through the accessor macros
`/usr/include/flint/arb.h:38-39`. The result is checked, not assumed.

## The local backend

The local backend is work package 1.8 and does not exist, so no local input can be built in the tests. The
code reads the ball through `adf_fball_get_fmpz3` alone, the accessor of `fball.h` whose header documents
the triple of a local value as well (`A0/g`, `K/g`, `d/g`, conventions 5.3, policies P24.1 and Lemma 18),
and forms the centre and the radius from that triple with `adf_rat_set_fmpz2`. The other two accessors,
`adf_fball_get_center` and `adf_fball_get_radius`, are not used: as `src/fball.c` stands they read the
stored fields `A/d` and `H/d`, which for a raw local value are the centre `0` and the radius `0` (its own
report records this as a HEADER-FINDING), so they would answer about the point `0` for every local ball.
`adf_fball_get_fmpz3` in `src/fball.c` likewise returns without writing for a local value today, so the
reconstruction of a local ball is only correct once that lane implements the documented CRT recombination;
no change in `src/recon.c` will then be needed. The set of candidates does not depend on the
representative of the centre, because conventions 5.2 says that any element of `a + N Z` is an equally
valid centre of the same set, so the global and the local form of one ball give the same answer.

The cost of the accessor is a copy of three integers for the global backend and, as its header says, a CRT
recombination and a gcd for a local one.

## Aliasing

`recon.h` says "the output is an adf_rat and aliases no input", while conventions 4.1(1) says that an output
may be the same object as any input of the same type. The stricter reading is the one that forbids
aliasing, the looser one allows it; the implementation satisfies both: every input is read into a
temporary, and the output is written last, only on `ADF_OK`. The tests use the looser reading.

## Mutation

`make mutate FILES=src/recon.c JOBS=2` produces 35 mutants, in 148.5 s on this machine. A run with
`JOBS=1` took 942 s, which is over the three minutes a laptop run should take; the numbers below are
from the two-core run, and the run above it was repeated five times because the tool printed a different
set of survivors each time. Repeated runs reported between two and five
survivors (the tool is not stable in which survivors it prints; the union over five runs is five). Each of
the five was then applied to the tree by hand, the library and the two test programs were rebuilt, and
every test still passed (52740 and 21194 checks, 0 failures in each case), which confirms that they are
equivalent mutants and not claims the tests do not make. The five, with the current line numbers of
`src/recon.c`:

```
src/recon.c:90:23  cmp       | fmpz_sgn(exp) >= 0 as > 0: for exp = 0 the two
                            |   branches compute the same rational mn/1.
src/recon.c:90:26  zero_one  | the same token, 0 to 1: >= 0 as >= 1, the same
                            |   value for exp = 0, which is the only difference.
src/recon.c:122:34 zero_one  | fmpq_cmp(lo, hi) > 0 as > 1: fmpq_cmp returns -1, 0 or 1, so the
                             |   early return for the empty interval disappears. The rest of the
                             |   function still answers ADF_NO_SOLUTION when lo > hi: for N = 0 the
                             |   test lo <= a <= hi cannot hold when lo > hi, and for N > 0 the
                             |   lower bound is then strictly above the upper one.
src/recon.c:176:13 swap_args | adf_rat_mul(c, c, N) as adf_rat_mul(c, N, c): the product of two
                            |   rationals does not depend on the order.
src/recon.c:177:13 swap_args | adf_rat_add(c, c, a) as adf_rat_add(c, a, c): the sum does not.
```

`tools/mutate/equivalent.txt` is the file for these entries, but it is not in the list of paths this lane
owns, so I did not write it; the target `make mutate FILES=src/recon.c` therefore reports these five as
survivors until the five lines are added. The first two lines above are a judgement call: they could be
removed from the code instead by writing the sign test once, at the price of a clearer function.

One earlier survivor was a dead store, `status = ADF_OK;` after the two `adf_rat_set_fmpz2` calls, which
re-set a value that the two `goto done` guards had already checked; it was removed from the code rather
than excused.

## Benchmark

`./bench/bench_recon --run --trials 3`, written to `bench/results/2026-09-28T083803Z_recon.txt`, pinned to
cpu 2, `quiet_machine: no` (other lanes were running). ns per operation, min and median of three trials:

| row | kind | min | median |
|---|---|---|---|
| recon_fball_chain_word | latency chain | 3531.8 | 3685.1 |
| recon_adele_chain_word | latency chain | 9254.9 | 9619.1 |
| recon_fball_chain_4096 | latency chain | 6537.9 | 6554.9 |
| recon_adele_chain_4096 | latency chain | 119568.4 | 126493.5 |
| recon_fball_batch_word | independent batch | 3667.9 | 3687.2 |
| recon_adele_batch_word | independent batch | 9510.3 | 9629.8 |
| recon_fball_batch_4096 | independent batch | 6946.6 | 7029.0 |
| recon_adele_batch_4096 | independent batch | 111073.7 | 113796.2 |

The row is provisional, as the brief asks. Two observations for the report of the row:

- the adele call costs about 2.7 times the finite-ball call on the same operands, and 18 times more at
  4096 bits. The difference is the reading of the end points: `arb_get_interval_fmpz_2exp` builds two
  integers of about 5100 bits and then two rationals out of them, once per call. Nothing in the
  reconstruction itself is expensive.
- the cost of one word-sized call, about 3.7 microseconds, is dominated by the temporaries: the call
  initialises and clears five `adf_rat` and three `fmpz`, and `adf_rat_set_fmpz2` reduces twice. A cheaper
  version would work directly on the `fmpq` of the temporaries and would skip the second reduction by
  forming `a = A/d` and `N = H/d` once. That is an optimisation, and COMMON-C rule 7 asks for the simple
  correct version, so it was not done.

## Findings against the specification

None. Everything in SPEC 9.2 first item, conventions 6.8 (CV-51, D3) and quotient.md P11 is implemented
and tested as written. Three remarks, none of them a defect:

1. conventions 3.2 gives the class "Reconstruction and solvers" the statuses `OK`, `NO_SOLUTION`,
   `NOT_UNIQUE`, `NOT_DETERMINED` and `LIMIT`. A reconstruction from a full ball returns the first three
   and never the last two: `NOT_DETERMINED` is reserved by SPEC 9.2 for the problem from partial data,
   and `LIMIT` has nothing to limit, since the candidates are never enumerated. This is what
   `recon.h` says as well.
2. `adf_adele_reconstruct` answers `ADF_DOMAIN` when the real ball is infinite or NaN. That input is
   outside the contract (conventions 5.5 requires `arb_is_finite(x->inf)`, and conventions 4.4 makes a
   non-canonical input undefined). The check is there because `arb_get_interval_fmpz_2exp` aborts on such
   a ball (`arb.rst:468-470`), and answering is better than aborting inside FLINT. The test
   `an_adele_with_an_infinite_real_ball_is_rejected` says in its comment that the case is outside the
   contract.
3. `recon.h` says of the aliasing "the output is an adf_rat and aliases no input", which is stricter than
   conventions 4.1(1). See "Aliasing" above: the implementation allows it, so no reading breaks.

## Sources pending

- `[source pending: refs/src/flint-3.0.1/doc/source/arf.rst, for arf_get_fmpq]` and
  `[source pending: the entry of arb.rst for arb_get_interval_arf]`. Only seven files of the FLINT
  documentation are on this machine (arb.rst, acb.rst, acb_dirichlet.rst, fmpq.rst, fmpz_mod.rst,
  nmod.rst, padic.rst, ulong_extras.rst); arf.rst and mag.rst are not, and `refs/` is not a path this
  lane owns, so they were not fetched. Both functions are used in the tests only, as the second opinion
  on the exact interval, and both are declared in the matching headers that `docs/sources.md` records as
  local reference: `/usr/include/flint/arf.h:1125` and `/usr/include/flint/arb.h:355`. What was verified
  instead: `arf_get_fmpq` returns the exact rational of a finite dyadic `arf` in 5000 random cases with
  mantissas of up to 100 bits and exponents in `[-30, 33]` (a scratch program, `arf_set_fmpz_2exp` then
  `arf_get_fmpq` then a comparison with the fraction computed with `fmpz`). The claim of the header of
  `arb_get_interval_fmpz_2exp` is on disk and is cited in the code.
- `[source pending: mag.rst, for mag_set_fmpz_2exp_fmpz]`, used in the tests to build a real ball of an
  exact power-of-two radius; the declaration is `/usr/include/flint/mag.h:479`, and the exactness of a
  one-limb mantissa was verified by reading the interval back (the ball [1, 1] with the radius `1/2` is
  [1/2, 3/2], with `1` it is [0, 2], with `5 * 2^-8` it is [251/256, 261/256]).

## Not done

- No test with a local finite ball, because the backend does not exist; see "The local backend".
- No fuzz target: the two functions read no text and no untrusted input, and COMMON-C rule 2 asks for
  fuzzing of parsers.
- `tools/mutate/equivalent.txt` was not written, for the reason given under "Mutation".
- The partial-data reconstruction of SPEC 9.2 second item is a separate function on a separate type and is
  not in `recon.h`; it is not started.
- The benchmark row is provisional and the numbers come from a machine that was not quiet.
