# Lane s3-slice3: progress

One section for each function, in the order of the brief. The red-green runs are in `redgreen.log`.

## 0. Read

- `CLAUDE.md`, `lanes/COMMON-C.md`, `docs/api-s.md` sections 1 and 2 with all five notes.
- `docs/proofs/solvers.md` lines 60 to 494: Definition 1.1 (75), Lemma 1.2 (85), Definition 1.3 (117),
  Lemma 1.4 (133), Proposition 1.5 (162), Proposition 1.6 (216), Algorithm R and Proposition 1.7 (256),
  Proposition 1.10 (386), Proposition 1.11 (462).
- `proto/solvers_checks.py`: `sol_brute` (121), `eea_pair` (151), `cert_pair_ok` (163), `lattice_points` (173),
  `recon_partial` (193), `recon_first` (230), `is_solution` (244), `recon_verify_result` (249),
  `check_s3_verify` (493), `check_s3_forget` (658).
- `include/adelefeld/fball.h` (canonical triple, `adf_fball_get_fmpz3` at line 161), `rat.h`, `status.h`,
  `modctx.h` (`adf_fball_set_local`), `docs/conventions.md` 2.3 (line 136), 3.1, 3.2, 4.1, 4.3, 4.4.
- State on master: `src/resid.c` has `adf_resid_init/clear`, `adf_resid_set_fmpz2`, `adf_resid_get_fmpz2`,
  `adf_resid_is_canonical`, `adf_recon_cert_init/clear`, `adf_recon_cert_check`, `adf_resid_reconstruct`
  (static `cert_set_none`, static `resid_search`).

## 1. Life cycle: `adf_resid_set`, `adf_resid_swap`, `adf_resid_identical`,
   `adf_recon_cert_set`, `adf_recon_cert_swap`, `adf_recon_cert_identical`, `adf_recon_cert_is_canonical`

Header: declarations of the seven functions with the contract of each (conventions 2.3, the table of
docs/api-s.md section 1). Red 1: link error, the seven names undefined. Code: `adf_resid_set`,
`adf_resid_swap`, `adf_resid_identical` next to `adf_resid_clear`; `adf_recon_cert_is_canonical`,
`_set`, `_swap`, `_identical` next to `adf_recon_cert_clear`. Green 1: 4 tests, 71 checks, 0 failed (the
layout test of the two types is in this file, as the brief asks: 16 and 40 bytes, the five offsets).

## 2. `adf_resid_set_rat`, `adf_resid_contains_rat` (solvers L1.2, P1.10 (1))

The oracle is the definition of P(m, c) written in the test: `q = n/d` (reduced, `d > 0`) is in
P(m, c) exactly when `gcd(d, m) = 1` and `m | n - c d`. The grid: every `m <= 40`, every `d <= 12`, every
`|n| <= 12` with `gcd(n, d) = 1` (10374 cases), and for each of them every `c` in `[0, m)` (216480 calls
of the predicate). Red 2: link error, the two names undefined. Code: one gcd and one `fmpz_invmod`,
`m = 1` read separately (`P(1, 0) = Q`, no inverse). Green 2: 6 tests, 337773 checks, 0 failed.

## 3. `adf_resid_set_fball_forget` (solvers P1.10 (4))

The oracle is the definition of P(m, c) again: for every ball with canonical triple (a, h, d) (read
with `adf_fball_get_fmpz3`, the triple the header names), every rational (a + h k)/d with
-20 <= k <= 20, reduced, is in the result. Grid: `h <= 12` raw, `d <= 8`, `a` in `[0, h)`: 624 balls, 41
rationals each. DOMAIN exactly when `gcd(d, h) > 1` (on the canonical triple) or `h = 0`, and `x` is
untouched. Red 3: link error. Code: `adf_fball_get_fmpz3` first, then one gcd, then one `fmpz_invmod`.
Green 3: 10 tests, 363423 checks, 0 failed. The example of note 1 is its own test (5 + 6 Zhat:
`adf_fball_reconstruct` on [0, 1] NO_SOLUTION, B = 5 NOT_UNIQUE, B = 4 OK -1/1), and a local value of
the set (a + 210 Zhat)/d in a context with the blocks 6 and 35 gives the same residue as the global
value, for 100+ such balls.

## 4. `adf_resid_reconstruct_first` (S-D4)

Header: the declaration with the four statuses and the meaning of `count`. In `src/resid.c` the body of
`adf_resid_reconstruct` became the static `resid_solve(fn, fd, nfound, cert, x, A, B, limit)`: the same
steps 1 to 6 in the same order, writing the first point of Proposition 1.5 instead of `q`, and writing
the pair (c mod m, 1) of Proposition 1.6 (a) in the step that leaves the search at once. `resid_search`
needed no change: it already leaves its first reduced point in (fn, fd) on OK, on NOT_UNIQUE and on a
cut search that found one (it writes there at the first hit and only reads it afterwards).
`adf_resid_reconstruct` is now a wrapper that returns the same status and writes the same `q` and
`cert`; `tests/test_resid.c` (12 tests, 3395820 checks) and `tests/test_resid_full.c` (7 tests, 29391859
checks) pass with the same counts as before the change.

