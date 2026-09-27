# Proofs for the quotient by Q and for rational reconstruction

Status: written in work package 0.3b (lane m0-proofs-ideles), not yet reviewed. Checked numerically by
`proto/quotient_checks.py`. Covers `SPEC.md` section 6 (reduction modulo `Q` and splitting; review round 1, finding
M6; round 2, remainder of M6) and section 9.2 (finding M12). The additive character is not treated here.

## 0. Notation and what is used

`A = R x A_f`, with `Q` embedded diagonally; `A/Q` is the quotient group. Balls, `gcd` of rationals and `R | x` are
as in `proofs/policies.md`, section 0. A real interval `[lo, hi]` has rational (or real) endpoints `lo <= hi`; a
real ball of `arb` is such an interval after outward rounding. `floor` and `ceil` are the usual integer parts.

From `proofs/precision.md`: Lemma 1 (`R Zhat ∩ Q = R Z`, line 13), Lemma 2 (line 20), Proposition 3 (line 59).

Standard facts used and not proved here, with the places in `refs/` (cited as `key:file:line`) where they are
stated:

- (S1) `Z_p` has the valuation `v_p`; `Z_p^x = {v_p = 0}`; the integers are dense in `Z_p`: for `y` in `Z_p` and
  `k >= 0` there is an integer `n` with `y - n` in `p^k Z_p`. `Z_p` is complete: a sequence of integers `(x_n)` with
  `x_(n+1) - x_n` in `p^n Z_p` converges in `Z_p`. Sources: `baker-padic:padicnotes.txt:1182` (Proposition 2.28:
  every element of `Z_p` is a limit of integers, every Cauchy sequence of integers has a limit in `Z_p`);
  `baker-padic:padicnotes.txt:1287` (Theorem 2.29, expansion); `tate-kudla:kudla-1.txt:85` (units).
- (S2) `Zhat` is the product of the `Z_p`; `A_f` is the restricted product of the `Q_p` with respect to the `Z_p`.
  Sources: `tate-poonen:notes.txt:1603`; `tate-kudla:kudla-1.txt:93`.
- (D) Every `z` in `Zhat` is congruent modulo `N Zhat` to an integer, for each positive integer `N`. Source:
  `Zhat = lim Z/N Z`, `tate-poonen:notes.txt:1603` (the class of `z` modulo `N` is its component in `Z/N Z`).
- (T) `Zhat` is compact; `A` carries the product of the real topology and the restricted product topology, and the
  quotient map `A -> A/Q` is continuous. Sources: `Z_p` is compact, `baker-padic:padicnotes.txt:1914` (Theorem
  4.20); Tychonoff, `milne-cft:CFT.txt:9214`, "Tychonoff's theorem says that a product of compact spaces is compact"
  (lines 9214 to 9215); the restricted product topology of `A`, `milne-cft:CFT.txt:9373`.
