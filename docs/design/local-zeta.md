# Local zeta factors (1F.9, design before code)

Lane d-zeta, 2026-10-04. Proposed N-D20. No declaration below is implemented.
SPEC is unchanged. A complex ball means a closed rectangle, not a disk.

## 1. Sources and notation

The contracts read are SPEC 9.3.6:670-683, 9.3.7:685-703, 15.4:N-D18,N-D19;
PLAN 1F.9:279; conventions 2-5, 8.4:1105-1137 and 9.2:1173-1208;
api-1f.md:231-273 and api-1f9.md Y1-Y15. There is no docs/api-1f2.md.
The status definition numbered 3.1 is in conventions.md, not in SPEC.md section 3.
cadele is declared in include/adelefeld/adele.h, not a separate cadele.h.
The headers read are rfunc.h, sball.h, adele.h, place.h, symbol.h and catalogue.h.

External ground truth, read on disk:

- refs/src/tate-poonen/notes.txt:53-65 defines Gamma and states its recurrence, meromorphic
  continuation, simple poles at 0,-1,-2,..., and absence of zeros. In particular :62-64 closes
  the source obligation in catalogue.md Proposition 9:196-197 and its review table:359.
- refs/src/tate-poonen/notes.txt:1014-1016 gives the real factor; :1733 gives the finite factor.
- refs/src/flint-3.0.1/acb.rst:6-17 specifies separate real and imaginary interval radii;
  :224-230 specifies finiteness and exactness; :306-313 specifies zero and integer containment.
- refs/src/flint-3.0.1/acb.rst:637-643 defines general power through exp(y log x);
  :645-650 describes its analytic branch-cut option; :658-673 defines exp and expm1.
  The base p is positive, so log(p) is the real logarithm. No variable-base branch cut occurs.
- refs/src/flint-3.0.1/acb.rst:893-902 defines gamma and reciprocal gamma. Reciprocal gamma
  avoids division by zero at Gamma poles. This is an evaluation contract, not a pole detector.
- refs/src/flint-3.0.1/acb.rst:871-876 defines the rising product; fmpz.rst:1529-1534 and
  ulong_extras.rst:833-840 specify proved word primality, used only to validate oracle place inputs.
- refs/src/flint-3.0.1/arb.rst:6-16 guarantees enclosure and permits nonminimal output balls;
  :28-40 describes working precision and convergence, without a quantitative error bound;
  :1082-1087 gives exp's propagated-error bound.
- refs/src/flint-3.0.1/arf.rst:6-20,39 and mag.rst:6-9 give arbitrary integer exponents.
  The sentence at arf.rst:39 is "Since exponents are bignums, overflow or underflow cannot occur."

The acb/arb evaluator source files are absent from refs/src/flint-src-3.0.1. That snapshot contains
selected other modules. Do not attribute an evaluator's internal range threshold to this snapshot.
[source pending: FLINT 3.0.1 acb/arb exponential and Gamma evaluator source and range guards]

Write S = [x-rx,x+rx] + i[y-ry,y+ry], m=x+iy, rx,ry >= 0, and R=sqrt(rx^2+ry^2).
R is a disk cover radius; using max(rx,ry) as R is wrong. S is a point iff rx=ry=0.
All stored exact coordinates are dyadic rationals. Let a=log(p)>0. Distance means Euclidean distance.

## 2. Mathematical statements

### Z1. Finite continuation, pole set and residues

L_p(s)=1/(1-exp(-a s)) is meromorphic on C. Its poles are exactly 2 pi i k/a, k in Z,
all simple, with residue 1/a. It has no zeros at regular points.

Proof.

1. For Re(s)>0, |exp(-a s)|<1. Summing the geometric series gives the factor and tail
   exp(-J a s)/(1-exp(-a s)) after J terms. This agrees with the quoted finite factor.
2. exp(-a s) is entire. For s=u+iv its modulus is exp(-a u), so equality to 1 forces u=0.
   Then exp(-i a v)=1 iff a v is an integer multiple of 2 pi. Replacing k by -k gives the stated set.
3. D(s)=1-exp(-a s) has derivative a exp(-a s). At a zero s0 it is a, which is nonzero.
   Taylor expansion gives D(s0+h)=a h+O(h^2). Thus h L_p(s0+h) tends to 1/a.
4. The reciprocal of a nonzero holomorphic function is holomorphic. Its numerator is 1,
   so it has no regular zeros. These facts prove all assertions, including absence of other poles.

### Z2. Real continuation, poles, residues and absence of zeros

L_inf(s)=exp(-(s/2) log(pi)) Gamma(s/2) is meromorphic on C. Its poles are exactly -2n,
n >= 0, simple, with residues 2 (-1)^n pi^n/n!. It has no regular zeros.

Proof.

1. In Re(s)>0, substitute t=pi x^2 in 2 integral_0^infinity exp(-pi x^2)x^(s-1) dx.
   Since dx/x=dt/(2t), the result is pi^(-s/2) integral_0^infinity exp(-t)t^(s/2-1) dt.
2. Use precisely the continuation and zero-free theorem quoted at notes.txt:62-64, above.
   Its hypotheses concern the Gamma integral in step 1, so there is no change of normalisation.
3. Integration by parts gives Gamma(z+1)=z Gamma(z) in Re(z)>0. The identity continues
   meromorphically. Gamma(1)=integral_0^infinity exp(-t) dt=1.
