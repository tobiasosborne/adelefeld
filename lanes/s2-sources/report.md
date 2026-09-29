# Lane s2-sources: FLINT 3.0.1 documentation for roots modulo a prime of any size

Date: 2026-09-29. No code, no build, no test run of the project. Every claim below is either a quote from a
file under `refs/src/` with file and line, or a statement about such a file that can be checked with the command
given.

## 1. What was done

1. Found how the 22 files of `refs/src/flint-3.0.1/` were fetched: `refs/fetch_sources.sh`, two loops over
   `doc/source/<m>.rst` at the tag v3.0.1, URL
   `https://raw.githubusercontent.com/flintlib/flint/v3.0.1/doc/source/$m.rst`, the first loop at the comment
   "FLINT 3.0.1 documentation (.rst)" (line 67 to 70) with 13 modules, the second in the milestone S section
   (line 128 to 130) with 8 modules; `fmpq.rst` is in the first list, so 13 + 8 = 21 names and 22 files with
   `flint.rst` and `memory.rst` counted once. The tag and the directory are those of the brief.
2. Added a third loop to `refs/fetch_sources.sh`, in the same form, same tag, same directory, with
   `fmpz_mod_poly fmpz_mod_poly_factor fmpz_mod nmod_poly nmod_poly_factor`, and ran the script.
3. Added the four new checksums to `refs/manifest.sha256` (the script rewrites the whole manifest; the diff is
   four added lines and no changed line).
4. Read the five files on disk and answered the questions of the brief as 15 rows of table 3, section S.2, of
   `docs/sources.md`, plus 4 rows of table 1 for the new files, plus one item (number 4) in the list
   "Sources pending for milestone S".
5. Every one of the 15 quotes was checked mechanically against the files (section 4).

## 2. The files fetched

`./refs/fetch_sources.sh` (run from the repository root). The network answered at once; one run, no retry.
Tail of the output: `== manifest.sha256: 2785 files`, `== manifest-extra.sha256: 70 files`, real time 1842 s [orchestrator: the lane ran about 6 minutes in all, so this figure is false]
(the time is the `git clone`/`git archive` step of the Sage package, which the script repeats on every run).

| file under `refs/src/flint-3.0.1/` | bytes | sha256 | new |
|---|---|---|---|
| fmpz_mod_poly.rst | 94766 | e51f4d80c8c7d42d7c9ee3f6b4c1e67f149cf5b112bb7db56f1ffd0d3e3a2b72 | yes |
| fmpz_mod_poly_factor.rst | 9867 | 0f81560d9263a135b0e40c1a0eb5ed5d2bba2664e6afab73bf4419e1c2fdc27a | yes |
| nmod_poly.rst | 116652 | 2dd8bae5ae92f1762641f348e8edfaa2049271ad526609e13509524bc6cedafd | yes |
| nmod_poly_factor.rst | 8519 | 59aa4c4c74f084051e0a74b7e99955480f3c0f065d7e94ea4770dadea6a60348 | yes |
| fmpz_mod.rst | 5989 | dd75c63b7daa022f781b0ccbe2cf5ec6568a134622a65258b86bd08b9641ef8b | no, fetched 2026-09-27 by the first loop; hash unchanged |

All four are from the tag v3.0.1 of the FLINT repository, directory `doc/source/`, the same URL form as the 22
files already there. `refs/src/flint-3.0.1/` now holds 26 `.rst` files.

## 3. The files written

- `refs/fetch_sources.sh`: 5 lines added (a comment of 2 lines and a loop of 3), nothing else changed.
- `refs/manifest.sha256`: 4 lines added, 0 changed, 0 removed.
- `docs/sources.md`: 31 lines added, 0 changed, 0 removed (verified with
  `diff <(git show HEAD:docs/sources.md) docs/sources.md`): 4 rows of table 1 (before the `flint-src-3.0.1`
  rows), 15 rows of table 3 under `### S.2 roots` (lines 343 to 357), and item 4 of the list "Sources pending
  for milestone S" (12 lines).
- `lanes/s2-sources/notes.md`, `lanes/s2-sources/check_quotes.py`, this report.

## 4. Checks run, with command and result

1. `curl -sS -o /dev/null -w '%{http_code}\n' --max-time 30 https://raw.githubusercontent.com/flintlib/flint/v3.0.1/doc/source/fmpz_mod_poly.rst`
   -> `200`.
