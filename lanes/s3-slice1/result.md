# Lane s3-slice1: result (issue adf-1y1)

Partial rational reconstruction in the range `2 A B < m`, end to end: header, C code, tests against a brute-force
oracle, vectors from `recon_partial`, a differential fuzz run, a Julia `ccall`. No git, no `bd`.

## What was done

- `include/adelefeld/resid.h` (new): `adf_resid_struct {fmpz c, m}` (16 bytes), `adf_recon_cert_struct
  {fmpz Rp, Tp, R, T; int kind}` (40 bytes); `adf_resid_init/clear/set_fmpz2/get_fmpz2/is_canonical`,
  `adf_recon_cert_init/clear`, `adf_resid_reconstruct` with the final signature of `docs/api-s.md` section 2, and
  the four `adf_sizeof_*`/`adf_alignof_*` inline functions. The header says that `UNSUPPORTED` is temporary and goes
  away with slice 2, and that `limit` is unused. Nothing else of section 2 is declared. One include line added to
  `include/adelefeld.h`.
- `src/resid.c` (new). Order in `adf_resid_reconstruct`: `A < 0` or `B < 1` gives `NO_SOLUTION`, `cert.kind = 0`;
  `2 A B >= m` (an `fmpz`) gives `UNSUPPORTED`, nothing written; else the Euclidean loop of Lemma 1.4 (column `t`
  only, stop at the first `r <= A`), `abs(T) > B` gives `NO_SOLUTION`, `gcd(R, T) = 1` gives `OK` with
  `q = sigma R/abs(T)`, else `NO_SOLUTION` (row not divided by the gcd). `cert` gets the pair on the three
  statuses of the last step. `q` is written only on `OK`. Cited in the comment by file and line.
  `set_fmpz2` and `get_fmpz2` go through temporaries, so `c` and `m` may be members of `x`.
- `tests/test_resid.c` (new, 11 tests), `tests/ref/vectors/s3-slice1/recon.jsonl` (2643 lines, 1.5 MB), written by
  `lanes/s3-slice1/gen_vectors.py` (imports `recon_partial`, `is_solution` from `proto/solvers_checks.py`; the
  16 fixed cases of `check_s3_edge` are copied into the script because that list is local to the function),
  `tests/fuzz/diff_resid.py`, `tests/julia/resid.jl`, additions to `tests/test_abi.c` (sizes, alignments, offsets,
  field types of both structs; `adf_sizeof_*`).

## Red-green (`lanes/s3-slice1/redgreen.log`)

- Red 1: `tests/test_resid.c` without `src/resid.c`: link error (`undefined reference to adf_resid_set_fmpz2`,
  `adf_recon_cert_init`, ...). Red 2: a stub of the eight functions (`set_fmpz2` doing nothing, `reconstruct`
  returning `UNSUPPORTED`): 10 of 11 tests failed by assertion, 176786 failed checks, for example
  `m=1 c=2 A=0 B=2: want 0/1, status 8`. Green: `src/resid.c`: `11 tests, 3260773 checks, 0 failed checks`.
- One test expectation was wrong on the first green run (boundary test, `m = 2AB+1`, `c = 12345` is itself a
  small solution `12345/1`, so the status is `OK`); I replaced `c` by `10^25 + 7`, which `recon_partial` gives as
  `NO_SOLUTION`. The code was not changed for that.

## What the tests count (`./build/test_resid`)

- Brute force by Definition 1.1 (enumeration of `n`, `d` in the test): every `m <= 36`, `c` from `-m` to `2m`,
  `A` from 0 to `2m`, `B` in `{1,2,3,5,m-1,m,m+1,2m}` where positive: 804804 cases; 61214 inside `2AB < m`
  (`OK` 22622, `NO_SOLUTION` by (b) 29754, by (c) 8838), 743590 `UNSUPPORTED`. A case fails if the status differs
  from the enumeration, if `q` differs, if `q`/certificate is changed on a status other than the one that writes
  them, if the certificate breaks (C1) to (C4), or (for NO_SOLUTION by (c)) if `gcd(R, T) = 1`. The test fails if
  any of the three counts is 0 or if they do not sum to 61214.
