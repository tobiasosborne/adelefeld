# Lane m1-closure-text: closure check of the review `text` of milestone 1 (adf-igt)

Read `lanes/m1-closure/COMMON.md` and `lanes/m1-review/COMMON.md` completely: they are your rules. Where they
and `lanes/COMMON.md` differ, the two files named here win (you write under `docs/reviews/m1/text/` and in
your lane directory, nowhere else).

Your review is `docs/reviews/m1/text/review.md`, with its reproducers in `docs/reviews/m1/text/checks/`.

| Review | Findings | Repairs and decisions to read |
|---|---|---|
| text | R1 to R10 | `lanes/m1-repair-text/report.md`; lane m1-dump (R5); M1-D2, M1-D6, M1-D7; for R7 `lanes/m1-invariants/report.md` (the borrow count of contexts under `INV=1`, written by Claude Sonnet: attack it) |

The decisions are in `docs/SPEC.md` section 15. M1-D1 to M1-D9 are accepted by TJO; M1-D10 and M1-D11 are
proposed by the orchestrator and not yet accepted: judge the code against them, and say in `closure.md` if
you hold one of them to be wrong, with the input that shows it. Every lane named above has landed.

The flag: `make clean && make -j2 check INV=1` builds with `-DADF_CHECK_INVARIANTS`. Five older test
programs fail under the flag because the tests step outside the contract (`lanes/m1-invariants/report.md`;
a lane repairs them now, issue adf-6vy). That is known: do not report it again; report what is NOT known.

Other lanes build on this machine: `make -j2`, never two builds at once, and wait if `free -g` shows less
than 6 GB available. `valgrind` is `~/.local/bin/valgrind`. Leave no binary in the directories you write:
build into a directory `build-closure/` and delete it at the end.

Write `docs/reviews/m1/text/closure.md` as the rules say, new programs in
`docs/reviews/m1/text/closure-checks/`. Then write `lanes/m1-closure-text/report.md`: the counts by verdict,
the words `BLOCKER OPEN` or `NO BLOCKER OPEN`, the list of the files you wrote, every command you ran with
its result, and what you did not examine. Your final message contains the complete text of `closure.md`.
