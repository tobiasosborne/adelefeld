# Lane m1-repair-driver: findings R7 to R15 of reviewer `surface`

Worktree: `/home/tobias/Projects/adelefeld-wt/m1-repair-driver`, base commit a19cd71 plus the
tree as it was when the lane started.  Sources read: `CLAUDE.md`, `lanes/COMMON-C.md`,
`docs/reviews/m1/surface/review.md` and its reproducers, `docs/SPEC.md` section 15 (M1-D1,
M1-D6), `docs/conventions.md` (3.1, 4.1, 5.2, 8.2, 9.4, 9.5, 9.7, 10.1, 10.2, 12.1, 12.4),
`include/adelefeld/*.h`, `refs/src/flint-3.0.1/arf.rst`, `refs/src/flint-3.0.1/mag.rst`,
`tests/golden/dump.tsv`, the tools and tests of the driver.

## What was done

### R7: one rule on printing, the rule of the library

`tools/adf/adf.c` had its own bound (`ADF_DRV_MAX_EXP2 = 100000`) and its own test of the
midpoint and the radius (`adf_drv_exp2_ok`).  The rule of the library (decision M1-D6,
`include/adelefeld/text.h:32-38`, implemented in `src/text.c:1105-1120`) is: a printer of a
value with a real or complex part returns NULL with length 0 when a midpoint or a radius has
a binary exponent above `ADF_PRINT_EXP_MAX` in absolute value.  The driver now holds no
bound: `adf_drv_value_print` answers `ADF_LIMIT` for a NULL, and the constants
`ADF_DRV_MAX_EXP2`, `adf_drv_exp2_ok` and the two `#include <flint/arf.h>`,
`<flint/mag.h>` are gone.  `tests/test_driver.sh` gained a check that fails when the code of
the driver (its comments apart) writes the number 100000 down, so the second copy cannot come
back.  `tests/driver/11_guard.cmd` (new) pins the boundary of the library's rule in the
midpoint, in the radius and in the imaginary part of a complex adele, and on a value an
operation produced.

The boundary is the binary exponent FLINT stores, `x = m 2^e` with `0.5 <= |m| < 1`
(`refs/src/flint-3.0.1/arf.rst:14-20`), so `2^99999` has `e = 100000` (admitted) and
`2^100000` has `e = 100001` (refused).  The reviewer's expectation, that `2^100000` prints,
reads the words of M1-D1 with the other convention (`floor(log2 |x|)`); the brief of this
lane and the header of the printers name the rule of the library, and that is what the driver
does.  See "Findings against the specification".

### R8: the largest useful `prec`

Measured with `lanes/m1-repair-driver/prec_probe.c`: the real part of
`adf_adele_div_rat(y, x, 1/3, p)` has `MAG_EXP` -99999 at `p` 99999, -100000 at `p` 100000 and
-100001 at `p` 100001; the midpoint has `ARF_EXP` -1 throughout.  A real result rounded at
`prec` p therefore has a radius with the binary exponent -p, and a printer (M1-D6, `|exp| <=
ADF_PRINT_EXP_MAX`) refuses it as soon as p > `ADF_PRINT_EXP_MAX`.  The driver now takes
`ADF_PRINT_EXP_MAX` as its largest `prec` (it reads the constant of the library, it does not
write it down) and answers `ADF_LIMIT` for a larger setting: a size bound of an algorithm is
`ADF_LIMIT` (conventions 3.1), while a setting below 1 is `ADF_DOMAIN`, data outside a stated
domain.  `digits` is unchanged (`ADF_DIGITS_MAX` is a domain of the printer,
`include/adelefeld/text.h:39`, so `ADF_DOMAIN`).

### R9: whitespace around the number of a setting

`adf_drv_setting` trims the whitespace of conventions 8.2 (space, TAB, LF, CR) at both ends
of its argument and then reads an optional "-", decimal digits, nothing else.  A CRLF script
and a line with trailing blanks now run every setting.  The new hostile case
`h9_settings_ws` (written by `tests/driver/gen_hostile.sh`, expected lines in
`tests/driver/hostile/h9_settings_ws.out`) holds the bytes, since a trailing blank or CR
cannot be seen in a `.cmd` file that a diff is to read.

