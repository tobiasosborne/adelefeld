# Lane ux-atlas-r1: repair of the Place Atlas slice after its test drive

Claude Opus, worktree `adelefeld-wt/ux-atlas-r1` at 0bb9fa9. All five items of the brief are repaired, each
with a test that was red before the change and is green after it. No git state was changed, no `bd` was run.
Only `ux/atlas/**` and `lanes/ux-atlas-r1/**` were written. Every program ran under `timeout`, on at most 2 cores.

## What was repaired

1. **D1, the prime limit.** `atlas.js` now holds the drawing sizes in one `GEOMETRY` object (tree row width
   280, lit radius 5, unlit radius 2.2). The page reads every tree size from it. A row of n circles has
   centre spacing 280/(n+1). Two lit neighbours need spacing >= 2 x 5, so a row holds at most
   `rowCapacity() = floor(280/10) - 1 = 27` circles. Two branches that differ at their first digit put 2p
   siblings in the next row (a quadratic, the idempotents). So `MAX_PRIME` is the largest prime p with
   2p <= 27, which is 13. This is computed in `atlas.js`, not typed in. It is also the vision's own wheel
   threshold.
   - `scene.py` refuses a prime above 13 before any driver call, with exit 2 and one line:
     `scene: prime 101 is above the drawing limit 13 of this slice: a tree row in atlas.js holds 27 circles
     and two branches put 2p in one row`.
   - A scene that would draw a row of more than 27 circles (three or more lit parents) is also refused with
     exit 2. This check runs after the driver call, because only the driver's output gives the paths.
   - `validateScene` in the viewer refuses both cases too.
   - Non-primes are still refused with exit 1.
   - `scene.py` keeps `MAX_PRIME = 13` and `ROW_CAPACITY = 27`, and a test checks through `node` that these
     agree with `atlas.js`. The same test checks that `row_sizes` in Python and `chooseNodes` in JS count
     the same rows on four cases.
   - The README states the limit and the reason for it.
2. **Item 2, the solved polynomial.** The scene now carries `solved_polynomial`, written from the
   coefficients that were passed to the driver's `roots` and `realroots` commands: `X^2 - X` for
   idempotents30 and `X^2 - 9` for sqrt9.
   - The header lens reads `roots 2 · X^2 - X = 0`, and the document title starts with the same text.
   - The map's accessible label reads `Roots of X^2 - X = 0 at real, 2, 3, 5; input (* ; 0 mod 30)`.
   - For `roots_at` on a ball input, which has no explicit polynomial, the lens reads `roots 2 · X^2 = x`
     and the label reads `input x = ...`. Other lenses are unchanged (`exp of ... at ...`).
   - The functions are `sceneTitle` and `mapLabel` in `atlas.js`, and `polynomial_text` in `scene.py`.
3. **Item 3, a gate caused by the missing real coordinate.** `docs/conventions.md:1183` gives the grammar
   `fball_v = "(" "*" ";" fin ")"`: a printed `(* ; ...)` has no real coordinate.
   - When the real call's status is not OK, the driver's canonical `show` line starts with `(* ;`, and the
     driver also refused `project <value> with real`, the ruler gets
     `reason: "no real coordinate in the input"` and `reason_records` (the show and project records).
   - The gate draws that reason above `Reported failing place: not given`.
   - The raw status (`DOMAIN`, proved no) and `failing_place: not given` stay unchanged.
   - Both on the real ruler and on the trees, gate lines now come from one function, `gateLines`.
4. **Item 4, negative valuation.** The new `layoutTree(paths, p, startExponent, w, h, {topExponent,
   bottomExponent})` puts a node at depth d, with root exponent s, at exponent s + d. That node is the ball
   a + p^(s+d) Z_p (as `digit_path` builds it). The y axis is the exponent.
   - Each tree has an exponent scale on its left (`5^-1`, `5^0`, `5^1`, ...).
   - A dashed baseline marks the level of exponent 0, labelled `Z_5`.
   - The page puts all trees on one exponent axis, so the baselines line up across trees. For 1/5 at 5,
     the root (`5^-1`) stands one level above the Z_5 baseline. The 2-tree's root sits on its Z_2 baseline.
   - Hover balls are unchanged: no `node_balls` value changed in either fixture.
   - To make room for the scale, the panels widened from 310 to 330 (pitch 350). The tree row is still 280.
