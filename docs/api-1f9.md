# Slice 1F.9: arithmetic symbols

Proposed N-D18. Contract: include/adelefeld/symbol.h. Code: src/symbol.c.
The header records the choices and alternatives before implementation.

| Question | Choice | Alternative |
|---|---|---|
| Names and files | adf_<input>_<symbol>, symbol.h and symbol.c | separate symbol headers or input headers |
| Result | int pointer, status returned, optional where | symbol returned directly |
| Legendre lower entry | odd prime place, below 2^64 | arbitrary fmpz with prime certification |
| Other lower entries | fmpz, arbitrary bit lengths | word-only denominator |
| Finite certification | sufficient modulus divides stored N | Proposition 3 enumeration or sharper conductor |
| Limits | no extra integer cap; no enumeration or factorisation | bit cap or finite enumeration budget |

The functions take fmpz, adf_fball or adf_ucoset numerators. No symbol type is introduced.
On failure the value is untouched. Legendre names its supplied place; Jacobi and Kronecker name no place.
On success where is untouched. The input objects may coincide. Outputs cannot overlap any input.
The debug build checks canonical values on entry, as conventions 4.4 requires.

## Y1: exact quadratic symbols

The three exact routines compute Definition 1 of docs/proofs/catalogue.md:14-27.
The special conventions at 0, -1 and 2 are also on disk in
refs/src/pari-doc/usersch3.tex:9266-9271. This closes Definition 1's pending external convention source.
Jacobi is called only with positive odd lower entry, its documented domain in
refs/src/flint-3.0.1/fmpz.rst:1166-1168. Legendre is Jacobi at a prime.
Kronecker at a nonpositive lower entry uses FLINT's routine, whose domain is every pair of integers
(refs/src/flint-3.0.1/fmpz.rst:1170-1172). At a positive lower entry the supplement is explicit.
For odd a, residues 1 and 7 modulo 8 give +1 and 3 and 5 give -1. Even a gives 0 when 2 divides b.
An even exponent of the supplement gives +1 on odd a. These follow by substituting into Definition 1:18.

Check: residue_vectors, residue_identities, residue_statuses_and_witnesses in tests/test_symbol.c.
The fixture oracle factors the lower entry and applies Euler's criterion; it also counts squares
for primes through 65537. Large rows use Euler's criterion, without claiming full square enumeration.

## Y2: finite certification

For positive lower entry let K be Proposition 2's sufficient modulus (catalogue.md:31-39).
If the input is a+N Zhat or c U(N), and K divides N, all its points have the same residue modulo K.
Thus Proposition 3:43-57 proves that the exact centre symbol is the whole set's symbol.
Exact inputs use Y1, including the exact units [1] and [-1].
For positive radius and nonpositive Kronecker lower entry the result is NOT_DETERMINED.
No real sign or ordinary-integer equality can be inferred from a profinite congruence.

The brief says "determines it exactly when that modulus divides N; otherwise NOT_DETERMINED".
The proof says "These are sufficient moduli, not claims of minimality" (catalogue.md:33) and
"This condition is only sufficient; a coarser input can also happen to give one value" (:55-56).
The slice follows the brief's conservative algorithm and does not assert mathematical necessity.
For example, every point of 1+3 Zhat is a unit modulo 3, so its Jacobi symbol at 9 is +1.
The modulus 9 does not divide 3. There cannot be two points of that ball with different symbols.
In contrast, 1 and 5 belong to 1+4 Zhat and give opposite Kronecker symbols at 2.
The points 1 and 2 belong to 1+Zhat and give opposite Legendre symbols at 3.

Normalising a unit coset removes a factor 2 only when N=2 mod 4. Each K above is odd or divisible by 8.
Removing that factor cannot change K|N. Certification therefore respects equal coset sets.

Check: all finite residue_vectors and the hand ambiguity and constant-case tests.

## Y3: nonintegral finite balls

Write the canonical triple as (A+H Zhat)/d. A point is in Zhat exactly when A+Ht=0 mod d
for its profinite parameter t. Such a point exists exactly when gcd(H,d) divides A:

1. Every Ht modulo d is divisible by g=gcd(H,d), so solvability implies g|A.
2. If g|A, Bezout's identity for H/g and d/g gives an integer t satisfying the congruence.
3. If d>1, canonicality implies that d does not divide both A and H. Therefore some residue t
   has A+Ht nonzero modulo d. The ball then also contains a point outside Zhat.
4. If g does not divide A, every point is outside: DOMAIN. Otherwise both kinds of points exist:
   NOT_DETERMINED, as conventions 3.1 requires. For d=1 all points are in the domain.

Check: fixtures at (1,2,2), (0,1,2), (1,0,2), for all three symbols.

## Y4: scope of denominator multiplicativity

