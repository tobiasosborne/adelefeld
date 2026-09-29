# Proofs for milestone S: reconstruction from a residue, linear systems modulo N, roots

Status: written by lane `s-design` on 2026-09-29; reviewed by a refute review (`docs/reviews/s-design/review.md`,
verdict NOT READY before the repairs); repaired by lane `s-design-repair` on 2026-09-29, see "Review record" at the
end. Checked numerically by `proto/solvers_checks.py`, which also holds the reference algorithms. Covers `SPEC.md`
9.1 (solving) and 9.2, second item (reconstruction from partial data); `PLAN.md` section 6, milestone S, work
packages S.3, S.1, S.2. The interface that implements these statements is `docs/api-s.md`. The repaired file has
not been reviewed again.

Order of the sections: 1 is S.3, 2 is S.1, 3 is S.2, as the work packages are worked.

## 0. Notation and what is used

`Z/N` is the ring of integers modulo `N >= 1`; for `N = 1` it is the zero ring. An element of `Z/N` is written by
its representative in `[0, N)`. `gcd(a, 0) = |a|`, so `gcd(0, d) = d`; a fraction `n/d` with `d > 0` is
*reduced* when `gcd(n, d) = 1`, and the only reduced fraction with `n = 0` is `0/1`. `v_p` is the valuation at
the prime `p`, `|x|_p = p^(-v_p(x))`. `Zhat`, `Z_p` and finite balls are as in `SPEC.md` 2 and 4.1. For a real
`x`, `floor(x)` and `ceil(x)` are the integer parts. `len` of a matrix or vector is not used; sizes are named.

From `proofs/precision.md`: Lemma 1 (`R Zhat ∩ Q = R Z`, line 13). From `proofs/quotient.md`: Proposition 11
(line 257) and Proposition 13 (line 310).

Standard theorems used and not proved here. Each was read in the file named (cited as `key:file:line` against
`refs/src/`); the statement is given in the notation of this file.

- (EEA) Shoup, Theorem 4.3, `shoup-ntb:ntb-v2.txt:3641` to `3654`, proof `3655` to `3686`. For integers
  `a >= b >= 0` let `r_0 = a`, `r_1 = b`, `r_(i+1) = r_(i-1) - q_i r_i` with `q_i = floor(r_(i-1)/r_i)` be the
  remainder sequence, ending with `r_(l+1) = 0`, and `s_0 = 1, t_0 = 0, s_1 = 0, t_1 = 1`,
  `s_(i+1) = s_(i-1) - q_i s_i`, `t_(i+1) = t_(i-1) - q_i t_i`. Then (i) `a s_i + b t_i = r_i` for
  `0 <= i <= l + 1`; (ii) `s_i t_(i+1) - t_i s_(i+1) = (-1)^i` for `0 <= i <= l`; (iii) `gcd(s_i, t_i) = 1`;
  (iv) `t_i t_(i+1) <= 0` and `|t_i| <= |t_(i+1)|` for `0 <= i <= l`. The remainders satisfy
  `r_0 >= r_1 > r_2 > ... > r_l > r_(l+1) = 0` (Theorem 4.1 of the same text, `ntb-v2.txt:3498` to `3519`).
  Since `t_1 = 1`, (iv) gives `|t_i| >= 1` for `i >= 1`, and then the sign of `t_i` is `(-1)^(i-1)`.
- (EEA-cost) Shoup, Theorem 4.4, `shoup-ntb:ntb-v2.txt:3711`: the extended Euclidean algorithm on `a >= b >= 0`
  runs in time `O(len(a) len(b))`, `len` the number of bits.
- (Thue) Shoup, Theorem 2.33, `shoup-ntb:ntb-v2.txt:2184` to `2186`, proof `2188` to `2195`. Let `n, b, r*, t*`
  be integers with `0 < r* <= n < r* t*`. Then there are integers `r, t` with `r = b t` modulo `n`, `|r| < r*`,
  `0 < |t| < t*`.
- (Iso) Shoup, Theorem 6.23 (first isomorphism theorem), `shoup-ntb:ntb-v2.txt:6626`, and Theorem 6.17,
  `ntb-v2.txt:6360`: for a homomorphism `rho` of a finite abelian group `G`, with kernel `K` and image `H'`,
  `G/K` is isomorphic to `H'` and `|G/K| = |G|/|K|`. So `|G| = |K| |H'|`.
- (FAb) Shoup, Theorem 6.45, `shoup-ntb:ntb-v2.txt:7283` to `7288`, proved there in Lemmas 6.46 and 6.47: a
  finite abelian group with more than one element is isomorphic to a product `Z/m_1 x ... x Z/m_t` of cyclic
  groups with every `m_i > 1`.
- (Roots) Shoup, Theorem 7.13, `shoup-ntb:ntb-v2.txt:8033`: over an integral domain, distinct elements
  `x_1, ..., x_k` are roots of `g` exactly when `(X - x_1) ... (X - x_k)` divides `g`; Theorem 7.14,
  `ntb-v2.txt:8056`: a polynomial of degree `k >= 0` over an integral domain has at most `k` distinct roots;
  Theorem 2.14 (Fermat), `ntb-v2.txt:1838`: `a^p = a` for every `a` in `Z/p`, `p` prime.
- (EEA-poly) Shoup, Theorem 17.4, `shoup-ntb:ntb-v2.txt:20462` to `20470`, and the extended Euclidean algorithm
  for polynomials, `ntb-v2.txt:20510` to `20521`. For polynomials `g`, `h` over a field `F` (`Q` or `F_p`), with
  `g` not zero, the remainder sequence gives `d = gcd(g, h)`, monic, and polynomials `s`, `t` in `F[X]` with
  `g s + h t = d`.
- (H1) Conrad, Theorem 2.1, `conrad-hensel:hensel.txt:31` to `33`, proof `37` to `110`. If `f` is in `Z_p[X]`
  and `a` in `Z_p` satisfies `f(a) = 0` modulo `p` and `f'(a)` is not `0` modulo `p`, there is a unique `alpha`
  in `Z_p` with `f(alpha) = 0` and `alpha = a` modulo `p`. Every prime `p`, 2 included.
- (H2) Conrad, Theorem 4.1, `conrad-hensel:hensel.txt:315` to `319`. If `f` is in `Z_p[X]` and `a` in `Z_p`
  satisfies `|f(a)|_p < |f'(a)|_p^2`, there is a unique `alpha` in `Z_p` with `f(alpha) = 0` and
  `|alpha - a|_p < |f'(a)|_p`; moreover `|alpha - a|_p = |f(a)/f'(a)|_p` and `|f'(alpha)|_p = |f'(a)|_p`. Every
  prime `p`, 2 included. The source proves it twice (`hensel.txt:329`); the first proof, by Newton's method,
  begins at `hensel.txt:433`.
- (Taylor) Conrad, identity (2.1), `conrad-hensel:hensel.txt:53` to `66`: for `f` in `Z_p[X]` there is `g` in
  `Z_p[X, Y]` with `f(X + Y) = f(X) + f'(X) Y + g(X, Y) Y^2`. For `f` in `Z[X]` the same computation gives `g`
  in `Z[X, Y]`.
- (IVT) The intermediate value theorem: a continuous real function on `[lo, hi]` with
  `f(lo) f(hi) < 0` has a zero in the open interval `(lo, hi)`. `[source pending: a text of real analysis with
  the intermediate value theorem; it is used in `milne-cft:CFT.txt:1426` and named in
  `thorne-padic:jackthornenotes.txt:144`, stated in neither]`
- (Sturm) Used only through FLINT's promise, see 3.9. `[source pending: a text with Sturm's theorem]`

What FLINT 3.0.1 promises is quoted where it is used, from `flint-3.0.1` (documentation) and
`flint-src-3.0.1` (C sources).

## 1. Partial rational reconstruction (S.3)

### Definition 1.1 (the problem and its set of solutions).

Input: integers `m >= 1`, `c`, `A >= 0`, `B >= 1`. A *solution* is a pair of integers `(n, d)` with

    d > 0,  gcd(n, d) = 1,  gcd(d, m) = 1,  n = c d modulo m,  |n| <= A,  d <= B.

`Sol(m, c, A, B)` is the set of solutions. It is finite, with at most `(2 A + 1) B` elements, and depends on `c`
only through `c` modulo `m`. The *lattice* of the problem is `L(m, c) = {(n, d) in Z^2 : n = c d modulo m}`, and
the *box* is `{(n, d) : |n| <= A, 1 <= d <= B}`. A solution is a point of the lattice in the box with
`gcd(n, d) = 1` and `gcd(d, m) = 1`.

The sign convention is that of `SPEC.md` 9.2 and of FLINT (`flint-3.0.1:fmpq.rst:564`, `0 < d <= D`): the
denominator is positive and the sign is on `n`. Shoup states the problem with `0 < |t| <= t*`
(`shoup-ntb:ntb-v2.txt:4161`); his pair `(r, t)` with `t < 0` is our `(-r, -t)`.

### Lemma 1.2 (the condition `gcd(d, m) = 1` follows; the local meaning).

Hypotheses: `m >= 1`, `c` an integer, `(n, d)` integers with `d > 0` and `gcd(n, d) = 1`. Claim:

1. If `n = c d` modulo `m` then `gcd(d, m) = 1`.
2. `n = c d` modulo `m` holds exactly when `v_p(n/d - c) >= v_p(m)` for every prime `p` dividing `m` (with
   `v_p(0)` counted as infinite).
3. If `(n', d')` are integers, `d' > 0`, `gcd(d', m) = 1` and `n' = c d'` modulo `m`, and `g = gcd(n', d')`, then
   `(n'/g, d'/g)` satisfies the same two conditions and is reduced.

*Proof.*
1. Let `g = gcd(d, m)`. Then `g` divides `m`, and `m` divides `n - c d`, so `g` divides `n - c d`; `g` divides
   `d`, so `g` divides `n`. So `g` divides `gcd(n, d) = 1`.
2. "Only if": by 1, `p` does not divide `d`. Write `n - c d = k m`. Then `n/d - c = k m/d` and
   `v_p(n/d - c) = v_p(k) + v_p(m) >= v_p(m)`. "If": let `p` divide `m`. If `p` divided `d`, then `p` would not
   divide `n`, so `v_p(n/d) < 0 <= v_p(c)` and `v_p(n/d - c) = v_p(n/d) < 0 < v_p(m)`, against the hypothesis.
   So no prime of `m` divides `d`, and `v_p(n - c d) = v_p(n/d - c) + v_p(d) >= v_p(m)` for every prime `p` of
   `m`. So `m` divides `n - c d`.
3. `g` divides `d'`, so `gcd(g, m) = 1` and `gcd(d'/g, m) = 1`. `m` divides `n' - c d' = g (n'/g - c d'/g)` and
   is prime to `g`, so `m` divides `n'/g - c d'/g`.

Consequence: the condition `gcd(d, m) = 1` may be dropped from Definition 1.1 without changing `Sol`. FLINT's
statement of the problem (`flint-3.0.1:fmpq.rst:562` to `565`) has no such condition and describes the same set.

Check: `check_s3_coprime`.
Used by: `SPEC.md` 9.2 (second item, "a reduced fraction `n/d` with `gcd(d, m) = 1`").

### Definition 1.3 (certificate pair).

For `m >= 1`, an integer `c` and `0 <= A < m`, a *certificate pair* is a quadruple of integers
`(R', T', R, T)` with

- (C1) `R = c T` modulo `m` and `R' = c T'` modulo `m`;
- (C2) `|T R' - T' R| = m`;
- (C3) `0 <= R <= A < R'`;
- (C4) `T` is not zero and `T T' <= 0`.

The four conditions are tested with four multiplications, two reductions modulo `m` and comparisons.
`sigma` denotes the sign of `T`. By (C4), `T = sigma |T|` and `T' = - sigma |T'|`, so by (C3)
`T R' - T' R = sigma (|T| R' + |T'| R)`, and (C2) reads

    |T| R' + |T'| R = m.                                                                         (1.3.1)

### Lemma 1.4 (the Euclidean algorithm gives a certificate pair).

Hypotheses: `m >= 1`, `c` an integer, `0 <= A < m`. Let `b = c mod m` in `[0, m)`, let `(r_i, s_i, t_i)` be the
sequence of (EEA) for `(a, b) = (m, b)`, and let `j` be the smallest index with `r_j <= A`. Claim:

1. `j` exists and `j >= 1`;
2. `(R', T', R, T) = (r_(j-1), t_(j-1), r_j, t_j)` is a certificate pair;
3. for every certificate pair, `gcd(R, T) = gcd(T, m)`.

*Proof.*
1. `r_0 = m > A` and `r_(l+1) = 0 <= A`.
2. (C3): `r_j <= A` by the choice of `j`, `r_(j-1) > A` by its minimality, `r_j >= 0`. (C1): by (EEA)(i),
   `r_i = m s_i + b t_i`, so `r_i = b t_i = c t_i` modulo `m`. (C2): by (EEA)(i),
   `t_j r_(j-1) - t_(j-1) r_j = t_j (m s_(j-1) + b t_(j-1)) - t_(j-1) (m s_j + b t_j)
   = m (s_(j-1) t_j - t_(j-1) s_j)`, which is `m (-1)^(j-1)` by (EEA)(ii). (C4): `t_(j-1) t_j <= 0` by
   (EEA)(iv); `|t_j| >= |t_1| = 1` by (EEA)(iv) applied `j - 1` times.
3. Let `g = gcd(R, T)`. By (C2), `m = |T R' - T' R|` is divisible by `gcd(R, T) = g`. So `g` divides
   `gcd(T, m)`. Conversely let `h = gcd(T, m)`. By (C1), `m` divides `R - c T`, so `h` divides `R - c T`, and
   `h` divides `T`, so `h` divides `R`. So `h` divides `gcd(R, T) = g`.

