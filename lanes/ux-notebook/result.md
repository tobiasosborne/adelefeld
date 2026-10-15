# Lane ux-notebook: result

## What was built

The first slice of brief 1 (Adelica) under `ux/notebook/`, in the order of `lanes/ux-notebook/brief.md`:

1. `Adelica`, a Julia module loaded with `--project=julia/Adelefeld` and `push!(LOAD_PATH, "ux/notebook/src")`.
   It runs offline and is precompiled into `lanes/ux-notebook/depot`. It contains:
   - A parser for exactly the start symbols rat_v, fball_v, adele_v and cadele_v of conventions 9.2.
   - The place strip as `show(io, MIME"text/html"(), x)` for Rat, FBall, Adele and CAdele. Column R (or C) holds
     the package's real ball, `exact: q` for a Rat, or `*` for an FBall. Then come the primes below 1000 dividing
     the modulus or the centre's denominator (trial division in Julia), a `cofactor n` column, and
     `every other p`.
   - Digits: for a Rat, the pre-period and period with a cap of 64 digits and `...`. For a ball, the digits of
     the centre with fog from p^k on, and the p-adic point. Residue form only above p = 97.
2. `roots(x, n; at, prec = 8)`, a fan, and `root(x, n; at, seed, prec)`, one branch, through the driver's
   `roots_at` and `root_at`. A chip click rewrites the cell to `root(...; seed = s)`. A click on the fog of a single
   root rewrites `prec` to `prec + 12`.
3. Status cards for every status name of status.h, with the class and shape of D-UX4 and `place: not given`.
   Each card has a reason (status.h comments, or the driver README sentence for roots_at, root_at and recover) and
   the actions as text.
4. `Adelica.run(cells, out)` writes `out.html` and `out.records.jsonl`. `Adelica.replay(records, adf)` returns
   the number of differences; `replay.sh` exits 1 on a difference. The language: the package constructors,
   `adf"..."`, `+ - * /`, `reconstruct`, `roots`, `root`, `recover` and `project`.
5. The scenarios `a_roots.jl` (5 cells) and `b_which_rational.jl` (8 cells), their pages and records in
   `ux/notebook/out/`, and `run.sh`.
6. `README.md` (66 lines), `test/runtests.jl` (Julia) and `test/page.test.js` (Node, the click script).

The brief's slice reading is recorded in the README: no Pluto, so the page is static, a click edits the shown
text only, and the cell is marked "re-evaluate to keep".

## Files

- `ux/notebook/README.md`, `run.sh`, `replay.sh`, `test.sh`, `julia.sh`
- `ux/notebook/src/`: `Adelica.jl` (module root), `valuetext.jl`, `digits.jl`, `strip.jl`, `driver.jl`, `cards.jl`,
  `json.jl`, `notebook.jl`, `page.jl`, `make_pages.jl`, `replay_main.jl`
- `ux/notebook/test/runtests.jl`, `ux/notebook/test/page.test.js`
- `ux/notebook/scenarios/a_roots.jl`, `b_which_rational.jl`
- `ux/notebook/out/a_roots.html`, `a_roots.records.jsonl`, `b_which_rational.html`, `b_which_rational.records.jsonl`
- Lane files: `result.md`, `shot-a_roots.png`, `shot-b_which_rational.png` (headless Firefox), `mutate.py`,
  `mutate.log`, `wrap.py` (line wrapper), `jl.sh`, `tmp/make.log`, `tmp/exports.log`. The directory
  `lanes/ux-notebook/depot` (1.9 MB of precompile cache) should not be committed.

## Checks (all run from the root of the worktree)

- Build: `timeout 600 make -j2` exited 0, and `make -C tools/adf` exited 0. `sh tests/test_exports.sh` gave
  "passed: 625 of 625 declared functions are exported" and wrote `build/libadelefeld.so`.