Definition 1 at b=0 gives (-1/0)=1, while (-1/3)=-1. Thus
(-1/(0*3))=1 differs from (-1/0)*(-1/3)=-1.
The brief's unqualified denominator-multiplicativity request fails at zero under the stated definition.
The tests check the law for nonzero factors, retain all zero-denominator definition rows, and assert this
counterexample explicitly. The first identity run exhibited 15 such failures; the report keeps that result.
This is a finding against the brief, not a change to the specification or the oracle.
The PARI source also says "total multiplicativity in both arguments" at usersch3.tex:9263;
its explicit rules at :9266-9271 have the same zero-denominator exception.

Check: residue_identities, residue_statuses_and_witnesses.

## User calls

Driver: legendre X with P; jacobi X with B; kronecker X with B.
X is an integer rational text, finite ball or unit coset. B is an integer text; P is an odd prime below 2^64.
The result is a decimal sign or zero, or error: STATUS. Julia calls the same exported functions via ccall.

## Y5: Hilbert symbols and square classes

The four Hilbert routines use Definition 4 (catalogue.md:63-78), the odd formula in Proposition 5:82-99,
and the 2-adic formula and full square-class table in Proposition 6:101-133.
The real symbol is negative exactly for two negatives; the proof is catalogue.md:77-78.
Rational signs are read exactly, without converting them to real balls.

For a nonzero rational A/B, remove p from A and B separately. Its valuation parity is the xor of the
two nonnegative integer valuation parities. Its unit is the ratio of the stripped integers modulo p,
or modulo 8 at 2. The stripped denominator is invertible at that modulus. This proves that reduction
uses the correct square class, also for negative valuations, without subtracting large valuations.
For a local ball the stored unit and valuation give the same data directly.

The implementation uses full-word modular multiplication and inverses; their domains are
refs/src/flint-3.0.1/ulong_extras.rst:375-379 and :477-483. The removal routine's contract is
refs/src/flint-3.0.1/fmpz.rst:1142-1146. No factorisation or power p^N is formed.

Check: hilbert_vectors (608 primitive-solution rows, all used by rational, local and idele routines),
hilbert_identities, hilbert_word_primes_huge_and_extreme.

## Y6: why the finite solvability oracle decides the local question

The oracle searches primitive triples modulo p^2 for odd p and modulo 16 for p=2, after reducing
coefficient valuations to 0 or 1 and units modulo p or 8. It groups equal square residues together
with the flag that the coordinate is a unit. Grouping preserves exactly the primitive-triple test.
It does not evaluate a Hilbert formula or import proto/catalogue_checks.py.

The relevant lifting theorem is on disk: refs/src/thorne-padic/jackthornenotes.txt:874-877,
Lemma 4.12, states the strict inequality |f(alpha)| < |f'(alpha)|^2 for a monic polynomial.
Dividing by a unit leading coefficient makes each one-variable polynomial used below monic,
and preserves valuations of its value and derivative. The simple version is :574-580.
This closes the Hensel source pending in catalogue.md:95 and :129 for these applications.

1. Multiplying either coefficient by a square changes neither solvability nor the symbol.
   Dividing out even valuations leaves valuation 0 or 1. For odd p, units congruent modulo p differ
   by a square: apply simple Hensel to X^2-u at 1 for u=1 mod p. At 2 use the strong theorem with
   u=1 mod 8; the value has valuation at least 3 and the derivative valuation 1. Thus the unit
   representatives used by the oracle cover the same local square classes.
2. A local nonzero solution scales to a primitive integral triple, which reduces to a primitive
   modular solution at every modulus. Therefore absence of a primitive modular solution proves -1.
3. At odd p with both valuations 0, the square residues and the set u*x^2+w each have (p+1)/2
   elements in F_p. They meet. With y=1, x and z cannot both vanish since w is a unit. A derivative
   in x or z is a unit. Simple Hensel lifts. Thus local solvability always holds in this case.
4. At odd p with valuations (1,0), reduction gives w*y^2=z^2 mod p. If w is a nonsquare, y and z
   are divisible by p. Modulo p^2 then forces x divisible by p, contradicting primitivity.
   If w is a square, x=0,y=1 and a square root z lift. The interchanged case is the same.
5. With odd-p valuations (1,1), z is divisible by p. Dividing the equation by p modulo p^2 gives
   u*x^2+w*y^2=0 mod p with a unit coordinate. A unit derivative lifts this with z=0.
   Hence a primitive solution modulo p^2 suffices in all four valuation cases.
6. At 2, take a primitive solution modulo 16. If an odd coordinate has an odd coefficient,
   the derivative in that coordinate has valuation 1; the value has valuation at least 4.
   Strong Hensel lifts, with the other two coordinates fixed.
7. If no odd coordinate has an odd coefficient, an odd coordinate only at one even coefficient
   contradicts the equation modulo 4. The remaining case has both coefficients even, x and y odd,
   and z=2z'. Divide the equation by 2. Its value is 0 modulo 8 and its derivative in x or y has
   valuation 1. Strong Hensel again lifts. The result is nonzero because the lifted coordinate is odd.
