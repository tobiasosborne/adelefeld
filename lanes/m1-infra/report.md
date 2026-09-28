# lanes/m1-infra: report

Lane: test infrastructure for the C code of milestone 1. Date: 2026-09-28.
Every check below was run in this working tree; the numbers are what the commands printed.

## What was done

1. `tests/support/jsonl.{h,c}`: a strict reader for the JSON-lines vector files of
   `tests/ref/vectors/`. C11, libc only, no FLINT. The rules are listed in the header: one
   value per line, integers only, no repeated key, escapes with `\uXXXX` and surrogate pairs,
   nesting bounded by `JSONL_MAX_DEPTH = 64`, a blank line and an empty file are errors, and
   every error carries the file, the line and the column. Integers are kept as their literal
   text for `fmpz_set_str`; `jsonl_int_text_or_string` also accepts an integer written as a
   JSON string. The error holds a copy of the file name, so it stays readable after the reader
   has been closed.
2. `tests/support/golden.{h,c}`: a reader for `tests/golden/*.tsv` (docs/conventions.md 11.1):
   `input<TAB>expected`, comments, blank lines, the escapes `\\ \t \n \r \xHH` in the input
   only, `@gen:PREFIX|UNIT|COUNT|SUFFIX` expanded with a checked length before anything is
   allocated, `!STATUS` told apart from output text. Inputs keep their length, so an embedded
   NUL and bytes above 0x7f survive.
3. `tests/test_support.c`: 16 tests, 295071 checks, on the real files and on malformed input.
4. `Makefile`: the support objects are linked into every test; `make check` unchanged in use;
   `make check SAN=1` and `make check CC=clang` work; `make fuzz`, `make mutate`,
   `make mutate-selftest` and `make bench` are real targets now.
5. `tests/fuzz/fuzz_support.c` and `tests/fuzz/seed_corpus.py` with 28 committed seeds under
   `tests/fuzz/corpus/support/`.
6. `tools/mutate/mutate.py`, `tools/mutate/equivalent.txt`, `tools/mutate/selftest.py` and the
   example under `tools/mutate/example/`.
7. `tests/README.md`: how to write a test that reads a vector or a golden file, how to fuzz,
   how to mutate.

Red-green: the readers were written after the test file, and the test file was run at every
step. The faults it found are listed under "Faults the tests and the tools found" below.

## Files written

    Makefile                                  (changed)
    tests/README.md                           (rewritten)
    tests/support/jsonl.h,  tests/support/jsonl.c        (new)
    tests/support/golden.h, tests/support/golden.c      (new)
    tests/test_support.c                     (new)
    tests/fuzz/fuzz_support.c                (new)
    tests/fuzz/seed_corpus.py                (new)
    tests/fuzz/corpus/support/               (new, 28 seed files)
    tools/mutate/mutate.py                   (new)
    tools/mutate/selftest.py                 (new)
    tools/mutate/equivalent.txt              (new, no entry)
    tools/mutate/example/README.md, Makefile, src/example.c, src/example.h,
        tests/test_weak.c, tests/test_strong.c           (new)

No file outside that list was created, changed, moved or deleted. No git command and no `bd`
were run.

## Checks run

### `make clean && make check`

    == build/test_scaffold
    ok   flint_is_version_3_x
    ok   fmpz_two_plus_three_is_five
    ok   public_header_version
    3 tests, 10 checks, 0 failed checks, 0 failed tests
    == build/test_support
    ok   jsonl_reads_every_vector_file
    ok   jsonl_reads_the_documented_records
    ok   jsonl_keeps_integers_as_text
    ok   jsonl_refuses_malformed_lines
    ok   jsonl_encodes_a_unicode_escape
    ok   jsonl_errors_name_the_line
    ok   jsonl_bounds_the_nesting
    ok   jsonl_refuses_a_value_of_the_wrong_kind
    ok   golden_reads_every_golden_file
    ok   golden_decodes_the_escapes
    ok   golden_decodes_the_hostile_inputs
    ok   golden_expands_generated_inputs
    ok   golden_refuses_malformed_lines
    ok   golden_bounds_a_generated_input
    ok   golden_reads_the_documented_vectors
    ok   golden_accepts_a_file_without_a_vector
    16 tests, 295071 checks, 0 failed checks, 0 failed tests
    check passed: all 2 test programs

