# Slice 1F.5: roots at a prime

The contract is in `include/adelefeld/lroot.h` and the added declarations in `rfunc.h`.
The implementation is `src/lroot.c`, with named-place wrappers in `src/rfunc.c`.
The proofs used from `docs/proofs/functions.md` are Lemma 3:55, Proposition 4:92, Lemma 9:265,
Proposition 11:336, Proposition 13:410, Proposition 15:463 and Proposition 16:538.
This document adds statements R1 to R9 (R9 and the present R6 step 6 by lane f-repair5).
It does not change SPEC 9.3.3.
Decision N-D14 (`docs/SPEC.md` 15.4, last row; lane f-repair4) replaced the exponent rule of N-D13 for ball
inputs by `K = min(N, E)`; the statements below are written for that rule.

## Interface and decisions

| Operation | Result |
|---|---|
| `adf_lball_root_count` | Count after the whole-input existence test |
| `adf_lball_root_seed` | One explicitly identified branch |
| `adf_lball_sqrt_seed` | The seeded operation with degree 2 |
| `adf_lball_roots` | All branches in caller-owned arrays, sorted by identifier |
| `adf_sball_root_seed_at`, `sqrt_seed_at` | One branch as a partial ball over the named prime |
| `adf_sball_roots_at` | All local branches at the named prime |

1. The new functions live in `lroot.h/lroot.c`. Alternative: extending the series translation unit.
   A separate file keeps the existence test, finite-field enumeration and exact-root detection together.
2. At odd primes the identifier is the unit residue of the root modulo p. At 2 it is 1 or 3,
   the unit modulo 4, denoting torsion sign +1 or -1. Alternative: signed identifiers throughout.
   Unsigned identifiers cover primes above WORD_MAX and agree with decomposition indices.
   Identifier 0 is reserved for the sole exact-zero or degree-1 branch. Degree 1 ignores its seed.
3. Count is separate from enumeration; listing uses caller-owned initialized lballs and scalar arrays.
   Alternative: a new allocated root-list type. Caller-owned arrays need no new lifetime or layout API.
   Listing returns LIMIT if capacity is too small, leaving arrays and count untouched.
   That LIMIT is decided before any branch is listed (R6 step 6).
4. The principal-unit factor is exp(Log(unit)/n). Alternative: decompose_teich then log(u)/n.
   Proposition 11 and F3 of api-1f4.md identify both logarithms. Log saves the input torsion lift;
   the root's torsion factor still comes from teichmuller. No iteration with a nonunit derivative is used.
5. All-branch enumeration lists the d = gcd(n,p-1) roots of T^d - w^e as t0 zeta^i, zeta = g^((p-1)/d)
   for a primitive root g (R8). Until review f-review6 (F2) it factored that polynomial with Rabin's
   method (5.9 s at p = 65537, d = 65536); the listing takes 14 ms there. Alternative: searching every
   residue. ADF_LROOT_BRANCH_MAX = 2^26/64-1 now bounds the count d, that is the temporary identifier
   and lball arrays of the listing; no coefficient vector exists. This is not a time guarantee.
   Count and seeded evaluation can succeed when listing exceeds this limit.
6. A guarded ball input returns exponent K = min(N, E) (decision N-D14, as lfunc.h, F6 of api-1f4.md):
   the exact image when N >= E, the ball at N that contains it otherwise. An exact input uses N
   unless that branch is rational, in which case it is exact. Alternative (N-D13, until review
   f-review6): exponent E regardless of N. It let no request bound the cost of a ball root
   (51.8 s at N = 10 for a 2-adic ball of relative precision 10^5) and turned a result inside the
   limits into LIMIT (F1). A caller who wants the exact image passes N >= E.
7. Degree 1 returns the input, including uncertain zero and the coarse unit ball at 2.
   Other degrees keep the strong guard. Remark 15r is not implemented. Alternative: use its odd-degree
   refinement at 2. That would require a separate sign-changing branch convention on relative precision 1.
