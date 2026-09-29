# Lane f-slice6: functions at a prime of a partial ball, and the driver (milestone 1F, WP 1F.1 completed)

`adf_sball_exp_at`, `adf_sball_log_at` and the other `_at` functions of `include/adelefeld/rfunc.h`
return `ADF_UNSUPPORTED` when the place is a prime. The series at a prime exist now
(`include/adelefeld/lfunc.h`: `adf_lball_exp`, `adf_lball_log`, `adf_lball_Log`, reviewed:
`docs/reviews/f1/review-lfunc.md`). This lane connects them, so that `f_at` works end to end from an
adele at every place the function exists, and gives the user commands of the driver for it.

Functions of the slice:
- `adf_sball_exp_at`, `adf_sball_log_at` at a prime of the partial ball: the component at the prime
  goes through `adf_lball_exp` and `adf_lball_log`; the result is a partial ball over the one place
  (SPEC 9.3.1), as at the real place. A new `adf_sball_Log_at` (the Iwasawa logarithm at a prime; at the
  real place `log_abs`? read SPEC 9.3 and decide; say what you decided). The precision argument: at a
  prime the `prec` of the `_at` functions is the requested ABSOLUTE precision `N` of `lfunc.h`; say so
  in the header; the limit `ADF_REAL_PREC_MAX` applies at the real place, the limits of `lfunc.h` at a
  prime. The statuses of `lfunc.h` are passed on with `where` the prime.
- `sin_at`, `cos_at`, `sqrt_at`, `root_at`, `log_abs_at` at a prime stay `ADF_UNSUPPORTED` with `where`
  the prime (they are later slices); the header says so.
- The driver `tools/adf/adf.c`: commands `project X with PLACES` (an adele in the value form, a list of
  places: primes in decimal and the word `real`, separated by single spaces; output: the components
  in the canonical order, separated by `; `, each `real: <ball>` or `<p>: <local ball>`), and
  `exp_at X with PLACE`, `log_at X with PLACE` (output: the one component). The text of a local ball
  in the OUTPUT of the driver: `<centre> + O(<p>^<N>)` with the centre the canonical rational
  `p^v u` written as the driver writes a rational, and `<value>` alone for an exact local ball. This is
  a text of the driver, not a value form of the library (decision N-D1 is the pattern; the orchestrator
  records this one).

Read first: `CLAUDE.md`, `docs/workflow.md`, `lanes/COMMON.md`, `lanes/COMMON-C.md` (rule 6 does not hold
for `rfunc.h`; rule 5 is replaced by item 4 below), `docs/SPEC.md` 9.3.1, 9.3.2, 15 (15.4 too),
`include/adelefeld/rfunc.h`, `sball.h`, `lfunc.h`, `lball.h`, `src/rfunc.c`, `docs/api-1f.md` (slice 2),
`docs/api-1f4.md`, `tools/adf/adf.c` (the section "the solver commands" of lane drv-s is the pattern for
a command whose operands are not value texts), `tools/adf/README.md`, `lanes/drv-s/result.md`.

**You own:** `src/rfunc.c`, `include/adelefeld/rfunc.h`, `tests/test_rfunc.c` (additions),
`tests/test_rfunc_prime.c` (new), `tests/julia/f_at.jl` (new), `tools/adf/adf.c`, `tools/adf/README.md`,
`tests/driver/f-*.cmd` and `tests/driver/f-*.out` (new), `docs/api-1f.md` (a new section at its end),
`tests/ref/vectors/f-slice6/` (new, below 1 MB), `lanes/f-slice6/`. In `tests/test_julia.sh` you may add
the lines your file needs. Everything else is read-only: `src/lfunc.c` is being optimised by another
lane at this moment (its results do not change); use the public functions of `lfunc.h`.

1. Header sentences first. 2. Tests first, red then green (`lanes/f-slice6/redgreen.log`): the result
   of `_at` at a prime equals the result of the `lfunc.h` function on the component (identical
   fields); every status of `lfunc.h` arrives with `where` the prime; output untouched on a status;
   aliasing; an adele projected to `{2, 3, 5, real}` and `exp_at`, `log_at` at each place. The driver
   cases with expected lines written BY HAND from mathematics you can check in Python with exact
   rationals (say in the first comment of each case where every line comes from): for example
   `exp_at 5 with 5` at precision 8, `log_at 6 with 5`, `log_at -1 with 2` (the exact 0), `exp_at 1
   with 5` (`error: DOMAIN`), `project 2/3 with 2 5 real`; hostile input (a place that is not a prime, a
   repeated place, 1000 places, a missing operand).
3. The code. `tests/julia/f_at.jl`: a user makes the adele of `5/3`, projects it to `{5, real}`, and
   takes `exp` at 5 and at the real place.
4. Show that the tests bite: three faults of your choice in a scratch copy under `build/`. No mutation
   run, no fuzz target.
5. `make clean && make -j2 check-all`, `make clean && make -j2 check SAN=1`,
   `make clean && make -j2 check CC=clang`, `make clean && make -j2 check INV=1`,
   `sh lanes/m1-headers/check_headers.sh`, `SAN=1 sh tests/test_driver.sh` pass, each under
   `timeout 900`, in the foreground; give the last line of each.

Result: `lanes/f-slice6/result.md` and the same text as your final message. Times in your notes are
read from `date`. Leave no compiled binary in your lane directory.
