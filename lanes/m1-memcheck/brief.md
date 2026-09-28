# Lane m1-memcheck: find uses of uninitialised values in tests and library (issue adf-tgx)

Background: `tests/test_recon.c` used five `fmpq` values without `fmpq_init` and crashed in some runs; the
address and undefined-behaviour sanitizers do not see such an error. Read `lanes/COMMON-C.md`,
`tests/README.md`, the `Makefile`, `lanes/m1-cap-chain/report.md` (last sections).

**You own:** `tools/memcheck/` (a script `tools/memcheck/run.sh` and a `README.md`), `lanes/m1-memcheck/`.
Everything else is read-only: you report defects, you do not repair them.

1. Find out what is installed: `valgrind`, `clang` with `-fsanitize=memory`. No installation of packages. If
   valgrind is there, `tools/memcheck/run.sh` builds the tests without sanitizers at `-O1 -g` into
   `build-memcheck/` (a directory that git ignores; use `make BUILD=...` only if the Makefile allows it,
   otherwise compile in the script) and runs every test program under
   `valgrind --error-exitcode=9 --track-origins=yes --leak-check=full -q`, at most 2 at a time, with a time
   limit per program (say which), and prints one line per program: passed, errors (with the first error),
   or timed out. If only MemorySanitizer is there: FLINT and GMP are not instrumented, so every value that
   comes out of FLINT looks uninitialised; say whether a useful run is possible and do not fake one.
   If neither is there: write a checker in Python in `tools/memcheck/` that reads the C of `tests/` and
   `src/` and reports every local variable of a FLINT or adelefeld type (`fmpz_t`, `fmpq_t`, `arb_t`,
   `acb_t`, `arf_t`, `mag_t`, `adf_*_t`) for which no `*_init*` call with that variable occurs in the
   function before its first other use; test the checker on the old version of `tests/test_recon.c`
   (`git show cb268ba^:tests/test_recon.c`), where it must find the five variables, and on the new one,
   where it must find none.
   Write the Python checker in any case: it is cheap and runs in seconds.
2. Red-green for the tool: it must report the old `tests/test_recon.c` and pass the new one.
3. Run the tool over everything. Report every finding with file, line, variable, and whether you confirmed
   it by reading the code. Clear-without-init and init-without-clear (a leak) are findings too.
