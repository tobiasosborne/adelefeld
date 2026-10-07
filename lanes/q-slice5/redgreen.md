# Red and green runs, lane q-slice5

Build: `timeout 600 make -s -j2 BUILD=lanes/q-slice5/build lanes/q-slice5/build/<test>`; run under `timeout 120`.

## Class calls (3.2-b)

- RED 1: header declarations added, src/psi.c with stubs returning ADF_UNSUPPORTED. test_psi_class exit 1:
  `tests/test_psi_class.c:263: qeval(z, lift, 128, 1) == ADF_NOT_DETERMINED` (e3); golden, statuses, vectors and
  additivity groups each exit 1 on their first status assertion.
- Refactor of src/psi.c (psi_adele split into psi_exact, psi_numeric, psi_fold, psi_commit; the adele phase
  getter into psi_phase_entry) BEFORE the new code: test_psi still exit 0, 186799 checks.
- GREEN 1: class calls implemented. Two test expectations were wrong and were corrected (not the code): a
  hand-built PIECES value with entries out of key order (test_psi_class.c:346, not canonical), and a real
  midpoint 1-2^-(cap-1) expected OK, which the projected-distance bound of the adele call already rejects
  (LIMIT on the adele call too); now 1-2^-(cap-8), with the adele call checked to agree. A generator record
  had the non-dyadic midpoint 1/3 (setball assertion); now 3/8. test_psi_class exit 0, 148748 checks.

## Local and place calls (3.2-c)

- The local code was written in the same pass as the class code; for the red run src/psi.c was copied, the
  five public local/place functions were replaced by stubs returning ADF_UNSUPPORTED in the working file,
  built into lanes/q-slice5/build-red, and src/psi.c restored from the copy afterwards. RED: each group of
  test_psi_local exit 1 on an assertion: witnesses `test_psi_local.c:306` (getter at [p=2: 1/6]), limits :355
  (prec LIMIT), golden :93 (where = v on failure), local :174 (getter status), place :93.
- GREEN: one test expectation was wrong (3^(cap/2) has 0.79 cap bits, so it is admitted; the LIMIT case is
  now 3^(3cap/4) with 1.19 cap bits). test_psi_local exit 0, 165418 checks, 432 local records,
  576 place evaluations, 4608 hull coordinates.

## After the mutation sweep

- Survivor psi.c:290 (`st = ADF_NOT_DETERMINED` dropped in psi_adele, strict then returns OK unwritten) is
  killed by test_psi but not by the two new tests; golden() of test_psi_class now also checks the adele
  strict status of each lift. Survivor psi.c:507 (`strict = 0` at infinity) was a redundant statement (B = 1
  there): removed from src/psi.c, the comment says why.
