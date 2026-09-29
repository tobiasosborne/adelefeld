<!-- ROLE: live state. UPDATE POLICY: every session end, newest entry first. -->

# HANDOFF: adelefeld

## Session 2026-09-28 (night), 22:30 to 23:05: START HERE

**One line.** The decisions that waited for TJO are ratified; the comparison of Sonnet effort levels is run and
the level is MEDIUM (agent type `sonnet-medium`); no code of the library changed; nothing is running.

**The comparison** (one task, one run per level; every result rebuilt by the orchestrator against the original
and the three mutants, script and copies in `~/Projects/adelefeld-wt/effort-cmp/`):

| Level | Verdicts right | Wall time | Tokens | Checks | Remarks |
|---|---|---|---|---|---|
| low | 3 of 3 | 78 s | 58599 | 3 | did not read off the statuses of the mutants; one input that did not kill, repaired |
| medium | 3 of 3 | 106 s | 66751 | 9 | statuses recorded (`LIMIT`, `DOMAIN`), controls added |
| high | 3 of 3 | 157 s | 89510 | 26 | wrote a probe program outside its directory; line numbers of `dp_word` cited wrong |

The tests of the three agents are NOT merged: master has its own test for the two mutants that can be killed
(`grammar_before_limits_in_sball_and_rfun`). Where a section below says "Sonnet at effort high", read medium.

**Standing order of TJO (22:55).** Orchestrate without asking for each lane: `sonnet-medium` for demanding
lanes, `space-bunny-alpha` as much as possible, codex for every review (until its weekly meter reads 50%;
it read 36%), Fable exceptionally for a task that needs significant cognition, and work as long as Claude is
under or on pace. Memory `orchestration-model-tiers`.

**RUNNING at 03:44 on 2026-09-29 (one job; look at it first).**

| Job | Model | Where | State |
|---|---|---|---|
| the mutation sweep (adf-xf4) | none: a script of the orchestrator, `lanes/m1-sweep/sweep.sh` | `../adelefeld-wt/m1-sweep` (branch `lane/m1-sweep`, from `27a8ea0`) | RESTARTED at 02:03 with the judge `make check INV=1` and `--san`: in the release build the lines `ADF_INV_...` are compiled away, and all 34 survivors of `cap.c` and `rat.c` in the first run were such lines (first run: `lanes/m1-sweep/release-run/`). 14 files, small first, `--limit 200`, seed 20260928, 100 minutes at most for a file; about 30 s for a mutant, about 12 hours. Table: `lanes/m1-sweep/sweep.md`; logs `lanes/m1-sweep/<file>.log`. Stop: `touch lanes/m1-sweep/STOP` there. It resumes where it stopped when started again |

**QUOTA at 03:20: Claude weekly 91% used, 0.3 points behind pace; Fable weekly 90%, 1.3 behind** (reset
2026-09-29 18:00); codex 46% (limit of TJO 50%). The Claude windows are ON pace: no further Claude subagent
is launched until the meter shows them behind again or the window resets. Codex and the script go on.

**The design of milestone S is DRAFT 3 and its review is closed** (`9600357`, 03:43). The closure check
(`540f57c`; codex gpt-6-sol, 15 minutes; `docs/reviews/s-design/closure.md`): of the 33 statements of the
first review 32 CLOSED, 1 CLOSED WITH EDIT, 0 OPEN; of the 14 new items 13 VALID, 1 MINOR, 0 INVALID; every
repair attacked with neighbouring inputs; verdict NOT READY before four minor edits R11 to R14. The
orchestrator applied the four edits as written (isolation formula of 3.11(5); the flag `reduced`; the seed
exponent in S-D18; the regression `check_s2_rr_finish`) and showed that the new check fails on the mutant
it is for. `proto/solvers_checks.py`: 35 checks, 0 failures. What is left before an implementation lane
starts: the decisions S-D1 to S-D19 of TJO (`docs/api-s.md` section 5), among them E-S4 (does SPEC 9.1
"multiple roots later" mean multiplicities later: S-D13), and then the edits of SPEC, PLAN and
conventions (`docs/api-s.md` section 6), E-C5 deferred. Sources pending: the intermediate value theorem,
Sturm's theorem.

