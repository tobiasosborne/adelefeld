# adelefeld: scope and specification, draft 2

Date: 2026-09-27. Authors: TJO with Claude (Fable). Status: **draft; nothing is implemented.** Companion documents:
`PLAN.md` (work packages), `PERF.md` (floors), `proofs/precision.md` (proofs of the arithmetic rules).

Draft 2 applies the design review `reviews/astra-2026-09-27/review.md` (codex `gpt-6-astra`, 28 findings). Finding
ids (M3, D1, ...) are cited where a statement was changed because of them.

Labels used in this document:

- **[proved]** proved in `proofs/precision.md` or in the cited review finding (a second model family's proof, read
  by us; not yet checked by a third party).
- **[checked]** verified by a program in `proto/` or in the review's `checks/`.
- **[standard]** textbook mathematics, stated from memory, not yet quoted from a source on disk.
- **[design]** a decision of this project, open to change.
- **[unverified]** a statement about other people's software or papers, not yet checked against the source.

## 1. Goal

A programming surface for mathematics in which the adeles are an ordinary number type from the first day: defined,
printed, computed with, solved for and drawn with the same ease as the reals. Three hard requirements:

1. **Fast.** A C core on FLINT, measured against derived lower bounds (section 11).
2. **Arbitrary precision and ball arithmetic.** Every inexact value is a set that certainly contains the true value.
3. **Extensible.** The rationals are the first instance of a general construction (section 3). The semantic seams
   for later instances are fixed on paper before the first public header; concrete structs are not promised to stay
   unchanged (D4).

Version 1 covers the adeles of `Q`: arithmetic, elementary functions (section 9.3), ideles, characters, Fourier
analysis, and the Tate integral for the zeta function and Dirichlet L-functions. The
interaction plane and the visualisation are a separate problem, treated only through the constraints they place on
the core (section 10).

## 2. The picture, in elementary terms

**Two ways to know a number approximately.**

- A real number to finite precision is its *leading* digits: `3.14159 ± 0.00001`.
- An integer known modulo `N` is its *trailing* digits: `x = 7 mod 1000` says the decimal expansion ends in `007`.

A *profinite integer* is an integer known modulo every `N` consistently; the set of these is written `Zhat`.
Allowing denominators gives the *finite adeles* `A_f`; adding one real coordinate gives the *adeles* `A = R x A_f`.
**[standard]**

**The basic finite enclosure.** A *finite ball* is a set `a + N Zhat` with `a` rational and `N` a positive rational.
It fixes one p-adic ball at each prime, and is all of `Z_p` outside a finite set of primes. Every product of local
balls of that kind is one such set, by clearing denominators and the Chinese remainder theorem. **[proved]** (M2)
Radius `N = 0` is allowed in addition and means the single rational point `a`.

What this does and does not say:

- It needs only rational and integer arithmetic, and no factorisation. It does not say that an operation costs the
  same as one big-integer operation.
- A finite union of balls, or correlated information, is in general not one ball. The residues `{0, 1}` modulo 3 are
  two balls.
- A radius `p^k` still constrains every other prime (to `a + Z_q`). It is not a ball in `Q_p` alone. One place, or a
  set of places, has its own type (section 4.1).
- Radii are ordered by divisibility, not by size: `R Zhat` is inside `S Zhat` exactly when `R/S` is an integer.
  `(2/3) Zhat` and `Zhat` are not comparable.

So an adele of `Q` to finite precision is a number known from both ends:

    x  =  ( real ball ;  a mod N )

**Three facts to keep in view.**

1. `A` is a ring with zero divisors. In `Z_p` the equation `x^2 = x` has only the solutions 0 and 1; in `Zhat` the
   choice is free at every prime, so there are continuum many solutions. Modulo a positive integer `N` there are
   `2^k`, `k` the number of primes dividing `N` (`N = 30`: `0, 1, 6, 10, 15, 16, 21, 25`). **[proved]** (M13)
   **[checked]**
2. `Q` is a discrete subset of `A` and the quotient `A/Q` is compact. **[proved]** (M6)
3. The invertible adeles, the *ideles*, need their own type with its own notion of precision (section 5).

## 3. What is more general than the adeles of Q

