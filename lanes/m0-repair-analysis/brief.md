# Lane m0-repair-analysis: apply the review of the analysis proofs

Read `docs/reviews/m0-proofs/analysis-review.md` completely (the review of your proof file by a different model
family, with a section "Repairs" R1 to R7 holding replacement texts, and the list of surviving mutants and
untested statements), `docs/proofs/analysis.md`, `proto/analysis_checks.py`,
`docs/reviews/m0-proofs/analysis_review_checks.py` (read only) and `docs/SPEC.md` sections 6 to 8.

**You own:** `docs/proofs/analysis.md`, `proto/analysis_checks.py`.

1. Apply R1 to R6. Do not paste blindly: the replacement proofs are short and were not read by a second reader,
   so check each one step by step, complete it where it is only a sketch, and keep the old bound as a corollary
   where the review says it may be kept. Lemma 6 must state the bound for the sum of absolute values. Define
   "primitive" (R2). Supply the convergence estimate (R3). Correct the citations of the non-existent Lemma 15.
   Proposition 15: the sharper inequality, the statement on `gcd(n, C) > 1`, the remark on `Im(s)`, and the exact
   integral of each term by the upper incomplete Gamma function as the main method, the midpoint rule as a
   fallback only.
2. Apply R7 and close the gaps in the checks, red-green: plant the three surviving mutants (lattice factor 2
   dropped; continuation error bound times 1e-6; continuation error bound with conductor 1) and demand that the
   checks reject them; see that they survive; then strengthen the checks (bounds tested where they are larger than
   rounding, no additive slack beyond the rounding of the reference). Add checks for what the review lists as
   never tested: the right-side Poisson bound, theta at an idele other than 1, Proposition 12 with a general `f`
   with `f(0)` different from its transform at 0, the general adelic bound, conductors 9, 16, 25, 27 and a
   composite conductor. Where python-flint offers `arb`/`acb`, use certified balls for the inequalities.
3. At the end of `docs/proofs/analysis.md` add a section "Review record": date 2026-09-27, reviewer Claude
   fable, the file of the review, verdict counts, one line per change.

Run `python3 proto/analysis_checks.py` (must exit 0, under 3 minutes).
