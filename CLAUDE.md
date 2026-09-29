# adelefeld

Adeles as a first-class number type: C on FLINT, arbitrary precision, ball arithmetic. What is built: `docs/SPEC.md`.
In what order: `docs/PLAN.md`. How speed is judged: `docs/PERF.md`.

## Rules

1. **Red-green TDD.** Write the failing test first, see it fail, then write the code that makes it pass.
2. **Mutation testing and fuzzing where they pay.** Mutation testing of the files a change touched, with a
   bound on the run (`docs/workflow.md`); no sweep over the whole of `src/`. Fuzzing is a long differential run
   against the reference that asserts the contract (enclosure, statuses); a run of seconds that "did not crash"
   is a smoke test and is reported as one.
3. **Read the ground truth before coding.** Every formula, convention and algorithm is read in its source before it
   is implemented, and cited in the code by file and line.
4. **Ground truth is a local copy of the source document**, preferably its TeX source, under `refs/`. Memory, a
   summary, a web page read once, or another model's report is not ground truth. If the source is not on disk,
   fetch it first.
5. **Work is tracked in beads** (`bd ready`, `bd show <id>`, `bd close <id>`). State between sessions is in
   `HANDOFF.md` and `docs/worklog/`. Parallel lanes are run from `lanes/` (rules in `lanes/COMMON.md`); only the
   orchestrator commits. How the work is run, and what is not done any more: `docs/workflow.md`.
6. **Thin working slices.** A feature is built as the smallest piece that a user can call and that computes
   something true, end to end (header, code, test against the reference, driver or Julia call), before it is
   widened. Review effort goes where defects were found: adversarial review with the reviewer's own oracle,
   and review of a design before code.
