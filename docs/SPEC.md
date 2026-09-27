# adelfeld: scope and specification, draft 0

Date: 2026-09-27. Authors: TJO with Claude (Fable). Status: **draft for discussion; nothing is implemented.**

Labels used in this document:

- **[checked]** verified by `proto/precision_rules.py` in this repository (brute force on small cases).
- **[standard]** textbook mathematics, stated from memory, not yet quoted from a source on disk.
- **[design]** a decision of this project, open to change.
- **[unverified]** a statement about other people's software or papers, from memory.

## 1. Goal

A programming surface for mathematics in which the adeles are an ordinary number type from the first day: defined,
printed, computed with, solved for and drawn with the same ease as the reals. Three hard requirements:

1. **Fast.** A C core on FLINT, measured against derived lower bounds (section 10).
2. **Arbitrary precision and ball arithmetic.** Every inexact value is a set that certainly contains the true value.
3. **Extensible.** The rationals are the first instance of a general construction (section 3), and the core is
   written so that the later instances do not need a rewrite.

Decided so far: new repository; C on FLINT (3.0.1 is installed here); version 1 covers the adeles of `Q` through the
Tate integral for the zeta function (milestone 5 of section 11). The interaction plane and the visualisation are a
separate problem, treated only through the constraints they place on the core (section 9).

## 2. The picture, in elementary terms

**Two ways to know a number approximately.**

- A real number to finite precision is its *leading* digits: `3.14159 ± 0.00001`.
- An integer known modulo `N` is its *trailing* digits, in every base dividing a power of `N` at once: `x = 7 mod 1000`
  says the decimal expansion ends in `007`.

A *profinite integer* is an integer known modulo every `N` consistently; the set of these is written `Zhat`. A
profinite integer to finite precision is one residue `a mod N`. Allowing denominators gives the *finite adeles*
`A_f`; adding one real coordinate gives the *adeles* `A = R x A_f`. **[standard]**

So an adele of `Q` to finite precision is a number known from both ends:

    x  =  ( real ball ;  a mod N )          a in Q, N in Q, N > 0

The rational number `q` sits inside as the adele whose two ends are the same number: `(q ; q)`, exactly.

**Why this is one object and not a list of p-adic numbers.** The formal definition gives an adele one coordinate
in `R` and one in each `Q_p`, with the rule that all but finitely many of the p-adic coordinates are p-adic integers.
The Chinese remainder theorem says that finitely many residues modulo prime powers are the same thing as one residue
modulo their product. So at any finite precision the whole list of p-adic coordinates is carried by one rational
centre `a` and one radius `N`. **[standard]** This is the fact that makes the type as cheap as a big integer.

**Three facts to keep in view.**

1. `A` is a ring but not a field, and not even free of zero divisors. In `Zhat` the equation `x^2 = x` has two
   solutions at every prime independently, so infinitely many altogether; modulo `N` it has `2^k` solutions, `k` the
   number of primes dividing `N` (`N = 30`: the 8 solutions `0, 1, 6, 10, 15, 16, 21, 25`). **[checked]**
2. `Q` is a discrete subset of `A` (like `Z` inside `R`) and the quotient `A/Q` is compact (like the circle `R/Z`).
   **[standard]**
3. The invertible adeles, the *ideles*, need their own type with its own notion of precision (section 5).

## 3. What is more general than the adeles of Q

The adeles of `Q` are one instance of a single construction, the **restricted product**: a set of *places* `v`; at each
place a locally compact ring (or group) `X_v`; at all but finitely many places a compact open piece `O_v` of `X_v`; the
elements are the families `(x_v)` with `x_v` in `O_v` for all but finitely many `v`. **[standard]**

| Instance | Places | Local pieces | New cost over `Q` |
|---|---|---|---|
| `A_Q` | primes and one real place | `Q_p` with `Z_p`; `R` | none (version 1) |
| Finite and partial adeles `A_f`, `A_S` | a subset `S` of the places | the same | none; these are flags on the type |
| `A_K`, `K` a number field | prime ideals; real and complex places | finite extensions of `Q_p`; `R`, `C` | radius is an ideal, not a number; class group and units enter |
| `A_F`, `F` the function field of a curve over a finite field | the closed points of the curve | Laurent series over finite fields | no real place at all; every place is alike |
| `G(A)` for an algebraic group `G` | as for the base field | `G(Q_p)` with `G(Z_p)` | matrices; `GL_1` is the ideles; Heisenberg group, `GL_2`, unit groups of quaternion algebras |
| Adeles of a central simple algebra | as for the base field | matrix or division algebras | non-commutative local arithmetic |
| Higher adeles (Parshin, Beilinson) | chains point-curve-surface | iterated local fields | research level; named here, out of scope |

**[design]** Version 1 implements the first two rows only. The interfaces are written against an abstract *global
field* (a set of places, a radius type, a reduction map), so that rows three and four are new instances and not new
libraries. The rational function field `F_q(T)` is the cheapest second instance and the natural test that the
abstraction is right, because it has no real place.

## 4. The ring type

