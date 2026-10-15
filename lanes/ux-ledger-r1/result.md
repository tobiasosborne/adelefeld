# Lane ux-ledger-r1: the repair of the Ledger slice after its test drive (result)

Claude Opus, in the worktree `adelefeld-wt/ux-ledger-r1` at b02c739. All six items of the brief are repaired, each
with a test in `ux/ledger/test_ledger.py` that was seen red and then green. Repairs were done smallest first.
No git state command, no bd. Every program ran under `timeout` on at most 2 cores. The library was not changed.
Logs of every red and green run are in this directory (`*-red.log`, `*-green.log`).

## The state counts

Before (seal.py at b02c739): proposed 1, sealed 94, refuted 4, open 1, ratified 0; exit 1.
After (`lanes/ux-ledger-r1/seal-after.log`, then a second run): proposed 1, sealed 94, refuted 4, open 1,
ratified 0; exit 1. No claim of the committed ledger changed its state. The repairs change how states and places
are decided and drawn; on this ledger they change the footprints, the status tags and the claim files.

## The items

1. **D1, the README commands.** Test `TestReadme`. It takes every non-comment line of the fenced `sh` blocks, runs
   `bash -n` on each, and requires no indented code block and fewer than 80 lines (D-UX6). It then runs each line
   literally in `env -i PATH=... HOME=... bash --noprofile --norc -c`: the build line in the worktree, and the
   import, seal and render lines in a temporary root (a copy of `ux/ledger`, links to tests/, build/, include/,
   refs/, docs/). Afterwards it checks that all files of the copy are identical byte for byte. The firefox line
   and the unittest line get `bash -n` only.
   Red (`d1-red.log`): 0 fenced lines. Red with the old block fenced unchanged (`d1-red-literal.log`): the build
   line exits 2, `make: *** No rule to make target 'the'.  Stop.`, and `bash -n` rejects the import line
   (`syntax error near unexpected token '('`). Green (`d1-green.log`): 2 tests OK, 0.77 s. The explanations are
   now `#` comments. The README is 79 lines.
2. **D4, claim a6.** Test `TestClaimA6`. a6's statement now says that `padic_equal` returns "not equal" at
   precision 8. The centres are 958 and 5125; they differ by 4167 = 9 * 463, so they agree modulo 9 and not
   modulo 27. The test checks these numbers with `oracle.exp_mod` and with arithmetic. a6 keeps `script: []`.
   Its argument names the probe `ux/ledger/probes/flint-equality.c` (a copy of the drive's
   `lanes/ux-drive-ledger/flint-equality.c` with a header comment) and `refs/src/flint-3.0.1/padic.rst:263-266`.
   The test compiles the probe with `cc -std=c11 -Wall -Wextra -Werror ... -lflint -lgmp` and runs it. It checks
   that the four lines the argument quotes are in the probe's output: `exp(3): 958`, `exp(12): 5125`,
   `precision: 8 8`, `padic_equal: 0`. Red (`d4-red.log`): 2 failures (`'are equal' unexpectedly found`, and no
   quoted line). Green (`d4-green.log`): 2 OK. The dated note of 2026-10-16 (5 lines of text) is at the end of
   `docs/ux/brief-3-claim-ledger.md`; `git diff --numstat` gives 8 lines added, 0 removed. The scenario was not
   rewritten.
3. **D3, prefixed status lines.** Test `TestPrefixedStatus`. `ledger.line_status` now reads `error: S` at the start
   of a line or after a prefix ending in `: `. A line of several items is still a value line. The items are
   counted at `; ` outside brackets, because the Tate prefix itself holds ` ; ` inside parentheses: the first
   attempt with `[^;]*` stayed red. Checked: a prefixed NOT_DETERMINED or LIMIT gives open; a prefixed DOMAIN or
   PARSE gives refuted; an expected prefixed DOMAIN gives sealed; raw LIMIT and PARSE keep their classes.
   The drive's `drive-prefixed-open` claim now gives seal verdict `open`, a card with `cls cls-open` and
   `OPEN · NOT_DETERMINED`, and no REFUTED. The test also checks that the card of each Tate fixture has one class
   tag per status line. Red (`d3-red.log`): 3 failures (`'OK' != 'NOT_DETERMINED'`, `'refuted' != 'open'`,
   `0 != 2 : fx-tate-gamma`). Green (`d3-green.log`): 3 OK.
   fx-tate-gamma and fx-tate-fe were sealed before and are sealed after: their lines equal their .out files, so the
   class of a line cannot change their state. What changed is their cards. Class tags went from 0 to 2 on
   fx-tate-gamma, 2 to 8 on fx-tate-fe and 0 to 3 on fx-tate-local. The whole page went from 782 to 793 tags.
