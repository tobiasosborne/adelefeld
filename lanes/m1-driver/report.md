# Lane m1-driver: report

Work package 1.5 of `docs/PLAN.md` section 6: the command-line driver `adf`.  The driver is
one C file, `tools/adf/adf.c`, over the public interface; it parses nothing of the value
form itself, every operand is read by `adf_text_classify` and by the typed parser of its
kind, and printed by the typed printer.

## What was done

- `tools/adf/adf.c` (1089 lines): the line reader, the operand splitter, the operations,
  the printer with the guard on the binary exponent of a real ball, and the small `main`.
  The function `adf_driver_run(const char * text, size_t len, FILE * out)` runs a whole
  script; `adf_driver_run_v(..., int verbose)` is the same with the trace of `-v`, so that
  no state is global.  `main` reads the input whole and calls it.
- `tools/adf/Makefile`: builds `build/adf` (and `build/adf-san` with `SAN=1`) from
  `adf.c` and `build/libadelefeld.a`, and builds the library on demand.
- `tools/adf/README.md`: the language, the proof that the separator ` with ` cannot occur
  in a value text, the operations, the types and the refused pairs, the output, and the
  guard on printing a real ball.
- `tests/test_driver.sh` (195 lines): builds the driver, runs every case, stops at the
  first difference; `SAN=1` runs the same script against the sanitized build.
- `tests/driver/`: ten scripts with their expected output, the generator of the hostile
  inputs, the expected output of the hostile inputs, and a README that says which part of
  the specification each script is taken from.
- `tests/fuzz/fuzz_driver.c` and `tests/fuzz/corpus/driver/` (18 seeds).

The language, as decided by the brief and implemented: an input line is at most 65536
bytes (a longer line is `error: LIMIT` and the rest of it is skipped); empty lines, lines
of blanks, and lines whose first non-blank byte is `#` are ignored; every other line is
`<operation> <operand> [ with <operand> ]`, the separator being the byte string ` with `
(no printable byte lies outside the alphabet of conventions 8.2, and the run `with` is not
a keyword of the grammar of 9.2, so it cannot occur in a value text; the argument is in
`tools/adf/README.md`).  Settings: `prec <bits>` (1 to 1000000, default 64) and
`digits <n>` (1 to 1000000, default 20).  Operations: `show`, `type`, `add`, `sub`, `mul`,
`neg`, `div`, `equal`, `contains`, `overlaps`, `reconstruct`, `cap`.  `reconstruct` takes
one operand (an adele) or three (a finite ball and the interval `[lo, hi]` as two exact
rationals), the one documented extension of the two-operand form, because the value form
has no text for an interval.

Mixed types are combined only where SPEC 4.1 defines it: an exact rational with a finite
ball, an adele or a complex adele, and an adele with a complex adele (embedded with
`adf_cadele_set_adele`); every other pair is `error: DOMAIN`.  A kind of the value form
with no typed parser in this build gives `error: UNSUPPORTED`, decided before every other
check.  For the three set predicates an exact rational is read as the ball of radius 0,
which SPEC 4.2 handles as a single point; every other type is `error: DOMAIN`, because the
specification defines no set predicate on an adele.

## Files written

    tools/adf/adf.c            the driver
    tools/adf/Makefile        builds build/adf and build/adf-san
    tools/adf/README.md       the language, the separator argument, the guard
    tests/test_driver.sh      the acceptance test
    tests/driver/README.md    what each script is taken from
    tests/driver/01_spec_4_1.cmd|.out      SPEC 4.1, 16 commands
    tests/driver/02_spec_4_2.cmd|.out      SPEC 4.2, 16 commands
    tests/driver/03_spec_4_3.cmd|.out      SPEC 4.3, 5 commands
    tests/driver/04_spec_4_4_cap.cmd|.out  SPEC 4.4 item 3, 8 commands
    tests/driver/05_spec_4_5.cmd|.out      SPEC 4.5, 13 commands
    tests/driver/06_pairs.cmd|.out         every pair of the four types, 155 commands
    tests/driver/07_status.cmd|.out        every reachable status, 34 commands
    tests/driver/08_show_type.cmd|.out     conventions 9.3, 9.4 and 9.7, 46 commands
    tests/driver/09_settings.cmd|.out      conventions 9.5 through the settings, 15 commands
    tests/driver/10_line_language.cmd|.out the line language, 8 commands
    tests/driver/gen_hostile.sh (80 lines)  writes the hostile inputs into build/driver/
    tests/driver/hostile/h1_nul.out        a NUL byte in a line
    tests/driver/hostile/h2_high.out       the bytes 128 to 255
    tests/driver/hostile/h3_line_65536.out a line of exactly 65536 bytes
    tests/driver/hostile/h4_line_65537.out a line of 65537 bytes
    tests/driver/hostile/h5_no_final_newline.out   a file without a final newline
    tests/driver/hostile/h6_empty.out      an empty file (no output, status 0)
    tests/fuzz/fuzz_driver.c               the libFuzzer target
    tests/fuzz/corpus/driver/              18 seeds, 208 kB
    lanes/m1-driver/report.md, redgreen.log, fuzz.log