8. These implications prove that 9, 25 and 16 suffice for the reduced coefficients of the requested
   oracle, and p^2 suffices for every odd p. The numerical search is limited to p=2,3,5,7.
   Modulo 8 is insufficient at 2: (2,6) has the primitive solution (1,1,0) modulo 8, but its
   exhaustive modulo-16 search has no primitive solution.

Check: primitive_solution in proto/symbol_checks.py; hilbert_vectors, hilbert_finite_classes.

## Y7: finite precision, zero and the scale cofactor

For a ball excluding zero, valuation parity is fixed. At odd p its unit residue modulo p is fixed.
At 2 its known digits select a subset of {1,3,5,7} modulo 8. Every selected unit class lifts to a
point in that ball (choose an odd integer unit in that class with the prescribed known digits).
Evaluate every pair of admitted classes. A singleton sign is the exact value; two signs mean
NOT_DETERMINED. This is Proposition 7, catalogue.md:135-165, including its sufficient-only bounds.

For an idele the scale r fixes the valuation. Its unit is r' times a point of the unit coset,
where r'=r*p^(-v_p(r)). Proposition 7:142-160 proves the permitted unit classes and their independence.
The implementation multiplies by the residue of this cofactor. For scale 3 and exact unit 1 in both
arguments at 2, both units are 3. Their epsilon terms give -1. Ignoring the cofactor would give +1.
At an odd prime both even valuations give +1 for every unit choice.

Exact zero in either argument makes every pair of points outside the nonzero domain: DOMAIN.
A zero-containing ball and a nonzero input meet the domain and its complement: NOT_DETERMINED.
Exact zero dominates a mixed ball by conventions 3.3. Different local primes violate the stated
compatibility condition: DOMAIN, with the smaller prime as the report. All failures preserve the value.
Real balls follow the same zero rules. Canonical ideles exclude zero, so their only failure is
NOT_DETERMINED. No LIMIT applies: only parity and at most three unit digits are required, even for
compact local values with v or N outside the local-arithmetic limits.

Check: hilbert_finite_classes (1920 idele pairs and 800 local pairs), hilbert_statuses_and_precision.
The complete oracle checks both sides of the precision boundary and the cases where missing digits
are irrelevant. The witnesses 1 and 5 in 1+4 Z_2 against 2 give different signs; 3 and 9 in 3 Z_3
against 2 give different signs. For unknown scale-1 units at 2, (1,1) and (3,3) give different signs.
The 4097-bit square coefficients and endpoint-exponent compact inputs are checked separately.

## Y8: identities and the product formula

Bilinearity, symmetry and the two identities are stated on disk in
refs/src/hilbert-bristol/lecture19.txt:19-24 and :81-84, and follow from the formulas proved in
catalogue.md Propositions 5 and 6. The product formula is the statement of Theorem 3 on disk
in refs/src/hilbert-bristol/lecture19.txt:89-91. Catalogue Proposition 8:167-180 gives a proof
conditional on quadratic reciprocity and its supplements; that proof still marks their source pending.
This slice cites the on-disk product theorem, and tests its full finite support for 2000 rational pairs.
No product-formula operation is exported. Arbitrary pairs of ideles need not have product +1.

The Bristol text transcription is not used as the formula oracle. Its odd-prime display at :47-51
attaches exponent alpha to the u factor and beta to the w factor. Catalogue.md:68 attaches beta to u
and alpha to w, as its proof requires. For a=3,b=2,p=3, the catalogue and solvability search give -1;
the transposed display gives +1. Its 2-adic display at :52-57 also has extra Legendre factors.
These are source/transcription findings; the formula proofs and independent search govern this slice.

Check: hilbert_identities; hilbert_product_2000 (2000 pairs, 12058 finite factors, 2000 real factors).
A single wrong local sign or failure to return OK fails a case. Identities alone are not the value oracle.

## Second group: proposed N-D19 (f-slice13)

Contract: include/adelefeld/catalogue.h; implementation: src/catalogue.c.
The header records each slice's decisions before its implementation. Existing types only.
Every status other than OK preserves the value. where is optional and always untouched, since these
operations inspect the finite part without selecting a prime. Same-type output/input aliases are allowed.

| Question | Choice | Alternative |
|---|---|---|
| Files | catalogue.h and catalogue.c | four headers, or additions to input headers |
| Policies | separately named conservative and tight operations | one policy argument |
| Binomial degree | ulong, conservative <=4096, tight <=256 | fmpz or unbounded degree |
| Nonintegral input | DOMAIN if disjoint from Zhat, otherwise NOT_DETERMINED | rational polynomial extension |
| Limits | k limit before domain; arbitrary input bit lengths | output bit budget or caller budget |

### Y9: binomial enclosures and smallest ball