8. Existing seedless sball sqrt_at/root_at stay UNSUPPORTED at primes. New seeded functions select a branch.
   Alternative: changing the existing ABI or choosing a hidden branch. The latter contradicts SPEC 9.3.3.
   New root wrappers are prime-only; their real-place calls return UNSUPPORTED after checking membership.
9. Invalid seeds and degree 0 are DOMAIN as invalid arguments, not claims about existence in other branches.
   On valid guarded inputs, DOMAIN from the criterion proves that every input point has no root.
   Outside the guard the result is always NOT_DETERMINED, even if another obstruction could decide no.
10. Driver commands are `roots_at X with PRIME with DEGREE` and
    `root_at X with PRIME with DEGREE with SEED`. At 2 the driver accepts sign -1 as identifier 3.
    It prints every root with its identifier and forms the entire line before writing it.
11. (Lane f-repair5, 2026-10-03.) `roots` lifts two Teichmueller representatives for the whole list and
    multiplies (R9); `root_seed` lifts the one of its seed. Lane f-repair4 measured 149 us per branch for the lift
    at p = 2^64-59, N = 20 (44.6 s for d = 299756). Alternative: one lift per branch; rejected as an avoidable
    cost. The list is evaluated in the order t0 zeta^i of R8 and written in increasing order; the values are
    identical to those of one lift per branch (R9 (b)).

## R1 (constant existence test on a guarded ball)

Let x=a+p^M Z_p exclude zero, m=v_p(a), r=M-m, s=v_p(n), and c=1 at odd p, c=2 at 2.
For n>=2 and r>=c+s the three conditions of Proposition 13 are constant throughout x.
The finite test is n divides m, index^((p-1)/gcd(n,p-1))=1 at odd p (the sign test at 2),
and Log(a/p^m)=0 modulo p^(c+s).

Proof.

1. Every point is a(1+t), t in p^r Z_p. Its valuation is m and its torsion factor equals that of a,
   since 1+t is principal by r>=c. Thus the first two conditions do not change.
2. Proposition 11 gives Log(a(1+t))=Log(a)+log(1+t); Lemma 9 puts the second term in p^r Z_p.
   Since r>=c+s, membership of Log(a) in p^(c+s) Z_p is constant under this change.
3. By Lemma 3 the odd-prime torsion group is cyclic of order d0=p-1. Write its element as g^k.
   An n-th power exists iff gcd(n,d0) divides k. This is equivalent to raising the element to
   d0/gcd(n,d0) and obtaining 1. Reduction modulo p identifies this with the stated finite test.
4. At 2 the torsion group has order 2. For even n its image is {1}; for odd n it is the full group.
   The sign is the unit modulo 4. For squares, the additional Log condition is modulo 8:
   Proposition 11's isometry on 1+4 Z_2 identifies it with principal unit 1 modulo 8.
5. Log computed to c+s digits encloses the true logarithm. Its canonical residue is zero precisely
   when that logarithm lies in p^(c+s) Z_p. An exact zero also passes. No approximate equality is used.
6. Consequently success proves that every point has the stated number of roots; failure proves that none do.
   Outside the guard this argument is not applied. For uncertain zero no nonzero decomposition is used.
   Exact zero has one root by the field law. Degree 1 is the identity without a decomposition.

## R2 (the image of a branch, and the result ball)

On the guard, let j=m/n and b be the selected root of the centre. The image of the input under
the branch is exactly b+p^E Z_p, E=j+r-s=M-s-(n-1)j. The identifier selects its torsion factor
uniquely. The result is the ball of exponent K=min(N,E) that contains the image (N-D14): the
image itself when N>=E.

Proof.

1. Proposition 13 constructs b=p^j t exp(Log(a/p^m)/n), with t the selected torsion root.
   The criterion puts the divided logarithm in p^c Z_p.
2. Every input point is a(1+h), h in p^r Z_p. Its selected root is
   b exp(log(1+h)/n), by Proposition 15:479-485.
