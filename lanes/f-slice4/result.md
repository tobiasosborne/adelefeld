# Lane f-slice4: result (exp, log and Log at a prime, milestone 1F, WP 1F.4)

Worktree at f0630c9 (master). `refs/src` was linked to the main checkout's copy (not in git). No git command that
changes state, no `bd`. All runs under `timeout`, at most 2 jobs. No compiled binary in the lane directory.

## What was built

Three functions on `adf_lball` (header `include/adelefeld/lfunc.h`, code `src/lfunc.c`):

| Function | Encloses, for every t in x | Exact results | Statuses |
|---|---|---|---|
| `adf_lball_exp(y, x, N)` | `exp(t)`, domain `p^c Z_p` | `exp(0) = 1` | OK, DOMAIN, NOT_DETERMINED, LIMIT |
| `adf_lball_log(y, x, N)` | the series `log(t)`, domain `1 + p Z_p` (at 2 also `3 + 4 Z_2`) | `log(1) = 0`, `log(-1) = 0` at 2 | OK, DOMAIN, NOT_DETERMINED, LIMIT |
| `adf_lball_Log(y, x, N)` | `Log(t) = log(u)`, `t = p^m w u`, domain `t != 0` | `Log(+-p^m) = 0` | OK, DOMAIN (exact 0), NOT_DETERMINED (ball containing 0), LIMIT |

`N` is the requested absolute precision. A ball result is `f(a) + p^K Z_p`, `a` the exact input or the centre of the
input ball, `K = N` for an exact input and `K = min(N, E)` for a ball, where `E` is the exponent of the image:
`exp`: `M`; `log`: `M`, or 2 for `M = 1` at `p = 2`; `Log`: `r = M - v(a)`, or 2 for `r = 1` at `p = 2`
(Propositions 10, 11). If `N >= E` the result is the image itself, so it is the smallest ball. If `N < E` it is the
unique ball of exponent `N` that contains the image. `Log` loses `m` digits for `m > 0` and gains `-m` digits for
`m < 0`. `y` may be `x`. On any status other than OK the output is untouched.

Evaluation uses the library's own integer arithmetic modulo `p^W`:
- `exp` is a Horner sum over the one denominator `L!`. The term count is that of Proposition 7, and
  `W = K + v_p(L!)` is the working precision of Proposition 8 (statement F4).
- `log` is the term-by-term sum of Proposition 8, with the tight count of Proposition 7b (F5).
- `Log` never forms the root of unity `w` (F3):
  - at odd `p` it computes `log(a)` when `a = 1` modulo `p`, else `log(a^(p-1)) / (p - 1)`;
  - at 2 it computes `log(s a)` with `s a = 1` modulo 4.
- `log` is computed as `Log` on its domain (Proposition 11, step 3).
- The domain test is `adf_lball_contains` / `adf_lball_overlaps` against the domain written as a ball (F1).

FLINT's `padic` is used in the tests only.

Files written (all new unless marked):
- `include/adelefeld/lfunc.h`
- `src/lfunc.c`
- `tests/test_lfunc.c`
- `tests/julia/lfunc.jl`
- `proto/lfunc_checks.py`
- `tests/ref/vectors/f-slice4/lfunc_cases.jsonl` (790 rows) and `lfunc_points.jsonl` (2165 rows), 320 KB in all
- `docs/api-1f4.md`: the section of this slice, with decisions and statements F1 to F7 and their proofs
- `include/adelefeld.h` (changed): two lines, the include and the list entry
- `tests/test_julia.sh` (changed): `lfunc.jl` added in both paths
- `lanes/f-slice4/`: `gen_vectors.py`, `faults.py`, `bench_lfunc.c`, `progress.md`, `redgreen.log`, `faults.log`,
  `bench.log`, and the logs of the five checks

## Decisions (alternatives in docs/api-1f4.md, "Decisions")

1. **Absolute precision `N`.** It is the same kind as the lball field `N` and as SPEC 9.3.2. Alternative: relative
   precision, as a separate function later.
