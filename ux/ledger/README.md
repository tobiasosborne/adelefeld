# ux/ledger: the Ledger, where agents propose, the library seals and people ratify

The first slice of `docs/ux/brief-3-claim-ledger.md`. A claim is a file: a statement, driver lines, the exact
lines they must print, and the places it speaks about (`FORMAT.md`). `seal.py` runs the driver `build/adf` on
each claim and writes a seal; `render.py` writes one static page of all claims. Python 3 standard library only.

## Build and run (from the repository root)

    timeout 600 make -j2 && timeout 600 make -C tools/adf     the library and the driver build/adf
    python3 ux/ledger/import_fixtures.py       tests/driver/*.cmd with their .out as claims fx-NAME (86)
    python3 ux/ledger/seal.py                  seal every claim; exit 0 only when every claim is sealed
    python3 ux/ledger/seal.py --only a1-cos-4-at-2      seal one claim
    python3 ux/ledger/seal.py --dry-run        print the driver lines and expected lines, run nothing
    python3 ux/ledger/render.py                write ux/ledger/ledger.html
    firefox ux/ledger/ledger.html              open the page (file://, no server)
    timeout 300 python3 -m unittest discover -s ux/ledger -v       the unit tests (16)

`seal.py` prints one line per claim (its state and what happened: `first seal`, `unchanged`, `REOPENED`) and a
summary with the counts and the run time. On this ledger it exits 1, because four claims are refuted, one is
open and one is proposed on purpose. A reseal with the same results leaves every seal file as it is, and the
page is a function of the files, so running the three commands again changes nothing (`git diff` is empty).

## The files

- `claims/<id>.json`: the claims. `fx-*` are the 86 imported fixtures; `a1` to `a6` and `b1` to `b6` (with
  `b5-1`, `b5-2`) are the claims of scenarios A and B of the brief, written by hand.
- `seals/<id>.json`: the seal of a claim (verdict, actual lines, date in UTC, build); `seals/history/`: earlier
  seals. The build is the commit (`git rev-parse HEAD`), the sha256 of `build/adf`, FLINT_VERSION read from the
  `flint/flint.h` the build includes (`/usr/include/flint/flint.h` here), and the libflint file that
  `ldd build/adf` resolves. Two builds are the same build when the sha256 and FLINT agree.
- `FORMAT.md`: the fields and the five states. `oracle.py`: exp at a prime in exact rationals, the source of
  the expected centres 958, 4, 13, 22 of claims a3 and a5 (`python3 ux/ledger/oracle.py 3 3 8`).

## The page

- Top: the counts per state. Buttons filter by state; the select filters by question (fixtures, A, B).
- The list: the footprint first (`R 2 3 5 7`, then larger primes, `cof` for a cofactor not split below 1000,
  then the tail: hatched `all` = argued for every other place, dashed `...` = nothing said), then the state,
  the claim (id above the statement), the proposer. A filled box was computed; an outlined box is named by a
  claim that has no valid seal; a pale box is not named.
- States have shapes and colours: proposed (empty ring, grey), sealed (filled disc, green), refuted (ring
  struck through, red), open (dashed ring, amber, with the library status), ratified (disc with a tick).
- Click a claim: its card opens on the right (below on a phone; "back to the list" returns). The card has the
  statement, the script with line numbers, then a table: script line, expected line, the line the library
  wrote, and a mark (`=` agree, a crossed `=` contradicts, `?` open). A library status carries its class shape:
  dashed = open, solid = proved no, dotted = budget (`LIMIT`), underlined = not mathematics (`PARSE`,
  `UNSUPPORTED`); the footer lists every status name per class. Under a contradicting local line `p: c +
  O(p^N)` the card shows both centres in base-p digits and the first digit that differs. Then the footprint,
  the argument (blue italic: argued, not computed), the seal with the build, and the earlier seals.

## Scenario A: refereeing a table of 2-adic and 3-adic values

Choose the question "Scenario A". a1 (cos 4 = 1 mod 16 at 2) is refuted: the library wrote `2: 9 + O(2^4)`, and
the digits `...0001` against `...1001` differ at 2^3. a2 (v_2(log 3) = 2) and a4 (sin 3 = 3 mod 9) are sealed.
a3 (exp 3 = exp 12 mod 3^8) is refuted on its second line; its children a5 (agree mod 9, not mod 27: sealed) and
a6 (FLINT's equality test ignores precision: no driver line can say it, so it stays proposed) explain it.

## Scenario B: the overnight batch on powers and logarithms

Choose "Scenario B". Sealed: b1 (exp(Log 5) = 1 at 5, as two lines), b2 (the product formula for 6/35; its last
line was promoted from the driver's output, see its argument), b5-1, b5-2, b6 (the family: 2 is a square in Q_p
iff p = 1, 7 mod 8, computed for the 24 odd primes to 97, argued with sources for the rest). Refuted: b3 (the
deliberately wrong exp(Log 5) = 5) and b4 (x^2 + 1 has a root in Z_2; the library says `none`). Open: b5 (the
roots of 1 + 4 Z_2), `NOT_DETERMINED`, with its two sealed children.

## Write a claim, and see a reopening

Copy a claim file to `claims/<new id>.json`, change `id`, `statement`, `script`, `expect`, `places`; then
`python3 ux/ledger/seal.py --only <new id>` and `python3 ux/ledger/render.py`. To see a reopening without
touching the ledger: `cp -r ux/ledger /tmp/L`, edit `/tmp/L/seals/a2-v2-log-3.json` (another `adf_sha256` and
another first line of `actual`), run `python3 ux/ledger/seal.py --ledger /tmp/L` and
`python3 ux/ledger/render.py --ledger /tmp/L`: a2 is open with a REOPENED notice naming both builds.
Ratification is a state of the format only: no command of the slice sets it.
