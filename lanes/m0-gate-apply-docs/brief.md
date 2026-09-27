# Lane m0-gate-apply-docs: apply the gate review to the plan, the specification, one proof and the reference

Read `docs/reviews/m0-gate/review.md` completely (findings G1 to G16, each with locations, evidence and a
replacement text; the sections "Findings against the specification" and "Sources pending and work not done").

**You own:** `docs/SPEC.md`, `docs/PLAN.md`, `docs/PERF.md`, `docs/proofs/policies.md`,
`proto/policies_checks.py`, `tests/ref/` (all of it), `docs/sources.md`, `refs/README.md`.
Exception to the common rules: you do amend the specification here. Another lane applies the findings to
`docs/conventions.md`, `tests/golden/` and `proto/text_grammar.py`; do not edit those.

Apply:
1. G11 (PLAN 1.8) and the PLAN side of G1 (`docs/PLAN.md:208` and wherever the plan or the specification repeats
   the rule that scaled values fall back to the global backend or that an operation creates a context), G2 where
   SPEC or PLAN describe context ownership, G6 where SPEC 9.3.7 or the status list describe the pole status, and
   every other location the review names inside your files.
2. G16: the one sentence of Summary 26 in `docs/proofs/policies.md`; add the check that shows the difference,
   red-green, in `proto/policies_checks.py`; add a line to the file's review record.
3. G9: the Python reference enforces its shared-context precondition consistently; test first, red-green. Then
   run the whole reference suite and the mutants (`python3 -m unittest discover -s tests/ref/tests`,
   `python3 tests/ref/mutants.py` from `tests/ref`), and regenerate the vectors only if the change affects them
   (say whether it did).
4. The review's section "Findings against the specification": apply each.
5. `docs/sources.md` and `refs/README.md`: add the rows for the source key of the Intel uops.info pages fetched
   by `refs/fetch_intel.sh` (hashes in `refs/manifest-intel.sha256`), with the same columns as the other rows;
   note that these pages also settle the pending item on the Zen 2 register forms, if that is what the files
   on disk show (read them).
6. Decision ids: `docs/conventions.md` says D1 to D12 and SPEC says M0-D1 to M0-D12; add one sentence to SPEC
   section 15.2 stating the correspondence.
7. SPEC and PLAN become version 1.2 with a change log entry listing the findings applied.

Do not paste unread: reproduce the evidence of each finding first, and read the surrounding section again after
the change.
