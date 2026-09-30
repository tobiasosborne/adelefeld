# Proofs for functions at named places and at all places

These proofs cover SPEC 9.3.1 to 9.3.6 and PLAN 0.3, 1F.3 to 1F.8. They include the repairs called N1 to
N7, N10 and R4 in the two reviews, and R-1, R-2, O-1, O-2 of `docs/reviews/m0-proofs/functions-review.md`.
The reviews are task inputs; the arguments below are written out here. No external convention is attributed
to a source not present in refs. Pending sources are stated explicitly. The specification is unchanged.

Run `python3 -B proto/functions_checks.py`. Each Check names a function in that file. A finite computation
checks examples or finite quotients of an infinite statement; it does not replace the proof.

## Definition 1 (places, precision and series).

Let p be a prime. Q_p is the completion of Q for its p-adic valuation, normalised by v_p(p) = 1.
Write v for v_p, v(0) = infinity, and Z_p = {x: v(x) >= 0}. Use the valuation laws
v(xy) = v(x) + v(y) and v(x+y) >= min(v(x),v(y)), with equality if the two valuations differ.
Set c = 1 for odd p and c = 2 for p = 2. A ball of exponent N, where N is any integer, is
`a + p^N Z_p`. An exact point is denoted separately, or by N = infinity. Precision p^n means error
in p^n Z_p; n may be negative or zero. Greater n means a smaller error set.

Use the rational-coefficient series

    exp(x)  = sum k>=0 x^k/k!
    sin(x)  = sum j>=0 (-1)^j x^(2j+1)/(2j+1)!
    sinh(x) = sum j>=0 x^(2j+1)/(2j+1)!
    cos(x)  = sum j>=0 (-1)^j x^(2j)/(2j)!
    cosh(x) = sum j>=0 x^(2j)/(2j)!
    log(1+z) = sum k>=1 (-1)^(k+1) z^k/k.

The real series and the local series have different target fields. A finite adele has coordinates in Z_p
at all but finitely many primes. An idele has nonzero coordinates and unit coordinates at all but finitely
many primes, with a nonzero real coordinate. These are the meanings of the terms used below.

Reduction identifies Z_p/p^k Z_p with Z/p^k Z for k>=1: a p-integral rational reduces by inverting
its denominator modulo p^k, and approximation by such rationals supplies the same residue for a limit.
Two approximants sufficiently close to the limit differ by p^k Z_p, so this definition is independent
of the approximant. Every integer residue occurs and the kernel is p^k Z_p.

Check: `check_domains`, `check_global`. Used by: SPEC 9.3.1 to 9.3.6.

## Lemma 2 (the real analytic facts used without proof).

The intermediate value theorem says that a continuous real function on [a,b], a <= b, takes every value
between its endpoint values. Real polynomials are continuous. The real exponential is positive, satisfies
exp(x+y) = exp(x)exp(y), and is a bijection from R to the positive reals with inverse ln. Complex exp
satisfies the same addition identity, has period exactly 2 pi i Z, has |exp(i theta)|=1 for real theta, and
Re(exp(i theta)) = cos(theta), with cos(pi/2) = 0. The real series in Definition 1, apart from log, converge
for every real argument. Real ln is defined on positive reals.

These are standard theorems, used as stated, not proved by the local arguments below.
[source pending: real intermediate value theorem and real/complex exponential with these normalisations]

Check: `check_real_and_character` (exact root and phase examples, not a numerical proof of these theorems).
Used by: SPEC 9.3.2 to 9.3.6.

## Lemma 3 (convergence, lifting, and finite multiplicative groups).

For every prime p the following facts hold.

1. A series in Q_p converges if and only if its terms tend to zero. A family of terms can be regrouped
   when, for each integer n, only finitely many terms have valuation less than n.
2. If f is in Z_p[T], f(r) = 0 modulo p and f'(r) is nonzero modulo p, exactly one root of f in Z_p
   reduces to r. Here r is a residue in F_p.
3. A finite subgroup G of the multiplicative group of any field is cyclic. In a cyclic group of order d,
   the equation t^n = w, n >= 1, has either no solution or exactly gcd(n,d) solutions.

Proof.

1. A convergent series has terms tending to zero by subtraction of consecutive partial sums. Conversely,
   the ultrametric inequality bounds every finite tail by its largest term norm. The partial sums are
   Cauchy when the terms tend to zero, and converge by the definition of Q_p as a completion. The same
   argument for all finite subsets proves the regrouping assertion.
2. Suppose r_k solves f modulo p^k, k >= 1. Its lifts are r_k + t p^k, 0 <= t < p. Taylor expansion gives
   f(r_k + t p^k) = f(r_k) + t p^k f'(r_k) modulo p^(k+1). Its linear coefficient is a unit, so exactly
   one t solves the next congruence. The compatible residues converge; continuity of the polynomial
   gives a root. Every root with the initial residue has these same digits. This also proves uniqueness.
3. Let E be the least common multiple of all element orders in G. For each prime power dividing E to its
   full exponent, take an element whose order has that exponent, and raise it to remove the other factors.
   Multiplying the resulting elements gives an element of order E. Indeed, commuting elements of coprime
   orders have product order the product of their orders: if a^k=b^(-k), that element has order dividing
   both orders, hence is 1, and each order divides k. Every element of G is a root of T^E-1.
   A degree E nonzero polynomial over a field has at most
   E roots, by repeated division by T-r. Thus |G| <= E, while the element of order E gives |G| >= E.
   It generates G. In G = <g>, t = g^j solves t^n = g^k precisely when n j = k modulo d.
4. For completeness, Bezout's identity here follows from the integer Euclidean algorithm. Successive
   division remainders are integer linear combinations of n,d. The last nonzero remainder g divides
   both by reversing the divisions, and every common divisor divides g. Thus g=gcd(n,d)=h n+l d.
   The congruence in step 3 requires g to divide k. If it does, divide by g and use that identity
   to invert n/g modulo d/g. This gives exactly one class modulo d/g, or g classes modulo d.

Check: `check_lifting_and_groups`. Used by: SPEC 9.3.2, 9.3.3 and 9.3.6.

## Proposition 4 (unique multiplicative decomposition).

For every prime p and nonzero x in Q_p there is exactly one decomposition

    x = p^m w u,  m in Z,  u in 1+p^c Z_p,

where w^(p-1) = 1 for odd p and w is 1 or -1 for p = 2. Negative m is allowed. Zero has no such
decomposition. At 2, w is fixed by x/2^m modulo 4. Define the Teichmueller representative of a nonzero
residue at p to be its lift satisfying t^(p-1) = 1. At 2 that representative is always 1, and need not be w.
[source pending: the name and convention for the Teichmueller representative]

Proof.

1. Necessarily m = v(x), so a = x/p^m is a unit. For odd p, F_p^x is cyclic by Lemma 3, hence every
   nonzero residue solves T^(p-1)-1. Its derivative (p-1)T^(p-2) is a unit. Lemma 3 gives a unique lift
   w with the residue of a. Then u = a/w lies in 1+p Z_p. Uniqueness follows from the same lifting lemma.
2. At 2, an odd unit is 1 or 3 modulo 4. Choose respectively w = 1 or w = -1. Then a/w is 1 modulo 4.
   The two choices have different residues, which proves uniqueness. The equation for the Teichmueller
   representative at 2 is T^1 = 1, so its only solution is 1; for instance the sign factor of -1 is -1.
3. Products of the factors in two decompositions are again factors of the indicated kind. Uniqueness
   therefore makes valuation, torsion factor and principal-unit factor multiplicative in the required
   senses: valuations add, and the other two factors multiply.

Check: `check_decomposition`. Used by: SPEC 9.3 notation, 9.3.2 to 9.3.4 and 9.3.6.

## Lemma 5 (factorial valuations and denominator estimates).

