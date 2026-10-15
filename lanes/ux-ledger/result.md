# Lane ux-ledger: result (Claude Opus, 2026-10-16)

## What was built

The first slice of brief 3, the Ledger, in `ux/ledger/`, in the order of `lanes/ux-ledger/brief.md`, test first:

1. The claim format `claims/<id>.json` and `FORMAT.md` (51 lines): the fields of the brief, plus four optional
   ones (`expect_exit`, `question`, `parent`, `source`), the seal fields, and the five states.
2. The sealer `seal.py`: one `adf -v` process per claim, timeout 60 s; exact comparison; verdicts sealed,
   refuted, open; the build identity (commit, sha256 of `build/adf`, `FLINT_VERSION` from
   `/usr/include/flint/flint.h`, and the libflint file that `ldd build/adf` resolves: both are recorded);
   the sha256 of the claim file. When another build, or the same build, writes other lines for an unchanged
   claim, the earlier seal goes to `seals/history/` and the claim is `open`, with both builds named.
   `--only ID` and `--dry-run`. Exit 0 when every claim is sealed, 1 otherwise, 2 for a fault of the ledger.
3. The renderer `render.py`: one static page, `ledger.html` (873 KB). It has the list (footprint first, then the
   state with its shape and colour, the id and statement, the proposer), filters by state and by question
   (inline JavaScript), and one card per claim. A card shows the statement, the script, and a table of script
   line, expected line, library line and mark. Library statuses carry the D-UX4 class shapes; a contradicting
   `p: c + O(p^N)` line gets its base-p digit difference. Then come the footprint, the argument, the seal with
   its build, the earlier seals, the parent and children, and a loud REOPENED notice when there is one.
4. The first ledger: 86 claims imported by `import_fixtures.py` (every `tests/driver/*.cmd` with its `.out`),
   and 14 claims of scenarios A and B written by hand.
5. `README.md` (73 lines, D-UX6) and `test_ledger.py` (16 tests). Also `oracle.py`: exp at a prime in exact
   rationals. It gives the expected centres of claims a3 and a5, so that no expected line was copied from the
   driver.

Decisions taken and recorded (`FORMAT.md`):

- The ids are `fx-<fixture>`, then `a<k>-<slug>` and `b<k>-<slug>`; a child of b5 is `b5-1`.
- A family is one claim with one script line per member; `places` lists the members and `tail` says whether
  the rest is argued (b6).
- Open or budget statuses (including `LIMIT` and the timeout) where something else was expected give `open`,
  never `refuted`. Brief 3 puts `LIMIT` among the statuses of an open claim, while D-UX4 gives it its own class.
- `PARSE` and `UNSUPPORTED` where something else was expected give `refuted`; the card marks them "not
  mathematics".
- A later build that writes the same lines leaves the seal untouched. So a seal names the first build that gave
  its lines, and the seal files reproduce under any build that agrees.
- The seal also stores the hash of the claim without `state`, so that ratification (a person editing `state`)
  does not make a seal stale.

## Files (all new)

- `ux/ledger/`: `FORMAT.md`, `README.md`, `ledger.py` (shared), `seal.py`, `render.py`, `import_fixtures.py`,
  `oracle.py`, `test_ledger.py`, `.gitignore` (`__pycache__/`), `ledger.html`.
- `ux/ledger/claims/`: 100 files. `ux/ledger/seals/`: 99 files (a6 has no script, so no seal), and
  `seals/history/` is empty.
- `lanes/ux-ledger/`: `result.md`, and `shot.png` (334 KB, the a3 card at 1400 px; leave it out if too large).
- Sizes: claims 888 KB, seals 376 KB, page 873 KB.

## Checks (commands from the worktree root)

- Build: `timeout 600 make -j2` and `timeout 600 make -C tools/adf` both exit 0.
- Unit tests: `timeout 300 python3 -m unittest discover -s ux/ledger -v` runs 16 tests, OK, in 0.30 s.
- Import: `python3 ux/ledger/import_fixtures.py` writes 86 claims.
- Seal: `python3 ux/ledger/seal.py` gives 100 claims: 94 sealed, 4 refuted, 1 open, 1 proposed, 0 ratified;
  exit 1. The fixtures alone, at their first seal: 86 sealed, exit 0, 0.6 s.
- Sealer run time: 0.4 to 0.7 s as the sealer measures it, 0.68 to 0.80 s wall (`time`), for 100 driver runs.
- Reproduce: import, seal and render again, then `diff -r` against saved copies. Claims, seals and
  `ledger.html` were identical in 3 rounds; 99 seals "unchanged".