- Vectors: all 2643 lines run: `OK` 1117, `NO_SOLUTION` (b) 1384, (c) 140, empty box 2 (m of 20, 64, 300, 2000
  bits with `m` just above and far above `2AB`; `m = 12, c = 6, A = 1, B = 5` and 400 small `gcd > 1` cases).
  A line fails on a different status, `q`, or certificate rows.
- Certificate (C1) to (C4) is tested on every pair returned in the brute force, the vectors and the boundary tests.
- Boundary: `2AB = m - 1, m, m + 1` with `A = 10^15+1`, `B = 10^15+3` (planted `-A/B` returned at `m = 2AB+1`, `UNSUPPORTED`
  at the other two, `q` and `cert` sentinels untouched); `2AB = 2^65` against `m = 2^64` and `2^65 + 1` (does not fit
  in a word); `A = 0`; `m = 1`, `m = 2`; `B = 10^100` with `A = 0`; `A < 0`, `B < 1` (also with `2AB >= m`);
  `cert = NULL`; `limit` values -5, 0, 1, 2, 100 give equal answers; `set_fmpz2` with `m = 0, -1, -6` is `DOMAIN`,
  `x` untouched, reduction of `c` (also 3000 bit `m`), aliasing of `x->c`, `x->m`.
- Wrapper test against FLINT `fmpq_reconstruct_fmpz_2` (inside `2AB < m`, `m > 2`, `A >= 1`): 120651 cases
  (every `m` 3..60, plus 3000 random of 8 to 308 bits); adf `OK` iff FLINT returns 1, and `q` equal: 51645 both
  `OK` and equal, 69006 both none.

## Mutants (item 6; `sh lanes/s3-slice1/mutants.sh`, output `mutants.out`; scratch copy in `build/mut`)

| Mutant | Result |
|---|---|
| `r <= A` to `r < A` (loop `>= A`) | the test binary aborts (exit 134): with `A = 0` the loop reaches `r1 = 0` and FLINT divides by zero. Detected as a crash, in the first test that reaches `A = 0` (`brute_force_every_m_up_to_36`), not as an assertion. |
| `abs(T) > B` to `>= B` | 38157 failed checks in 5 tests: brute force, vectors (8), boundary, aliasing, wrapper |
| `2AB < m` to `2AB <= m` | 4338 failed checks in 3 tests: brute force, boundary_2AB_against_m, boundary_small_moduli_and_zero_A |
| gcd test removed | 28723 failed checks in 4 tests: brute force, vectors (141), boundary_small, wrapper |
| `sigma` dropped | 35023 failed checks in 4 tests: brute force, vectors (518), boundary_2AB, wrapper |

I also built a shared object of the `sigma` mutant and ran `diff_resid.py` on it for 5 s: `FAIL case 3 ... q (28, 15),
reference (-28, 15)`. `make mutate` was not run.

## Fuzz (a smoke test, not the long run)

`python3 tests/fuzz/diff_resid.py --seconds 180 --seed 20260929` (ctypes on `build/libadelefeld.so` built by
`sh tests/test_exports.sh`): `1022073 cases in 180.0 s, no disagreement; OK: 436179; NO_SOLUTION (b): 107813;
(c): 48500; (empty box): 61090; UNSUPPORTED: 368491; planted fractions returned: 296260`. It asserts equal status and
`q` against `recon_partial`, equal certificate (or `kind = 0`, or untouched with `cert = NULL`), `UNSUPPORTED`
exactly when `A >= 0, B >= 1, 2AB >= m` (with `q` and the certificate untouched), and that a planted fraction is
returned when `2AB < m`. Sizes: small, 30 to 63 bits, 200 to 3000 bits; `m` in `2AB - 2 .. 2AB + 2`, just above, far above.
For `UNSUPPORTED` cases the reference is not called (it has an answer only in slice 2). Output in `fuzz180.out`.