3. Log maps 1+p^r Z_p bijectively onto p^r Z_p. Division by n maps that onto p^(r-s) Z_p.
   Since r-s>=c, exp maps it bijectively onto 1+p^(r-s) Z_p.
   Multiplication by b gives exactly b+p^(j+r-s) Z_p, proving both inclusions.
4. Two different torsion roots have different residues modulo p, or modulo 4 at 2.
   Since E-j>=c, these balls are disjoint. Their count is gcd(n,p-1) or gcd(n,2).
5. The points b and b+p^E both belong to the image and differ by valuation exactly E.
   A ball of exponent E+1 cannot contain both. Negative j changes none of the bijections.
6. If N<E, the image b+p^E Z_p lies inside b+p^N Z_p, because E>N. A ball of exponent N is
   determined by any of its points, so this is the only ball of exponent N that contains the image.
   For N<=j it is p^N Z_p, since v(b)=j>=N.

## R3 (exact rational branches and large degrees)

For an exact nonzero x=p^m A/B with coprime signed A and positive B, the rational branches
are found by integer n-th roots of abs(A) and B, with n dividing m and the allowable sign.
The test detects every rational branch, including ones with negative valuation.

Proof.

1. Suppose a rational root is p^j C/D with coprime C,D and p dividing neither.
   Taking valuations gives nj=m. Equality of reduced unit fractions gives C^n/D^n=A/B.
   Coprimality forces abs(C)^n=abs(A), D^n=B. This also follows from Proposition 16:562-570.
2. Conversely those integer equalities produce a rational root. For odd n its sign is fixed by A.
   For even n, A must be positive and both signs are roots. The two signs have distinct branch identifiers.
3. Testing the resulting unit modulo p or 4 assigns the rational candidate to its unique branch.
   A different torsion branch need not be rational: at 7, x^3=8 has three branches but only root 2 is rational.
4. For a positive integer q>1, an integer root >=2 implies q>=2^n and bits(q)>=n+1.
   Hence n>=bits(q) rules out exactness without calling a signed-degree root API.
   q=1 has root 1 for every positive degree, including degrees above WORD_MAX.
5. FLINT's fmpz_root returns an exactness flag; its API is in
   refs/src/flint-3.0.1/fmpz.rst:983-988. Only positive magnitudes and signed-range degrees reach it.
   Unit roots remain coprime. Keeping the valuation separately avoids constructing p^abs(m).

## R4 (working precision and exact-input enclosures)

Let K=min(N,E) for ball inputs (N-D14) and K=N for an irrational exact-input branch. For K>j put
L=max(K-j,c). Log at L+s, exact division by n, exp at L, and multiplication by the
selected torsion factor at L determine the unit root modulo p^L. The result has exponent K.
The product z0=exp(Log(a/p^m)/n) does not depend on the branch; it is computed once for all
branches of one call, and each branch multiplies it by its torsion factor (R2 step 1).

Proof.

1. The stored unit centre is an exact rational unit representing the centre of x/p^m.
   Log ignores its torsion factor, so it gives the principal-unit logarithm of that centre.
   Evaluating at L+s loses no more than that precision, by lfunc.h and api-1f4.md F6.
2. Division by the exact n, of valuation s, changes the absolute precision to L.
   R1 proves every point of the divided-log enclosure is in p^c Z_p, since L>=c.
3. Exp is an isometry on that domain. The torsion factor is an exact unit or an enclosure
   of precision L. Their product encloses the desired unit root to L, by lball multiplication.
4. Reduction modulo p^(K-j) and scaling the valuation by j gives the canonical centre at K.
   For a ball input with N>=E, R2 shows this ball is the exact image, not just a safe enlargement;
   with N<E it is the ball at K=N that contains the image (R2 step 6), whose centre is b modulo
   p^K, b the root of the centre computed here. For an exact input it encloses the single
   selected point to the requested precision K. When the product is the exact 1 (unit part of
   the centre exactly +-1 and torsion factor 1), the centre is 1 and no power p^(K-j) is formed.
