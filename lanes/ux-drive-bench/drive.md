# Bench test drive

Started: 2026-10-16T00:37:46+11:00 (from `timeout 10 date -Is`).
Lane: ux-drive-bench. Only this lane is edited; prescribed build products are the setup exception.
The builder's report is unread until step 5.

## Preparation

Read CLAUDE.md first, then lanes/UX-DRIVE.md, lanes/COMMON.md, lanes/UX-COMMON.md,
docs/ux/brief-4-sweep-bench.md and ux/bench/README.md.
Read SPEC 9.1-9.3.3 and 10, and PLAN milestone 1F and S.
No AGENTS.md found in this repository or checked ancestor directories.
The first file-discovery and reading commands lacked timeout. No test or computation ran in those commands.
All subsequent shell commands use timeout. No git state command, tracker or installation is used.

## Step 1: setup from the README

2026-10-16T00:37:46+11:00.
Command: `timeout 10 ls -ld lanes/ux-drive-bench build/adf`.
Expected: a lane directory and a driver, or an explicit build prerequisite.
Result: lane exists; build/adf is missing (exit 2). Build is required.

The README's normal `sh ux/bench/run.sh` writes ux/bench/out, outside this lane's ownership.
That command will not be run in normal mode. Its documented --check mode can regenerate temporarily.
New experiments, if supported, will write within this lane. This is a lane constraint, not a product defect.

Setup build command:

```sh
timeout 180 bash --noprofile --norc -c \
  'timeout 600 make -j2 && timeout 600 make -C tools/adf' > lanes/ux-drive-bench/setup.txt 2>&1
```

Result: exit 0. Library and driver built with at most 2 jobs; enclosing timeout was 180 seconds.
`timeout 10 python3 ux/bench/sweep.py --help` and the same command for render.py both exit 0.
Both support --out, which the README does not mention. This permits lane-local output without a code fix.
The README has four columns below 100, whereas brief A asks for seven columns through 47 then 2 below 1000.
I will run the supplied example first, then express those two grids using the documented experiment format.

### setup_check: 2026-10-16T00:39:05+11:00

Expected: All three fixtures reproduce without differences.

Command:

```sh
timeout 120 sh ux/bench/run.sh --check
```

Exit 0; 0.601 seconds. Output: `setup_check.txt`.

### squares_firefox: 2026-10-16T00:39:16+11:00

Expected: A screenshot of the supplied squares page.

Command:

```sh
timeout 30 bash --noprofile --norc -c 'firefox --headless --profile \
  "$PWD/lanes/ux-drive-bench/firefox-profile" --screenshot "$PWD/lanes/ux-drive-bench/squares.png" \
  "file://$PWD/ux/bench/out/squares.html"'
```

Exit -11; 0.362 seconds. Output: `squares_firefox.txt`.

### a_supplied_sweep: 2026-10-16T00:39:53+11:00

Expected: 104 wells in a lane-local copy.

Command:

```sh
timeout 60 python3 ux/bench/sweep.py ux/bench/experiments/squares.json --out lanes/ux-drive-bench/out
```

Exit 0; 0.098 seconds. Output: `a_supplied_sweep.txt`.

### a_brief_sweep: 2026-10-16T00:39:53+11:00

Expected: 112 calls: seven integers at R and fifteen primes.

Command:

```sh
timeout 60 python3 ux/bench/sweep.py lanes/ux-drive-bench/brief_a.json --out lanes/ux-drive-bench/out
```

Exit 0; 0.107 seconds. Output: `a_brief_sweep.txt`.

### a_brief_render: 2026-10-16T00:39:53+11:00

Expected: A seven-column wall and findings.

Command:

```sh
timeout 20 python3 ux/bench/render.py lanes/ux-drive-bench/brief_a.json --out lanes/ux-drive-bench/out
```

Exit 0; 0.118 seconds. Output: `a_brief_render.txt`.

### a_extend_sweep: 2026-10-16T00:39:53+11:00

Expected: 168 prime rows ending at 997.

Command:

```sh
timeout 60 python3 ux/bench/sweep.py lanes/ux-drive-bench/brief_a_1000.json --out lanes/ux-drive-bench/out
```

