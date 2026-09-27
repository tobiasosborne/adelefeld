# Lane m0-scaffold: report

Work package 0.1, provisional scaffold. Date 2026-09-27.

## What was done

The build scaffold: a test runner, one test, an empty public header, a Makefile, and the test
instructions. Nothing else. No public type, no public function, no production C file.

Files written:

- `tests/test_runner.h` (new): header-only runner, C11, libc only. `ADF_TEST(name)`,
  `ADF_CHECK(cond)`, `ADF_CHECK_MSG(cond, fmt, ...)`, a `main` that runs the registered tests,
  one line per failed check with file, line, test name, the text of the condition and the
  message, then a per-test line and a summary line; exit status 1 if any check failed.
- `tests/test_scaffold.c` (new): `flint_is_version_3_x`, `fmpz_two_plus_three_is_five`,
  `public_header_version`. It proves the include path, the link path and the runner.
- `include/adelefeld.h` (new): include guard, `ADF_VERSION_MAJOR/MINOR/PATCH` = 0/0/0, the macro
  `ADF_VERSION`, and the comment that no public type exists before milestone 0 is reviewed.
  It includes no FLINT header.
- `Makefile` (new): `all`, `check`, `clean`, `fuzz`, `mutate`, `bench`, `help`.
  Flags `-std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror`; `SAN=1` adds
  `-fsanitize=address,undefined -fno-omit-frame-pointer` to the compile and the link.
  Libraries `-lflint -lgmp -lm`. Everything under `build/`.
- `tests/README.md` (new): ten lines on how to add a test.
- `src/.gitkeep` (new): keeps `src/` in the tree; `src/` holds no `.c` file yet.

## Red-green record

Red first, with `tests/test_runner.h` and `tests/test_scaffold.c` written and no Makefile:

    $ make check
    make: *** No rule to make target 'check'.  Stop.
    exit status 2

Two more red steps came out of the work and are recorded here as they happened.

`tests/test_scaffold.c` did not compile at first (`-Werror` did its work), so `#include <string.h>`
was missing:

    tests/test_scaffold.c:22:15: error: implicit declaration of function 'strcmp'
       [-Werror=implicit-function-declaration]
    make: *** [Makefile:64: build/test_scaffold] Error 1

A second temporary probe test that used only `ADF_CHECK` did not compile either:

    tests/test_runner.h:98:1: error: 'adf_test_fail_msg' defined but not used
       [-Werror=unused-function]
    make: *** [Makefile:64: build/test_tmp_probe] Error 1

The runner now marks its own helpers `__attribute__((unused))`, so a test file that uses only
`ADF_CHECK` still builds under `-Werror`.

## Checks run, with results

1. `make clean && make check`

       == build/test_scaffold
       ok   flint_is_version_3_x
       ok   fmpz_two_plus_three_is_five
       ok   public_header_version
       3 tests, 10 checks, 0 failed checks, 0 failed tests
       check passed: all 1 test programs

   exit status 0.

2. `make clean && make check SAN=1`

       3 tests, 10 checks, 0 failed checks, 0 failed tests
       check passed: all 1 test programs

   exit status 0. No report from the address or undefined-behaviour sanitizer, no leak report.

3. Deliberate failure, to show that `make check` fails. A test
   `ADF_TEST(deliberate_failure) { ADF_CHECK(1 == 1); ADF_CHECK_MSG(2 + 2 == 5, "two plus two is
   %d, not five", 4); }` was appended to `tests/test_scaffold.c` and the file restored afterwards:

       FAIL tests/test_scaffold.c:59: deliberate_failure: check failed: 2 + 2 == 5 | two plus two is 4, not five
       FAIL deliberate_failure (tests/test_scaffold.c:56, 1 failed checks)
       4 tests, 12 checks, 1 failed checks, 1 failed tests
       check FAILED
       make: *** [Makefile:67: check] Error 1

   `make check` exit status 2 (the recipe exits 1, make reports the error as 2). Both are
   non-zero. The temporary test is removed; the final run of check 1 is green.

4. Empty `src/`. `make clean && make all` with no file in `src/`:

       mkdir -p build
       ar rcs build/libadelefeld.a

   exit status 0; `ar t build/libadelefeld.a` prints nothing. GNU ar accepts an empty member
   list, checked separately: `ar rcs libempty.a` in a scratch directory gives exit 0 and a
   8-byte archive. So the empty case needs no special rule.

