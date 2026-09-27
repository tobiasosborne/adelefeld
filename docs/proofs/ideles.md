# Proofs for ideles, unit cosets and idele classes

Status: written in work package 0.3b (lane m0-proofs-ideles), not yet reviewed. Checked numerically by
`proto/ideles_checks.py`. Covers `SPEC.md` section 5 (review round 1, findings M4 and M5; round 2, finding N12), the
maps of `PLAN.md` 2.3 and the division of `PLAN.md` 2.4.

## 0. Notation and what is used

`v_p` is the p-adic valuation; for a rational `x != 0`, `v_p(x)` is the exponent of `p` in `x`. Balls, `gcd` of
rationals and `R | x` are as in `proofs/policies.md`, section 0. A *unit lift* of an integer `c` with
`gcd(c, N) = 1` is a unit of `Zhat` congruent to `c` modulo `N Zhat`. For an integer `N >= 1` and a prime `p` put
`V_p(p^0) = Z_p^x` and `V_p(p^a) = 1 + p^a Z_p` for `a >= 1`.

Standard facts used and not proved here, with the places in `refs/` (cited as `key:file:line`) where they are
stated:

- (S1) `Z_p` is an integral domain with fraction field `Q_p`. Every `x != 0` in `Q_p` is `p^v w` with a unique
  `v = v_p(x)` in `Z` and `w` in `Z_p^x`; `v_p(x y) = v_p(x) + v_p(y)`; `Z_p = {v_p >= 0} ∪ {0}`;
  `Z_p^x = {v_p = 0}`; `Z_p / p^k Z_p = Z/p^k` via the integers, for `k >= 0`. Sources: `Q_p` is a field,
  `baker-padic:padicnotes.txt:1326` (Theorem 2.31); `Z_p` is a subring and the integers are dense in it,
  `baker-padic:padicnotes.txt:1182` (Proposition 2.28); the p-adic expansion, `baker-padic:padicnotes.txt:1287`
  (Theorem 2.29); the units are the expansions with non-zero first digit, `tate-kudla:kudla-1.txt:85`. The
  statements about `v_p` are read off the expansion.
- (S2) `Zhat` is the product of the `Z_p` over all primes, with the integers embedded diagonally; for `N >= 1`, `z`
  in `Zhat` is congruent to the integer `n` modulo `N Zhat` exactly when `z_p = n` modulo `p^(v_p(N)) Z_p` for every
  `p` dividing `N`. Sources: `Zhat = lim Z/N Z = product of Z_p`, `tate-poonen:notes.txt:1603`; the second clause
  follows from it with the Chinese remainder theorem, `baker-padic:padicnotes.txt:374` (Theorem 1.20).
- (S3) `A_f` is the restricted product of the `Q_p` with respect to the `Z_p`; `A = R x A_f`; `Q` is embedded
  diagonally. `A^x` and `A_f^x` carry the restricted product topology with respect to the `Z_p^x`. Sources:
  `tate-kudla:kudla-1.txt:93` (adeles) and `:115` (ideles); `milne-cft:CFT.txt:9373` (restricted product and its
  topology, the citation for the topology); `tate-warwick:tatesthesis_notes.txt:414` (the ideles are the units of
  `A`, with a topology that is not the subspace topology).
- (S4) A positive rational is the finite product of `p^(v_p(r))`; it is an integer exactly when all `v_p(r) >= 0`.
  Source: `milne-ant:ANT.txt:321`, "The fundamental theorem of arithmetic says that every nonzero integer m can be
  written in the form, m = +-p1 ... pn, pi a prime number, and that this factorization is essentially unique" (lines
  321 to 323, symbols as extracted by pdftotext). Applied to the numerator and the denominator of `r` in lowest
  terms, it gives both statements.
- (S5) Fermat: `w^(p-1) = 1` modulo `p` for `w` in `Z_p^x` (via `Z_p/p Z_p = Z/p`). Source:
  `baker-padic:padicnotes.txt:462` (Theorem 1.26).
- (S6) A polynomial of degree `g > 0` over a field has at most `g` roots. Source: `baker-padic:padicnotes.txt:454`
  (Corollary 1.25).

## 1. Decomposition of a finite idele (M5)

### Definition 1.

A finite idele is a unit of the ring `A_f`. An idele is `(x_inf, x_f)` with `x_inf` a non-zero real and `x_f` a
finite idele.

### Lemma 2 (units of `A_f`).

Claim: `x` in `A_f` is a unit exactly when `x_p != 0` for every `p` and `x_p` is in `Z_p^x` for all but finitely
many `p`.

*Proof.*
1. If `x y = 1` in `A_f`, then `x_p y_p = 1` for every `p`, so `x_p != 0`. By (S3) `x_p` and `y_p` are in `Z_p` for
   all `p` outside a finite set; there `v_p(x_p) + v_p(y_p) = 0` with both `>= 0`, so `v_p(x_p) = 0`.
2. Conversely put `y_p = 1/x_p`. Where `x_p` is in `Z_p^x`, `y_p` is in `Z_p^x`; this is all but finitely many `p`,
   so `y` is in `A_f` and `x y = 1`.

Remark (M4): the adele `(p)_p` (coordinate `p` at each prime `p`) has no zero coordinate, but `v_p(p) = 1` at every
`p`, so it is not a unit: its inverse `(1/p)_p` is not in `A_f`.

Check: `check_decomposition`.
Used by: `SPEC.md` 4.5, 5.

### Proposition 3 (unique decomposition).

Hypotheses: `x` a finite idele. Claim: there is exactly one pair `(r, u)` with `r` a positive rational and `u` in
`Zhat^x` such that `x = r u`; `r` is the product of `p^(v_p(x_p))` over all `p`. For a non-zero rational `q`
(embedded diagonally) `r = |q|` and `u = sign(q)`.

