# Slice 1F.6: rational powers and powers of principal units at a prime

Lane f-slice9, 2026-10-02. The contract is the comment block of each declaration in `include/adelefeld/lpow.h` and
the two added declarations of `include/adelefeld/rfunc.h`. The implementation is `src/lpow.c`, with the named-place
forms in `src/rfunc.c` and the driver commands in `tools/adf/adf.c`. This document adds the statements P1 to P8, in
the style of `docs/api-1f5.md`, and P9 (lane f-repair5). It does not change SPEC 9.3.4.

Sources used, all on disk: `docs/proofs/functions.md` Lemma 3 (line 55), Proposition 4 (line 92), Lemma 9 (line
265), Proposition 11 (line 336), Proposition 13 (line 410), Proposition 15 (line 463), Proposition 17 (line 577),
Proposition 18 (line 616), Proposition 22 (line 725); `docs/api-1f.md` L3 (product of balls), L8 (predicates),
L12 (integer powers); `docs/api-1f4.md` F1 (domain test), F6 (precision of exp and Log); `docs/api-1f5.md` R1 to R6
(roots); `refs/src/flint-3.0.1/ulong_extras.rst:404` (`n_gcd`). Notation: `p` a prime, `v = v_p`, `c = 1` for odd
`p` and `c = 2` at 2, INF an absent term. The oracle is `proto/lpow_checks.py` (integers and fractions only).

## Interface and decisions

| Operation | Result |
|---|---|
| `adf_lball_powrat(y, x, e, n, seed, N)` | `x^(e/n)` on one branch: `e/n` reduced, the root of `x` named by `seed`, then the power |
| `adf_lball_powunit(y, u, s, N)` | `u^s = exp(s log u)`, at 2 `w^(s mod 2) exp(s log u')`; `u`, `s` balls at one prime |
| `adf_sball_powrat_at`, `adf_sball_powunit_at` | the same on the component at a named prime of partial balls |
| driver `powrat_at X with PRIME with E/N with SEED`, `powunit_at X with PRIME with S` | the same, one line |

1. Names. `powrat` and `powunit` at every level (library, `_at`, driver), so that the four operations of SPEC 9.3.4
   read apart: `pow_si` (integer), `powrat`, `powunit`, and later the quasi-character. Alternatives: `pow_q`,
   `pow_padic`; rejected as less plain about what the exponent is.
2. The rational exponent is a pair `slong e`, `ulong n`, as `adf_lball_pow_si` takes a `slong` and `lroot.h` a
   `ulong` degree. Alternative: an `fmpq` or `adf_rat`; it would reduce the fraction silently and hide the rule
   "reduce first", and it would need a `LIMIT` for a numerator beyond a word anyway.
3. The order is the root first, then the power, and `seed` is the identifier of the `n'`-th root of `x` in
   `lroot.h`. Alternative: the power first, with the identifier of the root of `x^e'`. For a reduced fraction both
   give the same set of values (P1); the root of `x` is the branch a reader of SPEC 9.3.3 has in hand, and the
   identifiers of `x` are what `roots_at` lists.
4. The degree `n' = 1` is `adf_lball_pow_si` exactly, `seed` and `N` ignored (P1 (d)). It follows from `lroot.h`
   (degree 1 is the identity and ignores seed and N) composed with L12, and the brief demands the agreement. So a
   ball around 0 with `e' < 0` and `n' = 1` is `UNIT_NOT_CERTIFIED` (the status of `pow_si`, as `inv`), not
   `NOT_DETERMINED`, and a ball result of `n' = 1` is the exact image, not capped at `N`. Alternative: `min(N, E)`
   and `NOT_DETERMINED` also for `n' = 1`; it would break the agreement with `pow_si`. Listed as a finding.
5. An exact `x` gives an exact result exactly when the selected root is a rational (detected by `lroot.h`); P1 (c)
   proves that no other exact case exists for a reduced fraction.
