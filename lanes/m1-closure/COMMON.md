# Closure check of the code review of milestone 1: rules for every reviewer (adf-igt)

You wrote, or you continue, the review `docs/reviews/m1/<name>/review.md`. The code has been repaired since.
You now judge each finding of that review against the code as it stands on master. The pattern is
`docs/reviews/m0-gate/closure.md`. The rules of `lanes/m1-review/COMMON.md` bind you as before (refute
mode, your own oracle, no file changed outside your directory, no git, no `bd`, 2 cores, 3 minutes).

1. **Do not trust the repair reports.** Read `lanes/m1-repair-*/report.md` for what was changed, then judge
   from the code and from runs. A finding is closed by a run, not by a sentence.
2. **Every reproducer is run again.** Build the library from master in your worktree (`make clean &&
   make -j2`) and run each program of `docs/reviews/m1/<name>/checks/` against it, unchanged. Where the
   interface changed by a decision (`docs/SPEC.md` section 15, M1-D1 to M1-D8) so that the old reproducer
   no longer states the requirement, write a new one next to it (`<old name>_closure.<ext>`) and say which
   decision changed what.
3. **Attack the repair.** For each repaired finding, search for an input near the old one that still
   breaks the contract: the neighbouring function with the same pattern, the aliased call, the other
   backend, the bound plus and minus one, the other sign. A repair that fixes only the reproducer's input
   is `OPEN`.
4. **Judge the new tests.** Each repair came with a test. Say whether the test would have failed on the
   old code (the lane's red log claims it; check one case yourself by reverting the hunk in a scratch
   copy) and whether its expected values come from the contract and not from the program.
5. **Decisions.** A finding settled by a decision and not by code is judged against the text of the
   decision: do the headers, `docs/conventions.md` and the code now say the same thing?
6. **New defects** that you meet on the way are new findings, numbered on from your review
   (`R<k>`), with severity, input and reproducer, as in the review.

Verdict for each finding, one of:
`CLOSED` (repaired, reproducer passes, no neighbouring input found, with the number of cases tried);
`CLOSED WITH EDIT` (the remaining edit is written out exactly: file, line, old text, new text);
`OPEN` (with the input that still fails and its reproducer);
`SETTLED BY DECISION` (with the identifier, and the places where the documents still disagree, if any).

Write `docs/reviews/m1/<name>/closure.md`: the title; on the next line the counts by verdict and the word
`BLOCKER OPEN` or `NO BLOCKER OPEN`; a table with one row for each finding (identifier, severity of the
review, verdict, evidence in one line); then a section for each finding that is not plainly `CLOSED`;
then the new findings; then what you did not examine. New programs go to
`docs/reviews/m1/<name>/closure-checks/`. Your final message contains the complete text of `closure.md`.

## Who judges what

Author and judge are of different model families. The repairs were written by pi models (recon, ctx,
adele, text, tools, driver) and by Claude (cap).

| Review | Findings | Judge | Repairs to read |
|---|---|---|---|
| local | R1 to R4 | codex gpt-6-astra xhigh | m1-repair-cap; M1-D2; m1-invariants (R3) |
| text | R1 to R10 | codex gpt-6-astra xhigh | m1-repair-text; m1-dump (R5); M1-D2, M1-D6, M1-D7; m1-invariants (R7) |
| dump | all | codex gpt-6-astra xhigh | the repairs of the dump review |
| arith | R1 to R6 | codex gpt-6-astra xhigh | m1-repair-recon, m1-repair-adele; M1-D3, M1-D4 |
| contexts | R1 to R6 | codex gpt-6-astra xhigh | m1-repair-ctx; m1-dump (R3, R4); M1-D5 |
| surface | R1 to R15 | codex gpt-6-astra xhigh | m1-repair-tools, m1-repair-driver; M1-D1, M1-D6 |

TJO, 2026-09-28 (night): codex is the reviewer of everything; the repairs of arith, contexts and surface
were written by pi models, so codex is of another family there too. The reviews arith, contexts and surface
were written by Claude opus: their judge continues a review that it did not write.

The brief of a judge is this file and the line of the table. A lane that has not landed when the closure
check starts is named in `closure.md` under "not examined", and its findings stay `OPEN`.