5. If K<=j the root lies in p^K Z_p, so the canonical ball is centred at zero, with exponent K.
   This shortcut needs no working power. Labels still distinguish roots when enclosures overlap.
   Under N-D14 it also serves a ball input with N<=j (R2 step 6).
6. R3 is tried before the requested N is tested. A rational exact branch ignores N, as does exact zero.
   A ball input is always a ball result even when its centre has a rational root.

## R5 (finite-field enumeration without a degree-n polynomial)

For odd p put d=gcd(n,p-1), h=(p-1)/d. Assume the torsion criterion w^h=1 holds.
If h>1 let e be the inverse of n/d modulo h; if h=1 let e=0. Then the roots of
T^d-w^e in F_p are exactly the n-th roots of w. They are d distinct nonzero residues.

Proof.

1. gcd(n/d,h)=1, so e exists. Write (n/d)e=1+kh. In the subgroup of order h,
   (w^e)^(n/d)=w. If h=1, w=1 and the same equality holds with w^e=1.
2. w^e lies in the order-h subgroup, which is exactly the image of the d-th power map
   on the cyclic group of order dh. Thus T^d=w^e has d solutions.
3. Each such solution has n-th power (w^e)^(n/d)=w. The original equation has d solutions
   by Lemma 3, so these are all its solutions.
4. Since d divides p-1, p does not divide d. The derivative d T^(d-1) is nonzero at each root.
   R8 lists the d roots without forming the polynomial. Sorting makes the order deterministic.
5. At 2 the list is the one prescribed sign for odd n or both signs for even n.
   Exact zero and degree 1 bypass finite-field enumeration and use identifier 0.

## R6 (limits, transactions and named places)

All exponent arithmetic and allocations are checked before the request can exceed the stated bounds.
Count, seeded roots and complete lists leave every output untouched on failure. The named-prime
wrappers apply the same operation to the entire projected component and retain only that place.

Proof.

1. Input |m| and |M| are tested against B=2^60 before taking M-m or abs(m).
   s<=63 and c+s<=65. For n>=2, |j|<=B/2. The formula E=j+(M-m)-s is safely within slong.
   No product (n-1)j is formed. Divisibility is computed with an unsigned abs(m).
   When m!=0 and n divides m, n<=abs(m)<=B, so the division by a signed n is safe.
2. K is checked before K-j. Both operands are bounded, so their difference cannot overflow.
   L+s is safe. No power is tested in lroot.c before Log: Log, div and exp (lfunc.h, lball.h),
   teichmuller, mul and unit_mod (lball.h) test the powers they form before they allocate.
   The exact unit +-1 forms none in Log and exp, and a result centre 1 forms none (R4 step 4;
   F1 of review f-review6). Exact rational branches skip those powers.
3. Modular word products are evaluated in fmpz, so p above WORD_MAX and n up to UWORD_MAX
   do not overflow. gcd uses unsigned words. The branch count is checked against
   ADF_LROOT_BRANCH_MAX before the list is allocated; allocations use that bounded count.
   R8 computes in words modulo p, p-1 and q^a: n_mulmod2_preinv takes every word operand
   (refs/src/flint-3.0.1/ulong_extras.rst:368-373); n_powmod2_ui_preinv takes a base below the
   modulus and every word exponent (:537-541), and every base passed is a residue modulo p.
4. Count uses temporary scalar outputs. A selected root uses temporary lballs; all branches are
   evaluated in temporary arrays before any caller array is touched. Step 6 decides the status
   before the list; were a branch to fail after others had been computed, the whole caller array
   would still be preserved. Reading x is finished before outputs are copied.
   This covers x aliasing any output slot, including an unused one. Overwritten output values are not checked.
5. The _at wrapper checks place membership first, copies the component, evaluates locally and builds
   a partial ball with arch NONE and one prime. That is Proposition 22:725 restricted to one place.
   The source may equal the destination. Failures preserve the destination and set optional where=v;
   success preserves where. The all-local-roots array is disjoint from the enclosing partial-ball storage.