**The design of milestone S is DRAFT 2** (`da222e9`; lane s-design-repair, Sonnet medium, 45 minutes, 542k
tokens; `lanes/s-design-repair/report.md`): every finding of the review answered, none contested; 34 checks
(were 25), both mutants of the reviewer killed (rerun by the orchestrator); decisions S-D16 to S-D19 added
(19 decisions wait for TJO in `docs/api-s.md` section 5); table 3 of `docs/sources.md` corrected. New
statements that no reviewer had seen: Propositions 1.11, 3.6, 3.12, 3.13 and new claims of P3.2, P3.10; the
closure check reviews them. No implementation lane starts before the closure says `MAY BE IMPLEMENTED` and
TJO has taken the decisions.

**The review of the design of milestone S is landed** (`1207ddc`; codex gpt-6-astra, 38 minutes, 4 points of
the codex week; `docs/reviews/s-design/review.md`): 28 VALID, 3 MINOR, 2 INVALID, verdict NOT READY. The
proofs of reconstruction, of the Howell form and its certificate, and of the search of roots at a prime
CLOSE (the three places the author was least sure of). INVALID: P3.9 (what FLINT's count of real roots does
outside its contract: `X^2`, `X^3`, `X^4` return 2, 3, 4) and 3.11 (a sufficient precision called the
smallest). Further: the reference for real roots raises on `X - 10^400` (float division); the root
interface has conflicting contracts (certificates for `f` or for its squarefree part; a verifier that
cannot certify completeness); two mutants of the author's checks survive. Repairs R1 to R10 are written
out. Two findings reproduced by the orchestrator (the overflow; the counts of FLINT). Codex is at 46% of
its week: 4 points remain to the limit of TJO, enough for the closure check on sol and the review of
`equivalent.txt`, not for a second review on astra.

**The design of milestone S is landed as DRAFT 1** (`79b0e3c` and before; lane s-design, Claude Fable, 62
minutes, 535k tokens; `lanes/s-design/report.md`): `docs/proofs/solvers.md` (33 statements, one open and
not used), `proto/solvers_checks.py` (25 checks, 30 s, 0 failures when run by the orchestrator),
`docs/api-s.md` (interface, decisions S-D1 to S-D15 for TJO, edits proposed for SPEC, PLAN, conventions,
8 work packages). No proof was checked by the orchestrator. NO implementation lane starts before the review
has judged it and TJO has taken the decisions. The design found table 3 of `docs/sources.md` wrong on
Thue's lemma (inequality reversed); read in the source by the orchestrator and corrected. Its seven other
corrections of table 3 (report 5.1) are not yet applied: the review judges them first.

**m1-repair-tools is landed** (`5baf3cb`; `lanes/m1-repair-tools/report.md`): keys of `equivalent.txt` without
line numbers, `--san`, `--make`, `--keys`, `--keep`, `tools/mutate/check_equivalent.py`, the memory checker
with `tools/memcheck/selftest.py`. `equivalent.txt` has 108 entries (72 carried with new reasons, 4 dropped,
36 added); each matches exactly one mutant on master. NOT checked: the truth of the 108 reasons; one model
wrote and judged them. To do after the sweep: a review of the reasons and of the survivors by codex
(`gpt-6-sol`), then tests for the survivors. Mutants on lines `ADF_INV_...` survive a sweep of the release
build by construction: judge them again with `--make "make -s -j2 check INV=1"`.
Still open of adf-4lj: a target `check-all` of the Makefile (suite, three scripts, the two selftests,
`check_equivalent.py`).
Master at 00:58: `make check` 42 programs in the five builds; the three scripts; both selftests;
`check_equivalent.py`; `pytest proto` 35.

