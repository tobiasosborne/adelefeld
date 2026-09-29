# Result of lane f-repair2

Findings R3 to R8 of `docs/reviews/f1/review-sball-rfunc.md` are repaired. R1 and R2 are not touched (the
orchestrator's decision). Files changed: `src/sball.c`, `src/rfunc.c`, `include/adelefeld/sball.h`,
`include/adelefeld/rfunc.h`, `tests/test_sball.c`, `tests/test_rfunc.c`, `tests/julia/sball.jl`, `docs/api-1f.md`
(L4a centre paragraph, S5, the table and decisions 3 to 4 of the slice-2 section only), and in the lane directory
`redgreen.log`, `redgreen-tests.log`, `progress.md`. No binary is left in the lane directory (the reviewer's
programs were built in the scratchpad).

## Per finding

R3 (a complex tag hid the LIMIT of a prime). `src/sball.c`: `adf_sball_neg` and the shared `binary` (add, sub, mul)
inspect every place; a helper `combine` keeps the maximum status in the numeric order of conventions 3.3 and the
first place, in canonical order (inf, then primes increasing), that carries it. The check of the places (DOMAIN)
stays first, as before. `sball.h`: the sentence "the statuses ... cannot mix" is replaced by the rule of 3.3 (also
for neg). Test: `complex_tag_does_not_mask_limit_at_a_prime` in `tests/test_sball.c`: the reviewer's input
(COMPLEX, exact 2^E at 2, E = 2^60, mul: LIMIT at 2, output untouched, also with x = y = z), the mirrored ones
(add and sub with LIMIT at 5 and 7 and at 7 only, operands exchanged; neg with a LIMIT at 3; a complex tag with no
failing prime still UNSUPPORTED at inf; DOMAIN of different places before a LIMIT at the common prime). The
statuses that the local functions can return here are LIMIT only (lball.h), so "two primes with different
statuses" is tested as LIMIT at two primes with the first reported.

R4 (six entry paths without the invariant check). `src/sball.c`: `adf_sball_arch`, `adf_sball_num_places`,
`adf_sball_identical` (both arguments), the self branch of `adf_sball_set`, `adf_sball_swap` (both arguments,
also x == y) check under ADF_CHECK_INVARIANTS. The internal swaps use a static `sb_swap` without check, so the
output argument of `set` is not checked (as in lball). The check of add, sub and mul moved from `binary` into the
three public functions, so the message names the public function. Test: `entry_check_of_every_public_function`
(INV build only) in `tests/test_sball.c`, one forked child per case, as in `tests/test_lball.c`: 9 forged cases,
controls that return silently, the forged output of `set` returns, add as a case.

R5 (prec = LONG_MAX aborts). `rfunc.h`: `ADF_REAL_PREC_MAX 2097152` (guarded, also in `sball.h`, because
`rfunc.h` includes `sball.h`); the sentence "the caller's error" is gone. `src/rfunc.c` (`real_apply`, `at_place`)
and `src/sball.c` (`binary`): prec above the limit returns LIMIT first, from prec alone, before any other status
and any allocation; outputs untouched; `where` = the archimedean place (also for a value with tag NONE, for a
prime as place, for a place not in x, for the complex tag, for root degree 0). `adf_sball_neg` has no prec. Tests:
`prec_limit` in `tests/test_rfunc.c` (each of exp, log, log_abs, sin, cos, sqrt, root at ADF_REAL_PREC_MAX, on
inputs whose value is cheap, contains the true value; at LIMIT+1, LIMIT+2, LONG_MAX-1, LONG_MAX: LIMIT at both
levels, also for a non-finite input, aliased, and the four place cases) and `prec_limit_of_the_ring_operations`
in `tests/test_sball.c` (add, sub, mul, the same). No result changed on inputs that gave OK: the vectors of
`tests/ref/vectors/f-slice2/` pass unchanged.

R6 (Julia leak). `tests/julia/sball.jl`: `realtext` clears and frees its temporary arb (`freearb`); `adele_of`
clears its two fmpz and lists each adele, which the test set clears and frees at its end.

R7. `docs/api-1f.md` L4a "Centre": one sentence with the exception for the centre 0 (nothing inverted, k not used;
the example of the reviewer). The quotient formula is unchanged.

R8. `docs/api-1f.md` S5: the exception of a result that is not finite (S7), and the LIMIT of prec.

## Red and green (details in `redgreen.log` and `redgreen-tests.log`)

Red, the reviewer's `lanes/f-review2/edge.c` built against the old tree: `precedence` printed
`partial_status=UNSUPPORTED where_inf=1`; the six invariant modes exited 0 (arch returned 9, num returned 1,
identical returned 1); `prec sin` and `prec root` aborted (allocation of 8070450532247928976 and
17293822569102704648 bytes). Red of the new tests: release test_sball 58 failed checks in 2 tests; test_rfunc
`prec_limit` failed and then aborted with the same allocation error; INV build against the old `sball.c` 17
failed checks in `entry_check_of_every_public_function`; the Julia probe under valgrind showed
"1,600 (960 direct, 640 indirect) bytes in 20 blocks are definitely lost".

Green (last lines, each under `timeout 900`, after the last source change):
- `make clean && make -j2 check-all`: exit 0, "check-all passed: make check, driver, exports, julia,
  mutate-selftest, memcheck-selftest" (its julia step runs `tests/julia/sball.jl`).
- `make clean && make -j2 check SAN=1`: exit 0, "check passed: all 65 test programs".
- `make clean && make -j2 check CC=clang`: exit 0, "check passed: all 65 test programs".
- `make clean && make -j2 check INV=1`: exit 0, "check passed: all 65 test programs" (an earlier INV run had one
  failed check in my own test, the message of add naming `binary`; repaired as said under R4).
- `sh lanes/m1-headers/check_headers.sh`: "check_headers: passed" (63 single-header compilations, 0 failures;
  clang++ skipped: absent).
- test_sball 18 tests, 873962 checks, 0 failed; test_rfunc 9 tests, 59670 checks, 0 failed.

The reviewer's programs after the change: `edge precedence`: `partial_status=LIMIT where_inf=0`; `edge s5`:
`status=NOT_DETERMINED` (the status is right; the text is corrected, R8); the six `edge-inv inv ...` modes abort
with "ADF_CHECK_INVARIANTS: adf_sball_arch / num_places / identical / set / swap / swap" and the control aborts as
before; `edge prec exp`, `sin`, `root` at LONG_MAX: `status=LIMIT`, no abort. The Julia valgrind probe: the
record of 20 blocks with 640 indirect bytes is gone; the process still exits 99 because of Julia's own JIT losses
(20 x 48 bytes from `__register_frame`, 8,099 bytes in 55 blocks in all; not adelefeld, as the review says).

## Not done

- R1 and R2 (lball `sub` and `div` LIMIT): not mine.
- No mutation run (brief, item 3).
- `lanes/f-review2/edge_runs.py` and the other reviewer scripts were not run; only the `edge` modes above and the
  Julia probe were.
- `ADF_REAL_PREC_MAX` is defined twice (guarded, same value) because `rfunc.h` includes `sball.h`; a single
  definition would need a header the brief did not give me.
- `docs/conventions.md` was not touched. The status classes of section 3.2 do not list LIMIT for the real
  functions; I did not check whether the row "Functions at places" (which lists LIMIT) covers the arb-level ones.
- The two other places in `docs/api-1f.md` that could mention statuses of the operations (outside the slice-2
  section) were not searched.
