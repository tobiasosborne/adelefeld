# Report of lane t-slice2: the value form of local balls and partial balls (`adf_lball`, `adf_sball`)

Worktree of `adelefeld`, branch of the lane `t-slice2` (a copy; the brief is `lanes/t-slice2/brief.md`). Nothing
was committed and no tracker was touched (no git command that changes state, no `bd`).

Files written:

- `include/adelefeld/text.h`: additions only. Two includes (`lball.h`, `sball.h`), the comment block of the
  four declarations (grammar, every status with the input that gives it, the output untouched on a status, who
  frees the string, the round trips), and six sentences in the header comment of the file. +103 lines.
- `src/text_local.c`: new, 1856 lines.
- `tests/test_text_local.c`: new, 1645 lines.
- `tests/julia/text_local.jl`: new, 163 lines.
- `tests/test_julia.sh`: the block that runs `tests/julia/text_local.jl` in its own process (+18 lines).
- `docs/api-1f.md`: one new section at the end, "Slice 1F.5-c" (+101 lines).

(`git diff --stat` for the three changed files: 222 insertions, 2 deletions in `docs/api-1f.md`,
`include/adelefeld/text.h` and `tests/test_julia.sh`; the two deletions are the last lines of the two comment
paragraphs that were extended.)

- `lanes/t-slice2/`: `assemble_text_local.py`, `text_local_tail.c`, `gen_vectors.py`,
  `vectors_local.jsonl`, `bite.py`, `redgreen.log`, `red1.out`, `green1.out`, `progress.md`,
  `check-all.out`, `san-build.out`, `san-run.out`, `bite-<name>.out`, `bite-<name>.build`, and this report.

`src/text_local.c` is written by `lanes/t-slice2/assemble_text_local.py`: the first 1008 lines are the static
machinery of `src/text.c` (49 functions and four structs, copied verbatim, each under a comment that names the
function and the line it comes from), and the last 849 lines are the reader and the printer of this lane, kept
in `lanes/t-slice2/text_local_tail.c`. The four public functions: `adf_lball_set_str` (line 1398),
`adf_lball_get_str` (1497), `adf_sball_set_str` (1558), `adf_sball_get_str` (1817).

## 1. What was built

`adf_lball_set_str` reads `[p=P: L]` with `L = rat ["+" "O" "(" uint ["^" sint] ")"]`, checks the seven stages of
conventions 8.5 in order (the grammar; `abs(N) <= max_prec` and `abs(N) <= ADF_LBALL_EXP_MAX` on the digit
string; `P < 2^64` on the digit string; then `DOMAIN` for a zero denominator, for a base inside `O(...)` that
is not `P`, and for a prime that is not prime), and stores the value with the constructors of `lball.h`
(`adf_lball_set_rat_ball`, statement L1, for the canonical centre of conventions 5.8; `adf_lball_set_rat` for an
exact value). `adf_lball_get_str` prints `[p=P: L]` with `L` the centre `q(p^v u)` and, for a ball, `+ O(p^N)`
with `N` in signed decimal.

`adf_sball_set_str` reads `{ E; E; ... }` with `E = inf: real`, `inf: (R) + (R)*i` or `p=P: L`, checks the
stages in order (the grammar of the whole text; the count of entries against `max_items`; the decimal
exponents of every real part against `max_exp10` and the exponent of every `O`-term against `max_prec`, for
every entry before any semantic check; `p < 2^64`; then `DOMAIN` for at most one `inf`, for a repeated prime,
for a prime that is not prime, for a base that is not the prime and for a zero denominator), builds the
components in the canonical order of places and commits only on `ADF_OK`. A complex `inf` entry is stored with
the tag `ADF_ARCH_COMPLEX`, which `sball.h` 5.9 provides. `adf_sball_get_str` prints the entries in the
canonical order, the archimedean one first, with the real balls by the printer of conventions 9.5 (no sign
condition applies to them).