6. `powunit` takes the exponent as a local ball at the same prime (SPEC 9.3.4 item 3: "the uncertainty of `s` and
   of `log u` both enter"); the `_at` form takes it as a partial ball and uses its component at the place.
   Alternative: an exact rational exponent only; it could not carry the error term `B + alpha` of Proposition 18.
7. At 2, where the sign of `u` or the parity of `s` is not fixed, the result is the smallest ball containing the
   union of the images, which is `1 + 2 Z_2` (P5 (e)), or the exact image where the union is one coset (P5 (d)).
   Alternative: `NOT_DETERMINED` or `NEEDS_SPLIT`. Rejected: the domain is certain, the result encloses the image,
   and `pow_si` and `Log` (image `4 Z_2` at relative precision 1) already return a hull in the same situation.
8. Exact `u` and exact `s` give the ball at `N` (as `exp` of an exact input), except the exact 1 and, at 2, the
   exact `+-1` for `u = -1`. No rational value is detected, also not for an integer `s` (`6^2 = 36` is returned as
   `36 + O(5^N)`). Alternative: the exact `u^k` for an integer `s = k`; it is a second operation (`pow_si`) and
   would need a size bound for `k` that a ball does not need.
9. The order of the statuses of `powunit`: different primes `DOMAIN`, then the input bounds `LIMIT` (as `lball.h`),
   then the domain: `DOMAIN` if either coordinate is outside, before `NOT_DETERMINED` (P4).
10. `alpha = v(log(w0 u0))` is computed as `v(w0 u0 - 1)` (P6 (a)), not by `Log`: a `Log` to the precision
    `min(A, N - B)` was the first implementation and returned `LIMIT` for an exact base at `N = 2^60`
    (`lanes/f-slice9/redgreen.log`).
11. Driver: the exponent `E/N` is the value text of an exact rational, so it arrives reduced; the seed is required
    as for `root_at` (also for `N = 1`, where it is ignored), `-1` is accepted at 2. Alternative: a pair
    `E with N`; it would make the line five operands long and test nothing the library does not test.
12. (Lane f-repair5, 2026-10-03; review f-review7, finding 2.) A ball `u` with an exact integer exponent `k` that
    fits a `slong` is computed as `pow_si(u, k)`, plus `p^N Z_p` when `N < R` (P9). `(6 + 5^(2^40) Z_5)^1` at
    `N = 2^40` was `LIMIT` (the `Log` needs `5^(2^40)`); the image `6 + 5^(2^40) Z_5` is small and `pow_si` returns
    it. P9 proves that the two ways give the same ball, so no result that was `OK` changes; a `LIMIT` of `pow_si` or
    of the sum falls back to the general way. Alternative: keep the `LIMIT` and state it; rejected because the
    proof holds. An exact `u` keeps decision 8.

## P1 (the fraction, the order of root and power, the degree 1)

Let `e` be an integer, `n >= 1`, `g = gcd(|e|, n)`, `e' = e/g`, `n' = n/g` (`gcd(0, n) = n`, so `e = 0` gives
`0/1`). Let `x != 0` have an `n'`-th root in `Q_p`.
(a) The map `b -> b^e'` from the set of `n'`-th roots of `x` to the set of `n'`-th roots of `x^e'` is a bijection.
    So distinct branches of the root of `x` give distinct values `b^e'`, and the set of values is the same for the
    order "power, then root". For `g > 1` the map from the `n`-th roots of `x` need not be injective.
(b) For a root `b`, `b^e'` is a rational exactly when `b` is (for `x` rational).
(c) For `n' = 1` the function is `x -> x^e'`, which is `adf_lball_pow_si` (L12) as a set and as a status, for every
    `x` and every seed and `N`.
(d) The exact 0 has the one root 0 (seed 0); `0^e'` is the exact 0 for `e' > 0` and not defined for `e' < 0`. A ball
    `p^N Z_p` around 0 with `n' >= 2` meets the domain of the root and its complement.

Proof.

1. (a) `(b^e')^(n') = (b^n')^e' = x^e'`, so the map is into the roots of `x^e'`. If `b1^e' = b2^e'` with
   `b1^n' = b2^n' = x`, put `z = b1/b2`: `z^e' = z^n' = 1`. Bezout (Lemma 3 step 4) gives integers `a, k` with
   `a e' + k n' = gcd(e', n') = 1`, so `z = z^(a e' + k n') = 1`: the map is injective. `x^e'` has the root `b^e'`,
   so by Proposition 13 (line 410) both sets have `gcd(n', p - 1)` elements (odd `p`) or `gcd(n', 2)` (at 2): an
   injective map between finite sets of the same size is a bijection.
2. For `g > 1`: at 5, `x = 4`, `e/n = 2/2`: the roots are 2 and -2 (residues 2 and 3), both with square 4.
   Each seed still names one root and one powered value; different seeds can give the same value.
   More precisely, if a nonzero `x` has an `n`-th root, its roots are `b z` as `z` runs over the `n`-th roots
   of unity. This group is cyclic of order `d = gcd(n, p - 1)` at odd `p`, or `d = gcd(n, 2)` at 2.
   Write `z = z0^i`, `0 <= i < d`: powering by `e` has kernel `e i = 0` modulo `d`, of size `gcd(e, d)`.
   Thus distinct roots give distinct powered values exactly when `gcd(e, gcd(n, p - 1)) = 1`, with `p - 1`
   replaced by 2 at 2. Reduction is sufficient, not necessary: `3/3` at 5 on `x = 1` is injective.
   Reduction also makes the operation independent of the representation of the fraction.
3. (b) If `b^e' = q` is a rational, then `b = b^(a e' + k n') = q^a x^k` is a rational (`b != 0`). The converse is
   clear.
4. (c) `n' = 1`: the root of degree 1 is the identity (`lroot.h`; Proposition 13 step 4: "At n=1 each condition is
   automatic and the construction gives a"), and the power is L12. The code calls `adf_lball_pow_si(y, x, e')`.
5. (d) Proposition 13 (line 420): "For a=0 there is exactly one root, 0". `0^e' = 0` for `e' > 0`; for `e' < 0`
   it is `1/0^|e'|`, not defined (Proposition 17, line 582: "A negative exponent excludes zero"), `NOT_UNIT` as for
   `inv`. For `e' > 0`, zero witnesses the domain. For `e' < 0`, choose an integer `l` with `n' l >= N`:
   the nonzero point `p^(n' l)` lies in the ball, has root `p^l`, and admits its negative power.
   The ball also contains `p^k` for every `k >= N`; for `n' >= 2` some such `k` is not divisible by `n'`,
   and `p^k` has no root (Proposition 13: `n` must divide `m`). So it meets the domain
   and its complement: `NOT_DETERMINED` (conventions 3.1). `lroot.h` returns that status, and `powrat` returns it.

Check: tests `degree_one_is_pow_si_and_numerator_one_is_root` (35360 calls, identical fields and statuses with
`pow_si` and with `root_seed`), `powrat_zero_and_hand_values`; `powrat_exact_rows` checks `r^n' = x^e'` for every
exact result.

## P2 (the image of a branch of a rational power)

Let `n' >= 2`, `gcd(e', n') = 1`, `x = a + p^M Z_p` a ball with `m = v(a) < M`, `r = M - m >= c + s`,
`s = v_p(n')`, `n'` dividing `m`, `j = m/n'`, and let the branch `seed` of the root exist (R1). Let `b` be the root
of `a` on that branch. The image of `x` under `t -> (root of t on the branch)^e'` is exactly

    b^e' + p^E' Z_p,   E' = e' j + (M - m) - v_p(n') + v_p(e').

At most one of `v_p(n')`, `v_p(e')` is positive. The result of `powrat` is the ball of exponent `K = min(N, E')`
that contains the image: the image when `N >= E'`, else the unique ball of exponent `N` containing it, which is
`p^N Z_p` when `N <= e' j`.

Proof.

1. R2 (`docs/api-1f5.md`): the image of the branch of the root is `b + p^E Z_p`, `E = j + r - s`, so it is the ball
   `p^j (beta + p^(r - s) Z_p)`, `beta = b / p^j` a unit, of relative precision `rel = r - s >= c`.
2. L12 (`lball.h`, `adf_lball_pow_si`): for a ball `p^j (beta + p^rel Z_p)` not containing 0 and `k = e' != 0` the
   set `{t^k}` is the ball `p^(k j) (beta^k + p^rel' Z_p)`, `rel' = rel + v_p(k) + eps`, `eps = 1` only if `p = 2`,
   `rel = 1` and `k` even; here `rel >= c = 2` at 2, so `eps = 0`. L12 states that this set IS a ball when 0 is not
   in the input, also for `k < 0` (the inverse keeps the relative precision, L4).
3. So the image of the composite is `p^(e' j) (beta^e' + p^(r - s + v(e')) Z_p) = b^e' + p^E' Z_p` with
   `E' = e' j + r - s + v_p(e')`, and `r = M - m`. If `v_p(n') > 0` and `v_p(e') > 0` then `p` divides
   `gcd(e', n') = 1`, which is false.
4. The same exponent follows from Lemma 9: every point is `a(1 + h)`, `h` in `p^r Z_p`; its value is
   `b^e' exp(e' log(1 + h)/n')`, and `e' log(1 + h)/n'` runs over `p^(r - s + v(e')) Z_p`, which lies in `p^c Z_p`.
   This is a second derivation, not used by the code.
5. The image is a ball, so it is the smallest ball containing itself. For `N < E'`, a ball of exponent `N` is
   determined by any one of its points (Definition 1, line 17), so exactly one contains the image. Every image
   point has valuation `e' j`; for `N <= e' j` it lies in `p^N Z_p` (R2 step 6 for the root).
6. An exact `x` with an irrational root on the branch: the result is the ball of exponent `N` that contains the
   point `b^e'`, by the same argument with a single point.

Check: `powrat_grid_balls` (6930 rows, 19152 branch rows in four scalings, 260496 calls; the oracle compares the
image as a set modulo `p^(Erel+1)` for 4788 unscaled rows, so a centre or an exponent that is off by one digit
fails), `powrat_exact_rows` (3024 rows, 18090 calls).

## P3 (the working precision of the composition)

Let `K = min(N, E')` (ball) or `K = N` (exact `x`, irrational branch), `K > e' j`. Ask the root at the absolute
precision `Nr = j + rel_r`, `rel_r = max(1, K - e' j - v_p(e'))`, and take `P = pow_si(root, e')`. Then `P` contains
the image (the point, for an exact `x`), its relative precision is at least `K - e' j`, and the ball of exponent `K`
with the unit centre `P` modulo `p^(K - e' j)` and valuation `e' j` is the result of P2.

Proof.

1. For a ball `x`, `K <= E'` gives `rel_r <= max(1, E' - e' j - v(e')) = r - s` (as `r - s >= 1`). `root_seed`
   returns the exponent `min(Nr, E) = j + rel_r` (N-D14, R2 step 6, R4): the ball of relative precision `rel_r` that
   contains the root image (for an exact `x`: the root `b`), centre `b` modulo `p^(j + rel_r)`.
2. L12 maps it to a ball of relative precision `rel' = rel_r + v(e') + eps >= rel_r + v(e')`, which contains
   `t^e'` for every `t` in it, so it contains the image of P2 (every root of a point of `x` on the branch lies in
   the root ball).
3. If `rel_r = K - e' j - v(e') >= 1`, `rel' >= K - e' j`. Otherwise `rel_r = 1 > K - e' j - v(e')`, and
   `rel' >= 1 + v(e') > K - e' j`. A ball of exponent `e' j + rel' >= K` that contains the image lies inside the
   ball of exponent `K` that contains the image; its unit centre modulo `p^(K - e' j)` is that ball's unit centre.
4. When `P` has exponent exactly `K` it is that ball and is returned as it is; when its unit centre is 1, the
   centre modulo every power is 1. Neither forms a power (decision 10's companion: `1 + 5^(2^60) Z_5` to the 1/2 is
   `OK` at `N = 2^60`, test `powrat_limits`).
5. For `N >= E'`, `rel_r = r - s`, the root ball is the root image (R2), and `P` is the image itself by L12.
6. For `K <= e' j` the root is asked at `j + 1` only for its status and for a rational branch; the result is
   `p^K Z_p` (P2 step 5) or the exact power, and no other power is formed.

Check: the coarse rows of `powrat_grid_balls` (`N = E'`, `E' - 1`, `E' - 3`, `e' j`, `e' j - 2`), and the
large-prime rows of `powrat_large_prime_and_big_inputs` (`y^n'` contains `x^e'`, unit residue `seed^e'`).

## P4 (the domain of the principal-unit power and its statuses)

The domain is `D = (1 + p Z_p) x Z_p`: `u` in `1 + p Z_p` (at 2 the odd units, at odd `p` the principal units),
`s` in `Z_p` (Proposition 17, lines 584 and 587). With the canonical balls `D1 = 1 + p^1 Z_p` and
`Zp = 0 + p^0 Z_p`: the status is `DOMAIN` if `u` does not meet `D1` or `s` does not meet `Zp`; else
`NOT_DETERMINED` if `u` is not inside `D1` or `s` is not inside `Zp`; else the value. A `u` inside `D1` is a ball
with `v = 0` and `N >= 1` or an exact unit; an `s` inside `Zp` has `N >= 0` and a centre of valuation `>= 0`.

Proof.

1. The input set is the product `u x s`. It misses `D` exactly when one factor misses its coordinate domain; it is
   inside `D` exactly when both are inside. Otherwise it contains a point of `D` (one point of each meeting factor)
   and a point outside (a factor point outside its domain, with any point of the other). This is the rule of
   conventions 3.1.
2. `D1` and `Zp` are canonical balls; L8 (`adf_lball_contains`, `adf_lball_overlaps`) decides "inside" and "meets"
   for exact values and balls (F1 of `api-1f4.md` uses the same test). At 2, `1 + 2 Z_2` is the set of odd units,
   the domain of `w^(s mod 2) exp(s log u')`.
3. A canonical ball inside `1 + p Z_p` has a unit centre (valuation 0, `v = 0`) and `N >= 1`; a canonical ball
   inside `Z_p` contains 0 with `N >= 0` or has a centre of valuation `v >= 0`.

Check: `powunit_statuses_and_limits` (each case with its reason), `pow-status.cmd`.

## P5 (the value of the principal-unit power)

Let `(u, s)` lie in `D` (P4); `u0`, `A` the centre and exponent of `u` (`A = INF` exact), `s0`, `B` those of `s`,
`beta = v(s0)` (INF for `s0 = 0`); at 2 `w0 = u0` modulo 4 when `A >= 2` or `u` is exact.
(a) `s` the exact 0, or `u` the exact 1: the value is the exact 1.
(b) `p = 2`, `u` the exact `-1`: the value is the exact `(-1)^(s0 mod 2)` when `s` is exact or `B >= 1`; for `B = 0`
    the values are `{1, -1}`, whose smallest ball is `1 + 2 Z_2`.
(c) Odd `p`; or `p = 2` with `w0` fixed and (`w0 = 1`, or `s` exact, or `B >= 1`): with `ell = log(w0 u0)` (`w0 = 1`
    at odd `p`), `alpha = v(ell)`, `R = min(A + beta, B + alpha, A + B)` (terms with INF absent), the image is
    exactly `w0^(s0 mod 2) exp(s0 ell) + p^R Z_p`; both exact: the single point.
(d) `p = 2`, `A = 1`, and `s` even (`s` exact and even, or `B >= 1` and `s0` even): the image is exactly
    `1 + 2^R Z_2`, `R = 2 + min(beta, B)`.
(e) `p = 2` and otherwise (`A = 1` with `s0` odd or `B = 0`; `w0 = -1` with `B = 0`): the smallest ball
    containing the image is `1 + 2 Z_2`. It equals the image when `A = 1`; it is strictly larger when `w0 = -1` and
    `B = 0` (the class `5 + 8 Z_2` is missed).

Proof.

1. (a) `u^0 = w^0 exp(0) = 1` for every `u`; `1^s = exp(s log 1) = exp(0) = 1` (Proposition 18, last sentence:
   "If an exact factor of the log product is zero, the output is the exact 1").
2. (b) `-1 = w u'` with `w = -1`, `u' = 1` (Proposition 4 step 2), so `(-1)^s = (-1)^(s mod 2) exp(s log 1)`. With
   `B >= 1` (or `s` exact) every `s` in the ball has the parity of `s0` (the ball is `s0 + 2^B Z_2`, `B >= 1`). With
   `B = 0` the ball is `Z_2` and contains 0 and 1. `v(1 - (-1)) = 1`, so no ball of exponent 2 holds both, and
   `1 + 2 Z_2` does.
3. (c) The principal parts: at odd `p` every `u` is principal. At 2 with `w0` fixed (`A >= 2`: every point is
   `u0` modulo 4), `t -> w0 t` maps `u0 + 2^A Z_2` onto `w0 u0 + 2^A Z_2`, inside `1 + 4 Z_2`, keeping `A`. The sign
   factor `w0^(s mod 2)` is constant on the exponent ball: `w0 = 1`, or all `s` have the parity of `s0`. So the
   image is the constant `w0^(s0 mod 2)` times the set of Proposition 18 (line 616) for the principal ball
   `w0 u0 + p^A Z_p` and the exponent ball, which is `exp(s0 ell) + p^R Z_p`. Proposition 18 needs `A >= c`: at
   odd `p`, `A >= 1 = c` (P4); at 2, `A >= 2 = c`. Multiplication by the unit `+-1` maps a ball onto a ball of the
   same exponent.
4. (d) `u` runs over all odd units, `u = w u'` with `u'` over all of `1 + 4 Z_2`; `s` even makes the sign factor 1.
   The set is `{exp(s l) : s in the exponent ball, l in log(1 + 4 Z_2) = 4 Z_2}` (Lemma 9). This is Proposition 18
   with `u0' = 1` (`alpha = INF`), `A' = 2`: `R = min(2 + beta, 2 + B)`.
5. (e) The image lies in the odd units, `1 + 2 Z_2`. It contains a point that is 1 and a point that is 3 modulo 4:
   for `A = 1`: `u = 1` gives 1 for every `s`; `u = -1` with an odd `s` (`s0` when odd, or `s = 1` when `B = 0`)
   gives `-1`. For `w0 = -1`, `B = 0`: `s = 0` gives 1, `s = 1` gives `u0 = 3` modulo 4. Two points at distance
   `2^1` force the exponent 1, and `1 + 2 Z_2` contains the image: it is the smallest ball.
6. Equality for `A = 1`: with an odd `s1` in the exponent ball, the values `w exp(s1 log u')` for `w = +-1` and
   `u'` in `1 + 4 Z_2` are `+-exp(s1 4 Z_2) = +-(1 + 4 Z_2)` (`s1` a unit, Lemma 9), whose union is `1 + 2 Z_2`.
7. Strictness for `w0 = -1`, `B = 0`: an even `s` gives `exp(s l)` with `v(s l) >= 1 + 2`, a value in `1 + 8 Z_2`;
   an odd `s` gives `-exp(s l)`, a value that is 3 modulo 4. No value is 5 modulo 8.
8. The function returns the ball of exponent `K = min(N, R)` (`min(N, 1)` in (e)) containing the image, as P2
   step 5.

Check: `powunit_rows` (4352 rows: 305 exact, 276 exact-input balls, 3748 images, 23 hulls; 28721 calls, each image
compared as a set modulo `p^(R+1)` by the oracle; the hull rows checked modulo 16 for equality (A = 1) or a missed
class (w0 = -1, B = 0)), `pow-values.cmd`.

## P6 (alpha, the centre and the working precisions of the principal-unit power)

In case (c) of P5, with `K = min(N, R)` (or `N` for two exact inputs):
(a) `alpha = v(w0 u0 - 1)` (INF for `w0 u0 = 1`).
(b) If `K <= c` or `s0 = 0`, the centre is `w0^(s0 mod 2)` modulo `p^K`.
(c) Otherwise let `P = max(K - beta, c)`. `Log(u0)` at the precision `P` is a ball `L` of exponent `P` containing
    `ell`, inside `p^c Z_p`; `s0 L` (L3) has the exponent `P + beta >= K` and lies in `p^c Z_p`; `exp` of it at `K`
    is the ball of exponent `K` containing `exp(s0 ell)`. With the sign and the conversion of an exact 1 to the
    ball at `K` (an exact plus the ball `p^K Z_p`, L2), the result of P5 (c) is obtained.

Proof.

1. (a) `w0 u0` lies in `1 + p^c Z_p` (P5 step 3) and `ell = log(w0 u0)`. Lemma 9 item 5: for `z` in `p^r Z_p`,
   `r >= c`, `v(log(1 + z)) = v(z)`. So `alpha = v(w0 u0 - 1)`. `Log(u0) = log(w0 u0)` (Proposition 11 step 3: `Log`
   kills `w`).
2. (b) `s0 ell` lies in `p^c Z_p` (`v(s0) >= 0`, `alpha >= c`), and Lemma 9 item 5 gives `v(exp(s0 ell) - 1) >= c`,
   so the value is `w0^(s0 mod 2)` modulo `p^c`, hence modulo `p^K` for `K <= c`. For `s0 = 0` it is exactly that.
3. (c) `lfunc.h`: the `Log` of an exact input at `P` is the ball `Log(u0) + p^P Z_p` (F6). `P >= c` and
   `ell` in `p^c Z_p`, so `L` lies in `p^c Z_p`. L3 for an exact factor of valuation `beta`: the product ball has
   the exponent `P + beta` and lies in `p^c Z_p` (`s0` in `Z_p`). `exp` at `K` returns the exponent
   `min(K, P + beta) = K` (F6: `K = min(N, E)`, `E = P + beta`) and contains `exp` of every point, in particular
   `exp(s0 ell)`. A ball of exponent `K` containing the point is the ball of P2 step 5.
4. The working precisions: `P <= max(K, c)` and `K <= N`; no step uses a precision above `max(K, c) + beta`.
   For a ball input `K <= R <= A + B` is bounded by the input; for two exact inputs by `N`.

Check: `powunit_rows` (all coarse `N`), `powunit_identities` (964 groups: `u^(s+t) = u^s u^t`, `(uv)^s = u^s v^s`
as equal sets at `N = 30`, or 200 at `p = 2^64 - 59`; `u^k` contains `pow_si(u, k)`, `k = -5..5`; `(u^n)^(1/n)`
contains `u` and the rational branch of `powrat` is `u`; 3000-bit `u`, `s` at 3: `u^s u^(1-s)` contains `u` at 40).

## P7 (what the oracle proves at its stated precision)

(a) Dominance. For a unit `y`, `t` in `Z_p`, `e >= c`, `k >= 1` with `s = v_p(k)`:
    `v((y + p^e t)^k - y^k) >= s + e`, with equality when `t` is a unit. Hence the unit roots `beta` of a unit ball
    `U + p^r Z_p` (`r >= c + s`), taken modulo `p^h` with `h >= c` and `h + s >= r`, are exactly the classes
    `{beta mod p^h : beta^n' = U mod p^r}`; and `beta` modulo `p^h` fixes `beta^e'` modulo `p^(h + v(e'))`.
(b) For `u` in the domain and `s, s'` in `Z_p` with `s = s'` modulo `p^k`, `k >= max(H - c, 1)`: `u^s = u^s'`
    modulo `p^H`; and `u^s` modulo `p^H` depends on `u` modulo `p^H` only. Integer exponents are the `p`-adic powers
    (Proposition 17). So `{u^s mod p^H}` over the balls is the finite set the oracle enumerates, with
    `k = max(H - 1, 1)` and `H' = max(0, k - B)` digits of the exponent.
(c) Scaling. `x -> p^(n' j) x` maps the roots `b` of a branch to `p^j b` with the same identifier, and the image of
    P2 to `p^(e' j)` times it: `E'` grows by `e' j`, the guard (relative precision) is unchanged.
(d) A branch `t` of the root of a unit `U` exists exactly when some unit `beta = t` (modulo `p`, or 4 at 2) has
    `beta^n' = U` modulo `p^(c + s)`.

Proof.

1. (a) `(y + p^e t)^k - y^k = sum_(i>=1) C(k, i) y^(k-i) p^(i e) t^i`. `C(k, i) = (k/i) C(k-1, i-1)` has valuation
   `>= s - v(i)`, so the `i`-th term has valuation `>= s - v(i) + i e`. For `i >= 2`, `(i - 1) e > v(i)`: at odd
   `p`, `e >= 1` and `p^v(i) <= i` gives `v(i) <= log_3(i) < i - 1`; at 2, `e >= 2` and `v(i) <= log_2(i) < 2(i-1)`.
   So the terms `i >= 2` have valuation `> s + e`, and the term `i = 1`, `k y^(k-1) p^e t`, has valuation
   `s + e + v(t)`. The root classes: a class `beta mod p^h` with `beta^n' = U mod p^r` has every member a root of a
   point of the ball (its own `n'`-th power is `U` modulo `p^r`, since `h + s >= r`); conversely a root reduces to
   such a class. The power `e'`: the same estimate with `k = |e'|`, and inversion of units preserves the congruence.
2. (b) `u^s / u^s' = u^(s - s')` with `s - s'` in `p^k Z_p`: at odd `p` it is `exp((s - s') log u)` with
   `v >= k + c`; at 2, `k >= 1` makes `s - s'` even, the sign factor is 1, and `exp((s - s') log u') - 1` has
   valuation `>= k + 2` (Lemma 9 item 5). So the quotient is in `1 + p^(k + c) Z_p`, inside `1 + p^H Z_p`. For
   `u = u~` modulo `p^H` (`H >= c`), `(u~/u)^s` lies in `1 + p^H Z_p` by the same estimate. Integer `s >= 0`:
   Proposition 17 ("agrees with every integer power"). At 2 and `H = 1`, every power of an odd unit is odd:
   `w^(s mod 2)` and the principal exponential are both units, so the value is always 1 modulo 2.
   Thus the omitted case `H < c` is constant, and the base congruence assertion holds there too.
3. (c) `(p^j b)^(n') = p^(n' j) b^n'`; the identifier is the unit residue of `b`, unchanged; `(p^j b)^e' =
   p^(e' j) b^e'`, and `M - m` is unchanged by the scaling.
4. (d) A root `rho` on the branch satisfies it with `beta = rho`. Conversely, if `beta^n' = U` modulo `p^(c + s)`,
   then `U / beta^n'` lies in `1 + p^(c+s) Z_p`; its `log` has valuation `>= c + s` (Lemma 9 item 5), `log/n'` lies
   in `p^c Z_p`, and `z = exp(log(U / beta^n') / n')` is a principal unit with `z^n' = U / beta^n'` (Lemma 9). Then
   `rho = beta z` is a root with `rho = beta` modulo `p^c`: the same identifier.

The fixture totals: `powrat_balls.jsonl` 6930 rows (4788 image witnesses), `powrat_exact.jsonl` 3024 rows,
`powunit.jsonl` 4352 rows (3748 image witnesses, 23 hulls), 909424 bytes together; generated in 1.6 s by
`python3 -B proto/lpow_checks.py`. The finite equality is not a substitute for P2 and P5; it checks them on the
grid.

Lane f-repair6 adds fixtures under `tests/ref/vectors/f-repair6/`, generated by
`timeout 30 python3 -B proto/lpow_checks.py --repair6`:

- `powunit.jsonl`: 30 rows at 3, 5, 7, 65537 and `2^64 - 59`, two per uniquely minimal term of `R`.
  Comparison precision is `H = R + 1`, from 2 to 5. The 18 small-prime rows compare the whole image set.
  The 12 large-prime rows certify two integer input pairs and their powers modulo `p^H`.
  Their difference has valuation exactly `R`; at least one misses the ball at `R + 1` with the returned
  centre. These certify tightness, not finite-set equality: even enumeration over `65537^2` is too large.
  The infinite image equality is P5; `repair6_powunit_integer_witnesses` checks the expected ball,
  the witness equations, input membership, output membership and aliases at `N = R - 1, R, R + 1`.
- `powrat.jsonl`: 144 ball rows at 65537 and `2^64 - 59`, `n' = 2, 3, 4, 5`, relative precision `K = 2, 3, 4`.
  Integer digit lifting starts at `seed^e'` and solves `t^n' = x^e'` modulo `p^K` directly.
  If `y` is correct modulo `p^k`, the next digit `d` solves
  `n' y^(n'-1) d = (x^e' - y^n') / p^k` modulo `p`; the derivative is a unit in these rows.
  This gives exactly one next digit, and the oracle checks the final equation and the seed again.
  `repair6_powrat_hensel_centres` checks every field and alias against these independent centres.
  `repair6_power_certificates_at` runs all 174 power rows through the named-prime wrappers.

The endpoint arguments of P1, P7 and P8 are also checked in `lanes/f-repair6/proof_checks.py`.

## P8 (limits, transactions, aliasing and the named-place forms)

1. Input bounds (`|v|, |N| <= 2^60`) are tested first (after the prime test of `powunit`). Every exponent sum is
   formed after clamping to `[-2^62, 2^62]` (`INF = 2^62` absorbs), and `e' j` with a saturating product
   (`|j| <= 2^60`, `|e'| <= 2^63`, including `e = LONG_MIN`), so no `slong` overflows;
   `K` is compared with the bound before it is used, and
   `e' j` before it is stored as a valuation.
2. `LIMIT` from `lroot.h`, `lfunc.h` and `lball.h` passes through unchanged: those functions test the powers they
   form. The results known without a power form none: the exact results, `p^K Z_p`, a centre 1 (`K <= c`, `s0 = 0`,
   a unit 1 of `pow_si`), the ball at `K` returned as it is (P3 step 4). One exception (P9): a `LIMIT` of `pow_si`
   or of the sum on the integer-exponent way of `powunit` is not returned; the general way is tried after it.
3. Every result is built in temporaries and copied to `y` only on `OK`; so `y` may be `x`, `u`, `s` or both, and
   `u` and `s` may be one object (read as two independent sets).
4. The `_at` forms (the pattern of R6 step 5): membership of the place in `x` (and in `s`) first (`DOMAIN`,
   `where = v`), the real place `UNSUPPORTED`, then the components are copied, the `lpow.h` function applied, and
   the partial ball over `v` alone built (Proposition 22, line 725); a failure leaves `y` untouched and sets
   `where = v`, success leaves `where` untouched.

Check: `powrat_limits`, `powunit_statuses_and_limits`, the aliasing in every `check_powrat`/`check_powunit` call
(`y = x`; `y = u`; `y = s`; `u = s`), `powers_at_prime` in `tests/test_rfunc_prime.c`.

## P9 (an exact integer exponent: the result of `pow_si`)

Lane f-repair5, 2026-10-03 (decision 12). Let `u` be a ball inside `1 + p Z_p` (P4: `v = 0`, exponent `A >= 1`,
centre `u0` an integer prime to `p`) and `s` the exact integer `k != 0` fitting a `slong`
(`|k| <= 2^63`, including `LONG_MIN`). Let `I = pow_si(u, k)`.
(a) `I` is the set `{t^k : t in u}`; it is the ball `u0^k + p^R' Z_p`, `R' = A + v_p(k) + e`, `e = 1` exactly when
    `p = 2`, `A = 1` and `k` is even (L12, `docs/api-1f.md:497`).
(b) The image of P5 for `(u, s)` is the same set, and its exponent `R` (P5 (c), (d), (e)) equals `R'`.
(c) The result of `powunit` (P5 step 8: the ball of exponent `K = min(N, R)` that contains the image) is `I` when
    `N >= R`, and `I + p^N Z_p` (the sum of L2, the smallest ball containing the sums) when `N < R`.
(d) If the way through `pow_si` and the general way (P5, P6) both return `OK`, the two results have identical
    fields. If the way through `pow_si` returns another status, it is `LIMIT`.
(e) Hence routing the exact integer exponents of a ball `u` through `pow_si`, and falling back to the general way
    after its `LIMIT`, changes no result that was `OK`, keeps every other status, and turns a `LIMIT` into `OK`
    exactly when `pow_si` (and the sum) can form the result.

Proof.

1. (a) is L12 for `x = u`, `v = 0`, `rel = A`, `n = |k|`; for `k < 0` the set of inverses keeps `rel'` (L12, last
   part). `u` does not contain 0 (`v(u0) = 0 < A`), so the set is a ball, not only enclosed by one.
2. (b), the set. Proposition 17 (`docs/proofs/functions.md:585`): `u^s` "agrees with every integer power", at odd
   `p` and at 2 (where `u^s = w^(s mod 2) exp(s log u')`). So for the single exponent `k` the image of P5 is
   `{t^k : t in u}`, the set of (a).
3. (b), the exponent, case by case (`B = INF` because `s` is exact, `beta = v_p(k)`, finite as `k != 0`):
   - odd `p`: P5 (c), `R = A + beta = A + v_p(k)` (the terms with `B` are absent); L12: `e = 0`, `R' = A +
     v_p(k)`.
   - `p = 2`, `A >= 2`: `w0` is fixed and `s` exact, so P5 (c) applies, `R = A + v_2(k)`; L12: `rel = A >= 2`,
     `e = 0`, `R' = A + v_2(k)`. The sign factor: P5's centre is `w0^(k mod 2) exp(k ell)` with `ell = log(w0 u0)`;
     `u0^k = w0^k (w0 u0)^k = w0^(k mod 2) exp(k ell)` (`w0 = +-1`; `(w0 u0)^k = exp(k log(w0 u0))` for the
     principal unit `w0 u0`, Lemma 9), so the point `u0^k` of `I` is P5's centre itself.
   - `p = 2`, `A = 1`, `k` even: P5 (d), `R = 2 + min(beta, B) = 2 + v_2(k)`; L12: `e = 1`, `R' = 1 + v_2(k) + 1`.
   - `p = 2`, `A = 1`, `k` odd: P5 (e), the hull `1 + 2 Z_2` is the image (equality for `A = 1`), exponent 1;
     L12: `e = 0`, `R' = 1 + 0 = 1`.
   The case `w0 = -1`, `B = 0` of P5 (e) does not occur: `s` is exact. In every case the two sets are equal by
   step 2, so their exponents agree also without this list; the list checks it against both statements.
4. (c) For `N >= R`, `K = R` and the result is the image, which is `I`. For `N < R`, `I` lies in the ball of
   exponent `N` around any of its points, which is the unique ball of exponent `N` containing the image (P2 step
   5); the sums `{a + b : a in I, b in p^N Z_p}` are exactly that ball, and `adf_lball_add` returns the smallest
   ball containing the sums (L2), so it returns that ball (for `N <= 0` the ball `p^N Z_p` around 0, as `I` lies in
   `Z_p`). `N` below `-2^60` is `LIMIT` on both ways (P8 step 1).
5. (d) A canonical value is determined by its set (`lball.h`: the centre of a ball is the unique element of
   `Z[1/p]` in `[0, p^N)` in the ball, with `v` the valuation of its points). Both ways return the canonical ball of
   (c), so their fields are identical. `pow_si` of a ball not containing 0 returns `OK` or `LIMIT` (`lball.h`:
   `NOT_UNIT` and `UNIT_NOT_CERTIFIED` need 0 in the input); `add` of two balls at one prime returns `OK` or
   `LIMIT`.
6. (e) The domain and the exact 1 are decided before the route (`src/lpow.c`), so `DOMAIN`, `NOT_DETERMINED` and the
   exact 1 are unchanged. After the route returns `OK` the result is the one of (c); after `LIMIT` the general way
   runs as before P9. By (d), a result that the general way returned `OK` is returned unchanged; a former `LIMIT`
   becomes `OK` exactly when the route succeeds.
7. Not routed: an exact `u` (decision 8: the ball at `N`, while `pow_si` gives the exact power, a different value),
   `s = 0` (the exact 1 first, as `pow_si(u, 0)`), a ball `s`, an `s` that is not an integer, or one beyond a
   `slong` (`|s| >= p^v >= 2^v`, so `v >= 64` is beyond it and `p^v` is formed only for `v < 64`).

Check: `powunit_integer_exponent_is_pow_si` in `tests/test_lpow.c`: the three inputs of review f-review7
(`lanes/f-review7/limits.in` lines 2 to 4) and four more (`N = 2^40 - 1`; `3 + 2^(2^30) Z_2` to the 3 and 2) red on
the code before P9 (`LIMIT`) and green after, `k = -1` still `LIMIT`; a grid of 41723 calls (`p = 2, 3, 5, 7,
65537`, every centre of `1 + p Z_p` modulo `p^A`, `A <= 4`, `k = -7..7`, nine `N`) identical to the coarse ball of
`pow_si` on the code before P9 and after. `lanes/f-repair5/diff_powunit.py`: 10000 random inputs through the old and
the new library, every old `OK` identical, every old `LIMIT` that became `OK` equal to the coarse ball of `pow_si`.

## Scope

P1 to P9 are proved here (P9 by lane f-repair5). The analytic facts rely on the proofs listed at the top. Not done:
the real place of both operations (`UNSUPPORTED`), all-places forms (1F.8), the quasi-character (milestone 3). No
new external source is pending; the naming sources already pending in `functions.md` (Teichmueller representative,
Iwasawa `Log`) remain pending there.