Exit 0; 0.09 seconds. Output: `a_extend_sweep.txt`.

### a_extend_render: 2026-10-16T00:39:53+11:00

Expected: 168 wells and the first unswept prime 1009.

Command:

```sh
timeout 20 python3 ux/bench/render.py lanes/ux-drive-bench/brief_a_1000.json --out lanes/ux-drive-bench/out
```

Exit 0; 0.067 seconds. Output: `a_extend_render.txt`.

## Step 2: Scenario A

The supplied wall reproduces. The brief's grid and extension run using lane-local JSON files.
No command exists to ask, accept, keep or run a skeptic; editing experiment and findings JSON is the interface.
The runner prints call count and budget then runs immediately, without a separate acceptance step.

Firefox attempt squares_firefox: process terminated by signal 11 before any screenshot was written.
This is a screenshot limitation, not evidence that the HTML fails to render. I will inspect DOM and scripts in Node.
No screenshot appearance is claimed. The profile path was lane-local and fresh.

Read sources used by F3/F4: refs/src/shoup-ntb/ntb-v2.txt:15027-15034 and
refs/src/conrad-hensel/hensel.txt:160-170. They support the two odd-prime claims.
The brief's "source not on disk" is now out of date: both source files are present.

### a_verify: 2026-10-16T00:41:03+11:00

Expected: Every recorded output replays; roots and existence counts satisfy independent arithmetic.

Command:

```sh
timeout 90 python3 lanes/ux-drive-bench/verify_records.py ux/bench/out/squares.records.jsonl \
  lanes/ux-drive-bench/out/brief_a.records.jsonl lanes/ux-drive-bench/out/brief_a_1000.records.jsonl
```

Exit 0; 0.104 seconds. Output: `a_verify.txt`.

### a_dom: 2026-10-16T00:41:53+11:00

Expected: All tooltips reproduce records; regroup and finding hover/focus work.

Command:

```sh
timeout 25 node lanes/ux-drive-bench/check_dom.js ux/bench/out/squares.html \
  lanes/ux-drive-bench/out/brief_a.html lanes/ux-drive-bench/out/brief_a_1000.html
```

Exit 0; 0.064 seconds. Output: `a_dom.txt`.

Scenario A result: 384/384 outputs replay byte-for-byte, 384 existence/count checks pass,
326 finite root centres satisfy their quadratic congruence, 16 real balls bracket a root.
At 2 all seven brief inputs give DOMAIN. For -1 the brief's six primes are 5, 13, 17, 29, 37, 41.
For 2 they are 7, 17, 23, 31, 41, 47. Below 1000 there are 168 primes, 80 OK and 88 DOMAIN.
The last row is 997. The next prime is 1009; 1000 through 1008 contain no prime.

Independent arithmetic: remove p factors from n; an odd valuation rules out a square because v_p(x^2)
is twice v_p(x). For unit n at odd p, enumerate x*x modulo p and use Conrad:160-170 for lifting.
At 2, odd squares are 1 modulo 8; the seven-input 2-row fails this necessary test or has odd valuation.
Positive real inputs have two roots, negative ones none; exact Fraction arithmetic checks opposite
signs of x*x-n at each printed interval's endpoints. Root centres are checked modulo their printed p^N.
The script does not infer existence from a successful driver status.

Node checked 384 tooltip records, 12 regroup settings, three hash paths, and finding hover/focus toggles.
It reads the HTML and runs its inline script against a minimal DOM adapter; it does not test pixel layout.
All referenced wells exist. Stop lines are 101, 53 and 1009 for supplied, brief and extended grids.

A usability limitation: no acceptance, skeptic or keep control is present. Findings are edited JSON.
This is disclosed by the README's no-agents limit and allowed by the first slice, but does not complete
Scenario A's human/team workflow. Source-backed claims and measured patterns have different badges and text.

## Step 3: Scenario B

### b_extend_sweep: 2026-10-16T00:43:05+11:00

Expected: 1216 wells, plus three rational-root checks, through 1999.

Command:

```sh
timeout 130 python3 ux/bench/sweep.py lanes/ux-drive-bench/brief_b_2000.json --out lanes/ux-drive-bench/out
```

