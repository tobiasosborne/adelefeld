# lanes/m1-scaled: red-green log

Per COMMON-C rule 1, in order.  `src/scaled.c` did not exist when the first tests were linked,
so the first red run of each test file was a link error (the one allowed for the first test of a
file); every later red run is an assertion failure.

1. `lanes/m1-scaled/gen_scaled_vectors.py` written; it cross-checks its P10.3 and P11 rows
   against `tests/ref/adfref/` and writes `tests/ref/vectors/m1-scaled/scaled_ops.jsonl`
   (1762 rows: add_rat 245, mul_exact 223, mul_tight 283, neg 129, scale 205, set_context 285,
   sub 263, to_fball 129).  Two runs are byte-identical (md5 77bff3f766ad9994ff5aefc053e6c56e).
   No assertion of the generator failed.
2. `tests/test_scaled_vectors.c` written; `make build/test_scaled_vectors`:
   **compile error** in `build_value` (`adf_scaled_set_rat` is void); fixed.  Then
   **link error**, `undefined reference to adf_scaled_*`, as expected for the first test of the
   file.
3. `tests/test_scaled.c` written; `make build/test_scaled`:
   **compile error** (`fmpz_init_set_str` is not FLINT 3.0.1); fixed to `fmpz_init_set_si`.
   Then **link error**, `undefined reference to adf_scaled_*`, as expected for the first test of
   the file.
4. First implementation of `src/scaled.c` (all 19 functions), then
   `make -j2 build/test_scaled build/test_scaled_vectors`:
   **compile error** (`const fmpq_t q` cannot be initialised from a ternary; `fmpq_t` is an
   array type); fixed to `const fmpq * q`.  Then both binaries build and
   `./build/test_scaled_vectors` is green at once: 11 tests, 62707 checks, 0 failed.
   `./build/test_scaled`: 22 tests, 88343 checks, **53 failed checks**, 5 failed tests,
   all red by assertion.  Causes: one real bug found by the aliasing rows of
   `aliasing_unary_and_scalar_ops` and by `tight_and_scaled_policies_containment` —
   `adf_scaled_mul_rat(x, x, q)` overwrote `x->s` with `|q|` before multiplying (49 of the 53
   failures); and two wrong test expectations (`fmpq_set_si(2, 4)` canonicalises to 1/2, so a
   raw field write is needed for non-canonical data; and `scaled_from` stores `u mod K`, not a
   `u` that is `>= K`).
5. After the fix: `./build/test_scaled` 22 tests, 88343 checks, 0 failed checks;
   `./build/test_scaled_vectors` 11 tests, 62707 checks, 0 failed checks.
6. Mutation run 1 (`make mutate FILES=src/scaled.c JOBS=2 LIMIT=300`, seed 20260928):
   300 mutants in 1597.3 s: 251 killed, 31 survived, 18 not compiled, 0 timed out.
   The new test `exact_results_store_a_zero_residue` was written for the three killable
   survivors (`drop_call` of `fmpz_zero(y->u)` in the exact paths of `set_rat`, `set_fball`
   R = 0 and `mul_rat`): each mutant was applied by hand (one line commented out), and each
   red run failed by assertion (2 failed checks of that test); the green run on the restored
   original is 23 tests, 88372 checks, 0 failed.  The other 28 survivors (19 commutative
   `swap_args`, 6 sign comparisons under the `q != 0` branch, 3 redundant zero stores after
   `fmpq_init`/`fmpz_init`) are equivalent for every input and are excused with their reasons
   in `tools/mutate/equivalent.txt` (28 lines).
7. Mutation run 2 (verification, same command and seed): 300 mutants in 1581.5 s:
   254 killed, 0 survived, 18 not compiled, 0 timed out, 28 excused; `mutate: passed`.
