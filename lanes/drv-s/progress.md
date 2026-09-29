# lanes/drv-s: running notes

Lane drv-s: the commands of the driver `adf` for the solvers of milestone S.  Owned files:
`tools/adf/adf.c`, `tools/adf/README.md`, `tests/driver/s-*.cmd`, `tests/driver/s-*.out`,
`tests/test_driver.sh` (nothing to change there), `lanes/drv-s/`.

## What was done, in the order it was done

1. Read `lanes/COMMON.md`, `CLAUDE.md`, `docs/SPEC.md` 9.1, 9.2, 15 (M1-D1, S-D1 to S-D21),
   `include/adelefeld/roots.h`, `include/adelefeld/resid.h`, `include/adelefeld/place.h`,
   `include/adelefeld/text.h` 9.4 and 9.5, `tools/adf/adf.c`, `tools/adf/README.md`,
   `tests/test_driver.sh`, `tests/driver/01..13`, `tests/julia/roots.jl`, `tests/julia/resid.jl`,
   `docs/conventions.md` 9.5 and `src/text.c` (`tx_put_real`, `tx_put_fmt`).

2. The grammar: one command per line, one to three operands separated by " with ".  The tests
   `tests/test_driver.sh` runs every `tests/driver/*.cmd`, so the four new case files are picked
   up without any change to the script: nothing was changed there.

3. Expected values, computed in this lane and not from the program:
   - `lanes/drv-s/expected_roots.py`: the centres of the p-adic lists by the digit rule
     a + t p^(k-1), the checks (the square, the residue mod 7, the size of the prime), and the
     cases with no root (a non-residue, an odd valuation).
   - `lanes/drv-s/expected_recover.py`: the solutions of Definition 1.1 by enumeration over d.
   - `lanes/drv-s/gen_s_hostile.py`: writes `tests/driver/s-hostile.cmd` and `.out`, the two long
     lines (a coefficient of 100000 digits, one of 60000) are written by it.
   - `lanes/drv-s/real_probe.c` and `lanes/drv-s/real_check.py`: the one line of
     `s-realroots.cmd` at `digits 20` and the line at `prec 100000` depend on the isolating ball
     that FLINT returns.  The check applies the algorithm of conventions 9.5 to the exact dyadic
     midpoint and radius of that ball, in exact rational arithmetic, and three more properties.

4. Red: `sh tests/test_driver.sh` with the four case files and no code gave `error: PARSE` for
   every command; the log of the failing run and of the four cases one by one is `red.log`.

5. The code: a section "the solver commands" in `tools/adf/adf.c` (the polynomial parser, the
   integer operand, the place, the precision, the two printers, the three operations and the
   dispatcher), three rows in the table of the operations, two constants (the depth 64, the limit
   1000), and the branch in `adf_drv_command`.

6. Green: `sh tests/test_driver.sh`, `SAN=1 sh tests/test_driver.sh`, `make clean && make
   check-all`, `make clean && make -j2 check CC=clang`, and `make -C tools/adf CC=clang && sh
   tests/test_driver.sh`.  Numbers in `report.md`.

## Two things the tests found, both kept as they are

- The printer of a rational writes `n` alone when `d = 1` (conventions 9.4), so
  `recover 18 mod 19 with 1 with 3` gives `-1` and not `-1/1`.  The expected line was wrong and
  the command file says so.
- The hostile file had three `error: PARSE` where there are four: `roots 1 2 x with 7 with 5` is
  a `PARSE` of the polynomial and not a `DOMAIN` of the zero polynomial, which is the next line.

## Checked and found sound, no finding

- The inner loop of Algorithm R over `y` (step 7) is not unbounded: the certificate pair has
  `0 <= R <= A < R'` (C3), so the interval of `y` in a round has length `2 A / R' < 2` and holds
  at most three integers.  A first worry, that `T' = 0` lets `d` stand still and the loop run
  `10^24` times, is false; `recover 1 mod 1000003 with 10^30 with 10^30` answers `NOT_UNIQUE` in
  4 ms.  No case of a hanging search is in the tests, and none was found.
- The cost of a p-adic search of degree 4999: `roots -2 0 ... 0 1 with 7 with 5` with 5000
  coefficients runs in 29 ms (three roots, lifted).
- `realroots` at the largest setting `prec 100000` answers `error: LIMIT` for a root of size about
  1, because the radius FLINT returns is `2^-131071` and the guard of M1-D6 refuses it.  That is
  the documented consequence of the guard (README, "Why prec stops at ADF_PRINT_EXP_MAX"), not a
  new finding; it is a test line.