Both functions compute Proposition 14, docs/proofs/catalogue.md:291-312.
For a canonical integral input a+N Zhat, the centre is binom(a,k). The conservative radius is
N/gcd(N,k!). The smallest radius is the gcd of the k differences in :296. For N=0 or k=0 it is 0.
The output constructor reduces the centre modulo the radius; this represents the same set.
The signed centre is evaluated by B_0=1 and B_i=B_(i-1)*(a-i+1)/i.
For nonnegative a this counts i-subsets. For a=-b<0 the falling product is
(-1)^i b*(b+1)*...*(b+i-1), giving (-1)^i binom(b+i-1,i). Thus every exact division is integral.
The recurrence admits centres of arbitrary bit length; fmpz_bin_uiui only admits a word-sized top entry
(refs/src/flint-3.0.1/fmpz.rst:1006-1008). No downcast of a is used.

Check: binomial_vectors uses all rows from the independent polynomial/period oracle.
The period proof is in proto/catalogue2_checks.py:3-11. It does not import the earlier checks.
The PLAN case gives radius 2. The case a=2,N=3,k=2 gives radius 9, so the smallest radius can exceed N.
The conservative radius there is 3. Limits, signed points, local backend and huge integers are separate tests.

### Y10: the integral-input domain

The proof is the congruence argument of Y3 above: (A+Ht)/d is integral for some t exactly when
gcd(H,d)|A. If d>1 and the triple is canonical, at least one of A,H is not divisible by d,
so not every t is integral. Disjoint inputs give DOMAIN; mixed inputs give NOT_DETERMINED.
The input (1/2) Zhat contains 0 and 1/2, witnesses of both cases. This rule also applies at k=0.
It specifies the stated Zhat domain, rather than extending the polynomial to rational inputs.
There is no NOT_DETERMINED boundary among wholly integral inputs: both enclosures always exist.

Driver: binom X with K; binomtight X with K. X is an integer or finite ball. K is nonnegative.
The result uses finite-ball value text, for example (* ; 0 mod 2). Julia calls the same exported functions.

### Y11: profinite target and coarser divisor

Definition 11 and Proposition 12 are docs/proofs/catalogue.md:218-241.
Normalise the input unit coset before comparing moduli. For M>0 calculate c^M modulo N and
D=gcd(N,c^M-1) from that residue. Replacing c^M by its residue changes c^M-1 by a multiple of N,
so it preserves the gcd. Strict succeeds exactly when D=N; coarse always encloses in c^e U(canon(D)).
For M=0 use the default integer power when e fits slong, otherwise modular powering of |e| with
the inverse for negative e. Exact zero returns [1]. These are the same sets as idpow.h's default powers.
The exponent is an adf_fball. Its integral-domain test is Y10, applied before any exact-base shortcut.
For [1] every power is [1]. For [-1] only parity matters; M even fixes it and M odd admits both signs.
The latter two-point set has differences 2; its smallest unit coset is U(2)=U(1).
Strict reports NOT_DETERMINED, while coarse and fine return [1 mod 1].

The default strict name is adf_ucoset_profpow. Alternatives: a coarsening default or a policy argument.
Coarsening is adf_ucoset_profpow_coarse; finest is adf_ucoset_profpow_fine.
All exponent data have arbitrary bit length. Only finest has the g<=256 limit; it follows the domain check
and constant exact cases. This cap avoids unbounded factorisation/divisor enumeration of g.
Alternative limits: a caller budget, supplied factorisation, or an unbounded operation.

Check: 1362 independent oracle rows enumerate full unit/exponent images at a common finite level.
The exponent period is computed by multiplication, rather than a totient or unit-group formula.
Two exponent points 0 and 2 at base residue 2 mod 5 give 1 and 4, proving the rejected target ambiguous.
The exact integer path is also compared with existing pow and pow_tight for e=-6..6.

### Y12: finest modulus without factoring N

This is Proposition 13, catalogue.md:245-287, written as two coprime blocks:

1. Let g=gcd(e,M)>0. Repeatedly divide the remaining g by its gcd with N, accumulating the factors.
   At a prime p|N each step removes min(v_p(remaining),v_p(N)), until none remain. Thus the accumulated
   number g_N has v_p(g_N)=v_p(g) for p|N and 0 elsewhere.
2. Put A0=N*g_N. For M>0 put A=gcd(A0,c^M-1), computing the power modulo A0.
   At p|N this has exponent min(v_p(N)+v_p(g),v_p(c^M-1)), the first row of Proposition 13.
   For M=0 put A=A0, since c^0-1=0.
3. Compute tight(U(1),g) by the existing adf_ucoset_pow_tight. Its modulus has the other two rows
   of Proposition 13, with a lone factor 2 already removed. Repeated division by gcd(B,N) removes
   exactly all its prime powers at primes dividing N. The remaining B is the outside-prime block.