### 4.1 Values

| Type | Data | Meaning | Equality |
|---|---|---|---|
| `adf_pball` (profinite ball) | centre `a` in `Q`, radius `N` in `Q`, `N >= 0` | the set `a + N Zhat`; `N = 0` is the exact rational `a` | decidable when both radii are 0; otherwise overlap test |
| `adf_adele` | an `arb` ball `x_inf` and an `adf_pball` | `x_inf` at the real place, the profinite ball at all primes | same |
| `adf_adele_c` | the same with an `acb` ball | complex-valued functions need it | same |

The centre is kept reduced modulo the radius. The precision at the prime `p` is the exponent of `p` in `N`: the
p-adic coordinate is known modulo `p^(v_p(N))`. A prime that does not divide `N` is known only to be "`a` plus a
p-adic integer".

**[design]** *Two storage forms of the same value.* (i) **Global**: `a` and `N` as FLINT integers over a common
denominator. No factorisation is ever needed for ring operations. (ii) **Local**: when the factorisation of `N` is
known (because the user built the value from prime powers, or chose a smooth modulus), a tuple of word-sized residues
modulo prime powers. Ring operations are then componentwise and vectorise. Conversion is CRT (`fmpz_multi_CRT`). A
value records which form it is in; the two forms must print identically.

### 4.2 Precision rules

With `gcd` of rational numbers meaning the positive generator of the group they generate:

    (a + N Zhat) + (b + M Zhat)  =  (a + b) + gcd(N, M) Zhat
    (a + N Zhat) * (b + M Zhat) is contained in  a b + gcd(a M, b N, N M) Zhat

Both are tight: no smaller ball contains the result. **[checked]** (6000 random cases with rational centres and
radii, and the examples below.)

| Operation | Result | Remark |
|---|---|---|
| `(3 mod 12) + (5 mod 18)` | `2 mod 6` | radius is the gcd, as `max` of radii for real balls |
| `(3 mod 12) * (5 mod 18)` | `3 mod 6` | |
| `12 * (5 mod 18)` | `60 mod 216` | multiplying by an exact integer *gains* precision at its primes |
| `(1/3) * (5 mod 18)` | `5/3 mod 6` | dividing by 3 loses one digit at 3 |
| `(1/2 mod 8) * (2/3 mod 9)` | `0 mod 1/6` | a radius may be a fraction: the ball is larger than `Zhat` |

The third and fourth rows are the profinite counterpart of a familiar fact about real balls: multiplying by a small
number shrinks the radius. Here "small at `p`" means "divisible by `p`".

### 4.3 What the ring type cannot do

A profinite ball with `N > 0` always contains non-invertible elements: at every prime not dividing `N` the
coordinate is "anything in `a + Z_p`", which includes multiples of `p`. So **division by an `adf_adele` is not
defined**; one divides by an idele (section 5). Dividing by an exact non-zero rational is always defined.

## 5. Ideles and idele classes

**[standard]** For `Q` every finite idele is uniquely a positive rational times a unit of `Zhat`:
`A_f^x = Q_{>0} x Zhat^x`. A unit of `Zhat` to finite precision is a residue `c mod N` with `gcd(c, N) = 1`.

| Type | Data | Product |
|---|---|---|
| `adf_idele` | non-zero real ball `x_inf`; `r` in `Q_{>0}`; unit class `c mod N` | componentwise; radius `gcd(N, M)` |
| `adf_idclass` (element of `A^x / Q^x`) | positive real ball `t`; unit class `c mod N` | componentwise |

Multiplicative precision never degrades under multiplication and inversion, in contrast with section 4.2.
**[checked]** on one example (`5 * 29 = 1 mod 36`); the general statement is that `(Z/N)^x` is a group.

Operations: the absolute value at each place, `|x|_p = p^(-v_p(r))`, `|x|_inf`; the idele norm
`|x| = |x_inf| / r`; the product formula `|q| = 1` for rational `q`, used as a self-test; the forgetful map from
ideles to adeles; reduction of an idele to its class, which divides by the rational `sign(x_inf) r`.

The idele class group of `Q` is therefore `R_{>0} x Zhat^x`: one positive real number and one unit residue. Its
characters are `t^s` times a Dirichlet character modulo `N`. This is the whole of "`GL_1` over `Q`" and it is two
machine words plus a ball at low precision. **[standard]**

## 6. Quotient by Q, characters

- **Reduction modulo `Q`.** Given `(x_inf ; a mod N)` with `N` an integer: subtract the fractional part of `a` from
  both ends, then an integer to bring the real end into `[0, 1)`. The result is a point of `[0,1) x Zhat`, the
  standard fundamental domain. **[standard]** If `N` is not an integer the class is known too coarsely to reduce and
  the function says so.
- **The standard additive character** `psi`, trivial on `Q`: a complex ball of modulus one computed from `x_inf`
  and the fractional part of `a`. The sign convention is to be fixed by quotation from Tate's thesis, not from
  memory. **[design]**
- **Dirichlet characters and Gauss sums** on `(Z/N)^x`: FLINT's `dirichlet` and `acb_dirichlet` modules.

