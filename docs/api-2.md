# adelefeld: interface of milestone 2 (ideles)

Status: section 1 written by lane `i-slice1` on 2026-09-29, the first slice of work packages 2.1 and 2.3; not
reviewed. The other slices of milestone 2 add their own sections. The contract of each function is the comment
block in its header; this file gives the map and the statements that `docs/proofs/ideles.md` does not contain.

## 1. Slice 1: unit cosets and ideles, product and inverse

Headers: `include/adelefeld/ucoset.h`, `include/adelefeld/idele.h`. Code: `src/ucoset.c`, `src/idele.c`.
Tests: `tests/test_ucoset.c`, `tests/test_idele.c`, vectors `tests/ref/vectors/i-slice1/` written by
`lanes/i-slice1/gen_vectors.py` from `proto/ideles_checks.py` (part 2). Julia: `tests/julia/ideles.jl`.

### 1.1 Types and set statements

| Type | Data | Set it means | Predicate `is_canonical` | Init |
|---|---|---|---|---|
| `adf_ucoset` | `c`, `N` (`fmpz`) | `N >= 1`: `c U(N)`, the units of `Zhat` congruent to `c` modulo `N` (`ideles.md` Definition 4, line 87). `N = 0`: the one unit `c`, `c = +1` or `-1` (SPEC 5, M0-D1) | `N >= 1`, `1 <= c <= N`, `gcd(c, N) = 1`; or `N = 0`, `c = +-1` (conventions 5.6) | `[1]`: `(1, 0)` |
| `adf_idele` | `inf` (`arb`), `r` (`fmpq`), `u` (`adf_ucoset`) | the ideles `(xi, r w)` with `xi` in the closed interval `inf` and `w` in the set of `u` (Statement D) | `inf` finite and not containing 0; `r` canonical and `> 0`; `u` canonical (conventions 5.7) | the exact idele 1 |

Layouts (64-bit, FLINT 3.0.1): `adf_ucoset_struct` 16 bytes, `c` at 0, `N` at 8; `adf_idele_struct` 80 bytes,
`inf` at 0 (48 bytes), `r` at 48 (16 bytes), `u` at 64 (16 bytes). Alignment 8 for both.

Normal form (conventions 5.6): `N' = N/2` if `N = 2 mod 4`, else `N' = N`; `c'` the residue of `c` in `1..N'`;
an exact unit is its own normal form. Decision D2-1 (orchestrator): results of operations are stored in normal
form; constructors keep the modulus as supplied (CV-17).

### 1.2 Functions

| Function | Result | Source | Statuses |
|---|---|---|---|
| `adf_ucoset_init`, `_clear`, `_set`, `_swap` | life cycle | conventions 2.3 | void |
| `adf_ucoset_set_fmpz2(x, c, N)` | `c U(N)` with `c` reduced into `1..N`, `N` as supplied; `N = 0`: `c` must be `+-1` | conventions 5.6 | `OK`, `DOMAIN` (`N < 0`; `N = 0` and `c != +-1`; `gcd(c, N) != 1`) |
| `adf_ucoset_one`, `adf_ucoset_minus_one` | `[1]`, `[-1]` | M0-D1 | void |
| `adf_ucoset_get_fmpz2`, `adf_ucoset_is_exact` | stored pair; `N = 0` | | void; predicate |
| `adf_ucoset_is_canonical`, `adf_ucoset_is_normal` | predicates of conventions 5.6 | | predicate |
| `adf_ucoset_identical` | same stored pair | conventions 2.3 | predicate |
| `adf_ucoset_normalise(y, x)` | normal form | Statement B | void |
| `adf_ucoset_equal_set`, `_contains`, `_overlaps` | set predicates | `ideles.md` P9 (152); Statement A | predicate |
| `adf_ucoset_mul(z, x, y)` | `c c' U(gcd(N, N'))`, normal | P10 (195), P11 (213); Statements A, C | void |
| `adf_ucoset_inv(y, x)` | `c^-1 U(N)`, normal | P10.2; Statements A, C | void |
| `adf_idele_init`, `_clear`, `_set`, `_swap`, `_identical`, `_is_canonical` | life cycle | conventions 2.3, 5.7 | void; predicate |
| `adf_idele_set_parts(x, inf, r, u)` | `(inf, r, u)`, `r` canonicalised, `u` as supplied | conventions 5.7 | `OK`, `DOMAIN` |
| `adf_idele_set_rat(x, q, prec)` | the idele of `q`: real ball of `q`, content `abs(q)`, unit `[sign(q)]` | P3.4 (line 80); Statement E | `OK`, `NOT_UNIT` (`q = 0`), `LIMIT` (`prec > ADF_IDELE_PREC_MAX`, since slice 2) |
| `adf_idele_mul(z, x, y, prec)` | componentwise; real part by kernel B | Statements C, D, E | `OK`, `NOT_DETERMINED`, `LIMIT` (since slice 2) |
| `adf_idele_inv(y, x, prec)` | componentwise; real part by kernel B | Statements C, D, E | `OK`, `NOT_DETERMINED`, `LIMIT` (since slice 2) |
| `adf_idele_get_real`, `adf_idele_content`, `adf_idele_get_unit` | copies of the three parts | conventions 5.7 (the content) | void |
| `adf_sizeof_ucoset`, `adf_alignof_ucoset`, `adf_sizeof_idele`, `adf_alignof_idele` | layout | conventions 12.4 | |

On every status other than `OK` the value output is untouched (conventions 4.3). Aliasing: every output may be
the same object as any input of its type (conventions 4.1).

Not in this slice: `adf_idclass`, powers, norms and valuations, hulls, division, `adf_idele_mul_rat`, text and
dump forms.

### 1.3 Statements to add

`ideles.md` states the coset rules for `N >= 1` and the decomposition of ideles; the slice also needs the exact
units, the normal form of results, the set of an idele value, and a real kernel that keeps the sign. The proofs
are written out here.

**Statement A (exact units).** Let `e, e'` be in `{1, -1}`, `N' >= 1`, `gcd(c', N') = 1`.

1. `{e} {e'} = {e e'}`, and `{e} * c' U(N') = (e c') U(N')`. This is the rule `c c' U(gcd(N, N'))` of SPEC 5 with
   `gcd(0, N') = N'`, and `gcd(0, 0) = 0` for two exact units.
2. `{e}^(-1) = {e}`.
3. `{e}` is inside `c' U(N')` exactly when `e = c'` modulo `N'`, equivalently modulo the normal modulus of `N'`.
   A coset `c U(N)` with `N >= 1` is inside no `{e}`.
4. `{e}` meets `c' U(N')` exactly when `e = c'` modulo `N' = gcd(0, N')`; `{e}` meets `{e'}` exactly when `e = e'`.
5. `{e} = c' U(N')` never holds; so two values are equal sets exactly when their normal forms are the same pair.

*Proof.*
1. `e e'` is again `+-1`. For a unit `u`, `u = c'` modulo `N' Zhat` exactly when `e u = e c'` modulo `N' Zhat`
   (multiply by `e`, and by `e` again, `e^2 = 1`). So `u -> e u` maps `c' U(N')` onto the set of units congruent to
   `e c'` modulo `N'`, which is `(e c') U(N')` by Definition 4 (`ideles.md:89`); `gcd(e c', N') = 1`.
2. `e^2 = 1`.
3. `e` is a unit of `Zhat`, so it is in `c' U(N')` exactly when `e = c'` modulo `N' Zhat` (Definition 4), which for
   the integers `e` and `c'` is congruence modulo `N'`. By Lemma 7 (`ideles.md:132`) `c' U(N') = c' U(N'bar)`, so
   the same holds with the normal modulus. For the second sentence: `c U(N)` contains two different units. Take a
   unit `u_0` in it (Lemma 5.3, line 98) and an odd prime `p` not dividing `N`; the unit `w` with `w_p = 2` and
   `w_q = 1` for `q != p` is in `U(N)` (Lemma 5.2) and `w != 1`, so `u_0` and `u_0 w` are two elements (the proof
   of P13.6, line 329, uses the same element).
4. `{e}` meets a set exactly when `e` is in it: item 3 for a coset, equality for `{e'}`.
5. By item 3 a coset with `N' >= 1` has two elements and `{e}` one. Equal normal forms give equal sets
   (Statement B). Conversely, for two exact units the sets `{e}`, `{e'}` are equal only if `e = e'`; an exact
   unit and a coset are never equal; for two cosets with `N, N' >= 1`, P9.2 (`ideles.md:157`) says the sets are
   equal exactly when the canonical forms of Definition 8 are equal. The normal form here differs from
   Definition 8 only in the residue range `1..N'` instead of `[0, N')`; the two residues differ only when the
   residue is 0 in `[0, N')`, which with `gcd(c, N') = 1` happens only for `N' = 1`, where both forms are one
   pair each (`(0, 1)` and `(1, 1)`). So equal normal forms and equal canonical forms are the same condition.

**Statement B (normal form).** For `N >= 1` and `gcd(c, N) = 1`, the normal form `(c', N')` of conventions 5.6
satisfies `c' U(N') = c U(N)`, `gcd(c', N') = 1`, `1 <= c' <= N'` and `N' != 2 mod 4`; the normal form of a
normal pair is itself.

