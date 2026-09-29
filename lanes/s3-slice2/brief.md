# Lane s3-slice2: partial rational reconstruction in the whole range

Slice 1 is on master (`include/adelefeld/resid.h`, `src/resid.c`, `tests/test_resid.c`,
`lanes/s3-slice1/result.md`): `adf_resid_reconstruct` answers for `2 A B < m` and returns a temporary
`ADF_UNSUPPORTED` otherwise. This slice removes that status. Decisions S-D3 and S-D4 are taken
(`docs/SPEC.md` 15.3). Do not widen the slice beyond what is listed.

Read first: `lanes/COMMON-C.md` (rule 6 does not hold for `resid.h`, which is yours; rule 5 is replaced by
item 5 below); `docs/proofs/solvers.md` lines 117 to 325 (Definition 1.3 to Proposition 1.7 with Algorithm
R) and Proposition 1.8; `docs/api-s.md` section 2 with its notes 2, 4, 5; `proto/solvers_checks.py`
(`lattice_points`, `recon_partial`, `check_s3_complete`, `check_s3_limit`, `check_s3_edge`); the files of
slice 1.

**You own:** `include/adelefeld/resid.h`, `src/resid.c`, `tests/test_resid.c`, `tests/test_resid_full.c`
(new), `tests/ref/vectors/s3-slice2/` (new), `tests/fuzz/diff_resid.py`, `tests/julia/resid.jl`,
`tests/test_abi.c` (additions), `lanes/s3-slice2/`. Everything else is read-only.

## What is built

1. `adf_resid_reconstruct` as `docs/api-s.md` section 2 states it, for every `A`, `B`, `limit`:
   - `A >= m`: `NOT_UNIQUE` (Proposition 1.6 (a)).
   - `A < m`: the certificate pair; `abs(T) > B`: `NO_SOLUTION`; `2 A B < m`: as in slice 1; otherwise
     Algorithm R with `ell = max(limit, 0)`: `NOT_UNIQUE` as soon as two reduced pairs are found (also
     inside a cut search); `NOT_DETERMINED` exactly under the four conditions of Proposition 1.7(3); else
     `OK` with the one solution, or `NO_SOLUTION`.
   - `cert` (may be NULL) is written on every status: the pair if `0 <= A < m` and `B >= 1`, else
     `kind = 0`. `q` is untouched unless `OK`.
   - The number of rounds is at most `min(ell, floor(B/abs(T)))` (Proposition 1.7(4)). `floor(B/abs(T))`
     can be far above a word: compare as `fmpz`, never convert it to `slong` before it is known to fit.
     The `y` of a round are found by division (Proposition 1.5(3)), not by a loop over `y`.
   - The header loses every sentence about the temporary status and about `limit` being unused.
2. `int adf_recon_cert_check(const adf_recon_cert_t cert, const adf_resid_t x, const fmpz_t A)`: (C1) to
   (C4), `docs/api-s.md` section 2. The tests use it in place of their own copy of the four conditions, and
   keep one independent test of the function itself (changed quadruples are refused).
3. Tests first, red then green (`lanes/s3-slice2/redgreen.log`). The tests of slice 1 that expect
   `UNSUPPORTED` are changed to the true answer; no other expectation of slice 1 is changed.
   `tests/test_resid_full.c`:
   - Brute force by Definition 1.1 written in the test: every `m <= 36`, `c` from `-m` to `2 m`, every `A`
     from 0 to `2 m`, `B` in `{1, 2, 3, 5, m - 1, m, m + 1, 2 m}` where positive, with `limit` so large
     that no search is cut: status and `q` against the enumeration (`OK` exactly when the set has one
     element, `NO_SOLUTION` when none, `NOT_UNIQUE` when two or more). Print the count of each status;
     fail if one is 0.
   - The limits -1, 0, 1, 2, 5 on the same grid restricted to `m <= 24`: the status against
     `recon_partial` through vectors written by `lanes/s3-slice2/gen_vectors.py` (import, do not copy),
     and against a recount in the test that does not use the loop of the library: the reduced points with
     `x <= ell` are counted from the brute-force list and Proposition 1.5. A solver that returns
     `NOT_DETERMINED` where two solutions had been found must fail; so must one that returns `OK` from a
     cut search.
   - The fixed cases of `check_s3_edge`; `m = 2`, `c = 1`, `A = 2`, `B = 1`, `limit = 0` is `NOT_UNIQUE`
     (note 5); `m = 2`, `c = 1`, `A = B = 1` has two solutions with denominator 1.
   - Large operands: `m` of 64, 300, 2000 bits with `A B` near `m / 2`, near `m`, and `B/abs(T)` above
     `2^64` with `limit` 0, 3 and `WORD_MAX` (the call must return at once for the small limits; do not
     run a search of more than a second in a test).
   - `q` and `cert` on every status; `cert = NULL`.
4. `tests/julia/resid.jl`: the case `UNSUPPORTED` replaced by one `NOT_UNIQUE` and one `NOT_DETERMINED`.
   `tests/fuzz/diff_resid.py`: the reference is called on every case, with random `limit` (small values,
   0, negative); equal status, equal `q` on `OK`, equal certificate. Cap the work of one case (choose
   `limit` so that at most about 1000 rounds run). Run it for 180 seconds; that run is a smoke test.
5. Show that the tests bite, as `lanes/s3-slice1/mutants.sh` did, in a scratch copy under `build/`, one
   change at a time: `A >= m` to `A > m`; `X > ell` to `X >= ell`; the count of two found solutions to
   three; the gcd test of a lattice point removed; the lower and the upper bound of `y` each moved by one;
   the rounds run to `ell + 1`. Record which test fails. No `make mutate`.
6. `make clean && make -j2 check`, the same with `SAN=1` and with `CC=clang`, `sh tests/test_exports.sh`,
   `sh tests/test_julia.sh` pass. Give the last line of each.

Not in this slice: `adf_resid_reconstruct_first`, `adf_resid_verify_result`, `set_rat`,
`set_fball_forget`, `contains_rat`, `swap`, `identical`, the driver, benchmarks.
