# Lane ux-ledger: the first buildable slice of brief 3, the Ledger

Read first: `CLAUDE.md`, `lanes/COMMON.md`, `lanes/UX-COMMON.md`, `docs/ux/README.md`,
`docs/ux/brief-3-claim-ledger.md` (all of it; the slice is its last section), `docs/ux/mock-3-claim-ledger.html`
(the look of the signature screen; reuse its markup and CSS where it fits), `tools/adf/README.md`,
`tests/driver/README.md` if it exists and `tests/test_driver.sh` (how fixtures are compared),
`include/adelefeld/status.h`, `docs/conventions.md` 9 and 11.1.

You own: `ux/ledger/**` and `lanes/ux-ledger/**`. Nothing else.

Build, in this order, each piece usable before the next:

1. The claim file format, `ux/ledger/claims/<id>.json`: `id`, `statement` (one sentence), `script` (a list of
   driver lines, each a string), `expect` (the exact output lines, one per script line that prints; a setting
   line prints nothing), `places` (the footprint: a list such as `["R", 2, 3, 5]`, and `"tail": "nothing said"`
   or `"tail": "argued"`), `argument` (prose, may be empty), `proposer` (a free string), `state` (`proposed`
   on creation). A claim about a root names its branch by seed in the statement or says "all branches" (brief,
   "Branches"). Write `ux/ledger/FORMAT.md` (under 60 lines) that defines each field and the states
   `proposed`, `sealed`, `refuted`, `open`, `ratified`.
2. The sealer, `ux/ledger/seal.py` (stdlib): for each claim, run `build/adf` on the script (one process per
   claim, `timeout 60`), compare the output lines with `expect` exactly, and write `ux/ledger/seals/<id>.json`:
   the verdict (`sealed` when every line matches; `refuted` when the driver printed a value or a status that
   contradicts the expectation; `open` when the driver printed a status of the open class, D-UX4, where a value
   was expected), the actual lines, the date, the build identity (the git commit `git rev-parse HEAD`, the
   sha256 of `build/adf`, the FLINT version read from the installed `flint/flint.h` `FLINT_VERSION` or from
   `ldd build/adf`; say in the README which), and the sha256 of the claim file. A seal of a later build that
   disagrees with an earlier seal reopens the claim: `seal.py` keeps the earlier seal under `seals/history/`
   and marks the claim `open` with both builds named. Exit status 0 when every claim is sealed, 1 otherwise;
   `--only <id>` and `--dry-run` (print the driver lines and expected lines, run nothing).
3. The renderer, `ux/ledger/render.py` (stdlib), writing one static page `ux/ledger/ledger.html`: the ledger
   list (id, state with its shape, the footprint strip `R 2 3 5 ...` with the tail, the statement), filters by
   state (inline JS), and one card per claim as the brief's "claim card" (statement, script and expected lines
   side by side with the actual lines of the seal, the footprint, the argument, the seal with build identity
   and date, and the history of seals). States with shapes AND colours (brief, "States"). The status classes
   of D-UX4 for the actual lines.
4. The first ledger: (a) import `tests/driver/*.cmd` with their `.out` as claims through
   `ux/ledger/import_fixtures.py`: one claim per fixture file (statement from the file's first comment line,
   places from the primes that occur in the file's operands, computed by a regular expression over `p=<n>`
   and `mod <n>` tokens with trial division of `n` below 1000, cofactor shown as `cofactor <n>` in the
   footprint when it is not 1), expectation the `.out` lines without the `#!exit` line; (b) the ten claims of
   the two scenarios of the brief, written by hand as claim files with the driver lines you verified
   (Scenario A: the table of 2-adic and 3-adic values; Scenario B: the overnight batch on powers and
   logarithms: read the scenarios and translate each claim into driver lines; where the driver cannot express
   a claim, record it in the report and write the claim with `state: "proposed"` and `script: []`, which the
   sealer leaves `proposed`). At least one claim must be refuted by construction (a deliberately wrong
   expectation, named as such in its statement) and at least one open, so that the page shows every state.
5. `ux/ledger/README.md` (D-UX6) and a test file `ux/ledger/test_ledger.py` (`python3 -m unittest`): the
   sealer on a sealed, a refuted and an open claim; the reopening on a changed build identity (simulate by a
   seal file with another sha256); the importer on two fixture files; the renderer producing a page that
   contains every claim id.

Checks to run and report with numbers: the count of claims, sealed, refuted, open, proposed; the sealer's
run time; `diff` of a regenerated `ledger.html` against the committed one; the unit tests; a walk of Scenario
A and B with the page open (what a referee sees, step by step, in the report).

Decisions you take yourself and record: the id scheme; how a family (a claim with a parameter) is written in
the slice (a decision: one file per instance is acceptable). Do not build: agents, a queue, ratification by a
person (the state exists in the format; no command sets it in the slice).