The document of the slice is `docs/api-1f.md`, section "Slice 1F.5-c" (functions, decisions u-1 to u-8, the
statements L14 and L15, what is not done, findings).

## 2. Checks that were run

All in the foreground, every program under `timeout`. Times from `time`.

- `timeout 900 make -j2 BUILD=lanes/t-slice2/build lanes/t-slice2/build/test_text_local` (the red run): the
  test compiles with `-Wall -Wextra -Wpedantic -Werror` and the link fails with 35 `undefined reference` lines
  for the four functions (`lanes/t-slice2/red1.out`).
- `timeout 300 ./lanes/t-slice2/build/test_text_local` (green 1):
  `10 tests, 68852 checks, 0 failed checks, 0 failed tests`, 0.15 s.
- `timeout 300 python3 lanes/t-slice2/gen_vectors.py`: 2000 records, 1195 of them a status, 805 valid; of the
  valid ones 731 have a real part that the C reader reads exactly (the printed text must then be the text of
  the reference) and 74 a rounded one (159 of the valid ones have an archimedean entry at all).
- `timeout 300 ./lanes/t-slice2/build/test_text_local` (the final run):
  `10 tests, 68868 checks, 0 failed checks, 0 failed tests`.
- `timeout 900 sh tests/test_julia.sh`:
  `test_julia: passed (with LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10)`; the block of this lane runs
  `tests/julia/text_local.jl`, 47 checks and 47 passes.
- `sh lanes/m1-headers/check_headers.sh`: `check_headers: passed`.
- `timeout 900 sh tests/test_exports.sh`:
  `test_exports: passed: 428 of 428 declared functions are exported, 0 are not implemented yet, no exported name
  is undeclared, no variadic function`.
- `timeout 900 make -j2 SAN=1 BUILD=lanes/t-slice2/build-san lanes/t-slice2/build-san/test_text_local`:
  builds, 10 s.
- `ASAN_OPTIONS=detect_leaks=1 timeout 900 ./lanes/t-slice2/build-san/test_text_local`:
  `10 tests, 68868 checks, 0 failed checks, 0 failed tests`, no leak, exit 0. One leak of 48 bytes was found by
  this run (a call of `adf_sball_init` on a value that holds a component, in the test file) and repaired before
  the run above.
- `timeout 1800 make -j2 check-all` (the last run, 4 min 16 s):
  `check-all passed: make check, driver, exports, julia, mutate-selftest, memcheck-selftest`, with
  `check passed: all 75 test programs`, `test_driver: 49 cases, 100916 expected lines, all equal (SAN=0)` and
  0 `FAIL` lines in the whole log.
- `timeout 400 python3 lanes/t-slice2/bite.py <name>`, seven times: the fault table in section 4.

### What the tests are, and what would have made a case fail

- `every_row_of_the_golden_file_lball`: all 52 rows of `tests/golden/lball.tsv` (29 texts, 23 statuses). For
  every text: the status, and on a status the untouched sentinel (the exact 3 at 7); for every value the
  canonical predicate, the printed text against the reference character by character, the classification
  `ADF_TEXT_LBALL`, the round trip field by field, and the printed text as a fixed point. It fails if one
  character of one text differs, or if one status differs, or if the value changes on a failure.
- `lball_stored_fields`: the fields `p`, `u`, `v`, `N`, `exact` of 12 hand-computed cases (conventions 5.8).
  `1/3 + O(5^4)` has the centre 417 (`3 * 417 = 2 * 625 + 1`), `1/125 + O(5^-2)` has `v = -3, u = 1`, `1/8` at 2
  has `v = -3` and at 7 has `v = 0`, `1/10 + O(5)` has the centre 13/5.
- `every_row_of_the_golden_file_sball`: all 25 rows (11 texts, 14 statuses), entry by entry: the local entries
  against the reference character by character, the archimedean entry as a real ball (the stored ball contains
  the exact interval of the text, the printed interval contains it, and the two texts are equal when the reader
  read the exact interval), the complex entries as two real balls, the places, the round trip and the
  classification.
