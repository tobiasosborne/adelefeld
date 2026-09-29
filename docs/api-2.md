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
