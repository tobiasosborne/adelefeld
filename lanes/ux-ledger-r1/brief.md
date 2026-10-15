# Lane ux-ledger-r1: the repair of the Ledger slice after its test drive

Read first: `CLAUDE.md`, `lanes/COMMON.md`, `lanes/UX-COMMON.md`, `ux/ledger/README.md`, `ux/ledger/FORMAT.md`,
`lanes/ux-ledger/result.md` (the builder's report), `lanes/ux-drive-ledger/report.md` (the test drive: defects
D1 to D4, usability U1 to U6, honesty H1 to H4, "FOR THE REPAIR" items 1 to 6) and `lanes/ux-drive-ledger/
drive.md` where the report points to it (the reproductions are under the lane's `status-checks.py`,
`extras.py`, `flint-equality.c`). The vision: `docs/ux/brief-3-claim-ledger.md`.

You own: `ux/ledger/**`, `lanes/ux-ledger-r1/**`, and ONE dated correction note in
`docs/ux/brief-3-claim-ledger.md` (item 2 below). Nothing else.

Repair, smallest first, each with a test in `ux/ledger/test_ledger.py` that was red before and green after:

1. D1: every README command line is pasteable: explanations become `#` comments or separate prose lines;
   the test runs each fenced command line of the README through `bash -n` at least, and the build and
   seal lines literally in a fresh shell (`env -i` as the builder did).
2. D4: the claim a6 is corrected: FLINT's `padic_equal` returns 0 for exp(3) and exp(12) at precision 8
   (the drive's probe `lanes/ux-drive-ledger/flint-equality.c`, output 958, 5125, `padic_equal: 0`;
   `refs/src/flint-3.0.1/padic.rst:263-266` for what `padic_equal` compares). a6's statement says what is
   true (the two centres differ by 4167 = 9 * 463, so they agree modulo 9 and not modulo 27; equality is
   false at precision 8) and keeps `script: []` with the probe named as its evidence. Add a dated note
   (three to five lines, plain) at the end of `docs/ux/brief-3-claim-ledger.md` saying that Scenario A
   step 4's sentence about FLINT's equality test is contradicted by the probe, and what is true instead.
   Do not rewrite the scenario.
3. D3: the sealer classifies a status wherever it appears on an output line: a line of the form
   `<prefix>: error: STATUS` (the Tate commands print `continuation S=...: error: NOT_DETERMINED`) gives the
   class of STATUS (open class -> `open`, budget -> `open`, proved-no -> compared as a line, not-mathematics
   -> `refuted` as now). Test with the drive's reproduction `drive-prefixed-open` and with the imported
   fixtures `fx-tate-gamma` and `fx-tate-fe` (their states before and after; say what changed).
4. D2 and U6: computed places are those with a driver line only. The footprint distinguishes `computed`
   (a filled box: a place at which a line of the script ran) from `named` (an outlined box: in `places` but
   without a line) and the tail. The importer takes the places of a fixture from its operands too: a token
   `with <prime>`, `with real`, `p=<n>`, `mod <n>` (trial division below 1000, cofactor shown); R when
   `real` or a real coordinate occurs. Test: the drive's `drive-footprint` claim (101 added to `places`)
   renders 101 outlined; the count of imported claims with an empty footprint before and after (29 before).
5. U3: branch scope: a claim whose script calls a root command (`root`, `root_at`, `roots_at`, `powrat_at`)
   must carry `"branch": "<seed>"` or `"branch": "all"` in the claim file; the sealer refuses to seal without
   it (state stays `proposed`, the reason printed). Test with the drive's seedless example and with the
   scenario claims that have roots (which ones need the field: say).
6. U2: `state: ratified` on a claim whose tail is `argued` without a citation of the form
   `refs/src/<path>:<lines>` or `docs/<path>:<lines>` in its argument is shown as "ratification refused:
   no source" (void), as a refuted claim's is void today. Test with the drive's `drive-citation` example.

Then regenerate the ledger (`import_fixtures.py`, `seal.py`, `render.py`), run the tests, and write
`result.md` (COMMON rule 8): each item with its red and green run, the state counts before and after
(94/4/1/1/0 before), what changed in the committed claims and seals and why, what is left (items 7 and 8 of
the drive are the next slice: ratify, comment, export; Firefox).