6. Listing capacity is checked after the input criterion. Negative capacity is invalid (DOMAIN);
   insufficient nonnegative capacity or an excessive count is LIMIT. No partial list is published.
   Both are decided before the list. So is every status that the evaluation of the branches
   would return (`early_status`, rewritten by lane f-repair5 after review f-review7, finding 3).
   Let general be the number of branches that are not rational (R3).
   a. general=0, degree 1 or exact zero: every branch is copied, OK. Else if K is outside the
      exponent bound, every general branch returns LIMIT. Else if K<=j, every general branch
      is the zero ball at K (R4 step 5), OK. Else every general branch needs z0 (R4), computed
      once: its status, if not OK, is the status of every general branch (Log, div, exp).
   b. With z0 computed and K>j, a general branch of seed t forms the torsion factor omega(t)
      (exact +-1 for t=1, p-1 and at 2; else the ball at L, lball.h teichmuller), z=z0*omega(t),
      and, unless z is the exact 1, the unit of z modulo p^(K-j). The powers formed are p^L (the
      lift and the product, both of valuation 0 and exponent at most L) and p^(K-j)<=p^L
      (unit_mod), and z has relative precision >= L >= K-j (R4), so unit_mod is not
      NOT_DETERMINED. Hence: if p^L is within the bound (power_ok), the branch is OK; if not, then
      L>c (p^c is always within it), L=K-j, and unit_mod at K-j returns LIMIT (it tests the bound
      before anything else, lball.h), unless the lift or the product returned LIMIT before it.
      So a general branch returns LIMIT exactly when p^L is beyond the bound and z is not the
      exact 1.
   c. z is the exact 1 only if both factors are exact (a product with a ball is a ball): z0 is
      the exact w=+-1 and omega(t)=w^(-1)=w, which names one seed (1 for w=1; p-1, or 3 at 2,
      for w=-1). So at most one general branch has z=1, and the evaluation returns LIMIT
      exactly when general exceeds that number (0 or 1) and p^L is beyond the bound. This is
      the test of `early_status`; it returns the status of the evaluation, before the list.
   The rule before lane f-repair5 counted the seeds 1 and p-1 both as free of powers. It missed
   the centre p^(K-j)-1 of the branch p-1 (z=-1): 1+7^(2^40) Z_7, n=2, N=LONG_MAX, and
   1+2^(2^27) Z_2, n=2, N=LONG_MAX, returned LIMIT only after the branch 1 had been computed
   (review f-review7, finding 3; `lanes/f-review7/early.in` lines 8 to 13). The status was the
   same; only the order was not as stated here.
   The order is a cost property: `repair6_early_limit_cost` refuses 824329 branches at `2^64 - 59`
   within 0.02 CPU seconds; a valid list at `N = 40` took 4.131082 CPU seconds on this machine.

## R7 (what the exhaustive oracle proves at its stated precision)

The oracle imports only Python's standard library. It uses modular integer powering, not logarithms,
Teichmueller lifting, a p-adic library, or the C implementation's finite-field factoring routine.

1. For p=2,3,5,7, n=1..12 and every a modulo p^M, it buckets every b modulo p^H by b^n modulo p^M.
   M=1..5 at 2 and M=1..3 at odd primes. H=M+1. This examines every integral input ball in those grids,
   including nonunit centres and centred balls. Additional rows scale unit balls by p^(n*j), j=-1,1.
2. On a guarded rootable ball of the integral grid (j>=0), E=M-s-(n-1)j<=M. Since H>E, enumeration
   resolves a complete extra digit beyond the promised image radius. Grouping roots by identifier
   determines their unique centre modulo p^E. For j<0 the inequality fails (p=3, n=2,
   x=3^-2(1+3 Z_3): M=-1, j=-1, E=0; F4 of review f-review6). The rows scaled by p^(n*shift),
   shift=-1 or 1, are not enumerated: their E=shift+E0 and centres are derived from the unscaled
   row by the scaling x -> p^(n*shift) x, whose roots are p^shift times the unscaled roots.
   The oracle demands equality between the bucket and the union of all returned-format balls modulo p^H.
   For every branch it also demands both b and b+p^E, which are distinct modulo p^H.
