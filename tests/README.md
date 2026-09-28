# tests: how to add a test

1. Name the file `tests/test_<module>.c`, one test program per file; the Makefile picks it up.
2. Include `"test_runner.h"` last: it supplies `main`, and it must come after `<adelefeld.h>`.
3. Write the tests as `ADF_TEST(name) { ... }`; names must be unique within the file.
4. Assert with `ADF_CHECK(cond)`; add a message with `ADF_CHECK_MSG(cond, "fmt", ...)`.
5. Each assertion states the claim, not the function that made it: `ADF_CHECK(a == b)`, not a boolean.
6. A test is not a test if a wrong implementation would pass it (`docs/PLAN.md` section 7).
7. Use FLINT directly for the exact cases; state the precision when the claim needs one.
8. Run `make check`; it builds every test, runs it, and exits 1 if any check failed.
9. Run `make clean && make check SAN=1` before committing anything that touches memory, and
   `make clean && make check CC=clang` once, since the tests are built by two compilers.
10. Reference data goes to `tests/golden/`, the Python reference to `tests/ref/`; neither is compiled.

A test runs from the repository root, which is what `make check` does, so that the paths
`tests/golden/...` and `tests/ref/vectors/...` resolve.

## Reading a vector file or a golden file

The two readers of `tests/support/` are linked into every test; they are not part of the
library and no public header includes them. Each accessor that can fail takes a
`<reader>_error_t *` and fills it, and a test that passes NULL throws the reason away, which
is not done in `tests/test_support.c` and should not be done elsewhere.

### `tests/ref/vectors/*.jsonl`: `tests/support/jsonl.h`

One JSON object per line (`tests/ref/README.md`, "Vector format"). The reader is strict: it
takes exactly what those files use, and every error carries the file, the line and the column.
Integers are kept as text and given to `fmpz_set_str`, so a vector file with a thousand-digit
integer is read without loss.

```c
#include <flint/fmpz.h>
#include "support/jsonl.h"
#include "test_runner.h"

ADF_TEST(the_sum_of_two_balls)
{
    jsonl_error_t err;
    jsonl_file *f = NULL;
    size_t i;

    ADF_CHECK_MSG(jsonl_open("tests/ref/vectors/add.jsonl", &f, &err) == 1, "%s",
                  jsonl_error_message(&err));
    if (f == NULL)
        return;
    for (i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value *rec = jsonl_record(f, i);
        const jsonl_value *a, *b, *result, *A;
        fmpz_t h;
        adf_fball_t x, y, z;
        char *expected = NULL;

        ADF_CHECK(jsonl_field(rec, "a", &a, &err) == 1);
        ADF_CHECK(jsonl_field(rec, "b", &b, &err) == 1);
        ADF_CHECK(jsonl_field(rec, "result", &result, &err) == 1);
        ADF_CHECK(jsonl_field(a, "H", &A, &err) == 1);
        fmpz_init(h);
        ADF_CHECK(fmpz_set_str(h, jsonl_int_text(A, &err), 10) == 0);
        ...                                    /* build x, y; add them into z; compare with result */
        fmpz_clear(h);
    }
    jsonl_close(f);
}
```

What the reader gives: `jsonl_count`, `jsonl_record`, `jsonl_field` (a missing key is an
error, not a zero value), `jsonl_at`, `jsonl_size`, `jsonl_key`, `jsonl_string` (with its
length), `jsonl_int_text`, `jsonl_int_text_or_string` (an integer written as a string),
`jsonl_bool`, `jsonl_is_null`, `jsonl_is`, `jsonl_kind`, `jsonl_kind_name`, `jsonl_line_of`.

### `tests/golden/*.tsv`: `tests/support/golden.h`

One `input<TAB>expected` per line (`docs/conventions.md` 11.1). The escapes of the input are
decoded, including `\xHH`, so an input may hold a NUL or a byte above 0x7f: read `input_len`,
not `strlen`. A line of the form `@gen:PREFIX|UNIT|COUNT|SUFFIX` is materialised, and a line
longer than `max_input` is refused before anything is allocated. A record with `is_status` set
carries the status name without the `!` in `status`.

