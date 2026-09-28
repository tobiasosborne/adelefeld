# Lane m1-driver: the command-line driver `adf` (work package 1.5)

Read `lanes/COMMON-C.md`. Then every header under `include/adelefeld/` except `dump.h`, `scaled.h`,
`modctx.h`; `docs/api-m1.md`; `docs/conventions.md` sections 3 (statuses and their names), 8, 9 (the value
form; 9.7 the type of a text); `docs/SPEC.md` sections 4.1 to 4.5 (the tables are your acceptance test) and
10; `docs/PLAN.md` row 1.5; `lanes/m1-text/report.md` (header findings on the printers).

**You own:** `tools/adf/` (the program: `tools/adf/adf.c`, `tools/adf/Makefile`, `tools/adf/README.md`),
`tests/test_driver.sh`, `tests/driver/` (scripts and expected outputs), `tests/fuzz/fuzz_driver.c`,
`tests/fuzz/corpus/driver/`. You do not edit the top-level `Makefile`; `tools/adf/Makefile` builds
`build/adf` from `tools/adf/adf.c` and `build/libadelefeld.a`.

The driver is a thin program over the public interface: it parses nothing of the value form itself; every
value is read by `adf_text_classify` and the typed parser, and printed by the typed printer.

**Language (decided by the orchestrator, M1-D1; keep it this small).** Input is read line by line from
standard input or from a file named on the command line. A line is at most 65536 bytes; a longer line is an
error for that line (`LIMIT`) and the rest of the line is skipped. Empty lines and lines starting with `#`
are ignored. Every other line is one command:

    <operation> <operand> [ <separator> <operand> ]

The separator is a token that cannot occur in any value text: read the alphabet of conventions 8.2 and choose
one or two bytes outside it; if no printable byte is outside it, use the word ` with ` surrounded by spaces
and show in the README why it is unambiguous against the grammar of 9.2. Operations: `show` (parse and print
canonically), `type` (the kind from `adf_text_classify`), `add`, `sub`, `mul`, `neg`, `div` (the second operand
an exact rational), `equal` (equality of sets, prints `true` or `false`), `contains`, `overlaps`,
`reconstruct` (an adele, or a finite ball and an interval given as two rationals: design the operand form and
document it), `cap` (a finite ball and a rational). Settings, one per line: `prec <bits>`, `digits <n>`.
Operands of different types are combined only where SPEC 4.1 defines it (an exact rational with a finite
ball, an adele, a complex adele; an adele with a complex adele); everything else is `DOMAIN`.

Output: one line per command on standard output, either the value text or `error: <STATUS NAME>` with the
name from `adf_status_str`. The exit status is 0 if no command failed, 1 otherwise, 2 for a usage error.
Nothing else is printed unless `-v` is given.

Tests (`tests/test_driver.sh`, run from the repository root, exit non-zero on the first difference):
- every row of the tables of SPEC 4.2, 4.3 and the examples of 4.1, 4.4 (cap only), 4.5, typed as commands,
  with the expected line written by you from the SPEC text and not from the output of the program; say in
  the report how many rows each table has and how many you ran;
- every operation with each pair of operand types, including the refused ones;
- every status the library can return through the driver;
- hostile input: a NUL byte in a line, bytes 128 to 255, a line of 65536 and 65537 bytes, a file without a
  final newline, an empty file, 100000 lines (time it), a file that is a directory;
- the run under the sanitizers: build the driver with `-fsanitize=address,undefined` and run the same script.

Fuzzing: `tests/fuzz/fuzz_driver.c` feeds the input as a script to the function that executes a script (so
write the driver as a function `adf_driver_run(const char * text, size_t len, FILE * out)` plus a small
`main`); run `make fuzz FUZZ_TARGET=driver FUZZ_SECONDS=120` and report executions and coverage. Guard against
the printer finding of lane m1-text: the driver refuses to print a real ball whose binary exponent is above
100000 in absolute value (`error: LIMIT`), and the README says so.

No mutation run is asked for (the mutation tool covers `src/` only). No benchmark.
