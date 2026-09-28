# Lane m1-local: report (Claude opus; summary by the orchestrator of the final message of the lane)

Work package 1.8, second part: the local backend of finite balls. Base `fee4734`.

## Decision of the orchestrator at the merge

Four tests of `tests/test_fball.c` (lane m1-fball) pinned the placeholder behaviour for local inputs with
values whose context pointer was fake (`&dummy`, `0x1`): `local_shaped_is_canonical`,
`identical_local_guard`, `predicates_local_and_radius_edges`, `arithmetic_local_input_untouched`. Such values
do not satisfy predicate L; a correct implementation reads the block count through the pointer. The lane did
not edit the test and proposed a replacement (`test_fball.patch`, `test_fball.proposed.c`): the same four
tests with real contexts of blocks (2, 3) and (4, 3), asserting what `fball.h` documents; the fourth is
renamed `arithmetic_local_input_as_set`. Every test of the global backend is unchanged. The orchestrator
read the patch and applied it. With it, in the worktree of the lane: `make clean && make -j2 check SAN=1`
and `make -j2 check`: 25 programs pass (run by the orchestrator).

## What was done

1. Reference `tests/ref/adfref/local_ref.py` from policies Definition 16 to Summary 26; self-checks
   `lanes/m1-local/check_local_ref.py` (13560 local values, 1800 balls against brute force, 4200 pairs, the 5
   examples of the proofs; 3 of 3 mutants of the reference killed). Vectors
   `tests/ref/vectors/m1-local/local.jsonl`, 1923 records; contexts of 1, 2, 3, 64, 128 blocks.
2. `src/fball_local.c`: `adf_fball_set_local`, `_set_local_enclose`, `_set_global`, `_is_local`, `_context`.
   `UNSUPPORTED` (no block) is checked before `DOMAIN` (conventions 3.3).
3. `src/fball.c`: life cycle with the residue array; `is_canonical` with predicate L; accessors through the
   canonical triple; neg, add, sub blockwise (P21); mul blockwise when h = 1, otherwise the tight global
   product and then `set_local`, which succeeds exactly when h divides d e (P19, P22.3); `mul_rat` local
   when |m| divides d (P23); predicates and `compare` on the global forms; different context pointers, or
   one local and one global input, give a global result. No modular inverse is computed (P25).

## Checks by the lane

- `build/test_fball_local`: 25 tests, 20554 checks, 0 failed. `build/test_fball_local_vectors`: 42682
  checks, 0 failed, all 1923 records, each binary operation also with the output aliasing the first input.
- valgrind on both: exit 0; no definite or indirect leak, no invalid access.
- Leak tests with counting memory functions, 1000 rounds: no growth.
- Mutation (seed 20260928, limit 300, with the proposed `test_fball.c`):

| File | Mutants | Killed | Survived | Not compiled | Timed out | Excused |
|---|---|---|---|---|---|---|
| `fball_local.c` | 78 | 59 | 0 | 14 | 0 | 5 |
| `fball.c` | 300 | 224 | 2 | 55 | 1 | 18 |

  Survivors of `fball.c`: 577 (the guard for absent place symbols, unreachable) and 784 (`i <= k`, reads one
  word past the arrays; only a sanitizer run kills it).
- The lines for `tools/mutate/equivalent.txt` are in `lanes/m1-local/equivalent-added.txt`.

## Benchmark (provisional), median ns per operation

| Row | 128 blocks, 4096 bits | 1 block |
|---|---|---|
| local add | 3871 | 46.6 |
| global add | 1768 | 596 |
| local mul (h = 1) | 14242 | 199 |
| global mul | 80490 | 1393 |
| set_local | 34816 | 413 |
| set_global | 18086 | 225 |

## Callers checked by tests

`adf_fball_reconstruct`, the printer `adf_fball_get_str`, `adf_adele_add` and friends with local finite
parts. `src/cap.c` still returns `ADF_UNSUPPORTED` for local inputs. The comment at `src/recon.c:50-58` is
stale.

## Header findings

1. `adf_fball_is_canonical` "never aborts, whatever the fields hold" cannot hold for a non-NULL `mctx` that
   is not a context.
2. "when all local inputs share one context pointer": a local and a global input give a global result
   (conventions 5.3, P21 to P23).

## Other findings

`lanes/m1-fball/report.md` says `get_den` was correct for local values; it was not ((2; 2) in (4) has raw
d = 2, canonical d = 1). Fixed.

## Not done

`ADF_CHECK_INVARIANTS` (conventions 4.6). No clang build.

## Avoidable costs

Local add on 128 blocks is 2.2 times slower than global: per block a `n_preinvert_limb` and a
`fmpz_fdiv_ui`, and a copy of K; the per-block `nmod_t` of the context is not reachable. The product with
h > 1 goes global and back. Every accessor on a local value recombines.