*Proof.* `N = 2 mod 4` is `v_2(N) = 1`; then `c U(N) = c U(N/2)` by Lemma 7 (`ideles.md:134`), and `N/2` is odd.
Otherwise `N' = N`. `c' = c` modulo `N'` and `gcd(c, N') = 1` (as `N'` divides `N`), so `gcd(c', N') = 1` and
`c' U(N') = c U(N')` (Definition 4 depends on `c` modulo `N'` only). `N'` is odd or divisible by 4, so a second
application changes nothing, and `c'` is already in `1..N'`.

**Statement C (product and inverse of unit cosets, as computed).** For canonical `x = (c, N)`, `y = (c', N')`:

1. With `g = gcd(N, N')` (`g = N'` if `N = 0`, `g = N` if `N' = 0`), `gcd(c c', g) = 1` and the product set
   `{u v : u in x, v in y}` equals the set of `(c c', g)`; `adf_ucoset_mul` stores its normal form. For
   `N, N' >= 1` it is the smallest coset containing the product set (P11.2, `ideles.md:218`).
2. For `N >= 2` let `c*` be the inverse of `c` modulo `N`; for `N = 1` let `c* = 1`. Then
   `{u^(-1) : u in x} = c* U(N)`; `adf_ucoset_inv` stores its normal form. For `N = 0` the inverse is `x`.

*Proof.* 1. `c` is prime to `N`, hence to its divisor `g`; the same for `c'`; so `gcd(c c', g) = 1` (for `g = 0`,
`c c' = +-1`). For `N, N' >= 1` the set is `(c c') U(g)` by P11.1 (line 217) and the smallest by P11.2. If one of
them is exact, Statement A.1. The normal form is the same set (Statement B, or A for `g = 0`). 2. P10.2
(`ideles.md:200`) for `N >= 2`; for `N = 1`, `U(1)` is the whole unit group, closed under inversion, and
`c* = 1` is the residue of `1..1`. For `N = 0`, Statement A.2.

**Statement D (the set of an idele value, product and inverse).** The value `(X, r, u)`, `X` a closed interval
not containing 0, `r > 0` rational, `u` a canonical unit coset, means the set `S = {(xi, r w) : xi in X, w in u}`
of ideles. By P3 (`ideles.md:67`) every finite idele is `r w` for exactly one positive rational `r` and one unit
`w`, so `S` is the set of ideles whose real coordinate lies in `X`, whose content is `r` and whose unit lies in
`u`.

1. For `(xi, r w)` in `S` and `(eta, s v)` in `T = (Y, s, u')`: the product is `(xi eta, (r s)(w v))`, whose
   decomposition is `(r s, w v)` (P3, uniqueness), with `w v` in the product coset of Statement C.1. So the product
   lies in `(Z, r s, u u')` for every real interval `Z` that contains `X Y = {xi eta}`.
2. The inverse of `(xi, r w)` is `(1/xi, (1/r) w^(-1))`, decomposition `(1/r, w^(-1))`, `w^(-1)` in the inverse
   coset of Statement C.2. So the inverse lies in `(Z, 1/r, u^(-1))` for every `Z` that contains `1/X`.
3. The idele of a rational `q != 0` is `(q, q)` (diagonal); its decomposition is `(abs(q), sign(q))` (P3.4,
   line 80). So it lies in `(Z, abs(q), [sign(q)])` for every `Z` that contains `q`.
4. The finite parts of 1 and 2 are exact: the content is the exact rational and the unit coset is the product
   set itself (P10, P11); only the real part is an enclosure.