2. `diff /tmp/manifest.before refs/manifest.sha256` (the manifest before the run and after) -> 4 added lines
   (the four hashes above), nothing else.
3. `cd refs && sha256sum -c manifest.sha256` -> exit 0, 2785 lines `: OK`, no other line.
4. For each of the four new files, the hash in `refs/manifest.sha256` compared with
   `sha256sum refs/src/flint-3.0.1/<file>`: equal in all four cases; each hash occurs exactly once in
   `docs/sources.md` (the new table 1 row).
5. `python3 lanes/s2-sources/check_quotes.py docs/sources.md 343 ... 357`: 15 rows, 0 mismatches. The script
   rebuilds each quote from the cited lines of the cited file, leading indentation dropped, joined with " / ",
   and compares with the cell. The row 342 of the file (the `padic_poly` row of an earlier lane) is reported as
   a mismatch by this script because that row's quote drops the trailing spaces of two lines of the extraction;
   that row is not mine and was not touched.
6. `ls *.rst | wc -l` in `refs/src/flint-3.0.1/` -> 26.
7. `grep -rl 'nmod_poly_roots' refs/src/flint-3.0.1/` -> 0 files.
8. `grep -c '^\.\. function::' refs/src/flint-3.0.1/nmod_poly_factor.rst` -> 28; the last is at line 197
   (`_nmod_poly_interval_poly_worker`); `wc -l` of the file -> 201.
9. `sed -n '128,135p' /usr/include/flint/nmod_poly_factor.h` -> `void nmod_poly_roots(...)` at line 130 and
   `int nmod_poly_roots_factored(...)` at line 133. That header is a local reference, recorded in
   `docs/sources.md` after table 1, not a fetched file; `flint.h` lines 94 to 96 give 3, 0, 1.
10. `grep -n 'S-D10' docs/SPEC.md` -> line 895, under `### 15.3 Decisions of TJO, 2026-09-29 (milestone S)`
    (line 878). The decision names the route of `solvers` P3.7(2), so the reference in the pending item is right.
11. `awk 'length($0)>116 && $0 !~ /^\|/' docs/sources.md` -> no new long prose line; every line over 116
    characters that I added is a table row, which the document exempts.
12. `git status --short` -> `M docs/sources.md`, `M refs/fetch_sources.sh`, `M refs/manifest.sha256`, and the
    untracked files of this lane. No other file of the tree is touched. No git command that changes state and no
    `bd` was run.

## 5. The questions of item 2, answered

### 5.1 `X^p` modulo `g` over `F_p`, `p` an `fmpz`

`fmpz_mod_poly_powmod_x_fmpz_preinv` (`fmpz_mod_poly.rst:690`). Documented: `e >= 0` (`:694`), `finv` the inverse
of the reverse of `f` (`:694`). The text of that entry stops in the middle of a word: line 695 is a lone
`` `` ``, so the sentence that would follow (and with it any further requirement) is not in the file on disk.
The degree requirement is stated only for the raw variant `_fmpz_mod_poly_powmod_x_fmpz_preinv`: `lenf > 2`
(`:687`), that is `deg f >= 2`, and the output must have room for `lenf - 1` coefficients (`:688`). The base is
not an argument of this routine; the base is `x` itself, of degree 1. The general statement for the binexp
variants is that the base is "already reduced modulo ``f`` and zero-padded as necessary to have length exactly
``lenf - 1``" (`fmpz_mod_poly.rst:653`), which is the condition `deg base < deg f`. A nonzero modulus: not said
in the powmod entries. A prime modulus: not said. There is no `fmpz_mod_poly_powmod_x_fmpz` without `_preinv`.

### 5.2 gcd of two polynomials over `F_p`, and a modulus that is not prime

`fmpz_mod_poly_gcd` (`fmpz_mod_poly.rst:1030`). It assumes `p` prime, and says why: the gcd is defined in
`(Z/(p Z))[X]` if and only if `p` is a prime number (`:1034` to `:1036`). Is the result monic: NOT SAID for this
function. In the same module monicity is stated only for the xgcd: `fmpz_mod_poly_xgcd` makes the gcd monic
except when it is zero (`:1131` to `:1134`), and `_fmpz_mod_poly_xgcd` makes no attempt (`:1120`). So
`docs/proofs/solvers.md` P3.7(2), which reads the number of roots off `deg gcd(g, X^p - X)`, may read the degree
of whatever the routine returns; the normalisation of that polynomial is not documented for `fmpz_mod_poly_gcd`.

