# adelefeld: scope and specification, version 1.3

Date: 2026-09-28 (version 1.0: 2026-09-27). Authors: TJO with Claude (Fable). Status: **milestone 0 passed its
gate after the listed closure edits, applied on 2026-09-28; nothing of the C library is implemented.**
Version 1.3 applies the edits E1 to E4 and the findings C1 to C5 of the gate closure check
(`reviews/m0-gate/closure.md`). Version 1.2 applies the milestone 0 gate review (`reviews/m0-gate/review.md`),
findings G1 to G16 and its findings against the specification. Version 1.1 applied the findings of milestone 0:
the proofs in `proofs/` and their four cross-family reviews (`reviews/m0-proofs/`), the sources on disk
(`sources.md`), the seams sketch (`seams.md`) and the conventions draft (`conventions.md`). Companion documents:
`PLAN.md` (work packages), `PERF.md`
(floors), `proofs/*.md` (proofs), `sources.md` (sources and quotations), `conventions.md` (conventions, draft).

**Change log.**

| Version | Date | Changes |
|---|---|---|
| 1.3 | 2026-09-28 | Gate closure edits applied (`reviews/m0-gate/closure.md`); no formula changed. E1 (4.4): the scaled shared-pointer check also applies to `adf_scaled_mul_tight`, including exact operands; it compares the two scaled inputs, and unary and exact-scalar operations borrow the scaled input's context. E2 (10.4): a context constructor never frees, mutates or reuses the context previously pointed to by `*out` and overwrites only the pointer slot. C4 (10.4): `NOT_DETERMINED` also covers a required result invariant that the computation cannot certify (the real sign of a conversion or inversion result). C5 (4.4 agrees): conversion between scaled contexts is the best enclosing conversion with loss reported through `int *lost`; the R=0 case of the conversion from a tight ball stores the exact rational. |
| 1.2 | 2026-09-28 | Milestone 0 gate review applied. G1: a default binary operation on `adf_scaled` requires the same context pointer in both operands and otherwise returns `ADF_DOMAIN`; the implicit global fallback is limited to `adf_fball` and types containing it (4.1, 4.4). G2: context constructors `adf_modctx_new_*` and `adf_modctx_free`, incomplete public struct, caller ownership (10.4). G3: the dump loader takes an array of caller-owned context bindings, one per occurrence, in traversal order (10.2). G5: idele and idele-class arithmetic validates the real sign on its result and otherwise returns `NOT_DETERMINED`, never `NOT_UNIT` (5). G6: an exact input at a proved nonremovable pole is `DOMAIN`; a ball meeting a pole and regular points, or an undecided exclusion, is `NOT_DETERMINED` (9.3.7). G14: string results carry a length and an explicit terminator; `adf_text_classify` writes a kind enum on `OK`; set predicates return `int` (10.4). G16: the canonical triple reduces the numerator modulo `H` before cancellation (`proofs/policies.md` Summary 26). Section 15.2 states the D1 to D12 correspondence to M0-D1 to M0-D12. |
| 1.1 | 2026-09-28 | Label **[quoted]** added; 7 of 7 **[unverified]**/**[standard]** statements quoted from `refs/src/` (one attribution, "section 2.2" of Tate's thesis, stays unverified); proved statements cite `proofs/*.md` by statement number. Section 1: C is the implementation, Python only for tests, oracles and checks, a Julia layer later (M0-D12). Section 3: rows `A_K`, `A_F` corrected; seams recommendations R1 to R9 adopted (M0-D8). Section 4.1: local backend stated as raw storage against canonical triple (`policies.md` P24, P25, S26). Section 4.4: exact tag in the scaled policy; tight scaled product a separate operation (M0-D5); the cap never touches an exact value (M0-D2). Section 5: exact units (M0-D1); power of a unit coset (M0-D6); division by an idele (M0-D7); the stored class-group character against the conjugate character of the Tate integral (section 8). Section 4.1: the local backend keeps raw data (`conventions.md` CV-55, proposed). Section 6: `k + 1` pieces, closed real balls that may exceed `[0,1]` by rounding (M0-D4); the Fourier convention against the expositions on disk (M0-D11). Section 8: continuation and functional equation proved. Section 9.2: closed real interval (M0-D3). Section 9.3.7: centre of the finest-modulus coset, unit part of an idele in the Hilbert symbol, status at a pole, reciprocity named by formula (M0-D10). Section 10: Julia-friendly interface (M0-D12), dump loader validates first (M0-D9). Section 15: decisions M0-D1 to M0-D12 |
| 1.0 | 2026-09-27 | Draft 4 ratified after the closure check of review round 3 |

Draft 2 applied the design review `reviews/astra-2026-09-27/review.md` (codex `gpt-6-astra`, 28 findings); draft 3
applies round 2, `reviews/astra-2026-09-27-r2/review.md` (12 new findings N1 to N12, and the remainders of round 1),
and adds the catalogue of section 9.3.7; draft 4 applies round 3, `reviews/astra-2026-09-27-r3/review.md` (findings R1
to R6, all in sections 9.3.3 and 9.3.7). Finding ids (M3, D1, N4, ...) are cited where a statement was changed.

Labels used in this document:

- **[proved]** proved in `proofs/precision.md` or in the cited review finding (a second model family's proof, read
  by us; not yet checked by a third party), or proved in the cited statement of `proofs/*.md` and reviewed there by
  a second model family (the review record at the end of each proof file gives the verdicts).
- **[checked]** verified by a program in `proto/` or in the review's `checks/`.
- **[quoted]** quoted from a local copy of the source under `refs/src/`, cited as `key:file:line`; the quotation
  and its verdict are in `sources.md` table 2, and the lines were read again for version 1.1.
- **[standard]** textbook mathematics, stated from memory, not yet quoted from a source on disk. (No statement of
  version 1.1 carries this label.)
- **[design]** a decision of this project, open to change.
- **[unverified]** a statement about other people's software or papers, not yet checked against the source. (One
  attribution in section 6 keeps it; the source is pending.)

## 1. Goal

A programming surface for mathematics in which the adeles are an ordinary number type from the first day: defined,
printed, computed with, solved for and drawn with the same ease as the reals. Three hard requirements:

1. **Fast.** A C core on FLINT, measured against derived lower bounds (section 11). The library is written in C.
   Python is used only for tests, reference oracles and proof checks. A Julia layer will later be built on the C
   library, so the public C interface is kept friendly to Julia's `ccall` (section 10; `conventions.md` section
   12). **[design]** (M0-D12, TJO, 2026-09-27)
2. **Arbitrary precision and ball arithmetic.** Every inexact value is a set that certainly contains the true value.
3. **Extensible.** The rationals are the first instance of a general construction (section 3). The semantic seams
   for later instances are fixed on paper before the first public header; concrete structs are not promised to stay
   unchanged (D4).

Version 1 covers the adeles of `Q`: arithmetic, functions of one variable (section 9.3), ideles, characters, Fourier
analysis, and the Tate integral for the zeta function and Dirichlet L-functions. The
interaction plane and the visualisation are a separate problem, treated only through the constraints they place on
the core (section 10).

## 2. The picture, in elementary terms

**Two ways to know a number approximately.**

- A real number to finite precision is its *leading* digits: `3.14159 ± 0.00001`.
- An integer known modulo `N` is its *trailing* digits: `x = 7 mod 1000` says the decimal expansion ends in `007`.

A *profinite integer* is an integer known modulo every `N` consistently; the set of these is written `Zhat`.
Allowing denominators gives the *finite adeles* `A_f`; adding one real coordinate gives the *adeles* `A = R x A_f`.
**[quoted]** (profinite integers as the projective limit `hertogh-thesis:thesis.txt:231-233`; the adeles as the
restricted product with `U_v = O_v` at finite and `U_v = K_v` at infinite places `milne-cft:CFT.txt:9373-9376`; the
name `Zhat` is our notation.)

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
2. `Q` is a discrete subset of `A` and the quotient `A/Q` is compact. **[proved]** (M6; `proofs/quotient.md`
   Corollary 4)
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
| `A_K`, `K` a number field | prime ideals; real and complex places | the radius is a fractional ideal (no single scalar when it is not principal); the archimedean part is one real or complex ball per infinite place (not one per embedding); the character uses the trace and the different (where a phase is determined is decided by the inverse different); class group and units enter |
| `A_F`, `F = F_q(T)` | monic irreducible polynomials, and the place at infinity | the radius is a divisor: a monic rational function (a fractional ideal of `F_q[T]`) for the affine places and a separate exponent at infinity; no ordered scale; the norm has discrete image |
| `G(A)` for an algebraic group `G` | as for the base field | matrices; `GL_1` is the ideles |
| Adeles of a central simple algebra | as for the base field | non-commutative local arithmetic |
| Higher adeles (Parshin, Beilinson) | chains point-curve-surface | not a restricted product of this kind; out of scope |

**[design]** Version 1 implements `Q` directly. Before the first public header is fixed, the interface is tested on
paper against two cases: a non-principal ideal of a number field, and the place at infinity of `F_q(T)`
(`docs/seams.md`, work package 0.6). The second field is implemented later. No universal abstraction is built in
advance. The rows `A_K` and `A_F` above are corrected after the seams sketch (`seams.md` section 6).

**Rules for the public header, from the seams sketch.** **[design]** (M0-D8: recommendations R1 to R9 of `seams.md`
section 5 are adopted.)

- R1. A place is an opaque handle `adf_place_t`, created and read only through functions; its integer value, if
  any, is not part of the contract.
- R2. The radius stays a positive rational in version 1, documented as the positive generator of the radius ideal;
  radii are compared only by inclusion, and the precision is read per place.
- R3. Archimedean operations go through `f_at(..., place)`; the dump records the number of archimedean components
  (1 for `Q`).
- R4. `adf_cadele` is documented as special to `Q`; it is not the adele ring of an imaginary quadratic field.
- R5. The idele scale is called the *content* (a fractional ideal, for `Q` its positive generator); the class
  coordinates `(t, u')` and the sign rule are accessors special to `Q`.
- R6. The canonical form of a unit coset is stated by residue fields: remove each prime with residue field `F_2`
  that divides the modulus exactly once (for `Q`: the factor 2 of a modulus twice an odd number).
- R7. The modulus context of the scaled policy holds an integer; no function returns "the canonical scale" of a sum
  as part of the contract.
- R8. The additive character is a named convention; its criterion is stated by duality: the phase of a ball is
  determined exactly when the character is trivial on its radius group.
- R9. The dump header names the field (`Q` in version 1); place labels (`p=5`, `inf`) are tokens.

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

**The type `adf_cadele`: the complex archimedean adele.** Replacing the real coordinate by a complex one gives the
ring `C x A_f`. It contains the adeles. It is an enlargement of the archimedean place only: the number `i` exists in
the first coordinate and not in the finite part, so `(i ; 0)` squares to `(-1 ; 0)`, which is not the `-1` of the
ring, and the ring is not an algebra over `C`. It is not the adele ring of `Q` and not that of `Q(i)`; adjoining `i`
at all places is the adele ring of `Q(i)` and belongs to the later work on number fields. Complex *values* of
functions on the adeles are plain `acb` balls and have nothing to do with this type. Operations defined only for a
real coordinate reject a complex one unless their complex branch is specified. **[design]** (D3, N9; confirmed by
TJO on 2026-09-27.)

**Canonical form of a finite ball.** For `H > 0`: `0 <= A < H` and `gcd(A, H, d) = 1`. For `H = 0`: `A/d` in lowest
terms. One denominator serves centre and radius. The precision at the prime `p` is `v_p(H/d)`. **[design]** (D2)

**Two storage backends for the same value.** (i) **Global**: `A, H, d` as FLINT integers. No factorisation is needed
for ring operations. (ii) **Local**: the same `d`, and the residues of `A` modulo pairwise coprime blocks `q_i` whose
product is `H`. In this backend the residues are the value; `A` is not stored. The blocks need not be prime powers
for ring arithmetic; operations that name a prime need certified prime powers. Every block fits a machine word; a
value whose radius has a larger block is stored in the global backend as a whole. The modulus data (blocks,
reduction constants, recombination tree) live in an immutable context that values refer to. **[design]** (D2)

**The denominator in the local backend.** The residues are those of the integer numerator `A`; `d` is stored once
per value and applied only after recombination, as the denominator of `(A + H Zhat)/d`. The congruence
`d x = A mod q_i` has a solution exactly when `gcd(d, q_i)` divides `A`, and then `gcd(d, q_i)` of them; the ball
determines a residue modulo `q_i` exactly when `gcd(d, q_i) = 1`. So `d` is inverted modulo a block only when it is
coprime to it; a solution of `d x = A mod q_i` is never stored or reported as a residue of the value. **[proved]**
(`proofs/policies.md` Proposition 25, repaired after review.)

**Raw storage and canonical form.** A result of an operation in a context is first a *raw* representation; the
canonical triple of the paragraph above is computed from it by cancellation. The two can need different moduli
**[proved]** (`proofs/policies.md` Proposition 24 and Summary 26):

- negation, sum and the blockwise product keep `H` in their raw form (the sum changes `d` to `lcm(d, e)`); the
  tight product keeps the context exactly when `h = gcd(A, B, H)` divides `d e`, and otherwise lives in the derived
  blocks `q_i h_i`;
- canonical cancellation by `g = gcd(A, H, d) > 1` leaves the *set* representable in the original context, but the
  canonical *triple* has numerator modulus `H/g` and needs the derived blocks `q_i/g_i`. So the canonical `H` can
  change after any operation, a sum included: `(1 + 2 Zhat)/2 + (1 + 2 Zhat)/2` has raw form `(2 + 2 Zhat)/2` and
  canonical form `(0 + 1 Zhat)/1`;
- every derived context is computed without factorisation; a derived block above one machine word forces the global
  backend.

A backend that keeps raw data may leave a value in its context; a backend whose invariant is canonical data must
move it to the global backend (P24.4). The local backend keeps **raw** data: its values need not satisfy
`gcd(A, H, d) = 1`, and the canonical form above is the invariant of the global backend and of the canonical triple
that equality and printing compute (`conventions.md` 5.3, CV-55, accepted by the milestone 0 gate review).
In version 1 no operation creates a context: contexts are owned by the caller and made with the explicit
constructors of section 10.4 (`conventions.md` CV-10, CV-11). The implicit global fallback applies only to
`adf_fball` and to types containing it (the adele types): a result of theirs whose raw form needs other blocks, or
that combines two different context pointers, is returned in the global backend, or in a derived context that the
caller passes. It does not apply to `adf_scaled`; the shared-context rule for scaled arithmetic is in 4.4. Equality
and printing always go through the canonical triple, never through raw residues. **[design]** (D2)

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
**[proved]** (`proofs/policies.md` Theorem 3: every policy result contains the tight result and the true value.)

1. **Tight.** Each value carries its own radius; section 4.3 is applied exactly.
2. **Scaled residue.** A modulus context holds one integer `K >= 1`. A value is `s (u + K Zhat)` with a positive
   rational scale `s` and an integer `0 <= u < K`. Its radius is `s K`, not `K`. With scales `s`, `t`, put
   `g = gcd(s, t)`, `A = s/g`, `B = t/g` (integers):

       sum:      scale g,   residue (A u + B v) mod K      (tight)
       product:  scale s t, residue (u v) mod K            (encloses the tight radius s t K gcd(u, v, K))

   **[proved]** (D1) **[checked]** (20000 random cases in the review; repeated in `proto/`). Conversion from a tight
   ball can lose precision and says so. The rules deferred in version 1.0 are now proved **[proved]**
   (`proofs/policies.md`):

   - Exact values (0 included) keep their exact tag; they are not scaled values (Definition 4). The stored scaled
     value therefore has an exact case (PLAN section 4; `conventions.md` CV-14, proposed).
   - Best scaled enclosure of a ball `c + R Zhat`: scale `gcd(c, R/K)` (Proposition 7). Addition of an exact
     rational `q != 0`: scale `gcd(s, q)`, exact when `s` divides `q` (Proposition 8). Product with an exact
     rational: exact (Proposition 9).
   - Conversion to a context `K'`: scale `s gcd(u, K/K')`, exact when `u K'/K` is an integer, always when `K`
     divides `K'`; two contexts meet losslessly in `lcm(K, K')` (Proposition 11, Corollary 12).
   - The product rule above loses exactly the factor `h = gcd(u, v, K)`. The **tight scaled product**
     `(s t h) (((u v / h) mod K) + K Zhat)` equals the tight product and stays in the context (Proposition 10); it
     costs an integer gcd at the size of `K`. The rule above stays the default product; the tight scaled product is
     a separately named operation. **[design]** (M0-D5)
3. **Absolute cap.** A cap `C` is given; after a tight operation the radius `R` is replaced by `gcd(R, C)`, which is
   coarser or equal. The radius stays with each value. The cap never touches an exact value: an exact result of
   exact inputs keeps its exact tag (section 4.1), although the literal formula `gcd(0, C) = C` would widen it.
   **[design]** (M0-D2) **[proved]** (`proofs/policies.md` Propositions 14, 15: the cap is sound and is the best
   enclosure among radii dividing `C`; sums of capped values need no cap.)

What is *not* a policy: forcing every result to a fixed radius. `(1 mod 2) * (1/2)` is `1/2 mod 1`; writing it as
`1/2 mod 2` excludes `3/2` and is wrong. **[proved]** (D1) (Draft 1 proposed this; it is withdrawn. FLINT's `padic`
type keeps its precision in each value, not in its context. **[checked]**)

**Context compatibility for scaled values.** The implicit global fallback of 4.1 does not apply to the scaled
policy. A default binary operation on `adf_scaled` requires the same context pointer in both inputs; if the pointers
differ, the function returns `ADF_DOMAIN` with its value output untouched. This check precedes every write, an
aliased write included. The result borrows that input context, so the caller keeps it alive as long as the result.
Exact operands follow the same context rule. To combine different contexts the caller constructs a context with
modulus `lcm(K, K')`, converts both operands to it without loss (Proposition 11, Corollary 12) and then calls the
ordinary operation. No operation creates this context. A separately named target-context operation may take an
explicit caller-owned context; its conversion loss and output context must then be documented. The
shared-pointer check also applies to `adf_scaled_mul_tight`, including exact operands. It compares the two scaled
inputs, not the old context of the initialized output. Unary and exact-scalar operations borrow the scaled input's
context; an `adf_rat` scalar has no context to compare. **[design]** (G1, closure E1)

### 4.5 What the additive types cannot do

Every finite ball with positive radius contains non-invertible elements: choose a prime `p` outside the numerators
and denominators of `a` and `N`; there the coordinate ranges over all of `Z_p`, which contains 0. So a finite ball
can never certify that a divisor is invertible, and **division by an `adf_adele` is not defined**. One divides by an
exact non-zero rational or by an idele (section 5). Note also that having no zero coordinate is not enough to be
invertible: the adele with coordinate `p` at each prime `p` has none, and its inverse is not an adele. **[proved]**
(M4; `proofs/ideles.md` Lemma 2, Proposition 17)

## 5. Ideles and idele classes

**Decomposition.** Every finite idele is uniquely `r u` with `r` a positive rational and `u` a unit of `Zhat`.
**[proved]** (M5; `proofs/ideles.md` Proposition 3) **[quoted]** (`milne-cft:CFT.txt:9853-9858`) The scale `r` is
exact and is part of the precision. (Its accessor is called the *content*, R5 of section 3.)

**Unit precision.** For an integer `N >= 1` let `U(N)` be the units of `Zhat` that are 1 modulo `N`. A unit to finite
precision is a coset `c U(N)`, `gcd(c, N) = 1`, where `c` stands for any unit of `Zhat` with that residue (the
integer `c` itself need not be a unit of `Zhat`). This is not the additive ball `c + N Zhat`. Every unit is 1 modulo
2, so `U(2N) = U(N)` for odd `N`: `5 mod 6` and `2 mod 3` are the same coset. The canonical form removes the factor 2
from a modulus that is exactly twice an odd number; equality of cosets compares the sets. **[proved]** (N12;
`proofs/ideles.md` Proposition 6, Lemma 7, Proposition 9)

**Exact units.** No coset with `N >= 1` is a single point, and there is no smallest coset containing `{1}`
(`proofs/ideles.md` Proposition 13.6). The unit coset type therefore has an exact case: **modulus 0 means the exact
unit `+1` or `-1`** (`U(0) = {1}`, "congruent modulo 0" being equality). It is the unit of an exact rational
(`sign(q)`), the result of the exponent 0, and the unit of the exact idele 1. The product rule below holds unchanged
with `gcd(0, N') = N'`. **[design]** (M0-D1; `conventions.md` CV-15, decided with M0-D1)

| Type | Data | Product |
|---|---|---|
| `adf_ucoset` | `c mod N`, `gcd(c, N) = 1`, `N >= 1`; or `N = 0` and `c = +1` or `-1` (exact) | `c c' mod gcd(N, N')` |
| `adf_idele` | real ball `x_inf` excluding 0; `r`; unit coset | componentwise |
| `adf_idclass` (element of `A^x / Q^x`) | positive real ball `t`; unit coset | componentwise |

At a common `N`, product and inverse keep the finite precision exactly. At different moduli the product is known
modulo `gcd(N, M)`, so the finer input's precision is lost; `gcd(N, M)` is the best modulus for a product of
independent cosets, up to the factor 2 of `U(2N) = U(N)`. The real ball is rounded as usual. **[proved]** (M5;
`proofs/ideles.md` Propositions 10, 11)

**Sign preservation.** Idele and idele-class arithmetic validates the required real sign on its result before it
commits the result. A kernel may use sign-preserving endpoint bounds and a suitable enclosing midpoint-radius ball.
If it cannot produce a finite ball with the required sign, it returns `ADF_NOT_DETERMINED` and leaves the value
output untouched. Failure to preserve a sign under an enclosure is never `ADF_NOT_UNIT`. The rule applies to
product, inverse, integer powers and the idele-to-class conversion. The ordinary rounded product can contain zero
even when the exact product is positive: with `x = y = 1 +/- (1 - 2^-30)`, `arb_mul` at precision 128 can return a
ball containing zero, while the exact product set is positive with lower endpoint `2^-60`. **[design]** (G5)

**Integer powers of a unit coset.** The default enclosure of `(c U(N))^k` is `c^k U(N)` (inverse for `k < 0`); it
contains every `u^k`, and it is the smallest coset exactly when the tight modulus `M_k` below equals the canonical
`N` (always for `k = 1, -1`). The smallest coset is `chat^k U(M_k)`, with `M_k` given prime by prime in
`proofs/ideles.md` Proposition 13 (for example every square of a unit lies in `U(24)`); it is offered as a
separately named operation. The exponent 0 gives the exact unit 1. **[proved]** (`proofs/ideles.md` Proposition
13) **[design]** (M0-D6, M0-D1)

**Operations.** Absolute value at each place, `|x|_p = p^(-v_p(r))`; the norm `|x| = |x_inf| / r`; the product
formula `|q| = 1` for rational `q`. The class of an idele has coordinates `t = |x_inf| / r` and
`u' = sign(x_inf) u`; the sign on the unit is essential. The idele class group of `Q` is `R_{>0} x Zhat^x`.
**[proved]** (M5; `proofs/ideles.md` Propositions 14, 15)

**Idele to adele.** The map returns an additive ball that contains the unit coset; it is an enclosure, not an exact
conversion. Two versions: the simple ball `r c mod r N`, and the smallest ball, of radius `r lcm(N, 2)` with an odd
representative of `c`. **[proved]** (M5; `proofs/ideles.md` Proposition 16) For an exact unit (`N = 0`) the finite
part is the exact rational `r c`.

**Division of an adele by an idele.** For an adelic ball `I x (a + M Zhat)`, `M >= 0`, and an idele
`(Y_inf, r, c U(N))`, let `L = lcm(N, 2)` and `e` an odd integer with `e = c^(-1)` modulo `N`. The finite part of
the quotient is the smallest ball containing all quotients:

    (a e)/r + (gcd(|a| L, M) / r) Zhat

(for `a = 0`: `(M/r) Zhat`; for `M = 0`: `a e/r + (|a| L/r) Zhat`). It is the product rule of section 4.3 applied to
`a + M Zhat`, the smallest ball `e + L Zhat` of the inverse coset and the exact `1/r`. The simple ball of the
inverse gives the radius `gcd(|a| N, M)/r`, up to a factor 2 coarser. The real part is `I / Y_inf`, rounded
outward. Division uses the smallest ball. **[proved]** (`proofs/ideles.md` Proposition 19, repaired after review)
**[design]** (M0-D7) Division by an exact non-zero rational `q` is exact on the finite part: `a/q + (M/|q|) Zhat`
(`proofs/ideles.md` Proposition 18).

**Characters of the class group.** The continuous quasi-characters are `t^s chi(u')` with `s` complex and `chi` a
Dirichlet character; they are unitary when `Re(s) = 0`. A character carries its conductor. Its value on a unit coset
`c U(N)` is one number only when the conductor divides `N` (or `N = 0`, an exact unit); otherwise the function
returns an enclosure of the possible values or a status. **[proved]** (M5) The type stores `(chi, s)` and means
`t^s chi(u')`, with no hidden conjugation (`conventions.md` 5.13, CV-58, proposed). The idele class character used
in section 8 for `L(s, chi)` is a different member of this family: `omega_chi = conj(chi(u'))`, that is, the stored
character of `conj(chi)` with `s = 0` (the variable `s` of the integral being separate); its value on a uniformizer
at a prime `p` not dividing the conductor is `chi(p)`. **[proved]** (`proofs/analysis.md` Proposition 11)

## 6. The quotient by Q and the additive character

**Reduction modulo `Q`.** An exact point has a unique representative in `[0,1) x Zhat`; the boundary is glued by
`(1, z) ~ (0, z - 1)`. **[proved]** (M6; `proofs/quotient.md` Lemma 1, Propositions 2, 3) For a ball this is an
operation on sets:

- A real ball that crosses an integer after the shift wraps around and gives two pieces with different finite parts.
  Example: real part `[0.9, 1.1]`, finite part `0 mod 2` gives one piece near 1 with `0 mod 2` and one near 0 with
  `-1 mod 2`.
- A fractional radius `N = A/B` (lowest terms) splits into the `B` balls `a + k N + A Zhat`, `0 <= k < B`, each of
  integer radius. A fractional radius is therefore not an obstacle, only a multiplication of cases; `B` is the
  least number of balls of integer radius that cover it. **[proved]** (`proofs/quotient.md` Proposition 8)

**Piece count.** Splitting at the `k` integers strictly inside the shifted real interval gives `k + 1` closed pieces
with the gluing rule; a one-point interval gives one piece. This counts the construction, not a minimum: pieces with
the same finite part can merge afterwards (with `N = 1` and real part `[0, 2]` the two pieces together are the whole
quotient). The image is all of `A/Q` exactly when the real width is at least `N`. **[proved]** (`proofs/quotient.md`
Propositions 5, 6, 7) **[design]** (M0-D4)

**Closed pieces and rounding.** A closed real ball cannot describe the half-open interval `[0,1)`; pieces are
therefore closed real balls together with the gluing rule: the point `(1 ; z)` is the point `(0 ; z - 1)`. The exact
pieces lie inside `[0,1]`, but a piece is stored as an `arb` enclosure, and the enclosure of an interval with an end
point that is not dyadic can exceed `[0,1]` by the rounding of the enclosure (probed: the enclosure of `[0.9, 1]`
exceeds 1 at precisions 20, 53 and 128, `conventions.md` finding F2). Pieces are therefore closed real balls that
contain the exact piece and may exceed `[0,1]` by that rounding; the invariant is that the midpoint of the real
part lies in `[0,1]` (`conventions.md` 5.10, CV-45, decided with M0-D4). **[design]** (M0-D4) Equality of two
reduced sets is decided on the canonical form of a union of pieces, exactly when the end points are exact (for balls
the comparison may be undecided), and translation by a rational does not change the pieces. **[proved]**
(`proofs/quotient.md` Propositions 9, 10)

The result type `adf_qclass` is either an unreduced lift with the meaning "modulo `Q`" (always available, always
exact as a set), or a finite union of pieces with the gluing rule. A function that may return only one piece
returns the status `NEEDS_SPLIT` otherwise. A limit on the number of pieces is an argument. (M6)

**The additive character.** The convention attributed to Tate's thesis, section 2.2; the attribution
**[unverified]** [source pending: a lawful copy of Tate's thesis, to pin the section number and Tate's own signs]:

    psi(x)  =  exp( 2 pi i ( - x_inf + sum over p of {x_p}_p ) )
    Fourier transform:  hat f(y) = integral of f(x) conj(psi(x y)) dx

Here `{x_p}_p` is the p-primary fractional part. `psi` is 1 on `Q`. The real Fourier kernel is `exp(+2 pi i x y)`
and the finite kernel is `exp(-2 pi i x y)`; changing only one of the two signs breaks triviality on `Q`.
**[quoted]** The character is exactly the standard `psi` of the exposition on disk: `e^(-2 pi i x)` at the real
place (`tate-poonen:notes.txt:693`) and `psi(1/p^n) = e^(2 pi i/p^n)` with `psi = 1` on `Z_p`
(`tate-poonen:notes.txt:700`); triviality on the rationals: "trivial on k" (`tate-kudla:kudla-1.txt:903`). The
transform carries the conjugate, which is Tate's original convention: "Tate originally took the complex conjugate of
the additive character, but many references since then have not done so" (`tate-poonen:notes.txt:740`). **Both
expositions on disk define the transform without the conjugate** (`tate-poonen:notes.txt:736-737`,
`tate-kudla:kudla-1.txt:706`). This project keeps the conjugate; formulas taken from those expositions must have the
sign of the kernel changed before use. The kernel signs above follow from `psi` by our own calculation
(`proofs/analysis.md` Definition 1, Lemma 2). **[design]** (M0-D11)

On a ball: for an integer radius the finite phase is `exp(2 pi i a)`. For a fractional radius `A/B` the value is not
determined: the possible phases are that one times all `B`-th roots of unity (`0 mod 1/2` gives `+1` and `-1`). The
function returns an enclosure of all possible values, or a status. **[proved]** (M7; `proofs/analysis.md` Lemma 2)

## 7. Functions, Fourier transform

Version 1 implements a stated subclass of the test functions on `A`, not all of them. **[design]** (M9)

**Finite part.** Positive integers `D`, `M`; the function is zero outside `(1/D) Zhat` and constant on the cosets of
`M Zhat`; stored as the array `f_j = f(j/D)`, `0 <= j < L = D M`. Haar measure gives `Z_p` volume 1, so `N Zhat` has
volume `1/N`. Then **[proved]** (M8; `proofs/analysis.md` Proposition 4) **[checked]**

    hat f(k/M)  =  (1/M) * sum_{j=0}^{L-1} f_j exp(-2 pi i j k / L)
    integral of f  =  (1/M) * sum_j f_j
    norm squared   =  (1/M) * sum_j |f_j|^2  =  (1/D) * sum_k |hat f_k|^2

The transform is zero outside `(1/M) Zhat` and constant modulo `D Zhat`; the second transform has the factor `1/D`
and is the reflection `f(-x)`. With Lebesgue measure at infinity, `A/Q` has volume 1 and Poisson summation holds with
no further constant. **[proved]** (`proofs/analysis.md` Lemma 2, Proposition 3, Proposition 7)

**Real part.** Finite sums of `P(x) exp(-pi A x^2 + B x + C)` with `P` a polynomial and `Re(A) > 0`. This family is
closed under product, translation, dilation and Fourier transform. (A polynomial times one fixed Gaussian, as in
draft 1, is not closed under translation.) **[proved]** (M9; `proofs/analysis.md` Proposition 5) Uncertain
parameters are kept as parameters with balls, not folded into the coefficients.

**Operations.** Evaluation at an adele (on a ball that crosses a jump of the finite part: an enclosure of all
values); sum and product (after bringing `D`, `M` to common values, whose cost is charged); translation by exact
values and dilation by exact non-zero rationals or certified ideles (dilation by 0 leaves the class), or by balls
precise enough to fix the array operation uniquely; `D_a f(x) = f(a x)`, with
`hat(D_a f)(y) = |a|^(-1) hat f(y/a)`; Fourier transform; Haar integral; Poisson summation with certified tails.

## 8. Tate integrals: the acceptance test

    Z(f, omega, s)  =  integral over the ideles of  f(x) omega(x) |x|^s d^x x

with `d^x x = dx/|x|` at infinity and, at each prime, the measure that gives `Z_p^x` volume 1.

- **Zeta.** `f = exp(-pi x^2)` times the indicator of `Zhat`, `omega` trivial, `Re(s) > 1`: the value is
  `pi^(-s/2) Gamma(s/2) zeta(s)`. **[proved]** (M10; `proofs/analysis.md` Propositions 9 to 11) **[quoted]**
  (`tate-warwick:tatesthesis_notes.txt:512`, the example from line 493)
- **Dirichlet L-functions.** For a non-trivial primitive character `chi` of conductor `C` and parity `e` (conductor 1
  is the zeta case above): the same test function gives zero (at a ramified prime the character averages to zero
  over the units). The correct choice is, at ramified
  primes, the inverse local character on `Z_p^x`; at the other primes the indicator of `Z_p`; at infinity
  `x^e exp(-pi x^2)`. The result is `pi^(-(s+e)/2) Gamma((s+e)/2) L(s, chi)`; multiplied by `C^((s+e)/2)` it is the
  completed L-function. The idele character is `conj(chi(u'))`, so that the Euler factors carry `chi(p)`; in the
  notation of section 5 it is the quasi-character of `conj(chi)`, not of `chi`.
  **[proved]** (M10; `proofs/analysis.md` Propositions 9 to 11)
- **Outside `Re(s) > 1`** the defining integral is not absolutely convergent. A separate algorithm (splitting the
  integral and using Poisson summation, with proved tail bounds) computes the continuation and states its behaviour
  at the poles. **[quoted]** (convergence for exponent above 1, meromorphic continuation and the functional equation
  `Z(f, chi) = Z(hat f, chi^vee)`: `tate-poonen:notes.txt:1720-1725`;
  `zeta(f, chi, s) = zeta(hat f, chi^-1, 1 - s)`: `tate-warwick:tatesthesis_notes.txt:520`.) **[proved]** for the
  implemented class (`proofs/analysis.md`): the splitting at norm 1 continues `Z` meromorphically, with at most
  simple poles, residue `-f(0)` at `s = 0` and `hat f(0)` at `s = 1`, and only for conductor 1 (Proposition 12);
  with `W_chi = tau(chi)/(i^e sqrt(C))`, where `tau(chi)` is the Gauss sum with the positive finite kernel, the
  completed function satisfies `Lambda(s, chi) = W_chi Lambda(1 - s, conj(chi))`, `|W_chi| = 1`, and is entire for
  `C > 1` (Proposition 13); the tails and the quadrature have explicit bounds (Lemma 14, Proposition 15).

**Acceptance.** The integral path is independent of FLINT's L-function code; its result is compared with FLINT's
zeta and L values, completed by hand (FLINT's `xi` is normalised differently **[checked]**); the width of the result
must shrink as the precision grows; both parities and a non-real character are tested; points at and near the poles
are tested.

## 9. Solving equations, reconstruction, functions

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

- **From a full adelic ball.** For `N > 0`, `(a + N Zhat)` meets `Q` in the arithmetic progression `a + N Z`.
  Intersect it with the closed real interval `[lo, hi]` (end points included): the candidates are `a + N k` with
  `ceil((lo - a)/N) <= k <= floor((hi - a)/N)`. If the interval is shorter than `N` there is at most one rational;
  if it is at least `N` long there is at least one. For `N = 0` the only candidate is `a`, tested against the closed
  real interval. No lattice reduction is needed. **[proved]** (M12; `proofs/quotient.md` Proposition 11) The real
  interval is closed. **[design]** (M0-D3)
- **From partial data** (a residue modulo `m`, nothing known at other primes): the classical bounded problem: given `m > 0`
  and `c`, find a reduced fraction `n/d` with `gcd(d, m) = 1`, `n = c d mod m`, `|n| <= A`, `0 < d <= B`;
  `2 A B < m` is sufficient for uniqueness, and `2 A B = m` can admit two solutions. A separate function on a
  separate type. **[proved]** (`proofs/quotient.md` Proposition 13; uniqueness only, the algorithm is not proved)
  `1/5` is `5 mod 6` in this sense, and yet `1/5` is not in the adelic ball `5 + 6 Zhat`.
- Results: one verified candidate; none; several; or "uniqueness not certified".

This is not a local-to-global principle. `(x^2 - 13)(x^2 - 17)(x^2 - 221)` has a root in `R` and in every `Q_p`,
and no rational root. **[proved]** (M12; `proofs/quotient.md` Proposition 12)

### 9.3 Functions (in the basic package; TJO, 2026-09-27)

Functions of one variable are part of version 1. This section was rewritten after review round 2 (findings N1 to
N7, N10). The proofs are now written stepwise in `proofs/functions.md` (22 statements, cited below by number);
its review by a second model family (`reviews/m0-proofs/functions-review.md`) found 20 valid, 2 minor, 0 invalid,
and no counterexample to a formula of this section; the two minor repairs (Propositions 8 and 12) are applied.
Proposition 7b (a tighter term count for `log`) and Remark 15r (odd-degree roots at 2) were added in the repair and
have had no second reader yet; this section does not rest on them (the guard of 9.3.3 is unchanged).

**Notation.** `v = v_p` is the valuation at `p`. Put `c = 1` for odd `p` and `c = 2` for `p = 2`. Every non-zero
`x` in `Q_p` is uniquely `p^m w u` with `m` an integer, `w` a root of unity (`w^(p-1) = 1` for odd `p`; `w = 1` or
`-1` for `p = 2`, fixed by `x / 2^m` modulo 4), and `u` in `1 + p^c Z_p`. At `p = 2` the factor `w` is not the
Teichmüller representative, which is 1 for every odd unit. **[proved]** (N1; `proofs/functions.md` Proposition 4)

#### 9.3.1 How functions are applied

**[design]** (N2) Three forms, with different types:

| Form | Meaning | Result |
|---|---|---|
| `f_at(x, S)`, `S` a finite set of places, mandatory | `f` at each place of `S` | a partial ball over `S` (for one place: a local or real ball). The other coordinates of `x` are not part of the result |
| `project(x, S)` then `f` | the same in two steps | the same |
| `f(x)` with no places | `f` at all places at once | an adele or idele; requires that the whole input is certified to lie in the domain, otherwise a status |

There is no default set of places. A partial ball carries, for its archimedean place, the tag real or complex. If
one place fails, the function returns the status with that place and no value; an optional variant returns a
per-place map of results and statuses.

Every function is an enclosure of the image of the whole input ball. Domain checks inspect the whole ball: a stored
centre 0 with finite precision is not the exact 0. A requested output precision is an argument; an exact input does
not give an exactly representable output.

#### 9.3.2 Power series at a prime

| Function | Converges exactly on | Radius of the result for an input ball `a + p^N Z_p` inside the domain |
|---|---|---|
| `exp`, `sin`, `sinh` | `p^c Z_p` | `p^N`; these maps preserve distances |
| `cos`, `cosh` | `p^c Z_p` | `p^N` is safe; not tight (on `p^N Z_p` the smallest ball is `1 + p^(2N - v_p(2)) Z_p`) |
| `log` (series) | `1 + p Z_p`, for every `p` including 2 | `p^N` is safe. It preserves distances on `1 + p^c Z_p`. On `1 + 2 Z_2` it is not injective: `log(-1) = 0` |
| `Log` (Iwasawa: `Log(p^m w u) = log u`) | all non-zero `x` | for a ball not containing 0, `m = v(a)`, `N > m`: the image is `Log(a) + p^(N-m) Z_p` (at `p = 2` for `N - m >= 2`; for `N - m = 1` the image is `4 Z_2`). Absolute precision drops by `m` digits when `m > 0` |

**[proved]** (N1, N3; `proofs/functions.md` Propositions 6, 7, 10, 11) **[checked]** (`cos 4 = 9 mod 16` at 2;
`v_3(cos 3 - 1) = 2`; `v_2(log 3) = 2`.) The rule "keep the input radius" is the default safe policy, not a claim of
tightness.

**On all primes at once.** The common domain of `exp`, `sin`, `cos`, `sinh`, `cosh` in the finite adeles is

    D  =  4 Z_2  x  product over odd p of  p Z_p.

It is not empty (it contains the adele with coordinate 4 at 2 and `p` at each odd `p`), and on it the functions
give adeles. They are partial functions on `A`, with domain `R x D`. But `D` contains no finite ball of positive
radius, and the only rational in it is 0. So with our types the all-places form applies only to a value whose
finite part is exactly 0 (any real part). This is partly a limit of finite data, not a statement that the functions
do not exist. **[proved]** (N1; `proofs/functions.md` Proposition 12) For everything else, use `f_at`.

`Log` at all places: the finite image of any idele lies in `4 Zhat`, which is the conservative result; it is
refined at finitely many named primes by the table above. The real coordinate needs a positive input, or the
separately named `log_abs`. **[proved]** (N5; `proofs/functions.md` Proposition 12)

**Implementation.** FLINT's `padic` has `exp`, `log`, `sqrt`, Teichmüller lift and integer powers
**[checked]**; it has no `sin`, `cos`, `sinh`, `cosh`, no n-th roots, no general powers. It evaluates at a centre to
the precision of the output variable: it does not propagate the uncertainty of a ball, its `log` accepts a smaller
domain than the series has, and its equality ignores precision **[checked]** (probe in the review: at precision 8,
`exp 3` and `exp 12` differ at the 3-adic digit 2, although 3 and 12 lie in the same ball of radius 9). Therefore
our wrapper owns: validation of the prime; exact and uncertain zero; domain certification for the whole ball;
output precision; removal of `p^m` and of `w` before calling FLINT; branches of roots. FLINT is called only for the
centre, after these checks. **[design]** (N4)

#### 9.3.3 Roots

Fix a degree `n >= 1` and `a = p^m w u` non-zero. **[proved]** (N5; `proofs/functions.md` Propositions 13, 14)

| Place | `a` has an n-th root exactly when |
|---|---|
| odd `p` | `n` divides `m`; `w` is an n-th power among the `(p-1)`-th roots of unity; `v(log u) >= 1 + v(n)` |
| `p = 2` | `n` divides `m`; `w` is an n-th power in `{1, -1}`; `v(log u) >= 2 + v(n)` |
| real | `n` odd: always, one real root. `n` even: `a >= 0`, and the non-negative root is returned |

Square roots: at odd `p`, `m` even and the unit part a square modulo `p`; at 2, `m` even and the unit part 1 modulo
8. (3 is a unit and not a square in `Q_2`; 9 is a square although the derivative `2x` is not a unit, so simple
Hensel lifting alone would miss it.) When roots exist there are `gcd(n, p-1)` of them at odd `p` and `gcd(n, 2)`
at 2.

**Branches.** There is no "positive" p-adic root. The function returns all roots with identifiers, or takes a seed
(a residue) that selects one. "Whatever the library returns" is not a branch.

**Precision.** If `b^n = a` and the input ball `a + p^N Z_p` satisfies `N - m >= c + v(n)`, the branch near `b` has
image exactly `b + p^(N - v(n) - (n-1) v(b)) Z_p`. Outside this guard the input is split or the status
`NOT_DETERMINED` is returned. Unit square roots lose nothing at odd `p` and one digit at 2; unit p-th roots lose one
digit. **[proved]** (`proofs/functions.md` Proposition 15)

**At all places.** A unit coset leaves every unit possible at the primes outside its modulus, and some of those
units are not squares. So an idele of finite precision can never be certified to have a square root at all places;
the all-places root of degree `n >= 2` returns `NOT_DETERMINED` for ideles (degree 1 is the identity). Use `root_at`.

For an exact rational the all-places root returns the **rational** root: for odd `n` the one real root, for even `n`
the non-negative one (optionally both), found by testing numerator and denominator for n-th powers; no
factorisation is needed. It does not list all adelic roots, and cannot: the rational 1 has the square roots `+1` or
`-1` chosen freely at every place, continuum many. All branches are listed only over a finite named set of places.
Nothing is lost for existence: a non-zero rational with an n-th root at every place has all valuations divisible by
`n` and the right sign, so it has a rational root. Exact 0 has the single root 0. **[proved]** (R4;
`proofs/functions.md` Proposition 16)

#### 9.3.4 Powers: four different operations

**[proved]** (N6; `proofs/functions.md` Propositions 17, 18) `exp(s Log x)` is not a power: it gives 1 for `x = p`.
The package offers separately:

1. **Integer powers**, by ring or idele arithmetic; negative exponents need an invertible base.
2. **Rational powers**, through the roots of 9.3.3 with their branches.
3. **Powers of principal units**: `u^s = exp(s log u)` for `u` in `1 + p^c Z_p` and `s` in `Z_p`; on all odd 2-adic
   units, `w^(s mod 2) exp(s log u)`. The uncertainty of `s` and of `log u` both enter.
4. **The quasi-character** `t^s chi(u')` on idele classes, `s` complex: the result is a complex ball, not an idele.

#### 9.3.5 Characters are not sine and cosine

The additive character `psi : A/Q -> C^x` (section 6) plays on `A/Q` the part that `exp(2 pi i x)` plays on the
circle `R/Z`. It is not an extension of the ordinary cosine: `psi` is 1 on every rational, so at the rational `1/4`
it is 1, while `cos(2 pi / 4) = 0`. It is offered under its own name, with named real and imaginary parts.
**[proved]** (N2; `proofs/functions.md` Proposition 20)

#### 9.3.6 Other functions of one variable, with their types

(N10) No blanket "everywhere". **[proved]** where a mathematical claim is made: no order on any `Q_p`, the
p-primary fractional part, typed values and projections (`proofs/functions.md` Propositions 19, 21, 22).

| Function | Domain | Result |
|---|---|---|
| absolute value | real, complex, local, idele | non-negative real ball (exact rational at a prime) |
| valuation at `p` | `Q_p`, adeles at a named prime | an integer or infinity; `NOT_DETERMINED` when the ball contains elements of different valuation |
| sign, floor, ceiling | real place only | `-1, 0, 1`; an integer; `NOT_DETERMINED` on a ball that crosses the jump. There is no sign or floor on `Q_p` (`Q_5` contains a square root of `-1`, so it has no order) |
| p-primary fractional part `{x}_p` | `Q_p` | a rational with denominator a power of `p`, in `[0,1)`; the complement `x - {x}_p` lies in `Z_p` and is in general not an ordinary integer |
| unit part, root-of-unity part, Teichmüller representative | non-zero local values; unit cosets | the factors `p^m`, `w`, `u` above |
| rational functions | denominator certified non-zero at the named places; at all places: exact non-zero rational or idele | |
| real and complex functions of `arb`/`acb` (Gamma, error function, Bessel, zeta, ...) | the archimedean place, through `f_at` | real or complex ball; on the complex place, branch cuts follow `acb` and a ball meeting a cut returns the enclosure `acb` gives, with a status |

#### 9.3.7 Functions special to arithmetic (catalogue, added after a survey on 2026-09-27)

These are natural on adeles and ideles. They are implemented as their types and algorithms arrive; their cost
depends on precision, conductor, support, and on any factorisation they need. (R5) Version 1.0 labelled the whole
catalogue **[standard]**, from memory. In version 1.1 each formula is either **[quoted]** from a source on disk
(`sources.md` 2.4, 2.5) or **[proved]** in `proofs/catalogue.md` (15 statements; review
`reviews/m0-proofs/catalogue-review.md`: 11 valid, 4 minor, 0 invalid, the four minor repairs applied) or in
`proofs/analysis.md`; the column "Source" says which. **[checked]** in `proto/precision_rules.py` and
`proto/catalogue_checks.py`: the Hilbert symbol formulas (product formula on 2000 pairs of rationals; agreement with
solvability modulo a prime power at 2, 3, 5), the criterion for the profinite power, the binomial enclosure.

**Tier A: in version 1.**

| Function | On | What it is | Precision needed | Source |
|---|---|---|---|---|
| Legendre symbol `(a/p)`, Jacobi symbol `(a/b)` | `p` an odd prime; `b` positive and odd; `a` an integer or a unit coset | quadratic residue symbols (FLINT has them) | `a` modulo the lower entry | **[quoted]** `flint-3.0.1:ulong_extras.rst:456-458`; **[proved]** `catalogue.md` 1 to 3 |
| Kronecker symbol `(a/b)` | `b = 2^t m` positive, `m` odd | the Jacobi symbol modulo `m` times `(a/2)^t`, where `(a/2)` is 0 for even `a` and `(-1)^((a^2-1)/8)` for odd `a` | `a` modulo `m` if `t = 0`; modulo `lcm(m, 8)` if `t > 0` (sufficient, not always necessary). Not the residue modulo `b`: `(1/2) = +1`, `(3/2) = -1`. Negative or zero `b`: exact integers `a` only. Otherwise the set of possible values or `NOT_DETERMINED` (R1) | **[quoted]** `pari-doc:usersch3.tex:9266-9271`; **[proved]** `catalogue.md` 1 to 3 |
| Hilbert symbol `(a, b)_v` | two non-zero values at a place; two ideles | `+1` if `a x^2 + b y^2 = z^2` has a non-zero solution at `v`, else `-1`. For rationals the product over all places is 1 | the parity of the valuations and the square class of the unit parts: unit parts modulo `p` at odd `p`, modulo 8 at 2, signs at the real place. For a ball `a + p^A Z_p`: `A - v(a) >= 1` at odd `p`, `>= 3` at 2 suffices (2 does not: `1 + 4 Z_2` against 2 gives both signs). For an idele the unit part includes the cofactor of the scale; see the notes below (R2) | **[quoted]** `hilbert-bristol:lecture19.txt:7-9, 46-56, 89-91`; **[proved]** `catalogue.md` 4 to 8 |
| local zeta factor (trivial character) | a place and complex `s` | `(1 - p^(-s))^(-1)`; `pi^(-s/2) Gamma(s/2)` at the real place | complex ball. Poles at `s = 2 pi i k / log p`, and at `s = 0, -2, -4, ...` for the real place, all simple. An exact input at a proved nonremovable pole returns `ADF_DOMAIN`; an input ball that meets a pole and also contains regular points, or whose exclusion of poles is undecided, returns `ADF_NOT_DETERMINED`. Both leave the value output untouched and no non-finite ball is stored. `DOMAIN` keeps its definition in 3.1 | **[quoted]** `tate-poonen:notes.txt:1733` (finite), `:1014-1016` (real); **[proved]** `catalogue.md` 9, `analysis.md` 10 |
| Gauss sums | a Dirichlet character with its conductor, extended by 0; the additive character of section 6 | needed for section 8; the root number of section 8 uses `tau(chi) = sum over a mod C of chi(a) exp(2 pi i a/C)`, the positive finite kernel | complex ball | **[quoted]** `tate-poonen:notes.txt:1053-1055` (local Gauss sum); **[proved]** `analysis.md` Lemma 8 |
| local constants | a quasi-character at a place and `s` | defined by the local functional equation `Z(hat f, chi^-1, 1-s) = gamma(s, chi) Z(f, chi, s)` with the transform and measure of sections 6 and 7; the epsilon factor in addition needs the normalisation of the L-factor. The gamma factors for the transform of section 6 are fixed at every prime and at the real place for both parities | complex ball | **[quoted]** `tate-poonen:notes.txt:933-937` (local functional equation; the gamma form is it rearranged); **[proved]** `analysis.md` Propositions 9, 10 |
| Haar volume of a finite ball | finite balls | `1/N` for radius `N > 0`; 0 for a point | exact | **[proved]** `catalogue.md` 10 (our measure convention, volume 1 for `Zhat`) |
| profinite power `a^x` | `a` a unit coset `c U(N)`, `x` a profinite integer `e mod M`, `N, M >= 1` | the power in the group of units of `Zhat`; negative exponents through inverses | the result is determined **modulo `N`** exactly when `c^M = 1 mod N`. Otherwise: `NOT_DETERMINED`, or the coarser coset `c^e U(D)` with `D = gcd(N, c^M - 1)`, the largest divisor of `N` at which it is determined (no factorisation). `D` is not the finest modulus overall. The finest modulus `F` (all output powers determined) is given prime by prime in `proofs/catalogue.md` Proposition 13; the returned modulus is `canon(F)`, and the centre of that coset is the CRT solution `r` of `r = c^e` modulo `p^(v_p(F))` for `p` dividing `N` and `r = 1` modulo `p^(v_p(F))` for the other primes, **not** `c^e` in general (`N = 5, c = 2, e = 2, M = 4` gives `F = 120`, `r = 49`); offered as a separately named operation. Exact integer exponent: always determined modulo `N`; exponent 0 gives the exact unit 1 of section 5 (modulus 0, printed `[1]`) (R3) | **[proved]** `catalogue.md` 11 to 13 |
| binomial coefficient `binom(x, k)` | `x` a profinite integer `a mod N`, `k` a non-negative integer | the polynomial `x (x-1) ... (x-k+1) / k!`, which maps `Zhat` to `Zhat` | conservative: modulo `N / gcd(N, k!)`. Smallest ball: centre `binom(a, k)`, radius `gcd` of `binom(a + N j, k) - binom(a, k)` for `j = 1, ..., k` (radius 0, the exact 1, for `k = 0`). **[proved]** (R3 review, section on binomials) **[checked]** | **[proved]** `catalogue.md` 14 |
| content of an idele | ideles | the positive rational `r` of section 5 (the fractional ideal) | exact | **[proved]** `catalogue.md` 10; **[quoted]** `milne-cft:CFT.txt:9853-9858` (decomposition) |
| theta series of a test function | an idele `x` | `sum over rational q of f(q x)`; Poisson summation gives `Theta_f(x) = |x|^(-1) Theta_{hat f}(1/x)` | with milestone 4 | **[proved]** `analysis.md` Proposition 7 |
| cyclotomic action (reciprocity map for `Q`) | idele classes | the class `(t, u')` acts on a root of unity `z` of order `n`; `t` acts trivially. Two functions, each named by its exponent (`conventions.md` 6.6, CV-53): `..._cyclo_exp_uinv`, `z -> z^(u'^(-1) mod n)` (Milne's global reciprocity map, the reciprocal of the canonical isomorphism), and `..._cyclo_exp_u`, `z -> z^(u' mod n)` (the canonical isomorphism) (M0-D10). Remark: version 1.0 called the first "arithmetic" and the second "geometric"; these names are not used in the interface [source pending: a source on disk that applies the names "arithmetic" and "geometric" to these two maps]. Test vector: the idele with `p` at the place `p` and 1 elsewhere has `u' = 1/p` away from `p`, so `_exp_uinv` gives `z -> z^p` for `n` prime to `p`, the Frobenius (on roots of order a power of `p` this idele acts trivially) | determined exactly when `U(N)` is inside `U(n)`, that is, when the canonical form of `n` divides the canonical form of `N` (`[2 mod 3]` determines the action on sixth roots) | **[quoted]** `milne-cft:CFT.txt:9883-9886` (both maps), `:9904-9908` and `:1307` (the test vector); **[proved]** `catalogue.md` 15 |

**Notes on the Hilbert symbol.** (R2)

- For two ideles with scales `r`, `s`: at an odd prime where both valuations are even the symbol is `+1`. The places
  that can matter are the real place, 2, and the primes at which `r` or `s` has odd valuation. Finding them may need
  the factorisation of the scales, or a supplied list.
- The component at `p` of an idele with scale `r` and unit coset `c U(N)` is `r u_p`: its valuation is `v_p(r)` and
  its unit part is `r' u_p` with the cofactor `r' = r p^(-v_p(r))`, which is part of the square class. At odd `p`
  the unit residue modulo `p` is `r' c mod p` if `p` divides the canonical `N`, and any unit residue otherwise; at 2
  the residue modulo 8 ranges over the odd residues congruent to `r' c` modulo `2^min(n, 3)`, `n = v_2` of the
  canonical `N`. Example: `r = s = 3` with unit exactly 1 gives `(3,3)_2 = -1`; unit parts without `r'` would give
  `(1,1)_2 = +1`. **[proved]** (`proofs/catalogue.md` Proposition 7, repaired after review)
- At each such place the function reports a sign only if it is the same for all values in the input; otherwise
  `NOT_DETERMINED`. The family over all places is determined exactly when every one of these is.
- Finite precision need not determine it: two ideles of scale 1 with unknown units at 2 allow `(1,1)_2 = +1` and
  `(3,3)_2 = -1`.
- The product formula holds for rationals. It does not hold for arbitrary pairs of ideles.
- The prototype tests the formula against solvability modulo `16`, `9`, `25`, for coefficients of valuation 0 or 1.
  That this finite test decides solvability in `Q_p` is proved in the review (R2) for such coefficients, and in
  `proofs/catalogue.md` Propositions 5, 6 (modulo Hensel's lemma, source pending there); higher powers of the same
  primes also suffice; other precisions, or coefficients that are not reduced, need their own justification.

**Tier B: later, named so that the interface leaves room.** Morita's p-adic Gamma function; the Artin-Hasse
exponential; p-adic polylogarithms; p-adic L-functions; Dwork's exponential (needs an extension of `Q_p`);
functions on `Q_p(i)` and other extensions (with number fields).

## 10. Constraints from the interaction plane (deferred)

**[design]** The plane itself is a separate document. From the first commit the core provides:

1. A C interface with no hidden global state; contexts are explicit and immutable; rules for ownership, aliasing of
   arguments, the state of outputs after a failure, and thread safety are written down.
2. Two text forms. The *value form* is canonical and does not depend on the backend. The *dump* is versioned, keeps
   backend and context, and writes real balls as exact dyadic numbers (`arb_dump_str`). A decimal `+/-` string is an
   enclosure on input, not a lossless form. A dump loader takes an array of caller-owned context bindings and its
   length. There is one binding per local-`adf_fball` or `adf_scaled` occurrence, in dump traversal order, nested
   pieces included. Every binding must match that occurrence's modulus and ordered blocks. Repeated occurrences may
   share a pointer. A validation/inspection function reports the number of occurrences and their context
   descriptors without constructing a value; the caller constructs any missing contexts before loading. Value
   fields and backend are restored exactly relative to those bindings. Loading with `identical()` also requires
   binding each occurrence to its original context pointer. The one-context loader stays a convenience for dumps
   whose occurrences all use that context. The loader validates the text completely before any FLINT load function
   sees it. FLINT documents that `arb_load_str` "Returns a nonzero value if *str* is not formatted
   correctly" (`flint-3.0.1:arb.rst:297-298`), but in FLINT 3.0.1 it aborts the process on some malformed strings
   and silently changes others **[checked]** (probe, `conventions.md` section 10.2 and finding F5). Fuzzing never
   passes raw text to it. **[design]** (M0-D9)
3. Status codes that distinguish: not certified to be a unit; proved not a unit; value not determined by the
   inputs, or a required result invariant not certified (a ball that meets a pole and also contains regular
   points, an undecided exclusion of poles, or a real sign on the result that the enclosure cannot certify, among
   other cases); needs splitting; several candidates; resource limit reached; parse error; domain error (an exact
   input at a proved nonremovable pole).
4. A public C interface that a Julia layer can call through `ccall` without C glue: every public operation is an
   exported function (inline fast paths only in addition); no variadic functions and no callbacks with closures;
   status as a plain `int`; value structs with a documented fixed layout and exported size functions; 64-bit
   platforms. The modulus context is an incomplete public struct, always used through a pointer. The library exports
   explicit constructors `adf_modctx_new_*`, each taking `adf_modctx_struct **out` as its first argument, and
   `adf_modctx_free(adf_modctx_struct *ctx)`. A successful constructor allocates and fully initializes an immutable
   context and writes its pointer to `*out`; on failure `*out` is untouched and no allocation is retained. The
   caller owns the context and frees it only after its borrowers are gone. Passing NULL to free does nothing. A
   constructor never frees, mutates or reuses the context previously pointed to by `*out`. On success it
   overwrites only the pointer slot; the caller must retain any previous owned pointer separately. Value init
   functions stay non-failing and require
   successfully constructed contexts where stated. `get_str` and `dump_str` take `size_t *len` as an output and
   return an allocated `char *`; the `len` bytes are ASCII with no embedded NUL, a terminating NUL is stored at
   `s[len]` and is not counted in `len`, and the caller frees the pointer with `adf_str_free`. `adf_text_classify`
   returns an `ADF` status and writes an `adf_text_kind` enum through an output pointer only on `OK`; the enum is
   defined in the public header with one named constant per supported start symbol. Set predicates return `int` 0 or
   1, not a status. The rules are in `conventions.md` section 12 (draft). **[design]** (M0-D12, TJO, 2026-09-27; G2,
   G14)

## 11. Performance discipline

**[design]** A measured time or size is reported as a ratio to a derived lower bound, and only when the measurement
and the bound are of the same kind (a chain against a latency bound, a batch against a throughput bound) and for the
same stated problem. The method is the skill `perf-bounds`. Floors and reference measurements: `PERF.md`, which
keeps model floors per hardware profile (since version 1.1: the AMD Zen 2 machine of the baseline, and the Intel
laptop on which development and the harness run); a measurement is compared only with the floors of its own
profile.

## 12. Milestones

One map, shared with `PLAN.md`. **[design]** (P1)

| # | Content | Gate |
|---|---|---|
| 0 | Provisional build scaffold; sources on disk; proofs of all rules; conventions fixed; storage invariants; benchmark contracts; seams sketch. No public representation is frozen before the proofs, the conventions and the seams sketch are reviewed | reviewed by a second model family |
| 1 | Finite balls (tight, global), exact rationals, adeles, text forms, command-line driver; then scaled policy; then local backend | section 4 reproduced in C; policies against each other |
| 1F | Functions (section 9.3): archimedean wrappers; local `exp`, `log`; roots and powers with branches; `sin`, `cos`, `sinh`, `cosh`; partial balls; Tier A of 9.3.7 as its types arrive | independent oracles with stated output precision; the domain and precision cases of 9.3 |
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
  result. **[quoted]** (A2; "By default, most of the \kbd{bnf} routines depend on the correctness of the GRH" and
  "You must use \kbd{bnfcertify} to certify the computations unconditionally", `pari-doc:usersch3.tex:19478-19485`;
  `bnfcertify` removes the assumption, `:19556-19558`.)

## 14. Prior art

A directly relevant implementation is M. Hertogh's Sage package `adeles`, with the Leiden master's thesis "Computing
with adèles and idèles" (2021): adeles and ideles over number fields, multiplicative p-adic numbers, ray class
groups. Its documented rules for sum and product of profinite numbers agree with section 4.3, and it keeps separate
multiplicative precision for ideles. **[quoted]** (A1; the thesis is on disk: the sum and product are the
representations "with smallest represented subsets (with respect to inclusion)", with
`ab = v(a)v(b) mod gcd(v(a)m(b), v(b)m(a), m(a)m(b))`, `hertogh-thesis:thesis.txt:374-394`; multiplicative p-adics
carry their own precision and multiply with `min(p(a), p(b))`, `:849-866`; the package is pinned at commit
`1acd6362`, `refs/src/adeles-pkg/COMMIT`.) Its equality is an overlap test ("loose equivalence"), which "fails to be
transitive in general" and was chosen deliberately (`hertogh-thesis:thesis.txt:1525-1538`); we do not adopt that. We
have made no complete survey. A list of what other systems offer, from memory, is in
`riemann-channel/notes/adeles/software-and-algorithmic-scope.md`.

## 15. Decisions

### 15.1 Decisions of TJO, 2026-09-27

| Question | Decision |
|---|---|
| Name and licence | `adelefeld`; AGPL-3.0 |
| May a radius be a fraction? | Yes |
| Default modulus family | None imposed: user-chosen (arbitrary integer, list of coprime blocks, prime-power list, factorial, primorial power, exact) |
| Complex type | Kept as the complex archimedean adele `C x A_f` (section 4.1), as the reviewer recommends |
| Functions applied to adeles | Places are always named (`f_at`); no default place (section 9.3.1), as the reviewer recommends |
| Review round 2 | Apply all findings (draft 3) |
| Review of draft 1 | Apply all findings (this draft) |
| Elementary functions (`sin`, `cos`, `exp`, `log`, ...) | In the basic package (section 9.3, milestone 1F) |
| Folder and project name | `adelefeld` |
| Implementation language (M0-D12) | The library is C. Python is for tests, reference oracles and proof checks only. A Julia layer will later be built on the C library, so the public C interface is kept friendly to Julia's `ccall` (sections 1, 10; `conventions.md` section 12) |

### 15.2 Decisions of 2026-09-27 on the findings of milestone 0 (orchestrator, open to reversal by TJO)

The orchestrator numbered these D1 to D11 (and TJO's decision above D12). They are written `M0-D1` to `M0-D12` in
this document because `D1` to `D4` are also the ids of findings of the first design review, cited since draft 2.
`docs/conventions.md` writes the same twelve decisions as D1 to D12; the correspondence is D1 = M0-D1, D2 = M0-D2,
and so on through D12 = M0-D12.

| Id | Question | Decision | Where |
|---|---|---|---|
| M0-D1 | Exact units | Unit cosets have an exact case: modulus 0 means the exact unit `+1` or `-1` | 5; `ideles.md` P13.6; `conventions.md` CV-15 |
| M0-D2 | Absolute cap on exact values | The cap never touches an exact value | 4.4; `policies.md` P14 |
| M0-D3 | Real interval in reconstruction | Intersect with the closed real interval (end points included) | 9.2; `quotient.md` P11 |
| M0-D4 | Pieces of a reduction modulo `Q` | `k` integers crossed give `k + 1` pieces; pieces are closed real balls that may exceed `[0,1]` by the rounding of the enclosure | 6; `quotient.md` P6; `conventions.md` CV-45 |
| M0-D5 | Scaled product | The rule of 4.4 stays the default; the tight variant is a separately named operation | 4.4; `policies.md` P10 |
| M0-D6 | Power of a unit coset | `c^k U(N)` is the default enclosure; the tight modulus `M_k` is a separately named operation | 5; `ideles.md` P13 |
| M0-D7 | Division of an adele by an idele | Uses the smallest ball | 5; `ideles.md` P19 |
| M0-D8 | Seams recommendations | R1 to R9 of `seams.md` section 5 are adopted | 3 |
| M0-D9 | Loading dumps | The loader validates the text completely before any FLINT load function sees it | 10 |
| M0-D10 | Reciprocity | Two functions named by their formula (`_exp_u`, `_exp_uinv`); the words "arithmetic" and "geometric" have no source on disk and appear only in a remark marked source pending | 9.3.7; `conventions.md` CV-53 |
| M0-D11 | Fourier convention | The convention of section 6 (with the conjugate) is kept; the expositions on disk define the transform without the conjugate, and this is said | 6 |
| M1-D1 | Language of the driver `adf` | One command per line: an operation name, then one to three value texts separated by the word ` with `, which no value text can contain (no printable byte lies outside the alphabet of `conventions.md` 8.2). Operations `show`, `type`, `add`, `sub`, `mul`, `neg`, `div`, `equal`, `contains`, `overlaps`, `reconstruct`, `cap`; settings `prec`, `digits`. Output: one line per command, a value text or `error: <STATUS>`. The driver refuses to print a real ball whose binary exponent exceeds 100000 in absolute value (`LIMIT`). Taken by the orchestrator on 2026-09-28; `tools/adf/README.md` | 10; PLAN 1.5 |
