# Red-green log, lane t5-slice3 (slice 5b: adf_local_gamma_at, adf_local_epsilon_at)

Build: `timeout 300 make -s -j2 BUILD=lanes/t5-slice3/build-plain lanes/t5-slice3/build-plain/test_tate_gamma`.
Run: `timeout 300 lanes/t5-slice3/build-plain/test_tate_gamma`.

1. Vectors. `timeout 180 python3 -B lanes/t5-slice3/generate_vectors.py`. First run: ZeroDivisionError in the
   exact-text helper at p = 2, alpha = 2, s = 2, which is an exact gamma pole (alpha = p^(s-1)); the grid now emits
   it as a DOMAIN row. Second run: 867380 bytes, above the 400 KB budget; radii shortened to three digits and the
   alpha radius put on one coordinate per row (alternately); 721 rows, 1950 samples.
2. Red (first test of a new file, a compile error): tests/test_tate_gamma.c before the declarations:
   "'adf_local_epsilon_at' undeclared" (red-link.log).
3. Declarations appended to localfactor.h; stage 1 of the code: unramified and real gamma; the ramified branch
   and epsilon returned a placeholder status. Red as assertions (red-stage1.log): 8 tests, 40069 checks,
   535 failed. Besides the placeholder branches: 6 failures of real_values_and_zeros (a ball beside a real zero
   contained 0) and 8 width failures of real vectors around -4 and -5. Cause: acb_rgamma on a ball near a pole of
   Gamma is loose. Probe (scratch, FLINT 3.0.1, 160 bits), z = -k +/- 2^-12 on both coordinates:
   k = 0..3 radii 3.7e-4, 6.1e-3, 3.8e-2, 8.8e-2; the variation is about k! 2^-12 sqrt(2) (3.5e-4 .. 2.1e-3).
   The shifted product z (z+1) ... (z+n-1) rgamma(z+n) gives radii 2.5e-4, 2.5e-4, 4.9e-4, 1.5e-3.
4. rgamma_shifted (the identity rgamma(z) = z ... (z+n-1) rgamma(z+n), no division): red-stage1b.log, 521 failed
   checks, all in the placeholder branches (ramified gamma vectors 165, epsilon vectors 166, and the ramified,
   epsilon, duality and status tests that call them).
5. Stage 2: G_minus from the phases, ramified gamma, epsilon. First run: 606 failed checks, all vector
   containment: the generator wrote exact values as decimal balls ("1.0 +/- 1.01e-59"), which an exact output
   cannot contain. Generator fix: exact integer coordinates are written exactly. Then 4 failures: the real
   part of gamma for the real character (3, 2) at real s is exactly 0 (the C sum of exact cardinal phases
   cancels), while the 60-digit reference was a ball around 2.6e-62. Generator fix: a coordinate is written as
   exact 0 when exact cyclotomic arithmetic (sympy, G against its conjugate X -> X^(m-1) modulo Phi_m) proves G
   real or imaginary and alpha^a, T^a lie on axes. Green (green-stage2.log): 8 tests, 47919 checks, 0 failed.
6. Driver. tests/driver/tate-gamma.cmd and .out written by hand first. Red: `timeout 300 sh tests/test_driver.sh`
   exit 1, ten "error: PARSE" lines (red-driver.log). Green after adding tate_gamma and tate_epsilon, except the
   radii of the two exactly representable decimals, which depend on internal rounding and had been guessed as
   1e-38: the run printed 3e-39 (-0.75) and 1.1e-38 (1). Corrected from the run; the script's comment says so.
   Every other expected line (the ramified value with its midpoint-rounding radii 3e-5 and 1.4e-5, the exact
   0, DOMAIN, NOT_DETERMINED, exact 1 and i) matched the hand derivation. Final: 98 cases, 101660 lines equal.
7. Julia. tests/julia/tate_gamma.jl was written after the C functions existed, so no red was observed for it.
   Without LD_PRELOAD the run stops at dlopen (__gmpn_modexact_1_odd, the known bundled-GMP mismatch); with
   LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10: 36 of 36 pass.
8. Faults (run_faults.py, faults.log): 17 of 17 killed. Mutation (mutation.log): 60 mutants, 27 killed,
   29 survived, 4 not compiled; in the new functions 24 mutants, 19 killed, 3 survived (equivalent), 2 not compiled.