Red 4: link error (the declaration and the refactoring were in, the function was not). Green 4: 13 tests,
3284612 checks, 0 failed.
- The grid of the brief (`m <= 24`, `c` in `[0, m)`, `A` from 0 to `2 m`, `B` in `{1, 2, 3, 5, m, 2 m}`,
  the limits -1, 0, 1, 2, 5, 10^9): 363048 calls. OK 289494, NOT_DETERMINED 48243, NO_SOLUTION 25311;
  count 0/1/2: 94455/30623/237970, of which 20901 are cut answers with OK. Status, `q` and `count`
  against the enumeration of Definition 1.1 in the test with the rounds (x, y) of every point; `q`
  untouched on the other two statuses; the certificate on every status, kind 0 exactly for the empty
  box and `A >= m`, and equal to the pair of an own Euclidean algorithm otherwise; the enumeration
  cross-checked against the literal double loop for `m <= 8`.
- `tests/ref/vectors/s3-slice3/recon_first.jsonl`: 15338 lines from `recon_first` of the reference, all
  run: OK 12144, NOT_DETERMINED 1809, NO_SOLUTION 1385, count 0/1/2 4204/1332/9802, 696 lines with `m`
  or `A` above a million, `m` up to 2000 bits.
- `tests/ref/vectors/s3-slice3/sets.jsonl`: 8610 lines for items 2 and 3 (set_rat 5490, forget 3120),
  all run.

## 5. `adf_resid_verify_result` (solvers P1.11, note 4 of section 2)

Header: the declaration with the tests of P1.11 in the order in which they are made, and the two
places where the interface of `docs/api-s.md` differs from the reference: the claim of a NOT_UNIQUE
carries one rational and not a list of two pairs, so it is certified by the complete enumeration (the
second way of note 4), and `q` is read only for OK, since the other statuses leave it untouched.

Code: two static helpers next to the function, `resid_pair` (the pair of Lemma 1.4, computed as
Algorithm R step 4 does, used where the checker needs a pair of its own) and `is_solution`
(Definition 1.1 for one pair), and `enumerate_all`, which runs `resid_search` and refuses only where
the enumeration cannot be run: a complete enumeration with `X = floor(B/abs(T))` above a word, or a pair
with `T = 0`. A cut search (`ell < WORD_MAX`) runs `min(X, ell)` rounds and is bounded by `ell`, so `X`
above a word does not stop it.

Red 5: link error. Green 5: 17 tests, 6822123 checks, 0 failed.
- The grid of item 4 again: 363048 results of `adf_resid_reconstruct`, every one accepted. The changed
  claims, refused / accepted and true by the enumeration: another status 1020122/69022, numerator + 1
  30623/332425, denominator + 1 25544/337504, sign 25544/337504, one certificate entry changed
  431989/272651, kind flipped 125078/237970. Every kind of change is refused at least once; no accepted
  claim is false.
- The 15338 vectors: every result accepted, 453 lines with `X` above a word, 9802 NOT_UNIQUE claims
  whose status was flipped and were all refused.
- `m = 5, c = 3, A = 2` with `B = 2^70`: the two solutions `-2/1` and `1/2` are both in the round
  `x = 1`, so the status is NOT_UNIQUE at limit 1 with `X = 2^70` above a word; the checker returns 0
  there, which the header states, while the NOT_DETERMINED claim at limit 0 is accepted because the
  four conditions of 1.7 (3) need no enumeration.
- The empty box, `m < 1` and a status outside the class.

## 6. `tests/julia/resid.jl`

Two new ccalls in the file that `tests/julia/smoke.jl` includes (line 282), so `sh tests/test_julia.sh`
runs them: `reconstruct_first(c, m, A, B, limit)` with the count as a `Ref{Cint}` (it is an `int *`
report argument) and `forget(A, H, d)`, which makes the ball with `adf_fball_set_fmpz3` and reads the
residue as `c/m` from the two fmpz of `adf_resid`. 16 and 14 checks. The first run had one failed check
(`Zhat/3` is the set `2 Zhat`, so it is a residue, not a DOMAIN); see `redgreen.log`.

## 7. The checks of the brief

- `make clean && make -j2 check`: `check passed: all 49 test programs` (log `check-plain.log`).
- `make clean && make -j2 check SAN=1`: `check passed: all 49 test programs` (`check-san.log`).
- `make clean && make -j2 check CC=clang`: `check passed: all 49 test programs`, no warning
  (`check-clang.log`).
- `sh tests/test_exports.sh`: `passed: 218 of 218 declared functions are exported` (`exports.log`; the
  12 new functions are 206 + 12).
- `sh tests/test_julia.sh`: `test_julia: passed (with LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10)`,
  with the three test sets of `resid.jl`: 28, 16 and 14 checks (`julia.log`).
- Mutation testing, `sh lanes/s3-slice3/mutants.sh`: 18 hand mutants of the added code, 14 killed, 4
  survivors, all of them equivalent on the inputs the contract admits, each with a one-sentence reason
  in `mutants.out`. One gap of the tests was found this way and repaired: the `m < 1` cases of
  `adf_resid_set_rat` all had a denominator 2 and were caught by the gcd, so a solver that did not test
  `m` itself passed; the cases `q = 3/1` with `m = 0, -1, -4` were added.