Exit 0; 0.183 seconds. Output: `b_extend_sweep.txt`.

### b_extend_render: 2026-10-16T00:43:05+11:00

Expected: Product and factor columns through 1999 with a separate argument.

Command:

```sh
timeout 20 python3 ux/bench/render.py lanes/ux-drive-bench/brief_b_2000.json --out lanes/ux-drive-bench/out
```

Exit 0; 0.175 seconds. Output: `b_extend_render.txt`.

### b_family_sweep: 2026-10-16T00:43:06+11:00

Expected: Every squarefree pair at R and all primes below 2000.

Command:

```sh
timeout 130 python3 ux/bench/sweep.py lanes/ux-drive-bench/brief_b_family.json --out lanes/ux-drive-bench/out
```

Exit 0; 5.008 seconds. Output: `b_family_sweep.txt`.

### b_dom: 2026-10-16T00:43:24+11:00

Expected: All B tooltips, findings, four regroup choices and stop lines agree with records.

Command:

```sh
timeout 25 node lanes/ux-drive-bench/check_dom.js ux/bench/out/local_global.html \
  lanes/ux-drive-bench/out/brief_b_2000.html
```

Exit 0; 0.101 seconds. Output: `b_dom.txt`.

### b_verify: 2026-10-16T00:43:24+11:00

Expected: All supplied and extended records replay; each root and count agrees with independent arithmetic.

Command:

```sh
timeout 90 python3 lanes/ux-drive-bench/verify_records.py ux/bench/out/local_global.records.jsonl \
  lanes/ux-drive-bench/out/brief_b_2000.records.jsonl
```

Exit 0; 0.181 seconds. Output: `b_verify.txt`.

### b_family_verify: 2026-10-16T00:44:18+11:00

Expected: Exactly the seven pairs in brief B survive all 304 places.

Command:

```sh
timeout 100 python3 lanes/ux-drive-bench/check_family.py
```

Exit 0; 1.358 seconds. Output: `b_family_verify.txt`.

Scenario B: the supplied 64 wells and three checks replay; the extended 1216 wells and three checks replay.
All 1280 well counts agree with factor square-existence tests. The factor expansion is
x^6 - 251*x^4 + 6851*x^2 - 48841, since 13+17+221=251,
13*17+13*221+17*221=6851, and 13*17*221=48841.
The local product has a root at all 304 tested places through 1999; the next prime is 2003.
At 2 it has roots 23 and 233 modulo 2^8 (from the factor x^2-17).
At 13 the factor x^2-17 is a square; at 17 the factor x^2-13 is a square.
The supplied and extended real lists each have six balls, each verified by an exact sign change.

The no-rational-root check is sound, independently: a rational x=u/v in lowest terms with x*x=n
has u*u=n*v*v; any prime dividing v would divide u, so v=1. But 9<13<16, 16<17<25,
and 196<221<225. A product zero over Q forces one factor zero. None can be zero for rational x.
An isolating ball's not being an exact point alone would not prove irrationality; the integer argument does.

The unswept-prime argument uses refs/src/shoup-ntb/ntb-v2.txt:15014 (multiplicativity), and
refs/src/conrad-hensel/hensel.txt:160-170 (odd-unit lifting).
For p not dividing 2*13*17, both symbols are nonzero. If either is +1 its factor has roots.
If both are -1 the product symbol is +1, so x^2-221 has roots. Exceptional R, 2, 13 and 17 were checked.
For the independent 2-adic family oracle, read Conrad:360-370: an odd unit is a square exactly when 1 mod 8.

The family JSON enumerates the 18 squarefree integers in [2,30], and their 153 distinct pairs.
No squarefree distinct pair has square product: a square product would have identical prime supports,
forcing equality. Each product polynomial was expanded by integer arithmetic, not copied from a table.
The page offers no family generator or fold control; I had to write this lane-local setup and summary script.
No builder source was read for those scripts.

The family replay and independent count check pass for all 46,512 cells.
There are 146 failing pairs and exactly the seven stated survivors:
(2,17), (5,29), (10,26), (13,17), (13,29), (17,19), (17,26).
134,456 finite root centres satisfy the expanded product congruence; 918 real intervals bracket a root.
The family summary is my own finite computation in family_summary.json; the slice does not display it.

