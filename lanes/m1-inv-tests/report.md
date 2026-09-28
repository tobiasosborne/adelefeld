# Report of lane m1-inv-tests (adf-6vy)

Written by the lane (Claude Sonnet 5.5, effort medium) as its final message; saved to this file by the
orchestrator, who added the last section. Base commit 66bfabf. Logs are in `lanes/m1-inv-tests/`.

## Result

The whole suite passes under `INV=1`, and the five programs give the same summary lines as before without the
flag. No file under `src/` or `include/` was changed.

## What was done, per program

1. **`tests/test_dump_golden.c`**
   - The sentinel `0x40` in the scaled value is now a live context (`adf_modctx_new_fmpz`, K = 6). The value
     is made with `adf_scaled_init` and cleared with `adf_scaled_clear`. The context is freed after the value.
   - "Untouched" now also requires `x->mctx == sent`, inside the same `ADF_CHECK` as before.
   - No check was added or removed.
2. **`tests/test_dump_ctx.c`**
   - `local_set` no longer writes `mctx` by hand. It builds the global ball (A0, K, d), with A0 the CRT lift
     of the residues, then calls `adf_fball_canonicalise` and `adf_fball_set_local`.
   - `sc_init` and `sc_clear` are now `adf_scaled_init` and `adf_scaled_clear`.
   - `sc_random` lost its hand write `x->mctx = ctx`, because every caller already ran `sc_init` with the
     same context.
   - The sentinels 0x10 (`scaled_status`) and 0x30 (`typed_check`) are live contexts from a new helper
     `sentinel_ctx()` (one block, 6). It makes no check, so the check count does not move. "Untouched" is
     compared with that context.
   - The sentinel 0x20 is untouched. It is an `adf_modctx_struct **` slot, not a pointer field of a value.
3. **`tests/test_fball_local.c`**
   - `mklocal_raw` (and so every `mklocal_si`) now builds the global ball from the CRT lift with
     `adf_fball_set_fmpz3`, then calls `adf_fball_set_local`. It aborts on a non-OK status, with no check.
   - This also repairs line 1650 (`local_output_of_another_context_is_resized`), which overwrote the context
     of a live counted value in a loop. `set_local` now retargets the count.
   - Line 415: `y`, a local value of c3, is used again after the free of c3. The free only cleans up, so
     `adf_fball_set_global(y, y);` was inserted before `adf_modctx_free(c3)`. `y` then leaves c3, and the
     contract holds with and without the flag. No check changed.
   - `is_canonical_rejects_a_context_without_blocks` builds a value that is not canonical and that no library
     function can make: `set_local` returns UNSUPPORTED for a context without blocks (`modctx.h:151`). It is
     kept by hand from a fresh `init`. A comment cites `fball.h:99-104` and M1-D2/M1-D10. It passes under the
     flag.
4. **`tests/test_fball.c`**
   - `make_local_shaped` now goes through `set_fmpz3` and `set_local`. It stops the program if the caller's
     expected A or H is not (0, K).
   - `canonicalise_raw_and_domains`: the hand-built local fixture is now built with `adf_fball_canonicalise`
     and `adf_fball_set_local`. This is review finding R4 of `local`. It is not a `mkball_si` call, because
     that would add a check.
   - `identical_local_guard` (M1-D11):
     - `y->mctx = x->mctx` became `make_local_shaped(y, ..., c1)`.
     - The non-canonical part (`y->backend = ADF_GLOBAL` with a context and a residue array) is
       `#ifndef ADF_CHECK_INVARIANTS` without the flag, with a comment citing M1-D11 and `fball.h:100-104`.
     - With the flag, a forked child calls `adf_fball_identical(x, y)` and the test requires SIGABRT and
       exactly the line `adelefeld: ADF_CHECK_INVARIANTS: adf_fball_identical: argument y is not a canonical
       adf_fball` (`src/invariants.h:53-57`).
   - The file gained `_POSIX_C_SOURCE` and the POSIX includes, all under `#ifdef ADF_CHECK_INVARIANTS`.
5. **`tests/test_recon.c`**
   - `an_adele_with_an_infinite_real_ball_is_rejected` (M1-D11): without the flag it is unchanged
     (`run_adele(x, ADF_DOMAIN, ...)`). With the flag, a forked child calls `adf_adele_reconstruct` and the
     test requires the abort line `... adf_adele_reconstruct: argument x is not a canonical adf_adele`.
   - The fork helper is written in the file itself, after `tests/test_invariants.c`.
   - The helper is the same for `test_fball.c` and `test_recon.c`; there is no shared header because the lane
     does not own one.
