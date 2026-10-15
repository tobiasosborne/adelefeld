# The Bench: a wall of places (first slice of docs/ux/brief-4-sweep-bench.md)

An experiment asks one question at many places. The sweep runner sends one driver call per cell to `build/adf`, row
by row (R first, then the primes increasing), and writes one record per cell. The renderer draws the records as a
static wall, with a stop line and a column of findings written by hand. No server, no network.

## Build and run (from the repository root, in a fresh shell)

    timeout 600 make -j2 && timeout 600 make -C tools/adf     # the library, then the driver build/adf
    sh ux/bench/run.sh                                        # sweep and draw every experiment into ux/bench/out/
    sh ux/bench/run.sh --check                                # redo all in a temporary directory, diff with out/
    firefox ux/bench/out/squares.html                         # or local_global, exp_edge, symbols, hilbert
    timeout 120 python3 -m unittest discover -s ux/bench -p 'test_*.py'

One experiment by hand: `python3 ux/bench/sweep.py ux/bench/experiments/squares.json` (prints the cells and calls,
then the stop line), then `python3 ux/bench/render.py` on the same file. Both take `--out DIR`; the findings are
read next to the experiment file. `squares.html#mod8` opens the wall regrouped by `p mod 8`.

## What the page shows

- **Question line**: the question, then inputs x places = cells, the operation, and the budget.
- **Sweeper line**: `sweeper: N calls, M done, K below the stop line`, counted from the records.
- **The wall**: one row per place (a prime row says `requested prec N`, the N sent to the driver), one column per
  input, its value form in the header. Each cell is a well; its shape is the status class: filled disc with a
  count (OK, a list), outlined ring `0` (OK, the list is empty: `none`), a pill with the value cut short (OK, a
  value or a symbol -1, 0, 1), ring with a slash (proved no: DOMAIN, NO_SOLUTION, NOT_UNIT), dashed ring `?`
  (open: NOT_DETERMINED, UNIT_NOT_CERTIFIED, NEEDS_SPLIT, NOT_UNIQUE), dotted square `L` (budget: LIMIT), `!`
  (not mathematics: PARSE, UNSUPPORTED), a dash (no driver command for this cell). A count followed by `O(5^6)`:
  the output prints a precision below the requested one; the smallest k of its terms `O(p^k)`, read off the
  output line by the renderer. Hover a well for its record: driver line, requested prec, output, status.
- **rows**: the selector regroups the rows by `p mod m` (R and the primes dividing m form the first group).
- **Stop line**: the thick rule after the last place swept. Below it nothing is drawn but one sentence: the next
  prime not swept, how many places were not run and why (max_cells or the timeout), the primes skipped in a gap.
- **Bands below the stop line** (hatched): a finding that covers unswept places, and only by its argument.
- **Checks**: driver lines outside the wall (for example `root 13 with 2`), with their outputs.
- **Findings** (right): id, column, text, the places it was computed at, the renderer's check of its rule on the
  records, sources, argument, and a badge for its state: `pattern` (rounded outline), `sourced` (double square
  border), `refuted` (struck out), `open` (dashed). A finding whose rule the records contradict is drawn
  `refuted` whatever its file says: a counterexample line, the badge, "written as pattern, refuted by the records
  at ...", no band below the stop line; hover lights its witnesses (else the wells it rests on). The footer names
  the records and findings files the page was drawn from.

## The scenarios

- **A: for which primes below 100 is 2 a square?** `squares.html`, column `2`: two roots at the odd primes 1 or 7
  mod 8, DOMAIN elsewhere and at 2; regroup by `p mod 8`. F1, F2 are patterns; F3, F4 are sourced (Shoup and
  Conrad in `refs/src/` by line) and cover the odd primes below the stop line by argument.
- **B: a root at every place, no rational root.** `local_global.html`: the product column has a root at R and at
  every prime below 50; the factor columns show which factor has it; the checks show that 13, 17 and 221 have no
  rational square root. F1 is a pattern with the argument for the places below the stop line.
- `exp_edge.html`: a budget cut (40 cells: the stop line at 19), value wells and NOT_DETERMINED. `symbols.html`:
  the Legendre symbol (n/p) for the columns of the squares wall: 1 where two roots, -1 where DOMAIN, 0 at
  p = n, DOMAIN at 2 (no symbol there). `hilbert.html`: `(a, -1)_v` at R and the primes below 50; each column's
  product is 1. The driver's symbol commands (`tools/adf/README.md:655-672`, no library demand): `legendre X with
  P`, `jacobi X with B`, `kronecker X with B` (the row prime as P or B; no cell at R) and `hilbert_at X with Y
  with PLACE` (operation `hilbert b`: Y = b, PLACE every row).

## Files

- `experiments/<name>.json`: `name`, `question`, `cell` (words), `operation` (`roots n`, `roots`, `realroots`,
  `exp`, `log`, `project`, `powrat e/n seed`, `legendre`, `jacobi`, `kronecker`, `hilbert b`), `inputs`
  (`{"value": "2"}`; `{"poly": "-2 0 1", "label": "x^2 - 2"}` for `roots`, `realroots`, constant first),
  `places` (`"R"`, primes, `"primes below N"`), `precision` (`R` in bits, `p` the p-adic N, or a prime as key),
  `budget` (`max_cells`: whole rows while they fit; `timeout`: seconds for the driver process), optional
  `digits`, `checks` (`{"id", "line"}`) and `regroup` (the moduli of the selector).
- `experiments/<name>.findings.json`: a list of `{id, state, column, text, rule?, evidence?, argument?,
  sources?, checks_shown?}`. A rule is `{places: all|primes|odd primes|[...], when?: {mod, residues} or
  {places}, then, else?, checks?}`, each condition `{status}`, `{count}` or `{count_min}`. A `sourced` finding
  needs a source `{file, lines, says}`; the renderer refuses a file that is not on disk or lines it does not have.
- `out/<name>.records.jsonl`: one record per cell (`row`, `column`, `input`, `line`, `prec`, `output`, `status`,
  `class`, `count`, `value`, and `reason` for a cell not run) and one per check. No times, so `--check` can diff.
- `out/<name>.run.json` (not compared): `process_time_s`, the lifetime of the driver process from start to exit
  or kill (a 4 s timeout gives about 4 s); `last_answer_s`, the time of its last output line; `cell_times`, the
  time between two output lines; `checks_process_time_s`; the sha256 of the driver; the commit.
- `out/<name>.html`: the page. `sweep.py`, `render.py`, `run.sh`, `test_bench.py`: the code and its tests.

**Limits.** The cell of `roots n` at R is `realroots` of `x^n - q`, so an input that is no rational has no cell
at R. The driver reads all its input before it runs, so a timeout ends the experiment: the rows not finished go
below the stop line. No agents: the findings are written by hand; the renderer checks only their rules.