- `sball_the_complex_tag`, `sball_the_canonical_order`: the tag of a complex entry (with one and with several
  primes), the exact zero imaginary part of a real entry, `{}`, the order of places and `adf_sball_get_place`.
- `the_reference_on_generated_texts`: the 2000 records of `lanes/t-slice2/vectors_local.jsonl`, written by
  `lanes/t-slice2/gen_vectors.py` from `proto/text_grammar.py`. Each record carries the verdict of the
  reference, whether the real part is read exactly and the exact balls of the real parts. For the 1195 records
  with a status: the same status from C. For the 805 valid ones: the printed text against the text of the
  reference where the real part is exact, otherwise the stored ball against the exact interval of the text, the
  printed interval against it, every local entry against the reference, the printed text read back, the same
  places and the same components, and the printed text a fixed point where conventions 9.6 promises it. A
  disagreement of one status or of one local entry fails.
- `round_trips_of_generated_values`: 2000 local balls and 2000 partial balls built here with the constructors
  of `lball.h` and `sball.h` (six primes, exponents from -4 to 6, exact values and balls, zero to four places,
  the empty set of places, a dyadic archimedean point with few digits). For each: print, read back, the same
  fields (`adf_lball_identical`, `adf_lball_equal_set`, the archimedean ball read back exactly and contained),
  and the text as a fixed point.
- `hostile_input`: a prime of 100 digits (`UNSUPPORTED`), `2^64` and `2^64 + 1` (`UNSUPPORTED`), `2^64 - 1`
  (`DOMAIN`, not prime), an exponent of 100 digits positive and negative (`LIMIT`), `N` above
  `ADF_LBALL_EXP_MAX` (`LIMIT`, on the digit string), `N = ADF_LBALL_EXP_MAX` with a fractional centre
  (`LIMIT`, from the constructor) and with a small integer centre (`OK`, no power is needed), `[p=5: 3 + O(5^4)]`
  cut at every position (24 cuts), missing and wrong brackets, `s = NULL` with `len = 0`, the empty text,
  whitespace only, a NUL byte at every position of three valid texts (49 positions), every prefix of three
  valid texts (each refused or a valid text of its own, never a crash), 10000 entries against `max_items`
  (9999 gives `LIMIT`, the default gives `DOMAIN` for the repetition) and against `max_len`, `max_prec` and
  `max_exp10` boundaries.
- `the_order_of_the_stages`: two faults in one text at each of the stages 1 to 6, the item limit before the
  semantic checks, `prec` above `ADF_REAL_PREC_MAX` before the text is read, and a `prec` below 2 taken as 2.
- `printer_bounds_and_digits`: the centre of an exact value that does not fit (`NULL`, `*len = 0`), the same
  inside a partial ball (`NULL`), the ball around it (prints), decision M1-D6 on a real part (`NULL`),
  `digits = 4` and `digits = 20`.

## 3. Decisions (the alternatives are in `docs/api-1f.md`)

1. **The code lives in `src/text_local.c`, whose first 1008 lines are a copy of the static machinery of
   `src/text.c`.** That file is read-only for this lane, and the machinery (the cursor, the literals, the exact
   decimals, the enclosing ball of 9.5, the printer of 9.5) cannot be reached from outside. Alternatives: a
   shared `src/text_internal.h` (a new file, not in the list of paths of the lane); `#include "text.c"`; the two
   hidden functions of `src/text_idele.c` extended to the two new forms, which needs `src/text.c` anyway. The
   copy is written by `lanes/t-slice2/assemble_text_local.py`, so it is reproducible and never edited by hand.
2. **A complex `inf` entry is read and stored** with the tag `ADF_ARCH_COMPLEX` (`sball.h` 5.9; the golden file
   has `{inf: (1) + (2)*i}`; the predicate of 5.9 allows it). Alternative: `ADF_UNSUPPORTED`, as every arithmetic
   function of `sball.h` does on a COMPLEX tag; reading text is not arithmetic, and 9.2 gives the syntax of both
   tags.
