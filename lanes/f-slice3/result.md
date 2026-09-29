# f-slice3: result

Lane f-slice3: the rest of work package 1F.3 (local balls completed). Five functions added to `lball.h`, the repair of
finding 1 of f-slice2 in `adf_lball_set_fball`. All five required checks pass. No mutation run and no fuzz run (the
brief excludes both).

## What was done

New functions (`include/adelefeld/lball.h`, block "slice 1F.3-b"; existing declarations not changed):
- `adf_lball_teichmuller(w, v, r, prec)`: the root of `T^(p-1) - 1` with residue `r`. Exact `+1`/`-1` when rational (always at
  `p = 2` and `p = 3`; at odd `p` when `r = +-1`), else a ball of precision `max(prec, 1)`. Newton lifting with doubled
  precision (`api-1f.md` L9, cites `functions.md` Lemma 3 item 2, line 72).
- `adf_lball_decompose_teich(m, w, index, u, x, prec)`: `x = p^m w u`, `w` Teichmueller factor (sign at `p = 2`), `u` principal
  unit, `index` the residue of the unit part modulo `p` (modulo 4 at 2). Determined exactly when the unit part is known modulo
  `p` (odd `p`: `x` does not contain 0) or modulo 4 (`p = 2`: exact `x`, or relative precision `k >= 2`); else `NOT_DETERMINED`
  (also the ball with 0); exact 0 is `DOMAIN` (Proposition 4, line 92; statement L10).
- `adf_lball_frac(r, x)`: `{x}_p` as an exact rational in `[0, 1)` (Proposition 19, line 656; L11). `NOT_DETERMINED` for a ball
  with `N < 0`.
- `adf_lball_unit_mod(out, x, k)`: the unit part modulo `p^k` as `fmpz` (L11).
- `adf_lball_pow_si(y, x, k)`: smallest ball containing `{s^k}`; `k = 0` gives the exact 1; negative `k` through the same set
  as the inverse. Radius rule (L12, proved with Lemma 9, line 265): relative precision `rel' = rel + v_p(k) + e`, `e = 1` iff `p = 2`,
  `rel = 1` and `k` even. The extra `e` is a case I found while proving the statement: the squares of `1 + 2 Z_2` are `1 + 8 Z_2`,
  not `1 + 4 Z_2`; the enumeration confirms it.
- `adf_lball_set_fball` repaired (L13): `LIMIT` is now decided before `v_p(H)` is formed to the end.

Files written: `include/adelefeld/lball.h`, `src/lball.c` (pow_si, set_fball), `src/lball_decomp.c` (new), `tests/test_lball_decomp.c` (new),
`tests/test_lball.c` (two set_fball tests appended), `tests/julia/lball2.jl` (new), `tests/test_julia.sh` (calls of `lball2.jl` on both
success paths), `proto/functions_checks.py` (section "f-slice3" at the end, called from `__main__`),
`tests/ref/vectors/f-slice3/lball_slice3.jsonl` (3000 lines, 528888 bytes), `docs/api-1f.md` (section "Slice 1F.3-b" at the end),
and in `lanes/f-slice3/`: `gen_vectors.py`, `run_python_checks.py`, `bite.py`, `bite.log`, `redgreen.log`, `progress.md`,
`check-all.log`, `check-san.log`, `check-clang.log`, `check-inv.log`, `python_checks.log`, this file.

## Decisions (each with its alternative; full text in `docs/api-1f.md`, "Decisions taken in this slice")

1. `decompose_teich` is a new function; `decompose` is unchanged (rule: no change of a declaration). Alternative: a flag.
2. `prec < 1` is taken as 1 (a ball of precision `<= 0` around a unit would contain 0). Alternative: `DOMAIN`.
3. `w` is exact when rational (the set is one rational point). Alternative: always a ball of precision `n`.
4. For a ball `x` of relative precision `k` the principal unit `u` has precision `k`, whatever `prec`: it is the exact set
   `u_bar + p^k Z_p` (L10). Alternative `min(k, prec)`: rejected, larger than the set. For an exact `x` with irrational `w`, `u` has
   precision `prec`.
5. `p = 2`, relative precision 1 is `NOT_DETERMINED` (the unit ball `1 + 2 Z_2` holds units of both signs). Alternative: hull.
6. `x^0 = 1` exactly for every `x`; a ball around 0 to a power `k > 0` is `O(p^(kN))` (the set is not a ball, the result is the
   smallest ball); an exact power is limited to `ADF_LBALL_BITS_MAX` bits in numerator and denominator (`+-1` never), decided before
   the power is formed. This last limit is new and stated in the header only for `pow_si`.
7. `frac` of a ball with `N < 0` is `NOT_DETERMINED`.
8. `set_fball`: no bit-length argument alone can decide `LIMIT` (bit lengths bound `v_p(H)` from above; `LIMIT` needs it large). What
   I prove (L13): `v_p(H) <= (bits(H) - 1)/(bits(p) - 1)` (skips the test when the bound is small, and shows when the small-integer
   centre can still avoid `LIMIT`), then `LIMIT` iff `p^(v_p(A) + kmax + 1)` divides `H`, one divisibility test, preceded by the
   tests `p | H` and `p^64 | H`. Alternative: a faster full valuation for huge integers; out of scope.

