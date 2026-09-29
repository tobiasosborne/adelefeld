# Lane s-protosync: the reference and the proofs follow what was decided and found on 2026-09-29

Python and documents only; no change of `src/`, `include/`, `tests/`.

Read first: `lanes/COMMON.md`, `lanes/PROOFSTYLE.md`, `docs/worklog/2026-09-29.md` (last section),
`docs/SPEC.md` 15.3, `docs/reviews/s13/review.md`, `docs/reviews/s1/review.md`, `lanes/s13-repair/result.md`,
`lanes/s1-slice1/result.md` (the finding about Algorithm H), `src/resid.c` (`adf_resid_verify_result`),
`include/adelefeld/resid.h`.

**You own:** `proto/solvers_checks.py`, `docs/proofs/solvers.md` (ONLY the places named below),
`docs/api-s.md` (ONLY the stale sentences named below), `lanes/s-protosync/`.

1. `recon_verify_result` of `proto/solvers_checks.py` verifies the whole set of solutions; the C function
   now verifies the status that `recon_partial` returns for the given `limit` (at most `min(ell, X)`
   rounds). Add `recon_verify_returned(m, c, A, B, limit, status, sols, cert)` with the C semantics, keep
   the old function under its name (Proposition 1.11 is about it), and add a check `check_s3_verify_returned`:
   for every `m <= 14` and the limits -1, 0, 1, 2, 5: every result of `recon_partial` is accepted; every
   changed claim is accepted exactly when it is what `recon_partial` returns; every claim it accepts is
   accepted by `recon_verify_result` (it accepts fewer); the three inputs of finding 1 of the review and
   the input of finding 2 (it must return at once). Print the counts.
2. `docs/proofs/solvers.md`: after Proposition 1.11 a Proposition 1.12 (the verifier of the returned
   status): statement, proof (it accepts a subset of the claims of 1.11, so it is sound; it accepts every
   result of Algorithm R; its cost is at most `min(ell, X)` rounds), the check. In Algorithm H (the third
   case, near line 635) a remark: the pending pair `((N/g) w', j + 1)` is implied by the others, with the
   proof of `docs/reviews/s1/review.md` written out, and what it means for the cost bound 2.5(4); the
   algorithm as stated is NOT changed. A check `check_s1_third_case` in the reference: Algorithm H with and
   without that pair gives identical output on at least 20000 row sets; print how often the third case
   was taken.
3. `docs/api-s.md`: the head (lines 20 to 24) still calls the two additions to the row of statuses
   "proposed"; note 2 of section 4 describes S-D13 "decided the other way". One sentence each, so that they
   agree with `docs/SPEC.md` 15.3. The row of `adf_linsolve_fball`: the order of the statuses is now
   `DOMAIN`, `LIMIT`, `UNSUPPORTED` (lane s13-repair); the row of `adf_roots_real`: the status `LIMIT`
   (`prec` above `ADF_ROOTS_REAL_PREC_MAX`, a ball that is not of admissible size), as
   `include/adelefeld/roots.h` says.
4. `python3 proto/solvers_checks.py` passes (under `timeout 900`; give its last lines and the time),
   `pytest proto` passes. Lines at most 116 characters.
