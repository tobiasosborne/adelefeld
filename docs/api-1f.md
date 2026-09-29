# adelefeld: the public C interface of milestone 1F (functions)

This file is written section by section, one section for each slice of milestone 1F. Other lanes append their own
sections; a slice does not change the sections of another. The contract of a function is the comment block above its
declaration in the header; this file lists the functions, states what the header cites, and proves it.

## Slice 1F.3-a: `adf_lball`, ball arithmetic and decomposition (lane f-slice1, 2026-09-29)

Header: `include/adelefeld/lball.h`. Implementation: `src/lball.c`. Tests: `tests/test_lball.c` (vectors
`tests/ref/vectors/f-slice1/*.jsonl` from `lanes/f-slice1/gen_vectors.py`, which imports the reference at the end of
`proto/functions_checks.py`), `tests/julia/lball.jl`. Sources: `docs/conventions.md` 5.8 (struct and predicate),
`docs/proofs/functions.md` Definition 1 (line 11) and Proposition 4 (line 92), and the statements L1 to L8 below.

### Functions

| Function | Result | Statuses |
|---|---|---|
| `adf_lball_init/clear/set/swap/is_canonical/identical` | life cycle (conventions 2.3) | none |
| `adf_lball_set_rat(x, v, q)` | exact `q` at the prime of `v` | `OK`, `DOMAIN` (archimedean place) |
| `adf_lball_set_rat_ball(x, v, c, N)` | `c + p^N Z_p` | `OK`, `DOMAIN`, `LIMIT` |
| `adf_lball_set_fball(x, v, f)` | projection of `(A + H Zhat)/d` to `p` (L1) | `OK`, `DOMAIN`, `LIMIT` |
| `adf_lball_place`, `_is_exact`, `_contains_zero`, `_get_prec`, `_get_center` | accessors | `_get_prec`: `DOMAIN` if exact; `_get_center`: `LIMIT` |
| `equal_set`, `overlaps`, `contains` | set predicates (L8) | none (0 or 1) |
| `neg`, `add`, `sub`, `mul` | smallest ball containing the results (L2, L3, L5) | `OK`, `DOMAIN` (two primes), `LIMIT` |
| `inv`, `div` | L4 | as above, `NOT_UNIT` (exact 0), `UNIT_NOT_CERTIFIED` (ball containing 0) |
| `valuation(v, is_inf, x)` | L7 | `NOT_DETERMINED` (ball containing 0) |
| `abs(a, x)` | `p^(-v)`, exact rational (L7) | `NOT_DETERMINED`, `LIMIT` |
| `decompose(m, unit, x)` | `x = p^m unit` (L6) | `DOMAIN` (exact 0), `NOT_DETERMINED`, `LIMIT` |
| `adf_sizeof_lball`, `adf_alignof_lball` | 48, 8 | none |

### Decisions taken in this slice (the orchestrator's two, and the lane's)

1. The arithmetic is the library's own, on `fmpz`/`fmpq` (centre, valuation, precision), not FLINT's `padic`
   (orchestrator; its C source is not on disk). Every result is the smallest ball that contains the set of results,
   proved in L2 to L5 and checked by enumeration.