3. **`prec > ADF_REAL_PREC_MAX` is `LIMIT`, decided from `prec` alone before the text is read**, as every function
   of `prec` of `sball.h` decides it, also when the text has no archimedean entry. Alternatives: no limit (as
   `adf_adele_set_str`); `LIMIT` at stage 4.
4. **Two limits of the library in addition to conventions 8.4**: `abs(N) > ADF_LBALL_EXP_MAX` (`LIMIT`, decided on
   the digit string after `max_prec`, so that `N` fits a `slong`), and a canonical centre that needs `p^k` with
   `k bits(p) > ADF_LBALL_BITS_MAX` (`LIMIT`, from `adf_lball_set_rat_ball`). Alternative: leave both to the
   constructor (it would need `N` to fit a word first).
5. **The order of the `DOMAIN` checks is free** (conventions 9.3 leaves it open; all are `DOMAIN`). The two `inf`
   entries and the repetition of a prime are decided before the components are built.
6. **The value is built in temporaries and moved into the output only on `ADF_OK`** (conventions 4.3); the old
   contents of the output are released at that moment, field by field, as `adf_sball_clear` does.
7. **`adf_lball_get_str` returns `NULL` with `*len = 0`** when the centre needs `p^|v|` with `|v| bits(p) >
   ADF_LBALL_BITS_MAX`; `adf_sball_get_str` does the same when one of its local components has such a centre.
   Alternative: refuse such a value in the reader too (the value is canonical, only its decimal form is large).
8. **The reader walks the text three times** (grammar and entry ranges, limits, semantics) and keeps only the
   byte range of each entry (two `size_t`), so that the memory is bounded by the length of the input and not by
   the size of an entry. Alternative: keep the entries (about 360 bytes each, so about 80 MB for a hostile text
   of the default `max_len`).

## 4. The tests bite (`lanes/t-slice2/bite.py`, no mutation run)

Seven faults, one run each (`lanes/t-slice2/bite-<name>.out` and `.build`); the file `src/text_local.c` is
regenerated from the tail before and after every fault, so the tree is left as it was.

| Fault | Replacement | The run |
|---|---|---|
| the sign of `N` in `O(p^N)` dropped | `x->N < 0 ? -x->N : x->N` | 3 failed tests, 3648 failed checks |
| the order not enforced (COMPLEX) | `t->loc[m - 1 - k]` | 1 test, 4 checks (`sball_the_complex_tag`) |
| the output written too early | the guard on the commit is dropped | 1 test, 1 check (`hostile_input`) |
| `p = 4` accepted | `st = ADF_OK;` instead of `adf_place_prime(&v, p)` | 5 failed tests, 8519 failed checks |
| the base inside `O(...)` not compared | `base != p` dropped | 3 failed tests, 6 failed checks |
| the item limit not applied | `if (0)` | 2 failed tests, 5 failed checks |
| the centre of a ball printed as `u` | `st = ADF_OK` instead of `adf_lball_get_center` | 7 tests, 7818 checks |

The third fault of the brief ("the output written before the last check") could not be injected in the reader of
a partial ball: every status of stage 6 is decided before the components are built, so there is nothing to write
early; it is injected in the reader of a local ball, where `adf_lball_set_rat_ball` can fail after the grammar,
and `hostile_input` sees it. A first attempt at the second fault (dropping the `qsort` of the reader) changed
nothing and changed nothing in the tests: `adf_sball_set_arb_lballs` sorts the components itself, so the order
is only visible at the tag `COMPLEX`, which fills the array by hand. That is recorded as decision u-7 of
`docs/api-1f.md`.

## 5. Not done

- The dump form (conventions 10) of the two types: `adf_lball_dump_str`, `_load_str`, `_dump_inspect` and the
  same for `adf_sball`. It needs the loader and the dumper of `src/dump.c`, which is not this lane's, and it is
  not small. The brief does not ask for it.