4. A and B are coprime. The centre is c^e modulo A and 1 modulo B. Use CRT only if A,B>1;
   when either is 1 the other residue suffices. Then normalise A*B. This gives canon(F) and its centre.
5. Exact nonzero e with |e|<=256 directly reuses pow_tight; exact 0 is the constant [1].

The CRT routine's domain is refs/src/flint-3.0.1/fmpz.rst:1292-1303. Signed powers use :923-929
and invmod :1154-1160. The reused tight routine cites the n_factor and n_is_prime contracts in
refs/src/flint-3.0.1/ulong_extras.rst:1126-1130, 1203-1216 and 833-840.
No factorisation of N or integer c^M is formed. A proposed 32-bit N cap was removed before implementation
when this gcd construction made it unnecessary; arbitrary-size N is tested by all three operations.

Driver: profpow A with X; profpowcoarse A with X; profpowfine A with X. A is a unit coset;
X is an integral finite ball or exact integer. Julia calls the same exports. The PLAN vector gives [49 mod 120].

### Y13: Haar volume and existing content

Reuse adf_fball_haar_volume(adf_rat_t,const adf_fball_t), already declared in fball.h:188 and
implemented in src/fball.c:618-625. It returns void, since every canonical finite ball has an exact volume.
Alternative: a new fmpq wrapper or a status with where. Neither adds a mathematical operation.
No output/input alias exists between these types. All rational centres and radii are admitted.
The result is d/H for (A+H Zhat)/d with H>0, and 0 for H=0. It handles the local backend too.

Proposition 10 is catalogue.md:202-212. Here is an index proof that needs no local scaling theorem:

1. Zhat/d Zhat has d residues for every integer d>0. Reduction modulo d gives them and its kernel
   is d Zhat; prime by prime, multiplication by the unit cofactor of d preserves Z_p.
2. Zhat is the disjoint union of H translates of H Zhat. Translation invariance and vol(Zhat)=1
   give vol(H Zhat)=1/H. Translation invariance is the Haar definition on disk,
   refs/src/tate-poonen/notes.txt:370-380.
3. Multiplication by H/d identifies the quotient (H/d Zhat)/(H Zhat) with Zhat/d Zhat.
   The first group is therefore the disjoint union of d translates of H Zhat, of total volume d/H.
4. Translation preserves the volume of a+(H/d) Zhat. A point lies in a+n Zhat for every n>0,
   whose volume is 1/n. Monotonicity forces its volume to be 0.
5. For an idele r*u, the valuation at p is v_p(r), since u_p is a unit. Thus the finite ideal is
   r Z and its positive generator is r. This is Proposition 10:207-208; the decomposition is quoted
   from refs/src/milne-cft/CFT.txt:9853-9858. The existing adf_idele_content is tested and not duplicated.

Check: haar_and_existing_content, including 1001 centre/radius/denominator triples, huge rational radius,
local backend and the existing content 15/14. The driver haar_volume X and Julia use the same accessor.

### Y14: the two cyclotomic exponents

Names are adf_idclass_cyclo_exp_u and adf_idclass_cyclo_exp_uinv, fixed by conventions 6.6, CV-53 and M0-D10.
The output is fmpz modulo the original positive fmpz order n. Alternatives: word order or a convention argument.
The output may equal the complete input n; no output overlaps the class or one of its fields.
No resource cap or real working precision applies. n<=0 gives DOMAIN, with output and where untouched.
Otherwise the precision certificate is exactly Proposition 15, catalogue.md:318-343:
an exact unit always works; at finite precision canon(n) must divide canon(N), else NOT_DETERMINED.

The on-disk attribution is refs/src/milne-cft/CFT.txt:9883-9886:
"the global reciprocity map is the reciprocal of" the canonical isomorphism.
The canonical exponent convention is specified in :3162-3166 and conventions 6.6.
We do not use the arithmetic/geometric names of catalogue.md:320, whose attribution is still pending at :342-343.
This follows the newer SPEC row and CV-53; it is a naming disagreement in the proof, not a formula change.

The precision test uses canonical moduli, but the result is modulo the original n. If n=2m with m odd,
the determined residue modulo m has exactly one odd lift modulo 2m. Add m to its even representative.
This is the exponent of a unit modulo n. Inverting it gives the inverse-exponent convention.
For n=1 the sole residue is 0; FLINT's invmod explicitly admits modulus 1
(refs/src/flint-3.0.1/fmpz.rst:1154-1160). Temporaries preserve n when j aliases n until the final write.

The specification vector is catalogue.md:324-326 and SPEC 9.3.7's cyclotomic row:
"the idele with p at the place p and 1 elsewhere has u' = 1/p away from p".
With p=3, its unit is 5 modulo 7 and 1 modulo 9, thus 19 modulo 63. The inverse convention returns
3 modulo 7 (Frobenius) and 1 modulo 9 (trivial action). Direct convention gives 5 modulo 7 and 1 modulo 9.
The external vector is refs/src/milne-cft/CFT.txt:9904-9908, with Frobenius in :1307.
The tests construct this as an idele of content 3 and convert it to a class before evaluating the action.

