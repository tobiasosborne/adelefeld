# Lane m1-repair-recon: report

Lane of the findings R1, R2 and part of R6 of reviewer `arith` (docs/reviews/m1/arith/review.md).
Work resumed on 2026-09-28 after an interruption from outside; the earlier part of the lane (the
red run, the first version of the code, one mutation run) was already in the worktree and was not
redone. This report describes the state of the worktree now.

## 1. What was done

R1 (BLOCKER), R2 (MAJOR) and the part of R6 that concerns `src/recon.c` are repaired. Nothing in
`docs/SPEC.md` was changed; the code follows row M1-D3 of section 15 (docs/SPEC.md:862), which the
orchestrator took for exactly these two findings.

### 1.1 The test first (red)

`tests/test_recon_limit.c` is new. It has four tests:

- `the_real_balls_of_the_review_are_answered_limit`: the six balls of R1 and R2, built with
  `arb_mul_2exp_fmpz` and `mag_mul_2exp_fmpz`: the real ball `3 * 2^(2^64+1)` against the exact 6,
  `2^-(2^64+3)` against 1/8, `2^(2^64)` against 1, `2^-(2^63)` against 1, `2^(2^36)` against 1 and
  `1 +/- 2^(-2^36)` against 1. Each must answer `ADF_LIMIT` and leave the output at the marker
  -99/7 (conventions 4.3). The builder checks the exponent it produced, so no case can pass because
  the exponent came out smaller than asked for.
- `one_above_the_bound_gives_limit`: exponents of the midpoint and of the radius exactly one step
  above `ADF_RECON_EXP_MAX` and two steps above it, both signs, for the midpoint and for the
  radius, with the finite ball the exact 1 and `Zhat`.
- `at_the_bound_the_ball_is_computed`: exponents exactly `ADF_RECON_EXP_MAX` and `ADF_RECON_EXP_MAX - 1`
  in both signs, with the answers `ADF_OK`, `ADF_NO_SOLUTION` and `ADF_NOT_UNIQUE` worked out by
  hand in the comment of every case (the comment gives the candidates as `a + N k` with the range
  of `k`).
- `a_zero_midpoint_a_zero_radius_and_two_zeros`: a zero midpoint with a radius, a zero radius with
  a midpoint, and both zero, each against five finite balls.

The expected answers at the bound are the following, with `P = 2^(2^20-1)` (the point whose
`ARF_EXP` is `2^20`), `Q = 2^(2^20-2)` (the radius whose `MAG_EXP` is `2^20-1`):

- `[P, P]` against 1: `P > 1`, no candidate, `ADF_NO_SOLUTION`.
- `[P, P]` against `Zhat`: the candidates are the integers `k` with `P <= k <= P`, so `k = P` and
  `ADF_OK` with `q = P`.
- `[P, P]` against `1 + 2 Zhat`: `1 + 2k = P` has no integer solution, since `P - 1` is odd.
  `ADF_NO_SOLUTION`.
- `[P, P]` against `P Zhat` and against `Q Zhat`: `k = 1` and `k = 2` respectively, `ADF_OK` with
  `q = P`.
- `[Q, 3Q]` against `Q Zhat`: `k = 1, 2, 3`, `ADF_NOT_UNIQUE`. Against 1: `1 < Q`,
  `ADF_NO_SOLUTION`.
- `[0, 2P]` against 1: `ADF_OK` with `q = 1`. Against `Zhat`: `2P + 1` candidates,
  `ADF_NOT_UNIQUE`.
- `1 +/- P` against 1 and against 2: both inside, `ADF_OK`. Against `Zhat`: `2P + 1` candidates,
  `ADF_NOT_UNIQUE`.
- `1 +/- 2^-(2^20-1)` (`MAG_EXP = -2^20`): against 1 `ADF_OK`, against 2 `ADF_NO_SOLUTION`,
  against `Zhat` the width is below `N = 1` and the only candidate is 1, `ADF_OK` with `q = 1`.
- the point `2^-(2^20+1)` (`ARF_EXP = -2^20`) against `2^-(2^20+1) + Zhat`: the canonical triple
  is `(1, 2^(2^20+1), 2^(2^20+1))` and `k = 0` is the only candidate, `ADF_OK`.
