# Lane f-slice14: result (the local zeta factors in C, WP 1F.9 last item)

All six slices (A to F) are done. `adf_local_zeta_factor_at(acb_t y, adf_place_t *where, const acb_t s,
adf_place_t v, slong prec)` is implemented as the design's rule Z4 says (docs/design/local-zeta.md), with the
name from CV-60. At a prime it computes `(1 - p^(-s))^(-1)`, at the real place `pi^(-s/2) Gamma(s/2)`, on complex
balls. Statuses are as in N-D20.

On the full oracle file every status matches the simulation: 1371 rows with an input. All 701 OK rows contain
all 5845 certified samples, and all 643 width checks at factor 64 pass. The exact count of each was checked.

The run uses no git command and no `bd`. The build directories (including `build/` of check-all) and the 11 MB
fixture file have been removed.

## Files

- New: `include/adelefeld/localfactor.h`, `src/localfactor.c`, `tests/test_localfactor.c`,
  `tests/ref/vectors/f-slice14/zeta.jsonl` (270 rows, 563960 bytes), `tests/julia/localfactor.jl`,
  `tests/driver/localfactor-values.{cmd,out}`, `tests/driver/localfactor-status.{cmd,out}`.
- Changed, appended only: `include/adelefeld.h` (+1 line; this is the export list that `tests/test_exports.sh`
  reads), `tests/test_julia.sh` (+2 lines), `tools/adf/adf.c` (+47: one enum entry, one table row, one function,
  one dispatch line), `tools/adf/README.md` (+19, a new section at the end), `docs/api-1f9.md` (+106: Y16, Y17,
  user calls). `Makefile` needed no change, because it uses a wildcard.
- Lane directory: `select_fixtures.py` (states the selection rule), `plant_faults.py`, `probes/*.c` (4 FLINT
  3.0.1 probes), `redgreen.md`, `progress.md`, and logs (faults, mutate, mutate2, check-all, san/inv/clang).

## Per slice

- **A (prime place).** The red run was a link error (the first test of the file). The first green run failed
  287 checks, all of them in the test reader, none in the code (see redgreen.md). Exact values: s = 1, 2, -1 at
  p = 2, 3 at five precisions. Pole lattice: k = -3..3 at the six primes up to 2^64-59, radii 10^-1..10^-60.
  That is 840 NOT_DETERMINED calls (around and segment) and 420 OK calls at distance 16r, each containing an
  acb_pow reference at the centre and corners. Pole membership is certified with a 1100-bit ball. Also tested:
  exact 0 DOMAIN; NaN, inf and inf-radius inputs DOMAIN; LIMIT first; prec -5, 1, 2, the cap and the cap+1;
  0, 1, 4, 9, 65535, 2^64-1 and 2^64-57 refused by `adf_place_prime`, with its output untouched; Re(s) = +-2^1000
  and 1 + i 2^1000 OK and enclosing; i 2^1000 NOT_DETERMINED.
- **B (real place).** Red: assertions, 990 failed checks against a stub. Exact values: every integer -41..41
  that is not a pole, checked against closed forms without Gamma (2: 1/pi, 1: 1, 4: 1/pi^2, -1: -2 pi). Poles
  0..-40 exact are DOMAIN. Balls, horizontal and vertical segments, and closed boundaries are NOT_DETERMINED
  (420 calls); near and near_imaginary balls are OK (210 calls). The design's box [-2.1,-1.9]+i[1.5,1.7] is OK
  at prec 256 and at prec 16. Recurrence bound: n = 64 is not LIMIT; n = 65 and 67 are LIMIT.