2. **`K = min(N, E)` for balls.** A requested `N` never exceeds what the input determines. Alternative: always `N`,
   with a status when `N > E`. Rejected, because it would lose a true result.
3. **Exact results only for the rational values 0 and 1** listed above (F2 proves that for `log` and `Log` these are
   all the rational values at rational points). For `exp` of a rational `x != 0` the result is always a ball.
4. **Order of checks.** An input beyond the lball exponent bounds is LIMIT, tested before the domain (as in lball).
5. **FLINT's domain is not ours.** `log` accepts `3 + 4 Z_2`; FLINT's `padic_log` refuses it (`padic.rst:506-507`).
6. **No Teichmüller in C** (F3). The reference does use it (statement T, proved in `proto/lfunc_checks.py`), so the
   two routes check each other.
7. **One limit is set by an intermediate value.** LIMIT is also returned when the working power `p^W`
   (`W = K + D`, Proposition 8) has more than `ADF_LBALL_BITS_MAX` bits:
   - This departs from the lball rule "never because of an intermediate value". The rule cannot hold for a sum that
     must be formed modulo `p^W` with `W > K`.
   - No power is formed where the centre is known without a sum (exact results; `K <= c` for log/Log; unit part 1;
     centre 0 or `K <= v(a)` for exp). So `Log(1 + 5^E Z_5) = 5^E Z_5` at `E = 2^60` is OK.
   - There is no bound on time. Cost is about `T` multiplications of `W bits(p)`-bit integers, and `N` is the
     caller's choice, as `prec` is in `rfunc.h`.
8. **No binary or rectangular splitting.** Per the brief, there is no optimisation.

## Checks run (commands and results)

- `timeout 300 python3 -B proto/lfunc_checks.py`: all checks passed.
  - `check_teichmuller`: 672.
  - `check_log_routes`: 604. The raw series equals the Teichmüller route. It also covers `v_2(log 3) = 2`,
    `v_3(log 4) = 1` and `v_2(log 5) = 2`.
  - `check_exp_log_inverse`: 1444.
  - `check_domains`: 1593 balls against sampled points.
  - `check_ball_enumeration`: 1020 balls and 13281 points. For each ball, every point value is inside the result,
    and when the exponent is `E` the values modulo `p^(E+1)` are not all equal.
- `timeout 300 python3 -B lanes/f-slice4/gen_vectors.py`: deterministic (two runs gave identical md5s).
  - The reference agrees with FLINT values of the earlier probe: `exp 3 = 958` and `exp 12 = 5125` mod `3^8`;
    `exp 4 = 77` mod `2^8`; `log 4 = 3 * 664` mod `3^8`; `log 5` and `log(-3)` at 2 agree with 31 and 61 mod `2^6`.
