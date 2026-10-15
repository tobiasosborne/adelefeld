# Lane ux-notebook-r1: result

The repair of the Adelica notebook slice after its test drive (`lanes/ux-drive-notebook/report.md`), items 1 to 6
of the brief, smallest first. Each item has tests in `ux/notebook/test/runtests.jl` that were run red before the
change and green after it. All commands ran from the worktree root with `ADELICA_LANE=lanes/ux-notebook-r1` (the
depot and TMPDIR of this lane), every one under `timeout`, at most 2 cores.

## What was repaired

1. **Item 1, the README.** The example `FBall(c, N)` (a MethodError for `FBall(0, 25)`) is now
   `FBall(Rat(c), Rat(N))`. The README holds an example notebook of 4 cells (`c = FBall(Rat(146), Rat(1000))`,
   `adf"146 mod 1000"`, `Rat(6, 35)`, `roots(-1, 2; at = 5)`). The test cuts that block out of the README and runs
   it. A section "What the slice does not do" names A5's `equal`/`compare`/`teichmuller`, B's certificates and
   acceptance state, and the missing failing place and reason codes. The README is 76 lines, widths at most 116.
2. **D3, the DOMAIN card.** `card_reason` (cards.jl) takes the call's arguments (`DriverError` now carries
   `:at`, `:n`, `:seed`). The slice checks the place for primality in Julia (trial division, exact below 10^12;
   above that the card says "not checked"), and the card says so. For `root(9, 2; at = 4, seed = 3)`: "the place 4
   is not prime (4 = 2 * 2; checked in Julia by this slice). The driver answers DOMAIN for a place that is not
   prime (tools/adf/README.md:241, the commands at places)". The action is "a prime place". At a prime, the card
   says the driver gives no reason code. It lists the possible causes of tools/adf/README.md:509-510 (the existence
   criterion, the seed for `root_at`, the degree 0) and what the slice checked ("the place 5 is prime, the degree
   is 2"). Its actions are "no single action: the driver does not say which cause holds" and "roots(x, n; at = p)
   lists the branches that exist". No card says "another seed" now. The records of status cards gain `reason` and
   `actions`.
3. **D4, above p = 97.** A single inexact root above 97 has a button `12 more digits (prec = N + 12)`. It carries
   the same `data-rewrite` as the fog, so the page's click script rewrites the cell in the same way. The note now
   says "no digits above 97: the button asks for 12 more digits (prec = 20)", not "click the fog".
4. **D1, replay.** `replay_summary` returns `(same, differs, unchecked)`. The last line of `replay.sh` is
   "replay: N same, M differs, K unchecked", and the exit status is 1 when M > 0. A record with no driver line is
   `unchecked` with its reason, never `same`. For every other record the replay compares:
   - the value text and the status;
   - the dump: through the `load` line itself, or as `dump_text(readvalue(value))` for a rat or fball (their text
     is exact); for an adele from a command it is noted as not checked, because its printed real ball is an
     enclosure;
   - the strip fields (real, primes, v, k, digits, cofactors, rest), recomputed from the driver's value;
   - the seeds, digits and local dumps of branches, and the entries and the dump of a projection;
   - the literal arguments of the input against the driver line (numbers, places, seed, prec, and a literal
     operand compared by the value it denotes). An operand that is a variable of the notebook is listed as not
     checked.
5. **D2, computed values.** The driver reads no dump as an operand (`project adf1 Q rat 1 1 with real` gives
   `error: PARSE`). So `lossless_operand` (driver.jl) gives a driver operand only when the text denotes exactly
   the value: the text of a Rat or FBall, the source text of a literal, or a text that reads back to the same dump.
   Literals written inline as arguments are recorded too (`hoist_literals`). A computed adele with no such text is
   handled as follows:
   - binary operations and `neg`: the package computes; `driver` is null and `unchecked` gives the reason;
   - `reconstruct(y)`: the package computes and gives NO_SOLUTION on the stored enclosure; the record says
     `computed_by: "package reconstruct"` and `unchecked`;
   - `roots`/`root`: the driver gets the exact finite part, since these commands project to the named prime
     (tools/adf/README.md:485), and the driver line reproduces the value;
   - `project`: the real component is the package's `realpart` (only at `prec = 64`), and the primes are the
     driver's projection of the finite part. `project(y, :real)` now gives `real: 0.99 +/- 0.01`, the driver's
     projection of the literal, where it gave `0.99 +/- 0.011` before.
6. **Item 6, provenance and inspection.** Every record has `dump` or `dump_reason`, plus `computed_by`,
   `unchecked` and `stamp`.
   - Branches: the driver's `dump [p=5: value]`, the lball body. It is lossless, since the branch text is exact
     data: `load` of it prints `[p=5: value]`, which is tested.
   - Fans: one dump per branch.
   - A projection with a real component: no dump, with the reason, and `dump_primes`, the dump of the components
     at the primes.
   - Statuses and Julia errors: a reason.
   Every value on the page has a `dump` details element. Chips and the projection strip have the hover text "the
   set ...", as the strips had. The footer and `stamp` give "Adelefeld.jl 0.1.0" (Base.pkgversion of the package's
   Project.toml) and the commit of the last change to `src`, `include` and `tools/adf` (`git log -1 -- ...`, which
   changes nothing). It is followed by "library and FLINT versions: not exposed by the package". The package's
   load gate only compares FLINT major and minor versions (julia/Adelefeld/src/load.jl:159-166) and returns no
   version string.