For every prime p and integer k >= 1, let s_p(k) be the sum of the base-p digits of k. Then

    v_p(k!) = sum j>=1 floor(k/p^j) = (k-s_p(k))/(p-1) <= (k-1)/(p-1),
    v_p(k) <= (k-1)/(p-1),   v_p(k) <= k/2.

For k = 0, v_p(0!) = 0. The factorial identity is often called Legendre's formula; it is proved here.
For nonzero x with v(x) = d, the degree k term of a factorial series has valuation k d - v_p(k!).
The degree k term of log(1+z), z != 0, has valuation k v(z) - v_p(k). Signs do not affect valuation.

Proof.

1. Each integer from 1 to k contributes one factor p for every power p^j dividing it. Counting these
   contributions gives the floor sum. Write k = sum a_i p^i and sum the floors digit by digit. The
   contribution of digit a_i is a_i(1+p+...+p^(i-1)); their sum is (k-s_p(k))/(p-1).
2. Since s_p(k) >= 1, the factorial bound follows. If e = v_p(k), then k >= p^e and
   p^e >= 1+e(p-1), by expanding (1+(p-1))^e. Thus e <= (k-1)/(p-1).
3. If e >= 1, k >= 2^e >= 2e; the last inequality follows by induction on e. If e = 0 the same bound
   is immediate. Hence e <= k/2. Valuations of the series terms now follow from multiplication and division.

Check: `check_legendre`. Used by: SPEC 9.3.2 and PLAN 1F.7.

## Proposition 6 (exact convergence domains, including boundaries).

For every prime p, each of exp, sin, sinh, cos and cosh converges in Q_p exactly on p^c Z_p. The log
series converges exactly on 1+p Z_p, including at p = 2. At zero the factorial series take their
constant terms; log(1) = 0. These assertions include negative valuations and do not assert any extension
of the series beyond their domains.

Proof.

1. For d = v(x) > 1/(p-1), Lemma 5 bounds the degree k valuation below by
   k(d-1/(p-1)) + 1/(p-1), which tends to infinity. This proves convergence for each parity subsequence.
   Since valuations in Q_p are integral, d >= c is exactly this strict inequality.
2. If d <= 0, each nonzero degree term has valuation <= 0. In each series infinitely many such terms
   occur, so they fail the term-to-zero test in Lemma 3. This covers all excluded arguments at odd p.
3. At p = 2 the remaining case is d = 1. In degrees k = 2^j, j >= 1, the term valuation is s_2(k) = 1.
   These terms occur in exp, cos and cosh. In degrees k = 2^j+1 it is s_2(k) = 2; these occur in sin
   and sinh. Thus all five diverge on 2 Z_2 outside 4 Z_2. Convergence on part of 2 Z_2 does not extend
   to its boundary shell. At p = 3, in contrast, d = 1 is strictly inside the convergence domain.
4. For log, put z = x-1. If v(z) >= 1, Lemma 5 gives k v(z)-v_p(k) >= k(v(z)-1/2), tending to infinity.
   If v(z) <= 0, the subsequence k = p^j has valuation p^j v(z)-j, which does not tend to infinity.
   This proves necessity too. In particular z = -2 and z = 2 are allowed at 2, so log(-1) and log(3)
   converge. The excluded boundary for this log series is v(x-1) = 0, not v(x-1) = 1.

Check: `check_domains`. Used by: SPEC 9.3.2.

## Proposition 7 (literal truncation counts).

Let p be any prime and n any integer. For a factorial series assume v(x) >= v, where v is an integer
at least c. Define ceildiv(a,b) = the least integer >= a/b, for integer a and positive b. Put

    d = (p-1)v - 1,
    K = max(1, ceildiv((p-1)n - 1, d)),
    T_exp = K,
    T_sin = T_sinh = floor(K/2),
    T_cos = T_cosh = ceildiv(K,2).

Take exactly the first T terms, in the order in Definition 1. In particular the constant term counts
for exp, cos and cosh; T = 0 means the empty sum for sin or sinh. Each omitted tail has valuation >= n.

For log(1+z) assume v(z) >= v, with an integer v >= 1, even at 2. Put

    J = max(1, ceildiv(2n, 2v-1)),    T_log = J-1.

Take degrees 1 through T_log. Its omitted tail has valuation >= n. Zero x or zero z is allowed and
can instead be evaluated exactly. These formulas are integer formulas, with no floating point logarithms.

Proof.

1. For k >= 1, Lemma 5 gives k v-v_p(k!) >= (k d+1)/(p-1). For k >= K the right side is at least n.
   All omitted degrees of exp are >= K. For the odd series their first omitted degree is
   2 floor(K/2)+1 >= K. For the even series it is 2 ceildiv(K,2) >= K.
2. Lemma 5 gives k v-v_p(k) >= k(2v-1)/2. Every omitted log degree is >= J and so has valuation >= n.
3. These bounds tend to infinity with k. Lemma 3 therefore puts the convergent sum of each omitted
   tail in the closed set p^n Z_p. This is a bound on the entire infinite tail, not just the next term.

Check: `check_truncation` (p = 2,3,5,13; four v values; every n from -3 to 40; degrees through 4096).
Used by: SPEC 9.3.2 and PLAN 1F.7.

## Proposition 7b (tight count for log).

The safe count T_log of Proposition 7 is correct but about twice what is needed. For log(1+z) under the
same hypotheses (v(z) >= v, an integer v >= 1, any integer n, even at 2), let e(k) be the largest e with
p^e <= k, and let J be the least integer k >= 1 with k v - e(k) >= n. Put T_tight = J-1 and take degrees
1 through T_tight. Then:

1. The omitted tail has valuation >= n, as with the safe count.
2. T_tight <= T_log, so this count is never longer than the safe count.

The C code of PLAN 1F.7 uses this tight count by default; its proof below is complete. The safe count of
Proposition 7 remains proved and is the fallback. Both counts are checked by enumeration. As in
Proposition 7, J is found by a loop over integers, with no floating point logarithm.

Proof.

1. v_p(k) <= e(k): if e = v_p(k) then p^e divides k, so p^e <= k and e <= e(k). The degree k term
   therefore has valuation at least k v - e(k). This quantity is nondecreasing in k: it grows by
   v >= 1 per step, and e(k) grows by at most 1 per step. J exists because k v - e(k) tends to
   infinity. Every omitted degree k >= J thus has valuation >= n, and Lemma 3 bounds the whole tail as in
   Proposition 7 step 3. For n <= 0 the value k = 1 already works and the sum is empty.
2. Lemma 5 gives e(k) <= k/2: for e = e(k) >= 1, p^e >= 2^e >= 2e and p^e <= k, and e = 0 is immediate.
   Hence k v - e(k) >= k(2v-1)/2 for every k. Every k >= max(1, ceildiv(2n, 2v-1)) satisfies
   k >= 2n/(2v-1), so its quantity is at least n. The least k with k v - e(k) >= n therefore satisfies
   J <= max(1, ceildiv(2n, 2v-1)), which is exactly Proposition 7's J for log. Hence T_tight <= T_log.

Check: `check_truncation` (both counts enumerated side by side; p = 2,3,5,13; four v values; every n from
-3 to 40; degrees through 4096). Used by: SPEC 9.3.2 and PLAN 1F.7 (the default count).

## Proposition 8 (working precision with denominators).

Under Proposition 7, a sufficient absolute working exponent for a direct partial sum is

    W = max(v, n+D),

where D is the largest p-valuation of a retained denominator. For a nonempty factorial sum with largest
degree L, take D = v_p(L!). For a nonempty log sum with largest degree L, take D = floor(log_p L),
computed as the largest e with p^e <= L. An empty sum has D = 0. This is sufficient, not optimal.

Represent x, or z for log, modulo p^W. Compute numerator powers by integer arithmetic modulo p^W.
For a term denominator d_k = p^e d'_k, with d'_k a p-adic unit, divide the numerator by p^e and
multiply by the inverse of d'_k modulo the required power of p. Retain at least n absolute digits for
the term and the sum. Exact rational arithmetic for the whole partial sum needs no guard precision.