Check: 2308 oracle rows enumerate every unit lift at lcm(N,n), with direct modular inversion;
cyclotomic_spec_boundary_huge adds the vector, odd lifts and thousands-bit n and N.
At N=3,n=9 the input contains unit residues 1 and 4 modulo 9, with inverse residues 1 and 7.
These are the two points proving ambiguity one digit below the precision boundary.
Driver names are cyclo_exp_u and cyclo_exp_uinv; Julia calls the same exports.

### Y15: elementary precision proof for the finest table

Catalogue.md:272-278 marks two external theorems pending. This proof supplies the needed valuation
and exponent claims directly, without citing either theorem from memory.

1. If v_p(z)=n, with p odd and n>=1, or p=2 and n>=2, then v_p((1+z)^p-1)=n+1.
   In the binomial expansion the first term p*z has valuation n+1. For 1<i<p, the coefficient
   binom(p,i) is divisible by p, so the valuation is at least 1+i*n>n+1. The last term has
   valuation p*n>n+1 under precisely these hypotheses. For p=2 there are no middle terms.
2. If m is prime to p, expansion gives v_p((1+z)^m-1)=n: its first term m*z has valuation n,
   and every later term has valuation at least 2n. Iterating step 1 and then step 2 gives
   v_p((1+z)^g-1)=n+v_p(g) for every positive integer g. Negative powers have the same valuation
   because u^(-g)-1=-(u^g-1)/u^g. Therefore the principal-unit depth at p|N is n+v_p(g),
   with the sharp witness 1+p^n. Combining with c^M=1 gives Proposition 13's inside-prime row.
3. At an odd prime, multiplication by b permutes the nonzero residues. Cancelling their product
   gives b^(p-1)=1 modulo p. The exponent E of this finite abelian group is realised by one element:
   for each prime divisor of E, take an element whose order has maximal prime valuation and raise
   it to remove the coprime part; the product of these commuting elements has order E.
   Every nonzero residue is a root of X^E-1, so p-1<=E by the polynomial root bound.
   Since each order divides p-1 by the permutation identity, E divides p-1, hence E=p-1.
4. Thus constancy of all unit g-th powers modulo p requires p-1|g. If this holds, each b^(p-1)
   lies in 1+p Z_p. Step 2 gives constancy modulo p^(1+v_p(g)). The unit 1+p attains exactly
   this depth and fails at the next digit. This proves the outside odd-prime row.
5. At 2 all units are odd. For odd g the unit 3 distinguishes modulo 4, so depth 1 is maximal.
   For even g, every odd b has v_2(b^2-1)>=3, since b-1 and b+1 are consecutive even numbers,
   one divisible by 4. Step 2 gives b^g=1 modulo 2^(2+v_2(g)). The unit 5 attains equality
   in this depth, again by step 2 with n=2. This proves the outside 2-adic row.
6. The exponent class e+M Zhat makes base variation trivial exactly when both its e-th and M-th
   powers are trivial. Bezout's identity makes this equivalent to triviality of the g-th power,
   g=gcd(e,M). Conversely g divides both exponents. Density and continuity extend the integer
   argument to every profinite exponent. Outside N the base 1 is allowed, so any fixed output residue
   must be 1. CRT combines these independent local conditions, giving precisely Proposition 13.

This proves the precision table used by Y12; no logarithm evaluation or unproved group-exponent formula
is needed by this implementation. The unused arithmetic/geometric name attribution remains pending.

## Hilbert user calls

Driver: hilbert_at X with Y with PLACE. PLACE is real or a prime. X and Y have the same kind:
exact rationals, ideles, local balls, or partial balls. A partial ball supplies its named component;
a missing component is DOMAIN, a complex real component is UNSUPPORTED. A local ball must have the
named prime. Examples: hilbert_at 3 with 3 with 2 prints -1;
hilbert_at [p=2: 1 + O(2^2)] with [p=2: 2] with 2 reports NOT_DETERMINED.
The real-ball form uses partial-ball text, for example {inf: -3}.
Library calls accept any permitted pair of their declared type; inputs may coincide.

## Local zeta factors (N-D20, lane f-slice14)

Decision N-D20 (orchestrator, from docs/design/local-zeta.md section 3): one function
`adf_local_zeta_factor_at(y, where, s, v, prec)` on raw `acb_t`, the name of conventions CV-60 (line 953),
in include/adelefeld/localfactor.h and src/localfactor.c. The place handle selects the prime or the real place;
primes are the word primes of place.h. No reciprocal, no product over places, no character (milestones 3, 5).

### Y16: the value and its certificate