## Step 4: three further promises

1. The surface, "The columns": finite-ball footprint and open statuses away from its modulus.
2. The surface, "The wall" and interaction, "Cost is shown before work": a budget cut and unswept rows.
3. The findings rail and "attack": contradict a hand-written conjecture and inspect the warning and badge.

### extra_ball_sweep: 2026-10-16T00:45:23+11:00

Expected: A real-place omission, two roots at 5, and open results elsewhere.

Command:

```sh
timeout 40 python3 ux/bench/sweep.py lanes/ux-drive-bench/finite_ball.json --out lanes/ux-drive-bench/out
```

Exit 0; 0.058 seconds. Output: `extra_ball_sweep.txt`.

### extra_ball_render: 2026-10-16T00:45:23+11:00

Expected: Footprint explains what the modulus says at each prime.

Command:

```sh
timeout 20 python3 ux/bench/render.py lanes/ux-drive-bench/finite_ball.json --out lanes/ux-drive-bench/out
```

Exit 0; 0.051 seconds. Output: `extra_ball_render.txt`.

### extra_budget_sweep: 2026-10-16T00:45:23+11:00

Expected: 40 answers and 15 unswept cells, first omitted prime 19.

Command:

```sh
timeout 40 python3 ux/bench/sweep.py ux/bench/experiments/exp_edge.json --out lanes/ux-drive-bench/out
```

Exit 0; 0.055 seconds. Output: `extra_budget_sweep.txt`.

### extra_budget_render: 2026-10-16T00:45:23+11:00

Expected: No results are drawn below the stop line.

Command:

```sh
timeout 20 python3 ux/bench/render.py ux/bench/experiments/exp_edge.json --out lanes/ux-drive-bench/out
```

Exit 0; 0.043 seconds. Output: `extra_budget_render.txt`.

### extra_attack_sweep: 2026-10-16T00:45:23+11:00

Expected: 112 cells for the adversarial finding.

Command:

```sh
timeout 40 python3 ux/bench/sweep.py lanes/ux-drive-bench/attack.json --out lanes/ux-drive-bench/out
```

Exit 0; 0.056 seconds. Output: `extra_attack_sweep.txt`.

### extra_attack_render: 2026-10-16T00:45:23+11:00

Expected: The false finding is refuted, with counterexample places.

Command:

```sh
timeout 20 python3 ux/bench/render.py lanes/ux-drive-bench/attack.json --out lanes/ux-drive-bench/out
```

Exit 0; 0.052 seconds. Output: `extra_attack_render.txt`.

### extra_verify: 2026-10-16T00:46:11+11:00

Expected: All new finite-ball and exp outputs replay, preserving statuses.

Command:

```sh
timeout 90 python3 lanes/ux-drive-bench/verify_records.py lanes/ux-drive-bench/out/finite_ball.records.jsonl \
  lanes/ux-drive-bench/out/exp_edge.records.jsonl
```

Exit 0; 0.043 seconds. Output: `extra_verify.txt`.

### extra_dom: 2026-10-16T00:46:12+11:00

Expected: Answered tooltips match; unswept wells are absent; false-rule warnings appear.

Command:

```sh
timeout 25 node lanes/ux-drive-bench/check_dom.js lanes/ux-drive-bench/out/finite_ball.html \
  lanes/ux-drive-bench/out/exp_edge.html lanes/ux-drive-bench/out/attack.html
```

Exit 1; 0.029 seconds. Output: `extra_dom.txt`.

### extra_dom_corrected: 2026-10-16T00:46:35+11:00

Expected: DOM classes map to the status shapes defined by this HTML.

Command:

```sh
timeout 25 node lanes/ux-drive-bench/check_dom.js lanes/ux-drive-bench/out/finite_ball.html \
  lanes/ux-drive-bench/out/exp_edge.html lanes/ux-drive-bench/out/attack.html
```

Exit 0; 0.029 seconds. Output: `extra_dom_corrected.txt`.

### extra_semantics: 2026-10-16T00:46:35+11:00

