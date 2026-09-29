# Lane s-closure: closure check of the review of the design of milestone S

You continue the review `docs/reviews/s-design/review.md` (28 VALID, 3 MINOR, 2 INVALID, NOT READY; written by
codex `gpt-6-astra`). The design has been repaired since by another model family (Claude Sonnet):
`lanes/s-design-repair/report.md` is the table of answers. You judge each finding of the review against the
design as it stands: `docs/proofs/solvers.md`, `proto/solvers_checks.py`, `docs/api-s.md`, and table 3 of
`docs/sources.md`. The pattern is `lanes/m1-closure/COMMON.md` (read it: rules 1 to 6 and the four verdicts
hold here, with "the code" read as "the design and its reference algorithms").

**You own:** `docs/reviews/s-design/closure.md`, `docs/reviews/s-design/closure-checks/`, `lanes/s-closure/`.
Everything else is read only.

1. **Do not trust the report of the repair.** For each finding of the review (the statements, the eight
   items of "Reference algorithm failure and interface contradictions", the corrections of table 3, the
   decisions, the edits, the limitations of the checks): read the repaired text and run the repaired check.
   Verdict: `CLOSED`, `CLOSED WITH EDIT` (the edit written out exactly), `OPEN` (with the input or the gap).
2. **Attack the repairs.** A repair that fixes only the reviewer's input is `OPEN`: try the neighbouring
   input (`X - 10^400` was repaired: try huge coefficients in other positions, a huge leading coefficient,
   negative ones; `27 X` at 3: try content at 2, content in the leading coefficient; and so on).
3. **The statements that no reviewer has seen** are new findings territory: Propositions 1.11, 3.6 (was a
   remark), 3.12, 3.13, the new Lemma 3.1(3), the new claims of P3.2 (2 and 6), P3.10 (step 6, claim 4),
   P1.7 claim 3 as restated, and the decisions S-D16 to S-D19. Review them in refute mode as the first
   review did: your own brute force in `closure-checks/`, the proof read step by step, the sources read at
   the cited lines under `refs/src/`. Verdict for each: VALID, MINOR (with the replacement text), INVALID.
4. **The checks of the repair:** `proto/solvers_checks.py` has 34 checks now. Judge the new ones: would a
   wrong algorithm pass? Make two or three mutants of the reference algorithms of your own choice and see
   whether the checks kill them.
5. Your suites `real` and `matrix` of `docs/reviews/s-design/review_checks.py` no longer run clean against
   the repaired file (the report of the repair, section 3, says why). Do not edit them; say in `closure.md`
   whether the reasons given are true.

Rules: no git command that changes state, no `bd`; at most 2 cores; no computation above about 3 minutes; a
mutation sweep runs on this machine, wait if `free -g` shows less than 6 GB available. Plain, sober English;
numbers, not adjectives; no praise; lines at most 116 characters. Leave no binary in the directories you
write.

Write `docs/reviews/s-design/closure.md`: the title; on the next line the counts by verdict and one of
`MAY BE IMPLEMENTED`, `MAY BE IMPLEMENTED AFTER THE EDITS BELOW`, `NOT READY`; the table of the findings of
the review with verdicts; the table of the new statements with verdicts; a section for each item that is not
plainly closed or valid; the edits, ready to paste; your checks with command and output; what you did not
examine. Then `lanes/s-closure/report.md` with the counts and the important items in one line each. Your
final message contains the complete text of `closure.md`.