Proof.

1. All inputs and their chosen representatives are integral. If they differ by p^W Z_p, the factorisation
   x^k-y^k = (x-y) sum i=0..k-1 x^(k-1-i)y^i shows their numerator powers differ by p^W Z_p.
   Integer products and sums modulo p^W preserve that fact, without loss from the number of terms.
2. Since W >= v, the representative y satisfies v(y) >= v, so for k >= 1 one has
   v(y^k) >= k v > v_p(k!) >= e by the
   estimate in Proposition 7 step 1 (for log, k v > v_p(k) >= e). The constant term needs no division.
   A nonconstant term is retained only
   when n >= 1: for the factorial series K >= 2 forces (p-1)n - 1 > (p-1)v - 1, i.e. n > v, and for log
   J >= 2 forces n >= v. Hence W >= n + D > e, and the integer residue y^k mod p^W is divisible by p^e;
   the division is exact. Dividing by d_k then loses exactly e absolute digits. The remaining error lies
   in p^(W-e) Z_p, contained in p^n Z_p because e <= D. Multiplication by a unit inverse loses no
   precision. The same argument shows that max(v, .) in W never binds when a nonconstant term is kept.
   This explains why rounding each numerator at n before dividing is not a valid substitute.
3. For factorial denominators, v_p(k!) is nondecreasing with k. For denominators 1,...,L of log, the
   greatest v_p(k) is the exponent of the largest power p^e <= L. These prove the stated choices of D.
4. Add the error of Proposition 7 to the partial-sum error. The ultrametric inequality still gives
   exponent n. For a whole input ball also take the minimum with its analytic image exponent from
   Propositions 10 and 11; extra working digits cannot recover missing input digits.

Check: `check_working_precision`. Used by: SPEC 9.3.1, 9.3.2 and PLAN 1F.7.

## Lemma 9 (series identities and inverse maps).

For every prime p, exp(x+y) = exp(x)exp(y) for x,y in p^c Z_p. For units a,b in 1+p Z_p,
log(ab) = log(a)+log(b), also at 2. The maps exp and log are inverse bijections between p^c Z_p
and 1+p^c Z_p. They restrict to inverse bijections between p^r Z_p and 1+p^r Z_p for every integer
r >= c. These statements concern the series, before extending log to other nonzero arguments.

Proof.

1. In rational formal series, the coefficient of X^i Y^j in exp(X+Y) is 1/(i! j!), proving the product
   identity. For L(X) = log(1+X), formal differentiation gives L'(X) = 1/(1+X). Differentiating
   L(X+Y+XY)-L(X)-L(Y) in X and Y gives zero; its constant coefficient is zero. Thus it vanishes.
2. Let E(X) = exp(X). Formal differentiation gives
   (L(E(X)-1))' = E'(X)/E(X) = 1; its constant term is zero, so it is X. Also F(X) = E(L(X))
   satisfies (1+X)F'(X) = F(X), F(0) = 1. Equating coefficients gives uniquely F(X) = 1+X.
   These computations are formal over Q, so division by positive integers is legitimate.
3. Justify evaluation and regrouping. In the exponential product, valuations of terms with total
   degree d tend to infinity by Lemma 5. In the logarithmic product, a term of outer degree j has
   total degree d between j and 2j and integral numerator coefficients; its valuation is at least
   d-j/2 >= d/2 by Lemma 5. This tends to infinity with d.
4. For either composite inverse, consider inner positive degrees k_1,...,k_j summing to d. In E(L),
   the denominator valuation is at most
   sum (k_i-1)/(p-1) + (j-1)/(p-1) = (d-1)/(p-1).
   The same bound holds in L(E-1), exchanging k_i and k_i! in the denominators and using Lemma 5.
   Input valuation at least c gives valuation at least d c-(d-1)/(p-1), tending to infinity.
   There are finitely many terms of any fixed total degree. Lemma 3 permits all the regroupings.
5. For x in p^r Z_p, r >= c, every term after the linear term of exp(x)-1 has valuation strictly
   greater than v(x): (k-1)c-v_p(k!) > 0 for k >= 2 by Lemma 5. For z in p^r Z_p the analogous
   inequality (k-1)c-v_p(k) > 0 gives v(log(1+z)) = v(z). At odd p, use p-1 >= 2; at 2 use c = 2.
   Thus the formal inverses map the indicated discs into each other and prove the claimed bijections.

Check: `check_series_identities` (with independent exact series values, not identities alone).
Used by: SPEC 9.3.2 to 9.3.4.

## Proposition 10 (factorial-series radii and the cosine hull).

Let p be any prime, x,y in p^c Z_p, and B = a+p^N Z_p a ball contained in that disc. Then N >= c
and v(a) >= c. Exp, sin and sinh preserve distances:

    v(f(x)-f(y)) = v(x-y).

Consequently f(B) has smallest enclosing ball f(a)+p^N Z_p. Cos and cosh are 1-Lipschitz there,
so retaining exponent N is safe. For the centred ball p^N Z_p, N >= c, the smallest enclosing ball
for either cos or cosh is exactly

    1+p^(2N-v_p(2)) Z_p.

This last assertion is about the hull, not equality of the image set with that ball. For exact points
there is no input uncertainty, but evaluating a transcendental centre still requires output precision.

Proof.

1. Since B is in p^c Z_p, its elements a and a+p^N, and therefore their difference, lie in that group.
   Thus v(a)>=c and N>=c. If x != y, factor their degree k difference as in Proposition 8. After division
   by k!, its valuation is at least v(x-y)+(k-1)c-v_p(k!). For k >= 2 the excess is a positive integer
   by Lemma 9(5).
2. In exp, sin and sinh the degree 1 difference is x-y. All other nonconstant differences have greater
   valuation and cannot cancel it. This proves the equality. All differences in cos and cosh have
   valuation at least v(x-y)+1. In particular exponent N is safe for every image point of B.
3. For each of the first three functions, choose y=a+p^N and x=a. The equality in step 2 gives a
   difference of valuation exactly N. Since a ball containing f(B) contains f(a), it cannot have a
   larger exponent. This proves the hull assertion without assuming surjectivity of sin or sinh.
4. On the centred ball, the quadratic term has valuation at least 2N-v_p(2). For even k >= 4,
   kN-v_p(k!) > 2N-v_p(2): at odd p use (k-1)/(p-1) <= (k-1)/2 < k-2; at 2 use
   v_2(k!)-1 <= k-2 < 2(k-2). All higher terms therefore have strictly greater valuation.
5. At x=p^N the quadratic term has exactly valuation 2N-v_p(2), and cannot be cancelled. At x=0
   the output is 1. These two values prove minimality of the displayed hull. For example,
   cos(4) = 9 modulo 16 at 2, while v_3(cos(3)-1) = 2.

Check: `check_series_radii`. Used by: SPEC 9.3.2.

## Proposition 11 (logarithm radii and Iwasawa Log).

For every prime p, log is an isometry on 1+p^c Z_p and is 1-Lipschitz on 1+p Z_p. At 2 it is
not injective on the latter: log(-1) = log(1) = 0 and log(1+2 Z_2) = 4 Z_2.

Define Log(x) = log(u) for the decomposition in Proposition 4. This convention sets Log(p) = 0;
it is an additive-valued homomorphism on Q_p^x and agrees with the series wherever the latter is defined.
[source pending: the name and normalisation of the Iwasawa logarithm]

For any integer N and a != 0 with m = v(a) < N, put r = N-m. Then

    Log(a+p^N Z_p) = Log(a)+p^r Z_p,  if r >= c,
    Log(a+2^N Z_2) = 4 Z_2,          if p=2 and r=1.

Thus negative m gains absolute digits and positive m loses m digits in the first formula. The formula
does not apply to a ball containing zero. For the log series, a ball in its domain has exponent N >= 1;
its image is log(a)+p^N Z_p if N >= c, and at 2 with N=1 its image is 4 Z_2.

