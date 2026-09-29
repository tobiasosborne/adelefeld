# Lane s3-slice3: result

The rest of `include/adelefeld/resid.h` (S.3 of milestone S, `docs/api-s.md` section 2). Twelve new
functions, the test file `tests/test_resid_rest.c` with 17 tests, two vector files and the two new Julia
ccalls. Every function was built after its test; the red and green runs are in `redgreen.log`, the
progress one section per function in `progress.md`.

## What was done

1. **Life cycle** (`docs/conventions.md` 2.3, the table of `docs/api-s.md` section 1):
   `adf_resid_set`, `adf_resid_swap`, `adf_resid_identical`, `adf_recon_cert_set`,
   `adf_recon_cert_swap`, `adf_recon_cert_identical`, `adf_recon_cert_is_canonical`. The two types have
   no context pointer and no heap of their own beyond their `fmpz`, so `set` is two (four) copies,
   `swap` is the FLINT one, and `identical` is a comparison of the fields. The layout of the two types
   (16 and 40 bytes, the five offsets) is pinned in `tests/test_resid_rest.c`, as the brief asks.

2. **`adf_resid_set_rat`, `adf_resid_contains_rat`** (`solvers` Lemma 1.2, Proposition 1.10 (1)). For a
   reduced `q = n/d`: `set_rat` is `OK` exactly when `gcd(d, m) = 1` and then gives the unique `c` in
   `[0, m)` with `c d = n` modulo `m`; `m = 1` is read separately (`P(1, 0) = Q`, no modular inverse).
   `contains_rat` is 1 exactly when `gcd(d, m) = 1` and `m | c d - n`.

3. **`adf_resid_set_fball_forget`** (`solvers` Proposition 1.10 (4)). The canonical triple of the ball
   is read with `adf_fball_get_fmpz3`, so a local value and a global value take the same path; `x` is
   `P(H, A d^(-1) mod H)`. `DOMAIN` for `H = 0` and for `gcd(d, H) > 1`, `x` untouched in both cases.

4. **`adf_resid_reconstruct_first`** (decision S-D4). The body of `adf_resid_reconstruct` became the
   static `resid_solve`, which returns the status of Algorithm R and reports the first point of
   Proposition 1.5 in the order of Algorithm R (and the pair `(c mod m, 1)` of Proposition 1.6 (a) when
   `A >= m`). `adf_resid_reconstruct` is a wrapper over it and returns and writes exactly what it
   returned and wrote before; `resid_search` needed no change, since it already leaves its first
   reduced point in the output on `OK`, on `NOT_UNIQUE` and on a cut search that found one. The
   statuses are `OK` (with `count` 1, 2 or 0), `NO_SOLUTION` and `NOT_DETERMINED`, as the table of
   `docs/api-s.md` section 2 says.

5. **`adf_resid_verify_result`** (`solvers` Proposition 1.11, note 4 of `docs/api-s.md` section 2).
   `adf_recon_cert_check` first for the three statuses that need a pair; then `|T| > B`, `2 A B < m` and
   `gcd(R, T)` for `NO_SOLUTION`, the row `(sigma R, |T|)` for `OK` in the range `2 A B < m`, the
   complete enumeration of Proposition 1.5 (4) otherwise, the four conditions of Proposition 1.7 (3)
   with the cut search for `NOT_DETERMINED`, and the complete enumeration for `NOT_UNIQUE` when
   `A < m` (Proposition 1.6 (a) without a search when `A >= m`).

6. **`tests/julia/resid.jl`**: one `ccall` of `adf_resid_reconstruct_first` (the count as a
   `Ref{Cint}`, it is an `int *` report argument) and one of `adf_resid_set_fball_forget`, 16 and 14
   checks. `tests/julia/smoke.jl` includes this file (line 282), so `sh tests/test_julia.sh` runs them.

## Files written

- `include/adelefeld/resid.h`: the twelve declarations with the contract of each (the sources are cited
  in the comment of every declaration), and `#include "adelefeld/fball.h"` for the ball argument of
  `adf_resid_set_fball_forget`.
- `src/resid.c`: the twelve functions, the statics `resid_solve`, `resid_pair`, `is_solution` and
  `enumerate_all`. The one change to an existing function is the extraction described above, which
  item 4 of the brief allows; nothing else of slices 1 and 2 was touched.
- `tests/test_resid_rest.c` (new, 17 tests, 7563562 checks).
- `tests/ref/vectors/s3-slice3/recon_first.jsonl` (15338 lines, 1.8 MB) and
  `tests/ref/vectors/s3-slice3/sets.jsonl` (8610 lines, 916 kB), both written by
  `lanes/s3-slice3/gen_vectors.py`, which imports `proto/solvers_checks.py` and does not copy it.
  Every line of both files is run by the tests.
