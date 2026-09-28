<!-- ROLE: live state. UPDATE POLICY: every session end, newest entry first. -->

# HANDOFF: adelefeld

## Session 2026-09-28 (evening), from 17:25: START HERE

**One line.** Both blockers of the review of milestone 1 are repaired and merged; the dump form is reviewed
(no blocker, 4 MAJOR); two repair lanes run (m1-repair-text, m1-repair-dump).

**Master** (`origin/master`, pushed): `make check` 38 test programs with gcc, clang, `SAN=1`;
`sh tests/test_driver.sh`, `sh tests/test_julia.sh`, `sh tests/test_exports.sh` pass (checked 18:50).

| Lane | State |
|---|---|
| m1-repair-recon (arith R1, R2) | merged `d378c31`; header `recon.h` edited by the orchestrator (`9ea33b6`) |
| m1-repair-adele (arith R3, R4, R6) | merged `df32c71`; adf-c43 closed |
| m1-repair-ctx (contexts R1, R2, R6) | merged `2efa43f`; adf-rki closed |
| m1-review-dump (codex) | merged `9d216fd`; adf-8ju closed; `docs/reviews/m1/dump/review.md` |
| m1-repair-text | resumed 18:30, pi deepseek (OpenRouter); running at the time of writing |
| m1-repair-dump (adf-tp2) | started 18:52, pi space-bunny-alpha; brief on master; running at the time of writing |
| m1-repair-tools | paused as before (WIP commit on its branch); run it ALONE, last |
| m1-repair-driver | brief on master, not started; start after m1-repair-text has landed |
| m1-invariants (adf-xk4) | brief on master (`lanes/m1-invariants/brief.md`); runs alone after the repairs |
| closure check (adf-igt) | rules and table of judges on master (`lanes/m1-closure/COMMON.md`) |

If a lane is found dead (no `pi` process, no `DONE` in `lanes/<lane>/lane.log` of its worktree), start it again
with the same command; it continues from its files.