- `sh ux/notebook/test.sh`: 129 passed of 129. By testset: parser on the five SPEC 4.3 rows and the operand texts
  45; Rat digits 11 (1/3 at 2, 3, 5 and 22/7 at 7, 11, hand computations in the comments); ball digits 6; places
  20; fans through the driver 13 (two chips for `roots(9, 2; at = 5)` and for `roots(-1, 2; at = 5)`, DriverError
  DOMAIN for `roots(2, 2; at = 5)`, the `+1` seed at 2); status classes 12; notebooks, records and replay 22
  (records = cells, a tampered record is caught, `replay` gives `same` for every record of both scenarios).
- `node --test ux/notebook/test/page.test.js`: 3 tests, 3 passed. They check that a chip click rewrites
  cell 1 to `root(9, 2; at = 5, seed = 3)` and marks it, that restore undoes it, and that the fog of cell 4 asks
  for `prec = 32`. (Node 24 does not take a directory for `--test`, so the README names the file.)
- Regeneration: copied `out/`, ran `sh ux/notebook/run.sh` again, `diff -r`: empty. This was repeated after every
  change, last in a clean environment (`env -i HOME PATH`).
- Replay: `sh ux/notebook/replay.sh ux/notebook/out/a_roots.records.jsonl` gave 5 of 5 `same`, exit 0. The same for
  `b_which_rational.records.jsonl` gave 8 of 8 `same`, exit 0.
- Run time of `Adelica.run_file`, over 8 runs on the shared machine: a_roots 1.3 to 2.4 s, b_which_rational 1.5
  to 3.2 s. The whole `run.sh`, Julia start included, takes about 7 s.
- Mutation run (`lanes/ux-notebook/mutate.py`): 10 hand-chosen mutants of `digits.jl`, `valuetext.jl`, `strip.jl`,
  `notebook.jl` and `driver.jl`. 7 were killed at first. Survivors: no test of text after a value (now tested),
  no test of the `+1` seed at 2 (now tested), and one equivalent mutant (`>=` against `>` at v_p(a) = k gives v = k
  in both branches). After the new tests 9 of 10 were killed, 1 is equivalent. A bounded run, not a sweep.
- Lines of 116 characters at most in every file of `ux/notebook` (counted in characters).

## What each step shows (the numbers are in the records files)

Scenario A (`a_roots.html`):
1. `roots(9, 2; at = 5)` gives a fan with the chips `5 [2]` -3, digits `…4̄442`, and `5 [3]` 3, digits `…0̄003`.
2. `root(9, 2; at = 5, seed = 3)` gives one branch, `5 [3] · exact`, 3.
3. `roots(-1, 2; at = 5)` gives `280182 + O(5^8)` with fog then 32431212, and `110443 + O(5^8)` with fog then
   12013233. These are the brief's digits.
4. `root(-1, 2; at = 5, seed = 2, prec = 20)` gives `79753541295807 + O(5^20)`, 20 digits known:
   40423140223032431212. The low 8 digits are those of step 3. A fog click writes `prec = 32`.
5. `roots(2, 2; at = 5)` gives a card: DOMAIN, proved no, solid border, place: not given, from: driver roots_at.

Scenario B (`b_which_rational.html`):
1. `c = adf"146 mod 1000"` gives a strip: R `*`; 2: v = 1, `~010`; 5: v = 0, `~041`; every other p: Z_p.
2. `recover(c; num = 30, den = 15)` gives 22/7 as a strip: R 3.142857142857142857 +/- 2.4e-19 exact; 2: v = 1,
   `(011)1010`; 7: v = -1, `(0)3.1`; 11: v = 1, `(4 7 9) 5 0`; every other p: unit.
3. `x = adf"(3.14 +/- 0.01 ; 1/7 mod 1)"` gives a strip: R 3.14 +/- 0.011; 7: v = -1, `~.1`; every other p: Z_p.
4. `reconstruct(x)` gives 22/7. Steps 5 and 6: `num = 40, den = 30` gives 22/7, and `num = 400, den = 300` gives a
   NOT_UNIQUE card (open, dashed).
7. `recover(adf"1 mod 2003"; num = 3, den = 1001)` gives a NOT_DETERMINED card. Its actions are a larger search
   limit (fixed at 1000 in the driver) and tighter bounds.
