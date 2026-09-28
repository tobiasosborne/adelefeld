# Red and green log of lane m1-repair-driver

Every entry: the command, the result before the change to the driver (red) and after (green).
The expected lines were written from docs/SPEC.md, docs/conventions.md and the headers before
the driver was run; where a line was wrong, the correction and the reason are given.

## R7: the guard on printing (the library's rule, one rule only)

Red: the new case tests/driver/11_guard.cmd (the boundary of M1-D6, the result guard, the
prec limit) fails at the line `prec 100001`:

    $ ./build/adf < tests/driver/11_guard.cmd > build/driver/11_guard.got; diff -u \
          tests/driver/11_guard.out build/driver/11_guard.got
    -error: LIMIT        (the line `prec 100001`, expected LIMIT, got nothing: the setting was
                          accepted, and the setting writes no line)
    (0.33333333333333333333 +/- 3.4e-21 ; 0)

    $ awk 'BEGIN { inc = 0 } ...' tools/adf/adf.c     # the check of tests/test_driver.sh
    60: #define ADF_DRV_MAX_EXP2 ((slong) 100000)
    205: static int adf_drv_exp2_ok(const arb_t x)      (and its uses)

    The two lines are the places the review names (docs/reviews/m1/surface/review.md, R7:
    "tools/adf/adf.c:205-218 (adf_drv_exp2_ok), constant at line 60"), which is where the
    second copy of the bound was before this repair.

Green: after the repair the six guard lines of 11_guard are equal to the expected ones and
the check of tests/test_driver.sh writes nothing:

    $ sh tests/test_driver.sh
    test_driver: 27 cases, 100376 expected lines, all equal (SAN=0)

Note: the boundary behaviour (2^99999 prints, 2^100000 does not) was already the rule of the
library, so only the `prec 100001` line of 11_guard was red; what the repair removes is the
second copy of the bound, and that is what the check in tests/test_driver.sh tests.  The
reviewer expected 2^100000 to print, reading the words of M1-D1 with the other convention for
the binary exponent; the brief of this lane and include/adelefeld/text.h:32-38 name the rule
of the library (ARF_EXP and MAG_EXP against ADF_PRINT_EXP_MAX), and that is what the driver
does now and what the test pins.

## R8: prec above the largest useful prec

Red: `prec 100001` was accepted, `prec 1000001` gave DOMAIN; both wrong.

    $ diff -u tests/driver/07_status.out <(./build/adf < tests/driver/07_status.cmd)
    -error: PARSE / -error: LIMIT ... +error: DOMAIN

Green: `error: LIMIT` for every prec above ADF_PRINT_EXP_MAX, in 07_status, 09_settings,
11_guard and the hostile case h9.  The largest useful prec is 100000 = ADF_PRINT_EXP_MAX:
a real result rounded at prec p has a radius with the binary exponent -p, measured with
lanes/m1-repair-driver/prec_probe.c (MAG_EXP -100000 at prec 100000, -100001 at 100001).

## R9: whitespace around the number of a setting

Red: the new hostile case h9 (tests/driver/hostile/h9_settings_ws.out) fails at its first
line; before the repair `prec 64 ` and `prec 64\r` are `error: PARSE`.

Green: h9 is equal, and the value command of a CRLF script prints 7/3.

## R10: the order of the checks

Red: tests/driver/12_status_order.cmd, the eight commands of the reproducer
docs/reviews/m1/surface/checks/status_order.cmd and five more:

    $ diff -u tests/driver/12_status_order.out <(./build/adf < tests/driver/12_status_order.cmd)
    -error: UNSUPPORTED x5      (got: DOMAIN, DOMAIN, LIMIT, PARSE, PARSE)
    +error: PARSE               (got: PARSE for `add (1 ; 1/0) with (1 ; 1e-100001)`, which is
                                  not a sentence of the grammar: the finite part of an adele
                                  is q(a) or q(a) mod q(N); the case was replaced by
                                  `add (1 ; 1) with (1 ; 1/0)`)

Green: all thirteen lines equal, and the README states the order.

## R11: a computed value that cannot be printed, with printable operands

The case is in tests/driver/11_guard.cmd: `prec 100000`, `show (2^51000 ; 0)` prints, and
`mul (2^51000 ; 0) with (2^51000 ; 0)` is `error: LIMIT`.  It was green before the repair as
well: the driver checked its results already; the finding was that no test covered it and
that the example of the README was refused for its operands, not for its result.

## The new commands: compare, dump, load

