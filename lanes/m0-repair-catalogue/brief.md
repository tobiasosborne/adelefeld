# Lane m0-repair-catalogue: apply the review of the catalogue proofs

Read `docs/reviews/m0-proofs/catalogue-review.md` completely (the review, with a section "Repairs" holding
replacement texts, and a list of weak checks), `docs/proofs/catalogue.md`, `proto/catalogue_checks.py`,
`docs/reviews/m0-proofs/catalogue_review_checks.py` (read only) and `docs/SPEC.md` section 9.3.7.

**You own:** `docs/proofs/catalogue.md`, `proto/catalogue_checks.py`.

1. Apply every repair of the review to `docs/proofs/catalogue.md`. Where the review offers a choice between
   changing the statement and changing `docs/SPEC.md`, change the statement so that it agrees with `docs/SPEC.md`
   (statement 9: a ball containing a pole returns a status, never an unbounded or finite ball).
   Statement 13: state the centre of the output coset (`c^e` at the primes dividing `N`, 1 at the other primes of
   the finest modulus, combined by the Chinese remainder theorem) and return the modulus in canonical form.
   Statement 7: the local unit part at `p` of an idele includes the cofactor of the scale. Statement 15: add the
   missing proof of the converse. Read each repaired proof again after the change: it must still be a proof.
2. Strengthen the checks the review names as weak, red-green: first add, to `proto/catalogue_checks.py`, a
   self-test that plants each wrong formula the reviewer names (wrong exponent in `(a/2)`, Jacobi symbol without
   prime multiplicities, `(0/-1) = -1`, the centre `c^e` for the finest modulus, the Hilbert symbol of ideles
   without the cofactor, Hilbert precision bound 2 instead of 3 at the prime 2, a missing `pi^(-s/2)`, a wrong
   pole set) and demands that the checks reject it; see it fail for the checks that are weak; then strengthen the
   checks until every planted error is rejected. Record the red and the green output in your report.
3. At the end of `docs/proofs/catalogue.md` add a section "Review record": date 2026-09-27, reviewer Claude opus,
   file of the review, verdict counts, and for each repair one line saying what was changed.

Run `python3 proto/catalogue_checks.py` (must exit 0, under 3 minutes) and
`python3 docs/reviews/m0-proofs/catalogue_review_checks.py` (must still exit 0).
