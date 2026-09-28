# Lane m1-repair-cap: report (Claude sonnet; summary by the orchestrator of the final message of the lane)

Findings R1 and R4 of `docs/reviews/m1/local/review.md`.

## R1: the cap functions accept local inputs

The five backend guards of `src/cap.c` are removed. The file read only `H`, `d` and the centre through
`adf_fball_get_center`, which all work for both backends; the tight operations of `fball.h` handle local,
mixed and different-context inputs. The result is global whenever the cap acts on a positive radius
(`adf_fball_set_center_radius` writes the global canonical triple); the comment of the file says why.

Red: `tests/test_cap_local.c` (6 tests, 253 checks) against the old `src/cap.c`: 122 failed checks, all
"local status 8". Green: 0 failed. The five tests of `tests/test_cap.c` that pinned the refusal are removed.
The reproducer of the reviewer, `docs/reviews/m1/local/checks/cap_local.c`: before `actual=8,8,8,8,8`, after
`actual=0,0,0,0,0`.

## R4: the fixture of `canonicalise_raw_and_domains`

Replaced by the valid raw local value `(2; 2)` in the context `(4)` made by `adf_modctx_new_blocks`; the
assertions are kept. The helper `make_local_shaped` is still used by five other tests of
`tests/test_fball.c` (left alone: the brief named one test). To be looked at in the closure check.

## Checks

`make -j2 check`, `make clean && make -j2 check SAN=1`, `make clean && make -j2 check CC=clang`: 31 programs
pass (in the worktree of the lane). Valgrind on `build/test_cap_local`: exit 0.
`make mutate FILES=src/cap.c JOBS=2 LIMIT=300`: 27 mutants, 15 killed, 3 survived, 9 not compiled. The three
survivors are swaps of the operands of `adf_fball_mul` (line 157), `fmpq_gcd` (97), `adf_fball_add` (127),
equivalent by commutativity (precision.md Propositions 1, 2).
