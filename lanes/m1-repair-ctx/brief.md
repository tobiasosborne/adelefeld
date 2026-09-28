# Lane m1-repair-ctx: findings R1, R2, R6 of reviewer `contexts`

Read `lanes/COMMON-C.md`. Then `docs/reviews/m1/contexts/review.md` completely, its reproducers
`docs/reviews/m1/contexts/checks/set_context_alias.c`, `primorial_time.c`, `primorial_time.sh`,
`ground_truth.sh`, `ctor_cases.c`, `ctor_oracle.py`; `include/adelefeld/modctx.h` (changed today: decision
M1-D5, `ADF_MODCTX_MAX_BLOCKS`, `ADF_MODCTX_MAX_PRIME`, the order of checks), `docs/SPEC.md` section 15 row
M1-D5; `include/adelefeld/scaled.h`; `src/scaled.c`, `src/modctx.c`, `src/modctx_internal.h`,
`tests/test_scaled.c`, `tests/test_modctx.c`; `refs/src/flint-3.0.1/fmpz.rst` lines 1270 to 1380 and
`ulong_extras.rst` on `n_prime_pi`, `n_nextprime`, `n_is_prime`, `n_root`, `n_pow`.

**You own:** `src/scaled.c`; `src/modctx.c` except the function `adf_modctx_new_from_dump` and its static
helpers (a running lane owns those: do not touch them, findings R3 and R4 are that lane's);
`src/modctx_internal.h` (comments only); `tests/test_scaled_alias.c`, `tests/test_modctx_limits.c` (both
new); `lanes/m1-repair-ctx/`. Existing tests are not changed.

1. **R1, red first.** `tests/test_scaled_alias.c`: for every function of `scaled.h` that has an output
   report (`lost`) or a status, the call with the output aliasing an input gives the same value, the same
   `*lost` and the same status as the call on copies without aliasing. Exhaustive for `set_context`: moduli
   `K`, `K'` in 1 to 24, every residue, scales `1`, `1/2`, `3`; and 10000 random cases with moduli up to
   4096 bits. The same for `set_fball` is not possible (other types); `add_rat` with `y = x`. With the code
   as it stands `set_context` with `y = x` gives a wrong `*lost` in 4865 of 7200 cases. Then the code:
   compute `*lost` from the input before the output is written, or work in temporaries; look at every other
   function of the file for the same pattern (a read of an input field after a write of the output) and
   say for each function what you found.
2. **R2, red first.** `tests/test_modctx_limits.c`: `adf_modctx_new_primorial_pow` with `n = 2^64 - 1`,
   `2^40`, `2^33`, `10^8` and `e = 1, 2, 3, 63, 64`; `adf_modctx_new_factorial` with `n` up to `2^64 - 1`;
   `adf_modctx_new_blocks` and `adf_modctx_new_prime_powers` with `k = 65536`, `65537`, `2^40`, `WORD_MAX`
   (with a short array: the function must not read it before it has refused `k`; place the array at the
   end of a page followed by a `PROT_NONE` page); each returns the status of the header within one second
   and with `*out` untouched; the largest admitted cases (`k = 65536` blocks of distinct primes; the
   primorial of `n = 821646` with `e = 1`) succeed. Run the test program under `ulimit -v 4000000` from a
   script in your lane directory. The expected status of each case is worked out in the test from the rule
   of the header, with the reason in a comment. Then the code. After a refused call no memory that depends
   on `n` stays allocated: measure the resident size before and after the call for `n = 10^8`, `e = 3`
   (the reviewer measured 131 MB left by FLINT's prime table) and report it.
3. **R6.** The comments the review names in `src/modctx.c` and `src/modctx_internal.h`: the source is on
   disk (`fmpz.rst`), cite the lines of the functions that are called; the text on `fmpz_comb` is wrong.
   On the promise that the recombined value lies in `[0, K)`: the documentation says "an integer of
   smallest absolute value" for one sign convention and does not define the argument; read the FLINT
   source if it is on disk, otherwise do not rely on it: reduce the result of `fmpz_multi_CRT_precomp`
   modulo `K` into `[0, K)` in `adf_modctx_recombine` (one `fmpz_fdiv_r`), say so in the comment, and add
   a test that pins `[0, K)` for residues whose balanced lift is negative.
4. `make -j2 check`, `make clean && make -j2 check SAN=1`, `make clean && make -j2 check CC=clang`; the
   reviewer's reproducers `set_context_alias.c` and `primorial_time.sh` against the repaired library, with
   their new output; valgrind on the two new test programs.
5. Mutation: `make mutate FILES=src/scaled.c JOBS=2 LIMIT=300` and the same for `src/modctx.c`; survivors
   killed by tests or listed in the report with the reason (do not edit `tools/mutate/equivalent.txt`).
