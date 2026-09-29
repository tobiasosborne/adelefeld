# Workflow (TJO, 2026-09-29, after the night of 2026-09-28)

What the first two milestones showed about the process, and the rules that follow. The record is
`docs/worklog/2026-09-29.md`, last section.

## What found defects, and what did not

| Activity | What it found | Kept? |
|---|---|---|
| Adversarial review of code, the reviewer's own oracle, refute mode (milestone 1, round 1) | 4 blockers (wrong `lost` under aliasing; wrong results and aborts at extreme exponents; a predicate that read a pointer), about 20 majors | yes |
| Review of a design before code (milestone S) | 2 false statements, an overflow of the reference, contradictions of the interface, 2 mutants of the checks | yes |
| Reading the source before use (CLAUDE.md rules 3, 4) | Thue's lemma reversed in our own table; FLINT's count of real roots described wrongly | yes |
| The debug build `INV=1` | 5 test programs outside the contract | yes, it is cheap now |
| Six closure checks of milestone 1 | no defect of the code; one false sentence of the specification | reduced |
| Fuzzing, runs of 30 to 170 s | no defect of the library on record; the harness of the dump had a defect itself | changed |
| The mutation sweep | about 380 mutants in 2.5 hours: 2 gaps of the tests, 15 survivors without meaning, a stop for memory | dropped |
| Handoff and worklog commits after every event; reports copied by hand | nothing | dropped |

## Rules

1. **Thin working slices** (CLAUDE.md rule 6). Milestone S starts with one slice of S.3, not with the whole
   certified design: see `HANDOFF.md`. `docs/proofs/solvers.md` and `docs/api-s.md` are the map; a slice
   implements the part of them it needs and no more. A decision of `docs/api-s.md` section 5 is asked of TJO
   when a slice needs it, not before.
2. **Use the library.** Each slice ends with a call that a user would make (the driver `adf`, or Julia through
   `ccall`) and its result compared with the reference.
3. **Review.** One adversarial review after a slice or a group of slices has landed (codex `gpt-6-sol`;
   `gpt-6-astra` for a design or a proof), with the reviewer's own oracle. One closure check for the blockers
   and majors of a review together; minors are repaired and not judged again.
4. **Mutation testing** is a measure of the tests of changed files, with a bound (`lanes/COMMON-C.md` rule 5).
   No sweep. `tools/mutate/equivalent.txt` is not maintained beyond what rule 5 says; its 108 reasons were
   written and judged by one model and are not trusted as proofs.
5. **Fuzzing.** One differential target for each reader and each arithmetic slice: the same input to the C
   function and to the Python reference, the assertion is the contract (the result contains the true value;
   the status is the one of the conventions). Run for an hour or more, at night, alone. A run of seconds is
   called a smoke test in every report.
6. **Evidence.** A count of cases that passed is given with what would have made a case fail. A check by
   the orchestrator that reruns the author's or the judge's own program says so; it shows that the report is
   true, not that the check proves much.
7. **Records.** `HANDOFF.md` and the worklog are written once, at the end of a session (and before a risky
   step). Times are read from the clock (`date`). Lanes write their own result file
   (`lanes/COMMON.md` rule 8); the orchestrator does not copy reports by hand.
8. **Reports to TJO.** One report when something needs a decision or a session ends; not one for each lane.
9. **Machine.** Tests that can allocate without bound (removed guards under mutation, hostile dumps) run with
   a limit of memory (`ulimit -v`) and one job.