**Landed: the sources of milestone S** (`90defed`, lane s-sources, space-bunny-alpha in 16 attempts):
Shoup (rational reconstruction, Theorems 4.8, 4.9), Storjohann's thesis (Hermite and Howell form), Conrad
(Hensel), FLINT 3.0.1 documentation (8 files) and C sources (54 files); table 3 of `docs/sources.md`, 57
quotes checked by `lanes/s-sources/check_quotes.py`; `refs/fetch_sources.sh --check` passes. Not on disk:
Wang 1981, Monagan 2004, Collins and Encarnacion, von zur Gathen and Gerhard, Storjohann and Mulders 1998,
Fiedler and Hofmann, Howell 1986 (TJO may supply copies). The manifest now leaves out
`refs/src/tate-thesis/log/` (orchestrator).

**The closure check is complete (adf-igt closed at 01:17).** All six reviews are judged, NO BLOCKER OPEN in
any, no new finding against the code. `surface` (`1fdd494`): 13 CLOSED, 2 SETTLED BY DECISION; its judge
showed that the reason given in M1-D1 for the cap of `prec` was false; the text is corrected in SPEC, the
driver and its README (`a974d59`), the cap is unchanged. Left from the checks, all in one issue (the review
of `equivalent.txt` and of the survivors): arith R4, R5 and contexts R5 (MINOR; the file is rewritten, its
reasons are not yet judged by another family); weak tests named by the judges: `test_modctx_limits` (range
pin passes without its line), the test of text R3 (passes on the old printer), text R9 (coverage only), the
builders of `test_fball_local.c` now use `set_local`.

**m1-invariants and m1-inv-tests are landed** (`0418288`, `1743426`; adf-xk4, adf-6vy closed). Master at
00:28: `make check` 42 test programs with gcc, clang, `SAN=1`, `INV=1`, `INV=1 SAN=1`; the three scripts.
The release objects are identical to those before m1-invariants. The code is Sonnet's; codex judged it in
the closure of `local` (R3 CLOSED) and judges it in `text` (R7).

**Waits for TJO** (adf-s04): M1-D10 (local values are made by the library; no raw setter that counts) and
M1-D11 (a status for an input outside the contract is a courtesy of the release build; under the flag the
call aborts), both PROPOSED in `docs/SPEC.md` section 15. The tests follow them already. After they are
accepted: one sentence each in `fball.h`, `scaled.h`, `recon.h` and conventions 4.6 (orchestrator; the
judge of `local` asks for it). Also adf-xrt, adf-qs9.

**Landed tonight.** Closure of `dump` (`fef212f`): 3 CLOSED, 1 SETTLED BY DECISION. Closure of `arith`
(`9aa97c9`): R1, R2 settled by M1-D3, R3 and R6 CLOSED, R4 and R5 (both MINOR, stale and false lines of
`equivalent.txt`) OPEN until m1-repair-tools lands. Closure of `contexts` (`0cc92bd`): R5 OPEN for the same
reason, the edit of R2 applied (`7e8f5f5`). Closure of `local` (`be32d8d`): nothing open. No blocker open in any. One check program of each
was rebuilt and rerun by the orchestrator (8036 and 24 cases, 0 failures). Observations: adf-mds.

**space-bunny-alpha tonight.** The provider returns "Provider returned an empty response" and pi ends the
session: 30 attempts in 25 minutes, most of 14 seconds, after two useful ones. It also wrote `report.md`
as a running record, which the runner takes for the end of the lane. Try it again for the next lane with a
brief that forbids `report.md` before the end; if the first three attempts end within a minute, use Sonnet.

The closure checks of `local`, `text`, `surface` wait for m1-invariants and m1-repair-tools; their briefs
are not written (pattern: `lanes/m1-closure-arith/brief.md`; model `gpt-6-sol`). `lanes/m1-closure/COMMON.md`:
codex judges every review. Codex meter 39% at 23:50 (limit of TJO: 50%).

**Order of work from here.** Land the three lanes as they end (rules "How to land a lane" below); the sweep
(script, overnight, alone); tests for its survivors; the other closure checks; sources for milestone S into
`refs/src/` (rational reconstruction, Hermite form, Hensel lifting), then its design by Fable, reviewed by astra; benchmarks on a quiet machine; milestone S.

