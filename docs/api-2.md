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
| `adf_idele_set_rat(x, q, prec)` | the idele of `q`: real ball of `q`, content `abs(q)`, unit `[sign(q)]` | P3.4 (line 80); Statement E | `OK`, `NOT_UNIT` (`q = 0`) |
| `adf_idele_mul(z, x, y, prec)` | componentwise; real part by kernel B | Statements C, D, E | `OK`, `NOT_DETERMINED` |
| `adf_idele_inv(y, x, prec)` | componentwise; real part by kernel B | Statements C, D, E | `OK`, `NOT_DETERMINED` |
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