**Statement E (the real kernel: end points, then a ball).** `RD_p`, `RU_p`, `RN_p` are the correct roundings to
`p` bits downwards, upwards and to nearest (FLINT `arf`, `refs/src/flint-3.0.1/arf.rst:24-39`: "Correct rounding
is guaranteed"; exponents are unbounded, so there is no overflow or underflow). For a dyadic `t > 0`, `e(t)` is
the exponent with `2^(e(t) - 1) <= t < 2^e(t)` (FLINT's `ARF_EXP`). `p >= 2` (a smaller `prec` is taken as 2,
M1-D4). A real ball `[m +- rho]` is the closed interval `[m - rho, m + rho]` (`arb.rst:7`).

1. (Bounds of one ball) Let `X = [m +- rho]` not contain 0, `sigma = sign(m)`, `l = RD_p(abs(m) - rho)`,
   `h = RU_p(abs(m) + rho)`. Then every `xi` in `X` has sign `sigma` and `l <= abs(xi) <= h`, and `l > 0`.
2. (Product) For `X`, `Y` as in 1: `xi eta = sigma_X sigma_Y abs(xi) abs(eta)` with
   `lo = RD_p(l_X l_Y) <= abs(xi eta) <= hi = RU_p(h_X h_Y)`, `lo > 0`.
3. (Inverse) `1/xi = sigma_X / abs(xi)` with `lo = RD_p(1/h_X) <= 1/abs(xi) <= hi = RU_p(1/l_X)`, `lo > 0`.
4. (Rational) For `q = n/d != 0`: `lo = RD_p(abs(n)/d) <= abs(q) <= hi = RU_p(abs(n)/d)`, `lo > 0`, and
   `e(hi) - e(lo) <= 1`.
5. (Ball from the ends, kernel B) Given dyadic `0 < lo <= hi` with at most `p` bits each and a sign `sigma`:
   - B1: if `e(hi) - e(lo) > p`, the status is `NOT_DETERMINED` and nothing is written;
   - B2: if `lo = hi`, the ball is the exact `sigma lo`;
   - B3: otherwise `m = RN_p((lo + hi)/2)` and `rho` an upper bound (a `mag`, 30 bits) of `max(hi - m, m - lo)`;
     if `m > rho`, the ball is `[sigma m +- rho]`;
   - B4: otherwise `rho'` an upper bound of `(hi - lo)/2` and `m' = lo + rho'` exactly; the ball is
     `[sigma m' +- rho']`.
   In B2 to B4 the ball contains `sigma [lo, hi]` and does not contain 0, and its midpoint has at most `2 p + 30`
   bits.
6. (What the status says) B1 is decided by `lo` and `hi` alone, and these are unique (correct rounding), so the
   status is a function of the input balls and `p`. If `hi < 2^(p-1) lo` the status is `OK`; if
   `hi >= 2^(p+1) lo` it is `NOT_DETERMINED`.

*Proof.*
1. `abs(m) > rho >= 0`, since 0 is not in `[m - rho, m + rho]`; so `m != 0` and every `xi` in the interval lies
   on the side of `m`: `abs(xi) = sigma xi` is in `[abs(m) - rho, abs(m) + rho]`. Rounding down a positive
   number `t` with `2^(e-1) <= t < 2^e` to `p >= 1` bits gives the largest `p`-bit number `<= t`, which is at
   least the `p`-bit number `2^(e-1) > 0`.
2. `abs(xi) abs(eta)` is in `[l_X l_Y, h_X h_Y]` (products of positive bounds), and rounding down (up) gives a
   lower (upper) bound; `lo > 0` as in 1.
3. `t -> 1/t` is decreasing on `t > 0`.
4. As 1. `lo` and `hi` are `p`-bit numbers with `lo <= abs(q) <= hi` and no `p`-bit number strictly between them
   (both are the neighbours of `abs(q)`, or both equal it), so `hi <= lo + 2^(e(lo) - p) <= 2 lo` for `p >= 1`,
   and `e(hi) <= e(lo) + 1`.
5. B2: `sigma lo != 0`. B3: `lo` and `hi` are `p`-bit numbers, so `RN_p` fixes them, and `RN_p` is monotone;
   `lo <= (lo + hi)/2 <= hi` gives `lo <= m <= hi`. Then `m - rho <= lo` and `m + rho >= hi`, so
   `[m - rho, m + rho]` contains `[lo, hi]`; `m > rho` says it does not contain 0. Multiplying by `sigma` maps
   the interval to `sigma [m - rho, m + rho]`. B4: `m' - rho' = lo > 0` and `m' + rho' = lo + 2 rho' >= hi`.
   B3 has a `p`-bit midpoint. The bits in B4: a `mag` has a 30-bit mantissa (`mag.rst:7`) and a bound "may be
   several ulps larger ... than the optimal bound" (`mag.rst:15`), far less than a factor 2, so
   `rho' < hi - lo` and `m' < hi < 2^e(hi)`. The lowest bit of `lo` is at least `2^(e(lo) - p)`; that of `rho'`
   is at least `2^(e(rho') - 30)`, and `rho' >= (hi - lo)/2 >= 2^(e(lo) - p - 1)` (two different `p`-bit numbers
   of which the smaller is `lo` differ by at least `2^(e(lo) - p)`), so `e(rho') >= e(lo) - p` and the lowest
   bit of `m'` is at least `2^(e(lo) - p - 30)`. The number of bits of `m'` is at most
   `e(hi) - (e(lo) - p - 30) <= 2 p + 30`, using `e(hi) - e(lo) <= p` (B1 did not stop).
6. If `hi < 2^(p-1) lo` then `2^(e(hi)-1) <= hi < 2^(p-1) 2^e(lo)`, so `e(hi) - e(lo) < p`. If
   `hi >= 2^(p+1) lo` then `2^e(hi) > hi >= 2^(p+1) 2^(e(lo) - 1)`, so `e(hi) - e(lo) > p`.

Consequences used by the tests: an exact product that fits in `p` bits (both inputs exact) gives `lo = hi` and an
exact result; the idele of a rational never gives `NOT_DETERMINED` (item 4 and B1 with `p >= 2`); the example of
SPEC 5, `x = y = 1 +- (1 - 2^-30)`, has `l = 2^-30`, `h = 2 - 2^-30`, the exact product set
`[2^-60, (2 - 2^-30)^2]`, and at `p = 128` the kernel returns a ball that excludes 0 (the reference computes the
status at every `p`: `NOT_DETERMINED` for `p <= 60`, `OK` for `61 <= p <= 139`, `check_api_real_kernel`), while
`arb_mul` returns a ball that contains 0 at every precision (lane d-ideles, finding 3).

Why not `arb_mul` and a sign test (decision D2-2): the ball product of `arb` has midpoint `m_x m_y` and radius at
least `abs(m_x) rho_y + abs(m_y) rho_x`, so its lower end is at most `m_x m_y - abs(m_x) rho_y - abs(m_y) rho_x`,
which can be negative while `(abs(m_x) - rho_x)(abs(m_y) - rho_y) > 0`: in the example it is
`1 - 2 (1 - 2^-30) < 0`.

### 1.4 Decisions taken in this slice

| Id | Question | Taken | Alternatives |
|---|---|---|---|
| i1-1 | When is `NOT_DETERMINED` returned? | B1: `e(hi) - e(lo) > p` on the rounded end points, checked first, so the status is a function of the inputs that the reference computes exactly | try B3 first and refuse only when B4 would need a long midpoint (a valid ball in more cases, but a status that depends on the rounding of `mag`); never refuse and let the midpoint grow with the gap (unbounded cost) |
| i1-2 | Name of the constructor from parts | `adf_idele_set_parts(x, inf, r, u)` | `adf_idele_set_arb_fmpq_ucoset` in the style of `adf_adele_set_arb_fball` |
| i1-3 | Type of the content in the constructor and the accessor | `fmpq_t` (raw data; the content is not a rational at every place, so not an `adf_rat`) | `adf_rat_t` |
| i1-4 | Does `set_parts` accept a non-canonical `fmpq`? | yes, if its denominator is not 0: it is canonicalised; `DOMAIN` for denominator 0 or `r <= 0` | require a canonical `fmpq` (precondition) |
| i1-5 | Real ball of `adf_idele_set_rat` | kernel B on `RD_p(abs(q))`, `RU_p(abs(q))` (never 0 by construction) | `arb_set_fmpq` and a test of `arb_is_nonzero` (the same ball in most cases, but the sign would rest on a property of `arb_set_fmpq` read from its source, not from its documentation) |
| i1-6 | A defect of the kernel (a result that fails its own sign test) | `flint_abort` with a message (as S-D20) | return `NOT_DETERMINED` |

## 2. Slice 2: idele classes, the class map, valuations, absolute values, norm

Status: written by lane `i-slice2` on 2026-09-29/30, work packages 2.1 (the class type), 2.2 and 2.3 (the class
map); not reviewed. Headers: `include/adelefeld/idclass.h` (new), `include/adelefeld/idele.h` (additions).
Code: `src/idclass.c`, `src/idele.c`, the kernel shared through `src/idele_internal.h`. Tests:
`tests/test_idclass.c`, `tests/test_idele_maps.c`, vectors `tests/ref/vectors/i-slice2/` written by
`lanes/i-slice2/gen_vectors.py` from `proto/ideles_checks.py` (part 3). Julia: `tests/julia/idclass.jl`.

### 2.1 Types and set statements

| Type | Data | Set it means | Predicate `is_canonical` | Init |
|---|---|---|---|---|
| `adf_idclass` | `t` (`arb`), `u` (`adf_ucoset`) | the classes `(tau, w)` with `tau` in the closed interval `t` and `w` in the set of `u` (Statement G; each class of `A^x/Q^x` has exactly one such representative, `ideles.md` P15.2, line 373) | `t` finite and positive; `u` canonical (conventions 5.7) | `<1 ; [1]>` |

Layout (64-bit, FLINT 3.0.1): `adf_idclass_struct` 64 bytes, `t` at 0 (48 bytes), `u` at 48 (16 bytes);
alignment 8.

### 2.2 Functions

| Function | Result | Source | Statuses |
|---|---|---|---|
| `adf_idclass_init`, `_clear`, `_set`, `_swap`, `_identical`, `_is_canonical` | life cycle | conventions 2.3, 5.7 | void; predicate |
| `adf_idclass_set_parts(x, t, u)` | `(t, u)`, `u` as supplied | conventions 5.7, 3.2 | `OK`, `DOMAIN` (`t` not finite or not positive) |
| `adf_idclass_set_idele(c, x, prec)` | the class `(abs(X)/r, sign(X) u)`, unit in normal form | `ideles.md` P15 (366), "On data" (393); Statements F, G.4 | `OK`, `NOT_DETERMINED`, `LIMIT` |
| `adf_idclass_mul(z, x, y, prec)`, `adf_idclass_inv(y, x, prec)` | componentwise; real part by kernel B with sign +1 | P15.1; Statements E, G.1, G.2 | `OK`, `NOT_DETERMINED`, `LIMIT` |
| `adf_idclass_get_t`, `adf_idclass_get_unit`, `adf_idclass_norm` | copies of `t` and `u`; the norm of a class is `t` | conventions 5.7; P14.2, P14.3 | void |
| `adf_idele_mul_rat(z, x, q, prec)` | `(Xq, r abs(q), [sign q] u)` | P3 (67); Statements F, H | `OK`, `NOT_DETERMINED`, `NOT_UNIT` (`q = 0`), `LIMIT` |
| `adf_idele_valuation_at(v, x, w)` | `v_p(r)` by removal of `p`, no factorisation | P14.1, P14.4 (347, 351) | `OK`, `DOMAIN` (archimedean place) |
| `adf_idele_abs_at(a, x, w)` | `p^(-v_p(r))`, an exact `adf_rat` | P14.1 | `OK`, `DOMAIN` (archimedean place) |
| `adf_idele_abs_inf(a, x)` | `abs(X)` as the ball `[abs(m) +- rho]`, exact | Statement I | void |
| `adf_idele_norm(t, x, prec)` | `abs(X)/r`, `1/r` exact before the rounding | P14.2, P14.3 (348, 350); Statements F, I | `OK`, `NOT_DETERMINED`, `LIMIT` |
| `adf_sizeof_idclass`, `adf_alignof_idclass` | layout | conventions 12.4 | |

On every status other than `OK` the value output is untouched (conventions 4.3). Aliasing: every output may be
the same object as any input of its type (conventions 4.1).

The precision limit (decision of the orchestrator, 2026-09-30, after lane i-review1, which found that
`adf_idele_inv` and `adf_idele_set_rat` of 1/3 at `prec = LONG_MAX` crash: FLINT tries to allocate `2^63` bits):
`ADF_IDELE_PREC_MAX = 2097152` in `idele.h`, the value of `ADF_ROOTS_REAL_PREC_MAX`. Every function of `idele.h`
and `idclass.h` that takes a `prec` returns `LIMIT` for a larger `prec`, decided from `prec` alone before any
allocation and before every other status (the maximum of conventions 3.3), outputs untouched. The rows of
section 1.2 are amended for this (they were right for slice 1). Conventions 3.2, row "Unit coset, idele, idele
class arithmetic", names `OK`, `NOT_DETERMINED`, `NOT_UNIT`; `LIMIT` is an addition to that row (conventions 3.1:
"a size bound of an algorithm").

Not in this slice: powers (`M_k`), hulls and the map to adeles, division, text and dump forms, set predicates of
classes (`equal_set`, `contains`, `overlaps`), characters.

### 2.3 Statements to add

**Statement F (the real kernel with an exact rational factor).** Let `X = [m +- rho]` be a finite ball that does
not contain 0, `l = RD_p(abs(m) - rho)`, `h = RU_p(abs(m) + rho)` (Statement E1), `a, b >= 1` integers and
`s = a/b`. Put `lo = RD_p(l a / b)` and `hi = RU_p(h a / b)`.

1. `0 < lo <= abs(xi) s <= hi` for every `xi` in `X`; `lo` and `hi` have at most `p` bits.
2. The computation `l a` exact (at most `p + bits(a)` bits), then one division by `b` rounded to `p` bits, gives
   exactly these `lo` and `hi`.
3. If `rho = 0` and `abs(m)` has at most `p` bits, and `abs(m) s` is a dyadic number of at most `p` bits, then
   `lo = hi = abs(m) s`, and kernel B returns the exact ball `sign * abs(m) s` (B2).
4. The status of kernel B on these ends is a function of `X`, `s` and `p` (as E6).

*Proof.*
1. `l <= abs(xi) <= h` (E1). Multiplication by `s > 0` keeps the order: `l s <= abs(xi) s <= h s`. Rounding
   down gives a number `<= l s` and rounding up a number `>= h s` (correct rounding, `arf.rst:24-39`). `l s > 0`,
   and rounding a positive number down to `p` bits gives at least `2^(e - 1) > 0` for its exponent `e` (as in the
   proof of E1).
2. `l` is a dyadic number of at most `p` bits and `a` an integer, so `l a` is a dyadic number of at most
   `p + bits(a)` bits: the product is exact at `ARF_PREC_EXACT` and fits in memory, the two conditions of
   `arf.rst:91-112`. `arf_div_fmpz` then rounds the exact quotient `(l a)/b` once in the given direction
   (`arf.rst:672`, `:676-680`, and the correct rounding of `arf.rst:24-39`).