The adeles of `Q` are one instance of the **restricted product**: a set of *places* `v`; at each place a locally
compact ring (or group) `X_v`; at all but finitely many places a compact open **subring (or subgroup)** `O_v`; the
elements are the families `(x_v)` with `x_v` in `O_v` for all but finitely many `v`. "Subring" matters: with an
arbitrary compact open subset the result need not be closed under addition. **[proved]** (D4)

| Instance | Places | What changes against `Q` |
|---|---|---|
| `A_Q` | primes and one real place | version 1 |
| One place, or a set `S` of places | stated explicitly, with the convention "over `S`" or "away from `S`" | own types with projection maps; version 1 |
| `A_K`, `K` a number field | prime ideals; real and complex places | the radius is a fractional ideal (no single scalar when it is not principal); the real part is a vector over the embeddings; the character uses the trace; class group and units enter |
| `A_F`, `F = F_q(T)` | monic irreducible polynomials, and the place at infinity | the radius is a divisor: a polynomial for the affine places and a separate exponent at infinity; no ordered scale; the norm has discrete image |
| `G(A)` for an algebraic group `G` | as for the base field | matrices; `GL_1` is the ideles |
| Adeles of a central simple algebra | as for the base field | non-commutative local arithmetic |
| Higher adeles (Parshin, Beilinson) | chains point-curve-surface | not a restricted product of this kind; out of scope |

**[design]** Version 1 implements `Q` directly. Before the first public header is fixed, the interface is tested on
paper against two cases: a non-principal ideal of a number field, and the place at infinity of `F_q(T)`
(`docs/seams.md`, work package 0.6). The second field is implemented later. No universal abstraction is built in
advance.

## 4. The additive types

### 4.1 Values

| Type | Data | Meaning |
|---|---|---|
| `adf_rat` (exact global rational) | one canonical `fmpq` | the rational `q` at every place at once; exact |
| `adf_fball` (finite ball) | `(A, H, d)`, integers, `d > 0`, `H >= 0` | the set `(A + H Zhat)/d`; `H = 0` is a single rational |
| `adf_lball` (local ball) | a prime `p` and a `padic`-like value | a ball in `Q_p` alone |
| `adf_sball` (partial ball) | a finite set of places and a ball at each | a ball in the product over those places |
| `adf_adele` | an `arb` ball and an `adf_fball` | real ball at infinity, finite ball at all primes |
| `adf_cadele` | an `acb` ball and an `adf_fball` | element of the ring `C x A_f`; see below |

**Exact rationals are a separate type.** A real ball has a dyadic midpoint, so it cannot hold `1/3` exactly. The
rational `q` as an adele is therefore the tagged exact value `adf_rat`, not a pair `(q ; q)` of balls. It stays exact
under arithmetic with other exact values, and is converted to an `adf_adele` at a requested real precision only when
it meets an inexact value. The exact tag also keeps the fact that both ends are the same number, which a pair of
balls forgets. **[proved]** (M3)

**The type `adf_cadele`.** Replacing the real coordinate by a complex one gives the ring `C x A_f`. This is not the
adele ring of `Q`, and not the adele ring of a number field. It is offered because it is useful (complex shifts of
the real coordinate) and is named for what it is. Complex *values* of functions on the adeles are plain `acb` balls
and have nothing to do with this type. **[design]** (D3; TJO decided that a complex type is wanted; its meaning is
fixed here and may be revised by TJO.)

**Canonical form of a finite ball.** For `H > 0`: `0 <= A < H` and `gcd(A, H, d) = 1`. For `H = 0`: `A/d` in lowest
terms. One denominator serves centre and radius. The precision at the prime `p` is `v_p(H/d)`. **[design]** (D2)

**Two storage backends for the same value.** (i) **Global**: `A, H, d` as FLINT integers. No factorisation is needed
for ring operations. (ii) **Local**: the same `d`, and the residues of `A` modulo pairwise coprime blocks `q_i` whose
product is `H`. The blocks need not be prime powers for ring arithmetic; operations that name a prime need certified
prime powers. A block too large for a machine word uses an integer fallback. The denominator is applied after
recombination; it is never inverted modulo a block it shares a factor with. The modulus data (blocks, reduction
constants, recombination tree) live in an immutable context that values refer to. A tight operation can change `H`,
and then the result falls back to the global backend or needs a new context. **[design]** (D2)

### 4.2 Set predicates

