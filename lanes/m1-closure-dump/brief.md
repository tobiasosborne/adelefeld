# Lane m1-closure-dump: closure check of the review `dump` of milestone 1 (adf-igt)

Read `lanes/m1-closure/COMMON.md` and `lanes/m1-review/COMMON.md` completely: they are your rules. Where they
and `lanes/COMMON.md` differ, the two files named here win (you write under `docs/reviews/m1/dump/` and in
your lane directory, nowhere else).

Your review is `docs/reviews/m1/dump/review.md`, with its reproducers in `docs/reviews/m1/dump/checks/`.

| Review | Findings | Repairs and decisions to read |
|---|---|---|
| dump | all (R1 to R4 and what else the review lists) | the repairs of the dump review: `lanes/m1-repair-dump/report.md`; M1-D5, M1-D9 |

The decisions are in `docs/SPEC.md` section 15 (M1-D1 to M1-D9, all accepted by TJO). Your worktree is at
the master of 2026-09-28, 23:30; every repair lane named above has landed. The lanes m1-invariants
(`ADF_CHECK_INVARIANTS`) and m1-repair-tools (mutation tool, memory checker) have NOT landed: a finding that
waits for one of them is named under "not examined" and stays `OPEN`.

Other lanes build on this machine: `make -j2`, never two builds at once, and wait if `free -g` shows less
than 6 GB available. `valgrind` is `~/.local/bin/valgrind`.

Write `docs/reviews/m1/dump/closure.md` as the rules say, new programs in
`docs/reviews/m1/dump/closure-checks/`. Then write `lanes/m1-closure-dump/report.md`: the counts by verdict,
the words `BLOCKER OPEN` or `NO BLOCKER OPEN`, the list of the files you wrote, every command you ran with
its result, and what you did not examine. Your final message contains the complete text of `closure.md`.