3. With `rho = 0`, `abs(m) - rho = abs(m) + rho = abs(m)`, a number of at most `p` bits, which both roundings
   return unchanged; so `l = h = abs(m)` and `l s = h s = abs(m) s`, which both roundings return unchanged when
   it has at most `p` bits. Then `lo = hi` and B2 applies (E5).
4. Correct rounding makes `l`, `h`, `lo`, `hi` unique; B1 is decided by `lo` and `hi` alone.

Used with `s = abs(n)/d` for `q = n/d` (`adf_idele_mul_rat`) and with `s = d/n` for `r = n/d > 0` (the norm and
the class). The integers of the rational are used, never a ball of it (M1-D4).

**Statement G (the set of a class value; product, inverse, invariance, the class of an idele value).** The value
`(T, v)` of `adf_idclass`, `T` a closed interval of positive reals, `v` a canonical unit coset, means
`C(T, v) = {(tau, w) : tau in T, w in v}`, a subset of `R_{>0} x Zhat^x`, which `Phi` of `ideles.md` P15
(line 368) identifies with a set of classes of `A^x/Q^x` (P15.1, line 370).

1. For `(tau, w)` in `C(T, v)` and `(tau', w')` in `C(T', v')` the product of the classes is `(tau tau', w w')`, as
   `Phi` is an isomorphism of groups onto the product group `R_{>0} x Zhat^x` (P15.1). `tau tau'` lies in
   `T T' = {tau tau'}` and `w w'` in the product coset (Statement C.1). So the product lies in `C(Z, v v')` for
   every interval `Z` that contains `T T'`; the ends of E2 bound `T T'` because `tau, tau' > 0`.
2. The inverse is `(1/tau, w^(-1))`, in `C(Z, v^(-1))` for every `Z` that contains `1/T` (Statement C.2; E3).
3. For an idele value `S = (X, r, u)` and a rational `q != 0`, the class of every point of `q S` equals the class
   of the point of `S` it comes from (P15.1: the kernel of `Phi` is `Q^x`). On the values:
   `adf_idclass_set_idele` of `adf_idele_mul_rat(S, q)` has the unit `sign(X q) ([sign q] u)`, the same set as
   `sign(X) u` (Statement A.1, `sign(q)^2 = 1`), and a real ball that contains `abs(X q)/(r abs(q)) = abs(X)/r`,
   as does the real ball of the class of `S`. So the two units are equal sets and the two real balls meet.
4. (The class of an idele value.) Let `S = (X, r, u)`, `sigma` the sign of `X` (every point of `X` has it, E1).
   The classes of the points `(xi, r w)` of `S` are `Phi(xi, r w) = (abs(xi)/r, sigma w)` (P15.1, and "On data",
   line 393), so they form `C(abs(X)/r, sigma u)`, and `sigma u` is the coset `(sigma c) U(N)` for `u = c U(N)`
   (Statement A.1), the same set as its normal form (Statement B). The computed class `(T, sigma u)` with
   `T` from Statement F (`s = 1/r`) contains this set; its unit is the exact set, only `T` is an enclosure.
5. (The class of the idele of a rational.) For `S` made by `adf_idele_set_rat` from `q`, `X` contains `q`,
   `r = abs(q)` and `u = [sign q]`: the unit of the class is `[sign(q) sign(q)] = [1]` and `T` contains
   `abs(q)/abs(q) = 1` (P14.3, line 350). If `X` is the exact `q` and `q` has at most `p` bits (`p` of the call
   that makes the class), `T` is the exact 1 (Statement F.3 with `abs(m) s = 1`).

*Proof.* Written in the items; they use P15.1 (homomorphism, kernel `Q^x`), P3 (decomposition), and Statements
A, B, C, E, F.

**Statement H (an idele times an exact rational).** For a point `(xi, r w)` of `S = (X, r, u)` and `q = n/d != 0`
diagonal, the product is `(xi q, (r abs(q)) (sign(q) w))`, and its decomposition is `(r abs(q), sign(q) w)` (P3,
uniqueness, line 78). `sign(q) w` lies in `[sign q] u` (Statement A.1). So the product lies in
`(Z, r abs(q), [sign q] u)` for every interval `Z` that contains `X q`; Statement F with `a = abs(n)`, `b = d`
bounds `abs(X q)`, and the sign of `X q` is `sign(X) sign(q)`. The finite part is exact.

*Proof.* `r q w = r abs(q) sign(q) w` in `A_f` (the diagonal `q` is `abs(q) sign(q)` at every prime);
`r abs(q) > 0` is rational and `sign(q) w` is a unit of `Zhat`, so P3 gives the decomposition.

**Statement I (valuations, absolute values and norm of an idele value).** For every point `(xi, r w)` of
`S = (X, r, u)` and every prime `p`:

1. `v_p(x_p) = v_p(r) = v_p(n) - v_p(d)` for `r = n/d` in lowest terms, the number of times `p` divides `n`
   minus the number of times it divides `d` (P14.1, P14.4, lines 347, 351). `abs(v_p(r))` is at most the bit
   length of `n` or of `d`, so it fits in an `slong`.
2. `|x_p|_p = p^(-v_p(r))`, an exact rational; `p^(abs(v))` divides `n` (if `v > 0`) or `d` (if `v < 0`), so its
   size is bounded by that of `r`.
3. `|xi|_inf = abs(xi)` lies in `abs(X) = [abs(m) - rho, abs(m) + rho]`, which is the ball `(abs(m), rho)`
   (E1, step 1 of its proof).
4. The norm `abs(xi) * product_p |x_p|_p = abs(xi)/r` (P14.2, line 348) lies in `abs(X)/r`, bounded by
   Statement F with `s = 1/r = d/n`; `1/r` enters exactly.
5. For the idele of a rational `q` the norm contains 1, and is the exact 1 under the condition of G.5.

*Proof.* 1, 2: P14.1 and P14.4; `v > 0` means `p^v` divides `n`, `v < 0` means `p^(-v)` divides `d`; a positive
integer divisible by `p^k` has at least `k log2(p) >= k` bits. 3: E1. 4: P14.2 and Statement F. 5: G.5.

### 2.4 Decisions taken in this slice

| Id | Question | Taken | Alternatives |
|---|---|---|---|
| i2-1 | Names of the class accessors | `adf_idclass_get_t`, `adf_idclass_get_unit`, the verb first as `adf_idele_get_unit` of slice 1 and FLINT's `arb_get_*` | `adf_idclass_t_get`, `adf_idclass_unit_get`, the spelling of conventions 5.7 (which slice 1 did not follow for ideles) |
| i2-2 | Real part of the norm, of the class and of `mul_rat` | E1 ends, then one correct rounding of the exact rational `l a / b` (Statement F), then kernel B | `arb_div_fmpz` or `arb_mul_fmpz` on the ball and a sign test (can lose the sign as `arb_mul` does, and rounds twice); exact end points `abs(m) +- rho` without E1 (exact, but the difference of two numbers of distant exponents can need unbounded memory, `arf.rst:103-106`) |
| i2-3 | Status of the norm | the kernel of the class (a certified positive ball or `NOT_DETERMINED`), so that the norm is the `t` of the class and never contains 0 | always `OK` with an `arb` enclosure that may contain 0 (never fails, but a norm that is not certified positive) |
| i2-4 | Valuation: output type, archimedean place | `slong *` (the bound of Statement I.1); `DOMAIN` at the real place | an `fmpz` output (no bound needed, heavier for a caller); `v = 0` or a flag at the real place |
| i2-5 | Absolute value at a place | at a prime an exact `adf_rat` (`adf_idele_abs_at`, `DOMAIN` at the real place); at the real place the exact ball `abs(X)` (`adf_idele_abs_inf`, void) | one function with an `arb` output for all places (the `p`-adic value `p^(-v)` is not dyadic for odd `p` and would be rounded) |
| i2-6 | The norm of a class | `adf_idclass_norm(t, x)`: a copy of `t`, void, no `prec` (P14.2 with `r = 1`, `x_inf = t > 0`) | no separate function (conventions 5.7 lists the norm among the class-level operations) |
| i2-7 | The unit of the class of an idele | `adf_ucoset_mul(u, [sign X])`, normal form (a result of an operation, D2-1) | the stored pair of `u` with `c` negated and reduced, modulus as stored |
| i2-8 | `adf_idele_mul_rat` by 0 | `NOT_UNIT`, output untouched (conventions 3.2: `NOT_UNIT` only for an exact zero input) | `DOMAIN` |
| i2-9 | Where the kernel lives | `src/idele_internal.h`, hidden symbols of `src/idele.c`, as `src/modctx_internal.h` | a copy in `src/idclass.c` (the brief forbids it) |

## 3. Slice 3: powers, hulls, division

Status: written by lane `i-slice3` on 2026-09-30, the rest of work packages 2.1 (powers), 2.3 (the maps between
ideles and adeles) and 2.4 (division); not reviewed. With it milestone 2 is complete except for the text and dump
forms of the three types. Headers: `include/adelefeld/idpow.h`, `include/adelefeld/idmap.h` (new). Code:
`src/idpow.c`, `src/idmap.c`. Tests: `tests/test_idpow.c`, `tests/test_idmap.c`, vectors
`tests/ref/vectors/i-slice3/` written by `lanes/i-slice3/gen_vectors.py` from `proto/ideles_checks.py` (part 4).
Julia: `tests/julia/idmap.jl`. No new type.