5. **H1, derived numbers.** The scene has a new `derived` block. It names each kind of derived number and the
   field that leads to its driver line: path digits, digit string and nodes; the sibling hover balls; the
   branch and lit-path counts; the fog counts; and the first digits in the modular table.
   - The fog now has a `source_record` (the `show` record). Its counts are now computed from the driver's
     printed canonical line rather than from the user's text; the values are identical.
   - The README's "Derived numbers" paragraph and the page footer say which numbers are derived under
     D-UX5 as amended. They also say how to trace one: a path's `source_record`, the fog's `source_record`
     and a table row's `records` index `provenance.records`, which holds the command and the driver's
     output line.
   - Each path card on the page now names its record and command: `digits derived from record 12: roots
     -9 0 1 with 2 with 5`.

## Red and green runs

All commands were run from the worktree root. The driver was built with
`timeout 600 make -j2 && timeout 600 make -C tools/adf` (exit 0, `build.log`). Before any change, a baseline
regeneration into a scratch directory gave a 0-line `diff -ru` against `ux/atlas/scenes`. The baseline suites
were Python 22 tests OK and Node 9 of 9.

Red (tests written first, code unchanged; `red-node.log`, `red-python.log`):

- `timeout 120 node --test --test-isolation=none ux/atlas/test_atlas.mjs`: 17 tests, 9 pass, 8 fail. The
  8 failures are exactly the new tests: item 1 (limits from geometry; primes 2..13 at depth 1 with centre
  distance >= 2r; the viewer refuses 17, 101 and a row of 39; no overlap in sqrt9; no overlap in
  idempotents30), item 2 (title and label), item 3 (gate lines) and item 4 (1/5 layout).
- `timeout 170 python3 -m unittest discover -s ux/atlas`: 30 tests, 2 failures and 5 errors, all in the new
  tests: the item 1 refusal, the item 1 sync with atlas.js, `polynomial_text`, the item 2 fixtures, the item
  3 reason, item 5 provenance, and the README statement. One new test was green before and after, by
  design: a guard that a genuine real `DOMAIN` (log of -1) gets no missing-coordinate reason.
- The old builder on prime 101:
  `timeout 60 python3 ux/atlas/scene.py 9 --lens 'roots 2' --primes 101 --driver /nonexistent/adf --output X`
  answered `scene: driver exit 127: ...` with exit 1. It accepted 101 and went on to call the driver.

Green:

- Node: 17 tests, 17 pass. Item 4 first failed on a `-0` (from `-top * step`), which was then written as
  `(0 - top) * step`. Item 2 turned green when the fixtures were regenerated.
- Python: 31 tests OK (`ADF=build/adf timeout 170 python3 -m unittest discover -s ux/atlas`). This includes
  two tests added after the first green run:
  - a boundary case in both suites: a row of exactly 27 circles is accepted and its circles do not overlap;
  - `test_fixtures_regenerate_identically`, which runs `build_scenes.sh` into a temporary directory and
    compares both files with the committed fixtures.
- The same refusal now gives exit 2, one line on stderr, and no output file. Prime 13 is accepted (exit 0).
- The README commands, run as written from a fresh shell: `make` exit 0; `build_scenes.sh` exit 0;
  unittest 31 OK; node 17 of 17.
- Page check (`lanes/ux-atlas-r1/page_check.cjs`). It runs the real page script in a minimal DOM stand-in,
  on the pattern of the drive's `check_dom.cjs`; it is not a browser. Each run exited 0:
  - idempotents30: 13 assertions;
  - sqrt9: 14 assertions;
  - a fresh 1/5 identity scene at 2 and 5: 10 assertions. The scale has `5^-1` above `5^0`, the baselines
    are `Z_2,Z_5`, `2^0` and `5^0` have the same y, and the hover text is still `[p=5: 0 + O(5^-1)]` and
    `[p=5: 1/5 + O(5^0)]`;
  - the README's exp example: 6 assertions. The gate shows `DOMAIN`, then "no real coordinate in the input",
    then "Reported failing place: not given".

  In all four, every path card's record index resolves to the command it names.
- Line style: README 79 lines (under 80). No line over 116 in any file of `ux/atlas`.

Bounded hand mutants (`timeout 170 python3 lanes/ux-atlas-r1/mutants.py SCRATCH SCRATCH`, `mutants.log`):
17 mutants of the lines this repair wrote in `atlas.js`, `scene.py` and `atlas.html`, run on a copy.