Proof.

1. Lemma 9 gives log(y)-log(x) = log(y/x). For x,y in 1+p^c Z_p, the valuation of y/x-1 equals
   v(y-x), and the linear-term calculation in Lemma 9(5) proves isometry.
2. At 2, the product identity gives 2 log(-1) = log(1) = 0. The field has characteristic zero, so
   log(-1) = 0. Proposition 4 writes every odd unit as w u, w = +/-1 and u in 1+4 Z_2. Its series
   log is log(u), with image exactly 4 Z_2 by Lemma 9. If two odd units differ by valuation 1, both
   logs are in 4 Z_2, so their log difference has valuation >= 2. If their difference has valuation
   >= 2, their quotient is in 1+4 Z_2 and step 1 gives equality. This proves the full Lipschitz claim.
3. Proposition 4(3) and Lemma 9 give Log(xy) = Log(x)+Log(y). It kills p and all w factors. At odd p
   the series domain has w=1. At 2 step 2 proves agreement even when w=-1.
4. Write the nonzero ball as a(1+p^r Z_p). For r >= c, step 3 and the exact bijections in Lemma 9
   give its image Log(a)+p^r Z_p. At 2 with r=1 the second factor is all odd units, whose log image
   is 4 Z_2. Since Log(a) also belongs to 4 Z_2, translation does not change this set.
5. For a series-log ball, v(a)=0. The same argument gives the stated images. In particular the safe
   exponent N=1 is loose at 2. The losses in the specification are witnessed by
   v_3(12-3)=2 but v_3(Log(12)-Log(3))=v_3(log(4))=1, and
   v_2(10-2)=3 but v_2(Log(10)-Log(2))=v_2(log(5))=2.

Check: `check_log_radii`. Used by: SPEC 9.3.2 and PLAN 1F.4.

## Proposition 12 (simultaneous finite domains and logarithm enclosure).

The common finite domain of the five factorial series is

    D = 4 Z_2 x product over odd p of p Z_p.

It contains nonzero finite adeles. The five componentwise functions map D into the finite adeles.
No finite ball a+R Zhat, a rational and R a positive rational, is contained in D. The diagonal rational
points of D consist of 0 alone. For the supported finite rational singletons and rational balls, therefore,
only the exact finite part 0 certifies the simultaneous domain. The real coordinate may be arbitrary.

For any idele, its finite Iwasawa Log belongs to D, hence to 4 Zhat. Its real logarithm needs a positive
real coordinate; ln(abs(x_inf)) is the separately defined real log_abs on nonzero real coordinates.

Proof.

1. Proposition 6 imposes precisely the coordinate conditions defining D. Every coordinate is integral,
   so D is contained in the finite adeles. The tuple with coordinate 4 at 2 and p at every odd p belongs
   to D and is nonzero. Proposition 10 gives integral values for all five functions on D, hence finite
   adeles. At finite argument 0 their outputs are respectively 1,0,0,1,1.
2. Outside the finitely many prime divisors of the numerator and denominator of R and the denominator
   of a, the projection of a+R Zhat is Z_p. Choose an odd such p; its projection contains 1, outside
   p Z_p. Such a prime exists: for any finite prime list, a prime divisor of one plus their product
   is outside the list. Include 2 in that list. Thus the ball is not contained in D.
3. A nonzero rational has valuation zero outside the finite set of prime divisors of its numerator and
   denominator. At an odd prime outside this set it fails the defining condition of D. Zero satisfies
   every condition. Notice that a ball can meet D without certifying that all its points are in D.
4. Proposition 4 writes each nonzero coordinate as p^m w u with u in 1+p^c Z_p, and Lemma 9 maps
   1+p^c Z_p onto p^c Z_p. Hence every local Log(x_p) = log(u) lies in p^c Z_p, independently of the
   valuation or torsion of x_p. These values form an integral finite adele in D. At 2 it is divisible
   by 4, and at odd p, 4 Z_p = Z_p, so D is contained in 4 Zhat. This proves the conservative enclosure.
   The assertions about the real coordinate use Lemma 2.

Check: `check_global`. Used by: SPEC 9.3.2 and PLAN 1F.8.

## Proposition 13 (local roots, counts and square specialisations).

Let p be any prime, n >= 1 an integer, and a != 0 in Q_p with decomposition a = p^m w u. Negative
m is allowed. An n-th root exists if and only if all three conditions hold:

    n divides m;
    w is an n-th power in mu_(p-1), for odd p, or in {1,-1}, for p=2;
    v_p(log(u)) >= c+v_p(n).

When they hold, there are exactly gcd(n,p-1) roots at odd p and gcd(n,2) roots at 2. A torsion root
identifies a branch. The principal-unit root in that branch is unique. For a=0 there is exactly one
root, 0, for every n >= 1, without using a decomposition or a logarithm.

For n=2 the criterion is: m even and the unit part a/p^m a square modulo p at odd p; at 2 it is m
even and a/2^m = 1 modulo 8. At degree 1 the root is always a.

Proof.

1. Write a proposed nonzero root as p^j t z by Proposition 4. Uniqueness of decomposition gives
   n j = m, t^n = w and z^n = u. Lemma 9 gives n log(z) = log(u), with log(z) in p^c Z_p. These
   imply all three necessary conditions, including when m is negative or log(u)=0.
2. Conversely, choose a torsion root t and put
   b = p^(m/n) t exp(log(u)/n). The last condition puts log(u)/n in p^c Z_p. Lemma 9 gives b^n = a.
   It also proves that exp(log(u)/n) is the only principal-unit root. Every root arises this way by
   step 1. The torsion groups are cyclic by Lemma 3 (and have orders p-1 and 2), giving the counts.
3. At odd p, v_p(2)=0, so the logarithmic condition is automatic. Reduction modulo p is a bijection
   of the torsion group with F_p^x by Proposition 4. Thus the torsion condition is exactly the stated
   quadratic-residue test. At 2, w must be 1, and the logarithmic condition is v_2(log(u)) >= 3.
   The isometry of Proposition 11 says this is equivalent to u = 1 modulo 8. It implies the sign
   condition too when phrased as a condition on the whole unit a/2^m.
4. Zero has no nonzero root in a field. At n=1 each condition is automatic and the construction gives a.
   Examples: 3 fails the square criterion at 2; 9 satisfies it and has roots +/-3. The derivative 2b
   is not a unit there, so the simple-root version of Lemma 3 alone is not the square-root algorithm.

Check: `check_root_criteria`. Used by: SPEC 9.3.3 and PLAN 1F.5.

## Proposition 14 (real roots).

For every integer n >= 1 and real a, odd n gives exactly one real n-th root, with the sign of a.
For even n, negative a has no real root, zero has the one root zero, and positive a has exactly two,
opposite in sign. Selecting the nonnegative root in the even case is a branch convention.

Proof.

1. For 0 <= x < y, y^n-x^n = (y-x) sum i=0..n-1 y^(n-1-i)x^i is positive. Hence x^n is strictly
   increasing on the nonnegative half line. Given a >= 0, choose M >= max(1,a); then M^n >= a.
   Lemma 2 gives a root in [0,M], and strict increase gives uniqueness there.
2. For odd n, (-x)^n=-x^n; reflect the nonnegative result to obtain existence and uniqueness for
   negative a too. For even n, (-x)^n=x^n and every n-th power is nonnegative. Reflecting gives
   exactly the additional negative root when a>0 and no extra root at zero.

Check: `check_real_and_character`. Used by: SPEC 9.3.3 and 9.3.6.

## Proposition 15 (guarded branch precision is an exact image).

Let p be any prime, n >= 1 an integer, b != 0 in Q_p, a=b^n, j=v_p(b), m=n j, and N an integer.
Suppose r=N-m >= c+v_p(n). Define the branch over a+p^N Z_p by choosing the root whose ratio to b
is in 1+p^c Z_p. Its image is exactly

    b+p^(N-v_p(n)-(n-1)j) Z_p.

