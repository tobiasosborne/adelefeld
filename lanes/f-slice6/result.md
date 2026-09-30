# Result of lane f-slice6 (2026-09-30, 01:50 to 02:30 by `date`)

Worktree at 9a039e8, `refs/src` linked. Done: `adf_sball_exp_at`, `adf_sball_log_at` and a new `adf_sball_Log_at` work at a
prime of a partial ball, through `adf_lball_exp/log/Log`; the driver has `project`, `exp_at`, `log_at`.

## Files

Changed or new: `include/adelefeld/rfunc.h`, `src/rfunc.c`, `tests/test_rfunc.c` (2 tests changed), `tests/test_rfunc_prime.c`
(new, 9 tests), `tests/julia/f_at.jl` (new), `tests/julia/sball.jl` (ONE assertion changed, not on the owned list, see below),
`tests/test_julia.sh` (lines for f_at.jl), `tools/adf/adf.c`, `tools/adf/README.md`, `tests/driver/f-at-prime.{cmd,out}`,
`f-project.*`, `f-places-hostile.*`, `f-places-1000.*`, `docs/api-1f.md` (section "Slice 1F.4-b" at the end),
`tests/ref/vectors/f-slice6/at_prime.jsonl` (1824 lines, 149 KB), `lanes/f-slice6/` (`gen_vectors.py`, `expected.py`,
`mk_thousand.py`, `faults.py`, logs, `redgreen.log`, `progress.md`, `red1.txt`, `red2.txt`). No binary is left in the lane
directory.

## What the library does now (header block "A PRIME" in rfunc.h)

- At a prime `p` of `x`: `exp_at`, `log_at`, `Log_at` copy the component, call the `lfunc.h` function with `N = prec`, and
  return the partial ball over `{p}` (arch NONE, len 1, `loc[0]` = the result, identical fields). The statuses of `lfunc.h`
  (DOMAIN, NOT_DETERMINED, LIMIT) arrive with `where` = the prime; `y` untouched on a status; `y` may be `x`.
- `log_abs_at`, `sin_at`, `cos_at`, `sqrt_at`, `root_at` at a prime: UNSUPPORTED, `where` = the prime (after the check that
  the prime is a place of `x`). A root of degree 0 at a prime is UNSUPPORTED too (the prime is looked at first).
- `prec` at a prime is the ABSOLUTE precision N. It is passed unchanged: no clamp to 2, no `ADF_REAL_PREC_MAX`. At the real
  place nothing changes (bits, clamp to 2, LIMIT above 2^21 with `where` = the archimedean place, before all other checks).

## Decisions, each with its alternative

1. `Log_at` at the real place is the real logarithm (domain t > 0, the same as `log_at`), not log |t|. Source: SPEC 9.3.2
   (`docs/SPEC.md` 597-599): "The real coordinate needs a positive input, or the separately named `log_abs`"; `log_abs_at`
   exists. Alternative: log |t|. Not done.
2. One `prec` argument with two meanings (bits at the real place, N at a prime), by the place. Alternative: separate names.
   Consequence, written in the header and in `docs/api-1f.md`: the LIMIT from `ADF_REAL_PREC_MAX` is decided only when `v` is
   the archimedean place. A prime that is not a place of `x` is now DOMAIN with `where` = that prime at every `prec` (it was
   LIMIT with the archimedean place). This is the change of contract of the brief.
3. Driver output of `exp_at`/`log_at`: the partial ball WITH its label (`5: 349831 + O(5^8)`, `real: 1`), as `project` prints
   it. The brief says "the one component"; the bare component was the alternative. Not done because `1` alone does not say
   whether it is real or p-adic. If the orchestrator wants the bare form it is one line (`adf_drv_put_sball` call) and 25
   lines of `f-at-prime.out`.
4. Driver: the value X of `exp_at`/`log_at`/`project` may be a rational (made into `(q ; q)` at the setting prec), a finite
   ball (no real place: `real` is DOMAIN) or an adele; a complex adele and every other kind are UNSUPPORTED. Places: `real` or a
   decimal without a leading zero; other tokens PARSE; a decimal that is no prime (0, 1, negative, composite, above 64 bits)
   DOMAIN; a repeated place DOMAIN; `exp_at`/`log_at` need exactly one place (PARSE otherwise). Syntax first, as in the README's
   order of the checks. A driver `Log_at` command was not added (not asked).
