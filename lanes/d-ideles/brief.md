# Lane d-ideles: design of the public interface of milestone 2 (ideles)

A design document, the map for thin slices (`docs/workflow.md` rule 1), as `docs/api-s.md` was for
milestone S. No change of `src/` or `include/`. Write for a reviewer who will try to refute you.

Read first: `CLAUDE.md`, `docs/workflow.md`, `docs/SPEC.md` (the sections on ideles, units, idele classes,
and section 15 with every decision M0-D*, M1-D*), `docs/PLAN.md` section 6, milestone 2 (2.1 to 2.4 and
what follows), `docs/proofs/ideles.md` (all of it), `docs/proofs/precision.md` and `policies.md` where
ideles.md cites them, `docs/conventions.md` (sections 2 to 5, 7, 9 to 12), `docs/api-m1.md` and the
headers of milestone 1 (`include/adelefeld/*.h`) for the style, `docs/api-s.md` for the form of a design
and of its table of decisions, `docs/reviews/s-design/review.md` for what a review of a design found
(contradictions of the interface, statements described unfairly, overflow of the reference).

**You own:** `docs/api-2.md` (new), `proto/ideles_checks.py` (new: reference algorithms and the oracles of
the tests, exact arithmetic, with check functions that print counts and fail on a wrong answer),
`lanes/d-ideles/`. Everything else is read-only.

What `docs/api-2.md` must contain.
1. Headers and types: for each type its data, its set statement (which set of ideles, units or classes a
   value means), its predicate `is_canonical`, its init value, its layout. Every type and function of
   work packages 2.1 to 2.4.
2. For each function: the declaration, the set statement of the result, the proposition of
   `docs/proofs/ideles.md` it implements (with the line), the statuses and what is written on each, the
   aliasing rule, the cost. Where `ideles.md` has no statement for something the plan asks for, say so:
   do not invent a proof in a table cell; write the missing statement and its proof in a section
   "Statements to add to ideles.md".
3. Acceptance tests for each function: what the test must check so that it fails for a wrong
   implementation (enumeration modulo small moduli, negative rationals, the exact units `[1]`, `[-1]`,
   the norm exact before rounding, containment and tightness of hulls).
4. Decisions for TJO: a table, each with the question, the recommendation and the alternatives stated
   so that each can be chosen on its merits. Few and real: a decision that the SPEC already takes is cited,
   not asked again.
5. Thin slices: the order in which the milestone is built, each slice the smallest piece a user can call
   (through Julia `ccall`, `tests/julia/`), with its functions, its oracle in `proto/ideles_checks.py`, and
   the three decisions at most that it needs first.
6. Findings against `docs/SPEC.md`, `docs/PLAN.md` or `docs/proofs/ideles.md`: contradictions,
   statements that are false (with the counterexample computed by `proto/ideles_checks.py`).

`proto/ideles_checks.py` must run: `python3 proto/ideles_checks.py` prints each check with its counts and
ends with the number of checks, under 3 minutes. Every example of the design is computed by it, not by
hand.

Report (`lanes/d-ideles/result.md` and your final message): the decisions; the findings; what the
reference checks, with numbers; what is not designed and why.