No other file was changed; the top-level `Makefile` was not touched, and the driver is
built by `tools/adf/Makefile`.

## The acceptance test: rows run

The expected lines of every script were written by hand from `docs/SPEC.md` and
`docs/conventions.md` before the program existed.  Where a line was wrong, the line was
corrected and the correction is written down under "Wrong expectations" below; where the
program was wrong, the program was changed.

| Part of the specification | Rows in the table | Rows run as commands |
|---|---|---|
| SPEC 4.1, the table of the types | 6 (`adf_rat`, `adf_fball`, `adf_lball`, `adf_sball`, `adf_adele`, `adf_cadele`) | 6, plus 10 commands for the examples of the section (the exact value, `(i ; 0)` squared, the canonical form, the centre in `[0, N)`, `mod 0` dropped) |
| SPEC 4.2, the table of the set predicates | 3 (`equal_set`, `overlaps`, `contains`) | 3, in 8 commands, plus 7 commands for "radius zero is handled as a single point" and 1 for "overlap is not equality" |
| SPEC 4.3, the table of the tight operations | 5 | 5, one command each |
| SPEC 4.4 item 3, the absolute cap | 3 claims (the radius becomes `gcd(R, C)`; the cap never touches an exact value; "what is not a policy") | 3, in 8 commands |
| SPEC 4.5 | no table, 4 claims (no division by an adele, no division by a finite ball, division by an exact non-zero rational, the exact zero divisor) | 4, in 13 commands |

Beyond the specification: every operation with every pair of the four implemented types
(16 pairs for each of `add`, `sub`, `mul`, `div`, `equal`, `contains`, `overlaps`; 4 for
`neg`; 1 or 3 operands for `reconstruct`; 2 for `cap`; 1 for `show` and `type`), the
unimplemented kinds, and the thirteen kinds of `type`.

## Checks, with the command and the result

| Command | Result |
|---|---|
| `make -C tools/adf` | `build/adf` built, no diagnostic (`-std=c11 -O2 -Wall -Wextra -Wpedantic -Werror`) |
| `sh tests/test_driver.sh` | `test_driver: 23 cases, 100320 expected lines, all equal (SAN=0)`; the 100000 lines of the case h7 in 0.024 s to 0.067 s over several runs |
| `/usr/bin/time -f "%e s %M kB" build/adf < build/driver/h7_many_lines.cmd` | 0.03 s, 4480 kB, three runs, for 100000 lines (`show 7/3` each) |
| `SAN=1 sh tests/test_driver.sh` | `test_driver: 23 cases, 100320 expected lines, all equal (SAN=1)`; the same 100000 lines in 0.134 s; `ldd build/adf-san` shows the address sanitizer |
| `CC=clang make -C tools/adf && sh tests/test_driver.sh` | 23 cases, 100320 lines, all equal |
| `make -j2 check` (top level, not changed by this lane) | `check passed: all 21 test programs` |
| `sh tests/driver/gen_hostile.sh` | `h3_line_65536.cmd: the first line is 65536 bytes`, `h4_line_65537.cmd: the first line is 65537 bytes`; the files h1 (37 bytes with two NUL), h2 (543), h5 (8, no final newline), h6 (0), h7 (900000, 100000 lines) |
| `make fuzz FUZZ_TARGET=driver FUZZ_SECONDS=120` | `Done 46047 runs in 121 second(s)`, 380 exec/s, corpus 18 files to 298 units, `fuzz passed: 1 target(s), no crash`; coverage of `tools/adf/adf.c`: 444 regions, 12 missed (97.30%), 26 functions, 0 missed (100%), 654 lines, 20 missed (96.94%), 342 branches, 21 missed (93.86%).  The log is `lanes/m1-driver/fuzz.log` |

