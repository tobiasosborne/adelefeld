# Lane m0-gate: gate review of milestone 0 (contracts)

You are the second model family that `docs/PLAN.md` section 8 requires before any public type is written in C.
After your verdict, milestone 1 implements these contracts in C on FLINT 3.0.1; a Julia layer will later be built
on the C interface. A defect you let pass becomes a wrong enclosure or a broken interface.

**You own:** `docs/reviews/m0-gate/review.md`, `docs/reviews/m0-gate/checks/` (all of it). Everything else is
read only.

Objects of review, in this order of importance:

1. `docs/conventions.md` (version 0.2): canonical forms and storage invariants of every type; status codes and
   the state of outputs; aliasing and ownership; the text grammar of value and dump form; the foreign-function
   section; the table of decisions (decided and proposed). Judge every decision: accept, or reject with the
   replacement. Look for: invariants that two programmers could read differently; invariants that an operation
   of the specification cannot maintain; a status that leaves an output in an unusable state; grammar
   ambiguities (give the two parses); inputs that make the reference parser `proto/text_grammar.py` disagree
   with the grammar or with the golden vectors in `tests/golden/`; anything that blocks Julia's `ccall`.
2. `docs/SPEC.md`, `docs/PLAN.md`, `docs/PERF.md` (version 1.1): are the amendments consistent with the proofs,
   with `docs/conventions.md`, with `docs/seams.md` and with each other? Is any finding of the lanes lost
   (`lanes/*/report.md`, sections "Findings against the specification")? Every label **[quoted]** in SPEC: open
   the cited file under `refs/src/` at the cited line and check that the text there says what is claimed.
3. What is new since the proof reviews and has had no second reader: `docs/proofs/functions.md` Proposition 7b
   (tight term count for `log`) and Remark 15r; the repaired statements policies P24, P25, L6, P10, Summary 26,
   P14 and ideles P19; the repaired bounds of `docs/proofs/analysis.md` (Lemma 6, Lemma 14, Proposition 15,
   the convergence estimate in Proposition 12); the repaired statements 7, 9, 13, 15 of
   `docs/proofs/catalogue.md`. In `proto/analysis_checks.py` the group `check_general_splitting` records a
   maximum error of 2.8e-14 where its neighbours record 1e-55: find out why, and whether the check tests
   anything. Try to refute each by your own computation in your `checks/` directory.
4. `docs/seams.md`: is any recommendation R1 to R9 wrong, or missing from the conventions?
5. The Python reference `tests/ref/` against the proofs and conventions: does it implement the decided
   conventions (exact values under the cap, closed interval in reconstruction)? It is the oracle for the C
   code.

The earlier reviews are in `docs/reviews/m0-proofs/` and `docs/reviews/astra-2026-09-27*/`; do not repeat what
they settled, unless you find it wrong.

Write `docs/reviews/m0-gate/review.md`:

- Verdict, one of: GATE PASSED (milestone 1 may freeze the public header for the types of milestone 1);
  GATE PASSED AFTER THE LISTED EDITS (list them; none needs a second review); GATE NOT PASSED (list the
  blockers).
- Findings, numbered G1, G2, ..., each with severity BLOCKER, MAJOR or MINOR, the location (file and line), the
  evidence (a computation, a counterexample, two conflicting quotations), and the replacement text ready to
  paste.
- A table of the decisions of `docs/conventions.md` with accept or reject.
- A section on what you checked and found in order, so that the reader knows what was covered.
- Checks: commands and outputs (numbers).

Sober and concrete; no praise. Then `report.md` in your lane directory: the verdict, the counts by severity, the
blockers and majors in one line each.
