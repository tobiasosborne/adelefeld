# lanes/drv-ball: report

## What was done

Three steps, in the order of the brief.

**Step 1, `lball` and `sball` as value kinds.**  `adf_lball` and `adf_sball` are two more kinds the driver
holds.  The changes in `tools/adf/adf.c` follow the seven kinds that were there: the enum `adf_drv_type` gained
`ADF_DRV_LBALL` and `ADF_DRV_SBALL` before `ADF_DRV_OTHER`, the value union gained an `adf_lball_t b` and an
`adf_sball_t s` with their `init` and `clear`, `adf_drv_kind_type` maps `ADF_TEXT_LBALL` and `ADF_TEXT_SBALL`,
`adf_drv_value_read` calls `adf_lball_set_str` and `adf_sball_set_str` (the partial ball at the session `prec`,
as for an adele and an idele), and `adf_drv_value_print` calls `adf_lball_get_str` and
`adf_sball_get_str(len, x, digits)` (the session `digits`).  `type` names them, because it asks
`adf_text_classify` alone, and it named them before this lane as well.  `dump` is unchanged and answers
`error: UNSUPPORTED`, since `adf_drv_body_slot` has no entry for `lball` and `sball`.

Two guards were needed where the old code would have read a field of the wrong type:

* `adf_drv_arith` (add, sub, mul): after the pair of types has given the type of the result, a pair that has a
  local ball or a partial ball is `error: DOMAIN`.  Without it the code would have taken the branch that writes
  the result into `z->a` (an adele) for an `adf_sball` result and printed the field of the wrong type.
* the case `ADF_DRV_NEG` of the command: a type outside the four of the top of the file (the three kinds of
  milestone 2 have been taken already by `adf_drv_units_involved`) is `error: UNSUPPORTED`, since the driver
  implements no negation of a local ball or of a partial ball.  Without it `adf_drv_neg` would have printed the
  init value of the field.

`div`, `cap`, `equal`, `contains`, `overlaps`, `compare` and `reconstruct` needed no change: each of them
already answers `error: DOMAIN` for a type it does not know.

**Step 2, a stored `sball` as the operand X.**  `project`, `exp_at`, `log_at`, `sin_at`, `cos_at`, `sinh_at`,
`cosh_at`, `root_at`, `roots_at`, `powrat_at` and `powunit_at` accept a partial ball of the value form as X (and,
for `powunit_at`, as the operand S) in the place where they accepted an adele.  The new function
`adf_drv_sball_restrict` takes the component of the partial ball at each of the named places with the three
public functions of `include/adelefeld/sball.h`: `adf_sball_has_place`, `adf_sball_get_arb`,
`adf_sball_get_lball` and `adf_sball_set_arb_lballs`.  No function of the library is missing and none was
written; the only new driver code is the gathering of the components.

Statuses of the new operand form: `ADF_OK` when every place of the list is a place of the partial ball;
`ADF_DOMAIN` when a place of the list is no place of the partial ball (there is no component to take) or when a
place of the list occurs twice (as `adf_sball_project` answers it); `ADF_UNSUPPORTED` when the archimedean tag
is complex, because the functions at places are the real ones (`include/adelefeld/sball.h`, "A PRIME");
`ADF_LIMIT` from `adf_sball_set_arb_lballs`.  The second operand of `powunit_at` goes through the same
function, so it has the same rule.

**Step 3, one printer.**  Nothing was changed.  The comparison is `lanes/drv-ball/printer-diff.md`: 122 lines,
0 byte-identical, 122 different.  The two texts differ in the template, and only in the template: the driver
prints `real: r; 5: c + O(5^N)` where the library prints `{inf: r; p=5: c + O(5^N)}` (conventions 9.4).  Inside
the two texts the components are byte-identical in 115 of the 122 lines; the 7 that are not are archimedean
ones, where the value read back from the printed text loses a digit of the radius (the value form of a real
ball is an enclosure, not a lossless form: conventions 9.6, gate finding G4).  Since they differ, the brief
says to change nothing, and nothing was changed.

`tools/adf/README.md` was updated: the list of the kinds it reads (now nine of thirteen), the paragraph on the
pairs that are refused, the paragraph on `dump` (a local ball and a partial ball are read and printed, and
their dump form is still not implemented), and the section on the commands at places, which now names the
partial ball as an operand and its statuses.

## The files written

