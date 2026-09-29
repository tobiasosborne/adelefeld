# s-protosync: result

## Done, by item
1. `proto/solvers_checks.py`: new `recon_search_status` (step 7/8 of Algorithm R alone, min(ell, X) rounds) and
   `recon_verify_returned(m, c, A, B, limit, status, sols, cert)` with the C semantics (status must be the one
   `recon_partial` returns; for OK also the point; `sols` read for OK only; certificate read for OK, NO_SOLUTION,
   NOT_DETERMINED; NOT_UNIQUE uses its own EEA pair). `recon_verify_result` unchanged. New
   `check_s3_verify_returned` (in PART1). Output: 116760 results accepted (m <= 14, limits -1,0,1,2,5); 3502800
   changed claims, 670450 accepted, all and only the returned status/point (0 mismatches); 0 accepted by the new
   verifier and refused by `recon_verify_result` (for non-OK statuses the result's list is used, since the C
   function does not read it); 11305 claims the old verifier accepts and the new refuses; 98061 changed
   certificates; finding 1: 3 of 3 refused (old accepts, function returns NOT_DETERMINED); finding 2
   (m=2,c=0,A=1,B=5e9,limit 0, OK): refused in 0.0000 s.
2. `docs/proofs/solvers.md`: Proposition 1.12 inserted before section 2 (statement, verifier in order, proof,
   the finding-1 and finding-2 inputs, check, used by). Remark after the proof of Proposition 2.5 (Algorithm H third
   case): the pair `((N/g) w', j+1)` is implied, proof of the s1 review written out; meaning for cost bound 2.5(4)
   (at most m + n pairs without it; stated bound remains true). Algorithm as stated NOT changed. Reference: `howell`
   got options `third_pair=True, stats=None`; new `check_s1_third_case` (in PART2): 22800 row sets, 19 moduli up to
   6^20, up to 5 columns, up to 6 rows: output with and without the pair identical and in Howell form, 0 failures;
   third case taken on 12371 row sets, 36980 times; 450 small sets equal the brute-force Howell form.
3. `docs/api-s.md`: head (lines 20 to 24) says the two additions are decided (conventions.md 3.2, SPEC 15.3);
   note 2 of section 4 says S-D13 is decided as `g` and describes the other way as not chosen; row of
   `adf_linsolve_fball`: statuses in the order DOMAIN, LIMIT, UNSUPPORTED; row of `adf_roots_real`: LIMIT
   (`prec` above `ADF_ROOTS_REAL_PREC_MAX`, ball not of admissible size, before any allocation).
4. `timeout 900 python3 proto/solvers_checks.py`: rc 0, `total time 59.5 s; failures: 0`, 0 FAIL lines; last line
   before it: `PASS probe_s2_flint_real: FLINT 3.0.1: 60 calls ...`. `pytest proto -q`: `35 passed, 13 subtests
   passed in 42.16s`.

## Other checks
- One mutant of the new verifier (search with ell + 1) run against `check_s3_verify_returned`: FAIL (killed).
  No mutation sweep. The check was written after the code and passed at its first run; red-first was not shown
  except by this mutant.

## Not done / notes
- The "Table of statements" of `docs/proofs/solvers.md` has no row for P1.12 (the brief named only the places
  above); the orchestrator should add: P1.12, verifier of the returned status, proved here, `check_s3_verify_returned`.
- Lines over 116 characters in my new text: none in proto or the new proofs text; `docs/api-s.md` still has
  long table lines that predate the lane (two rows I edited are table rows and were long before).
- Red run was not made for `check_s1_third_case` (a check of an equality that holds; the mutant would be a
  wrong pair-free variant, not run).
- I did not read the C code beyond `adf_resid_verify_result` and `enumerate_all`; the Python semantics were
  matched to it by reading, not by a differential run against the C function.

## Files
proto/solvers_checks.py, docs/proofs/solvers.md, docs/api-s.md, lanes/s-protosync/result.md