3. A canonical ball at exponent E is determined by its residue modulo p^E. The C test compares that
   residue, exponent, identifier, count and exactness with the fixture, plus every permitted local alias.
   It requests N=ADF_LBALL_EXP_MAX>=E, so that the exact image is compared (N-D14); it also requests
   N=E, E-1 and E-3 and compares with the ball at that exponent that contains the image.
   The finite equality is not a substitute for the infinite proof; R2 supplies the latter.
4. Exact rational inputs use output N=4 and modular-root enumeration modulo p^(4+s).
   Each identifier must have one residue modulo p^4; larger powers of p in n are accounted for by s.
   The finite criterion and R1 certify liftability. Rational branches are detected independently by
   exhaustive small integer numerator/denominator powers; others must return a ball at N=4.
5. The fixture totals are 8110 ball rows and 1408 exact-input rows, 643421 bytes together.
   Ball enumeration examines 45708 residues and certifies 5062 distance witnesses.
   C also checks 2400 reproducible seeded ball inputs, fractional valuations, word-size degrees,
   and p=2^64-59 at requested precision 200. Each comparison states and checks its output precision.

## R8 (the branches as powers of a primitive root)

For odd p let d=gcd(n,p-1)>=2, h=(p-1)/d, P=p-1, and A=w^e as in R5, so A^h=1. Let g be the
primitive root returned by `n_primitive_root_prime` (refs/src/flint-3.0.1/ulong_extras.rst:1410-1413)
and zeta=g^h. Then (a) `dth_root` in src/lroot.c returns t0 with t0^d=A; (b) the roots of
T^d-A in F_p are t0 zeta^i, i=0..d-1, and they are distinct; (c) sorted, they are the identifiers
of R5 in increasing order. At 2, and for d=1 (the one root A), nothing changes.

Proof.

1. F_p^* is cyclic of order P (Lemma 3:55) and g generates it. zeta=g^h has order P/gcd(h,P)=d.
2. (b) (t0 zeta^i)^d=t0^d (zeta^d)^i=A. If t0 zeta^i=t0 zeta^k with 0<=i<k<d, then zeta^(k-i)=1
   with 0<k-i<d, against the order d. A polynomial of degree d over a field has at most d roots,
   so these d values are all the roots.
3. Let q run over the primes of d, a=v_q(P), b=v_q(d)<=a, Q=q^a; D is the product of the Q and
   R=P/D. Then gcd(D,R)=1 and d divides D. For R>1 put eps_R=D (D^(-1) mod R), and put
   eps_q=(P/Q)((P/Q)^(-1) mod Q). Each is 1 modulo its own modulus and 0 modulo the others,
   so their sum is 1 modulo R
   and every Q, hence modulo P (the moduli are pairwise coprime with product P). Since A^P=1,
   A=A_R prod_q A_q with A_R=A^eps_R and A_q=A^eps_q. For R=1 the component is trivial:
   set A_R=t_R=1, omit eps_R and both inverses modulo R; the remaining projectors sum to 1 modulo P.
4. For R>1, A_R^R=1, because eps_R R is a multiple of D R=P. gcd(d,R)=1;
   with d'=d^(-1) mod R, t_R=A_R^d' has t_R^d=A_R.
   The code forms A^(eps_R d' mod P), the same element; for R=1 it starts with t_R=1.
5. A_q^Q=1, so A_q lies in the cyclic subgroup of order Q, generated by gamma=g^(P/Q): A_q=gamma^k,
   0<=k<Q. R5 step 2 gives A=y^d for some y, so A_q=(y^eps_q)^d with y^eps_q=gamma^l in the same
   subgroup; then k=d l modulo Q, and q^b divides k because it divides d and Q.
6. Write k=sum c_i q^i, 0<=c_i<q; c_i=0 for i<b by step 5. zeta_q=gamma^(q^(a-1)) has order q.
   If k' is the sum of the digits below i, (A_q gamma^(-k'))^(q^(a-1-i)) = zeta_q^(c_i): the terms
   with digits above i have exponents divisible by q^a, the order of gamma. The values zeta_q^c,
   c<q, are distinct, so the search over c<q finds c_i. After the last digit k'=k.