4. Gamma(z)=Gamma(z+1)/z near zero, hence its residue at zero is 1. Near z=-n the recurrence gives
   Gamma(z)=Gamma(z+n+1)/(z(z+1)...(z+n)). The numerator at -n is 1; the product of the
   other n denominator factors is (-1)^n n!. The residue is (-1)^n/n!.
5. Put z=s/2. Since z+n=(s+2n)/2, substitution multiplies the residue by 2.
   The prefactor at s=-2n is pi^n. It is entire and nowhere zero and cannot cancel a pole or create a zero.
6. The quoted Gamma theorem excludes all other poles and all regular zeros. This proves the statement.

This closes the "modulo Gamma continuation" gap by a checked on-disk theorem, rather than by
pretending that recurrence alone proves the absence of zeros. analysis.md Proposition 10:444-448
already proves the residues. Its separate Fourier and meromorphic-identity imports are outside this design.

### Z3. Exact dyadic poles

At infinity the exact poles are the non-positive even integers. At a prime the only exact dyadic
pole is zero, conditional on the following explicitly pending theorem.

[source pending: Gelfond-Schneider theorem, including every value of an algebraic power]
The theorem needed is: if alpha is algebraic, alpha != 0,1, and beta is algebraic but not rational,
every value exp(beta Log(alpha)) is transcendental.

Proof.

1. Z2 lists the real poles. They have dyadic coordinates, and no other exact point is in that list.
2. Z1 shows that a finite exact pole must have real part zero. Zero is a pole.
3. Suppose q=2 pi k/log(p) is rational and k != 0. Then q != 0. Set alpha=p and beta=i q.
   alpha is algebraic and different from 0,1. beta is algebraic and nonreal, hence is not rational.
4. The pending theorem implies that exp(i q log(p)) is transcendental. But the assumed equation
   makes it exp(2 pi i k)=1. This contradiction proves q is not rational, and hence not dyadic.
5. The implementation need not use this theorem: it recognises exact zero, and returns
   NOT_DETERMINED for any other point whose denominator cannot be excluded. It never infers
   DOMAIN from a computed zero-containing denominator. Z3 is a classification obligation, not a code premise.

### Z4. A sound three-way decision rule

The three-way rule below concerns valid finite S and permitted working precision. Invalid raw input and
resource limits are dealt with separately in section 3. "Certified pole-free" additionally requires a finite
computed value enclosure. The certificate alone does not promise that an acb Gamma call will be finite.

Finite place procedure, using working bits w=max(2,prec)+32:

1. If S is the exact zero, return DOMAIN.
2. Choose b=-a if x>=0, b=a if x<0, using the exact midpoint sign. At the exact midpoint compute
   T0 enclosing exp(b m), and D0 enclosing 1-exp(b m), the latter with -acb_expm1(b m).
   Intersect D0 with 1-T0, retaining the finite direct enclosure if expm1 is non-finite.
   The uncertainty in the computed real log(p) is included in both calls. This intersection avoids
   unnecessary refusal when a huge uncertain phase loses correlation in an expm1 formula.
3. Compute outward real bounds Rplus >= R and
   Eplus >= exp(-a |x|) (exp(a Rplus)-1), using arb and expm1. Form T by adding Eplus to
   both component radii of T0, and D by adding Eplus to both component radii of D0.
   Use a lower bound for a|x| in the first exponential and upper bounds in the second.
4. If any bound is non-finite, or D contains zero, return NOT_DETERMINED.
5. If b=-a, compute Y=1/D. If b=a, compute Y=-T/D. Check acb_is_finite(Y).
   Failure gives NOT_DETERMINED. Otherwise round Y outward to max(2,prec) bits, check finiteness
   again, commit Y and return OK. There is no unbounded ball stored on any failure.

Real place procedure:

1. If S is a point with imaginary part zero and real part a non-positive even integer, return DOMAIN.
2. Inspect the closed rectangle S/2 for non-positive integers. If its imaginary interval excludes zero,
   it contains none. Otherwise intersect its real interval with (-infinity,0]; a nonempty intersection
   contains an integer iff ceil(lower)<=floor(upper). Endpoint equality counts as containment.
   Use certified outward endpoints; ambiguous rounding of this test gives NOT_DETERMINED.
3. If a non-positive integer is possible, return NOT_DETERMINED. Otherwise first evaluate
   Y=exp(-(s/2) log(pi)) Gamma(s/2) on S at w bits.
4. If Y is non-finite, try the recurrence certificate. Put Z=S/2 and choose n>=0 so
   min Re(Z+n)>=1. The fallback permits n<=64. If n>64 return LIMIT: this is an explicit
   algorithm size bound, not a claimed Gamma pole or an arf overflow. If n<=64 compute
   Q=product_(j=0..n-1)(Z+j), then Gamma(Z+n)/Q times the same prefactor. Each factor excludes
   zero mathematically, but interval multiplication or division can still fail to certify it.
5. Check, round outward and check again as above. A still non-finite candidate gives
   NOT_DETERMINED. Before rounding a finite candidate, refine it when n<=64 and max Re(Z+n)<=64:
   evaluate the exact midpoint with Gamma, bound variation by Z6, and intersect the candidate with
   that midpoint enclosure enlarged by R B in each component. If the extra midpoint/bound computation
   is non-finite, retain the original finite candidate. Both sets enclose the same image, so intersection
   preserves enclosure. Finite Y gives OK, with a committed enclosure. The n<=64 proposal is open to TJO.

