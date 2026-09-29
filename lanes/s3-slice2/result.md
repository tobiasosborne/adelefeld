# Lane s3-slice2: result

## What was done

`adf_resid_reconstruct` now answers in the whole range (Algorithm R, `docs/proofs/solvers.md:256` to `324`):
`A >= m` NOT_UNIQUE; `abs(T) > B` NO_SOLUTION; `2 A B < m` as in slice 1; otherwise the search `resid_search` with
`ell = max(limit, 0)`: NOT_UNIQUE at the second reduced point (also in a cut search), NOT_DETERMINED when
`X > ell` and fewer than two were found, else OK or NO_SOLUTION. `X = floor(B/abs(T))` is an fmpz, compared with
`ell` as an fmpz and converted to a word only when `X <= ell`. The `y` of a round come from two divisions.
`cert` is written on every status (none for an empty box and for `A >= m`). New function
`adf_recon_cert_check(cert, x, A)` (C1) to (C4). `ADF_UNSUPPORTED` no longer occurs. The header lost the
sentences about the temporary status and the unused limit.

## Files written

- `include/adelefeld/resid.h`, `src/resid.c` (both changed)
- `tests/test_resid.c` (changed), `tests/test_resid_full.c` (new), `tests/test_abi.c` (one test added)
- `tests/ref/vectors/s3-slice2/recon.jsonl` (11298 lines, 1.8 MB, from `lanes/s3-slice2/gen_vectors.py`, which
  imports `proto/solvers_checks.py`)
- `tests/fuzz/diff_resid.py`, `tests/julia/resid.jl`
- `lanes/s3-slice2/`: `gen_vectors.py`, `mutants.sh`, `mutate_one.py`, `mutants.out`, `mutants_sigma.out`,
  `checks.sh`, `check-plain.log`, `check-san.log`, `check-clang.log`, `fuzz180.out`, `redgreen.log`, `result.md`

## Checks (command, result)

- Red-green: `lanes/s3-slice2/redgreen.log`. Red 1 a link error (first test of the files); red 2 with
  `adf_recon_cert_check` written and the slice-1 reconstruct: `test_resid` 1518087 failed checks, `test_resid_full`
  2640224 failed checks (assertions); green: `test_resid` 12 tests, 3395820 checks, 0 failed;
  `test_resid_full` 7 tests, 29391859 checks, 0 failed. Three faults in my own new tests were found and repaired in
  the tests (details in the log); no test expectation of slice 1 was changed except the `UNSUPPORTED` ones and
  the "limit is ignored" test (now: same limit with and without `cert`).
- `test_resid_full` output (what makes a case fail: status, `q` or certificate differing from the enumeration
  written in the test, `adf_recon_cert_check`, or `q` written on a status other than OK):
  - enumeration against the literal double loop of Definition 1.1: 79664 problems (m <= 12), equal lists;
  - whole range, `m <= 36`, no cut: 804804 cases; NO_SOLUTION 52674, OK 92246, NOT_UNIQUE 659884; of them in the
    range `2AB >= m`: 14082, 69624, 659884 (none of the counts is 0; NOT_DETERMINED cannot occur without a limit);
  - limits -1, 0, 1, 2, 5, `m <= 24`: 1236900 calls; NO_SOLUTION 71481, OK 70868, NOT_UNIQUE 785882,
    NOT_DETERMINED 308669; NOT_UNIQUE inside a cut search 108402 times; OK from a cut search 0 times; the wrong
    reading "limit 0 is `2AB >= m`" fails 127564 times; the own Euclidean pair equals the library's certificate;
    247380 calls with `cert = NULL`. The expected status is recounted in the test from the enumeration and Cramer's
    rule (round of a point), not from a loop like the library's;
  - vectors: 11298 lines, all run; NO_SOLUTION 1114, OK 1293, NOT_UNIQUE 6281, NOT_DETERMINED 2610; 314 lines with
    `X >= 2^64`; 237 lines with `limit = WORD_MAX`; `m` up to 4001 bits; each call under 1 s (the slowest read
    0.000 s);
  - large operands (64, 300, 2000 bit `m`, `2AB` near `m`, `AB` near `m`): 368 calls, each timed under 1 s, plus
    24 with `X` above `2^64` (c = 1, `X = 2^70`, NOT_DETERMINED at limits 0, 3, 10^6; A = m/3, `B = 2^70 m`,
    NOT_UNIQUE at `WORD_MAX`, checked to have `X >= 2^65`);
  - fixed cases of `check_s3_edge`, note 5 (`m=2, c=1, A=2, B=1, limit 0` NOT_UNIQUE, cert none), `m=2, c=1,
    A=B=1` (two solutions with denominator 1: NOT_DETERMINED at limit 0, NOT_UNIQUE at limit 1); aliasing.