### R10: the order of the checks

The driver decides, and stops at the first failure: (1) the line (operation word, arity),
(2) the syntax of every operand in order (`adf_text_classify`), (3) the kind of every operand
in order (a kind with no typed parser in this build is `ADF_UNSUPPORTED`), (4) the value of
every operand in order (the typed parser), (5) the operation (the pair, the domain, the
problem), (6) the printer.  The two choices are stated in `tools/adf/README.md`, section "The
order of the checks": the syntax of a line comes before the kinds, because a line whose
operand is not a sentence of the grammar of 9.2 is malformed whatever the other operand is; a
kind this build does not implement comes before every value, because a request on a type
version 1 does not implement is not a proved domain error.  The eight commands of
`docs/reviews/m1/surface/checks/status_order.cmd` and five more are
`tests/driver/12_status_order.cmd`.  The two end points of the interval of `reconstruct` are
now read like every other operand (through the classifier and the typed parser), which is
what makes their `UNSUPPORTED` answers come out.

### R11: a result that cannot be printed with printable operands

`tests/driver/11_guard.cmd`: `prec 100000`, then `show (2^51000 ; 0)` prints and
`mul (2^51000 ; 0) with (2^51000 ; 0)` is `error: LIMIT`.  The operand is exact at that
precision (conventions 9.5, "Reading": a dyadic whose odd mantissa has at most `prec` bits is
read exactly), so the expected text is the text of conventions 9.5 of the exact dyadic
`2^51000` with radius 0.  The example of the README (`add (1e50000 ; 0) with (1e50000 ; 0)`)
was replaced by this one, because each of its operands is already over the limit.

### R12: `tests/test_dlopen.c` builds the shared object

The test now asks whether `build/libadelefeld.so` is there and no older than
`build/libadelefeld.a`; when it is not, it runs `sh tests/test_exports.sh` through
`system()` (the one place of the tree that builds the shared object) and asks again.  A build
that fails is a failed check, so `make check` cannot pass with 0 checks any more.  All five
functions are asked of `dladdr`, not one.  The test runs from the repository root, as the
Makefile runs it, and says so.

### R13: `tests/test_exports.sh` judges the names that begin with `_adf`

