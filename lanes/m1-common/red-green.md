# Red-green log of lane m1-common

Every line is a command run in the repository root, with what it printed. `make check` runs from
the root, so that `tests/golden` and `tests/ref/vectors` resolve.

## 1. tests/test_common.c against an empty src/common.c

    $ make build/test_common
    tests/test_common.c:22:10: fatal error: ../src/common.c: No such file or directory
    make: *** [Makefile:86: build/test_common] Error 1

Red (the first test of the file may fail with a build error, COMMON-C rule 1).

## 2. the first run of the test with the first version of src/common.c

    $ ./build/test_common
    FAIL tests/test_common.c:170: ...: "3.10.0" against "3.1.0": the major and the minor do not agree (case 10)
    FAIL tests/test_common.c:170: ...: "3.1.0" against "3.10.0": ... (case 12)
    FAIL tests/test_common.c:170: ...: "03.00.1" against "3.0.1": the major and the minor agree (case 18)
    7 tests, 64 checks, 6 failed checks, 1 failed tests

Two faults, both in the reader of a version string:

- the digits of the minor were compared from the left without padding the shorter run on the
  left, so "1" and "10" compared equal;
- the position of the '.' was taken from the length of the run after the leading zeros had been
  dropped, so "03.00.1" was read as a run that is not followed by a dot.

## 3. after the fix (the comparison reads the digits from the right and keeps the end of the run)

    $ ./build/test_common
    7 tests, 74 checks, 0 failed checks, 0 failed tests

Green. Six cases were added to the table of the brief (the two-digit minor against a one-digit
one, in both orders, the digit 0 and the digit 9) so that a reader that stops a digit early, or
forgets a leading digit, is caught.