For finite balls with positive radii `N`, `M` (as rationals) and centres `a`, `b`: **[proved]** (M3)

| Predicate | Condition |
|---|---|
| `equal_set` | `N = M` and `(a - b)/N` is an integer |
| `overlaps` | `(a - b)/gcd(N, M)` is an integer |
| `contains` (first inside second) | `N/M` is an integer and `(a - b)/M` is an integer |

Radius zero is handled as a single point. These are three different functions. Overlap is not equality and is not
transitive: `0 mod 2` overlaps `0 mod 1`, which overlaps `1 mod 2`, and the outer two are disjoint. Equality of the
*sets* is decidable. Equality of two unknown *points* inside overlapping balls is in general not decided; the
comparison of points returns one of "certainly equal" (both exact), "certainly different" (disjoint), "undecided".

### 4.3 Precision rules (tight)

With `gcd` of rational numbers meaning the non-negative generator of the group they generate (zero if all are zero):

    (a + N Zhat) + (b + M Zhat)  =  (a + b) + gcd(N, M) Zhat
    (a + N Zhat) * (b + M Zhat) is contained in  a b + gcd(a M, b N, N M) Zhat

Both are tight: no smaller ball contains all results when the two inputs vary independently. **[proved]**
(`proofs/precision.md`; covers rational and zero radii, negative and zero centres.) **[checked]** Tight does not
mean that correlations are kept: `x - x` evaluated on a ball is a ball around 0, not 0.

| Operation | Result | Remark |
|---|---|---|
| `(3 mod 12) + (5 mod 18)` | `2 mod 6` | |
| `(3 mod 12) * (5 mod 18)` | `3 mod 6` | |
| `12 * (5 mod 18)` | `60 mod 216` | multiplying by an exact integer gains precision at its primes |
| `(1/3) * (5 mod 18)` | `5/3 mod 6` | dividing by 3 loses one digit at 3 |
| `(1/2 mod 8) * (2/3 mod 9)` | `0 mod 1/6` | a radius may be a fraction |

### 4.4 Precision policies

**[design]** (D1) Three policies. Every one is an enclosure; none may ever replace a radius by a finer one.

1. **Tight.** Each value carries its own radius; section 4.3 is applied exactly.
2. **Scaled residue.** A modulus context holds one integer `K >= 1`. A value is `s (u + K Zhat)` with a positive
   rational scale `s` and an integer `0 <= u < K`. Its radius is `s K`, not `K`. With scales `s`, `t`, put
   `g = gcd(s, t)`, `A = s/g`, `B = t/g` (integers):

       sum:      scale g,   residue (A u + B v) mod K      (tight)
       product:  scale s t, residue (u v) mod K            (encloses the tight radius s t K gcd(u, v, K))

   **[proved]** (D1) **[checked]** (20000 random cases in the review; repeated in `proto/`). Exact zero, exact
   scalars and conversion between contexts have their own rules, to be proved in work package 0.3. Conversion from a
   tight ball can lose precision and says so.
3. **Absolute cap.** A cap `C` is given; after a tight operation the radius `R` is replaced by `gcd(R, C)`, which is
   coarser or equal. The radius stays with each value.

What is *not* a policy: forcing every result to a fixed radius. `(1 mod 2) * (1/2)` is `1/2 mod 1`; writing it as
`1/2 mod 2` excludes `3/2` and is wrong. **[proved]** (D1) (Draft 1 proposed this; it is withdrawn. FLINT's `padic`
type keeps its precision in each value, not in its context. **[checked]**)

### 4.5 What the additive types cannot do

Every finite ball with positive radius contains non-invertible elements: choose a prime `p` outside the numerators
and denominators of `a` and `N`; there the coordinate ranges over all of `Z_p`, which contains 0. So a finite ball
can never certify that a divisor is invertible, and **division by an `adf_adele` is not defined**. One divides by an
exact non-zero rational or by an idele (section 5). Note also that having no zero coordinate is not enough to be
invertible: the adele with coordinate `p` at each prime `p` has none, and its inverse is not an adele. **[proved]**
(M4)

## 5. Ideles and idele classes

**Decomposition.** Every finite idele is uniquely `r u` with `r` a positive rational and `u` a unit of `Zhat`.
**[proved]** (M5) The scale `r` is exact and is part of the precision.

