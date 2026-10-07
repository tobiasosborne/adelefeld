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
   The driver splits a line at successive occurrences of that byte string, so
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
| `roots` | three: a polynomial, a prime, a precision | the roots of the polynomial in `Z_p` (SPEC 9.1) |
| `realroots` | one: a polynomial | the real roots of the polynomial, in isolating balls (SPEC 9.1) |
| `recover` | three: a finite ball and two bounds | the rational of a residue class in a box (SPEC 9.2) |
| `project` | two: a value and a list of places | the partial ball of a rational, finite ball or adele over the places (SPEC 9.3.1) |
| `exp_at`, `log_at` | two: a value and one place | `exp`, `log` at the place, a prime or `real` (SPEC 9.3.1, 9.3.2) |
| `sin_at`, `cos_at`, `sinh_at`, `cosh_at` | two: a value and one place | factorial series at a prime or `real` |
| `powrat_at` | four: a value, a prime, a rational exponent, a seed | `x^(e/n)` on a root branch at the prime (SPEC 9.3.4 item 2) |
| `powunit_at` | three: a value, a prime, a value | `u^s` for a principal unit `u` and `s` in `Z_p` (SPEC 9.3.4 item 3) |
| `inv` | one | `1/x` of a unit coset, an idele, an idele class or a rational (ideles and classes below) |
| `pow`, `powtight` | two: a unit coset, idele or class, and an integer | the power `x^k`, `pow` by the default enclosure, `powtight` by the smallest coset |
| `norm` | one: an idele or a class | the norm, a positive real ball |
| `class` | one: an idele | the idele class of the idele |
| `idele` | one: a rational | the idele of the rational |
| `hull`, `hullsimple` | one: an idele | the adele that contains the idele: the smallest ball, the simple ball |
| `unitof` | one: an adele | the idele of an adele that certifies one |
| `valuation` | two: an idele and a prime | `v_p` of the content |
| `abs` | two: an idele and a place | `abs(x_p)_p` at a prime, `abs(x_inf)` at `real` |
| `root` | two or three: a rational, adele or idele; a degree; optionally a sign | the root at all places, the rational branch (SPEC 9.3.3; below) |
| `exp`, `sin`, `sinh`, `cos`, `cosh` | one: a rational or adele | the series at all places, finite part exactly 0 (SPEC 9.3.2; below) |
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
section 10 that the driver has no dump for (`scaled`, `qclass`, `ffun`, `rfun`, `char`,
`modctx`).  The nine bodies the driver reads and writes are `rat`, `fball`, `adele`, `cadele`,
`ucoset`, `idele`, `idclass`, `lball` and `sball`; the five of lane `u-dump1` since this text was
written: `dump` of a unit coset, an idele, a class, a local ball or a partial ball answers
`error: UNSUPPORTED` no more, although the driver read and printed the first three, and the last
two since lane `drv-ball`.  A fourth token that is no
body of section 10 at all is `error: PARSE`, and a text that does not begin `adf1 Q ` is
read by the loader of `adf_rat`, whose status is then the status of the text: the version,
the field and the syntax of section 10.1 do not depend on the body.  The dump form has no
whitespace other than the single spaces of its tokens (conventions 8.2), and `load` passes
the bytes of its operand on to the loader without trimming any of them: a space at the
beginning of the operand after the blanks of the line reader, or a space at its end, is a
fault of the text (`error: PARSE`).  The blanks that the line reader skips are the ones
between the operation name and the first operand and the ones around a ` with ` separator,
for every command (item 3 above): `load  adf1 Q ucoset 1 0` and `load<TAB>adf1 Q ucoset 1 0`
read the dump, `load adf1 Q ucoset 1 0 ` does not.

The bodies of the five kinds hold no context occurrence (conventions 10.2), so `load` of a
valid text of one of them needs no context and answers with the value form of 9.4.  A text
that breaks the predicate of the type (conventions 5.6 to 5.9) is `error: DOMAIN`; the two
fixtures `tests/driver/u-dump-units` and `tests/driver/u-dump-local` hold the lines, and
`tests/test_dump_units.c` and `tests/test_dump_local.c` the statuses of the texts.

### The commands of the solvers: `roots`, `realroots` and `recover`

These three commands of `docs/SPEC.md` 9.1 and 9.2 (milestone S) read operands that the value
form of conventions 9.2 does not have in every position, so they have a reading of their own.
The grammar of the line is unchanged: one operation name and one to three operands separated by
` with `, and a wrong number of operands is `error: PARSE` as for every other command.

**The polynomial operand.**  A polynomial is its integer coefficients in decimal, separated by
single spaces, the constant term first, at least one coefficient: `-2 0 1` is `X^2 - 2`.  A
leading `-` is allowed on a coefficient, a leading `+` is not, and a decimal point, an
exponent and a second space are not: an operand that is not of that form is `error: PARSE`, the
status the driver gives a text it cannot read (conventions 8.5, stage 1).  The zero polynomial
is `error: DOMAIN`, the domain of every function of `include/adelefeld/roots.h` ("Statuses",
edit E-C1).  A line is at most 65536 bytes, so a coefficient of 100000 digits gives
`error: LIMIT` for the line and the polynomial is never read
(`tests/driver/s-hostile.cmd` has both a coefficient of 100000 digits and one of 60000).

**`roots POLY with P with K`.**  `P` and `K` are exact rationals of the value form, read by
`adf_rat_set_str`, and they must be integers.  The command calls `adf_roots_padic` with the
depth `64` and prints the roots in the order of the list, separated by `; `, each as
`A mod P^K` with `A` the centre of the certificate in `[0, P^K)` and `K` the precision of that
certificate, which is `max(K_of_the_command, s + 1)` for a root whose `s = v_p(g'(alpha))` is
`s` (include/adelefeld/roots.h, D3.2), so the `K` of a line is not always the `K` of its
entries.  An empty complete list is the word `none`; an empty list is the answer of the function
and not a failure.

