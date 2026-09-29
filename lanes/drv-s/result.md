# Lane drv-s: the commands of the driver `adf` for milestone S

## What was built

Three commands were added to the driver `tools/adf/adf.c`, so that a user can type the solvers of
milestone S at the prompt.  The grammar of M1-D1 is unchanged: one command per line, an operation
name and one to three operands separated by the word ` with `, one line of output per command, a
result or `error: <STATUS>`.

- `roots POLY with P with K` calls `adf_roots_padic(L, f, P, K, 64)` and prints the roots in the
  order of the list, separated by `; `, each as `A mod P^K` with `A` the centre in `[0, P^K)` and
  `K` the precision of its certificate, and `none` for an empty complete list.
- `realroots POLY` calls `adf_roots_real(L, f, prec)` at the setting `prec` and prints the balls
  in the order of the list, separated by `; `, each the real-ball text of conventions 9.5 with the
  setting `digits`, and `none` for an empty list.
- `recover C mod M with A with B` calls `adf_resid_reconstruct` with `limit` 1000 on the residue
  class read out of the finite ball `C mod M` by `adf_resid_set_fball_forget`, and prints the
  unique solution as the driver prints a rational.  The name `reconstruct` and its one and three
  operand forms are untouched.

`POLY` is the integer coefficients in decimal, separated by single spaces, the constant term
first, at least one coefficient.  `P`, `K`, `A` and `B` are exact rationals of the value form,
read by the same typed parser as every other operand, and they must be integers.  The depth 64 and
the limit 1000 are two constants of the driver (`ADF_DRV_ROOTS_DEPTH`, `ADF_DRV_RECON_LIMIT`); the
driver holds no other bound of its own: a real ball is printed through `adf_adele_get_str`, the
only printer the library has for one, and a NULL from it is `error: LIMIT` as for every value.

Every status of the library is passed on, every value is cleared on every path, and the driver
does not abort on a text of the user.

## The files written

Owned by the lane, all under `/home/tobias/Projects/adelefeld-wt/drv-s`:

- `tools/adf/adf.c`: the section "the solver commands" (`adf_drv_poly_syntax`,
  `adf_drv_poly_value`, `adf_drv_int_value`, `adf_drv_place_operand`, `adf_drv_prec_operand`,
  `adf_drv_put_roots`, `adf_drv_arb_str`, `adf_drv_put_reals`, `adf_drv_solve_roots`,
  `adf_drv_solve_realroots`, `adf_drv_solve_recover`, `adf_drv_solver`), three rows in the table of
  the operations, three values of `adf_drv_op`, the two constants, the branch in
  `adf_drv_command` and the paragraph of the header comment.
- `tools/adf/README.md`: the three rows of the table of the operations and the section "The
  commands of the solvers: `roots`, `realroots` and `recover`".
- `tests/driver/s-roots.cmd` (9 commands), `s-roots.out` (9 lines),
  `tests/driver/s-realroots.cmd` (13 commands, 2 of them settings), `s-realroots.out` (10 lines),
  `tests/driver/s-recover.cmd` (19 commands), `s-recover.out` (19 lines),
  `tests/driver/s-hostile.cmd` (19 commands, 4 of them long lines), `s-hostile.out` (19 lines).
  Each file says in its first comment where every expected line comes from.
- `tests/test_driver.sh`: **not changed**.  The script runs every `tests/driver/*.cmd` it finds,
  so the four new cases run without a change; the only thing the brief asked for there was not
  needed.
- `lanes/drv-s/expected_roots.py`, `expected_recover.py`, `gen_s_hostile.py`, `real_probe.c`,
  `real_check.py`, `progress.md`, `red.log`, this report.

## The tests, and how the expected lines were obtained

Nothing was taken from the output of the program.

- `lanes/drv-s/expected_roots.py` lifts the roots modulo `p^K` one digit at a time and checks
  them: for `X^2 - 2` at 7 the two centres are 4567 and 12240 with `a^2 - 2` equal to 16807 times
  1241 and 8914, both 3 and 4 mod 7, and `4567 + 12240 = 16807`; at 5, 2 is not a square modulo
  5, so the list is empty; the prime of 64 bits is prime by `pow(2, p-1, p) = 1` and the planted
  roots 3, 5, 7 have `g'` values -8, -4, 20, none divisible by it, so `s = 0` and `K = 3`.
- `lanes/drv-s/expected_recover.py` enumerates the solutions of Definition 1.1 over `d`, and
  checks the five conditions of the definition for the planted 21-digit pair.  The statuses are
  the steps of Algorithm R (`docs/proofs/solvers.md:256-324`), quoted in the comment of each
  command of `s-recover.cmd`.
