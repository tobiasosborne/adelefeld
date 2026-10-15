# Bench test-drive report

Verdict: the mathematical walls reproduce. Three display or metadata defects need repair.
The full brief A and B computations work through experiment files. The team workflow is still manual.

## Work and counts

Followed lanes/UX-DRIVE.md in order: setup, A, B, three further promises, builder comparison, report.
Read lanes/ux-bench/result.md only at step 5, at 2026-10-16T00:47:02+11:00.
No slice source was changed. No git state command, tracker, installation or subagent was used.

- 6 procedural steps; 34 logged command checks, plus the build and 2 CLI-help probes.
- 48,227 recorded outputs replayed byte-for-byte through build/adf; 0 mismatches.
- 48,176 independent existence/count comparisons; 0 mismatches.
- 136,632 finite root centres checked by congruence; 958 real intervals checked by exact sign changes.
- 1,821 answered well tooltips checked; 28 regroup settings checked in Node.
- 30 builder tests passed; 1 selected baseline test passed and its 1 targeted mutant was rejected.
- 6/6 fixture files reproduced twice, including once with an empty initial environment.
- 3 defects, 5 usability findings, 3 honesty findings. Honesty findings refer to the same 3 defects.
- 0 findings against docs/SPEC.md; 0 pending mathematical sources.
- 0 screenshots obtained. Firefox exited by signal 11 before making the requested image.

Counts include repeated places in different experiments. They are not counts of distinct mathematical questions.
The six supplied fixture checks are three record files and three HTML files.
The complete command log, timestamps, expectations and outputs are in drive.md and checks.jsonl.
The appendix below gives every logged check's command, exit code, duration and output file.

## Results of the scenarios

A: the supplied 104 wells reproduce. The brief's 112-well grid also runs.
At 2 all seven inputs give DOMAIN. For -1 the six primes are 5, 13, 17, 29, 37, 41.
For 2 the six primes are 7, 17, 23, 31, 41, 47.
The extension has 168 rows through 997: 80 OK and 88 DOMAIN. The next prime, 1009, is the stop line.
The page checks the residue rule on the swept records; I checked it independently by square residues.
Sources for the odd-prime theorem now exist, contrary to the brief's old "source not on disk" wording.

B: the supplied 64 wells and 3 rational-root checks reproduce.
The extended product and factor wall has 1216 wells at R and 303 primes through 1999, plus the 3 checks.
The product has a root at all 304 places. The next prime is 2003.
The three rational-root checks give DOMAIN. An independent denominator argument excludes rational roots;
nonexact isolating balls alone would not establish that conclusion. The argument is written in drive.md.

I generated the 153 pairs of distinct squarefree integers in [2,30] using the documented experiment format.
All 46,512 product cells replay and agree with the independent factor oracle.
There are exactly seven survivors: (2,17), (5,29), (10,26), (13,17), (13,29), (17,19), (17,26).
The other 146 pairs have a first failing place, saved in family_summary.json.
The family summary is my computation, not a facility of the slice. I did not render its 153-column wall.

The three extra promises tested were the finite-ball footprint, the budget stop line and attack of a finding.
For (* ; 9 mod 15625), the runner gives two roots at 5, NOT_DETERMINED at 2,3,7,11, and no real command.
The roots 3 and 15622 satisfy x*x=9 modulo 15625. Their actual precision is O(5^6).
The exp fixture gives 40 answers and omits 15 cells at 19,23,29. No omitted cell is drawn.
The deliberately false finding gets eight correct counterexamples, but retains a contradictory pattern badge.

## DEFECTS

D1. Custom output has false provenance paths.

Command, from the repository root:

```sh
timeout 20 python3 ux/bench/render.py lanes/ux-drive-bench/brief_a.json \
  --out lanes/ux-drive-bench/out
```

Output: `brief_a: 112 wells, 4 findings -> lanes/ux-drive-bench/out/brief_a.html`.
Its footer names ux/bench/out/brief_a.records.jsonl and ux/bench/experiments/brief_a.findings.json.
Both paths are nonexistent. Expected paths are lanes/ux-drive-bench/out/brief_a.records.jsonl and
lanes/ux-drive-bench/brief_a.findings.json, the files actually used by the command.
The driver records themselves are right. check_findings.py confirms both absent targets.
This affects the supported --out option and a custom experiment directory.

D2. A contradicted rule is still labelled as an observed pattern.

