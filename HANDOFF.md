<!-- ROLE: live state. UPDATE POLICY: every session end, newest entry first. -->

# HANDOFF: adelefeld

## Session 2026-09-27 (evening): orchestration set up, first wave of milestone 0 launched

**Mode of work** (TJO, 2026-09-27): wide, fast, rigorous; an orchestrator (Claude Fable) dispatches lanes and is
the only one who commits. Model tiers: pi `openrouter/stealth/space-bunny-alpha` (generic code), pi
`deepseek/deepseek-flash` and `openrouter/xiaomi/mimo-v2.6-pro` (more demanding), codex `gpt-6-luna` / Claude
sonnet (routine), codex `gpt-6-sol` xhigh / Claude opus (hard), codex `gpt-6-astra` xhigh (critical derivations and
reviews). Budgets: Claude while the quota app shows the weekly window behind pace
(`~/Projects/quota-app/target/release/quota`); codex up to 30 points of the weekly window (meter was 8% at the
start). Code review rounds only after major features land. Author and reviewer of a proof or of code come from
different model families.

**Mechanics.** `lanes/COMMON.md` holds the rules for every lane (disjoint file ownership, no git, no `bd`, report
in `lanes/<lane>/report.md`). `tools/orch/codex_lane.sh <lane> <model> [effort]` and
`tools/orch/pi_lane.sh <lane> <provider/model> [thinking]` run a lane from `lanes/<lane>/brief.md`, with resume.
`touch lanes/STOP` stops new attempts. Tracker: `bd ready`.

**State (2026-09-28 03:50).** Milestone 0 is complete: the gate passed after edits, and the edits are applied
(conventions 0.4; SPEC, PLAN 1.3; PERF 1.1). Milestone 1 has begun: public headers for its types
(`include/adelefeld/`, `docs/api-m1.md`; not yet reviewed), test infrastructure (`tests/support/`, `make fuzz`,
`make mutate`). No function of the library is implemented on master yet.

**Running (wave A, each in its own worktree under `../adelefeld-wt/<lane>`, branch `lane/<lane>`):** m1-rat,
m1-fball, m1-modctx. When a lane has written `lanes/<lane>/report.md` in its worktree: run `make check` and
`make check SAN=1` there, commit in the worktree, merge the branch into master, remove the worktree.

**Next.** Wave B after wave A is merged: adeles and complex adeles (1.3), value text with a fuzz target (1.4),
reconstruction (1.6), scaled policy and cap (1.7), local backend (1.8), dump. Wave C: driver `adf` (1.5), Julia
interface check (1.9), quiet-machine benchmark rows. Then the first code review round (codex gpt-6-astra for the
code written by pi; task: an input that breaks enclosure), which also reviews the headers.

**Open from milestone 0.** A quiet-machine benchmark run; `lanes/m0-conventions/write_golden.py` does not
reproduce `tests/golden/` (do not run it); `ideles.md` Definition 8 against CV-16; four sources pending
(`lanes/m0-sources/report.md`); E3, C3, C4 of the closure check were applied by a lane that wrote no report and
were not checked line by line.

**Machine.** The laptop ran out of memory once (02:39, swap full). Build with `make -j2`; fuzzing is the likely
cause and needs a memory cap.