## Session 2026-09-28 (late), 21:00 to 21:50

**One line.** Lane m1-repair-dump is landed (adf-tp2 closed); Sonnet 5.5 replaces the nonfree pi models; a
comparison of Sonnet at effort low, medium, high is prepared and needs a restarted session; nothing is running.

**Master** (pushed): `make check` 40 test programs with gcc, clang, `SAN=1`; `sh tests/test_driver.sh` (27
cases), `sh tests/test_julia.sh`, `sh tests/test_exports.sh`, `pytest proto` (35) pass (21:45). First commands
as in the section below, with 40 for 39.

**Model tiers (TJO, 21:10).** The nonfree pi models (`deepseek-flash`, `mimo-v2.6-pro`, through any provider)
are replaced by Claude Sonnet 5.5. Wherever a section below names one of them in a command, use Sonnet. The
free pi model `space-bunny-alpha`, codex and Claude opus are used as before.
- A Sonnet lane is a normal Claude subagent of the orchestrator (Agent tool, worktree isolation), NOT pi and
  NOT a headless `claude -p` runner (TJO). Rules of the section of the day: push first, paste
  `lanes/COMMON.md` and the whole brief into the prompt, save the report from the final message.
- Code written by Sonnet is code of the Claude family: its reviewer and its judge in the closure check is
  codex, not Claude opus. The table of judges in `lanes/m1-closure/COMMON.md` is right for the repairs already
  landed (pi models wrote them; the report of m1-repair-dump was finished by Claude Fable, its code is pi's).
- Quota at 21:00: Claude weekly 83%, 4.5 points behind pace; Fable weekly 79%, 8.5 behind (both reset
  2026-09-29 18:00); codex weekly 36%.

**The comparison of effort levels (TJO, 21:30: "sonnet low or medium is better as it is faster and cheaper,
try out a couple of levels and compare"). NOT RUN.** The Agent tool takes the effort from the agent's
definition, and definitions are read when a session starts: `.claude/agents/sonnet-low.md`,
`sonnet-medium.md`, `sonnet-high.md` were written in this session and the harness did not know them. In the
next session: check that the three agent types are offered; launch all three in one message, each with the
text of `~/Projects/adelefeld-wt/effort-cmp/PROMPT.md` (LEVEL replaced) and its directory
`~/Projects/adelefeld-wt/effort-cmp/<level>/` (a copy of the lane's tree before the orchestrator's test, with
a finished build); compare verdicts, tests, wall time and tokens. The task is the three survivors of the
mutation run of `src/dump.c`; the expected answers are in the comment at the head of `PROMPT.md` and in
`lanes/m1-repair-dump/report.md` section 6, which the agents must not be shown. Then choose the level and
write it into the memory `orchestration-model-tiers`.

**m1-repair-dump as landed** (`lanes/m1-repair-dump/report.md`; sections 4 to 9 by the orchestrator):
- The four timeouts of the mutation run are killed mutants, not loops without end: each fails checks at once,
  and the time is the building of contexts of 65537 blocks that the mutant lets through.
- Survivors: `:938` and `:883` killed by the new test `grammar_before_limits_in_sball_and_rfun` of
  `tests/test_dump.c` (seen red with each mutant); `:503` is equivalent (line for `equivalent.txt` in the
  report; to be added by m1-repair-tools in its new key format).
- A context of 65536 blocks costs 64.1 CPU seconds to build, 62.3 of them in `fmpz_multi_mod_precompute` and
  `fmpz_multi_CRT_precompute`; the reader of the dump takes 1.3 s. Factor 4 for each doubling of `k`.
- `ADF_DUMP_QCLASS_EXP_MAX` is in `include/adelefeld/dump.h` and conventions 8.4. Only 60 of the 1012
  mutants of `src/dump.c` were run.

**Waits for TJO.** Nothing. TJO ratified on 2026-09-28 (night session): M1-D9 as written (the bound holds for
the form `pieces`, not for `lift`); the wording of M1-D1 and M1-D6; the driver keeps `equal`, `different`,
`undecided` for `compare`, and M1-D1 maps them to the terms of SPEC 4.2. The list "Waits for TJO" of the
section below is settled by this.

**Order of work from here.** The comparison of effort levels (one hour, first thing); m1-invariants alone
(adf-xk4; Sonnet at the level chosen, codex as reviewer); m1-repair-tools alone, overnight (adf-xf4, adf-4lj);
closure check (adf-igt); benchmarks on a quiet machine; milestone S.

## Session 2026-09-28 (evening), 17:25 to 21:05, closed by TJO for a restart

**One line.** Five repair lanes and the review of the dump form are merged; lane m1-repair-dump has its code
done and checked but no finished report and is NOT merged; nothing is running.

**Master** (`origin/master` `c7743e1` plus this commit, pushed): `make check` 39 test programs with gcc, clang,
`SAN=1`; `sh tests/test_driver.sh` (27 cases, also `SAN=1`), `sh tests/test_julia.sh`,
`sh tests/test_exports.sh` pass (20:50).

**First commands.**

    cd ~/Projects/adelefeld && git pull && bd ready | head -20
    make clean && make -j2 check 2>&1 | tail -1          # expect: check passed: all 39 test programs
    free -g; ~/Projects/quota-app/target/release/quota

| Lane | State |
|---|---|
| m1-repair-recon (arith R1, R2) | merged `d378c31`; `recon.h` edited by the orchestrator (`9ea33b6`) |
| m1-repair-adele (arith R3, R4, R6) | merged `df32c71`; adf-c43 closed |
| m1-repair-ctx (contexts R1, R2, R6) | merged `2efa43f`; adf-rki closed |
| m1-review-dump (codex) | merged `9d216fd`; adf-8ju closed; no blocker, 4 MAJOR |
| m1-repair-text (text R2 to R4, R9, R10) | merged `705a998`; adf-nhk, adf-b8l, adf-5qq closed |
| m1-repair-driver (surface R7 to R15) | merged; adf-jvi closed; clang failure of `test_dlopen` repaired by the orchestrator (`c7743e1`) |
| m1-repair-dump (adf-tp2) | merged in the late session (section above); what follows about it below is history |
| m1-repair-tools (adf-xf4, adf-4lj) | paused as before (WIP commit on its branch); run it ALONE, overnight |
| m1-invariants (adf-xk4) | brief on master; runs alone, after m1-repair-dump has landed |
| closure check (adf-igt) | rules and table of judges in `lanes/m1-closure/COMMON.md` |

**To land m1-repair-dump** (the next step; about 30 minutes of the orchestrator, no lane needed).
The pi model ended two sessions without finishing `report.md`: sections 1 to 3 are written (the stage of the
cap; the bound applies to the form `pieces`, not to `lift`; the bytes of a field token), section 4 is an empty
heading, and the first paragraph promises checks the file does not contain. The checks were run and their
logs are in `lanes/m1-repair-dump/` of the worktree:
- `check-gcc.log`, `check-san.log`, `check-clang.log`: 39 test programs each. The orchestrator ran
  `make clean && make -j2 check` itself at 20:18: 39 pass.
- `fuzz.log`, `fuzz2.log`: 748916 and 822202 runs in 121 s, no crash. `valgrind-seed2.log`: 0 errors.
- `mutate.log`: 60 mutants, 47 killed, 3 survived (`src/dump.c:938:30`, `:503:30`, `:883:28`), 6 not
  compiled, 4 timed out at 120 s (`:1026:20`, `:656:12`, `:1008:13`, `:358:16`). None is judged yet. A
  mutant that times out may be a loop without end on hostile text: look at each.
- Run by the orchestrator against the repaired library: the reviewer's `status_findings.py` (R4: both
  inputs `UNSUPPORTED`; R2: `LIMIT` before `DOMAIN`, as M1-D9 says; the script still expects the old
  contract there) and `cost.py 65537` (`UNSUPPORTED` in 0.013 s; it was `OK` after 65 CPU seconds).
Still to do: write the rest of the report from the logs (as the orchestrator, and say so in it); item 5 of
the brief (where the 60 CPU seconds of a valid 1 MB dump go) has no recorded numbers; judge the 3 survivors
and 4 timeouts; put `ADF_DUMP_QCLASS_EXP_MAX` into `include/adelefeld/dump.h` and conventions 8.4 and remove
the `#ifndef` in `src/dump.c`; delete `lanes/m1-repair-dump/checks/red/libadelefeld.a` from the branch;
`make check SAN=1` and plain in the worktree; merge; all checks on master (also `CC=clang`, and the three
scripts); close adf-tp2.

**Waits for TJO.**
- M1-D9 (`docs/SPEC.md` section 15), PROPOSED: the bound `2^20` on the binary exponents of a `qclass` piece
  in the dump form is kept, named, documented, applied at stage 4. The lane applies it to the form `pieces`
  only (a `lift` forms no range; reference vectors hold lifts above the bound): the row should say so.
- The wording of M1-D1 and M1-D6 was changed by the orchestrator to agree with `text.h` (the exponent is the
  one FLINT stores, so `2^99999` is the largest power of two printed; a zero is exempt; `prec` of the driver
  is 1 to `ADF_PRINT_EXP_MAX`). The reviewer `surface` read the bound the other way.
- The driver prints `equal`, `different`, `undecided` for `compare`; SPEC 4.2 says "certainly equal".

**Left open by the landed lanes** (none blocks; adf-xf4, adf-xrt, adf-4lj):
- Survivors of mutation: 10 in `src/recon.c`, 13 in `src/adele.c` (reasons in the lane reports). Only 40
  mutants per file were run for `src/scaled.c`, `src/modctx.c`. The run of `src/text.c` (911 mutants, limit
  300) was killed by the tool's timeout of 40 minutes and printed no survivors: `mutate.py` should print a
  survivor when it finds it. The tool cannot mutate `tools/adf/adf.c`.
- `tools/mutate/equivalent.txt` lines 52 to 65 (`src/adele.c`): stale lines, and the reason "same value" is
  false for `arb_mul`, `acb_mul`. Also stale for `src/scaled.c` (447, 547 are now 508, 540).
- `tests/ref/vectors/m1-adele/set_rat.jsonl`: one record edited by hand (M1-D4); the generator
  `lanes/m1-adele/gen_adele_vectors.py` still writes the old answer. Do not regenerate before it is changed.
- `tests/test_modctx_limits.c` runs its three slow cases (about 5 minutes) only with
  `ADF_MODCTX_LIMITS_FULL=1` (`lanes/m1-repair-ctx/run_limits.sh`).
- `make check` runs neither the driver tests nor `test_exports.sh` (adf-4lj). A lane brief must ask for the
  clang build: the driver lane's did not, and the failure reached master's checks.
- Sources pending: the code of `arb_get_interval_fmpz_2exp`; the documentation of `n_root` and of the
  `sign` argument of `fmpz_multi_CRT_precomp`.
- HEADER-FINDING of m1-repair-ctx: for `k` above the cap `adf_modctx_new_blocks` returns `UNSUPPORTED` before
  the per-element `DOMAIN` checks; the header's "DOMAIN first" needs a clause.

**About the lanes on pi.** A runner counts a lane as done when `report.md` exists, whatever it holds: read
the end of the report before believing `DONE`. `space-bunny-alpha` twice returned after 18 to 90 seconds
on its first attempt and worked on the second. The time limit of a lane is 90 minutes (`LANE_TIMEOUT`).

**The clock.** The WIP commits of the afternoon pause carry the time 20:20 and the system clock read 17:27
when this session began (synchronised). Times of the afternoon in logs and in the section below are about
three hours ahead.

**Order of work from here.** Land m1-repair-dump; m1-invariants alone; m1-repair-tools alone (overnight);
closure check; benchmarks on a quiet machine; milestone S. The rules of the section below still bind.

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
