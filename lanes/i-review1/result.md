# Lane i-review1: result

Worktree at adf-commit adf87c1 (`build/libadelefeld.a` built with `make -j2`, default flags, no sanitizer).
All programs are in `lanes/i-review1/`. Nothing outside that directory was changed.

## Defects

None found. I did not find an input for which the code returns something false.

Two observations that are not defects against the contract, stated so that nobody reads them as findings:

1. Huge precision. `adf_idele_inv` and `adf_idele_set_rat` at `prec = LONG_MAX` (9223372036854775807) on
   1/3 segfault (the division is inexact and FLINT tries to allocate 2^63 bits). `mul` at the same precision on
   exact or short inputs works. Reproduce: `python3 lanes/i-review1/each.py lanes/i-review1/edge.txt`, lines 3
   and 4 give `rc=-11`. Same behaviour as any arb call at that precision; the headers state no bound on `prec`.
2. Under valgrind the harnesses show 134,720 bytes "possibly lost" in 4,065 blocks, all under
   `fmpz_set_str` -> `_fmpz_promote` (FLINT's mpz pool, harness calls no `flint_cleanup`). "Definitely lost" is
   0 bytes and "indirectly lost" 0 in every run.

## What was run (commands, counts)

Build of the harnesses (from the worktree root):
`cc -Iinclude -std=c11 -O1 -g -w lanes/i-review1/kern.c build/libadelefeld.a -lflint -lgmp -lm -o lanes/i-review1/kern`
(same for `uc.c`, `misc.c`).

1. Real kernel, exact oracle in Python `fractions` (`fuzz_kernel.py`, harness `kern.c`). Oracle: rounded end
   points `l, h` per ball, `lo = RD_p(l l')`, `hi = RU_p(h h')` (or `1/h`, `1/l`; `|q|`), true sign; checks:
   the result ball contains the true set of the products/inverses (end points of `|x|-r`, `|x|+r`), excludes
   0, has the true sign; status `OK` exactly when `e(hi) - e(lo) <= p` (B1), `NOT_DETERMINED` exactly
   otherwise; output untouched on `NOT_DETERMINED` (aliases z=x, z=y, z=x=y included); `set_rat` never
   `NOT_DETERMINED`; midpoint bits `<= 2p + 30`.
   `python3 fuzz_kernel.py SEED N` with p in {1, 2, 3, 4, 8, 30, 64, 100, 4096}, mantissas up to 6000 bits,
   radii from 0 up to `|m|(1 - 2^-k)` for k up to 200:
   seeds 1, 2, 3, 4, 5 with N = 3000, 20000, 20000, 250000, 250000 = 543000 cases; OK 392036 (2156 + 14369 + 14456 + 180764 +
   180291), NOT_DETERMINED 150964; violations 0. Largest midpoint size
   relative to the bound: `bits - (2p + 30)` was -3 at p = 2, -7 at p = 4, -4050 at p = 4096 (never above 0).
   Note: a first run reported one "ND in OK zone"; that was my oracle using the true end points, whereas
   Statement E6 (`api-2.md:153`) is about the rounded ones. With the rounded ones there was no violation.
2. Exponents near +-2^60..2^63 (`scale.py`): 3000 cases, each run once with small exponents and once with the
   mid/radius exponents of both balls shifted by one of 14 shifts (2^60, -2^60, 2^62 +- small, 3 * 2^60,
   2^63 - 2^20, ...); result must be the same shifted, same status. 3000 cases, 1833 with `OK`, 0 differences,
   0 crashes.
3. Precision edge and exactness: `edge.txt` via `each.py` (prec -5, 0, LONG_MAX,
   radius = 2^30 - 1 in the mantissa): all as expected except observation 1. `exact.py`: 18134 cases (dyadic
   `q` with at most p bits; exact products that fit in p bits; exact power-of-2 inverses): every result exact
   (radius 0, midpoint equal to the true value), 0 failures. Inputs with radius >= |midpoint| are refused by
   `set_parts` (`DOMAIN`), tested in `misc.c` (radius 1 on midpoint 1, radius 2 on 1, radius inf, NaN midpoint, 0).
4. Unit cosets against enumeration in (Z/L)^* (`fuzz_uc.py`, harness `uc.c`; L = lcm of the moduli times a prime
   >= 5 not dividing it, so that a coset is never a single exact unit at that level; exact units as
   one-element sets): all canonical `(c, N)` for N <= 64 plus both exact units (`-1` included), non-normal moduli
   (N = 2 mod 4) included. `python3 fuzz_uc.py 64 3000 2 40000`: 153188 checks (contains, equal_set,
   overlaps, 23752 mul enumerations, inv, normalise), 0 wrong. `python3 fuzz_uc.py 64 1500 1 3000`: 21902
   checks, 0 wrong. Each `mul`, `inv`, `normalise` was also run with the output aliased to an input and
   with x = y = z; the harness prints `ALIASMISMATCH` on a difference (none) and `NOTNORMAL` if the result is
   not in normal form (none).
5. Raw data and big moduli (`big_uc.py`, 1069 checks, 0 wrong): `set_fmpz2` for 17 values of `c` (negative,
   zero, > N, +-(2^70 + 1)) and 17 values of `N` (-5, -1, 0, 1, 2, 3, 4, ..., 2^64, 2^64 + 1, 2 * 3^50, 10^30):
   status (`DOMAIN` = 7) and stored pair against Python; the aliased call (c and N members of x) gives the same;
   on `DOMAIN` x is untouched. 60 pairs of moduli of 2000, 5000 and 20000 bits: `mul`, `inv`, `normalise`,
   `contains`, `equal_set`, `overlaps` against `pow(c, -1, N)`, `gcd`; plus refinements `(c2, N k)` inside
   `(c, N)` and products with `[+-1]`.
6. States and predicates (`misc.c`, 0 failures): `set_parts` DOMAIN cases leave x untouched (0, containing-0 ball,
   inf radius, NaN, r = 0, r < 0, denominator 0), `6/4` stored as `3/2`, `-6/-4` accepted as 3/2;
   `is_canonical` and `is_normal` on garbage (zero denominator, negative denominator, N < 0, c = 0, c = 7 with
   N = 0, N = 1 with c = 0) return 0/1 without abort; `set_rat` on 0 gives `NOT_UNIT` and leaves x untouched;
   `set_rat(-7, p = 2)` contains -7, unit `[-1]`; `(-1) * (-1)` and `inv(-1)` exact units; `get_fmpz2` into
   the members of x.
7. Memory: `valgrind --leak-check=full` on `misc` (no report), on `kern` with `vg_kern.txt` (600 lines) and
   `uc` with `vg_uc.txt` (600 lines): 0 bytes definitely or indirectly lost, no invalid read/write, no use of
   uninitialised value; possibly-lost as in observation 2. No abort reached from valid input in any run
   (`kernel_defect`, `flint_abort` in `adf_ucoset_inv`): 0.
8. Tests that cannot fail (`mut.py`, hand-picked mutants compiled over `src/idele.c` or `src/ucoset.c` and run
   against `tests/test_idele.c` and `tests/test_ucoset.c`): 32 mutants (B1 comparison, every rounding direction,
   B3 max, B3/B4 branch, B4 halving, p floor 2 in three places, content, unit, sign, set_parts sign, normal form,
   reduce 0 to N, contains direction, overlaps and gcd with an exact unit, set_fmpz2 gcd and N < 0 checks,
   is_canonical range checks): 32 killed, 0 survived (one pattern "noop" was a placeholder and not applied).

## Tried without result

- Enclosure, sign, zero, status, untouched output, midpoint size: 543000 kernel cases, 0 defects.
- Scale: 3000 cases, 0 defects. Exactness: 18134, 0 defects.
- Cosets: 175090 enumeration/formula checks, 0 defects. Big moduli: 1069, 0.
- Aliasing: every combination for `mul` (z=x, z=y, z=x=y) in both idele and coset kernels, `inv`, `normalise`,
   `set_fmpz2` (c, N members of x), `get_fmpz2`.
- Headers: sentences on `NOT_DETERMINED`-only-B1, exact results, sign of `set_rat`, `set_parts` canonicalising
  r, untouched on non-OK: all held.

## Not done

- No run with `-DADF_CHECK_INVARIANTS` and no sanitizer build of my harnesses.
- No test of moduli of thousands of bits for the idele level (only unit cosets); no Julia binding checks.
- Statement A to E proofs were not re-read; only the code's behaviour was tested.
- Exponents above 2^63 (fmpz exponents beyond `slong`) cannot be fed through my harness (`slong` arguments).
