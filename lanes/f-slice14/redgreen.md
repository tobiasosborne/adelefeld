# f-slice14 red and green runs

## Slice A (prime place)

- RED 1 (link error, the first test of the file): tests/test_localfactor.c with the slice A tests
  (prime_exact_values, prime_pole_lattice, prime_statuses_and_outputs, prime_handles, prime_extreme_arguments,
  prime_vectors) and include/adelefeld/localfactor.h, no src/localfactor.c.
  `make -j2 BUILD=lanes/f-slice14/build lanes/f-slice14/build/test_localfactor`:
  `undefined reference to 'adf_local_zeta_factor_at'`, collect2 exit 1.
- First run with src/localfactor.c (prime place; real place a stub returning NOT_DETERMINED): 287 failed checks,
  all in prime_vectors and all of the test, not the code: (a) the radius of a fixture row was not stored exactly
  (arf_get_mag rounds a 30-bit dyadic up by one unit on FLINT 3.0.1; probe lanes/f-slice14/probes/mag_exact.c;
  the reader now uses mag_set_ui_2exp_si and checks the round trip); (b) 26 sample boxes overlapped but were not
  inside the result: components of modulus about 1e-302 (s = +-1000 + i) and 2^(-2^1000) (s = -2^1000), below the
  oracle's absolute point error 1e-229; the test now raises the certificate precision as design section 4
  (:421-423) says, by acb_pow at 4000 bits; (c) the width target was applied to the point row 1 + i 2^1000, which
  the oracle does not do (Z7 item 7): points are now checked by containment only.
- GREEN A: `timeout 300 ./lanes/f-slice14/build/test_localfactor`: 6 tests, 23383 checks, 0 failed.
  prime_vectors: 222 rows, 81 OK, 201 samples contained, 30 width checks, 0 missed, status differs 0, 26 raised
  certificates. With ADF_ZETA_FIXTURES=lanes/f-slice14/zeta-fixtures.jsonl (the full 11 MB file): 918 prime rows,
  483 OK, 3939 samples, 432 width checks, 0 missed, 0 status differences, 52 raised certificates.

## Slice B (real place)

- RED B (assertions; the real place is the stub that returns NOT_DETERMINED): real_exact_values 497 failed
  checks, real_poles 465, real_recurrence 6, real_vectors 22; slice A tests still pass. 10 tests, 34447 checks,
  990 failed (log lanes/f-slice14/red-B.log).
- First run with the real procedure: 29 failed checks in real_poles (near -8 .. -40 and near_imaginary -8 ..,
  radius 10^-1 .. 10^-60: NOT_DETERMINED instead of OK). Cause in the code: the recurrence product was
  acb_rising_ui, which on FLINT 3.0.1 returns for z = -2 +/- 0.05 + i (0.8 +/- 0.05), n = 6 a ball of radius
  about 110 that contains 0 (probe lanes/f-slice14/probes/rec_probe.c); the design prescribes the product
  (:142) and the oracle multiplies in a loop. Replaced by the loop of acb_mul.
- GREEN B: 10 tests, 36986 checks, 0 failed, 2.9 s. real_vectors: 51 rows, 14 OK, 42 samples, 7 width checks,
  0 missed, 0 status differences. With the full fixture file: 453 real rows, 218 OK, 1906 samples, 211 width
  checks, 0 missed, 0 status differences; with the prime rows, 1371 rows (all rows that have an input), 701 OK,
  5845 samples, 643 width checks: the oracle's counts.
- INV entry check: the test debug_entry_check (forged handles 4, 1, 2^64 - 1 abort with a line naming the
  function; prec above the cap returns LIMIT first) passes under INV=1; with the line ADF_INV_PLACE(v) commented
  out it fails with 6 failed checks ("forged 0 not caught", ...); restored.

## Slice D (driver, Julia)

- Expected lines of tests/driver/localfactor-values.cmd (12 lines) and localfactor-status.cmd (20 lines) written
  from the formulas and conventions 9.5 before the command existed (mpmath 90 digits for the decimals).
- RED D: `build/adf < tests/driver/localfactor-values.cmd`: every line `error: PARSE` (unknown operation), exit 1.
- GREEN D: after the command: both scripts agree byte for byte with the expected files on the first run (exit 0
  and exit 1 as their #!exit lines say). No expected line was changed after the run.
- Julia RED: tests/julia/localfactor.jl against a library without the function (the main checkout's
  build/libadelefeld.so of 2026-10-04 23:31): "Error During Test" (the symbol is missing), 2 passed, 1 error.
- Julia GREEN: against build/libadelefeld.so of this worktree (tests/test_exports.sh: 463 of 463 declared
  functions exported): status=0, status=7, 14 of 14 passed (LD_PRELOAD of the system libgmp, the known quirk of
  tests/test_julia.sh).
