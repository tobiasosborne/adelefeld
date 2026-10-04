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

## Hilbert user calls

Driver: hilbert_at X with Y with PLACE. PLACE is real or a prime. X and Y have the same kind:
exact rationals, ideles, local balls, or partial balls. A partial ball supplies its named component;
a missing component is DOMAIN, a complex real component is UNSUPPORTED. A local ball must have the
named prime. Examples: hilbert_at 3 with 3 with 2 prints -1;
hilbert_at [p=2: 1 + O(2^2)] with [p=2: 2] with 2 reports NOT_DETERMINED.
The real-ball form uses partial-ball text, for example {inf: -3}.
Library calls accept any permitted pair of their declared type; inputs may coincide.