### 3.1 Functions

| Function | Result | Source | Statuses |
|---|---|---|---|
| `adf_ucoset_pow(y, x, k)` | `c'^k U(N')` of the normal form `(c', N')`; `[e^k]` for an exact unit; `[1]` for `k = 0` | `ideles.md` P13.4 (284), P13.6 (288); Statement J.1 | void |
| `adf_ucoset_pow_tight(y, x, k)` | `chat^k U(M_k)`, the smallest coset containing the powers | P13.2, P13.3 (279, 282); Statement J.2 | void |
| `adf_idele_pow(z, x, k, prec)`, `adf_idele_pow_tight` | `(Z, r^k, u^k)`, the unit by the two rules above; `k = 0` the exact idele 1 | P3 (67); Statements K, L | `OK`, `NOT_DETERMINED`, `LIMIT` (prec, and the bits of `r^k`) |
| `adf_idclass_pow(z, x, k, prec)`, `adf_idclass_pow_tight` | `(T^k, u^k)`; `k = 0` the exact class 1 | P15.1 (370); Statements K, L.3 | `OK`, `NOT_DETERMINED`, `LIMIT` |
| `adf_adele_set_idele(y, x)` | `(X ; r c' + r lcm(N, 2) Zhat)`, `c'` odd; exact `r e` for an exact unit | P16.2, P16.3 (410, 411); Statement M.1 | void |
| `adf_adele_set_idele_simple(y, x)` | `(X ; r c' + r N' Zhat)` of the normal form | P16.1, P16.4 (409, 412); Statement M.2 | void |
| `adf_idele_set_adele(y, x)` | `(X, abs(a), [sign a])` for an exact finite part `a != 0` and `X` excluding 0 | P3.4 (80), P17 (443); Statement N | `OK`, `UNIT_NOT_CERTIFIED`, `NOT_UNIT` |
| `adf_adele_div_idele(z, x, y, prec)` | finite part `(a e)/r + (gcd(abs(a) L, M)/r) Zhat`; real part `arb_div` | P18, P19 (462, 472); Statement O | `OK`, `NOT_DETERMINED` (a non-finite `arb_div`), `LIMIT` |

On every status other than `OK` the output is untouched (conventions 4.3). Aliasing: the output may be the input
of its type (conventions 4.1); an adele and an idele are different types. The limit `ADF_IDELE_PREC_MAX` of
section 2.2 holds for every function with a `prec`. `ADF_IDELE_POW_BITS_MAX = 2^26` (in `idpow.h`) bounds the
content of a power (Statement L.4). The division of an adele by an exact rational is `adf_adele_div_rat` of
milestone 1; `tests/test_idmap.c` checks it against P18. There is no division by an adele (SPEC 4.5): the way
from an adele to a divisor is `adf_idele_set_adele`, which returns `UNIT_NOT_CERTIFIED` for a finite part of
positive radius.

Not in this slice: text and dump forms of unit cosets, ideles and classes; set predicates of ideles and classes
(decision i3-1); characters.

### 3.2 Examples (computed by the C and by the reference)

- `[1 mod 1]^2`: `adf_ucoset_pow` gives `[1 mod 1]`, `adf_ucoset_pow_tight` gives `[1 mod 24]` (P13.5).
  `[2 mod 5]^2`: `[4 mod 5]` and `[49 mod 120]`. `[5 mod 6]^1`: `[2 mod 3]` for both (P13.4: `M_1 = Nbar = 3`,
  not `N = 6`; finding 1 of lane i-slice1 against `PLAN.md` 2.1 stands, and the code follows P13.4).
- `U(1)^(-2^63)` tight: `[1 mod 2^65 * 3 * 5 * 17 * 257 * 65537]` (the Fermat primes are the `p` with `p - 1`
  dividing `2^63`).
- The idele of `-6/35` squared is `(real ball ; content 36/1225, unit [1 mod 0])`; its smallest hull is the adele
  with the exact finite part `36/1225`; the adele of `1/2` divided by it has the exact finite part `1225/72`
  (`tests/julia/idmap.jl`).
- `(3 ; 1 [2 mod 3])` has the smallest hull `5 mod 6` and the simple hull `2 mod 3`; `(1 ; 1 mod 4)` divided by
  `(3 ; [1 mod 1])` has the finite part `1 mod 2` (the example of P19).

### 3.3 Statements to add

**Statement J (the two powers of a unit coset, as computed).** Let `x` be a canonical unit coset value, `k` an
integer, and `P_k = {u^k : u in x}`.

1. (`adf_ucoset_pow`) For `k = 0`, `P_0 = {1}` and the result `[1]` is that set (P13.6, line 288: no coset with
   `N >= 1` is). For an exact unit `[e]`, `P_k = {e^k}`, and `e^k = e` for odd `k`, `1` for even `k`. For
   `x = c U(N)`, `N >= 1`, with normal form `(c', N')` (the same set, Statement B): if `N' = 1` the result is
   `U(1) = Zhat^x`, which contains every unit; if `N' >= 2` the result is `(c'^k mod N') U(N')`, `c'^k` the power
   of the inverse of `c'` modulo `N'` for `k < 0` (it exists: `gcd(c', N') = 1`). It contains `P_k` by P13.4
   (line 284) applied to `(c', N')`. The stored residue is a unit modulo `N' >= 2`, so it is not 0 and lies in
   `1..N'`; `N'` is normal; so the result is in normal form.
2. (`adf_ucoset_pow_tight`) For `x = c U(N)`, `N >= 1`, `k != 0`, normal form `(c', N')`, `a_p = v_p(N')`,
   `e_p = v_p(k)`, put `A = N' * prod_{p | N'} p^(e_p)` and `B = 2^(2 + e_2)` if `N'` is odd and `k` even, else 1,
   times `p^(1 + e_p)` for every odd prime `p` that does not divide `N'` and for which `p - 1` divides `k`. Then
   `A B = M_k` of P13, `gcd(A, B) = 1`, and the residue `rho` with `rho = c'^k` modulo `A` and `rho = 1` modulo
   `B` is `chat^k` modulo `M_k` for an integer `chat` admissible in P13.2. So `rho U(M_k)` is the smallest coset
   containing `P_k` (P13.3), and it is stored in normal form.
3. For `k = 1` and `k = -1` the two results are the same pair.
4. `bits(M_k) <= bits(N') + bits(abs(k)) + 2 + 64 (tau(abs(k)) - 1)`, `tau` the number of divisors.

*Proof.*
1. Written in the item.
2. (a) *Prime by prime.* For `p | N'` (so `a_p >= 1`, and `a_2 >= 2` because `N'` is normal):
   `v_p(A) = a_p + e_p`, the row "`a_p >= 1`" (odd `p`) or "`a_2 >= 2`" of the table of P13
   (`ideles.md:267-274`), and `v_p(B) = 0` because every prime of `B` does not divide `N'` (the factor 2 enters
   `B` only for odd `N'`). For `p = 2` not dividing `N'`: `v_2(A) = 0` and `v_2(B) = 2 + e_2` for even `k`, 0 for
   odd `k`: the rows "`a_2 = 0`". For an odd `p` not dividing `N'`: `v_p(A) = 0` and `v_p(B) = 1 + e_p` when
   `p - 1` divides `k`, else 0: the rows "`a_p = 0`". So `A B = prod p^(b_p) = M_k`. The primes of `A` divide
   `N'` and those of `B` do not, so `gcd(A, B) = 1`.
   (b) *Completeness of the search.* An odd prime `p` with `(p - 1) | k` has `p - 1 = d` for a divisor `d >= 2`
   of `abs(k)`, so it is among the `d + 1` that the code tests; every `d + 1` is tested by `n_is_prime`, a proof of
   primality below `2^64` (`refs/src/flint-3.0.1/ulong_extras.rst:833-840`), and `d + 1 <= 2^63 + 1`.
   `e_p = v_p(abs(k))` is read from the factorisation of `abs(k)` (`n_factor`, `ulong_extras.rst:1203`), 0 when
   `p` is not one of its primes. The divisors are enumerated by a counter over the exponent vectors of that
   factorisation, which visits each divisor once.
   (c) *The residue.* Let `chat` be the integer in `[0, M_k)` with `chat = c'` modulo `A` and `chat = 1` modulo
   `B` (Chinese remainder theorem, `gcd(A, B) = 1`). `N'` divides `A`, so `chat = c'` modulo `N'`, which is the
   condition of P13.2 (line 279) for the set `c' U(N') = c U(N)`. `gcd(chat, M_k) = 1`: a prime of `A` divides
   `N'` and not `c'`; a prime of `B` does not divide `chat = 1` modulo it. Then `chat^k = c'^k` modulo `A` (for
   `k < 0`, the inverses of `chat` and of `c'` modulo `A` agree, as the two numbers do) and `chat^k = 1` modulo
   `B`, so `rho = chat^k` modulo `M_k`. P13.2 gives the containment and P13.3 (line 282) the minimality.
   (d) *Normal form.* `v_2(M_k) = b_2` is 0 or at least 2 (P13.1), so `M_k` is not 2 modulo 4; `rho` is stored in
   `1..M_k` and `gcd(rho, M_k) = 1` by (c).
3. P13.4: `M_k = N'` for `k = +-1`. In the construction `e_p = 0` for every `p`, so `A = N'`; no odd prime has
   `p - 1 | +-1` and `k` is odd, so `B = 1`; `rho = c'^(+-1)` modulo `N'`, the residue of item 1.