The statuses: `P` below 2, negative, or not below `2^64` is `error: DOMAIN`, because a place
holds a prime of one word (conventions 7, `include/adelefeld/place.h:38-40`), and a composite
`P` is `error: DOMAIN` from `adf_place_prime` itself; `K` below 1 is `error: DOMAIN`, the
domain of the function; a `K` that does not fit in a `slong`, the type of the `prec_p` argument,
is `error: LIMIT`, the status of a size bound (conventions 3.1), the status of a `prec` setting
above `ADF_PRINT_EXP_MAX` as well; and `error: LIMIT` is also what the function returns when
`2 K bits(P)` is above `ADF_ROOTS_BITS_MAX` (decision S-D18), for example
`roots -2 0 1 with 7 with 3000000`.

**`realroots POLY`.**  The command calls `adf_roots_real` at the setting `prec` and prints the
balls in the order of the list, separated by `; `, and the word `none` for a list without a
root.  Each ball is printed as the driver prints a real part, that is with the real-ball
printer of conventions 9.5 and the setting `digits`.

**`recover C mod M with A with B`.**  The first operand is a finite ball of the value form, whose
canonical global triple is `(C, M, 1)`, and the command reads the residue class `P(M, C)` of it
with `adf_resid_set_fball_forget`; `A` and `B` are exact rationals that must be integers and
may be of any size, since `adf_resid_reconstruct` takes `fmpz` arguments.  The command passes
the search limit `1000` and prints the unique solution `n/d` as the driver prints a rational
(`n` alone when `d = 1`, conventions 9.4).  The statuses are those of the function:
`error: NO_SOLUTION` (no solution, which includes the empty box `A < 0` or `B < 1`, decision
S-D5), `error: NOT_UNIQUE` (at least two solutions), `error: NOT_DETERMINED` (the search limit
ended the search before the set was decided, so uniqueness is not certified), `error: DOMAIN`
for a bound that is not an integer, and `error: DOMAIN` from `adf_resid_set_fball_forget` for an
exact ball, since a single rational is in no residue class with `M > 1`.  The command `reconstruct`
keeps its name and its one and three operand forms: the two names are different operations.

**The first operand of `recover` must be a finite ball.**  As for `cap` below, an adele or a
complex adele is `error: UNSUPPORTED` and a rational is `error: DOMAIN`.

**The order of the checks** is the order of the section below, with the polynomial in the place
of the first operand: the syntax of every operand in order (the polynomial by its own parser,
the others by `adf_text_classify`), then the kind of every operand that has one, then the value
of every operand in order, then the operation: the zero polynomial, the two integers, the prime
and the precision, and the domain of the function.  So a limit of the second operand is reported
before a domain error of the third, and the zero polynomial is reported before the prime.

**One line of the test depends on FLINT 3.0.1.**  The two-digit text of an isolating ball of
`realroots` depends on which ball the isolation of FLINT returns, and one line of
`tests/driver/s-realroots.cmd` is such a line.  It is checked by `lanes/drv-s/real_check.py`,
which applies the algorithm of conventions 9.5 to the exact dyadic midpoint and radius that
`lanes/drv-s/real_probe.c` reads out of the list, in exact rational arithmetic and written from
the algorithm; see the comment of that command.

### The commands at places: `project`, `exp_at` and `log_at`

These three commands of milestone 1F (`docs/SPEC.md` 9.3.1: `f_at(x, S)` and `project(x, S)`) read a set of
places as their second operand, which the value form of conventions 9.2 does not have.  The grammar of the line is
unchanged: one operation name and two operands separated by ` with `; a wrong number of operands is
`error: PARSE`.  The test cases are `tests/driver/f-at-prime.cmd`, `f-project.cmd`, `f-places-hostile.cmd` and
`f-places-1000.cmd`.

    project X with PLACES     the partial ball of X over the places (adf_sball_project)
    exp_at X with PLACE       exp of X at the one place (adf_sball_exp_at)
    log_at X with PLACE       log of X at the one place (adf_sball_log_at)

**X** is an exact rational, a finite ball, an adele or, since lane drv-ball, a partial ball.  A rational is made
into the adele `(q ; q)` at the setting `prec` (SPEC 4.1).  A finite ball has no real coordinate: the place
`real` is `error: DOMAIN` for it.  A partial ball is not made into an adele: the partial ball over the places is
its component at each of them (`adf_sball_get_arb` and `adf_sball_get_lball`, put together with
`adf_sball_set_arb_lballs`, the three functions of `include/adelefeld/sball.h` that the driver needs; the
projection takes no component away, docs/proofs/functions.md Proposition 22).  A place of **PLACES** that is no
place of the partial ball is `error: DOMAIN`: there is no component to take.  A complex partial ball is
`error: UNSUPPORTED` (the functions at places are the real ones).  A complex adele
is `error: UNSUPPORTED` (the complex functions are later), and so is a kind of the value form with no typed parser,
and so is a unit coset, an idele or a class (`tests/driver/f-cross.cmd`, review n-review1 D1: the driver used to read
the field of another type and printed a wrong value, `project (5 ; 5 * [1]) with 5` gave `5: 0`).  The test cases of
the partial ball as an operand are `tests/driver/drv-ball-ops.cmd`.

