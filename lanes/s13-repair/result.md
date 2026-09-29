# s13-repair: result

## What was done
1. Findings 1 and 2. `adf_resid_verify_result` (src/resid.c) now verifies "adf_resid_reconstruct returned this status
   for this limit". In the range A < m <= 2AB, |T| <= B it runs the function's own `resid_search` with
   ell = max(limit, 0), at most min(ell, X) rounds, and accepts the claimed status exactly when the search returns
   it (for OK also q equal to the point found). The special case ell = WORD_MAX and the "X above a word: 0"
   rule of `enumerate_all` are gone. Fast cases unchanged. Every result of the function is accepted; no case
   remains in which a true claim about the call is refused, so the sentence "0 also where the claim may be true but
   cannot be decided" was removed. Header comment, cost sentence, row of docs/api-s.md section 2 and its note 4
   rewritten.
2. Finding 3. New static `fball_precheck` in src/linsolve.c: DOMAIN, then LIMIT (r > MAX or c > MAX - r), then
   UNSUPPORTED (`adf_fball_is_exact(b + i)`, reads stored H; a local ball has H = K >= 2 and is never exact, so
   no ball is recombined and no allocation is needed, even for local balls). Used by `adf_linsolve_fball` and
   `adf_linsol_verify_fball`; `fball_system` runs only afterwards. Header of linsolve.h corrected: the order of
   statuses is now DOMAIN, LIMIT, UNSUPPORTED (before: UNSUPPORTED before LIMIT; a system with an exact ball
   and r + c over the limit now gives LIMIT); `adf_linsol_verify_fball` also returns 0 for r + c over the limit
   (no sol can exist for it).
3. Finding 4. The `NOT_UNIQUE` branch in `verify_result_on_the_vectors` (tests/test_resid_rest.c) now asserts: q
   untouched (-12345), certificate equal to the vector's `cert` (or kind 0 for null), verifier accepts NOT_UNIQUE,
   verifier refuses OK with the vector's first solution (n, d). The test was extended, not removed.
4. Existing tests changed to the new contract (tests/test_resid_rest.c): `claim_true` (a claim is true exactly
   when its status is the function's for this limit, OK also q), the vectors test (now `undecided == 0`), and
   `verify_result_when_X_is_above_a_word` (NOT_UNIQUE at limit 1 is accepted).

## Files written
include/adelefeld/resid.h (comment only), include/adelefeld/linsolve.h (comment only), src/resid.c, src/linsolve.c,
tests/test_resid_rest.c, tests/test_s13_repair.c (new: 4 tests), docs/api-s.md (row of verify_result and note 4
only), lanes/s13-repair/redgreen.log, lanes/s13-repair/probe_linsolve_limit_first.c, lanes/s13-repair/result.md.
No signature changed.

## Checks
- Red first (scratch copy build/red, with HEAD's src/resid.c and src/linsolve.c, new test): hand certificates 4
  failed checks (the four wrong acceptances of the review, plus OK at limit 4); grid of 500192 claims 16466 FAIL
  lines; cost test did not end in `timeout 60` (5e9 rounds); linsolve 6 failed checks (LIMIT: 2 malloc + 4 calloc;
  exact ball + LIMIT gave UNSUPPORTED with 3 calloc; UNSUPPORTED 3 calloc). The changed tests/test_resid_rest.c
  against the old src: 69023 FAIL lines. (Note: I edited the source before running the new test; the red was
  shown afterwards on a scratch copy with the old sources, not on the live tree. Log: redgreen.log.)
- Green: `./build/test_s13_repair`: 4 tests, 649359 checks, 0 failed (grid: 500192 claims, 125048 accepted = every
  claim equal to the function's status and no other; results OK 15181, NO_SOLUTION 41551, NOT_UNIQUE 38596,
  NOT_DETERMINED 29720; a case fails if the verifier accepts a claim that is not the function's status/q, or
  refuses the function's own). Finding-2 input: 6 verifier calls with B = 5e9 in 0.012 s CPU.
  `test_resid_rest`: 17 tests, 7607041 checks, 0 failed. `test_linsolve_rest`: 13 tests, 30328 checks, 0 failed.
- Item 4, mutations one at a time in scratch build/mut (test_s13_repair): M1 OK case without X <= ell
  (`!complete && !found`): 6 failed checks; M2 NO_SOLUTION case without X <= ell: 10 failed checks; M3 verifier search
  to X instead of min(ell, X): killed (timeout 60, rc 124, 46190 FAIL lines before it); M4 LIMIT test dropped from
  the precheck (allocation before LIMIT): 3 failed checks. M1 and M2 mutate the shared `resid_search`, so they change
  the function too; they are caught by the constants in the hand-certificate test, not by the grid.
- `make clean && make check-all`: exit 0, last line `check-all passed: make check, driver, exports, julia,
  mutate-selftest, memcheck-selftest`; 0 FAIL lines.
- `make clean && make -j2 check SAN=1`: exit 0, `check passed: all 55 test programs`.
- `make clean && make -j2 check CC=clang`: exit 0, `check passed: all 55 test programs`.
- `sh lanes/m1-headers/check_headers.sh`: `check_headers: passed`.
- Reviewer's programs against the repaired library (compiled as in the review):
  `probe_resid`: grid baseline=643440 first=643440 verified=643440 bad-cert-rejected=333851, large baseline=720, the
  three claims `accepted=0` each (actual=NOT_DETERMINED). `probe_sets`: set_rat=18000 contains=279000 forget=22448
  local=1032 huge=48 lifecycle=1000 raw-negative=70, exit 0. `probe_verify_time 5000000000`: `accepted=0`, at once.
  `probe_linsolve` as written ABORTS at its line 137, because it expects UNSUPPORTED for 4097 rows with an
  exact ball, and the new decided order gives LIMIT. A copy with that one expectation changed to LIMIT
  (lanes/s13-repair/probe_linsolve_limit_first.c) prints: oracle cases=2060 points=141672 coords=3292 OK=299
  NO_SOLUTION=1761 false-cert=2060; limit status=LIMIT allocations=0; exact-ball-and-limit status=LIMIT
  allocations=0; mod limit status=LIMIT allocations=0; exit 0.

## Not done
- No mutation tool run (replaced by item 4). No run of the four mutations against the whole test-suite, only
  against tests/test_s13_repair.c. No leak check beyond what `make check SAN=1` does.
- The header comment of the exact-ball case in `docs/api-s.md` or SPEC was not touched (not owned); if a
  text there still lists UNSUPPORTED before LIMIT for `adf_linsolve_fball`, it is out of date.
- Lines over 116 characters exist in src/resid.c (max 136) and docs/api-s.md; I did not check whether they predate
  this lane.

## Header findings
- linsolve.h: the order of statuses of `adf_linsolve_fball` changed as the brief decided (LIMIT before
  UNSUPPORTED); the comment now says so. Zero allocations on all three refusals, local balls included, since
  `adf_fball_is_exact` reads H and a local ball never has H = 0.

## Findings against the specification or docs/proofs/solvers.md
None. Proposition 1.11 is unaffected (the verifier accepts a subset of the claims of before).