4. `s = prod_{p | N'} p^(e_p)`, the factor `2^(e_2)` of `B` (when present) and `prod p^(e_p)` over the odd `p` of
   `B` are products of the exact prime powers `p^(v_p(k))` over disjoint sets of primes, so their product divides
   `abs(k)`. Hence `M_k <= 4 N' abs(k) prod_{p in P} p`, `P` the set of odd primes of `B`. Each `p` in `P` is
   `d + 1` for a divisor `d >= 2` of `abs(k)`, so `P` has at most `tau(abs(k)) - 1` elements, each below `2^64`.
   `bits(u v) <= bits(u) + bits(v)`.

**Statement K (the real kernel of a power).** Let `X = [m +- rho]` be a finite ball that does not contain 0,
`sigma` its sign, `p >= 2`, `k != 0`, `n = abs(k)`, and `l, h` the ends of Statement E1 (`0 < l <= abs(xi) <= h`
for `xi` in `X`). The code sets `lo = hi = 1`, `bl = l`, `bh = h`, and for each bit of `n` from the lowest: if the
bit is 1, `lo = RD_p(lo * bl)`, `hi = RU_p(hi * bh)`; then, if a higher bit remains, `bl = RD_p(bl^2)`,
`bh = RU_p(bh^2)`. For `k < 0` it then sets `lo, hi = RD_p(1/hi), RU_p(1/lo)`.

1. After the bits `0 .. i` are processed, `0 < bl <= t^(2^i) <= bh` (while it is formed) and
   `0 < lo <= t^(n mod 2^(i+1)) <= hi` for every `t` in `[l, h]`; at the end `0 < lo <= t^n <= hi`, and `lo`, `hi`
   have at most `p` bits.
2. For `k < 0`: `0 < lo <= t^k <= hi` for every `t` in `[l, h]`.
3. Kernel B (Statement E5) on `lo`, `hi` and the sign `sigma^k` (`sigma` for odd `k`, `+1` for even `k`) returns
   a ball that contains `{xi^k : xi in X}` and excludes 0, or `NOT_DETERMINED` (B1). The status is a function of
   `X`, `k` and `p`.
4. If `rho = 0`, `k > 0` and `abs(m)^k` has at most `p` bits (odd mantissa), then `lo = hi = abs(m)^k` and the
   result is the exact ball `m^k`.

*Proof.*
1. Induction over the bits. At the start `lo = hi = 1 = t^0` and `bl = l <= t <= h = bh`. A product of positive
   numbers is monotone in each factor: `0 < a <= alpha` and `0 < b <= beta` give `a b <= alpha beta`. Rounding a
   positive number down gives a positive number no larger (the proof of E1), rounding up one no smaller (correct
   rounding, `arf.rst:24-39`). So `RD_p(lo bl) <= t^(n mod 2^i) t^(2^i)` when bit `i` is 1, and likewise for the
   upper end and for the squares `t^(2^(i+1)) = (t^(2^i))^2`. Every `lo`, `hi` after the first multiplication is a
   rounding to `p` bits; `n >= 1`, so there is one.
2. `t -> 1/t` is decreasing on `t > 0`; `RD_p(1/hi) <= 1/t^n <= RU_p(1/lo)`, both positive (E3).
3. Every `xi` in `X` has the sign `sigma` (E1), so `xi^k = sigma^k abs(xi)^k` with `abs(xi)` in `[l, h]`; by 1
   and 2 `abs(xi)^k` lies in `[lo, hi]`. E5 gives the ball. Every rounding is correct, so unique, and the sequence
   of operations is fixed by `k`; B1 is decided by `lo` and `hi` alone (as E6).
4. `l = h = abs(m)` (E1 with `rho = 0`: `abs(m)` has at most `p` bits, and both roundings return it). Every number
   formed is `abs(m)^j` with `1 <= j <= n` as long as no rounding changed anything; the odd mantissa of
   `abs(m)^j` is `w^j` for the odd mantissa `w >= 1` of `abs(m)`, and `w^j <= w^n` has at most `p` bits, so every
   rounding is exact. Then `lo = hi` and B2 returns the exact ball `sigma^k abs(m)^k = m^k`.

The ends are not the correctly rounded `RD_p(l^n)`, `RU_p(h^n)`: each product rounds once, and a squaring doubles
the relative error of its input, so the relative gap between `lo` and `l^n` grows roughly linearly in `n` (not
proved here as a bound; nothing depends on it). The reference (`ref_real_pow`) makes the same sequence of
roundings, so the status and the ends agree bit for bit (the vectors check `lo`, `hi` and the status).

**Statement L (the power of an idele value and of a class value).**

1. For a point `(xi, r w)` of `S = (X, r, u)` and an integer `k`, the power in the ring `A` is
   `(xi^k, r^k w^k)` (ideles are units, so `k < 0` is allowed), and its decomposition is `(r^k, w^k)` (P3,
   uniqueness, line 78: `r^k > 0` is rational, `w^k` a unit). `w^k` lies in `P_k(u)`, hence in the unit of
   `adf_ucoset_pow` and of `adf_ucoset_pow_tight` (Statement J); `xi^k` lies in `Z` (Statement K). So the power lies
   in the result value; the content `r^k` is exact.
2. For `k = 0` every point has the power `(1, 1 * 1)`, the idele 1; the result is the exact idele 1.
3. For a class value `C(T, v)` (Statement G): `Phi` is an isomorphism of groups onto `R_{>0} x Zhat^x` (P15.1,
   line 370), so the `k`-th power of the class `(tau, w)` is `(tau^k, w^k)`; `tau^k` lies in `Z` (K with
   `sigma = +1`), `w^k` in the unit of the result. `k = 0` gives the exact class 1.
4. (The limit.) For `r = n/d` in lowest terms, `r^k` is `n^k/d^k` or `d^(-k)/n^(-k)`, again in lowest terms, and
   `bits(n^j) <= j bits(n)` (`n < 2^b` gives `n^j < 2^(j b)`). So the test
   `abs(k) (bits(n) + bits(d)) > ADF_IDELE_POW_BITS_MAX` bounds both integers of `r^k` before they are formed; it
   reads two bit counts and makes one integer division, `abs(k) > floor(MAX / b)` (for integers
   `abs(k) b > MAX` exactly then), without overflow and without allocation. For `r = 1` there is no limit. For
   `r != 1`, `bits(n) + bits(d) >= 3`, so `k = WORD_MIN` always gives `LIMIT` and `fmpq_pow_si` (`fmpq.rst:480`)
   is never called with it.
5. (Against repeated multiplication.) The product of `abs(k)` independent copies of `x` by `adf_idele_mul` has
   the content `r^abs(k)` and, by Statement C.1 applied `abs(k) - 1` times at the one modulus `N'`, the unit
   `c'^abs(k) U(N')`: the default power loses nothing against the product rule, and its real part encloses the
   subset `{xi^k}` of the product set. `tests/test_idpow.c` checks this, and that the tight unit lies inside.

**Statement M (the two hulls, as computed).** For `x = (X, r, u)`, `u = (c, N)` as stored, `r = n/d`:

1. `N >= 1`: `adf_adele_set_idele` stores `F = r c' + r L Zhat`, `L = lcm(N, 2)`, `c' = c` for odd `c` and
   `c + N` for even `c` (then `N` is odd and `c + N` is odd). By P16.2 (line 410) `F` contains the finite part
   `r w` of every point, by P16.3 (line 411) every finite ball that contains them contains `F`, and by P16.4
   (line 412) it depends only on the set: for `N = 2 mod 4` the normal modulus `N/2` is odd and
   `lcm(N/2, 2) = N = lcm(N, 2)`, and an odd integer congruent to `c` modulo `N/2` is congruent to `c` modulo `N`.
   The ball is `(n c' + n L Zhat)/d`, stored as its canonical triple by `adf_fball_set_fmpz3`, a function of the
   set (`fball.h`).
2. `adf_adele_set_idele_simple` stores `r c'' + r N'' Zhat` for the normal form `(c'', N'')` of `u`. It contains
   `r c'' U(N'') = r c U(N)` (P16.1, line 409; Statement B). It equals the smallest hull exactly when `N''` is even
   and has twice its radius when `N''` is odd (P16.4 for `(c'', N'')`).
3. `N = 0`: the finite parts are the one rational `r e`; both functions store it exactly (SPEC 5).
4. The real coordinate is `X` itself, so every point `(xi, r w)` of `x` lies in the adele value.

**Statement N (adele to idele).** A point `(t, f)` of `A = R x A_f` is a unit exactly when `t != 0` and `f` is a
unit of `A_f`. For the adele value `(I ; F)`:

1. If `F` is the exact rational `a != 0` and `I` excludes 0, every point `(t, a)` is a unit (the rational `a` is
   invertible in `A_f`), and the set of points is the set of the idele value `(I, abs(a), [sign a])`
   (Statement D: its points are `(xi, abs(a) sign(a)) = (xi, a)`; P3.4, line 80).
2. If `F` is the exact 0 or `I` is the exact 0, no point is a unit: `NOT_UNIT` is proved (conventions 3.1).
3. If `F` has a positive radius, it contains non-units (P17, line 443) and units (its non-zero rational points;
   it has infinitely many rational points and at most one is 0): no idele value has this set, and invertibility
   is not certified: `UNIT_NOT_CERTIFIED` (SPEC 4.5). The same if `I` contains 0 and a non-zero real.