- `[-P, -P]` against `Zhat`: `ADF_OK` with `q = -P`; `[-3Q, -Q]` against `Zhat`: `ADF_NOT_UNIQUE`.
- `3Q` (a two-bit mantissa at the bound) with the radius `Q` gives `[P, 2P]`: against `P Zhat` the
  candidates are `P` and `2P`, `ADF_NOT_UNIQUE`; against 1, `ADF_NO_SOLUTION`.

Before the repair the file failed: the three balls with an exponent above `2^64` answered `ADF_OK`
and wrote the output, the ball of the exponent `-2^63` answered `ADF_NO_SOLUTION`, and the ball
`2^(2^36)` then aborted in GMP (`Cannot reallocate memory (old_size=16 new_size=8589934600)`,
exit 134); see `red-run.log` and `old-library-run.log`. A copy of the test with the huge exponents
replaced by exponents of `2^20` (`/tmp/test_recon_limit_nohuge.c`, the file is gone; the log is
`red-run-nohuge.log`) failed in 12 checks and did not abort, so the failure of the file is not only
the allocation.

### 1.2 The repair of `src/recon.c`

- `adf_adele_reconstruct` tests the two exponents of the real ball before
  `arb_get_interval_fmpz_2exp` is called, in the new function `arb_exponents_within_limit`: the
  exponent of the midpoint (`ARF_EXPREF`) and of the radius (`MAG_EXPREF`) must be within
  `ADF_RECON_EXP_MAX` in absolute value, unless that field is zero (`arf_is_zero`, `mag_is_zero`).
  The status is `ADF_LIMIT`. The test uses `fmpz_cmp_si` only, never a machine word.
- The test comes after `arb_is_finite`, so a ball outside the contract still answers `ADF_DOMAIN`
  and not `ADF_LIMIT`: the exponent of an infinite midpoint is `ARF_EXP_POS_INF`, which is above
  the bound.
- `fmpq_set_dyadic` takes the bound `max_exp` and returns an `int`. It takes `fmpz_abs` of the
  exponent, compares it with `max_exp`, and only then reads it as a machine word
  (`fmpz_get_si` on a value already shown to fit). The negation of the exponent as a `slong`, which
  was the undefined behaviour the sanitizer reported for the exponent `-2^63`
  (docs/reviews/m1/arith/review.md R1), is gone: the shift is the absolute value.
- The old comment that said an exponent outside the range of a machine integer "is not reachable in
  practice" is removed.
- `fmpq_canonicalise` stays, and the comment at the call gives the reason: every function of the
  `fmpq` module assumes canonical input (refs/src/flint-3.0.1/fmpq.rst:21-26).
- The comment block on the local backend (the old lines 50 to 61) is brought up to date: the local
  backend exists (src/fball_local.c), `adf_fball_get_fmpz3` gives the canonical global triple
  (fball.h:155-160), and `adf_fball_get_center` and `adf_fball_get_radius` are not used because the
  radius of a local value is `K/d`, not the stored `H/d` (fball.h:162-169).
- The citation of `arb_get_interval_fmpz_2exp` now quotes the whole of what the manual says,
  including the warning of memory and swapping (refs/src/flint-3.0.1/arb.rst:472-477), and the
  paragraph on the bound of M1-D3 was added.

## 2. Files written

- `src/recon.c` (changed; the only source file of the lane).
- `tests/test_recon_limit.c` (new, 614 lines).
- `lanes/m1-repair-recon/report.md` (this file).
- `lanes/m1-repair-recon/run_limit.sh`: runs `build/test_recon_limit` under `ulimit -v 2000000`.
- `lanes/m1-repair-recon/probe_exp.c`, `probe_exp.out`: the measurement of the exponents and of the
  sizes that `arb_get_interval_fmpz_2exp` returns (see section 4).
- `lanes/m1-repair-recon/recon_nocanon.c`: a copy of `src/recon.c` with the one mutant
  `fmpq_canonicalise(q)` removed, used for the third reproducer of the review.
