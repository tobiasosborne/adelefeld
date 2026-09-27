# Lane m0-repair-functions: apply the review of the functions proofs

Read `docs/reviews/m0-proofs/functions-review.md` completely (the review, with a section "Repairs" holding
replacement texts R-1, R-2, the optional items O-1, O-2, and a list of weak checks), `docs/proofs/functions.md`,
`proto/functions_checks.py`, `docs/reviews/m0-proofs/functions_review_checks.py` (read only) and
`docs/SPEC.md` section 9.3.

**You own:** `docs/proofs/functions.md`, `proto/functions_checks.py`.

1. Apply the repairs R-1 (Proposition 8: prove that the division by `p^e` is exact) and R-2 (Proposition 12,
   step 4: the correct citation). Read each repaired proof again: it must still be a proof.
2. Optional item O-1 (the tighter term count for `log`): add it as a second, separate proposition ("tight count"),
   with proof, and keep the existing safe count. State clearly which one the C code is to use by default (the
   tight one, if its proof is complete) and check both by enumeration.
   Optional item O-2 (odd degree roots at 2 need only `r >= 1`): add it as a remark with proof, marked as not part
   of `docs/SPEC.md` 9.3.3, which keeps the stronger guard. Do not change the guard of the main statement.
3. Strengthen the checks the review names as weak, red-green: real mutation tests in place of fixed values (plant
   each wrong rule in a copy of the rule under test and demand rejection), among them the variant `W = n + D`
   without the maximum; `check_power_precision` with `alpha` above `c` and with real content at `p = 13`;
   `check_root_precision` with more roots `b`, and with degrees 9 and 25. Record the red and green output.
4. At the end of `docs/proofs/functions.md` add a section "Review record": date 2026-09-27, reviewer Claude
   opus, file of the review, verdict counts, and one line per change.

Run `python3 proto/functions_checks.py` (must exit 0, under 3 minutes) and
`python3 docs/reviews/m0-proofs/functions_review_checks.py` (must still exit 0).