* `tools/adf/adf.c`: the two kinds, the guards, `adf_drv_sball_restrict`, `adf_drv_value_sball`, the comments of
  the top of the file and of the section on the commands at places.
* `tools/adf/README.md`: the paragraphs named above.
* `tests/driver/drv-ball-values.cmd` and `.out`: 22 lines, step 1.
* `tests/driver/drv-ball-ops.cmd` and `.out`: 13 lines, step 2.
* `lanes/drv-ball/progress.md`, `lanes/drv-ball/printer-diff.md`, `lanes/drv-ball/lib_text.c`,
  `lanes/drv-ball/printer_diff.py`, this report.

`lanes/drv-ball/lib_text.c` is the instrument of step 3: it reads the text the driver printed, rewrites the
labels to those of conventions 9.4, reads the value with `adf_sball_set_str` or `adf_lball_set_str` and prints
it with the printer of the library.  It is in the lane directory and no Makefile of the repository builds it.
`lanes/drv-ball/printer_diff.py` writes `printer-diff.md` from it.  Both were removed at the end except the
sources.

## The checks

Every program ran under `timeout`, with at most 2 jobs.

* baseline of the suite, before any change, `timeout 900 sh tests/test_driver.sh`:
  61 cases, 101124 expected lines, all equal (SAN=0).
* red, `drv-ball-values`, `./build/adf < tests/driver/drv-ball-values.cmd`:
  22 lines, 13 of them `error: UNSUPPORTED`, exit 1.
* red, `drv-ball-ops`, `./build/adf < tests/driver/drv-ball-ops.cmd`:
  10 of 13 lines `error: UNSUPPORTED`, exit 1.
* red, the suite with the two new fixtures, `timeout 900 sh tests/test_driver.sh`:
  stops at `drv-ball-ops`, 8 differences in the first 11 lines.
* green, `drv-ball-values`: 22 lines equal to the fixture, exit 1.
* green, `drv-ball-ops`: 13 lines equal to the fixture, exit 1.
* the driver builds, `timeout 900 make -C tools/adf`: no warning (the flags of the Makefile are
  `-Wall -Wextra -Wpedantic -Werror`).
* the suite as it stands, `timeout 900 sh tests/test_driver.sh`: stops at `01_spec_4_1`, the exit status is 0
  where the fixture expects 1 (the table below).
* the suite with the six old fixtures corrected in a scratch copy, `timeout 900 sh /tmp/td.sh`:
  63 cases, 101159 expected lines, all equal (SAN=0).
* the same under the sanitizers, `SAN=1 timeout 900 sh /tmp/td.sh`:
  63 cases, 101159 expected lines, all equal (SAN=1).
* clang: `make -j2 BUILD=lanes/drv-ball/build-clainv CC=clang all`, then the driver built by

      clang -Iinclude -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror tools/adf/adf.c \
          lanes/drv-ball/build-clainv/libadelefeld.a -lflint -lgmp -lm -o lanes/drv-ball/build-clang/adf

  no warning; the 49 fixtures all equal; standard error empty on every one of them.
* the invariant checks: `make -j2 BUILD=lanes/drv-ball/build-inv INV=1 all`, then the same driver command with
  `-DADF_CHECK_INVARIANTS` added and the archive of that build: no warning; the 49 fixtures all equal;
  standard error empty.
* the sanitizers: `make -j2 BUILD=lanes/drv-ball/build-san SAN=1 all`, then the same driver command with
  `-fsanitize=address,undefined -fno-omit-frame-pointer` added and the archive of that build: no warning; the 49
  fixtures all equal; no sanitizer diagnostic on any of them.
* the table of step 3:

      python3 lanes/drv-ball/printer_diff.py ./build/adf ./lanes/drv-ball/lib_text \
          lanes/drv-ball/printer-diff.md

  122 rows, 0 byte-identical, 122 different, 115 with identical components.
* the four new fixture files end with a newline (`od -c` on each).
* `git status --short`: `tools/adf/README.md` and `tools/adf/adf.c` changed, the four new files of
  `tests/driver/` and the files of `lanes/drv-ball/` added; no other path of the tree.
* `awk 'length > 116'` on every file this lane wrote: no line over 116.

`git diff --stat` at the end of the lane:

     tools/adf/README.md |  43 +++++++++---
     tools/adf/adf.c     | 190 ++++++++++++++++++++++++++++++++++++++++++----------
     2 files changed, 187 insertions(+), 46 deletions(-)