Two small changes outside the items:
- `ux/notebook/julia.sh` takes the lane directory from `$ADELICA_LANE` (default `lanes/ux-notebook`, as
  before), so that this lane did not write into another lane's directory.
- `runtests.jl` makes its temporary directories under TMPDIR, which `julia.sh` sets, instead of
  `lanes/ux-notebook/tmp`.

## Red and green runs

| Item | Red (`lanes/ux-notebook-r1/red_all.txt`) | Green |
|---|---|---|
| 1 README | 3 pass, 8 fail | 11 of 11 (run2.txt) |
| 2 D3 | 12 pass, 10 fail, 2 errors | 24 of 24 (run2.txt) |
| 3 D4 | 2 pass, 3 fail | 4 of 4 (run4.txt; one test is conditional on "click the fog" and no longer runs) |
| 4 D1 | 1 error (`replay_summary` undefined) | 18 of 18 (run5.txt) |
| 5 D2 | 18 pass, 12 fail, 3 errors | 23 of 23 (run5.txt) |
| 6 dumps, versions, hover | 6 pass, 30 fail, 8 errors | 44 of 44 (run4.txt) |

Red command: `ADELICA_LANE=lanes/ux-notebook-r1 timeout 600 sh ux/notebook/test.sh`. Result: 170 passed, 63
failed, 14 errors of 247; the 129 old checks all passed. The green run of item 3 alone was lost, because /tmp was
full for a few minutes; its first green run on disk is run4.txt.

After the first green attempt (run4), one expectation of repair 5 was changed. I had written the finite part as
`9 mod 625`; the FBall printer writes `(* ; 9 mod 625)`, which is the same set. The value of that record is still
compared with the driver on the literal adele. Two old tests were adapted to the new markup and paths:
- the chip regex of `page.test.js` now allows the `title` attribute;
- `mktempdir()` replaces the path under another lane's directory.
The new Node test of the refine button passed on its first run, because the click handler is generic. The red of
D4 is the Julia testset above.

## Final checks

- `timeout 300 sh ux/notebook/test.sh`: 253 passed of 253 (`final_tests.txt`).
- `timeout 60 node ux/notebook/test/page.test.js`: 4 tests, 4 pass (`final_node.txt`).
- `timeout 300 sh ux/notebook/run.sh`, twice, then `diff -r`: empty. Page times are 2.0 s and 1.8 s.
- `timeout 120 sh ux/notebook/replay.sh ux/notebook/out/a_roots.records.jsonl`: "replay: 5 same, 0 differs,
  0 unchecked", exit 0. `b_which_rational`: "replay: 8 same, 0 differs, 0 unchecked", exit 0.
- The drive's own notebooks, run with `run_file` into `lanes/ux-notebook-r1/drive/` and replayed:
  - `builder_followups`: 6 same, 0 differs, 4 unchecked, exit 0. Cell 9 is `real: 0.99 +/- 0.01`; cell 10 is
    NO_SOLUTION by the package; cells 5 and 6 are Julia errors. Cell 4 (101-adic) has the button with
    `prec = 20` and no "click the fog".
  - `a_flow`: 2 same, 0 differs, 2 unchecked, exit 0.
  - `extras`: 7 same, 0 differs, 2 unchecked, exit 0.
