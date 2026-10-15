# Lane ux-bench-r1: result (the repair of the Bench slice after its test drive)

Five repairs of `lanes/ux-bench-r1/brief.md`, smallest first. Every new test was written first and seen red.
All changes are under `ux/bench/` and `lanes/ux-bench-r1/`. No git command that changes state was run, and no
bd command. The driver was built in the worktree with `timeout 600 make -j2 && timeout 600 make -C tools/adf`
(exit 0).

## Red and green runs

Red: `cd ux/bench && timeout 120 env PYTHONDONTWRITEBYTECODE=1 python3 -m unittest test_bench`, saved as
`lanes/ux-bench-r1/red.txt`. Result: 44 tests, 14 of them new; 6 failures and 8 errors, all in the new tests.
The 30 old tests passed. A test helper (`well_text`) lacked `re.S` and missed wells whose tooltip has newlines.
I fixed it before writing any product code; it then failed for the right reason (no `O()` in the well).
Two tests were added later: `test_refuted_covers_nothing_below_the_stop_line` and
`test_smallest_printed_precision`. They were not run red against the old code. They were shown to fail against
mutants 12, 7 and 8 below.

Green: `timeout 120 env PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover -s ux/bench -p 'test_*.py'`,
saved as `green.txt`. Result: 46 tests OK in 5.1 s. Of that time, 4 s is the D3 test's 4 s timeout.

1. **Item 1: the symbol commands.** The driver has them at `tools/adf/README.md:655-672`: `legendre X with P`
   (line 657), `jacobi X with B` (658), `kronecker X with B` (659) and `hilbert_at X with Y with PLACE` (668).
   - New operations in `sweep.py`: `legendre`, `jacobi` and `kronecker` (column input X, row prime as P or B;
     no cell at R, the reason cites the README), and `hilbert b` (`hilbert_at X with b with PLACE` at every
     row, R included). A line of -1, 0 or 1 is a value well with the symbol.
   - Red: `ValueError: unknown operation 'legendre'` (3 tests). Also `not a line of a command at a place: '1'`,
     a FileNotFoundError for `symbols.json` and for `hilbert.json`, and the README test found no citation.
     Green: SymbolTest, 7 tests OK.
   - Fixture `experiments/symbols.json`: columns 2, -1, 3, 5; rows the 25 primes below 100; `legendre`;
     100 cells. Counts: 43 cells of 1, 51 of -1, 2 of 0, and 4 DOMAIN.
   - `test_symbols_wall_agrees_with_squares_wall` compares the 100 cells with `squares.records.jsonl` place by
     place. At the 96 odd-prime cells: 1 wherever squares has two roots; -1 wherever squares says DOMAIN and p
     does not divide n; 0 at 3 for 3 and at 5 for 5, where squares says DOMAIN (odd valuation).
   - At 2: `legendre n with 2` is DOMAIN, because P must be an odd prime (line 657). The squares wall also says
     DOMAIN for 2, -1, 3 and 5 at 2, for another reason: 2 has odd valuation, and -1, 3, 5 are not 1 mod 8.
     So the walls agree in status at 2, but not in meaning; the test asserts exactly this.
   - Fixture `experiments/hilbert.json`: `(a, -1)_v` for a = -1, 2, 3, 5, 6, 7, -3 at R and the primes below
     50; 112 cells, 102 of 1 and 10 of -1. `test_hilbert_product_formula` checks that each column's product is
     1, and that the rows hold every prime dividing 2ab.
   - Wording: the README now cites the commands and says they are no library demand. The test asserts there is
     no "Tier A" claim, and that the cited lines contain the four command forms. The corrected demand list is
     below. The builder's `lanes/ux-bench/result.md` is not mine, so it is not edited.
2. **D1: the footer names the files read.** `render()` takes `records_path` and `findings_path`. `main()`
   passes the paths it read: relative to the repository root, or absolute outside it. Without a findings file
   the footer says "There is no findings file".
   - Red: `TypeError: render() got an unexpected keyword argument 'records_path'`; the drive's
     `brief_a.json` with `--out <tmp>` named `ux/bench/out/brief_a.records.jsonl`, which does not exist.
   - Green: ProvenanceTest, 2 tests. Both named paths exist: `<tmp>/brief_a.records.jsonl`, and
     `lanes/ux-drive-bench/brief_a.findings.json`.
   - `run.sh --check` renders into a temporary directory, so its pages name that directory. Before the diff,
     `run.sh` replaces `<code><tmpdir>/` by `<code>ux/bench/out/` in those pages. The comment in `run.sh` says
     so.
3. **Item 3: requested and printed precision.** A prime row's label is now `requested prec N`, and so is the
   tooltip. A count well whose output prints `O(p^k)` with k below the requested N shows `O(p^k)` next to the
   count, outside the tooltip. It uses the smallest k of the output's terms, read off the output line (D-UX5 as
   amended; the README says so). The legend explains the label.
   - Red: the well text was `['2']`, not `['2', 'O(5^6)']`, and 0 row labels said `requested prec 8`.
   - Green: PrecisionTest, 3 tests. The drive's `finite_ball.json` through the driver gives `2 O(5^6)` at 5
     and 5 rows labelled `requested prec 8`. `O(7^8)` at the requested 8 shows no label. A mixed output
     `O(5^6); O(5^8)` shows `O(5^6)`.
