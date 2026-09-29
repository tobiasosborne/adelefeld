# Lane d-functions: design of the public interface of milestone 1F (functions), work packages 1F.1 to 1F.4

A design document, the map for thin slices (`docs/workflow.md` rule 1), as `docs/api-s.md` was for
milestone S. No change of `src/` or `include/`. Write for a reviewer who will try to refute you.

Read first: `CLAUDE.md`, `docs/workflow.md`, `docs/SPEC.md` (section 9 on functions, the sections on partial balls and local balls,
and section 15 with every decision M0-D*, M1-D*), `docs/PLAN.md` section 6, milestone 1F (1F.1 to 1F.9 and
what follows), `docs/proofs/functions.md` (all of it), `docs/proofs/analysis.md`, `docs/proofs/precision.md` and `policies.md` where
functions.md cites them, `docs/conventions.md` (sections 2 to 5, 7, 9 to 12), `docs/api-m1.md` and the
headers of milestone 1 (`include/adelefeld/*.h`) for the style, `docs/api-s.md` for the form of a design
and of its table of decisions, `docs/reviews/s-design/review.md` for what a review of a design found
(contradictions of the interface, statements described unfairly, overflow of the reference).

**You own:** `docs/api-1f.md` (new), `proto/functions_checks.py` (new: reference algorithms and the oracles of
the tests, exact arithmetic, with check functions that print counts and fail on a wrong answer),
`lanes/d-functions/`. Everything else is read-only.

What `docs/api-1f.md` must contain.
1. Headers and types: for each type its data, its set statement (which set a
   value means), its predicate `is_canonical`, its init value, its layout. Every type and function of
   work packages 1F.1 (`adf_sball`, `f_at`), 1F.2 (archimedean wrappers), 1F.3 (`adf_lball`) and 1F.4 (local `exp`, `log`, `Log`). 1F.5 to 1F.9 are named in section 5 only.
2. For each function: the declaration, the set statement of the result, the proposition of
   `docs/proofs/functions.md` it implements (with the line), the statuses and what is written on each, the
   aliasing rule, the cost. Where `functions.md` has no statement for something the plan asks for, say so:
   do not invent a proof in a table cell; write the missing statement and its proof in a section
   "Statements to add to functions.md".
3. Acceptance tests for each function: what the test must check so that it fails for a wrong
   implementation (enumeration modulo small prime powers, the regression `exp 3` and `exp 12` at precision 8,
   the precision cases of `SPEC.md` 9.3.2, a failure at one place reported with that place).
4. Decisions for TJO: a table, each with the question, the recommendation and the alternatives stated
   so that each can be chosen on its merits. Few and real: a decision that the SPEC already takes is cited,
   not asked again.
5. Thin slices: the order in which the milestone is built, each slice the smallest piece a user can call
   (through Julia `ccall`, `tests/julia/`), with its functions, its oracle in `proto/functions_checks.py`, and
   the three decisions at most that it needs first.
6. Findings against `docs/SPEC.md`, `docs/PLAN.md` or `docs/proofs/functions.md`: contradictions,
   statements that are false (with the counterexample computed by `proto/functions_checks.py`).

`proto/functions_checks.py` must run: `python3 proto/functions_checks.py` prints each check with its counts and
ends with the number of checks, under 3 minutes. Every example of the design is computed by it, not by
hand.

Report (`lanes/d-functions/result.md` and your final message): the decisions; the findings; what the
reference checks, with numbers; what is not designed and why.