**PLACES** is a list of places separated by single spaces: a prime in decimal, or the word `real`.  A token that is
neither is `error: PARSE` (a `+`, a letter, a leading zero, a decimal point, two spaces in a row, a blank at the end).
A token that is a decimal but no place of `Q` is `error: DOMAIN`: a negative number, 0, 1, a composite, a number of
more than 64 bits (`adf_place_prime`, as for the solver commands).  A place named twice is `error: DOMAIN`
(`adf_sball_project`).  `exp_at` and `log_at` take exactly one place; a list of another length is `error: PARSE`.  The
order of the checks is that of the section below: the arity of the line; the syntax of X and of every token; the
number of places; the kind of X; the value of X; then the tokens that are no place, the type of X against the places,
the projection and the function.  So a syntax fault in one token wins over a token that is no place, and an
unsupported kind wins over a token that is no place.  A list may be as long as a line (`f-places-1000.cmd` has 1000).

**The precision.**  At the place `real` the setting `prec` is the working precision of arb in bits.  At a prime it is
the requested ABSOLUTE p-adic precision `N` of `include/adelefeld/lfunc.h` (`rfunc.h`, "A PRIME"): `prec 8` gives a
result modulo `p^8` for an exact input.  A finite ball keeps its own precision when that is smaller (`exp` of `5 +
5^2 Z_5` is `6 + O(5^2)` at every `prec` of 2 or more).  The statuses are those of the library
(`DOMAIN`, `NOT_DETERMINED`, `LIMIT`); the place that the library reports is not printed.

**The output** is one line, the partial ball: its components in the canonical order (the real place first, then the
primes in increasing order), separated by `; `, each

    real: <ball>                  the real-ball text of conventions 9.5, with the setting digits
    <p>: <value>                  an exact local ball: the rational p^v u as the driver prints a rational
    <p>: <centre> + O(<p>^<N>)    a local ball: the canonical centre p^v u in [0, p^N) as a rational, N its absolute
                                  precision

This is a text of the driver, not a value form of the library: the value form of a partial ball (conventions 9.2) is
a different text, `{inf: ...; p=5: ...}`.  The driver has a typed parser for it since lane drv-ball (a value of the
kind `sball` is read by `adf_sball_set_str` and printed by `adf_sball_get_str` in `show`, `type` and the
commands at places), but the line of the commands at places keeps the text above and is not changed by it
(`lanes/drv-ball/printer-diff.md`, step 3: the two texts differ in every line of every fixture).  `exp_at` and
`log_at` print the partial ball over the one place, so their line has
the label: `5: 349831 + O(5^8)` for `exp_at 5 with 5` at `prec 8`; `real: 1` for `exp_at 0 with real`; `2: 0` for
`log_at -1 with 2` (`log(-1) = 0` exactly at 2).  A centre with a negative valuation prints as a rational: `project
(* ; 1/5 mod 25) with 5` is `5: 1/5 + O(5^2)`.

### Types of the operands, and the pairs that are refused

The driver reads nine of the thirteen kinds of the value form: `adf_rat`, `adf_fball`,
`adf_adele`, `adf_cadele`, and, since lane t-slice1 (milestone 2), `adf_ucoset`, `adf_idele` and
`adf_idclass`, and, since lane drv-ball, `adf_lball` and `adf_sball`.  A kind with no typed parser in this build
(`adf_qclass`, `adf_ffun`, `adf_rfun`, `adf_char`; work packages 1.8 and later) gives `error: UNSUPPORTED`.
`type` still names those kinds, since the classifier knows all thirteen.

The two kinds that lane drv-ball added are values, and not operands of the operations: `show` prints them
through `adf_lball_get_str` and `adf_sball_get_str` (the setting `digits` for the partial ball, as for an adele;
`tests/driver/drv-ball-values.cmd`), and every other operation answers as the section "the pairs that are refused"
says: `add`, `sub`, `mul`, `div`, `cap`, `equal`, `contains`, `overlaps`, `compare` and `reconstruct` are
`error: DOMAIN` for them, because SPEC 4.1 combines no pair of types with a local ball or a partial ball, and
`neg` is `error: UNSUPPORTED`, because the driver implements no negation of them.  `dump` is the dump form
of 10.1, as for every other kind, since lane `u-dump1`.  The commands at places take a
partial ball as **X** (see the section above); a local ball is `error: UNSUPPORTED` there.

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
   domain error (conventions 3.1).  So `add [p=5: 3] with 1/0` is `error: UNSUPPORTED` and
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

### Ideles and classes

