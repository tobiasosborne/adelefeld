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

**State (2026-09-28 00:40).** Milestone 0: all work packages landed and committed (scaffold, sources, four proof
packages each reviewed by a different model family and repaired, conventions 0.2, benchmark harness, seams
sketch); SPEC, PLAN and PERF are version 1.1. The gate review (codex `gpt-6-astra`, lane `m0-gate`) is running.
The Python reference for the ring (`tests/ref/`) exists. Nothing of the C library is implemented; the public
header is empty until the gate passes. TJO: actual code is C, Python for tests only, a Julia layer later.

**Next.** Read `docs/reviews/m0-gate/review.md`; apply its edits; then milestone 1 in C: first the header for the
milestone-1 types (per-module headers under `include/adelefeld/`, so that parallel lanes own disjoint files), then
WP 1.2 to 1.8 in parallel lanes, tests against `tests/ref/vectors/` and `tests/golden/`; a code review round when
the ring has landed. Still open from milestone 0: a quiet-machine benchmark run; `docs/sources.md` row for
`uops-intel`; `ideles.md` Definition 8 against CV-16; four sources pending (`lanes/m0-sources/report.md`).