- Logs in `lanes/m1-repair-recon/`: `red-run.log`, `red-run-nohuge.log`, `old-library-run.log`,
  `old-library-run-nohuge.log` (the red runs), `green-limit.log`, `limit-run.log`, `check1.log`,
  `check-san.log`, `check-clang.log`, `build-default.log`, `mutate-recon.log`,
  `recon_huge_exp.new.out`, `recon_big_alloc.new.out`, `recon_san.new.out`,
  `recon_san_ulimit_fail.out`, `recon_mut111.new.out`, `repro-build.log`, `repro-build-san.log`,
  `lane.log`, `probe_exp.out`.
- `docs/reviews/m1/arith/checks/`: the review's scripts `build.sh` and `build_san.sh` wrote the four
  binaries `recon_huge_exp`, `recon_huge_exp.san`, `recon_big_alloc`, `recon_big_alloc.san` into
  that directory. No file of the review was changed.

`tests/test_recon.c` was not changed: no case was missing there, and the tests of this lane belong
in the new file. No file outside the list above was touched.

## 3. Every check that was run

Times are wall clock, on 2 cores at most.

1. Red run against the unrepaired library, whole file:
   `make -j2 build/test_recon_limit && ./build/test_recon_limit` -> 8 failed checks, then
   `GNU MP: Cannot reallocate memory (old_size=16 new_size=8589934600)`, `Aborted (core dumped)`,
   `exit=134`, `real 0m0,169s`. Log `red-run.log`.
2. Red run with the four huge exponents replaced by exponents of `2^20` (so that the file finishes):
   12 failed checks, 1 failed test, `exit=1`. Log `red-run-nohuge.log`, source
   `/tmp/test_recon_limit_nohuge.c`, not kept.
3. `make -j2 check` (default compiler) -> `check passed: all 31 test programs`, 352 `ok` lines,
   `exit=0`, `real 0m6,423s`. Log `check1.log`. The test of this lane is in it:
   `4 tests, 778 checks, 0 failed checks, 0 failed tests`.
4. `make clean && make -j2 check SAN=1` -> `check passed: all 31 test programs`, `exit=0`,
   `real 0m13,949s`, no sanitizer report. Log `check-san.log`. This is the run that covers the
   undefined behaviour of R1: the case `2^-(2^63)` is in the file.
5. `make clean && make -j2 check CC=clang` -> `check passed: all 31 test programs`, `exit=0`,
   `real 0m6,336s`, no warning. Log `check-clang.log`.
6. `sh lanes/m1-repair-recon/run_limit.sh` -> `4 tests, 778 checks, 0 failed checks, 0 failed
   tests`, `0.00user 0.01system 0:00.01elapsed`, `maxresident 6220k`, `exit=0`,
   `real 0m0,017s`, under `ulimit -v 2000000`. Log `limit-run.log`. The same program without the
   limit took `real 0m0,030s`.
7. `docs/reviews/m1/arith/checks/recon_huge_exp` built with `build.sh` against
   `build/libadelefeld.a`, run under `ulimit -v 2000000` and `timeout 60`: all 8 cases
   (`A`, `B`, `B'`, `A2`, `C2`, `C3`, `B2`, `D`) answer `status = LIMIT (10), q = -99`, `exit=0`.
   The review had `ADF_OK` with `q = 6`, `q = 1/8` and `q = 1` for the first three.
   Output `recon_huge_exp.new.out`.
8. `docs/reviews/m1/arith/checks/recon_big_alloc 1`, `2`, `3`, built with `build.sh`, each under
   `ulimit -v 2000000`: `status = LIMIT`, `exit=0` for all three. The review had exit 134 with
   `Cannot reallocate memory` for cases 1 and 2. Output `recon_big_alloc.new.out`.
9. The two sanitized builds of those programs (`build_san.sh`, ASan and UBSan over `src/*.c`, with
   `-fno-sanitize-recover=undefined`), without a memory limit: all cases `LIMIT`, `exit=0`, no
   sanitizer report. Output `recon_san.new.out`. Under `ulimit -v 2000000` they cannot start at
   all, because ASan reserves 15 TB of address space at start-up
   (`failed to allocate 0xdfff0001000 (15392894357504) bytes`); output
   `recon_san_ulimit_fail.out`.
10. The third reproducer, `recon_mut111`: the comparison of `docs/reviews/m1/arith/checks/recon_mut111.out`
    repeated on the repaired file. `docs/reviews/m1/arith/checks/recon_fuzz.c` was linked once
    against `src/*.c` and once against `lanes/m1-repair-recon/recon_nocanon.c` with the rest of
    `src/*.c`; seeds 1, 2, 3, 20000 cases each, bits 8: byte-identical in all three seeds
    (40000 lines each). Output `recon_mut111.new.out`.