**Waits for TJO.** Decision M1-D9 (`docs/SPEC.md` section 15), PROPOSED by the orchestrator: the bound `2^20`
on the binary exponents of a `qclass` piece in the dump form is kept, named `ADF_DUMP_QCLASS_EXP_MAX`,
documented, and applied at stage 4 of conventions 8.5. Lane m1-repair-dump follows it. When it lands, the
constant goes into `include/adelefeld/dump.h` and conventions 8.4 (the lane's report gives the text).

**Left open by the landed lanes** (none blocks; all go to the mutation sweep adf-xf4 or to adf-xrt):
- Survivors of mutation: 10 in `src/recon.c`, 13 in `src/adele.c` (reasons in the lane reports). Lane
  m1-repair-ctx ran only 40 mutants per file (`src/scaled.c`, `src/modctx.c`): the full run did not finish
  in 40 minutes.
- `tools/mutate/equivalent.txt` lines 52 to 65 (`src/adele.c`): stale lines, and the reason "same value" is
  false for `arb_mul`, `acb_mul`. Also stale for `src/scaled.c` (lines 447, 547 are now 508, 540).
- `tests/ref/vectors/m1-adele/set_rat.jsonl`: one record edited by hand (M1-D4); the generator
  `lanes/m1-adele/gen_adele_vectors.py` still writes the old answer. Do not regenerate before it is changed.
- `tests/test_modctx_limits.c` runs its three slow cases (about 5 minutes) only with
  `ADF_MODCTX_LIMITS_FULL=1` (`lanes/m1-repair-ctx/run_limits.sh`); `make check` uses `k = 2000`.
- Sources pending: the code of `arb_get_interval_fmpz_2exp`; the documentation of `n_root` and of the
  `sign` argument of `fmpz_multi_CRT_precomp`.
- HEADER-FINDING of m1-repair-ctx: for `k` above the cap, `adf_modctx_new_blocks` returns `UNSUPPORTED`
  before the per-element `DOMAIN` checks (it may not read the array); the header's "DOMAIN first" needs a
  clause.

**The clock.** The WIP commits of the pause carry the time 20:20 and the system clock read 17:27 when this
session began (synchronised). Times of the afternoon in logs and in the section below are about three hours
ahead.

**Order of work from here.** Land m1-repair-text and m1-repair-dump; start m1-repair-driver; then
m1-invariants alone; m1-repair-tools alone (overnight); closure check; benchmarks on a quiet machine;
milestone S. The rules of the section below still bind.

## Session 2026-09-28 (day), paused at 16:10 by TJO (network down)

**One line.** Milestone 1 is implemented (193 of 193 functions) and reviewed; the review found defects;
six lanes of repair and review are paused with their work saved on branches; nothing is running.

**Master** (`origin/master`, everything pushed): `make check` 34 test programs, with gcc, clang, `SAN=1`;
`sh tests/test_driver.sh`, `sh tests/test_julia.sh`, `sh tests/test_exports.sh` pass. Run `make clean`
between a `SAN=1` build and a plain one (stale objects give link errors).

**First commands of the next session.**

    cd ~/Projects/adelefeld && git pull && bd ready | head -20
    make clean && make -j2 check 2>&1 | tail -1          # expect: check passed: all 34 test programs
    git worktree list; git branch -r                      # the six lane branches below
    free -g; ~/Projects/quota-app/target/release/quota    # memory and quota before launching anything

**The six paused lanes.** Each has a worktree `../adelefeld-wt/<lane>/` on branch `lane/<lane>` (pushed to
origin), with ONE commit "WIP <lane>" on top of its base. The work is unfinished, unreviewed, unmerged. None
has a `report.md`. The briefs (`lanes/<lane>/brief.md` in the worktree) end with a resume note that tells the
lane to continue from its files.

| Lane | Task | Model used | What the WIP commit holds | Resume with |
|---|---|---|---|---|
| m1-repair-recon | review arith R1, R2: wrong result and abort of `adf_adele_reconstruct` for extreme exponents (M1-D3) | pi space-bunny-alpha | `src/recon.c` repaired, red and green logs, a mutation run with 10 survivors of 89 to deal with; `make check` passed at 15:35 | `tools/orch/wt_lane.sh m1-repair-recon pi openrouter/stealth/space-bunny-alpha high` |
| m1-repair-ctx | review contexts R1 (wrong `lost` with `y = x`), R2 (primorial, M1-D5), R6 | pi deepseek (OpenRouter) | 25 minutes of work, 8 files; state not checked | `tools/orch/wt_lane.sh m1-repair-ctx pi "openrouter/~deepseek/deepseek-flash-latest" high` |
| m1-repair-adele | review arith R3, R4, R6 (M1-D4) | pi deepseek (OpenRouter) | `src/adele.c`, comments in `rat.c`, `fball.c`, `tests/test_adele_lowprec.c`; one vector record changed (accepted, see its brief); `make check` passed | same command with `m1-repair-adele` |
| m1-repair-text | review text R2, R3, R4, R9, R10 (M1-D6, M1-D7) | pi deepseek (OpenRouter) | `src/text.c`, `proto/text_grammar.py`, fuzz corpus prefixed (575 files); `make check` passed | same command with `m1-repair-text` |
| m1-repair-tools | review surface R1 to R6; keys of `equivalent.txt` without line numbers; `--san`; then the mutation sweep over all of `src/` | pi space-bunny-alpha | `mutate.py`, `selftest.py`, `equivalent.new.txt`; `make check` passed | `LANE_TIMEOUT=14400 tools/orch/wt_lane.sh m1-repair-tools pi openrouter/stealth/space-bunny-alpha high`; run it ALONE |
| m1-review-dump | review of the dump form (adf-8ju) | codex gpt-6-astra xhigh | 20 minutes of reading and reproducers, no review yet | `MAXRETRY=3 tools/orch/wt_lane.sh m1-review-dump codex gpt-6-astra xhigh` (no `session.id` was saved, so the runner starts the review again; its reproducers so far are in the WIP commit) |

The runners `tools/orch/*.sh` refuse to start while `lanes/STOP` exists in the worktree; none exists now.
Large files `lanes/<lane>/events.jsonl` are ignored by git and are not on the branches.

**How to land a lane** (the orchestrator does this, never the lane): read `lanes/<lane>/report.md`; rerun
`make clean && make -j2 check SAN=1` and `make clean && make -j2 check` in the worktree; read the diff of
anything the lane did not own; `git add -A && git commit` in the worktree (delete binaries first);
`git merge --no-ff lane/<lane>` on master; run all checks on master; `bd close`; push. Expect conflicts in
`tools/mutate/equivalent.txt` (take the lines out of the lane commit and add them after the merge) and in
`src/fball.c` comments between m1-repair-adele and later lanes.

**Rules of this session that bind the next** (memory: `orchestration-model-tiers`, `claude-worktree-base`):
- At most two or three lanes at once, never two mutation runs at once: at 15:30 the harness killed five
  lanes because the laptop was low on memory.
- Cheapest model that can do it. pi models for repairs (free or cents). Author and reviewer of different
  model families. Claude subagents only while the quota app shows the weekly window behind pace (16:00: weekly
  81%, 3 points behind; Fable weekly 75%, 9 behind; both reset 2026-09-29 18:00). Codex: TJO authorised up
  to 48% of the weekly window (meter 32% at 15:25).
- Claude subagents with worktree isolation start from the PUSHED commit: push first, paste the whole task
  into the prompt, save their report from the final message (they cannot write `report.md`).
- Never `pkill -f <pattern>`, and never `kill` from a command line that contains the pattern: it kills the
  calling shell. Use `ps` first, then `kill <pid>` in a second command.

**Decisions of 2026-09-28, all accepted by TJO** (`docs/SPEC.md` section 15): M1-D1 language of the driver;
M1-D2 what `is_canonical` promises about pointers; M1-D3 `ADF_LIMIT` in `adf_adele_reconstruct`
(`ADF_RECON_EXP_MAX = 2^20`); M1-D4 `prec` below 2 is 2, exact integers for rational factors; M1-D5 a
context has at most 65536 blocks; M1-D6 a printer may return NULL (`ADF_PRINT_EXP_MAX = 100000`); M1-D7 no
hidden bound on decimal exponents; M1-D8 milestone S comes directly after milestone 1. The headers carry
M1-D2 to M1-D6 already; the code follows when the repair lanes land.

**The review of milestone 1** (`docs/reviews/m1/<name>/review.md`, rules `lanes/m1-review/COMMON.md`):

| Reviewer | Model | Findings | State |
|---|---|---|---|
| local | codex gpt-6-astra | 1 BLOCKER, 2 MAJOR, 1 MINOR | R2 settled by M1-D2; R1, R4 merged (lane m1-repair-cap); R3 open: adf-xk4 |
| arith | Claude opus | 1 BLOCKER, 2 MAJOR, 3 MINOR | lanes m1-repair-recon, m1-repair-adele (paused) |
| contexts | Claude opus | 1 BLOCKER, 2 MAJOR, 3 MINOR | lane m1-repair-ctx (paused); R3, R4 done by lane m1-dump |
| text | codex gpt-6-astra | 1 BLOCKER, 6 MAJOR, 3 MINOR | R1 by M1-D2; R5 by m1-dump; R6 merged; R7 is adf-xk4; R8 by the header edit; lane m1-repair-text (paused) |
| surface | Claude opus | 0 BLOCKER, 3 MAJOR, 12 MINOR | lane m1-repair-tools (paused); lane m1-repair-driver: brief committed, never started |
| dump | codex gpt-6-astra | none yet | lane m1-review-dump (paused) |

No reviewer found a wrong enclosure in the tight arithmetic, the local backend, the scaled arithmetic or the
reader of the value form (about 600000 cases with their own oracles).

**Order of work to close milestone 1.**
1. Resume m1-repair-recon and m1-repair-ctx (the two blockers); land them.
2. Resume m1-repair-adele and m1-repair-text; land them. Resume m1-review-dump next to them (codex is light).
3. Start m1-repair-driver (`tools/orch/wt_lane.sh m1-repair-driver pi openrouter/stealth/space-bunny-alpha
   high`; its brief is on master) after m1-repair-text has landed (it uses the printer's new rule).
4. `ADF_CHECK_INVARIANTS` (adf-xk4): write the brief; it touches every file of `src/`, so it runs alone after
   the repairs have landed; add a way to run the suite with the flag.
5. Repairs of what the dump review finds.
6. m1-repair-tools alone (best overnight): new key format, then the mutation sweep over all of `src/`.
   `tools/mutate/equivalent.txt` on master is stale for `src/fball.c` and `src/scaled.c`; entries proposed by
   lanes wait in `lanes/m1-local/`, `lanes/m1-scaled/`, `lanes/m1-dump/` (`equivalent-added.txt`), in
   `lanes/m1-repair-cap/report.md` (three lines) and in `lanes/m1-modctx-b/report.md` (two of its ten are
   out-of-bounds reads, not equivalent).
7. Closure check (adf-igt): each reviewer re-judges its findings against the repaired code; codex for the
   code written by Claude (text, local, cap, dump), Claude opus for the rest. Briefs to be written on the
   pattern of `docs/reviews/m0-gate/closure.md`.
8. Benchmarks on a quiet machine (`make bench`, nothing else running); then mark milestone 1 done in
   `docs/PLAN.md` section 6 and update the numbers there.
9. Then milestone S (M1-D8): briefs for S.3 (partial rational reconstruction) and S.1 (systems modulo `N`,
   Hermite form with transformation).

**Open issues** are in beads (`bd ready`, `bd list --status=in_progress`). In progress at the pause:
adf-c43 (repairs arith), adf-rki (repairs contexts), adf-8ju (dump review), adf-gf1 (review round 1).

**Things to know.**
- `deepseek/deepseek-flash` at its own provider has no credit; use `openrouter/~deepseek/deepseek-flash-latest`.
  `openrouter/xiaomi/mimo-v2.6-pro` was slow twice (no report after two hours).
- Valgrind 3.22: `~/.local/bin/valgrind` (unpacked into `~/.local/opt/valgrind`, no root).
- Julia's bundled libgmp lacks a symbol that the system FLINT needs; `tests/test_julia.sh` uses `LD_PRELOAD`.
- TJO's `~/.local/bin/codex-vision` was patched (one argument `errors="replace"`).
- Tate's thesis: `refs/src/tate-thesis/` (not in git): two scans, TeX of 43 pages refereed twice, a README with
  the state. Rule in `docs/sources.md`: the scan is the ground truth, the TeX is an aid. One edit is owed:
  p331, the product for |d| runs over p NOT in S_infty (read in the second scan, recorded in the file, the
  formula not yet changed).
- Old worktrees of finished lanes under `../adelefeld-wt/` and `.claude/worktrees/` can be removed with
  `git worktree remove` when disk space matters; their branches are merged.

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
