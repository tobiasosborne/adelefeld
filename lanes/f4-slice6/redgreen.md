# Red-green log of lane f4-slice6 (slice 4g)

A. Vectors: `timeout 900 python3 -B lanes/f4-slice6/gen_vectors.py` writes 51 records (50 tensors: (D, M) in
   (1,1), (2,3), (3,2), (4,1), (6,6) times 7 functions, two value families at (2,3) and (6,6), delta_1 at (2,3);
   one theta record), 182 KB. First run with reference tails 2^-230 at 300 bits; rerun at 2^-460 and 600 bits
   (see B: the reference's imaginary part carried the oracle's own tail 6.12e-111 as its radius, wider than the
   code's certified 6.117e-111 at bits 128, so containment of the reference ball failed although both enclose).

B, C. Red then green:
1. RED A: `tests/test_poisson.c` and the declaration in `include/adelefeld/tensor.h` written in full;
   `make -j2 BUILD=lanes/f4-slice6/build lanes/f4-slice6/build/test_poisson` is a link failure, 6 lines of
   undefined references to `adf_tensor_poisson` (`red-a.log`).
2. Code: `src/poisson.c`, the whole function in one step; the per-check red evidence is the fault table of F.
3. First green attempts, corrections (none in the code):
   - test:252 (`acb_contains(l, rl)`, record 0, bits 128): the reference ball, not the code (vectors regenerated
     at 2^-460 tails, A).
   - test: my witness width test assumed N = 2 at bits 13 with a value of E typed from memory; the oracle's
     E(1) = 6.97e-6 < 2^-16 gives N = 1. TEST changed: E read from the vector, N = 1, S_1.
   - test: theta at bits 0 expected N >= 1; the oracle's E(0) = 0.0864 <= 1/8 gives N = 0 (1 +/- 0.0864 contains
     theta). TEST changed to N = 0 and the radius window.
   GREEN: `poisson: 47703 checks` (plain): 200 vector calls, 1651 planted tail comparisons, 351 radius windows,
   every NL and NR equal to the oracle's choose_cutoffs.

After green, tests added (each first seen to fail against a planted fault or a mutation survivor, then green):
- theta at bits 100 from prec 20 (the retries must pass 64 bits): planted fault 12 survived the earlier tests.
- `f = [1, -1]` at (1, 2), `g[0] = 0`: planted fault 16 (the right tail with `|g[0]|`) survived.
- processor time of the witness < 1 s: planted fault 22 (the stall rule weakened) survived (it doubles to 2^21).
- mutation run 1 survivors: a zero polynomial term after a nonzero one (200, 201); 2^16 terms at the coefficient
  boundary (99); the preflight of the prefix within 0.25 s (131, 135); the terms cap before the bits check (95;
  planted as fault 21, caught).

D. Driver: `tests/driver/poisson.cmd` with 11 expected lines derived by hand (P1, Lemma 6, conventions 9.5);
   the first run of the fixture agreed on all 11 lines. Julia `tests/julia/poisson.jl`: 13 of 13 (with the
   libgmp preload fallback of tests/test_julia.sh, as tensor.jl).
