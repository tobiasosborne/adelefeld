# Lane m-tool2: three defects of the mutation tool (`tools/mutate/mutate.py`)

The mutation tool mutates C source lines and runs the tests against each mutant. Two lanes reported three defects
and did not repair them (`lanes/f-slice9/result.md`, "Defects of the mutation tool";
`lanes/f-repair3/result.md` line 121). You repair them, test first. This is Python only; no C is written.

Read first: `tools/mutate/mutate.py` (the docstring at the top, lines 1 to 60, states what each status means;
the argument handling near lines 975 to 1080; the summary and exit code near lines 1100 to 1136; `swap_args`
near line 670), `tools/mutate/selftest.py` (how the tool is tested: runs over `tools/mutate/example/` and
`test_generation_rules()`), the `mutate-selftest` target of `Makefile` (line 258).

**You own:** `tools/mutate/mutate.py`, `tools/mutate/selftest.py`, `tools/mutate/example/` (only if a new test
needs a file there), `lanes/m-tool2/`. Everything else is read-only. No git command that changes state, no `bd`.
At most 2 cores; every command under `timeout` (for example `timeout 600 make mutate-selftest`).

## The three defects

1. **`--san` is defeated by `ASAN_OPTIONS`.** `mutate.py` line 1066: `if args.san and "SAN" not in args.command`.
   A `--make` command such as `ASAN_OPTIONS=detect_leaks=0 make -s -j2 check` contains the letters "SAN", so
   `SAN=1` is not set, the mutants are built without sanitizers against a sanitized archive, and none compiles.
   The test must be whether the command sets the make variable `SAN` (a word `SAN=...` as its own token, at the
   start of the command or after whitespace), not whether the three letters occur.
2. **A run in which mutants did not compile is reported as passed.** With 60 of 60 mutants "not compiled" the
   tool printed "mutate: passed: every mutant was killed" and exit code 0
   (`lanes/f-slice9/mutate-run1-notcompiled.log`). A mutant that does not build was not tested. Decide and state
   in the docstring: the summary line must not say "every mutant was killed" when a mutant was not compiled;
   when MORE THAN HALF of the mutants (or all of them) did not compile, the run fails with a non-zero exit code
   and a message that names the likely cause (the build command, not the mutants). A few not-compiled mutants
   among many are normal (a mutant can be a type error) and do not fail the run, but the last line gives their
   count.
3. **`swap_args` offers a mutant identical to the original.** For `fmpz_mul(x2, x, x)` the two exchanged
   arguments are the same text, the mutant is the original line, it always survives and costs a run. Do not
   generate a `swap_args` mutant whose two arguments are equal after stripping whitespace.

## Order of work (red then green; keep `lanes/m-tool2/redgreen.log`)

1. Tests first, in `tools/mutate/selftest.py`, each seen to FAIL before the repair:
   - for 3: in `test_generation_rules()`, a snippet with `fmpz_mul(x2, x, x)` yields no `swap_args` mutant, and
     `fmpz_mul(x2, x, y)` still yields one;
   - for 1: the decision "does this command set SAN" as a small function of `mutate.py` that the self-test calls
     directly: `ASAN_OPTIONS=detect_leaks=0 make check` does not set it; `make check SAN=1` does;
     `SAN=1 make check` does; `make check UBSAN_OPTIONS=x` does not; `make -s -j2 check INV=1` does not;
   - for 2: a run over `tools/mutate/example/` with a `--make` command that cannot build (for example a
     compiler flag that does not exist, passed the way the example's build takes flags: read how the self-test
     invokes the tool) must end with a non-zero exit code and must not print "every mutant was killed".
2. The repairs in `mutate.py`; the docstring at the top updated where it states the old behaviour.
3. Checks: `timeout 600 make mutate-selftest` (give its last lines and exit code);
   `python3 tools/mutate/mutate.py --help` still works; one real run:
   `timeout 900 python3 tools/mutate/mutate.py --files src/lpow.c --limit 6 --seed 1 --jobs 2 --san --timeout 150`
   (if these option names differ, read `--help` and use the right ones): give its last three lines and say how
   many of the 6 compiled.

Report: `lanes/m-tool2/report.md`, written once, at the end: what was changed (function and line), the red and
green runs, the checks with their commands and output lines, what is not done. Your work counts as finished only
when this file exists.