- `lanes/drv-s/real_check.py` is an implementation of the algorithm of conventions 9.5 in exact
  rational arithmetic, applied to the exact dyadic midpoint and radius that
  `lanes/drv-s/real_probe.c` reads out of the list of `adf_roots_real`.  It reproduces the line of
  `s-realroots.cmd` at the setting `digits 20`, and checks that the printed interval contains the
  exact ball, that the exact end points give opposite signs of `X^2 - 2` (the test of solvers
  P3.8), that the ball is on one side of 0 with 2 strictly between the squares of its end points,
  and that the relative accuracy is at least 64 bits (126 and 154 for the two balls).  The only
  thing taken from the program is which ball FLINT chose.
- `lanes/drv-s/gen_s_hostile.py` writes the two files of the hostile case; the coefficients of
  100000 and 60000 digits and the two polynomials of 5000 coefficients are written by it so that
  they are exact.  The case file says why each of them has the answer it has: the 100000-digit
  line is over the line limit of the driver; `X^2 - (10^60000 - 1)` has no root in `Z_7` because
  `v_7(10^60000 - 1) = 1` is odd and a square has an even valuation; `X^2 - 3` has none because 3
  is not a square modulo 7; `X^4999 - 2` has the single root with centre 7891 modulo 16807.

Two of the expected lines were wrong at first and were corrected, not the program:

- `recover 18 mod 19 with 1 with 3` was expected to be `-1/1`; the printer of a rational writes
  `n` alone when `d = 1` (conventions 9.4), so the line is `-1`.  The command file says so.
- The hostile file had three `error: PARSE` where there are four: `roots 1 2 x with 7 with 5` is
  a `PARSE` of the polynomial, and the zero polynomial is the next line.

## Every check that was run, with its command and its result

Red, before the code (log in `lanes/drv-s/red.log`):

- `timeout 900 sh tests/test_driver.sh` -> `test_driver: s-hostile: the output differs from
  tests/driver/s-hostile.out`, exit 1.  Every command of the four new cases answered
  `error: PARSE`, 18 lines instead of 17 for the hostile case.
- `build/adf < tests/driver/s-*.cmd` for the four cases one by one -> exit status 1 and a diff
  against each `.out` (all lines `error: PARSE`).

Green, after the code:

- `timeout 900 make -j2 && timeout 900 sh tests/test_driver.sh` ->
  `test_driver: h7_many_lines: 100000 lines in 0.081 s` and
  `test_driver: 31 cases, 100433 expected lines, all equal (SAN=0)`.
- `timeout 900 env SAN=1 sh tests/test_driver.sh` ->
  `test_driver: h7_many_lines: 100000 lines in 0.737 s` and
  `test_driver: 31 cases, 100433 expected lines, all equal (SAN=1)`.
- `timeout 1700 make clean && timeout 1700 make check-all` ->
  `check-all passed: make check, driver, exports, julia, mutate-selftest, memcheck-selftest`, with
  `check passed: all 57 test programs` and `262 declared and exported: 262` inside it.
- `timeout 1700 make clean && timeout 1700 make -j2 check CC=clang` ->
  `check passed: all 57 test programs`.
- `make -C tools/adf CC=clang && sh tests/test_driver.sh` ->
  `test_driver: 31 cases, 100433 expected lines, all equal (SAN=0)`.  No warning with
  `-Wall -Wextra -Wpedantic -Werror` under cc and under clang.
- `python3 lanes/drv-s/expected_roots.py`, `python3 lanes/drv-s/expected_recover.py`,
  `python3 lanes/drv-s/gen_s_hostile.py` -> the values quoted above.
- `python3 lanes/drv-s/real_check.py` -> `the two agree`, the text of conventions 9.5 for each of
  the two balls and the line of the driver.
- Timings, each case on its own: `s-roots.cmd` 0.004 s, `s-realroots.cmd` 0.004 s,
  `s-recover.cmd` 0.003 s, `s-hostile.cmd` 0.008 s (with a coefficient of 100000 digits, one of
  60000 digits and two polynomials of 5000 coefficients).  A 5000-coefficient polynomial whose
  root lifts to `Z_7` runs in 0.029 s.
- Ad hoc, 20 commands of garbage and edge shapes, no crash and a status for each: `roots`,
  `realroots` and `recover` with no operand `error: PARSE`; `roots -` and `roots - 0 1`
  `error: PARSE`; `roots 1 2 3 with` `error: PARSE`; `realroots 1 1 1 1 1 1 1 1`
  `-1 +/- 4.2e-38`; `recover * with 1 with 1` `error: PARSE`; `recover [1 mod 2] with 1 with 1`
  `error: UNSUPPORTED`; `roots 007 0 1 with 07 with 05` `none` (leading zeros are canonicalised
  by the value form and by `fmpz_set_str`); `roots -0 with 7 with 5` and `realroots -0`
  `error: DOMAIN`; `recover 6 mod 12 with 0 with 0` `error: NO_SOLUTION`.

## What is not done

- The depth 64 of `roots` and the limit 1000 of `recover` are fixed in the driver, as the brief
  decided.  There is no way to type another depth, no command for the unresolved classes
  (`adf_roots_padic_partial`), none for the seed function, for `adf_resid_verify_result` and none
  for the linear solvers: out of this lane.