Proof of soundness.

1. Z1 and Z2 justify precisely the recognised DOMAIN cases. No positive-radius rectangle is a singleton.
2. For h=s-m, |h|<=R. The power series gives |exp(b h)-1|<=exp(a |h|)-1.
   Hence |exp(b s)-exp(b m)|<=exp(-a |x|)(exp(a R)-1)<=Eplus.
   Adding that radius to both components encloses the disk of possible changes, so T and D enclose
   exp(b S) and 1-exp(b S). This remains true if only one component of S has positive radius.
3. If S contains a finite pole, exp(b s0)=1 for either choice of b. Thus 0 is in D.
   Step 4 cannot pass. This implication uses enclosure, not a floating point equality test.
4. When D excludes zero, division encloses the pointwise ratios. For b=a,
   -exp(a s)/(1-exp(a s))=1/(1-exp(-a s)); for b=-a the formula is direct.
5. At infinity the exact pole list is Z2. The closed-interval integer test can exclude it only
   when S/2 is disjoint from that list. Gamma and the entire prefactor then enclose every regular value.
   If the fallback is used, Gamma(z+n)/Q(z)=Gamma(z) by the recurrence proved in Z2.
   Ball division either encloses this ratio or is rejected by the finiteness test. Z6 justifies the
   additional midpoint enclosure. Intersecting two enclosures of the image still contains that image.
6. Outward rounding preserves enclosure. The final finiteness test prevents non-finite values being committed.
   Computing in temporaries also proves the untouched-output and aliasing rules of section 3.
7. Therefore OK is impossible on any pole-containing S. A non-point S that contains a pole is
   NOT_DETERMINED. A pole-free but too wide S can also be NOT_DETERMINED under this conservative certificate.

Alternatives. A direct acb_pow(p,-S) or acb_exp(-a S) denominator test is simpler, but interval dependency
can make it wide, and subtraction loses digits near zero. expm1 and the midpoint bound make its error explicit.
A finite geometric pole-lattice test is sharper for pole exclusion, but does not itself enclose the value.
At infinity geometry is cheaper and sharper than acb_rgamma(S/2): it reads endpoints and does not evaluate a
special function. Reciprocal Gamma can contain zero on a pole-free wide ball. Its exclusion of zero is sound,
but its failure is inconclusive. The recommended geometry test followed by Gamma avoids that extra cost.

### Z5. Quantitative separation and working precision

Let d be the distance from m to the nearest pole and R<d. This alone does not imply that an arbitrary
library enclosure excludes zero at a fixed prec. It also supplies no universal precision depending only on R/d.
For the finite midpoint certificate there is the following explicit sufficient condition.

1. Reduce a Im(m) modulo 2 pi to theta in [-pi,pi]. Put u=a |Re(m)|. For either stable sign,
   |1-exp(b m)|^2=4 exp(-u)(sinh(u/2)^2+sin(theta/2)^2).
2. Since sinh(u/2)>=u/2 and |sin(theta/2)|>=|theta|/pi, this is at least
   exp(-u) (4/pi^2) (u^2+theta^2). Also sqrt(u^2+theta^2)=a d.
3. Consequently E/|D(m)| <= (pi/2)(R/d) ((exp(a R)-1)/(a R)) exp(-u/2),
   with the ratio interpreted as 1 when R=0. In particular it is at most (pi/2)(R/d) exp(a R).
4. One component of D(m) has absolute value at least |D(m)|/sqrt(2). Let eps bound each
   component error of D0 and let delta bound the excess Eplus-E, including mag-radius rounding.
   If E+eps+delta < |D(m)|/sqrt(2), the rectangular D cannot contain zero.
5. A convenient sufficient regime is R/d<=1/8, a R<=1, and
   (eps+delta)/|D(m)|<=1/16. Then the left side relative to |D(m)| is at most
   pi*e/16+1/16 < 0.597 < 1/sqrt(2). Thus pole exclusion is guaranteed in this regime.
6. The condition on eps and delta must be certified from actual outward bounds. arb.rst:28-40
   gives no error constant from which a universal numeric prec threshold follows. Exact or refined
   inputs converge; fixed positive radii and fixed 30-bit mag rounding do not converge to a point.
   An implementation must not replace this measured inequality by "32 guard bits always suffice".
7. At infinity, if exact endpoints are compared, every pole-free rectangle passes the geometric
   test, including all R<d. Finite Gamma evaluation and the fallback limit remain separate requirements.
   If outward endpoints
   have error eta, they are certainly sufficient when eta<dist(S,pole set)/sqrt(2).
8. Even for R=0, d can be arbitrarily small. Taking dyadic approximations to a nonzero finite pole
   makes log(p)*Im(m) need arbitrarily many accurate bits to separate its phase. Huge Im(m) also
   amplifies error in log(p). Thus no promise for all inputs follows from prec and R/d alone.

The inequality in step 4 is executable. It is the stated tightness guarantee, rather than an unsupported
claim about acb's hidden rounding constants. Geometry at infinity has no unnecessary reciprocal-Gamma refusal.

### Z6. Rigorous variation bounds for the oracle

On a convex pole-free S, |L(s)-L(m)|<=R B if B bounds |L'| on S.