### 5.3 roots of a polynomial over `F_p`

`fmpz_mod_poly_roots` (`fmpz_mod_poly_factor.rst:187`). Requires: `f` nonzero, it throws otherwise (`:192`).
Requires a prime modulus: it "is expected and not checked" (`:190`). Requires squarefree: not asked. Requires
monic: not asked. Promises: all the DISTINCT roots of `f` in `Z/pZ` (`:189`), each as a factor `x - r_i`, and
the exponent is the multiplicity when `with_multiplicity` is set (`:191`), otherwise 1. Randomised: NOT SAID. A
composite modulus: nothing said beyond `:190`; the sibling routine for a composite modulus is
`fmpz_mod_poly_roots_factored` (`:194`), which expects and does not check a prime factorisation of the modulus
(`:197`), lifts and combines (`:199`) and may fail, "possibly because there are too many of them" (`:200`).
For the same ring the documentation also offers `fmpz_mod_poly_factor` (`:152`, into monic irreducible factors,
no condition on the modulus) and `fmpz_mod_poly_is_squarefree` (`:107`, no condition on the modulus).

### 5.4 the same three questions for `nmod_poly`, one-word `p`

- `X^p` mod `g`: `nmod_poly_powmod_x_ui_preinv` (`nmod_poly.rst:920`, `e` is the word `p`), same two
  requirements, `e >= 0` and `finv` (`:923` to `:925`); the raw variant states `lenf > 2` (`:917`).
  `nmod_poly_powmod_x_fmpz_preinv` (`:936`) is the same with an `fmpz` exponent; its raw variant states
  `lenf > 2` (`:933`). Nothing said about the modulus being prime or nonzero.
- gcd: `nmod_poly_gcd` (`nmod_poly.rst:1716`). The result is made monic except when the gcd is zero (`:1721`).
  The modulus: NOT SAID, and in particular no primality is assumed, unlike `fmpz_mod_poly_gcd`. The raw variant
  `_nmod_poly_gcd` makes no attempt at monicity (`:1713`).
- roots: THERE IS NO DOCUMENTED ROOT FINDING for `nmod_poly`. The only root routine documented in
  `nmod_poly.rst` is `nmod_poly_find_distinct_nonzero_roots` (`:2392`), which returns 1 only when `A` has
  `deg(A)` distinct NONZERO roots (`:2394`), so it never returns the root 0; it returns 0 without saying
  anything when there are fewer; it is the probabilistic method of Rabin (`:2396`) and it assumes a prime
  modulus (`:2395`). The module `nmod_poly_factor` has no root-finding section at all: 28 entries, the last at
  line 197, the file has 201 lines, and no fetched `.rst` file of `flint-3.0.1` mentions `nmod_poly_roots`. The
  installed header declares it (`/usr/include/flint/nmod_poly_factor.h:130`). `[source pending: the
  documentation of nmod_poly_roots of FLINT 3.0.1]`.

### 5.5 Does any of these functions test that `p` is prime?

No. No function of `fmpz_mod_poly`, `fmpz_mod_poly_factor`, `nmod_poly` or `nmod_poly_factor` is documented as
testing the primality of the modulus. `fmpz_mod_poly_roots` says the opposite: "expected and not checked"
(`fmpz_mod_poly_factor.rst:190`). `fmpz_mod_ctx_init` only expects the modulus to be positive and says nothing
about a prime (`fmpz_mod.rst:22`). The one place where the documentation reports something about a composite
modulus is `fmpz_mod_poly_is_irreducible_rabin_f` (`:85`), which either answers the irreducibility question "even
for composite `f`, or it finds a factor of `p`" (`:91` to `:93`) or returns a nontrivial factor of the modulus;
the `_f` variants of `is_squarefree` and `xgcd` are documented the same way. That is a way of learning that the
modulus is composite, not a primality test. A design that promises every prime (decision S-D10) has to establish
primality itself.

## 6. What is not done

- No code, no build, no test of the project; that was not in this lane.
- The C sources at the tag v3.0.1 of `fmpz_mod_poly/powmod_x_fmpz_preinv.c`, `fmpz_mod_poly/gcd.c`,
  `fmpz_mod_poly_factor/roots.c` and `nmod_poly_factor/roots.c` were NOT fetched. The brief asked for the
  documentation; the four items above are what the documentation cannot answer (randomness, monicity of the gcd,
  behaviour on a composite modulus, and the body of `nmod_poly_roots`). They are listed as `[source pending: ...]`
  in the new item 4 of "Sources pending for milestone S".