## Checks (last line of each)

- `make clean && make -j2 check`: `check passed: all 43 test programs` (`13 tests, 18122 checks, 0 failed checks, 0 failed tests` is the last test program).
- `make clean && make -j2 check SAN=1`: `check passed: all 43 test programs`.
- `make clean && make -j2 check CC=clang`: `check passed: all 43 test programs`.
- `sh tests/test_driver.sh`: `test_driver: 27 cases, 100376 expected lines, all equal (SAN=0)` (unchanged: the driver was not touched).
- `sh tests/test_exports.sh`: `test_exports: passed: 205 of 205 declared functions are exported, 0 are not implemented yet, no exported name is undeclared, no variadic function`.
- `julia --startup-file=no tests/julia/resid.jl build/libadelefeld.so`, with `LD_PRELOAD` of the system libgmp (the known quirk of `tests/test_julia.sh`; without it Julia cannot load libflint): `adf_resid_reconstruct through ccall | Pass 14 Total 14`. `tests/test_julia.sh` itself was not changed and does not run this file.

## Driver: not done, on purpose

`adf resid <c> <m> <A> <B>` needs four operands. The driver's documented grammar has one to three operands per
command (`tools/adf/adf.c` header comment and `README.md`; the splitter has room for four, but the table of arities
and the three values `x, y, w` of `adf_drv_command` are for three). As the brief says, I did not change the
grammar, and wrote `tests/julia/resid.jl` instead. `tools/adf/adf.c` and `tests/test_driver.sh` are unchanged; the three
driver cases of the brief do not exist. The Julia file has the same three cases (a fraction, `NO_SOLUTION`,
`UNSUPPORTED`) and more.

## Not done

- The range `m <= 2AB`, `_reconstruct_first`, the verifier, `set_rat`, `set_fball_forget`, `swap`, `identical`, text and dump forms, benchmarks (all out of the slice).
- No mutation run of the tool (`make mutate`), by the brief. No `INV=1` build (`ADF_CHECK_INVARIANTS`): `resid.c` has no invariant checks.
- The long fuzz run (orchestrator). `tests/test_julia.sh` and `tests/test_headers.c` do not know the new header/file.

## Header findings and findings against the specification

1. `docs/api-s.md` section 2 says `cert` "is written on every status: the pair if `0 <= A < m` and `B >= 1`". In this slice
   `UNSUPPORTED` writes nothing, also when `A < m <= 2AB` where a pair exists. This follows the brief; slice 2 must write it.
2. `2AB < m` with `B >= 1` implies `A < m`, so the case `A >= m` is always `UNSUPPORTED` in this slice, while the
   proof (Prop. 1.6 (a)) knows it is `NOT_UNIQUE`. Not a conflict, but the header says it.
3. `docs/proofs/solvers.md` Lemma 1.4: nothing found wrong. One remark on the loop: with `A = 0` the mutant `r < A`
   divides by zero, so a correct implementation must test `r <= A` before dividing (the code does).
4. The header's aliasing sentence for `adf_resid_get_fmpz2` (`c` and `m` different objects, each may be a member of `x`)
   is met by using temporaries; the test swaps `x->m` and `x->c` in place.
5. `check_s3_edge` keeps its case list local; `gen_vectors.py` copies it (with the line reference). If that list
   changes, the copy does not follow.

## Files

`include/adelefeld/resid.h`, `include/adelefeld.h` (one line), `src/resid.c`, `tests/test_resid.c`,
`tests/test_abi.c` (additions), `tests/ref/vectors/s3-slice1/recon.jsonl`, `tests/fuzz/diff_resid.py`,
`tests/julia/resid.jl`, `lanes/s3-slice1/{gen_vectors.py, mutants.sh, mutate_one.py, redgreen.log, mutants.out,
fuzz180.out, check-plain.log, check-san.log, result.md}` (all under `/home/tobias/Projects/adelefeld/.claude/worktrees/agent-a043d11d7298c838d/`).