**Unit precision.** For an integer `N >= 1` let `U(N)` be the units of `Zhat` that are 1 modulo `N`. A unit to finite
precision is a coset `c U(N)`, `gcd(c, N) = 1`. This is not the additive ball `c + N Zhat`.

| Type | Data | Product |
|---|---|---|
| `adf_ucoset` | `c mod N`, `gcd(c, N) = 1` | `c c' mod gcd(N, N')` |
| `adf_idele` | real ball `x_inf` excluding 0; `r`; unit coset | componentwise |
| `adf_idclass` (element of `A^x / Q^x`) | positive real ball `t`; unit coset | componentwise |

At a common `N`, product and inverse keep the finite precision exactly. At different moduli the product is known
modulo `gcd(N, M)`, so the finer input's precision is lost. The real ball is rounded as usual. **[proved]** (M5)

**Operations.** Absolute value at each place, `|x|_p = p^(-v_p(r))`; the norm `|x| = |x_inf| / r`; the product
formula `|q| = 1` for rational `q`. The class of an idele has coordinates `t = |x_inf| / r` and
`u' = sign(x_inf) u`; the sign on the unit is essential. The idele class group of `Q` is `R_{>0} x Zhat^x`.
**[proved]** (M5)

**Idele to adele.** The map returns an additive ball that contains the unit coset; it is an enclosure, not an exact
conversion. Two versions: the simple ball `r c mod r N`, and the smallest ball, of radius `r lcm(N, 2)` with an odd
representative of `c`. **[proved]** (M5)

**Characters of the class group.** The continuous quasi-characters are `t^s chi(u')` with `s` complex and `chi` a
Dirichlet character; they are unitary when `Re(s) = 0`. A character carries its conductor. Its value on a unit coset
`c U(N)` is one number only when the conductor divides `N`; otherwise the function returns an enclosure of the
possible values or a status. **[proved]** (M5)

## 6. The quotient by Q and the additive character

**Reduction modulo `Q`.** An exact point has a unique representative in `[0,1) x Zhat`; the boundary is glued by
`(1, z) ~ (0, z - 1)`. **[proved]** (M6) For a ball this is an operation on sets:

- A real ball that crosses an integer after the shift wraps around and gives two pieces with different finite parts.
  Example: real part `[0.9, 1.1]`, finite part `0 mod 2` gives one piece near 1 with `0 mod 2` and one near 0 with
  `-1 mod 2`.
- A fractional radius `N = A/B` (lowest terms) splits into the `B` balls `a + k N + A Zhat`, `0 <= k < B`, each of
  integer radius. A fractional radius is therefore not an obstacle, only a multiplication of cases.

The result type `adf_qclass` is a finite union of pieces, or an unreduced lift with the meaning "modulo `Q`". A
function that may return only one piece returns the status `NEEDS_SPLIT` otherwise. A limit on the number of pieces
is an argument.

**The additive character.** Convention of Tate's thesis, section 2.2 **[unverified]** (read by the reviewer in a
scan; to be quoted from a copy on disk, work package 0.2):

    psi(x)  =  exp( 2 pi i ( - x_inf + sum over p of {x_p}_p ) )
    Fourier transform:  hat f(y) = integral of f(x) conj(psi(x y)) dx

Here `{x_p}_p` is the p-primary fractional part. `psi` is 1 on `Q`. The real Fourier kernel is `exp(+2 pi i x y)`
and the finite kernel is `exp(-2 pi i x y)`; changing only one of the two signs breaks triviality on `Q`.

On a ball: for an integer radius the finite phase is `exp(2 pi i a)`. For a fractional radius `A/B` the value is not
determined: the possible phases are that one times all `B`-th roots of unity (`0 mod 1/2` gives `+1` and `-1`). The
function returns an enclosure of all possible values, or a status. **[proved]** (M7)

## 7. Functions, Fourier transform

Version 1 implements a stated subclass of the test functions on `A`, not all of them. **[design]** (M9)

**Finite part.** Positive integers `D`, `M`; the function is zero outside `(1/D) Zhat` and constant on the cosets of
`M Zhat`; stored as the array `f_j = f(j/D)`, `0 <= j < L = D M`. Haar measure gives `Z_p` volume 1, so `N Zhat` has
volume `1/N`. Then **[proved]** (M8) **[checked]**

    hat f(k/M)  =  (1/M) * sum_{j=0}^{L-1} f_j exp(-2 pi i j k / L)
    integral of f  =  (1/M) * sum_j f_j
    norm squared   =  (1/M) * sum_j |f_j|^2  =  (1/D) * sum_k |hat f_k|^2