*Proof.*
1. By Lemma 2, `n_p = v_p(x_p)` is 0 for all but finitely many `p`, so `r = product of p^(n_p)` is a positive
   rational with `v_p(r) = n_p` (S4).
2. Put `u = x / r`. Then `v_p(u_p) = n_p - n_p = 0` for every `p` (S1), so `u_p` is in `Z_p^x` for every `p`, and
   `u` is in `Zhat^x` (S2).
3. If `r u = r' u'`, then `r/r' = u'/u` is a positive rational with `v_p = 0` at every `p`, hence equal to 1 (S4);
   so `r = r'` and `u = u'`.
4. For a rational `q != 0`, `v_p(q)` at each `p` gives `r = |q|` (S4), and `u = q/|q| = sign(q)`.

Check: `check_decomposition`.
Used by: `SPEC.md` 5 ("Decomposition").

## 2. Unit cosets (M5, N12)

### Definition 4.

For an integer `N >= 1`, `U(N)` is the set of `u` in `Zhat^x` with `u = 1` modulo `N Zhat`. For an integer `c` with
`gcd(c, N) = 1`, `c U(N)` is the set of unit lifts of `c`: the `u` in `Zhat^x` with `u = c` modulo `N Zhat`.

### Lemma 5 (local description).

Hypotheses: `N >= 1`, `gcd(c, N) = 1`, `a_p = v_p(N)`. Claim:

1. `u` in `Zhat^x` is in `c U(N)` exactly when `u_p = c` modulo `p^(a_p) Z_p` for every `p` dividing `N`.
2. `U(N)` is the product over all `p` of `V_p(p^(a_p))`; it is a subgroup of `Zhat^x`.
3. `c U(N)` is not empty, and for any `u_0` in it, `c U(N) = u_0 U(N)`.

*Proof.*
1. (S2) applied to `u - c`.
2. By 1 with `c = 1`, `u` is in `U(N)` exactly when `u_p` is in `V_p(p^(a_p))` for every `p` (for `p` not dividing
   `N` the condition is `u_p` in `Z_p^x = V_p(p^0)`). Each `V_p(p^a)` is a group: for `a >= 1`,
   `(1 + p^a x)(1 + p^a y) = 1 + p^a (x + y + p^a x y)` and `(1 + p^a x)^(-1) = 1 - p^a x (1 + p^a x)^(-1)`.
3. Put `u_p = c` for `p` dividing `N` (a unit of `Z_p`, since `p` does not divide `c`) and `u_p = 1` otherwise. By
   1, `u` is in `c U(N)`. For `u_0` in `c U(N)` and `w` in `U(N)`, `u_0 w = c` modulo `N Zhat` by 1, prime by prime.
   For `u` in `c U(N)` and `p | N`, `u_p` and `u_{0,p}` are both `c` modulo `p^(a_p)` and `u_{0,p}` is a unit, so
   `u_p/u_{0,p} = 1` modulo `p^(a_p)`; hence `u/u_0` is in `U(N)`.

Check: `check_unit_cosets`.
Used by: `SPEC.md` 5 ("Unit precision").

### Proposition 6 (a unit coset is not the additive ball).

Hypotheses: `N >= 1`, `gcd(c, N) = 1`. Claim:

1. `c U(N) = (c + N Zhat) ∩ Zhat^x`.
2. For every prime `p` not dividing `N`, `c + N Zhat` contains an element whose `p`-coordinate is 0; so `c U(N)` is
   a proper subset of `c + N Zhat`.
3. Different additive balls can carry the same unit coset: `5 U(6) = 2 U(3)`, while `5 + 6 Zhat` and `2 + 3 Zhat`
   differ.

*Proof.*
1. Definition 4.
2. At `p` not dividing `N`, `N` is in `Z_p^x`. Let `z` have `z_p = -c/N` (in `Z_p`) and `z_q = 0` for `q != p`. Then
   `z` is in `Zhat` and `c + N z` has `p`-coordinate 0; it is not a unit.
3. The first equality is Lemma 7 below. The balls differ by `precision.md` Proposition 3(1) (radii 6 and 3).

Check: `check_unit_cosets`.
Used by: `SPEC.md` 5.

### Lemma 7 (the factor 2).

Hypotheses: `N >= 1` with `v_2(N) = 1`, `gcd(c, N) = 1`. Claim: `c U(N) = c U(N/2)`. In particular
`U(2) = U(1) = Zhat^x` and `U(2 N) = U(N)` for odd `N`.

*Proof.*
1. `c` is odd, since `2 | N` and `gcd(c, N) = 1`.
2. By Lemma 5.1 the two sets are described by the same conditions at the odd primes dividing `N`; the only
   difference is the condition `u_2 = c` modulo 2 in `c U(N)`.
3. Every `u_2` in `Z_2^x` is odd (S1: `v_2(u_2) = 0` means `u_2` is not in `2 Z_2`), and `c` is odd, so that
   condition always holds.

Check: `check_canonical`.
Used by: `SPEC.md` 5 (N12).

### Definition 8 (canonical form).

For `(c, N)` with `gcd(c, N) = 1`: `Nbar = N/2` if `v_2(N) = 1`, otherwise `Nbar = N`; `cbar = c mod Nbar` in
`[0, Nbar)`. Note `v_2(Nbar)` is never 1. For `N = 1` or `N = 2` the canonical form is `(0, 1)`, the whole `Zhat^x`.

### Proposition 9 (containment, equality, overlap of cosets).

Hypotheses: `(c, N)` and `(c', N')` with `N, N' >= 1`, `gcd(c, N) = gcd(c', N') = 1`. Claim:

1. `c U(N)` is inside `c' U(N')` exactly when `Nbar'` divides `Nbar` and `c = c'` modulo `Nbar'`.
2. `c U(N) = c' U(N')` exactly when the canonical forms are equal. So equality of cosets is decidable, and the
   canonical form is a complete invariant of the set.