This includes negative j, negative N allowed by the guard, and degree 1. It is not asserted for b=0.
Different root branches of a give disjoint balls under the guard. Unit square roots lose zero digits
at odd p and one at 2; unit p-th roots lose one digit, with the guard still required.

Proof.

1. Every y in the input ball is uniquely a(1+t) with t in p^r Z_p. In particular y != 0. Define

       F(y) = b exp(log(1+t)/n).

   Lemma 9 maps t bijectively to p^r Z_p via log(1+t). Division by n maps that ball bijectively
   to p^(r-v_p(n)) Z_p. The guard puts the latter inside p^c Z_p. Exp maps it bijectively to
   1+p^(r-v_p(n)) Z_p. Multiplication by b gives the ball in the statement, since
   j+r-v_p(n) = N-v_p(n)-(n-1)j.
2. Lemma 9 also gives F(y)^n = b^n exp(log(1+t)) = y. Thus every image point is the chosen root
   of an input point. Conversely each element of the stated output ball is obtained by reversing the
   bijections in step 1, proving equality of sets, not merely an error estimate.
3. A principal-unit n-th root of unity has zero logarithm and therefore equals 1 by Lemma 9. The
   root condition on the ratio to b consequently selects exactly one branch. Distinct roots of a
   differ by a nontrivial torsion factor, whose residue differs modulo p at odd p or modulo 4 at 2.
   Their ratio cannot lie in 1+p^c Z_p, so the branch balls are disjoint.
4. When j=0, the exponent simplifies to N-v_p(n), giving the advertised unit losses. At n=1 the
   image is the original ball. For exact nonzero inputs the branch is an exact point as a set, though
   its numerical centre may require approximation. Zero and balls containing zero require other treatment.
5. Outside the guard a total branch can fail to exist. At 2, a=1, n=2, N=2 gives 1+4 Z_2,
   containing 5, which is not a square modulo 8. The formally predicted ball 1+2 Z_2 squares into
   1+8 Z_2, not onto 1+4 Z_2. At 3, a=1, n=3, N=1 includes 4, while unit cubes modulo 9 are
   only 1 and 8. These exhibit the need to split or return NOT_DETERMINED rather than apply the formula.

Check: `check_root_precision`. Used by: SPEC 9.3.3 and PLAN 1F.5.

## Remark 15r (odd degree roots at 2 need only r >= 1). Not part of SPEC 9.3.3.

In Proposition 15 let p = 2 and let n be odd, and replace the guard r >= c + v_p(n) = 2 by r >= 1. Keep
all other hypotheses and notations. Then every point of the input ball a+2^N Z_2 has exactly one n-th
root in Q_2, the branch map is a bijection from the input ball onto

    b+2^(N-v_p(n)-(n-1)j) Z_2 = b+2^(N-(n-1)j) Z_2,

and this displayed ball is the smallest enclosing ball of the image. For r >= 2 the formula is the one
of Proposition 15 with v_2(n) = 0. SPEC 9.3.3 keeps the stronger guard N - m >= c + v(n), and the main
statement of Proposition 15 is unchanged. This remark is an optional refinement: it turns
NOT_DETERMINED results at 2 into values. Degree 1 is included.

Proof.

1. r = N - m = 1 gives a+2^N Z_2 = a(1+2 Z_2), and 1+2 Z_2 is the group Z_2^x of all odd units.
2. The map x -> x^n is a bijection of Z_2^x for odd n. By Proposition 4 every odd unit is uniquely w u
   with w in {1,-1} and u in 1+4 Z_2. On the w factor the map is the identity, since n is odd. On the u
   factor it is exp(n log(u)) by Lemma 9, and multiplication by n is a bijection of 4 Z_2 because
   v_2(n) = 0. Lemma 9 maps back bijectively. The two factors are independent, being cosets.
3. The torsion of Q_2^x is exactly {1,-1}. Let w u be torsion, so (w u)^m = 1 for some m >= 1. Then
   u^m = w^(-m), whose left side lies in 1+4 Z_2 and whose right side is 1 or -1. So u^m = 1, w^m = 1,
   and m log(u) = log(1) = 0 (Lemma 9). The characteristic is zero, so log(u) = 0 and u = 1 by Lemma 9.
   Hence w u = w is 1 or -1. For odd n the only n-th root of unity is 1, since (-1)^n = -1.
4. Let y be any point of the input ball. By step 1, y = a u with u in Z_2^x, and step 2 gives exactly one
   w in Z_2^x with w^n = u. Then (b w)^n = a u = y, so a root exists. If also z^n = y, then
   (z/(b w))^n = 1, so z = b w by step 3. The root is unique in Q_2 and lies in b Z_2^x.
5. The image of the branch is therefore b Z_2^x. Writing b = 2^j times an odd unit, this set is exactly
   b+2^(j+1) Z_2. Since a = b^n has m = n j and N = m+1, the exponent of the statement is
   N-(n-1)j = n j+1-(n-1)j = j+1.
   By step 4 the branch map is injective and its image is exactly this set. Equality of sets follows,
   and the displayed ball is the smallest enclosing ball of the image.

Check: `check_root_precision` (the blocks counting guard_r1_at_2 and unique_roots_at_2). Used by: not by
SPEC 9.3.3, which keeps the stronger guard; optional for PLAN 1F.5.

## Proposition 16 (all places and the rational-root contract).

Let n >= 2. An ordinary idele unit coset of finite precision, with only finitely many restricted primes,
cannot certify n-th-power membership at all places. The obstruction applies even if every inspected
place has a root. Degree 1 is the identity. For an exact rational a and n >= 1, having an n-th root
at every place is equivalent to having a rational n-th root. For a=0 the sole adelic root is zero.
For a!=0 an exact-rational operation may therefore return the rational branch from Proposition 14.
It must not claim to enumerate all adelic branches: 1 has continuum many adelic square roots.

Proof.

1. Choose a prime ell dividing n and let S contain ell and all restricted primes and valuation-scale
   primes of the coset. There exists a prime q outside S with ell dividing q-1, by this elementary
   argument. Put A = ell times the product of primes in S, and H = 1+A+...+A^(ell-1). Choose a prime
   divisor q of H. Every prime in S divides A, so H = 1 modulo that prime and q is outside S.
   Also A^ell = 1 modulo q. If A=1 modulo q then H=ell modulo q, forcing q=ell, a contradiction.
   Thus A has order exactly ell modulo q. Lemma 3 implies ell divides q-1.
2. In F_q^x the n-th power map has kernel size gcd(n,q-1) >= ell and is not surjective. Choose a
   unit residue outside its image at this unrestricted prime. Complete it with any permitted coordinates
   elsewhere. Changing one unit coordinate preserves an idele in the coset, and the chosen coordinate
   has no n-th root. This proves the finite-precision obstruction, for every n>=2 without invoking a
   theorem on primes in arithmetic progressions. For squares an arbitrary unrestricted odd prime suffices.
3. Write a nonzero rational in lowest terms as a sign times a product of prime powers p^e_p, with
   finitely many nonzero exponents, allowed to be negative. Integer prime factorisation follows by
   induction taking a prime divisor; uniqueness follows from prime division of a product, obtained
   from Bezout when a factor is coprime to that prime. Proposition 13 at every p forces n to divide
   every e_p. The product of p^(e_p/n) is a positive rational n-th root of abs(a).
4. Proposition 14 supplies the allowable sign: for even n, a must be positive; for odd n its sign
   is the sign of its root. The resulting rational root embeds as a root at every place. For an
   implementation, testing numerator and denominator separately for exact integer n-th powers gives
   the same criterion and needs no prime factorisation. Zero has only zero at each field coordinate.
