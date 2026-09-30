# Lane n-repair1: result

Worktree at 674c9db. Run 04:38 to 05:06 (2026-09-30). No state-changing git command, no `bd`, no mutation run (brief).
Only files under "You own" were written; the lane directory holds logs, no binary. Every finding was seen red before
the change (logs in `lanes/n-repair1/`).

## D1 (BLOCKER, driver): repaired

Cause: `adf_drv_places` (`project`, `exp_at`, `log_at`) refused only the complex adele; a unit coset, an idele or a
class fell through to `adf_adele_set(a, x.a)` and read the field of another type. Change (`tools/adf/adf.c`): after
the kind and the value are read, any type other than rational, finite ball, adele is `ADF_UNSUPPORTED`.
Test: `tests/driver/f-cross.cmd` (`#!exit 1`, the 21 lines of `lanes/n-review1/driver-cross.cmd`) and `.out` (the
review's expected lines, copied from `driver-cross.expected`).
Red (`redgreen.log`): 15 lines `5: 0` / `5: 1` / `error: DOMAIN` instead of `error: UNSUPPORTED`. Green: `sh
tests/test_driver.sh` 42 cases, 100746 lines, all equal. The other commands with an adele-like operand (`valuation`,
`abs` check `x.type != IDELE`; arithmetic goes through `adf_drv_units_op`; `roots`, `realroots`, `recover`,
`reconstruct`, `add`, `sub`, `cap`, `equal`, ... with `[5 mod 6]`, `(5 ; 5 * [1])`, `<1 ; [1]>`) I probed by hand: each
gives `DOMAIN` or `PARSE` or a documented value; I found no other wrong read. `tools/adf/README.md` says it.

## C1 (MAJOR): repaired

Audit of `src/lball.c` and `src/lball_decomp.c` for every difference or sum of exponents: `lb_addsub`, `mul`, `inv`,
`div`, `pow_si`, `decompose`, `get_center`, `abs`, `frac`, `unit_mod` all test `in_bounds` (or `exp_ok(v)`) before
forming `N - v`, `N + M`, `N - 2v`; the only site that did not is `adf_lball_decompose_teich`
(`src/lball_decomp.c`): `x->N - x->v == 1` (p = 2) came before the `in_bounds` test. Moved after it; header sentence
in `lball.h` says the limit test comes before the p = 2, k = 1 case.
Test (`tests/test_lball.c`, `inputs_beyond_the_exponent_limit_are_limit_not_overflow`): 8 canonical balls (p = 2:
v = -1 N = LONG_MAX; v = LONG_MAX - 1 N = LONG_MAX; exact v = LONG_MAX; exact v = LONG_MIN + 1; p = 3: v = LONG_MIN + 1
N = LONG_MIN + 2; v = 0 N = LONG_MAX; exact v = LONG_MAX; v = -1 N = LONG_MAX); `neg`, `inv`, `pow_si` (k = -2..2),
`decompose`, `decompose_teich`, `frac` on each must be LIMIT with the output untouched; `add`, `sub`, `mul`, `div` on
all 64 pairs LIMIT (DOMAIN across primes); `unit_mod`, `abs`, `valuation`, `get_prec`, `get_center`, the predicates
are called (no status is promised for them). Red (`c1-red.log`, SAN build): UBSan `src/lball_decomp.c:213:34: signed
integer overflow: 9223372036854775807 - -1` and one failed check (`decompose_teich`). Green: 0 failed checks, no
"runtime error" in `check-san.log` (count 0).
Not done: the balls around 0 with huge N (u = 0) and pairs of one huge and one small ball are not in the test; the code
paths for them test `in_bounds` first as above (read, not run).

## D2 (MAJOR): the constrained printer, decided and repaired

Decision: a bound on the work, then NULL (the third of your alternatives). `src/text.c`: `TX_COND_WORK_MAX = 2^25`;
`tx_put_real_cond` adds S (largest bit length of the four integers of mid and rad of the pass, at least 64) for every
level it forms, over the whole call (search and repetitions), and when the sum passes the bound it returns 1;
`adf_tx_write_unit_form` then frees the buffer and returns NULL with `*len = 0`; the driver prints `error: LIMIT`
(M1-D6, unchanged). Documented in `include/adelefeld/text.h` (the sentence "NULL in one case only" is now two cases),
`docs/api-2.md` 4.2 (Statement Q, the paragraph "The bound on the work") and row N-D11 of 4.4, `tools/adf/README.md`.
Alternatives, and why not:
- a proved lower bound of the level from the exponents: the level that succeeds depends on the digits of mid and rad,
  not on their exponents; I found no proof and did not try to prove that none exists. Not proved.
- a search logarithmic in the level: needs that "level k satisfies" is monotone in k; not proved, and the levels are
  not nested (conventions 9.5). Not proved.
- a bound on b alone: would refuse balls that print in milliseconds.
Measured on the family `2^(b-1) + 1/2 +/- 2^(b-1)` (`printer-times.log`, one core): b = 4000 prints in 0.10 s (2438
bytes, prefix `(1.31820409343094310...` as before); b = 8000 refused after 0.37 s; 16000 after 0.50 s; 40000 after
0.80 s; 100000 (the worst admitted input) after 1.17 s (before: no end in 170 s; test red = no end in 120 s,
`d2-red.log`). Test `constrained_printer_ends_in_bounded_time` (`tests/test_text_idele.c`): idele, negative idele and
class at b = 100000 are NULL, len 0, under 2 s; the 4000-bit case still prints.
Cost of the decision, stated plainly: balls that printed before and are refused now: those whose search needs more than
about 2^25 / S levels; for this family from b about 5500 on (b = 16000 printed in 2.0 s before). Another example
(driver, `prec 20000`): `show (1 +/- 0.<n nines> ; 1 * [1])` prints for n = 1000 and is `error: LIMIT` for n = 1500 and
3000. The golden files and the vectors of `t-slice1` pass unchanged (`test_text_idele`: 14 tests, 73465 checks, 0
failed; `check` passes in all four builds). The value 2^25 was chosen from the times above, not derived.
N-D11 in one line: the constrained printer of ideles and classes refuses (NULL, `error: LIMIT`) when its level search
passes a work bound of 2^25 bit-levels; nothing else about printing changed.

## R5 (MAJOR, open item): repaired

`src/sball.c`: `adf_sball_add`, `sub`, `mul` test `prec > ADF_REAL_PREC_MAX` first, before `ADF_INV_SBALL(x)` and
`(y)` (LIMIT, `where` = the real place). `src/rfunc.c`: `at_place` (all `_at` functions, seven names) tests it before
`ADF_INV_SBALL(x)`. The arb-level `adf_real_*` already tested `prec` first. Tests, allocator counter after
`lanes/i-repair1` (input: exact 5-adic rational (2^4096 + 3)/(2^2048 + 7)): `LIMIT_before_any_allocation_of_add_sub_mul`
(`tests/test_sball.c`, prec = MAX + 1 and WORD_MAX) and `LIMIT_before_any_allocation_of_the_functions_at_a_place`
(`tests/test_rfunc.c`: exp, log, Log, sin, cos, sqrt, root; 14 cases). Red under INV=1 (`r5-red.log`): 6 and 14 failed
checks, each "status 10, 14 allocator calls" (the reviewer's 14). Green under INV=1: 0 failed checks. `log_abs_at` is
covered by the code path (`at_place`), not by a test case.

## C2 (MINOR): repaired, sound

`src/lball.c` `fball_limit_certain`: the upper bound of v_p(H) is now also `bits(H) / log2(p) + 1` (p^v <= H < 2^bits;
double arithmetic, the +1 far above its rounding error), taken as the minimum with the old one. Every use of the
bound needs an upper bound, so the change can only turn "not certain" into a test that decides; it cannot make a
non-LIMIT case LIMIT (the divisibility test that follows is unchanged). Case of the review (H = 6^33554433, A =
2^67108866, p = 3): LIMIT in 0.69 s (`projection-after.log`) instead of 16.8 s. Test
`set_fball_limit_when_the_numerator_alone_is_above_the_bound` (`tests/test_lball.c`), red: 5.2 s against the bound 3.5
s (`c2-red.log`), green. Header sentence in `lball.h`. Not done: a case that this bound does not decide (v_p(H) close to
bits(H)/log2(p)) still computes the valuation.

## Checks (last lines; all under `timeout 900`, foreground, `-j2`)

- `make clean && make -j2 check-all`: `check-all passed: make check, driver, exports, julia, mutate-selftest,
  memcheck-selftest` (`check passed: all 71 test programs`; the line "mutate: FAILED: 34 mutant(s) survived" in that log is
  the mutation self-test, which the run counts as passed: "selftest: passed").
- `make clean && make -j2 check SAN=1`: `check passed: all 71 test programs`; 0 "runtime error" lines.
- `make clean && make -j2 check CC=clang`: `check passed: all 71 test programs`.
- `make clean && make -j2 check INV=1`: `check passed: all 71 test programs`.
- `sh lanes/m1-headers/check_headers.sh`: `check_headers: passed`.
- `SAN=1 sh tests/test_driver.sh`: `42 cases, 100746 expected lines, all equal (SAN=1)`.
Logs: `check-all.log`, `check-san.log`, `check-clang.log`, `check-inv.log`, `check-headers.log`, `check-driver-san.log`.
Inputs of the review again: `driver-cross.cmd` prints the 21 expected lines (test f-cross); `edges overflow` is the C1
test (UBSan clean); `edges projection`: `status=LIMIT seconds=0.69`; `edges printer 100000 99999 0`: NULL after 1.17 s;
`partial_alloc.c` is the R5 test (0 calls).

## Not done, and notes

- No mutation testing and no fuzzing (brief). The new tests are not mutation-checked.
- SAN builds do not use `-fno-sanitize-recover`; an overflow is reported on stderr but does not fail the test by itself.
  The C1 test therefore also asserts the statuses, and I grepped `check-san.log` for "runtime error" (0). The Makefile is
  not mine.
- `docs/SPEC.md` 15.4 N-D11 is not written (SPEC not mine); the decision text is in `docs/api-2.md` 4.4 row N-D11.
- The Julia bindings and the ABI are unchanged; `check-all` (exports, julia) passed.
- Findings against the specification: none new. The header of `adf_lball_decompose_teich` said "checked in this order"
  with the p = 2 case before LIMIT; the sentence now names the exception.
