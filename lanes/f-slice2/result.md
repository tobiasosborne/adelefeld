# Lane f-slice2: result (milestone 1F, WP 1F.1 and 1F.2: `adf_sball` and the real functions)

Date 2026-09-29/30. Worktree at adf87c1 plus my files. I ran no git command that changes state and no `bd`.

## What was built

`include/adelefeld/sball.h`, `include/adelefeld/rfunc.h` (new, both included from `include/adelefeld.h`),
`src/sball.c`, `src/rfunc.c`. 35 functions and the 2 layout queries, all exported (`tests/test_exports.sh`: 362 of 362 declared functions
exported, none undeclared, none variadic).

- `adf_sball` (struct of conventions 5.9, 120 bytes, alignment 8: arch 0, inf 8, len 104, loc 112): `init clear set swap
  is_canonical identical`; the raw constructor `set_arb_lballs(y, where, r, loc, n)` (sorts, rejects repeated primes and
  non-canonical components); `project(y, where, x, places, n)` (an adele to a set of places, any order, the real place
  and primes); accessors `arch num_places get_place has_place get_lball get_arb`; predicates `equal_set overlaps
  contains`; `neg add sub mul` over the same set of places; `adf_sizeof_sball`, `adf_alignof_sball`.
- `rfunc.h`: on an `arb`: `adf_real_exp log log_abs sin cos sqrt root(n)`; at the archimedean place of a partial ball:
  `adf_sball_exp_at log_at log_abs_at sin_at cos_at sqrt_at root_at(n)`. The result of `_at` is a partial ball over the
  one place (SPEC 9.3.1: the other coordinates are not part of the result).
- Every builder computes into a temporary and swaps on `OK`: aliasing is free and a status leaves the output untouched.
  `where` (an `adf_place_t *`, may be `NULL`) is written on a status other than `OK` and untouched on `OK`.
- Statements S1 to S7 with stepwise proofs, the function table and the decisions: `docs/api-1f.md`, new section "Slice
  1F.1-a and 1F.2-a" (the section of slice 1 is unchanged; a paragraph reflow touched only my section).

Statuses: `project`: `OK`, `DOMAIN` (repeated place, `where` = that place), `LIMIT` (`where` = the first prime, in the
canonical order, at which `adf_lball_set_fball` fails). Operations: `DOMAIN` (places or tags differ, `where` = the first
place in one operand only), `UNSUPPORTED` (complex tag, `where` = inf), `LIMIT` (`where` = the first failing prime).
Real functions: `DOMAIN` only when every point of the ball is outside the domain, `NOT_DETERMINED` when the ball meets
the domain and its complement, `OK` inside (log: `hi <= 0` DOMAIN, `lo > 0` OK; log_abs: DOMAIN only for the exact 0;
sqrt and even roots: `hi < 0` DOMAIN, `lo >= 0` OK; odd roots, exp, sin, cos: OK; root of degree 0: DOMAIN). `_at`: place not
in the ball `DOMAIN` (where = v), a prime of the ball `UNSUPPORTED` (where = v), complex tag `UNSUPPORTED` (where = inf).

## Decisions taken (alternatives)

1. Order of places: conventions 7 (inf first, then primes increasing) holds; the brief said "real place last" and yields.
2. Sets of places are an array of `adf_place_t` plus a length. `adf_places_t` is named in conventions 2.2 and 7 but has no
   struct; alternative: define one. Constructors accept any order, sort, and reject a repetition (conventions 7).
3. `where` as a report argument on every status (conventions 2.2, 4.3, 3.3). Alternative: no place for operations.
4. Complex tag: stored, copied, compared; arithmetic and functions return `UNSUPPORTED` with the archimedean place.
   Alternative: `acb` arithmetic (a few lines); not done because no function of the slice can make a complex tag.
5. The odd root of a negative ball is minus the root of the negated ball (orchestrator); a ball containing 0 is enclosed
   by the images of its outer end points (root is increasing, Proposition 14); the exact 0 gives the exact 0. `arb_root_ui`
   is called only on a strictly positive ball (probe: NaN for negative and for 0; the tests bite on it, fault 12 below).
6. A result that `arb_is_finite` rejects is `NOT_DETERMINED`, never stored (conventions 4.4, CV-08). Measured on FLINT
   3.0.1: `exp(2^60)` is OK, `exp(2^1000)` and `exp(2^(2^62))` are NOT_DETERMINED, `exp(-2^1000)` is OK (a ball around 0);
   log, log_abs, sin, cos, sqrt and the cube root are OK at `+-2^60`, `+-2^1000`, `+-2^100000` (the negative
   ones: DOMAIN for log and sqrt), and at `2^(2^62)` for log, log_abs, sin, cos, sqrt. Alternative for exp overflow: `LIMIT`.