- `tests/julia/resid.jl` (additions only).
- `lanes/s3-slice3/`: `progress.md`, `redgreen.log`, `gen_vectors.py`, `mutants.sh`, `mutants.out`,
  `check-plain.log`, `check-san.log`, `check-clang.log`, `exports.log`, `julia.log`, this file.

## Checks (command, result)

Red-green, `lanes/s3-slice3/redgreen.log`. Red 1, 2, 3, 4, 5: a link error, the name of the function of
the item undefined (`adf_resid_set`, `adf_resid_swap`, `adf_resid_identical`, `adf_recon_cert_set`,
`adf_recon_cert_swap`, `adf_recon_cert_identical`, `adf_recon_cert_is_canonical`; then
`adf_resid_set_rat`, `adf_resid_contains_rat`; then `adf_resid_set_fball_forget`; then
`adf_resid_reconstruct_first`; then `adf_resid_verify_result`). Green: `./build/test_resid_rest` ends
with `17 tests, 7563562 checks, 0 failed checks, 0 failed tests`. The log also lists, for every item,
the faults that the first run found in my own tests (a wrong expectation of canonicality, a loop
variable that the helper overwrote, the canonical triple of a ball, the round of a point and the
rounds a cut search visits, the truth of a claim of Proposition 1.11, one status name). No claim was
weakened; in five of the six cases the library was right and the test was wrong.

What the test file checks, with the numbers of the last run:

- `set_rat` and `contains_rat`: 7320 reduced fractions with `|n| <= 12`, `d <= 12`, over every
  `m <= 40`; `set_rat` OK 5176, DOMAIN 2144, and for each of them `m | n - c d` for the `c` it gives,
  `is_canonical` after every `OK` and `x` untouched on every `DOMAIN`; `contains_rat` for every `c` in
  `[0, m)`: inside 5176, outside 144884, each against the definition. `m = 1`, `m = 0`, `m < 0`, huge
  operands of 200 digits and a fraction of 430 bits are separate cases.
- `forget`: 624 balls (every raw `H <= 12`, `d <= 8`, `a` in `[0, H)`), read on the canonical triple:
  475 kept and 149 refused; every rational `(a + h k)/d` with `-20 <= k <= 20`, reduced, is in the
  result (41 per kept ball, 19475 memberships), `DOMAIN` exactly for `gcd(d, h) > 1` on the canonical
  triple and for the 8 exact balls, `x` untouched in every `DOMAIN`. The canonical triple is exercised
  directly: `(6 + 12 Zhat)/2 = 3 + 6 Zhat` gives `P(6, 3)`, `(7 + 6 Zhat)/1` gives `P(6, 1)`, `H = 1`
  gives `P(1, 0)`, the inclusion is proper (`21/5` is in `P(6, 3)` and not in `3 + 6 Zhat`). The
  example of note 1 of `docs/api-s.md` section 2 is its own test: `5 + 6 Zhat` gives
  `adf_fball_reconstruct` on `[0, 1]` = `NO_SOLUTION`, then `adf_resid_reconstruct` with `A = 1`,
  `B = 5` = `NOT_UNIQUE` and with `B = 4` = `OK` with `-1/1`. A local value of `(a + 210 Zhat)/d` in a
  context with the blocks 6 and 35 gives the same canonical triple and the same residue as the global
  value, for 100+ such balls, and every rational of the ball is in it.
- `reconstruct_first` on the grid of item 4 of the brief: 363048 calls; status, `q` and `count` against
  the enumeration of Definition 1.1 written in the test, with the round `(x, y)` of every point recovered
  by Cramer's rule from an own Euclidean algorithm and the list sorted by `(x, y)`. OK 289494,
  NOT_DETERMINED 48243, NO_SOLUTION 25311; count 0/1/2 = 94455/30623/237970, of which 20901 are `OK`
  with count 0 after a cut search. `q` untouched on the 73554 calls with another status; the certificate
  canonical on every call, `kind = 0` with the four integers 0 exactly for the empty box and `A >= m`,
  and otherwise equal to the pair of the own Euclidean algorithm and accepted by
  `adf_recon_cert_check`. The enumeration is cross-checked against the literal double loop of
  Definition 1.1 for `m <= 8`.
- `tests/ref/vectors/s3-slice3/recon_first.jsonl`: 15338 lines, all run, from `recon_first` of the
  reference. OK 12144, NOT_DETERMINED 1809, NO_SOLUTION 1385; count 0/1/2 = 4204/1332/9802, the same
  numbers as the generator printed; 696 lines with `m` or `A` above a million, `m` up to 2000 bits; the
  status, `q`, `count`, the certificate and its `kind` of every line.