- In the first run, 13 of 17 were killed. M14 (fog `source_record` pointed at the wrong record) and M15
  (`solved_polynomial` set to the raw coefficients) survived: the fixture tests read the committed scenes,
  not the builder's output. The regeneration test was added for this.
- In the second run, 15 of 17 were killed.
- The 2 survivors, M11 and M12, are equivalent. Each drops one half of the missing-coordinate condition:
  either the `(* ;` grammar test or the driver's refusal of the real projection. `parse_value` admits only
  rationals and finite balls. A probe (`projection.cmd`, `projection.out`) shows that the driver refuses the
  real projection of all 6 finite-ball forms tried (`error: DOMAIN`) and projects all 5 rationals tried.
  So the two halves agree on every accepted input. Both are kept, so the reason is claimed only when the
  grammar and the driver agree.

## Fixtures

`ADF=build/adf sh ux/atlas/build_scenes.sh` regenerated both fixtures. A second regeneration into a fresh
directory gives a 0-line `diff -ru`. Against 0bb9fa9, `git diff --stat` shows 12 lines changed in each file
(22 insertions and 2 deletions in total). A key-by-key comparison of the two JSON trees found no key removed
and no value changed, only keys added:

- item 2: `solved_polynomial` (`"X^2 - 9"`, `"X^2 - X"`);
- item 5: `derived` (`rule`, `digits`, `node_balls`, `counts`, `fog`, `modular_digits`) and
  `fog.source_record` (0, the `show` record).

Item 3 adds `reason` only to a failed real gate of a `(* ; ...)` input. Both fixture rulers are OK, so it
does not appear in them. Item 4 changes no data. This matches the brief's expectation (items 2 and 5 only).

## Files

Changed: `ux/atlas/atlas.js`, `ux/atlas/atlas.html`, `ux/atlas/scene.py`, `ux/atlas/README.md`,
`ux/atlas/test_atlas.mjs`, `ux/atlas/test_scene.py`, `ux/atlas/scenes/sqrt9.json`,
`ux/atlas/scenes/idempotents30.json`.
Lane files: `result.md`, `page_check.cjs`, `mutants.py`, `mutants.log`, `projection.cmd`, `projection.out`,
`red-node.log`, `red-python.log`, `build.log`, `firefox.log`.

## What is left

- Item 6 of the drive report (save branch selections as a partial-product scene) and the usability finding
  U1 (scenarios are a file-generation workflow) are the next slice, as the brief says. Not touched.
- Residue wheels above 13 are not built; such primes are refused instead.
- Digit labels at p = 13. By estimate (not measured in a browser), a two-digit label at 11 px monospace is
  about 13 px wide. In a row of 26 circles (two branches at p = 13) the spacing is 280/27 = 10.4. So the
  labels "10", "11" and "12" of neighbouring circles may touch or overlap, although the circles do not.
  At p = 11 only "10" has two digits and its neighbours fit. This is worth a look in a real browser.
- On the shared exponent axis, a tree whose own root is at exponent 0 still shows the scene's upper levels
  on its scale (in the 1/5 scene the 2-tree has a `2^-1` label with no node). This is deliberate, so the
  baselines align, but it is a design choice a user may question.
- `layoutTree` assumes that all paths of one result share their start exponent. This holds today: the
  polynomial solver lists Z_p roots only (start 0), and the roots of X^n - q all have valuation v(q)/n. It
  would need care if another lens produced mixed valuations.
- No browser check. One headless Firefox screenshot attempt exited 0 but wrote no PNG (`firefox.log`); it
  was not retried. A page opened from `file://` shows only the empty state before the picker in any case.
  The drive's Firefox exit 139 was not reproduced.

## Sources pending

None. The only outside convention used is `docs/conventions.md:1183` (`fball_v`) and the canonicalisation at
`docs/conventions.md:1236` (a bare `a mod N` prints as `(* ; a mod N)`); both were read on disk.

## Findings against the brief or the specification

None against the specification. Against the brief, one note: the brief asks for the prime refusal before
any driver call. That alone does not make "no overlapping sibling circles in any accepted scene" true. A
scene at p <= 13 with three or more lit parents in one row (for example a degree-3 polynomial with three
roots at 13) would draw 39 circles in a row. The row-capacity refusal closes this gap, but it can only run
after the driver call.
