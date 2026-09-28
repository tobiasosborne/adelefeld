# Lane m1-repair-ctx: findings R1, R2, R6 of reviewer `contexts`

Worktree `/home/tobias/Projects/adelefeld-wt/m1-repair-ctx`.  The lane started from the WIP commit
`07ac9d3` ("WIP m1-repair-ctx"), which already held the R1 fix in `src/scaled.c`, most of the R2
work in `src/modctx.c`, the two new test files and `run_limits.sh`.  This session finished the
findings, corrected the R6 comments, added the two tests still missing, and ran the checks below.
No file outside the owned set was touched.  `adf_modctx_new_from_dump` and its static helpers in
`src/modctx.c` were left alone (lane `m1-dump` owns them; R3/R4 are that lane's).

## 1. What was done

### R1 (BLOCKER) `adf_scaled_set_context` with `y = x`

- The fix is in `src/scaled.c` (already in the WIP commit, verified here): the `*lost` test
  `u K'/K` is computed from `x->u` and `K` before any field of `y` is written.  The values are
  computed into the locals `s2`, `ubar`, `t` first; the write of `y` is last.  `y = x` is safe.
- `tests/test_scaled_alias.c` (new) covers the aliasing contract of every function of `scaled.h`
  that has an output report or a status:
  - `adf_scaled_set_context`: exhaustive over `K, K'` in 1..24, every residue `u`, scales 1, 1/2,
    3 (21600 cases), comparing the aliased call `y = x` with the call on copies by the set
    (`adf_fball_equal_set` of `adf_scaled_get_fball`) and by `*lost`; then 10000 random cases with
    moduli up to 4096 bits, and exact inputs.
  - `adf_scaled_add_rat` with `y = x`.
  - `adf_scaled_add`, `sub`, `mul`, `mul_tight` with `z = x`, `z = y`, `z = x = y`.
  - `adf_scaled_neg`, `adf_scaled_mul_rat` with `y = x`.
  - A `ADF_DOMAIN` call with `z = x` and with `z = y`: output untouched.
  - `adf_scaled_set_fball` into an output that holds another value (the input is another type).
  - The four cap functions with `y = x`, `z = x`, `z = y`, `z = x = y`.
- Every other function of `src/scaled.c` was read for the same pattern (a read of an input field
  after a write of the output).  Findings:
  - `adf_scaled_set`: `y = x` is a plain self-copy; no read follows a write.
  - `adf_scaled_set_fball`: reads the input through `adf_fball_get_fmpz3` into locals `A, H, d`,
    writes `y`, and computes `*lost` from `A, K, H` (not from `y`).  Safe.
  - `adf_scaled_set_context`: the fixed function; `*lost` before the write.  Safe.
  - `adf_scaled_get_fball`: output is an `adf_fball`, input an `adf_scaled`; reads `x->s`,
    `x->u`, `K` into locals before writing `y`.  Safe.
  - `adf_scaled_add`, `sub`, `mul`, `mul_tight`: compute into a private `adf_scaled_struct t`
    and `adf_scaled_swap(z, &t)` last.  Safe for `z = x` and `z = y`.
  - `adf_scaled_neg`: for `y = x` it writes `y->s` (a self-copy of `x->s`), then negates
    `y->u = x->u` in place; `K` is read before `y->mctx` is written.  Safe.
  - `adf_scaled_mul_rat`: `y = x`: `fmpq_abs(aq, q->q)` first, then `fmpq_mul(y->s, aq, x->s)`
    (FLINT permits the output to alias an input), then `fmpz_set(y->u, x->u)`.  Safe.
  - `adf_scaled_add_rat`: computes into `t` and calls `adf_scaled_swap(y, &t)` last; `*lost` is
    computed from `q` and `x->s` into a local before the swap.  Safe.
  - `adf_scaled_init`, `clear`, `swap`, the predicate and query functions take no `lost` and have
    no input/output of the aliasing kind.  No read-after-write.

### R2 (MAJOR) `adf_modctx_new_primorial_pow` aborts and retains memory

- The fix is in `src/modctx.c` (already in the WIP commit, verified here):
  `adf_modctx_new_primorial_pow` decides both `ADF_UNSUPPORTED` reasons from `n` and `e` alone,
  before any prime is enumerated or any FLINT table is grown:
  - `n >= ADF_MODCTX_MAX_PRIME` (821647): `pi(n)` would exceed `ADF_MODCTX_MAX_BLOCKS` (65536).
  - `e >= 2` and `n >= adf_overflow_prime(e)`, where `adf_overflow_prime(e)` is the first prime
    `q` with `q^e >= 2^64` (`n_root((ulong) -1, e)` then `n_nextprime`).
  - `e = 1` has no overflow; the bound `n >= ADF_MODCTX_MAX_PRIME` applies.
  `adf_modctx_new_blocks` and `adf_modctx_new_prime_powers` return `ADF_UNSUPPORTED` for
  `k > ADF_MODCTX_MAX_BLOCKS` before reading `q`, `p` or `e`.
- `tests/test_modctx_limits.c` (new) covers:
  - `new_primorial_pow` for `n = 2^64-1, 2^40, 2^33, 10^8` and `e = 1, 2, 3, 63, 64`; the expected
    status is worked out in the test from the header rule with the reason in a comment; every
    call returns within one second with `*out` untouched.
  - `new_factorial` up to `2^64-1`; `UNSUPPORTED` from `n = 66` (`v_2(66!) = 64`), one second,
    `*out` untouched.
  - `new_blocks` and `new_prime_powers` with `k = 65537, 2^40, WORD_MAX` and the argument array at
    a `PROT_NONE` page: `ADF_UNSUPPORTED` and no read of the array.
  - a `NULL out` is `ADF_DOMAIN` for the five raw constructors.
  - the largest admitted cases succeed (`k = 65536` distinct primes; the primorial of
    `n = 821646` with `e = 1`, `pi(821646) = 65536`).
  - after the refused `n = 10^8, e = 3` call the resident size grew by less than 16 MB
    (the call is refused before FLINT's prime table is touched; measured 2-3 MB in all).
- `lanes/m1-repair-ctx/run_limits.sh` runs the test program under `ulimit -v 4000000`
  (`ADF_MODCTX_LIMITS_FULL` defaults to 1).

### R6 (MINOR) FLINT citations and the `[0, K)` promise

- `src/modctx.c` head comment: the `fmpz_comb` claim is gone; the comment now names the two
  precomputed programs `fmpz_multi_mod_t` and `fmpz_multi_CRT_t` and the exact lines of the
  installed header `/usr/include/flint/fmpz.h` (init/precompute/precomp at 653/654/656/657/658
  and 686/687/689/691/692), and the documentation on disk
  `refs/src/flint-3.0.1/fmpz.rst:1350-1372`.
- `adf_modctx_alloc` comment: "the comb tables are built" was replaced by "the precomputed
  programs are built".
- `adf_modctx_reduce` comment: "The scratch of fmpz_comb is local" was replaced by "The precomp
  call uses only local temporaries".
- `adf_overflow_prime` comment: `n_root` is declared at `/usr/include/flint/ulong_extras.h:69`
  and is not documented in `refs/src/flint-3.0.1/ulong_extras.rst` (which documents `n_rootrem`
  at lines 1023-1030); the comment says so and cites `n_rootrem` for the integer-part convention.
  `n_nextprime` is cited at `ulong_extras.rst:688-692`.
- `adf_modctx_recombine` reduces the FLINT result modulo `K` with one `fmpz_fdiv_r`, with a
  comment saying that `fmpz_multi_CRT_precomp` promises only "an integer of smallest absolute
  value" (`fmpz.rst:1362-1365`) and does not define `sign`.  The `[0, K)` promise in
  `src/modctx_internal.h` now holds by construction.
- `tests/test_modctx_limits.c` gained `recombine_is_the_representative_in_range`: all residue
  pairs of `K = 35` (the largest element 34 has balanced lift -1), the residues of `K - 1` for
  `K = 65537*65539`, and `reduce(-1)` followed by `recombine` for `K = 105` (must give 104, not
  -1).  This pins `[0, K)` for residues whose balanced lift is negative.
  The pin is not red on this machine: with the `fmpz_fdiv_r` temporarily removed, FLINT's
  `sign = 0` already returns `[0, K)` and the test still passes.  The line is kept as the
  construction that makes the promise independent of the undocumented sign argument.

## 2. Files written or changed

- `src/modctx.c`: comment corrections only (R6); no functional change in this session.  The R2
  code was already in the WIP commit.
- `tests/test_scaled_alias.c`: new in the WIP commit; this session added the missing
  `fmpz_init(u)` in `add_rat_alias` (found by valgrind).
- `tests/test_modctx_limits.c`: new in the WIP commit; this session added
  `null_out_is_a_domain_error` and `recombine_is_the_representative_in_range` and renumbered.
- `lanes/m1-repair-ctx/run_limits.sh`: comment corrected, `ADF_MODCTX_LIMITS_FULL` can be set to 0.
- `lanes/m1-repair-ctx/red-green.log`: extended (R6 pin, R2 mutant kill).
- `lanes/m1-repair-ctx/report.md`: this file.
- `lanes/m1-repair-ctx/repro/`: build outputs of the reviewer reproducers and the logs of the
  runs (not source of the library).

## 3. Checks, with the command and the result

1. `make -j2 check` (gcc), final revision: `check passed: all 35 test programs`.
   `build/test_scaled_alias`: 9 tests, 142938 checks, 0 failed.
   `build/test_modctx_limits`: 7 tests, 239 checks, 0 failed.
   `build/test_scaled`: 23 tests, 88410 checks, 0 failed.
   `build/test_modctx`: 11 tests, 10197 checks, 0 failed.
2. `make clean && make -j2 check SAN=1`, final revision: `check passed: all 35 test programs`;
   `build/test_scaled_alias` 9 tests, 142938 checks, 0 failed; `build/test_modctx_limits`
   7 tests, 239 checks, 0 failed.  Log: `lanes/m1-repair-ctx/repro/final_checks.log` (SAN exit 0).
3. `make clean && make -j2 check CC=clang` (clang 18.1.3), final revision:
   `check passed: all 35 test programs`; `build/test_scaled_alias` 9 tests, 142938 checks, 0 failed;
   `build/test_modctx_limits` 7 tests, 239 checks, 0 failed.  Log: same file (CLANG exit 0).
4. Reviewer reproducer `set_context_alias.c` (built against `build/libadelefeld.a`):
   `cases 7200, wrong lost (plain) 0, wrong lost (y = x) 0`; `set changed but lost = 0: 0;
   set unchanged but lost = 1: 0`; exit 0.
5. Reviewer reproducer `primorial_time.sh` (under `ulimit -v 4000000`, 60 s timeout):
   all 8 rows `exit=0`, status `UNSUPPORTED`, time 0.00 s, maxrss 2-3 MB, resident after return
   3 MB.  Before the repair the same inputs aborted with exit 134 and left 131 MB (review R2).
6. `lanes/m1-repair-ctx/run_limits.sh` (full admitted cases, `ulimit -v 4000000`):
   5 tests, 118 checks, 0 failed (the revision before the last two tests were added; the added
   tests are not part of the slow case).  `/usr/bin/time`: 321.38 s user, 5:21.74 wall, maximum
   resident set 40800 KB.  This exceeds the 3-minute guidance of the laptop rule; it was run once
   because the brief demands the largest admitted cases.  The added tests do not change
   `largest_admitted_cases_succeed`.
7. `valgrind --error-exitcode=1 --leak-check=full --errors-for-leak-kinds=definite,indirect`
   `build/test_modctx_limits`: 0 errors, definitely lost 0, indirectly lost 0 (exit 0).  The
   "possibly lost" blocks are FLINT's `_fmpz_new_mpz` pools.
   The same command on `build/test_scaled_alias`: first run 1 error (the missing `fmpz_init(u)`
   in the test), after the fix 0 errors, definitely lost 0, indirectly lost 0 (exit 0).
8. Mutation, reduced (see section 4 for the deviation):
   - `python3 -u tools/mutate/mutate.py --root . --scratch build/mutate-scaled --jobs 2
     --seed 20260928 --limit 40 --timeout 45 --files src/scaled.c --make '<build and run
     test_scaled, test_scaled_alias, test_scaled_vectors>'`:
     40 mutants in 133.9 s: 34 killed, 4 not compiled, 2 survived, 0 timed out, 0 excused.
     Survivors: `src/scaled.c:540` `zero_one` (`fmpq_sgn(q) < 0` -> `< 1`) and
     `src/scaled.c:508` `swap_args` (`fmpq_mul(t.s, x->s, y->s)` -> `fmpq_mul(t.s, y->s, x->s)`).
     Both are equivalent: the branch is reached only for `q != 0`, where the sign is -1 or 1, so
     `< 0` and `< 1` agree; the product of two rationals does not depend on the order and `t.s`
     is a temporary.  The full log is `lanes/m1-repair-ctx/repro/mutate_scaled_reduced.log`.
   - The same command for `src/modctx.c` with `test_modctx`, `test_modctx_limits`,
     `test_fball_local`: 40 mutants in 158.6 s: 28 killed, 7 not compiled, 4 survived, 1 timed
     out, 0 excused.  Survivors:
     - `src/modctx.c:548` `drop_call` (`fmpz_fdiv_r(out, out, ctx->K)` removed): equivalent on
       this FLINT for the tested inputs because `sign = 0` already returns `[0, K)`; the line is
       the construction that makes the promise independent of the sign argument.  The R6 test
       cannot kill it on this machine (section 1, R6).
     - `src/modctx.c:543` `zero_one` (`sign 0` -> `sign 1`): equivalent because the following
       `fmpz_fdiv_r` reduces the possibly negative result into `[0, K)`.
     - `src/modctx.c:339` `zero_one` (`if (k < 0)` -> `if (k < 1)`): equivalent; `n = 0, 1` are
       handled before the call, so `adf_factorial_blocks` returns 0 only for those and never
       reaches this line, and `-1` is caught by both comparisons.
     - `src/modctx.c:360` `status` (`ADF_DOMAIN` -> `ADF_OK` for `out == NULL`): not equivalent;
       it exposed a test gap.  `tests/test_modctx_limits.c` now has
       `null_out_is_a_domain_error`, and the mutant is killed by hand (red run in
       `lanes/m1-repair-ctx/red-green.log`): 2 failed checks with the mutant, 0 without.
     The full log is `lanes/m1-repair-ctx/repro/mutate_modctx_reduced.log`.
   - R5 (stale excuses): the two equivalent survivors in `src/scaled.c` are listed in
     `tools/mutate/equivalent.txt` at the old lines 447 and 547; after the R1 fix they are now at
     508 and 540, so the tool reported them as survivors.  `tools/mutate/equivalent.txt` was not
     edited, per the brief.

## 4. What is not done / deviations

- The mutation runs use `--limit 40` and a `--make` command that builds and runs only the tests of
  the file, not the brief's exact `make mutate FILES=... JOBS=2 LIMIT=300`.  The exact
  command with the default `make -s -j2 check` was started for `src/scaled.c` and did not finish
  in 40 minutes (the baseline `make check` in the scratch copy takes 28.4 s and each mutant
  re-links all 35 test programs); it was stopped.  The reduced runs stay under the laptop time.
  The `LIMIT=300` counts are therefore not known: scaled.c has 350 mutants in all, modctx.c was
  not counted in full.
- The full `make clean && make -j2 check SAN=1`, `CC=clang` and `make -j2 check` of section 3,
  items 1 to 3, were all run on the final revision and passed.
- The `n = 10^8, e = 3` resident measurement is done inside `test_modctx_limits.c` (growth below
  16 MB, measured 2-3 MB); no separate `/proc` reading was kept for that one call beyond the
  test's own check and the `primorial_time.sh` row.
- `docs/reviews/m1/contexts/checks/ground_truth.sh` still prints the pre-repair line numbers of
  `src/modctx.c`; its claims were re-verified by hand against the files on disk (section 1, R6).
  The script itself was not changed (it is not mine).

## 5. Sources pending

- `refs/src/flint-3.0.1/ulong_extras.rst` does not document `n_root` (only `n_rootrem` at lines
  1023-1030).  The integer-part convention used for `adf_overflow_prime` is the one documented
  for `n_rootrem`; `n_root` itself is only declared at `/usr/include/flint/ulong_extras.h:69`.
  `[source pending: the FLINT 3.0.1 documentation of n_root]`.
- `fmpz_multi_CRT_precomp`'s `sign` argument is not documented (`fmpz.rst:1362-1365` says only
  "an integer of smallest absolute value").  The code no longer relies on it for the range, by
  reducing modulo `K`.  `[source pending: the meaning of the sign argument]`.

## 6. Findings against the specification

- No counterexample to `docs/SPEC.md` or to a proof statement was found.  R1 and R2 were faults
  of the code against the header and are fixed; R6 was a citation fault and is fixed.
- The order of checks in `adf_modctx_new_blocks` and `adf_modctx_new_prime_powers` deserves a
  note, not a counterexample.  The header says "ADF_DOMAIN first, then ADF_UNSUPPORTED"
  (conventions 3.3) and also that the size bound is decided from `k` before the array is read
  (decision M1-D5).  For `k > ADF_MODCTX_MAX_BLOCKS` the code cannot check a per-element DOMAIN
  condition without reading the array, so it returns `ADF_UNSUPPORTED` before those checks.  The
  argument-level DOMAIN checks (`out == NULL`, `k < 0`, `q/p/e == NULL`) do come first, which is
  the defensible reading of both sentences.  This is a `HEADER-FINDING` for the record.