4. **D2: a contradicted finding is drawn refuted.** The state drawn is `refuted` whenever the rule check finds
   exceptions, whatever the file says. The file is not touched.
   - The card has the counterexample line, the struck-out `refuted` badge, and the line "State in the findings
     file: written as pattern, refuted by the records at ...". Below the stop line it says "nothing: the
     records refute it", and no argument band is drawn for it. Its hover lights the witness wells.
   - Every card carries `data-state`.
   - Red: the card had `badge st-pattern` and "pattern: seen in the swept rows, not proved".
   - Green: RefutedTest, 3 tests. The drive's `attack.json` with `attack.findings.json` through the driver
     draws `st-refuted`, has no `st-pattern`, and says "written as pattern, refuted by the records at 3, 5, 11,
     13, 19, 29, 37, 43." It lights `w-3-1 ... w-43-1` (8 witnesses). The findings file is byte-identical
     afterwards and still says `pattern`.
5. **D3: process time.** `run_driver` measures the lifetime from before `Popen` to the return of `wait()`,
   after the exit or the kill. `run.json` now has `process_time_s` (the lifetime), `last_answer_s` (the time
   of the last output line, the old value) and a `time_note`. `checks_time_s` became `checks_process_time_s`,
   the lifetime of the check process. `run.json` is gitignored, so no committed file changes.
   - The sweep's printed line now names both times. The README says which is which.
   - Red: `KeyError: 'last_answer_s'`.
   - Green: LifetimeTest. It uses the drive's `timeout_real.json` (4 s timeout) and a fake driver that answers
     2 lines and hangs. Result: `process_time_s` is at least 4.0 and below 5.0; `last_answer_s` is below 1.0.
   - The real recipe, `timeout 15 python3 ux/bench/sweep.py lanes/ux-drive-bench/timeout_real.json --out
     <scratch>`, gave `process_time_s` 4.004347 (the drive saw 0.004311), `last_answer_s` 0.005821, exit -9,
     2 answered, a wall time of 4.09 s, and 0 `adf` processes left.

## Other checks

- Fixtures: `timeout 300 sh ux/bench/run.sh` regenerated all five experiments. Then a clean shell ran
  `timeout 300 env -i PATH="$PATH" PYTHONDONTWRITEBYTECODE=1 bash --noprofile --norc -c 'timeout 300 sh
  ux/bench/run.sh --check'`: exit 0, 10 of 10 files identical (`check.txt`).
- Mutants: `timeout 300 python3 lanes/ux-bench-r1/mutants.py` ran 16 hand mutants of the repaired lines, one
  at a time in place, with the files restored and their hashes checked. All 16 were killed (`mutants.txt`).
  The first run had 14 of 16: mutants 7 and 8 (largest instead of smallest k) survived until
  `test_smallest_printed_precision` was added.
- The drive's DOM checker, `timeout 25 node lanes/ux-drive-bench/check_dom.js` on the 5 pages: exit 0. It
  matched 420 wells to their records and ran 16 regroup checks.
- No line over 116 characters in the changed files. `ux/bench/README.md` has 79 lines (D-UX6 asks for under
  80). No `__pycache__` was left.

## Fixtures: diff summary

- `squares`, `local_global`, `exp_edge`: the records files are byte-identical. In the pages, 134, 81 and 51
  lines changed, and 2 lines were added to each.
- A script classified every changed line. Each one is either `prec` becoming `requested prec` (tooltip and
  row label), the new `data-state` attribute, or `<code>` around the footer paths. The 2 added lines are the
  `.got` CSS rule and its legend entry. The script found 0 changed lines outside these kinds.
- No old well gained an `O()` label: every old output prints at the requested precision.
- New: `experiments/symbols.json` and `experiments/hilbert.json`, each with its `out/*.records.jsonl` and
  `out/*.html`. There are no findings files for them, and the footer says so.

## Library demands (D-UX3, against brief 4), corrected

1. Batch records: partly met. The failing place, the value form, the result precision as data, dumps and the
   time per call are not given. The slice reads `O(p^k)` off the text.
2. Thread safety: not exercised.
3. Cost estimates: unmet. `cost ...` gives PARSE.
4. Tier A catalogue: **met by the driver** (`tools/adf/README.md:655-672`), and now used by the slice
   (`legendre`, `jacobi`, `kronecker`, `hilbert b`). The builder's "unmet" was wrong.
5. Places as data: unmet. `places ...` gives PARSE.
6. Statuses are met. Reasons are unmet: DOMAIN for 2 at 2 and for -1 at 3 are the same word.

## Decisions taken without the brief

- A refuted finding draws no argument band below the stop line.
- The witness line "Counterexample: the records contradict this finding at ..." is kept next to the new state
  line, so the places appear twice on such a card. The old test of that wording is unchanged.
- `kronecker` and `jacobi` at R have no cell. `kronecker X with -1` would be a sign, not a symbol at the real
  place, so it is not used.

## What is left (the next slice; items 6 to 8 of the drive)

- 6: an experiment preview with separate counts of wells, checks and total, and an explicit unknown cost.
- 7: the finite-ball lesson: information at 5 only, from `project` at named places, with a disclosed
  derivation.
- 8: the family fold: 7 survivors and 146 first failures for the 153 squarefree pairs.
- Also: a findings file and rules for the new symbol walls, which needs a `{value}` condition in a rule.

## Files

`ux/bench/sweep.py`, `ux/bench/render.py`, `ux/bench/run.sh`, `ux/bench/test_bench.py`, `ux/bench/README.md`,
`ux/bench/experiments/{symbols,hilbert}.json`, `ux/bench/out/{symbols,hilbert}.{records.jsonl,html}`,
`ux/bench/out/{squares,local_global,exp_edge}.html` (regenerated). Lane files: `lanes/ux-bench-r1/result.md`,
`red.txt`, `green.txt`, `check.txt`, `mutants.py`, `mutants.txt`.

No sources pending. No findings against docs/SPEC.md.