4. **D2 and U6, the footprint.** Test `TestFootprint`. `ledger.line_places` gives the places of one driver line.
   For a command with a place operand, it takes only that operand. The table `PLACE_OPERAND` is read from
   tools/adf/README.md, with the lines cited in `ledger.py`: exp_at, roots, roots_at, root_at, powrat_at,
   Log_at, valuation, abs, legendre, hilbert_at (third operand), project and Log_refine (lists), the Tate local
   commands, and others. In that operand, `real` gives R, a prime below 1000 is a place, a number with no factor
   below 1000 is a cofactor, and a composite is no place. For any other command, the rule is the moduli of
   `p=<n>` and `mod <n>` (trial division, cofactor shown), plus R for the word `real`, for a real coordinate
   other than `*` in `(A ; ...)` or `<A ; ...>`, or for `realroots`. The importer writes this as `places`.
   The renderer draws a box filled when a script line that printed (by the seal's `script_line`) names the place,
   and outlined when the place is in `places` with no such line. The legend and the card say so.
   The drive's `drive-footprint` (b6 with 101 added to places) now shows `11 ... 97` filled with the title
   `21 primes: 11 ... 97` and `<span class="n">101</span>` outlined. Red (`d2-red.log`): 3 failures
   (`('22', '101') != ('21', '97')`; `[] != [2]` for `exp_at 4 with 2`; and the count of fixtures with
   `with real`, see finding 2). Green (`d2-green.log`): 3 OK. Imported claims with an empty footprint: 29 before,
   7 after (fx-10_line_language, fx-char-dump, fx-char-values, fx-rfun-dump, fx-rfun-eval, fx-rfun-fourier,
   fx-rfun-text). All 19 fixtures with `with real` on a non-comment line now name R. Filled boxes in the list:
   147 before, 250 after; outlined boxes: 1 before and 1 after (a6, which has no script). On the committed ledger,
   no other claim names a place that its script does not compute.
5. **U3, branch scope.** Test `TestBranch`. A new field `branch` (FORMAT.md) is required when the script calls
   `root`, `root_at`, `roots_at` or `powrat_at`. Its value is a seed (every root line must use that seed, read
   from the seed operand, `root`'s sign 1 by default, and no line may be `roots_at`) or `"all"`. Otherwise the
   sealer prints `refused: ...`, writes no seal and leaves the claim proposed, and the card says why. The drive's
   seedless example ("The square root of -1 at 5 is 2 mod 5.", `roots_at -1 with 5 with 2`) is refused. With
   `"all"` it seals; `root_at ... with 2` and `... with 3` with branches `2` and `3` seal. Branch `2` against seed
   3, or a seed with `roots_at`, is refused. Red (`u3-red.log`): 2 failures and 1 error; the positive control
   passed before and after. Green (`u3-green.log`): 4 OK. Claims that need the field: b5, b5-1, b5-2 (all
   `roots_at`; `"all"`, and their statements already said "all branches"). Among the fixtures: fx-gfunc-root,
   fx-pow-status, fx-pow-values, fx-root-status, fx-root-values. The importer gives a fixture its one common seed,
   or `"all"`; all five got `"all"`. b4 uses the polynomial `roots`, which is not a root command of the brief.
6. **U2, ratification needs a source.** Test `TestRatification`. A claim file set to `ratified` with an argued
   tail is shown sealed with the note "ratification refused: no source (...): the ratification is void" when
   its argument has no citation `refs/src/<path>:<lines>` or `docs/<path>:<lines>` whose file exists and has
   those lines. `seal.py` then counts it as sealed and prints the reason. A refuted claim's void ratification is
   unchanged. The drive's `drive-citation` (b6 without its citations) shows SEALED with the note; a missing file,
   or line 900000 of an existing file, is refused. b6 with its real citations ratifies, and so does a ratified
   claim with the tail "nothing said" and no argument. Red (`u2-red.log`): 2 failures (`drive-citation ratified`;
   a missing file ratified). Green (`u2-green.log`): 3 OK.

## What changed in the committed claims, seals and page

- Claims: 73 files. 69 fixtures got new `places`: R added to most, operand primes added, and three primes that
  were only moduli of a value operand removed. Those three are 3 of fx-drv-ball-ops (`exp_at {p=3: 4} with 2`),
  3 of fx-f-cross (`[5 mod 6] ... with 5`) and 5 of fx-trig-real (`sin_at (* ; 0 mod 25) with real`). Five of
  these 69 also got `"branch": "all"`. a6 got its new statement and argument. b5, b5-1 and b5-2 got
  `"branch": "all"`.
- Seals: 72 files rewritten, `sealed again (the claim changed)`, because a claim file's content hash changed.
  Compared with the committed seals there are 0 differences in `actual`, `exit`, `verdict`, `run_verdict`,
  `script_line` and `reopened`; only the claim hashes and dates differ. The 72 earlier seals moved to
  `ux/ledger/seals/history/` as the format says. That directory is new and untracked. a6 has no seal.
- Page: `ux/ledger/ledger.html` is regenerated, 873531 bytes before and 942115 after.
- Code: `ledger.py` (status lines, places, branch, citations), `seal.py` (branch refusal, ratification count),
  `import_fixtures.py` (places and branch through ledger.py), `render.py` (computed against named boxes),
  `FORMAT.md` (the `branch` field, status lines, ratification, a section "The footprint"), `README.md`, and
  `test_ledger.py`. New: `probes/flint-equality.c`.
- Two of the builder's tests were adapted to the new rules. Neither was weakened. `t-open` calls `roots_at` and
  now carries `branch='all'`. `test_two_fixtures` now accepts `R` among the places and checks that `R` is first
  for fx-i-02-ucoset.

## Checks

- `timeout 600 make -j2 && timeout 600 make -C tools/adf`: exit 0.
- `timeout 600 python3 -B -m unittest discover -s ux/ledger -v` (`unittest-final.log`): 33 tests OK, 2.6 s.
  These are the builder's 16 and 17 new ones.
- Reproduction: import, seal and render run again after the last change. The sha256 of all 284 files under
  ux/ledger was the same before and after (`diff` empty). The README test makes the same check in its temporary
  root.
- `timeout 900 python3 -B lanes/ux-ledger-r1/mutants.py` (`mutants.log`): 12 bounded hand mutants of the changed
  code (the prefix rule, the item count, the bracket depth, named-as-computed, the place-operand index, R from a
  real coordinate, `*` as real, branch not required, seed not compared, sealer ignoring the refusal, any `refs/`
  string as a citation, line range not checked). Each ran in a temporary copy: 12 killed, 0 survived.
- `node --check` on the page's script: OK. Lines over 116 characters in the changed files: 0.
- Not run: the drive's own scripts `lanes/ux-drive-ledger/status-checks.py` and `extras.py`, because they write
  into that lane's directory, which this lane does not own. Their claims `drive-prefixed-open`,
  `drive-footprint`, `drive-branch` and `drive-citation` are rebuilt in the tests above. No fuzzing. No
  screenshot: Firefox was not tried, item 8 of the drive.

## Findings against the brief

1. `with <prime>` read literally takes degrees, seeds and precisions as places: `roots_at -1 with 5 with 2`
   would name 2. The slice uses the place operand of each command, from the driver README. For such a command,
   the moduli in its value operand are not places of the computation, which is why three fixtures lost a prime.
   "Computed" means that a line which ran names the place. This is weaker than "evaluated there": for example,
   `type [p=5: ...]` names 5.
2. The drive's count of 20 imported scripts with `with real` includes tests/driver/i-01-show-type.cmd, where
   `with real` is only in a comment. 19 scripts have it on a command line, the builder's number.
3. D3 cannot change the state of an imported fixture whose lines equal its .out. Only the class tags of the
   cards change, as reported under item 3.
4. U2: the slice also requires the cited file to exist and to have the cited lines. The brief asks only for the
   form. A citation in another form, for example `refs/...` without `src/`, is not a source.
5. U3: `"all"` is not checked against the branches the lines actually cover. A claim with one `root_at` line and
   `"all"` seals. The prose of the statement is not checked either. A seed is checked against the script.
6. Places of 1000 or more with no factor below 1000 are shown as `cofactor n`, as the builder did, although every
   such n below 10^6 is prime (fx-f-places-1000).

## What is left

- Items 7 and 8 of the drive, the next slice: ratify, comment and export a referee report (U1), and Firefox
  screenshots at desktop and phone widths.
- U4 (no claim bar) and U5 (no review queue order) are unchanged. The library demands (D-UX3) are as the
  builder's report lists them.
