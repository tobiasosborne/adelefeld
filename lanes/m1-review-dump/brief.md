# Lane m1-review-dump: code review of milestone 1, reviewer `dump`

You are a reviewer, not an author. Read `lanes/m1-review/COMMON.md` (the rules of the review; where they
differ from the rules for lanes above, the rules of the review hold) and then `lanes/m1-review/dump.md`
(your task). You own `docs/reviews/m1/dump/` and `lanes/m1-review-dump/` and nothing else. Memory of the
laptop is short: one build at a time, `make -j2`, no more than two processes of yours at once; delete
build directories and binaries under `docs/reviews/m1/dump/checks/` when you are done (keep sources,
scripts and logs).

Write the review to `docs/reviews/m1/dump/review.md`, your reproducers to `docs/reviews/m1/dump/checks/`, and
finish with `lanes/m1-review-dump/report.md`: the verdict line, the list of findings with severity in one
line each, and the commands you ran with their results.
