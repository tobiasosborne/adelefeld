# f-repair1: result

All six findings of docs/reviews/f1/review-lball.md are repaired. The new tests were seen red on the old code,
then green. All required checks pass. No mutation run (the brief excludes it).

## Red run (before any change)
Reviewer's programs against the old library (lanes/f-repair1/red-reviewer.log):
- `boundaries`: 7 of 7 cases return LIMIT, exit 1.
- `check_invariants.py`: 9 missed checks, exit 1.
- `test_assertion`: exit 1 (the old assertion accepts the wrong unit 5).
- `proof_probe.py`: assertion failure (sign in L0).
New tests against the old code (red-tests.log): release 26 tests, 17 failed checks in 3 tests
(inv_of_an_exact_power..., sub_of_small_results..., div_of_small_results...); INV build 27 tests, 39 failed checks in
4 tests (adds entry_check_of_every_public_function, 22 failed checks).

## Findings
- F1 (exact inverse at the exponent limit). `src/lball.c` adf_lball_inv: the guard on N - 2v now applies to balls
  only. Test: inv_of_an_exact_power_at_the_exponent_limit (both signs, 3/2 * 5^(+-E), aliased, plus the LIMIT cases that
  stay right: ball with N' = E + 1, 3 + O(5^E)).
- F2 (sub). sub and add now share `lb_addsub(z, x, y, sgn)`; the sign is applied to the unit part of y inside the sum
  (tracked by position, so sub(x, x) with one object is right). No canonical -y is formed. Test:
  sub_of_small_results_at_the_exponent_limit (x - x, Z_5 - fine, 2 - 1, aliased, at v = -E, and LIMIT for 0 - (1 + O(5^E))
  which is right: centre 5^E - 1).
- F3 (div). div is computed directly (statement L4a): K = min(v + M - 2w, N - w, N + M - 2w) from valuations
  and precisions, centre `u/t mod p^k` with k the relative precision of the result via lb_make; exact 0 / unit = exact 0.
  The order of the checks is kept (different primes, y bounds, y = 0 statuses, x bounds). Tests:
  div_of_small_results_at_the_exponent_limit (the 3 reviewer cases and 12 more, LIMIT where the result is outside),
  sub_and_div_agree_with_their_compositions (7396 + 3721 + 3249 pairs at p = 5, 3, 2: sub = add of neg, div = mul by
  inv wherever those work, same status otherwise). The existing enumeration test already covers sub and div at mixed
  valuations (all pairs p = 2, 3; samples p = 5, 7); I did not extend it. proto/functions_checks.py (f-slice1 section):
  new `lb_ref_div_direct` and `check_lball_quotient`, 12729 pairs, passes (`timeout 170 python3 -B proto/functions_checks.py`).
- The rule "LIMIT only if an input or the result is outside" is stated in lball.h (Limits paragraph, and at neg, add, sub, mul,
  inv, div) and in api-1f.md decision 3. add, mul, neg, inv were checked for the same defect: no other defect found.
  Test add_mul_neg_at_the_exponent_limits (inputs at +-E). neg of 1 + 5^E Z_5 is LIMIT because the result centre needs 5^E;
  this is said in the header and tested.
- F4 (entry checks). ADF_INV_LBALL added to place, is_exact, contains_zero, get_prec, set (x), swap (x and y),
  identical (x and y); ADF_INV_RAT (from src/invariants.h) added to set_rat and set_rat_ball, before the archimedean check.
  Internal copy, swap and identity are unchecked statics (lb_copy, lb_swap, lb_identical), so an output that is overwritten is
  not checked. Exempt and documented in lball.h: init, clear (must release a forged value), is_canonical (never aborts),
  layout queries (no argument), outputs of set and of the arithmetic functions. set_fball is not given its own check:
  it reads f through adf_fball_get_fmpz3, which checks (the message names that function; with the archimedean place it returns
  DOMAIN before reading f). Test entry_check_of_every_public_function (INV build only; fork per case, SIGABRT and the
  function name on stderr, plus a canonical control and a forged output of set that must not abort). Without the flag,
  set_rat with a non-canonical adf_rat gives no promise (M1-D11).
- F5. api-1f.md L0 proof: `t - u = (a - b u)/b`.
- F6. Assertion replaced by `unit_part_ok` (canonical, v = 0, centre != 0, library valuation 0), plus the valuation of the input
  equals m. Test unit_part_check_rejects_the_wrong_unit (exact 5 with v = 1 is rejected).

## Checks (each under timeout 900; last line)
- `make clean && make -j2 check-all`: `check-all passed: make check, driver, exports, julia, mutate-selftest, memcheck-selftest`
- `make clean && make -j2 check SAN=1`: `check passed: all 60 test programs`
- `make clean && make -j2 check CC=clang`: `check passed: all 60 test programs`
- `make clean && make -j2 check INV=1`: `check passed: all 60 test programs`
- `sh lanes/m1-headers/check_headers.sh`: `check_headers: passed`
- test_lball with INV: 27 tests, 3982430 checks, 0 failed. The vectors of tests/ref/vectors/f-slice1/ pass unchanged.
- Reviewer's programs again (green-reviewer.log): `boundaries` exit 0 (7 cases, 0 failures); `check_invariants.py` exit 0
  (9 calls, 0 missed, 1 aborting control). The reviewer's `test_assertion` and `proof_probe.py` are self-contained (they
  restate the old assertion and the old equality and do not read my files), so they still exit 1 by construction; the
  repaired assertion and equation are tested by unit_part_check_rejects_the_wrong_unit and by the text of api-1f.md.

## Files
src/lball.c, include/adelefeld/lball.h, tests/test_lball.c, docs/api-1f.md (section 1 only), proto/functions_checks.py
(f-slice1 section), lanes/f-repair1/ (redgreen scripts revprogs.sh, revrun.sh, redrun.sh, checks.sh; logs red-*.log, green-*.log,
check-*.log, summary.log; progress.md).
Note: lanes/f-repair1/redgreen.log was not written; the red and green runs are in red-reviewer.log, red-tests.log, green1.log,
green-reviewer.log.

## Not done
- No mutation run, no fuzz run. No exhaustive check at the exponent limits: the limit cases are hand-worked values.
- Julia binding test (tests/julia) ran only as part of check-all.
- api-1f.md: only section 1 was edited; docs/conventions.md 3.2 still lacks an lball row (reviewer's note, not mine to change).
- Avoidable cost: the added L4a formula duplicates the K logic of mul; not merged.

## Findings against the specification
None.