- **C (fixtures).** `timeout 170 python3 proto/zeta_checks.py --fixtures lanes/f-slice14/zeta-fixtures.jsonl`
  exited 0 with the design's counts. The subset has every status, every prime, both places, the 12 precision
  pairs (computed with the oracle's own functions) and the extreme arguments. Rule 7 adds the 4 rows that need
  the midpoint refinement. Committed subset: 270 rows, 92 OK, 228 samples, 34 width checks, 0 missed. The full
  file is run with `ADF_ZETA_FIXTURES=<path>`: 1371 rows, 0 status differences.
- **D (user calls).** Driver command: `local_zeta_factor_at S with PLACE`. The finite coordinate of the carrier
  is ignored, as the README says. The 12 + 20 expected lines were written from the formulas (conventions 9.5 at
  digits 5) before the command existed. Red: every line `error: PARSE`. Green: byte-equal on the first run.
  Julia `localfactor.jl`: red against a library without the symbol; green with 14 of 14.
- **E.** Y16 covers the value and the 8 steps the code adds to the design, each with why it keeps the enclosure.
  Y17 covers statuses, outputs, aliasing, limits, cost, and the FLINT 3.0.1 behaviour the statuses depend on.
  Each has a Check: line.
- **F.** Fault table and mutation testing are below.

## Checks (commands and results)

- `timeout 300 ./lanes/f-slice14/build/test_localfactor`: 10 tests, 36725 checks, 0 failed, about 3 s.
  - With `ADF_ZETA_FIXTURES`: 132283 checks, 0 failed.
  - It would fail on: a wrong status, an output written on failure, a missing `where`, a non-finite OK ball, a
    sample outside the ball, or a missed width target.
- `timeout 1700 make -j2 check-all` (once): exit 0. "check passed: all 80 test programs"; driver 66 cases with
  101211 lines, all equal; exports 463/463; Julia passed (with LD_PRELOAD of the system GMP); mutate and
  memcheck self-tests passed. No part had to be rerun.
- SAN: `make -j2 BUILD=lanes/f-slice14/build-san SAN=1 .../test_localfactor`, run with
  `ASAN_OPTIONS=detect_leaks=1`: 0 warnings, 0 sanitizer reports, 0 failed.
- INV+SAN: `INV=1 SAN=1`: 11 tests (including `debug_entry_check`), 36744 checks, 0 failed, 0 reports. With
  `ADF_INV_PLACE(v)` removed, `debug_entry_check` fails 6 checks.
- clang: `CC=clang`: 0 warnings, 0 failed. The driver built with clang against the clang library gives
  byte-equal output on both localfactor scripts (exit 0 and exit 1).
- Driver with the sanitizers: `ASAN_OPTIONS=detect_leaks=1 SAN=1 timeout 1200 sh tests/test_driver.sh`: 66
  cases, all equal.
- `sh tests/test_exports.sh`: 463 of 463 exported, including `adf_local_zeta_factor_at`.

## Fault table (design section 4; `plant_faults.py`, each fault alone in a scratch copy)

| Fault | Caught | First failing assertion |
|---|---|---|
| 1 finite poles tested only at k = 0 | **no** | none: masked (see findings) |
| 1+7 (fault 1 without the final finiteness check) | yes | `acb_is_finite` on OK (prime_pole_lattice, 2880 checks; the test then crashes, exit -11) |
| 2 DOMAIN on a positive-radius pole ball, at p | yes | `call(...) == ADF_NOT_DETERMINED`, around p=2 k=-3 (976 checks) |
| 2r the same at infinity (geometry) | yes | real_poles `around 0 j=1` (439 checks) |
| 3 exponent sign reversed | yes | `arb_contains_fmpq`, p=2 s=1 (test crashes later, exit -11) |
| 4 Gamma(s) instead of Gamma(s/2) | yes | real_exact_values s=1 value outside (374 checks) |
| 5 pi^(-s) instead of pi^(-s/2) | yes | real_exact_values s=-41 outside (501 checks) |
| 6 y written before the status | yes | `acb_equal(y, before)`, y changed on status 1 (4467 checks) |
| 7 final finiteness checks omitted | yes | `acb_is_finite` on OK in real_recurrence (2^1000 + i) (6 checks) |
| 8 where not written on failure | yes | `where is not v on status 1` (3003 checks) |
| 8b where written on OK | yes | `where written on OK` (2031 checks) |

## Mutation testing

The tool ran on a minimal copy of the tree (Makefile, include, src/{localfactor,place}.c, the test and its
vectors) under `--san --make 'make -s -j1 check SAN=1 INV=1'`, with `--limit 40 --seed 14 --jobs 2`. The limit
was 40 rather than 60 because one mutant takes about 53 s; the two runs took 18.3 minutes in total.

- **Run 1:** 25 killed, 12 survived, 3 did not compile. The survivors at 313, 319 and 329 (the refinement
  switched off) were a gap in the tests: the subset had no row that needs the refinement. Rule 7 added 4 such
  rows.
- **Run 2:** same mutants. 30 killed, 7 survived, 3 did not compile.

Surviving mutants, one line each (no entry was added to equivalent.txt; that file is not mine):

- 219 `mag_add(m0, f, inva)` and 222 `mag_add(m1, f, m1)`: equivalent, because addition commutes.
- 157 `acb_mul(q, f, q, w)`: equivalent, same reason.
- 289 `arf_sgn(lo) <= 1`: equivalent. For lo > 0 the next test, floor(min(hi,0)) = 0 >= lo, is false anyway.
- 62 `arf_sgn(x) <= 0` selects b = +log p at Re(m) = 0. Equivalent as an enclosure: |exp(+-a m)| = 1 there, and
  both stable formulas are valid with the same E.
- 240 (drop `+ log pi`) and 241 (drop `* M0`) in the bound B of Z6: **not equivalent**. They make B smaller,
  which is unsound in principle. They survive because B overestimates the true variation by a wide margin, so
  every tested sample is still inside. Catching them would need a direct test of B, and B is a static
  function. These two are not covered.

## Findings against the design (FLINT 3.0.1)

1. **Design step 4 says "Q = product (Z+j)".** acb_rising_ui must not be used for that product: on 3.0.1, for
   z = -2 +/- 0.05 + i(0.8 +/- 0.05) and n = 6, it returns a ball of radius about 110 that contains 0. The plain
   loop of acb_mul excludes 0 (probes/rec_probe.c). Using acb_rising_ui made 29 near-pole real boxes
   (s = -8 .. -40) NOT_DETERMINED. The code uses the loop.
2. **The design's direct-Gamma counterexample holds on 3.0.1 as well.** acb_gamma(S/2) for
   [-2.1,-1.9]+i[1.5,1.7] is non-finite at 48, 160 and 288 bits, so the recurrence gives the OK value. The same
   holds for the recurrence-limit boxes, and Gamma(2^999 + i/2) is [+/- inf] (probes/gamma_probe.c). So there are
   no rows where the 3.0.1 status differs from the 3.3.1 simulation: 0 differences over 1371 rows.
3. **Fault 1 of the design is not observable alone.** Replacing the denominator test by a k = 0 test still
   returns NOT_DETERMINED. Division by a ball that contains 0 gives a non-finite acb on 3.0.1, and step 5's
   finiteness check refuses it. The design's two checks overlap here, so the required fault test cannot fail.
   Combined with fault 7, it is caught.
4. **The oracle's point error is absolute** (about 1e-229 max(1,|v|)). It cannot resolve components of modulus
   1e-302 (s = +-1000 + i) or 2^(-2^1000) (s = -2^1000), which the returned ball resolves with relative
   precision. As design section 4 says, the test then raises the certificate (acb_pow at 4000 bits): 26 samples
   in the subset, 52 in the full file. With design Z7 item 7, points are tested by containment only. The
   width target applies to positive-radius rows, as in the oracle. The point 1 + i 2^1000 at prec 128 is wide.
5. **Test-side only.** arf_get_mag and mag_set_fmpz_2exp_fmpz round a 30-bit dyadic up by one unit; the reader
   uses mag_set_ui_2exp_si and checks the round trip (probes/mag_exact.c).
6. **Design :493 names `lanes/d-zeta/zeta-driver.adf`, which does not exist.** The driver lines were written as
   tests/driver files instead.

There are no findings against the specification. SPEC 9.3.7, catalogue Proposition 9 and conventions :944-951
hold on every test.

## Fixture rows whose status differs from the simulation

None: 0 of 1371.

## Not done

- The real place at the cap prec = 2^21 is not run (Gamma at 2M bits is slow). Only cap+1 (LIMIT) is tested
  there; the cap itself is run at p = 2.
- No long differential fuzz run.
- The run time of s with gigantic exponents (|Re s| or |Im s| near 2^(2^60)) is not measured. The tested
  magnitudes stop at 2^1000.
- Mutation testing ran 40 of 218 mutants, not 60.
- Two survivors in B are not covered (above).
- The orchestrator still has to add LIMIT to the conventions.md:223 row (N-D20).
- Avoidable costs, noted and not optimised:
  - the midpoint refinement evaluates Gamma even when the candidate is already narrow;
  - the prefactor exponential is computed again at the midpoint.

## Sources pending

- Gelfond-Schneider (design Z3). The code does not use it: it recognises only the exact 0 at p.
- The FLINT 3.0.1 acb/arb exp and Gamma evaluator source and range guards (design :38). This lane measured the
  behaviour with probes; it did not read the source.