6. **`tests/README.md`**: one paragraph added at the end of the section "The debug build `INV=1`".

## Files changed

`tests/test_dump_ctx.c`, `tests/test_dump_golden.c`, `tests/test_fball_local.c`, `tests/test_fball.c`,
`tests/test_recon.c`, `tests/README.md`, and the logs in `lanes/m1-inv-tests/`.

## Checks run

Each was preceded by `make clean`, one build at a time, with 23 GB available before each.

| command | last line | log |
|---|---|---|
| `make -j2 check` | check passed: all 42 test programs | `after-release.log` |
| `make -j2 check SAN=1` | check passed: all 42 test programs | `after-san.log` |
| `make -j2 check CC=clang` | check passed: all 42 test programs | `after-clang.log` |
| `make -j2 check INV=1` | check passed: all 42 test programs | `after-inv.log` |
| `make -j2 check INV=1 SAN=1` | check passed: all 42 test programs | `after-inv-san.log` |

The state before is in `before-release.log` (passed, 42 programs) and `before-inv.log`
(`make -j2 -k check INV=1`: FAILED, as the report of m1-invariants says).

## Summary lines of the five programs

Format: tests / checks. Failed checks and failed tests are 0 in every "after" row.

| program | before, no flag | after, no flag (also SAN, clang) | before, INV=1 | after, INV=1 (also INV+SAN) |
|---|---|---|---|---|
| test_dump_ctx | 14 / 28587 | 14 / 28587 | segfault, no summary | 14 / 28587 |
| test_dump_golden | 1 / 761 | 1 / 761 | segfault, no summary | 1 / 761 |
| test_fball_local | 25 / 20554 | 25 / 20554 | abort, no summary | 25 / 20554 |
| test_fball | 20 / 412 | 20 / 412 | abort, no summary | 20 / 412 |
| test_recon | 19 / 52740 | 19 / 52740 | abort, no summary | 19 / 52737 |

Without the flag all numbers are equal before and after, and the same in the SAN and clang runs. Under the
flag, `test_recon` has 3 checks fewer than without it. This is the M1-D11 test: the release branch is
`run_adele` with 4 checks (2 of its own and 2 in `set_rat`), and the flag branch is 1 check (the abort).
`test_fball` counts equal in both modes: the replaced check is 1 for 1.

## Not done

- The mutation tool was not run (forbidden). No fuzzing (nothing parses new input). `make bench` was not run.
- No scan build was run to look for silent clamped releases in the five programs. What is known is that the
  programs pass under the flag.
- Hand-built values that remain, by design:
  - the value without blocks in `test_fball_local.c`;
  - field mutations of canonical values in the predicate tests (`x->res[0] = 4`, `x->H`, `x->d`,
    `x->backend = 2`, and `x->mctx = NULL` then restored in `test_fball.c` `local_shaped_is_canonical`). These
    use only the exempt predicates (`fball.h:99`). The case of NULL and restore leaves the count unchanged.
  - the raw global triples in `canonicalise_raw_and_domains`, and `x->mctx = &x` in a check of the predicate G;
  - the hand write `y->backend = ADF_GLOBAL` in `identical_local_guard`, which is the M1-D11 case above.

## Findings against the specification and headers

- The local builders now depend on `adf_fball_set_local`, so the tests that compare `set_local` with an
  independent global form (`test_fball_local.c`, `global_independent` and `lift_independent`) are less
  independent than before. The rule of the brief (build through the public interface) causes this. The
  comparison against `global_independent` and the vectors in `test_fball_local_vectors.c` still exist.
- `identical` (`fball.h:100-104`) and `adf_adele_reconstruct` (recon.h) do not say that a non-canonical input
  aborts under the flag. M1-D11 covers this. The header comments could cite M1-D11, which is a matter for the
  orchestrator.
- The comment at `test_recon.c` about "arb_get_interval_fmpz_2exp aborts" is kept, and now says the answer is
  a courtesy of the release build.

## Checks of the orchestrator before the merge (2026-09-29, 00:25)

- The lane changed the six files it owns and nothing else (`git status`).
- The diff removes three `ADF_CHECK` lines and adds them again with one condition more each
  (`x->mctx == sent`): no check was weakened. 21 further lines with `ADF_CHECK` are added.
- In the worktree, each after `make clean`: `make -j2 check`, `make -j2 check INV=1`,
  `make -j2 check INV=1 SAN=1`: 42 test programs pass in each.
- Not repeated in the worktree: `SAN=1` and `CC=clang` without the flag (run on master after the merge).
- Open: the first finding above (tests of `set_local` built with `set_local`) is a loss of independence of
  the test oracle; it goes to the closure check of `local`.