- `timeout 300 ./build/test_lfunc`: 10 tests, 374421 checks, 0 failed (4.6 s). With INV=1: 11 tests, 374430 checks,
  0 failed. Per test, with what would have made it fail:
  - **`cases_from_the_reference`**: 790 lines (OK 649, DOMAIN 100, NOT_DETERMINED 41).
    - Every field is compared with the reference, and the call is repeated with the output aliased to the input.
    - Fails on any different status or field, a written output on a status, or a changed aliased input.
    - The lines cover the SPEC 9.3.2 precision cases (the losses at 3, 12 at `p = 3` and at 2, 10 at `p = 2`,
      `log(-1)`, balls at the edge of each domain), random inputs at `p = 2, 3, 5, 7, 11, 13` and `2^64 - 59`,
      precision 2000 at `p = 2, 3, 5`, and 60 at `2^64 - 59`.
  - **`points_from_the_reference`**: 2165 exact grid points, 28 of them exact results.
    - C at precision `M + 2` must equal the reference residue. The reference sums 3 digits past the needed count.
  - **`ball_enumeration_over_the_grid`**: 7600 calls, 61032 point values enclosed, 3852 smallest-ball witnesses.
    - Covers every ball of the grids at `p = 2, 3, 5, 7`, including centres with valuation down to -2 for `Log`,
      with `N_req = E, E - 1, E + 3, LONG_MAX`.
    - The status must match the grid's domain membership, the exponent must be `min(N_req, E)`, every point value
      must lie inside the result, and at `K = E` there must be two values that differ modulo `p^(E+1)`.
  - **`spec_precision_cases`**: by hand.
    - The regression of review N4: `exp(3)` and `exp(12)` at `3^8` are disjoint, and both lie in the result of
      `exp(3 + 9 Z_3)`, which has exponent 2.
    - `log(-1) = 0` exactly; `v_2(log 3) = 2`; `log(1 + 2 Z_2) = 4 Z_2`.
    - `Log(3 + 9 Z_3) = 3 Z_3`, `Log(2 + 8 Z_2) = 4 Z_2`; a loss of 4 digits at `m = 4`, a gain of 3 at `m = -3`.
    - `Log(p) = 0` at 2, 3, 5, 7 and `2^64 - 59`.
  - **`identities_as_containment`**: 1046 round trips and 360 homomorphism checks at `p = 2, 3, 5, 7, 13, 2^64 - 59`.
    - For balls, `log(exp B) = B` and `exp(log B) = B` as sets; for exact `x`, `log(exp x)` contains `x`.
    - `Log(xy)` meets `Log x + Log y`, and they are equal when both are balls; `Log(p x) = Log x`.
  - **`flint_second_opinion`**: 1000 random inputs compared, 0 refused, at `p` in {2, 3, 5, 7, 11, 13, 101,
    2^31 - 1, 2^64 - 59} and precision 1 to 200.
    - Compared with `padic_exp`, `padic_log`, and `padic_teichmuller` plus `padic_log` for `Log`.
    - At 2, the library's `log` of `x = 3` mod 4 is compared with FLINT's `log(-x)`.
  - **`precision_2000`**: compared with FLINT at precision 2000 (300 for `log(1 + p)` at `2^64 - 59`), and the result
    at 2000 contains the result at 100.
  - **`every_status`**: every status of the header, each with its output untouched and with the aliased call.
    - Covers LIMIT of each kind: the input, the result exponent, and the working power.
    - Also covers the no-power cases at `E = 2^60`.
  - **`exact_results_and_shortcut_values`**: the values behind the shortcuts.
  - **`entry_check_of_every_function`** (INV only): a composite `p` aborts with the function named; the control
    returns silently.
- **Red and green** (`lanes/f-slice4/redgreen.log`).
  - Red by a link error first.
  - Then red by assertions against a stub that returns UNSUPPORTED: 15505 failed checks in 9 tests.
  - Green after one change. The coverage guard `balls > 10000 && tight > 1000` had been written before the universe
    was counted; it was set to the universe's size (`>= 7600`, `>= 3800`). No other check was changed.
- **Faults** (`timeout 900 python3 -B lanes/f-slice4/faults.py`, `faults.log`): all 7 caught.

  | # | Fault | How it was caught |
  |---|---|---|
  | 1 | `Log` radius from the input precision, without the loss | 6825 failed checks |
  | 2 | `exp`: one term too few | 1447 failed checks |
  | 3 | `log`: one term too few | 25745 failed checks |
  | 4 | no division by `p - 1` | 12124 failed checks |
  | 5 | `exp` domain `p Z_p` at 2 | the program aborted (division by 0 in the count for `v = 1` at 2), not an assertion |
  | 6 | `E = r` for `r = 1` at 2 | 64 failed checks |
  | 7 | `log` domain `Z_p` | 24762 failed checks |

- **Julia.** `timeout 900 sh tests/test_julia.sh`: `exp, log, Log through ccall`, 12 passed. It checks `exp(5)` and
  `log(6)` at 5 to 20 digits against Julia's own `Rational{BigInt}` series, `log(-1) = 0` at 2, and `exp(1)` at 5
  giving DOMAIN. The Julia file was written after the C code and was not seen red.