Proof and computable bounds.

1. The line m+t(s-m), 0<=t<=1, stays in S. Integrating L' along it gives the assertion.
2. Finite place: let ell>0 be a lower bound for |1-exp(b s)|, obtained as the distance
   from zero to D's rectangle. Let U>=exp(b x+a rx). Differentiating either stable formula gives
   |L_p'(s)|=a |exp(b s)|/|1-exp(b s)|^2 <= a U/ell^2. Use outward real bounds throughout.
3. Real place: put Z=S/2. Choose integer n>=0 so A=min Re(Z+n)>=1, and let B0=max Re(Z+n).
   For j=0,...,n-1 put delta_j=dist(Z,-j)>0 and P=product delta_j (empty product 1).
   Put M0=1/A+(ceil(B0)-1)! and M1=1/A^2+ceil(B0)!.
4. For w in Z+n the defining integral bounds |Gamma(w)| by M0. On (0,1) integrate t^(A-1),
   giving 1/A. On [1,infinity) bound t^(Re(w)-1) by t^(ceil(B0)-1), whose complete integral
   is (ceil(B0)-1)!. Discarding its part below 1 only enlarges the bound.
5. Differentiate under the integral (the displayed integrable majorants justify it).
   On (0,1), integral t^(A-1)|log t| dt=1/A^2. On [1,infinity), log t<=t,
   so the complete integral of t^ceil(B0) exp(-t) is ceil(B0)!. This proves |Gamma'(w)|<=M1.
6. Recurrence gives Gamma(z)=Gamma(z+n)/Q(z), Q(z)=product_(j=0..n-1)(z+j).
   |Q(z)|>=P, and |Q'(z)/Q(z)|<=sum 1/delta_j. Differentiate this quotient.
   Hence |Gamma'(z)|<=(M1+M0 sum 1/delta_j)/P.
7. The prefactor has modulus at most pi^(-min Re(S)/2). Its derivative contributes log(pi)/2.
   Thus a valid real-place derivative bound is

       B = pi^(-min Re(S)/2) [M1+M0(log(pi)+sum 1/delta_j)]/(2 P).

8. These bounds need no complex Gamma or digamma evaluation. They can be loose, especially when many
   recurrence factors are small. A small certified upper bound is not a claim of the true image width.

### Z7. Near poles, extreme arguments, and quality comparisons

1. Z1 and Z2 give L(s0+h)=c/h+H(h), with H holomorphic on a small disk and nonzero c equal
   to the stated residue. On |h|<=rho, let |H|<=M. If m is at distance d from s0 and R<d,
   then |L(s)|<=|c|/(d-R)+M. At m, |L(m)|>=|c|/d-M. The characteristic scale is 1/(d-R).
2. For two opposite radial points h=d-R and h=d+R, the principal-part difference is
   2|c|R/(d^2-R^2). The remainder may subtract at most 2M. This is an asymptotic description,
   not a numerical width certificate when that difference is negative.
3. At finite places on a right half-plane Re(s)>=h>0, |L(s)|<=1/(1-p^(-h)). On
   Re(s)<=-h, the stable expression gives |L(s)|<=p^(-h)/(1-p^(-h)). Both bounds are
   independent of Im(s). They explain the finite limits 1 and 0 as the real part tends to +infinity and
   -infinity, respectively. These are limits, not exact finite-input values.
4. There is no finite arf exponent range to overflow or underflow: see arf.rst:39. Exponential
   evaluation can still return a non-finite bound for finite input. That is NOT_DETERMINED under
   conventions 4.4/CV-08, not an invented LIMIT. The stable sign avoids an unnecessary huge exponential.
   A precision beyond the declared cap is LIMIT. Text and printer limits are separate from this API.
5. Huge |Im(s)| increases phase-reduction cost and the needed accuracy of log(p). The real factor can
   be tiny or huge and suffer recurrence/reflection overestimation. No magnitude or relative-accuracy
   promise follows from finiteness. Failure to obtain a finite candidate is NOT_DETERMINED.
6. To check output width against the true image, certify two sampled point enclosures V1,V2 independently.
   Their separation has lower bound Wlo=max(0,|mid(V1)-mid(V2)|-rad(V1)-rad(V2)). Then the
   image diameter is at least Wlo, and at most 2 R B by Z6. This avoids calling an upper derivative
   bound the "true width". For fixtures with Wlo>0, require the output rectangle diameter <=64 Wlo
   plus the stated rounding allowance. That is a test target, not a global API promise.
7. At a point Wlo=0. Test containment and reference error only, or compare the returned radius with
   its certified rounding budget. Do not divide a width by zero or silently omit such a failed test.

### Z8. Independent certified Gamma reference

The oracle evaluates reference points with mpmath at at least 320 digits. It adds digits for huge phases.
It validates each reference against an
independent rectangle made from scalar arb operations at 2048 bits. No acb Gamma value is used as a reference.
Its certified Gamma range is -64<=Re(s)<=64 and |Im(s)|<=64. An out-of-range reference raises an error;
it does not invent an adelefeld status. All fixtures needing an OK real value are in this range.
The finite reference permits |Im(s)|<=2^1024 and uses no cap on the real exponent in the stable formula.
The installed python-flint is 0.8.0 and reports FLINT 3.3.1. The repository target is FLINT 3.0.1.
The scalar reference and prototype enclosures run on 3.3.1; the cited on-disk contracts are 3.0.1.
[source pending: checked FLINT 3.3.1 scalar arb enclosure contracts for this oracle runtime]
The mathematical bounds and pole statuses are version independent. Real candidate finiteness, fallback
LIMIT triggers, and last radius bits must be rechecked by the implementation lane against 3.0.1.
The status fixtures describe the declared algorithm as simulated here; they do not prove 3.0.1 evaluator behavior.

