# Lane m0-scaffold: work package 0.1 (provisional scaffold)

Read `docs/PLAN.md` sections 1, 2 and the row 0.1.

**You own:** `Makefile`, `include/adelefeld.h`, `src/` , `tests/test_runner.h`, `tests/test_scaffold.c`,
`tests/README.md`.

Task, in red-green order (record in the report that the test was seen to fail first, with the output):

1. `tests/test_runner.h`: a minimal header-only test runner in C11 (macros `ADF_TEST(name)`, `ADF_CHECK(cond)`,
   `ADF_CHECK_MSG`, a `main` that runs the registered tests, prints one line per failure with file and line, a
   summary, exit status 1 on any failure). No dependency besides libc.
2. `tests/test_scaffold.c`: one trivial test that links against FLINT (for example `fmpz` 2 + 3 = 5 and the FLINT
   version macro is 3.x), so that the build proves the include and link paths.
3. `include/adelefeld.h`: include guard, version macros `ADF_VERSION_MAJOR 0`, `MINOR 0`, `PATCH 0`, the comment
   that no public type exists before milestone 0 is reviewed. No types, no functions.
4. `Makefile` with targets `all` (builds `build/libadelefeld.a` from `src/*.c`, which may be empty: handle the empty
   case), `check` (builds and runs every `tests/test_*.c`, fails if any fails), `clean`, and the placeholders
   `fuzz`, `mutate`, `bench` that print what they will do and exit 0. Flags: `-std=c11 -O2 -g -Wall -Wextra
   -Wpedantic -Werror`, plus a variable `SAN=1` that adds `-fsanitize=address,undefined`. Everything is built under
   `build/`. FLINT headers are in `/usr/include/flint`; link `-lflint -lgmp -lm` (check what is needed).
   A new `tests/test_<x>.c` or `src/<x>.c` must be picked up without editing the Makefile. Each test is its own
   executable. `make -j` must work.
5. `tests/README.md`: ten lines on how to add a test.

Done when `make clean && make check` and `make clean && make check SAN=1` pass, and a deliberately failing check
makes `make check` exit non-zero (show it, then remove it).