- **The five checks of the brief, item 5.** Logs are in `lanes/f-slice4/check-*.log`. Last lines:
  - `make clean && timeout 900 make -j2 check-all`: "check-all passed: make check, driver, exports, julia,
    mutate-selftest, memcheck-selftest" (66 test programs; exports: 385 declared and exported, 0 exported and not
    declared).
  - `make clean && timeout 900 make -j2 check SAN=1`: "check passed: all 66 test programs"; 0 sanitizer reports.
  - `make clean && timeout 900 make -j2 check CC=clang`: "check passed: all 66 test programs".
  - `make clean && timeout 900 make -j2 check INV=1`: "check passed: all 66 test programs".
  - `timeout 900 sh lanes/m1-headers/check_headers.sh`: "check_headers: passed" (`lfunc.h`: 3 functions).
- **Timing row** (`bench.log`, single runs, noisy; the library against FLINT's `padic`):

  | Case | Library | FLINT |
  |---|---|---|
  | `exp`, p = 3, N = 100 | 46 us | 10 us |
  | `exp`, p = 3, N = 2000 | 1.95 ms | 0.60 ms |
  | `log`, p = 3, N = 2000 | 5.9 ms | 0.87 ms |
  | `Log`, p = 3, N = 1000 | 1.16 ms | 0.10 ms |
  | `Log`, p = 2^64 - 59, N = 100 | 2.8 ms | 1.8 ms |

## What is proved and what is not

- **Proved** in `docs/api-1f4.md`, stepwise, on top of the propositions of `functions.md` it cites:
  - F1: the domain test.
  - F2: the exact values.
  - F3: `Log` without `w`.
  - F4: `exp` with one common denominator.
  - F5: the log sum, and the lower bound of the valuation taken from a residue.
  - F6: the precision and minimality of a ball result.
  - F7: the limits, with no `slong` overflow.
- **Not proved:** these statements have had no second reader. Propositions 7b and 8 of `functions.md`, which the code
  uses, have had no second reader either (SPEC 9.3 says so for 7b). The tests are bounded enumerations and samples,
  not proofs. Only one fault was caught by an abort rather than an assertion.

## Not done

- `adf_sball_exp_at` and the other `_at` functions still return UNSUPPORTED at a prime (`sball.c` is not this lane's
  file). Wiring them to `lfunc.h` is the next step.
- The statements F1 to F7 are not merged into `functions.md` (not owned).
- No benchmark under `bench/` (not owned); there is only the lane-local timing row. No mutation run and no fuzz target,
  per the brief.
- No optimisation: `log` does one `fmpz_invmod` per term. The avoidable costs:
  - the per-term inverse (one inverse of `lcm(1..T)` would do);
  - the second modular reduction of `zk` and `S` in every step;
  - Horner and splitting methods (FLINT is 3 to 10 times faster here).

## Sources pending

None new. `functions.md` already marks as pending the Teichmüller naming and the Iwasawa normalisation. Statement T
(Teichmüller by powering) is proved in the reference, not cited.

## Findings against the specification

- **SPEC 9.3.2 and PLAN row 1F.4 describe the implementation as "our wrapper around FLINT's centre evaluation"**
  ("FLINT is called only for the centre, after these checks"). By the orchestrator's decision the library evaluates
  the series itself and FLINT is only a test oracle. The specification text now describes a design that was not
  built and should be amended.
- **The lball rule "LIMIT never because of an intermediate value" cannot be kept by series functions** (decision 7
  above). `lfunc.h` states the one exception; `conventions.md` 3.2 has no row for these functions yet.
- No mathematical statement of SPEC 9.3.2 was found false. Every precision case of its table is a test and passes.

## Next slice proposed

1. **1F.7:** `sin`, `cos`, `sinh`, `cosh` at a prime by the same Horner-with-one-denominator code (F4 carries over
   with `T_sin`, `T_cos` of Proposition 7). This includes the cosine hull `1 + p^(2N - v_p(2)) Z_p` as a separately
   tested case.
2. **The `_at` wiring:** connect `adf_sball_*_at` at a prime to `lfunc.h`, so that `f_at(x, {p})` works end to end
   from an adele.
3. **Review:** an adversarial review of this slice with its own oracle, with first attention on F4, F5 and the
   no-power shortcuts.