Expected: Check precise finite-ball, budget and contradictory-badge behavior.

Command:

```sh
timeout 20 python3 lanes/ux-drive-bench/check_extra.py
```

Exit 0; 0.032 seconds. Output: `extra_semantics.txt`.

Step 4 results:

- Finite ball: two roots at 5, four NOT_DETERMINED cells elsewhere, no real command, all honest.
  The header says "footprint not given (the driver gives no places of a value)"; it does not teach
  "information at 5 only" as promised. The displayed row says prec 8, but returned precision O(5^6)
  is available only inside the tooltip. This can be misread as eight known digits.
  The two centres are 3 and 15622=-3 mod 15625. At other primes 15625 is a unit, so the projection
  is Z_p. It includes both squares and nonsquares; the open result is required.
- Budget: 40 results and 15 unswept cells at 19, 23 and 29; no below-budget wells are drawn.
  The page and console show the cap before work but no time/memory estimate and no acceptance control.
- Attack: the false rule reports exactly 3, 5, 11, 13, 19, 29, 37, 43 as counterexamples, but its badge
  remains "pattern: seen in the swept rows, not proved". That badge asserts evidence the rule does not have.
  The warning itself is right. A refuted badge, or an explicit conflicting state, is needed.

extra_dom first failed because my adapter equated record class open with CSS class open.
The HTML actually uses nd (dashed) and lim (dotted); I corrected the adapter's mapping, not product output.
The rerun passed 157 tooltip records and eight regroup settings across the three pages.
The test remains strict about the class, tooltip, count, referenced well and event behavior.
The five finite-ball and 40 exp outputs replay exactly, including digit setting 10 for exp_edge.

## Step 5: builder comparison

The builder report is read for the first time now, after the two scenarios and three extra promises.

### builder_tests: 2026-10-16T00:47:35+11:00

Expected: All 30 builder tests pass.

Command:

```sh
timeout 120 env PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover -s ux/bench -p 'test_*.py'
```

Exit 0; 1.109 seconds. Output: `builder_tests.txt`.

### builder_demands: 2026-10-16T00:48:09+11:00

Expected: Check each claimed driver gap, including the four arithmetic symbols.

Command:

```sh
timeout 20 build/adf lanes/ux-drive-bench/demands.cmd
```

Exit 1; 0.003 seconds. Output: `builder_demands.txt`.

### builder_timeout: 2026-10-16T00:48:09+11:00

Expected: R and 3 finish; timeout omits 7 and 17.

Command:

```sh
timeout 15 env PYTHONDONTWRITEBYTECODE=1 python3 ux/bench/sweep.py lanes/ux-drive-bench/timeout_real.json \
  --out lanes/ux-drive-bench/out
```

Exit 0; 4.069 seconds. Output: `builder_timeout.txt`.

### builder_timeout_render: 2026-10-16T00:48:13+11:00

Expected: Stop at 7 and disclose the skipped prime 2.

Command:

```sh
timeout 20 env PYTHONDONTWRITEBYTECODE=1 python3 ux/bench/render.py lanes/ux-drive-bench/timeout_real.json \
  --out lanes/ux-drive-bench/out
```

Exit 0; 0.041 seconds. Output: `builder_timeout_render.txt`.

Builder tests: 30 tests, 0 failures, 1.039 seconds. Fixture reproduction: 6/6 identical.
A report correction: the Tier A catalogue is available in the driver. Direct calls return
legendre(2,7)=1, jacobi(2,15)=1, kronecker(3,2)=-1, hilbert_at(3,3,2)=-1.
The slice's supported-operation list omits them; this is a slice gap, not a missing library capability.
Direct calls confirm bare DOMAIN with no reason for both 2 at 2 and -1 at 3.
The driver prints p-adic values and O() text, without a structured record, time or dump in the same response.
`project` at named primes prints precisions 0,0,6,0 for the finite ball at 2,3,5,7.
It supplies enough local data to explain its information at a named place. It is not a place iterator.
The attempted `places primes below 10` and `cost roots_at 2 with 7 with 2` both give PARSE;
no iterator or cost-query command is documented. This is evidence about the driver, not every C API.