5. The text of a local ball in the driver output: `<centre> + O(<p>^<N>)`, `<value>` when exact, the centre a rational as the
   driver prints one (`1/5 + O(5^2)` for a negative valuation). It is a driver text, not a value form (N-D1 pattern);
   documented in the README and in the comment of `adf.c`. The orchestrator records it (as the brief says).

## Tests, in the order written

Red first (`lanes/f-slice6/redgreen.log`):
- `tests/test_rfunc_prime.c`: first a link error (`adf_sball_Log_at` undefined), then with a stub (Log_at = log at the real
  place, UNSUPPORTED at a prime) 7 of 9 tests failed by assertion: 1825, 790, 30, 35, 3, 9, 3 failed checks (`red1.txt`; the
  run also ended in a segfault of the test on an unwritten `y`, which I guarded afterwards).
- `tests/test_rfunc.c` changed to the new contract, run on the old code: `sball_at_statuses_and_places` 6 failed checks,
  `prec_limit` 112 (`red2.txt`).
- Then `at_prime` in `src/rfunc.c`: `test_rfunc_prime` 9 tests, 98016 checks, 0 failed; `test_rfunc` 9 tests, 59670 checks, 0
  failed. Two defects of MY tests appeared at the first green run and were repaired in the tests (vector field `c` is a
  string; exp of O(7^5) is OK, not NOT_DETERMINED, since M >= c).
- Driver: I wrote the driver code BEFORE the four case files (a break of the order). To have a genuine red I built the
  driver of the main checkout (before this lane) and ran the cases: 25/25, 13/13, 17/38, 5/6 expected lines not produced.
  Then all 35 cases passed at the first run of the new driver, except that my first `f-at-prime.out` had no label; that was my
  decision 3 taken after the run and the file was changed to the labelled form.
- Julia `f_at.jl` (21 tests) passed at the first run; there was no red run of it.

Oracles:
- `at_prime.jsonl`: exp, log, Log of 38 exact rationals at p = 2, 3, 5, 7, N = 1, 4, 9, 16 (1824 lines: OK 876, of which 132
  exact, DOMAIN 948), computed by `gen_vectors.py` with Fractions only (series; Teichmueller by a^(p^n) for Log), no line of
  `src/lfunc.c`. Each line runs through the `_at` function on the sball {real 7, p, q} and aliased.
- The 790 lines of `tests/ref/vectors/f-slice4/lfunc_cases.jsonl` (balls, exact, statuses) through `_at`: status, fields of the
  component, `where`, output untouched. Note: that file has no LIMIT line (LIMIT 0 in the count); LIMIT is covered by hand rows.
- Hand tests: 16 status rows (DOMAIN, NOT_DETERMINED, LIMIT with `where` = the prime, `y` untouched, NULL `where`); `prec` as
  N (N = 2^21+1, 2^21+2, LONG_MAX for the exact results; N = 1, 0, -1, -3, -10^6, LONG_MIN+1 equal to `adf_lball_*`); other
  functions UNSUPPORTED/DOMAIN; COMPLEX tag with a prime; `Log_at` at the real place (2, -2, ball 0 +- 1, limit); the adele of
  20/3 and 31 projected to {2, 3, 5, real} and every function at every place; aliasing with 4 places; exp(5) = 6 mod 25 by hand.
- Driver cases (`tests/driver/f-*.cmd`): expected lines from `lanes/f-slice6/expected.py` (exact Fractions; real balls from
  `proto/text_grammar.py print_real` with two different radii to show the text does not depend on arb's radius; the 1000-place
  file by `mk_thousand.py`). Cases: exp_at 5 with 5 (`5: 349831 + O(5^8)`), log_at 6 with 5 (`329930`), log_at -1 with 2
  (`2: 0`), exp_at 1 with 5 (DOMAIN), project 2/3 with 2 5 real (`real: 0.666667 +/- 3.4e-7; 2: 2/3; 5: 2/3`), finite balls,
  prec 1/3/20, the 64-bit prime 2^64-59, hostile input (38 lines in `f-places-hostile`: non-primes, repeated places, syntax
  faults, missing operands, cadele, lball kind), 1000 places (`f-places-1000`: 6 lines, the largest prime 7907).