- Mutants, `sh lanes/s3-slice2/mutants.sh` (scratch copy `build/mut`, one change at a time; output in
  `mutants.out`, the sigma mutant re-run in `mutants_sigma.out` because the first form did not compile). All 13 killed:
  | mutant | failing tests |
  |---|---|
  | `A >= m` to `A > m` | `test_resid` brute force; `test_resid_full` whole range, limits grid, vectors, edge cases |
  | `X > ell` to `X >= ell` | `test_resid_full` limits grid, vectors (`test_resid` passes: not enough there) |
  | two found to three | both brute-force tests, limits grid, vectors, edge cases |
  | gcd test of a lattice point removed | both brute-force tests, limits grid, vectors, edge, large operands |
  | gcd(R, T) test of 1.6 (c) removed | both brute-force tests, slice-1 vectors, FLINT wrapper, full grid |
  | lower bound of y +1 / -1 | brute-force tests, limits grid (the +1 run hit the 170 s timeout, exit 124: killed) |
  | upper bound of y +1 / -1 | brute-force tests, limits grid (the -1 run hit the 170 s timeout: killed); +1 also vectors, edge, large |
  | rounds to `ell + 1` | `test_resid_full` limits grid, vectors, edge, large operands |
  | `d > B` to `d >= B` | brute-force tests, limits grid, vectors, edge |
  | sigma dropped in the search | brute-force tests, limits grid, vectors, edge, large |
  | X compared as its low word (`fmpz_get_ui`) | `test_resid_full` large operands only (X = 2^70 with limit 0/3) |
- `make clean && make -j2 check`: `check passed: all 44 test programs`; with `SAN=1`: `check passed: all 44 test
  programs`; with `CC=clang`: `check passed: all 44 test programs` (logs in the lane directory).
- `sh tests/test_exports.sh`: `passed: 206 of 206 declared functions are exported, 0 are not implemented yet, no
  exported name is undeclared, no variadic function`.
- `sh tests/test_julia.sh`: `adf_resid_reconstruct through ccall | 28 28`; `test_julia: passed (with
  LD_PRELOAD=...)` (the retry path is the known gmp quirk). Julia cases: NOT_UNIQUE, NOT_DETERMINED (limits 0, -3),
  `A >= m`, huge `X`, planted 200-digit fraction.
- Fuzz: `python3 tests/fuzz/diff_resid.py --seconds 180 --seed 20260930`: 592042 cases, no disagreement; OK 286242,
  NO_SOLUTION 144506, NOT_UNIQUE 103517, NOT_DETERMINED 57777; cut searches 85477 (NOT_UNIQUE inside one 27700);
  `A >= m` 68406; empty box 35285; planted fractions 278648. Random limits from {-3,...,1000}. This is a 180 s
  smoke test of the contract (status, q, certificate, untouched outputs), not more.

## Not done

- No `make mutate`; mutation testing is the 13 hand mutants above, on `src/resid.c` only.
- `adf_resid_reconstruct_first`, `adf_resid_verify_result`, `set_rat`, `set_fball_forget`, `contains_rat`, `swap`,
  `identical`, driver, benchmarks (as the brief says).
- A call with a large limit (up to `WORD_MAX`) on a problem with one solution and huge `X` runs for as long as `X`;
  the header says so; no test runs such a search.
- The vectors do not cover `limit` near `WORD_MAX` with a cut search (the reference would not end either).
- Clang was only run on `make check`; the fuzz run used the gcc build.

## Header findings

- None blocking. Written into the header (it is mine in this lane): `cert` for `A >= m` is `kind = 0`, as
  `docs/api-s.md` says ("the pair if `0 <= A < m` and `B >= 1`"); `adf_recon_cert_check` refuses `kind = 0` and
  `m < 1`.

## Findings against the specification / `docs/proofs/solvers.md`

- None. Avoidable cost, not optimised: the gcd of each lattice point is done with `fmpz_gcd` on full values;
  `n` and `d` could be tested against `R`, `T` rows first. The Euclidean loop keeps the `t` column only.