4. When several apply the maximum of conventions 3.3 is returned: `NOT_UNIT` as soon as one coordinate proves
   that no point is a unit. A local finite part is never exact (`fball.h`), so it gives `UNIT_NOT_CERTIFIED`.

**Statement O (the division, as computed).** Let `x = (I ; a + M Zhat)` and `y = (Y, r, u)`, `u = c U(N)` or an
exact unit. A quotient of points is `(t/eta, f/g)` componentwise.

1. `N >= 1`: `adf_ucoset_inv` gives the normal form of `c* U(N)` (P10.2, line 200; Statement C.2), and the
   smallest hull of M.1 with content 1 gives `e + L Zhat`, `L = lcm(N, 2)`, `e` odd and `e = c*` modulo `N` (M.1,
   a function of the set). `adf_fball_mul` gives `a e + gcd(a L, e M, M L) Zhat` (precision.md Proposition 2,
   line 34), which is `a e + gcd(abs(a) L, M) Zhat` because `gcd(e, L) = 1` (P19.6, line 506). `adf_fball_div_rat`
   by the exact `r > 0` gives `(a e)/r + (gcd(abs(a) L, M)/r) Zhat` (P18, line 462), the smallest ball that
   contains the finite parts of all quotients (P19, line 472).
2. `N = 0`, `u = [e]`: the inverse unit is `[e]` (A.2) and its hull is the exact `e`; the product rule with a
   factor of radius 0 gives `a e + gcd(0, e M, 0) Zhat = a e + M Zhat`; divided by `r`, `(a e)/r + (M/r) Zhat`, the
   set of the quotients by the exact rational `e r` (P18; conventions 5.7).