- Fresh shell: the README's commands under `env -i HOME=$HOME PATH=/usr/bin:/bin bash -lc '...'` all ran, and
  the files were identical afterwards.
- Page script: `node --check` on the page's script passes.
- Mutation testing, bounded: 10 hand mutants of `ledger.py`, `seal.py` and `import_fixtures.py`. 7 were killed
  at first; 2 survivors led to 2 new tests and were then killed; 1 is equivalent (`n > 1` against `n > 1000`
  after trial division below 1000).
- Reopening walk: the README recipe on a copy (another `adf_sha256`, another first line). a2 becomes open; the
  REOPENED notice names both builds and the first differing line.
- Screenshots: `firefox --headless --screenshot` at 1400 px and 420 px are readable. The narrow layout stacks
  the rows, and the card sits below the list.
- Line lengths: `awk 'length > 116'` over `ux/ledger/*.py` and `*.md` finds 0 lines.

The states of the 14 hand claims, as designed:

- a1 (cos 4 = 1 mod 16 at 2) is refuted: the library writes `2: 9 + O(2^4)`, base-2 digits `...0001` against
  `...1001`, first difference at 2^3.
- a2 (v_2(log 3) = 2, as `2: 4 + O(2^3)`) is sealed.
- a3 (exp 3 = exp 12 mod 3^8) is refuted on line 2: 958 against 5125, base-3 first difference at 3^2.
- a4 (sin 3 = 3 mod 9) is sealed.
- a5 (agree mod 9, not mod 27; centres 4, 4, 13, 22) is sealed.
- a6 (FLINT's equality ignores precision) is proposed: no driver line can say it.
- b1 (exp(Log 5) = 1, as `5: 0` and `5: 1`) is sealed.
- b2 (the product formula for 6/35) is sealed.
- b3 (exp(Log 5) = 5, a deliberately wrong expectation, named so in its statement) is refuted.
- b4 (x^2 + 1 has a root in Z_2; the library writes `none`) is refuted.
- b5 (the roots of 1 + 4 Z_2 to 2 digits) is open, `NOT_DETERMINED`, with its two children.
- b5-1 and b5-2 are sealed.
- b6 (2 is a square in Q_p iff p = 1, 7 mod 8, for the 24 odd primes to 97, argued beyond them with
  `refs/src/shoup-ntb/ntb-v2.txt:15017,15032-15034` and `refs/src/conrad-hensel/hensel.txt:160-169`) is
  sealed.

## Walk of the scenarios with the page open (what a referee sees)

Scenario A:

1. Choose the question "Scenario A". Six rows: two red struck rings (a1, a3), three green discs (a2, a4, a5),
   one grey ring (a6). Each footprint has one box at 2 or at 3, filled (outlined for a6, named but not
   computed), and a dashed tail ("nothing said").
2. Open a1. The table shows `cos_at 4 with 2`, expected `2: 1 + O(2^4)`, library `2: 9 + O(2^4)` with a
   crossed `=`. Under it: "base-2 digits, lowest 4: expected ...0001, library ...1001; the digit of 2^3 is the
   first that differs". The argument cites `docs/SPEC.md:587` (cos 4 = 9 mod 16).
3. Open a3. Line 1 agrees (958); line 2 is refuted (5125), with the digit difference at 3^2. "Children" links
   a5 (sealed) and a6 (proposed).
4. a5 shows four agreeing lines: equal at prec 2, 13 and 22 at prec 3. a6 says it has no script.
5. Not possible in the slice: ratifying, writing a comment on a refuted claim, exporting a referee report.

Scenario B:

1. Choose "Scenario B": eight rows.
2. b1 is sealed (two lines, `5: 0` and `5: 1`). b3 is refuted on line 2: `5: 5` expected, `5: 1` written; it
   has no digit line because the values are exact.
3. b4 is refuted: `1 mod 2^8` expected, `none` written.
4. b5 is open: an amber dashed ring "OPEN · NOT_DETERMINED"; the library cell has a dashed `NOT_DETERMINED`
   tag; "Children: b5-1 (sealed), b5-2 (sealed)"; "Failing place ...: not given". b5-2 expects
   `error: DOMAIN`, which carries the solid "proved no" tag, and agrees.
5. b2's footprint is the filled R 2 3 5 7 11 with a hatched `all` tail; its argument says, in capitals, that
   the last line was promoted from the driver's output.
6. b6's footprint is R 2 3 5 7 (3, 5, 7 filled), then a wide box `11 ... 97` and a hatched tail; 24 agreeing
   lines.

## Library demands of brief 3 (D-UX3), met and unmet by the slice

1. *A batch runner with records.* Partly met. The driver gives one line per command, and `adf -v` gives the
   script line and the status name of every command, which the sealer uses to tie output lines to script
   lines. In 1 of 100 claims (`fx-s-hostile`) the trace does not match the output and the card falls back to
   index order. Unmet: the kind, the dump, the failing place (the card says `not given`;
   tools/adf/README.md:253) and the precision per place.
2. *Determinism and a provenance stamp.* Determinism is observed: 100 scripts, 4 seal rounds, the same lines.
   Unmet: no library or driver call returns version, FLINT and build as one string, so the slice puts its
   own stamp together (commit, sha256 of `build/adf`, `FLINT_VERSION` of the header, the libflint file).
3. *Certificates with independent checkers.* Unmet: the driver prints none (no `adf_recon_cert_check`, no
   root certificate).
4. *Expectation predicates in the library.* Unmet beyond finite balls and unit cosets. "Contains 1" for a
   real ball (b2), "equal modulo 3^8" (a3) and "differ modulo 27" (a5) had to be written as exact lines.
5. *Dump forms for every type.* The driver's `dump` covers nine bodies, but a seal cannot hold the dump of a
   result: the driver has no variables and no record mode. Unmet in the slice.
6. *Stable status semantics.* Met as far as the slice can: the 11 names of `include/adelefeld/status.h` are
   classified, a test checks the list against the header, and a changed line reopens a claim.

## What the test driver should try

- The README's commands in order from a fresh shell. Then run the three commands again and check that nothing
  changed.
- The two scenarios with the filters, and the narrow layout at phone width.
- Write a new claim, seal it with `--only`, render.
- Claims meant to fail in other ways: a malformed line (`PARSE`, refuted), an oversized `prec` (`LIMIT`,
  open), a claim file with a missing field (the sealer exits 2 and names the fault).
- Ratify by hand: set `state` to `ratified` in a sealed claim, which should show the tick, and in a refuted
  claim, which should show "the ratification is void".
- Change a claim after its seal (the page shows it proposed until resealed). The reopening recipe of the README.
- Rebuild `build/adf` elsewhere and reseal. The slice expects "confirmed by build ..." and unchanged seal
  files; this is not verified, because only one build was run.

## Findings against the brief

1. UX-COMMON says the first line `#!exit N` is in the `.out`. It is in the `.cmd`: 81 of 86 `.cmd` files have
   one, no `.out` does, and `tests/test_driver.sh` reads it from the `.cmd`. The importer follows the files.
2. The footprint rule of item 4(a) (`p=<n>` and `mod <n>`) misses places given as operands. 29 of 86 imported
   claims get no place; 15 of these name a prime after ` with `. Up to 36 claims miss up to 52 such primes (a
   rough count: it also counts precisions and degrees that happen to be prime). 19 claims name `real`, and R
   is never in an imported footprint. I kept the rule as briefed.
3. Exact-line expectations force the claimant to know the printed text in advance. a3 and a5 needed centres
   computed beforehand (`oracle.py`). b2's real line had to be promoted from the driver's output, because the
   printed ball `1 +/- 8.4e-27` cannot be predicted and no predicate decides "contains 1".
4. The driver has no variables, so exp(Log 5) is written as two lines with the printed value retyped (b1, b3).
5. The idele of 6/35 cannot be typed with real coordinate 6/35, because a real part is a decimal
   (`error: PARSE`). `prec` is both the p-adic precision and the real bits: after `prec 8`, the real part
   prints as `0.17188 +/- 0.00099`.
6. a6 (FLINT's equality test) cannot be expressed: the driver never calls FLINT's padic functions. It stays
   proposed with an empty script.
7. 35 of 86 imported statements are not whole sentences: the fixture's first comment line breaks mid-sentence.
8. The brief counts "ten claims" for the two scenarios. The scenarios as written yield 13 (4 typed in A, the
   automatic claim, 6 in B, 2 children); I wrote 14, adding a6 as the note of A, step 4.
9. "Argued" (brief 3) is not a state of the slice. It appears only as the footprint tail; the argument of b6
   cites its sources, but nothing checks the citation.

## Not done; sources pending

- Not built, as the brief says: agents, a queue, ratification by a command, comments, the export of a referee
  report, the scratchpad.
- No source pending. Every formula used is cited by file and line, or proved in place: the convergence bound of
  `oracle.py`, the odd squares mod 8 (b5-2), and v_2 = 2 iff 4 mod 8 (a2).
- `shot.png` is optional and over 100 KB.
