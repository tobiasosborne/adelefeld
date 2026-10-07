# Red-green log of lane f4-slice5 (slice 4f)

A. Vectors: `timeout 300 python3 -B lanes/f4-slice5/gen_vectors.py` writes eval (1603), local (56), sball (140),
   tensor (25), integral (14) records, 184 KB. The generator asserts the oracle's `evaluate` against rational
   samples (as functions4_checks.py:430-435) and against the canonical triple, and `evaluate_partial` against its
   own transcription of E1 step 4.

B, C. Red then green:
1. RED A: `tests/test_tensor.c` and `include/adelefeld/tensor.h` written in full; `make BUILD=lanes/f4-slice5/build
   lanes/f4-slice5/build/test_tensor` is a link failure, 20 undefined references to the 7 functions
   (`red-a.log`).
2. Code: `src/tensor.c`, all seven functions in one step; the per-function red evidence is the fault table of F
   (`faults.log`, `faults-8.log`: every planted fault makes the test fail).
3. First green attempts, corrections:
   - test (exact_cases:259): the hand hull was built by chained `acb_union`, and the code's by chained
     `acb_union` too, but `acb_equal` compares representations. Investigation: in the installed FLINT 3.0.1 the
     union of `2 +/- 1` and 5 is `(3 - 2^-30) +/- (2 + 2^-27)`, and `arb_set_interval_arf(1, 5)` gives
     `3 +/- (2 + 2^-28.4)`. CODE changed: the hull is formed over all balls at once with an exact radius where it
     fits a mag (`tn_box`, `tn_set_interval`); a single ball is copied exactly. TEST changed: its own hull from
     endpoints (independent of the code's); tightness bound restated as `2^-120 (1 + |h|) + 2^-26 rad(h)`,
     since every arb operation (a product with the exact 1 included) rounds a radius up by a mag ulp.
   - test (`all_seven`): the untouched check applied to the OK case; restricted to failures.
   - test: the sball hull contains the adele hull only up to rounding (both are rounded products); checked
     against the widened sball result; the "places agree" case is counted (46 of 49 per family).
   - test: `exact_ball` for `[1, 5]` at prec 2 is false (5 rounds up to 6 at 2 bits); replaced by containment.
   GREEN: `tensor: 149838 checks, 49644 exclusions` (plain).

D. Driver: `tests/driver/tensor-eval.cmd` with 21 expected lines derived by hand (one line, the NONE box
   `(0 +/- 1.1) + (0 +/- 1.1)*i`, was also seen in an exploratory run before the fixture); the first run of the
   fixture agreed on all 21 lines. Julia `tests/julia/tensor.jl`: 10 of 10 (with the LD_PRELOAD of libgmp that
   tests/test_julia.sh applies).

F. Faults and mutation:
- `plant_faults.py`: 18 faults (faults_44 a-f, the brief's list, 6 more); 17 caught on the first run, the 18th
  (support test dropped) did not build (`;` after `if`, -Werror) and was caught after a compilable replacement
  (`faults-8.log`).
- Mutation run 1 (`mutate-run1.log`, seed 20261008, 60 mutants, INV=1 SAN=1, test_tensor only): 50 killed,
  7 survived, 3 not compiled. Three survivors were test gaps:
  - the integral loop to `j <= L`;
  - `arf_clear(b->hi - k)`;
  - tensor_eval without the ffun entry check.
  Tests were added for them (`guard_cases`, INV cases 9-13). Each was then confirmed killed as a planted fault
  (`faults-survivors.log`, 19, 20, 22). Fault 21 (no rfun entry check in tensor_eval) is not caught, since
  adf_rfun_eval checks phi itself.
- Mutation run 2 (`mutate-run2.log`, same seed): 53 killed, 4 survived (equivalent or unreachable), 3 not
  compiled.
- Final: plain, SAN=1 (detect_leaks=1), INV=1, CC=clang all pass (`variants.sh`); `sh tests/test_driver.sh`:
  88 cases, 101543 lines, all equal.
