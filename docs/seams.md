# Seams: the interface of version 1 tested against Q(sqrt(-5)) and F_3(T)

Date: 2026-09-27. Work package 0.6 of `PLAN.md`. Status: **draft for review; nothing here is implemented.**
Checks: `proto/seams_checks.py` (standard library and `python-flint`; about 15 seconds; prints one line per check).

Purpose. Version 1 implements the adeles of `Q` directly (`SPEC.md` section 3). Before the public header is frozen,
every public type and concept of `PLAN.md` sections 4 and 5 is tested on paper against two other instances of the
restricted product, so that nothing in the public interface silently assumes `Q`:

- **K** = `Q(sqrt(-5))`, class number 2, with the non-principal ideal `P = (2, 1 + w)`, `w = sqrt(-5)`;
- **F** = `F_3(T)` with its place at infinity.

The result is the table of section 4 (one row per type and per concept, each marked "unchanged", "generalises by
...", or "special to Q") and the recommendations of section 5.

Labels. **[proved here]**: a proof is given in this file. **[checked: Kn]** or **[checked: Fn]**: verified by exact
computation in small cases by the check of that name in `proto/seams_checks.py`. **[source pending: ...]**: a
standard fact used and not proved here; no copy of a source is on disk yet (`refs/` does not exist at the time of
writing). Nothing is cited from memory as if it were quoted.

Notation as in `SPEC.md`: `Zhat`, `v_p`, `a + N Zhat`. For K: `O = O_K`, `Ohat = O (x) Zhat`, `I Ohat` for a
fractional ideal `I`, `N(I)` the absolute norm. For F: `s = 1/T`, `O_inf = F_3[[s]]`, `Ohat_aff` the product of the
local rings at the affine places.

## 1. Test case K = Q(sqrt(-5))

### 1.1 The field, the ideal P, units, class number

**K.0.1 [proved here]** `O_K = Z[w]`, with Z-basis `(1, w)`. Let `x = r + s w` (`r, s` rational) be integral. Its
minimal polynomial `X^2 - 2 r X + (r^2 + 5 s^2)` has integer coefficients [source pending: an algebraic number is
integral exactly when its minimal polynomial over `Q` has integer coefficients]. So `R = 2r` is an integer and
`r^2 + 5 s^2` is an integer; then `20 s^2 = 4 (r^2 + 5 s^2) - R^2` is an integer, so the denominator of `s`
divides 2. With `S = 2s`: `R^2 + 5 S^2 = 0 mod 4`, hence `R^2 + S^2 = 0 mod 4`; squares are 0 or 1 modulo 4, so
`R` and `S` are even and `r, s` are integers. Conversely `w` is a root of `X^2 + 5`.

Lattices in coordinates. A Z-lattice of rank 2 in `K` is stored as `(1/d) (Z (a, 0) + Z (b, c))` in the basis
`(1, w)`, with `a, c > 0`, `0 <= b < a`, `d` least with the lattice times `d` inside `Z^2` (Hermite normal form,
HNF). The norm of a lattice is `N(I) = a c / d^2`; for integral `I` it is the index `[O : I]`, because the box
`0 <= y < c, 0 <= x < a` is a set of representatives of `O / I` (reduce `y` by the row `(b, c)`, then `x` by
`(a, 0)`; two points of the box that differ by an element of `I` are equal). **[proved here]**

**K.0.2 [proved here]** For `alpha = x + y w` not 0, `N(alpha O) = x^2 + 5 y^2 = N_{K/Q}(alpha)`: the lattice
`alpha O` has basis `alpha = (x, y)` and `alpha w = (-5y, x)`, of determinant `x^2 + 5 y^2`. **[checked: K2]**
(300 random `alpha`). An integral ideal `I` is principal exactly when some `alpha` in `I` has `N(alpha) = N(I)`:
then `alpha O` is inside `I` with the same index.

**K.0.3 [proved here]** `P = (2, 1 + w)` is the lattice `Z (2, 0) + Z (1, 1)`, `N(P) = 2`; `P^2 = (2)`; `P` is not
principal; `O^x = {1, -1}`. The products of the generators, `4`, `2 + 2w`, `(1 + w)^2 = -4 + 2w`, lie in `2O`, and
`2 = (2 + 2w) - (-4 + 2w) - 4` lies in `P^2`, so `P^2 = 2O`. A generator of `P` would have norm 2, and
`x^2 + 5 y^2 = 2` has no integer solution. A unit has norm 1 (norms of elements of `O` are non-negative integers
and multiplicative), and `x^2 + 5 y^2 = 1` only for `(x, y) = (1, 0), (-1, 0)`. **[checked: K1]** Also checked:
`(1 + w) = P Q3a` with `Q3a = (3, 1 + w)`; `(3) = Q3a Q3b`, `Q3b = (3, 1 - w)`; `(5) = (w)^2`;
`(7) = Q7a Q7b`, `Q7a = (7, 3 + w)`; `Q3a`, `Q3b`, `Q7a`, `Q7b` are not principal.

**K.0.4** The class number is 2. That it is at least 2 is K.0.3. That it is at most 2 uses the Minkowski bound
(every class contains an integral ideal of norm at most `(2/pi) sqrt(20) < 2.85`) [source pending: Minkowski bound
for an imaginary quadratic field]; the ideals of norm 1 and 2 are `O` and `P`. **[checked: K2]** Every one of the
83 integral ideals of norm at most 60 is principal (42) or becomes principal after multiplication by `P` (41).

The discriminant is `-20`: the trace form on the basis `(1, w)` has Gram matrix `[[2, 0], [0, -10]]`. **[proved
here]**

### 1.2 Finite balls with an ideal radius

The finite adeles of K are `K (x)_Q A_Q,f`, and `Ohat = O (x) Zhat` is the product of the completed local rings
[source pending: `K (x)_Q Q_p` is the product of the completions of K at the primes above `p`, and the same for
integers]. In the basis `(1, w)`, a finite adele of K is `x_1 + x_2 w` with `x_1, x_2` in `A_Q,f`, and
`Ohat = Zhat + Zhat w`. For a Z-lattice `L` in `K` with basis `e_1, e_2` put `Lhat = Zhat e_1 + Zhat e_2`; it does
not depend on the basis (a change of basis lies in `GL_2(Z)`), and it is the Zhat-span of any finite set of
Z-generators of `L`. For a fractional ideal `I`, `Ihat = I Ohat`.

**Definition.** A finite ball of K is `a + I Ohat` with `a` in `K` and `I` a fractional ideal, or `I = 0` (the
single point `a`). When `I` is not principal there is no element `N` of `K` with `I = N O`: the radius is an ideal,
not a scalar.

**Lemma K.1 [proved here]** `Lhat ∩ K = L`. An element `x` of `K ∩ Lhat` is `z_1 e_1 + z_2 e_2` with `z` in `Zhat^2`
and also `r_1 e_1 + r_2 e_2` with `r` in `Q^2`. Coordinates in the basis `e_1, e_2` of `K (x) A_Q,f = A_Q,f^2` are
unique, so `z = r` lies in `Q^2 ∩ Zhat^2 = Z^2` (`proofs/precision.md`, Lemma 1). Consequence: `Ihat` is inside
`Jhat` exactly when `I` is inside `J`.

**Lemma K.2 [proved here]** `Ihat + Jhat = (I + J)hat`; `a Jhat = (a J)hat` for `a` in `K`; `x y` lies in
`(I J)hat` for `x` in `Ihat`, `y` in `Jhat`. The first two: compare Zhat-spans of generators. The third: with
`x = sum z_i e_i`, `y = sum z'_j f_j`, `x y = sum z_i z'_j e_i f_j` and every `e_i f_j` lies in `I J`.

**Proposition K.3 (sum and product) [proved here]**

    (a + I Ohat) + (b + J Ohat)  =  (a + b) + (I + J) Ohat
    (a + I Ohat) * (b + J Ohat)  is contained in  a b + (a J + b I + I J) Ohat

and no ball `c + R Ohat` with `R` a lattice (or 0) and `R Ohat` properly inside `G Ohat`, `G = a J + b I + I J`,
contains all products. This is `SPEC.md` 4.3 with the gcd of rationals replaced by the sum of ideals.

*Proof.* The sum is Lemma K.2. Enclosure: `(a + x)(b + y) - a b = a y + b x + x y`, and each term lies in
`G Ohat` by Lemma K.2. Tightness: suppose `c + R Ohat` contains all products. The product for `x = y = 0` gives
`c + R Ohat = a b + R Ohat`. Take `x` in `I` and `y` in `J` (elements of `K`): `a y + b x + x y` lies in
`R Ohat ∩ K = R` (Lemma K.1). With `x = 0`: `a J` is inside `R`; with `y = 0`: `b I` is inside `R`; hence `x y` lies
in `R` for all `x, y`, and `I J`, which is spanned by such products, is inside `R`. So `G` is inside `R`. If
`G = 0` every product is `a b`. `G` is an O-module, so the result is again a ball with an ideal radius.

**[checked: K3]** 400 random cases with fractional centres and ideals (including radius 0): every sampled result
lies in the predicted ball, and the lattice spanned by the sampled differences equals the predicted radius. The
wrong rule without the term `I J` fails in 118 of the 400 cases.

Examples. **[checked: K3]**

- `2 Ohat + (1 + w) Ohat = P Ohat`: the sum of two balls with principal radii has a radius that is not principal.
  So a scalar radius is not closed under addition. This is the concrete form of review finding D4.
- `(0 + P Ohat)(0 + P Ohat) = 0 + 2 Ohat`, tight (`G = P^2 = (2)`).
- For rational data the rule reduces to that of `Q`: `(3 + 12 Ohat)(5 + 18 Ohat)` has radius `6 O`, since
  `n O + m O = gcd(n, m) O` for integers.

### 1.3 Canonical form

**Proposition K.4 [proved here]** Let `d` be the least positive integer with `d a` in `O` and `d I` inside `O`; let
`d I` have HNF `(A, 0), (B, C)`; let `(x, y)` be the coordinates of `d a` reduced into the box `0 <= y < C`,
`0 <= x < A`. The tuple `(d; x, y; A, B, C)` is determined by the set `a + I Ohat` and determines it. Minimality of
`d` is equivalent to `gcd(x, y, A, B, C, d) = 1`.

*Proof.* The set determines `I` (the differences of its elements that lie in `K` form `I`, Lemma K.1) and `a` modulo
`I`. The integers `m` with `m a` in `O` and `m I` inside `O` form an ideal of `Z`, and the condition does not depend
on the representative of `a` modulo `I` once `m I` is inside `O`; so `d` is determined. The HNF of `d I` is unique
and the box is a set of representatives of `O / d I`. For the last claim: `(d/p) a` in `O` and `(d/p) I` inside
`O` hold exactly when the prime `p` divides `x, y` (the reduced centre differs from `d a` by an element of `d I`,
which is then divisible by `p`) and `A, B, C`.

This is `SPEC.md` 4.1 (D2) with `H` replaced by a 2x2 HNF: `(A, H, d)` with `gcd(A, H, d) = 1` is the case of
degree 1. It needs gcds only, no factorisation. **[checked: K4]** 300 random balls rebuilt from other
representatives give the identical tuple, and the gcd condition holds.

### 1.4 Set predicates

**[proved here]** For balls with non-zero radii `I`, `J`:

| Predicate | Condition in K | Condition in Q (`SPEC.md` 4.2) |
|---|---|---|
| `equal_set` | `I = J` and `a - b` in `I` | `N = M` and `(a - b)/N` an integer |
| `overlaps` | `a - b` in `I + J` | `(a - b)/gcd(N, M)` an integer |
| `contains` (first in second) | `I` inside `J` and `a - b` in `J` | `N/M` and `(a - b)/M` integers |

The proof is that of `proofs/precision.md` Proposition 3 with Lemmas K.1 and K.2. "`N/M` is an integer" becomes
"`I` is inside `J`", that is, `J` divides `I`: radii are ordered by divisibility, as already for `Q`.
**[checked: K5]** 120 pairs of integral balls compared with the finite sets of their classes modulo `m O` (with
`m O` inside both radii, so the comparison is exact): all 120 agree; the three relations hold in 17, 108 and 45 of
them.

### 1.5 The archimedean part and the exact global type

`K (x)_Q R = C`: K has one complex place, which is a pair of conjugate embeddings. One of them must be fixed, say
`sigma(w) = i sqrt(5)`. The adele is (a complex ball; a finite ball). For a general number field the archimedean
part is one real ball per real place and one complex ball per complex place (with a fixed embedding for each), not
one ball per embedding. **[proved here]** (A complex place is a conjugate pair; two balls for one place would be
redundant data that could disagree.)

The shape (`acb` ball; finite ball) coincides with `adf_cadele` of `Q`, but the meaning is different: here it is
the adele ring of K, there it is `C x A_Q,f`, which is not the adele ring of any field (`SPEC.md` 4.1). A value
must therefore carry its field; the shape of the struct does not identify it.

The exact global type is an element of K (for example FLINT's `nf_elem`, `SPEC.md` 13). Its conversion to an adele
needs the fixed embedding `sigma` and a requested precision, as for `adf_rat`.

### 1.6 Unit cosets

For a non-zero integral ideal `n` put `U(n) = {u in Ohat^x : u - 1 in n Ohat}`. A coset is `c U(n)` with `c` in `O`
and `c O + n = O`, stored with `c` reduced into the HNF box of `n`. `Ohat / n Ohat = O / n` **[proved here]**
(`O + n Ohat = Ohat` because `m Ohat` is inside `n Ohat` for an integer `m` in `n`, and `Zhat = Z + m Zhat`;
`O ∩ n Ohat = n` by Lemma K.1). That `Ohat^x / U(n)` is `(O/n)^x` needs the local description of `Ohat^x`
[source pending: `Ohat` is the product of the local rings `O_P`].

- Product at different moduli: `c U(n) * c' U(n')` is inside `c c' U(n + n')` **[proved here]** (`u - 1` in
  `n Ohat` implies `u - 1` in `(n + n') Ohat`). This is `SPEC.md` 5 with `gcd(N, N')` replaced by `n + n'`.
- Canonical form. For `Q`, `U(2N) = U(N)` for odd `N` (`SPEC.md` 5). In K: **[proved here]** if `N(Q) = 2` and `Q`
  does not divide `n`, then `U(Q n) = U(n)`: by the Chinese remainder theorem for coprime ideals
  `(O / Q n)^x = (O / n)^x x (O / Q)^x` [source pending: CRT for coprime ideals], and `(O/Q)^x = F_2^x` is
  trivial. For `Q^2`: `(O / Q^2)^x` has 2 elements, so `U(Q^2 n)` is strictly smaller than `U(Q n)`.
  **[checked: K6]** `phi(P) = 1`, `phi(P^2) = 2`, `phi(3P) = phi(3) = 4`, `phi(6) = 8`, `phi(Q7a P) = phi(Q7a) = 6`,
  `phi(w P) = phi(w) = 4`, where `phi(n) = |(O/n)^x|` is counted by enumeration.
  Consequence: the rule of `Q` read literally ("a modulus that is exactly twice an odd number is halved") is wrong
  in K: `6 O = P^2 Q3a Q3b` is "twice an odd modulus", but `U(6) != U(3)`. The rule that generalises is: **remove
  every prime with residue field `F_2` that divides the modulus exactly once.** For `Q` this is the prime 2. In K
  it is `P`, and it needs the factorisation of the modulus only at the primes above 2.

### 1.7 Ideles: the decomposition r u fails, and what replaces it

For `Q`, every finite idele is uniquely `r u` with `r` a positive rational and `u` in `Zhat^x` (`SPEC.md` 5).

Facts used [source pending: the finite ideles of K are the restricted product of the `K_P^x` with respect to the
`O_P^x`; `v_P : K_P^x -> Z` is onto with kernel `O_P^x`]. Then the **content** map
`x -> prod P^(v_P(x_P))` from finite ideles onto the group `I_K` of fractional ideals has kernel `Ohat^x`. For `Q`,
`I_Q` is the group of positive rationals (each fractional ideal of `Z` has one positive generator), which gives
`x = r u`.

In K the decomposition fails for two separate reasons. **[proved here]**

1. **The content need not be principal.** Let `pi_P` be the idele with `1 + w` at `P` and 1 at every other prime;
   `v_P(1 + w) = 1` because `(1 + w) = P Q3a` with `Q3a != P` (K.0.3). Its content is `P`. If `pi_P = alpha u` with
   `alpha` in `K^x` and `u` in `Ohat^x`, its content would be `alpha O = P`, which is not principal.
2. **A generator is not canonical.** When the content is principal, `alpha O = alpha' O` gives `alpha' = e alpha`
   with `e` in `O^x = {1, -1}`, and K has no ordering (in an ordered field squares are `>= 0`, but `w^2 = -5 < 0`),
   so "the positive generator" has no meaning. In other fields `O^x` is infinite: in `Q(sqrt(2))`, `e = 1 + sqrt(2)`
   has norm `-1` and `|e| > 1`, so its powers are distinct units.

**What replaces it.** There are two exact sequences

    1 -> Ohat^x -> (finite ideles of K) -> I_K -> 1,        1 -> O^x -> K^x -> (principal ideals) -> 1,

and `Cl_K = I_K / (principal ideals)`. Fix once per field (a) integral ideals `c_1, ..., c_h` representing the
classes (here `O` and `P`) with ideles `t_j` of content `c_j` (here `1` and `pi_P`), and (b) a rule that picks one
element in each orbit of `O^x`. Then **every finite idele is `x = alpha t_j u`**, with `j` the class of its content,
`alpha` in `K^x` unique up to `O^x`, and `u` in `Ohat^x` (changed by the inverse unit). *Proof:* the content of
`x / t_j` is principal, `= alpha O`; then `x / (alpha t_j)` has content `O`, so it lies in `Ohat^x`. If
`alpha t_j u = alpha' t_k u'`, the classes agree, so `j = k`, and `alpha O = alpha' O`, so `alpha' = e alpha`,
`u' = e^(-1) u`. The choice (b) is a convention and is not multiplicative (for the rule "first non-zero coordinate
in the basis `(1, w)` positive", `w` is normalised and `w * w = -5` is not).

Precision. The content, the class index and `alpha` are exact; `u` is known as a coset `c U(n)` (section 1.6). The
"scale" of `SPEC.md` 5 is the content, a fractional ideal; the norm needs only the content:
`|x| = |x_inf|_C * N(content)^(-1)` with `|z|_C = |z|^2` (multiplication by `z` on `C = R^2` has determinant
`|z|^2`) [source pending: normalisation of the absolute value at a complex place]. For `alpha` in `K^x`,
`|sigma(alpha)|^2 = x^2 + 5 y^2 = N(alpha O)` (K.0.2), so the product formula holds. **[proved here]**

**Idele classes.** **[proved here]** Let `C_K` be the ideles modulo `K^x`. The map to `Cl_K` (content, then class)
is onto; its kernel is the set of ideles with principal content, which is `K^x (C^x x Ohat^x)`; and
`K^x ∩ (C^x x Ohat^x) = O^x` (such an element has content `O`, so it and its inverse lie in `O`). So

    1 -> (C^x x Ohat^x) / O^x -> C_K -> Cl_K -> 1,        here O^x = {1, -1} acting diagonally, Cl_K of order 2.

For `Q` the kernel is `(R^x x Zhat^x) / {1, -1}`, and the sign of the real coordinate picks one element of each
orbit, which gives `R_{>0} x Zhat^x` and the coordinates `t = |x_inf| / r`, `u' = sign(x_inf) u` of `SPEC.md` 5. At
a complex place there is no sign, and the class group enters. The coordinates `(t, u')` are special to `Q`.

### 1.8 The additive character, the trace and the different

The standard character of K is `psi_K = psi_Q o Tr_{K/Q}` [source pending: Tate's thesis, section on the global
character; to be quoted in work package 0.2]. In coordinates `Tr(x_1 + x_2 w) = 2 x_1`, extended A_Q-linearly.

**Where the phase of a ball is determined. [proved here]** `Tr(I Ohat) = (Tr I) Zhat` (the trace is Q-linear and
`Tr I = t Z` for a rational `t >= 0`). By `SPEC.md` 6 (M7) the finite part of `psi_Q` is constant on `t Zhat`
exactly when `t` is an integer, and otherwise takes all `B`-th roots of unity, `t = A/B`. Hence the phase of `psi_K`
on `a + I Ohat` is determined exactly when `Tr(I)` is inside `Z`, that is, when `I` is inside the inverse different
`D^(-1) = {y : Tr(y O) inside Z}`; otherwise the values are `psi_K(a)` times the `B`-th roots of unity with
`Tr(I) = (A/B) Z`.

**[checked: K7]** `D^(-1) = (2w)^(-1) O` (the trace dual of `O`), `D = (2w)`, `N(D) = 20 = |d_K|`. Example: the ball
`0 + (1/2) Ohat` has a radius that is not integral, and yet `Tr((1/2) O) = Z`, so its phase is determined (it is 1).
The rule of `Q` ("integer radius: determined; fractional radius `A/B`: `B`-th roots of unity") read literally with
"integral ideal" in place of "integer" would claim the image `{1, -1}` here, which is false as an image (though a
valid enclosure).

**Annihilators. [proved here]** Let `e_1, e_2` be a basis of `I` and `f_1, f_2` its trace-dual basis
(`Tr(e_i f_j) = 1` if `i = j`, else 0). Write `y = y_1 f_1 + y_2 f_2`; then `Tr(e_i y) = y_i`, so `psi_K(x y) = 1`
for all `x` in `I Ohat` exactly when each `y_i` lies in the annihilator of `Zhat` for `psi_Q,f`, which is `Zhat`
[source pending: the case of `Q`, used in `SPEC.md` 7]. So the annihilator of `I Ohat` is `I^dual Ohat`, and
`I^dual = I^(-1) D^(-1)` **[checked: K7]** (45 ideals) [source pending: the identity in general].

Consequences for the Fourier transform of `adf_ffun` (`SPEC.md` 7). A function supported on `d^(-1) Ohat` and
constant modulo `m Ohat` has a transform supported on `(m^(-1) D^(-1)) Ohat` and constant modulo `(d D^(-1)) Ohat`
(the annihilators of the invariance group and of the support) **[proved here]**: translating `x` by an element `b`
of the invariance group multiplies `hat f(y)` by `conj(psi(b y))`, so `hat f(y) = 0` unless `y` annihilates that
group; and for `y'` in the annihilator of the support, `psi(x y') = 1` on the support, so
`hat f(y + y') = hat f(y)`. For `Q`, `D = 1` gives `SPEC.md` 7. The array is indexed by the finite abelian group
`d^(-1) / m`, of order `N(d) N(m)`, which need not be cyclic: `O / 2O` has order 4 and exponent 2 (`2x` lies in `2O`
for every `x`). So the one-dimensional index `j/D, 0 <= j < DM` and the one-dimensional DFT of `SPEC.md` 7 become an
index over a product of cyclic groups (Smith normal form of the lattice pair) and a multi-dimensional DFT. The
measure with `vol(Ohat) = 1` is not self-dual for `psi_K`; the self-dual one gives `Ohat` the volume `N(D)^(-1/2)`
[source pending: Tate's thesis], so the constants of `SPEC.md` 7 change.

### 1.9 Haar volume, the quotient by K, splitting

**Volume. [proved here]** With `vol(Ohat) = 1`, `vol(a + I Ohat) = 1 / N(I)`. For integral `I`, `Ohat / I Ohat =
O / I` (as in 1.6) has `N(I)` elements. For `I = (1/d) L`: multiplication by `d` multiplies the Haar measure of
`A_Q,f^2` by `d^(-2)` (on `A_Q,f`, `N Zhat` has volume `1/N`, `SPEC.md` 7), so `vol(I Ohat) = d^2 / det(L)`. The
volume is an exact rational read off the HNF; no factorisation. The discriminant enters only through the self-dual
normalisation (1.8).

**Quotient. [proved here]** `A_K,f = K + Ohat`, coordinatewise from `A_Q,f = Q + Zhat` (`SPEC.md` 6, M6), and
`K ∩ Ohat = O` (Lemma K.1). Hence `A_K / K = (C x Ohat) / O` with `O` embedded diagonally, and a fundamental domain
is `Pi x Ohat`, `Pi = {u + v i sqrt(5) : 0 <= u, v < 1}`, a parallelogram of area `sqrt(5)` in `dx dy` (the lattice
`sigma(O) = Z + Z i sqrt(5)`). The gluing rule `(1 ; z) ~ (0 ; z - 1)` of `SPEC.md` 6 becomes two rules, one per
basis vector: `(zeta + 1 ; z) ~ (zeta ; z - 1)` and `(zeta + i sqrt(5) ; z) ~ (zeta ; z - w)`, and a complex ball
splits into one piece per translate of `Pi` that it meets. With the measure `2 dx dy` at a complex place the volume
is `2 sqrt(5) = sqrt(|d_K|)` [source pending: the convention `2 dx dy`].

**Splitting a fractional radius. [proved here]** A ball `a + I Ohat` with `I` not inside `O` is the union of
`[I : I ∩ O] = [I + O : O]` balls of radius `I ∩ O` (second isomorphism theorem). For `Q` this is `SPEC.md` 6: `B`
pieces for `N = A/B`. **[checked: K8]** `I = P/2`: `I ∩ O = O`, 2 pieces.

### 1.10 Rational reconstruction

**[proved here]** `(a + I Ohat) ∩ K = a + I` (Lemma K.1). The candidates are the points of the lattice `a + I`
whose image under `sigma` lies in the complex ball. There is at most one when the ball has diameter less than
`lambda_1(sigma(I)) = min |sigma(x)|` over non-zero `x` in `I`, which is the square root of the least norm of a
non-zero element. **[checked: K9]** `lambda_1(sigma(P)) = 2` (least norm 4, attained by `2`). Listing the
candidates is the enumeration of the points of a two-dimensional lattice in a disc, which in general needs a reduced
basis [source pending: Lagrange-Gauss reduction]; for a field of degree `n` it is a lattice problem in `R^n` and
needs every archimedean component. For `Q` the lattice is one-dimensional and no reduction is needed
(`SPEC.md` 9.2). The partial problem (`SPEC.md` 9.2, second item) becomes a lattice problem as well; its
uniqueness bound is not derived here.

### 1.11 Precision policies when the radius is an ideal

- **Tight**: Proposition K.3.
- **Absolute cap**: `R -> R + C` (sum of ideals), coarser or equal. **[proved here]** (`R` is inside `R + C`.)
- **Scaled residue.** A context holds a modulus. The cheap choice keeps a rational integer `k` (the `K` of
  `SPEC.md` 4.4): then `O / k O` is `(Z/k)^2` with the product
  `(x_1 + x_2 w)(y_1 + y_2 w) = (x_1 y_1 - 5 x_2 y_2) + (x_1 y_2 + x_2 y_1) w` modulo `k`, so word kernels survive
  with residue vectors of length `[K : Q] = 2`. A value is `s (u + k Ohat)` with a scale `s` in `K^x` and `u` in
  `O / k O`.
  - Product: `s t (u v + k Ohat)` encloses the tight result. **[proved here]** (As `proofs/precision.md`
    Proposition 5(2): the tight radius `s t k (u O + v O + k O)` is inside `s t k O`.)
  - Sum: the tight radius is `(s O + t O) k`. A result of the form `g (r + k Ohat)` needs `g O` to contain
    `s O + t O` (divide the radii by the integer `k`); when `s O + t O` is not principal, no `g` gives the tight
    radius. **Example [checked: K10]:** `2 (u + 3 Ohat) + (1 + w)(v + 3 Ohat)` has tight radius `3P`, not principal.
    The minimal principal ideals that contain `3P` are `3 P Q^(-1)` for the non-principal primes `Q`; for
    `Q = P, Q3a, Q3b, Q7a, Q7b` they are generated by `3`, `1 - w`, `-1 - w`, `(9 - 3w)/7`, `(-9 - 3w)/7`, and they
    are pairwise incomparable. So the scaled sum loses precision, and there is no canonical best result: the rule
    `g = gcd(s, t)` of `SPEC.md` 4.4 has no analogue. *Proof of minimality:* `g O` contains `3P` exactly when
    `c = 3P (g O)^(-1)` is integral; `c` is then in the class of `P` (class number 2); a non-principal integral `c`
    has a non-principal prime factor `Q` [source pending: unique factorisation of ideals], and `3P c^(-1)` contains
    `3P Q^(-1)`. Each `3P Q^(-1)` is minimal: for a principal `g' O` between `3P` and `3P Q^(-1)`,
    `c' = 3P (g' O)^(-1)` is integral and contains `Q`, so `c' = O` (then `g' O = 3P`, not principal) or `c' = Q`.
  - What a "scaled residue context" is when the radius is an ideal: the context holds an integral ideal (preferably
    `k O`); the scale is an element; the scaled policy remains an enclosure, but its sum is tight only when the
    ideal gcd of the scales is principal, which is always the case for `Q` and never guaranteed for K.

### 1.12 Places, local and partial balls, characters, real test functions

- **Places** are prime ideals and the complex place. An integer prime does not name a place: `3` has the two places
  `Q3a`, `Q3b`. The completion at a split prime is `Q_p` with a chosen root of `X^2 + 5` (at `Q3a`, `w = -1`
  modulo `Q3a`); at `P` and at `(w)` it is a ramified quadratic extension of `Q_2`, `Q_5`. FLINT's `qadic` covers
  unramified extensions only (`SPEC.md` 13). So `adf_lball` generalises to (place, element of the completion), with
  a representation that depends on the kind of the place.
- **Partial balls** (`adf_sball`): a finite set of places; the archimedean tag real or complex of version 1 already
  fits a number field with `r_1` real and `r_2` complex places.
- **Characters** (`adf_char`): the conductor is an integral ideal; the finite part is a character of a ray class
  group, which maps onto `Cl_K`, so it is not determined by a character of `(O/f)^x` alone; the parity becomes an
  infinity type at the complex place [source pending: Hecke characters, their infinity types and compatibility with
  units]. The value on a coset `c U(n)` is determined when `f` divides `n` (`n` inside `f`), as in `SPEC.md` 5 with
  ideal divisibility.
- **Real test functions** (`adf_rfun`): at a complex place, functions of two real variables (polynomial in `z` and
  `conj(z)` times a Gaussian) [source pending: Tate's standard function at a complex place]. The class of
  `SPEC.md` 7 generalises to several variables.
- **Functions of one variable** (`SPEC.md` 9.3) at a prime of K need the completion `K_P`, which is ramified at `P`
  and at `(w)`; this is Tier B (`SPEC.md` 9.3.7) and out of scope for version 1 (`PLAN.md` 10).

## 2. Test case F = F_3(T) with the place at infinity

### 2.1 Places and valuations

The places are the monic irreducible polynomials `P` (the affine places, residue field of `3^(deg P)` elements) and
the place at infinity, with `v_inf(num/den) = deg den - deg num`, uniformiser `s = 1/T`, completion `F_3((s))`,
residue field `F_3` [source pending: these are all the places of `F_q(T)`]. For `f` in `F^x`, the divisor has
degree 0: `sum_P deg P * v_P(f) = deg num - deg den` (unique factorisation into monic irreducibles) and
`v_inf(f) = deg den - deg num`. **[proved here]**

### 2.2 Balls: the affine part, the part at infinity, and why the two stay separate

**Affine part. [proved here]** `F_3[T]` is a principal ideal domain with units `F_3^x`. Every statement of
`proofs/precision.md` holds verbatim with `Z` replaced by `F_3[T]`, "prime" by "monic irreducible", "positive" by
"monic", and `0 <= A < H` by `deg A < deg H`: a product of local balls over finitely many affine places is one
`a + N Ohat_aff` with `a` in `F` and `N` a monic rational function; the sum and product rules hold with the monic
gcd; the canonical form is `(A + H Ohat_aff)/d` with `d` monic, `H` monic or 0, `deg A < deg H`,
`gcd(A, H, d) = 1`. The proofs use only the Chinese remainder theorem, Bezout, and `R Ohat_aff ∩ F = R F_3[T]`.
**[checked: F2]** product rule, 300 random cases with rational-function centres and radii (including radius 0):
enclosure and tightness; the rule without the term `N M` fails in 142 cases.

**Infinity. [proved here]** A ball at infinity is `c + s^n O_inf`, `c` a Laurent polynomial in `s`. Sum:
`(c + c') + s^(min(n, m)) O_inf`. Product of `a + s^n O_inf` and `b + s^m O_inf`: centre `a b`, radius
`s^min(v(a) + m, v(b) + n, n + m)`; the proof is that of
`proofs/precision.md` Proposition 2 with valuations (the three terms `a M v`, `b N u`, `N M u v` and the four
witnesses). **[checked: F3]** 300 random cases.

**Why the place at infinity is stored separately (D4). [proved here]**

- Multiplication by the exact `T` gains precision at the place `T` and loses one digit at infinity
  (`v_T(T) = 1`, `v_inf(T) = -1`): `T * (0 + Ohat_aff ; c + s^n O_inf) = (0 + T Ohat_aff ; T c + s^(n-1) O_inf)`.
  **[checked: F1]** A single polynomial radius cannot record both.
- A ball around a given adele need not contain an element of `F`, so a single global centre cannot serve all
  places. The neighbourhood of the adele (`s` at infinity, 0 at every affine place) given by `v_inf(x - s) >= 2`
  and `x_P` in `O_P` for all `P` contains no element of `F`: such an element `x` would be a polynomial, whose
  expansion in `s` has no term `s^1`, so `v_inf(x - s) = 1`. So the centre at infinity must be stored apart from
  the affine centre, as the real ball of `Q` is stored apart from the finite ball.

The **radius** of an adele ball of F is therefore a divisor: the fractional ideal `N F_3[T]` (a monic rational
function, not only a polynomial; see Finding 1) for the affine places, and an exponent `n` at infinity.

**No order, no archimedean ball. [proved here]** `F_3((s))` has no ordering: `-1 = 1^2 + 1^2` in characteristic 3,
while in an ordered field a sum of squares is not negative. So there is no sign, no floor, no positive scale;
"monic" (leading coefficient 1) takes the place of "positive". The absolute value `|x|_inf = 3^(-v_inf(x))` has
discrete image.

**Real precision as an argument.** In version 1 the real working precision is an argument of each operation, as in
`arb` (`PLAN.md` 4). At the place at infinity of F arithmetic on truncated Laurent series is exact: the radius of
the result is fixed by the radii of the inputs (the rules above), as at a prime; there is no rounding and no working
precision. What remains of the argument is at most an optional cap on the exponent `n` (the absolute cap policy at
infinity). The precision is carried by each value, as in FLINT's `padic` (`SPEC.md` 4.4).

**Exact type.** A rational function `num/den` over `F_3` with `den` monic and `gcd(num, den) = 1`. (Whether FLINT
3.0.1 offers such a type over `F_q` is not checked here.)

### 2.3 The quotient A_F / F: a direct sum, no gluing

**Proposition F.4 [proved here]** `A_F = F (+) (Ohat_aff x s O_inf)`, a direct sum of groups; so `A_F / F` is
isomorphic to the compact open subgroup `Ohat_aff x s F_3[[s]]`, of volume `1/3` when every `O_v` has volume 1.

*Proof.* (a) Affine part: for each of the finitely many `P` with `x_P` not in `O_P`, the principal part of `x_P` is
an element `y_P` of `F` whose denominator is a power of `P` [source pending: the `P`-adic expansion in the
completion at `P`]; `y_P` is integral at every other affine place, so `x - sum y_P` has affine part in `Ohat_aff`.
(b) `F ∩ Ohat_aff = F_3[T]`. (c) `F_3((s)) = F_3[T] (+) s F_3[[s]]` (terms `s^k` with `k <= 0` form a polynomial in
`T = 1/s`). Given `x`, subtract `c = sum y_P`, then the polynomial part `p` of `x_inf - c`; the result lies in
`Ohat_aff x s O_inf` because `p` is integral at every affine place. Uniqueness: an element of `F` in that group is a
polynomial with `v_inf >= 1`, hence 0. **[checked: F4]** (250 polynomials.)

Compared with `Q` (`SPEC.md` 6): the fundamental domain `[0,1) x Zhat` has a boundary and the gluing rule
`(1 ; z) ~ (0 ; z - 1)`; here the domain is a subgroup, and there is no boundary and no gluing rule. Splitting
remains: a ball `c + s^n O_inf` with `n <= 0` meets `3^(1-n)` polynomial parts and gives that many pieces
**[checked: F4]**; a fractional affine radius splits as in `SPEC.md` 6. The status `NEEDS_SPLIT` and the piece limit
remain; the closed-interval pieces do not.

### 2.4 The additive character: residues of a differential

**[source pending: the character `psi_omega(x) = psi_0(sum over all places v of Res_v(x_v omega))` for a non-zero
differential `omega` and a non-trivial `psi_0 : F_q -> C^x`; the residue at a place of degree `d` includes the trace
from `F_(q^d)` to `F_q`; the residue theorem `sum_v Res_v(f omega) = 0` for `f` in `F`, which makes
`psi_omega` trivial on `F`.]** For `q = 3` take `psi_0(a) = exp(2 pi i a / 3)`.

- **A choice of differential is needed.** There is no canonical `omega`; replacing `omega` by `g omega` (`g` in
  `F^x`) gives `x -> psi_omega(g x)`. With `omega = dT`: `dT = -s^(-2) ds`, so `v_inf(dT) = -2` and
  `Res_inf(f dT) = -c_1(f)`, where `c_1` is the coefficient of `s^1` in the expansion of `f` **[proved here]**; at
  the affine places `v_P(dT) = 0`.
- **[checked: F5]** The residue theorem for `f dT`: 300 random `f` over `F_3` with poles of any order at the places
  `T, T - 1, T - 2` and simple poles at the places `T^2 + 1`, `T^2 + T + 2` of degree 2 (154 of them with a non-zero
  residue there), and at infinity: the sum is 0 in every case; with the sign of `Res_inf` reversed, 72 cases fail.
- **Where the phase is determined.** At infinity `psi(x) = psi_0(-x_1)` is constant on `c + s^n O_inf` exactly when
  `n >= 2 = -v_inf(dT)`; for `n <= 1` it takes all three values. **[checked: F6]** So the divisor of `omega` plays
  the part of the different: the rule "integral radius: determined" becomes "radius divisor at least `-(omega)`".
- **Values.** `psi` takes values in the `p`-th roots of unity (`p` the characteristic), since `psi_0` does. The
  image of a ball is `psi(a)` times a subgroup of the `p`-th roots of unity, hence one value or `p` values
  **[proved here]**; for `Q` it is `psi(a)` times all `B`-th roots of unity for any `B`.

### 2.5 Ideles, the norm, the degree map, the class group

**The decomposition r u survives, with "monic" for "positive". [proved here]** The content of the affine part of an
idele is a fractional ideal of the principal ideal domain `F_3[T]`, with one monic generator `r`; so the affine
part is uniquely `r u`, `u` in `Ohat_aff^x`. The idele is `(x_inf ; r ; u)` with `x_inf` in `F_3((s))^x` (a ball at
infinity that does not contain 0, so that its valuation is determined), `r` a monic rational function, and `u` a
unit coset `c U(N)`, `N` monic.

**Norm.** `|x| = 3^(-v_inf(x_inf) - deg r)` with `deg r = deg num - deg den` **[proved here]**: the image is
`3^Z`, discrete. The product formula for `f` in `F^x` is `deg div f = 0` (2.1). A quasi-character `|x|^s` depends
on `s` only modulo `2 pi i / log 3`.

**Degree map and class group. [proved here]** The degree `x -> v_inf(x_inf) + deg r` maps the idele class group
onto `Z` (the idele with `s` at infinity and 1 elsewhere has degree 1, since infinity has degree 1). Every divisor
of degree 0 is principal: for `D = sum n_P P + n_inf inf` with `n_inf = -sum n_P deg P`, the function
`f = prod P^(n_P)` has `div f = D`. So `Pic^0` of the projective line is trivial **[checked: F8]** (200 divisors).

**Idele classes. [proved here]** Let `sgn(y)` be the leading coefficient of the expansion of `y` in `s` (an element
of `F_3^x`). Dividing `x` by the global element `r sgn(x_inf / r)` gives the class coordinates

    t  = x_inf / (r sgn(x_inf / r))      (leading coefficient 1),        u' = sgn(x_inf / r)^(-1) u,

so the idele class group is `s^Z x (1 + s F_3[[s]]) x Ohat_aff^x`. This is the exact analogue of `SPEC.md` 5,
`t = |x_inf| / r`, `u' = sign(x_inf) u`, with the sign replaced by the leading coefficient (the constants `F_3^x`
are the global units, as `{1, -1}` are for `Q`). **[checked: F9]** (200 ideles and multipliers from `F^x`: the
coordinates do not change.) Invariance: for `f = lambda m` (`lambda` in `F_3^x`, `m` monic), `f x` has `r m`,
`lambda u`, `f x_inf`, and `sgn(f x_inf / (r m)) = lambda sgn(x_inf / r)`, so `t` and `u'` do not change.
Injectivity: two ideles with the same `(t, u')` differ by a global element `r sgn`. Surjectivity: `(t ; 1 ; u')` has
coordinates `(t, u')`.

This works because `F_3[T]` is a principal ideal domain with a canonical generator of each ideal (monic). For a
curve of higher genus, or a function field with several places at infinity, the class group and the units enter
as for K (review D4).

**Unit cosets. [checked: F10]** For `q = 3` every residue field has at least 2 units, so `U(P N) != U(N)` always:
no canonical-form collapse occurs (`phi(T) = 2`, `phi(T^2) = 6`, `phi(T (T+1)) = 4`). For `q = 2` the places `T` and
`T + 1` have residue field `F_2`, and the rule of 1.6 applies.

### 2.6 Rational reconstruction

**[proved here]** `(a + N Ohat_aff) ∩ F = a + N F_3[T]`. With a ball `c + s^n O_inf` at infinity, the solutions are
`a + N g` with `v_inf(a + N g - c) >= n`. Two solutions differ by `N h` with `v_inf(N h) >= n`, that is
`deg h <= -n - deg N`. So the set is empty or a coset of the polynomials of degree at most `-n - deg N`: **at most
one solution when `n + deg N >= 1`**, otherwise 0 or `3^(1 - n - deg N)`. It is linear algebra over `F_3` (solve
for the coefficients of `g`): no lattice reduction, no rounding. This is the analogue of "an interval shorter than
`N` contains at most one rational" (`SPEC.md` 9.2). **[checked: F7]** 60 cases, 40 non-empty, 24 with several
solutions; every count is as predicted.

### 2.7 Policies, backends, functions

- Tight: sections 2.2. Absolute cap: monic gcd at the affine places and a cap exponent at infinity.
- Scaled residue: `proofs/precision.md` Proposition 5 holds verbatim for monic scales in `F_3[T]` (it uses only
  Bezout and `gcd(A, B) = 1`) **[proved here]**; the context holds a monic polynomial. It applies to the affine
  part; the part at infinity carries its own exponent.
- Backends: global `(A, H, d)` as polynomials; local: residues modulo pairwise coprime polynomial blocks, where
  "fits a machine word" becomes a bound on the degree of a block.
- Test functions at infinity are locally constant with compact support, of the same kind as the finite part; there
  is no Gaussian, so `adf_rfun` has no counterpart.
- The functions of `SPEC.md` 9.3 (`exp`, `sin`, `cos`, ...) do not exist in characteristic 3: the coefficient
  `1/3!` of the exponential series has no meaning in `F_3`. **[proved here]**

## 3. What carries over in general

In both test cases the following held without change of meaning: the exact tag of `SPEC.md` 4.1 (M3); enclosure as
the contract; radii ordered by inclusion, not size; the three set predicates with "divides" in place of "is an
integer"; division only by exact non-zero elements or ideles (`SPEC.md` 4.5: at a place where neither the centre nor
the radius of a ball of positive radius has a denominator or a factor, its coordinate ranges over the whole local
ring, which contains 0); `f_at` with named places; the status codes. What changed is concentrated in five places:
the radius (a scalar becomes an ideal or a divisor), the place (an integer becomes a handle), the archimedean part
(one real ball becomes a family indexed by the infinite places, of kind real, complex, or non-archimedean), the
idele scale (a positive rational becomes a content ideal, plus a class and a unit normalisation when the class group
or the units are not trivial), and the additive character (the trace and the different, or a chosen differential).

## 4. The table

Verdict rule: **unchanged** if the version-1 contract reads correctly in both test cases; **generalises by ...** if
it holds in both after the stated replacement; **special to Q** if in at least one test case there is no object of
the same kind (the general object is named). The section with the details is given in brackets.

### 4.1 Public types

| # | Type | Verdict, and by what or the general object (section) |
|---|---|---|
| T1 | `adf_rat` | generalises by: an exact element of the field (K: `nf_elem`; F: reduced `num/den`) (1.5, 2.2) |
| T2 | `adf_fball` | generalises by: radius a fractional ideal, HNF over an integer `d` (K); monic ratio (F) (1.3) |
| T3 | `adf_scaled` | generalises by: scale in the field, residues mod an integer as vectors; K: not tight (1.11) |
| T4 | `adf_lball` | generalises by: (place handle, element of the completion; ramified; Laurent at inf) (1.12) |
| T5 | `adf_sball` | generalises by: set of place handles; tag real, complex, or non-archimedean infinite (1.12) |
| T6 | `adf_adele` | generalises by: one ball per infinite place (real, complex, Laurent), a finite ball (1.5) |
| T7 | `adf_cadele` | special to Q: general object `(K (x) C) x A_K,f`; nothing of this kind for F (1.5) |
| T8 | `adf_ucoset` | generalises by: modulus an integral ideal, `c` in `O/n`, product modulo `n + n'` (1.6, 2.5) |
| T9 | `adf_idele` | generalises by: archimedean family; content, class index, generator mod units; coset (1.7) |
| T10 | `adf_idclass` | special to Q: `C_K`, an extension of `Cl_K` by `(K_inf^x x Ohat^x) / O^x` (1.7; F: 2.5) |
| T11 | `adf_qclass` | generalises by: K: parallelogram, two gluing rules; F: a subgroup, no gluing (1.9, 2.3) |
| T12 | `adf_ffun` | generalises by: ideals `d`, `m`; index group `d^-1 / m`, not cyclic; the different (1.8) |
| T13 | `adf_rfun` | special to Q: Schwartz functions on `K_inf` (number fields); none for F (1.12, 2.7) |
| T14 | `adf_char` | generalises by: ideal conductor, ray class characters, infinity type; F: `s` periodic (1.12) |
| T15 | `adf_modctx` | generalises by: integer blocks with residue vectors (K); polynomial blocks (F) (1.11, 2.7) |

Types: 0 unchanged, 12 generalise, 3 special to `Q`.

### 4.2 Public concepts

| # | Concept | Verdict, and by what or the general object (section) |
|---|---|---|
| C1 | status codes | unchanged; `ADF_DOMAIN` reports a place handle (R1) (1.12) |
| C2 | exact tag (M3) | unchanged: both ends are the same element of the field (1.5, 2.2) |
| C3 | `f_at`, named places | unchanged; places are named by handles (1.12) |
| C4 | division rule (4.5) | unchanged: at a free place the coordinate ranges over a local ring containing 0 (3) |
| C5 | value text form | generalises by: centre in the field, `mod` an ideal expression, place labels as tokens |
| C6 | dump form | generalises by: a field descriptor in the header; the radius as an HNF (R9) |
| C7 | tight policy | generalises by: gcd of rationals -> sum of ideals; min of valuations at inf (1.2, 2.2) |
| C8 | scaled policy | generalises by: integer modulus, residue vectors; sum tight only for a principal gcd (1.11) |
| C9 | cap policy | generalises by: `R -> R + C`; a cap exponent at infinity (1.11, 2.7) |
| C10 | global backend | generalises by: `A` a vector, `H` an HNF matrix (K); polynomials (F) (1.3, 2.7) |
| C11 | local backend | generalises by: residue vectors per integer block (K); polynomial blocks (F) (1.11, 2.7) |
| C12 | set predicates | generalises by: "is an integer" -> "lies in the ideal"; `N/M` -> `I` inside `J` (1.4) |
| C13 | radius in `Q_>0` | special to Q: a fractional ideal (K); a divisor: monic ratio, exponent at inf (F) (2.2) |
| C14 | place = prime `p` | special to Q: a prime ideal (K); a monic irreducible or infinity (F) (1.12, 2.1) |
| C15 | `prec` argument | special to Q: one per archimedean place (K); no counterpart at inf of F (1.5, 2.2) |
| C16 | idele = `r u` | special to Q: K: `alpha t_j u`, class and units enter; F: `r` monic (1.7, 2.5) |
| C17 | content | generalises by: the fractional ideal of the finite part (1.7) |
| C18 | norm | generalises by: `|z|^2` and `N(content)`; F: image `3^Z`, degree map (1.7, 2.5) |
| C19 | coset canonical form | generalises by: drop each prime with residue field `F_2` dividing once (1.6, 2.5) |
| C20 | character `psi` | generalises by: `psi_Q o Tr`, criterion `D^-1`; F: residues of `x omega` (1.8, 2.4) |
| C21 | Haar volume | generalises by: `1/N(I)`, `3^(-deg)`; self-dual volume `N(D)^(-1/2)` (1.9, 2.3) |
| C22 | reconstruction | generalises by: lattice points in a disc (K); linear algebra over `F_q` (F) (1.10, 2.6) |
| C23 | functions of 9.3 | special to Q: completions `K_P` (Tier B) for K; no `exp` in characteristic 3 (2.7) |

Concepts: 4 unchanged, 14 generalise, 5 special to `Q`. In all: 38 rows; 4 unchanged, 26 generalise, 8 special to
`Q`.

Notes to the rows. T3 and C8: the modulus is the integer `k`, residues lie in `O/kO = (Z/k)^2`; the scale is an
element of `K^x`; in F the scaled policy holds verbatim with monic scales. T7: a complexification of all
archimedean coordinates, `K (x)_Q C = C^n`, one complex ball per embedding; for an imaginary quadratic field its
shape equals that of `A_K`, with another meaning. T9: for F the idele is `(x_inf ; r ; u)` with `r` monic, as for
`Q`. T10: for F the class coordinates carry over (`s^Z x (1 + s F_3[[s]]) x Ohat_aff^x`), for K they do not. T14:
for F, `s` in `|x|^s` matters modulo `2 pi i / log q`. C15: for K the argument is kept, one per archimedean
component; at the infinite place of F arithmetic is exact and the precision is carried by the value.

## 5. Recommendations for the public header of version 1

Version 1 implements `Q` directly. These are cheap decisions now that avoid a break later; none asks for a general
abstraction.

- **R1. A place is an opaque handle, not an integer prime.** Every function that names a place takes an
  `adf_place_t` (`f_at`, `adf_lball`, `adf_sball`, valuation, absolute value, fractional part, Hilbert symbol, the
  payload of `ADF_DOMAIN`), and user code creates and reads places only through functions (for example
  `adf_place_inf()`, `adf_place_prime(p)`, `adf_place_is_archimedean(v)`, `adf_place_prime_get(v)`), never by
  writing or comparing integers. The present draft of `conventions.md` (section 7, CV-18) proposes
  `typedef ulong adf_place_t` with 0 for the archimedean place; that is cheap and acceptable for version 1 only if
  the value is not part of the contract (no documented "0 means infinity", no arithmetic on places). Reason: in K
  the prime 3 has two places, and in F the infinite place is non-archimedean and the finite places are polynomials;
  the canonical order of places then needs a tie-break among places above one prime. (C3, C14, T4, T5.)
- **R2. The radius is not an opaque type in version 1, but its contract is written for ideals.** Store the positive
  rational; document it as "the positive generator of the radius ideal"; compare radii only by inclusion
  (`contains`), never by size; read the precision per place (`v_p(H/d)` through a place handle); and offer no
  public function whose meaning needs a scalar radius beyond the constructors and getters of `Q`. (C13, T2.)
- **R3. The archimedean part is addressed through the place.** Keep one `arb` in `adf_adele` for `Q`, but put every
  archimedean operation behind `f_at(..., place)` (as `SPEC.md` 9.3.1 already does), and write the dump with a count
  of archimedean components (1 for `Q`). No public function takes "the real part" of an adele as a concept on its
  own beyond a `Q` convenience. (T6, C6.)
- **R4. `adf_cadele` is documented as special to `Q`.** Its shape coincides with the adele ring of an imaginary
  quadratic field; the documentation says that it is not that ring, and no generic name refers to it. (T7.)
- **R5. The idele scale is called the content.** Name the accessor "content" (a fractional ideal, represented for
  `Q` by its positive generator), not "scale" or "r"; mark the class coordinates `(t, u')` and the sign rule as
  accessors special to `Q`, and offer class-level operations (multiply, norm, character value) that do not expose
  them. (T9, T10, C16, C17.)
- **R6. State the coset canonical form by residue fields.** `conventions.md` writes the rule as "remove each prime
  whose residue field is `F_2` and which divides the modulus exactly once"; for `Q` this is the present rule, and
  the implementation does not change. (C19.)
- **R7. Keep the modulus context an integer, and do not promise a canonical scale.** The scaled policy's context
  holds an integer (in K this keeps word kernels, with residue vectors); document that the tightness of its sum
  rests on the gcd of two scales being a scale, which holds for `Q` and `F_q[T]` but not for K; expose no function
  that returns "the canonical scale" of a sum as part of the contract. (T3, C8.)
- **R8. The additive character is a named convention, and its criterion is stated by duality.** The function name
  or a context records the convention (for `Q`, Tate's); `conventions.md` states the criterion as "the phase of a
  ball is determined exactly when the character is trivial on its radius group" (for `Q`: an integral radius; for
  K: a radius inside the inverse different; for F: a radius divisor at least `-(omega)`). (C20.)
- **R9. The dump names the field.** The versioned dump header carries a field descriptor (`Q` in version 1); the
  value grammar keeps `mod` followed by a radius expression and place labels as tokens (`p=5`, `inf`), so that an
  ideal (`(2, 1+w)`) or a place such as `P=(3, 1+w)` can be added without breaking old files. (C5, C6.)

Not recommended: an opaque radius type, a vector of archimedean balls, a field parameter in every function, or a
generic idele type in version 1. Each would cost work now and buy nothing that R1 to R9 do not already keep open.

## 6. Findings against the specification

1. **`SPEC.md` 3, row `A_F`: "the radius is a divisor: a polynomial for the affine places".** Too narrow. As for
   `Q`, where a radius may be a fraction (`SPEC.md` 15), the affine radius of F is a fractional ideal of
   `F_q[T]`, that is, a monic rational function (for example `1/T`: the ball `a + (1/T) Ohat_aff`). With the
   exponent at infinity this is a divisor of arbitrary sign. Proposed wording: "a monic rational function for the
   affine places and a separate exponent at infinity".
2. **`SPEC.md` 3, row `A_K`: "the real part is a vector over the embeddings".** It is a vector over the infinite
   *places*, with one fixed embedding for each complex place. For K = `Q(sqrt(-5))` there are two embeddings and
   one place, hence one complex ball, not two (section 1.5). Proposed wording: "the archimedean part is one real or
   complex ball per infinite place".
3. **`SPEC.md` 3, row `A_K`: "the character uses the trace".** Correct but incomplete in a way that matters for the
   interface: where the phase of a ball is determined is decided by the inverse different, not by integrality of the
   radius; `0 + (1/2) Ohat` has a determined phase in K (section 1.8). Proposed addition: "and the different".

No statement about `Q` in `SPEC.md` was found wrong. Observation on review D4: for `F_q(T)` the decomposition `r u`
and the class coordinates do carry over, with "monic" and the leading coefficient in place of "positive" and the
sign (section 2.5); the obstruction D4 names (class group and units) appears for number fields and for function
fields of higher genus, not for `F_q(T)` with one place at infinity.

## 7. Sources pending

Standard facts used and not proved here, each to be quoted from a source on disk (work package 0.2):

1. An algebraic number is integral exactly when its minimal polynomial has integer coefficients (K.0.1).
2. The Minkowski bound for imaginary quadratic fields (K.0.4).
3. `K (x)_Q A_Q,f` is the finite adele ring of K, and `O (x) Zhat` the product of the local rings (1.2, 1.6).
4. Finite ideles of K as a restricted product; `v_P` onto `Z` with kernel `O_P^x` (1.7).
5. The Chinese remainder theorem for coprime ideals, and unique factorisation of ideals (1.6, 1.11).
6. Tate's global character `psi_Q o Tr`; the self-dual measure `N(D)^(-1/2)`; the measure `2 dx dy` at complex
   places; the standard function at a complex place (1.8, 1.9, 1.12).
7. The annihilator of `Zhat` for `psi_Q,f` is `Zhat` (used in `SPEC.md` 7); the identity `I^dual = I^(-1) D^(-1)`
   in general (1.8).
8. Normalisation `|z|^2` of the absolute value at a complex place (1.7).
9. Lagrange-Gauss reduction for enumerating lattice points in a disc (1.10).
10. Hecke characters: ray class groups, infinity types, compatibility with units (1.12).
11. The places of `F_q(T)`; the `P`-adic expansion in the completion at `P` (2.1, 2.3).
12. The character `psi_omega` through residues of a differential, residues at places of higher degree through the
    trace, and the residue theorem (2.4).

## 8. Statements and their status

| # | Statement | Status | Check |
|---|---|---|---|
| K.0.1 | `O_K = Z[w]` | proved mod source 1 | |
| K.0.2 | `N(alpha O) = N(alpha)`; principal test by the norm | proved | K2 |
| K.0.3 | `P = Z 2 + Z (1+w)`, `P^2 = (2)`, `P` not principal, `O^x = {1,-1}` | proved | K1 |
| K.0.4 | class number 2 | proved mod source 2 | K2 |
| K.1 | `Lhat ∩ K = L`; inclusion of balls is inclusion of ideals | proved mod source 3 | K5 |
| K.2 | sums, multiples and products of the `Lhat` | proved | K3 |
| K.3 | sum rule exact; product rule `aJ + bI + IJ` enclosing and tight | proved | K3 |
| K.4 | canonical form `(d; x, y; A, B, C)` with `gcd = 1` | proved | K4 |
| 1.4 | three predicates with ideals | proved | K5 |
| 1.5 | one archimedean ball per infinite place | proved | |
| 1.6 | `Ohat/n Ohat = O/n`; product mod `n + n'`; `U(Qn) = U(n)` for `N(Q) = 2` | proved mod 3, 5 | K6 |
| 1.7 | `r u` fails; `x = alpha t_j u`; the sequence for `C_K` | proved mod 4 | K1 |
| 1.7 | product formula `|sigma(alpha)|^2 = N(alpha O)` | proved mod 8 | K2 |
| 1.8 | phase determined iff `I` inside `D^(-1)`; `D = (2w)`; `(1/2) Ohat` | proved mod 6, 7 | K7 |
| 1.8 | annihilator `I^dual Ohat`; support and period of the transform | proved mod 7 | K7 |
| 1.9 | volume `1/N(I)`; `A_K/K = (C x Ohat)/O`; `[I + O : O]` pieces | proved | K8 |
| 1.10 | at most one candidate below `lambda_1(sigma(I))` | proved | K9 |
| 1.11 | scaled sum: tight radius not principal; minimal enclosures incomparable | proved mod 5 | K10 |
| 2.1 | a principal divisor has degree 0 | proved | F8 |
| 2.2 | affine rules as for `Q` with the monic gcd | proved | F2 |
| 2.2 | product rule at infinity | proved | F3 |
| 2.2 | multiplication by `T`; no global centre for the ball around `(s ; 0)` | proved | F1 |
| F.4 | `A_F = F (+) (Ohat_aff x s O_inf)`; `3^(1-n)` pieces | proved mod 11 | F4 |
| 2.4 | residue theorem for `f dT` | source 12 | F5 |
| 2.4 | `Res_inf(f dT) = -c_1`; phase at infinity fixed iff `n >= 2`; values in `mu_p` | proved | F6 |
| 2.5 | `r u` with monic `r`; norm in `3^Z`; degree onto `Z`; `Pic^0 = 0` | proved | F8 |
| 2.5 | class coordinates `(t, u')` with the leading coefficient | proved | F9 |
| 2.5 | no coset collapse for `q = 3` | proved | F10 |
| 2.6 | reconstruction count `0`, `1` or `3^(1 - n - deg N)` | proved | F7 |
| 2.7 | scaled policy verbatim for monic scales; no `exp` in characteristic 3 | proved | |
