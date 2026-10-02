# Slice 1F.5: roots at a prime

The contract is in `include/adelefeld/lroot.h` and the added declarations in `rfunc.h`.
The implementation is `src/lroot.c`, with named-place wrappers in `src/rfunc.c`.
The proofs used from `docs/proofs/functions.md` are Lemma 3:55, Proposition 4:92, Lemma 9:265,
Proposition 11:336, Proposition 13:410, Proposition 15:463 and Proposition 16:538.
This document adds statements R1 to R7. It does not change SPEC 9.3.3.

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
4. The principal-unit factor is exp(Log(unit)/n). Alternative: decompose_teich then log(u)/n.
   Proposition 11 and F3 of api-1f4.md identify both logarithms. Log saves the input torsion lift;
   the root's torsion factor still comes from teichmuller. No iteration with a nonunit derivative is used.
5. All-branch enumeration factors a degree gcd(n,p-1) finite-field polynomial (R5).
   Alternatives: discrete logarithms or searching every residue. Neither is needed.
   The degree is bounded by ADF_LROOT_BRANCH_MAX = 2^26/64-1, so its coefficient vector fits
   the existing bit budget. This is an additional enumeration limit, not a time guarantee.
   Count and seeded evaluation can succeed when listing exceeds this limit.
6. A guarded ball input returns exponent E exactly, independent of requested N. An exact input uses N
   unless that branch is rational, in which case it is exact. Alternative: min(N,E) for ball inputs,
   as in lfunc.h. This slice chooses the exact image required by the brief; callers can coarsen it later.
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

## R2 (the result ball is the exact branch image)

On the guard, let j=m/n and b be the selected root of the centre. The result is exactly
b+p^E Z_p, E=j+r-s=M-s-(n-1)j. The identifier selects its torsion factor uniquely.

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

Let K=E for ball inputs and K=N for an irrational exact-input branch. For K>j put
L=max(K-j,c). Log at L+s, exact division by n, exp at L, and multiplication by the
selected torsion factor at L determine the unit root modulo p^L. The result has exponent K.

Proof.

1. The stored unit centre is an exact rational unit representing the centre of x/p^m.
   Log ignores its torsion factor, so it gives the principal-unit logarithm of that centre.
   Evaluating at L+s loses no more than that precision, by lfunc.h and api-1f4.md F6.
2. Division by the exact n, of valuation s, changes the absolute precision to L.
   R1 proves every point of the divided-log enclosure is in p^c Z_p, since L>=c.
3. Exp is an isometry on that domain. The torsion factor is an exact unit or an enclosure
   of precision L. Their product encloses the desired unit root to L, by lball multiplication.
4. Reduction modulo p^(K-j) and scaling the valuation by j gives the canonical centre at K.
   For a ball input, R2 shows this ball is the exact image, not just a safe enlargement.
   For an exact input it encloses the single selected point to the requested precision K.
5. If K<=j the root lies in p^K Z_p, so the canonical ball is centred at zero, with exponent K.
   This shortcut needs no working power. Labels still distinguish roots when enclosures overlap.
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
   FLINT lists all distinct nonzero roots with the contract in
   refs/src/flint-3.0.1/nmod_poly.rst:2392-2398. Sorting makes the order deterministic.
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
   L+s is safe and is checked against the working-power budget before any growing power.
   lfunc.h makes its own further working-power checks. Exact rational branches skip those powers.
3. Modular word products are evaluated in fmpz, so p above WORD_MAX and n up to UWORD_MAX
   do not overflow. gcd uses unsigned words. The finite-field degree is checked before casting to slong.
   Coefficient/list allocations use a bounded count and cannot overflow size_t.
4. Count uses temporary scalar outputs. A selected root uses temporary lballs; all branches are
   evaluated in temporary arrays before any caller array is touched. Even a late LIMIT after an
   exact first branch preserves the whole caller array. Reading x is finished before outputs are copied.
   This covers x aliasing any output slot, including an unused one. Overwritten output values are not checked.
5. The _at wrapper checks place membership first, copies the component, evaluates locally and builds
   a partial ball with arch NONE and one prime. That is Proposition 22:725 restricted to one place.
   The source may equal the destination. Failures preserve the destination and set optional where=v;
   success preserves where. The all-local-roots array is disjoint from the enclosing partial-ball storage.
6. Listing capacity is checked after the input criterion. Negative capacity is invalid (DOMAIN);
   insufficient nonnegative capacity or an excessive degree is LIMIT. No partial list is published.

## R7 (what the exhaustive oracle proves at its stated precision)

The oracle imports only Python's standard library. It uses modular integer powering, not logarithms,
Teichmueller lifting, a p-adic library, or the C implementation's finite-field factoring routine.

1. For p=2,3,5,7, n=1..12 and every a modulo p^M, it buckets every b modulo p^H by b^n modulo p^M.
   M=1..5 at 2 and M=1..3 at odd primes. H=M+1. This examines every integral input ball in those grids,
   including nonunit centres and centred balls. Additional rows scale unit balls by p^(n*j), j=-1,1.
2. On a guarded rootable ball, E=M-s-(n-1)j<=M. Since H>E, enumeration resolves a complete extra digit
   beyond the promised image radius. Grouping roots by identifier determines their unique centre modulo p^E.
   The oracle demands equality between the bucket and the union of all returned-format balls modulo p^H.
   For every branch it also demands both b and b+p^E, which are distinct modulo p^H.
3. A canonical ball at exponent E is determined by its residue modulo p^E. The C test compares that
   residue, exponent, identifier, count and exactness with the fixture, plus every permitted local alias.
   The finite equality is not a substitute for the infinite proof; R2 supplies the latter.
4. Exact rational inputs use output N=4 and modular-root enumeration modulo p^(4+s).
   Each identifier must have one residue modulo p^4; larger powers of p in n are accounted for by s.
   The finite criterion and R1 certify liftability. Rational branches are detected independently by
   exhaustive small integer numerator/denominator powers; others must return a ball at N=4.
5. The fixture totals are 8110 ball rows and 1408 exact-input rows, 643421 bytes together.
   Ball enumeration examines 45708 residues and certifies 5062 distance witnesses.
   C also checks 2400 reproducible seeded ball inputs, fractional valuations, word-size degrees,
   and p=2^64-59 at requested precision 200. Each comparison states and checks its output precision.

## Scope

R1-R7 are proved here. The analytic bijections and the criterion rely on the existing proofs listed above.
No rational-power API, principal-unit variable-exponent API or all-places root API is added.
The optional Remark 15r is not implemented. No new external mathematical source is pending.
The naming sources already pending in functions.md for Teichmueller representatives and Iwasawa Log
remain pending there; this slice uses their explicitly proved definitions.