11. `make mutate FILES=src/recon.c JOBS=2 LIMIT=300` -> `89 mutants in 16034.9 s: 71 killed,
    10 survived, 8 not compiled, 0 timed out, 0 excused`; the target fails, `exit=2`. Log
    `mutate-recon.log`. The ten survivors and the reason for each are in section 5.
    The same run in the earlier part of the lane (`git`-uncommitted state of 15:21) gave the same
    counts and the same survivors, in 777 s; the run of 16034 s is the same work slowed by the
    other lanes on the machine.
12. The measurement `lanes/m1-repair-recon/probe_exp` (15 configurations): `probe_exp.out`. It gives
    the two conventions the test file relies on and the sizes that decide the bound. See
    section 4.

## 4. Sources pending

- `[source pending: the code of arb_get_interval_fmpz_2exp, which is not on disk]`. The manual is
  (refs/src/flint-3.0.1/arb.rst:461-477), the declaration is /usr/include/flint/arb.h:346. Two
  facts the code and the tests use are therefore measurements, not quotations:
  1. the exponent that `arb_get_interval_fmpz_2exp` returns lies between the smaller of the two
     exponents of the ball, less one, and the larger of them (`probe_exp.out`, 15 configurations
     with exponents from `-2^20` to `2^20` and 5: for example `E = 0, R = -1048576` gives
     `exp = -1048577`, `E = 1048576, R = 0` gives `exp = -1`, `E = R = 1048576` gives
     `exp = 1048576`). This is why `fmpq_set_dyadic` is called with `ADF_RECON_EXP_MAX + 1` and why
     its check never rejects in the tests. The comment at the call in `src/recon.c` says this and
     marks the source as pending. If it ever rejected, the answer would be `ADF_LIMIT`, a refusal
     and not a wrong answer.
  2. the size of the two integers: at `ARF_EXP = MAG_EXP = 1048576` the integers have 1048577 bits
     (`a_bits = 0 b_bits = 1 exp = 1048576` means the interval is `[1, 2] * 2^1048576`), at
     `ARF_EXP = 1048576, R = 0` they have 1048577 and 1048578 bits. So a ball at the bound needs
     integers of about `ADF_RECON_EXP_MAX + 2` bits.
- `[source pending: the code of mag_set_ui_2exp_si, of mag_mul_2exp_fmpz and of arb_set_fmpz_2exp]`.
  Only the declarations are on disk (/usr/include/flint/mag.h:615, /usr/include/flint/arb.h:187). The
  two conventions measured on this machine and used by `tests/test_recon_limit.c`: the arf value
  `m 2^e` (m odd) is stored with `ARF_EXP = e + bits(m)` (refs/src/flint-3.0.1/arf.rst:14-20), so
  the arf of the value `2^(E-1)` has `ARF_EXP = E`; and `mag_set_ui_2exp_si(r, 1, y)` stores the
  value `2^y` with `MAG_EXP = y + 1`, so the mag of the value `2^(R-1)` has `MAG_EXP = R`. Every
  builder of the test checks the exponent it produced with `ARF_EXPREF` or `MAG_EXPREF`, so the
  tests do not depend on these conventions being right, only on the builders producing what the
  case asks for.
- `[source pending: the code of fmpz_mul_2exp]`: `docs/reviews/m1/arith/checks/probe_mul_2exp.out`
  of the review reports that `fmpz_mul_2exp(t, 1, 2^62 + 1)` returns 2 on this machine. Nothing in
  this lane depends on it: the guard of M1-D3 refuses every exponent above `2^20` before any
  `fmpz_mul_2exp` is called with it.

## 5. Survivors of the mutation run

Ten mutants of `src/recon.c` survive the whole test suite (`mutate-recon.log`). They are listed
with the reason; none of them is listed in `tools/mutate/equivalent.txt`, which this lane did not
edit.