The step-5 test inspection is limited to reproducing claimed test kinds, not a source review.
One incidental Python bytecode file was generated by the earlier README/help execution:
ux/bench/__pycache__/sweep.cpython-312.pyc (00:38, 27237 bytes). It is outside lane ownership.
It has not been edited or removed; subsequent Python runs disable bytecode writing.
This is an execution side effect outside the lane; the orchestrator should remove it if unwanted.

### builder_mutation: 2026-10-16T00:49:30+11:00

Expected: One original regression passes, its targeted mutant fails; metadata and timeout agree.

Command:

```sh
timeout 20 env PYTHONDONTWRITEBYTECODE=1 python3 lanes/ux-drive-bench/check_builder.py
```

Exit 0; 0.075 seconds. Output: `builder_mutation.txt`.

### builder_source_refusal: 2026-10-16T00:50:22+11:00

Expected: Rendering refuses a source range beyond end of file.

Command:

```sh
timeout 20 env PYTHONDONTWRITEBYTECODE=1 python3 ux/bench/render.py lanes/ux-drive-bench/invalid_source.json \
  --out lanes/ux-drive-bench/out
```

Exit 1; 0.103 seconds. Output: `builder_source_refusal.txt`.

### builder_clean_shell: 2026-10-16T00:50:23+11:00

Expected: Six fixture files reproduce in a shell with an empty initial environment.

Command:

```sh
timeout 120 env -i \
  PATH="$PATH" \
  PYTHONDONTWRITEBYTECODE=1 bash --noprofile --norc -c 'timeout 100 sh ux/bench/run.sh --check'
```

Exit 0; 0.326 seconds. Output: `builder_clean_shell.txt`.

### defects_confirm: 2026-10-16T00:50:23+11:00

Expected: The three reported display and metadata defects reproduce.

Command:

```sh
timeout 20 python3 lanes/ux-drive-bench/check_findings.py
```

Exit 0; 0.026 seconds. Output: `defects_confirm.txt`.

### defects_paths_corrected: 2026-10-16T00:51:15+11:00

Expected: Footer targets are checked without sentence punctuation.

Command:

```sh
timeout 20 python3 lanes/ux-drive-bench/check_findings.py
```

Exit 0; 0.033 seconds. Output: `defects_paths_corrected.txt`.

Additional step-5 findings:

- The mutation check runs the original inclusive-count test once: 1 pass. An in-memory copy changes
  >= to >: 1 assertion failure, 0 errors. The owned copy is render_count_min_mutant.py; ux files are unchanged.
  This confirms one of the builder's 12 mutant kinds, not all 12 historical mutants.
- A source range 99999999-99999999 is refused, exit 1. The actual Shoup file has 25298 lines.
- The empty-environment fixture check passes again: 6/6 files identical.
- The real-driver timeout lasts 4.069 seconds, returns R and 3, omits 7 and 17, and discloses the gap at 2.
  No build/adf process remains. But process_time_s is 0.004311, the sum of the two received line intervals.
  It omits the four seconds spent on the timed-out call, so it is not the driver's process duration.
- A lane-local rendered page gives footer paths ux/bench/out/brief_a.records.jsonl and
  ux/bench/experiments/brief_a.findings.json. Neither exists. Actual data and findings are under this lane.
  This breaks provenance after the supported --out option and a custom experiment path.

Claims about pixel appearance remain unverified because Firefox terminated by signal 11.
DOM checks confirm the contents, shapes declared in CSS, regroup ordering and event wiring, not the screenshot.
The builder's report says the two sourced badges differ from patterns; both CSS and text agree.
The product root counts and listed factor carriers all agree with the independent count checks.

D-UX5 was amended after the build: deriving finite data from driver lines is now allowed with disclosure.
The builder's reason for omitting a finite-ball footprint is therefore no longer an absolute restriction.
The current omission is explicit, not a fabricated answer.

## Step 6: report

2026-10-16T00:55:46+11:00: report.md written. Elapsed from the initial dated setup check: 1080 seconds.
34 logged command checks; 3 defects, 5 usability findings, 3 overlapping honesty findings.
No mathematical mismatch and no finding against SPEC.