The function returns, for the closed rectangle S = [x - rx, x + rx] + i [y - ry, y + ry] of the input, an enclosure
of the image of L_v on S: L_p(s) = (1 - p^(-s))^(-1) = 1/(1 - exp(-s log p)) at a prime
(refs/src/tate-poonen/notes.txt:1733 for Re(s) > 0, continued meromorphically by design Z1), and
L_inf(s) = pi^(-s/2) Gamma(s/2) at the real place (notes.txt:1014-1016; continuation, poles and absence of zeros
of Gamma at :62-64; design Z2). The poles are 2 pi i k / log p at p and 0, -2, -4, ... at infinity, all simple
(Z1, Z2). The exact dyadic poles are 0 at p and the non-positive even integers at infinity (Z3; at p this rests on
the pending Gelfond-Schneider source, which the code does not use: it recognises only the exact 0, and an exact
point it cannot separate from a pole is NOT_DETERMINED). The procedure is the decision rule Z4 with its proof of
soundness (local-zeta.md:107-170), at the working precision w = max(2, prec) + 32, rounded outward to
max(2, prec) bits at the end. No relative accuracy and no smallest ball are promised.

The steps the code adds to the design's procedure, and why each keeps the enclosure:

1. At p, the variation radius E+ of Z4 step 3 is computed in mag arithmetic: a lower bound of a |x| as the
   lower product of lower bounds of log p and |x| (arb_get_mag_lower, arf_get_mag_lower, mag_mul_lower), then
   mag_expinv, an upper bound of exp(-t); an upper bound of a R from the upper bound of log p and mag_hypot(rx, ry)
   (an upper bound of the Euclidean radius R, not max(rx, ry)), then mag_expm1, an upper bound of exp(t) - 1.
   Every rounding enlarges E+, so E+ >= exp(-a |x|)(exp(a R) - 1) and proof step 2 applies.
2. The intersection of D0 with 1 - T0 is made per component and only when arb_intersection reports an overlap;
   otherwise D0 is kept. Both enclose 1 - exp(b m), so either set, and their intersection, encloses it.
3. At infinity the integer geometry of Z4 step 2 uses the endpoints of Re(S) rounded outward at w bits
   (arb_get_lbound_arf, arb_get_ubound_arf), halved exactly. The test is: an imaginary interval that excludes 0
   excludes every pole; else [lo, min(hi, 0)] contains an integer iff floor(min(hi, 0)) >= lo (arf_floor and
   arf_cmp, exact for every exponent, arf.rst:39). The outward interval contains the true one, so it can only
   find more integers: rounding can cause a refusal, never an acceptance.
4. The shift of Z4 step 4 is n = 1 - floor(lo) for lo < 1, else 0, from the outward lo <= min Re(S/2), so
   min Re(S/2 + n) >= lo + n >= 1 holds; n is at most one more than with exact endpoints. lo below -64 gives
   n > 64 at once, without forming floor(lo) of a large number.
5. The product Q = z (z+1) ... (z+n-1) is a loop of acb_mul, each of which encloses its product, so Q encloses
   Q(t) for every t of z = S/2; Gamma(z+n)/Q encloses Gamma(z) by the recurrence (Z2 step 4) or is not finite.
   acb_rising_ui is not used: on FLINT 3.0.1 it returned for z = -2 +/- 0.05 + i (0.8 +/- 0.05), n = 6, a ball
   of radius about 110 containing 0, where the loop excludes 0 (lanes/f-slice14/probes/rec_probe.c).
6. The derivative bound B of Z6 (step 7) is computed with every quantity rounded so that B grows:
   A >= lo + n rounded down; ceil(B0) <= ceil(hi + n rounded up) <= 64; 1/A and the factorials as mag upper
   bounds; delta_j >= sqrt(dx^2 + dy^2), dx and dy lower bounds of the distances of Re(z) from -j and of Im(z)
   from 0 (arb_get_mag_lower; Euclidean distance to the rectangle is at least this), with mag lower arithmetic;
   P as the lower product; sum 1/delta_j with mag_inv upper bounds; pi^(-min Re(S)/2) <= exp(-lo log pi).
   A delta_j bound of 0 makes B infinite, and then the refinement is skipped.