The missed lines of the fuzz run are the arms that the caller has already made
unreachable (the `default:` of four switches over the type of a value, since a value of
type `ADF_DRV_OTHER` is refused before they are reached) and the two `return 2` of the
path of an error on the output stream, which a `tmpfile` does not raise.  The rest of the
report is a boundary artefact of `llvm-cov show` (an `else if` line that is certainly
executed is shown with the count of the other side of the branch).

A first fuzz run of 20 s found a "crash" at once: it was the assertion of the target, not
the driver.  The first version of the target required every output line to be a kind name
or `true`/`false`, which no value text ever is; the target now checks what is checkable:
the status is 0, 1 or 2; no more lines than the input has; every line that begins with
`error: ` is followed by a status name of `adf_status_str`; the status 0 comes with no
error line and the status 1 comes with at least one.

## Bugs of the driver that the test found

1. The four printers return `char *` and write the length through a pointer; the driver
   dropped the return value, so every value line was empty.  The test `01_spec_4_1` failed
   on the second line.
2. `sub` and `add` of a complex adele with an adele returned the complex adele unchanged:
   only the left operand was embedded into `C x A_f`.  Found by the case of SPEC 4.1 that
   compares `(i ; 0)` squared with the `-1` of the ring.
3. `type` called the typed parser, so `type 1/0` answered `error: DOMAIN` instead of the
   kind: conventions 9.7 says the classifier checks the syntax only.  The driver was
   changed, not the test; `08_show_type` has a line for each of the four kinds of a text
   with a semantic defect.
4. The sign of the argument of a setting was dropped, so `prec -1` and `digits -1` were
   accepted.  Found by `07_status`.

## Wrong expectations, corrected in the test files

These were my arithmetic, not the program; each is noted in the script itself.

1. `neg 1` is the exact rational 1, not the adele 1; the command of SPEC 4.1 is now
   `neg (1 ; 1)`.
2. `contains` is "first inside second": the two commands of SPEC 4.4 were reversed.
3. `(1 mod 2) * (1/2)` is the set `1/2 + Zhat`, because `(1/2)(2 Zhat) = Zhat`, and not
   the set `(1/2) Zhat`; its canonical text is the `(* ; 1/2 mod 1)` that SPEC 4.4 itself
   writes, and my expected line `(* ; 0 mod 1/2)` was the text of the larger set.
4. `div X with 1/2` is `2 X`, not `X / 2`; one line of SPEC 4.5 had the product.
5. In `06_pairs` five lines used the finite ball `3 mod 12` where the operand was
   `5 mod 18`, and one line took the intermediate `11/2` for `1/2 - (5 + 18 Zhat)`, which
   is `9/2` before the negation.
6. `01_spec_4_1` expected the exit status 0 while two of its commands are `UNSUPPORTED`;
   `10_line_language` was one line short (five `show` commands, not four).
7. The generator of the hostile input built a line of 65542 bytes where its comment said
   65537; `gen_hostile.sh` now pads to 65537 exactly and prints the length of the first
   line of h3 and of h4, which are 65536 and 65537.

## Not done

- The library functions the brief does not put in the operation list are not exposed: the
  dump form, the context, `adf_fball_add_cap`, `_sub_cap`, `_mul_cap`, `_mul_rat_cap`
  (only `adf_fball_cap` is the operation `cap`), `adf_fball_compare`, `adf_fball_prec_at`,
  `adf_fball_haar_volume`, `adf_fball_get_*`, and every type of work package 1.8 and later.
- The three-valued point comparison of SPEC 4.2 (the last paragraph of the section,
  `adf_fball_compare`) has no operation in the fixed list of the brief, so it is not
  exercised.  The other claims of 4.2 are.