Remark. Nothing below uses the Euclidean algorithm: Propositions 1.5 and 1.6 hold for every quadruple that
satisfies (C1) to (C4). An implementation may therefore compute the pair by any method (word-size steps,
Lehmer's method, a routine of FLINT) and a checker tests (C1) to (C4); the proof of the answer then does not
depend on the method.

Check: `check_s3_certificate` (the four conditions and claim 3 for all `m < 200` and for random `m` up to 1000
bits; changed quadruples are refused or give the same set).
Used by: `SPEC.md` 9.2.

### Proposition 1.5 (the points of the lattice in the box).

Hypotheses: `m >= 1`, `c` an integer, `0 <= A < m`, `B >= 1`, and a certificate pair `(R', T', R, T)` for
`(m, c, A)`, with `sigma` the sign of `T`. Let

    I = {(x, y) in Z^2 : x >= 1, y >= 0, |x R - y R'| <= A, x |T| + y |T'| <= B},
    phi(x, y) = (sigma (x R - y R'),  x |T| + y |T'|).

Claim:

1. `phi` is a bijection from `I` onto the set of points of `L(m, c)` in the box.
2. For `(n, d) = phi(x, y)`: `n |T| - sigma R d = - sigma y m`.
3. For each `x` there are at most two `y` with `(x, y)` in `I`, namely among the integers of the interval
   `[(x R - A)/R', (x R + A)/R']`.
4. Every point of `L(m, c)` in the box has `d >= |T|`. If `|T| > B` the box contains no point of the lattice.
   In general `x <= floor(B/|T|)`, and the box contains at most `2 floor(B/|T|)` points of the lattice.

*Proof.* Write `u = (R, T)` and `u' = (R', T')`.
1. *`L(m, c)` is generated by `(m, 0)` and `(c, 1)`.* Both are in it. If `n - c d = k m` then
   `(n, d) = k (m, 0) + d (c, 1)`.
2. *`u` and `u'` generate `L(m, c)`.* By (C1) they are in it: `u = a (m, 0) + T (c, 1)` and
   `u' = a' (m, 0) + T' (c, 1)` with integers `a = (R - c T)/m`, `a' = (R' - c T')/m`. Then
   `R T' - R' T = (a m + c T) T' - (a' m + c T') T = m (a T' - a' T)`. By (C2) and `m >= 1`,
   `a T' - a' T` is `1` or `-1`. So the integer matrix with rows `(a, T)` and `(a', T')` has an integer inverse,
   and `(m, 0)`, `(c, 1)` are integer combinations of `u` and `u'`. With step 1 the claim follows. The
   determinant `R T' - R' T` is not zero, so the coefficients `(mu, nu)` of a point `mu u + nu u'` are unique.
3. *The points of the box.* Let `P = (n, d) = mu u + nu u'` be a point of the lattice with `|n| <= A` and
   `d >= 1`. Three cases.
   - `nu = 0`. Then `d = mu T >= 1`, so `mu = sigma x` with `x = |mu| >= 1`, and `P = phi(x, 0)`.
   - `nu` not zero and `mu nu >= 0`. By (C3) `R >= 0` and `R' > 0`, and `mu`, `nu` do not have opposite signs,
     so `|n| = |mu R + nu R'| = |mu| R + |nu| R' >= R' > A`. This contradicts `|n| <= A`. The case does not
     occur.
   - `mu nu < 0`. By (C4), `(mu T)(nu T') = (mu nu)(T T') >= 0`, and `mu T` is not zero. If `mu T < 0` then
     `nu T' <= 0` and `d = mu T + nu T' < 0`, against `d >= 1`. So `mu T > 0`: `mu = sigma x` with
     `x = |mu| >= 1`, and `nu = - sigma y` with `y = |nu| >= 1`. Then `n = sigma (x R - y R')` and
     `d = x |T| - sigma y T' = x |T| + y |T'|`, because `T' = - sigma |T'|`. So `P = phi(x, y)`.
   In both cases that occur, `(x, y)` is in `I` because `P` is in the box.
4. *Conversely* `phi(x, y) = (sigma x) u + (- sigma y) u'` by the computation of step 3, so it is a point of
   the lattice, and it is in the box by the definition of `I` (`d >= x |T| >= 1`). `phi` is injective by the
   uniqueness of the coefficients (step 2). This proves claim 1.
5. *Claim 2.* `n |T| - sigma R d = sigma (x R - y R') |T| - sigma R (x |T| + y |T'|)
   = - sigma y (R' |T| + R |T'|) = - sigma y m` by (1.3.1).
6. *Claim 3.* `|x R - y R'| <= A` means `x R - A <= y R' <= x R + A`. The interval for `y` has length
   `2 A/R' < 2` by (C3), so it contains at most two integers.
7. *Claim 4.* `d = x |T| + y |T'| >= x |T| >= |T|`. So `x <= d/|T| <= B/|T|`, and the rest follows from claim 3.

Claim 4 contains Shoup's Theorem 4.9(i) (`shoup-ntb:ntb-v2.txt:4202`: if the problem has a pair within the
bounds, the row of the Euclidean algorithm has `|t_j| <= t*`), with no hypothesis on `2 A B`. The case
distinction of step 3 is that of Shoup's proof (`ntb-v2.txt:4222` to `4232`).

Check: `check_s3_param` (every `m <= 40`, every `c`, every `A < m`, eight values of `B`; the list of `phi` is
compared with the enumeration of the box, as a set and for repetitions).
Used by: `SPEC.md` 9.2; Proposition 1.6.

### Proposition 1.6 (the complete answer).

Hypotheses: `m >= 1`, `c` an integer, `A >= 0`, `B >= 1`; `c_0 = c mod m` in `[0, m)`. In (b), (c), (d):
`A < m` and `(R', T', R, T)` is a certificate pair with sign `sigma`. Claim:

- (a) If `A >= m`, then `(c_0, 1)` and `(c_0 - m, 1)` are two different solutions. There are several.
- (b) If `A < m` and `|T| > B`, there is no solution.
- (c) If `A < m`, `|T| <= B` and `2 A B < m`: if `gcd(R, T) = 1` the only solution is `(sigma R, |T|)`; if
  `gcd(R, T) > 1` there is no solution.
- (d) If `A < m` and `|T| <= B`: `Sol(m, c, A, B) = {phi(x, y) : (x, y) in I, gcd(phi(x, y)) = 1}`, with `I`
  and `phi` as in Proposition 1.5. It has at most `2 floor(B/|T|)` elements and is found by
  `floor(B/|T|)` steps of the enumeration of 1.7.

*Proof.*
- (a) Both pairs have `d = 1 <= B`, `gcd(n, 1) = 1`, `gcd(1, m) = 1`, and `n = c` modulo `m`. `0 <= c_0 <= m - 1
  < A` and `-m <= c_0 - m <= -1`, so `|c_0 - m| <= m <= A`. They differ because `m >= 1`.
- (d) By Lemma 1.2(1) a solution is a point `(n, d)` of the lattice in the box with `gcd(n, d) = 1`, and every
  such point is a solution. By Proposition 1.5(1) these points are the `phi(x, y)`, and by 1.5(4) there are at
  most `2 floor(B/|T|)` of them.
- (b) Proposition 1.5(4).
- (c) Let `(n, d) = phi(x, y)` be a point of the lattice in the box. By Proposition 1.5(2),
  `y m = |n |T| - sigma R d| <= |n| |T| + R d <= A B + A B < m`, using `|T| <= B`, `R <= A`. So `y = 0` and
  `(n, d) = x (sigma R, |T|)`, with `gcd(n, d) = x gcd(R, T)`. This is 1 exactly when `x = 1` and
  `gcd(R, T) = 1`. The point `phi(1, 0) = (sigma R, |T|)` is in the box because `R <= A` and `|T| <= B`. With
  (d) the claim follows.

Remarks.
1. In case (c) with `gcd(R, T) > 1` the row is not reduced, and by Lemma 1.4(3) its denominator is not prime
   to `m`. Dividing the row by `gcd(R, T)` does not give a solution: it gives a point that is not in the
   lattice. Example: `m = 12`, `c = 6`, `A = 1`, `B = 5`: the pair is `(6, 1, 0, -2)`, the row is `0/2`,
   `gcd = 2 = gcd(2, 12)`; the reduced fraction `0/1` has `0 - 6 * 1 = -6`, not divisible by 12; there is no
   solution.
2. (c) is `proofs/quotient.md` Proposition 13(1) together with the existence or absence of the one candidate.
3. Cases (b) and (d) need no hypothesis on `2 A B`. For `m <= 2 A B` the set is still finite and is computed
   exactly; what the function returns is decided in 1.7.

Check: `check_s3_complete` (every `m <= 36`, `c` inside and outside `[0, m)`, `A` from 0 to `2 m`, eight values
of `B`: status and solutions against the enumeration of Definition 1.1); `check_s3_edge`.
Used by: `SPEC.md` 9.2 (second item and "Results").

### Algorithm R and Proposition 1.7 (what the function returns; "uniqueness not certified").

Algorithm R. Input: integers `m`, `c`, `A`, `B` and a search limit `limit` (any integer; `ell = max(limit, 0)`
is used).

1. If `m < 1`: invalid input (`DOMAIN`; the type `adf_resid` of `api-s.md` excludes it). If `A < 0` or
   `B < 1`: the box is empty, return `NO_SOLUTION` (decision S-D5). A `limit` below 0 is taken as 0: `ell` is
   `limit` if `limit >= 0` and 0 otherwise.
2. Replace `c` by `c mod m` in `[0, m)`.
3. If `A >= m`: return `NOT_UNIQUE`.
4. Compute a certificate pair: `(r_0, t_0, r_1, t_1) = (m, 0, c, 1)`; while `r_1 > A`:
   `q = floor(r_0/r_1)`, `(r_0, r_1) = (r_1, r_0 - q r_1)`, `(t_0, t_1) = (t_1, t_0 - q t_1)`. Then
   `(R', T', R, T) = (r_0, t_0, r_1, t_1)`. (Any other method may be used; then (C1) to (C4) are tested.)
5. If `|T| > B`: return `NO_SOLUTION`.
6. If `2 A B < m`: if `gcd(R, T) = 1` return `OK` with `(sigma R, |T|)`, else return `NO_SOLUTION`.
7. Otherwise search. `X = floor(B/|T|)`, `found` empty. For `x = 1, 2, ..., min(X, ell)`: for each integer
   `y` with `max(0, ceil((x R - A)/R')) <= y <= floor((x R + A)/R')`, in increasing order, and
   `d = x |T| + y |T'| <= B`: let `n = sigma (x R - y R')`; if `gcd(n, d) = 1` add `(n, d)` to `found`; if
   `found` has two elements return `NOT_UNIQUE`.
8. If `X > ell` return `NOT_DETERMINED`. Otherwise return `OK` with the element of `found` if there is one,
   and `NO_SOLUTION` if there is none.

Claim:

1. The loop of step 4 ends and gives the pair of Lemma 1.4.
2. `OK` is returned only if `Sol` has exactly one element, which is returned; `NO_SOLUTION` only if `Sol` is
   empty; `NOT_UNIQUE` only if `Sol` has at least two elements.
3. `NOT_DETERMINED` is returned exactly when all four of the following hold: `A < m <= 2 A B`; `|T| <= B`;
   `floor(B/|T|) > ell`; and among the points `phi(x, y)` with `x <= ell` fewer than two are reduced. It is
   never returned when `2 A B < m`, when `A >= m`, when `|T| > B`, or when `ell >= B`. `NOT_UNIQUE` is returned
   as soon as two reduced points are found, also when `floor(B/|T|) > ell` and the search would be cut: a cut
   search that has found two solutions is decided. With `ell = 0` (that is, `limit <= 0`) step 7 does nothing:
   `A >= m` gives `NOT_UNIQUE` (case (a) of Proposition 1.6), `|T| > B` gives `NO_SOLUTION` (case (b)),
   `2 A B < m` gives the answer of case (c), and every other problem, that is `A < m <= 2 A B` with
   `|T| <= B`, gives `NOT_DETERMINED`. So with `limit = 0` the status is NOT "`NOT_DETERMINED` exactly when
   `2 A B >= m`": for `m = 2`, `c = 1`, `A = 2`, `B = 1` the function returns `NOT_UNIQUE` (`A >= m`), and a
   problem with `2 A B >= m` and `|T| > B` gives `NO_SOLUTION`. (`check_s3_limit` counts 6210 problems with
   `limit = 0` that refute the wrong reading.)
4. Cost: step 4 is one run of the extended Euclidean algorithm, `O(len(m)^2)` bit operations by (EEA-cost);
   step 7 makes at most `min(X, ell)` rounds, each with a constant number of multiplications and divisions
   and at most two gcds of integers bounded by `max(A B, m)`.

*Proof.*
1. While `r_1 > A >= 0` the division is defined and `r_1` decreases strictly, so the loop ends. The values are
   those of (EEA) for `(m, c)`: the loop stops at the first index with `r_j <= A`.
2. Steps 3, 5, 6 are Proposition 1.6 (a), (b), (c). In step 7 every element of `found` is a solution by 1.6(d),
   and two different `(x, y)` give different points by 1.5(1); so `NOT_UNIQUE` is right. If `X <= ell` the
   loop has visited every `x <= floor(B/|T|)`, and for each `x` every `y` admitted by `I`: the bounds on `y`
   are those of 1.5(3) with `y >= 0`, and `d` increases with `y`, so the loop over `y` may stop at the first
   `d > B`. So `found` is all of `Sol` by 1.6(d).
3. The function reaches step 8 with `X > ell` exactly under the stated conditions, and reaches step 8 only if
   fewer than two reduced points were found in the rounds `x <= ell` (else step 7 returned `NOT_UNIQUE`); the
   points found are exactly the reduced points of those rounds (2). `X <= B`, so `ell >= B` excludes it. For
   `ell = 0` the four conditions reduce to `A < m <= 2 A B` and `|T| <= B`, since `X >= 1` then.
4. Read off the steps; `x R <= X A <= A B`, `y R' <= x R + A`, `d <= B`.

**What "uniqueness not certified" means.** It is the status `NOT_DETERMINED` of claim 3: the bounds admit more
than one solution (`m <= 2 A B`), the caller's limit did not allow the complete search, and the part searched
holds at most one solution. It says nothing about existence. It is not returned on the ground `m <= 2 A B`
alone: in that range the function still returns `NO_SOLUTION` when `|T| > B`, `NOT_UNIQUE` when two solutions
are found, and the exact answer when `floor(B/|T|) <= ell`.

Check: `check_s3_complete` (limit large), `check_s3_limit` (limits -1, 0, 1, 2, 5, 329875 calls: `NOT_DETERMINED`
exactly under the four conditions of claim 3, with the reduced points of the rounds `x <= ell` recounted from the
enumeration of Definition 1.1 and Cramer's rule, not from the loop; every other status is the true one;
`NOT_UNIQUE` found inside a cut search occurs 51291 times), `check_s3_edge`. The first version of the check tested
only necessary conditions; a solver that returned `NOT_DETERMINED` where two solutions had been found survived it
(reviewer's mutant, `docs/reviews/s-design/checks/mutation_checks.py`); it is now killed.
Used by: `SPEC.md` 9.2 ("Results"); `conventions.md` 6.8 (CV-51).

### Proposition 1.8 (in which ranges the answer can be "none").

Hypotheses: `m >= 1`, `c` an integer, `A >= 0`, `B >= 1`. Claim:

1. If `2 A B < m` there is at most one solution. Both "none" and "one" occur.
2. If `A >= m` there are at least two solutions. "None" and "one" do not occur.
3. (An obstruction that no `B` removes.) If `m` does not divide `c` and `A < gcd(c, m)`, there is no solution,
   whatever `B` is. So for `A < m` "none" occurs for arbitrarily large `B`: `Sol(4, 2, 1, B)` is empty for
   every `B`, in the range `2 A B < m` (`B = 1`), in the range `A B < m <= 2 A B` (`B = 2`) and in the range
   `m <= A B` (`B >= 4`).
4. (Existence.) If `A < m < (A + 1)(B + 1)` and every prime factor of `m` is greater than `B`, there is at
   least one solution.
5. The condition on the prime factors cannot be dropped, even for `c` prime to `m`: `Sol(8, 3, 2, 2)` is empty
   although `2 < 8 < 9`.
6. `2 A B = m` admits two solutions: `Sol(2, 1, 1, 1) = {1/1, -1/1}`.

*Proof.*
1. Proposition 1.6 (b), (c) for `A < m`; for `A >= m`, `2 A B >= 2 m > m`. `Sol(4, 2, 1, 1)` is empty by
   claim 3, and `Sol(3, 1, 1, 1) = {1/1}`: `d = 1`, `n` in `{-1, 0, 1}`, `n = 1` modulo 3.
2. Proposition 1.6(a).
3. Let `(n, d)` be a solution. `n = c d` modulo `m` gives `gcd(n, m) = gcd(c d, m)`, and `gcd(d, m) = 1` gives
   `gcd(c d, m) = gcd(c, m)`. If `n = 0` then `gcd(c, m) = gcd(0, m) = m`, so `m` divides `c`, which is
   excluded. So `n` is not zero and `|n| >= gcd(n, m) = gcd(c, m) > A`, against `|n| <= A`. For the example,
   `gcd(2, 4) = 2 > 1`.
4. Apply (Thue) with `n = m`, `b = c`, `r* = A + 1`, `t* = B + 1`: the hypotheses `0 < A + 1 <= m` and
   `m < (A + 1)(B + 1)` hold. It gives integers `r, t` with `r = c t` modulo `m`, `|r| <= A`,
   `0 < |t| <= B`. Replace `(r, t)` by `(-r, -t)` if `t < 0`. A prime dividing `gcd(t, m)` would be a prime
   factor of `m` that is at most `t <= B`; there is none. So `gcd(t, m) = 1`, and by Lemma 1.2(3) the reduced
   pair `(r/g, t/g)`, `g = gcd(r, t)`, is a solution; its entries are within the bounds.
5. `d` must be prime to 8 and at most 2, so `d = 1`, and `n = 3` modulo 8 with `|n| <= 2` does not exist.
6. `proofs/quotient.md` Proposition 13(2).

Remark. Thue's lemma gives a point of the lattice in the box, not a solution: the point may fail to be
reduced, and then no reduced multiple of it need be in the lattice. In the example of claim 5 the lattice
contains `(-2, 2)`, which is in the box.

Check: `check_s3_ranges` (every `m <= 40`, every `c`, `A` from 0 to `m + 2`, `B` from 1 to 11: the number of
problems with none, one, several in each range; claims 1 to 6 on every problem).
Used by: `SPEC.md` 9.2.

### Proposition 1.9 (what FLINT 3.0.1 promises and what the implementation calls).

Read: `flint-3.0.1:fmpq.rst:556` to `567` (contract), `flint-src-3.0.1:fmpq/reconstruct_fmpz_2.c:923` to `1062`
(the function), `:259` to `308` (the path for a modulus of one limb), and
`flint-src-3.0.1:fmpq/reconstruct_fmpz_2_naive.c:18` to `72`.

1. (Contract.) The documentation asks for `m > 2`, `0 <= a < m`, positive `N`, `D` with `2 N D < m`, and
   promises a fraction with `|n| <= N`, `0 < d <= D`, `gcd(n, d) = 1`, `n = a d` modulo `m`, return value 1 if
   there is one and 0 if there is none. It promises nothing outside these hypotheses.
2. (Proved from the source, naive version and one-limb path.) For `m >= 1`, `0 <= a < m`, `1 <= N < m`,
   `D >= 1`, the function `_fmpq_reconstruct_fmpz_2_naive` returns 1 exactly when the row `(sigma R, |T|)` of
   Lemma 1.4 (with `c = a`, `A = N`) has `|T| <= D` and `gcd(R, T) = 1`, and then the outputs are that row.
   The same holds for `_fmpq_reconstruct_fmpz_2` when `m` has one limb.
3. (Consequence, with Proposition 1.6.) For these functions and arguments: return value 1 means that the
   outputs are a solution. Return value 0 means "no solution" when `2 N D < m`, and also whenever the row has
   `|T| > D`; when `m <= 2 N D` and the row is within `D` and not reduced, return value 0 says nothing, and
   solutions may exist. Return value 1 with `m <= 2 N D` says nothing about other solutions.
4. (Not proved here.) The paths of `_fmpq_reconstruct_fmpz_2` for a modulus of two limbs or more
   (`_fmpq_reconstruct_fmpz_2_uiui`, `_ui_array`, `_lehmer`, `_split`, about 600 lines) are not read line by
   line. FLINT itself compares them with the naive version only when built with assertions
   (`reconstruct_fmpz_2.c:930` to `937`, `1050` to `1057`).
5. (State of the outputs.) The function writes its outputs before it decides: `n = a - m` at
   `reconstruct_fmpz_2.c:949` before the second shortcut is tested, and `n`, `d` at `:1026`, `:1027` in the
   general path. After return value 0 the outputs may hold values that are not a solution (probe:
   `a = 2`, `m = 5`, `N = D = 1` returns 0 and leaves `n = -3`).
6. (Arguments outside the contract.) `N = 0` violates the assertion `fmpz_sgn(N) > 0`
   (`reconstruct_fmpz_2.c:981`), which is compiled only with assertions; without them the behaviour is not
   specified. `m <= 2` is outside the documented contract.

*Proof of 2.* Naive version: lines 26 to 37 return `(a, 1)` if `a <= N`, else `(a - m, 1)` if `m - a <= N`. In
the first case `r_1 = a <= N`, so `j = 1` and the row is `(r_1, t_1) = (a, 1)`. In the second case
`a > N >= m - a` gives `a > m/2`, so `q_1 = 1`, `r_2 = m - a <= N`, `t_2 = t_0 - t_1 = -1`, `j = 2`, and the row
with positive denominator is `(a - m, 1)`. Both have `gcd = 1` and `d = 1 <= D`. Otherwise lines 44 to 52 run
the recurrence of (EEA) on `(r, n) = (m, a)` and `(s, d) = (0, 1)` until `|n| <= N`, which is the loop of
Algorithm R, step 4, with `n = r_j`, `d = t_j`. Lines 54 to 58 make `d` positive, lines 60 to 64 test `d <= D`
and `gcd(n, d) = 1`. One-limb path: after the same two shortcuts, lines 270 to 285 run the recurrence on
`(A, B) = (r_(i-1), r_i)`, starting at `i = 1` with `m11 = 1 = |t_1|`, `m12 = 0 = |t_0|`, `mdet = 1`. One
step sets `t = m12 + m11 Q`, which is `|t_(i+1)| = |t_(i-1)| + q_i |t_i|` (true because `t_(i-1)` and `t_i`
do not have the same sign, (EEA)(iv)), and changes the sign of `mdet`. So after `k` steps `B = r_(1+k)`,
`m11 = |t_(1+k)|` and `mdet = (-1)^k`, which is the sign of `t_(1+k)` by (EEA). The loop is entered with
`r_1 = a > N` and runs until `B <= N`, so it stops at the index `j` of Lemma 1.4. Lines 289 to 290 return 0 if
`|t_j| > D`; lines 292 to 297 write `n = r_j` if `mdet > 0` and `n = - r_j` otherwise, and `d = |t_j|`: the
row with positive denominator; lines 299 to 307 return whether `gcd(r_j, |t_j|) = 1` (for `r_j = 0` whether
`|t_j| = 1`).

**Decision proposed (S-D2 in `api-s.md`).** The implementation runs its own loop (Algorithm R, step 4). It needs
both rows of the pair for the cases (b) and (d) of Proposition 1.6 and for the certificate; FLINT returns one
row and no reason for a return value 0. A call of `fmpq_reconstruct_fmpz_2` is admitted only as a fast path
when `2 A B < m`, `m > 2`, `A >= 1`, with outputs in temporaries; return value 1 is accepted after the four
tests `0 < d <= B`, `|n| <= A`, `gcd(n, d) = 1`, `n = c d` modulo `m` (then Proposition 1.6(c) gives
uniqueness); after return value 0 the own loop runs.

Check: `probe_s3_flint` (every `3 <= m <= 60`, every `a`, seven values of `N`, six of `D`, inside and outside
`2 N D < m`: claim 2 and claim 3; it counts the calls outside the contract in which FLINT returns 0 although
solutions exist, and those in which it returns one of several solutions).
Used by: `SPEC.md` 9.2; `PLAN.md` section 7 ("Agreement with FLINT ... only where our path does not call the
same routine").

### Proposition 1.10 (the residue is not the adelic ball; the type of the input).

Hypotheses: `m >= 1`, `c` an integer. Let `P(m, c)` be the set of rationals `n/d` (reduced, `d > 0`) with
`gcd(d, m) = 1` and `n = c d` modulo `m`. Claim:

1. `P(m, c)` is the set of rationals `q` with `q` in `c + m Z_p` for every prime `p` dividing `m`. For
   `m = 1` it is `Q`.
2. `(c + m Zhat) ∩ Q = c + m Z`, and this is the set of integers in `P(m, c)`: a rational of `P(m, c)` is in
   the adelic ball `c + m Zhat` exactly when its reduced denominator is 1.
3. `1/5` is in `P(6, 5)` and not in `5 + 6 Zhat`.
4. (Forgetting the other places.) Let `X = (A + H Zhat)/d` be a finite ball with integers `H >= 1`, `d >= 1`,
   `A`, and `gcd(d, H) = 1`, and let `c` be the integer in `[0, H)` with `c d = A` modulo `H`. Then every
   rational of `X` is in `P(H, c)`. The inclusion is proper.

*Proof.*
1. Lemma 1.2(2) and (1): `v_p(q - c) >= v_p(m)` is `q - c` in `m Z_p`.
2. The first equality is `proofs/precision.md` Lemma 1. An integer `n` is the reduced fraction `n/1`, and it is
   in `P(m, c)` exactly when `n = c` modulo `m`. A rational in `c + m Z` is an integer.
3. `1 = 5 * 5` modulo 6 and `gcd(5, 6) = 1`; `1/5` is not an integer. (`proofs/quotient.md` P13(3).)
4. `c` exists and is unique because `d` is invertible modulo `H`. By `proofs/precision.md` Lemma 1 the
   rationals of `X` are the `q = (A + H k)/d` with `k` an integer. Let `p` divide `H`. Then
   `q - c = (A - c d + H k)/d`; `H` divides `A - c d + H k` and `p` does not divide `d`, so
   `v_p(q - c) >= v_p(H)`. By claim 1, `q` is in `P(H, c)`. Proper: let `e >= 2` be an integer prime to `H d`.
   The rational `q' = c + H/e` is in `P(H, c)` by claim 1. If it were in `X`, then `d q' = A + H k`, so
   `d H/e = A - c d + H k` would be an integer; but `e` is prime to `d H` and `e >= 2`.

**The type (decision S-D1 in `api-s.md`).** The input of the partial problem is a value of a type of its own,
`adf_resid`: a pair `(c, m)` with `m >= 1`, `0 <= c < m`, whose meaning is the set `P(m, c)`, that is the ball
`c + m Z_p` at the primes dividing `m` and nothing at the other places. It is a partial ball in the sense of
`SPEC.md` 4.1 whose set of places is not stored, so that no factorisation of `m` is needed. No function of
reconstruction from a residue accepts an `adf_fball`, and `adf_fball_reconstruct` (`recon.h`) accepts no
`adf_resid`. The conversion from a finite ball is a separately named function that forgets the places outside
`m`; it is the only passage between the two problems.

Check: `check_s3_coprime` (claim 1, by valuations), `check_s3_edge` (claims 2 and 3 for `1/5`),
`check_s3_forget` (claim 4).
Used by: `SPEC.md` 9.2 ("A separate function on a separate type").

### Proposition 1.11 (checking a result of Algorithm R).

Hypotheses: `m >= 1`, `c` an integer, `A`, `B` integers, `limit` an integer, `ell = max(limit, 0)`; a status and a
list `S` of pairs `(n, d)`; for `A < m`, `A >= 0`, `B >= 1` a quadruple `(R', T', R, T)`. Claim: the following
tests, made in this order, accept a claim only if it is true.

1. `NOT_UNIQUE` with `S`: accept if `S` has two different pairs and each is a solution (Definition 1.1, tested
   directly: `d > 0`, `d <= B`, `|n| <= A`, `gcd(n, d) = 1`, `n = c d` modulo `m`). Then `Sol` has at least two
   elements. For `A >= m` the pairs `(c_0, 1)` and `(c_0 - m, 1)` of 1.6(a) are such a pair.
2. For `A < m` every other status needs a quadruple that satisfies (C1) to (C4); then Proposition 1.6 applies.
   `NO_SOLUTION`: accept if `|T| > B` (1.6(b)); or if `2 A B < m` and `gcd(R, T) > 1` (1.6(c)); or if the
   enumeration of 1.5(4), which has `floor(B/|T|)` rounds, finds no reduced point (1.6(d)). `OK` with `S = {s}`:
   accept if `s` is a solution and either `2 A B < m`, `|T| <= B` and `s = (sigma R, |T|)` (1.6(c)), or the
   enumeration finds exactly the reduced point `s`. `NOT_DETERMINED` with `S`: accept if `m <= 2 A B`,
   `|T| <= B`, `floor(B/|T|) > ell`, and `S` is the list of the reduced points of the rounds `x <= ell`, at most
   one (Proposition 1.7(3)).
3. A `NOT_UNIQUE` is not certified by a second call with `B` lowered below the first denominator. For
   `m = 2`, `c = 1`, `A = B = 1` the two solutions `1/1` and `-1/1` both have denominator 1, and with `B = 0`
   the box is empty: both are lost. In `check_s3_verify` the second call never finds a second solution (0 of
   4742 `NOT_UNIQUE` problems with `m <= 14`), so the method is not a general one. Two solutions, or the complete
   enumeration, are the certificate.

*Proof.* 1 is Definition 1.1. 2: each accepted case is one of the cases of Propositions 1.6 and 1.7, whose
hypotheses are exactly the tests. For `NOT_DETERMINED`, the list `S` is the set that Algorithm R has found
(1.7, proof of 3), so the tests state the four conditions. 3: the example is computed above.

Check: `check_s3_verify` (93408 results of Algorithm R for `m <= 14` are accepted; 2021284 changed claims, with
another status, another list or a wrong quadruple, are refused or true, the truth being decided by the
enumeration and the four conditions recounted: 0 accepted and false; the `count` of `recon_first` on every
status; the method of note 3 fails 4742 times). It also tests that `sol_brute_fast`, the enumeration used by
the larger checks, is `sol_brute`.
Used by: `api-s.md`, `adf_resid_reconstruct` (checker of a result), decisions S-D3, S-D4.

### Proposition 1.12 (the verifier of the returned status).

Hypotheses: as in Proposition 1.11. The verifier `V` of `adf_resid_verify_result` (reference:
`recon_verify_returned`) is given `(m, c, A, B, limit)`, a status, for `OK` a pair `s = (n, d)`, and for `A < m`
a quadruple. Claim:

1. `V` accepts a claim exactly when the status is the status that Algorithm R returns for the same
   `(m, c, A, B, limit)`, and for `OK` also `s` is the point that Algorithm R returns. The list of a claim of
   another status is not read (the C function reads `q` for `OK` only). The quadruple is read for `OK`,
   `NO_SOLUTION` and `NOT_DETERMINED`, and must satisfy (C1) to (C4); for `NOT_UNIQUE` the pair is computed
   again from `(m, c, A)` and the given one is not read.
2. Soundness: what `V` accepts is true in the sense of 1.11: `OK` with `s`, `NO_SOLUTION` and `NOT_UNIQUE` are
   statements about `Sol`, `NOT_DETERMINED` is the statement of 1.7(3). So `V` accepts a subset of the claims
   that the tests of 1.11 accept (for the list of the result, where 1.11 reads a list).
3. Every result of Algorithm R is accepted.
4. Cost: the same tests as Proposition 1.11, but a search of at most `min(ell, X)` rounds, `X = floor(B/|T|)`,
   `ell = max(limit, 0)`: never the whole of `Sol`. For `limit <= 0` it makes no round.

`V` in order. `m < 1`: accept `DOMAIN` only. `A < 0` or `B < 1`: accept `NO_SOLUTION` only. `NOT_UNIQUE`: accept
if `A >= m` (1.6(a)); else with the own pair `(R', T', R, T)` of Lemma 1.4: refuse if `|T| > B` or `2 A B < m`,
else run the search of step 7 of Algorithm R and accept if it returns `NOT_UNIQUE`. `OK`, `NO_SOLUTION`,
`NOT_DETERMINED`: refuse if `A >= m` or the quadruple does not satisfy (C1) to (C4). `NO_SOLUTION`: accept if
`|T| > B`; else if `2 A B < m` accept iff `gcd(R, T) > 1`; else accept iff the search of step 7 ends with
`NO_SOLUTION`. `OK`: refuse if `s` is not a solution of 1.1 or `|T| > B`; if `2 A B < m` accept iff
`s = (sigma R, |T|)`; else accept iff the search returns `OK` with the point `s`. `NOT_DETERMINED`: refuse if
`2 A B < m`, `|T| > B` or `X <= ell`; else accept iff the search returns `NOT_DETERMINED`. Any other status is
refused. "The search" is steps 7 and 8 of Algorithm R with the same `ell`.

*Proof.* 3 and 1: for `m < 1`, an empty box and `A >= m` the statuses of steps 1 and 3 are unique. For
`A < m` the quadruple of Lemma 1.4 is unique: (C3) fixes `R <= A < R'` and the consecutive rows of (EEA) are
determined by `(m, c)`; so a quadruple that satisfies (C1) to (C4) is the one of step 4, and the tests of 5 and 6
are those of steps 5 and 6. For the rest, `V` runs the search of steps 7 and 8, which is what Algorithm R does
after them, with the same inputs, so the status and the point are the same. The two directions of 1 follow:
Algorithm R returns exactly one status. 2: each accepted case is a case of Proposition 1.6 or 1.7(2), (3),
with the hypotheses that are tested; a `NOT_UNIQUE` is accepted only where the search has found two reduced points
(a solution each, by 1.6(d) and 1.5(1)) or `A >= m`. 4: the tests before the search cost one run of (EEA) and
constant work; the search is bounded by 1.7(4) with `min(X, ell)` rounds.

The case that showed the difference (review `docs/reviews/s13/review.md`, finding 1): `limit = 0`,
`(m, c, A, B) = (10, 1, 2, 5)` and `OK` with `1/1` and the quadruple `(10, 0, 1, 1)`. The claim is true
(`Sol = {1/1}`) and 1.11 accepts it; Algorithm R returns `NOT_DETERMINED` (`2 A B = 20 >= 10`, `|T| = 1 <= 5`,
`X = 5 > 0`), so `V` refuses. The same holds for `NOT_UNIQUE` at `(2, 1, 1, 1)` and `NO_SOLUTION` at
`(4, 2, 1, 2)` (quadruple `(2, 1, 0, -2)`). Finding 2: `(m, c, A, B) = (2, 0, 1, 5 * 10^9)`, `limit = 0`, `OK`
with `0/1`: 1.11 walks `B` rounds; `V` makes none and refuses at once.

Check: `check_s3_verify_returned` (116760 results of Algorithm R for `m <= 14` and the limits -1, 0, 1, 2, 5 are
accepted; 3502800 claims of another status or list: 670450 accepted, all and only those that name the returned
status (and for `OK` the returned point), 0 differences; 0 accepted by `V` that 1.11 refuses, with the list of the
result read for the statuses other than `OK`; 11305 claims that 1.11 accepts and `V` refuses; 98061 changed
quadruples, none accepted unless it satisfies (C1) to (C4); the three inputs of finding 1 refused, and the input
of finding 2 refused in under a second). `check_s3_verify` stays the check of Proposition 1.11.
Used by: `api-s.md` section 2 (`adf_resid_verify_result`), `src/resid.c`, decisions S-D3, S-D18, S-D20.

## 2. Linear systems modulo N (S.1)

Throughout: `N >= 1` is an integer, of any shape (composite, with square factors, `N = 1`); no factorisation of
`N` is used anywhere. `A` is an integer matrix with `r >= 0` rows and `c >= 0` columns, `b` an integer vector
with `r` entries. A vector `x` of `(Z/N)^c` is a column for the product `A x` and is written as a row when it
is a row of a matrix of generators. The system is `A x = b` modulo `N`. Its *kernel* is
`K = {x in (Z/N)^c : A x = 0}`, a submodule; its *image* is `Im A = {A x}`, a submodule of `(Z/N)^r`. The set
of solutions is empty or a coset `x_0 + K`. Only `A` and `b` modulo `N` matter.

### Definition 2.1 (span, echelon form, Howell form over `Z/N`).

For a matrix `M` with `n` columns and entries in `Z/N`, `S(M)` is the set of all combinations of its rows with
coefficients in `Z/N` (the zero module for a matrix without rows), and `S_j(M)` is the set of the vectors of
`S(M)` whose first `j` entries are zero (`0 <= j <= n`; `S_0 = S`).

A matrix `H` with `k >= 0` rows and entries written in `[0, N)` is in *echelon form* when

- (E1) every row is non-zero, and the columns `j_1 < j_2 < ... < j_k` of the first non-zero entries of the rows
  (the *pivot columns*) increase strictly;
- (E2) every pivot `h_i = H[i, j_i]` is a divisor of `N` with `1 <= h_i < N`.

`H_(>i)` is the matrix of the rows after row `i`. `H` is in *Howell form* when moreover

- (E3) `0 <= H[l, j_i] < h_i` for all `l < i`;
- (E4) for every `i` with `0 <= i <= k`, the rows of `H_(>i)` generate `S_(j_i)(H)`, with `j_0 = 0`.

This is the Howell form of Storjohann's dissertation for the ring `Z/N`: (E1) is his (r1), (E2) and (E3) his
(r2), (E4) his (r4) (`storjohann-thesis:diss2up.txt:2058` to `2070`), with the prescribed associates and
residues inherited from his choice for `Z`, `A(Z) = {0, 1, 2, ...}` and `R(Z, b) = {0, ..., |b| - 1}`
(`diss2up.txt:481`). For a nonzero representative `a` in `[0, N)` the prescribed associate is `gcd(a, N)`. For the
zero residue the prescribed representative is `0`, not the integer `gcd(0, N) = N`, which is not in `[0, N)`
(`Ass(gcd(0, N)) = N`, whose class in `Z/N` is `0`). The inheritance from `Z` to `Z/N` is stated at
`diss2up.txt:565` to `567` (Definition 1.4: `Ass(b) = phi(Ass(Gcd(b, N)))`), the residue formula
`Rem(a, b) = phi(Rem(a, Ass(Gcd(b, N))))` at `diss2up.txt:521`. These choices give (E2) and (E3) for the nonzero
pivots, which are the only entries that these conditions constrain: a pivot is never zero (E1). The source
leaves the two choices to the user ("prescribed", `diss2up.txt:468`, `477`); they are fixed here, and the
canonical form depends on them. Zero rows are not stored: the form has exactly `k` rows.
Check: `check_s1_assoc` (for every `N <= 60` and every residue `a` in `[0, N)`, the least element of the orbit of
`a` under the units of `Z/N` is `gcd(a, N)` for `a` not zero and `0` for `a = 0`).

For `N = 1` every entry is zero, so a matrix in echelon form has no rows.

### Lemma 2.2 (an echelon form has a large span).

Hypotheses: `H` in echelon form, with `k` rows, pivots `h_1, ..., h_k`. Claim: the combinations
`c_1 H_1 + ... + c_k H_k` with integers `0 <= c_i < N/h_i` are pairwise different. So `S(H)` has at least
`(N/h_1) ... (N/h_k)` elements.

*Proof.* Let two such combinations be equal, with coefficients `c_i` and `c'_i`, and let `i` be the first
index with `c_i` different from `c'_i`. In column `j_i` the rows after row `i` are zero (E1), and the rows
before row `i` have equal coefficients. So `(c_i - c'_i) h_i = 0` modulo `N`, that is `N/h_i` divides
`c_i - c'_i`, which has absolute value below `N/h_i`. So `c_i = c'_i`, a contradiction.

Check: `check_s1_howell` (the number of elements of the span).
Used by: Propositions 2.6, 2.7.

### Lemma 2.3 (the Howell property; membership; the number of elements).

Hypotheses: `H` in echelon form with `k` rows. Claim:

1. (E4) holds exactly when, for every `i` from 1 to `k`, the vector `(N/h_i) H_i` is in `S(H_(>i))`.
2. If (E4) holds, a vector `v` is in `S(H)` exactly when the *greedy reduction* brings it to zero: for
   `i = 1, ..., k` in this order, if `h_i` divides the entry `v[j_i]` (as an integer of `[0, N)`), replace `v`
   by `v - (v[j_i]/h_i) H_i`. The coefficients `q_i = v[j_i]/h_i` then give `v = q_1 H_1 + ... + q_k H_k`, and
   `0 <= q_i < N/h_i`.
3. If (E4) holds, `S(H)` has exactly `(N/h_1) ... (N/h_k)` elements.
4. If (E4) holds for `H`, it holds for every `H_(>i)`.

*Proof.*
1. "Only if": `(N/h_i) H_i` is in `S(H)`; its entries before column `j_i` are zero (E1), and its entry in
   column `j_i` is `N = 0`. So it is in `S_(j_i)(H)`, which is `S(H_(>i))` by (E4). "If": let `v` be in
   `S_(j_i)(H)`, `v = c_l H_l + ... + c_k H_k` with `l <= i` (at the start `l = 1`). In column `j_l` the rows
   after row `l` are zero, so `v[j_l] = c_l h_l`; and `v[j_l] = 0` because `j_l <= j_i`. So `N/h_l` divides
   `c_l`, and `c_l H_l` is a multiple of `(N/h_l) H_l`, which is in `S(H_(>l))` by hypothesis. So `v` is a
   combination of the rows after row `l`. After `i - l + 1` such steps `v` is in `S(H_(>i))`. The inclusion of
   `S(H_(>i))` in `S_(j_i)(H)` holds by (E1). For `i = 0` there is nothing to prove.
2. Let `v` be in `S(H)`. Claim: before step `i` the current vector is in `S(H_(>i-1))`. True for `i = 1`.
   If it holds before step `i`, then `v = c_i H_i + ...`, the entries of `v` before column `j_i` are zero, and
   `v[j_i] = c_i h_i` modulo `N`; as `h_i` divides `N`, `h_i` divides the representative of `v[j_i]` in
   `[0, N)`. After the step the entries up to column `j_i` are zero and the vector is still in `S(H)`, so it
   is in `S_(j_i)(H) = S(H_(>i))` by (E4). After step `k` the vector is in the zero module. Conversely, if
   the reduction ends with zero, `v` is the sum of the multiples subtracted. A step that is skipped leaves a
   non-zero entry in column `j_i`, which no later step changes, so the reduction does not end with zero.
   `q_i < N/h_i` because `v[j_i] < N`.
3. By 2 every element of `S(H)` is one of the combinations of Lemma 2.2, which are pairwise different.
4. The criterion of 1 for `H_(>i)` is part of the criterion for `H`.

Check: `check_s1_howell` ((E4) is tested from its definition, by enumeration of the span; the function
`is_howell` tests claim 1; the two agree on every case).
Used by: Propositions 2.4 to 2.8.

### Proposition 2.4 (the Howell form is canonical over `Z/N`).

Hypotheses: `N >= 1`, `n >= 0`, `S` a submodule of `(Z/N)^n`. Claim: there is at most one matrix `H` in
Howell form with `S(H) = S`. (Existence is Proposition 2.5.) So two matrices of generators have the same
span exactly when their Howell forms are equal, entry by entry.

*Proof.* Let `H` be in Howell form with `S(H) = S`. For `1 <= j <= n` let `I_j` be the set of the entries
`v[j]` of the vectors `v` of `S_(j-1)`, an ideal of `Z/N`.
1. *The pivot columns are those `j` with `I_j` not zero.* If `j = j_i`, row `i` is in `S_(j-1)` with entry
   `h_i`, not zero. If `j` is not a pivot column, let `i` be the number of pivot columns before `j`. Then
   `S_(j-1)` is inside `S_(j_i) = S(H_(>i))` (E4), and every row after row `i` has its pivot column after
   `j`, so it is zero in column `j`. So `I_j` is zero.
2. *`h_i` is the smallest positive representative of an element of `I_(j_i)`.* `S_(j_i - 1)` is inside
   `S_(j_(i-1)) = S(H_(>i-1))` (E4), and in column `j_i` the rows after row `i` are zero. So every element of
   `I_(j_i)` is `c h_i` modulo `N`, a multiple of `h_i` as an integer of `[0, N)` because `h_i` divides `N`.
   And `h_i` itself occurs.
3. So the number of rows `k`, the pivot columns and the pivots are determined by `S`. Let `H'` be a second
   matrix in Howell form with span `S`. We show `H_i = H'_i` for `i = k, k - 1, ..., 1`. Let the rows after
   row `i` be equal in `H` and `H'`. The difference `d = H_i - H'_i` is in `S`, and its entries up to column
   `j_i` are zero, because the pivots are equal. So `d` is in `S_(j_i) = S(H_(>i))`. `H_(>i)` is in Howell
   form (Lemma 2.3(4)), so by Lemma 2.3(2) `d = q_(i+1) H_(i+1) + ... + q_k H_k` with integers
   `0 <= q_l < N/h_l`. Suppose some `q_l` is not zero and let `l` be the smallest such index. In column `j_l`
   the rows after row `l` are zero, so `d[j_l] = q_l h_l`, an integer with `0 < q_l h_l < N`. On the other
   hand `d[j_l]` is congruent modulo `N` to `delta = H[i, j_l] - H'[i, j_l]`, a difference of two integers of
   `[0, h_l)` by (E3), so `|delta| < h_l`. `h_l` divides `N` and `q_l h_l`, so it divides `delta`; so
   `delta = 0`, and `q_l h_l = 0` modulo `N`, against `0 < q_l h_l < N`. So every `q_l` is zero and `d = 0`.

Check: `check_s1_howell` (the matrix found by search in the span from conditions (E1) to (E3) alone is unique
and equals the output of Algorithm H); `check_s1_canonical` (generators changed by invertible operations,
permuted and padded give the same form; on all pairs of rows over `(Z/N)^2`, `N` in 4, 6, 8, 9, 12, the
number of different forms equals the number of different modules).
Used by: `PLAN.md` section 7 ("Canonical form: the same set from different inputs prints identically");
`SPEC.md` 9.1.

### Algorithm H and Proposition 2.5 (existence; the algorithm).

Algorithm H. Input: `N >= 1`, `n >= 0`, a list of rows with `n` integer entries. State: a table `T` with at
most one row `T[j]` for each column `j`, empty at the start, and a list of *pending* pairs `(v, j)` of a
vector and a column position; at the start the pending pairs are `(v mod N, 1)` for the input rows.

While a pending pair `(v, j)` exists, remove it and do:

1. While `j <= n` and `v[j] = 0`: `j = j + 1`. If `j > n`, the pair is finished (`v = 0`).
2. Let `a = v[j]`, in `[1, N)`.
   - If `T[j]` is empty: compute `g = gcd(a, N)` and integers `s, t` with `s a + t N = g`. Set
     `T[j] = s v mod N` (its entry in column `j` is `g`), and add the pending pair `((N/g) v mod N, j + 1)`.
     The pair is finished.
   - If `T[j] = w` with pivot `h` and `h` divides `a`: replace `v` by `v - (a/h) w mod N`, `j` by `j + 1`,
     and go to step 1.
   - Otherwise compute `g = gcd(a, h)` and `s, t` with `s a + t h = g`. Set `w' = s v + t w mod N` and
     `v' = (h/g) v - (a/g) w mod N`. Set `T[j] = w'` (pivot `g`), add the pending pair
     `((N/g) w' mod N, j + 1)`, replace `v` by `v'`, `j` by `j + 1`, and go to step 1.

When no pair is pending: let `H` be the rows of `T` in the order of their columns. For `i = 1, ..., k` in
this order and every `l < i`: replace `H_l` by `H_l - floor(H[l, j_i]/h_i) H_i mod N`. Output `H`.

Claim, for every `N >= 1`, `n >= 0` and every list of rows (the empty list included):

1. The algorithm ends.
2. The output is in Howell form and its span is the span of the input rows. With Proposition 2.4: every
   submodule of `(Z/N)^n` (it is finite, so it is generated by finitely many vectors) has exactly one Howell
   form, and Algorithm H computes it from any list of generators.
3. The output has at most `n` rows, whatever the number of input rows is, and it can have more rows than the
   input (`[2 1]` over `Z/4` gives the rows `(2, 1)` and `(0, 2)`).
4. Cost: at most `m + n (1 + log2 N)` pairs are processed, `m` the number of input rows; each costs at most
   `n` extended gcds and `O(n^2)` multiplications modulo `N`. The final reduction costs `O(n^3)`
   multiplications.

*Proof.*
1. A pair is processed with increasing `j`, so its processing ends. A pending pair is added only when a
   pivot is created or replaced. A replaced pivot `g = gcd(a, h)` is a proper divisor of `h` (the case
   `h | a` is the second one), so the pivot of a column is replaced at most `log2 N` times. So at most
   `n (1 + log2 N)` pairs are added.
2. *Invariants.* At every moment between two steps, with the pair in process counted as pending at its
   current position:
   - (I1) the span of the rows of `T` and of all pending vectors is `S`, the span of the input;
   - (I2) every row `T[j]` has zero entries before column `j` and its entry in column `j` is a divisor of
     `N` in `[1, N)`; every pending pair `(v, j)` has zero entries of `v` before column `j`;
   - (I3) for every non-empty `T[j]` with pivot `h`, the vector `(N/h) T[j]` is in the span of the rows
     `T[j']` with `j' > j` and of the pending vectors at positions greater than `j`.

   They hold at the start. Step 1 changes nothing. First case of step 2: `g` divides `N` and
   `1 <= g <= a < N`; `s a = g` modulo `N`. From `s a + t N = g`, `s (a/g) + t (N/g) = 1`, so
   `v - (a/g)(s v) = t (N/g) v`: the old `v` is a combination of the new `T[j] = s v` and of the new pending
   vector `(N/g) v`, and both are multiples of `v`. So the spans of (I1) and of (I3) for the columns before
   `j` do not change. The entry of `(N/g) v` in column `j` is `(N/g) a`, a multiple of `N`. (I3) for column
   `j`: `(N/g)(s v) = s ((N/g) v)`. Second case: `v` is replaced by `v` minus a multiple of `T[j]`; spans
   unchanged; the new entry in column `j` is zero. Third case: the integer matrix with rows `(t, s)` and
   `(-a/g, h/g)` has determinant `t h/g + s a/g = 1`, so `(w', v')` and `(w, v)` generate the same module; the
   entry of `w'` in column `j` is `g`, a divisor of `h` and so of `N`, that of `v'` is `(h a - a h)/g = 0`;
   `(N/g) w'` is added as pending at `j + 1`, its entry in column `j` is `N`. (I3) for column `j` holds by
   this pending vector; (I3) for a column before `j` refers to a span that contains `w` and `v` before and
   `w'`, `v'` after, and only grows. At the end nothing is pending.
   So at the end: `S(T) = S` (I1); (E1), (E2) hold (I2); and `(N/h_i) H_i` is in `S(H_(>i))` (I3), which is
   (E4) by Lemma 2.3(1). The final reduction replaces a row by itself minus multiples of later rows. This
   keeps the span, (E1), (E2) (entries in and before the own pivot column do not change, since later rows
   are zero there), and (I3): `(N/h_l)(H_l - q H_i) = (N/h_l) H_l - q (N/h_l) H_i` is in `S(H_(>l))`. It
   establishes (E3): the reduction in column `j_i` makes `H[l, j_i] < h_i`, and later reductions use rows
   `i' > i`, which are zero in column `j_i`.
3. One row per column. For the example: `a = 2`, `g = 2`, `s = 1`: `T[1] = (2, 1)`, pending `2 (2, 1) =
   (0, 2)`, which becomes `T[2]`.
4. Read off the steps, with the bound of 1.

**Remark (the third case; the algorithm is not changed).** The pending pair `((N/g) w', j + 1)` of the third case
is implied by the others: the algorithm without it gives the same output. Proof (`docs/reviews/s1/review.md`,
section "Algorithm H third-case pending pair"). Let `U` be the span of the rows `T[j']` with `j' > j` and of the
pending vectors at positions greater than `j`, before the third case. By (I3), `(N/h) w` is in `U`. After the
step `v'` is a pending vector at position `j + 1`. From `v' = (h/g) v - (a/g) w` one gets, modulo `N`,

    (N/g) v = (N/h) v' + (a/g) (N/h) w,        (N/g) w = (h/g) (N/h) w.

The first holds because `(N/h) v' = (N/g) v - (a/g)(N/h) w`; the second is `(h/g)(N/h) = N/g`. Both `(N/g) v`
and `(N/g) w` are in `U + <v'>`. Since `w' = s v + t w`, so is `(N/g) w'`. So (I3) for column `j` holds without
the pair, and the span of (I1) and of (I3) for the columns before `j` is the same with and without it: the pair
only adds a vector that is already in the span. The proof of claim 2 goes through unchanged; the other cases do
not use the pair. What it means for the cost bound 4: pairs are then added only in the first case, at most one per
column, so at most `m + n` pairs are processed, not `m + n (1 + log2 N)`; the stated bound stays true (it is
larger), and the factor `log2 N` remains only for the replacements of pivots, which continue the same pair. The
statement, the algorithm and the reference `howell` are as written above; the option `third_pair = False` of the
reference leaves the pair out. Check: `check_s1_third_case` (22800 row sets, 19 moduli up to `6^20`, up to 5
columns and 6 rows: the output with and without the pair is identical and in Howell form, on all 22800; the third
case was taken on 12371 of the row sets, 36980 times in all; 450 small sets also equal the Howell form of the
definition without the pair).

Check: `check_s1_howell` (1869 sets of rows over `Z/N`, `N` in 1, 2, 3, 4, 5, 6, 8, 9, 10, 12, 16, 18: span,
(E1) to (E4) from the definitions, number of elements), `check_s1_canonical` (moduli up to 40 bits, up to 6
columns).
Used by: `SPEC.md` 9.1; Propositions 2.6 to 2.8.

### Proposition 2.6 (the kernel, with a certificate that a checker verifies).

A *kernel certificate* for `(A, N)` is a triple of matrices `(E, V, G)` with entries in `[0, N)`: `E` with
`e` rows and `r` columns, `V` with `e` rows and `c` columns, `G` with `k` rows and `c` columns, such that

- (K1) `E` and `G` are in echelon form (Definition 2.1, (E1) and (E2)), with pivots `p_1, ..., p_e` and
  `g_1, ..., g_k`;
- (K2) `A V_i = E_i` modulo `N` for every row `i` of `V` (both written as columns);
- (K3) `A G_i = 0` modulo `N` for every row `i` of `G`;
- (K4) `(N/p_1) ... (N/p_e) (N/g_1) ... (N/g_k) = N^c`, as integers;
- (K5) `G` satisfies (E3), and for every `i` the greedy reduction of `(N/g_i) G_i` by the rows after row `i`
  ends with zero.

Claim:

1. (Soundness.) If (K1) to (K4) hold, the rows of `G` generate the whole kernel: `S(G) = K`. Moreover
   `S(E) = Im A`, and `K` has `(N/g_1) ... (N/g_k)` elements.
2. (Canonical form.) If (K1) to (K5) hold, `G` is the Howell form of `K`. It depends on `A` modulo `N` and on
   `N` only, not on the certificate, the algorithm or the order of the equations.
3. (Existence.) Let `H` be the Howell form of the matrix `[A^T | I_c]` with `c` rows and `r + c` columns
   (row `j` is column `j` of `A` followed by the unit vector `e_j`), entries modulo `N`. Write a row of `H`
   whose pivot column is among the first `r` as `(E_i | V_i)`, and the right block of each other row as
   `G_i`. Then `(E, V, G)` is a kernel certificate with (K1) to (K5).
4. (Cost of the check.) (K2) and (K3) are one product of `A` with a matrix of at most `r + c` columns, at
   most `r c (r + c)` multiplications modulo `N`; (K1) and (K4) are linear in the size of the matrices; (K5)
   is at most `k^2 c` multiplications. No elimination is made.

*Proof.*
1. The map `x -> A x` is a homomorphism of the group `(Z/N)^c`, of `N^c` elements, with kernel `K` and image
   `Im A`. By (Iso), `N^c = |K| |Im A|`. By (K2) the rows of `E` are in `Im A`, so `S(E)` is inside `Im A`;
   by (K3), `S(G)` is inside `K`. By Lemma 2.2 and (K1), `|S(E)| >= (N/p_1) ... (N/p_e)` and
   `|S(G)| >= (N/g_1) ... (N/g_k)`. So
   `N^c = |K| |Im A| >= |S(G)| |S(E)| >= (N/p_1) ... (N/p_e) (N/g_1) ... (N/g_k) = N^c` by (K4). All
   inequalities are equalities; a subset with the same finite number of elements is the whole set.
2. By induction from the last row, every `G_(>i)` is in Howell form: for the empty matrix this is clear;
   if `G_(>i)` is in Howell form, the greedy reduction of (K5) decides membership in `S(G_(>i))` (Lemma
   2.3(2)), so (K5) gives that `(N/g_i) G_i` is in `S(G_(>i))`, and the criterion of Lemma 2.3(1) holds for
   `G_(>i-1)`. So `G` is in Howell form. `S(G) = K` by 1. Proposition 2.4 gives the uniqueness.
3. The span of `[A^T | I_c]` is `{(A x | x) : x in (Z/N)^c}`: the combination of the rows with coefficients
   `x_j` is `(A x | x)`. Every row of `H` is in this span, so it is `(A x | x)` with `x` its right block:
   this is (K2) for the rows with a pivot among the first `r` columns, and (K3) for the others, whose left
   block is zero. (K1): the rows of `H` with pivot among the first `r` columns come first (E1), their left
   blocks have the same first non-zero entries, so `E` is in echelon form; the other rows have zero left
   blocks, so their right blocks `G` are in echelon form. (K4): the map `x -> (A x | x)` is injective, so the
   span has `N^c` elements, and by Lemma 2.3(3) this number is the product of `N/h` over all pivots of `H`.
   (K5): (E3) for `G` is part of (E3) for `H`; `(N/g_i)(0 | G_i)` is in the span of the later rows of `H`
   (Lemma 2.3(1)), all of which have zero left blocks, so `(N/g_i) G_i` is in `S(G_(>i))`, and the greedy
   reduction finds it (as in 2).
4. Count.

Remark (the transformation). `SPEC.md` 9.1 asks for "the transformation matrices as certificate". Here the
matrix that transforms `[A^T | I_c]` into its Howell form `H` is the right block of `H` itself, `U = [V; G]`:
`H = U [A^T | I_c] = [U A^T | U]`. It has up to `r + c` rows and is in general not square, so it is not
unimodular in the sense of `storjohann-thesis:diss2up.txt:209`; it is not used as an invertible matrix, only
through (K2) to (K4). Storjohann's Howell transform `(Q, U, C, W, r)` (`diss2up.txt:2078`) is not needed.

Check: `check_s1_solve` (4432 systems modulo 4, all `A` and `b` with `r, c <= 2`, and 4232 random systems:
`S(G)` against the kernel found by enumeration of `(Z/N)^c`; `G` against the Howell form of the list of all
kernel elements), `check_s1_cert_sound` (changed certificates: refused, or accepted and true).
Used by: `SPEC.md` 9.1 (row "Linear systems modulo `N`"); `PLAN.md` section 7 ("the kernel is complete").

### Proposition 2.7 (no solution: the certificate, its soundness and its existence).

Hypotheses: `A`, `b`, `N` as above. Claim:

1. (Soundness.) If `y` is an integer vector with `r` entries, `y^T A = 0` modulo `N` and `y^T b` not `0`
   modulo `N`, then `A x = b` modulo `N` has no solution.
2. (Existence.) If `A x = b` modulo `N` has no solution, such a `y` exists.
3. If the rows of a matrix `Y` generate the left kernel `{y : y^T A = 0}`, and there is no solution, then
   some row of `Y` has `Y_i b` not `0` modulo `N`.
4. Cost of the check: `r c + r` multiplications modulo `N`.

*Proof.*
1. A solution would give `y^T b = y^T A x = 0` modulo `N`.
2. Let `M = Im A` and `Q = (Z/N)^r / M`, a finite abelian group, with `pi` the projection. `b` is not in `M`,
   so `pi(b)` is not zero and `Q` has more than one element. By (FAb) there is an isomorphism `theta` of `Q`
   onto `Z/m_1 x ... x Z/m_t`. `N Q = 0`, so `N` times the element with 1 in coordinate `i` and 0 elsewhere
   is zero: `m_i` divides `N`. Some coordinate `theta(pi(b))_i` is not zero in `Z/m_i`. The map
   `Z/m_i -> Z/N`, `z -> (N/m_i) z`, is a well defined and injective homomorphism of groups. Let
   `psi(v) = (N/m_i) theta(pi(v))_i`, a homomorphism of groups from `(Z/N)^r` to `Z/N` that is zero on `M`
   and not zero at `b`. Put `y_l = psi(e_l)` for the unit vectors `e_l`. For `v = v_1 e_1 + ... + v_r e_r`
   with integers `v_l`, `psi(v) = v_1 psi(e_1) + ... + v_r psi(e_r) = y^T v`. Every column of `A` is in `M`,
   so `y^T A = 0`; and `y^T b = psi(b)` is not zero.
3. The `y` of 2 is a combination `c_1 Y_1 + ... + c_s Y_s`, so `y^T b = c_1 (Y_1 b) + ... + c_s (Y_s b)`; if
   every `Y_i b` were zero, `y^T b` would be zero.
4. Count.

Status: 1, 3, 4 proved here; 2 proved modulo (FAb), whose proof is on disk.

Check: `check_s1_duality` (420 submodules `M` of `(Z/N)^r`, every `b`: a separating `y` exists exactly when
`b` is outside `M`, by enumeration of all `y`; and `|M|` times the number of `y` that annihilate `M` is
`N^r`), `check_s1_solve` (the systems without solution: the `y` found is accepted by the checker and the
enumeration finds no solution).
Used by: `SPEC.md` 9.1 ("or a certificate that there is none").

### Algorithm L and Proposition 2.8 (the solver; zero matrix, `N = 1`, `r = 0`, `c = 0`).

Algorithm L. Input: `A` (`r` by `c`), `b`, `N`.

1. If `N < 1`: invalid input. Reduce `A` and `b` modulo `N`.
2. Compute the kernel certificate `(E, V, G)` of Proposition 2.6(3) by Algorithm H.
3. Greedy reduction of `b` (as a row of `r` entries) by `E`, with coefficients `q_i`. If it ends with zero:
   `x_0 = q_1 V_1 + ... + q_e V_e mod N`; the answer is "the coset `x_0 + S(G)`", with `(E, V, G)`.
4. Otherwise compute the generators `Y` of the kernel of `A^T` (Proposition 2.6(3) for the matrix `A^T`), and
   take the first row `y` of `Y` with `y b` not `0` modulo `N`; the answer is "no solution", with `y`.
5. Before the answer is returned the checker is run: (K1) to (K5), and (K6) `A x_0 = b` modulo `N`, or
   (K7) `y^T A = 0` and `y^T b` not `0` modulo `N`.

Claim:

1. Step 3 ends with zero exactly when the system has a solution, and then `A x_0 = b` modulo `N`.
2. In step 4 a row `y` is found.
3. If the checker accepts: in the first case the set of all solutions is `x_0 + S(G)`, a coset of the kernel
   `K = S(G)`, with `|K| = (N/g_1) ... (N/g_k)` elements; in the second case there is no solution. This
   follows from the conditions checked alone, whatever produced the certificate.
4. Special cases, all covered by 1 to 3:
   - `N = 1`: `(Z/1)^c` has one element. `E`, `V`, `G` have no rows, `x_0 = 0`, the answer is the coset `{0}`.
   - `r = 0` (no equation), `N >= 2`: `G` is the unit matrix of size `c`, `x_0 = 0`.
   - `c = 0` (no unknown): `E`, `V`, `G` have no rows; there is a solution, the empty vector, exactly when
     `b = 0` modulo `N`; otherwise `y` is the first unit vector `e_i` with `b_i` not `0`.
   - `A = 0` modulo `N`, `N >= 2`, `c >= 1`: `G` is the unit matrix; solvable exactly when `b = 0`.
   - One equation `a z = b`: solvable exactly when `gcd(a, N)` divides `b`, and `G` is the one row
     `(N/gcd(a, N))`, or empty when `gcd(a, N) = 1`. This agrees with Shoup, Theorem 2.5
     (`shoup-ntb:ntb-v2.txt:1221` to `1225`).
5. Cost: two runs of Algorithm H on matrices of `r + c` columns at most, and the check.

*Proof.*
1. The span of `[A^T | I_c]` is `{(A x | x)}`, and `H` is its Howell form. If `A x = b` for some `x`, then
   `(b | x)` is in the span, and its greedy reduction by `H` ends with zero (Lemma 2.3(2)); the steps for
   the rows with pivot among the first `r` columns act on the left block exactly as the greedy reduction of
   `b` by `E`, and after them the left block is zero, since the later rows of `H` have zero left blocks and
   the final result is zero. Conversely, if the reduction of `b` by `E` ends with zero then
   `b = q_1 E_1 + ... + q_e E_e = A (q_1 V_1 + ... + q_e V_e)` by (K2).
2. Proposition 2.7(3), with Proposition 2.6(1) applied to `A^T`: `Y` generates the kernel of `A^T`, which is
   the left kernel of `A`.
3. Proposition 2.6(1) and (K6): the solutions are `x_0 + K`. Proposition 2.7(1) and (K7).
4. Read off Algorithm H. `N = 1`: every entry is zero. `r = 0`: the matrix is `I_c`, in Howell form.
   `c = 0`: the matrix has no rows; `A^T` has no rows and `r` columns, its kernel is `(Z/N)^r`, `Y = I_r`.
   `A = 0`: the rows are `(0 | e_j)`. One equation: the matrix is `[a | 1]`; Algorithm H gives the rows
   `(g | s)` and `(0 | N/g)` with `g = gcd(a, N)`, the second only if `N/g` is not `0` modulo `N`.
5. Count.

Check: `check_s1_solve`, `check_s1_edge` (the special cases as fixed vectors; 20540 single equations against
`gcd(a, N) | b`; 60 systems with `N` up to 122 bits, where a planted solution must differ from `x_0` by an
element of `S(G)`), `check_s1_cert_sound`.
Used by: `SPEC.md` 9.1.

### Proposition 2.9 (the adelic form: right-hand sides that are finite balls).

Hypotheses: `A` an integer matrix, `r` by `c`; for `1 <= i <= r` a finite ball
`X_i = (B_i + H_i Zhat)/d_i` with integers `B_i`, `H_i >= 1`, `d_i >= 1` (any triple that describes the ball;
the radius is positive). Let `N = lcm(H_1, ..., H_r)` (`N = 1` for `r = 0`),
`A'[i, j] = (N/H_i) d_i A[i, j]` and `b'_i = (N/H_i) B_i`. Claim: for `x` in `Zhat^c`, with `xbar` its class
in `(Zhat/N Zhat)^c = (Z/N)^c`,

    (A x)_i is in X_i for every i   exactly when   A' xbar = b' modulo N.

So the set of the `x` in `Zhat^c` with `A x` in `X_1 x ... x X_r` is empty, or it is
`{x : xbar in x_0 + K'}` with `K'` the kernel of `A'` modulo `N`, that is
`x_0 + Zhat G_1 + ... + Zhat G_k + N Zhat^c` for the generators `G_i` of `K'`.

*Proof.*
1. `(A x)_i` is in `X_i` exactly when `z_i = d_i (A x)_i - B_i` is in `H_i Zhat`.
2. For an integer `u >= 1`, an integer `H >= 1` and `z` in `Zhat`: `z` is in `H Zhat` exactly when `u z` is in
   `u H Zhat`. "Only if" is clear. "If": `u z = u H w` gives `u (z - H w) = 0`, and `Zhat`, a product of the
   integral domains `Z_p` of characteristic 0, has no element other than 0 that a non-zero integer
   annihilates.
3. With `u = N/H_i`: `z_i` is in `H_i Zhat` exactly when `(N/H_i) z_i = (A' x)_i - b'_i` is in `N Zhat`.
4. The reduction `Zhat -> Zhat/N Zhat = Z/N` (fact (D) of `proofs/quotient.md`, section 0) is a homomorphism
   of rings with kernel `N Zhat`, and `A'`, `b'` are integers. So `(A' x)_i - b'_i` is in `N Zhat` exactly
   when `(A' xbar)_i = b'_i` in `Z/N`.
5. The last sentence: Proposition 2.8(3) for `(A', b', N)`, and the preimage of `S(G)` in `Zhat^c` is
   `Zhat G_1 + ... + Zhat G_k + N Zhat^c`.

What is not covered, and what the function returns:

- An exact right-hand side (`H_i = 0`) makes equation `i` an equation in `Zhat`, not a congruence. Version 1
  does not solve it: `UNSUPPORTED`.
- The unknowns are in `Zhat^c`. Solutions in `A_f^c` with denominators are not sought: for a matrix with a
  rational kernel they form a set that no modulus describes. Solutions with a given common denominator `D`,
  `x = x'/D` with `x'` in `Zhat^c`, are those of the system with the same `A` and the balls `D X_i`, which
  the caller forms.
- A matrix with entries that are balls is not covered (`SPEC.md` 9.1: "exact coefficients, or all
  coefficients in a ball"): version 1 solves exact integer matrices only, and the result says so.

Check: `check_s1_adelic` (396 systems: membership of `A x` in the balls decided by exact rational arithmetic
on integer points `x`, also on `x + N z`, against the solutions of the reduced system).
Used by: `SPEC.md` 9.1 ("the adelic form").

### Proposition 2.10 (the solution set as a vector of balls).

Hypotheses: as in 2.9, with a solution `x_0` and generators `G_1, ..., G_k` of `K'`. Let
`rho_j = gcd(N, G[1, j], ..., G[k, j])` for `1 <= j <= c`. Claim: the set of the `j`-th coordinates of the
solutions is the ball `x_0[j] + rho_j Zhat`. So the product of these balls contains the set of solutions,
and no product of smaller sets does. The product is in general larger than the set of solutions.

*Proof.* The `j`-th coordinates of the elements of `K'` are the combinations of the `G[i, j]` modulo `N`,
that is the ideal `rho_j Z/N`. The preimage of `x_0[j] + rho_j Z/N` in `Zhat` is
`x_0[j] + rho_j Zhat + N Zhat = x_0[j] + rho_j Zhat`, because `rho_j` divides `N`. Larger: `x_1 + x_2 = 0`
modulo 2 has the solutions `{(0, 0), (1, 1)}` modulo 2, and both coordinates run through all of `Zhat`.

Check: `check_s1_adelic` (the residues of the coordinates of all solutions, against `rho_j`).
Used by: `SPEC.md` 9.1, 4.1.

### Proposition 2.11 (what FLINT 3.0.1 offers, and what is called).

Read: `flint-3.0.1:nmod_mat.rst:26` to `31`, `:533` to `575`, `:700` to `723`;
`flint-3.0.1:fmpz_mod_mat.rst:278` to `293`, `:366` to `388`; `flint-3.0.1:fmpz_mat.rst:1159` to `1176`;
`flint-src-3.0.1:fmpz_mat/strong_echelon_form_mod.c`, `fmpz_mat/howell_form_mod.c`,
`fmpz_mod_mat/howell_form.c`, `nmod_mat/strong_echelon_form.c`.

1. The solving functions are for prime moduli: `nmod_mat_can_solve` ("the modulus of `X` which must be a prime
   number", `nmod_mat.rst:562`), `fmpz_mod_mat_solve` and `fmpz_mod_mat_can_solve` ("The modulus is assumed
   to be prime", `fmpz_mod_mat.rst:376`, `388`). They are not used.
2. For a general modulus FLINT offers the Howell form, in three modules: `fmpz_mat_howell_form_mod(A, mod)`
   (`fmpz_mat.rst:1168`), `fmpz_mod_mat_howell_form` (`fmpz_mod_mat.rst:287`; it is a call of the former,
   `fmpz_mod_mat/howell_form.c:16`), and `nmod_mat_howell_form` for a modulus of one word
   (`nmod_mat.rst:716`). Their entries name no condition on the modulus. Each works in place, returns the
   number of non-zero rows, returns no transformation, and asks that the matrix "must have at least as many
   rows as columns" (`fmpz_mat.rst:1176`, `nmod_mat.rst:723`, and, for the wrapper `fmpz_mod_mat_howell_form`,
   `fmpz_mod_mat.rst:293`). The source works with gcds against the modulus
   (`fmpz_mat/strong_echelon_form_mod.c:105`, `138`, `222`, `270`, `nmod_mat/strong_echelon_form.c:72`, `91`,
   `149`). For a NONEMPTY matrix the source reads the entry `(col, col)` for every column
   (`strong_echelon_form_mod.c:250`, the loop `col < m` with `m = A->c` at `:180`), so a nonempty matrix with fewer
   rows than columns is read outside its rows: it must be padded with zero rows. An EMPTY matrix (no rows or no
   columns) returns before this loop: `fmpz_mat/strong_echelon_form_mod.c:167` to `168` and
   `fmpz_mat/howell_form_mod.c:20` to `21` (`if (fmpz_mat_is_empty(A)) return`; the Howell form returns 0). The
   probe `probe_s1_flint_empty` calls the function on 28 empty matrices (`0 x c`, `r x 0`; moduli 1, 2, 12,
   `2^40 + 15`): rank 0, no crash. A call on a nonempty matrix with fewer rows than columns is undefined and is
   not made.
3. (Not proved from the source; probed.) On 2080 matrices with moduli up to 30 bits, padded with zero rows,
   `fmpz_mat_howell_form_mod` returns exactly the output of Algorithm H. The normalisation agrees by the
   source: the pivot is multiplied by a unit to become `gcd(pivot, N)` (`_fmpz_unit`,
   `strong_echelon_form_mod.c:93` to `126`, called at `:252`), and the entries above a pivot are reduced by
   floor division (`:262`, `:324`).
4. The introduction of `nmod_mat` says: "The modulus is assumed to be a prime number in functions that
   perform some kind of division, solving, or Gaussian elimination" (`nmod_mat.rst:28` to `30`). Read
   literally this covers the Howell form; the entry of the function and its source do not ask for a prime.
   The design does not lean on the sentence either way.

**Decision proposed (S-D7 in `api-s.md`).** The engine is Algorithm H in the library's own code, with `nmod`
arithmetic for a modulus of one word and `fmpz` arithmetic otherwise; for it existence, canonical form and
termination are proved (2.5). A call of FLINT's Howell form on `[A^T | I_c]`, padded with `r` zero rows, is an
admitted replacement, because the checker (K1) to (K7) is run on every result before it is returned and its
conditions imply the result whatever produced the certificate (2.6(1), (2), 2.7(1), 2.8(3)); if the checker
refuses the output of FLINT, Algorithm H is run. The order matters (review, S-D7): the greedy reduction of step 3
of Algorithm L divides by pivots and assumes that they are divisors of `N` in echelon position, and the count
`x_0 = q_1 V_1 + ...` assumes the shapes of `E` and `V`. So a certificate taken from FLINT is first tested for
its dimensions, entries in `[0, N)`, (K1), (K2), (K3), (K4), (K5) (none of them uses `b`), and only then consumed
by step 3; (K6) or (K7) is tested last. A test after the answer is built would come too late to protect the
operations that assume valid pivots. For Algorithm H itself no such guard is needed: its invariants are proved
(2.5).

**One computation modulo `N`, no prime powers (decision S-D9).** Algorithm H needs gcds with `N`, not the
factorisation of `N`. A computation modulo the blocks of a context, with recombination, is not proposed: the
Chinese remainder image of canonical forms is a generating set and not the canonical form. Example: the
module generated by `(3, 0)` and `(0, 2)` in `(Z/6)^2` has the Howell form with these two rows; modulo 2 its
form is `(1, 0)`, modulo 3 it is `(0, 1)` (the form of the row `(0, 2)`); the vector that is `(1, 0)` modulo
2 and `(0, 1)` modulo 3 is `(3, 4)`, which generates the same module alone and is not in Howell form. A
second run of Algorithm H modulo `N` would be needed, so nothing is saved for the canonical form. A
right-hand side in the local backend is recombined to its canonical triple before Proposition 2.9 is
applied.

Check: `probe_s1_flint`; `check_s1_edge` (the example modulo 6).
Used by: `SPEC.md` 9.1; `PLAN.md` section 6 (S.1).

### Proposition 2.12 (the Howell form and the Hermite form of `PLAN.md`, S.1).

`PLAN.md` names the "Hermite form with transformation" for S.1. Over `Z/N` the Hermite form is not canonical
(`storjohann-thesis:diss2up.txt:887` to `888`). Over `Z` it is, and the two are related as follows.

Hypotheses: `N >= 1`; `M` an integer matrix with `n` columns; `Lambda` the subgroup of `Z^n` generated by the
rows of `M` and by `N e_1, ..., N e_n`. Let `B` be an integer matrix with `n` rows and `n` columns whose rows
generate `Lambda`, upper triangular, with diagonal entries `d_j > 0` and `0 <= B[l, j] < d_j` for `l < j`
(the row Hermite normal form of a matrix of generators of `Lambda`, in the form that
`flint-src-3.0.1:fmpz_mat/is_in_hnf.c:32` to `57` tests). Claim:

1. every `d_j` divides `N`;
2. a row with `d_j = N` is `N e_j`;
3. the rows of `B` with `d_j < N`, in their order, are the Howell form of `S(M mod N)`.

*Proof.* Let `Lambda_j` be the set of the vectors of `Lambda` whose entries before column `j` are zero.
Rows `j, ..., n` of `B` generate `Lambda_j`: a vector of `Lambda` is an integer combination of all rows, and
if its entries before column `j` are zero, the coefficients of rows `1, ..., j - 1` are zero one after the
other, because `B` is triangular with non-zero diagonal.
1. The `j`-th entries of the vectors of `Lambda_j` are the multiples of `d_j`, and `N e_j` is in `Lambda_j`.
2. `B_j - N e_j` is in `Lambda_(j+1)`, so it is `c_(j+1) B_(j+1) + ... + c_n B_n`. Its entry in column
   `j + 1` is `B[j, j+1]`, in `[0, d_(j+1))`, and it is `c_(j+1) d_(j+1)`; so `c_(j+1) = 0`. In the same way
   every `c_l` is zero.
3. `Lambda` is the set of the integer vectors whose class modulo `N` is in `S = S(M mod N)`. Let `H` be the
   matrix of the rows with `d_j < N`. Its entries are in `[0, N)`: a pivot is `d_j < N`, an entry above a
   pivot `d_j` is below `d_j`, and an entry in a column `j` with `d_j = N` is below `N`. (E1): the pivot of a
   row is its diagonal entry. (E2): claim 1. (E3): the entries above a pivot are those of `B`. Span: the
   rows of `B` modulo `N` generate `S`, and the rows left out are zero modulo `N` by claim 2. (E4), by
   Lemma 2.3(1): `(N/d_j) B_j - N e_j` is in `Lambda_(j+1)`, a combination of the rows of `B` after row `j`;
   modulo `N` this says that `(N/d_j) H_j` is in the span of the rows of `H` after it. Proposition 2.4
   gives the claim.

Consequence for the design: the Hermite form over `Z` of the matrix `[A^T | I_c]` with the rows `N e_j`
appended, with FLINT's `fmpz_mat_hnf` (`flint-3.0.1:fmpz_mat.rst:1224` to `1227`), is a third engine for
Proposition 2.6(3). It is not proposed as the first choice: its entries are not bounded by `N` during the
computation.

Check: `probe_s1_hnf` (the Hermite form of python-flint on 2016 matrices against Algorithm H).
Used by: `PLAN.md` section 6 (S.1).

## 3. Roots (S.2)

Throughout: `f` is a polynomial with integer coefficients, not the zero polynomial; `f'` is its derivative.
`p` is a prime; every statement of 3.1 to 3.7 holds for every prime, `p = 2` included, and 3.11 says where
the prime 2 behaves differently in practice. A *root of `f` in `Z_p`* is an `alpha` in `Z_p` with
`f(alpha) = 0`; it is *simple* if `f'(alpha)` is not zero, *multiple* otherwise. `f` has at most `deg f` roots
in `Z_p` ((Roots), `Z_p` is an integral domain). `v = v_p`, with `v(0)` infinite. A ball `a + p^k Z_p` is
given by an integer `a` in `[0, p^k)` and `k >= 0`; two balls meet exactly when one contains the other, that is
when the centres agree modulo `p` to the smaller of the two exponents.

Only roots in `Z_p` are treated. The roots of `f` in `Q_p` outside `Z_p` are the inverses of the non-zero
roots in `p Z_p` of the reversed polynomial `X^(deg f) f(1/X)`; the caller forms it.

### Lemma 3.1 (content at `p`; the squarefree part).

1. If `p^w` divides every coefficient of `f`, then `f` and `f/p^w` have the same roots in `Z_p`, with the same
   simple and multiple ones.
2. Let `K` be a field that contains `Q` (here `R`, `Q_p` or `C`), `h = gcd(f, f')` in `Q[X]` and `g = f/h`. Then
   `g` and `f` have the same roots in `K`, and every root of `g` in `K` is simple. Moreover `gcd(g, g') = 1` in
   `Q[X]`.
3. (The normalised polynomial.) For a nonzero rational `c` the polynomials `g` and `c g` have the same roots in
   `K`, all simple. The *normalised polynomial* `g*` of `f` is `c g` with `c` the rational for which `g*` is in
   `Z[X]`, has content 1 and a positive leading coefficient; for `f` constant, `g* = 1`. The certificates of
   Section 3 for the roots of `f` at a place refer to `g*` (decision S-D13, `api-s.md`).

*Proof.*
1. `f = p^w f_1` and `f' = p^w f_1'`, and `p^w` is not zero in `Z_p`.
2. There are `u, v` in `Q[X]` with `h = u f + v f'` ((EEA-poly) in `Q[X]`), and `h` divides `f` and
   `f'`. Let `alpha` in `K` be a root of `f`, and write `f = (X - alpha)^m q` with `m >= 1`, `q` in `K[X]`,
   `q(alpha)` not zero ((Roots), Theorem 7.12 of the source, applied `m` times). Then
   `f' = (X - alpha)^(m-1) (m q + (X - alpha) q')`, and the second factor has the value `m q(alpha)` at
   `alpha`, not zero because `K` has characteristic 0. So `(X - alpha)^(m-1)` divides `f'` and
   `(X - alpha)^m` does not. `h` divides `f'`, so `(X - alpha)^m` does not divide `h`; `h = u f + v f'`, so
   `(X - alpha)^(m-1)` divides `h`. So `g = f/h` is `(X - alpha)` times a polynomial that is not zero at
   `alpha`: `alpha` is a simple root of `g`. Conversely `g` divides `f`, so a root of `g` is a root of `f`,
   and by what was shown it is a simple root of `g`. Finally, a root `alpha` of `g` in `C` is simple, so
   `g = (X - alpha) q` with `q(alpha)` not zero and `g'(alpha) = q(alpha)` is not zero. If `gcd(g, g')` were not
   constant it would have a root `alpha` in `C`, a common root of `g` and `g'`: impossible. So `gcd(g, g') = 1`.
3. A nonzero constant is a unit of `K`; multiplying by it changes no root and no multiplicity.

Check: `check_s2_real_completeness` (cases with multiple roots), `check_s2_examples` (content), `check_s2_lists`
(lists of roots of polynomials with multiple factors, built on `g*`).
Used by: 3.5, 3.6, 3.10, 3.12, 3.13.

### Definition 3.2 (root certificate) and Proposition 3.2 (what it proves).

A *root certificate* for `(f, p)` is a triple of integers `(a, k, s)` with

- (R1) `k > s >= 0` and `0 <= a < p^k`;
- (R2) `v(f'(a)) = s`, that is `f'(a) = 0` modulo `p^s` and not modulo `p^(s+1)`;
- (R3) `v(f(a)) >= k + s`, that is `f(a) = 0` modulo `p^(k+s)`.

Claim, for a root certificate `(a, k, s)`:

1. There is exactly one root `alpha` of `f` in the ball `a + p^(s+1) Z_p`. It lies in `a + p^k Z_p`. It is a
   simple root, and `v(f'(alpha)) = s`.
2. Let `k'` be an integer with `s < k' <= k` and `a'` an integer with `a' = a` modulo `p^(k')`. Then
   `v(f'(a')) = s` and `v(f(a')) >= k' + s`. In particular, for `k' = k`, (R2) and (R3) hold for every integer
   `a'` with `a' = a` modulo `p^k` in place of `a`: the certificate is a property of the ball `a + p^k Z_p`.
3. For every `k'` with `s < k' <= k`, `(a mod p^(k'), k', s)` is a root certificate.
4. Two balls with root certificates hold the same root exactly when they meet.
5. The check costs two evaluations of polynomials modulo `p^(k+s)` and `p^(s+1)`.
6. (Equality is not enough.) The strict inequalities `k > s` and `v(f(a)) >= k + s` cannot be weakened to
   equality `k = s`: for `f = X^2 + 3`, `p = 2`, `a = 1`, `v(f(1)) = v(4) = 2` and `v(f'(1)) = v(2) = 1`, so
   `(1, 1, 1)` satisfies (R2) and (R3) and has `k = s`, and `f` has no root in `Z_2` at all (a square modulo 8 is
   0, 1 or 4, and `-3 = 5` modulo 8). The checker refuses `(1, 1, 1)` by (R1). (`check_s2_certificate` counts, for
   every certificate, `p^s` roots by enumeration.)

*Proof.*
1. By (R2), (R3), (R1): `|f(a)|_p <= p^(-k-s) < p^(-2s) = |f'(a)|_p^2`. (H2) gives exactly one root `alpha`
   with `|alpha - a|_p < |f'(a)|_p = p^(-s)`, that is with `alpha` in `a + p^(s+1) Z_p`; and
   `|alpha - a|_p = |f(a)/f'(a)|_p <= p^(-k-s+s) = p^(-k)`; and `|f'(alpha)|_p = |f'(a)|_p`.
2. Let `a' = a + p^(k') t`, `t` an integer. `f'(a') = f'(a)` modulo `p^(k')` (every coefficient of the
   difference is a multiple of `a' - a`), and `k' >= s + 1`, so `v(f'(a')) = s`. By (Taylor),
   `f(a') = f(a) + f'(a) p^(k') t + z p^(2 k') t^2` with `z` an integer; the three terms have valuations at least
   `k + s >= k' + s` (by (R3) and `k' <= k`), `s + k'`, and `2 k' >= k' + s + 1` (as `k' > s`). So
   `v(f(a')) >= k' + s`.
3. (R1) holds for `k'`. The reduced centre `a mod p^(k')` is an integer `a'` with `a' = a` modulo `p^(k')`, so
   (R2) and (R3) hold for `(a', k', s)` by 2.
4. Let the balls be `B` and `B'` with roots `alpha`, `alpha'`. If `alpha = alpha'` it is in both. If they
   meet, one contains the other, say `B` is inside `B'`; `B'` is inside `a' + p^(s'+1) Z_p`, which holds
   exactly one root, and `alpha` is a root in it. So `alpha = alpha'`.
5. (R2) needs `f'(a)` modulo `p^(s+1)`, (R3) needs `f(a)` modulo `p^(k+s)`.

Check: `check_s2_certificate` (for every certificate, the `x` modulo `p^(k+s+3)` with `x = a` modulo
`p^(s+1)` and `f(x) = 0` modulo `p^(k+s+3)` are counted by enumeration: there must be exactly `p^s`, all equal
to `a` modulo `p^k`; a ball without a root or with two roots fails this count. Certificates are also found
by search in 300 random polynomials, 869 of them).
Used by: `SPEC.md` 9.1 ("simple roots by Hensel lifting with certificate").

### Proposition 3.3 (lifting: one Newton step doubles the precision beyond `s`).

Hypotheses: `(a, k, s)` a root certificate for `(f, p)`. Let `F = f(a)/p^(k+s)` and `D = f'(a)/p^s`, integers,
`D` prime to `p`; let `u` be an integer with `u D = 1` modulo `p^(k-s)`; let `k+ = 2 k - s` and
`a+ = (a - p^k F u) mod p^(k+)`. Claim:

1. `(a+, k+, s)` is a root certificate, `a+ = a` modulo `p^k`, and `k+ > k`.
2. `a+` is determined by `f(a)` modulo `p^(2k)` and `f'(a)` modulo `p^k`.
3. The root of the new ball is the root of the old one. Repeating the step `n` times gives the precision
   `s + 2^n (k - s)`.

*Proof.*
1. `k+ - k = k - s >= 1`. Let `h = p^k F u`, so `a+ = a - h` modulo `p^(k+)`. By (Taylor),
   `f(a - h) = f(a) - f'(a) h + z h^2 = p^(k+s) F - p^s D p^k F u + z h^2 = p^(k+s) F (1 - D u) + z h^2`.
   `1 - D u` is divisible by `p^(k-s)`, so the first term is divisible by `p^(2k)`; so is `h^2`. So
   `v(f(a - h)) >= 2 k = k+ + s`. `f'(a - h) = f'(a)` modulo `p^k` and `k > s`, so `v(f'(a - h)) = s`. So
   (R2), (R3) hold for the integer `a - h` with `k+`; by the computation of Proposition 3.2(2), which uses
   only `k+ > s`, they hold for its residue `a+` modulo `p^(k+)`. (R1): `k+ > k > s`.
2. `F` is needed modulo `p^(k-s)`, that is `f(a)` modulo `p^(2k)`; `u` needs `D` modulo `p^(k-s)`, that is
   `f'(a)` modulo `p^k`.
3. `a+ + p^(k+) Z_p` is inside `a + p^k Z_p`, inside `a + p^(s+1) Z_p`, which holds one root.
   `k+ - s = 2 (k - s)`.

The step is Newton's iteration `a - f(a)/f'(a)` of the first proof of (H2) in the source
(`conrad-hensel:hensel.txt:436` to `441`), cut to the precision that is certified.

Check: `check_s2_newton` (five steps from every certificate of the named cases: the new triple satisfies
(R1) to (R3) by direct evaluation, `k+ = 2k - s`, and the centres agree modulo `p^k`; requested precisions
1, 2, 5, 17, 60 give nested balls of precision `max(requested, s + 1)`).
Used by: `SPEC.md` 9.1 ("to a requested precision").

### Proposition 3.4 (one level of the search).

Hypotheses: `f` not zero modulo `p` (Lemma 3.1(1)); `a` an integer, `e >= 0`. The polynomial
`f(a + p^e Y)` of `Z[Y]` is not zero; let `w = w(a, e) >= 0` be the largest exponent such that `p^w` divides
all its coefficients, and `g = g_(a,e) = f(a + p^e Y)/p^w`, a polynomial of `Z[Y]` that is not zero modulo `p`.
Claim:

1. `alpha -> (alpha - a)/p^e` is a bijection from the roots of `f` in `a + p^e Z_p` onto the roots of `g` in
   `Z_p`.
2. If `g` has no root modulo `p`, then `f` has no root in `a + p^e Z_p`.
3. Let `b` in `[0, p)` with `g(b) = 0` and `g'(b)` not `0` modulo `p`. Then `f` has exactly one root `alpha` in
   the class `a + p^e b + p^(e+1) Z_p`; `w >= e` and `v(f'(alpha)) = w - e`; and for every `j >= 1` with
   `j > w - 2 e` and every integer `beta_j` with `beta_j = beta` modulo `p^j`, where `beta = (alpha - a)/p^e`,
   the triple `((a + p^e beta_j) mod p^(e+j), e + j, w - e)` is a root certificate for `(f, p)`.
4. Let `b` in `[0, p)` with `g(b) = 0` and `g'(b) = 0` modulo `p`, and let `a_1 = a + p^e b`. Then
   `w(a_1, e + 1) >= w + 1`; and if `g_(a_1, e+1)` has a root modulo `p`, then `w(a_1, e + 1) >= w + 2`.

*Proof.*
1. `f(a + p^e beta) = p^w g(beta)` for `beta` in `Z_p`, and `p^w` is not zero.
2. A root `beta` of `g` in `Z_p` gives the root `beta mod p` of `g` modulo `p`. With 1.
3. By (H1), `g` has exactly one root `beta` in `Z_p` with `beta = b` modulo `p`; with 1, `f` has exactly one
   root `alpha = a + p^e beta` in the class. Differentiating `f(a + p^e Y) = p^w g(Y)` gives
   `p^e f'(a + p^e Y) = p^w g'(Y)`. At `Y = beta`: `e + v(f'(alpha)) = w + v(g'(beta)) = w`, because
   `g'(beta) = g'(b)` modulo `p` is a unit. So `w >= e` and `s = v(f'(alpha)) = w - e`. Let
   `a_j = a + p^e beta_j`. `f(a_j) = p^w g(beta_j)` and `g(beta_j) = g(beta) = 0` modulo `p^j`, so
   `v(f(a_j)) >= w + j = (e + j) + s`: (R3) with `k = e + j`. `p^e f'(a_j) = p^w g'(beta_j)` and
   `g'(beta_j) = g'(b)` modulo `p`, a unit: `v(f'(a_j)) = w - e = s`, (R2). `k > s` is `j > w - 2 e`, (R1).
   By Proposition 3.2(2) the residue modulo `p^k` may be taken.
4. `f(a_1 + p^(e+1) Y) = p^w g(b + p Y)`. By (Taylor), `g(b + p Y) = g(b) + g'(b) p Y + z(Y) p^2 Y^2` with
   `z` in `Z[Y]`. `g(b)` is divisible by `p`, `g'(b) p` by `p^2`. So every coefficient of `g(b + p Y)` is
   divisible by `p`, and `w(a_1, e + 1) >= w + 1`. If `g(b)` is not divisible by `p^2`, the content of
   `g(b + p Y)` is exactly `p`, and `g_(a_1, e+1)` is the constant `g(b)/p` modulo `p`, not zero: it has no
   root. So a root modulo `p` of `g_(a_1, e+1)` needs `p^2 | g(b)`, and then every coefficient is divisible by
   `p^2`.

Check: `check_s2_descent`; claim 4 is an assertion inside the reference `padic_roots`, tested on every node
of every run.
Used by: Proposition 3.5.

### Algorithm P and Proposition 3.5 (the list of roots in `Z_p`; what "complete" means).

Algorithm P. Input: `f` not zero, a prime `p`, a requested precision `k_req >= 1`, a depth limit `D >= 0`.

1. Divide `f` by the largest power of `p` that divides all its coefficients.
2. The list `L` of certificates and the list `U` of unresolved classes are empty; the class `(a, e) = (0, 0)`
   is open.
3. For an open class `(a, e)`: compute `w` and `g` of Proposition 3.4. Find the complete list of the roots
   `b` of `g` modulo `p` by either method of Proposition 3.7. For each root `b` in that list:
   - if `g'(b)` is not `0` modulo `p`: let `s = w - e`, `j = max(1, w - 2 e + 1)`; lift `b` to `beta_j` with
     `g(beta_j) = 0` modulo `p^j` by Proposition 3.3 applied to `g` (certificate `(b, 1, 0)`); form the
     certificate of 3.4(3) with `k = e + j`; lift it by Proposition 3.3 until `k >= K = max(k_req, s + 1)`;
     reduce the centre modulo `p^K` (Proposition 3.2(3)); add `(a, K, s)` to `L`;
   - else if `e + 1 > D`: add the class `(a + p^e b, e + 1)` to `U`;
   - else: the class `(a + p^e b, e + 1)` is open.
   The class `(a, e)` is closed.
4. When no class is open: return `L` and `U`.

Claim:

1. (Soundness.) Every `(a, K, s)` of `L` is a root certificate for `f` (after step 1). So the ball
   `a + p^K Z_p` holds exactly one root of `f`, a simple one, and `K >= k_req`.
2. (Partition.) The balls of `L` and the classes of `U` are pairwise disjoint, and every root of `f` in `Z_p`
   lies in exactly one of them.
3. (Completeness.) If `U` is empty, the roots of `f` in `Z_p` are exactly the roots of the balls of `L`: their
   number is the length of `L`, which may be 0, and all of them are simple. This is what "the list is
   complete" means, and only in this case is it said.
4. (Simple roots are found.) A simple root `alpha` with `v(f'(alpha)) = s` (for `f` after step 1) lies in a
   ball of `L` if `D >= s + 1`.
5. (Multiple roots are never resolved.) A multiple root of `f` in `Z_p` lies in a class of `U`, for every `D`.
   So if `f` has a multiple root in `Z_p`, `U` is not empty and the list is never called complete. What is
   returned then: the certificates of the simple roots found, and the unresolved classes; a class of `U` may
   hold no root, one root or several, simple or multiple.
6. (Termination.) The algorithm ends for every `D`. If `f` has no multiple root in `Z_p`, there is a `D_0`
   such that `U` is empty for every `D >= D_0`. This holds in particular if `gcd(f, f') = 1` in `Q[X]`, and
   for the squarefree part of any `f` (Lemma 3.1(2)).
7. Cost: for each class opened, find the roots of its current polynomial g modulo p. The evaluation
   method of Proposition 3.7(1) uses p evaluations of g. The degree method of Proposition 3.7(2) uses
   O(log p) polynomial products and reductions modulo g, one polynomial gcd, the root finding of d,
   and deg d evaluations of g to check the candidates. For g constant, d = 1 and the list is empty.
   FLINT's root finding uses repeated pseudorandom splitting; no worst-case time bound is asserted.
   After either method, evaluate g' once for each root found. For each child opened, make one change
   of variable and remove its content at p. For each certified root, perform the lifting of 3.3.
   These counts exclude coefficient bit costs and the sorting of the output lists.

*Proof.* A class `(a, e)` is opened only if it is `(0, 0)` or the child `(a' + p^(e-1) b, e)` of an opened
class `(a', e - 1)` with `b` a root modulo `p` of `g_(a', e-1)` and `g'(b) = 0` modulo `p`. The children of a
class are disjoint subsets of it.

(W) *An opened class `(a, e)` whose `g_(a,e)` has a root modulo `p` has `w(a, e) >= 2 e`.* For `e = 0` this is
`w >= 0`. For `e >= 1` the class is the child of an opened class `(a', e - 1)` that has a root modulo `p`
(the digit `b`), so `w(a', e - 1) >= 2 (e - 1)` by induction, and `w(a, e) >= w(a', e - 1) + 2` by
Proposition 3.4(4). Consequence: a root certified at level `e` has `s = w - e >= e`, so the `j` of step 3 is
`w - 2 e + 1 >= 1`, and `K >= s + 1 >= e + 1`.
1. Proposition 3.4(3), 3.3(1), 3.2(3).
2. By induction on `e`: the roots in an opened class `(a, e)` lie in the classes `a + p^e b + p^(e+1) Z_p`
   with `b` a root of `g` modulo `p` (3.4(1), (2)), which are disjoint; for a simple `b` the class holds one
   root (3.4(3)) and the ball of `L` made from it is inside this class (its centre is `a + p^e b` modulo
   `p^(e+1)`, because `beta_j = b` modulo `p`, the liftings keep the centre modulo `p^k` with `k >= e + 1`,
   and `K >= e + 1` by (W)) and holds that root; for the other `b` the class is
   opened or put into `U`. Disjointness: two items come from different children of some class, or one from
   a child and the other from inside another child.
3. From 2 and 1.
4. Let `a` be the integer of `[0, p^e)` with `alpha` in `a + p^e Z_p`, `beta = (alpha - a)/p^e`, a root of
   `g = g_(a,e)`. From `p^e f'(a + p^e Y) = p^w g'(Y)`: `v(g'(beta)) = e + s - w`. We show `w = e + s` for
   `e >= s + 1`. Write `f(alpha + Z) = d_1 Z + d_2 Z^2 + ...` with `d_i` in `Z_p`, `d_1 = f'(alpha)`. Then
   `f(a + p^e Y) = f(alpha + p^e (Y - beta)) = d_1 p^e (Y - beta) + d_2 p^(2e) (Y - beta)^2 + ...`. For
   `e >= s + 1` the first term is `p^(e+s)` times a polynomial that is not zero modulo `p`, and the others
   are divisible by `p^(2e)`, `2 e > e + s`. So `w = e + s`, and `beta mod p` is a simple root of `g` modulo
   `p`. So on the chain of classes that contain `alpha`, the first level at which the digit of `alpha` is a
   simple root is at most `s + 1`; if `D >= s + 1` that class is opened and the root is certified there.
5. For a multiple root, `f'(alpha) = 0`, so `g'(beta) = p^(e-w) f'(alpha) = 0` at every level: the digit of
   `alpha` is never a simple root, and the chain of its classes ends in `U` at level `D + 1`.
6. Each class has at most `p` children and levels beyond `D + 1` are not opened. Suppose `f` has no
   multiple root and `U` is not empty for arbitrarily large `D`. The classes that are opened for some `D`
   form a tree in which every node has at most `p` children and which has nodes at every level; so it has
   an infinite chain of nested classes `(a_e, e)` (at each level choose a child below which the tree is
   infinite). Along it every digit is a multiple root modulo `p`, and by (W) `w(a_e, e) >= 2 e`. So
   `v(f(a_e)) >= w(a_e, e) >= 2 e`, and from `p^e f'(a_e + p^e Y) = p^w g'(Y)` at `Y = 0`:
   `v(f'(a_e)) >= w - e >= e`. The `a_e` converge in `Z_p` to a point `alpha` (fact (S1) of
   `proofs/quotient.md`), and `f(alpha) = f'(alpha) = 0`: a multiple root. Contradiction. If
   `gcd(f, f') = 1` there are `u, v` in `Q[X]` with `u f + v f' = 1`, so `f` and `f'` have no common root.
7. For evaluation, count the residues. For the degree method, binary powering processes the bits of p
   (refs/src/flint-src-3.0.1/nmod_poly/powmod_ui_binexp.c:40 to 48), so it uses O(log p) products and
   reductions. The gcd is computed once. The accepted candidate list has length deg d, and each
   candidate is evaluated once. Step 3 evaluates the derivative only at those roots, makes a change
   of variable only when a child is opened, and applies 3.3 only when a root is certified. Root finding
   itself has no finite retry bound in the cited source; its repeated splitting cost stays separate.

Check: `check_s2_descent` (31 named cases, depth limits 0, 1, 3, 8: disjointness; every `x` modulo `p^M` with
`f(x) = 0` modulo `p^M`, found by enumeration, lies in exactly one ball or class; every certified ball
holds `p^s` of them; the expected number of roots; a polynomial with a multiple root never gives a complete
list; 150 random products of linear factors with a factor without roots: every planted root is in exactly
one ball and the list is complete), `check_s2_examples`. The oracle skips one run of the 124 (the multiple
root `(x-1)^2 (x+2)` at 5: more than 200000 approximate residues); the check now counts and names it.
Used by: `SPEC.md` 9.1 (rows "Polynomial roots at a given prime" and "Multiple roots ... later").

### Proposition 3.6 (a depth bound from an integer Bezout identity).

Hypotheses: `p` a prime; `f` in `Z[X]` not zero modulo `p`, that is `f` after Algorithm P, step 1 (the content at
`p` removed); `u`, `v` in `Z[X]` and `R` a non-zero integer with `u f + v f' = R`. Claim:

1. Every opened class `(a, e)` of Algorithm P that has a child has `e <= v_p(R)`.
2. If `D >= v_p(R) + 1` then `U` is empty: the list is complete.
3. Such `u`, `v`, `R` exist for every nonconstant `f` with `gcd(f, f') = 1` in `Q[X]`, in particular for the
   normalised polynomial `g*` of any nonconstant polynomial (Lemma 3.1(3)). A nonzero constant has no root and
   needs no bound.

*Proof.*
1. Let `(a, e)` be an opened class with a child. A child comes from a root `b` of `g_(a,e)` modulo `p`, so (W)
   gives `w = w(a, e) >= 2 e`. The constant coefficient of `f(a + p^e Y) = p^w g_(a,e)(Y)` is `f(a)`, so
   `v_p(f(a)) >= w >= 2 e`. Differentiating `f(a + p^e Y) = p^w g_(a,e)(Y)` gives
   `p^e f'(a + p^e Y) = p^w g'_(a,e)(Y)`; at `Y = 0`, `f'(a) = p^(w - e) g'_(a,e)(0)` and
   `v_p(f'(a)) >= w - e >= e`. As `a` is an integer and `u`, `v` have integer coefficients, `u(a)` and `v(a)` are
   integers, and `R = u(a) f(a) + v(a) f'(a)` has `v_p(R) >= min(v_p(f(a)), v_p(f'(a))) >= e`.
2. An unresolved class is added only by an opened class `(a, e)` with a child at level `e + 1 > D`, that is
   `e >= D >= v_p(R) + 1`. By 1 such a class does not have a child. So nothing is added to `U`.
3. By (EEA-poly) in `Q[X]` (with `f` of larger degree than `f'`) there are `u_0`, `v_0` in `Q[X]` with
   `u_0 f + v_0 f' = gcd(f, f') = 1`. Let `R` be a positive common denominator of the coefficients of `u_0` and
   `v_0`; then `u = R u_0`, `v = R v_0` are in `Z[X]` and `u f + v f' = R`. For `g*`: Lemma 3.1(2) gives
   `gcd(g, g') = 1`, and (3) the same for `g*`, a constant multiple.

Remark (what is not claimed). A bound in terms of the resultant `Res(f, f')` or the discriminant is not asserted.
`[source pending: a text with the identity u f + v g = Res(f, g) over Z[X]]`. The passage
`flint-3.0.1:fmpz_poly.rst:1276` to `1280` describes a function, not the theorem. When `p` divides the leading
coefficient of `f` the resultant and the discriminant differ in their `p`-parts, so a bound for one is not a bound
for the other. No implementation relies on the resultant; the function takes `D` from the caller.
The identity of the review is an example: `f = X^2 - c`, `u = -4`, `v = 2 X`, `R = 4 c`, so `D = v_2(4 c) + 1`
is enough at `p = 2`.

Check: `check_s2_bezout_depth` (211 polynomials with `p` in 2, 3, 5: the identity `u f + v f' = R` holds exactly,
`D` equal to `v_p(R) + 1` gives a complete list, 42 opened classes have a child, all with `e <= v_p(R)` and
the children are counted from the definition; the identity `-4 (X^2 - c) + 2 X (2 X) = 4 c` for seven values of `c`).
Used by: Algorithm P (the depth to give); `api-s.md` (optional depth). Status: proved here, with (EEA-poly).

### Proposition 3.7 (the roots modulo `p` are complete: by evaluation, or by a degree).

Hypotheses: `p` prime, `g` a polynomial over `F_p`, not zero. Claim:

1. The list of the `b` in `[0, p)` with `g(b) = 0` is the list of all roots of `g` in `F_p`. Cost: `p`
   evaluations.
2. Let `d = gcd(g, X^p - X)` in `F_p[X]` (for `g` constant, `d = 1`). The number of distinct roots of `g` in
   `F_p` is the degree of `d`. So a list of distinct roots of `g`, found by any method, is complete exactly
   when its length is `deg d`.

*Proof.*
1. By definition.
2. `d = u g + v (X^p - X)` for some `u, v` in `F_p[X]`, and `d` divides both ((EEA-poly) in `F_p[X]`, the
   polynomial of larger degree first). Every `a` of `F_p` is a root of `X^p - X` (Fermat). So the roots of `d`
   in `F_p` are the roots of `g` in `F_p`: a root of `g` is a root of `u g + v (X^p - X)`, and a root of `d`
   is a root of `g`. Write
   `X^p - X = d q`. Every `a` of `F_p` is a root of `d` or of `q`, because `F_p` is an integral domain. `d` has
   at most `deg d` roots and `q` at most `deg q = p - deg d` ((Roots), Theorem 7.14). So
   `p <= #roots(d) + #roots(q) <= deg d + (p - deg d) = p`, and `d` has exactly `deg d` roots.

Decision S-D10 (TJO, 2026-09-29): the root finder accepts every prime that a place can hold. The place type
holds the primes below 2^64, each proved prime by adf_place_prime; it is not widened. A bound on p is a
property of an implementation slice. In the implemented slice, p <= ADF_ROOTS_P_EVAL_MAX = 128 uses 1;
larger primes use 2. The bound is the point where the method changes, measured by bench/bench_roots_modp.c.
For each class polynomial h of Algorithm P, its content at p is removed, so h modulo p is not zero
(Proposition 3.4). Form t = X^p modulo h by nmod_poly_powmod_ui_binexp and d = gcd(h, t - X) by
nmod_poly_gcd. Since t - X = X^p - X modulo h, this d is the gcd of h and X^p - X. For h constant, take d = 1.
The candidates are the roots of d returned as linear factors by nmod_poly_roots(r, d, 0). Accept them only
when they are distinct residues in [0, p), every candidate evaluates to zero in h, and their number is
deg d. Proposition 3.7(2) then proves completeness. A refused list aborts under S-D20.
The correctness of the powering, gcd, coefficient reduction and modular evaluation is trusted in FLINT.
The candidate test checks the root finder's output; it does not independently certify deg d. In particular,
a wrong gcd of smaller degree can cause an incomplete list to pass. The completeness verifier repeats the
same count and therefore has the same trust base.
Sources on disk: refs/src/flint-3.0.1/nmod_poly.rst:858 to 861 and 1716 to 1721;
refs/src/flint-src-3.0.1/nmod_poly/powmod_ui_binexp.c:26 to 52 and 67 to 88;
refs/src/flint-src-3.0.1/nmod_poly/gcd.c:16 to 24 and 27 to 74;
refs/src/flint-src-3.0.1/nmod_poly_factor/roots.c:17 to 19 and 148 to 204;
refs/src/flint-src-3.0.1/nmod_poly/find_distinct_nonzero_roots.c:15 to 51.
The root routine retries a random split until proper; it returns no failure status or finite time bound.
flint_randinit uses fixed seeds (refs/src/flint-src-3.0.1/flint.h.in:245 to 250). The implementation therefore
uses a fixed pseudorandom sequence. The algorithm's random-shift analysis is not a worst-case time bound
for that sequence. No fmpz_mod_poly source is needed for this one-word implementation.

Check: `check_s2_count_mod_p` (458 polynomials, `p` up to 257, against the evaluation at all residues).
Used by: Proposition 3.5 (step 3 of Algorithm P).

### Proposition 3.8 (real roots: count plus isolation gives completeness).

Hypotheses: `f` a real polynomial, not zero, with exactly `n` distinct real roots. Let
`[lo_1, hi_1], ..., [lo_m, hi_m]` be real intervals with `hi_i < lo_(i+1)` for all `i`, such that for each `i`
either (a) `lo_i < hi_i` and `f(lo_i) f(hi_i) < 0`, or (b) `lo_i = hi_i` and `f(lo_i) = 0`. Claim:

1. Every interval contains at least one real root of `f`, and `m <= n`.
2. If `m = n`, every interval contains exactly one real root, and every real root lies in one of the
   intervals. In case (a) the root lies in the open interval.
3. Without the count nothing is known about roots outside the intervals, and an interval of kind (a) may
   contain any odd number of roots counted with multiplicity.

*Proof.*
1. (a): (IVT), the polynomial function is continuous; the root is in the open interval since the end values
   are not zero. (b): the point is a root. The intervals are pairwise disjoint, so the `m` roots are
   different, and `m <= n`.
2. The `n = m` roots found in 1 are all the roots; two roots in one interval would give `m + 1 <= n`.
3. `f = (X - 1)(X - 2)(X - 3)` with the one interval `[0, 4]`: `f(0) f(4) = -36 < 0`.

Status: proved modulo (IVT).

Remark (a stored count is not a count). Claim 2 assumes that `n` is the number of distinct real roots of `f`; it
is a hypothesis about `f`, not a number that may be read from a list. For `f = (X - 1)(X - 2)(X - 3)` the single
interval `[0, 4]` with the supplied `n = 1` satisfies every other hypothesis of claim 2 (`f(0) f(4) = -36 < 0`,
`m = 1 = n`) and holds three roots. A verifier of a list therefore recomputes the count (Proposition 3.13). The
reference checker `real_cert_ok(f, n, balls)` takes a trusted `n` and is the checker of this proposition; the
verifiers of a list are `real_verify_entries` and `real_verify_complete`.

Check: `check_s2_real_completeness` (18 polynomials; the checker accepts the isolated intervals; the number
of roots inside each interval and in the gaps is counted independently; lists with an interval removed,
two intervals merged, or an interval moved off its root are refused).
Used by: `SPEC.md` 9.1 (row "Real roots: isolating balls with a completeness status").

### Proposition 3.9 (what FLINT 3.0.1 offers for real roots, and what it certifies).

Read: `flint-3.0.1:fmpz_poly.rst:3240` to `3268`, `:1271` to `1274`, `:2277` to `2280`;
`flint-3.0.1:arb_fmpz_poly.rst:66` to `105`; `flint-3.0.1:arb_calc.rst:112` to `138`;
`flint-3.0.1:arb.rst:461` to `466`; `flint-src-3.0.1:fmpz_poly/num_real_roots.c`,
`fmpz_poly/num_real_roots_sturm.c`.

1. (Count.) `fmpz_poly_num_real_roots(pol)` "Returns the number of real roots of the squarefree polynomial
   `pol`"; "The polynomial is assumed to be squarefree" (`fmpz_poly.rst:3264` to `3268`). For a squarefree
   polynomial every root is simple, so this is the number `n` of distinct real roots. It returns a number and no
   intervals. Outside the squarefree-input hypothesis there is NO promised count of distinct roots, and the
   behaviour in the source (`fmpz_poly/num_real_roots.c:107` to `129` and `:155` to `159`) is this: the zero
   polynomial makes the function throw (`:158` to `159`). Otherwise the code strips the full power `X^i` that
   divides the polynomial and ADDS `i` to its answer (`:107` to `117`): if nothing is left it returns `i` (so
   `X^d` returns `d`), a linear rest gives `i + 1`, a quadratic rest gives `i` plus 2 or 0 (`:34` to `40`: 2 if
   `b^2 > 4ac`, else 0, so a non-squarefree quadratic rest, `b^2 = 4ac`, contributes 0), a rest of degree 3 or 4
   with discriminant 0 makes it throw (`:118` to `129`), and a rest of higher degree goes to the Sturm sequence
   (`:140` to `149`). The installed FLINT 3.0.1 returns 2 for `X^2`, 3 for `X^3`, 4 for `X^4`, 3 for
   `X^3 - X^2` (which has two distinct roots) and 0 for `(X - 1)^2` (one distinct root); none throws
   (`probe_s2_flint_real`). So "a non-squarefree quadratic returns 0" and "a non-squarefree cubic or quartic
   throws" are both false in general (they hold for `(X - 1)^2` and for cubics and quartics whose rest after
   stripping `X^i` is degree 3 or 4 with discriminant 0). The first version of this proposition stated these two
   blanket claims (review finding P3.9, INVALID). Nothing in the design depends on a behaviour outside the
   hypothesis: Algorithm RR passes only a nonzero squarefree polynomial (its squarefree part) to this routine,
   and a constant `g` (no real root) is handled before the call.
2. (Isolation.) `arb_fmpz_poly_complex_roots(roots, poly, flags, prec)` writes "all the real and complex
   roots"; "The root enclosures are guaranteed to be disjoint, so that all roots are isolated"; "The real
   roots are written first in ascending order (with the imaginary parts set exactly to zero)"; "The input
   polynomial *must* be squarefree" (`arb_fmpz_poly.rst:68` to `79`). So for a squarefree argument it promises
   `deg` disjoint enclosures, one root in each, and the real roots are those with imaginary part exactly
   zero.
3. (Isolation of the zeros of a function.) `arb_calc_isolate_roots` promises that a subinterval with flag 1
   contains exactly one root and that all roots are isolated if no other flag occurs
   (`arb_calc.rst:127` to `133`), and cannot isolate roots of multiplicity above one or at the end points
   (`arb_calc.rst:136` to `138`). It takes a function pointer; it is not used (`conventions.md` 12.7, and 2
   serves polynomials).
4. (Squarefree part, exact evaluation.) `fmpz_poly_gcd` gives the greatest common divisor in `Z[X]`
   (`fmpz_poly.rst:1271` to `1274`); `fmpz_poly_evaluate_fmpq` evaluates exactly at a rational
   (`fmpz_poly.rst:2277` to `2280`); `arb_get_interval_fmpz_2exp` gives the exact end points of a ball
   (`arb.rst:461` to `466`).

What the design trusts and what it verifies: the enclosures of 2 are not trusted; each is tested by the
exact signs of Proposition 3.8 at its exact end points. The count of 1 is trusted: it is FLINT's promise,
and behind it Sturm's theorem, which is not on disk. `[source pending: Sturm's theorem]` The number of real
enclosures of 2 must equal the count of 1; the two routines are different algorithms.

Check: `probe_s2_flint_real` (20 polynomials of degree at least 1, three of them with coefficients of 80 to 400
digits, precisions 16, 64, 200, FLINT 3.0.1: 60 calls; the count equals the expected number, which the reference
confirms with its own Sturm chain in exact arithmetic; the enclosures pass the checker of 3.8 after `rr_finish`;
20 enclosures are exact points; the counts of `X^2`, `X^3`, `X^4`, `X^3 - X^2`, `(X - 1)^2` are recorded as
above). The probe is a bounded comparison, not a proof of the internal paths of FLINT for large coefficients.
Used by: `SPEC.md` 9.1; Algorithm RR.

### Algorithm RR and Proposition 3.10 (real roots with a completeness status).

Algorithm RR. Input: an integer polynomial `f`, a precision `prec`.

1. If `f = 0`: invalid input (every real number is a root).
2. `g = f/gcd(f, f')`, an integer polynomial (the squarefree part). If `g` is constant: the list is empty and
   complete.
3. `n` = the count of 3.9(1) for `g`.
4. The enclosures of 3.9(2) for `g` at `prec`; keep those with imaginary part exactly zero, in the order
   given; let `[lo_i, hi_i]` be the exact end points of their real parts.
5. For each `i`: if `lo_i = hi_i`, test `g(lo_i) = 0`; else test `g(lo_i) g(hi_i) < 0`, both by exact
   evaluation. If the test fails, replace the interval once by `[lo_i - r, hi_i + r]` with `r` its radius
   (`2^(-prec)` for radius 0) and test again.
6. Test `hi_i < lo_(i+1)` for all `i`; that the number of intervals is `n`; and that every ACTUAL output ball
   (after any widening of step 5) has `arb_rel_accuracy_bits(ball) >= max(prec, 2)`, an exact ball allowed
   (`flint-3.0.1:arb.rst:499` to `509`: the effective relative accuracy in bits, the negative of the position of
   the top bit of the radius minus that of the midpoint, plus one). A `prec` below 2 is taken as 2.
7. If every test holds: the list is complete and is output. Otherwise: `NOT_DETERMINED` at this precision, the
   output untouched.

Claim:

1. If the tests of steps 5 and 6 hold, the intervals contain exactly one real root of `f` each, and every
   real root of `f` lies in one of them; multiple roots of `f` are counted once.
2. The status "complete" is returned only in this case. The answer does not depend on any promise of FLINT
   other than the count of step 3.
3. A real root of multiplicity above one is found like any other; its multiplicity is not reported.
4. (Accuracy.) When the list is complete, every ball has `arb_rel_accuracy_bits >= max(prec, 2)`. This is TESTED
   on the balls that are output, not derived from the promise of the engine: `arb_fmpz_poly_complex_roots` promises
   "a relative accuracy of at least prec bits" for the balls it returns (`arb_fmpz_poly.rst:68` to `69`, `:98`),
   and step 5 may double a radius, which can cost one bit. Example
   (`docs/reviews/s-design/checks/repair_checks.py`): `X - 1`, `prec = 8`, an incoming ball with midpoint
   `1 + 3/1024` and radius `3/1024` (root at the left end
   point) has accuracy 8; widened to radius `3/512` it has accuracy 7, and the sign test and the count still hold.
   Deriving the final accuracy from the incoming promise is therefore wrong; the design returns `NOT_DETERMINED`
   when the test fails (a larger `prec` may succeed). This is a contract counterexample, not an observed output
   of FLINT for `X - 1`; on 60 calls of FLINT (`probe_s2_flint_real`) no ball had to be widened.

*Proof.* Lemma 3.1(2): `g` has the real roots of `f`, all simple. Proposition 3.8 for `g` with the count `n`.
Claim 4 is step 6 itself.

Status: proved modulo (IVT) and the promise of FLINT for the count (3.9(1)).

Check: `check_s2_real_completeness` (the reference, with its own count; accuracy of every ball by a formula
different from the one the reference uses; balls and gaps counted by mpmath), `check_s2_real_planted` (roots known
exactly: repeated, `10^30`, `2^-40` apart, near 0; `X - 10^400`; the first version of the reference raised
`OverflowError` there because it formed the ratio of the coefficients as a float; the bound is now found in
integer arithmetic: `B` doubles while `B |lead| <= |lead| + max |c_i|`), `probe_s2_flint_real` (the engine).
Used by: `SPEC.md` 9.1.

### 3.11 The prime 2, and the examples of the specification.

The statements above hold for `p = 2` without change. What is different at 2 in practice:

1. For `f = X^2 - u` the derivative `2 X` is never a unit at 2, so no root modulo 2 is simple: level 0 of
   Algorithm P never certifies a square root at 2. A certificate has `s >= 1`.
2. `X^2 + 1` at 2 (`SPEC.md` 9.1): `f = (X + 1)^2` modulo 2, the root 1 is double. Level 1:
   `f(1 + 2 Y) = 4 Y^2 + 4 Y + 2`, `w = 1`, `g = 2 Y^2 + 2 Y + 1 = 1` modulo 2, no root. So `f` has no root
   in `1 + 2 Z_2`, hence none in `Z_2`: the list is empty and complete for `D >= 1`. For `D = 0` the class
   `1 + 2 Z_2` is unresolved.
3. `X^2 - 9` at 2 (`SPEC.md` 9.3.3: "9 is a square although the derivative `2x` is not a unit"):
   `f(1 + 2 Y) = 4 (Y^2 + Y - 2)`, `w = 2`, `g = Y (Y + 1)` modulo 2 with the simple roots 0 and 1. So there
   are two roots, in `1 + 4 Z_2` and `3 + 4 Z_2` (they are `-3` and `3`), with `s = w - e = 1`, and the
   certificates need `k >= 2`.
4. `X^2 - 3` at 2: `f(1 + 2 Y) = 4 Y^2 + 4 Y - 2`, `w = 1`, `g = 1` modulo 2: no root. 3 is not a square in
   `Z_2`.
5. The loss of precision: a Newton step from a certificate with derivative valuation `s` gives certified
   precision `2 k - s` (Proposition 3.3), not `2 k`. The certificate requires `k >= s + 1`, and that is
   SUFFICIENT for isolation (Proposition 3.2(1)): the ball `a + p^k Z_p` holds exactly one root. It is not the
   smallest precision of an isolating ball: a particular root may be isolated by a ball of smaller precision. For
   `f = X^3 + 2 X` at 2 the derivative at 0 is 2, so `s = 1`; a nonzero root would satisfy `X^2 = -2`, but a square
   is 0 or 1 modulo 4 and `-2 = 2`, so 0 is the only root in `Z_2` and the ball `Z_2`, of precision 0, isolates
   it (even `2 Z_2`, of precision 1); both are below `s + 1 = 2`. For `X - 1` (`s = 0`) the ball `Z_p` isolates the
   root, precision 0 < 1. If the roots of `f` in `Z_p` are exactly the distinct integers `r_i`,
   the least isolating precision of `r_i` is 0 for a single root and `max_(j != i) v(r_i - r_j) + 1` otherwise.
   It is at most `s_i + 1`, where `s_i = v(g*'(r_i))`. If `g* = product_i (X - r_i)`, then
   `s_i = sum_(j != i) v(r_i - r_j)`; for another `g*` the sum can be smaller than `s_i`
   (`X (X^2 + 2 X + 2)` at 2: the only root is 0, the sum is 0, `s = 1`; edit R11 of the closure check,
   `docs/reviews/s-design/closure.md`). The precision returned by Algorithm P is
   `K = max(k_req, s + 1)`, a sufficient one; it was called the smallest isolating precision in the first
   version, which is false. (The smallest precision allowed by the certificate (R1) is `s + 1`; that is a
   different claim.)

Check: `check_s2_examples` (these cases with their values, and the examples 4.2, 4.3, 4.4 of the source,
`conrad-hensel:hensel.txt:331` to `361`), `check_s2_isolation` (574 planted roots: the least isolating precision is
never above `s + 1`, below it 133 times, equal 440 times; `X^3 + 2 X` at 2).
Used by: `SPEC.md` 9.1, 9.3.3.

### Proposition 3.12 (the seed function).

Hypotheses: `f` in `Z[X]` not zero; `g = g*` its normalised polynomial (Lemma 3.1(3)); `p` a prime; `a` an integer;
`k_req >= 1`. Suppose `g'(a)` is not zero and `v(g(a)) > 2 s`, with `s = v(g'(a))` and `v(0)` infinite (the strong
form for `g`). Claim:

1. There is exactly one root `alpha` of `g` in `a + p^(s+1) Z_p`. It is a root of `f`, a simple root of `g`, and
   `v(g'(alpha)) = s`.
2. Let `K = max(k_req, s + 1)`. There is a root certificate `(a', K, s)` for `(g, p)` with `a' = a` modulo
   `p^(s+1)`; its ball holds exactly `alpha`. It is computed as follows: if `g(a) = 0` it is `(a mod p^K, K, s)`;
   otherwise `k_0 = v(g(a)) - s >= s + 1`, `(a mod p^(k_0), k_0, s)` is a certificate, and it is reduced to `K`
   if `k_0 >= K` (3.2(3)) or lifted by Newton steps until `k >= K` and then reduced (3.3, 3.2(3)).
3. The certificate refers to `g`, not to `f`. The condition and `s` of the seed function are those of `g`.
   Example: `f = 27 X`, `p = 3`, `a = 0`. `g = X`, `g'(0) = 1`, `s = 0`, the certificate is `(0, K, 0)`. With `f`
   in place of `g` one would find `f(0) = 0`, `f'(0) = 27`, `s = 3` and the triple `(0, K, 3)`, which is not a
   certificate for the stored polynomial `g = X`: (R2) fails, `v(g'(0)) = 0`. The strong form for `f` and for `g`
   differ in general (`check_s2_seed` counts 186 seeds among 3276 with `k_req = 1` where one holds and the other
   not).
4. If `g'(a) = 0` or `v(g(a)) <= 2 v(g'(a))` nothing is claimed: the seed function returns `NOT_DETERMINED`.
5. The list of a seed has one ball and says nothing about other roots: `n = 1`, `nu = 0`, `complete = 0`
   (scope SEED, `api-s.md`). A seed list is never complete, and `padic_verify_complete` refuses it.

*Proof.* 1. `v(g(a)) > 2 s` with `s = v(g'(a))` is `|g(a)|_p < |g'(a)|_p^2`. (H2) for `g` (in `Z_p[X]`) gives a
unique `alpha` in `Z_p` with `g(alpha) = 0` and `|alpha - a|_p < |g'(a)|_p = p^(-s)`, that is `alpha` in
`a + p^(s+1) Z_p`, and `|g'(alpha)|_p = |g'(a)|_p`. Roots of `g` in `Q_p` and of `f` are the same (3.1(2)); any
other root of `g` in `a + p^(s+1) Z_p` would violate the uniqueness. 2. If `g(a) = 0`: (R1) `K > s`; (R2)
`v(g'(a mod p^K)) = s` and (R3) `v(g(a mod p^K)) >= K + s` follow as in the proof of 3.2(2) (Taylor at `a`, which
does not use the range of the centre, `K > s`, `v(g(a)) = infinity`). Otherwise `v(g(a)) = k_0 + s`, `k_0 >= s + 1`
by the strong form, and the same computation gives (R2), (R3) for `a mod p^(k_0)` with `k_0`. Reduction and
lifting keep the centre modulo `p^(s+1)` (3.2(3), 3.3(1)); by 3.2(1) the ball `a' + p^(s+1) Z_p = a + p^(s+1) Z_p`
holds one root, `alpha`. 3. Computed. 4, 5. Definitions.

Check: `check_s2_seed` (6552 seeds: 36 polynomials, `p` = 2, 3, 5, negative and large `a`, `k_req` 1 and 4;
1802 give a certificate with the properties above, checked by the oracle of approximate roots, 4750 give
`NOT_DETERMINED` exactly when the condition on `g` fails; `f = 27 X` at 3 is a fixed case).
Used by: `api-s.md`, `adf_root_padic_from_seed`; `SPEC.md` 9.3.3 (the square root at 2).

### Proposition 3.13 (what the two verifiers of a list of roots verify).

Hypotheses: `f` in `Z[X]` not zero; `g*` its normalised polynomial; a list `L` with fields as in `api-s.md`
(`g`, `n`, certificates or balls, classes, `complete`, `scope`). At a prime `p`:

1. (Entries.) If `g` equals `g*`, every listed triple `(a, K, s)` satisfies (R1) to (R3) for `g*`, and the
   listed balls and classes are pairwise disjoint (3.2(4) for the criterion), then every listed ball holds exactly
   one root of `g*` (so of `f`), and different listed balls hold different roots. This says nothing about roots
   outside the listed balls, and nothing about the meaning of a flag `complete` or of a count `n`.
2. (Complete.) If in addition `scope = PARTITION`, `complete = 1`, `nu = 0`, and Algorithm P run on `g*` through a
   depth `D` leaves no unresolved class, and every root ball of that run meets exactly one listed ball and every
   listed ball exactly one root ball of the run, then the roots of `f` in `Z_p` are exactly the roots of the listed
   balls, one in each. If `D` is too small the run leaves a class unresolved and the verifier refuses without
   any claim about `L`.
3. A `SEED` list is never complete.

At the real place:

4. (Entries.) If every ball passes the exact test of Proposition 3.8 for `g*` and `hi_i < lo_(i+1)`, then every
   ball holds at least one real root of `f` and the number of balls is at most the number of real roots
   (3.8(1)), whatever number `n` the list carries.
5. (Complete.) If in addition the number of balls equals the number of real roots of `g*` as recomputed by the
   verifier (the count of 3.9(1), or Sturm), and equals the `n` of the list, then every ball holds exactly one
   real root of `f` and every real root of `f` is in a ball (3.8(2)).

*Proof.* 1. 3.2(1) for each triple; two balls that do not meet hold different roots (3.2(4)). 2. By 3.5(2), (3)
the run of Algorithm P with `U` empty lists every root of `g*` in `Z_p`, each in exactly one root ball, and one
root in each (3.2(1)). Two root balls that meet hold the same root (3.2(4)). Each root of the run lies in exactly
one listed ball, so every root is listed; each listed ball meets exactly one root ball, so it holds a root of the
run and no other. 3. By definition of the scope. 4. 3.8(1). 5. 3.8(2), the count being that of `g*` and
`g*` having the real roots of `f` (3.1(2)).

Examples. (a) `f = X (X - 1)`, `p = 3`, `g = f`, `n = nu = 0`, `complete = 1`, no certificates: every check of 1
passes because there is nothing to check; the flag is false (two roots, 0 and 1). The rerun of 2 finds them, the
matching fails, and the complete verifier returns 0. This is not a counterexample to Proposition 3.5, which is
about the output of Algorithm P; it shows that the entries verifier cannot certify `complete`. (b) `f = (X-1)(X-2)
(X-3)` and the single interval `[0, 4]`: 4 holds (`f(0) f(4) = -36 < 0`, one ball), 5 fails (the count is 3 and
`n = 1` or, with `n = 3`, the number of balls is 1). A stored count is not a trusted count.

Check: `check_s2_lists` (158 lists built on `g*`, 99 of them complete, pass the entries verifier; the complete ones
pass the complete verifier at depth 12 and are refused one depth below the least depth that completes them
(4 polynomials need 2 or more); 151 changed lists are refused: a class kept with `complete = 1`, a certificate
dropped, a ball doubled, a wrong centre, a false `n`; the two examples; `(x-1)^2 (x+2)` at 3 gives a complete list
of two roots because the list is built on `g*`).
Used by: `api-s.md`, `adf_rootlist_verify_entries`, `adf_rootlist_verify_complete`.

## Table of statements

Statements are Definitions (D), Lemmas (L), Propositions (P) and 3.11. The column "Review" is the verdict of
`docs/reviews/s-design/review.md` (V = VALID, M = MINOR, I = INVALID, new = added by the repair, not reviewed) and
what the repair did. The three definitions D1.1, D1.3, D2.1 were omitted from the first table.

| No. | Content | Status | Review | Check |
|---|---|---|---|---|
| D1.1 | the problem and its set of solutions `Sol(m, c, A, B)` | definition | V | `check_s3_coprime` |
| L1.2 | `gcd(d, m) = 1` follows for a reduced fraction; local meaning | proved here | V | `check_s3_coprime` |
| D1.3 | certificate pair (C1) to (C4) | definition | V | `check_s3_certificate` |
| L1.4 | the Euclidean algorithm gives a certificate pair; `gcd(R, T) = gcd(T, m)` | proved modulo (EEA), proof on disk | V | `check_s3_certificate` |
| P1.5 | the points of the lattice in the box, from any certificate pair | proved here | V | `check_s3_param` |
| P1.6 | the complete answer for every `(m, c, A, B)` | proved here | V | `check_s3_complete`, `check_s3_edge` |
| P1.7 | Algorithm R: statuses, "uniqueness not certified", cost | proved here; cost modulo (EEA-cost) | V; claim 3 restated with `ell = max(limit, 0)`, the `limit = 0` reading corrected | `check_s3_limit` |
| P1.8 | ranges in which the answer is "none"; existence | proved here; claim 4 modulo (Thue), proof on disk | V | `check_s3_ranges` |
| P1.9 | FLINT's `fmpq_reconstruct_fmpz_2`: contract, source, outside the contract | claims 1, 2, 3, 5, 6 read from the source; claim 4 (multi-limb paths) not proved, probed only | V | `probe_s3_flint` |
| P1.10 | the residue is not the adelic ball; forgetting map | proved here | V | `check_s3_coprime`, `check_s3_edge`, `check_s3_forget` |
| P1.11 | checking a result of Algorithm R; NOT_UNIQUE is not certified by lowering `B` | proved here | new (repair R9) | `check_s3_verify` |
| D2.1 | span, echelon form, Howell form; prescribed associate (0 for the zero residue) | definition | M; associate of zero and source lines corrected (R1) | `check_s1_assoc` |
| L2.2 | an echelon form has at least `prod N/h_i` elements in its span | proved here | V | `check_s1_howell` |
| L2.3 | Howell property, greedy membership, exact number of elements | proved here | V | `check_s1_howell` |
| P2.4 | the Howell form is canonical over `Z/N` | proved here | V | `check_s1_howell`, `check_s1_canonical` |
| P2.5 | Algorithm H: existence, correctness, cost | proved here | V | `check_s1_howell`, `check_s1_canonical` |
| P2.6 | kernel certificate: soundness, canonical form, existence | proved modulo (Iso), proof on disk | V | `check_s1_solve`, `check_s1_cert_sound`, `check_s1_checker_mutants` |
| P2.7 | no solution: dual vector, soundness and existence | 1, 3 proved here; 2 proved modulo (FAb), proof on disk | V | `check_s1_duality` |
| P2.8 | Algorithm L; zero matrix, `N = 1`, `r = 0`, `c = 0` | proved here, from 2.3 to 2.7 | V | `check_s1_solve`, `check_s1_edge` |
| P2.9 | right-hand sides that are finite balls | proved modulo (D) of `quotient.md` | V | `check_s1_adelic` |
| P2.10 | the solution set as a vector of balls | proved here | V | `check_s1_adelic` |
| P2.11 | FLINT's Howell form: what is promised, what is called; empty matrices | claims 1, 2, 4 read from the sources; claim 3 (equal normal form) not proved, probed only | M; the padding claim restricted to nonempty matrices, line 293 (R2) | `probe_s1_flint`, `probe_s1_flint_empty` |
| P2.12 | the rows of the integer Hermite form of the lattice with pivot below `N` are the Howell form | proved here | V | `probe_s1_hnf` |
| L3.1 | content at `p`; squarefree part has the same roots, all simple; `gcd(g, g') = 1`; the normalised polynomial | proved here, with (EEA-poly) | V; part 3 added | `check_s2_examples`, `check_s2_real_completeness`, `check_s2_lists` |
| D3.2, P3.2 | root certificate; it gives exactly one root, in which ball; equality is not enough | proved modulo (H2), proof on disk | V; claims 2, 3 restated with `k'` (detail asked by the review) | `check_s2_certificate` |
| P3.3 | Newton step: precision `2k - s` | proved here, with (Taylor) | V | `check_s2_newton` |
| P3.4 | one level of the search | proved modulo (H1), proof on disk | V | `check_s2_descent` |
| P3.5 | Algorithm P: soundness, partition, completeness, multiple roots, termination | proved modulo (H1), (H2), (S1) | V | `check_s2_descent`, `check_s2_examples` |
| P3.6 | depth bound `D >= v_p(R) + 1` from an integer Bezout identity `u f + v f' = R` | proved here, with (EEA-poly); a resultant bound is NOT asserted (source pending) | M; was Remark 3.6 (open), now a proposition (R3) | `check_s2_bezout_depth` |
| P3.7 | roots modulo `p` complete: evaluation, or `deg gcd(g, X^p - X)` | proved modulo (Roots), proof on disk, with (EEA-poly) | V | `check_s2_count_mod_p` |
| P3.8 | real roots: count plus isolation gives completeness | proved modulo (IVT), source pending; a stored count is not a count | V | `check_s2_real_completeness` |
| P3.9 | FLINT for real roots: what it offers, what it certifies; outside the squarefree hypothesis nothing is promised | read from the sources; Sturm's theorem source pending | I; claim 1 replaced by the behaviour of the source (R4) | `probe_s2_flint_real` |
| P3.10 | Algorithm RR, with the accuracy test of the final balls | proved modulo (IVT) and FLINT's promise of the count | V; step 6, claim 4 added (R8) | `check_s2_real_completeness`, `check_s2_real_planted`, `probe_s2_flint_real` |
| 3.11 | the prime 2 and the examples; the loss of precision | proved here, by computation | I; item 5 replaced (R5) | `check_s2_examples`, `check_s2_isolation` |
| P3.12 | the seed function; condition, `s` and certificate refer to `g*` | proved modulo (H2), proof on disk | new (repair R7) | `check_s2_seed` |
| P3.13 | what the entries verifier and the complete verifier of a list of roots verify | proved here | new (repair R7) | `check_s2_lists` |

Not proved and stated as such: P1.9 claim 4, P2.11 claim 3 (probed only); IVT and Sturm's theorem (source pending);
the resultant bound (source pending, not asserted); the route 2 of decision S-D10, which the later slice takes
for larger primes (source pending). No numbered statement is open.

## Review record

**Review.** `docs/reviews/s-design/review.md` (refute review, codex `gpt-6-astra`, lane s-review, 2026-09-29):
33 statements, 28 VALID, 3 MINOR (D2.1, P2.11, Remark 3.6), 2 INVALID (P3.9, 3.11), verdict NOT READY before the
repairs. The reconstruction (section 1), the modular linear algebra (section 2) and the p-adic search survived its
independent finite checks: 60480 boxes of reconstruction, 1875272 systems modulo `N`, 32946 Howell probes, 1878
polynomials at each of 2 and 3.

**Repairs of this file (lane s-design-repair, 2026-09-29).**

| Repair | Where | What |
|---|---|---|
| R1 | D2.1 | associate of the zero residue is 0; source lines 481, 521, 565 to 567 |
| R2 | P2.11(2) | padding is for nonempty matrices; empty matrices return early (`strong_echelon_form_mod.c:167`, `howell_form_mod.c:20`); line `fmpz_mod_mat.rst:293` |
| R3 | P3.6 | Remark 3.6 became Proposition 3.6 with its proof; the resultant is not asserted |
| R4 | P3.9(1) | the behaviour outside the squarefree hypothesis is that of the source (`X^d` returns `d`, `X^3 - X^2` returns 3); the two blanket claims are gone |
| R5 | 3.11(5) | `k >= s + 1` is sufficient for isolation, not minimal; `X^3 + 2X` at 2 |
| R6 | reference | `real_roots_ref` finds its bound in integer arithmetic; regression `X - 10^400` |
| R7 | 3.1(3), 3.12, 3.13 | normalisation to `g*`; the seed scope; the entries and the complete verifier |
| R8 | 3.10 | final accuracy tested on the output balls, `arb_rel_accuracy_bits >= max(prec, 2)` |
| R9 | 1.7(3), 1.11 | the four conditions with `ell = max(limit, 0)`; verifier of a result; `count` on every status |
| P3.7 | 3.7, 3.6, 3.1 | the polynomial Euclidean algorithm is cited: `ntb-v2.txt:20462`, `20510` |
| other | D2.1, 2.11 | the order of the checks before a certificate of the optional FLINT engine is consumed (S-D7) |

**Places where the review is held to be wrong.** None of its 33 verdicts is contested; each replacement text was
read against its source (lines quoted above) and none was found false. Two places differ in detail from the review:
the reference `real_roots_ref` refines each ball until the accuracy of R8 holds (it has no engine whose balls could
be widened) and returns `NOT_DETERMINED` only if a test fails afterwards; and Proposition 3.6 is stated for
`gcd(f, f') = 1` (which Lemma 3.1(3) gives for `g*`) rather than for "nonconstant squarefree `f`". The reviewer's
suite `real` still prints its finding `untrusted-real-count`, because `real_cert_ok` keeps its signature and its
meaning (the checker of 3.8 with a trusted count); the repair is `real_verify_complete`. Its check "overflow
reproduction" fails (one failure), because the overflow is repaired.

**What was not done.** The repaired file has not been reviewed again. The multi-limb paths of
`fmpq_reconstruct_fmpz_2` are not proved. Nothing was computed with a C implementation.