The four new files of `tests/driver/` and the five new files of `lanes/drv-ball/` are untracked; `git status
--short` shows no other path of the tree.

## What is not done

**Step 1 changes six existing fixtures, and this lane may not edit them.**  The brief says so, and the rule of
the lanes says the `.out` files of the tree are read-only here.  The run `timeout 900 sh tests/test_driver.sh`
therefore stops at `01_spec_4_1`.  Each of the twenty lines below was written when the driver had no typed
parser for the local ball and the partial ball; the new line is the answer of the driver after this lane, and
the change in each case follows from the code and from conventions 9.7.  The orchestrator has to write the new
lines and, for `01_spec_4_1`, the `#!exit` line: no command of that script fails any more, so it becomes
`#!exit 0` (or the line is removed).

| fixture | line | command | old | new |
|---|---|---|---|---|
| `01_spec_4_1` | 9 | `show [p=5: 3 + O(5^4)]` | `error: UNSUPPORTED` | `[p=5: 3 + O(5^4)]` |
| `01_spec_4_1` | 10 | `show {inf: 1; p=5: 2 + O(5^2)}` | `error: UNSUPPORTED` | `{inf: 1; p=5: 2 + O(5^2)}` |
| `06_pairs` | 136 | `add 1/2 with [p=5: 3]` | `error: UNSUPPORTED` | `error: DOMAIN` |
| `06_pairs` | 137 | `sub 1/2 with [p=5: 3]` | `error: UNSUPPORTED` | `error: DOMAIN` |
| `06_pairs` | 138 | `mul 1/2 with [p=5: 3]` | `error: UNSUPPORTED` | `error: DOMAIN` |
| `06_pairs` | 140 | `div 1/2 with [p=5: 3]` | `error: UNSUPPORTED` | `error: DOMAIN` |
| `06_pairs` | 141 | `equal 1/2 with [p=5: 3]` | `error: UNSUPPORTED` | `error: DOMAIN` |
| `06_pairs` | 142 | `contains 1/2 with [p=5: 3]` | `error: UNSUPPORTED` | `error: DOMAIN` |
| `06_pairs` | 143 | `overlaps 1/2 with [p=5: 3]` | `error: UNSUPPORTED` | `error: DOMAIN` |
| `06_pairs` | 144 | `reconstruct [p=5: 3]` | `error: UNSUPPORTED` | `error: DOMAIN` |
| `06_pairs` | 145 | `cap [p=5: 3] with 2` | `error: UNSUPPORTED` | `error: DOMAIN` |
| `06_pairs` | 150 | `show [p=5: 3]` | `error: UNSUPPORTED` | `[p=5: 3]` |
| `07_status` | 32 | `show [p=5: 3]` | `error: UNSUPPORTED` | `[p=5: 3]` |
| `12_status_order` | 1 | `add [p=5: 3] with 1/0` | `error: UNSUPPORTED` | `error: DOMAIN` |
| `12_status_order` | 2 | `add 1/0 with [p=5: 3]` | `error: UNSUPPORTED` | `error: DOMAIN` |
| `12_status_order` | 3 | `add [p=5: 3] with (1e100001 ; 0)` | `error: UNSUPPORTED` | `error: LIMIT` |
| `12_status_order` | 4 | `reconstruct (* ; 1 mod 2) with [p=5: 3] with 1` | UNSUPPORTED | DOMAIN |
| `12_status_order` | 5 | `reconstruct (* ; 1 mod 2) with 0 with [p=5: 3]` | UNSUPPORTED | DOMAIN |
| `13_dump` | 7 | `compare [p=5: 3] with 1/2` | `error: UNSUPPORTED` | `error: DOMAIN` |
| `f-places-hostile` | 35 | `exp_at [p=5: 3 + O(5^4)] with 4` | `error: UNSUPPORTED` | `error: DOMAIN` |

`06_pairs` line 139 (`neg [p=5: 3]`) and `13_dump` line 8 (`dump [p=5: 3]`) do not change: the first is
`error: UNSUPPORTED` from the new guard of `neg`, the second is `error: UNSUPPORTED` from the dump.

Two of the six fixtures lose what they were written to test, and the orchestrator should know it:

* `12_status_order` used the local ball as its example of "a kind of conventions 9.7 with no typed parser in
  this build", to show that step 3 of a command is decided before step 4 (the value of no operand is read).
  That example is gone.  A kind that has no parser still: the quotient class, `qclass`, whose texts the
  classifier knows (`(0.5 ; 0) + Q`, `union((0.5 ; 0 mod 1)) + Q`).