3. The real part `arb_div(I, Y, p)` contains every `t/eta` (`arb.rst:9-11`: the result "contains the result of
   the (mathematically exact) operation applied to any choice of points in the input balls"); `Y` excludes 0, so
   the case "If y contains zero, z is set to 0 +- inf" (`arb.rst:870-871`) does not arise. A non-finite result is
   never stored (conventions 4.4, CV-08): `NOT_DETERMINED`. No input that reaches it is known; the probe
   `Y = [1 + 2^-100 +- 1]` gives a finite (wide) ball at every precision from 2 to 256.
4. `adf_fball_set_global` does not change the set; the result is global.

### 3.4 What was taken from the design lane d-ideles

The unreviewed reference of lane d-ideles (worktree `agent-acc17965b8910c1f2`, `proto/ideles_checks.py` part 2)
was read. Taken, each proved again above before use: the tight modulus as a product of a part at the primes of
`N'` and a part at the other primes with the centre `c'^k` modulo the first and 1 modulo the second (its
"statement E3", lines 583-602; here Statement J.2, with the part at the primes of `N'` taken from the
factorisation of `k` and not by its gcd loop `smooth_part`); binary powering of the ends with directed rounding
(lines 891-904; here Statement K); the limit on the bits of `r^k` with the value `2^26` (line 476); the
statuses of adele to idele (lines 1281-1299; here Statement N); the smallest hull with an odd representative and
the simple hull of the normal form (its decisions D2-4, D2-5); the finite part of the division by the formula of
P19 (lines 1302-1310), which the reference of part 4 keeps as the independent oracle of Statement O. Not taken:
its checks against Bernoulli denominators (no source on disk for the fact that `M_k(1, k)` is the denominator of
`B_k/(2k)`), and its unit-coset functions of slice 1, which are already on master.

### 3.5 Decisions taken in this slice

| Id | Question | Taken | Alternatives |
|---|---|---|---|
| i3-1 | Set predicates of ideles and classes | not offered: conventions 2.1 and SPEC 4.2 name `equal_set`, `overlaps`, `contains` for finite balls; the unit cosets have them (slice 1); `adf_adele` has none, and no plan item asks for them | offer them (real part as closed intervals, content by equality, unit by the coset predicates) |
| i3-2 | Type of the exponent | `slong`, every value; `abs(k)` as a `ulong` | `fmpz` (any size: `M_k` and the real part would allow it, the content needs the limit anyway) |
| i3-3 | Limit of the content of a power | `ADF_IDELE_POW_BITS_MAX = 2^26` on `abs(k) (bits(n) + bits(d))`, before `r^k` is formed; `LIMIT` | `2^24` (shorter calls); a limit tested after the power (unbounded memory first); none (`k = 2^40` would exhaust memory) |
| i3-4 | Simple hull of the stored pair or of the normal form | the normal form: a function of the set | the stored pair (`[5 mod 6]` and `[2 mod 3]` would give different balls for one set) |
| i3-5 | Real part of a power | binary powering of the E1 ends, directed rounding after each product, then kernel B (Statement K) | the correctly rounded `RD_p(l^n)` (needs `l^n` exactly, `abs(k) p` bits); `arb_pow_ui` and a sign test (loses the sign as `arb_mul` does, SPEC 5) |
| i3-6 | `adf_ucoset_pow_tight`: void or a status | void: the size of `M_k` is bounded by a function of `k` (J.4), and the search costs `tau(abs(k))` primality tests of a word | `LIMIT` above a size of `M_k` |
| i3-7 | Real part of the division | `arb_div` at `p` bits ("rounded as usual", SPEC 5); `NOT_DETERMINED` for a non-finite ball (CV-08) | the inverse of `Y` by kernel B, then `arb_mul` (two roundings; fails where kernel B fails) |
| i3-8 | Finite part of the division | the product rule of `fball.h` with the smallest hull of the inverse coset, then the exact division by `r`: one code path with the tested ring rules; P19.6 proves it equal to P19 | the formula of P19 directly (the reference does this, as the oracle) |
| i3-9 | Backend of the finite part of a result | global always (`adf_fball_set_fmpz3`, and `adf_fball_set_global` after the division) | keep a local result where `fball.h` would keep one |
| i3-10 | Adele to idele when the real ball contains 0 but is not the exact 0 | `UNIT_NOT_CERTIFIED` (the input is not certified, conventions 3.1) | `NOT_DETERMINED` (the row of conventions 3.2 admits it, for a result sign) |
| i3-11 | Division of an adele by an exact rational | `adf_adele_div_rat` of milestone 1, tested here against P18 | a second function in `idmap.h` |
| i3-12 | Name of the default hull | `adf_adele_set_idele` is the smallest hull (the brief; d-ideles D2-5); the simple one has the suffix `_simple` | the reverse |

## 4. Slice 4: the value form of unit cosets, ideles and classes, and the driver

Status: written by lane `t-slice1` on 2026-09-30; not reviewed. It closes milestone 2 except for the dump form of
the three types (not done: see 4.5). Header: `include/adelefeld/text.h` (a new block at its end, and three new
includes). Code: `src/text.c` (a new section at the end: the reader, the constrained printer, the two hidden
functions), `src/text_idele.c` (the six public functions). Tests: `tests/test_text_idele.c` (13 tests),
vectors `tests/ref/vectors/t-slice1/` written by `lanes/t-slice1/gen_vectors.py` from `proto/text_grammar.py`,
`tests/driver/i-*.cmd` and `.out` (6 scripts, checked by `lanes/t-slice1/check_driver_cases.py`), the fuzz target
`tests/fuzz/fuzz_text_idele.c`, Julia `tests/julia/text_idele.jl`. The grammar, the constraints and the
templates are those of `docs/conventions.md` 9.2 to 9.5; nothing in them is changed.

### 4.1 Functions

| Function | Result | Source | Statuses |
|---|---|---|---|
| `adf_ucoset_set_str(x, s, len, lim)` | the unit coset of `"[" int ["mod" uint] "]"`: residue in `1..N`, modulus as written (CV-17), `mod 0` dropped | conventions 9.2, 9.3, 5.6; `proto/text_grammar.py` `_ucoset` (668) | `OK`, `PARSE`, `LIMIT` (`len` only), `DOMAIN` |
| `adf_ucoset_get_str(len, x)` | the normal form `[c mod N]`, `[c]` for `N = 0` | 9.4, 5.6; `_fmt_ucoset` (736) | none (never NULL) |
| `adf_idele_set_str(x, s, len, prec, lim)` | the idele of `"(" real ";" urat "*" ucoset ")"` | 9.2, 9.3, 9.5; Statement P | `OK`, `PARSE`, `LIMIT`, `DOMAIN`, `NOT_DETERMINED` |
| `adf_idele_get_str(len, x, digits)` | `(r(x_inf) ; q(r) * U)`, `x_inf` by the constrained printing (excludes 0) | 9.4, 9.5; Statement Q | NULL, `*len = 0` beyond the bound of M1-D6 |
| `adf_idclass_set_str(x, s, len, prec, lim)` | the class of `"<" real ";" ucoset ">"` | as the idele; the real part lies in `(0, infinity)` | as the idele |
| `adf_idclass_get_str(len, x, digits)` | `<r(t) ; U>`, `t` by the constrained printing (positive) | 9.4, 9.5; Statement Q | as the idele |

`adf_text_classify` already recognised the three kinds (conventions 9.7); `tests/test_text_idele.c` checks it on
every golden row and on 2200 generated and mutated texts against the reference.

### 4.2 Statements to add

**Statement P (reading the real part of an idele or a class).** Let `[lo, hi]` be the exact decimal interval of
the real part (conventions 9.5) and `p = max(prec, 2)`. The conditions are decided on `[lo, hi]` first, in
exact rational arithmetic (`DOMAIN`, stage 6 of conventions 8.5). Then the ball is built:
1. the enclosing ball `B0` of conventions 9.5 ("Reading"), exact when `m` is dyadic with an odd mantissa of at
   most `p` bits and `r` dyadic with an odd mantissa below `2^30` (the tightness of 9.5). If `B0` satisfies the
   condition (excludes 0, respectively is positive) it is the result.
2. else kernel B of Statement E, on the end points of the absolute interval `[a, b]` (`a = lo`, `b = hi` if
   `lo > 0`, else `a = -hi`, `b = -lo`): `l = RD_p(a)`, `h = RU_p(b)`, both dyadic with at most `p` bits and
   `0 < l <= h`; `adf_idele_ball_from_ends` returns a ball that contains `sign * [l, h]`, so `[lo, hi]`, and
   excludes 0, or `NOT_DETERMINED` when `e(h) - e(l) > p` (B1).
Consequences (proved from B1, `e(RD_p(a)) = e(a)` and `e(RU_p(b)) <= e(b) + 1`): the status is `OK` whenever
`e(b) - e(a) <= p - 1`; it can be `NOT_DETERMINED` only when `e(b) - e(a) >= p`; and for every text a `prec`
large enough gives `OK` (the sentence of conventions 9.3, "a higher prec will succeed", holds; it would be false
for step 1 alone, because the radius of an `arb` has 30 bits whatever `prec` is: `1 +/- 0.99999999999` is
`[1e-11, 2 - 1e-11]` and the enclosing ball `1 +/- 1` contains 0 at every `prec`). The oracle of
`tests/test_text_idele.c` is exactly this gap rule (the vectors carry `e(b) - e(a)`).

**Statement Q (constrained printing, as implemented).** `tx_put_real_cond` of `src/text.c` is the algorithm of
conventions 9.5 ("Constrained printing"): one level `k` is `proto/text_grammar.py` `print_real_detail(mid, rad, n,
k)` with exact rationals (`tx_real_level`; `tx_ceil_k` is `_ceil_k`); the least `k >= 2` for which the printed
interval `[M - R, M + R]` satisfies the condition is taken; the text is then read back exactly (`mid = M`,
`rad = R`) and printed again until the text does not change (`print_real`, 231 to 250). The text equals the text
of the reference for the exact interval of the `arb`: `tests/test_text_idele.c` compares 1500 dyadic balls
(`tests/ref/vectors/t-slice1/print_constrained.jsonl`) with it, the real part of the printed idele or class
character by character. The value must satisfy the predicate of conventions 5.7; if the ball did not (a violated
precondition), the loop would not end, and the printer writes the unconstrained text (`k = 2`) instead.
Cost: `k` grows with the number of leading digits that the radius and the midpoint share (about the number of
decimal digits of `|mid| / (|mid| - rad)`), and a level costs the size of the numbers: the ball `2^b + 1 +/- 2^b`
prints in 0.10 s for `b = 4000`, 2.1 s for `b = 16000`, 24 s for `b = 40000`; the bound of decision M1-D6 admits
`b` up to 10^5, about seven minutes. An avoidable cost: a lower bound of the least `k` would skip the levels that
must fail; none is proved here (the levels are not nested), so the search is linear, as the specification says.

### 4.3 The driver

`adf` accepts the three kinds in `show`, `type`, `mul`, `div`, `neg`, `equal`, `contains`, `overlaps` and has the
new commands `inv`, `pow`, `powtight`, `norm`, `class`, `idele`, `hull`, `hullsimple`, `unitof`, `valuation`,
`abs`; the table is in `tools/adf/README.md` ("Ideles and classes") and in the comment of `adf_drv_units_op`.
Every pair of types that an operation does not define is `DOMAIN`; `dump` of the three kinds is `UNSUPPORTED`
(4.5). The expected lines of `tests/driver/i-*.out` were written by hand and are re-derived by
`lanes/t-slice1/check_driver_cases.py` with exact integers and rationals (207 lines, 0 disagreements); the script
also checks the facts behind each expected `NOT_DETERMINED` (the enclosing ball contains 0, the gap of the end
points exceeds `prec`).

### 4.4 Decisions taken in this slice

| Id | Question | Taken | Alternatives |
|---|---|---|---|
| t-1 | `prec` above `ADF_IDELE_PREC_MAX` in the two readers with a real part | `LIMIT`, decided from `prec` alone before the text is read (the rule of `idele.h`; conventions 8.5 has no stage for it) | no limit, as `adf_adele_set_str` (which allocates `prec` bits: the defect that `i-review1` found in `set_rat`); `LIMIT` at stage 4 (after the grammar) |
| t-2 | The real ball of the readers | enclosing ball first, kernel B from the exact end points if that ball fails the sign condition (Statement P) | the enclosing ball only (`NOT_DETERMINED` at every `prec` for `1 +/- 0.99999999999`, against conventions 9.3); kernel B only (never tight for a dyadic input whose end points need more than `p` bits, against conventions 9.5) |
| t-3 | The value form of the unit in the printers | the normal form (5.6, 9.4), by `adf_ucoset_normalise` | the stored pair (would print `[5 mod 6]` for `[2 mod 3]`) |
| t-4 | Where the code lives | reader and printers in `src/text.c` (they use its cursor, literals, exact decimals and printer), the six public functions in `src/text_idele.c`, joined by two hidden functions (`adf_tx_read_unit_form`, `adf_tx_write_unit_form`); the two declarations are repeated in both files | `#include "text.c"`; a header `src/text_internal.h` (not in the list of files of the lane); a second copy of the tokenizer |
| t-5 | `neg` of an idele | every coordinate negated, exactly: `(-X, r, [-1] u)` (no rounding, never fails) | multiplication by the idele of `-1` (a kernel B pass, `NOT_DETERMINED` possible) |
| t-6 | `neg` of a class | `DOMAIN`: the class of `-x` is the class of `x` (the idele `-1` of `Q^x` is trivial in `A^x/Q^x`), and `[-1] u'` is another class, not the negative | multiply by the class `<1 ; [-1]>` (a different operation) |
| t-7 | `equal`, `contains`, `overlaps` of ideles and classes | `DOMAIN`; unit cosets have them (`equal_set`, `contains`, `overlaps`) | `adf_idele_identical` (the identity of a representation, not a set predicate, conventions 2.1); none offered by the library (decision i3-1) |
| t-8 | Products with an exact rational | `idele * rational` and `rational * idele` (`adf_idele_mul_rat`; `0` is `NOT_UNIT`), `idele / rational` as the product with the exact inverse; `rational / idele` is `DOMAIN` | refuse all mixed pairs; offer `rational / idele` through `adf_idele_inv` (two roundings) |
| t-9 | Second operand of `valuation` and `abs` | a place, read as in `project` (`real` or a prime in decimal); exactly one token, else `PARSE`; `real` gives `DOMAIN` for `valuation` and the real ball `|x_inf|` for `abs`; a non-prime is `DOMAIN` | a value-form operand (a rational); the place list of `project` |
| t-10 | Exponent of `pow`, `powtight` | an exact rational that is an integer; another type or a denominator other than 1 is `DOMAIN`; an integer beyond a word is `LIMIT` | `DOMAIN` for a big integer |
| t-11 | Output of `norm` and of `abs ... with real` | the real ball as the text of 9.5 without a sign condition (the helper of the solver commands) | the constrained printing (positive) |
| t-12 | Existing driver cases that used `[5 mod 6]` as "a kind with no typed parser" | `[p=5: 3]` in `06_pairs`, `07_status`, `12_status_order`, `13_dump` (the expected files are unchanged; `type [5 mod 6]` still gives `ucoset`) | delete the cases; keep `[5 mod 6]` and change the expected lines |

### 4.5 Not done, findings

Not done: the dump form (conventions 10) of the three types (`adf_ucoset_dump_str`, `_load_str` and the same for
`adf_idele`, `adf_idclass`; `adf_drv_load` and `dump` of the driver have no body for them): it needs the
loader of `src/dump.c`, which is not this lane's, and the work is not small (the grammar of 10.1 has the bodies
`ucoset`, `idele`, `idclass`).

Findings: (1) the example of `PLAN.md` 5, `(2.5 +/- 1e-9 ; 3/2 * [5 mod 36])`, prints back from C as
`(2.5 +/- 1.1e-9 ; 3/2 * [5 mod 36])`: the radius `1e-9` is rounded up to 30 bits, and two significant digits
then round it up again (`<1.25 +/- 1e-30 ; [5 mod 36]>` prints `1.1e-30`). This is conventions 9.6 (C value texts
enclose and need not be fixed points); the golden files hold the reference text. (2) conventions 5.7 names the
accessors `adf_idclass_t_get` and `adf_idclass_unit_get`; the code has `adf_idclass_get_t` and
`adf_idclass_get_unit`; the code stands (as in the brief). (3) conventions 3.2 has no `LIMIT` for the functions
of ideles; the code has it (`prec` above `ADF_IDELE_PREC_MAX`), and so has this slice. (4) the sentence of
conventions 9.3 about stage 7 holds only with Statement P (see 4.2). (5) conventions 9.3 leaves the order of the
`DOMAIN` checks among themselves open; all are `DOMAIN`, so no status depends on it.