5. A square root of 1 in any characteristic-zero field is 1 or -1, because (x-1)(x+1)=0. Choose these
   signs independently at every prime and at the real place. Every tuple is an idele and squares to 1.
   The set of choices is the set of binary sequences on a countably infinite set, of cardinality
   2^aleph_0, called the continuum. Hence no finite list of branches can enumerate it. Restricting the
   output to diagonal rational roots +/-1 resolves the issue in R4 while retaining the existence test.

Check: `check_global_roots`. Used by: SPEC 9.3.3 and PLAN 1F.8.

## Proposition 17 (the four powers and the lost factors in exp(s Log x)).

Integer powers are ring powers for nonnegative exponents, with exponent 0 defined as the multiplicative
identity, including at base 0. Negative integer powers require an invertible base. A rational exponent
e/n, e an integer and n>=1, means integer powers and the selected n-th-root branches; reduce the fraction
first and state the branch. A negative exponent excludes zero. These are not the operation below.

For every prime p, u in 1+p^c Z_p and s in Z_p, define u^s = exp(s log u). This is continuous in
both variables, agrees with every integer power (including negative integers), and satisfies
u^(s+t) = u^s u^t and (uv)^s = u^s v^s for principal units u,v and s,t in Z_p.
For an arbitrary odd 2-adic unit x=w u, the integer-compatible extension is

    x^s = w^(s mod 2) exp(s log u),    s in Z_2.

For a general nonzero x, exp(s Log x), whenever defined, discards valuation and torsion. It is not
an integer-compatible power. Separately, for t>0 real, complex s and a complex multiplicative character
chi on the chosen unit factor, t^s chi(u') means exp(s ln(t)) chi(u'), a complex-valued quasi-character.
Here the idele-class decomposition and the choice of chi are input data, not assertions proved in this file.

Proof.

1. The integer operations use associativity and the inverse; if x is not invertible, x^(-1) is not
   defined. Rational branches use Proposition 13 or 14 and inherit their existence conditions. At base
   zero positive rational exponents give zero; exponent zero follows the explicit convention above.
2. Since v_p(s)>=0 and log(u) lies in p^c Z_p, its product with s lies in the exp domain. Lemma 9
   gives all stated power identities, first for integers using repeated addition and negatives, then
   directly for s,t using the product identities. Proposition 11 and multiplication prove continuity.
3. At 2 the sign exponent depends continuously on s modulo 2. It agrees with integer parity, while
   step 2 handles the principal factor. Thus the displayed formula extends integer powers to all odd
   units. At odd p a nontrivial torsion unit w cannot have a continuous Z_p exponent agreeing with
   integers: p^k tends to 0 in Z_p, but w^(p^k)=w != 1, since p=1 modulo p-1. A separate torsion
   exponent modulo p-1 is needed. For valuation m!=0, the powers of p^m at exponents p^k likewise
   fail to tend to 1: they tend to 0 if m>0 and have valuations tending to negative infinity if m<0.
4. Proposition 11 gives Log(p)=0, hence exp(Log(p))=1 != p. At 2, exp(Log(-1))=1 != -1 as well.
   These already fail at s=1. Finally Lemma 2 gives multiplicativity of exp(s ln t) for positive real
   t, and multiplication by chi preserves it. Its values belong to C^x; no local p-adic value is asserted.

Check: `check_powers`. Used by: SPEC 9.3.4 and PLAN 1F.6.

## Proposition 18 (independent base and exponent uncertainty).

Let p be any prime, A and B finite integers with A>=c and B>=0, u_0 in 1+p^c Z_p, and s_0 in Z_p.
Let the base range independently over u_0+p^A Z_p and the exponent over s_0+p^B Z_p. Put

    ell = log(u_0),   alpha = v_p(ell),   beta = v_p(s_0),
    R = min(A+beta, B+alpha, A+B).

Use v_p(0)=infinity. The set of all principal-unit powers is exactly the ball

    exp(s_0 ell)+p^R Z_p.

Thus this is a safe radius rule and the smallest ball, including u_0=1 or s_0=0. Both uncertainties
matter, even when both centres give a zero product. If a base or exponent is exact, omit its error terms:
an exact base gives R=B+alpha; an exact exponent gives R=A+beta; both exact give no input error.
If an exact factor of the log product is zero, the output is the exact 1, irrespective of the other factor.

Proof.

1. Proposition 11 identifies the logarithms of the base ball exactly with ell+p^A Z_p. Multiplying
   ell+delta and s_0+epsilon gives s_0 ell+s_0 delta+ell epsilon+delta epsilon. These three errors
   have valuations at least A+beta, B+alpha and A+B. This proves containment in s_0 ell+p^R Z_p.
2. The product set is that entire ball. If alpha<A and beta<B, the cross term has strictly higher
   valuation than the smaller of the first two bounds. Hold the corresponding nonzero centre fixed
   and vary the other factor; this already fills a ball of exponent min(A+beta,B+alpha) about the
   product centre. If alpha<A and beta>=B, the exponent ball is p^B Z_p: fixing ell fills the
   product ball p^(B+alpha) Z_p. The case alpha>=A and beta<B is symmetric.
3. If alpha>=A and beta>=B, the factors range over p^A Z_p and p^B Z_p. Their product set is
   p^(A+B) Z_p: to attain an arbitrary element fix the first factor to p^A and vary the second.
   These cases exhaust the possibilities, including infinite alpha or beta.
4. Each factor product lies in p^c Z_p and R>=c. Lemma 9 gives
   exp(s_0 ell+p^R Z_p) = exp(s_0 ell)(1+p^R Z_p) = exp(s_0 ell)+p^R Z_p, proving exactness.
   If either input is exact, multiplication by that fixed value gives the stated separate formula.
5. For arbitrary odd 2-adic units one first fixes or splits the base sign modulo 4 and exponent parity
   modulo 2, as required by Proposition 17. If A>=2 and B>=1 these factors are fixed and the same
   radius applies after multiplication by the known sign. At A=1 or B=0 one must take the union of
   the applicable sign/parity images; the principal-unit formula alone must not discard that uncertainty.

Check: `check_power_precision`. Used by: SPEC 9.3.4 and PLAN 1F.6.

## Proposition 19 (the p-primary fractional part).

Let p be any prime and x in Q_p. There is exactly one rational r with denominator a power of p,
0<=r<1, and x-r in Z_p. It is denoted {x}_p. If v_p(x)>=0, including x=0, it is 0. Otherwise
put k=-v_p(x)>0, choose the residue a of p^k x modulo p^k with 0<=a<p^k, and set r=a/p^k.
The complement x-{x}_p need not be an ordinary integer. On a ball x+p^N Z_p the fractional part
is constant exactly when N>=0; an exact point has its exact fractional part.

Proof.

1. The construction gives p^k x-a in p^k Z_p, hence x-a/p^k in Z_p. The chosen bounds on a give
   0<=a/p^k<1. For integral x, r=0 satisfies the same conditions.
2. If r and r' both work, r-r' is p-integral and has denominator a power of p. Its reduced denominator
   is therefore 1, so it is an integer. But -1<r-r'<1, forcing r=r'.
3. Addition of any integral element preserves the defining residue class, proving constancy for N>=0.
   For N<0, the ball contains x and x+p^N, whose difference is not integral. Equal fractional parts
   would make their difference integral by step 1, a contradiction.
4. At p=2, x=1/3 is integral, so its fractional part is 0. Its complement 1/3 is not an ordinary
   integer. This directly distinguishes a p-adic integral part from a real floor.

Check: `check_fractional_parts`. Used by: SPEC 9.3.5, 9.3.6 and PLAN 1F.3.

## Proposition 20 (the additive character is not diagonal rational cosine).

Adopt the convention in SPEC 6 as a definition:

    psi(x) = exp(2 pi i (-x_inf + sum_p {x_p}_p)),    x in A.