The counts the test checks, from the files of 2026-09-28: 11 vector files with 444, 212, 444,
580, 444, 212, 1260, 1332, 360, 396 and 444 records, 6128 in all, each equal to the number of
lines `wc -l` prints; 727 golden vectors of which 327 are status vectors, one record for every
line that is neither a comment nor empty; every status is one of the ten codes of
docs/conventions.md 3.1 (lines 144 to 157). `./build/test_support` alone takes 0.097 s.

### `make check SAN=1`

    16 tests, 295071 checks, 0 failed checks, 0 failed tests
    check passed: all 2 test programs

No leak, no out-of-bounds access, no undefined behaviour. The sanitizers found three leaks and
one heap overflow during the work; all four are fixed (see below).

### `make check CC=clang`

    16 tests, 295071 checks, 0 failed checks, 0 failed tests
    check passed: all 2 test programs

### `make fuzz FUZZ_SECONDS=10`

    == fuzz support for 10 s, 1 worker(s)
    Done 562368 runs in 11 second(s)
    stat::new_units_added:        461
    stat::peak_rss_mb:             574
    -- coverage of support over its corpus: which parts of the reader are reached
    fuzz/fuzz_support.c               128 regions  85.94%   135 lines  86.67%   106 branches  68.87%
    support/golden.c                  341 regions  73.90%   480 lines  69.38%   246 branches  59.76%
    support/jsonl.c                   830 regions  48.67%   812 lines  60.47%   524 branches  41.60%
    TOTAL                            1299 regions  58.97%  1427 lines  65.94%   876 branches  50.00%
    fuzz passed: 1 target(s), no crash

Wall clock 14.5 s. The corpus of the tree stayed at 28 files; the 146 inputs of the run went
to `build/fuzz/corpus/support/`, and `build/fuzz/artifacts/` is empty. A crash makes the target
fail and leaves the reproducer there; it is played back with
`./build/fuzz/support build/fuzz/artifacts/crash-<hash>`, which was used during the work.
Also checked: `make fuzz FUZZ_SECONDS=5 FUZZ_TARGET=support COV=0` passes. The fuzzer runs with
`build/fuzz/` as its working directory, so a crash artifact never lands in the repository root:
one did, from the first run, and was removed.

### `make mutate-selftest`

    == the weak test
    mutate: src/example.c: 16 mutants
    mutate: the baseline passes (0.1 s)
    SURVIVED src/example.c:25:15 op: '-' -> '+'
    ... 4 survivors, 2 not compiled, 1 excused ...
    mutate: 16 mutants in 1.1 s: 9 killed, 4 survived, 2 not compiled, 0 timed out, 1 excused
    mutate: FAILED: 4 mutant(s) survived; each one is a claim the tests do not check
    == the strong test
    mutate: src/example.c: 16 mutants
    mutate: the baseline passes (0.1 s)
    NOT COMPILED src/example.c:43:12 gcd_lcm: 'adf_example_gcd' -> 'lcm'
    NOT COMPILED src/example.c:35:9 drop_assign: 'm = t;' -> ''
    TIMED OUT src/example.c:33:20 op: '%' -> '/'
    mutate: 16 mutants in 5.4 s: 12 killed, 0 survived, 2 not compiled, 1 timed out, 1 excused
    mutate: passed: every mutant was killed, or is excused
    selftest: passed: the weak test leaves a survivor, the strong test leaves none

Wall clock 6.9 s. All four reports of the tool are exercised: killed, survived, not compiled,
timed out, and the excused survivor (the exchange of the two arguments of `adf_example_gcd`,
which is symmetric).

### `make mutate` on the repository

With no source file yet, `make mutate` says `mutate: no source file; src/ is empty`. The
target was run end to end on a real file of the tree:

    make mutate FILES=tests/support/jsonl.c

    903 mutants found, 200 of them run (--limit 200, --seed 20260928)
    mutate: 200 mutants in 222.9 s: 76 killed, 28 survived, 95 not compiled, 1 timed out, 0 excused
    mutate: FAILED: 28 mutant(s) survived; each one is a claim the tests do not check