5. A new `src/<x>.c` and a new `tests/test_<x>.c` are picked up without editing the Makefile.
   Temporary files `src/tmp_probe.c` (defines `adf_tmp_probe_value`, returns 42) and
   `tests/test_tmp_probe.c` (calls it, plus one deliberately failing test) were created. The
   commands and the output:

       cc -Iinclude -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror -MMD -MP -c src/tmp_probe.c -o build/tmp_probe.o
       ar rcs build/libadelefeld.a build/tmp_probe.o
       ok   probe_lib
       1 tests, 1 checks, 0 failed checks, 0 failed tests
       check passed: all 2 test programs

   The archive listed the new object (`ar t` printed `tmp_probe.o`). A failing second program
   gave `check FAILED` and exit 2, and the other programs still ran. Both temporary files were
   removed afterwards.

6. `make -j4 check` from clean: exit status 0, output as in check 1.

7. `make clean && make check CC=clang`: `3 tests, 10 checks, 0 failed checks, 0 failed tests`,
   exit status 0. The runner's `__attribute__((constructor))` and `format(printf, ...)` work
   under Clang as well. clang 18 is on this machine; GCC is 13.3.0.

8. `make fuzz`, `make mutate`, `make bench`: each prints what it will do, exit status 0.

9. `make clean`: `rm -rf build`; `ls build` then reports "No such file or directory".

10. Header dependency tracking. `touch include/adelefeld.h` and `make -n check` listed the
    recompile of `tests/test_scaffold.c`, from the `-MMD -MP` dependency files.

## Notes for the next lanes

- FLINT 3.0.1, headers under `/usr/include/flint`, so the sources use `<flint/fmpz.h>` and need
  no extra `-I`. The version macros are in `/usr/include/flint/flint.h`, lines 94 to 99:
  `__FLINT_VERSION` 3, `__FLINT_VERSION_MINOR` 0, `__FLINT_VERSION_PATCHLEVEL` 1, `FLINT_VERSION`
  "3.0.1", `__FLINT_RELEASE` 30001, `flint_version` "3.0.1". `tests/test_scaffold.c` checks all
  six, so a different FLINT fails the build rather than passing silently.
- `-lflint -lgmp -lm` is enough for `fmpz`. MPFR is installed but not linked, since nothing in
  the scaffold uses `arb` or `acb`. A later lane that uses `arb` may need `-lmpfr`; adding it to
  `LDLIBS` in the Makefile is the only change.
- `CC`, `CFLAGS`, `CPPFLAGS`, `LDFLAGS`, `LDLIBS` use `?=`, so the environment can override them.
  GNU make predefines `ARFLAGS` as `rv`, so the Makefile replaces it with `rcs` only when
  `ARFLAGS` comes from make's default.
- The test runner needs GCC or Clang: registration uses the constructor attribute, because C11
  has no portable way to run code before `main`. A file with its own `main` defines
  `ADF_TEST_NO_MAIN` before including the header.
- `make check` runs the test programs one after another, so that a failure is reported before the
  rest run; the build itself is parallel.
- `tests/ref/` and `bench/` belong to other lanes; the Makefile does not touch them.

## What is not done

- No `fuzz`, `mutate` or `bench` target does any work: each prints two lines of intent and exits 0.
- No public type or function, by design: work package 0.4 fixes the types, and the header is
  frozen only after 0.3, 0.4 and 0.6 are reviewed.
- Mutation testing, coverage measurement and the benchmark harness belong to 0.5 and later.
- No `.gitignore` entry for `build/` was added: the file is not mine, and it already lists `build/`.

## Sources pending

None. The only statement taken from outside this repository is the location and content of the
FLINT version macros, quoted from `/usr/include/flint/flint.h` lines 94 to 99 in this report and
asserted in `tests/test_scaffold.c`. `refs/` is not needed for this work package.

## Findings against the specification

None. The specification says nothing about the scaffold beyond `docs/PLAN.md` section 2, the
layout, and section 6, work package 0.1, which asks for exactly these files and states that
`make check` passes with one trivial test. The brief's requirement that `src/` "may be empty" is
met without a special case, as check 4 shows.
