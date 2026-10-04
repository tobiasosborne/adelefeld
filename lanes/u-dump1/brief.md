# Lane u-dump1: dump forms of the unit coset, the idele, the idele class, the local ball and the partial ball

`docs/conventions.md` section 10 (lines 1382-1494) defines the dump form `adf1 Q <body>` for every type. The
library implements it for `rat`, `fball`, `scaled`, `adele`, `cadele`, `modctx` (`include/adelefeld/dump.h`,
`src/dump.c`, 1855 lines; tests `tests/test_dump.c`, `test_dump_ctx.c`, `test_dump_golden.c`,
`test_dump_limits.c`; golden vectors `tests/golden/dump.tsv`). For five types that exist since then there is
no dump: `adf_ucoset` (body `ucoset c N`), `adf_idele` (body `idele <arch> num(r) den(r) c N`), `adf_idclass`
(body `idclass <arb> c N`), `adf_lball` (body `lball <lb>`), `adf_sball` (body `sball (n | r <arb> | c <acb>)
<count> {<lb>}`). The grammar is at conventions 10.1 (lines 1384-1426), the rules at 10.2: read both
completely, and section 5 for the canonical predicate of each type (5.6 to 5.9), 8.4, 8.5 (limits and the
order of the checks of a reader). The driver answers `error: UNSUPPORTED` to `dump` of these kinds
(`tools/adf/adf.c`, `adf_drv_body_slot`, line 527).

This lane also follows `lanes/COMMON-C.md` (read it), with one difference: you ADD declarations to
`include/adelefeld/dump.h` (three per type, in the style and with comment blocks of the `adf_rat` group, lines
83-91: `adf_x_load_str(x, s, len, ctx, lim)` where `ctx` is accepted and ignored because these bodies have no
context occurrence, `adf_x_dump_str(len, x)`, `adf_x_dump_inspect(nctx, descs, s, len, lim)` which writes 0;
`adf_x_load_str_binds` too if the `rat` group has it). An idele and a class need no working precision: the
real ball is stored in the dump exactly.

**The work, one type at a time, in this order: `ucoset`, `idele`, `idclass`, `lball`, `sball`.** For each type:
1. RED: a test in a new file `tests/test_dump_units.c` (`ucoset`, `idele`, `idclass`) or
   `tests/test_dump_local.c` (`lball`, `sball`), registered as the other test programs are (read `Makefile`):
   - every row of `tests/golden/dump.tsv` with that body (read `tests/golden/README.md` for the columns; if the
     file has no row for the body, say so in the report and write at least twelve vectors by hand from the
     grammar into `tests/ref/vectors/u-dump1/<type>.tsv`, each with a comment line saying how it was derived:
     NOT from the output of your code);
   - round trips on 2000 random canonical values built through the public constructors (`ucoset.h`, `idele.h`,
     `idclass.h`, `lball.h`, `sball.h`), integers up to 2000 bits: `load(dump(v))` is identical to `v` (the
     library's `identical` predicate of the type if there is one, else equality of every field) and
     `dump(load(t)) = t` byte for byte;
   - strictness (10.2): for each field, a text that is well formed but not canonical (a centre not reduced, a
     modulus in the wrong form for the unit coset: read 5.6 and "Unit moduli are dumped as stored"; `den(r) =
     0` or a fraction not in lowest terms; a non-prime `p`; `v` and `N` inconsistent; primes of a partial
     ball not in canonical order or repeated; an `arb` with an even mantissa or a non-finite one; an
     archimedean count other than 1) gives `ADF_DOMAIN` and leaves the output untouched; a missing or an extra
     token, an upper-case hexadecimal digit, a leading zero, two spaces, a trailing space give `ADF_PARSE`;
     version `adf2` and field `K` give `ADF_UNSUPPORTED`; the limits of the limits struct (`max_len`,
     `max_prec`, `max_items`: which apply to which field, from 8.4) give `ADF_LIMIT`; the order of these
     checks as 8.5 states it, tested with texts that violate two rules at once;
   - `adf_x_dump_inspect` returns the loader's status for every text above and writes `*nctx = 0` on `OK` only.
   Run it, see it fail by an assertion (a link error counts only for the first test of a file).
2. GREEN: the code at the end of `src/dump.c`, reusing its static helpers (token reader, hexadecimal
   reader and writer, the `arb` validation of 10.2 and CV-52: the whole text is validated before any FLINT
   load function sees a byte). Do not copy a helper and do not write a second one of the same purpose.
3. The driver: `dump X` prints the dump of a value of the kind and `load <text>` (or what the driver's command
   is: read `tools/adf/README.md` and fixture `tests/driver/13_dump.cmd`) reads it. New fixtures
   `tests/driver/u-dump-<type>.cmd` and `.out`, expected lines written from the grammar by hand, with the
   first line `#!exit N` as `tests/driver/README.md` says. Existing fixtures that now answer differently
   (`13_dump` line `dump (0.5 ; 0) + Q` stays; look for `dump` of a unit coset, an idele or a class in
   `tests/driver/i-*.cmd`): do NOT edit them; list fixture, line, old and new text in the report.
4. A note in `lanes/u-dump1/progress.md`: the red run, the green run, the counts.

If the time is short, stop after a type is complete (code, test, driver): three complete types are worth more
than five half ones. Say in the report which are done.

**You own:** `src/dump.c`, `include/adelefeld/dump.h`, the two new test files, new files under
`tests/ref/vectors/u-dump1/`, new fixtures `tests/driver/u-dump-*`, `tools/adf/adf.c`, `tools/adf/README.md`,
the lines of `Makefile` and of the export list (`tests/test_exports.sh` or the file it reads) that register
your tests and your functions, `lanes/u-dump1/`. Everything else is read-only, in particular the other test
files, the golden vectors and the type headers. If a type header lacks an accessor you need (a field you
cannot read through the public interface), do NOT reach into the struct from `dump.c` unless `dump.c` already
does so for the other types (then follow it and cite the line); otherwise leave that type out and name the
missing declaration in the report.

**Checks at the end**, each under `timeout`, at most 2 jobs, command and numbers in the report:
`timeout 1500 make -j2 check` (all test programs; the count before and after); `timeout 900 sh
tests/test_driver.sh`; `sh tests/test_exports.sh`; then your two test programs and the driver built with
`CC=clang`, with `INV=1` and with `SAN=1` in build directories under `lanes/u-dump1/` (`make -j2
BUILD=lanes/u-dump1/build-san SAN=1 lanes/u-dump1/build-san/test_dump_units` and so on): no warning, no
sanitizer report. Files end with a newline; no line over 116 characters. Mutation testing of `src/dump.c` as
`lanes/COMMON-C.md` rule 5 says, restricted to the lines you added if the tool has an option for a line range
(read `tools/mutate/README.md`), at most 40 mutants and 15 minutes; survivors listed with one line each.
Remove your build directories at the end; leave no files in `/tmp`.

Report: `lanes/u-dump1/report.md` (rule 8 of `lanes/COMMON.md`): per type what is done; the declarations
added; decisions you had to take where section 10 is silent (each with the alternative); findings against
`docs/conventions.md` section 10 (a body that cannot hold a canonical value, or two texts for one value, is a
finding with the example).