```sh
timeout 20 python3 ux/bench/render.py lanes/ux-drive-bench/attack.json \
  --out lanes/ux-drive-bench/out
```

The finding asks whether 2 has two roots at every odd prime.
The page says: `Warning: the records contradict this finding at 3, 5, 11, 13, 19, 29, 37, 43.`
It also says: `pattern: seen in the swept rows, not proved`.
Expected: a refuted or explicitly conflicting state, not a badge asserting that this pattern was seen.
For the smallest counterexample, squares modulo 3 are 0 and 1; 2 is absent.
The direct line `roots_at 2 with 3 with 2` gives `error: DOMAIN`, retained in attack.records.jsonl.
The warning is correct; the badge is not. The first-slice manual authorship does not resolve this contradiction.

D3. A timeout's process duration omits the time spent waiting for the timed-out call.

```sh
timeout 15 env PYTHONDONTWRITEBYTECODE=1 python3 ux/bench/sweep.py \
  lanes/ux-drive-bench/timeout_real.json --out lanes/ux-drive-bench/out
```

Measured command duration: 4.069 seconds, including Python startup and cleanup.
The runner has a 4-second driver timeout; R and 3 finish, and the work at 7 is killed.
Output says `2 calls answered in 0.004 s`; timeout_real.run.json has `timed_out: true`, exit -9,
and `process_time_s: 0.004311`. That value is exactly the sum of the two recorded line intervals.
Expected process_time_s includes the four-second driver lifetime, not only time until its last answer.
The README promises "the process time". The same value could be retained under a last-answer-time name.
The row cut is correct, the gap at 2 is disclosed, and 0 build/adf processes remain.

## USABILITY

In decreasing cost to someone attempting the full scenarios:

1. Family setup and summary are manual. The brief promises "fold (a wall to a column summary)" and
   "The human asks the question in one line and leaves." I wrote JSON for 153 inputs and a separate summary
   script to find the seven survivors. Ask, fold, skeptic, keep and ledger export are absent.
   The README and builder disclose the absence of agents; this is a scope gap, not a first-slice regression.
2. Cost before work is incomplete. The brief promises "number of calls and an estimate of time and memory"
   and human acceptance above a budget. The runner prints a count and cap, then starts immediately.
   Time and memory estimates, a preview/accept step and pause controls are absent.
   The 64-cell B preview also has 3 extra check calls outside that count; total work needs a separate total.
3. Requested precision can be read as result precision. The brief says a well can show `O(5^6)`.
   The finite-ball page labels its row `prec 8`, while the returned roots have only O(5^6).
   The tooltip preserves the true precision, but the visible row does not label 8 as requested precision.
4. The finite-ball lesson is missing. The brief promises the header "information at 5 only".
   The actual header is "footprint not given (the driver gives no places of a value)".
   This is an honest omission. D-UX5 now allows disclosed finite derivations from retained driver output,
   and project at named places exposes O(2^0), O(3^0), O(5^6), O(7^0) for this input.
5. Arithmetic-symbol sweeps are absent despite available driver commands. The brief calls them the
   "cheapest and most natural wall cells". The runner's supported operation list omits all four.
   Direct driver calls work; treating this as missing C work misdirects the next implementation step.

The README is sufficient for its three small fixtures. Matching the larger brief requires hand-edited JSON.
I discovered --out through CLI help because the lane cannot overwrite ux/bench/out.

## HONESTY

H1, D1: the custom-output footer names data files it did not use. Provenance is false after relocation.
H2, D2: the pattern badge claims an observed state that the checked records contradict.
H3, D3: process_time_s gives a partial interval as the process duration, hiding the timeout's cost.

No mathematical result was fabricated, no returned library status was hidden, and no unswept well was filled.
The 11 status names match the class table in the builder's passing test.
The driver's OK/none result remains OK with a count of 0; it is not relabelled DOMAIN.
"Footprint not given" is an explicit omission, not a false numerical value.
The no-command real cell has no made-up status or result; its tooltip states the unavailable operation route.
Requested precision and actual printed O() are both retained, but need clearer visible wording.
The actual supplied source citations support F3/F4, and the argument band is separate from measured wells.

## Builder comparison and library demands

The builder's 30-test suite, six fixture comparisons and one inclusive-count mutant check reproduce.
A range beyond the source file is refused: Shoup has 25298 lines, not 99999999.
The timeout behavior reproduces, except for the process-time defect described above.
Driver SHA256 in a regenerated run record matches the actual executable; 112 cell timings accompany 112 answers.
The report's claims about root counts, factor carriers, rule checks and regrouping match the records and DOM.
Its screenshot claims could not be repeated here; no claim about pixel layout is made in this report.

