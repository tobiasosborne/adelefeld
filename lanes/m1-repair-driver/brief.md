# Lane m1-repair-driver: findings R7 to R15 of reviewer `surface`

Read `lanes/COMMON-C.md`. Then `docs/reviews/m1/surface/review.md` findings R7 to R15 with their
reproducers (`hostile.py`, `guard_example.cmd`, `status_order.cmd`, `exports_underscore.py`, `oracle.py`);
`docs/SPEC.md` section 15 rows M1-D1 and M1-D6; `include/adelefeld/text.h` (`ADF_PRINT_EXP_MAX`; a printer
may return NULL); `tools/adf/`, `tests/test_driver.sh`, `tests/driver/`, `tests/test_dlopen.c`,
`tests/test_exports.sh`, `tests/test_place.c`.

**You own:** `tools/adf/`, `tests/test_driver.sh`, `tests/driver/`, `tests/fuzz/fuzz_driver.c`,
`tests/test_dlopen.c`, `tests/test_exports.sh`, `tests/test_place.c` (comments only),
`lanes/m1-repair-driver/`.

Red first for each: add the command and the expected line (worked out from the specification, with the
reason in a comment of the script), see `tests/test_driver.sh` fail, then change the driver.
1. R7: the guard uses the library's rule (`ADF_PRINT_EXP_MAX` on FLINT's exponent, as `text.h` states it)
   and the driver prints `error: LIMIT` when a printer returns NULL. R8: `prec` above the usable range:
   decide from R7's rule what the largest useful `prec` is, refuse larger settings with `error: LIMIT`, and
   document it. R11: a test in which each operand is printable and the computed result is not.
2. R9: a setting accepts blanks, tabs and a CR around its number. R10: the order of the checks is what the
   README says, or the README says what the driver does; choose the order that gives the user the most
   specific status, state it in the README, and test all eight commands of `status_order.cmd`.
3. R15: the two expected lines that pin FLINT's rounding are marked as such in the script (a comment) and
   the test accepts any text whose ball encloses the exact value: if the shell test cannot do that, keep
   the exact lines and say in the README that they depend on FLINT 3.0.1.
4. R12: `tests/test_dlopen.c` builds the shared library itself when it is absent or older than the static
   library (call `tests/test_exports.sh` through `system`, or compile in the test; say which), so that
   `make check` never passes with 0 checks; all five functions are checked with `dladdr`. R13:
   `tests/test_exports.sh` judges names that begin with `_adf`. R14: the comment of `tests/test_place.c`.
5. The new operations of the library that have landed since the driver was written get commands:
   `compare` (`adf_fball_compare`, the three-valued comparison of SPEC 4.2), `dump` and `load` (the dump
   form of a value, without contexts), with tests from `tests/golden/dump.tsv`.
6. `sh tests/test_driver.sh`, `SAN=1 sh tests/test_driver.sh`, `make -j2 check`, `make clean && make -j2
   check SAN=1`, `make fuzz FUZZ_TARGET=driver FUZZ_SECONDS=60`.