## Red and green (`lanes/f-slice3/redgreen.log`)

- RED 1: header and tests, no source: link errors (`undefined reference to adf_lball_pow_si` and the other four).
- RED 2: stub `lball_decomp.c` (all five functions `ADF_UNSUPPORTED`): `23 tests, 153104 checks, 103633 failed checks, 23 failed tests`,
  every failure an assertion.
- RED 3: the new timing test against the old `set_fball`: `p = 3: LIMIT took 5.2 s (bound 3 s)` (bound later 3.5 s; old code 5.2 s in the test,
  6.1 s in a scratch program; the sball lane saw 13 s on its machine state).
- GREEN: first run of `test_lball_decomp` `459220 checks, 3 failed checks`: two defects of MY tests (an expected `LIMIT` for a centre `8` that is a
  small integer and correctly `OK`; a forged ball built from the ball around 0), repaired in the tests. Then 0 failed. The vectors (added last)
  failed 420 checks in the first run only because my generator wrote one integer as a string; repaired in the generator. The vectors
  found no fault of the C code. Final: `test_lball_decomp` `24 tests, 572484 checks, 0 failed checks`; `test_lball` `28 tests, 3982410 checks,
  0 failed checks`.
- The Julia test `lball2.jl` was written after the C code was green and was NOT run red. It runs green: `adf_lball slice 1F.3-b through ccall | 34 34`.

## What the tests check, and what would make a case fail

`tests/test_lball_decomp.c` (24 tests):
- Teichmueller (`teichmuller_against_the_limit_of_powers`, 2 tests): `p` = 2, 3, 5, 7, 11, residues to `4p`, precisions -3 to 1000, and
  `2^64 - 59`. Oracle: `r^(p^L) mod p^L` by `fmpz_powm` (the limit of powers), independent of the library's Newton method; also
  `w^(p-1) = 1 modulo p^n`, `w = r modulo p`, exact iff rational. Fails on a wrong sign or step of Newton (bite fault 1).
- Split of balls (`decompose_teich_p2/p3/p5/p7/p11`, `..._the_big_prime`): every ball `p^m (u + p^k Z_p)` of a universe (p = 2: k <= 5, all u;
  p = 3: k <= 3; p = 5: k <= 2 all and k = 3 sampled; p = 7, 11; m = -2..2), precisions 1, 3, 7. For `p + 2` points of the unit ball:
  the brute-force factor is `w`, the principal unit lies in the returned `u`, is 1 modulo `p` (modulo 4), and the `p` classes modulo `p^(k+1)` are all
  met (`u` not too large); `w * u` (library product) contains the unit ball and equals it for `prec >= k`; the residue `index`; aliasing of `w` and
  `u` with `x`.
- Split of exact values (`decompose_teich_exact_values`, 5 primes, all `a/b`, `m` -2, 0, 3, 3 precisions), statuses and limits
  (`decompose_teich_statuses_and_limits`: exact 0, ball with 0, `p = 2` with `k = 1` and `k = 2`, `LIMIT` for `1 + O(5^(2^40))`-type inputs
  with an irrational `w`, `OK` for the rational `w`, `N - m` above the bound, `prec` at the bit bound; outputs and `m`, `index` untouched).
- `frac_of_exact_values` (4 primes including `2^64 - 59`, valuations -4..2, definition of Proposition 19), `frac_of_balls` (every ball of
  the universe, points, constancy for `N >= 0`; for `N < 0` two points with different fractional parts), `frac_statuses_and_limits`.
- `unit_mod_against_the_congruences`, `pow_balls_p2/p3/p5/p7` (every ball of the universes, `k` in -9..27 (21 values), the exponent of the result
  equals the smallest valuation of a difference of two `k`-th powers of sampled points (tightness) and each power is inside),
  `pow_the_factor_v_p_of_k` (p = 3, 5, 7, k = 1..60, both signs), `pow_exact_values_and_k_zero`, `pow_limits` (hand-worked limit cases at the exponent and bit bounds),
  `pow_aliasing_and_composition` (inverse of `x^-k` is `x^k`; the power is inside the repeated product), `pow_the_big_prime...`.
- `vectors_slice3`: all 3000 lines: 360 `teich`, 720 `split`, 480 `frac`, 480 `unit_mod`, 960 `pow`, 6 primes including `2^64 - 59`; the reference
  is Python (`proto/functions_checks.py`, section f-slice3), whose Teichmueller function is cross-checked against the digit-by-digit lifting and
  Newton, and whose `pow`, split and frac are checked by enumeration (`python3 -B lanes/f-slice3/run_python_checks.py`, 0.35 s).