The transform is zero outside `(1/M) Zhat` and constant modulo `D Zhat`; the second transform has the factor `1/D`
and is the reflection `f(-x)`. With Lebesgue measure at infinity, `A/Q` has volume 1 and Poisson summation holds with
no further constant.

**Real part.** Finite sums of `P(x) exp(-pi A x^2 + B x + C)` with `P` a polynomial and `Re(A) > 0`. This family is
closed under product, translation, dilation and Fourier transform. (A polynomial times one fixed Gaussian, as in
draft 1, is not closed under translation.) **[proved]** (M9) Uncertain parameters are kept as parameters with balls,
not folded into the coefficients.

**Operations.** Evaluation at an adele (on a ball that crosses a jump of the finite part: an enclosure of all
values); sum and product (after bringing `D`, `M` to common values, whose cost is charged); translation and dilation
by exact values, or by balls precise enough to fix the array operation uniquely; `D_a f(x) = f(a x)`, with
`hat(D_a f)(y) = |a|^(-1) hat f(y/a)`; Fourier transform; Haar integral; Poisson summation with certified tails.

## 8. Tate integrals: the acceptance test

    Z(f, omega, s)  =  integral over the ideles of  f(x) omega(x) |x|^s d^x x

with `d^x x = dx/|x|` at infinity and, at each prime, the measure that gives `Z_p^x` volume 1.

- **Zeta.** `f = exp(-pi x^2)` times the indicator of `Zhat`, `omega` trivial, `Re(s) > 1`: the value is
  `pi^(-s/2) Gamma(s/2) zeta(s)`. **[proved]** (M10)
- **Dirichlet L-functions.** For a primitive character `chi` of conductor `C` and parity `e`: the same test function
  gives zero (at a ramified prime the character averages to zero over the units). The correct choice is, at ramified
  primes, the inverse local character on `Z_p^x`; at the other primes the indicator of `Z_p`; at infinity
  `x^e exp(-pi x^2)`. The result is `pi^(-(s+e)/2) Gamma((s+e)/2) L(s, chi)`; multiplied by `C^((s+e)/2)` it is the
  completed L-function. The idele character is `conj(chi(u'))`, so that the Euler factors carry `chi(p)`.
  **[proved]** (M10)
- **Outside `Re(s) > 1`** the defining integral diverges. A separate algorithm (splitting the integral and using
  Poisson summation, with proved tail bounds) computes the continuation and states its behaviour at the poles.
  The functional equation, with Gauss sum, conductor and conjugate character, is a proof obligation of work package
  0.3. **[standard]**