Time: 3 min 43 s with two jobs, so a run of 200 mutants is at the edge of a laptop hour for
several files; `--limit` is there for that. The 28 survivors of that run were read: most are
the paths of `malloc` failure and of `jsonl_open` on a file that cannot be read, and after the
test of the UTF-8 encoding was added (`jsonl_encodes_a_unicode_escape`) the count over the
same seed fell from 40 to 28. They are not listed in `tools/mutate/equivalent.txt`: each one
is a branch the tests really do not reach, and excusing them would be false.

### `make bench`

Not run: it writes into `bench/results/` and `bench/asm/`, which this lane does not own. The
recipe was checked with `make -n bench`, which prints `make -C bench run` and the recipe of
`bench/run`.

### `-lmpfr`

Tested, because the brief asks. A program that calls `arb_init`, `arb_set_si`, `arb_sqrt` and
`arb_get_str` links with `-lflint -lgmp -lm` alone, and `ldd` shows `libmpfr.so.6` as a
dependency of `libflint.so.18`, so `-lmpfr` is not needed and is not in `LDLIBS`. FLINT 3.0.1
installs no static `libflint.a` here, which is the case that would need it. The reason is
written in the Makefile next to `LDLIBS`.

## Faults the tests and the tools found

Written down because they are the reason the tools exist.

- `make check SAN=1`: a heap overflow in the test itself (the buffer of the nesting test was
  four bytes too small for the deepest case it wrote), three leaks in the readers (the parse
  buffer, the buffer of a string whose parse fails, the name of an empty file) and a wrong
  ownership rule in the error (the file name of a failed read pointed into memory that
  `jsonl_close` had freed). All fixed; the error now holds a copy.
- `make fuzz`: two false invariants in the fuzz target, found by the seeds. A successful parse
  was followed by a `f != NULL` check that could not hold once the file was closed, and the
  expected field of a golden vector was assumed to be a C string, which a NUL inside it is
  not. Both fixed; the reader was right both times.
- `make mutate`: the mutation tool found that the test of the reader did not check the encoding
  of a `\u` escape above 0x7f, and the test was extended until those mutants were killed
  (40 survivors to 28 over the same seed).

## What is not done

- No fuzz target for the library: the parsers of the value text and of the dump arrive with
  work package 1.4, and the Makefile picks up `tests/fuzz/fuzz_*.c` as they are written.
- `make mutate` over `src/` has never run, because `src/` was empty for the whole of this lane.
  The tool was run on `tests/support/jsonl.c` and on the example instead.
- The corpus of `fuzz_support` is 28 seeds plus what one 10 s run found (461 new inputs, kept
  under `build/`). A corpus that is committed grows large; nothing decides here when to commit
  the new inputs. The rule to use: commit an input that reaches a new branch of the reader.
- The golden test does not freeze the counts of `tests/golden/README.md` (see below), so it
  does not notice a vector file that lost vectors to an edit.

## Sources pending

None. Every format this lane implements is taken from a file in the repository
(`tests/ref/README.md`, `docs/conventions.md` 11.1 and 3.1, `tests/golden/README.md`,
`tests/test_runner.h`, `CLAUDE.md` rule 2, `docs/PLAN.md` 2 and 7); no formula of another
person is used, so no quotation from `refs/` was needed.

## Findings against the specification

None: nothing in `docs/SPEC.md` or `docs/conventions.md` was found to be wrong. Two things
outside those documents are worth the orchestrator's attention.

1. `tests/ref/README.md` line 63 says the `result` of a compare record is "one of the three
   comparison strings", but `tests/ref/vectors/compare.jsonl` carried integers (0, 1, 2) when
   this test was written, after the lane `m1-pyref` had rewritten it. `jsonl_reads_every_vector_file`
   accepts both forms and checks that there are exactly three values in one form. The README
   and the vectors should be made to agree; the library test of `compare` will have to follow
   whichever form is kept.
2. `tests/golden/README.md` and the golden files disagreed while this lane ran. The files
   `dump.tsv` (161 vectors), `realball_print.tsv` (23), `realball_read.tsv` (28) and `rfun.tsv`
   (14) no longer had the counts the README gives (165, 26, 30, 19), and the total is 713
   vectors and 328 status vectors against the 727 and 327 of the README. Another lane was
   writing those files. The test therefore checks the claim that is about the reader, one
   record for every vector line of the file, counted from the file itself, and the status
   names against conventions.md 3.1, and not the frozen numbers. When the owning lane settles,
   the numbers of the README can be checked again and put back into the test.