1. `src/recon.c:123:38` zero_one `'0' -> '1'`: `ok = (fmpz_cmp_si(m, max_exp) <= 0)` becomes
   `<= 1`. A magnitude of `max_exp + 1` is then accepted as well. The only caller passes
   `max_exp = ADF_RECON_EXP_MAX + 1`, and the test of the ball keeps the exponent of the interval
   at most `ADF_RECON_EXP_MAX + 1` (measurement of section 4), so the accepted set of the helper is
   the same for every input the function accepts. The only difference is one bit of shift.
2. `src/recon.c:124:40` zero_one `'0' -> '1'`: `sh = ok ? (ulong) fmpz_get_si(m) : 0` becomes
   `: 1`. `sh` is read only inside `if (ok)`, where the mutant computes the same value.
3. `src/recon.c:130:27` cmp `'>=' -> '>'`: for `exp = 0` the first branch gives `mn * 2^0 / 1` and the
   second gives `mn / 2^0`, the same rational, and `fmpq_canonicalise` is called on both paths.
4. `src/recon.c:130:30` zero_one `'0' -> '1'`: the same statement as 3 (`>= 1`), same reason.
5. `src/recon.c:134:13` drop_call `fmpz_one(fmpq_denref(q));`: the output of the helper is an
   `adf_rat` of the caller, which `adf_rat_init` leaves with the denominator 1, and the numerator is
   overwritten with `t = mn * 2^sh` on the line above, so the call writes 1 where there already is 1.
6. `src/recon.c:177:34` zero_one `'0' -> '1'`: `fmpq_cmp(lo->q, hi->q) > 0` becomes `> 1`. `fmpq_cmp`
   returns only -1, 0 and 1, so the mutant never answers `ADF_NO_SOLUTION` there.
7. `src/recon.c:231:13` swap_args `adf_rat_mul(c, c, N)` -> `adf_rat_mul(c, N, c)`: `adf_rat_mul` is
   `fmpq_mul` (src/rat.c:236-239), which is commutative on the value and produces canonical output
   (refs/src/flint-3.0.1/fmpq.rst:21-26), so the stored fraction is the same.
8. `src/recon.c:232:13` swap_args `adf_rat_add(c, c, a)` -> `adf_rat_add(c, a, c)`: the same
   argument for `adf_rat_add`, which is `fmpq_add`.
9. `src/recon.c:230:13` drop_call `fmpz_one(fmpq_denref(c->q));`: `c` is a fresh `adf_rat` of the
   function, its denominator is 1, and the numerator is set from `kmin` on the line above, so the
   call writes 1 where there already is 1. Same reason as 5.
10. `src/recon.c:143:9` drop_call `fmpq_canonicalise(q);`: this is the mutant the review discussed
    (checks/recon_mut111.out). Check 10 of section 3 repeats the review's comparison on the
    repaired file: the fuzzer output is byte-identical for three seeds and 20000 cases each, so no
    test of the project can separate it. The call is not equivalent in general and stays: the
    negative branch writes the numerator `mn` and the denominator `2^sh`, and `mn` need not be odd,
    so `lo->q` and `hi->q` can leave the helper in a form that `fmpq_cmp` (src/recon.c:177),
    `adf_rat_sub` and `adf_rat_div` then use; every function of the `fmpq` module assumes canonical
    input (refs/src/flint-3.0.1/fmpq.rst:21-26), and the code below relies on it. The tests cannot
    check a precondition of a function of FLINT. The reason is in the comment at the call.

The eight mutants that do not compile are counted as not compiled, not as survivors, and are not
listed here; they are in `mutate-recon.log`.

## 6. What is not done

- `tests/test_recon.c` was not extended. No case of the bound belongs there, since the set statement
  and its three statuses are already covered there and the bound is a property of the adele call.
- No mutant of section 5 was killed by a new test. Nine of the ten are equivalent by the reason
  given; the tenth depends on a precondition of FLINT that no test of this project can observe. A
  test that would separate it would have to rely on behaviour of `fmpq` on non-canonical input,
  which the manual does not promise.
- The local backend of `adf_fball_reconstruct` is not fuzzed here. The comment block was brought up
  to date, but the review says the local backend of `src/fball.c` is another reviewer's, and
  `tests/test_fball_local.c` is not a file of this lane.
- The findings R3, R4 and R5 of the review, and the other citations of R6 (src/rat.c,
  src/adele.c, src/fball.c), were not touched; they are not in this lane.