Lane t-slice1 (milestone 2) gives the driver the unit coset `[5 mod 36]`, the idele `(2.5 +/- 0.125 ; 3/2 * [5 mod 36])`
and the idele class `<1.25 +/- 0.25 ; [5 mod 36]>` (conventions 9.2, 9.3, 9.4; `adf_text_classify` classifies them as
`ucoset`, `idele`, `idclass`).  The real part is read at the setting `prec` and printed by the constrained printing
of conventions 9.5 with the setting `digits` (an idele's real part excludes 0, a class's is positive, and so is the
printed text's).  The tests are `tests/driver/i-01-show-type.cmd` to `i-06-status.cmd`; their expected lines were
written by hand and are re-derived with exact rationals by `lanes/t-slice1/check_driver_cases.py`.

| Command | Operands | Result |
|---|---|---|
| `mul X with Y` | ucoset * ucoset, idele * idele, class * class; idele * rational, rational * idele | the product (SPEC 5); with a rational `adf_idele_mul_rat`, and `0` is `NOT_UNIT` |
| `div X with Y` | the same pairs but not rational / idele; an adele by an idele | `X * Y^-1`: the inverse, then the product (two roundings); with a rational the exact inverse; adele / idele by `adf_adele_div_idele` (conventions 5.7) |
| `neg X` | a unit coset, an idele | the unit coset times `[-1]`; every coordinate of an idele negated, exactly, `(-X, r, [-1] u)`; a class has no negation (the class of `-x` is the class of `x`): `DOMAIN` |
| `inv X` | a unit coset, an idele, a class, a rational | `adf_ucoset_inv`, `adf_idele_inv`, `adf_idclass_inv`; `0` is `NOT_UNIT` |
| `pow X with K`, `powtight X with K` | a unit coset, an idele or a class; `K` an integer that fits a word | `adelefeld/idpow.h`; `K` not an integer is `DOMAIN`, beyond a word `LIMIT` |
| `norm X` | an idele or a class | the real ball `abs(x_inf) / r`, resp. `t` (`adf_idele_norm`, `adf_idclass_norm`), printed as a real part |
| `class X` | an idele | `adf_idclass_set_idele` |
| `idele Q` | a rational | `adf_idele_set_rat` at `prec`; `0` is `NOT_UNIT` |
| `hull X`, `hullsimple X` | an idele | `adf_adele_set_idele`, `adf_adele_set_idele_simple` |
| `unitof A` | an adele | `adf_idele_set_adele`: `NOT_UNIT`, `UNIT_NOT_CERTIFIED` |
| `valuation X with P`, `abs X with PLACE` | an idele; a prime, or `real` for `abs` | `v_p(r)`; `abs(x_p)_p` as a rational, `abs(x_inf)` as a real ball; `real` for `valuation` and a non-prime are `DOMAIN`; a second operand of more than one token is `PARSE` |
| `equal`, `contains`, `overlaps` | two unit cosets | `adf_ucoset_equal_set`, `adf_ucoset_contains`, `adf_ucoset_overlaps`; ideles and classes have no set predicate (`docs/api-2.md` 3.5, i3-1): `DOMAIN` |
| `add`, `sub`, `cap`, `compare`, `reconstruct` | with an operand of these kinds | `DOMAIN` |
| `dump`, `load` | | the dump form of the three kinds is the dump form of 10.1 (lane `u-dump1`) |

Every other pair is `error: DOMAIN`.  The statuses of the library are the statuses of the command:
`NOT_DETERMINED` when the real kernel cannot certify the sign of a result (or of a text read at the setting `prec`;
`prec 30` and `show (1 +/- 0.99999999999 ; 1 * [1])` is one), `NOT_UNIT`, `UNIT_NOT_CERTIFIED`.  The order of the checks is
the one below: the kinds of these operands are supported, so step 3 does not refuse them, and the rules above are step 5.
For `valuation` and `abs` the second operand is a place read as in `project` (`real` or a prime in decimal without a
leading zero): its syntax is decided in step 2 (`PARSE`), its value after the first operand's (`DOMAIN`).

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
and an exponent beyond a machine word aborted the process.  The constrained printer of the real
part of an idele or a class has a second, independent refusal (decision N-D11, `include/adelefeld/text.h`,
`docs/api-2.md` 4.2): a ball whose level search needs more than a fixed work is NULL as well, and so
`error: LIMIT`; `show`, `mul` and the other commands that print an idele or a class inherit it (`norm` and
`abs` print a real ball without the sign condition and do not).  For example, after `prec 20000`, `show (1 +/-
0.999...9 ; 1 * [1])` with 1000 nines prints and with 1500 nines is `error: LIMIT` (run on 2026-09-30).

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

### The four factorial-series commands (1F.7)

`sin_at X with PLACE`, `cos_at X with PLACE`, `sinh_at X with PLACE` and
`cosh_at X with PLACE` use the same operands, one-place syntax, status reporting and output
labels as `exp_at`. At `real` they apply the real function with arb's enclosure and working
precision in bits. At a prime p they use the corresponding series on p^c Z_p, with c=1 for
odd p and c=2 at 2. The setting `prec` is the requested absolute output precision N there.

Exact zero gives exact 0 for sin/sinh and exact 1 for cos/cosh. Other exact inputs give a
ball of exponent N. A ball a+p^M Z_p gives exponent min(N,M), except that cos/cosh of a
centred ball p^M Z_p gives the smaller enclosure 1+p^min(N,2M-v_p(2)) Z_p. A ball that
meets the domain and its complement is `NOT_DETERMINED`; a disjoint input is `DOMAIN`.

For example, at `prec 8`, `cos_at (* ; 0 mod 4) with 2` prints `2: 1 + O(2^3)`;
`sin_at 2 with 2` gives `error: DOMAIN`, while `sin_at (* ; 0 mod 2) with 2` gives
`error: NOT_DETERMINED`. The fixtures are `tests/driver/trig-*.cmd` and `.out`.

### Local roots and their branches (1F.5)

    roots_at X with PRIME with DEGREE
    root_at X with PRIME with DEGREE with SEED

`X` is a rational, finite ball or adele, as for `exp_at`. Both commands project to the named prime.
`roots_at` lists every branch in increasing identifier order. `root_at` selects one branch.
The degree and seed are exact integer operands; the degree is in 1..2^64-1.
At an odd prime the seed is the nonzero residue of the root's unit part modulo the prime.
At 2 the seed is a sign: `1` selects unit 1 modulo 4 and `-1` selects unit 3 modulo 4.
The API identifier `3` is also accepted for the latter. Zero uses identifier `0`; degree 1
is the identity, uses identifier `0` and ignores the seed.

The output labels each branch, for example:

    roots_at 9 with 5 with 2
    5 [2]: -3; 5 [3]: 3
    roots_at 9 with 2 with 2
    2 [+1]: -3; 2 [-1]: 3
    roots_at (* ; 1 mod 16) with 2 with 2
    2 [+1]: 1 + O(2^3); 2 [-1]: 7 + O(2^3)

The sign at 2 labels the torsion factor, not the real sign of a rational root.
For exact inputs, rational roots print exactly; irrational roots use absolute precision `prec`.
For ball inputs the result has the exponent `min(prec, E)`, `E` the exact image exponent of
Proposition 15 (decision N-D14): at `prec 3`, `root_at (* ; 9 mod 15625) with 5 with 2 with 3` prints
`5 [3]: 3 + O(5^3)`; with `prec` at least 6 it prints the image `3 + O(5^6)`.
The strong guard is retained. Thus `roots_at (* ; 1 mod 4) with 2 with 2` reports
`error: NOT_DETERMINED`. So do balls containing zero, except at degree 1.
A failed existence criterion on the entire guarded ball reports `error: DOMAIN`.
Invalid seeds and degree 0 also give `DOMAIN`; excess operands or malformed text give `PARSE`.
`real` is `UNSUPPORTED` for these local-root commands. The old polynomial `roots` command is unchanged.
The all-branch command has the branch-count limit `ADF_LROOT_BRANCH_MAX` of `lroot.h`;
seeded evaluation has no branch-array cost. Limits leave no partial output line.

### Powers at a prime (1F.6)

    powrat_at X with PRIME with E/N with SEED
    powunit_at X with PRIME with S

`X` and `S` are rationals, finite balls or adeles, as for `exp_at`; each is projected to the named prime.
`powrat_at` is `adf_sball_powrat_at` (`include/adelefeld/lpow.h`): the root of degree `N` of `X` on the branch
`SEED` (the identifier of `root_at`: the unit residue at an odd prime, `1` or `-1` at 2, `0` for the zero),
raised to the power `E`. `E/N` is the value text of an exact rational, so the fraction is already reduced, as
the library requires; another type is `error: DOMAIN`, a numerator beyond a signed word or a denominator beyond
a word is `error: LIMIT`. An integer exponent (`N = 1`) is the integer power, whatever the seed. The line
carries the identifier, as for `root_at` (`[0]` for `N = 1` and for the zero):

    powrat_at 9 with 5 with 3/2 with 3
    5 [3]: 27
    powrat_at 9 with 2 with 3/2 with -1
    2 [-1]: 27

For a ball the exponent is `min(prec, E')`, `E' = e' j + (M - m) - v_p(n') + v_p(e')` (`docs/api-1f6.md` P2): at
`prec 3`, `powrat_at (* ; 9 mod 15625) with 5 with 3/2 with 3` prints `5 [3]: 27 + O(5^3)`, at `prec 10` the image
`5 [3]: 27 + O(5^6)`. The statuses are those of `root_at` (`DOMAIN`, `NOT_DETERMINED` outside the guard and for a
ball around 0), and `NOT_UNIT` for the exact 0 with a negative exponent.

`powunit_at` is `adf_sball_powunit_at`: `u^s = exp(s log u)` for `u` in `1 + p Z_p` and `s` in `Z_p`, at 2
`w^(s mod 2) exp(s log u')` for every odd `u = w u'` (Proposition 17). The line is the partial ball, as for
`exp_at`. The precision of a ball result is `min(prec, R)`, `R = min(A + beta, B + alpha, A + B)` of
Proposition 18: `powunit_at 6 with 5 with (* ; 2 mod 25)` is `5: 36 + O(5^3)`. At 2, where the sign of `u` or the
parity of `s` is not fixed, the result is the hull `1 + 2 Z_2`: `powunit_at -1 with 2 with (* ; 0 mod 1)` prints
`2: 1 + O(2^1)`. `u` outside `1 + p Z_p` or `s` outside `Z_p` is `error: DOMAIN`; a ball that meets the domain and
its complement is `error: NOT_DETERMINED`; `real` is `UNSUPPORTED` for both commands. The fixtures are
`tests/driver/pow-values.cmd` and `pow-status.cmd`, with the reason of every expected line in a comment.

### Roots at all places (1F.8)

    root X with N
    root X with N with SIGN

The root of degree `N` of `X` at all places at once (SPEC 9.3.3 lines 636-646; `include/adelefeld/gfunc.h`;
`docs/api-1f8.md` G1 to G4). `X` is a rational (`adf_rat_root`), an adele (`adf_adele_root`) or an idele
(`adf_idele_root`), the two last at the setting `prec`; a finite ball, a unit coset, a class or a complex adele is
`error: DOMAIN`. `N` is an integer from 0 to 2^64 - 1 (another value is `error: DOMAIN`). `SIGN` selects the branch:
`1` (the default) is the non-negative root for even `N` and the only root for odd `N`; `-1` is the non-positive
root for even `N`, in every coordinate. The result is the RATIONAL root, found by integer root tests of the
numerator and the denominator; the command does not list the adelic roots (1 has continuum many):

    root 1 with 2
    1
    root 1 with 2 with -1
    -1
    root 8 with 3
    2
    root 2 with 2
    error: DOMAIN
    root (4 ; 9/4) with 2 with -1
    (-2 ; -3/2)
    root (4 ; 4 * [1]) with 2
    (2 ; 2 * [1])

`error: DOMAIN` for a rational with no rational root (no adelic root exists either, Proposition 16), for a negative
real coordinate with even `N`, for `N = 0`, for a sign other than `1` and `-1` (and `-1` with odd `N`, except for
the zero, whose root is 0 for either sign). `N = 1` is the identity, the sign ignored. `error: NOT_DETERMINED` for
an adele whose finite part is not exactly a rational (`root (4 ; 2 mod 4) with 2`: no prime of the modulus is
examined), for a real coordinate that meets both signs with even `N`, and for an idele of finite precision
(`root (4 ; 4 * [1 mod 8]) with 2`: some unit outside the modulus has no square root). An idele with an exact unit
follows the rational contract. The library reports the failing place (the real place, or none for the finite part);
the driver prints the status only. The fixture is `tests/driver/gfunc-root.cmd`, with the reason of every expected
line in a comment.

### The series at all places (1F.8)

    exp X
    sin X
    sinh X
    cos X
    cosh X

The five factorial series at all places at once (SPEC 9.3.2 lines 588-596; `include/adelefeld/gfunc.h`;
`docs/api-1f8.md` G5, G6). `X` is an adele, or a rational `q`, read as the adele `(q ; q)` at the setting `prec`;
another type is `error: DOMAIN`. At a prime `p` the series converge exactly on `p^c Z_p` (`c = 2` at 2), so the
common finite domain contains no ball of positive radius and no rational but 0: the commands apply to an adele whose
finite part is exactly 0, with any real part. The result is the real function at `prec` bits and the exact constant
`f(0)`, 1 for `exp`, `cos`, `cosh` and 0 for `sin`, `sinh`:

    exp (0 ; 0)
    (1 ; 1)
    digits 10
    exp (0.5 ; 0)
    (1.648721271 +/- 3e-10 ; 1)

An exact finite part other than 0 is `error: DOMAIN` (the library names the first prime outside the domain: 2 for
`1`, 3 for `4`, 5 for `12`; the driver prints the status only), and so is `exp 1`. A finite part of positive radius
is `error: NOT_DETERMINED`, without a look at the primes of its modulus (`exp (0 ; 0 mod 4)`). A real part that arb
cannot evaluate is `error: NOT_DETERMINED` (`exp (1e300 ; 0)`), unless the finite part is `DOMAIN`, which is the
larger status. The fixture is `tests/driver/gfunc-series.cmd`.

### Log on ideles (1F.8, slice A)

    Log X
    logabs X
    Log_at X with PLACE
    log_abs_at X with PLACE

X is an idele. Log returns an adele with real log of X's real coordinate and finite ball
0 + 4 Zhat. The positive real coordinate is required. logabs (also spelled log_abs) uses
log of the absolute value and accepts either sign. Even exact input gives the conservative
finite ball. These commands use gfunc.h, IL1-IL5 of docs/design/idele-log.md and api-1f8.md G7-G9.

Log_at returns only the named place. At real it uses the same real log. At a prime it uses
the exact local image, capped at the setting prec: K=min(prec,E), E=max(v_p(M),d), where
M is the unit modulus and d=2 at 2, 1 at odd primes. An unrestricted prime gives p^d Z_p.
An exact unit uses prec digits, except for the proved exact local zero. log_abs_at is
real-only and gives UNSUPPORTED at a prime. The setting prec is bits at real and absolute
p-adic digits at a prime. A kind other than idele gives DOMAIN. The old log_at is the series
operation on an additive projection and keeps its meaning.

For example, at prec 5, Log_at (1 ; 4 * [1 mod 9]) with 3 prints 3: 3 + O(3^2).
At 5 it prints 5: 0 + O(5^1). Log of the same idele prints (0 ; 0 mod 4).
Golden files gfunc-log-values and gfunc-log-status use exact real log(1)=0; real log(2)
is checked by the C endpoint fixtures and the Julia example.

### All-places Log with named refinements (1F.8, slice B)

    Log_refine X with 2 3 5
    log_abs_refine X with 2 3 5
    Log_refine X with none

The places are primes separated by one space, following the existing project grammar.
Commas are PARSE; repeats and real are DOMAIN. none denotes an empty list. The alias
logabs_refine is accepted. X must be an idele. The setting prec supplies both requested
absolute finite precision N and real working bits; the C API has separate arguments.

The finite result intersects 4 Zhat with the local Log enclosures at the listed primes.
Unlisted odd primes remain Z_p; the factor 4 at 2 remains even when N<2 or 2 is absent.
An exact local zero is enclosed at N here, because a singleton at one prime cannot be a
global finite ball. Integer CRT forms a + R Zhat. At prec 5,
Log_refine (1 ; 4 * [1 mod 9]) with 3 prints (0 ; 12 mod 36).
With 3 5 the result is (0 ; 120 mod 180). An empty list has the same result as Log.
These are enclosures at all places, with the named factors refined; they contain integral
coordinates at unlisted odd primes without imposing p Z_p there. See IL5 and api-1f8.md G10-G12.

Arithmetic residue symbols (WP 1F.9, proposed N-D18):

- `legendre X with P`: P is an odd prime below 2^64.
- `jacobi X with B`: B is a positive odd integer.
- `kronecker X with B`: B is any integer for exact X; finite precision requires B > 0.

X is an integer, an integral finite ball, or a unit coset. The line is `-1`, `0`, `1`, or
`error: STATUS`. Finite certification uses the sufficient modulus in `symbol.h` and requires that
it divide the input modulus. For Kronecker at an even B it uses 8 times the odd part of B.
For example, `kronecker 1 with 2` prints `1` and `kronecker 3 with 2` prints `-1`.
`kronecker 1 mod 4 with 2` reports `NOT_DETERMINED`: 1 and 5 give different symbols.
See `docs/api-1f9.md` for the conservative refusals and the nonintegral-ball domain test.

`hilbert_at X with Y with PLACE` computes the Hilbert symbol at one prime or at `real`.
X and Y have the same kind: exact rationals, ideles, local balls (`[p=2: 3]`), or partial balls
(`{inf: -3}`, `{p=2: 1 + O(2^3)}`). A local ball must match PLACE; partial balls supply their
component there. The result is `-1` or `1`, or `error: STATUS`.
For example, `hilbert_at 3 with 3 with 2` prints `-1`.
The idele form includes the scale cofactor in its local unit square class; it uses the real
coordinate at `real`. Coarse inputs return a sign whenever it is constant on every allowed
square class, and `NOT_DETERMINED` otherwise. Zero-containing inputs are `NOT_DETERMINED`;
either exact zero makes the pair `DOMAIN`. See `docs/api-1f9.md` Y5-Y8.
# Arithmetic catalogue (WP 1F.9)

`binom X with K` returns the conservative binomial ball; `binomtight X with K` returns the smallest ball.
X is a finite ball or exact integer. K is a nonnegative integer, at most 4096 or 256 respectively.
`binomtight 0 mod 8 with 4` prints `(* ; 0 mod 2)`; either command at K=0 prints `(* ; 1)`.
A nonintegral ball gives DOMAIN when no point is integral, otherwise NOT_DETERMINED.
Oversized K gives LIMIT. These commands use no real working precision.

`profpow A with X` requires determination modulo the normal modulus of the unit coset A.
`profpowcoarse A with X` returns the largest determined divisor of that modulus.
`profpowfine A with X` returns the finest modulus overall, with its CRT centre.
X is an integral finite ball or exact integer; exact negative exponents use inverses, and 0 gives `[1]`.
`profpowfine [2 mod 5] with 2 mod 4` prints `[49 mod 120]`.
The finest variant refuses g=gcd(e,M)>256 with LIMIT (M=0 for an exact exponent), except constant exact bases.
The existing `pow` and `powtight` take signed-word exact integer exponents; the new commands admit
arbitrary-size profinite exponent data and preserve the distinctions of catalogue.h.

`haar_volume X` returns the exact rational 1/N for a finite ball of rational radius N>0,
and 0 for a point. An exact rational operand is treated as a point.
`haar_volume 1/2 mod 8/3` prints `3/8`. This calls the existing function in fball.h.

`cyclo_exp_u X with N` and `cyclo_exp_uinv X with N` return the exponent modulo the positive
integer N for the two conventions of cyclotomic action. X is an idele class; its real coordinate acts trivially.
The unit's normal modulus must be divisible by the normal form of N, else NOT_DETERMINED.
An exact unit always determines the action. `cyclo_exp_u <17 ; [2 mod 3]> with 6` prints `5`.
The exponent for N=1 is `0`; nonpositive N gives DOMAIN. The convention names are fixed by CV-53.

# Local zeta factors (WP 1F.9, lane f-slice14)

`local_zeta_factor_at S with PLACE` returns the local zeta factor of the trivial character at one place:
`(1 - p^(-s))^(-1)` at a prime p below 2^64, `pi^(-s/2) Gamma(s/2)` at `real` (library call
`adf_local_zeta_factor_at`, `include/adelefeld/localfactor.h`; `docs/api-1f9.md` Y16, Y17).
S is the text of a complex adele, used only as the carrier of the complex number s: its complex coordinate is s
and its finite coordinate is read and ignored, so `((2) + (0)*i ; 5 mod 7)` and `((2) + (0)*i ; 0)` give the
same line. The output is the complex ball `(re) + (im)*i`, printed as the complex coordinate of a complex
adele, with no finite part. `prec` is the working precision of the parse and of the call.
For example, at `prec 128` and `digits 5`, `local_zeta_factor_at ((2) + (0)*i ; 0) with 2` prints
`(1.3333 +/- 3.4e-5) + (0)*i` (4/3) and `local_zeta_factor_at ((2) + (0)*i ; 0) with real` prints
`(0.31831 +/- 1.2e-7) + (0)*i` (1/pi).
An exact pole (0 at a prime; 0, -2, -4, ... at `real`) is `DOMAIN`; a ball that meets a pole, or whose
exclusion of the poles is not certified, is `NOT_DETERMINED`; at `real` a ball whose Gamma value needs more than
64 recurrence factors is `LIMIT`. Another kind of S (a rational, a real adele) is `UNSUPPORTED`; a place token
that is not `real` or a decimal is `PARSE`, and a number that is not a prime below 2^64 is `DOMAIN`. The order of
the checks: the syntax of S, the syntax of the place token, the kind of S, the value of S, the place, the call.
Tests: `tests/driver/localfactor-values.cmd`, `tests/driver/localfactor-status.cmd`.

## The quotient class (lane q-slice1, slice 3.1-a of `docs/api-3.md`)

`qclass` is a value kind the driver holds, lift form only, and `qadd_rat X with R` translates a class by a
rational:

```text
show (0.5 ; 0) + Q
type (0.5 ; 0) + Q
qadd_rat (0 ; 1/3) + Q with -7/3
```

These print `(0.5 ; 0) + Q`, `qclass`, and `(0 ; 1/3) + Q`. Rational translation preserves
the stored lift exactly. Pair arithmetic and set queries give `DOMAIN` in this slice.
Union input is `UNSUPPORTED` after syntax and text-limit checks. Dump and the character belong to later slices.
Julia calls are in `tests/julia/qclass.jl`.

`qreduce X with LIMIT` reduces a quotient lift to a printed union of closed balls.
LIMIT is an integer and counts algorithm R's construction before rounding and duplicate removal.
An exceeded limit, including a limit below 1, gives `error: LIMIT`. The setting `prec` supplies real precision.

```text
qreduce (1 +/- 0.1 ; 0 mod 2) + Q with 2
qreduce (0.5 +/- 0.5 ; 0 mod 2) + Q with 1
```

The output encloses the represented quotient set. Its balls may spill beyond 0 and 1 (CV-45).
Union input remains `UNSUPPORTED`. Checks: `tests/driver/qclass-reduce.cmd`, `tests/julia/qclass.jl`.
A fractional finite radius A/B uses B fibres of integer radius A. For example,
`qreduce (0 ; 0 mod 1/2) + Q with 2` prints `union((0 ; 0 mod 1), (0.5 ; 0 mod 1)) + Q`.

`qneg Q with LIMIT` and `qadd Q1 with Q2 with LIMIT` (slice 3.1-f) negate a class and add two classes.
The real end points are negated or added exactly, the finite balls give `(a+b) + gcd(N,M) Zhat`, and the
result is reduced and rounded as by `qreduce`. LIMIT counts that construction over all pairs, before
rounding and duplicate removal; it is read as for `qreduce`. For example

```text
qadd (0.75 +/- 0.25 ; 0 mod 3) + Q with (0.5 +/- 0.5 ; 1 mod 3) + Q with 2
qneg (1 +/- 1 ; 0 mod 1) + Q with 2
```

print `union((0.5 +/- 0.51 ; 0 mod 3), (0.75 +/- 0.26 ; 1 mod 3)) + Q` (the sum crosses 1) and
`union((0.5 +/- 0.51 ; 0 mod 1)) + Q` (two constructed pieces, one stored; with 1 it is `error: LIMIT`).
The two operands vary independently: `qadd X with X` is the set of all sums, wider than twice X.
Checks: `tests/driver/qclass-arith.cmd`, `tests/julia/qclass_arith.jl`.

### Tate additive character (slices 3.2-a to 3.2-c)

`psi X` and `psi_strict X` take a real adele, a quotient class or a local ball. They print the ordinary complex
ball as `(re) + (im)*i`, using the settings `prec` and `digits`, as `local_zeta_factor_at` does.
The character is `E(a-m)` for an exact input `(m ; a)`, where `E(t)=exp(2 pi i t)`.
For example, `psi (0 ; 1/3)` at `prec 128`, `digits 5` prints
`(-0.5) + (0.86603 +/- 4.6e-6)*i`.

Default `psi` encloses all phases of a ball using the rectangular hull of Q4 in `docs/api-3.md`.
`psi_strict` returns `NOT_DETERMINED` for fractional finite radius. It permits real uncertainty.
Integer finite radius, including zero, passes this strict test (conventions 6.1, CV-59).
On a class, the result encloses the union of the images of all stored entries, and `psi_strict` applies the
finite-radius test to each stored entry (D3-3): `psi_strict (0 ; 0 mod 1/2) + Q` is `NOT_DETERMINED`, while
the exact two-piece reduction of that lift passes. The class text is the lift `(m ; F) + Q`; the union text
`union(...) + Q` of design section 7 is read once slice 3.1-d lands (until then the reader answers
`UNSUPPORTED`). On a local ball `[p=P: a + O(P^e)]` the character is `E(fp_p(a))`, one phase for `e >= 0` or
an exact value, all `p^(-e)` roots times it for `e < 0` (strict: `NOT_DETERMINED`): `psi [p=2: 1/6]` prints
`(-1) + (0)*i`, since `fp_2(1/6) = 1/2`.

`psi_at X with P` and `psi_strict_at X with P` take an adele and a place, named as by the other commands with
places: `real` or a prime. At `real` the factor is `E(-I)` of the real ball; at a prime `p` it is the local
character of `a + p^(v_p(N)) Z_p`. Strict is `NOT_DETERMINED` only at a prime with `v_p(N) < 0`.
`psi_at (0 ; 1/3) with 3` prints `(-0.5) + (0.86603 +/- 4.6e-6)*i`, and `with 5` prints `(1) + (0)*i`.
A token that is not a place is `PARSE` or `DOMAIN`, as for `local_zeta_factor_at`.

`psi_phase X` prints the exact angle `t` of a singleton image `E(t)` as a rational in `[0,1)`, for a finite ball,
an adele, a class or a local ball; otherwise the status (`NOT_DETERMINED` for more than one phase).
`psi_phase (0.5 ; 1/3)` prints `5/6`.

Other value kinds return `UNSUPPORTED`. Syntax and value parsing precede the library call.
The library can return `LIMIT` for precision or exact arithmetic, or `NOT_DETERMINED` if the numerical
certificate cannot be obtained. No result is printed on a failure.
Checks: `tests/driver/psi-values.cmd`, `tests/driver/psi-status.cmd`, `tests/driver/psi-class.cmd`,
`tests/driver/psi-local.cmd`, `tests/julia/psi.jl` and `tests/julia/psi_class.jl`.

### Quotient set queries (slice 3.1-e)

`qequal Q1 with Q2 with LIMIT`, `qcontains Q1 with Q2 with LIMIT`, and
`qoverlaps Q1 with Q2 with LIMIT` print 1 or 0 for represented sets in A/Q.
`qcontains` asks whether the first set is inside the second.
For example, these classes meet only at the glued endpoint:

```text
qoverlaps (0.75 +/- 0.25 ; 0 mod 3) + Q with (0.25 +/- 0.25 ; 2 mod 3) + Q with 42
```

This prints 1. With limit 41 it prints `error: LIMIT`.
LIMIT must be an integer fitting a slong. Fractional limits give DOMAIN; out-of-range limits give LIMIT.
The query uses exact stored endpoints, with no precision argument. The driver setting `prec` affects reading.
The raw exact construction, common modulus, and Q2 fibre budget must all fit the limit, even on identical inputs.
Failures print the ordinary status line. Checks: `tests/driver/qclass-sets.cmd`, `tests/julia/qclass_sets.jl`.
