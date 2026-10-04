# lanes/drv-ball: progress

Every red and every green run of the lane, with the command.  The rules of lanes/COMMON.md.

## 0. Baseline

`timeout 900 sh tests/test_driver.sh`

green: 61 cases, 101124 expected lines, all equal (SAN=0).

## 1. `lball` and `sball` as value kinds

Fixture `tests/driver/drv-ball-values.cmd` and `.out`, 22 lines, written by hand before the code.

Red, `step 1, fixture written`:

    ./build/adf < tests/driver/drv-ball-values.cmd

22 lines, 13 of them `error: UNSUPPORTED`, exit 1.

Green, `step 1, code written`:

    ./build/adf < tests/driver/drv-ball-values.cmd

22 lines equal to the fixture, exit 1 (8 of the lines are the errors of the readers).

## 2. A stored `sball` as the operand X

Fixture `tests/driver/drv-ball-ops.cmd` and `.out`, 13 lines, written by hand before the code.

Red, `step 2, fixture written`:

    ./build/adf < tests/driver/drv-ball-ops.cmd

13 lines, 10 of them `error: UNSUPPORTED`, exit 1.

Red, `step 2, the suite with the two new fixtures`:

    timeout 900 sh tests/test_driver.sh

stops at `drv-ball-ops`, 8 differences in the first 11 lines.

Green, `step 2, code written`:

    ./build/adf < tests/driver/drv-ball-ops.cmd

13 lines equal to the fixture, exit 1 (the three error lines).

## 3. The table of the two printers

    python3 lanes/drv-ball/printer_diff.py ./build/adf ./lanes/drv-ball/lib_text \
        lanes/drv-ball/printer-diff.md

122 rows, 0 byte-identical, 122 different, 115 with byte-identical components.
Nothing was changed in the driver.

## 4. The suite

As it stands, `timeout 900 sh tests/test_driver.sh` stops at `01_spec_4_1`: the exit status is 0 where the
fixture expects 1.  Six old fixtures hold expected lines that step 1 changes; they are read-only for this lane.

With those six fixtures corrected in a scratch copy (`D` of a copy of `tests/driver`, six `.out` files and their
`#!exit` lines):

    timeout 900 sh /tmp/td.sh

63 cases, 101159 expected lines, all equal (SAN=0).  Before the lane: 61 cases, 101124 lines.
The two new fixtures add 35 lines: 22 of `drv-ball-values` and 13 of `drv-ball-ops`.

    SAN=1 timeout 900 sh /tmp/td.sh

63 cases, 101159 expected lines, all equal (SAN=1).

## 5. The three other builds

    make -j2 BUILD=lanes/drv-ball/build-clainv CC=clang all
    clang -Iinclude -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror tools/adf/adf.c \
        lanes/drv-ball/build-clainv/libadelefeld.a -lflint -lgmp -lm -o lanes/drv-ball/build-clang/adf

no warning; the two new fixtures equal.

    make -j2 BUILD=lanes/drv-ball/build-inv INV=1 all
    cc -Iinclude -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror -DADF_CHECK_INVARIANTS tools/adf/adf.c \
        lanes/drv-ball/build-inv/libadelefeld.a -lflint -lgmp -lm -o lanes/drv-ball/build-inv/adf

no warning; the two new fixtures equal; no output on standard error for any of the 49 fixtures.

    make -j2 BUILD=lanes/drv-ball/build-san SAN=1 all
    cc -Iinclude -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined \
        -fno-omit-frame-pointer tools/adf/adf.c lanes/drv-ball/build-san/libadelefeld.a -lflint -lgmp -lm \
        -fsanitize=address,undefined -o lanes/drv-ball/build-san/adf

no warning; the two new fixtures equal; no sanitizer diagnostic on any of the 49 fixtures.

The three binaries were also run over all 49 fixtures and compared with the corrected copy of the expected
files: all equal, for each of the three.