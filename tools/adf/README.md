# adf: the command-line driver of adelefeld

    make                build ../../build/adf from adf.c and ../../build/libadelefeld.a
    make SAN=1          build ../../build/adf-san with the address and undefined-behaviour
                        sanitizers
    adf [-v] [file]     read a script of commands from the file, or from standard input

This is work package 1.5 of `docs/PLAN.md` section 6.  The driver is a thin program over
the public interface: it parses nothing of the value form itself.  Every operand is read
by `adf_text_classify` and by the typed parser of its kind (`docs/conventions.md` 9.7), and
printed by the typed printer (conventions 9.4).  It creates no context, so every value it
holds is in the global backend, which the value form does not record (SPEC 4.1,
conventions 9.8 A11).

## The language

Input is read line by line.  A line is at most 65536 bytes; a longer line is an error for
that line (`error: LIMIT`) and the rest of it is skipped.  A line is ignored when it is
empty, when it holds nothing but spaces and tabs, or when its first non-blank byte is `#`.
Every other line is one command:

    <operation> <operand> [ <separator> <operand> ]

Only the space (0x20) and the tab (0x09) separate tokens of a line of the driver.  Every
other byte outside the alphabet of conventions 8.2 is passed on unchanged to the parsers,
which reject it, so a byte 128 to 255 or a NUL byte in an operand gives `error: PARSE`.

### The separator, and why it is ` with `

The brief asks for a separator token that cannot occur in any value text, and for a proof
that the one chosen is unambiguous.

1. The alphabet of conventions 8.2 is the bytes 0x20 to 0x7E together with TAB, LF and CR.
   That is every printable ASCII byte, so no printable byte lies outside the alphabet and
   none can be used as a separator.  A control byte cannot be used either: TAB, CR and LF
   are the whitespace of the value form (8.2), and any other control byte is a forbidden
   byte (stage 2 of 8.5 gives `ADF_PARSE`).
2. So a word is used.  A token of the value form is a number, a keyword, `+/-`, or one of
   `( ) [ ] < > { } ; , : * + = ^` (conventions 8.3).  A keyword is a maximal run of ASCII
   letters that must equal the keyword the grammar expects, and the keywords of the
   grammar of 9.2 are `mod`, `inf`, `i`, `Q`, `union`, `ffun`, `rfun`, `term`, `char` and
   the single letters `D M P A B C q n s`.  The run `with` is not one of them, so it
   derives no sentence of any of the thirteen start symbols: a value text cannot contain
   the four letters `with`, and therefore cannot contain ` with ` either.
3. The separator of the driver is the byte string ` with ` (a space, the word, a space).
   The driver splits a line at the first and the second occurrence of that byte string, so
   an operand can never contain it, and every operand is the exact text between two
   separators, with no trimming: the value form allows whitespace before the first token
   and after the last one (conventions 8.2), so a command may be written with spaces
   around it.

### The operations

| Operation | Operands | What it does |
|---|---|---|
| `show` | one | parse the operand and print it canonically (conventions 9.4) |
| `type` | one | print the kind that `adf_text_classify` gives (conventions 9.7) |
| `add`, `sub`, `mul` | two | the ring operations of the two operands (SPEC 4.3) |
| `neg` | one | the negation of the operand |
| `div` | two, the second an exact rational | division by an exact rational (SPEC 4.5) |
| `equal` | two | `true` or `false`: equality of the two sets (SPEC 4.2) |
| `contains` | two | `true` or `false`: the first set inside the second (SPEC 4.2) |
| `overlaps` | two | `true` or `false`: the two sets meet (SPEC 4.2) |
| `compare` | two | `equal`, `different` or `undecided`: the comparison of the two unknown points (SPEC 4.2) |
| `reconstruct` | one or three | see below |
| `cap` | two, a finite ball and a rational | the absolute cap (SPEC 4.4 item 3) |
| `dump` | one | the dump form of the value (conventions 10.1) |
| `load` | one | read a dump form, print the value in the value form (conventions 10) |
| `prec <bits>` | a setting | the precision of the real coordinate, 1 to `ADF_PRINT_EXP_MAX`, default 64 |
| `digits <n>` | a setting | the digits of the real-ball printer, 1 to 1000000, default 20 (`ADF_DIGITS_DEFAULT`) |

