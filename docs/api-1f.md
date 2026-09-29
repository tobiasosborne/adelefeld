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
   only if an INPUT or the RESULT is outside the limits, never because of an intermediate value. "The result is
   outside" means: its `v` or `N` is beyond `ADF_LBALL_EXP_MAX`, or its stored centre needs `p^k` with
   `k bits(p) > ADF_LBALL_BITS_MAX`. Examples where `LIMIT` is right: `neg` of `1 + 5^E Z_5` (centre `5^E - 1`),
   `inv` of `3 + 5^E Z_5` (centre `1/3 mod 5^E`), `(1 + 5^E Z_5) / (3 + 5^E Z_5)` (the same centre), the exact sum
   `5^E + 5^(-E)`. Examples where `OK` is required although a natural intermediate is outside: `inv` of the exact
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