Proof of the reference certificate.

1. At a point z=s/2, shift to w=z+n with Re(w)=A>=1. For negative z choose n so A is in [1,2).
   For our positive test range A<=32. Set T=512 and N=2200.
2. Taylor's integral remainder, or the real-variable Lagrange remainder, gives
   |exp(-t)-sum_(j=0..N)(-t)^j/j!|<=t^(N+1)/(N+1)! for 0<=t<=T.
3. Termwise integration of this finite sum against t^(w-1) gives

       I_N(w)=T^w sum_(j=0..N) (-T)^j/(j!(w+j)).

   Each denominator has positive real part. Its integral remainder has absolute value at most
   T^(A+N+1)/((A+N+1)(N+1)!). The code computes this as T^A times the next coefficient over A+N+1.
4. For t>=T, log(t/T)<= (t-T)/T. Since A>=1 and A-1<T, integration of the resulting exponential
   majorant bounds the infinite tail by exp(-T) T^(A-1)/(1-(A-1)/T).
5. Add the sum of both tails to both component radii of I_N(w). This encloses Gamma(w).
   Divide by product_(j=0..n-1)(z+j) and multiply by exp(-z log(pi)). These are our own complex
   interval operations on real arb inputs. They enclose L_inf(s) by Z2.
6. Finite reference points use the stable exponential formulas in Z4, independently assembled from
   real log, exp, expm1, sin and cos at 2048 bits. Their variable domains have no branch cuts.
7. Convert the mpmath binary point to exact dyadic coordinates. The maximum distance from that point
   to the certified reference rectangle is an outward scalar bound point_error. Assert

       point_error <= 10^-200 max(1,|reference point|).

   This is a verified error bound, not an assertion that mpmath promises correctly rounded Gamma.
8. Export the point with 230 significant decimal digits. Add 10^-229 max(1,|point|) to point_error
   for decimal formatting. Bounds are exported as exact dyadics {man,exp}, meaning man*2^exp.
   No decimal display of an arb midpoint is used as a directed endpoint.

### Z9. The oracle's status and width contract

expected(place,s_mid,s_rad,prec) accepts 'real' or a word prime, a dyadic midpoint pair, and radius pairs.
A scalar radius means equal component radii, not a Euclidean disk. Non-dyadic decimal radii are rounded
outward to arb's stored radius. The function returns these effective radii. Test exactly that returned S.

1. The finite status simulator uses our own scalar complex assembly of Z4 at prec+32 bits.
   It is independent of acb_pow and acb_exp. Last radius bits can differ from a C acb implementation;
   close certificate boundaries are tested by their inequalities, not by demanding last-bit status identity.
2. At infinity pole geometry is independent exact rational arithmetic. The prospective candidate and
   its finite/non-finite predicate are simulated with python-flint acb, including recurrence and refinement.
   Its values are the object being checked, never the reference. The independent reference is Z8.
3. For OK, return an independently certified point and point_error, a Z6 derivative bound B,
   variation_bound=R B and image_bound=|point|+point_error+R B. These are rigorous upper bounds.
4. For positive-radius OK fixtures, certify a lower image diameter from nine grid points, including
   corners, edge midpoints and the centre. The witnesses are independent Z8 point enclosures.
   width_lower is a lower bound. variation_bound is an upper bound. They are never interchanged.
5. Both simulated outputs are checked against factor 64 times width_lower, and against every sampled
   certified point enclosure. This is also the proposed implementation test for both places. The bounded
   real midpoint refinement is part of the proposed algorithm, following four observed quality failures
   of its unrefined candidate. Neither a pole fixture nor the factor 64 was weakened.
6. Point cases have no positive width witness. Their containment and certified point errors are still tested.
   No general relative output accuracy is promised by this API, as in rfunc.h.

## 3. Proposed interface and decisions (N-D20)

The declaration belongs in a proposed local_zeta.h, beside symbol.h and catalogue.h. No header is changed here.