- (E) Euler's criterion in the form used: for an odd prime `p` and integers `b, b'` prime to `p`, if neither `b` nor
  `b'` is a square modulo `p`, then `b b'` is. This follows from the cyclicity of `(Z/p)^x`,
  `baker-padic:padicnotes.txt:473` (Theorem 1.28): with a generator `g`, the squares are the even powers of `g`, and
  the product of two odd powers is an even power.

## 1. The fundamental domain (M6)

### Lemma 1 (`A_f = Q + Zhat`).

Claim: every `x` in `A_f` is `q + z` with `q` rational and `z` in `Zhat`; `q` is unique modulo `Z`.

*Proof.*
1. By (S2) there is a finite set `S` of primes outside which `x_p` is in `Z_p`. For `p` in `S` choose `k >= 0` with
   `p^k x_p` in `Z_p` (S1: `x_p = p^v w`), and by density (S1) an integer `n_p` with `p^k x_p - n_p` in `p^k Z_p`;
   so `x_p - n_p/p^k` is in `Z_p`.
2. Put `q = sum over p in S of n_p/p^k` (each term with its own `k`). At `p` in `S`:
   `x_p - q = (x_p - n_p/p^k) - sum over p' != p of n_(p')/p'^(k')`, and each `n_(p')/p'^(k')` is in `Z_p` since
   `p' != p`. At `p` outside `S`, `x_p` and `q` are in `Z_p`. So `z = x - q` is in `Zhat`.
3. If `q + z = q' + z'`, then `q - q'` is a rational in `Zhat`, hence an integer (`precision.md` Lemma 1).

Check: `check_fundamental_domain`.
Used by: `SPEC.md` 6 ("Reduction modulo `Q`").

### Proposition 2 (unique representative in `[0,1) x Zhat`).

Claim: every `x = (x_inf, x_f)` in `A` is congruent modulo `Q` to exactly one point of `F = [0,1) x Zhat`. With `q`
from Lemma 1 and `n = floor(x_inf - q)`, it is `(x_inf - q - n, x_f - q - n)`.

*Proof.*
1. `x - q = (x_inf - q, z)` with `z` in `Zhat`. Subtracting the integer `n` (in `Q`, and in `Zhat`) gives a real
   part in `[0, 1)` and a finite part `z - n` in `Zhat`.
2. If two points of `F` are congruent, they differ by a rational `r` whose finite part lies in `Zhat - Zhat = Zhat`,
   so `r` is an integer (`precision.md` Lemma 1). Their real parts lie in `[0, 1)` and differ by `r`, so `|r| < 1`
   and `r = 0`.

Check: `check_fundamental_domain`.
Used by: `SPEC.md` 6; `SPEC.md` 2, fact 2.

### Proposition 3 (the gluing of the closed domain).

Claim: two points `(s, z)`, `(s', z')` of `[0, 1] x Zhat` are congruent modulo `Q` exactly when they are equal, or
`{s, s'} = {0, 1}` and the finite part of the point with real part 1 is the other finite part plus 1. In particular
`(1, z)` is the point `(0, z - 1)`.

*Proof.*
1. As in Proposition 2, step 2, a difference `r` in `Q` must be an integer, and `|r| = |s - s'| <= 1`.
2. `r = 0` gives equality. `r = 1` or `-1` forces `{s, s'} = {0, 1}`, and `z' = z + r`.

Check: `check_fundamental_domain`.
Used by: `SPEC.md` 6 ("the point `(1 ; z)` is the point `(0 ; z - 1)`").

### Corollary 4 (`Q` is discrete, `A/Q` is compact).

1. `Q ∩ ((-1/2, 1/2) x Zhat) = {0}`, so `Q` is a discrete subgroup of `A`.
2. `A/Q` is the image of the compact set `[0, 1] x Zhat`, hence compact (modulo (T)).
3. `F = [0,1) x Zhat` is not compact, and the closed domain `[0,1] x Zhat` maps onto `A/Q` two-to-one exactly over
   the points of Proposition 3.

*Proof.* 1: a rational in `Zhat` is an integer; an integer in `(-1/2, 1/2)` is 0. 2: Proposition 2 and (T). 3:
`[0,1)` is not compact; Proposition 3.

Check: `check_fundamental_domain`.
Used by: `SPEC.md` 2, fact 2.

## 2. Reduction of a ball with integer radius

Throughout this section: `X = I x (a + N Zhat)` with `I = [lo, hi]`, `a` rational, `N` a positive integer; put
`lo' = lo - a`, `hi' = hi - a`. For an integer `n` let

    H_n = ( ([lo', hi'] ∩ [n, n+1)) - n )  x  (-n + N Zhat)       (half-open piece)
    C_n = ( [max(lo', n), min(hi', n+1)] - n )  x  (-n + N Zhat)   (closed piece, real part inside [0, 1])

### Proposition 5 (reduction, one piece per integer part).

Claim: the set of representatives in `F` of the points of `X` is the union of the `H_n` over the integers
`floor(lo') <= n <= floor(hi')`; each of these pieces is non-empty; there are `floor(hi') - floor(lo') + 1` of them.

*Proof.*
1. Let `(t, z)` be in `X`. As `z - a` is in `N Zhat`, which is inside `Zhat`, Lemma 1 holds with `q = a`. With
   `n = floor(t - a)`, Proposition 2 gives the representative `(t - a - n, z - a - n)`. Here `t - a` is in
   `[lo', hi'] ∩ [n, n+1)` and `z - a - n` is in `-n + N Zhat`; so the representative is in `H_n`, and
   `floor(lo') <= n <= floor(hi')`.
2. Conversely let `(s, w)` be in `H_n`: `s + n` is in `[lo', hi']` and `w + n` is in `N Zhat`. The point
   `(s + n + a, w + n + a)` is in `X` and differs from `(s, w)` by the rational `n + a`; since `(s, w)` is in `F`,
   it is the representative of that point (Proposition 2).
3. For `n = floor(lo')`, `lo'` is in `[n, n+1)`; for `floor(lo') < n <= floor(hi')`, `n` itself is in `[lo', hi']`.
   So each piece is non-empty.

Example (`SPEC.md` 6): `I = [0.9, 1.1]`, `a = 0`, `N = 2`: pieces `[0.9, 1) x (0 + 2 Zhat)` and
`[0, 0.1] x (-1 + 2 Zhat)`.

Check: `check_reduction`.
Used by: `SPEC.md` 6 (first bullet and "one piece for each integer it crosses").

### Proposition 6 (closed pieces with the gluing).

Claim:

1. The image in `A/Q` of the union of the `C_n`, `floor(lo') <= n <= floor(hi')`, equals the image of `X`: closed
   pieces add no point.
2. If `lo' < hi'`, the closed pieces with `floor(lo') <= n <= ceil(hi') - 1` already suffice. Their number is
   `k + 1`, where `k` is the number of integers in the open interval `(lo', hi')`. If `lo' = hi'`, one piece.
3. So a real interval whose shifted interior contains `k` integers gives `k + 1` closed pieces; "crossing one
   integer gives two pieces" (`SPEC.md` 6) is the case `k = 1`.

*Proof.*
1. `C_n` contains `H_n`. A point `(s, w)` of `C_n` has `s + n` in `[lo', hi']` and `w + n` in `N Zhat`, so as in
   Proposition 5, step 2, it is congruent to the point `(s + n + a, w + n + a)` of `X`.
2. If `hi'` is an integer `m > lo'`, the piece `n = m` of Proposition 5 is `H_m = {0} x (-m + N Zhat)`. A point
   `(0, w)` of it is congruent (Proposition 3) to `(1, w + 1)`, with `w + 1` in `-(m - 1) + N Zhat`; this point lies
   in `C_(m-1)`, whose real part ends at `min(hi', m) - (m - 1) = 1` (and `m - 1 >= floor(lo')`, since `lo' < m`).
   So `C_m` is not needed. If `hi'` is not an integer, `ceil(hi') - 1 = floor(hi')`. Either way the indices run from
   `floor(lo')` to `ceil(hi') - 1`.
3. The integers `j` with `lo' < j < hi'` are `floor(lo') + 1, ..., ceil(hi') - 1`, so
   `k = ceil(hi') - floor(lo') - 1`, and the number of pieces `ceil(hi') - floor(lo')` is `k + 1`. If `lo' = hi'`,
   Proposition 5 gives one piece.

Remark: the wording of `SPEC.md` 6, "gives one piece for each integer it crosses", should read: splitting at the `k`
integers strictly inside the shifted interval gives `k + 1` closed pieces with the gluing rule; a one-point interval
gives one piece. This counts the construction, not a minimum: pieces with the same finite part (`n = n'` modulo `N`)
can merge afterwards (with `N = 1` and `I = [0, 2]` the two pieces together are the whole quotient).

Check: `check_reduction`.
Used by: `SPEC.md` 6 ("pieces are therefore closed intervals inside `[0,1]` together with the gluing rule").

### Proposition 7 (when the image is everything).

Claim: the image of `X` in `A/Q` is all of `A/Q` exactly when `hi - lo >= N`. In general a point `(s, w)` of `F`
(`w` an element of `Zhat`) is in the image exactly when the interval `[lo' - s, hi' - s]` contains an integer `j`
with `j = -w` modulo `N`.

*Proof.*
1. `(s, w)` is congruent to a point `(s + q, w + q)` of `X` with `q` rational exactly when `s + q` is in `I` and
   `w + q - a` is in `N Zhat`. The second condition makes `q - a` a rational in `Zhat`, hence an integer `j`
   (`precision.md` Lemma 1), with `w + j` in `N Zhat`; the first says `j` is in `[lo' - s, hi' - s]`.
2. If `hi - lo >= N`, every closed interval of length at least `N` contains an integer of each class modulo `N`.
3. If `l = hi - lo < N`: take `delta` with `0 < delta < N - l`, and `s = (lo' - delta) - m` with
   `m = floor(lo' - delta)`, so `s` is in `[0, 1)` and `lo' - s = m + delta`. The interval
   `[m + delta, m + delta + l]` lies inside `(m, m + N)` and contains at most `N - 1` integers, so some class `j0`
   modulo `N` has no member in it. With `w = -j0` (an integer), `(s, w)` is not in the image.

Check: `check_full_image`.
Used by: `SPEC.md` 6 (a piece limit can be met by returning "everything" when `hi - lo >= N`).

## 3. Fractional radius

### Proposition 8 (splitting `A/B` into `B` balls of integer radius).

Hypotheses: `a` rational, `N = A/B > 0` with `A, B` positive coprime integers. Claim:

1. `a + N Zhat` is the disjoint union of the `B` balls `a + k N + A Zhat`, `0 <= k < B`, each of integer radius `A`.
2. A ball of positive integer radius meets at most one of these `B` pieces. Hence `a + N Zhat` is not covered by
   fewer than `B` balls of integer radius (contained in it or not).

*Proof.*
1. (Cover) Let `z` be in `Zhat`. By (D) there is an integer `k`, `0 <= k < B`, with `z - k` in `B Zhat`, say
   `z = k + B z'`. Then `a + N z = a + k N + A z'`.
2. (Disjoint) If `a + k N + A z = a + k' N + A z'`, then `(k - k') N = A (z' - z)` is in `A Zhat`, so
   `(k - k') N / A = (k - k')/B` is a rational in `Zhat`, an integer (`precision.md` Lemma 1). With `|k - k'| < B`,
   `k = k'`.
3. (Item 2) Let `c + R Zhat`, `R` a positive integer, meet the pieces `k` and `k'`: points `x = a + k N + A z` and
   `y = a + k' N + A z'` with `x - y` in `R Zhat`, which is inside `Zhat`. Then `(k - k') N = (x - y) - A (z - z')`
   is in `Zhat`, a rational, hence an integer; so `B` divides `(k - k') A`, hence `k - k'`, and `k = k'`.
4. A cover by balls of integer radius needs one ball per piece, since every piece is non-empty.

Example: `N = 1/2` gives `Zhat` and `1/2 + Zhat`. After splitting, each piece is reduced by section 2.

Check: `check_split`.
Used by: `SPEC.md` 6 (second bullet).

## 4. Equality of reduced sets

### Proposition 9 (canonical form of a finite union of closed pieces).

Hypotheses: a finite family of closed pieces `J_i x (m_i + N_i Zhat)` with `J_i` a closed interval inside `[0, 1]`,
`m_i` an integer, `N_i` a positive integer. Let `N'` be a common multiple of the `N_i`. For each class `m` modulo
`N'` let `T_m` be the subset of `[0, 1)` formed by

- `J_i ∩ [0, 1)` for every `i` and every `j`, `0 <= j < N'/N_i`, with `m_i + j N_i = m` modulo `N'`;
- the point `0` for every `i`, `j` as above with `m_i + j N_i = m + 1` modulo `N'` and `1` in `J_i`.

Claim: the image of the union in `A/Q`, read in `F`, is the union over `m` of `T_m x (m + N' Zhat)`. Two families
have the same image exactly when, with a common `N'`, their sets `T_m` agree for every `m`. Each `T_m` is a finite
union of intervals; with rational (or dyadic) endpoints the comparison is exact.

*Proof.*
1. Put `B' = N'/N_i`. `m_i + N_i Zhat` is the disjoint union of the `m_i + j N_i + N' Zhat`, `0 <= j < B'`: for `z`
   in `Zhat`, (D) gives `z = j + B' z'` with `0 <= j < B'`, so `m_i + N_i z = m_i + j N_i + N' z'`; and if
   `j N_i - j' N_i` is in `N' Zhat`, then `(j - j')/B'` is a rational in `Zhat`, an integer, so `j = j'`.
2. A point `(s, z)` of a piece with `s < 1` is its own representative in `F`; with `s = 1` its representative is
   `(0, z - 1)` (Proposition 3), and `z - 1` is in the class `m` when `z` is in the class `m + 1`.
3. The classes `m + N' Zhat`, `0 <= m < N'`, partition `Zhat`, so a subset of `F` of the form
   `union_m T_m x (m + N' Zhat)` determines each `T_m`, and conversely.
4. Two finite unions of intervals in `[0, 1)` are equal exactly when they agree at every endpoint and at one
   interior point of each gap between consecutive endpoints, because membership is constant on each such open gap.
   With exact endpoints this is a finite exact comparison.

Check: `check_mixed_families` (arbitrary families with different moduli `N_i`, against membership read off
Proposition 3; the review's witness `[1/4, 1/2] x (0 + 2 Zhat)` with a modulus-3 piece), `check_translation`.
Used by: `SPEC.md` 6 (`adf_qclass` as a union of pieces); `PLAN.md` 3.1.

### Proposition 10 (translation by a rational changes nothing).

Hypotheses: `X = I x (a + N Zhat)` with `N >= 0` rational, `q0` rational. Claim:

1. `X + q0 = (I + q0) x (a + q0 + N Zhat)` has the same image in `A/Q` as `X`, so, for `N > 0`, the same canonical
   form (Proposition 9).
2. The pieces produced by Propositions 8 and 5 (reduction with the shift `q` equal to the centre of each split
   piece) are literally the same for `X` and `X + q0`.
3. If the real interval of `X + q0` is rounded outward to `I'`, containing `I + q0`, its pieces contain those of `X`
   (the intervals `[lo', hi']` only grow).

*Proof.*
1. `x` and `x + q0` differ by an element of `Q`.
2. Splitting uses only `N`; the split centres are `a + q0 + k N`; the shifted endpoints are
   `lo + q0 - (a + q0 + k N) = lo - a - k N`, the same as for `X`; so are the finite parts `-n + A Zhat`.
3. Each `H_n` and `C_n` is monotone in `[lo', hi']`.

For radius `N = 0` the finite part is the point `a`; the reduction shifts by `a` and gives one piece per integer
part of `[lo', hi']`, with the points `-n` as finite parts; items 1 to 3 hold in the same way.

Check: `check_translation`.
Used by: `PLAN.md` 3.1 ("equality of the sets after translation by a rational").

## 5. Rational reconstruction (M12)

### Proposition 11 (from a full adelic ball).

Hypotheses: `X = I x (a + N Zhat)`, `I = [lo, hi]`, `a` rational, `N >= 0` rational. Claim: the rationals `q` with
`(q, q)` in `X` are

- for `N > 0`: the `a + N k` with `k` an integer, `ceil((lo - a)/N) <= k <= floor((hi - a)/N)`; there are
  `max(0, floor((hi - a)/N) - ceil((lo - a)/N) + 1)` of them; at most one if `hi - lo < N`; at least one if
  `hi - lo >= N`;
- for `N = 0`: `a` if `lo <= a <= hi`, and none otherwise.

*Proof.*
1. `N > 0`: a rational `q` is in `a + N Zhat` exactly when `q - a` is in `N Zhat ∩ Q = N Z` (`precision.md` Lemma
   1), that is `q = a + N k`. The condition `q` in `I` is `lo <= a + N k <= hi`, that is the stated range of `k`.
2. Two distinct candidates differ by at least `N`, so at most one fits into an interval of length less than `N`; an
   interval of length at least `N` contains one.
3. `N = 0`: the finite part is the single point `a`.

No lattice reduction and no height bound is needed.

Check: `check_reconstruct_full` (membership tested prime by prime, `v_p(q - a) >= v_p(N)`, not by Lemma 1).
Used by: `SPEC.md` 9.2 (first item); `PLAN.md` 1.6.

### Proposition 12 (no local-to-global principle).

Claim: `f(x) = (x^2 - 13)(x^2 - 17)(x^2 - 221)` has a real root, a root in `Z_p` for every prime `p`, and no
rational root. The family of these roots is an adele `x` (all finite coordinates in `Z_p`) with `f(x) = 0` in `A`.

*Proof.*
1. *No rational root.* `Q` is a field, so a rational root is a root of one factor. If `(m/n)^2 = b` with `m/n` in
   lowest terms, `n > 0`, and `b` an integer, then `m^2 = b n^2`; a prime dividing `n` would divide `m`, so `n = 1`
   and `b` is a square. `13`, `17`, `221 = 13 * 17` are not squares (`3^2 < 13 < 4^2`, `4^2 < 17 < 5^2`,
   `14^2 < 221 < 15^2`).
2. *Real root.* `sqrt(13)`.
3. *Lifting at an odd prime.* Let `p` be odd, `b` an integer prime to `p`, and `x_1` an integer with `x_1^2 = b`
   modulo `p`. Then `p` does not divide `x_1`. Given `x_n` with `x_n^2 = b` modulo `p^n` and `p` not dividing `x_n`,
   put `x_(n+1) = x_n + t p^n`, where `t` solves `2 x_n t = -(x_n^2 - b)/p^n` modulo `p` (possible: `2 x_n` is prime
   to `p`). Then `x_(n+1)^2 - b = (x_n^2 - b) + 2 x_n t p^n + t^2 p^(2n)`, which is 0 modulo `p^(n+1)`. The sequence
   converges in `Z_p` (S1) to a root of `x^2 - b`.
4. *Odd `p` other than 13, 17.* If 13 or 17 is a square modulo `p`, step 3 applies to it. Otherwise 221 is a square
   modulo `p` by (E), and step 3 applies to 221 (prime to `p`).
5. *`p = 13`:* `17 = 4 = 2^2` modulo 13; step 3 with `b = 17`. *`p = 17`:* `13 = 64 = 8^2` modulo 17; step 3 with
   `b = 13`.
6. *`p = 2`:* `17 = 1` modulo 8. Given an odd `x_n` with `x_n^2 = 17` modulo `2^n`, `n >= 3` (start: `x_3 = 1`),
   either `x_n^2 = 17` modulo `2^(n+1)`, and `x_(n+1) = x_n`, or `x_n^2 - 17 = 2^n` modulo `2^(n+1)`, and
   `x_(n+1) = x_n + 2^(n-1)` gives `x_(n+1)^2 - 17 = (x_n^2 - 17) + 2^n x_n + 2^(2n-2)`, which is
   `2^n (1 + x_n) = 0` modulo `2^(n+1)` because `x_n` is odd and `2n - 2 >= n + 1`. The sequence converges in `Z_2`.
7. All roots chosen are in `Z_p` (they are limits of integers), so the family is in `R x Zhat`, inside `A`.

Check: `check_local_global` finds roots modulo `2^12` at 2 (exhaustive search), modulo `p^4` for odd `p < 50` and
modulo `p^2` for `50 <= p < 200` (a root modulo `p` found by search, lifted by the formula of step 3, and verified).
It checks the absence of rational roots among the signed divisors of the constant term.
Used by: `SPEC.md` 9.2 (last paragraph).

### Proposition 13 (partial data: uniqueness when `2 A B < m`).

Hypotheses: `m > 0`, an integer `c`, bounds `A >= 0`, `B >= 1`. A solution is a reduced fraction `n/d` with `d > 0`,
`gcd(d, m) = 1`, `n = c d` modulo `m`, `|n| <= A`, `d <= B`. Claim:

1. If `2 A B < m`, there is at most one solution.
2. The bound is sharp in the sense that `2 A B = m` admits two solutions: `m = 2`, `A = B = 1`, `c = 1` has the
   solutions `1/1` and `-1/1`.
3. This is a different problem from Proposition 11: `1/5` solves it for `c = 5`, `m = 6`, yet `1/5` is not in the
   ball `5 + 6 Zhat`.

*Proof.*
1. Let `n_1/d_1`, `n_2/d_2` be solutions. Then `n_1 d_2 = c d_1 d_2 = n_2 d_1` modulo `m`, so `m` divides
   `n_1 d_2 - n_2 d_1`, whose absolute value is at most `|n_1| d_2 + |n_2| d_1 <= 2 A B < m`. So `n_1 d_2 = n_2 d_1`
   and the fractions are equal; both being reduced with positive denominators, `n_1 = n_2` and `d_1 = d_2`.
2. `1 = 1 * 1` and `-1 = 1 * 1` modulo 2; both have `d = 1`.
3. `1 = 5 * 5` modulo 6. By Proposition 11, `1/5` is in `5 + 6 Zhat` only if `(1/5 - 5)/6 = -4/5` is an integer.

Results of the function are therefore one verified candidate, none, several, or "uniqueness not certified" (when
`2 A B >= m` and a search is not made).

Check: `check_partial`.
Used by: `SPEC.md` 9.2 (second item).

## Table of statements

| No. | Content | Status | Check |
|---|---|---|---|
| L1 | `A_f = Q + Zhat`, `q` unique modulo `Z` | proved modulo (S1), (S2) | `check_fundamental_domain` |
| P2 | unique representative in `[0,1) x Zhat` | proved here | `check_fundamental_domain` |
| P3 | gluing `(1, z) = (0, z - 1)`, and nothing else | proved here | `check_fundamental_domain` |
| C4 | `Q` discrete, `A/Q` compact | proved modulo (T), cited | `check_fundamental_domain` |
| P5 | reduction of a ball, one half-open piece per integer part | proved here | `check_reduction` |
| P6 | closed pieces add nothing; `k + 1` pieces for `k` integers crossed | proved here | `check_reduction` |
| P7 | image is all of `A/Q` exactly when `hi - lo >= N` | proved here | `check_full_image` |
| P8 | splitting `A/B` into `B` balls of radius `A`; `B` is minimal | proved modulo (D) | `check_split` |
| P9 | canonical form of a union of pieces; decidable equality | proved here | `check_mixed_families` |
| P10 | translation by a rational: same pieces | proved here | `check_translation` |
| P11 | reconstruction from a full ball, including `N = 0` | proved here | `check_reconstruct_full` |
| P12 | `(x^2-13)(x^2-17)(x^2-221)`: local roots, none in `Q` | proved mod (S1), (E) | `check_local_global` |
| P13 | partial data: unique when `2 A B < m`; sharp; not the full ball | proved here | `check_partial` |

## Review record

Date 2026-09-27. Reviewer: codex gpt-6-astra, `docs/reviews/m0-proofs/ideles-review.md` (checks
`docs/reviews/m0-proofs/ideles_review_checks.py`). Verdicts on this file: 13 VALID, 0 MINOR, 0 INVALID.

- (T): the pending Tychonoff source is replaced by `milne-cft:CFT.txt:9214` (read and quoted); the topology of `A`
  is cited from `milne-cft:CFT.txt:9373` instead of `tate-warwick:tatesthesis_notes.txt:386`, which the review found
  too narrow read literally.
- P6: the remark on the wording of `SPEC.md` 6 now says it counts the construction, not a minimum after merging.
- P9: `check_mixed_families` added; the review's surviving mutant (refinement over `N'/N_i` reduced to one class)
  now fails it.
- P12: the Check line states the actual depth (`p^4` below 50, `p^2` from 50 to 200) and method.