The sum is finite. This defines an additive character with complex values of absolute value 1, trivial
on diagonal Q, hence a character on A/Q. On diagonal 1/4 it equals 1, whose real part is 1, while
the ordinary real cos(2 pi/4) is 0. On (r;0) its value is exp(-2 pi i r). Named real and imaginary
parts of psi are consequently different functions from componentwise sine and cosine.
[source pending: on-disk Tate source for SPEC 6's attribution of this sign convention]

Proof.

1. A finite adele is integral at almost every prime, where Proposition 19 makes its fractional part
   zero. At each p, {x_p+y_p}_p-{x_p}_p-{y_p}_p is p-integral with denominator a power of p,
   hence an integer. Summing gives the additive identity for psi by Lemma 2's exponential period.
2. For a rational q, put r=q-sum_p {q}_p. At any prime p, q-{q}_p is p-integral, while each other
   summand has denominator a power of a different prime and is p-integral. Thus r is integral at
   every prime. A rational integral at every prime has denominator 1, hence r is an integer. The
   phase for diagonal q is -r, so psi(q)=1. This also proves independence of the lift from A/Q.
3. At q=1/4 only the 2-primary fractional part is nonzero and equals 1/4, giving phase zero.
   Lemma 2 gives cos(pi/2)=0. If the entire finite part is zero, all its fractional parts vanish,
   giving the displayed value on (r;0). The codomain and these values prove the distinction claimed.

Check: `check_real_and_character`. Used by: SPEC 9.3.5.

## Proposition 21 (no compatible local order).

No Q_p, for any prime p, admits an ordered-field order, and therefore none has a real-style sign or
floor defined from such an order. In particular Q_5 contains a square root of -1. This does not prohibit
an arbitrary set ordering or the separately defined fractional part of Proposition 19.

Proof.

1. At p=5, 2 is a root of T^2+1 modulo 5 and the derivative at 2 is 4, a unit. Lemma 3 lifts it
   to a root in Q_5. A square is nonnegative in an ordered field, contradicting -1<0.
2. More generally at odd p, the squares in F_p have (p+1)/2 elements: nonzero squares have exactly
   two square roots by Lemma 3, and include zero once. The sets {a^2} and {-1-b^2} therefore meet,
   giving a^2+b^2=-1 modulo p. At least one of a,b is nonzero. Fix the other coordinate and apply
   Lemma 3 to lift the nonzero one, since its derivative twice that coordinate is a unit. Then -1
   is a sum of two squares in Q_p, again impossible in an ordered field.
3. At 2, -7=1 modulo 8, so Proposition 13 gives z with z^2=-7. Then
   z^2+1^2+1^2+2^2=-1, again a sum of squares. This covers the last prime.

Check: `check_no_order`. Used by: SPEC 9.3.3, 9.3.6.

## Proposition 22 (typed values, uncertainty and named-place evaluation).

For p prime and x!=0 in Q_p, v_p(x) is an integer and |x|_p=p^(-v_p(x)) is a positive rational.
At zero use v_p(0)=infinity and |0|_p=0. On a finite-exponent ball a+p^N Z_p, valuation is
determined exactly when v_p(a)<N, in which case it is v_p(a). Otherwise there are different valuations
in the ball. Real sign has values -1,0,1; real floor and ceiling have integer values. A ball determines
such a value exactly when all its points give that value; meeting both sides of a jump cannot do so.

At named field places, a rational function is defined when its denominator is nonzero everywhere on
the input. In the adele ring, an element is invertible exactly when it is an idele. An exact nonzero
rational is an example. All-place division therefore requires the corresponding invertibility certificate.

For a finite named place set S and coordinatewise functions f_v, applying f after projection to S is
identical to forming (f_v(x_v)) for v in S. The result is typed over S. An unqualified all-place map
requires every coordinate's domain condition and an adelic output. A stored uncertain zero does not
certify an exact zero. Archimedean branch-cut and status policies are API conventions, not mathematical
claims about global functions; no arb/acb implementation theorem is asserted here.

Proof.

1. The valuation and absolute-value assertions follow from Definition 1. If v_p(a)<N, the two terms
   a and h in a+h have unequal valuations, so every a+h in the ball has valuation v_p(a). Otherwise
   a belongs to p^N Z_p and the ball is p^N Z_p, containing both 0 and p^N. Thus its valuation is
   not a single value. Its absolute values are bounded above by p^(-N) and include zero.
2. The real functions have the stated values by their definitions; if two represented arguments have
   different values a single one cannot be certified. Real and complex absolute values are nonnegative
   real scalars. The idele norm is a positive real scalar: its finite-place norm factors are 1 at all
   but finitely many primes. These output scalars are not local p-adic outputs of the same type as x.
3. A field denominator has a reciprocal exactly when it is nonzero. For an adele x with an inverse,
   every coordinate is nonzero. Both x and its inverse are integral almost everywhere, forcing
   v_p(x_p)>=0 and -v_p(x_p)>=0 almost everywhere. Thus x has unit coordinates almost everywhere,
   which is the idele condition. Conversely that condition makes the coordinatewise inverse an adele.
   Merely having no zero coordinate is insufficient: x_p=p at all finite primes is an adele, but
   its coordinatewise inverse is nonintegral at every prime and is not an adele.
4. The named-place equality follows by writing out the coordinates of projection and composition.
   An output that retained untouched coordinates elsewhere would not be this operation. For balls,
   taking the image means including every represented argument, which entails whole-ball domain
   certification. In particular p^N Z_p contains p^N as well as 0 and is not the exact singleton 0.
   The factors and their zero exclusions in the unit/Teichmueller row are those of Proposition 4;
   its explicit distinction at 2 still applies.

Check: `check_typed_and_projection`. Used by: SPEC 9.3.1, 9.3.6 and PLAN 1F.3.

## Statement index and proof status

Definitions have status "proved here" to mean explicitly specified and used with the given hypotheses.
Numerical checks do not certify the infinite or real analytic standard theorems. The two pending names
in Propositions 4 and 11 do not affect the proved formulas. No statement below is silently left open.

| Number | Content | Status | Check |
|---|---|---|---|
| 1 | Local and adelic meanings; six series | proved here | check_domains, check_global |
| 2 | Real analytic facts | proved modulo the theorems named in Lemma 2 | check_real_and_character |
| 3 | Complete-series criterion, simple lifting, cyclic finite groups | proved here | check_lifting_and_groups |
| 4 | Unique p^m w u; separate 2-adic sign | proved here | check_decomposition |
| 5 | Factorial valuation and denominator bounds | proved here | check_legendre |
| 6 | Exact domains, including excluded boundary shells | proved here | check_domains |
| 7 | Literal six-series term counts at any integer target | proved here | check_truncation |
| 7b | Tight count for log from k v - e(k) >= n; the default for 1F.7 | proved here | check_truncation |
| 8 | Denominator guard precision | proved here | check_working_precision |
| 9 | Convergent identities and exp/log inverse discs | proved here | check_series_identities |
| 10 | Isometries, safe cosine radii, exact centred hull | proved here | check_series_radii |
| 11 | Series log and Iwasawa images, including r=1 at 2 | proved here | check_log_radii |
| 12 | Domain D and finite Log enclosure | proved modulo Lemma 2 (real clause) | check_global |
| 13 | Local root existence, counts, square tests | proved here | check_root_criteria |
| 14 | Real roots | proved modulo Lemma 2 (intermediate value theorem) | check_real_and_character |
| 15 | Exact guarded branch image and failure outside guard | proved here | check_root_precision |
| 15r | Remark: at 2, odd degree roots need only r >= 1; not in SPEC 9.3.3 | proved here | check_root_precision |
| 16 | Global roots and rational contract | proved modulo Lemma 2 (real clause) | check_global_roots |
| 17 | Four power operations | proved modulo Lemma 2 (complex clause) | check_powers |
| 18 | Exact independent base/exponent image ball | proved here | check_power_precision |
| 19 | Unique p-primary fractional part and its precision | proved here | check_fractional_parts |
| 20 | Character and cosine | proved modulo Lemma 2 (complex exponential) | check_real_and_character |
| 21 | No ordered-field structure at any prime | proved here | check_no_order |
| 22 | Scalar outputs, invertibility, projections, ball domains | proved here | check_typed_and_projection |

### Statements proved in the lane documents (not copied into this file)

The lanes of milestone 1F proved the statements below in their own interface documents, stepwise, because this file
has no statement for them. They are NOT copied here (decision of 2026-09-30, lane i-repair1); this table names each,
the file and line where it is proved, and its review status as the reviews under `docs/reviews/f1/` give it.
Reviews: f-review1 = `docs/reviews/f1/review-lball.md` (2026-09-29, adf_lball); f-review2 =
`docs/reviews/f1/review-sball-rfunc.md` (adf_lball again, adf_sball, real functions); f-review3 =
`docs/reviews/f1/review-lfunc.md` (exp, log, Log at a prime). "No counterexample" means: the review tested the
statement by enumeration or by its own oracle and found none; it is not a proof by the reviewer unless it says
"read". "Not reviewed" means that no review under `docs/reviews/f1/` covers the statement.

`docs/api-1f.md`, slice 1F.3-a (adf_lball):

| Statement | Content | Proved at | Review status |
|---|---|---|---|
| L0 | rational to its canonical centre | api-1f.md:75 | f-review1 F5 (MINOR, sign); closed (f-review2) |
| L1 | projection of a finite ball to a prime | api-1f.md:86 | no counterexample (f-review1, f-review2) |
| L2 | sum of balls | api-1f.md:98 | no counterexample (f-review1, f-review2) |
| L5 | negation | api-1f.md:109 | no counterexample (f-review1, f-review2) |
| L3 | product of balls | api-1f.md:114 | read, no false step (f-review1); no counterexample |
| L4 | inverse of a ball | api-1f.md:134 | read, no false step (f-review1); no counterexample |
| L4a | quotient from valuations and precisions | api-1f.md:155 | f-review2 R7 (MINOR, centre); repaired |
| L6 | decomposition of a ball | api-1f.md:179 | no counterexample (f-review1, f-review2) |
| L7 | valuation and absolute value | api-1f.md:190 | no counterexample (f-review1, f-review2) |
| L8 | set predicates | api-1f.md:197 | no counterexample (f-review1, f-review2) |

f-review1 F1 to F3 (MAJOR) and f-review2 R1, R2 (MAJOR) concern the storage algorithm and the limit rule, not a
formula of L2 to L4a: the set formulas gave the right small ball. The rule is reworded by N-D7 (SPEC 15.4).

`docs/api-1f.md`, slice 1F.1-a and 1F.2-a (adf_sball, real functions):

| Statement | Content | Proved at | Review status |
|---|---|---|---|
| S1 | projection of an adele to a set of places | api-1f.md:284 | no counterexample (f-review2, 310 cases) |
| S2 | order and sets of places | api-1f.md:300 | no counterexample (f-review2) |
| S3 | componentwise ring operations | api-1f.md:305 | f-review2 R3 (MAJOR: masked LIMIT); repaired |
| S4 | set predicates of partial balls | api-1f.md:323 | no counterexample (f-review2: 1800 calls) |
| S5 | domains and statuses of the real functions | api-1f.md:336 | f-review2 R8 (MINOR: range); repaired |
| S6 | the image of a ball by the end points | api-1f.md:353 | no counterexample (f-review2: 3660 cases) |
| S7 | no non-finite ball with OK | api-1f.md:377 | no counterexample (f-review2) |

R3 concerns the code and the header (the combination of statuses of S3 at several places); R4 and R5 (entry checks
under ADF_CHECK_INVARIANTS; a `prec` of LONG_MAX) concern the code; all three are repaired by lane f-repair2.

`docs/api-1f.md`, slice 1F.3-b (lane f-slice3) and slice 1F.4-b (lane f-slice6):

| Statement | Content | Proved at | Review status |
|---|---|---|---|
| L9 | Newton lifting of the Teichmueller representative | api-1f.md:444 | not reviewed |
| L10 | the split of a ball | api-1f.md:464 | not reviewed |
| L11 | fractional part, unit modulo p^k | api-1f.md:486 | not reviewed |
| L12 | integer powers | api-1f.md:497 | not reviewed |
| L13 | early decision of LIMIT in `adf_lball_set_fball` | api-1f.md:531 | not reviewed |
| S8 | `f_at` at a prime is the function of `lfunc.h` on the component | api-1f.md:615 | not reviewed |
| S9 | the limit ADF_REAL_PREC_MAX | api-1f.md:632 | not reviewed |

`docs/api-1f4.md` (lane f-slice4; the file has F1 to F7, no F8; exp, log, Log at a prime):

| Statement | Content | Proved at | Review status |
|---|---|---|---|
| F1 | the domain test | api-1f4.md:72 | referee note, no failure (f-review3) |
| F2 | exact values | api-1f4.md:85 | f-review3 R2 (MINOR: prose beyond the proof); corrected |
| F3 | Log without the root of unity | api-1f4.md:102 | referee note, no failure (f-review3) |
| F4 | exp with one common denominator | api-1f4.md:118 | proof re-read, no failure (f-review3) |
| F5 | the sum of log, and a lower bound of the valuation | api-1f4.md:140 | proof re-read, no failure (f-review3) |
| F6 | the precision of a result | api-1f4.md:158 | referee note, no failure (f-review3) |
| F7 | limits | api-1f4.md:176 | referee note, no failure (f-review3) |

f-review3 R1 (MAJOR) is a cost finding (`log(1 + p)` at a prime of one word, N = 10000, 59 s), not a statement; the
brief of lane f-slice5 is written, and `lanes/f-slice5/` has no result. After F2 (api-1f4.md, f-review3 R2) it is
left undecided whether log or Log takes a nonzero rational value at a rational argument: `[source pending: a proof
that log and Log take no nonzero rational value at a rational argument]`; the code returns a ball there.

## Review record

Date: 2026-09-27. Reviewer: Claude opus. Review file: `docs/reviews/m0-proofs/functions-review.md`, with
the reviewer's checks in `docs/reviews/m0-proofs/functions_review_checks.py`. Verdict counts on the 22
numbered statements of that review: 20 VALID, 2 MINOR, 0 INVALID.

Changes applied, one line each:

- R-1 (Proposition 8, proof step 2): the division by p^e is now proved exact (R-1 text, verbatim).
- R-2 (Proposition 12, proof step 4): corrected citation: Proposition 4 and Lemma 9, not Proposition 11.
- O-1: added Proposition 7b, the tight count for log, with proof; Proposition 7's safe count is kept.
- O-1: the C code of PLAN 1F.7 uses the tight count by default (its proof is complete), safe is fallback.
- O-2: added Remark 15r (odd degree roots at 2 need only r >= 1) with stepwise proof, marked as not part
  of SPEC 9.3.3. (Not one line: this entry and the next were written as one item with a line break.)
- O-2: the guard of Proposition 15 is unchanged; SPEC 9.3.3 keeps its stronger guard.
- `check_truncation` now checks both log counts by enumeration and asserts T_tight <= T_log.
- `check_working_precision` now uses three inputs and three lifts, asserts the exact division of R-1 and
  the precondition W >= v, and records W-1 and W-without-D counterexample counts.
- `check_power_precision` now covers alpha above c at every prime and real content at p = 13 (k = 3).
- `check_root_precision` now uses five roots b and degrees 1 to 25 (including 9 and 25), tests the scaled
  image in both directions at p^3 sample points, and checks Remark 15r.
- `check_typed_and_projection` now exhibits two valuations in every NOT_DETERMINED ball.
- `check_regression_mutations` is replaced by `check_mutation_testing`: 28 planted wrong rules, each
  demanded rejected by the named check (red-green record in `lanes/m0-repair-functions/report.md`).