2. Statuses. The inverse of a ball that contains 0 is `ADF_UNIT_NOT_CERTIFIED` (conventions 3.1: "the enclosure does
   not prove invertibility", SPEC 4.5), of the exact 0 `ADF_NOT_UNIT` (conventions 3.2, the row of division by an
   exact rational). Valuation, absolute value and decomposition of a ball that contains 0 are `ADF_NOT_DETERMINED`
   (SPEC 9.3.6). The decomposition of the exact 0 is `ADF_DOMAIN` (Proposition 4: "Zero has no such decomposition"),
   while the valuation of the exact 0 is reported as infinity through `*is_inf` (SPEC 9.3.6: "an integer or
   infinity").
3. Limits. `ADF_LBALL_EXP_MAX = 2^60` on `|v|` and `|N|` of every input and result, so that the sums the functions
   form (`N - 2 v` at most `3 * 2^60`) do not overflow `slong`; `ADF_LBALL_BITS_MAX = 2^26` on the size of a power
   of `p` that is formed. A stored ball is an integer below `p^(N - v)`, so its size is the relative precision times
   `bits(p)`; a larger request is `ADF_LIMIT`, decided before any allocation that grows with it. The predicates
   return no status and form no power of `p` (they compare valuations of differences, L8).
   The rule (repair f-repair1, after the review f1, findings F1 to F3): an arithmetic function returns `ADF_LIMIT`
   if an INPUT or the RESULT is outside the limits, or if a power `p^k` above the bit limit would have to be
   formed to compute the centre of the result (reworded after review f-review2, R1 and R2, SPEC 15.4 N-D7: the
   functions avoid such a power in the cases listed below and do not promise to avoid it always). "The result is
   outside" means: its `v` or `N` is beyond `ADF_LBALL_EXP_MAX`, or its stored centre needs `p^k` with
   `k bits(p) > ADF_LBALL_BITS_MAX`. Examples where `LIMIT` is right: `neg` of `1 + 5^E Z_5` (centre `5^E - 1`),
   `inv` of `3 + 5^E Z_5` (centre `1/3 mod 5^E`), `(1 + 5^E Z_5) / (3 + 5^E Z_5)` (the same centre), the exact sum
   `5^E + 5^(-E)`. Examples where `OK` is returned although a natural intermediate is outside: `inv` of the exact
   `5^E` (an exact value has no precision), `x - x` for `x = 1 + 5^E Z_5` (the sign is applied inside the sum),
   `(5^(-E) + Z_5) / (5^(-E) + Z_5)` (the inverse has `N = 2E`, the quotient `N = E`; L4a).
4. Two operands at different primes: `ADF_DOMAIN`, outputs untouched (conventions 3.1, compatibility requirement).
   The set predicates return 0.
5. `div` is offered besides the five functions of the brief: it is `x (1/y)` and tight (L4 and L3), so it costs one
   function and one status row.
6. The centre of an exact input that is not a `p`-adic integer is never converted to a power of `p` unless the
   result needs it: the product forms `K` from valuations and multiplies the unit parts (`p^v` is not formed); the
   sum drops an operand whose valuation is at least the minimum precision `K` (it lies in `p^K Z_p`).

### Statements to add to functions.md

Notation: `p` a prime, `v = v_p`, `v(0) = infinity`, `Z_p = {x : v(x) >= 0}`. A ball is `c + p^N Z_p`, `N` an
integer, `c` in `Q_p`; the exact value is a point. The facts used are the two valuation laws of `functions.md`
Definition 1 (line 11): `v(xy) = v(x) + v(y)` and `v(x+y) >= min(v(x), v(y))` with equality when the two valuations
differ (this is the strict ultrametric inequality). Consequences used below: `p^a Z_p` is a group under addition,
`p^a Z_p` contains `p^b Z_p` exactly when `b >= a`, and `t p^a Z_p = p^(a + v(t)) Z_p` for `t != 0` (multiplication
by `t` is a bijection of `Q_p` and `Z_p` is invariant under multiplication by units). "Canonical" is the predicate
of `conventions.md` 5.8.

**L0 (reduction of a rational to the canonical centre).** Let `q` be a rational, `N` an integer, `w = v(q)`. If
`q = 0` or `w >= N`, then `q + p^N Z_p = p^N Z_p`, stored as `u = 0, v = 0`. Otherwise `q = p^w t` with `t = a/b`,
`a, b` integers prime to `p`; put `k = N - w >= 1` and `u = a b^(-1) mod p^k` in `(0, p^k)`. Then `u` is an integer,
prime to `p` (it is a unit modulo `p` because `a b^(-1)` is), `0 < u < p^k`, and `q + p^N Z_p = p^w u + p^N Z_p`.

*Proof.* If `w >= N` then `q` lies in `p^N Z_p`, a group, so `q + p^N Z_p = p^N Z_p`. Otherwise `a - b u` is
divisible by `p^k`, so `t - u = (a - b u)/b` lies in `p^k Z_p` (`b` is a unit) and `q - p^w u = p^w (t - u)` lies in
`p^(w + k) Z_p = p^N Z_p`. Hence the two balls, having centres in one class of `p^N Z_p`, are equal. The residue `u`
is not 0 modulo `p^k` because `a b^(-1)` is a unit and `k >= 1`, so it is in `(0, p^k)`. Uniqueness of the stored
form is `conventions.md` 5.8. *Check:* `proto/functions_checks.py`, `check_lball_reduction`.

**L1 (projection of a finite ball to a prime).** Let `f = (A + H Zhat)/d` with `d >= 1`, `H >= 0`, `N = H/d`. The
set of `p`-th coordinates of the elements of `f` is the exact rational `A/d` if `H = 0`, and the ball
`A/d + p^e Z_p`, `e = v_p(H) - v_p(d)`, if `H > 0`.

*Proof.* Elements of `f` are `a + N z`, `a = A/d`, `z` in `Zhat = prod_q Z_q` (the product over all primes `q`), and
the `p`-th coordinate of `a + N z` is `a + N z_p`. The projection `Zhat -> Z_p` is onto, so `z_p` takes every value
in `Z_p`. If `N = 0` the coordinate is `a`. Otherwise `N = p^e s` with `s = N/p^e` a rational whose numerator and
denominator are prime to `p`, that is a unit of `Z_p`, so `N Z_p = p^e Z_p` and the coordinate set is `a + p^e Z_p`,
`e = v(N) = v(H) - v(d)`. Its stored form is L0. This is the fact used in `adf_fball_prec_at` (`fball.h`: "the ball
constrains the coordinate at `p` exactly to `a + p^(v_p(N)) Z_p`"). *Check:* enumeration in
`check_lball_projection`.

**L2 (sum of balls).** For `x = c + p^N Z_p` and `y = d + p^M Z_p` the set `x + y = {s + t}` equals
`(c + d) + p^min(N,M) Z_p`; for an exact `q` and a ball `y`, `q + y = (q + d) + p^M Z_p`; for two exact values the
sum is the exact `q + q'`. In each case the set of sums is a single ball or point, so the result is the smallest
ball containing it.

*Proof.* `x + y = (c + d) + (p^N Z_p + p^M Z_p)`. Of the two groups one contains the other (`p^a Z_p` contains
`p^b Z_p` when `b >= a`), so their sum is the larger, `p^min(N,M) Z_p`. The exact cases are `p^M Z_p + {0}`.
Dropping an operand: if `v(c) >= K = min(N, M)` (or `c = 0`) then `c + d + p^K Z_p = d + p^K Z_p` because `c` lies
in the group `p^K Z_p`; this is why the code does not form `p^v` for an operand of such a valuation. *Check:*
`check_lball_enumeration` (add).

**L5 (negation).** `-(c + p^N Z_p) = (-c) + p^N Z_p`, `-(q) = (-q)`.

*Proof.* `p^N Z_p = -p^N Z_p` (a group). Negation is a bijection of `Q_p`, so nothing is lost or added: exact set.
Difference: `x - y = x + (-y)` and L2 apply. *Check:* enumeration (sub).

**L3 (product of balls).** Let `x = c + p^N Z_p`, `y = d + p^M Z_p` be canonical (`c = 0` or `v(c) < N`, same for
`d`; `N` or `M` may be infinity for an exact operand, meaning `a = 0` or `b = 0` below). Put
`K = min( v(c) + M, v(d) + N, N + M )`, where a term with an infinite part is left out, and at least one operand is
a ball. If neither operand is the exact 0, the set `xy = {s t : s in x, t in y}` equals `c d + p^K Z_p`. If an
operand is the exact 0 the set is `{0}`. Two exact operands give the exact `c d`.

*Proof.* Write `s = c + a`, `t = d + b`, `a` in `p^N Z_p`, `b` in `p^M Z_p`. Then `s t = c d + c b + d a + a b`.
*Enclosure.* `v(c b) >= v(c) + M`, `v(d a) >= v(d) + N`, `v(a b) >= N + M`, so each of the three terms lies in
`p^K Z_p`, and so does their sum. *Every point is reached.* Case (i): `K = v(c) + M` (so `c != 0` and `y` is a
ball). Take `a = 0` (allowed: `s = c` lies in `x`). Then `s t - c d = c b`, and as `b` runs over `p^M Z_p` the point
`c b` runs over `c p^M Z_p = p^(v(c) + M) Z_p = p^K Z_p`. Case (ii): `K = v(d) + N`, symmetric with `b = 0`. Case
(iii): neither (i) nor (ii) holds, so `K = N + M` and both operands are balls, with `v(c) + M > N + M` (the term of
(i) is larger, or absent because `c = 0`) and likewise for `d`. Canonicity gives `v(c) < N` when `c != 0`, i.e.
`v(c) + M < N + M`, so `c = 0`; likewise `d = 0`. Then `xy = p^N Z_p p^M Z_p`, which contains `p^N p^M z` for every
`z` in `Z_p`, i.e. all of `p^(N+M) Z_p`, and is inside it. In the exact-operand cases the same proof holds with
`a = 0` or `b = 0` throughout (case (i) then needs `y` a ball and `c != 0`; if `c = 0` and `x` is exact the set is
`{0}`). So `xy = c d + p^K Z_p`, an exact set, hence the smallest ball. Its stored form is L0 applied to `c d` and
`K`; `c d = p^(v(c) + v(d)) u u'` is formed as the product of the unit parts, so no power of `p` is needed. *Check:*
enumeration (mul).

**L4 (inverse of a ball).** Let `x = c + p^N Z_p` be canonical with `c != 0` (so `v(c) < N`). Then `0` is not in
`x`, and `{1/s : s in x} = (1/c) + p^(N - 2 v(c)) Z_p`, the exact set of inverses. For the exact `q != 0` the
inverse is the exact `1/q`. A ball with `c = 0` contains 0, and the set of inverses of its non-zero points is
`{y : v(y) <= -N}`, which is contained in no ball; there is no result ball. `1/0` does not exist.

*Proof.* `s = c + a` has `v(a) >= N > v(c)`, so `v(s) = v(c)` (strict ultrametric inequality) and `s != 0`. Put
`k = N - v(c) >= 1` and `z = a/c`; as `a` runs over `p^N Z_p`, `z` runs over `p^k Z_p` (multiplication by `1/c` is a
bijection with `v(a/c) = v(a) - v(c)`). Now `1/s = (1/c) (1 + z)^(-1)`. The map
`g(z) = (1 + z)^(-1) - 1 = -z/(1 + z)` sends `p^k Z_p` to itself (`1 + z` is a unit since `k >= 1`, and
`v(z/(1+z)) = v(z)`), and `g(g(z)) = z`: `1 + g(z) = 1/(1 + z)`, so `g(g(z)) = -g(z) (1 + z) = z`. So `g` is a
bijection of `p^k Z_p`, and `(1 + z)^(-1)` runs over `1 + p^k Z_p`. Hence
`{1/s} = (1/c)(1 + p^k Z_p) = 1/c + (1/c) p^k Z_p`, and `(1/c) p^k Z_p = p^(k - v(c)) Z_p = p^(N - 2 v(c)) Z_p`. For
a ball around 0: the elements `p^j` with `j >= N` are in `x` and their inverses `p^(-j)` have valuations `-j <= -N`;
every `y` with `v(y) <= -N` is the inverse of `1/y`, whose valuation is `>= N`. This set is not bounded in `Q_p`, so
no ball contains it. *Check:* enumeration (inv), and the identity of the relative precision
`(N - 2 v) - (-v) = N - v`.

*Quotient.* `{s/t} = {s} {1/t}` and the set `{1/t}` is a ball (or a point) by L4, so by L3 `x/y` is the exact
product set: `div` is tight and has the statuses of `inv`. The closed form that the code uses, which forms no
inverse, is L4a.

**L4a (quotient from valuations and precisions).** Let `x = c + p^N Z_p` and `y = d + p^M Z_p` be canonical (`N`
or `M` infinite for an exact operand), `d != 0`, `c = p^v u` (`u = 0` for a zero centre), `d = p^w t`. Then
`{s/t' : s in x, t' in y}` is the exact 0 if `x` is the exact 0; the exact `(u/t) p^(v - w)` if both are exact;
and otherwise the ball `(u/t) p^(v - w) + p^K Z_p` with
`K = min( v + M - 2w, N - w, N + M - 2w )`, where the first term is absent if `u = 0` or `y` is exact, the second
if `x` is exact, the third unless both are balls.

*Proof.* By L4 the set of inverses of `y` is the ball `1/d + p^(M - 2w) Z_p` (the point `1/d` if `y` is exact),
whose centre `1/d = p^(-w) (1/t)` has valuation `-w`. By L3 with the operands `x` and this ball, the product set is
the ball with centre `c/d = p^(v - w) (u/t)` and exponent `min( v(c) + M', v(1/d) + N, N + M' )` with
`M' = M - 2w`, `v(c) = v`, `v(1/d) = -w`, which is `min( v + M - 2w, N - w, N + M - 2w )`, the terms of an exact
operand being left out as in L3. The exact-operand cases are those of L3 (`0 * anything = 0`). Nothing in this
formula involves the inverse as a stored value, so it holds where the inverse alone is outside the limits.
*Centre.* The stored centre is L0 applied to `(u/t) p^(v - w)` and `K`, that is `u t^(-1) mod p^k` with
`k = K - (v - w)`; `t` is a unit, so the inverse modulo `p^k` exists. The computation needs `p^k`, with `k` the
relative precision of the RESULT, and nothing larger. (It equals the residue that the product of the unit parts of
`x` and of the stored inverse would give: the stored inverse is `1/t` modulo `p^(M - w)`, and `k <= M - w`, because
`K` is at most its first term `v - w + (M - w)` when `x` is a ball with `u != 0` and `y` a ball; the other cases
have no stored inverse of finite precision to compare with.) *Check:* `proto/functions_checks.py`,
`check_lball_quotient` (the closed form against the quotient by way of the inverse, 12729 pairs), and the
enumeration for `div` in `tests/test_lball.c`.

**L6 (decomposition of a ball).** Let `x = c + p^N Z_p` be canonical with `c != 0`, `m = v(c)`, `c = p^m u`. Then
every element of `x` has valuation `m`, and `x = p^m U` with `U = u + p^(N - m) Z_p`, a canonical ball of relative
precision `N - m >= 1`; the map `t -> p^m t` is a bijection from `U` onto `x`. For an exact `q != 0`: `q = p^m t`
with `t = q/p^m` an exact unit. The exact 0 has no decomposition (Proposition 4, `functions.md` line 92).

*Proof.* `p^m` is a unit of `Q_p^*`, multiplication by it is a bijection of `Q_p` and takes `u + p^(N-m) Z_p` to
`p^m u + p^N Z_p`. `U` is canonical by the predicate: `u` is an integer prime to `p` in `(0, p^(N-m))` (the stored
form of `x` says so) and the valuation 0 of `u` is below `N - m`. The valuation of every element is `m` by L7. The
finer split of a unit into a root of unity and a principal unit (Proposition 4) is not part of this slice. *Check:*
`check_lball_decompose`.

**L7 (valuation and absolute value).** For a canonical ball with `c != 0`, every element has valuation `v(c)`, so
`|x|_p = p^(-v(c))`. For a ball around 0 the elements `p^N` and `p^(N+1)` have valuations `N` and `N + 1`, and 0 has
valuation infinity: the valuation is not determined.

*Proof.* For `s = c + a`, `v(a) >= N > v(c)`, so `v(s) = v(c)` by the strict inequality. The second claim is direct.
`|s|_p = p^(-v(s))` is the definition of the normalised absolute value (Definition 1: normalised by `v(p) = 1`).

**L8 (set predicates).** For canonical balls `x = c + p^N Z_p` and `y = d + p^M Z_p` at one prime:
(1) `x = y` iff `N = M` and `c = d` (as canonical centres); an exact value and a ball are never equal; two exact
    values are equal iff their rationals are.
(2) `x` meets `y` iff `v(c - d) >= min(N, M)`; an exact `q` meets `y` iff `v(q - d) >= M`; two exact values meet iff
    equal.
(3) `x` is inside `y` iff `N >= M` and `v(c - d) >= M`; an exact `q` is inside `y` iff `v(q - d) >= M`; a ball is
    never inside an exact value; exact inside exact iff equal.

*Proof.* (3) If `x` is inside `y`, the point `c` is in `y`, so `v(c - d) >= M`; the point `c + p^N` is in `x`, hence
in `y`, so `v(c + p^N - d) >= M`, so `v(p^N) = N >= M` (the difference of the two conditions). Conversely, for `a`
in `p^N Z_p` and `N >= M`, `v(c + a - d) >= min(v(c - d), N) >= M`. (2) `s + a = t + b` has a solution exactly when
`c - d` lies in `p^N Z_p + p^M Z_p = p^min(N,M) Z_p`. (1) follows from (3) in both directions: `N >= M`, `M >= N`
and `c - d` in `p^N Z_p`, and two centres in `Z[1/p] ∩ [0, p^N)` in the same class of `p^N Z_p` are equal
(`conventions.md` 5.8). Exact cases: a point is inside a ball iff it is one of its points; an infinite set is inside
a point never. *The computation of `v(c - d)` from stored forms:* with `c = p^v u`, `d = p^w u'` (`c = 0` or `d = 0`
giving infinity): `v(c - d) = min(v, w)` if `v != w` (strict inequality), and `v + v_p(u - u')` if `v = w` (infinite
if `u = u'`), where `v_p` of a rational is the valuation of its numerator minus that of its denominator. No power of
`p` is formed.

## Slice 1F.1-a and 1F.2-a: `adf_sball` and the real functions (lane f-slice2, 2026-09-29)

Headers: `include/adelefeld/sball.h`, `include/adelefeld/rfunc.h`. Implementation: `src/sball.c`, `src/rfunc.c`.
Tests: `tests/test_sball.c`, `tests/test_rfunc.c` (vectors `tests/ref/vectors/f-slice2/*.jsonl` from
`lanes/f-slice2/gen_vectors.py`, which imports the reference at the end of `proto/functions_checks.py`, section
"f-slice2"), `tests/julia/sball.jl`. Sources: `docs/conventions.md` 5.9 (struct and predicate), 7 (places, canonical
order), 3.1 to 3.3 (statuses, the reported place), 4.3, 4.4; `docs/SPEC.md` 9.3.1, 9.3.3, 9.3.6;
`docs/proofs/functions.md` Proposition 14 (line 446) and Proposition 22 (line 725); `refs/src/flint-3.0.1/arb.rst`
(cited by line in `src/rfunc.c` and `src/sball.c`); the statements S1 to S7 below.

### Functions

| Function | Result | Statuses |
|---|---|---|
| `adf_sball_init/clear/set/swap/is_canonical/identical` | life cycle (conventions 2.3, 5.9) | none |
| `adf_sball_set_arb_lballs(y, where, r, loc, n)` | the partial ball with real part `r` (or none) and the components `loc`, sorted | `OK`, `DOMAIN` (with the place) |
| `adf_sball_project(y, where, x, places, n)` | the projection of an adele to the places (S1) | `OK`, `DOMAIN` (repeated place), `LIMIT` (with the prime) |
| `adf_sball_arch`, `_num_places`, `_get_place`, `_has_place`, `_get_lball`, `_get_arb` | accessors; the canonical order of places | `_get_place`, `_get_lball`, `_get_arb`: `DOMAIN` |
| `adf_sball_equal_set`, `_overlaps`, `_contains` | set predicates (S4) | none (0 or 1) |
| `adf_sball_neg`, `_add`, `_sub`, `_mul` | componentwise, over the same set of places (S3) | `OK`, `DOMAIN` (places differ, with the first place), `UNSUPPORTED` (complex tag), `LIMIT` (with the prime) |
| `adf_real_exp`, `_log`, `_log_abs`, `_sin`, `_cos`, `_sqrt`, `_root` | real functions on an `arb` (S5, S6) | `OK`, `DOMAIN`, `NOT_DETERMINED` |
| `adf_sball_exp_at`, `_log_at`, `_log_abs_at`, `_sin_at`, `_cos_at`, `_sqrt_at`, `_root_at` | the same at the archimedean place of a partial ball; the result is a partial ball over that one place | as above, plus `DOMAIN` (place not in the ball), `UNSUPPORTED` (a prime, or the complex tag) |
| `adf_sizeof_sball`, `adf_alignof_sball` | 120, 8 | none |

### Decisions taken in this slice (the orchestrator's, and the lane's)

1. The list of places is stored in the canonical order of conventions 7: the archimedean place first, then the
   primes increasing (conventions 5.9 keeps the primes in `loc`, strictly increasing, and the archimedean coordinate
   in `inf`). The brief said "the real place last"; conventions 7 says the opposite and holds. Index 0 of
   `get_place` is the archimedean place when the tag is not `NONE`.
2. Sets of places are passed as an array of `adf_place_t` and a length. No `adf_places_t` struct is defined in the
   conventions (the name occurs in 7 and 2.2 only). Alternative: define the struct; not done, because nothing else
   needs it yet.
3. `where` is a report argument (conventions 2.2, 4.3): written on a status other than `OK`, untouched on `OK`, may
   be `NULL`. On `DOMAIN` from the projection it is the repeated place; from two operands over different places it
   is the first place, in the canonical order, that belongs to one operand only (a different tag is a difference at
   the archimedean place); on `LIMIT` it is the first prime, in the canonical order, at which a component fails
   (conventions 3.3: the statuses of the components are all `LIMIT`, so the maximum is `LIMIT` and the first place
   carrying it is reported).
4. Complex components (`arch = COMPLEX`) are stored, copied, compared and checked by the predicate; arithmetic and
   the functions of `rfunc.h` return `ADF_UNSUPPORTED` with the archimedean place. No function of the slice makes a
   complex tag. Alternative: `acb` arithmetic (four lines); not done, because no test could reach it through the
   interface.
5. Functions at a place of a partial ball produce a partial ball over that one place (SPEC 9.3.1: "the other
   coordinates of `x` are not part of the result"). The functions at a prime (milestone 1F.4) are `UNSUPPORTED` with
   that prime.
6. The domain rule is the one of conventions 3.1: `DOMAIN` only if every point of the input ball is outside the
   domain, `NOT_DETERMINED` if the ball meets the domain and its complement (S5). The exact 0 is `DOMAIN` under
   `log` and `log_abs`, and gives 0 under `sqrt` and every root. `sqrt` is the root of degree 2 (the non-negative
   root, SPEC 9.3.3). A root of degree 0 is `DOMAIN` with no place (invalid degree).
7. The odd root of a negative ball is computed as minus the root of the negated ball (orchestrator); a ball that
   contains 0 is enclosed by the images of its outer end points (S6). Alternative: `exp(log(x)/n)` for the positive
   part; it is worse near 0.
8. A result that `arb_is_finite` rejects is `NOT_DETERMINED` and is never stored (conventions 4.4, CV-08). So
   `exp(2^1000)` is `NOT_DETERMINED` (measured; `exp(2^60)` is `OK`); alternative: `LIMIT` for an overflow of arb's
   range. A `prec` below 2 is 2 (M1-D4). There is no upper bound on `prec` (as for `adf_adele_mul`).
9. `adf_sball_set_arb_lballs` is a raw-data constructor (conventions 3.2: `OK`, `DOMAIN`); it also sorts, so that a
   binding need not. It is what makes a partial ball of chosen components without an adele.

### Statements S1 to S7 (to be merged into functions.md)

Notation as in L0 to L8 above: `p` a prime, `v = v_p`, balls `c + p^N Z_p`. `Zhat = prod_q Z_q` over all primes `q`.
A real ball `m +- r` (`arb`, exact midpoint `m` and radius `r >= 0`) is the closed interval `[m - r, m + r]`
(`arb.rst`, lines 639 to 649: the predicates are stated for "all points p in the interval represented by x"). A
partial ball is the set of the tuples described in `sball.h`.

**S1 (projection of an adele to a set of places).** Let `x = (I ; F)` with `I` a real ball and `F = (A + H Zhat)/d`
a finite ball, and let `S = {inf} u {p_1, ..., p_k}` or `S = {p_1, ..., p_k}` (distinct primes). The set of the
tuples `(a_v)_{v in S}` of the coordinates of the elements of `I x F` is `I x prod_i B_i`, `B_i` the set of L1 at
`p_i` (the exact rational `A/d` if `H = 0`, the ball `A/d + p^(e_i) Z_p` otherwise). The order of the factors is
immaterial.

*Proof.* An element of `I x F` is `(t, a + N z)` with `t` in `I`, `a = A/d`, `N = H/d`, `z` in `Zhat = prod_q Z_q`;
its coordinates are `t` at the real place and `a + N z_q` at the prime `q`. The real coordinate `t` and the finite
part `z` are independent (the set is a product). The coordinates `z_q` of `z` are independent too (`Zhat` is the
product), so for the distinct primes `p_1, ..., p_k` every tuple `(z_{p_1}, ..., z_{p_k})` in `prod Z_{p_i}` occurs
(the other coordinates are chosen as 0). Hence the set of tuples of `(a + N z_{p_i})_i` is `prod_i (a + N Z_{p_i})`,
and each factor is the set of L1. The coordinates at the primes outside `S` are not part of the tuple, and they are
not constrained: nothing else is claimed. Each factor is stored by L0. *Check:* `check_sball_projection_enumeration`
(600 cases: every point `A/d + H z/d`, `z < p^2`, lies in the component and the `p` classes modulo the next digit
are all met), the vectors `sball_project.jsonl`, the enumeration `projection_enumeration` in `tests/test_sball.c`.

**S2 (order and sets of places).** The canonical order is `inf` first, then the primes increasing (conventions 7). A
constructor accepts any order, sorts, and rejects a repeated place: the set of places is a set. The tuples over two
different sets of places are elements of different spaces; no function of the slice compares or combines them
(`DOMAIN` for the operations, 0 for the predicates).

**S3 (componentwise ring operations).** Let `X = prod_{v in S} X_v` and `Y = prod_{v in S} Y_v` be partial balls
over the same `S`, and `o` one of `+`, `-`, `.`. Then `{x o y : x in X, y in Y} = prod_{v in S} {x_v o y_v : x_v in
X_v, y_v in Y_v}`, and the same for `-x`.

*Proof.* The operations of the ring `prod_v Q_v` (with `Q_inf = R`) act coordinatewise, so the tuple of results of
`(x, y)` is `(x_v o y_v)_v`. The choices of `(x_v, y_v)` at the different places are independent because `X` and `Y`
are products, so every tuple of results of one point pair at each place is the result of a pair of points of `X` and
`Y`, and conversely. At a prime the factor is a ball or a point, and its smallest enclosing ball is the result of
L2, L3, L5 (the factor itself is that ball). At the real place the factor is the interval of results of the interval
operation, `[lo + lo', hi + hi']`, `[lo - hi', hi - lo']`, `[min P, max P]` with `P` the four products of end points
(Moore); `arb_add`, `arb_sub`, `arb_mul` return a ball that contains it (`arb.rst`, lines 767, 785, 798) with a
radius at most the exact half width plus a rounding term (the tests assert: at most the width, times two for a
product, plus `2^(4 - prec)` of the largest end point, plus the `2^-29` relative rounding of a `mag`). So the result
contains the set of results, and at the primes equals it. *Check:* `check_sball_tuple_enumeration` (21600 tuple
coordinates), the vectors `sball_ops.jsonl`, `tuples_of_points_through_the_operations` in `tests/test_sball.c`
(48000 coordinates, over places `{inf, 2, 3, 5, 7}`: a tuple of results lies in the result component at every place,
which pairs the components of the two operands correctly).

**S4 (set predicates).** For nonempty sets `X = prod X_v`, `Y = prod Y_v` over the same `S`: `X = Y` iff `X_v = Y_v`
for all `v`; `X` meets `Y` iff `X_v` meets `Y_v` for all `v`; `X` is inside `Y` iff `X_v` is inside `Y_v` for all
`v`. For a real ball the sets are closed intervals with exact end points: equal iff each contains the other
(`arb_contains`, `arb.rst` line 668, which is exact), meeting iff `max(lo, lo') <= min(hi, hi')` (`arb_overlaps`,
line 651), inside iff `arb_contains`. A complex ball is the product of two intervals: the same with `acb_contains`,
`acb_overlaps`. At a prime L8 applies.

*Proof.* Products of nonempty sets: `X = Y` iff the projections agree (a product is determined by its factors, and
the factors are nonempty); a tuple in `X` and `Y` has each coordinate in `X_v` and `Y_v`, and conversely a choice of
a common point at each place gives a common tuple; `X` inside `Y` gives each projection inside, and conversely each
tuple of `X` has its coordinates in the `Y_v`. *Check:* `sball_pred.jsonl` (1200 rows, both argument orders) and
`predicates_by_hand`.

**S5 (domains and statuses of the real functions).** For a real ball `B` with end points `lo <= hi`: `log` has the
domain `t > 0`: `B` inside the domain iff `lo > 0`, disjoint iff `hi <= 0`; `log_abs` (`log |t|`) has `t != 0`:
inside iff `lo > 0` or `hi < 0`, disjoint iff `lo = hi = 0`; `sqrt` and the roots of even degree have `t >= 0`:
inside iff `lo >= 0`, disjoint iff `hi < 0`; `exp`, `sin`, `cos` and the roots of odd degree have the domain R. The
wrapper returns `OK` when `B` is inside, `DOMAIN` when it is disjoint, `NOT_DETERMINED` otherwise (conventions 3.1:
"a ball that meets both the domain and its complement gives `ADF_NOT_DETERMINED`"). `arb_is_positive`,
`arb_is_nonnegative`, `arb_is_negative`, `arb_is_nonpositive` decide exactly these conditions on `lo` and `hi`
(`arb.rst`, lines 639 to 649).

*Proof.* Each clause is the definition of the domain applied to the closed interval. `log_abs`: `t = 0` is the only
excluded point of R, and `B` contains it iff `lo <= 0 <= hi`; `B` is inside the domain iff it avoids it, and is
disjoint from the domain iff `B = {0}`. *Check:* `rf_status` in the reference (exact rationals), the vectors
`rfunc_real.jsonl` (the status of every line, decided on the exact end points), the table test `domain_table`.

**S6 (the image of a ball by the end points).** Let `f` be continuous and increasing on an interval `J` that
contains `B = [lo, hi]`. Then `f(B) = [f(lo), f(hi)]`, and any ball that contains `f(lo)` and `f(hi)` contains
`f(B)`. The `n`-th root is increasing on `R` for odd `n` and on `[0, infinity)` for even `n` (Proposition 14, step
1: `x^n` is strictly increasing on `[0, infinity)`; step 2: the odd power is odd, so the root is odd and increasing
on `R`). Consequences used: (a) for odd `n` and `hi < 0`: `root(t) = -root(-t)`, so the image is the negation of the
image of `-B`; (b) for `B` that contains 0, and outer end points `a <= lo`, `b >= hi` (so `a <= 0 <= b`), the image
lies in `[root(a), root(b)] = [-root(-a), root(b)]` (odd `n`), resp. `[0, root(b)]` (even `n`, where `lo = 0`
exactly because `B` is inside the domain and contains 0); (c) the exact 0 has the root 0 for every `n >= 1`.
`arb_root_ui` is applied only to a strictly positive ball or a strictly positive exact point (the probe of lane
d-functions found NaN for the odd root of a negative ball and of 0: arb.rst line 979 states the error bound for `0
<= r <= m`). `exp`, `log`, `sin`, `cos` are the `arb` functions (`arb.rst` lines 1082, 1050, 1101, 1103): the
enclosure of the image by `arb`, not tight; the tests bound the radius by 4 times the true width where the function
is monotone on the ball and the ball is small relative to the point (`exp`: radius at most 1; `log`, `sqrt`, roots:
radius at most half the midpoint; `sin`, `cos`: radius at most 1/16 and derivative at least 3/4 at the midpoint) and
otherwise by the Lipschitz bound (`sin`, `cos`: the output radius is at most the input radius plus `2^(4 - prec)`).

*Proof.* The first sentence is the intermediate value theorem for a continuous increasing `f` (Lemma 2 of
`functions.md`), and monotonicity gives the endpoints as extremes. (a) and (b) are consequences; in (b), `lo <= hi`,
`a <= lo`, `hi <= b` and monotonicity give `root(lo) >= root(a)` and `root(hi) <= root(b)`. *Check:* the vectors
`rfunc_real.jsonl` (1301 rows: the image of the ball is enclosed by mpmath intervals at 800 bits, or by exact
integer roots; the result must contain it widened by `2^-350` of the largest end point),
`odd_roots_of_negative_and_zero`, `known_values`. The reference is checked against `python-flint`'s `arb` at 300
bits (`check_real_reference_against_arb`, 200 cases).

**S7 (no non-finite ball with OK).** A wrapper that returns `OK` has written a ball for which `arb_is_finite` holds
(conventions 4.4, row "Non-finite ball produced inside a computation from finite inputs": `NOT_DETERMINED`, CV-08).
An input that is not finite is `DOMAIN` (a constructor-like invalid input, conventions 3.1). *Check:*
`non_finite_inputs`, `huge_arguments_never_give_a_nonfinite_ok` (arguments up to `2^(2^62)`).

### What the oracles are

Projection: enumeration in C (S1) and the reference `lb_ref_project` of L1, which f-slice1 tested by enumeration.
Operations: tuples of points (S3), the reference of L2 to L5 at the primes, exact interval arithmetic at the real
place. Predicates: the reference of L8 at the primes, exact end points at the real place. Real functions: mpmath
intervals at 800 bits and exact integer roots (no `arb` in the reference), the status decided on exact rational end
points, and hand tests of what `arb` does badly.

## Slice 1F.3-b: the split of a unit, the fractional part, powers (lane f-slice3, 2026-09-30)

Header: `include/adelefeld/lball.h`, section "slice 1F.3-b". Implementation: `src/lball_decomp.c` (Teichmueller,
split, fractional part, unit modulo `p^k`) and `src/lball.c` (`adf_lball_pow_si`, the change of
`adf_lball_set_fball`). Tests: `tests/test_lball_decomp.c` (vectors `tests/ref/vectors/f-slice3/lball_slice3.jsonl`
from `lanes/f-slice3/gen_vectors.py`, and enumerations written in the test), `tests/test_lball.c` (`set_fball`),
`tests/julia/lball2.jl`. Reference: `proto/functions_checks.py`, section f-slice3. Sources:
`docs/proofs/functions.md` Lemma 3 (line 55; item 2, proof at line 72), Proposition 4 (line 92), Lemma 9 (line 265),
Proposition 19 (line 656).

### Functions

| Function | Result | Statuses |
|---|---|---|
| `adf_lball_teichmuller(w, v, r, prec)` | the root of `T^(p-1) - 1` with residue `r`, exact if `+-1`, else a ball of precision `max(prec, 1)` | `OK`, `DOMAIN` (real place, `p` divides `r`), `LIMIT` |
| `adf_lball_decompose_teich(m, w, index, u, x, prec)` | `x = p^m w u`: `w` Teichmueller factor (sign at 2), `u` principal unit, `index` the residue | `OK`, `DOMAIN` (exact 0), `NOT_DETERMINED` (ball with 0; `p = 2`, relative precision 1), `LIMIT` |
| `adf_lball_frac(r, x)` | `{x}_p` as an exact rational in `[0, 1)` | `OK`, `NOT_DETERMINED` (ball with `N < 0`), `LIMIT` |
| `adf_lball_unit_mod(out, x, k)` | the unit part of `x` modulo `p^k` | `OK`, `DOMAIN`, `NOT_DETERMINED`, `LIMIT` |
| `adf_lball_pow_si(y, x, k)` | smallest ball containing `{s^k : s in x}`; `k = 0` gives the exact 1 | `OK`, `NOT_UNIT`, `UNIT_NOT_CERTIFIED`, `LIMIT` |
| `adf_lball_set_fball` | unchanged results; `LIMIT` decided early (L13) | as before |

### Decisions taken in this slice (each with the alternative)

1. **Names.** `decompose_teich` (not a change of `decompose`, which the header keeps: an existing declaration is not
   changed) and a separate `teichmuller`. Alternative: one function with a flag; rejected, two results of different
   types.
2. **`prec < 1` is taken as 1.** A ball of precision `n <= 0` around a unit would contain 0. Alternative: `DOMAIN`;
   rejected: the caller who passes 0 wants "the least precision", and 1 is the residue. The same clamp as `prec` in
   `adf_rfunc` (`prec_below_two_is_two`).
3. **The factor `w` is exact when it is rational** (`+1`, `-1`; at `p = 2` and `p = 3` always), else a ball of
   precision `n`. The set `{w}` is one point; the smallest ball containing a rational point is the point.
   Alternative: always a ball of precision `n`; rejected: the exact value is smaller and costs no power `p^n`. A
   caller that wants a uniform type reads `adf_lball_is_exact`.
4. **The precision of `u`.** For a ball `x` with relative precision `k`, `u` has precision `k`, whatever `prec`: the
   set of principal units of the points of `x` is the ball `u_bar + p^k Z_p` (L10), exactly, and `k` digits of
   `u_bar` need only `k` digits of `omega`. Alternative: `min(k, prec)`; rejected: it would make `u` larger than the
   set. For an exact `x` whose `w` is irrational, `u = t/omega` is not a rational and gets precision `prec`.
5. **`p = 2` with relative precision 1 is `NOT_DETERMINED`.** The unit ball is `1 + 2 Z_2` and contains the points 1
   and 3 modulo 4, which have different signs (Proposition 4 step 2). Alternative: return the hull `w in {1, -1}`;
   rejected: not a ball of Q_2 that is a factor, and the product `w u` would not be the ball.
6. **Powers.** `x^0 = 1` exactly for every `x`, also the exact 0 and a ball with 0 (the convention `0^0 = 1` of an
   empty product; alternative `DOMAIN` for `0^0`: rejected, the brief asks for the exact 1). A ball around 0 to a
   power `k > 0` is `O(p^(k N))` (the smallest ball containing the set, which itself is not a ball, L12). An exact
   power is a rational whose numerator and denominator may have at most `ADF_LBALL_BITS_MAX` bits, decided before it
   is formed (`+-1` never limited): this is a limit of the library that the header states for `pow_si` only.
7. **`frac` of a ball with `N < 0` is `NOT_DETERMINED`** (Proposition 19 step 3: two points of the ball have
   different fractional parts). Alternative: return the set of fractional parts; there is no type for it.
8. **`set_fball`.** No bit-length argument alone decides `LIMIT` (L13, step 2 shows why); the change is a bounded
   number of divisibility tests. Alternative: a faster valuation for huge integers everywhere; out of scope.

### Statements to add to functions.md

Notation as in L0 to L8. `omega_p(r)` is the Teichmueller representative of Proposition 4.

**L9 (Newton lifting of the Teichmueller representative).** Let `p` be odd, `1 <= r <= p - 1`, `f(T) = T^(p-1) - 1`.
Then `f(r) = 0` modulo `p` (Fermat: `F_p^x` has order `p - 1`, Lemma 3 item 3) and `f'(r) = (p-1) r^(p-2)` is a
unit, so by Lemma 3 item 2 there is exactly one root `omega` of `f` in `Z_p` with `omega = r` modulo `p`. If an
integer `w` satisfies `w = omega` modulo `p^j`, `j >= 1`, then `w' = w - f(w)/f'(w)` satisfies `w' = omega` modulo
`p^(2j)`. Also `omega` is rational exactly when `r = 1` (`omega = 1`) or `r = p - 1` (`omega = -1`).

*Proof.* `f(w) = f(w) - f(omega) = (w - omega) H` with `H = sum_{i=0}^{p-2} w^i omega^(p-2-i)`, which is `(p-1)
r^(p-2)` modulo `p`, a unit; so `v(f(w)) = v(w - omega) >= j`. `f'(w) = (p-1) w^(p-2)` is a unit (`w = r` modulo
`p`). Hence `h = -f(w)/f'(w)` has valuation `>= j`. Taylor expansion of a polynomial with integer coefficients: `f(w
+ h) = f(w) + f'(w) h + h^2 G(w, h)` with `G` in `Z[w, h]`; the first two terms cancel, so `v(f(w + h)) >= 2j`. Now
`w' = w + h = w = omega` modulo `p^j`, so the factor `H'` of `f(w') = (w' - omega) H'` is again a unit and `v(w' -
omega) = v(f(w')) >= 2j`. The code works modulo `P = p^min(2j, K)`: `h` modulo `P` is `-(f(w) mod P) (f'(w)^(-1) mod
P)` because `f'(w)` is a unit, and `w' mod P` does not depend on the integer representative of `w` chosen, only on
`w` modulo `p^j` (the difference of two representatives is in `p^j Z_p`, and by the same computation the result
changes by `p^(2j) Z_p`). Rationality: a rational root of `T^(p-1) - 1` is `+-1` (rational root theorem), `1` has
residue 1, `-1` has residue `p - 1` and both are roots (`p - 1` even), so by uniqueness `omega(1) = 1` and
`omega(p-1) = -1`; any other residue has an irrational `omega`. At `p = 3` the residues are 1 and 2. *Check:*
`check_teichmueller_three_ways` (the limit of `r^(p^K)`, the digit-by-digit lifting of Lemma 3 and Newton agree, `p
= 3` to 13, `K` up to 12), and `tests/test_lball_decomp.c` against `r^(p^L) mod p^L`.

**L10 (the split of a ball).** Let `x = p^m (u + p^k Z_p)` be a canonical ball with `k = N - m >= 1`, `u` an integer
prime to `p`. (a) `p` odd: put `r = u mod p` and `omega = omega_p(r)`. Every `a` in `U = u + p^k Z_p` has `w(a) =
omega` and `u'(a) = a/omega`; the set of `u'(a)` is the ball `u_bar + p^k Z_p` with `u_bar = u omega^(-1)` modulo
`p^k`, and `omega (u_bar + p^k Z_p) = U`. (b) `p = 2`, `k >= 2`: put `s = 1` if `u = 1` modulo 4, else `s = -1`.
Every `a` in `U` has sign `s` and `u'(a) = s a`; the set of `u'` is `s u + 2^k Z_2`, a ball in `1 + 4 Z_2`. (c) `p =
2`, `k = 1`: `U` contains points `= 1` and `= 3` modulo 4, with signs `+1` and `-1`: the sign is not determined. (d)
exact `x = p^m t`, `t = a/b` a unit: the residue is `a b^(-1)` modulo `p` (modulo 4 at `p = 2`, where `b^(-1) = b`);
`w` is as above, `u' = t/w`, a rational when `w = +-1`, else the `p`-adic number `t/omega`, which is irrational
(else `omega` would be rational); it lies in the ball of precision `n` with centre `t omega^(-1)` modulo `p^n`.

*Proof.* (a) `k >= 1`, so `a = u` modulo `p`, so `a` has the residue `r`; Proposition 4 step 1 gives `w(a) =
omega(r)` and `u'(a) = a/omega`, which is `1` modulo `p` (`a = omega = r`). Multiplication by the unit `omega^(-1)`
is a bijection of `Z_p` that maps `p^k Z_p` onto itself, so `a/omega` runs over `u/omega + p^k Z_p` exactly as `a`
runs over `U`. (b) `k >= 2` gives `a = u` modulo 4. Proposition 4 step 2: `w = 1` if `a = 1` modulo 4, `w = -1` if
`a = 3`; `a/w = w a`, and `w a - w u` runs over `p^k Z_2`. (c) `1` and `3` are in `1 + 2 Z_2 = U`; by Proposition 4
step 2 their signs differ, and the sign is part of the decomposition (uniqueness). (d) The same computation for one
point. `x = p^m w u'` holds as sets by (a) to (d): `omega (u_bar + p^k Z_p) = U` and `p^m U = x`. The centre `u_bar`
in `(0, p^k)` is prime to `p` (product of units), so the stored ball is canonical. *Check:*
`check_split_enumeration` (`p` = 2, 3, 5, 7: the returned `w`, that each principal unit `a/w_bf` lies in the ball,
and that the `p` classes modulo `p^(k+1)` are all met), `tests/test_lball_decomp.c` (the same, in C, with `w u`
recomposed by the library product).

**L11 (fractional part, unit modulo `p^k`).** Proposition 19 gives `{x}_p = a/p^k` with `k = -v_p(x) > 0` and `a =
p^k x` modulo `p^k` in `[0, p^k)`. For `x = p^v t` (exact, `t` a unit rational, `v < 0`): `p^k x = t`, so `a = t mod
p^k` (`a' b^(-1)` for `t = a'/b`); for a ball `p^v (u + ...)`, the same with the centre. For a ball `c + p^N Z_p`
with `N >= 0`, `x - c` is in `p^N Z_p` inside `Z_p`, so `{x}_p = {c}_p` (Proposition 19 step 3); for `N < 0` it is
not constant (`c` and `c + p^N`). The result `a/p^k` has denominator exactly `p^k` (`a` is a unit), so `v < 0` and
`k bits(p) > ADF_LBALL_BITS_MAX` puts the result outside the limits. The unit modulo `p^k` of `x = p^v u` is the
integer in `[0, p^k)` congruent to `u` (to `a' b^(-1)` for an exact `a'/b`); for a ball it is determined exactly for
`k <= N - v` (L6: the ball fixes `u` modulo `p^(N - v)`, and two points of `u + p^(N-v) Z_p` need not agree modulo
`p^(k)` for `k` larger). *Check:* `check_frac_and_unit_mod`, and the definitions (`0 <= r < 1`, denominator a power
of `p`, `x - r` in `Z_p`, on the points of every ball of the universes) in `tests/test_lball_decomp.c`.

**L12 (integer powers).** Let `x = p^v (u + p^rel Z_p)`, `rel >= 1`, `u` an integer prime to `p`, and `n >= 1`. Then
`{s^n : s in x} = p^(n v) (u^n + p^rel' Z_p)` with `rel' = rel + v_p(n) + e`, `e = 1` if `p = 2`, `rel = 1` and `n`
even, else `e = 0`. The set `{s^(-n)}` is `p^(-n v) ((u^n)^(-1) + p^rel' Z_p)`. For the ball `p^N Z_p` around 0 the
smallest ball containing `{s^n}` is `p^(n N) Z_p`. An exact `x` gives the exact power; `0^n = 0`.

*Proof.* Take out `p^v`: `s = p^v t`, `t` in `U = u + p^rel Z_p`. Write `t = u (1 + z)`, `z` in `p^rel Z_p` (`z = (t
- u)/u`, a bijection of `p^rel Z_p` onto itself since `u` is a unit). So `t^n = u^n (1 + z)^n`. Ordinary case, `rel
>= c` (`c = 1` odd `p`, `c = 2` for `p = 2`): by Lemma 9 (line 265) `exp` and `log` are inverse bijections between
`p^r Z_p` and `1 + p^r Z_p` for `r >= c`, `exp(y + y') = exp(y) exp(y')`. So `1 + z = exp(y)` with `y = log(1 + z)`
running over `p^rel Z_p`, and `(1 + z)^n = exp(n y)`. As `y` runs over `p^rel Z_p`, `n y` runs over `n p^rel Z_p =
p^(rel + v_p(n)) Z_p` (`n` is `p^(v_p(n))` times a unit), and `exp` maps that onto `1 + p^(rel + v_p(n)) Z_p` (Lemma
9 with `r = rel + v_p(n) >= c`). Hence `{t^n} = u^n (1 + p^(rel + v_p(n)) Z_p) = u^n + p^(rel + v_p(n)) Z_p` (`u^n`
is a unit). Special case `p = 2`, `rel = 1`: `U` is the set of all odd 2-adic integers. Each odd `t` is `s t'` with
`s = +-1` and `t'` in `1 + 4 Z_2` (take `s = 1` if `t = 1` modulo 4, else `s = -1`), and every pair `(s, t')`
occurs. `t^n = s^n t'^n` and, by the ordinary case with `rel = 2`, `{t'^n} = 1 + 2^(2 + v_2(n)) Z_2`. For even `n`:
`s^n = 1`, the set is `1 + 2^(2 + v_2(n)) Z_2` and `u^n` lies in it, so it is `u^n + 2^(rel + v_2(n) + 1) Z_2`. For
odd `n`: `v_2(n) = 0` and the set is `(1 + 4 Z_2) union (-1 + 4 Z_2) = 1 + 2 Z_2 = u^n + 2 Z_2`, which is `rel' = 1
= rel + v_2(n)`. Negative exponent: the set of `n`-th powers is a ball `c + p^(N') Z_p` with `c != 0` and `v(c) = n
v < N'`, whose set of inverses is, by L4, `1/c + p^(N' - 2 n v) Z_p`: the relative precision `N' - n v = rel'` is
kept, and the centre is `u^(-n)` modulo `p^rel'`. Ball around 0: `s = p^N t`, `t` in `Z_p`, so `s^n = p^(n N) t^n`
is in `p^(n N) Z_p`; the set contains `0` (`t = 0`) and `p^(n N)` (`t = 1`), whose difference has valuation `n N`,
so no ball of exponent above `n N` contains it: the smallest ball is `p^(n N) Z_p`. (The set of `n`-th powers is not
that ball: a point `p^(n N) t` with `t` not an `n`-th power is not a `n`-th power; the header says "smallest ball".)
*Check:* `check_pow_enumeration` (`p` = 2, 3, 5, 7; the exponent of the result equals the smallest valuation of a
difference of two `k`-th powers of points; each `k`-th power lies in it; `k` from -9 to 27), and the same in
`tests/test_lball_decomp.c` and by the vectors.

*Limits.* `v n` beyond `ADF_LBALL_EXP_MAX` for `x` with `v != 0` is decided from `|v| > EXP_MAX / n` (no overflow,
`n` up to `2^63`); `N' = n v + rel'` is checked; the centre `u^n mod p^rel'` is formed by `fmpz_powm` when `p^rel'`
is within `ADF_LBALL_BITS_MAX` bits, else, for `k > 0`, as the integer `u^n` if its bit length allows (`n (bits(u) -
1) <= ADF_LBALL_BITS_MAX`, since `bits(u^n) > n (bits(u) - 1)`), stored unreduced by `lb_make` when below `p^rel'`.
The centre `1` needs no power at all. For an exact `x` the same bit argument bounds the numerator and the
denominator of `x^k`.

**L13 (early decision of `LIMIT` in `adf_lball_set_fball`).** Let `f = (A + H Zhat)/d` with `H > 0`, `A > 0`, `k =
v_p(H) - v_p(A)` and `kmax = ADF_LBALL_BITS_MAX / bits(p)`. (1) `set_fball` returns `LIMIT` only if the exponent `e
= v_p(H) - v_p(d)` or `v_p(A/d)` is beyond `ADF_LBALL_EXP_MAX` (impossible for integers that fit in memory: `e <=
bits(H)`), or `k > 0`, `k > kmax` and the centre is not the small integer that `lb_make` keeps (integer numerator `>
0`, denominator a power of `p`, `k >= ceil(bits(nr)/(bits(p) - 1))` for the cofactor `nr` of the numerator). (2)
`v_p(H) <= vHmax = floor((bits(H) - 1)/(bits(p) - 1))`. (3) `k > kmax` if and only if `p^(v_p(A) + kmax + 1)`
divides `H`; and if `p^j` does not divide `H` then `p^T` does not for `T >= j`.

*Proof.* (1) `lb_make` with `N = e`, centre `A/d` of valuation `w = v_p(A) - v_p(d)`: if `w >= e` (`k <= 0`) the
result is the ball around 0 (no power); otherwise it needs the residue of the unit part modulo `p^(e - w)` and `e -
w = v_p(H) - v_p(A) = k`; it does not form `p^k` for a positive integer numerator with denominator 1 below `2^(k
(bits(p) - 1)) <= p^k`, else it needs `p^k` and returns `LIMIT` exactly when `k > kmax` (`pow_ok`). (2) `p^v <= H`
(`p^v` divides `H > 0`) and `p >= 2^(bits(p) - 1)`, so `v (bits(p) - 1) <= log2(H) < bits(H)`. (3) `k > kmax` means
`v_p(H) >= v_p(A) + kmax + 1`, i.e. `p^(v_p(A) + kmax + 1)` divides `H`. The decision therefore is: (i) `v_p(A) +
kmax >= vHmax`: `k <= vHmax - v_p(A) <= kmax`, never `LIMIT` by (1); (ii) the denominator of `A/d` is a power of `p`
and `v_p(A) + ceil(bits(nr)/(bits(p) - 1)) <= vHmax`: the shortcut can hold (`k >= need` is possible), so the full
valuation is computed as before; (iii) otherwise either the shortcut cannot hold (`k <= vHmax - v_p(A) < need`, or
the denominator is not a power of `p`) and `LIMIT` is exactly `k > kmax`, tested by (3). The test is preceded by
`p^1 | H` and `p^64 | H` (necessary conditions), so where `v_p(H)` is small it costs one pass over `H`. *Measured*
(`build/scratch/t2`, `H = 6^(2^25 + 1)`, `A = H - 1`): `p = 3`: 6.1 s before, 1.0 to 1.8 s after; `p = 2`: 0.03 s;
`p = 5` (the ball around 0): 0.03 s. No bit-length argument alone decides `LIMIT`: (2) bounds `v_p(H)` from above,
and `LIMIT` needs `v_p(H)` large. *Check:* `tests/test_lball.c`
`set_fball_limit_is_decided_without_the_full_valuation` (red on the old code: 5.2 s against the bound 3.5 s),
`set_fball_small_centre_above_the_bound_is_ok` (case (ii): `H = 2^(2^25 + 5)`, `A = 3`: `OK`; `A = 1`, `d = 3`:
`LIMIT`), and the vectors of lane f-slice1 (`set_fball` lines), unchanged.