```c
#include "support/golden.h"
#include "test_runner.h"

ADF_TEST(the_value_text_of_rat)
{
    golden_error_t err;
    golden_file *f = NULL;
    size_t i;

    ADF_CHECK_MSG(golden_open("tests/golden/rat.tsv", GOLDEN_DEFAULT_MAX_INPUT, &f, &err) == 1,
                  "%s", golden_error_message(&err));
    if (f == NULL)
        return;
    for (i = 0; i < golden_count(f); i++)
    {
        const golden_record *r = golden_record_at(f, i);
        adf_rat_t q;
        char *printed = NULL;

        if (r->is_status)
        {
            adf_rat_clear(q);
            ADF_CHECK_MSG(adf_rat_set_str(q, r->input, r->input_len) == ADF_PARSE,
                          "the input \"%s\" should not parse", r->input);
            continue;
        }
        ADF_CHECK(adf_rat_set_str(q, r->input, r->input_len) == ADF_OK);
        adf_rat_get_str(printed, q);
        ADF_CHECK_MSG(strcmp(printed, r->expected) == 0, "the input \"%s\" printed \"%s\", "
                                                        "expected \"%s\"",
                      r->input, printed, r->expected);
        adf_str_free(printed);
        adf_rat_clear(q);
    }
    golden_close(f);
}
```

Follow `docs/conventions.md` 11.3 for what a test of a golden vector must compare: the status
for a status vector, the printed text for a type without a real part, and the enclosure for a
real or complex ball.

## Fuzzing

`make fuzz` builds every `tests/fuzz/fuzz_<name>.c` with clang's libFuzzer and the address and
undefined-behaviour sanitizers (`-fsanitize=fuzzer,address,undefined`,
`-fno-sanitize-recover=undefined`), and runs each of them for `FUZZ_SECONDS` seconds (30 by
default) on at most two cores.

    make fuzz                        # every target, 30 s each
    make fuzz FUZZ_SECONDS=10        # a short run
    make fuzz FUZZ_TARGET=support    # one target
    make fuzz FUZZ_WORKERS=2         # two libFuzzer workers, still two cores in all
    make fuzz COV=0                  # no coverage report

The committed seeds of a target are the files of `tests/fuzz/corpus/<name>/`, taken from the
vector and golden files by `tests/fuzz/seed_corpus.py` plus a few hand-written cases:

    python3 tests/fuzz/seed_corpus.py            # write the seeds that are missing
    python3 tests/fuzz/seed_corpus.py --check    # report what is missing, write nothing

Before a run the seeds are copied to `build/fuzz/corpus/<name>/`, and libFuzzer writes the
inputs it keeps to that copy, so a run never adds files to the working tree. A crash exits
non-zero and leaves the reproducer under `build/fuzz/artifacts/` (the fuzzer itself runs with
`build/fuzz/` as its working directory, so nothing is left in the repository root); it is
played back with

    ./build/fuzz/support build/fuzz/artifacts/crash-<hash>