**Acceptance.** The integral path is independent of FLINT's L-function code; its result is compared with FLINT's
zeta and L values, completed by hand (FLINT's `xi` is normalised differently **[checked]**); the width of the result
must shrink as the precision grows; both parities and a non-real character are tested; points at and near the poles
are tested.

## 9. Solving equations, reconstruction, elementary functions

### 9.1 Solving

Every result states what problem it solves (exact coefficients, or all coefficients in a ball) and whether the list
of solutions is complete. **[design]** (M11)

| Problem | Version 1 offers |
|---|---|
| `a x = b` | division by an exact non-zero rational or a certified idele. In general the solutions are a coset of the annihilator of `a`, or none |
| Linear systems modulo `N` | a particular solution and generators of the kernel, with the transformation matrices as certificate; or a certificate that there is none |
| Polynomial roots at a given prime | simple roots by Hensel lifting with certificate. A root modulo `p` need not lift: `x^2 + 1` has the root 1 modulo 2 and none modulo 4 |
| Real roots | isolating balls with a completeness status |
| Multiple roots, roots at all primes | later, with their own certificates |

Optional for version 1, outside the Tate acceptance gate.

### 9.2 Rational reconstruction

- **From a full adelic ball.** `(a + N Zhat)` meets `Q` in the arithmetic progression `a + N Z`. Intersect it with
  the real interval. If the interval is shorter than `N` there is at most one rational. No lattice reduction is
  needed. **[proved]** (M12)
- **From partial data** (a residue modulo `m`, nothing known at other primes): the classical bounded problem, with
  bounds `|n| <= A`, `0 < d <= B`; `2 A B < m` is sufficient for uniqueness. A separate function on a separate type.
  `1/5` is `5 mod 6` in this sense, and yet `1/5` is not in the adelic ball `5 + 6 Zhat`.
- Results: one verified candidate; none; several; or "uniqueness not certified".

This is not a local-to-global principle. `(x^2 - 13)(x^2 - 17)(x^2 - 221)` has a root in `R` and in every `Q_p`,
and no rational root. **[proved]** (M12)

### 9.3 Elementary functions (in the basic package; TJO, 2026-09-27)

Elementary functions are part of version 1 and arrive with milestone 1, not after the analysis.

**Where each function is defined.** **[standard]** for the convergence statements; to be quoted from a source on
disk (work package 0.2).

| Function | Real place | Prime `p` | Several places at once |
|---|---|---|---|
| polynomials | everywhere | everywhere | full adeles |
| rational functions | denominator not 0 | denominator not 0 | denominators must be exact non-zero rationals or ideles |
| `exp`, `sin`, `cos`, `sinh`, `cosh` | everywhere | the power series converges exactly for `x` in `p Z_p` (`4 Z_2` for `p = 2`) | partial adeles: the real place and a finite set of primes. **Not on full adeles** (see below) |
| `log` | positive reals | units of `Z_p`, and all of `Q_p^x` after fixing `log p = 0` (Iwasawa's convention) | ideles: all places at once |
| `sqrt`, n-th roots | non-negative reals | where Hensel's lemma gives a root; the choice of root is stated | partial adeles; on ideles where every local root exists |
| `x^s` | positive reals | through `log` and `exp` where both converge | on idele classes: the quasi-character `t^s chi` of section 5 |
| `exp(2 pi i x)`, hence `cos(2 pi x)`, `sin(2 pi x)` | everywhere | the local character `psi_p` | full adeles, and `A/Q`: the character `psi` of section 6 |
| absolute value, valuation, sign, floor and fractional part | everywhere | everywhere | ideles (norm); full adeles (fractional part, through section 6) |
| Teichmüller representative | none | units of `Z_p` | unit cosets, prime by prime |
| Gamma, zeta, L-functions | `arb`, `acb_dirichlet` | later | through the Tate integrals of section 8 |

**Why `exp`, `sin`, `cos` are not functions on full adeles.** A finite ball with positive radius is all of `Z_p` at
almost every prime, and the series does not converge on all of `Z_p`. A non-zero exact rational is divisible by only
finitely many primes. So in the finite adeles the only point of our types in the domain is 0. **[proved]** (from the
convergence statement.) This is a fact about the adeles, not a gap in the software. The function that plays the part
of `cos` and `sin` on the adeles as a whole is the character `psi`: it is to `A/Q` what `exp(2 pi i x)` is to the
circle `R/Z`.

**How the package offers them. [design]**

- One generic entry per function, `adf_<type>_<fn>`, for the types real ball, local ball, partial ball, idele and
  adele, whenever the row above allows it.
- Applied to a full adele, `exp`, `sin`, `cos` act on the places the caller names (by default the real place
  only) and return a partial ball over those places. The result type says which places it covers; nothing is
  silently dropped.
- Outside the domain the function returns the status `DOMAIN` with the place at which it failed. A ball that is only
  partly inside the domain returns `NOT_DETERMINED`.
- Every function is an enclosure: the image of the input ball is inside the output ball. For a p-adic power series
  `f` with `f(x + h) - f(x)` in `h Z_p` on its disc (true for `exp`, `sin`, `cos`, and for `log` on `1 + p Z_p`), the
  output radius equals the input radius; this is to be proved for each function in work package 0.3.
- Implementation: the real place by `arb`/`acb`; `exp`, `log`, square root and Teichmüller lift at a prime by
  FLINT's `padic` module, which has them **[checked]** (declared in `padic.h` of FLINT 3.0.1); p-adic `sin`, `cos`,
  `sinh`, `cosh` are not in FLINT and are written here, by their power series with a proved truncation bound.

## 10. Constraints from the interaction plane (deferred)

**[design]** The plane itself is a separate document. From the first commit the core provides:

1. A C interface with no hidden global state; contexts are explicit and immutable; rules for ownership, aliasing of
   arguments, the state of outputs after a failure, and thread safety are written down.
2. Two text forms. The *value form* is canonical and does not depend on the backend. The *dump* is versioned, keeps
   backend and context, and writes real balls as exact dyadic numbers (`arb_dump_str`). A decimal `+/-` string is an
   enclosure on input, not a lossless form.
3. Status codes that distinguish: not certified to be a unit; proved not a unit; value not determined at this
   precision; needs splitting; several candidates; resource limit reached; parse error; domain error.

## 11. Performance discipline

**[design]** A measured time or size is reported as a ratio to a derived lower bound, and only when the measurement
and the bound are of the same kind (a chain against a latency bound, a batch against a throughput bound) and for the
same stated problem. The method is the skill `perf-bounds`. Floors and reference measurements: `PERF.md`.

## 12. Milestones

One map, shared with `PLAN.md`. **[design]** (P1)

| # | Content | Gate |
|---|---|---|
| 0 | Sources on disk; proofs of all rules; conventions fixed; storage invariants; benchmark contracts; seams sketch | reviewed by a second model family |
| 1 | Finite balls (tight, global), exact rationals, adeles, text forms, command-line driver; then scaled policy; then local backend | section 4 reproduced in C; policies against each other |
| 1F | Elementary functions (section 9.3): real place, local and partial balls; with milestone 2, `log` and roots on ideles | enclosure on random balls; agreement with `arb` and `padic`; domain statuses |
| 2 | Unit cosets, ideles, idele classes, division | exact coset identities; norm and product formula |
| 3 | Quotient by `Q`, additive character, class-group characters | wrapping and fractional radius cases; non-trivial phases |
| 4 | Test functions, weighted Fourier transform, Poisson summation | section 7 identities with certified tails |
| 5 | Tate integrals and continuation | section 8 acceptance |
| R | Rational reconstruction from a full ball (small; may ship with milestone 1) | section 9.2 |
| S | Solvers (optional for version 1) | section 9.1 |
| 6 | Second field: `F_q(T)`; number fields after that | after version 1 |

## 13. Limits

- Equality of unknown points inside overlapping balls is not decided; equality of the sets is.
- Any operation that needs the primes of a radius costs a factorisation, unless one is supplied. Ring operations in
  the global backend never do.
- FLINT 3.0.1 has number field elements (`nf`, `nf_elem`), unramified p-adic extensions (`qadic`) and quadratic
  forms (`qfb`). It has no maximal orders, fractional ideals, class groups or unit groups. **[checked]** (A2) Number
  fields therefore need a backend to be chosen.
- Fast class group computations are conditional on the generalised Riemann hypothesis by default; unconditional
  certification exists (PARI's `bnfcertify`) at extra cost. The guarantee obtained is to be recorded with the
  result. **[unverified]** (A2; the reviewer read PARI's documentation.)

## 14. Prior art

A directly relevant implementation is M. Hertogh's Sage package `adeles`, with the Leiden master's thesis "Computing
with adèles and idèles" (2021): adeles and ideles over number fields, multiplicative p-adic numbers, ray class
groups. Its documented rules for sum and product of profinite numbers agree with section 4.3, and it keeps separate
multiplicative precision for ideles. **[unverified]** (A1: the reviewer read the package documentation; the thesis
itself is still to be fetched and read, work package 0.2.) Its equality is an overlap test, which is not transitive;
we do not adopt that. We have made no complete survey. A list of what other systems offer, from memory, is in
`riemann-channel/notes/adeles/software-and-algorithmic-scope.md`.

## 15. Decisions (TJO, 2026-09-27)

| Question | Decision |
|---|---|
| Name and licence | `adelefeld`; AGPL-3.0 |
| May a radius be a fraction? | Yes |
| Default modulus family | None imposed: user-chosen (arbitrary integer, list of coprime blocks, prime-power list, factorial, primorial power, exact) |
| Complex type | Wanted. Meaning fixed in section 4.1 as the ring `C x A_f`, pending TJO's confirmation |
| Review of draft 1 | Apply all findings (this draft) |
| Elementary functions (`sin`, `cos`, `exp`, `log`, ...) | In the basic package (section 9.3, milestone 1F) |
| Folder and project name | `adelefeld` |
