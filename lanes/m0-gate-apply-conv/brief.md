# Lane m0-gate-apply-conv: apply the gate review to the conventions

Read `docs/reviews/m0-gate/review.md` completely (findings G1 to G16, each with locations, evidence and a
replacement text; the decision table with 8 rejected decisions), then `docs/conventions.md` (version 0.2) and the
checks under `docs/reviews/m0-gate/checks/` that belong to the findings you apply (read only).

**You own:** `docs/conventions.md`, `tests/golden/`, `proto/text_grammar.py`, `proto/test_text_grammar.py`.
Another lane applies the findings that concern `docs/SPEC.md`, `docs/PLAN.md`, `docs/proofs/policies.md` and
`tests/ref/` (G9, G11, G16 and the PLAN side of G1); do not edit those files.

Apply to your files: G1, G2, G3, G4, G5, G6, G7, G8, G10, G12, G13, G14, G15 (if its location is in your files),
and the 8 rejected decisions (CV-11, CV-12, CV-29, CV-30, CV-35, CV-37, CV-39, CV-41), each with the replacement
the review gives.

Rules for this work:
1. Do not paste unread. For each finding first reproduce the evidence (run the reviewer's check or write the
   two-line computation), then apply the replacement, then read the surrounding section again: the document must
   be consistent after the change (status tables, the decision table in section 13, cross-references, the
   grammar, the dump loader rules).
2. Where a finding changes the behaviour of the reference parser or a golden vector, work red-green: first the
   test or vector that shows the defect (G4: the value `0.13` and its rereading; G8: a dump whose context block
   count exceeds `max_items`; G3: a quotient dump with two contexts; G7: a polynomial with a zero leading
   coefficient in text), see it fail, then repair the parser or the vectors.
3. If a replacement text of the review is itself wrong or incomplete, say so with the evidence, and write the
   correct text.
4. Version 0.3, with a change log entry that lists the findings applied; in the decision table mark each decision
   as decided (accepted by the gate review) or replaced (with the finding).

Run `python3 -m unittest proto/test_text_grammar.py` and `python3 lanes/m0-conventions/mutants.py` (read its
usage first; run in ranges under 2 minutes each) at the end.