- Julia `f_at.jl`: adele of 5/3 projected to {5, real}, exp at 5 (N = 12) equal to Julia's own series mod 5^12, exp at real (200
  bits), log at 5 DOMAIN, Log at 5 equal to Julia's Teichmueller-based value, DOMAIN at 3, UNSUPPORTED sin, N = 2^22 exact vs
  LIMIT at the real place.

## The tests bite (`lanes/f-slice6/faults.py`, log `faults.log`; scratch copies under build/, no mutation tool)

9 faults, 9 caught: A where = the archimedean place at a prime (1109 failed checks in test_rfunc_prime, 4 in test_rfunc);
B prec clamped to 2 (584); C Log_at computes log at a prime (677); D `ADF_REAL_PREC_MAX` applies at a prime (17 and 112); E
Log_at at real is log |t| (2); F sin_at at a prime computed (7 and 3); G driver accepts a leading zero (case f-places-hostile);
H driver prints in the given order (f-project, f-places-1000); I exp_at calls log_at (f-at-prime). No mutation run, no fuzz
target, as the brief says.

## Suites (last lines; logs in `lanes/f-slice6/`)

- `make clean && make -j2 check-all`: `check-all passed: make check, driver, exports, julia, mutate-selftest,
  memcheck-selftest` (`check passed: all 68 test programs`; `test_driver: 35 cases, 100519 expected lines, all equal
  (SAN=0)`; `test_julia: passed (with LD_PRELOAD=...libgmp.so.10)`). The log has a line `mutate: FAILED: 34 mutant(s) survived`
  from the self-test of the mutation tool (it is a self-test of the tool; exit 0), not a result on this lane.
- `make clean && make -j2 check SAN=1`: `check passed: all 68 test programs`.
- `make clean && make -j2 check CC=clang`: `check passed: all 68 test programs`.
- `make clean && make -j2 check INV=1`: `check passed: all 68 test programs`.
- `sh lanes/m1-headers/check_headers.sh`: `check_headers: passed`.
- `SAN=1 sh tests/test_driver.sh`: `test_driver: 35 cases, 100519 expected lines, all equal (SAN=1)`.
Each under `timeout 900`, foreground (the tool limit is 600 s for a call and every suite ended in about 3 to 7 minutes).

## Files that I changed and that the brief does not list (need the orchestrator's eye)

- `tests/julia/sball.jl`: one assertion (`exp_at` at 2 of 2/3 was UNSUPPORTED, now DOMAIN, since v_2(2/3) = 1 < 2) plus one added
  line (`sin_at` at 2 is UNSUPPORTED). Without it `make check-all` fails at the Julia step. No test was weakened.
- `tests/test_rfunc.c` is on the owned list, but two of its existing tests changed their expectations (the change of contract,
  decision 2): `sball_at_statuses_and_places` (exp, log at the primes: DOMAIN) and `prec_limit` (a prime that is not a place: DOMAIN
  with `where` = the prime, not LIMIT).

## Not done, and things to know

- No mutation run and no fuzz (the brief says so). No review by a second model.
- The Julia test has no red run (written after the code was green in C).
- `docs/SPEC.md` is not changed. The N-D decision for the driver text of a local ball is not recorded in SPEC 15.4 (the
  orchestrator records it).
- No `Log_at` command in the driver. No `sin`, `cos`, `sqrt`, `root`, `log_abs` at a prime (UNSUPPORTED by contract).
- Avoidable cost, as in rule 7: `at_prime` copies the component and the result once more than needed (one `adf_lball_set` in
  `get_lball` and one in `set_arb_lballs`); the sum in `lfunc.c` dominates. The driver forms every line text before it writes.
- A finding, not against the spec: `tests/ref/vectors/f-slice4/lfunc_cases.jsonl` contains no LIMIT case, so LIMIT at a prime
  is tested only by the hand rows of `statuses_arrive_with_the_prime` (input N above `ADF_LBALL_EXP_MAX`, requested N =
  LONG_MAX) and by `prec_is_the_absolute_precision_at_a_prime`.
- Lines over 116 characters: a few in `rfunc.h` and `src/rfunc.c` exist in the parts I did not write; my own new comment lines
  are mostly within the limit, but I did not run a formatter over them.
