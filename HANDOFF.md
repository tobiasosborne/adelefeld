<!-- ROLE: live state. UPDATE POLICY: every session end, newest entry first. -->

# HANDOFF: adelefeld

## Session 2026-09-28 (day): milestone 1 implemented and reviewed; repairs of the review in progress

**State (15:40).** All 193 declared functions of milestone 1 are on master (`make check`: 34 test programs,
with gcc, clang and `SAN=1`; `tests/test_driver.sh`, `tests/test_julia.sh`, `tests/test_exports.sh` pass).
Everything is pushed. The milestone is NOT closed: the code review found defects.

**Code review, round 1** (`docs/reviews/m1/<name>/review.md`, rules `lanes/m1-review/COMMON.md`):

| Reviewer | Model | Findings | Repair |
|---|---|---|---|
| local | codex gpt-6-astra | 1 BLOCKER, 2 MAJOR, 1 MINOR | R2 settled by M1-D2; R1, R4 merged (`lanes/m1-repair-cap`); R3 open (adf-xk4) |
| arith | Claude opus | 1 BLOCKER, 2 MAJOR, 3 MINOR | lanes m1-repair-recon, m1-repair-adele (stopped, see below) |
| contexts | Claude opus | 1 BLOCKER, 2 MAJOR, 3 MINOR | lane m1-repair-ctx (stopped); R3, R4 done by lane m1-dump |
| text | codex gpt-6-astra | 1 BLOCKER, 6 MAJOR, 3 MINOR | R1 by M1-D2; R5 by lane m1-dump; R6 merged; lane m1-repair-text (stopped) |
| surface | Claude opus | 0 BLOCKER, 3 MAJOR, 12 MINOR | lane m1-repair-tools (stopped); lane m1-repair-driver (brief written, not started) |

No reviewer found a wrong enclosure in the tight arithmetic, the local backend, the scaled arithmetic or the
reader of the value form. Decisions M1-D1 to M1-D7 of the orchestrator are in `docs/SPEC.md` section 15; TJO has
not yet said whether he accepts M1-D2 to M1-D7 (M1-D6 changes the interface: a printer may return NULL).

**Five repair lanes were stopped at 15:30 by the harness because the machine ran low on memory** (not a
failure of the lanes; do not restart without TJO's word). Their work is in the worktrees under
`../adelefeld-wt/<lane>/`, uncommitted; in each of them `make check` passes as the lane left it:

| Lane | Model | What is there |
|---|---|---|
| m1-repair-recon | pi space-bunny-alpha | `src/recon.c` changed; red and green logs; a mutation run of the lane was still running at 15:35 (orphan, `nohup`) |
| m1-repair-adele | pi deepseek (OpenRouter) | `src/adele.c`, comments in `rat.c`, `fball.c`; `tests/test_adele_lowprec.c`; it changed `tests/ref/vectors/m1-adele/set_rat.jsonl`, which it does not own: look at that first |
| m1-repair-ctx | pi mimo-v2.6-pro | nothing written |
| m1-repair-text | pi deepseek (OpenRouter) | `src/text.c`, `proto/text_grammar.py`, corpus prefixed (288 files) |
| m1-repair-tools | pi space-bunny-alpha | `mutate.py`, `selftest.py`, a converted `equivalent.new.txt` |

To resume a lane: `tools/orch/wt_lane.sh <lane> pi <model> high` starts a new first attempt in the same
worktree (the files are kept; the brief tells the lane to read what is there). Run at most two or three
lanes at once, and never two mutation runs at once: the memory pressure came with five lanes, each with its
own build and mutation run, next to the desktop.

**Next.**
1. TJO: accept or change M1-D2 to M1-D7; say which budget the dump review and the closure check may use
   (codex is at 32% of the 38% authorised; Claude weekly 81%, 3 points behind pace).
2. Resume the five repair lanes, two at a time; then m1-repair-driver; then `ADF_CHECK_INVARIANTS` (adf-xk4).
3. Review of the dump form (adf-8ju), by a model of another family than Claude opus.
4. Closure check (adf-igt): each reviewer re-judges its findings against the repaired code.
5. Mutation sweep with keys without line numbers (part of m1-repair-tools); `equivalent.txt` is stale for
   `src/fball.c` and `src/scaled.c`; entries proposed by lanes are in `lanes/*/equivalent-added.txt`.
6. Benchmarks on a quiet machine; then close milestone 1 in `docs/PLAN.md`.

**Things to know.**
- Claude subagents with worktree isolation start from the pushed commit: push before launching; paste the
  task into the prompt; they cannot write `report.md` (save it from the final message).
- `pkill -f` with a pattern that matches the calling shell kills it: use `ps` and `kill <pid>`.
- `deepseek/deepseek-flash` at its own provider has no credit; use `openrouter/~deepseek/deepseek-flash-latest`.
- Valgrind 3.22 is in `~/.local/opt/valgrind` (wrapper `~/.local/bin/valgrind`), installed without root.
- Julia's bundled libgmp lacks a symbol that the system FLINT needs; `tests/test_julia.sh` uses `LD_PRELOAD`.
- Tate's thesis: `refs/src/tate-thesis/` (not in git), two scans, TeX refereed twice, README there.
  `docs/sources.md` has the rule: the scan is the ground truth, the TeX is not.


## Session 2026-09-27 (evening) to 2026-09-28 (morning): orchestration set up, first wave of milestone 0 launched

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
- Done (see below): the repeat of `make mutate FILES=src/fball.c` with the three excused mutants.
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

**Mutation tool fixed (2026-09-28, later session; issue adf-98j closed).** `run_make` of
`tools/mutate/mutate.py` starts each run in a session of its own and kills the whole process group on timeout.
`make mutate-selftest` now also fails if a process of a mutant is alive after the runs (seen failing before the
fix, with two processes left). `make mutate` may be run again. `make mutate FILES=src/fball.c` was run to the
end: 184 mutants, 146 killed, 0 survived, 35 not compiled, 0 timed out, 3 excused; no process left. The run
took 4030 s of wall time and not 320 s; a `cargo build` of another project ran on the machine at the end (load
average 16), the rest of the cause was not examined. The note below is kept for the record.

**Found after the session (2026-09-28 10:45).** `tools/mutate/mutate.py` does not kill the test program of a
mutant that times out: nine such processes ran for eight hours (load average 10) until killed by hand. Do not
run `make mutate` before this is fixed (issue in beads, priority 1). The full swap file is not a fault: 2 GB of
swap hold pages of long-running desktop programs after five days of uptime, with 23 GB of memory available.
The OpenRouter credit is used up (TJO): the models `space-bunny-alpha` (free) may still work, `mimo-v2.6-pro`
through OpenRouter does not; `deepseek-flash` goes through its own provider.