## 7. Functions, Fourier transform, the acceptance test

**[standard]** The test functions on `A` are finite sums of products `f_inf * f_fin`:

- `f_inf`: a polynomial times a Gaussian. The Fourier transform maps this class to itself by an exact formula
  (Hermite functions).
- `f_fin`: supported on `(1/D) Zhat` and constant on the cosets of `M Zhat`. This is an array indexed by the finite
  group `(1/D) Z / M Z`, of length `D M`. Its Fourier transform is the finite Fourier transform of the array,
  supported on `(1/M) Zhat` and constant modulo `D`.

Operations: evaluation at an adele; sum, product, translation, dilation by an idele; Fourier transform; Haar
integral; Poisson summation (the sum of `f` over `Q` equals the sum of its transform over `Q`), used as a self-test;
the local and global zeta integrals of Tate.

**Acceptance test for version 1.** With `f_inf` the Gaussian and `f_fin` the indicator of `Zhat`, the global zeta
integral returns the completed zeta function `pi^(-s/2) Gamma(s/2) zeta(s)` as a certified ball, agreeing with
`acb_dirichlet` at a list of points; the same with a Dirichlet character.

## 8. Solving equations

| Problem | Meaning over `A` | Method | Version |
|---|---|---|---|
| Linear `a x = b` | `a` an idele: unique solution. Otherwise a coset of solutions or none | section 5 | 1 |
| Linear systems | matrices over `Zhat` to precision `N` | Hermite and Smith forms modulo `N` (`fmpz_mat`) | 1 |
| Polynomial roots | the solution set is a product of local solution sets, in general infinite (section 2, fact 1) | real roots by `arb`; roots modulo `N` with Hensel lifting certified prime by prime; Newton and Hensel are the same iteration at different places | 1 for given primes |
| Roots at *all* primes | needs the splitting behaviour of the polynomial at every prime | density theorems; number fields | later |
| Rational solutions from adelic ones | local-to-global | rational reconstruction from `(x_inf ; a mod N)` | 1 |

The last row is the everyday use: a rational number of bounded height is determined by a good enough adelic ball,
and lattice reduction recovers it. **[standard]**

## 9. Constraints from the interaction plane (deferred)

**[design]** The plane itself is a separate document. It needs three things from the core, from the first commit:

1. A stable C interface with no hidden global state (contexts are explicit arguments).
2. A canonical text form for every value that reads back to the identical value, including radius and storage form.
3. Every function that can fail returns a status that says why (for example "ball contains a non-unit").

## 10. Performance discipline

**[design]** A measured time or size is reported as a ratio to a derived lower bound, with the assumptions stated;
an improvement is a reduction of that ratio. The method is the user-level skill `perf-bounds` (in preparation
alongside this draft). For each core operation the repository will carry a row: object, operation, size, floor and
its model, measured, ratio. No floor is quoted in this draft because none has been derived yet.

## 11. Milestones

| # | Content | Done when |
|---|---|---|
| 0 | Repository skeleton, build, test harness, house rules | `make check` runs an empty suite |
| 1 | `adf_pball`, `adf_adele`: both storage forms, arithmetic, printing and parsing | section 4.2 table reproduced in C; round trip of text form |
| 2 | Ideles, idele classes, absolute values | product formula self-test on random rationals |
| 3 | Reduction modulo `Q`, additive character, Dirichlet characters | `psi` is 1 on random rationals, to the ball |
| 4 | Test functions, Fourier transform, Poisson summation | Poisson self-test; double transform is reflection |
| 5 | Tate integrals | section 7 acceptance test |
| 6 | Second instance: `F_q(T)`, then number fields | the version 1 tests, restated, pass for the new instance |

House rules, carried over from `zst`: a failing test precedes every change; fuzzing and mutation testing; every
formula in `src/` cites a source on disk by file and line.

## 12. Limits

- Equality of two inexact adeles is undecidable, as for real numbers; only "certainly different" and "overlapping".
- Any operation that needs the primes of `N` costs a factorisation. Ring operations in global form never do.
- Number fields bring ideals, class groups and units, which FLINT does not have. **[unverified]**
- Class-group computations are certified only under the generalised Riemann hypothesis. **[unverified]**

## 13. Prior art

**[unverified]** The one implementation known to us is M. Hertogh's Sage package with a Leiden thesis (2021), covering
the ring and the ideles over number fields with the precision rules of section 4.2. It must be fetched and read, and
the rules compared, before milestone 1 is fixed. A survey of what other systems offer is in the parent project,
`riemann-channel/notes/adeles/software-and-algorithmic-scope.md` (also from memory).

## 14. Open decisions

1. Name of the project (`adelfeld` is a placeholder) and licence.
2. Whether the radius may be a fraction (section 4.2, last row) or is restricted to integers with a separate
   denominator. This draft allows fractions because the rules are then uniform.
3. Default modulus family for the local form: factorials, primorial powers, or user-chosen.
4. Whether version 1 includes the complex-valued adele type or adds it at milestone 3.