8. `project(r, :real, 2, 7)` gives `real: 3.142857142857142857 +/- 2.4e-19; 2: 22/7; 7: 22/7`.

## Library demands met by the slice (D-UX3, against the 8 items of the brief's "What the library must provide")

1. Result record: not met by the library. The slice puts the record together from separate package calls
   (value_text, dump_text) and the driver's line. The failing place is `not given` everywhere: the driver does
   not print it (tools/adf/README.md:253, :579-580) and the package's `AdfResult.where` is `nothing` for every
   function so far. There is no precision per place; the strip records give `k` per column.
2. Value forms of lball and sball in the driver: the driver's `project` prints its own text, not the value form
   (tools/adf/README.md:255-266). The slice parses that text. Not met for the commands at places.
3. Dump forms of unit cosets, ideles and classes: the driver has them now (README, lane u-dump1). Not used: the
   package has no Idele type, so the language has none.
4. Digits: not met. They are computed in Julia (`digits.jl`) from the value text.
5. Interesting places without factorisation: not met. Trial division below 1000 in Julia, with the cofactor
   shown.
6. Reason codes: not met. The cards use sentences per operation from the driver README, not sub-codes.
7. The per-place `f_at`: not used.
8. Determinism: observed (regeneration diff empty, replay same). There is no provenance stamp: the page names no
   library, FLINT or build version.

Also unmet: the brief's slice wants an `LBall` wrapper; the package has none (J-b).

## What the test driver should try

- Follow the README alone. Open both pages in Firefox. Click the chip [3] of In [1], then restore. Click the fog
  of In [4].
- Write a notebook of your own with `run_file`. Try a ball with a cofactor, `FBall(Rat(1), Rat(8 * 1009 *
  1013))`; division by 0 (a NOT_UNIT card from the package); `roots(adf"(* ; 1 mod 4)", 2; at = 2)` (a
  NOT_DETERMINED card); a prime above 97; a negative valuation; Julia syntax errors.
- Edit a record's `value` and check that `replay.sh` exits 1.
- Check the digits against your own hand computation, and hover a strip for the set in words.

## Findings against the brief

1. Brief item 2 asks for the polynomial `roots` command on X^n - x. That command takes integer coefficients only
   and prints `A mod P^K` without seeds (tools/adf/README.md:160-166): `roots -9 0 1 with 5 with 8` gives
   `3 mod 5^8; 390622 mod 5^8`. The slice uses `roots_at` and `root_at` (README:482-510), which print the seed and
   take rationals, balls and adeles.
2. "The driver line that reproduces the value" cannot be `show <value text>` for a real coordinate. Conventions
   9.6 (docs/conventions.md:1336-1340) lets a printed real ball widen when it is read again. Measured:
   `show (3.14 +/- 0.011 ; 1/7 mod 1)` prints `(3.14 +/- 0.012 ; 1/7 mod 1)`, and replay first said `differs` for
   cell 3 of Scenario B. The slice now writes `load <dump>` for a value with no command of its own, and the source
   text of a literal for its operands. A driver call on a computed adele still gets the printed, wider text. None
   occurs in the scenarios.
3. At 2 the driver prints the identifier `+1` but reads only `1`: `root_at 9 with 2 with 2 with +1` gives
   `error: PARSE`. The chip rewrite maps `+1` to `1`.
4. Scenario B's NOT_DETERMINED card cannot come from 146 mod 1000 with the brief's bounds. 40/30 gives 22/7, as
   the brief says, and 400/300 gives NOT_UNIQUE. The slice uses the fixture case `recover 1 mod 2003 with 3 with
   1001` (tests/driver/s-recover.cmd) for that card.
5. For roots_at the call has one place, yet the card says `place: not given`, as D-UX3 asks. The asked place is
   in the record (`driver` line), not claimed as the failing one.
6. The fog drag is a click on the fog of a single root (+12 digits), not a drag, and there is none on fan chips.
   The "count" in `every other p` is always "infinitely many primes".

No sources are pending. Every citation in the code was checked against the file and line. The specification and
`julia/Adelefeld/` were not changed.