- `realroots` uses the setting `prec` and `roots` uses its own operand `K`; the driver has no
  command that shows a root list as a value, and S-D12 gives the types of milestone S no text form
  and no dump form, so `show` and `dump` cannot reach them.
- The scripts that derive the two lines of `s-realroots.cmd` that depend on FLINT live in this
  lane and are not part of the test suite; the case file points at them.  Every other expected
  line is derived in the case file itself.
- The four new scripts are not in `tests/fuzz/corpus/driver/`, which is a copy of the scripts of
  `tests/driver/` and is not a file of this lane.  Whoever owns the corpus may add `s-roots.cmd`,
  `s-realroots.cmd` and `s-recover.cmd` as seeds of the fuzzer; `s-hostile.cmd` is 180 KB and
  would be an awkward seed.
- No mutation testing of `tools/adf/adf.c` and no fuzzing run was made in this lane.
- Two lines of the test data are longer than 116 characters (`s-recover.cmd:68`, 149 characters,
  a command with three numbers of 41 digits, and the two 10020-byte lines of `s-hostile.cmd`).
  They are data, not prose, and cannot be shortened.

## Sources pending

- `[source pending: FLINT 3.0.1, fmpz_set_str and a leading sign]`, marked in the comment of
  `adf_drv_poly_syntax`.  The driver reads the sign itself and negates, so nothing depends on the
  answer.  `fmpz_set_str` is documented at `refs/src/flint-3.0.1/fmpz.rst:427-431` (null
  terminated, base 2 to 62, returns 0 or -1); the sign is not mentioned there and the copy of the
  sources under `refs/src/flint-src-3.0.1/` holds no `fmpz/` directory.
- The isolating ball of `arb_fmpz_poly_complex_roots` is not derived in this lane.  It is not
  quoted as ground truth: the exact midpoint and radius that FLINT returns are measured with
  `lanes/drv-s/real_probe.c`, the rest of the line is computed from them, and the case file says
  that the line depends on FLINT 3.0.1, as the two lines of `tests/driver/09_settings.cmd` do.

## Findings against the headers

1. **M1-D1 does not list the three operations.**  `docs/SPEC.md:866` names the operations of the
   driver as `show, type, add, sub, mul, neg, div, equal, contains, overlaps, reconstruct, cap`
   with the further operations `compare, dump, load`, and it says "one to three value texts".  The
   driver now has `roots`, `realroots` and `recover`, whose first operand is not a value text but
   a list of decimal coefficients, and whose third and second operands must be integers.  The
   brief of the lane says the orchestrator records this as an addition to M1-D1; the text of the
   decision has to be extended, because as it stands a reader of the specification does not find
   these operations anywhere.  The same holds of the sentence "Output: one line per command, a
   value text or `error: <STATUS>`": the output of `roots` is a list of `A mod P^K` and that of
   `realroots` is a list of balls, which are not value texts.
2. **No finding against `docs/SPEC.md` 9.1, 9.2 or the decisions S-D1 to S-D21.**  Every status
   the driver can print for the new commands is a status of conventions 3.1, every claim the
   output makes is one the header of the function makes, and nothing the driver prints claims
   more than the function proved.  The zero polynomial, a prime that is not a prime of a place, a
   precision below 1 and a bound that is not an integer are all `DOMAIN`, which is what
   conventions 3.1 and 4.4 call data outside a stated domain; a precision of 30 digits is `LIMIT`,
   the status of a size bound, the status the driver already gives a `prec` setting above
   `ADF_PRINT_EXP_MAX`.
3. **An observation, not a defect.**  `realroots` at the largest setting `prec 100000` answers
   `error: LIMIT` for a polynomial whose real roots are of size about 1, because the balls
   `adf_roots_real` returns then have a radius below `2^-100001` and the guard of M1-D6
   (`include/adelefeld/text.h:32-38`) refuses to print them.  This is the consequence the README
   of `tools/adf` already states in "Why `prec` stops at `ADF_PRINT_EXP_MAX`", and the case file
   says why the line is not forced by the accuracy contract alone: a ball of exactly 100001 bits
   would have an exponent of -100000, which the guard admits, and FLINT 3.0.1 returns 131071
   bits here.
4. **A worry that was checked and is false, recorded so that it is not looked for again.**  The
   inner loop of step 7 of Algorithm R over `y` is not unbounded when `T' = 0`, although `d =
   x abs(T) + y abs(T')` then stands still: the certificate pair satisfies `0 <= R <= A < R'`
   (C3 of Definition 1.3, checked by `adf_recon_cert_check`), so the interval of `y` in a round
   has length `2 A / R' < 2` and holds at most three integers.
   `recover 1 mod 1000003 with 1000000000000000000000000000000 with 1000000000000000000000000000000`
   answers `error: NOT_UNIQUE` in 0.004 s.  The documented cost of the search therefore holds and
   no case of a hanging search is in the tests.
