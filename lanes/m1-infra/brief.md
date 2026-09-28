# Lane m1-infra: test infrastructure for the C code of milestone 1

Read `CLAUDE.md` (rules 1 and 2), `docs/PLAN.md` sections 1, 2, 7, the `Makefile`, `tests/test_runner.h`,
`tests/README.md`, `tests/ref/README.md` (format of the JSON-lines vectors under `tests/ref/vectors/`),
`tests/golden/README.md` and `docs/conventions.md` section 11 (format of the golden vectors and how a C test uses
them).

**You own:** `Makefile`, `tests/support/` (new), `tests/test_support.c`, `tools/mutate/` (new), `tests/fuzz/`
(new), `tests/README.md`.

The library code of milestone 1 will be written by other lanes after you; you build what they will test with.
Everything red-green: the test first, see it fail, then the code.

1. `tests/support/jsonl.h` and `jsonl.c`: a small, strict reader for the JSON-lines vector files (objects with
   string, integer-as-string, boolean and null values, and nested arrays or objects only as far as the vector
   files use them: read the files to see). Big integers and rationals are kept as strings for the caller to
   pass to `fmpz_set_str` / `fmpq_set_str`. No dependency besides libc. Errors are reported with file and line,
   never ignored. `tests/support/golden.h`, `golden.c`: reader for the tab-separated golden files (input, expected
   output or status; escapes as the README defines them, including embedded NUL and non-ASCII bytes).
   `tests/test_support.c` tests both readers on the real files (counts of records per file match `wc -l` or the
   README's counts) and on malformed input.
2. `Makefile`: support objects are linked into every test; `make check` as before. Add `-lmpfr` if `arb` needs
   it (test it). Targets:
   - `make check SAN=1` (exists), `make check CC=clang`.
   - `make fuzz`: builds each `tests/fuzz/fuzz_*.c` with clang's libFuzzer and the address and undefined
     sanitizers (`-fsanitize=fuzzer,address,undefined`), runs each for `FUZZ_SECONDS` (default 30) on at most 2
     cores, with the corpus under `tests/fuzz/corpus/<name>/` seeded from the golden vectors; a crash fails the
     target and leaves the reproducer under `build/fuzz/`. Provide `tests/fuzz/fuzz_support.c` as the first
     target: it fuzzes your own two readers. Coverage of a fuzz run is reported with `llvm-cov` if installed;
     otherwise say how to get it.
   - `make mutate`: see 3.
   - `make bench`: calls `make -C bench run`.
3. `tools/mutate/mutate.py`: mutation testing for C without external tools. For a given source file it generates
   mutants by single token changes (arithmetic operators, comparison operators and their boundaries, `gcd` against
   `lcm` function names, swapped arguments of commutative-looking calls, constants 0 and 1, removed statements of
   the form `x = ...;`, negated conditions), one at a time; builds the library and the tests with each mutant in
   a scratch directory under `build/mutate/` (never touching the source tree); runs the tests with a timeout;
   reports killed, survived (with file, line and the change, as a unified diff), not compiled, timed out.
   Options: the files to mutate, a limit on the number of mutants, a seed, the number of parallel jobs (default
   2). `make mutate FILES=src/x.c` runs it and fails if any mutant survives, unless the mutant is listed with a
   reason in `tools/mutate/equivalent.txt`. Test the tool itself on a small example under `tools/mutate/example/`
   with a function and a deliberately weak test: the tool must find the surviving mutant; with the test
   strengthened it must find none.
4. `tests/README.md`: how to write a test that reads vectors, how to run fuzzing and mutation testing.

Done when `make clean && make check`, `make check SAN=1`, `make check CC=clang`, `make fuzz FUZZ_SECONDS=10` and
the self-test of the mutation tool all pass; show the outputs.