Red: tests/driver/13_dump.cmd gave `error: PARSE` for every line, the commands did not exist.

    $ diff -u tests/driver/13_dump.out <(./build/adf < tests/driver/13_dump.cmd)
    -equal / -different / -adf1 Q rat 7 3 ... (25 lines)   +error: PARSE ...

Green: all 25 lines equal.  Two expected lines were wrong and were corrected from the
specification, not from the output:
  * `compare (* ; 1 mod 2) with (* ; 5 mod 2)`: I wrote `equal`, because equal_set of
    SPEC 4.2 is true for the two sets.  SPEC 4.2 compares the two unknown *points*, and
    "certainly equal" means both values are exact (include/adelefeld/fball.h:257-264), so the
    answer is `undecided`.  The comment of the case says so.
  * `load adf1 Q rat 2 4`: DOMAIN, the predicate G of conventions 5.2 on the raw triple
    (2, 4, 1) fails, as tests/golden/dump.tsv has it.

## R12: tests/test_dlopen.c builds the shared object itself

Red: the reproducer of the review, run from a directory without build/libadelefeld.so:

    $ cd docs/reviews/m1/surface/checks && ../../../../../build/test_dlopen
       build/libadelefeld.so is not there (run sh tests/test_exports.sh first); nothing to load
    ok   the_shared_object_is_loaded
       build/libadelefeld.so is not there; the names cannot be looked up
    ok   the_four_functions_are_found_by_name_in_the_loaded_object
    2 tests, 0 checks, 0 failed checks, 0 failed tests          (exit 0)

Green: the test asks whether build/libadelefeld.so is there and not older than
build/libadelefeld.a, and runs `sh tests/test_exports.sh` through system() when it is not:

    $ rm -f build/libadelefeld.so && ./build/test_dlopen
    ... test_exports: passed: 193 of 193 declared functions are exported ...
    ok   the_shared_object_is_loaded
    ok   the_five_functions_are_found_by_name_in_the_loaded_object
    2 tests, 29 checks, 0 failed checks, 0 failed tests          (exit 0)

    $ touch build/libadelefeld.a && ./build/test_dlopen | head -1
    == building the shared object              (the age is part of the question)

    $ mv tests/test_exports.sh /tmp/te.sh; rm -f build/libadelefeld.so; \
      touch build/libadelefeld.a; ./build/test_dlopen | tail -2
    sh: 0: cannot open tests/test_exports.sh: No such file
       sh tests/test_exports.sh: the command failed
    FAIL the_five_functions_are_found_by_name_in_the_loaded_object (tests/test_dlopen.c:130, ...)
    2 tests, 2 checks, 2 failed checks, 2 failed tests          (exit 1)

All five functions are asked of dladdr now, not one.

## R13: tests/test_exports.sh judges the names that begin with _adf

Red: the reproducer of the review:

    $ python3 docs/reviews/m1/surface/checks/exports_underscore.py
    _adf_undeclared exported: exit 0
        == exported and not declared: 0 (must be empty)
        == exported names that begin with an underscore, not judged
        _adf_undeclared
        test_exports: passed: 193 of 193 declared functions are exported, ...
    adf_undeclared exported: exit 1

Green:

    $ python3 docs/reviews/m1/surface/checks/exports_underscore.py
    _adf_undeclared exported: exit 1
        == exported and not declared: 1 (must be empty)
        _adf_undeclared
        test_exports: FAILED
    adf_undeclared exported: exit 1
    $ sh tests/test_exports.sh | tail -1
    test_exports: passed: 193 of 193 declared functions are exported, 0 are not implemented yet,
    no exported name is undeclared, no variadic function            (exit 0)

## R14: the comment of tests/test_place.c

No test: the comment said 2^64, the array holds 2^64 - 2 (2^64 is not a ulong).  The comment
now says so, and names the primes and the composites of the array.  The test itself was right
and is unchanged.

## R15: the two lines that pin the rounding of FLINT

No red: the two expected lines of tests/driver/09_settings.out stay as they are, since the
test compares bytes and cannot decide an enclosure.  The script now says why the line is what
it is, and tools/adf/README.md has the section "Two lines of the test depend on FLINT 3.0.1".

## The build of the driver (found while running the checks of the brief)

`sh tests/test_driver.sh` after `make clean && make check SAN=1` failed at the link: the
top-level Makefile had left a sanitized build/libadelefeld.a, and the plain driver cannot
link it.  tools/adf/Makefile now builds one archive per SAN setting below build/
(build/drv-plain, build/drv-san), so the two never mix, and with SAN=1 the library itself is
instrumented as well.  Verified in both orders; see report.md.