A name that begins with an underscore is reserved to the implementation (C11 7.1.3) and
belongs to the linker or to another library: those are printed and not judged.  A name that
begins with `_adf` is a name of this library (conventions 4.1 item 5: "Underscore functions
state their aliasing rules"), and is judged like every other name.  The rule is in the
comment of the script and in the code that builds the list.

### R14: the comment of `tests/test_place.c`

The comment said the rejected arguments include `2^64`; the array holds `2^64 - 2` and `2^64`
is not a `ulong`.  The comment now names the primes and the composites of the array and says
that the two largest values of the type are the ones tested.  Comments only; the test is
unchanged.

### R15: the two lines that pin the rounding of FLINT

`tests/driver/09_settings.cmd` now says, at both places, that the line is what it is because
FLINT rounds to nearest, and that conventions 9.5 asks only that the printed interval contain
`[mid - rad, mid + rad]` (`1.25 +/- 0.38` would conform as well at `prec` 2).  The expected
lines are kept: the shell test of the driver compares bytes and has no exact rational
arithmetic, so it cannot accept "any text whose ball encloses the exact value".
`tools/adf/README.md` has the section "Two lines of the test depend on FLINT 3.0.1" and says
that every other expected line of the driver test is derived from the specification alone.

### The new operations of the library: `compare`, `dump`, `load`

* `compare` is `adf_fball_compare`, the three-valued comparison of the two unknown points of
  SPEC 4.2, and prints `equal`, `different` or `undecided` (the names of `ADF_CMP_EQUAL`,
  `ADF_CMP_DIFFERENT`, `ADF_CMP_UNDECIDED`; SPEC 4.2 calls them "certainly equal", "certainly
  different" and "undecided", and the README says so).  As for the other predicates of 4.2 an
  exact rational is read as the ball of radius 0.
* `dump` writes the dump form of conventions 10.1 of a value; every value of the value form is
  in the global backend (conventions 9.8, A11), so the dump has the form `g` and no context
  occurrence.
* `load` reads a dump form and prints the value in the value form.  The driver has no
  context: a dump with a context occurrence is `ADF_UNSUPPORTED` (the occurrence count comes
  from the inspector of the body, which validates the whole text first), and so is a body of
  section 10 that the driver has no type for.  A fourth token that is no body at all is
  `ADF_PARSE`; a text that does not begin `adf1 Q ` is read by the loader of `adf_rat`, whose
  status is then the status of the text, since the version, the field and the syntax of
  section 10.1 do not depend on the body.  This is the same shape as
  `tests/test_dump_golden.c`, which picks the loader of a row of `tests/golden/dump.tsv` the
  same way.
* The tests are `tests/driver/13_dump.cmd` (25 lines, with the reason in a comment of each),
  and the dump texts of `tests/golden/dump.tsv` are their inputs.

### The build of the driver (found while running the checks)

`sh tests/test_driver.sh` after `make clean && make check SAN=1` failed at the link: the
top-level Makefile had left a sanitized `build/libadelefeld.a` and a plain driver cannot link
it.  `tools/adf/Makefile` now builds one archive per SAN setting, `build/drv-plain` and
`build/drv-san`, so the two never mix; with `SAN=1` the library itself is instrumented as well,
which the old arrangement was not (it linked the plain archive and watched only the driver).
Both orders of the two driver runs now work.

### The fuzzer

`tests/fuzz/fuzz_driver.c` gained one claim: no line of the output is empty.  A printer that
returned nothing without a status used to be possible in the driver (the old
`adf_drv_value_print` wrote a newline and returned `ADF_OK` when `s == NULL`); a command now
answers a value text, `true`, `false`, a word of `compare`, a kind name, a dump form or
`error: <STATUS>`.

## The files written

* `tools/adf/adf.c` — the guard of R7 and the bound of R8 gone; the setting parser of R9; the
  order of checks of R10; the three new operations; `adf_drv_kind_type`, `adf_drv_value_read`,
  `adf_drv_value_dump`, `adf_drv_dump_bodies`, `adf_drv_dump_body`, `adf_drv_load`,
  `adf_drv_is_ws`; the header comment rewritten.
* `tools/adf/README.md` — the new operations, the settings and their whitespace, "The order of
  the checks", the guard (the rule of the library, the convention of the binary exponent, the
  reason for the example), "Why `prec` stops at `ADF_PRINT_EXP_MAX`", "Two lines of the test
  depend on FLINT 3.0.1", the tests section.
* `tools/adf/Makefile` — one archive per SAN setting.
* `tests/test_driver.sh` — the hostile case `h9_settings_ws`; the check that the driver holds
  no bound of its own on printing.
* `tests/driver/11_guard.cmd`, `11_guard.out` (new), `12_status_order.cmd`, `.out` (new),
  `13_dump.cmd`, `.out` (new), `07_status.cmd`, `.out` (the limit of `prec` and its status),
  `09_settings.cmd`, `.out` (R15 and the range of the settings), `gen_hostile.sh` (case h9),
  `hostile/h9_settings_ws.out` (new), `README.md` (the two tables).
  `11_guard.cmd` is 230 KB: the boundary of the guard is the binary exponent 100000, and a
  value with that exponent needs 30103 decimal digits, so the script holds the decimal
  expansions of `2^99999`, `2^100000` and `2^51000` (six and two times).
* `tests/test_dlopen.c` — builds the shared object, all five functions through `dladdr`.
* `tests/test_exports.sh` — judges the names that begin with `_adf`.
* `tests/test_place.c` — the comment only.
* `tests/fuzz/fuzz_driver.c` — the claim "no empty line".
* `lanes/m1-repair-driver/`: `log.md` (red and green of every finding), `report.md`,
  `prec_probe.c` (the measurement behind R8), `probe.py`, `probe_before.txt`,
  `probe_after.txt`, `replay_old.py`.

## Every check that was run

* `sh tests/test_driver.sh`
  `27 cases, 100376 expected lines, all equal (SAN=0)`, exit 0; the timed case h7: 100000
  lines in 0.085 s.
* `SAN=1 sh tests/test_driver.sh`
  `27 cases, 100376 expected lines, all equal (SAN=1)`, exit 0; h7 in 0.671 s.  With this
  repair the library itself is instrumented too, not only the driver.
* `make -j2 check`
  `check passed: all 39 test programs`, exit 0 (31 s from a clean build; the run was repeated
  after the last change, with the same result).
* `make clean && make -j2 check SAN=1`
  `check passed: all 39 test programs`, exit 0 (53 s).
* `make fuzz FUZZ_TARGET=driver FUZZ_SECONDS=60`
  `Done 17235 runs in 61 second(s)`, `fuzz passed: 1 target(s), no crash`; the coverage of
  `tools/adf/adf.c` is 79.9 % of the regions.  An earlier run of the same command, over the
  corpus of the tree as it was before the new scripts, gave `Done 17020 runs` and the same
  result.
* `build/fuzz/driver_cov` over a corpus of the thirteen scripts plus the committed seeds
  (built in `build/`, since `tests/fuzz/corpus/` is not this lane's):
  `Done 21030 runs in 61 second(s)`, exit 0, no artifact, no assertion of the target.
* `valgrind -q --error-exitcode=9 --leak-check=full build/adf < tests/driver/*.cmd`
  0 `definitely lost`, 0 `Invalid read/write`, 0 `uninitialised`; 58 records of
  `possibly lost`, all of them in FLINT's mpz cache (`fmpz_promote_val`, `fmpz_set_str`),
  which is what the reviewer found as well.
* `sh tests/test_exports.sh`
  `test_exports: passed: 193 of 193 declared functions are exported, 0 are not implemented
  yet, no exported name is undeclared, no variadic function`, exit 0.
* `python3 docs/reviews/m1/surface/checks/exports_underscore.py`
  `_adf_undeclared exported: exit 1` with `exported and not declared: 1` and the name listed;
  `adf_undeclared exported: exit 1`.
* `rm -f build/libadelefeld.so && ./build/test_dlopen`
  the script builds the shared object, then `2 tests, 29 checks, 0 failed checks, 0 failed
  tests`, exit 0.
* `touch build/libadelefeld.a && ./build/test_dlopen`
  `== building the shared object` (the age of the file is part of the question), 29 checks.
* `./build/test_dlopen` with `tests/test_exports.sh` moved away and the `.so` removed
  `FAIL the_five_functions_are_found_by_name_in_the_loaded_object`,
  `2 tests, 2 checks, 2 failed checks, 2 failed tests`, exit 1: a build that fails is a
  failed check.
* `python3 docs/reviews/m1/surface/checks/hostile.py`
  5 mismatches, all of them intended: `guard: mid 2^100000`, `guard: rad 2^100000` and
  `guard: mid 2^100000 in cadele imag` (the driver refuses, the script expects a print, see
  the findings), `prec huge` (`error: LIMIT` where the script expects `DOMAIN`), and
  `1e100000 at prec 1e6` (the setting `prec 1000000` is now `error: LIMIT`, so the script
  sees two lines where it expects one).  The cases of R9 and of R10 are `ok`.
* `python3 lanes/m1-repair-driver/replay_old.py`
  the replay of the review over the scripts 01 to 08 and 10, with the assertion of the
  reviewer's script relaxed: `commands 299, expected lines 299, agree 221, differ 0, not
  modelled 78`.  The review reported `299 lines, 221 modelled and all equal, 0 different`.
* `python3 docs/reviews/m1/surface/checks/replay_scripts.py`
  trips on the new scripts: `AssertionError: ('11_guard.cmd', 12, 10)`, since it asserts one
  expected line per command line and the new scripts hold settings and statuses.  The file is
  under `docs/reviews/` and is not this lane's; the same computation over the old scripts is
  the row above.
* `make mutate FILES=tools/adf/adf.c MUTATE_LIMIT=8`
  the tool fails with `FileNotFoundError: .../build/mutate/run-*/w00000/tools/adf/adf.c`: it
  copies the named file as if it were under `src/` and then runs `make check`, which does not
  build or run the driver.  No mutation run is possible for this lane with the tool as it
  stands.
* `build/adf -v`, the usage errors, the exit status of every script: covered by
  `sh tests/test_driver.sh` (the `-v` case, three usage cases, the `#!exit` line of each
  script).

### Ground truth of the expected lines

All of it is on disk:

* `docs/SPEC.md:860` (M1-D1), `:865` (M1-D6), `:219-225` (SPEC 4.2: the three predicates and
  the comparison of points).
* `docs/conventions.md`: 3.1 (the meanings of the statuses), 4.1 item 5, 8.2 (the whitespace),
  9.4, 9.5 (the six steps of the printer and the reading), 9.7, 10.1 (the grammar of the dump
  form), 10.2 (the loaders, the dumpers, the inspectors), 12.1, 12.4.
* `include/adelefeld/text.h:32-38` (M1-D6 in the header of the printers) and `:66`
  (`ADF_PRINT_EXP_MAX`); `include/adelefeld/fball.h:257-264` (the three values of the
  comparison); `include/adelefeld/dump.h` (the rules of the loaders, the dumpers and the
  inspectors); `include/adelefeld/status.h:35-37` (`ADF_CMP_*`).
* `refs/src/flint-3.0.1/arf.rst:14-20` and `refs/src/flint-3.0.1/mag.rst:3-6`: the binary
  exponent FLINT stores.
* `tests/golden/dump.tsv`: the dump texts and their statuses.

The expected line of `show (2^99999 ; 0)` and of `show (0 +/- 2^99999 ; 0)` was worked out
step by step from conventions 9.5 and the computation is in the comment of
`tests/driver/11_guard.cmd` (X = 30102, q = 30083, M = 49950104650719225397 * 10^30083,
R = ceil2(0.2016... * 10^30083) = 2.1e30082).  The independent oracle of the reviewer
(`docs/reviews/m1/surface/checks/oracle.py`, a file under `docs/reviews/`, not under `refs/`)
gives the same three texts, and the driver gives them too.

## What is not done

* The corpus of the fuzzer, `tests/fuzz/corpus/driver/`, still holds the ten old scripts, so a
  `make fuzz FUZZ_TARGET=driver` run starts from seeds without the words `compare`, `dump` and
  `load`; the new commands are reached in the run I made with a corpus built in `build/` from
  the thirteen scripts (21030 runs, no crash), but the committed corpus should get the three
  new scripts.  That path is not in the list this lane owns.
* Mutation testing of `tools/adf/adf.c` is not possible with `tools/mutate/mutate.py` as it
  stands (the command above); the driver is also not in `make check`, so a mutation run would
  not test it even if the tool accepted the file.
* `docs/SPEC.md`, `docs/conventions.md`, the headers and `docs/reviews/` were not touched.
* The corpus of the fuzzer was not changed (see above), and no other path outside the list
  this lane owns was touched: not `docs/`, not `include/`, not `src/`, not the top-level
  `Makefile`, not `tests/fuzz/corpus/`, not `tests/golden/`.  `tests/driver/README.md` lists
  every script and every hostile case in two tables and was extended with the three new
  scripts and the new hostile case.

## Sources pending

None.  Every formula, convention and status used here is on disk and cited above.  One
statement is not taken from a source on disk: that `1.25 +/- 0.38` (or `1.25 +/- 0.375` printed
by 9.5) would be a conforming text for 1.625 at `prec` 2.  It follows from conventions 9.5
(the printed interval must contain the ball, and the `arb` need not be the nearest rounding)
and is argued in the comment of `tests/driver/09_settings.cmd`; it is not quoted from anyone.

## Findings against the specification

1. **M1-D1 does not say which convention the binary exponent uses, and the two readings
   differ by one.**  `docs/SPEC.md:860`: "refuses to print a real ball whose binary exponent
   exceeds 100000 in absolute value".  FLINT stores the exponent as `x = m 2^e` with
   `0.5 <= |m| < 1` (`refs/src/flint-3.0.1/arf.rst:14-20`), which is one more than
   `floor(log2 |x|)`.  With the stored exponent, `2^99999` has `e = 100000` and is the last
   admitted value, `2^100000` has `e = 100001` and is refused; the reviewer of milestone 1
   read the words with `floor(log2 |x|)` and expected `2^100000` to print (R7, and the three
   `MISMATCH` lines of `checks/hostile.py` that remain).  The header of the printers
   (`include/adelefeld/text.h:32-38`) and the brief of this lane name the rule of the library,
   so the driver follows it, and `tools/adf/README.md` and `tests/driver/11_guard.cmd` say
   which reading is used.  M1-D1 should name the convention, or defer to M1-D6, which does
   name it by naming `ARF_EXP` and `MAG_EXP`.
2. **M1-D6 and the header differ on the zero midpoint and the zero radius.**  `docs/SPEC.md:865`:
   "returns NULL with length 0 when a midpoint or radius has a binary exponent above
   `ADF_PRINT_EXP_MAX` in absolute value"; `include/adelefeld/text.h:33-35`: "the midpoint or
   the radius of one of its real balls **is not zero** and has a binary exponent ... above".
   The header is the right one (the exponent of a zero is an encoding, not a magnitude), and
   the driver follows the header.  The wording of M1-D6 should carry the same clause.
3. **The specification gives no `prec` range for the driver, and the driver needs one.**  The
   largest useful `prec` is `ADF_PRINT_EXP_MAX` (R8) because a result rounded at `prec` p has
   a radius with the binary exponent -p, but this is nowhere written down: M1-D1 names the two
   settings and nothing else.  The driver decides it and `tools/adf/README.md` says why, with
   the measurement.  A line in M1-D1 ("prec is 1 to `ADF_PRINT_EXP_MAX`, a larger setting is
   `LIMIT`") would put it where the other decisions are.
4. **The words of `compare` are not the words of SPEC 4.2.**  SPEC 4.2 (line 227) says the
   comparison returns "certainly equal", "certainly different" or "undecided"; the driver
   prints `equal`, `different`, `undecided`, the names of `ADF_CMP_EQUAL`,
   `ADF_CMP_DIFFERENT` and `ADF_CMP_UNDECIDED` (`include/adelefeld/status.h:35-37`).  Both
   spellings are in the tree, the driver uses the shorter one, and the README states the
   correspondence.  If the orchestrator prefers the words of SPEC 4.2, only three strings in
   `tools/adf/adf.c` and three lines of `tests/driver/13_dump.out` change.
5. **`prec` above the bound: `LIMIT` and not `DOMAIN`.**  The reviewer's `checks/hostile.py`
   expects `error: DOMAIN` for `prec 99999999999999999999999999` ("out of range").  A setting
   above the largest useful `prec` is a size bound of the driver, and conventions 3.1 gives
   `ADF_LIMIT` for "a size bound of an algorithm"; `ADF_DOMAIN` is kept for a setting below
   its range and for `digits`.  The status of `prec` out of range is not named in the
   specification either way.
6. **`tests/test_exports.sh` did not judge the names that begin with `_adf`, and conventions
   12.1 does not say that it should.**  12.1 says every public operation is an exported
   function; it says nothing about the other way round, and the script took the stricter line
   for every name but the reserved ones.  Conventions 4.1 item 5 puts the underscore functions
   in the interface, so the script now judges `_adf*`; a sentence in 12.1 would settle it.
7. **The reproducer `docs/reviews/m1/surface/checks/replay_scripts.py` assumes that every
   command line has an expected line.**  That is true of the ten scripts it was written for
   and false of the new ones, which hold settings (no line) and statuses.  The file is under
   `docs/reviews/` and belongs to the reviewer; the same computation over the old scripts
   gives the reviewer's numbers (221 modelled lines, 0 different).
8. **`tools/adf/Makefile` and `tests/test_driver.sh` could not be run in any order** (fixed
   here): the driver linked `build/libadelefeld.a`, whose sanitizer flags are whatever the
   last top-level build left, so `sh tests/test_driver.sh` after `make check SAN=1` failed at
   the link.  Each setting now has its own archive below `build/`.  The top-level Makefile
   still has no target for the driver and no target for `tests/test_driver.sh` or
   `tests/test_exports.sh`, so `make check` runs neither; that is the orchestrator's decision
   to make and is not this lane's to take.