- I did not add a numbered item to the "Notes for the design of milestone S", because that list is prose and the
  brief limits me to new rows. The differences that matter for the design are inside the statement cells of the 15
  rows.
- I did not touch the `[source pending: flint-3.0.1 fmpz_mod_poly.rst, fmpz_mod_poly_factor.rst, nmod_poly.rst
  (for nmod_poly_powmod, nmod_poly_gcd and the root finding)]` at `docs/proofs/solvers.md:1308`; that file is not
  mine. It can now be shortened: the three files are on disk and the rows above say what they settle. Note one
  difference: the pending line names `nmod_poly_powmod`, but the documented entries are
  `nmod_poly_powmod_x_ui_preinv` and `nmod_poly_powmod_x_fmpz_preinv`; `nmod_poly_powmod` itself does not occur
  in `nmod_poly.rst`.

## 7. Sources pending

1. `[source pending: the documentation of nmod_poly_roots of FLINT 3.0.1. The installed header
   /usr/include/flint/nmod_poly_factor.h:130 declares it and :133 declares nmod_poly_roots_factored, but the 26
   fetched .rst files of flint-3.0.1 do not document either: the module nmod_poly_factor ends at its worker
   entry (line 197) and has no root-finding section]`
2. `[source pending: the C sources at the tag v3.0.1 of fmpz_mod_poly/powmod_x_fmpz_preinv.c,
   fmpz_mod_poly/gcd.c, fmpz_mod_poly_factor/roots.c, nmod_poly_factor/roots.c, for: whether
   fmpz_mod_poly_roots is randomised, whether fmpz_mod_poly_gcd returns a monic polynomial, what these routines
   do for a composite modulus, and the body of nmod_poly_roots]`
3. Nothing is pending for the answers to the four questions of item 2 themselves: every answer above is a quote
   from a file on disk with file and line, or the words "not said" with the grep that shows the silence.

## 8. Findings against the specification

1. `docs/proofs/solvers.md:1308` reads `fmpz_mod_poly.rst, fmpz_mod_poly_factor.rst, nmod_poly.rst` "for
   nmod_poly_powmod, nmod_poly_gcd and the root finding". Two of the three names are right;
   `nmod_poly_powmod` is not a function of the documentation (the entries are `nmod_poly_powmod_x_ui_preinv`
   and `nmod_poly_powmod_x_fmpz_preinv`, `nmod_poly.rst:920` and `:936`). The root finding of `nmod_poly` is
   not documented at all, so for that module the pending line cannot be closed by documentation alone; the C
   source `nmod_poly_factor/roots.c` or the upstream repository would be needed.
2. The entry of `fmpz_mod_poly_powmod_x_fmpz_preinv` in the FLINT 3.0.1 documentation is truncated: line 695 of
   `fmpz_mod_poly.rst` is a lone `` `` `` and the sentence after it is missing. This is a defect of the upstream
   file, not of the fetch (the hash is in the manifest and `sha256sum -c` passes). A design that reads only the
   public entry of that routine would take the degree requirement from the raw variant, which the file does
   state (`lenf > 2`).
3. Decision S-D10 (SPEC 15.3, line 895) says the later slice adds "the route of `solvers` P3.7(2) (roots by a
   routine of FLINT, each tested by evaluation, completeness by `deg gcd(g, X^p - X)`) for larger primes, with
   no bound of one word". Two of the three pieces are documented on disk and are as stated: the routine
   (`fmpz_mod_poly_roots`) and the gcd (`fmpz_mod_poly_gcd`, which assumes the prime). The third, the degree of
   that gcd, is not affected by the missing monicity statement, since a nonzero scalar multiple has the same
   degree; the point to note is only that the documentation does not promise monicity. Nothing here contradicts
   the decision.
4. Item 3 of the list "Sources pending for milestone S" in `docs/sources.md` says "Nothing is pending for S.2". I
   did not change that line, which belongs to another lane. My new item 4 scopes itself to the modular part of
   S.2 and says so in its first sentence. If the reader takes item 3 as a claim about all of S.2, the two items
   read as a contradiction; the owner of that list may want to reword item 3. I could not do it, the brief gives
   me new rows only.
