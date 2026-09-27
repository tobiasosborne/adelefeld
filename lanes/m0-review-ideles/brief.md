# Lane m0-review-ideles: adversarial review of the proofs of work package 0.3b

**You own:** `docs/reviews/m0-proofs/ideles-review.md`, `docs/reviews/m0-proofs/ideles_review_checks.py`.
Everything else is read only; do not edit the proof files.

Object of review: `docs/proofs/policies.md` (22 statements), `docs/proofs/ideles.md` (16), `docs/proofs/quotient.md`
(13), written by a different model family (Claude opus), with the checks `proto/policies_checks.py`,
`proto/ideles_checks.py`, `proto/quotient_checks.py`. They prove the statements of `docs/SPEC.md` sections 4.4, 5,
6 and 9.2, and make seven findings against the specification (`lanes/m0-proofs-ideles/report.md`). Milestones 1 to
3 will implement these rules literally in C; every result must be a set that certainly contains the true value.

Your task is to REFUTE. For each numbered statement:
1. Try to break it by your own independent computation in your checks file (do not import the author's
   functions): zero and negative centres, radius 0, fractional radii, moduli 1 and 2 and twice an odd number,
   shared factors between scale, residue and context, blocks that share a factor with the denominator, balls
   whose real part has an integer endpoint, real balls of width exactly N.
2. Read the proof step by step. Is each step justified? Is a hypothesis used that is not stated? Is every
   "tight", "smallest", "best possible" and "exactly" proved in both directions?
3. Judge each of the seven findings against the specification: is it right? Is the proposed remedy sound?
4. Judge the author's checks: would a wrong formula pass them?
5. Verdict per statement: VALID; MINOR (true, but needs a stated repair: give the replacement text); INVALID
   (false, or a gap you cannot close: give the counterexample or the exact gap).

Write the review file with: a summary table (file, statement, verdict, one line); details for every statement
that is not plainly VALID; a verdict on each of the seven findings; a section "Repairs" with replacement texts
ready to paste; a section "Checks" with the command and output of your own checks (numbers). Sober and
concrete; no praise. Where you find nothing, say what was tried. Then `report.md` in your lane directory with
the counts VALID / MINOR / INVALID and the important items in one line each.