7. The midpoint value of Z4 step 5 is the candidate procedure (direct Gamma, then the recurrence with the
   midpoint's own shift, at most n) on the exact midpoint, which lies in the pole-free rectangle. Enlarged by R B
   in both components it encloses the image (Z6 step 1); intersecting with the candidate per component, when
   the two overlap, keeps an enclosure. A non-finite midpoint value or B leaves the candidate unchanged.
8. Every result is computed into a temporary, rounded outward (acb_set_round), tested with acb_is_finite, and
   only then swapped into y.

Check: test_localfactor: prime_exact_values (s = 1, 2, -1 at p = 2, 3, five precisions), prime_pole_lattice
(42 poles 2 pi i k / log p, k = -3..3, six primes up to 2^64 - 59, radii 10^-1 .. 10^-60: 1260 calls),
real_exact_values (every integer -41..41 that is not a pole, against closed forms without Gamma),
real_poles (0..-40, 630 calls),
real_recurrence (the design's direct-Gamma counterexample), prime_vectors and real_vectors (the 273 rows of
tests/ref/vectors/f-slice14/zeta.jsonl; with ADF_ZETA_FIXTURES the 1371 rows with an input of the full
oracle file: 701 OK rows, 5845 certified samples contained, 643 width checks at factor 64, 0 missed, every
status equal to the simulation). Driver: tests/driver/localfactor-values.cmd. Julia: tests/julia/localfactor.jl.

### Y17: statuses, outputs, aliasing, limits, cost

Statuses, in the order of the checks: LIMIT for prec above ADF_REAL_PREC_MAX = 2^21, before every other check;
then (debug build only) the entry check that v is the archimedean place or a certified prime (a forged handle
aborts, conventions 4.4); DOMAIN for a non-finite input (acb_is_finite false); then at p: DOMAIN for the exact
0, NOT_DETERMINED for a denominator enclosure that contains 0 or a non-finite bound, value or rounded value;
at infinity: DOMAIN for an exact non-positive even integer with imaginary part exactly 0, NOT_DETERMINED when
the geometry finds a possible pole, LIMIT when direct Gamma is not finite and the shift needs more than
ADF_LOCAL_ZETA_SHIFT_MAX = 64 factors, NOT_DETERMINED for a non-finite candidate or rounded value. Otherwise OK.
No other status is returned. The row of conventions.md:223 does not list LIMIT yet; the orchestrator adds it
with N-D20 (brief of lane f-slice14, decision 3). A ball containing a pole is never OK (Z4 proof step 3 at p,
step 5 at infinity); on OK the ball is finite and contains L_v(t) for every t of S.

Outputs: on OK y is written once and where is untouched; on every other status y is untouched (its
representation: tests compare with acb_equal against a nontrivial sentinel) and *where = v, also for LIMIT;
where may be NULL. Aliasing: y may be s; where overlaps neither. Limits: the precision cap and the 64 factors;
every prime below 2^64; no cap on the size of s (Re(s) = +-2^1000 and Im(s) = 2^1000 at p = 2 are tested; at the
real place 2^1000 + i is NOT_DETERMINED on FLINT 3.0.1 because acb_gamma returns a non-finite ball).
FLINT 3.0.1 behaviour that the statuses depend on: acb_gamma of the design's box [-2.1, -1.9] + i [1.5, 1.7]
halved is non-finite at 48, 160 and 288 bits, as the design observed on 3.3.1, so the recurrence gives the OK
value; the same holds for the boxes of the recurrence limit (lanes/f-slice14/probes/gamma_probe.c).
Cost: at p one arb_log_ui, one acb_exp and one acb_expm1 at the midpoint, a few mag operations and one complex
division (or a division and a negation); no loop over poles or over Im(s). At infinity endpoint arithmetic, one
acb_exp and one acb_gamma; when Gamma is not finite a second acb_gamma and at most 64 products; for a ball of
positive radius with n <= 64 and max Re(S/2 + n) <= 64 at most two further acb_gamma calls (the midpoint) and the
O(n) bound of Z6. Measured: test_localfactor, 37005 checks, about 3 s (one core); the call at the cap
prec = 2^21 at p = 2, s = 1 is part of it. Avoidable cost noted: the midpoint refinement evaluates Gamma at the
midpoint even when the candidate is already narrow, and computes the prefactor exp(-z log pi) a second time.

Check: prime_statuses_and_outputs (exact 0, NaN and infinite components and radius, LIMIT before DOMAIN, prec
-5, 1, 2, the cap, where on OK and on failure), prime_handles (0, 1, 4, 9, 65535, 2^64 - 1, 2^64 - 57 refused by
adf_place_prime, which leaves its output untouched; 2^64 - 59 accepted), prime_extreme_arguments, real_poles,
real_recurrence (n = 64 not LIMIT, n = 65 and 67 LIMIT, LIMIT before DOMAIN), debug_entry_check (INV=1), and the
helper call() of the test, which runs every call three times (separate y, y = s, where = NULL) and checks the
outputs after each status. Driver: tests/driver/localfactor-status.cmd.

## Local zeta user calls

Driver: `local_zeta_factor_at S with PLACE`; S is the text of a complex adele, the carrier of s (its finite
coordinate is ignored); PLACE is real or a prime. The line is the complex ball `(re) + (im)*i` or `error: STATUS`.
At prec 128 and digits 5, `local_zeta_factor_at ((2) + (0)*i ; 0) with 2` prints `(1.3333 +/- 3.4e-5) + (0)*i`.
Julia calls the export with a 96-byte acb (the documented target ABI) and a cadele as text carrier.