* `f-places-hostile` line 35 was written as "a value that the driver cannot use, decided before the places are
  judged: UNSUPPORTED before DOMAIN".  The place `4` is not a prime, so the answer is now the `error: DOMAIN`
  of the place, decided in step 5 before the type of X.  The same line with a quotient class as X would keep
  the meaning of the test.

**Not done otherwise.**

* The printer of the driver is left as it is (step 3, the brief says so when the texts differ).
* `make check` (the tests of the library) was not run: this lane changed no file under `src/` or `include/`.
* No line of the expected output of `drv-ball-ops` or `drv-ball-values` was taken from the output of the
  program; each was written from conventions 9.3, 9.4, from the golden files `tests/golden/lball.tsv` and
  `tests/golden/sball.tsv`, from `tests/driver/f-project.out` and `f-at-prime.out` (which
  `lanes/f-slice6/expected.py` computed from the series of `docs/proofs/functions.md` with exact Fractions),
  and from `proto/text_grammar.py print_real`, as each comment of the two scripts says.

**One expected line of `drv-ball-values` was wrong and was corrected in the script** (rule of
`tests/driver/README.md`).  I had written `show {inf: 1.5 +/- 1e-9; p=2: 1 + O(2^3); p=5: 3 + O(5^4)}` to
`{inf: 1.5 +/- 1e-9; ...}`, taking the expected column of `tests/golden/sball.tsv` line 2 for a byte-level
round trip.  That column is an enclosure, not bytes: `check_sball_text` of `tests/test_text_local.c` compares
the midpoint and the radius as ranges, and conventions 9.6 with gate finding G4 says that a decimal string is
an enclosure on input.  The driver prints `1.1e-9`, which is `ceil2` of a radius above `1e-9`, and that is what
the line says now.  Checked: `adf_sball_get_str` of `{inf: 1.5 +/- 1e-9}` gives `{inf: 1.5 +/- 1.1e-9}` at
   every prec from 2 to 4096 that is a power of two.

## Sources pending

None.  Every formula and every convention used here is in the repository: `docs/conventions.md` 9.2, 9.3, 9.4,
9.6 and 7, `include/adelefeld/text.h` lines 286 to 342, `include/adelefeld/sball.h`, `docs/proofs/functions.md`
Proposition 22, `tests/golden/lball.tsv` and `tests/golden/sball.tsv`.

## Findings against the specification

1. **`docs/conventions.md` 9.6 and gate finding G4 against the golden file `tests/golden/sball.tsv`.**  The
   golden file reads as a byte-level round trip (`{inf: 1.5 +/- 1e-9}` gives `{inf: 1.5 +/- 1e-9}`), and no
   value read at any prec prints that text, because the radius of the stored ball is at least `1e-9` and
   `ceil2` of it is `1.1e-9` (measured at every prec from 2 to 4096 that is a power of two).  The test passes
   only because
   `check_sball_text` compares the archimedean entry as ranges.  The file is not wrong, and conventions 9.6
   says the round trip is a fixed point of the printed text and not of the input text, but a reader of the
   golden file alone is misled.  Nothing in `docs/SPEC.md` is contradicted; the wording of the golden file
   could be clearer.

2. **The brief of this lane against six fixtures of the tree.**  Step 1 asks for `lball` and `sball` as value
   kinds, and the `.out` files of six fixtures hold `error: UNSUPPORTED` for texts of those kinds.  Both cannot
   hold.  The code follows the brief; the table above gives the new lines, and the two fixtures that lose
   their subject are named.  The orchestrator decides.

3. **The status of `neg` for a kind the driver holds but does not combine.**  The conventions give no rule:
   `add`, `sub`, `mul` and `div` answer `error: DOMAIN` for a pair of types the operation does not define, and
   `neg` is a unary operation.  This lane answers `error: UNSUPPORTED` (a request on a combination version 1 of
   the driver does not implement, conventions 3.1).  `error: DOMAIN` would also be defensible; the choice is
   written in the README.  The same question arises for every unary command that takes the new kinds, and the
   other unary commands of the driver (`inv`, `pow`, `norm`, ...) already answer `error: DOMAIN` or
   `error: UNSUPPORTED` type by type.

## `git diff --stat` at the end of the lane

See the last command of the session; the list of paths is the one under "The files written".  `git status
--short` shows no other path of the tree.