# lanes/u-repair1/redgreen.md

Red first, then green, for each part. Every run under `timeout`, at most 2 jobs. The library is
`build/` (plain), the tree is the worktree of this lane.

## Parts 1 and 2: the two defects of `dp_arb_sign` (review u-review1, F1 and F2)

Build: `timeout 900 make -j2 build/test_dump_units`.

### Red, before the repair of `src/dump.c`

    $ timeout 300 ./build/test_dump_units; echo "exit=$?"
    ok   golden_rows_of_dump_tsv
    ok   ucoset_round_trip_random
    ok   idele_round_trip_random
    ok   idclass_round_trip_random
    ok   ucoset_strictness
    ok   idele_strictness
    ok   idclass_strictness
    timeout: the monitored command dumped core
    exit=134

The run stops in the new test `idele_negative_midpoint_domain`: the signal is SIGABRT (exit 134,
`Aborted`), not a failed check. The abort is `flint_abort()` at `src/dump.c:2229` of `dp_load_idele`
("cannot happen: stage 6 checked the predicate of 5.7"), reached from the first text
`adf1 Q idele 1 -1 0 1 0 1 1 1 0`. The backtrace with gdb:

    #5  flint_abort () from /lib/x86_64-linux-gnu/libflint.so.18
    #6  dp_load_idele (...) at src/dump.c:2247
    #7  adf_idele_load_str (...) at src/dump.c:2259
    #8  adf_test_fn_golden_rows_of_dump_tsv (or the new test) at tests/test_dump_units.c:314

The same through the driver:

    $ printf 'load adf1 Q idele 1 -1 0 1 0 1 1 1 0\n' | timeout 60 ./build/adf; echo "exit=$?"
    Aborted
    exit=134

The second defect (F2), before the repair:

    $ printf 'load adf1 Q idele 1 3 0 1 1 1 1 1 0\n' | timeout 60 ./build/adf; echo "exit=$?"
    error: DOMAIN
    exit=1

True status ADF_OK: the ball is `[1, 5]`.

### Green, after the repair of `src/dump.c`

    $ timeout 300 ./build/test_dump_units; echo "exit=$?"
    ...
    ok   idele_negative_midpoint_domain
    ok   tie_of_the_leading_bits_is_a_strict_inequality
    ok   sign_of_the_small_balls
    12 tests, 32505050 checks, 0 failed checks, 0 failed tests
    exit=0

## Part 3: no abort reachable from a text

Red: the pre-repair source of `src/dump.c` (a copy of `lanes/u-review1/ft/dump.orig.c`) built
against the new tests, through the fault runner with `FT_SRC=dump.prerepair.c`:

    $ FT_SRC=dump.prerepair.c FT_PLAIN=1 timeout 600 python3 lanes/u-repair1/faults.py F00
    F00 baseline, no change
         test_dump_units: rc=-6 ok   idclass_strictness
         test_dump_local: rc=1 10 tests, 36177 checks, 6 failed checks, 2 failed tests

`rc=-6` is SIGABRT. Green, with the repaired source: the faults F01, F15, F17 (each of which the
review saw abort) now end with failed checks and a count in the release build (numbers in
`lanes/u-repair1/report.md`), and under `INV=1` they stop at the new assertion of the loader.

## Part 4: the exponents of a dumped local ball

Red: the fault F20 of `lanes/u-repair1/faults.py` (the word check of a negative token as it was)
gives no failure, which is the finding: the text `-8000000000000000` is `ADF_LIMIT` under every
limits struct because its absolute value 2^63 is above every `max_prec` (an slong), so the word
check never decides that text. The old text of the review (F3) therefore cannot be made to load,
and no test can separate the two versions of `dp_fits_si`.

Green: the tests `lball_exponent_word_and_prec` and `sball_exponent_and_radius_mantissa` in
`tests/test_dump_local.c` pin both sides of `max_prec`, of the word and of `ADF_LBALL_EXP_MAX`.

## Part 5: the driver and the blanks around the operand of `load`

Red: the sentence of `tools/adf/README.md` ("`load` does not trim its operand") is false for a blank
between the operation name and the operand, which the line reader of every command skips:

    $ printf 'load  adf1 Q ucoset 1 0\nload adf1 Q ucoset 1 0 \n' | timeout 60 ./build/adf
    [1]
    error: PARSE

Green: the README sentence now says what the driver does, and the four lines appended to
`tests/driver/u-dump-units.cmd` and `.out` pass in `sh tests/test_driver.sh`:

    $ timeout 600 sh tests/test_driver.sh
    test_driver: 69 cases, 101312 expected lines, all equal (SAN=0)