- The mutation run took 16034 s against 777 s for the same run earlier in the lane. The reason is
  the load of the other lanes on the machine, not a change of the file: the mutant count, the
  killed count and the survivors are the same in both runs.

## 7. Findings against the specification

Nothing in `docs/SPEC.md` had to be changed, and no line of it is contradicted by this lane. Two
points for the orchestrator.

1. `include/adelefeld/recon.h:18-19` gives two formulations of the same condition, and the first is
   not the one that holds. It says `adf_adele_reconstruct` returns `ADF_LIMIT` "when the exact end
   points of the real ball would need more than ADF_RECON_EXP_MAX bits", then gives the real rule,
   the exponent test of row M1-D3. The two differ at the boundary: a ball whose midpoint exponent is
   exactly `ADF_RECON_EXP_MAX = 2^20` is converted, and `probe_exp.out` measures its end points at
   1048577 and 1048578 bits, that is `ADF_RECON_EXP_MAX + 2` bits. The code follows row M1-D3 of
   `docs/SPEC.md:862`, which is the exponent test; the first formulation in the header is loose. The
   header is not a file of this lane, so it was left alone. Nothing fails because of it: the tests
   `at_the_bound_the_ball_is_computed` and `one_above_the_bound_gives_limit` state both readings and
   the code satisfies the second, which is the one that decides.
2. R1 and R2 are repaired by refusing, not by deciding. For a ball with an exponent above `2^20`
   the set statement of SPEC 9.2 and conventions 6.8 has an answer (`ADF_NO_SOLUTION` in five of
   the six cases of the review), and the function now answers `ADF_LIMIT`. This is decision M1-D3,
   taken by the orchestrator on 2026-09-28 (`docs/SPEC.md:862`) and written in `recon.h:16-23`, so
   it is not a finding against the specification; it is recorded because the cost is that
   `adf_adele_reconstruct` is no longer a decision procedure on admitted balls with extreme
   exponents, only within `-2^20 <= exponent <= 2^20`.

## 8. Sources quoted in the files of this lane

Checked on this machine on 2026-09-28, with the line numbers as they are on disk:

- refs/src/flint-3.0.1/arb.rst:461-466 (the form of the interval), :468-470 (the abort on an
  infinite or NaN ball, and on a failed allocation), :472-477 (the warning quoted in
  `src/recon.c:45-49`), :606-609 (`arb_is_finite`).
- refs/src/flint-3.0.1/fmpq.rst:21-26 ("all functions in the fmpq module assume that inputs are in
  canonical form, and produce outputs in canonical form").
- refs/src/flint-3.0.1/arf.rst:14-20 (the internal exponent `e` with `0.5 <= |m| < 1`), :39
  ("Since exponents are bignums, overflow or underflow cannot occur").
- /usr/include/flint/arf.h:81 `ARF_EXP_ZERO`, :87 `ARF_EXP`, :88 `ARF_EXPREF`, :235-238
  `arf_is_zero`, :694-698 `arf_set_fmpz_2exp` (which changes nothing for a zero).
- /usr/include/flint/mag.h:212 `mag_zero` (`fmpz_zero(MAG_EXPREF(x))`), :229-233 `mag_is_zero`,
  :615 the declaration of `mag_set_ui_2exp_si`.
- /usr/include/flint/arb.h:346 the declaration of `arb_get_interval_fmpz_2exp`, :187
  `arb_set_fmpz_2exp`.
- /usr/include/flint/fmpz.h:472 `fmpz_cdiv_q`, :473 `fmpz_fdiv_q` (rounding towards +infinity and
  towards -infinity).
- include/adelefeld/fball.h:155-160 and :162-169, cited in the comment block of `src/recon.c`.
- src/rat.c:236-239 `adf_rat_mul` is `fmpq_mul`, cited in section 5.

Two citations that were wrong in this lane were corrected while writing this report: `src/recon.c`
cited `arf.rst:9-11` for "Since exponents are bignums" (the sentence is at `arf.rst:39`), and
`tests/test_recon_limit.c` cited `arf.rst:9-15` for the internal exponent (`arf.rst:14-20`) and
`arb.h:187` for the declaration of `arb_get_interval_fmpz_2exp` (`arb.h:346`). Both are fixed and
all three builds above were run again after the fix.
