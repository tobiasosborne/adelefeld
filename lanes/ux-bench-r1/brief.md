# Lane ux-bench-r1: the repair of the Bench slice after its test drive

Read first: `CLAUDE.md`, `lanes/COMMON.md`, `lanes/UX-COMMON.md`, `ux/bench/README.md`,
`lanes/ux-bench/result.md` (the builder's report), `lanes/ux-drive-bench/report.md` (the test drive: defects D1
to D3, usability 1 to 5, "FOR THE REPAIR" items 1 to 5) and `lanes/ux-drive-bench/drive.md` and its experiment
files where the report points to them (`brief_a.json`, `attack.json`, `timeout_real.json`,
`finite_ball.json`, `demands.cmd`). The vision: `docs/ux/brief-4-sweep-bench.md`.

You own: `ux/bench/**` and `lanes/ux-bench-r1/**`. Nothing else.

Repair, smallest first, each with a test in `ux/bench/test_bench.py` that was red before and green after:

1. Item 1: the README and the records' "library demands" wording: the driver has `legendre`, `jacobi`,
   `kronecker` and `hilbert_at` (the drive ran `legendre 2 with 7` = 1, `jacobi 2 with 15` = 1,
   `kronecker 3 with 2` = -1, `hilbert_at 3 with 3 with 2` = -1; find their README lines in
   `tools/adf/README.md` and cite them). Then add them as sweep operations (`legendre`, `jacobi`,
   `kronecker` with the column input as the first operand and the row prime as the second where that is the
   command's form; `hilbert a b` at every place as the brief's "natural wall cell"): a value well with the
   symbol; a fixture `symbols.json` (columns 2, -1, 3, 5; rows the primes below 100; `legendre`) whose wall
   agrees with the squares wall (a test compares the two record files place by place: 1 where two roots,
   -1 where DOMAIN at odd p; say what happens at 2).
2. D1 (item 2): the page footer names the records and findings files actually read (with `--out` and a custom
   experiment directory); test with the drive's `brief_a.json` and `--out` into a temporary directory: both
   named paths exist.
3. Item 3: the row label says `requested prec 8`; a well whose output carries a smaller `O(p^k)` shows `k` in
   the well (not only in the tooltip); test with the drive's `finite_ball.json` (roots at 5 of `(* ; 9 mod
   15625)`: O(5^6) against prec 8).
4. D2 (item 4): a finding whose rule the renderer finds contradicted by the records is rendered in the state
   `refuted` with its witnesses (the badge, the shape and the words agree); its file state stays as written
   and the card says "written as pattern, refuted by the records at ..."; test with the drive's `attack.json`
   (eight witnesses).
5. D3 (item 5): `process_time_s` of the run file is the driver process's lifetime (start to exit or kill);
   the sum of line intervals keeps its own name (`last_answer_s`); the README says which is which; test with
   the drive's `timeout_real.json` recipe (a 4 s timeout gives a lifetime near 4 s) using the fake hanging
   driver the tests already have, so that the test is fast.

Then regenerate the fixtures (`sh ux/bench/run.sh`, then `run.sh --check`), run the tests, and write
`result.md` (COMMON rule 8): each item with its red and green run, the fixtures' diff summary, what is left
(items 6 to 8 of the drive are the next slice: cost preview, the finite-ball lesson, the family fold).
