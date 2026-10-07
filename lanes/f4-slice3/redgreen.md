# Red and green runs, lane f4-slice3 (slice 4e)

Builds: `make -j2 BUILD=lanes/f4-slice3/build lanes/f4-slice3/build/test_rfun_fourier`; runs under `timeout`.

1. RED A (the first test of the file): `tests/test_rfun_fourier.c` written in full, declarations appended to
   `include/adelefeld/rfun.h`, no code: link failure, 74 undefined references to adf_rfun_fourier,
   adf_rfun_derivative, adf_rfun_integral, adf_rfun_norm2 (`red-a.log`).
2. Code for the four functions appended to `src/rfun.c` in one step (not one function at a time; the
   per-function red evidence is the fault table of step F, where each function's planted faults fail the
   test). First green attempt failed in the test, five corrections, all in the test and none in the code:
   - F(F(x)) tightness at `Re(A) = 10^-30`: coefficients of size `(2 pi A)^-j` cancel at 128 bits; the bound
     is kept for `Re(A) >= 1/100` and the boundary records are checked by containment (stated in the test).
   - Integrals, norms and values at `Re(A) = 10^-30`: exponents of size 2^100 at 128 bits; bound 2^-16 there
     (stated), first tried 2^-24 (failed at a value, relative error near 2^-23).
   - The same bound must not tighten a ball record (record 41): `max(te, -16)`.
   - An exact result (`-16`) does not contain the oracle's 400-bit ball around `-16`: an exact result part
     must lie in the oracle ball instead (`part_in`).
   - Two sentinels (`z`, `n`) were not reset after earlier OK calls.
   GREEN: `rfun_fourier: 54248 checks`, 3.6 s.
3. Driver (step D): commands coded first (no red run of the driver, as in slice 4d); fixture
   `tests/driver/rfun-fourier` derived by hand (mpmath digits, printer of 9.5). First run: 16 of 17 lines
   equal; the eval line differed only in the format of the radius: I wrote `3.7e-4`, the printer writes
   `0.00037` (fmt of 9.5: exponent -4 is positional). A defect of my derivation, corrected in the fixture.
   Second run: `sh tests/test_driver.sh`: 84 cases, 101507 expected lines, all equal (SAN=0).
4. Faults (step F, `plant_faults.py`): 19 of 20 caught in the first run; "transform work cap one unit low"
   survived (the cap test was 724/725, not at the exact sum). RED for the new test: the exact boundary
   (472 terms charging exactly 2^20 pass; 475 terms charging 2^20 + 1 give LIMIT). Rerun: 20 of 20 caught
   (`faults2.log`).
5. Mutation survivor 262:40 (`adf_rfun_add`, slice 4d): new test in `caps` (a sum of exactly 2^16 coefficients
   passes, 2^16 + 1 gives LIMIT); the mutant, planted again, fails at that check.
6. Final: plain 64923 + 54258 checks; SAN=1 with leak detection 64923 + 54258; INV=1 64939 + 54271; clang
   64923 + 54258; all exit 0.