- `tests/ref/vectors/s3-slice3/sets.jsonl`: 8610 lines, all run: `set_rat` 5490 (OK 3612, DOMAIN 1878)
  with the predicate for every `c` in `[0, m)` (inside 3612, outside 125037) and the claim that exactly
  one residue contains the rational when `gcd(d, m) = 1` and none otherwise; `forget` 3120 (OK 2375,
  DOMAIN 745) with the membership of `(a + h k)/d` for `k` in `{-20, -1, 0, 1, 20}`.
- `verify_result`: the same grid again, 363048 results of `adf_resid_reconstruct`, every one accepted.
  The changed claims, refused / accepted and true by the enumeration of the test: another status
  1020122/69022, `q` with the numerator + 1: 30623/332425, with the denominator + 1: 25544/337504, with
  the sign changed: 25544/337504, one certificate entry changed (`Rp + m`, `T` negated, `R + 1`, `Tp`
  negated): 431989/272651, `kind` flipped: 125078/237970. Every kind of change is refused at least once,
  and no accepted claim is false. A status outside the class, `ADF_LIMIT`, `ADF_DOMAIN` and the integer
  12345 give 0. The 15338 vectors: every result accepted, 453 lines with `X` above a word, 9802 NOT_UNIQUE
  claims with the status flipped, all refused. `m = 5, c = 3, A = 2, B = 2^70`: the two solutions
  `-2/1` and `1/2` are both in the round `x = 1`, so the status is `NOT_UNIQUE` at limit 1 with
  `X = 2^70` above a word and the checker returns 0 there (stated in the header), while the
  `NOT_DETERMINED` claim at limit 0 is accepted, since the four conditions of Proposition 1.7 (3) need no
  enumeration. The empty box, `m < 1` and `m < 0` are separate cases.
- `tests/test_resid.c` and `tests/test_resid_full.c` after the extraction of `resid_solve`:
  `12 tests, 3395820 checks, 0 failed checks` and `7 tests, 29391859 checks, 0 failed checks`, the same
  counts as the report of slice 2.
- `make clean && make -j2 check`: `check passed: all 49 test programs` (`check-plain.log`; the file of
  this lane is one of the 49).
- `make clean && make -j2 check SAN=1`: `check passed: all 49 test programs` (`check-san.log`; no
  sanitizer report in the log).
- `make clean && make -j2 check CC=clang`: `check passed: all 49 test programs`, no warning
  (`check-clang.log`).
- `sh tests/test_exports.sh`: `test_exports: passed: 218 of 218 declared functions are exported, 0 are
  not implemented yet, no exported name is undeclared, no variadic function` (`exports.log`). The 218
  are the 206 of slice 2 and the 12 of this lane.
- `sh tests/test_julia.sh`: `test_julia: passed (with LD_PRELOAD=/lib/x86_64-linux-gnu/libgmp.so.10)`,
  the three test sets of `resid.jl`: `adf_resid_reconstruct through ccall | 28 28`,
  `adf_resid_reconstruct_first through ccall | 16 16`,
  `adf_resid_set_fball_forget through ccall | 14 14` (`julia.log`; the LD_PRELOAD is the known gmp
  quirk of `tests/test_julia.sh`, not a fault of this lane).
- Mutation testing, `sh lanes/s3-slice3/mutants.sh` (18 hand mutants of the added code, one at a time,
  `build/test_resid_rest` after each): 14 killed, 4 survived and are equivalent on the inputs the
  contract admits, each with a one-sentence reason in `mutants.out`. The run found one gap of the
  tests, now repaired: every `m < 1` case of `adf_resid_set_rat` had a denominator 2 and was caught by
  the gcd instead of the test of `m`, so a solver that did not test `m` passed; the cases `q = 3/1` with
  `m = 0`, `m = -1` and `m = -4` were added. `tools/mutate/equivalent.txt` is not a file of this lane,
  so the four equivalent mutants are listed in `mutants.out` instead.

## Not done

- No `make mutate` and no fuzzing; the mutation testing is the 18 hand mutants above, on `src/resid.c`
  only, and the differential fuzzing of slice 2 (`tests/fuzz/diff_resid.py`) was not extended: it
  exercises `adf_resid_reconstruct`, whose behaviour did not change.
- No driver, no text form and no dump form (decision S-D12: none for these types), no benchmark.
- The layout pins of the two types are in `tests/test_resid_rest.c`, as the brief asks; no new type and
  no new layout was added, so `tests/test_abi.c` was not touched.
- `adf_resid_verify_result` is not decided when the complete enumeration it needs has more rounds than
  a word holds (`X = floor(B/abs(T))` above `WORD_MAX`) and the claim is not a cut one. This is stated
  in the header and tested; it is not a gap of the tests but a limit of the checker, and the reference
  has no such limit because its enumeration is a Python loop.