```c
/* Trivial local zeta factor, SPEC 9.3.7; design local-zeta.md Z1-Z9.
   Complex parameter s is the same spectral variable at every place. It is not a Q_p value.
   At v=p: y=1/(1-exp(-s log(p))). At v=infinity: y=exp(-s log(pi)/2)*Gamma(s/2).
   A finite initialized acb input denotes its entire closed rectangle. Output is a plain acb.

   prec is complex working precision in bits at BOTH kinds of place. Below 2 use 2.
   Above ADF_REAL_PREC_MAX (2097152) return LIMIT before every other check, where=v.
   Internal working precision is max(2,prec)+32; round the final enclosure outward to prec.
   Guard bits improve certificates but promise no fixed relative accuracy or automatic retry.

   OK: finite enclosure of every L_v(s) for s in the input; y written, where untouched.
   DOMAIN: non-finite raw input, or a recognised exact pole: zero at p; a non-positive even
   real integer at infinity. y untouched; where=v. Never infer DOMAIN from a failed enclosure.
   NOT_DETERMINED: mixed pole ball, undecided finite denominator exclusion, or non-finite
   result from permitted finite inputs. y untouched; where=v. No non-finite value is stored.
   LIMIT: prec above the cap; or the real Gamma recurrence fallback needs more than 64 factors.
   The latter is checked only after exact/mixed pole checks and failure of direct evaluation.
   y untouched; where=v. There is no other argument-magnitude or prime-size cap.
   No UNIT_NOT_CERTIFIED, NEEDS_SPLIT or branch-cut status applies to these meromorphic factors.

   v is a valid place handle made by adf_place_inf or adf_place_prime. The latter takes ulong,
   certifies primality, and returns DOMAIN for 0,1 or composites without changing its output.
   A prime >=2^64 is outside this handle type; its text reader returns UNSUPPORTED.
   Forged handles violate the input precondition; no arbitrary fmpz-prime interface is proposed.

   y may equal s. Neither may overlap where or a subobject of the other. where may be NULL.
   Compute into temporaries; commit only after pole exclusion and both finiteness checks.
   Cost at p: one word log, two midpoint exponential/expm1 calls, scalar radius propagation,
   and one complex division. No pole enumeration, factorisation, or loop over |Im(s)|.
   At infinity: integer geometry, Gamma and exponential, optional shifted Gamma with at most
   64 product factors, and bounded midpoint/derivative refinement (at most four Gamma calls total).
   Bit cost depends on prec and argument magnitude.
   A declared cap is LIMIT; a returned non-finite acb is NOT_DETERMINED (CV-08).
*/
int adf_complex_local_zeta_at(acb_t y, adf_place_t *where, const acb_t s,
                              adf_place_t v, slong prec);
```

Choices and alternatives:

| Decision | Recommendation and reason | Alternative |
|---|---|---|
| Input/output | acb_t directly, as rfunc.h uses arb_t | a complex one-place adf_sball |
| Name | adf_complex_local_zeta_at, input namespace and named place | adf_local_zeta_at or adf_zeta_factor_at |
| Dispatch | one function, valid adf_place_t selects finite or real | separate prime and real entry points |
| Prime size | existing word handle, all primes below 2^64 | fmpz p with certification and new size policy |
| Files | local_zeta.h with the single declaration | catalogue.h, or complex wrappers in rfunc.h |
| Certificate | stable midpoint exponential at p; integer geometry at infinity | raw pow; reciprocal Gamma |
| Limits | existing precision cap; 64 factors for optional recurrence fallback | unbounded shift or caller budget |
| Reciprocal | defer a public function until a use exists | separately named local_zeta_inv_at |
| Products | milestone 5, explicit finite lists only | an all-places operation in this slice |

An sball at a prime contains a Q_p input, so it cannot hold this complex s at that prime. Attaching a
complex archimedean component to a prime would confuse the parameter place with the selected factor place.
cadele contains an irrelevant independent finite coordinate. It is useful only as the driver's existing
complex text carrier; the driver must extract its complex coordinate explicitly.

If the reciprocal is later added, at p it is 1-exp(-s log(p)), entire, with zeros rather than poles.
At infinity it is exp(s log(pi)/2) rgamma(s/2), entire by Z2 and the quoted zero-free theorem.
It needs the same finite-output checks but no pole DOMAIN or mixed-pole NOT_DETERMINED.
It must have a separate name; a switch that changes the mathematical operation is unnecessary here.
A finite product has accumulation, per-place failures and conductor conventions. An unrestricted Euler
product is not a finite-place wrapper and has a different convergence domain. Both belong with Tate integrals.

The class row "Characters, Gauss sums, local factors" in conventions.md:223 omits LIMIT, while the common
function row and wrapper precision caps allow it. TJO must reconcile that row if this recommendation is taken.
This lane does not edit conventions or SPEC. No claim that an undeclared exponent overflow is LIMIT is made.

## 4. Implementation fixtures and checks

Generate the fixtures with:

```text
timeout 120 python3 proto/zeta_checks.py --fixtures lanes/d-zeta/zeta-fixtures.jsonl
```

Load midpoint and radius strings as exact rationals and construct the returned effective dyadic boxes.
Bounds {man,exp} are exact binary values. The exported point error includes its decimal conversion margin.
Read reference points at at least 1024 bits before comparing to the returned prec-bit output.
On OK, check acb_is_finite and containment of all independently certified sampled values. A rigorous
point enclosure wholly inside the returned ball is sufficient; if boundary intervals overlap, raise the
independent certificate precision instead of declaring failure from an approximate midpoint alone.
Check the width target 64*width_lower plus rounding allowance 2^(-max(2,prec)+8)*max(1,image_bound).
Record failures of that target separately from soundness failures. Do not weaken the factor to hide a failure.

The cases cover k=-3..3 at all six requested primes, real poles 0,-2,...,-40, radii 10^-j
for j=1,4,12,30,60, both stable-sign branches, horizontal and vertical degenerate rectangles, exact poles,
regular negative odd integers, complex regular boxes, closed pole boundaries, real parts +/-2^1000,
imaginary parts 2^1000, and wide pole-free refusals. Pole membership itself is certified with scalar arb
intervals for 2 pi k/log(p), rather than inferred from a small mpmath residual.
The precision pairs hold the same pole-free input fixed and change only prec=16 to prec=256.
Add implementation-only tests for non-finite raw inputs, every failure's untouched output, y=s aliases,
where=NULL, where sentinels on OK, precision below 2, precision cap boundaries and fallback boundaries.
Test complex midpoint signs and prime validation at 0,1,4,9 and the word boundary. Text failures precede calls.