- No fuzz run and no mutation run (the brief replaces item 5 of COMMON-C by item 4, the faults above). A fuzz
  target `tests/fuzz/fuzz_text_local.c` was not written; `tests/fuzz/` is not in the list of paths of the lane.
- `tools/adf/adf.c` prints a partial ball with its own code (`adf_drv_put_sball`, read only for this lane);
  nothing of the driver was changed, so the driver does not read or write these two forms through the library
  yet. That is another lane's file.
- `lanes/t-slice2/vectors_local.jsonl` (236 kB) is written by the generator into the lane directory, as the brief
  asks. COMMON-C rule 2 would put the vectors under `tests/ref/vectors/<lane>/`, which is not in the list of
  paths of this lane; **the orchestrator should either commit the file with the lane directory or move it to
  `tests/ref/vectors/t-slice2/` and change the one path in the test** (`#define VECTORS` in
  `tests/test_text_local.c`), otherwise `make check` fails for a tree that does not have it.

## 6. Sources pending

None. Every formula and convention used here is in `docs/conventions.md`, the headers of the library, or
`proto/text_grammar.py` on disk, and each function of `src/text_local.c` cites the lines it uses. The one
`[source pending]` of the lane is inherited in the copied code of `src/text.c` (`tx_mag_upper`, the exactness of
`mag_set_ui_2exp_si`), which is quoted there and not used differently here.

## 7. Findings against the specification and the conventions

1. **The C value text of a partial ball with a decimal archimedean component is not a fixed point of printing**,
   and the pass widens it: `{inf: 1.5 +/- 1e-9}` prints from C as `{inf: 1.5 +/- 1.1e-9}` and the next pass as
   `{inf: 1.5 +/- 1.2e-9}`. This is what conventions 9.6 and gate finding G4 allow ("Repeated C value-text round
   trips may widen the value and change their text on every pass"), so it is not a defect; it is recorded
   because the golden file holds the exact text of the reference and the C cannot reproduce it (the same
   finding as `docs/api-2.md` 4.5 (1) for ideles).
2. **`adf_sball_set_str` has to fill the struct by hand for the tag `COMPLEX`**, because no function of
   `sball.h` makes a COMPLEX value (`sball.h`, "Complex components"). The struct is filled as a binding does
   (`flint_malloc` for `loc`, `adf_lball_set` per component), and `-DADF_CHECK_INVARIANTS` checks the predicate
   of conventions 5.9 before the reader returns. The conventions do not name a way to build such a value from
   text; this is the reading of conventions 5.9 and 9.2 that the golden file requires.
3. **The reader of a partial ball holds the byte ranges of the entries and walks the text three times.** A single
   walk would need the parsed entries in memory (about 360 bytes each); the limits of conventions 8.4 bound the
   text at 1 MiB by default, so a hostile text of the default `max_len` would cost about 80 MB. The three walks
   keep the memory at 16 bytes per entry. Not a fault, a remark.
4. `adf_place_prime` is called twice per prime on the paths that build a component (once for the check, once in
   the build loop): an avoidable cost (`n_is_prime` on a word is fast; the check happens for the same `p` twice).
5. The brief says the golden files have 55 and 29 lines; `tests/golden/README.md` says 52 and 25 vectors. The
   files have 55 and 29 lines, of which 52 and 25 are vectors. No disagreement between the conventions, the
   reference and the files.
6. Nothing was found against `docs/SPEC.md`.

## 8. Files outside the list of the brief that were changed

- `tests/test_julia.sh`: the one block the brief allows ("In `tests/test_julia.sh` you may add the line your file
  needs"); the block follows the pattern of the five blocks above it.
- No other file outside the list was changed. `src/text.c`, `tools/adf/adf.c`, `tests/golden/`, `proto/` and
  `docs/conventions.md` were read only.