- The examples of SPEC 4.1 that concern the local backend cannot be run: the driver
  creates no context, so every value it holds is global, which is what the value form
  records anyway (A11).
- The precision of a real ball is read at 64 bits by default and at 2 bits in one test;
  the effect of an intermediate precision is not tabulated.
- Stage 1 of conventions 8.5 (`len > max_len`, 1048576 bytes) is not reachable through the
  driver, because the line limit of 65536 bytes is the smaller one.  The limits that are
  reached are the line limit, `max_exp10` and the binary exponent guard.
- No mutation run and no benchmark; the brief asks for neither, and the mutation tool
  covers `src/` only.
- `ADF_NOT_DETERMINED`, `ADF_UNIT_NOT_CERTIFIED` and `ADF_NEEDS_SPLIT` cannot be produced
  through the driver: the operations call no function that returns them (the sign
  conditions of 9.3 belong to the idele, the idele class and the term, and
  `adf_adele_set_str` has none, `include/adelefeld/adele.h:143`).  The reason is written
  in the header comment of `07_status.cmd`.

## Sources pending

None.  Everything the driver relies on is a declaration of `include/adelefeld/` read on
disk (`arf.h:87` and `mag.h:114` for the binary exponents, named in the source), the
specification and the conventions, and the golden files of `tests/golden/`, which were
used only to check the kind names and the canonical texts, not to produce an expected
line.  The claim about the value of `flint_malloc` on exhaustion, which the header of the
printers leaves open, is not used by the driver: the guard on the binary exponent keeps
the printer away from it, and the driver never allocates with `flint_malloc` itself.

## Findings against the specification

1. **The canonical form of a finite ball with a non-integer radius is worth stating
   precisely.**  SPEC 4.1 gives the predicate "for `H > 0`: `0 <= A < H` and
   `gcd(A, H, d) = 1`", and conventions 5.2 adds that the centre is "the unique element
   of `a + N Z` in `[0, N)`".  Together they do determine the triple: the set fixes the
   radius `N = H/d`, the interval `[0, N)` holds exactly one element of `a + N Z` because
   its length is `N`, and `gcd(A, H, d) = 1` then fixes `d`.  I checked this because my
   first hand computation of the row `(1 mod 2) * (1/2)` of SPEC 4.4 produced
   `(* ; 0 mod 1/2)`, a text that seemed to denote the same set as `(* ; 1/2 mod 1)`; it
   does not, `(0 + 1 Zhat)/2` is the larger set `(1/2) Zhat` while `(1 + 2 Zhat)/2` is
   `1/2 + Zhat`.  The two triples of that example are therefore not a counterexample, and
   the specification stands.  Nothing to correct; the observation is written here because
   the question is easy to get wrong and the driver is a place where it shows.
2. **The operation list of the brief leaves the point comparison of SPEC 4.2 without a
   command.**  The section defines a three-valued comparison of two unknown points
   (`certainly equal`, `certainly different`, `undecided`) and the library declares
   `adf_fball_compare` for it, but the brief fixes the operations of the driver and does
   not include it, so the driver neither offers nor tests it.  The last paragraph of 4.2
   is the one part of the section that this lane does not cover.
3. **`type` has to be a second entry into the parser.**  Conventions 9.7 says the
   classifier checks the syntax only, and `adf_text_classify` and the typed parsers are
   separate functions, so a caller that wants the kind of a text with a semantic defect
   (a zero denominator, a decimal exponent over `max_exp10`) must call the classifier and
   not the parser.  That is what the driver does.  It is not a defect of the interface,
   but it is the reason `type 1/0` answers `rat` and `show 1/0` answers `error: DOMAIN`.
4. **A compiler diagnostic worth knowing.**  Not a finding against the specification: GCC
   13.3.0 reports `-Wstringop-overread` on `adf_adele_get_str(&len, v->a, digits)` when
   the `arb` inside the same member has been read a few lines before, because it applies
   the object size of the `arb` to the enclosing value.  The driver prints from a local
   copy of the value, which the guard needs anyway, and the diagnostic is gone at `-O1`,
   `-O2` and `-O3`; no warning is switched off.  The sanitized run is the runtime check
   that no read leaves the object.
