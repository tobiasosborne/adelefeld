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
| `reconstruct` | one or three | see below |
| `cap` | two, a finite ball and a rational | the absolute cap (SPEC 4.4 item 3) |
| `prec <bits>` | a setting | the precision of the real coordinate, 1 to 1000000, default 64 |
| `digits <n>` | a setting | the significant digits of the real-ball printer, 1 to 1000000, default 20 (`ADF_DIGITS_DEFAULT`) |

`reconstruct` takes one operand, an adele, and reconstructs with the interval of its real
coordinate, or three operands, a finite ball and the closed interval `[lo, hi]` given as
two exact rationals.  The two separator uses of this one operation are a documented
extension of the two-operand form: the value form has no text for an interval, so the two
end points cannot be one operand.  `lo > hi` is the empty interval.

A wrong number of operands is a line that is not a sentence of the grammar of the driver:
`error: PARSE`.  An operation word that is unknown, a line with no operation word, and a
setting whose argument is not a decimal integer are `error: PARSE` as well.  A setting
whose value is outside its range is `error: DOMAIN`.

### Types of the operands, and the pairs that are refused

The driver reads four of the thirteen kinds of the value form: `adf_rat`, `adf_fball`,
`adf_adele` and `adf_cadele`.  A kind with no typed parser in this build (`adf_ucoset`,
`adf_idele`, `adf_idclass`, `adf_lball`, `adf_sball`, `adf_qclass`, `adf_ffun`,
`adf_rfun`, `adf_char`; work packages 1.8 and later) gives `error: UNSUPPORTED`, and that
is decided before every other check, because a request on a type that version 1 does not
implement is not a proved domain error (conventions 3.1).  `type` still names those kinds,
since the classifier knows all thirteen.

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

One line per command on standard output: the value text, `true` or `false`, the name of a
kind for `type`, or `error: <STATUS NAME>` with the name of `adf_status_str`
(conventions 11.1).  The exit status is 0 when no command failed, 1 otherwise, and 2 for a
usage error: an unknown option, more than one file, a file that cannot be opened, and a
read error (a directory opens and then fails to read).  A usage error writes one line to
standard error.  Nothing else is printed unless `-v` is given, which writes a trace of
every command to standard error.

### The guard on printing a real ball

The printer of a real ball (conventions 9.5) costs time and memory that grow with the
binary exponent of the `arb`, and an exponent beyond a machine word aborts the process
(this is the first finding of the report of lane m1-text; the header of the printer offers
no way to refuse).  **The driver therefore refuses to print a real ball whose binary
exponent is above 100000 in absolute value**, in the midpoint or in the radius, of either
coordinate of a complex adele, and answers `error: LIMIT`.  The same guard covers a value
that an operation produced: `add (1e50000 ; 0) with (1e50000 ; 0)` is `error: LIMIT`.

## Tests

`sh tests/test_driver.sh` from the repository root builds the driver and runs every case;
`SAN=1 sh tests/test_driver.sh` runs the same script against the sanitized build.  The
fuzzer is `tests/fuzz/fuzz_driver.c`; `make fuzz FUZZ_TARGET=driver FUZZ_SECONDS=120` runs
it over `tests/fuzz/corpus/driver/`, whose seeds are the scripts of `tests/driver/` and
the hostile inputs.