Eight faults to plant, each removed after its test fails:

1. Test finite poles only at k=0: nonzero-k around and segment fixtures must reject OK.
2. Return DOMAIN on a positive-radius pole ball: all around fixtures require NOT_DETERMINED.
3. Reverse the exponent sign without changing the stable ratio: exact positive/negative values detect it.
4. Use Gamma(s) instead of Gamma(s/2): s=1,2,4 and negative odd regular points detect it.
5. Use pi^(-s) instead of pi^(-s/2): s=2 requires 1/pi; s=4 requires 1/pi^2.
6. Write y before deciding the status: preserve a nontrivial acb sentinel on every DOMAIN, ND and LIMIT,
   including y=s. Compare dump-level midpoint/radius representation, not overlapping value sets.
7. Omit the final finiteness check: pole-near low-precision and deliberately wide gamma/exponential cases
   must never return OK with a non-finite ball. A branch that fabricates a finite truncation is also caught
   by the independently certified sample containment checks.
8. Omit where on failure, or write it on OK: seed place 97 and assert the supplied v on every failure,
   unchanged on OK. Repeat with NULL to exercise the optional-report path.

Additional targeted faults: replace R by max(rx,ry), test open rather than closed pole boundaries,
declare an integer pole from a rounded acb value, omit the recurrence scale factor, or use a Gamma branch cut.
The eight above are the required mutation set; these extras follow only when implementation code warrants them.

### Driver surface, proposed and not run

The existing driver reads cadele text through adf_cadele_set_str (tools/adf/adf.c:380-392) and uses
the complex grammar in conventions.md:1176,1184. Its place tokens are "real" and word primes
(adf.c:1624-1659). A proposed command is local_zeta_at CADELE with PLACE. Parse the whole cadele,
extract its complex coordinate with adf_cadele_get_complex, and call the acb interface. The finite
coordinate is a carrier field and has no mathematical role in this command; document that explicitly.
Print the plain complex ball using the existing complex-coordinate printer technique, without inventing
an adele-valued result. Status lines keep the driver's existing "error: STATUS" form; where is tested in C.

```text
prec 128
digits 20
local_zeta_at ((1) + (0)*i ; 0) with 2
local_zeta_at ((0) + (0)*i ; 0) with 2
local_zeta_at ((0 +/- 0.0001) + (0 +/- 0.0001)*i ; 0) with 2
local_zeta_at ((2) + (0)*i ; 0) with real
local_zeta_at ((-2) + (0)*i ; 0) with real
local_zeta_at ((-2 +/- 0.0001) + (0)*i ; 0) with real
local_zeta_at ((1) + (0)*i ; 0) with 4
```

Expected lines, after implementation, with printer radius fields matched by enclosure rather than literals:

```text
(2) + (0)*i                          [or an enclosing printed ball around 2]
error: DOMAIN
error: NOT_DETERMINED
(0.31830988618379067154 +/- R) + (0)*i [R accounts for printer rounding]
error: DOMAIN
error: NOT_DETERMINED
error: DOMAIN
```

The bracketed explanations are assertions in this plan, not proposed bytes of driver output.
For a nonzero finite pole, generate decimal y=2 pi/log(2) at 80 digits and give each component
radius 10^-60. The pole is inside that parsed rectangle; the expected line is error: NOT_DETERMINED.
The implementation lane should run `timeout 120 build/adf lanes/d-zeta/zeta-driver.adf` after adding
the command. No existing driver command or Julia entry point is claimed to exist for local zeta today.

### Julia call, proposed and not run

This uses the existing cadele parser as a text carrier and the documented target ABI's acb layout.
It follows tests/julia/symbol.jl's explicit initialization and ccall style. A future binding can expose
acb directly. Save the snippet as a Julia test only in the implementation lane's owned paths.