With `COV=1` (the default) and `llvm-cov` on the path, a second binary of each target is built
with `-fprofile-instr-generate -fcoverage-mapping`, run over the corpus only (`-runs=0`), and
`llvm-cov report` prints the regions, functions, lines and branches reached. Without
`llvm-cov`, install it or read the percentages yourself:

    clang -Iinclude -Itests -std=c11 -O1 -g -fno-omit-frame-pointer \
        -fsanitize=fuzzer,address,undefined -fprofile-instr-generate -fcoverage-mapping \
        tests/fuzz/fuzz_support.c tests/support/*.c -lflint -lgmp -lm -o build/fuzz/support_cov
    LLVM_PROFILE_FILE=build/fuzz/prof.profraw ./build/fuzz/support_cov \
        build/fuzz/corpus/support -runs=0
    llvm-profdata merge -sparse build/fuzz/prof.profraw -o build/fuzz/prof.profdata
    llvm-cov report build/fuzz/support_cov -instr-profile=build/fuzz/prof.profdata

Writing a fuzz target: it must be a `LLVMFuzzerTestOneInput(const uint8_t *, size_t)` that
feeds the bytes to the code under test, and it must state what it asserts, because "it did not
crash" is only half a claim. `tests/fuzz/fuzz_support.c` is the model: it reads the bytes as a
vector file and as a golden file, and checks the invariants of a successful parse. Targets for
the parsers of the library come with work package 1.4 (`docs/PLAN.md` 6), and they must not
reach a FLINT loader with raw text: our loader validates first (M0-D9).

## Mutation testing

`make mutate` runs `tools/mutate/mutate.py` over every file of `src/`, one token change at a
time, in a scratch copy under `build/mutate/`; the source tree is never written to. Each
mutant is built and the tests are run with a timeout, and the mutant is reported as killed,
survived, not compiled or timed out. The target fails if a mutant survives.

    make mutate                                   # every src/*.c, 200 mutants, 2 jobs
    make mutate FILES=src/fball.c                 # one file
    make mutate FILES='src/a.c src/b.c' LIMIT=20 SEED=7 JOBS=2
    make mutate MUTATE_LIMIT=20                   # the same, with the other spelling

The self-test of the tool, over `tools/mutate/example/`, is

    make mutate-selftest

It runs the tool with a deliberately weak test, which must leave a surviving mutant, and then
with a strong test, which must leave none. Its own output is in the report of the lane.

### Options of the tool

`make mutate` passes `--files`, `--limit`, `--seed` and `--jobs`; the other options are those of
`python3 tools/mutate/mutate.py` itself (`--help` lists them all):

    python3 tools/mutate/mutate.py --root . --files src/fball.c --limit 50 --seed 7 --jobs 2
    python3 tools/mutate/mutate.py --root . --files src/dump.c --limit 30 --san
    python3 tools/mutate/mutate.py --root . --files src/rat.c --list       # only list the mutants

- `--list` prints the mutants (after the choice by `--limit` and `--seed`) and runs none; `--limit 0`
  means all of them. `--keys` does the same but prints, for each mutant, the line of
  `tools/mutate/equivalent.txt` that would excuse it, with the word `REASON` where the reason goes,
  so that a key is copied and never typed.
- `--san` builds and runs every mutant with `SAN=1` in its environment, which is what the Makefile
  reads for the address and undefined-behaviour sanitizers, so that a mutant that only reads or
  writes out of bounds, or only leaks, is killed and not reported as a survivor (issue adf-pf5). It
  roughly doubles the time of a run. The `mutate` target of the Makefile does not pass it, and the
  Makefile is not changed for it; call the tool directly as above.
- `--keep` keeps the scratch copy of every mutant under `build/mutate/run-<pid>/keep/` and judges a
  mutant exactly as it does without it: a mutant that does not build is `not compiled`, one that
  does not finish is `timed out`, one whose tests fail is `killed` (surface R2). Use it to read the
  diff or run the tests of one mutant by hand.
- A mutant is `not compiled` only when the compiler or the linker says so in its own format
  (`<file>:<line>:<col>: error:`, `fatal error:`, `undefined reference`) at the start of a line, so a
  test whose name ends in `error` and fails is `killed` (surface R5).
- SIGTERM and SIGINT stop the tool the way a normal end does: the process groups of the running
  mutants are killed and the scratch directory is removed (surface R6). A survivor is printed, and
  flushed, the moment it is found, so a run that is stopped by `timeout` leaves its survivors in
  the log; the counts come at the end.
- `--make "<command>"` replaces `make -s -j2 check` as the judge of a mutant. The command runs in the
  scratch copy; it passes when it exits with 0, and then the mutant survived. The file to mutate need
  not be under `src/`: name the directories the scratch copy needs with `--copy` (the default is
  `Makefile include src tests`). So the command-line driver, `tools/adf/adf.c`, is mutated with the
  driver's own acceptance test as the judge:

      python3 tools/mutate/mutate.py --root . --files tools/adf/adf.c \
          --copy Makefile include src tests tools --make 'sh tests/test_driver.sh' \
          --limit 50 --jobs 2 --timeout 300

  `--san` puts `SAN=1` into the environment of that command too, and `tests/test_driver.sh` reads
  it (`build/adf-san`).

### The excused survivors

A survivor is a claim the tests do not check (`docs/PLAN.md` 7). A survivor that computes the
same thing as the original for every input may be excused by listing it in
`tools/mutate/equivalent.txt`. An entry is one line, and it has no line number, because a line number
does not survive an edit above the mutant (surface review R3: 45 of the 76 entries of the old file
matched no mutant, and one excused another statement):

    <file> | <kind> | <the text of the line the mutant changes, stripped> | '<old>' -> '<new>' [#n] | <reason>

`<kind>` is one of `op`, `cmp`, `logic`, `gcd_lcm`, `swap_args`, `status`, `drop_call`, `call_swap`,
`prec`, `zero_one`, `drop_assign`, `negate_if`. `#n` follows the change when the same line text with
the same change stands more than once in the file; it counts them from 1 in the order of the file, the
first one being `#1`, and an entry without it names none of a group of two or more. Print the
line of a mutant with `--keys`. A reason must not hold a ` | `.

The reason has to say why the mutant is equivalent, not that the tests do not notice, and an entry
is accepted only after the mutant has been built and the tests passed with it. Two examples of what
that means: exchanging the two operands of `arb_mul` is not "the same value is written" (the radius of
the product depends on the order, in 38216 of 200000 random pairs), it is "both orders give an
enclosure, which is all the library promises"; and dropping `fmpz_fdiv_r(out, out, ctx->K)` in
`adf_modctx_recombine` is not listed, because the documentation of `fmpz_multi_CRT_precomp` does not
say what its `sign` argument does, so the line is not redundant by the ground truth.

    python3 tools/mutate/check_equivalent.py       # every entry matches exactly one mutant?

`check_equivalent.py` builds nothing. It lists the mutants of `src/*.c` and prints every entry that
matches none (the mutant was edited away or changed), every entry that stands twice, and every line
that is not an entry; it exits with 1 if there is one. Run it after every edit of a source file that
has entries, and delete the entry whose mutant is gone.

## Memory check

`tools/memcheck/` finds reads of uninitialised FLINT and adelefeld values, which neither the address
nor the undefined-behaviour sanitizer sees (`fmpz_t` is an array of one struct, so an `fmpz_t a;`
that was never initialised is a valid pointer to garbage; the program fails later, inside GMP).

    tools/memcheck/run.sh                  # valgrind over every tests/test_*.c when valgrind is
                                           # there, else the static checker
    tools/memcheck/run.sh --checker        # the static checker over src/*.c tests/*.c
    python3 tools/memcheck/check_uninit.py src/*.c tests/*.c tools/adf/adf.c
    python3 tools/memcheck/selftest.py     # the test of the checker and of run.sh

valgrind is installed on the development machine at `~/.local/bin/valgrind`, which is not on every
shell's `PATH`; `run.sh` looks there when `command -v valgrind` finds nothing
(`run.sh --valgrind-path` prints the one it uses). Under valgrind, `run.sh` builds each test at
`-O1 -g` into `build-memcheck/` and runs it with `--track-origins=yes --leak-check=full`, two at a
time, 120 s each.

The static checker reports a use that stands before the first init in the text (`fmpz_add_ui(a, a,
k); fmpz_init(a);`), a clear before an init, and an init without a clear. It reads the text in order
and is blind to three things, which `tools/memcheck/README.md` explains and `selftest.py` pins with a
snippet each: an init on one branch of an `if` only, an array element (`v[0]` initialised, `v[1]`
read), and a `goto` over the init. valgrind sees all three when the path runs, which is why the two
are used together.