`tests/test_lball.c` (2 new tests): `set_fball_limit_is_decided_without_the_full_valuation` (`H = 6^(2^25 + 1)`, `A = H - 1`: `LIMIT` at 2 and 3,
`OK` at 5, time bound 3.5 s, red on the old code), `set_fball_small_centre_above_the_bound_is_ok` (`H = 2^(2^25 + 5)`, `A = 3`: OK, the small-integer
shortcut; `A = 1`, `d = 3`: `LIMIT`; at 3 the ball around 0). Time of the new `set_fball` at `p = 3` on this machine: 0.97 s, 1.75 s (two runs);
`p = 2`: 0.03 s; `p = 5`: 0.05 s.

## The tests bite (`lanes/f-slice3/bite.py`, `bite.log`): 12 of 12 caught

Faults planted one at a time in a scratch copy under `build/bite3/`, linked with the library, run against the test of that file:
Newton step sign (7 tests fail); sign at 2 from the wrong residue (4); `p = 2`, `k = 1` not refused (3); principal-unit precision `prec` instead of `k` (5); `p - 1`
not recognised as the exact `-1` in `decompose_teich` (1: only the vectors catch it; the enumeration checks the value, not the exactness); `frac` with `N = -1` called
determined (3); `unit_mod` accepting `k = N - v + 1` (2); `pow` without `v_p(k)` (7); `pow` without the extra digit at 2 (2); `pow` of a negative exponent using the centre (7);
`set_fball` divisibility test always yes (1); the small-centre shortcut (ii) not recognised (1). Four faults were required; twelve were run. Not a mutation run.

## Checks of the brief (each under `timeout 900`, run after the last change to a source, header or test; last line)

- `make clean && make -j2 check-all` (4 min 3 s): `check-all passed: make check, driver, exports, julia, mutate-selftest, memcheck-selftest`
  (`check passed: all 66 test programs`; `test_exports: passed: 387 of 387`; `test_julia: passed (with LD_PRELOAD=...)`). The first run of check-all failed in
  `tools/memcheck` (a finding on my test: an array `fmpz_t seen[16]` used in text order before its init); repaired in the test, rerun green.
- `make clean && make -j2 check SAN=1` (4 min 2 s): `check passed: all 66 test programs`; 0 lines with `runtime error`, `AddressSanitizer` or `LeakSanitizer`.
- `make clean && make -j2 check CC=clang` (2 min): `check passed: all 66 test programs`.
- `make clean && make -j2 check INV=1` (2 min 16 s): `check passed: all 66 test programs`.
- `sh lanes/m1-headers/check_headers.sh`: `check_headers: passed` (63 single-header compilations, 0 failures; `lball.h` declares 33 functions).
- `python3 -B proto/functions_checks.py`: exit 0; new lines `check_teichmueller_three_ways: cases=408`, `check_split_enumeration:
  balls_and_precisions=384`, `check_pow_enumeration: pow_cases=7011`, `check_frac_and_unit_mod: frac_cases=273`.
- `python3 -B lanes/f-slice3/gen_vectors.py` twice: identical bytes.

## What is not done

- No mutation run, no fuzz target (brief item 4). The 12 planted faults are not a substitute.
- `lball2.jl` not run red.
- The invariant-entry test of `test_lball.c` (`entry_check_of_every_public_function`) is not extended to the five new functions (the brief allows
  additions for `set_fball` only). They call `ADF_INV_LBALL` on `x` (except `teichmuller`, which reads no value); under `INV=1` all tests pass,
  but no case forges an input to the new functions.
- `adf_lball_decompose`'s header comment still says the second split "is not in this slice"; not changed (existing declaration).
- No test of `decompose_teich` or `teichmuller` at a precision between 1000 and the bit bound (22 million digits at `p = 5`), only the two limits.
  The early-`LIMIT` decision of `set_fball` is tested at three points of the `(p, H, A, d)` space (two `p` and the two branch cases); the boundary
  `k = kmax` itself is not tested (it needs a 33-million-bit modular inverse).
- Unit-part accessors: only `unit_mod`. No accessor for the residue as a separate function (`index` of `decompose_teich` gives it).
- Lines in the test files exceed 116 characters in places (header, sources and `api-1f.md` prose were wrapped to 116; the tables of `api-1f.md` and
  some test lines were not).
- `docs/conventions.md` 3.2 has no row for these functions (not my file).

## Findings against the specification

None. One thing to know: L12's `e` term (squares of `1 + 2 Z_2`) is not in `functions.md`; `api-1f.md` L12 proves it.

## Avoidable costs seen (not repaired)

- `set_fball` forms the valuation of `A` and `d` twice on the path where the early test does not return (once in `fball_limit_certain`, once in `lb_make`).
- `decompose_teich` computes `omega` modulo `p^max(n, k)` and reduces it twice; `pow_si` reduces `u^n` modulo `p^rel'` and `lb_make` reduces again
  (a modulo of an already reduced integer).
- `adf_lball_teichmuller` builds a temporary and swaps; a direct write would save one copy.