7. d_q=d/q^b is prime to q. t_q=gamma^((k/q^b)(d_q^(-1) mod Q)) satisfies
   t_q^d=gamma^(k d_q^(-1) d_q)=gamma^k=A_q, the exponent being taken modulo the order Q.
8. t0=t_R prod_q t_q has t0^d=A_R prod_q A_q=A. The code checks t0^d=A before it lists; by steps
   3-7 the check holds. A failure would return NOT_DETERMINED and publish nothing.
9. (c) By (b) and R5 the set is the set of n-th roots of w that the polynomial route found;
   sorting the words gives the same list. The stored fixtures and a search over all residues
   (every odd p<110, n in 2..p-1 and 2(p-1), every unit w: 61324 lists) compare it.
10. Cost: factoring d (d<=ADF_LROOT_BRANCH_MAX<2^20) and p-1 (inside n_primitive_root_prime),
    the digit loop has at most sum over q of (a-b) q explicit products modulo p and two modular
    powerings per digit. Binary powering uses O(log P) word products per call
    (refs/src/flint-3.0.1/ulong_extras.rst:537-545); the integer power of q and its update also fit
    O(log P) products per digit. Thus the digit work is
    O(sum over q of (a-b)(q+log P)), plus setup, CRT reconstruction and the final power check.
    Enumeration adds d word products and a sort of d words. No exact FLINT multiplication count is claimed.
    Measured: 14 ms at p=65537, d=65536; 1.7 ms at p=2^64-59, d=6028; 84 ms for d=299756, all
    at N=0 (zero balls, so the time is the listing).

F8 of review f-review8: `f2_all_branches_cost_and_early_limit` now checks every centre of the
65536-branch list at 65537 and of the 6028-branch word-prime list, at `K = 20`, by the independent
integer equation `t_i^n = x` modulo `p^K`. It also checks the 4096-branch nontrivial list.
`r9_one_teichmuller_lift_per_list` checks every centre of its three word-prime lists by that equation.
The existing comparisons with `root_seed` additionally check agreement of the two interfaces.
The checks measure CPU time and require less than 5 seconds per complete centre verification.
`proto/lroot_checks.py --repair6` adds 12 integer-certified seed-42 square-root rows at 65537 and
`2^64 - 59`, `K = 2, 3, 4`; both the seeded result and the complete list are checked.

## R9 (one Teichmueller lift for the list; lane f-repair5, 2026-10-03)

For odd p, write omega(a) for the Teichmueller representative of a nonzero residue a: the unique
root of T^(p-1)-1 in Z_p that reduces to a (Lemma 3 item 2, functions.md:61; Proposition 4,
functions.md:99-100; lball.h adf_lball_teichmuller). Let d>=2, t0 and zeta as in R8, seed_i =
t0 zeta^i mod p, K>j, L=max(K-j,c) and kq=K-j<=L.
(a) omega(ab mod p)=omega(a) omega(b) for nonzero residues a, b; so omega(seed_i) =
    omega(seed_i0) omega(zeta)^(i-i0) for i>=i0.
(b) If z0 is exact or has relative precision >= kq, let c0 be the unit of z0 modulo p^kq
    (unit_mod), and y_i0 = c0 omega(seed_i0) mod p^kq, y_(i+1) = y_i (omega(zeta) mod p^kq) mod
    p^kq. Then y_i is the centre that branch() computes for seed_i when seed_i is general and not
    +-1: for z=z0*t, t the ball at L of omega(seed_i) (lball.h teichmuller), unit_mod(z,kq) = y_i,
    and the branch is the ball p^j y_i + p^K Z_p (v=j, N=K), identical in all fields to that of
    root_seed.
