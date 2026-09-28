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

A survivor is a claim the tests do not check (`docs/PLAN.md` 7). A survivor that computes the
same thing as the original for every input may be excused by listing it in
`tools/mutate/equivalent.txt` as `FILE:LINE:KIND | reason`, where `KIND` is one of `op`, `cmp`,
`logic`, `gcd_lcm`, `swap_args`, `zero_one`, `drop_assign`, `negate_if`. The reason has to say
why the mutant is equivalent, not that the tests do not notice.

## The debug build `INV=1`

`make clean && make check INV=1` builds the library and every test with `-DADF_CHECK_INVARIANTS`
(`docs/conventions.md` 4.4 and 4.6): every public function checks the predicates of its inputs on
entry and aborts with one line on stderr, and a context counts the values that borrow it and
`adf_modctx_free` aborts if the count is not zero. `tests/test_invariants.c` and
`tests/test_invariants_lifetime.c` test this with child processes (`fork`); built without the flag
they print that they were skipped and pass. The list of checked functions is
`lanes/m1-invariants/functions.tsv`; `test_invariants.c` requires that its table agrees with it.
`make clean` between builds with different flags. `INV=1 SAN=1` works.

The whole suite passes under `INV=1`. Rules for a test that builds a local value (`adf_fball` with the
local backend, `adf_scaled`): it is made and cleared through the library (`adf_fball_set_local`,
`adf_scaled_init`, `adf_scaled_clear`), never by writing the context field, because the borrow count
sees only such values (decision M1-D10); a sentinel context in a value is a live context made for the
purpose and "untouched" is checked by comparing the field with it (M1-D2); a value that is not
canonical and that no function of the library can make is built by hand from a fresh `init`, with a
comment that names the header line that admits the call. A test of a status that a function returns
for a non-canonical input outside its contract is compiled only without the flag; with the flag a test
of the same name forks a child and requires the abort line (M1-D11: `test_fball.c`
`identical_local_guard`, `test_recon.c` `an_adele_with_an_infinite_real_ball_is_rejected`).