`reconstruct` takes one operand, an adele, and reconstructs with the interval of its real
coordinate, or three operands, a finite ball and the closed interval `[lo, hi]` given as
two exact rationals.  The two separator uses of this one operation are a documented
extension of the two-operand form: the value form has no text for an interval, so the two
end points cannot be one operand.  `lo > hi` is the empty interval.

A wrong number of operands is a line that is not a sentence of the grammar of the driver:
`error: PARSE`.  An operation word that is unknown, a line with no operation word, and a
setting whose argument is not a decimal integer are `error: PARSE` as well.  Around that
number there may be blanks, tabs and a CR, the whitespace of the value form
(conventions 8.2), so a script with CRLF line endings runs every setting line.

A setting below its range is `error: DOMAIN`: the data violate the stated domain of the
setting (`prec` needs at least 1 bit, `digits` the range 1 to `ADF_DIGITS_MAX` of
`include/adelefeld/text.h:39`).  A `prec` above `ADF_PRINT_EXP_MAX` is `error: LIMIT`
instead, because it is a size bound of the driver and not a domain: see "The guard on
printing a real ball" below for the bound and the reason.

`compare` is the three-valued comparison of the two unknown points of SPEC 4.2.  It answers
`equal` when both values are exact and equal, `different` when the two sets are disjoint,
and `undecided` otherwise.  Those are the words of SPEC 4.2 ("certainly equal", "certainly
different", "undecided") in the spelling of the three values of `adf_fball_compare`
(`include/adelefeld/fball.h:257-264`).  Two balls that are the same set but are not points
are `undecided`: the two points they hold may or may not be equal.

`dump` writes the dump form of conventions 10.1 of the value, one token per field, and
never fails.  `load` reads a dump form and writes the value in the value form of
conventions 9.4.  The driver has no context, so the two commands cover the values of the
global backend only, which is where every value of the value form is (conventions 9.8,
A11): a dump with a context occurrence is `error: UNSUPPORTED`, and so is a body of
section 10 that the driver has no type for (`scaled`, `ucoset`, `idele`, `idclass`,
`lball`, `sball`, `qclass`, `ffun`, `rfun`, `char`, `modctx`).  A fourth token that is no
body of section 10 at all is `error: PARSE`, and a text that does not begin `adf1 Q ` is
read by the loader of `adf_rat`, whose status is then the status of the text: the version,
the field and the syntax of section 10.1 do not depend on the body.  The dump form has no
whitespace other than the single spaces of its tokens (conventions 8.2), so `load` does not
trim its operand.

### Types of the operands, and the pairs that are refused

The driver reads four of the thirteen kinds of the value form: `adf_rat`, `adf_fball`,
`adf_adele` and `adf_cadele`.  A kind with no typed parser in this build (`adf_ucoset`,
`adf_idele`, `adf_idclass`, `adf_lball`, `adf_sball`, `adf_qclass`, `adf_ffun`,
`adf_rfun`, `adf_char`; work packages 1.8 and later) gives `error: UNSUPPORTED`.  `type`
still names those kinds, since the classifier knows all thirteen.

### The order of the checks

One command is decided in this order, and the driver stops at the first step that fails.
`tests/driver/12_status_order.cmd` is the test of every step.

1. **The line.**  An operation word of the table, and the right number of operands; else
   `error: PARSE`.  A line is not a sentence of the grammar of the driver when it has too
   few or too many operands.
2. **The syntax of every operand, in order.**  `adf_text_classify` alone, which is stages 1
   to 3 of conventions 8.5: the length, the alphabet and the grammar of section 9.  A
   sentence of no language of section 9.2 is `error: PARSE`, and a text over `max_len` is
   `error: LIMIT`.  This comes first because it is a fact about the line: no other operand
   makes a malformed operand into a sentence.
3. **The kind of every operand, in order.**  A kind of conventions 9.7 with no typed parser
   in this build is `error: UNSUPPORTED`, and it is decided before any value of the line is
   read, because a request on a type that version 1 does not implement is not a proved
   domain error (conventions 3.1).  So `add [5 mod 6] with 1/0` is `error: UNSUPPORTED` and
   not `error: DOMAIN` for the zero denominator, and the two end points of `reconstruct`
   are read by the same rule as every other operand.
4. **The value of every operand, in order**, by the typed parser of its kind: the limits of
   conventions 8.4 and the semantic constraints of 9.3, that is stages 4 to 7.  The first
   operand whose value fails gives its status, `error: LIMIT`, `error: DOMAIN` or
   `error: NOT_DETERMINED`.
5. **The operation.**  The pair of types, the domain of the operation, and the problem to be
   solved: `error: DOMAIN`, `error: NOT_UNIT`, `error: NO_SOLUTION`,
   `error: NOT_UNIQUE`.
6. **The printer.**  A printer that returns NULL is `error: LIMIT` (see below).

So a limit or a constraint of the first operand is reported before a limit of the second
one, and a kind that this build does not implement is reported before both.

Operands of different types are combined only where SPEC 4.1 defines it:

- an exact rational with a finite ball, an adele or a complex adele: the rational is
  converted only in the coordinate where it is inexact, `adf_fball_add` /
  `adf_fball_mul_rat`, `adf_adele_add_rat` / `adf_adele_mul_rat` and the same for
  `adf_cadele`.  For `sub` with the rational on the left, `q - x` is the negation of
  `x + (-q)`, which is exact in every coordinate;
- an adele with a complex adele: the adele is embedded in `C x A_f` with
  `adf_cadele_set_adele`, the embedding of SPEC 4.1, and the operation is a
  `adf_cadele` one;
- a finite ball with a finite ball, an adele with an adele, a complex adele with a complex
  adele: the ordinary ring operations.

Every other pair is `error: DOMAIN`.  In particular `div` needs an exact rational as its
second operand, because division by an adele is not defined (SPEC 4.5) and no finite ball
ever certifies a divisor (SPEC 4.5).

The three set predicates of SPEC 4.2 are the predicates of two finite balls.  An exact
rational is read as the ball of radius 0, which SPEC 4.2 handles as a single point, so a
rational operand is accepted; every other type is `error: DOMAIN`, because the
specification defines no set predicate on an adele or a complex adele and the driver does
not invent one.  `cap` needs a finite ball as its first operand; an adele or a complex
adele is `error: UNSUPPORTED` and a rational is `error: DOMAIN`.

## Output and exit status

One line per command on standard output: the value text, `true` or `false`, one of
`equal`, `different`, `undecided` for `compare`, the name of a kind for `type`, the dump form
for `dump`, or `error: <STATUS NAME>` with the name of `adf_status_str`
(conventions 11.1).  A setting writes no line, and no line is ever empty.  The exit status
is 0 when no command failed, 1 otherwise, and 2 for a usage error: an unknown option, more
than one file, a file that cannot be opened, and a read error (a directory opens and then
fails to read).  A usage error writes one line to standard error.  Nothing else is printed
unless `-v` is given, which writes a trace of every command to standard error.

### The guard on printing a real ball

The guard is the rule of the library, and the driver has none of its own.  Decision M1-D6
(`docs/SPEC.md:865`, `include/adelefeld/text.h:32-38`): a printer of a value with a real or
complex part returns NULL with length 0 when the midpoint or the radius of one of its real
balls is not zero and has a binary exponent above `ADF_PRINT_EXP_MAX = 100000` in absolute
value, tested before any conversion, and `adf_rat_get_str` and `adf_fball_get_str` never
return NULL.  **The driver answers `error: LIMIT` for a NULL from a printer.**  The reason
is in the decision: the cost and the memory of the printer grow with the binary exponent,
and an exponent beyond a machine word aborted the process.

The binary exponent is the one FLINT stores, `x = m 2^e` with `0.5 <= |m| < 1`
(`refs/src/flint-3.0.1/arf.rst:14-20`), which is one more than the exponent
`floor(log2 |x|)` of the other reading; a `mag` holds the same form
(`refs/src/flint-3.0.1/mag.rst:3-6`).  So `2^99999` has `e = 100000`, the last admitted
exponent, and `2^100000` has `e = 100001`, the first refused one.  The words of M1-D1
("a binary exponent exceeds 100000 in absolute value") do not say which of the two readings
is meant; the header of the printers does, and the driver follows the header.
`tests/driver/11_guard.cmd` is the test of the boundary, in the midpoint, in the radius and
in the imaginary part of a complex adele.

The same rule covers a value that an operation produced, and
`tests/driver/11_guard.cmd` has a case in which each operand is printable and only the
result is not: `prec 100000`, then `show (2^51000 ; 0)` prints and
`mul (2^51000 ; 0) with (2^51000 ; 0)` is `error: LIMIT`, because the product `2^102000`
has the binary exponent 102001.

### Why `prec` stops at `ADF_PRINT_EXP_MAX`

A real result of magnitude about `2^k` rounded at `prec` p has a radius with a binary exponent
of about `k - p`: `div (1 ; 0) with 3` at `prec` 100000 has `MAG_EXP` -100000 and at `prec`
100001 `MAG_EXP` -100001 (measured with `lanes/m1-repair-driver/prec_probe.c`), and
`2^50000 / 3` at `prec` 100001 has `MAG_EXP` -50001 and is printed
(`docs/reviews/m1/surface/closure-checks/prec_reason_closure.c`).  Above `ADF_PRINT_EXP_MAX`
an inexact real result of magnitude about 1 or below is not within the reach of a printer, and
a larger one is.  The cap is therefore a choice (decision M1-D1) and not a consequence of the
printer's bound: **a setting above `ADF_PRINT_EXP_MAX` is `error: LIMIT`**, the status
of a size bound (conventions 3.1).  A setting below 1 is `error: DOMAIN`, the status of
data outside a stated domain.  `digits` has no such bound: `ADF_DIGITS_MAX = 1000000` is a
domain of the printer (`include/adelefeld/text.h:39`), and a value outside it is
`error: DOMAIN`.

### Two lines of the test depend on FLINT 3.0.1

`tests/driver/09_settings.cmd` pins the rounding of FLINT in two expected lines, `(1.5
+/- 0.13 ; 1)` and `(2 +/- 0.63 ; 1)`.  Conventions 9.5 asks two things of a read real
ball: the printed interval contains `[mid - rad, mid + rad]`, and the `arb` is exact when
the odd mantissa of the literal has at most `prec` bits.  It asks nothing of the rounding
of the midpoint.  1.625 at `prec` 2 is contained in the ball `1.5 +/- 1/8` and also in the
ball `1.25 +/- 3/8`, and both texts conform; the driver prints the first, because FLINT
rounds to nearest.  The shell test of the driver compares bytes and has no exact rational
arithmetic, so the two lines are kept as they are and the driver is said here to depend on
FLINT 3.0.1 for them.  Every other expected line of the driver test is derived from the
specification alone.

## Tests

`sh tests/test_driver.sh` from the repository root builds the driver and runs every case;
`SAN=1 sh tests/test_driver.sh` runs the same script against the sanitized build.  The
cases are `tests/driver/*.cmd` with the expected lines in `tests/driver/*.out`, and the
hostile inputs, which `tests/driver/gen_hostile.sh` writes into `build/driver/`.  The
script also checks that the code of the driver does not write the number 100000 down: the
bound on printing is read from the library, never copied.

The fuzzer is `tests/fuzz/fuzz_driver.c`; `make fuzz FUZZ_TARGET=driver FUZZ_SECONDS=120`
runs it over `tests/fuzz/corpus/driver/`, whose seeds are the scripts of `tests/driver/`
and the hostile inputs.