```julia
using Libdl
flib = Libdl.dlopen("libflint.so.18", Libdl.RTLD_GLOBAL)
lib = Libdl.dlopen(ARGS[1])
fl(n) = Libdl.dlsym(flib, n)
ad(n) = Libdl.dlsym(lib, n)
struct Place
    opaque::UInt64
end
csize = ccall(ad(:adf_sizeof_cadele), Csize_t, ())
c = Ptr{UInt8}(Libc.malloc(csize))
ccall(ad(:adf_cadele_init), Cvoid, (Ptr{UInt8},), c)
# Target ABI: sball.h/adele.h give acb_struct as 96 bytes, alignment 8.
s = Ptr{UInt8}(Libc.malloc(96))
y = Ptr{UInt8}(Libc.malloc(96))
saved = Ptr{UInt8}(Libc.malloc(96))
two = Ref{Clong}(0)
ccall(fl(:fmpz_init), Cvoid, (Ref{Clong},), two)
ccall(fl(:fmpz_set_si), Cvoid, (Ref{Clong},Clong), two,2)
for t in (s,y,saved)
    ccall(fl(:acb_init), Cvoid, (Ptr{UInt8},), t)
end
try
    txt = "((1) + (0)*i ; 0)"
    @assert ccall(ad(:adf_cadele_set_str), Cint,
        (Ptr{UInt8},Cstring,Csize_t,Clong,Ptr{Cvoid}), c,txt,sizeof(txt),128,C_NULL)==0
    ccall(ad(:adf_cadele_get_complex), Cvoid, (Ptr{UInt8},Ptr{UInt8}), s,c)
    v = Ref(Place(0))
    @assert ccall(ad(:adf_place_prime), Cint, (Ref{Place},Culong), v,2)==0
    w = Ref(ccall(ad(:adf_place_inf), Place, ()))
    st = ccall(ad(:adf_complex_local_zeta_at), Cint,
        (Ptr{UInt8},Ref{Place},Ptr{UInt8},Place,Clong), y,w,s,v[],128)
    println("status=", st) # expected: status=0
    @assert st==0
    @assert ccall(fl(:acb_contains_fmpz), Cint, (Ptr{UInt8},Ref{Clong}), y,two)==1
    @assert ccall(ad(:adf_place_is_archimedean), Cint, (Place,), w[])==1
    ccall(fl(:acb_set), Cvoid, (Ptr{UInt8},Ptr{UInt8}), saved,y)
    ccall(fl(:acb_zero), Cvoid, (Ptr{UInt8},), s)
    st = ccall(ad(:adf_complex_local_zeta_at), Cint,
        (Ptr{UInt8},Ref{Place},Ptr{UInt8},Place,Clong), y,w,s,v[],128)
    println("status=", st) # expected: status=7
    @assert st==7
    @assert ccall(ad(:adf_place_prime_get), Culong, (Place,), w[])==2
    @assert ccall(fl(:acb_equal), Cint, (Ptr{UInt8},Ptr{UInt8}), y,saved)==1
finally
    ccall(fl(:fmpz_clear), Cvoid, (Ref{Clong},), two)
    for t in (s,y,saved)
        ccall(fl(:acb_clear), Cvoid, (Ptr{UInt8},), t)
        Libc.free(t)
    end
    ccall(ad(:adf_cadele_clear), Cvoid, (Ptr{UInt8},), c)
    Libc.free(c)
end
```

Run after implementation: `timeout 120 julia zeta.jl build/libadelefeld.so`.
Expected printed lines are `status=0` and `status=7`. The call also checks containment of the exact value 2,
where preservation on OK, and representation preservation after DOMAIN. The literal 96-byte acb allocation
is confined to the documented target ABI; a portable production binding needs a FLINT sizeof query/shim.

## 5. Open points for TJO

1. Names and files: dedicated local_zeta.h with adf_complex_local_zeta_at, or catalogue.h with
   adf_local_zeta_at. Recommend the dedicated header and input namespace used by rfunc.h.
2. Types: raw acb, or a new complex sball constructor/wrapper. Recommend raw acb: spectral s is complex
   even when the factor place is finite, and no finite-coordinate projection is involved.
3. Certificate policy: midpoint bound plus geometry and bounded recurrence, or adaptive subdivision.
   Recommend the explicit conservative certificate first; ND remains permitted for pole-free wide balls.
4. Limits: existing prec cap and n<=64 fallback, or a caller work budget/unbounded shift. Recommend the
   caps for the first slice and reconcile conventions.md:223's missing LIMIT. Preserve CV-08 for non-finite results.
5. Scope: expose entire reciprocals/products now, or defer. Recommend defer: one-place factors meet 1F.9;
   finite products and normalisations belong with milestone 5. No infinite Euler-product wrapper here.

## 6. Runs and findings

Final command, after all oracle repairs and added cases:

```text
timeout 120 /usr/bin/time -f 'elapsed_seconds=%e peak_kib=%M exit=%x' \
  python3 proto/zeta_checks.py --fixtures lanes/d-zeta/zeta-fixtures.jsonl
```

Results: 63 pole/residue cases; 7 independent Gamma-integral checks; 12 precision cases (6 refusals at
16 bits, 6 OK at 256 bits); 1377 status fixtures (701 OK, 642 NOT_DETERMINED, 32 DOMAIN, 2 LIMIT);
5845 independently certified sampled values contained by simulated outputs; 643 positive width witnesses
with simulated output diameter <=64 times the certified lower image diameter. Exit 0, 108.59 s, 41896 KiB peak.
This is a deterministic fixture run, not a long differential fuzz run and not a check of C code.

A case fails on any wrong status, wrong pole membership, failed reference error certificate, failed
variation bound, missed sampled value, non-finite accepted output, missing positive width witness,
or diameter above factor 64. Every exported point includes its certified and decimal formatting error.

Observed direct Gamma counterexample: S=[-2.1,-1.9]+i[1.5,1.7] is pole-free, but direct Gamma(S/2)
at 288 working bits returned non-finite. The recurrence fallback is finite. Four other unrefined real
candidates failed the factor-64 target; their worst ratio was 279.62142125231634. Refinement repaired
them without changing their input sets, expected statuses, or width factor.

No counterexample to SPEC 9.3.7, catalogue Proposition 9, or analysis Proposition 10 was found.
The Gamma continuation source gap is closed at notes.txt:62-64. Gelfond-Schneider, the omitted
FLINT evaluator source, and the oracle runtime's scalar contracts remain source obligations.
The missing LIMIT in conventions.md:223 is an
interface reconciliation point, not a false assertion about the meromorphic factors.