- The drive's six variant files (`lanes/ux-drive-notebook/replay_*.jsonl`, old record format):
  - `unchanged`: 8 same, exit 0.
  - `wrong_value`, `wrong_status`, `wrong_dump`, `wrong_digit`, `wrong_input`: each 7 same, 1 differs, exit 1.
    The messages are "value: record 23/7 ...", "status: record NO_SOLUTION, from the driver OK", "dump: record
    adf1 Q rat 17 7, from the driver adf1 Q rat 16 7", "digits at 2" and "input: num = 0, the driver line has
    30; input: den = 1, the driver line has 15".
- Mutation run (`timeout 175 python3 lanes/ux-notebook-r1/mutate.py A B` in 4 batches): 12 hand-chosen mutants of
  cards.jl, driver.jl and notebook.jl, 12 killed (`mutate.log`). This was a bounded run, not a sweep.
- Line widths in `ux/notebook/src`, `test`, the scripts and the README: none above 116 characters. The generated
  pages have long lines, as before.

## Diff of pages and records (`lanes/ux-notebook-r1/records_diff.py` against HEAD)

- Both scenarios: every value and every status is unchanged. Every record gains `dump_reason`, `computed_by`,
  `unchecked` and `stamp`; status records gain `reason` and `actions`; the projection gains `dump_primes`.
- A cells 2 and 4 (single roots): `dump` was null and is now `adf1 Q lball 5 x 3 1 0` and
  `adf1 Q lball 5 b 4889117f9ebf 0 14`. Every branch of the fans gains `dump`.
- The driver lines are unchanged.
- Pages: each value gains a `dump` details element and chips gain "the set ..." titles. A step 5's card now lists
  the possible causes instead of "another prime or another degree". Each page gains a footer with the stamp.
- `git diff --stat -- ux/notebook`: 13 files, 851 insertions, 170 deletions.

## What is left

- Item 7 of the drive, the next slice: A5's comparison (`equal`, `compare`, `teichmuller`; the driver has
  `equal` and `compare`, and there is no driver command for the local Teichmueller) and B's certificates and
  acceptance state.
- Two kinds of record are reported `unchecked`, not `same`: a computed adele at the real place or under
  `reconstruct`, and Julia errors. A driver that read a dump as an operand would make the first kind checkable.
- `project` of a computed adele at `prec` other than 64 is refused (an ArgumentError, shown as a Julia error);
  the slice does not round the stored real ball.
- Replay checks the literal arguments of an input, not its variables: it runs driver lines, not cells.
- Primality above 10^12 is not checked by the card.
- No screenshot was taken.

## Library demands (D-UX3) seen in this repair

1. The driver reads no dump as an operand, and has no variables. Without one of them a computed value cannot be
   sent to the driver losslessly.
2. The package exposes no version of the library or of FLINT (load.jl reads `adf_flint_version_compiled` only for
   an error message).
3. There are no reason codes for DOMAIN: a composite place and a failed existence criterion print the same line.
4. The failing place is still not printed.

## Files

- Changed: `ux/notebook/README.md`, `julia.sh`, `src/cards.jl`, `src/driver.jl`, `src/notebook.jl`, `src/page.jl`,
  `src/replay_main.jl`, `test/runtests.jl`, `test/page.test.js`, `out/a_roots.html`, `out/a_roots.records.jsonl`,
  `out/b_which_rational.html`, `out/b_which_rational.records.jsonl`.
- Lane: `result.md`, `.gitignore` (depot/, tmp/: the 2.1 MB depot is not to be committed), `mutate.py`,
  `mutate.log`, `mutate1-4.txt`, `records_diff.py`, `red_all.txt`, `run2.txt`, `run4.txt`, `run5.txt`,
  `final_*.txt`, `regen1.txt`, `regen2.txt`, `replay_*.txt`, `node.txt`, `d3_probe.jl`, `probe5.jl`, and `drive/`
  (the drive's three notebooks rerun, with their pages, records, replays and the six variant replays).

No sources are pending. There are no findings against the specification. `julia/Adelefeld/` was not changed.