7. `prec < 2` is 2 (M1-D4); no upper bound on `prec` is checked (as `adf_adele_mul`).
8. The limits of lane f-slice1 are accepted, as the brief says; `LIMIT` from a component names the prime.
9. `adf_sball_set_arb_lballs` was added (not in the brief's list): without it a binding cannot make a partial ball with
   chosen components, and the tests need it for forged values. `adf_sball_get_lball` and `get_arb` are the component
   accessors (a place not in the ball is `DOMAIN`).
10. "log at the real place" of the Julia example: the user reads the three components of the projection to
    `{2, real, 5}` and then the log result, which is a partial ball over the one place (decision 5 of the section in `api-1f.md`).

## Files written (all under "You own")

`include/adelefeld/sball.h`, `include/adelefeld/rfunc.h`, `src/sball.c`, `src/rfunc.c`, `tests/test_sball.c`,
`tests/test_rfunc.c`, `tests/julia/sball.jl`, `proto/functions_checks.py` (section "f-slice2" before the `__main__` guard,
plus `sball_main()` in the guard; nothing above it changed), `tests/ref/vectors/f-slice2/` (4 files, 949824 bytes:
`rfunc_real.jsonl` 1301 lines, `sball_project.jsonl` 242, `sball_ops.jsonl` 310, `sball_pred.jsonl` 1200), `docs/api-1f.md`
(new section), `include/adelefeld.h` (the two includes and two comment lines), `tests/test_julia.sh` (a call of `sball.jl`
after `lball.jl` on both success paths), and in `lanes/f-slice2/`: `gen_vectors.py`, `check_ref.py`, `plant_ref.py`,
`bite.py`, `bite.log`, `redgreen.log`, `python_checks.log`, `check-all.log`, `check-san.log`, `check-clang.log`,
`progress.md`, this file.

## Red and green (`lanes/f-slice2/redgreen.log`)

- RED 1: `make -j2 build/test_sball build/test_rfunc` with headers and tests but no sources: 386 lines
  `undefined reference`.
- RED 2: a stub of both sources (init and clear only, predicates 0, every other function `ADF_UNSUPPORTED`): `test_rfunc`
  `8 tests, 38134 checks, 6677 failed checks, 7 failed tests`; `test_sball` `16 tests, 723867 checks, 214975 failed
  checks, 16 failed tests`. Every failure an assertion (earlier stub runs that died with SIGSEGV, because a test
  dereferenced the array the stub left NULL, are not counted; guards were added and the run repeated). One test passes
  under the stub, `huge_arguments_never_give_a_nonfinite_ok` (a status with an untouched output is allowed); it bites only on
  a wrong `OK` (fault 21).
- GREEN: `test_rfunc` first run 59145 checks, 0 failed. `test_sball` first run 11 failed checks, all a defect of my test (the
  radius bound ignored that `mag` rounds a radius up by a relative 2^-29); after the repair `16 tests, 873696 checks, 0 failed
  checks`. The Julia test was written after the C code was green and was NOT run red (see "Not done").
- Debug build (`make clean; make build/test_sball build/test_rfunc INV=1`): both green (59145 and 873696 checks).

## What the tests check, and what would make a case fail

`tests/test_sball.c` (16 tests, 873696 checks, 12.3 s: 12.2 s of it in one test, see Findings 1):
- `vectors_project` (242 lines), `vectors_ops` (310), `vectors_predicates` (1200): every line. The expected values are made
  by the Python reference from L1 to L8 at the primes and exact interval arithmetic at the real place, and are written into
  the struct by hand, not by the constructor under test. Lines of the vectors include statuses and the reported place.
  A projection that does not sort, drops the real place or mispairs components fails; a wrong real interval rule fails the
  containment or the radius bound (twice the width for a product).
- `projection_enumeration` (8624 component checks, independent of the reference): for p = 2, 3, 5, 7 and 2156 adeles
  `(A + H Zhat)/d`, every point `A/d + H z/d`, `z < p^2`, lies in the component, and the p classes of these points modulo
  the next digit are all met (a component that is too large or too small fails).
- `tuples_of_points_through_the_operations` (400 random pairs of partial balls over `{inf, 2, 3, 5, 7}`, 4 operations, 6 random
  tuples of points each, 48000 coordinates): the tuple of results lies in the result component at every place. It fails if
  the components of the two operands are mispaired (fault 2) or a real rule is wrong (faults 5, 7).
- `is_canonical_rejects_each_clause` (arch out of range, imaginary part of a real tag, inf of the empty tag, each
  non-finite kind, `len < 0`, `len > 0` with NULL, non-canonical component, primes equal, decreasing), `set_arb_lballs`,
  `accessors`, `layout_and_init` (offsets 0, 8, 104, 112), `set_swap_identical`, `predicates_by_hand` (direction of
  `contains`, closed intervals meeting at an end point, complex boxes, tags, different places), `project_orders_and_statuses`
  (all 6 orders of three places, n = 0 with NULL, repeated prime and repeated inf), `domain_unsupported_and_where` (17 rows of
  place sets and tags, 4 operations each), `limit_names_the_first_failing_prime` (forged components `5^(2^30)`,
  `1 + O(3^(2^40))`: the output untouched, also aliased, `where` = 5, then 7, then 3, 11), `project_limit_names_the_first_failing_prime`
  (an adele with `H = 6^(2^25 + 1)`: `where` = 2, then 3, then no failure), `the_example_of_the_brief` (2/3 projected to
  `{2, 5, real}`; the product of the projection with itself equals the projection of 4/9; `x + (-x)` is the exact 0 at the primes).
- Aliasing of every binary operation in the three combinations `(x, x, y)`, `(y, x, y)`, `(x, x, x)`, at `OK` and at a status,
  is run on every vector line of `sball_ops.jsonl`.

`tests/test_rfunc.c` (8 tests, 59145 checks, both levels: `arb` and partial ball, for every line):
- `vectors_real` (1301 lines, 7 functions, roots of degree 1 to 2^20, precisions -3 to 300): the status (decided on exact
  end points); the output untouched on a status; on `OK` the result contains the bounds of the image widened by
  `2^-350` of the larger bound; where the line says tight (758 of the 1116 `OK` lines; 91 lines are `DOMAIN` and 94 `NOT_DETERMINED`), the radius is at most 4 times the true width
  plus `2^(5 - prec)` of the larger end point; for sin and cos otherwise the radius is at most the input radius plus
  `2^(4 - prec)`. The bounds come from mpmath intervals at 800 bits (exp, log, sin, cos, including the critical values of sin
  and cos inside the ball) and from exact integer roots (sqrt, roots), rounded outward to 400 bits. Aliased result equal to the
  unaliased one; the partial-ball result equal to the arb result, `where` = inf on a status.
- `odd_roots_of_negative_and_zero`, `non_finite_inputs` (NaN, +inf, -inf, `[0 +- inf]` under every function: DOMAIN, output
  untouched), `huge_arguments_never_give_a_nonfinite_ok`, `prec_below_two_is_two` (identical to prec 2, for prec 1, 0, -1, -100,
  LONG_MIN + 1), `domain_table` (23 rows of corners: exact 0, the end point 0, a ball across 0), `known_values` (e, log 2, sin 1,
  cos 1, sqrt 2, cube roots of 27 and -27, the fourth root of 16 without -2), `sball_at_statuses_and_places` (the order of the
  checks on v, complex tag, tag NONE, NULL `where`, y = x, the degree 0 with `where` untouched, the value `-0.405465108108164`
  of log 2/3).

Python (`python3 -B proto/functions_checks.py`, 15 s, exit 0; the earlier 28 planted wrong rules of the file are still all
rejected): `check_sball_projection_enumeration` 600 cases, `check_sball_tuple_enumeration` 21600 tuple coordinates,
`check_real_reference_against_arb` 200 cases (the reference bounds against python-flint's `arb` at 300 bits at both end
points). `lanes/f-slice2/plant_ref.py`: 4 planted faults in the reference (sub uses `lo - lo'`, mul uses `lo lo'` and `hi hi'`,
projection one digit too fine, exp upper bound too small), 4 of 4 rejected (the first version of the arb check let the last
one through; it was strengthened to test both end points and rerun). The vector files are reproduced byte for byte by a second
run of `gen_vectors.py`.

`tests/julia/sball.jl` (35 tests, run by `tests/test_julia.sh`, last line `test_julia: passed (with LD_PRELOAD=...)`): the
adele of 2/3 at 200 bits projected to `[2, real, 5]` (the order of the result is real, 2, 5); the components read back (exact
2/3 = 2^1 (1/3) at 2, exact 2/3 at 5, the real ball around 2/3 within 2^-190 in BigFloat); log at the real place
(-0.4054651081...), the statuses and places (`DOMAIN` with 3, `UNSUPPORTED` with 2, `DOMAIN` with inf for log and sqrt of
-2/3, the cube root -0.8735..., exp(-2/3)), sum 4/3 and product 4/9 with the exact local components, `DOMAIN` with place 5 and
with inf for different place sets, a repeated place.

## The tests bite (`lanes/f-slice2/bite.py`, `bite.log`)

22 faults, one at a time, in a scratch copy of `src/sball.c` or `src/rfunc.c` under `build/bite2/`, each against the test of
that file. 20 caught, 2 survive, both equivalent by reading the code. Caught: project without sorting, project without the
real place, a pairing shift of the components in `binary`, `places_differ` reporting the larger prime, `contains` with the
arguments swapped, `neg` without the real negation, `is_canonical` without the increasing order, `mul` adding the real parts,
`binary` writing on a status, `set_arb_lballs` accepting a repeated prime, UNSUPPORTED before DOMAIN, `overlaps` by
`contains`, the odd root of a negative ball through `arb_root_ui`, log of the exact 0 as `NOT_DETERMINED`, sqrt of the
exact 0 as `DOMAIN`, no clamp of `prec` (the test process crashes: SIGSEGV, counted as caught), `where` not written for a place
not in the ball, sin computed as cos, the lower end of the odd root across 0 not negated, a non-finite result stored with `OK`.
Survivors: fault 19 (the branch of the exact 0 in `root_of_checked` removed: the general branch for a ball containing 0 gives
the exact 0 too, since both outer end points are 0; the branch is a fast path) and fault 20 (log_abs of a ball containing 0
computed: `arb_log` of a ball around 0 is non-finite and the finite check returns `NOT_DETERMINED`, so the status is the same).
This is not a mutation run (the brief excludes it).

## Checks of the brief (all after the last change to a `.c` or `.h` file except one comment, see the note)

- `make clean && make check-all` (single job, 4 min 25 s): `check-all passed: make check, driver, exports, julia,
  mutate-selftest, memcheck-selftest`; inside it `check passed: all 62 test programs`, `test_exports: passed: 362 of 362
  declared functions are exported`, `test_julia: passed (with LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10)`.
- `make clean && make -j2 check SAN=1` (4 min 39 s): `check passed: all 62 test programs`; 0 lines with `runtime error`,
  `AddressSanitizer` or `LeakSanitizer` in the log.
- `make clean && make -j2 check CC=clang` (2 min 37 s): `check passed: all 62 test programs`.
- `sh lanes/m1-headers/check_headers.sh`: `check_headers: passed` (both new headers compile alone as C11 and C++17).
- Note: after these runs I changed one comment block of `include/adelefeld/rfunc.h` (the measured `exp` overflow) and the
  documentation; I reran `check_headers.sh` and `tests/test_exports.sh` (both passed) but not the three long suites.

## Findings

1. `src/lball.c` (not changed): `adf_lball_set_fball` needs 13 s at p = 3 (10 s at the prime 2^64 - 59) on an adele whose H has
   87 million bits, to return `LIMIT` (in `fmpz_remove` of the valuation of H); `LIMIT` is decided only after the valuation is
   formed. The test that shows the reported place needs it once (12 s). A bound from the bit length of H before the valuation
   could avoid it. No defect of the results.
2. `docs/conventions.md` 3.2 has no row for `adf_sball` or for functions with a `where` report; I used the rows "Functions at
   places" and "Constructors from raw data" (`OK`, `DOMAIN`, `NOT_DETERMINED`, `UNSUPPORTED`, `LIMIT`, `DOMAIN` with place). Section
   7 and 2.2 name `adf_places_t`, which is defined nowhere. Conventions 7 and the brief disagree on the position of the real place
   (I followed 7).
3. `docs/proofs/functions.md` has no statement for a projection to a set of places as a product set, for the operations on
   tuples, or for the enclosure of a root of a ball containing 0; S1 to S7 in `api-1f.md` are to be merged.
4. `arb_root_ui` returns NaN for the odd root of a negative ball and of 0 (confirmed here by the tests: the bite of fault 12); the
   arb documentation (`arb.rst:979`) states its error bound only for `0 <= r <= m`.
5. `proto/functions_checks.py` says "standard library only" at the top; the new section imports `mpmath` and `flint` inside
   the functions that need them (run by `sball_main` and `gen_vectors.py`, not by `main()`).
6. Avoidable costs seen (not repaired): `adf_sball_get_place` and `adf_sball_project` build a place through `n_is_prime` for
   each call; `is_canonical` does a primality test per component; the operations allocate a temporary and swap.

## Not done

- No mutation run and no fuzz target (brief item 5): the 22 planted faults are not a substitute.
- The Julia test was not run red; it was run green against the built library and its checks were not seen to fail against a
  stub. The C tests were seen red.
- No complex components made by any function; no functions at primes (1F.4); no special functions; no text or dump form.
- No upper bound on `prec`; no test of `prec` above a few hundred bits (the vectors go to 300).
- The suite runs of the brief were not repeated after the comment edit of `rfunc.h`.
- The test file lines are not all at most 116 characters (a few of the tests are at most 130); the Markdown prose is.

## Next slice proposed

1F.4/1F.8 on top of this: `adf_sball_exp_at` etc. at a prime (the local series with the precision rules of SPEC 9.3.2), and the
all-places forms on an adele projected to a named set of places; before that, the second half of 1F.3 proposed by lane f-slice1
(the `w u` split of `decompose` and `{x}_p`). Independent of both: a driver command `project x to S` and `log_at`, to use the
slice from `tools/adf`.