3. `c U(N)` and `c' U(N')` meet exactly when `c = c'` modulo `gcd(N, N')`.

*Proof.*
1. By Lemma 7 we may replace `(c, N)` by `(c, Nbar)` and `(c', N')` by `(c', Nbar')`.
2. (If) Let `u` be in `c U(Nbar)`: `u = c` modulo `Nbar Zhat`, hence modulo `Nbar' Zhat`, hence `u = c'` modulo
   `Nbar' Zhat`. So `u` is in `c' U(Nbar')`.
3. (Only if, divisibility) Suppose the containment and suppose some prime `p` has `m = v_p(Nbar') > a = v_p(Nbar)`.
   Put `w_p = 1 + p^(m-1)` and `w_q = 1` for `q != p`.
   - `w_p` is a unit: if `m >= 2`, `w_p = 1` modulo `p`; if `m = 1`, then `p` is odd (as `v_2(Nbar') != 1`) and
     `w_p = 2` is a unit of `Z_p`.
   - `w_p` is in `V_p(p^a)`: `a <= m - 1`.
   - `w_p` is not in `V_p(p^m)`: `w_p - 1 = p^(m-1)` has valuation `m - 1`. For `p = 2`, `m >= 2` because
     `v_2(Nbar') != 1` and `m > a >= 0`; so `m - 1 >= 1` and `w_2` is odd.
   - Consequently `w` is in `U(Nbar)` (Lemma 5.2) and not in `U(Nbar')`. Take `u_0` in `c U(Nbar)` (Lemma 5.3). Then
     `u_0` and `u_0 w` are in `c U(Nbar)`, hence in `c' U(Nbar') = u_0 U(Nbar')` (Lemma 5.3), so `w` is in
     `U(Nbar')`. This is a contradiction; hence `v_p(Nbar') <= v_p(Nbar)` for all `p`, that is `Nbar' | Nbar`.
4. (Only if, residues) With `u_0` as in step 3: `u_0 = c` modulo `Nbar`, hence modulo `Nbar'`; and `u_0 = c'` modulo
   `Nbar'`. So `c = c'` modulo `Nbar'`.
5. Item 2: both containments give `Nbar | Nbar'` and `Nbar' | Nbar`, so `Nbar = Nbar'`, and `c = c'` modulo `Nbar`,
   so `cbar = cbar'`. Conversely equal canonical forms give the same set by Lemma 7.
6. Item 3: if `u` is in both, `c = u = c'` modulo `gcd(N, N')`. Conversely, if `c' - c = t gcd(N, N')`, write
   `gcd(N, N') = alpha N + beta N'` (Bezout) and put `x = c + alpha N t = c' - beta N' t`. Then `x = c` modulo `N`
   and `x = c'` modulo `N'`; it is coprime to `N` and to `N'`. A unit lift of `x` for the modulus `lcm(N, N')`
   (Lemma 5.3) lies in both cosets.

Examples: `5 mod 6` and `2 mod 3` have the canonical form `(2, 3)`; `1 mod 2` has `(0, 1)`; `3 mod 4` and `1 mod 4`
are different cosets.