1. Batch records: partly met as described. The driver returns value/status text, without a same-response
   dump, structured result precision, timing, failing-place field or reason. The slice constructs the records.
   The place is known from each submitted single-place line; a separate failing-place field is not supplied.
   The missing result fields were checked by direct exp, roots and project commands in demands.cmd.
2. Thread safety: not exercised. This serial driver test does not establish or refute C thread safety.
3. Cost query: not exposed by the documented driver. The trial cost command gives PARSE.
   The slice's limits are time and whole-row counts; its timeout metadata needs repair.
4. Tier A catalogue: the builder's "unmet" claim is false as a library-demand statement.
   legendre 2 with 7 gives 1; jacobi 2 with 15 gives 1; kronecker 3 with 2 gives -1;
   hilbert_at 3 with 3 with 2 gives -1. This is an unimplemented slice route.
5. Places as data: the documented driver has no prime iterator. The trial places command gives PARSE.
   Projection at supplied places works, which differs from enumerating places. No claim about all C APIs is made.
6. Status reasons: not supplied. Roots of 2 at 2 and of -1 at 3 both give only `error: DOMAIN`.
   One is ruled out by odd valuation, the other by the square residues modulo 3.

## VISION

The wall makes the finite evidence legible, and its computed records withstand the full scenarios.
The cost, provenance and evidence-state defects prevent treating every annotation as reliable yet.
The next slice would gain most from a family fold that gives each pair its first failing place or a full column.

## FOR THE REPAIR

Smallest changes first; each item has a concrete acceptance check.

1. Correct the demand list: the four symbol commands above must be marked available in the driver.
2. Make custom-output provenance name the actual records and findings paths; both D1 targets must exist.
3. Label prec as requested and expose returned O() separately; the finite-ball example must show 8 and 6 clearly.
4. Change the computed finding state on contradiction; attack.json must show eight witnesses and a refuted badge.
5. Measure the entire driver lifetime; the 4-second timeout must record a process duration near 4 seconds.
6. Add an experiment preview with separate well/check/total counts, and retain an explicit unknown cost estimate.
7. Explain finite-ball information at named places using retained projection results and a disclosed derivation.
8. Add arithmetic-symbol sweep routes and a family fold; the family must yield 7 survivors and 146 first failures.

## Sources and specification

Read and used: refs/src/shoup-ntb/ntb-v2.txt:15014, :15027-15034;
refs/src/conrad-hensel/hensel.txt:160-170 and :360-370.
These support multiplicativity, the two residue rules and the odd-prime and 2-adic lifting criteria.
Own finite computations, factor expansion and rational-root exclusion are written stepwise in drive.md.
No source pending. No finding against docs/SPEC.md. The brief's missing-source remark is stale, not a SPEC defect.

## Files written and limits

All authored drive artifacts are under lanes/ux-drive-bench; files.txt inventories them.
Main files: drive.md, report.md, checks.jsonl, family_summary.json and per-check .txt outputs.
The scripts are run_check.py, verify_records.py, check_family.py, check_dom.js, check_extra.py,
check_builder.py and check_findings.py. render_count_min_mutant.py is an uninstalled mutant copy.
The experiment JSON, findings JSON, out/ records/pages/run metadata, and direct replay .cmd/.out files are retained.
The setup log is setup.txt. The Firefox profile is retained; no screenshot exists.

Prescribed make commands created build products outside the lane. One incidental Python cache also appeared:
ux/bench/__pycache__/sweep.cpython-312.pyc, 27237 bytes. It was generated by the early CLI execution.
That cache violates the strict lane-only artifact rule; I did not delete or alter it outside my ownership.
Later runs disable bytecode writing. The orchestrator can remove it. No ux source file was changed.
The initial discovery/reading commands lacked timeout; no test or computation ran in those commands.
All test programs and later commands were bounded; the build had a 180-second enclosing timeout and 2 jobs.

Not done: visual layout verification, all 12 historical mutants, C thread tests, a precision-axis sweep,
automated agents, family rendering/folding by the product, ledger export, fixes to ux code, or a browser interaction
check beyond Node's DOM adapter. The adapter tests actual inline JavaScript but cannot establish browser layout.
One adapter failure was corrected from record class open to the HTML's CSS class nd; it was not a product fix.
