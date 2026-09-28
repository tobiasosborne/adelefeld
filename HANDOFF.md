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

**State (2026-09-28 07:15), session closed by TJO.** Milestone 0 is complete (gate passed after edits, applied).
Milestone 1: public headers (`include/adelefeld/`, `docs/api-m1.md`; not yet reviewed), test infrastructure
(`tests/support/`, `make fuzz`, `make mutate`), and on master the first library code: status strings, places,
exact rationals (`src/status.c`, `place.c`, `rat.c`, `inlines.c`) and finite balls with tight arithmetic in the
global backend (`src/fball.c`), with `bench/bench_fball.c`. `make check` and `make check SAN=1`: 9 test programs
pass. 78 of the 193 declared functions are implemented. No lane is running. Everything is pushed to origin.

**Next.**
1. Lane m1-modctx failed without output (`pi` with `openrouter/xiaomi/mimo-v2.6-pro` returned at once, three
   times). Check the provider's credit with a one-line prompt, then run the lane again, with another model if
   needed: `tools/orch/wt_lane.sh m1-modctx pi <model>`.
2. Wave B, each lane in its own worktree: adeles and complex adeles (1.3), value text with a fuzz target (1.4),
   reconstruction (1.6), scaled policy and cap (1.7), local backend (1.8, after modctx), dump; the remaining
   functions of `common.h` (`adf_str_free`, version check). Briefs are still to be written; the pattern is
   `lanes/m1-fball/brief.md` with `lanes/COMMON-C.md`.
3. Wave C: driver `adf` (1.5), Julia interface check (1.9), benchmark rows on a quiet machine.
4. Then the first code review round (codex `gpt-6-astra`; task: an input that breaks enclosure). It also
   reviews the headers, the 15 choices of `docs/api-m1.md`, and the closure edits E3, C3, C4.

**Open items.**
- `make mutate FILES=src/fball.c` takes about 320 s; the orchestrator's repeat after adding the three excused
  mutants to `tools/mutate/equivalent.txt` was cut off and must be run once to the end.
- `ADF_CHECK_INVARIANTS` (conventions 4.4) is promised and not implemented (finding of lane m1-rat).
- `fball.h` documents local-backend behaviour that `src/fball.c` does not implement (HEADER-FINDING 1 of lane
  m1-fball); it belongs to 1.8.
- `lanes/m0-conventions/write_golden.py` does not reproduce `tests/golden/`: do not run it.
- A quiet-machine benchmark run; `ideles.md` Definition 8 against CV-16; four sources pending
  (`lanes/m0-sources/report.md`).
- When committing in a lane's worktree, never build into a directory that git does not ignore (`build-*/` is
  ignored now).

**Machine and quota.** The laptop ran out of memory once (02:39, swap full); build with `make -j2`. After that
the harness stopped the orchestrator's waiting loop, so lanes on pi or codex end without a signal: look at
`lanes/<lane>/lane.log` in the worktree. Quota at 07:10: Claude weekly 69%, Fable weekly 67% (both reset
2026-09-29 18:00), codex 19% of the week (11 of the 30 points TJO authorised are used; resets 2026-10-03).