Check: `check_canonical`.
Used by: `SPEC.md` 5 ("equality of cosets compares the sets"); `PLAN.md` 5 (printing), 7 ("`[5 mod 6]` and
`[2 mod 3]` are equal").

## 3. Product, inverse and power

Products of sets are taken with independent factors: `X Y = {x y : x in X, y in Y}`.

### Proposition 10 (product and inverse at one modulus).

Hypotheses: `N >= 1`, `gcd(c, N) = gcd(c', N) = 1`, `c*` an integer with `c c* = 1` modulo `N`. Claim:

1. `c U(N) * c' U(N) = (c c') U(N)`, as sets.
2. `{u^(-1) : u in c U(N)} = c* U(N)`, as sets.
3. `c U(N) * c* U(N) = U(N)`; in particular it contains 1.

*Proof.*
1. With unit lifts `u_0`, `u_0'` (Lemma 5.3): the left side is `u_0 u_0' U(N) U(N)`, and `U(N) U(N) = U(N)` because
   `U(N)` is a group containing 1. `u_0 u_0' = c c'` modulo `N`, so the set is `(c c') U(N)` by Lemma 5.3.
2. `(u_0 w)^(-1) = u_0^(-1) w^(-1)` and `w -> w^(-1)` is a bijection of `U(N)`; `u_0^(-1) = c*` modulo `N` because
   `u_0 c* = c c* = 1` modulo `N`.
3. By 1 with `c' = c*`: `(c c*) U(N) = 1 U(N)`.

Check: `check_products`.
Used by: `SPEC.md` 5 ("At a common `N`, product and inverse keep the finite precision exactly"); `PLAN.md` 2.1.

### Proposition 11 (product at different moduli; the modulus `gcd(N, N')` is best possible).

Hypotheses: `N, N' >= 1`, `gcd(c, N) = gcd(c', N') = 1`, `g = gcd(N, N')`. Claim:

1. `U(N) U(N') = U(g)`, and `c U(N) * c' U(N') = (c c') U(g)`, as sets.
2. A coset `c'' U(M)` contains the product set exactly when `Mbar | gbar` and `c'' = c c'` modulo `Mbar`. So
   `(c c') U(g)` is the smallest coset containing the product: `gcd(N, N')` is the best possible modulus, up to the
   factor 2 of Lemma 7 (the moduli giving the same set are exactly those with canonical form `gbar`).
3. `gbar = gcd(Nbar, N'bar)`.

*Proof.*
1. Write `a_p = v_p(N)`, `b_p = v_p(N')`, so `v_p(g) = min(a_p, b_p)`. For `a >= b >= 0`, `V_p(p^a)` is inside
   `V_p(p^b)` (for `b >= 1`, `1 + p^a Z_p` is inside `1 + p^b Z_p`; for `b = 0` everything is inside `Z_p^x`).
2. `U(N)` and `U(N')` are inside `U(g)` (Lemma 5.2 and step 1), and `U(g)` is a group, so `U(N) U(N')` is inside
   `U(g)`.
3. Let `w` be in `U(g)`. Put `u_p = w_p, v_p = 1` where `a_p <= b_p`, and `u_p = 1, v_p = w_p` where `a_p > b_p`.
   Where `a_p <= b_p`, `w_p` is in `V_p(p^(min)) = V_p(p^(a_p))`; otherwise in `V_p(p^(b_p))`. By Lemma 5.2, `u` is
   in `U(N)` and `v` in `U(N')`, and `u v = w`. So `U(g)` is inside `U(N) U(N')`.
4. With unit lifts `u_0`, `u_0'`: the product set is `u_0 u_0' U(N) U(N') = u_0 u_0' U(g) = (c c') U(g)`.
5. Item 2 is Proposition 9.1 applied to the coset `(c c') U(g)`.
6. Item 3: for odd `p`, `v_p` is unchanged by the canonical form. At 2: if `min(a_2, b_2) = 1`, then one of
   `a_2, b_2` is 1 and becomes 0, and `v_2(gbar) = 0 = min` of the new values; if `min = 0` both sides are 0; if
   `min >= 2` nothing changes.

The finer input's precision is lost because the product *set* is this coset, not because of the rule.

Check: `check_products` (the smallest coset is found by trying every modulus up to a bound).
Used by: `SPEC.md` 5 ("At different moduli the product is known modulo `gcd(N, M)`").

### Lemma 12 (lifting the exponent).

Hypotheses: `p` a prime, `x` in `Z_p` with `v_p(x - 1) = a`, where `a >= 1`, and `a >= 2` if `p = 2`; `k` a non-zero
integer. Claim: `v_p(x^k - 1) = a + v_p(k)`.

*Proof.*
1. `m >= 1` not divisible by `p`: `x^m - 1 = (x - 1)(1 + x + ... + x^(m-1))`; each `x^j = 1` modulo `p`, so the
   second factor is `m` modulo `p`, a unit. So `v_p(x^m - 1) = a`.
2. `k = p`: write `x = 1 + y`, `v_p(y) = a`. Then `x^p - 1 = y S` with `S = sum_{i=1}^{p} C(p, i) y^(i-1)` (binomial
   theorem). The term `i = 1` is `p`. For `2 <= i <= p - 1`, `p | C(p, i)` and `v_p(y^(i-1)) >= a >= 1`, so the
   valuation is `>= 2`. The term `i = p` is `y^(p-1)` with valuation `a (p - 1)`, which is `>= 2` for `p >= 3`; for
   `p = 2` there are only the terms `2` and `y`, and `v_2(y) = a >= 2`. So `S = p` modulo `p^2`, `v_p(S) = 1`, and
   `v_p(x^p - 1) = a + 1`. The element `x^p` satisfies the hypothesis with `a + 1`.
3. `k = p^e m` with `e >= 0`, `m >= 1`, `p` not dividing `m`: step 2 `e` times, then step 1.
4. `k < 0`: `x^k - 1 = -x^k (x^(-k) - 1)` and `x^k` is a unit; `v_p(k) = v_p(-k)`.

Check: `check_power` (indirectly) and `check_lte`.
Used by: Proposition 13.

### Proposition 13 (integer power of a unit coset).

Hypotheses: `N >= 1`, `gcd(c, N) = 1`, canonical modulus `Nbar`, `a_p = v_p(Nbar)` (so `a_2 != 1`), `k` a non-zero
integer, `e_p = v_p(k)`. Let `P_k = {u^k : u in c U(N)}` (one unknown raised to the power `k`). Define
`M_k = product of p^(b_p)` with

| prime | condition | `b_p` |
|---|---|---|
| `p` odd | `a_p >= 1` | `a_p + e_p` |
| `p` odd | `a_p = 0`, `p - 1` divides `k` | `1 + e_p` |
| `p` odd | `a_p = 0`, `p - 1` does not divide `k` | 0 |
| 2 | `a_2 >= 2` | `a_2 + e_2` |
| 2 | `a_2 = 0`, `k` even | `2 + e_2` |
| 2 | `a_2 = 0`, `k` odd | 0 |

Claim:

1. `M_k` is a positive integer (only finitely many `b_p > 0`), `v_2(M_k) != 1`, and `Nbar | M_k`.
2. Let `chat` be an integer with `chat = c` modulo `Nbar` and `gcd(chat, M_k) = 1` (one exists: an integer congruent
   modulo `M_k` to a unit lift of `c`, by (S2)). Then `P_k` is inside `chat^k U(M_k)`, and the residue
   `chat^k mod M_k` does not depend on the choice of `chat`.
3. A coset `c'' U(M)` contains `P_k` exactly when `Mbar | M_k` and `c'' = chat^k` modulo `Mbar`. So `chat^k U(M_k)`
   is the smallest coset containing `P_k`.
4. The simple rule `c^k U(N)` (with `c^k` read modulo `Nbar`, inverse for `k < 0`) contains `P_k`; it is the
   smallest coset exactly when `M_k = Nbar`. For `k = -1` and `k = 1` this always holds.
5. `P_k` is in general a proper subset of its smallest coset: for `N = 1`, `k = 2`, `M_2 = 24`, and `P_2` contains
   no element that is 2 modulo 5, while `U(24)` does.
6. For `k = 0`, `P_0 = {1}`. No coset `c'' U(M)`, `M >= 1`, equals it, and there is no smallest coset containing it
   (the `U(M)` decrease without bound as `M` grows).

*Proof.*
1. `b_p > 0` only for `p | Nbar`, for `p = 2`, and for odd `p` with `(p - 1) | k`, which is a finite set as
   `k != 0`. `b_2` is 0 or at least 2. In each row `b_p >= a_p` (rows with `a_p = 0` trivially), so `Nbar | M_k`.
2. *Local images.* For each prime `p` let `W_p = {w^k : w in V_p(p^(a_p))}`. We show: `W_p` is inside
   `V_p(p^(b_p))`, and (tightness) `W_p` is not inside `V_p(p^(b_p + 1))`, except when `p = 2` and `b_2 = 0`, where
   `W_2` is not inside `V_2(4)`.
   - Rows `a_p >= 1` (odd `p`) and `a_2 >= 2`: for `w != 1` in `V_p(p^(a_p))`, `v_p(w - 1) >= a_p` and Lemma 12
     gives `v_p(w^k - 1) = v_p(w - 1) + e_p >= a_p + e_p`. For `w = 1 + p^(a_p)`, it is exactly `a_p + e_p`.
   - Odd `p`, `a_p = 0`, `(p - 1) | k`: for `w` in `Z_p^x`, `x = w^(p-1)` is 1 modulo `p` (S5). If `x != 1`, Lemma
     12 with `k/(p - 1)` gives `v_p(w^k - 1) = v_p(x - 1) + v_p(k/(p-1)) >= 1 + e_p` (as `p` does not divide
     `p - 1`). For `w = 1 + p`, Lemma 12 gives exactly `1 + e_p`.
   - Odd `p`, `a_p = 0`, `(p - 1)` not dividing `k`: containment in `Z_p^x` is trivial. Let `t = gcd(k, p - 1)`,
     `t < p - 1`. If `w^k = 1` modulo `p` then `w^t = 1` modulo `p` (write `t = alpha k + beta (p - 1)`; S5). By
     (S6) at most `t` of the `p - 1` non-zero residues satisfy `w^t = 1`; take an integer `w` in `[1, p - 1]` that
     does not. Then `w^k` is not in `V_p(p)`.
   - `p = 2`, `a_2 = 0`, `k` even: for odd `w = 1 + 2 s`, `w^2 = 1 + 4 s (s + 1)` and `s (s + 1)` is even, so
     `v_2(w^2 - 1) >= 3`. Lemma 12 with `x = w^2` and `k/2` gives `v_2(w^k - 1) >= 3 + e_2 - 1 = 2 + e_2` (if
     `x != 1`). For `w = 3`, `v_2(9 - 1) = 3` and the value is exactly `2 + e_2`.
   - `p = 2`, `a_2 = 0`, `k` odd: `w = -1` gives `w^k = -1`, and `-1 - 1 = -2` is not in `4 Z_2`.
3. *Reduction to local images.* Let `u_0` be a unit lift of `c` modulo `Nbar` that is also a unit lift of `chat`
   modulo `M_k` (Lemma 5.3 with the modulus `lcm(Nbar, M_k) = M_k` and the residue `chat`). Then `P_k = u_0^k W`
   with `W = {w^k : w in U(Nbar)}`, and by Lemma 5.2, `W` is the product of the `W_p`, the components being chosen
   independently.
4. *Containment.* By step 2, `W` is inside the product of the `V_p(p^(b_p))`, which is `U(M_k)`. So `P_k` is inside
   `u_0^k U(M_k) = chat^k U(M_k)` (`u_0^k = chat^k` modulo `M_k`).
5. *Independence of `chat`.* The coset `chat^k U(M_k)` contains `P_k`, which does not depend on `chat`; two cosets
   of the same `U(M_k)` that share a point are equal (Lemma 5.3), so `chat^k mod M_k` does not depend on `chat`.
6. *Minimality.* Let `c'' U(M)` contain `P_k`. It contains `u_0^k`, so it equals `u_0^k U(M)`, and `U(M)` contains
   `W`. For each `p`, taking `w` with arbitrary `w_p` in `V_p(p^(a_p))` and `w_q = 1` elsewhere shows that
   `V_p(p^(v_p(M)))` contains `W_p`. By the tightness in step 2, `v_p(M) <= b_p`, or `p = 2`, `b_2 = 0` and
   `v_2(M) <= 1`. In canonical form this says `Mbar | M_k`. The residue condition follows from Proposition 9.1.
   Conversely such a coset contains `chat^k U(M_k)` by Proposition 9.1.
7. Item 4: `c^k U(N) = c^k U(Nbar)` contains `chat^k U(M_k)` by Proposition 9.1, since `Nbar | M_k` and
   `chat^k = c^k` modulo `Nbar`. For `k = +1, -1`: `e_p = 0`, and `(p - 1) | k` forces `p = 2`, excluded in that
   row; `k` is odd; so `M_k = Nbar`.
8. Item 5: `M_2`: `b_2 = 2 + 1 = 3`, `b_3 = 1 + 0 = 1` (`2 | 2`), and no other odd `p` has `(p - 1) | 2`. So
   `M_2 = 24`. The squares of units are 1 or 4 modulo 5; a unit lift of 97 modulo 120 lies in `U(24)` (97 = 1 modulo
   24) and is 2 modulo 5, so it is not in `P_2`.
9. Item 6: for an odd prime `p` not dividing `M`, the element with `p`-coordinate 2 and all other coordinates 1 is
   in `U(M)` (Lemma 5.2) and is not 1; so `c'' U(M) = {1}` is impossible. A coset containing 1 is `U(M)`, and
   `U(M p^2)` is properly inside `U(M)` for any odd prime `p` (Proposition 9.1: `Mbar p^2` does not divide `Mbar`),
   so there is no smallest.

Consequence for the specification: a power operation on unit cosets needs this rule to be tight; the rule `c^k U(N)`
is an enclosure. The exponent 0 cannot be represented exactly by the coset type (item 6). See the report.

Check: `check_power` (cosets modulo 16, `|k| <= 4`), `check_power_local` (each row prime by prime, `|k| <= 30`, and
`k = 0`).
Used by: `PLAN.md` 2.1 ("multiply, invert, power"); `SPEC.md` 9.3.4 item 1.

## 4. Absolute values, norm, class map (M5)

### Proposition 14 (absolute values, norm, product formula).

Hypotheses: an idele `x = (x_inf, r u)` (Proposition 3). Claim:

1. `|x_p|_p = p^(-v_p(x_p)) = p^(-v_p(r))` for every `p`; it is 1 for all but finitely many `p`.
2. The product of the `|x_p|_p` over all `p` is `1/r`, and the norm is
   `|x| = |x_inf| * product_p |x_p|_p = |x_inf| / r`.
3. For a non-zero rational `q` (diagonal), `|q| = 1`.
4. `v_p(r)` at a named prime `p` is computed by repeated division of the numerator and the denominator of `r` by
   `p`; no factorisation of `r` is needed.

*Proof.*
1. `x_p = r u_p` with `u_p` a unit, so `v_p(x_p) = v_p(r)` (S1). `v_p(r) = 0` for `p` outside the primes of `r`.
2. `product_p p^(-v_p(r)) = 1/r` by (S4).
3. By Proposition 3, `r = |q|`; `|q|_inf = |q|`; so `|q| = |q|/|q| = 1`.
4. `v_p(r) = v_p(numerator) - v_p(denominator)`, each the number of times `p` divides.

Agrees with: the absolute value of an idele, `tate-warwick:tatesthesis_notes.txt:405` (Definition 1.18), and the
product formula, `tate-poonen:notes.txt:1579`.

Check: `check_norm`.
Used by: `SPEC.md` 5 ("Operations"); `PLAN.md` 2.2.

### Proposition 15 (the idele class group of `Q`).

Define `Phi(x) = (|x_inf| / r, sign(x_inf) u)` for an idele `x = (x_inf, r u)`. Claim:

1. `Phi` is a surjective group homomorphism `A^x -> R_{>0} x Zhat^x` with kernel `Q^x`. Hence
   `A^x / Q^x = R_{>0} x Zhat^x` as groups, and the class of `x` has the coordinates `t = |x_inf|/r`,
   `u' = sign(x_inf) u`.
2. Every class contains exactly one idele of the form `(t, u')` with `t > 0` and `u'` in `Zhat^x`, namely
   `x / (sign(x_inf) r)`.
3. The map without the sign, `Psi(x) = (|x_inf|/r, u)`, is not constant on classes.
4. The isomorphism is a homeomorphism, modulo (S3) and the continuity statements in the proof.

*Proof.*
1. *Homomorphism.* If `x = (x_inf, r u)` and `y = (y_inf, s w)`, then `x y = (x_inf y_inf, (r s)(u w))`, and by the
   uniqueness in Proposition 3 its decomposition is `(r s, u w)`. `|.|` and `sign` are multiplicative on `R^x`.
2. *Kernel.* `Phi(x) = (1, 1)` means `|x_inf| = r` and `u = sign(x_inf)`, so `x_f = r sign(x_inf) = x_inf`: `x` is
   the diagonal rational `x_inf`. Conversely for `q` in `Q^x`, `r = |q|` and `u = sign(q)` (Proposition 3), so
   `Phi(q) = (1, sign(q)^2) = (1, 1)`.
3. *Surjective.* `Phi((t, u')) = (t, u')` for `t > 0`, `u'` in `Zhat^x`.
4. Item 2: `q = sign(x_inf)/r` is in `Q^x` and `x q = (|x_inf|/r, sign(x_inf) u)`. Two such representatives in one
   class have `Phi` values equal, and `Phi((t, u')) = (t, u')`, so they are equal.
5. Item 3: `Psi(-1) = (1, -1)` and `Psi(1) = (1, 1)`, but `-1` and `1` are in the same class (both in `Q^x`).
6. Item 4: on each open set `R^x x r * product_p Z_p^x` (these cover `A^x`, S3), `Phi` is
   `x -> (|x_inf|/r, sign(x_inf) x_f / r)` with `r` fixed, which is continuous; so `Phi` is continuous and induces a
   continuous bijection from the quotient. Its inverse is `(t, u') -> class of (t, u')`, the composition of the
   inclusion `R_{>0} x Zhat^x -> A^x` (continuous for the restricted product topology, S3) and the quotient map.

On data: for an idele `(X_inf, r, c U(N))` with a real ball `X_inf` that excludes 0, all points of `X_inf` have the
same sign, so the class is `(|X_inf|/r, (sign(X_inf) c) U(N))`; the finite precision `U(N)` is kept exactly.

Agrees with: the decomposition `A^x = Q^x x R_{>0} x Zhat^x`, `x = alpha t u`, `tate-kudla:kudla-1.txt:123`
(equation (1.1)), and `tate-poonen:notes.txt:1603`.

Check: `check_class_map`.
Used by: `SPEC.md` 5 ("The class of an idele ... the sign on the unit is essential").

## 5. Idele to adele (M5)

### Proposition 16 (the simple ball and the smallest ball).

Hypotheses: `r > 0` rational, `N >= 1`, `gcd(c, N) = 1`; `L = lcm(N, 2)`; `c'` an odd integer with `c' = c` modulo
`N` (`c' = c` if `N` is even; `c` or `c + N` if `N` is odd). Claim:

1. (Simple ball) `r c U(N)` is inside `r c + r N Zhat`.
2. (Smallest ball) `r c U(N)` is inside `r c' + r L Zhat`.
3. Every ball `b + R Zhat` (`R >= 0` rational) that contains `r c U(N)` contains `r c' + r L Zhat`.
4. The smallest ball depends only on the set: `(c, N)` and its canonical form give the same ball. The simple ball is
   the smallest exactly when `N` is even.

*Proof.* Multiplication by `r` is a bijection of `A_f` taking `a + R Zhat` to `r a + r R Zhat`; so take `r = 1`.
1. Proposition 6.1.
2. Let `u` be in `c U(N)`. Then `u = c` modulo `N Zhat`, and `u_2` is odd (S1), so `u = 1` modulo `2 Zhat` (S2). As
   `c'` is odd and `c' = c` modulo `N`, the integer `c'` satisfies both congruences, so `u - c'` is in
   `N Zhat ∩ 2 Zhat = L Zhat` (prime by prime, S2).
3. Fix `u_0` in `c U(N)` (Lemma 5.3); then `b + R Zhat = u_0 + R Zhat`. For each prime `p` choose `w` in `c U(N)`
   with `w_q = u_{0,q}` for `q != p` and
   - (a) `w_p = u_{0,p} (1 + p^a)` if `a = v_p(N) >= 1`: a unit, `= u_{0,p}` modulo `p^a`; the difference
     `u_{0,p} p^a` has valuation `a = v_p(L)`;
   - (b) `w_p = 2 u_{0,p}` if `p` is odd and does not divide `N`: a unit; the difference `u_{0,p}` has valuation
     `0 = v_p(L)`;
   - (c) `w_2 = 3 u_{0,2}` if `N` is odd: a unit; the difference `2 u_{0,2}` has valuation `1 = v_2(L)`.
   - In each case, by Lemma 5.1, `w` is in `c U(N)` (the conditions at `p | N` hold), and `w != u_0`, so `R > 0` (a
     ball of radius 0 is one point). So `w - u_0` is in `R Zhat`, whose `p`-component gives
     `v_p(R) <= v_p(w_p - u_{0,p}) = v_p(L)`. This holds for every `p`, so `L/R` is a positive rational with all
     valuations `>= 0`, an integer (S4). By `precision.md` Proposition 3(3), `u_0 + L Zhat` (which is `c' + L Zhat`
     by item 2) is inside `u_0 + R Zhat`.
4. If `v_2(N) = 1`, `L = N = 2 Nbar = lcm(Nbar, 2)`, and the odd representative agrees; otherwise `Nbar = N`. If `N`
   is even, `L = N` and `c' = c`; if `N` is odd, `L = 2 N` and the simple ball is strictly coarser (Proposition 3(1)
   of `precision.md`).

Example: `5 U(6) = 2 U(3)`: simple balls `5 + 6 Zhat` and `2 + 3 Zhat`; smallest ball `5 + 6 Zhat` for both.

Check: `check_idele_to_adele`.
Used by: `SPEC.md` 5 ("Idele to adele"); `PLAN.md` 2.3 ("tightness of the small hull").

## 6. Non-invertibility and division (M4)

### Proposition 17 (no finite ball of positive radius certifies invertibility).

Hypotheses: `a` rational, `N > 0` rational. Claim: for every prime `p` that divides neither the numerator nor the
denominator of `N` nor the denominator of `a` (all but finitely many primes), `a + N Zhat` contains an element whose
`p`-coordinate is 0. Hence every adelic ball `I x (a + N Zhat)` with `N > 0` contains non-invertible adeles,
whatever the real interval `I`.

*Proof.*
1. At such `p`, `a` is in `Z_p` and `N` is in `Z_p^x`, so `a + N Z_p = Z_p`, which contains 0.
2. Let `z_p = -a/N` (in `Z_p`) and `z_q = 0` for `q != p`; `z` is in `Zhat`, and `x = a + N z` has `x_p = 0` and
   `x_q = a` elsewhere.
3. By Lemma 2, `x` is not a unit of `A_f`; the adele `(t, x)` is not a unit of `A` for any real `t`.

The condition on the denominator of `a` matters: `1/2 + Zhat` contains no element with 2-coordinate 0 (M4).

Check: `check_noninvertible`.
Used by: `SPEC.md` 4.5 ("division by an `adf_adele` is not defined"); `PLAN.md` 2.4 (status when the divisor is only
an adele).

### Proposition 18 (division by an exact non-zero rational).

For a rational `q != 0` and an adelic ball `I x (a + M Zhat)`, `M >= 0`: the set of quotients is
`(I/q) x (a/q + (M/|q|) Zhat)`. The finite part is exact; the real part is rounded as usual.

*Proof.* Multiplication by `1/q` is a bijection of `A_f` taking `Zhat` to `(1/|q|) Zhat` (as `-Zhat = Zhat`).

Check: `check_division`.
Used by: `SPEC.md` 4.5, 9.1 (first row); `PLAN.md` 2.4.

### Proposition 19 (division by an idele: the radius rule).

Hypotheses: an adelic ball `X = I x (a + M Zhat)`, `M >= 0`; an idele enclosure `y = (Y_inf, r, c U(N))` with
`Y_inf` a real ball excluding 0, `r > 0`, `N >= 1`. Let `c*` be the inverse of `c` modulo `N`, `L = lcm(N, 2)` and
`e` an odd integer with `e = c*` modulo `N`. Claim: the set of finite parts of `x/y` (`x` in `X`, `y` in the idele
set) is

    S = (1/r) * { (a + M z) w : z in Zhat, w in c* U(N) },

and the smallest ball containing `S` is

    (a e)/r + (gcd(|a| L, M) / r) Zhat.

For `a = 0` this is `(M/r) Zhat`; for `M = 0` it is `a e/r + (|a| L/r) Zhat`. The same ball is obtained by the
product rule of `SPEC.md` 4.3 applied to `a + M Zhat`, the smallest ball `e + L Zhat` of `c* U(N)` (Proposition 16)
and the exact `1/r`. With the simple ball `c* + N Zhat` in its place the radius is `gcd(|a| N, M)/r`, which can be
coarser by the factor 2. The real part is `I / Y_inf`, rounded as usual.

*Proof.*
1. By Proposition 3 and Proposition 10.2, the finite parts of `1/y` are `(1/r) c* U(N)`.
2. For a unit `w`, `(a + M Zhat) w = a w + M Zhat`, since `w Zhat = Zhat`. So `r S = T + M Zhat` with
   `T = a c* U(N)`.
3. For `a != 0`, the smallest ball containing `T` is `a e + |a| L Zhat`: multiplication by `a` is a bijection taking
   balls to balls (`a (b + R Zhat) = a b + |a| R Zhat`), and Proposition 16 with `r = 1`. For `a = 0`, `T = {0}`.
4. *Smallest ball of `T + M Zhat`.* Let `t_0 + L' Zhat` be the smallest ball containing `T` (`L' = |a| L`,
   `t_0 = a e`; for `a = 0`, `L' = 0`, `t_0 = 0`). Then `T + M Zhat` is inside `t_0 + gcd(L', M) Zhat`
   (`precision.md` Lemma 2). Conversely let a ball `B` contain `T + M Zhat`. `T` is not empty; choose `t` in `T`
   (`t_0` itself need not lie in `T`: the integer `e` need not be a unit of `Zhat`). `B` contains `t` and `t + M`.
   `B` also contains `T` (as `0` is in `M Zhat`), hence `t_0 + L' Zhat` by the minimality in step 3; so `t_0` is in
   `B` and `B = t_0 + R Zhat`. If `R = 0`, `B` is one point, so `t = t + M` gives `M = 0` and `t_0 + L' Zhat` being
   one point gives `L' = 0`. Otherwise `M = (t + M) - t` and `L' = (t_0 + L') - t_0` are rational differences of
   elements of `B`, so `R | M` and `R | L'` (`precision.md` Lemma 1), so `R | gcd(L', M)` (G2 of `policies.md`), and
   `B` contains `t_0 + gcd(L', M) Zhat`.
5. Multiplication by `1/r` gives the claim.
6. *Agreement with the product rule.* `precision.md` Proposition 2 for `(a + M Zhat)(e + L Zhat)` gives the radius
   `gcd(a L, e M, M L)`. As `e` is odd and coprime to `N`, `gcd(e, L) = 1`, so `gcd(e M, L M) = M` (G1) and the
   radius is `gcd(|a| L, M)`. With the simple ball: `gcd(a N, c* M, N M) = gcd(|a| N, M)` in the same way. Since
   `L/N` is 1 or 2, the two radii differ at most by the factor 2.

Example: `a = 1`, `M = 4`, `r = 1`, `c U(N) = U(1)` (so `L = 2`, `e = 1`): the quotients `(1 + 4 z) w` all have odd
2-coordinate, and the smallest ball is `1 + 2 Zhat`; the simple ball gives radius `gcd(1, 4) = 1`, all of `Zhat`.

Check: `check_division`.
Used by: `SPEC.md` 4.5, 5; `PLAN.md` 2.4 ("against multiplication by the inverse").

## Table of statements

| No. | Content | Status | Check |
|---|---|---|---|
| D1 | finite idele, idele | definition | |
| L2 | units of `A_f`; `(p)_p` is not a unit | proved modulo (S1), (S3) | `check_decomposition` |
| P3 | unique decomposition `x = r u` | proved modulo (S1), (S2), (S4) | `check_decomposition` |
| D4 | `U(N)` and `c U(N)` | definition | |
| L5 | local description; subgroup; cosets not empty | proved modulo (S1), (S2) | `check_unit_cosets` |
| P6 | unit coset is not the additive ball | proved here | `check_unit_cosets` |
| L7 | `U(2N) = U(N)` for odd `N` | proved modulo (S1) | `check_canonical` |
| D8 | canonical form | definition | |
| P9 | containment, equality by canonical form, overlap | proved here | `check_canonical` |
| P10 | product and inverse at one modulus, exact | proved here | `check_products` |
| P11 | product at different moduli is `U(gcd)`; best modulus | proved here | `check_products` |
| L12 | lifting the exponent | proved here | `check_lte` |
| P13 | tight power rule `M_k`; `k = 0` not representable | proved modulo (S5), (S6) | `check_power_local` |
| P14 | absolute values, norm, product formula | proved modulo (S4) | `check_norm` |
| P15 | class map with the sign; `A^x/Q^x = R_{>0} x Zhat^x` | proved; topology modulo (S3) | `check_class_map` |
| P16 | simple ball; smallest ball `r lcm(N, 2)`, minimal | proved mod (S1), (S2), (S4) | `check_idele_to_adele` |
| P17 | no finite ball of positive radius certifies invertibility | proved here | `check_noninvertible` |
| P18 | division by an exact rational | proved here | `check_division` |
| P19 | division by an idele, smallest ball `gcd(abs(a) L, M)/r` | proved here | `check_division` |

## Review record

Date 2026-09-27. Reviewer: codex gpt-6-astra, `docs/reviews/m0-proofs/ideles-review.md` (checks
`docs/reviews/m0-proofs/ideles_review_checks.py`). Verdicts on this file: 15 VALID, 1 MINOR (P19), 0 INVALID.

- P19: the minimality step used `t_0 + M` in `B` before showing it; now a point `t` of `T` gives `t, t + M` in `B`,
  and the zero cases `R = 0`, `M = 0`, `a = 0` are treated.
- (S4): the pending source is replaced by `milne-ant:ANT.txt:321` (read and quoted).
- (S3): `milne-cft:CFT.txt:9373` is named as the citation for the topology.
- Checks: `check_power_local` added (every row of the table of Proposition 13 prime by prime, `|k| <= 30`, `k = 0`);
  `check_division` now covers `a = M = 0` and division by negative exact rationals.
