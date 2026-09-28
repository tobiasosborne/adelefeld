# Lane m0-gate-closure: closure check of the gate review of milestone 0

Your gate review `docs/reviews/m0-gate/review.md` (verdict: GATE NOT PASSED; G1 to G16; 8 decisions rejected) has
been applied by two lanes. Their reports: `lanes/m0-gate-apply-conv/report.md`,
`lanes/m0-gate-apply-docs/report.md`. The documents are now `docs/conventions.md` 0.3, `docs/SPEC.md` and
`docs/PLAN.md` 1.2. See the changes with `git diff a46b161 HEAD -- docs proto tests` (read-only git commands are
allowed) .

**You own:** `docs/reviews/m0-gate/closure.md`, `docs/reviews/m0-gate/closure-checks/`.

1. For each of G1 to G16 and each of the 8 rejected decisions: is it closed? Read the new text, not the report
   about it. Verdict per item: CLOSED, CLOSED WITH EDIT (give the exact edit, at most a few lines), or OPEN (say
   what is missing). G15 has a half in `docs/proofs/analysis.md` that no lane owned: say exactly what edit closes
   it.
2. Did the repairs introduce a new defect? Look in particular at: the scaled operations with a shared context
   (status, aliasing, exact operands); the context constructors and the value types that borrow contexts; the
   dump grammar with bindings per occurrence; the real-ball printing with its fixed point (test it by your own
   computation on at least 10000 random dyadic balls and on the hard cases of your review); the statuses at
   poles; consistency between conventions, SPEC and PLAN. New findings are numbered C1, C2, ... with severity
   and replacement text.
3. Final verdict, one of: GATE PASSED; GATE PASSED AFTER THE LISTED EDITS (each edit is given in full and needs
   no further review); GATE NOT PASSED. The question the verdict answers: may milestone 1 now write the public
   header for the types of milestone 1 (`adf_rat`, `adf_fball` in the global backend first, `adf_adele`,
   `adf_cadele`, text forms, then the scaled policy and the local backend with `adf_modctx`)? If only a part may
   proceed, say which part.

Keep it short: the closure file is a table, the edits, the new findings, the verdict, and the commands with
their outputs. Then `report.md` in your lane directory with the verdict and the open items, one line each.