(c) The listing computes in this way: two lifts (omega(seed_i0) at the first general branch
    whose seed is not +-1, and omega(zeta)), then one product modulo p^kq for each later i. It
    returns no status that the code with one lift per branch would not return.

Proof.

1. (a) omega(a)omega(b) is a root of T^(p-1)-1, since (omega(a)omega(b))^(p-1)=1*1, and it reduces
   to ab mod p. The derivative (p-1)T^(p-2) is a unit at every nonzero residue, so Lemma 3 item 2
   gives exactly one such root in Z_p: it is omega(ab mod p). By induction on i, using
   seed_(i+1)=seed_i zeta mod p (R8: the identifiers are t0 zeta^i), omega(seed_i) =
   omega(seed_i0) omega(zeta)^(i-i0).
2. (b) By (a) and induction, y_i = c0 omega(seed_i) mod p^kq: the residue of a product modulo p^kq
   is the product of the residues. In branch(), t=teichmuller(seed_i, L) is the ball at L that
   contains omega(seed_i) (seed_i is not +-1, so it is a ball); z=mul(z0,t) is the smallest ball
   containing the products (lball.h), of valuation 0 and exponent min(N(z0), L) >= kq (or L for
   an exact z0), and it contains the point c0' omega(seed_i), c0' the centre of z0 (the value
   for an exact z0). unit_mod(z, kq) is defined (kq <= the relative precision of z) and is the
   unit modulo p^kq of every point of z (lball.h, L11 of api-1f.md), so it is c0' omega(seed_i)
   mod p^kq. c0 = unit_mod(z0, kq) is c0' mod p^kq for the same reason. So unit_mod(z,kq) = y_i.
   z is a ball, never the exact 1, so branch() always goes through unit_mod here, and it writes
   v=j, N=K and the centre y_i (an integer in (0, p^kq), a unit): the canonical ball of (b).
   root_seed calls branch() for the one seed, with the same z0 (computed by the same principal()
   from the same x, n, N), so its result is identical.
3. (c) The seeds 1 and p-1 (omega exact), the rational seeds (R3) and every seed before the chain
   starts go through branch() as before; K<=j or K outside the bound start no chain. The chain
   forms p^L (two lifts) and p^kq <= p^L and calls unit_mod(z0, kq). It starts at a general
   seed that is not +-1; such a branch with z not the exact 1 (z is a ball) exists, so R6
   step 6 b, c has already returned LIMIT unless p^L is within the bound; then no step of the
   chain returns LIMIT. The condition on z0 holds always (R4: exp at L gives z0 of relative
   precision L >= kq); it is tested, and if it failed every branch would lift its own
   representative, as before.
4. Identity, not only equality as sets, is the right test: a canonical ball is determined by its
   set (lball.h: the centre is the unique element of Z[1/p] in [0,p^N) in the ball).

Measured (CPU seconds; p=2^64-59, N=20; machine shared with other jobs, load average 6 to 9):
d=24068, x=w+p^30 Z_p with w=3^n mod p: 2.3 s before (3.9 to 7.8 s under heavier load),
0.04 to 0.09 s after;
d=299756, x=1: 44.6 s before (lane f-repair4), 1.2 s after. The guard of
`r9_one_teichmuller_lift_per_list` is 0.45 s. The test also compares every branch at d=1094 and
d=6028 and every 97th at d=24068 with root_seed (7371 branches, identical fields).

## Scope

R1-R9 are proved here (R9 and the rewritten R6 step 6 by lane f-repair5).
The analytic bijections and the criterion rely on the existing proofs listed above.
No rational-power API, principal-unit variable-exponent API or all-places root API is added.
The optional Remark 15r is not implemented. No new external mathematical source is pending.
The naming sources already pending in functions.md for Teichmueller representatives and Iwasawa Log
remain pending there; this slice uses their explicitly proved definitions.