- The review of the existing functions that runs in parallel was not touched: no line of
  `adf_resid_reconstruct` (outside the extraction) and no line of `tests/test_resid.c`,
  `tests/test_resid_full.c` was changed.

## Sources pending

None. Every formula used is quoted from a file of this repository with its line: `docs/proofs/solvers.md`
Definition 1.1 (line 75), Lemma 1.2 (line 85), Definition 1.3 (line 117), Lemma 1.4 (line 133),
Proposition 1.5 (line 162), Proposition 1.6 (line 216), Algorithm R and Proposition 1.7 (line 256),
Proposition 1.10 (line 386), Proposition 1.11 (line 462); `docs/conventions.md` 2.3 (line 136), 3.1,
3.2, 4.1, 4.3, 4.4, 5.1, 5.2; `docs/api-s.md` section 2 with its five notes; the oracle
`proto/solvers_checks.py` (`recon_first` line 230, `recon_verify_result` line 249, `check_s3_verify`
line 493, `check_s3_forget` line 658, `lattice_points` line 173, `is_solution` line 244); the FLINT
declarations used (`/usr/include/flint/fmpz.h:572` `fmpz_invmod`, `fmpq.h:176` `fmpq_get_str`,
`fmpq.h:117` `fmpq_is_canonical` as cited in `rat.h`) and the headers of the library
(`include/adelefeld/fball.h:54 to 58` for predicate G, `:118 to 124` for the canonical triple, `:161`
for `adf_fball_get_fmpz3`, `include/adelefeld/modctx.h:153` for `adf_fball_set_local`,
`include/adelefeld/status.h` for the codes).

## Findings against the specification

Nothing in `docs/SPEC.md` or `docs/proofs/solvers.md` was found to be wrong. Four points of
`docs/api-s.md` section 2 are worth the attention of the reviewer; none of them is a contradiction, and
in each case the reading that the code implements is written in the header.

1. **The interface of `adf_resid_verify_result` carries one `adf_rat`, not the list `S` of
   Proposition 1.11 (1).** The claim "NOT_UNIQUE with `S`" of `solvers.md:471` is accepted when `S` has
   two different pairs that are both solutions; with one rational such a claim cannot be expressed. Note
   4 of `docs/api-s.md` names the second way ("or by repeating the complete enumeration until two are
   found"), so the code uses it: for `A < m` the checker runs the complete enumeration of
   Proposition 1.5 (4) and accepts iff two solutions are found, and for `A >= m` it accepts by
   Proposition 1.6 (a). The certificate is deliberately not read for this status, since the claim is
   about the set. The same gap applies to `NOT_DETERMINED`, where Proposition 1.11 (2) asks for the list
   of the reduced points of the rounds `x <= ell`: `adf_resid_reconstruct` leaves `q` untouched on that
   status (conventions 4.3), so `q` is not part of the claim, and the code checks the four conditions of
   Proposition 1.7 (3) and runs the cut search, which is the fourth condition.
2. **A predicate that is `0` for a true claim.** The row of `docs/api-s.md` section 2 says `1` if the
   claim is true, and the cost column admits "the cost of the search itself". The search has
   `floor(B/abs(T))` rounds and the round counter is a word, so for `X` above `WORD_MAX` a claim of
   `NOT_UNIQUE` or of `OK` in the range `m <= 2 A B` is true and not decided. The header says so
   ("`0` also where the claim may be true but this function cannot decide it") and
   `verify_result_when_X_is_above_a_word` tests it with `m = 5, c = 3, A = 2, B = 2^70`. If the
   milestone wants `1` there, the checker needs an enumeration that does not count the rounds in a word.
3. **Note 5 of `docs/api-s.md` section 2 and `adf_resid_reconstruct_first`.** The note says that with
   `limit <= 0` the status is not "`2 A B >= m`", and gives `m = 2, c = 1, A = 2, B = 1` as an example of
   `NOT_UNIQUE`. That is the status of `adf_resid_reconstruct`; for `adf_resid_reconstruct_first` the
   same problem is `OK` with `q = 1/1` and `count = 2`, since that function reports a solution whenever
   there is one. The code follows the table of the function (decision S-D4 and `recon_first` of the
   reference); the note reads as if it applied to both rows.
4. **"The first in the order of Algorithm R" for `A >= m`.** Algorithm R returns at step 3 when
   `A >= m` and never enumerates, so the order is empty there. The parenthetical of the table
   ("for `A >= m`: `c/1`") and `recon_first` (line 236, which returns `sols[0] = (c, 1)`) fix the
   meaning; the code takes the first pair of Proposition 1.6 (a), with `c` reduced into `[0, m)`.
