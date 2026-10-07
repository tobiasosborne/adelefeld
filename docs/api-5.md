# Milestone 5: Tate integrals

Design before code. SPEC 5, 8 and 9.3.7 and conventions 6 remain fixed. Here analysis means
docs/proofs/analysis.md. P9-P15 are prerequisites, not proposals to replace those proofs.
The oracle is proto/tate_checks.py: exact phases and cyclotomic identities, numerical references at
60 decimal digits with margin 1e-45 max(1,|reference|), and separately certified ball quadrature.
It prints its python-flint/FLINT versions. Its installed FLINT is not evidence about the C library ABI.

## 1. Values, scope and shared contract

D1: no new owning result or pole type. An initialized acb output, the supplied spectral rectangle S,
an explicit half-plane or continuation entry point, and a status specify the result and its domain.
This satisfies PLAN 4's "results of integrals with their domain" without storing a second copy of S.
The output encloses every pointwise value on S. A pole is never an acb value. Pole residues are stated
below; no lifecycle, predicate, text grammar or dump tag is needed. The driver prints the mode and S
alongside the value/status. Existing acb text/dump mechanisms suffice.
Implementation placement: the three local calls extend localfactor.h; tate.h declares the vector and
global calls and joins adelefeld.h. This lane changes none of those headers.

The global entry points select exactly the vectors of SPEC 8, including conductor 1. They take chi,
not omega_chi. They read only the finite character, like adf_char_chi; chi->s is ignored explicitly.
Internally omega is adf_char_conj(chi) with its exponent reset to exact zero. No second conjugation.
Local eta0 similarly means only the finite character. Arbitrary tensors and arbitrary quasi-character
exponents are not a new public integration interface in this milestone; the general formula is in section 4.

All inputs/outputs are initialized; canonical object inputs and certified place handles are preconditions.
Numerical outputs may alias raw s or alpha; they may not alias character members. Distinct outputs do not
overlap. Work is in temporaries. Every failure preserves every value/count output bit for bit.
For _at calls, where may be NULL; failure writes v there, success leaves it untouched. Global calls have
no where: a spectral pole is not a failing place. They do not use gfunc's placewise merge machinery.

First reject prec>ADF_REAL_PREC_MAX=2097152 with LIMIT, then size preflight, then INV, then domain.
Working precision starts at max(2,prec); guards count against the precision cap. Nonfinite raw s, or
alpha when used at p, is DOMAIN; a nonfinite computed value is NOT_DETERMINED. No nonfinite result is stored.
Local factors use the "Characters, Gauss sums, local factors" row; global calls use "Integrals, Poisson
summation" (conventions 3.2). Both use OK, DOMAIN, NOT_DETERMINED, LIMIT here. If a character dependency
reports UNSUPPORTED for setup, the composite call returns NOT_DETERMINED: its value is uncertified.
Do not convert that failure into a false character predicate or a mathematical DOMAIN.

D2: retain ADF_CHAR_MOD_MAX=65536; test it before group setup. Retain milestone 4 D1 on every factor:
DM<=2^20, terms/coefficients<=2^16, integer intermediates<=2^20 bits, direct transform L^2<=2^20.
Propose ADF_TATE_WORK_MAX=2^20 total charged operations, including retries: a phase/array entry, a
transform multiply-add, a series-tail prefix step, or one Taylor coefficient costs one. Preflight
counts, byte sizes and projected integer products. Charge every attempted step, not just accepted terms.
The transform-based global path can return LIMIT already for C>1024, despite the character cap 65536.
This is an algorithm limit, not a stronger predicate. No silent truncation or unbounded prime search.

## 2. Local integrals and constants: 5.1

At p use the explicit pair (alpha,eta0), alpha=eta(p). eta0 must have conductor 1 or p^a and be primitive
as an adf_char; extract a by exact divisions. Thus p=2 cannot have a=1. Do not infer alpha from eta0.
For the global omega at ramified p, eta0=conj(chi_p) and
alpha=product_(l|C,l!=p) chi_l(p), as conventions 6.5 and P11 state. At unramified p alpha=chi(p).
Local balls lball.h identify 1_Zp and the unit residue classes in the acceptance tests. The spectral
parameter is complex at every place; it is never a p-adic lball or an adele of gfunc.h.

Let T=exp(-s log p), using the real logarithm. P9 gives the following meromorphic values:

| Quantity | a=0 | a>0 |
|---|---|---|
| Standard test vector | 1_Zp | eta0^-1 1_(Zp^x) |
| Integral and L_p | 1/(1-alpha T) | 1 |
| gamma_p | (1-alpha T)/(1-alpha^-1 p^(s-1)) | alpha^a T^a G_minus(eta0^-1) |
| epsilon_p | 1 | gamma_p |

G_minus=sum_(u mod p^a,p does not divide u) eta0(u)^-1 E(-u/p^a).
Obtain the exact phase of eta0(u) through adf_char_chi_phase, negate it, add -u/p^a modulo 1, then
call adf_phase_get_acb. Its name is a phase evaluator, never "the Fourier kernel". Do not call the
positive Gauss sum and silently identify it with G_minus. Sum with outward rounding.
The naive 1_Zp against ramified eta has integral zero in its convergence half-plane; the oracle tests it.
The unramified defining integral needs Re(s)>log|alpha|/log p. The standard ramified vector is integrable
for every s. These local calls return continuations and therefore do not impose that half-plane.

At infinity e=eta0->parity, alpha is ignored (there is no uniformizer). P10 gives

    phi_e=x^e exp(-pi x^2), eta(x)=sign(x)^e,
    Z_inf=L_inf=pi^(-(s+e)/2) Gamma((s+e)/2), initially Re(s)>-e,
    gamma_inf=i^e pi^(s-1/2) Gamma((1-s+e)/2) rgamma((s+e)/2), epsilon_inf=i^e.

The trivial calls delegate to adf_local_zeta_factor_at; at infinity use that function at s+e.
General |x|^w at infinity is caller substitution s+w. Epsilon is evaluated in its simplified entire
form, including at points where individual gamma or L factors have poles; never divide three factors.

Poles of the finite unramified integral satisfy alpha exp(-s log p)=1; for alpha=1 they are
2 pi i k/log p. Its gamma poles satisfy alpha^-1 exp((s-1)log p)=1. Numerator and denominator cannot
both vanish, since their exponential products equal 1/p. Ramified integral/gamma/epsilon have no s poles.
Real integral poles are -e-2k; real gamma poles are 1+e+2k, k>=0, and its zeros are -e-2k.
Use rgamma at the latter, not division by a nonfinite Gamma. Real epsilon has no poles.

Certify finite alpha!=0: exact zero DOMAIN; a rectangle containing zero NOT_DETERMINED. For a denominator
1-alpha exp(-s log p), enclose it directly, then optionally refine at midpoints: with disk radii rA,rS,
the variation is at most |exp(-mS log p)|[rA exp(rS log p)+|mA| expm1(rS log p)].
All quantities are outward bounds. Exclusion of zero certifies division. Otherwise NOT_DETERMINED,
except proved exact poles: recognize exact integral s=k and alpha=p^k (integer k, bounded exact power),
and the corresponding dual test for gamma. In particular (alpha,s)=(1,0) and (1,1) are recognized.
Never infer DOMAIN from a computed zero-containing denominator. More exact-pole recognition is optional.
At infinity use exact integer geometry on (s+e)/2 or (1-s+e)/2, then N-D20's Gamma certificate and
64-factor recurrence bound. A pole-free rectangle can still be NOT_DETERMINED. No branch cut is imposed.

```c
/* Standard-vector local integral, meromorphically continued; finite enclosure of all (alpha,s).
   Pair and vector semantics above. DOMAIN for nonfinite inputs, invalid prime-power conductor,
   alpha=0 at p, or a recognized exact pole; NOT_DETERMINED for mixed/undecided poles or alpha=0,
   or certification failure; LIMIT for D2/precision and inherited Gamma shift cap. At infinity
   ignores alpha; uses eta0 parity. Aliasing/where/atomicity as section 1. Cost O(1) special
   functions after conductor validation; trivial case is exactly the existing local-factor call. */
int adf_local_tate_at(acb_t z, adf_place_t *where, const acb_t s, adf_place_t v,
    const acb_t alpha, const adf_char_t eta0, slong prec);
/* gamma in the fixed local functional equation, including its zeros. Same input/status contract,
   using its OWN poles above. Cost O(p^a) phase calls if ramified, otherwise O(1) exponentials;
   at infinity one Gamma, one rgamma and one exponential, with bounded fallback. */
int adf_local_gamma_at(acb_t z, adf_place_t *where, const acb_t s, adf_place_t v,
    const acb_t alpha, const adf_char_t eta0, slong prec);
/* epsilon=1 unramified, gamma ramified, i^e real. Same validation/atomicity/aliasing/caps.
   No s poles: do not inherit gamma's pole rejection in the unramified or real branches.
   Cost O(1) in those branches, O(p^a) phases in the ramified branch. */
int adf_local_epsilon_at(acb_t z, adf_place_t *where, const acb_t s, adf_place_t v,
    const acb_t alpha, const adf_char_t eta0, slong prec);
```

## 3. Global value and completion: 5.2

P11 gives I_chi(s)=Z(f_chi,omega_chi,s)=pi^-z Gamma(z)L(s,chi), z=(s+e)/2, Re(s)>1.
For C=1 this is pi^(-s/2) Gamma(s/2) zeta(s). Lambda_chi=C^z I_chi. The conductor factor belongs
to Lambda, not to the value of the prescribed adelic integral.

D3: primary algorithm is theta Mellin integration after splitting and Poisson, not an Euler product.
It already evaluates I in Re(s)>1 independently of acb_dirichlet. Section 4 removes just the domain
gate for continuation. Euler products are an optional independent acceptance check, with T1's bound.
Near Re(s)=1 their elementary certified cutoff is too costly for the primary algorithm.

Build the vector with existing setters: D=1,M=C,f[j]=chi(j) (zero on nonunits), P=x^e,A=1,B=C_real=0.
For zeta f[0]=1. Transform with adf_ffun_fourier and adf_rfun_fourier. Evaluate the dual theta from
those transformed factors. The balanced change A=1/C, P=x^e represents C^z I after Mellin integration:
it is C^(e/2) times real dilation by 1/sqrt(C), a scalar times an idele dilation of the original vector.
It puts P13's split at theta variable t=1. Its dual coefficients reduce to W_chi conj(chi(n)) n^e;
this reduction follows from the actual negative finite transform and positive real transform, not a
new Fourier implementation. Keep the generic transformed-factor expression available in tests.

adf_tensor_poisson on the dilated paired factors supplies the theta identity used to replace the lower
half of the Mellin integral. For quadrature, integrate its finite theta expansion analytically by T3;
do not call a whole infinite Poisson sum for every Taylor coefficient. At sample positive t, compare
both independent Poisson enclosures to that expansion plus its Lemma 6 tails (T5). This is the seam
between milestones 4 and 5. Consume the existing transforms/tail kernels; do not reimplement their APIs.

FLINT reference, tests only: acb_dirichlet_l returns L(s,chi), so multiply by pi^-z Gamma(z) for I,
or by (C/pi)^z Gamma(z) for Lambda. For zeta use acb_dirichlet_zeta with the same real completion.
FLINT's xi= s(s-1) Lambda_zeta/2; hence Lambda_zeta=2 xi/[s(s-1)] away from 0,1. At 0,1 that
conversion is a meromorphic identity, not permission to divide by zero. T2 supplies the exact proof.

```c
/* Build precisely f_chi,phi_e above, ignoring chi->s; initialized independent factor outputs.
   OK commits both. LIMIT for precision/C/D1; NOT_DETERMINED for uncertified character phases.
   No mathematical DOMAIN on canonical input. Cost O(C) character phases and factor storage.
   Existing lifecycle, text and dump calls own the resulting factors. */
int adf_tate_vector(adf_rfun_t phi, adf_ffun_t f, const adf_char_t chi, slong prec);
/* I_chi over the WHOLE spectral ball, with certified Re(s)>1. DOMAIN if upper(Re s)<=1;
   NOT_DETERMINED if it also contains points to the right of 1 but lower(Re s)<=1.
   Nonfinite s DOMAIN. bits in [0,2^21], else DOMAIN after precision/size checks.
   OK certifies finite output, each coordinate diameter <=2^-bits. Failure preserves z.
   Aliasing z=s allowed, not chi->s. LIMIT for D2; numerical/width failure NOT_DETERMINED.
   Cost the vector/character calls plus O(C^2) transform and O(N sum_panels(J+1)) Taylor terms
   and bound searches. Phase calls include their character setup cost, as char.h specifies. */
int adf_tate_integral(acb_t z, const adf_char_t chi, const acb_t s, slong bits, slong prec);
```

## 4. Splitting, certificates and poles: 5.3

For any finite sum of implemented tensors, P12 defines

    H_f,chi(t)=integral_(Zhat^x) Theta_f((t,u)) conj(chi(u)) du,
    delta=[C=1], a=delta f(0), b=delta hat_f(0),
    J_f,chi(s)=integral_1^infinity (H_f,chi(t)-a)t^s dt/t
             +integral_1^infinity (H_hat_f,conj(chi)(t)-b)t^(1-s) dt/t
             +b/(s-1)-a/s.

The Poisson step is H_f,chi(t)=t^-1 H_hat_f,conj(chi)(1/t), with u replaced by u^-1.
Use both factor dilations by the idele (t,u), not dilation by the diagonal rational t. The latter has
norm 1 and leaves the rational theta sum unchanged. For a general finite factor the unit average is
over units modulo lcm(DM,C), not necessarily modulo C alone. This general integrator is deferred;
the prescribed vector cancels chi(u) against conj(chi(u)) exactly, so no unit enumeration is needed.

For the balanced prescribed vectors P13 gives the computation actually used:

    V_chi(t)=sum_(n>=1) chi(n)n^e exp(-pi n^2 t/C), z=(s+e)/2, z'=(1-s+e)/2,
    Lambda=integral_1^infinity V_chi(t)t^(z-1)dt
          +W_chi integral_1^infinity V_conj(chi)(t)t^(z'-1)dt
          +delta[1/(s-1)-1/s].

Evaluate both terms and the rational terms independently. For I multiply by C^-z afterwards, with
propagated ball error. Transform-generated coefficient balls and the root-number error are included.
Never evaluate Gamma(z)L(s,chi) for continuation: at trivial L zeros that product can be 0 times infinity.

Use a0=pi/C and S_e of analysis L6, including its ratio-prefix terms. P15's absolute omitted-region bounds:

    E_n(z,N)=exp(a0) S_e(a0,N) J_bound(Re(z)-1,a0,1),
    E_t(z,R)=exp(a0) S_e(a0,0) J_bound(Re(z)-1,a0,R).

J_bound is BOTH branches of L14: if b>=2 max(r,0)/R, use 2 R^r exp(-bR)/b; otherwise r>0,
R0=2r/b, t*=min(max(r/b,R),R0), and use (R0-R)t*^r exp(-b t*)+2 R0^r exp(-bR0)/b.
Use lower positive decay rates and upper real exponents uniformly over the input rectangle (t>=1).
At each working precision recompute these bounds outward. Choose N from 0,1,2,4,... until
E_n(z,N)+E_n(z',N)<=epsilon/32; choose R from 1,2,4,... independently until the analogous E_t sum
is <=epsilon/32. These explicit searched integers and certified inequalities are the cutoff certificate.
For Lambda epsilon=2^-bits. For I budget epsilon=2^-bits/max(1,upper|C^-z|), before final width testing.

D4: use the library's own composite Taylor rule T3, not arb_calc or undocumented incomplete Gamma.
Split [1,R] into [1,2],[2,4],... . For each panel [l,u], m=(l+u)/2,h=(u-l)/2,d=m/2,q=h/d<=2/3.
For each finite term g_n(t)=exp(-b_n t)t^(z-1), b_n=a0 n^2, use

    c_-1=0, c_0=exp(-b_n m)m^(z-1),
    c_(k+1)=((z-1-b_n m-k)c_k-b_n c_(k-1))/(m(k+1)),
    integral polynomial=2h sum_(0<=2j<=J) c_(2j) h^(2j)/(2j+1).

Multiply by chi(n)n^e and add with ball arithmetic. With r=Re(z)-1, an outward coefficient majorant is
B=sum_(n=1)^N n^e exp(-b_n(m-d)) max((m-d)^r,(m+d)^r) exp(pi |Im z|/2).
Panel error <=2h B q^(J+1)/(1-q). Increase J from 0 until it is <=epsilon/(64 number_of_panels).
Do each Mellin integral separately. Add their errors, E_n and E_t, to BOTH coordinate radii; |W|=1
bounds the true omitted second tail, while its finite multiplication uses the computed root ball.
R=1 needs no panels. This rule is exponential in J and supports targets that midpoint's h^2 rule cannot
reach under D2. P15's midpoint M2 and error (R-1)h^2 M2/24 remain an independent low-precision check.

Check the final coordinate widths after completion/pole terms and outward rounding. If they fail, double
working precision up to the cap, charging repeated work. Stop with NOT_DETERMINED if two successive
attempts from precision >=64 fail to halve the largest diameter; this is a refusal heuristic, not a
proof of an input-width floor. A work-cap exhaustion is LIMIT. Never return OK with an unmet target.
Exact inputs permit shrinking errors; fixed input radii can prevent the requested width (N-D23).
No relative-accuracy promise is made near zeros or on high vertical lines.

For prescribed vectors C=1 gives a=b=1: residues -1 at 0 and +1 at 1; C>1 is entire.
At exact 0 or 1 return DOMAIN only for C=1. A rectangle containing either pole gives NOT_DETERMINED;
closed endpoint contact counts. A pole-free rectangle near a pole uses the explicit reciprocal term
and T4, then either gives a finite enclosure meeting the width target or NOT_DETERMINED/LIMIT.
Do not reject a whole numerical neighborhood of a pole, and do not report local Gamma poles globally.

```c
/* Meromorphic I_chi on S, all-plane version of adf_tate_integral, same finite-character semantics,
   aliasing, bits/precision/work bounds and atomicity. C=1: DOMAIN at exact 0,1;
   NOT_DETERMINED on mixed/undecided pole intersection. C>1: no spectral poles.
   OK means finite, every coordinate diameter <=2^-bits. Cost as the half-plane call.
   Integral path uses only characters, factor/Poisson primitives and elementary ball quadrature;
   acb_dirichlet L/zeta/xi/Hurwitz/theta/AFE functions are forbidden on this path. */
int adf_tate_continue(acb_t z, const adf_char_t chi, const acb_t s, slong bits, slong prec);
/* Lambda_chi=C^((s+e)/2) I_chi, same continuation/status/width/alias/cost contract.
   Computes the balanced split directly; it does not multiply an already width-limited I without
   rechecking width. C=1 has the same poles; C>1 entire. No new result object. */
int adf_tate_completed(acb_t z, const adf_char_t chi, const acb_t s, slong bits, slong prec);
```

## 5. Functional equation: 5.4

The caller computes A=adf_tate_completed(chi,s), B=adf_tate_completed(conj(chi),1-s), then
R=adf_char_root_number(chi)*B. W=tau/(i^e sqrt(C)), with the positive finite Gauss sum, P13.
There is no function that can return an equation by copying or intersecting a side. Use guard targets
for B and W and check final widths of A and R. Both must be finite and overlap, for both parities and
a non-real character. Also compare each to an independent completed FLINT value; overlap alone can
hold for two equal wrong values. Nonoverlap means at least one enclosure, sign, completion, conjugation
or transform contract is wrong. An honest LIMIT/NOT_DETERMINED is a failed acceptance case, not a refutation
of the mathematical equation. C=1 exact poles are status tests, not finite-equality tests.

## 6. Statements proposed for analysis.md

T1. Euler-product tail, an alternative certificate in Re(s)>1.
1. For sigma=lower Re(s)>1 and |chi(p)|<=1, the absolute logarithm of the omitted product p>P is
bounded by sum_(p>P) sum_(k>=1) p^(-k sigma)/k <=sum_(n>P) n^-sigma/(1-P^-sigma).
2. Monotonicity gives sum_(n>P)n^-sigma <=integral_P^infinity x^-sigma dx.
Thus B=P^(1-sigma)/[(sigma-1)(1-P^-sigma)] bounds that logarithm.
3. If E_P is the finite product, |L-E_P|<=|E_P| expm1(B), by the exponential series and triangle inequality.
Multiply by the upper absolute completion factor for a uniform absolute error in I or Lambda.
Since |E_P|<=sum_(n>=1)n^-sigma<=1+1/(sigma-1), an absolute target epsilon is ensured by
B<=b=log1p(epsilon/[K(1+1/(sigma-1))]), where K bounds the chosen completion factor on S.
4. To target B<=b, take integer P>=2 with P>= [2/((sigma-1)b)]^(1/(sigma-1)); since P^-sigma<1/2,
this suffices. Sieve and count all primes <=P; test the work cap BEFORE sieving. There is no claim of
a short cutoff near 1. The oracle's sigma=2,b=1/100,P=201 uses 46 primes and verifies the bound.

T2. Completion against the reference, no normalization by recollection.
1. P11's real integral is pi^-z Gamma(z), all ramified factors are 1, and the others multiply to L.
Thus I=pi^-z Gamma(z)L. Multiplying by C^z gives (C/pi)^z Gamma(z)L.
2. refs/src/flint-3.0.1/acb_dirichlet.rst:567-569 specifies L; :524-534 supplies the Hurwitz decomposition.
The latter is only a reference algorithm here. At nonprincipal s=1 its residues cancel; the oracle uses
-sum chi(a) digamma(a/C)/C [source pending: Hurwitz constant term -digamma(a)]. This numerical reference
identity is not a premise of the continuation certificate; the split independently evaluates s=1.
3. The explicit xi formula in the same file :129-133 is s(s-1)I_zeta/2. Algebraic division proves the
stated conversion away from 0,1. The original split proves the poles; xi's entire values are not residues.

T3. Taylor certificate actually applied to a spectral rectangle.
1. In |u|<m, the binomial series for (m+u)^(z-1) converges: it terminates or the ratio of successive
coefficient terms tends to |u|/m<1. Its derivative solves (m+u)f'=(z-1)f with f(0)=m^(z-1),
hence it is the stated power.
Multiply by the everywhere convergent exponential series. The differential equation
(m+u)g'=((z-1)-b_n(m+u))g gives the recurrence in section 4 by equating powers.
2. On |u|=d<m, Re(m+u)>=m-d and |m+u| is between m-d and m+d; its argument is between -pi/2 and pi/2.
Consequently |g|<=exp(-b_n(m-d)) max((m-d)^r,(m+d)^r) exp(pi |Im z|/2).
3. Uniform absolute convergence on that circle permits termwise integration against exp(-ik theta).
All unequal powers integrate to zero; the coefficient integral gives |c_k|<=B_n/d^k.
Sum the geometric majorant from J+1 onwards on |u|<=h, and integrate over length 2h. This gives T3's
panel bound. Odd powers integrate to zero exactly. Sum over n and panels by the triangle inequality.
4. Every inequality is uniform in S using outward interval endpoints and upper absolute values.
Ball arithmetic contains the finite polynomial integral (arb.rst:6-12); adding the analytic remainder
therefore contains the true integral, including rounding. No special-function enclosure theorem is used.

T4. Enclosure near a pole and the width qualification.
1. Write F(s)=r/(s-s0)+G(s), where G is the two entire split integrals and the other pole term.
For midpoint m and disk radius R<|m-s0|, put d=|m-s0|-R>0. Subtracting reciprocals gives
|1/(s-s0)-1/(m-s0)|<=R/(|m-s0| d). This bound plus ball arithmetic certifies a finite pole term.
2. Disk exclusion is sufficient only. If the rectangle excludes s0 but its covering disk does not,
direct acb division can still certify it; otherwise refuse. Never declare intersection from the disk alone.
3. With C=1, residues are exact +/-1; adding the certified entire integrals proves the returned enclosure.
As s approaches a pole, sensitivity grows like inverse squared distance. Increasing working precision
reduces arithmetic error, not this image width. A fixed ball spanning two distinct regular values has
a positive width floor. The oracle uses the distinct values at s=1.125 and s=1.25 as a witness.

T5. Idele dilation and Poisson cutoffs.
1. For a=(h,r u), f_a(x)=f(ax). Changing variables in the additive transform gives
hat_f_a(y)=|a|^-1 hat_f(y/a), |a|=|h|/r. The finite factor contributes r, the real factor 1/|h|.
Both are required. For diagonal rational a the norm is 1. P7 now gives the Poisson step in section 4.
2. For a=(t,u), t>0, r=1: the finite array is permuted by u, so D,M are unchanged, while a real term
becomes P(t x) exp(-pi A t^2 x^2+B t x+C_real). Use the two existing idele dilation calls atomically.
3. Recompute P1/L6 lattice bounds from the dilated parameters and from their transforms. The left
Gaussian decay is proportional to t^2 and the right decay to t^-2; reusing a common old cutoff is unsound.
For t in a positive ball use the lower transformed decay, not the midpoint. General r can change D,M;
query the resulting finite factor. For balanced vectors t=sqrt(theta_variable), not theta_variable.
4. For a spectral Mellin integral the n/t tail is P15's bound in section 4; a pointwise Poisson tail
alone is not an integrated error. Integrating the common majorant requires L14's factor. This explains
why an accurate theta call at a quadrature point does not by itself certify a Tate integral.

## 7. Acceptance and faults

Every C slice: red test before code, INV on/off, aliases z=s and z=alpha, failure byte snapshots,
where on each _at failure, precision/cap boundary and one above, no allocation/setup before cap refusal.
Reject nonfinite raw balls. Driver and Julia results must contain the oracle value, not just its midpoint.
Tests for every numerical entry point check finite width and the whole input rectangle at selected corners.

5.1: local_exact/local_numeric/real: exact phase/DFT identities; shell integrals; both parities;
local poles, zeros and epsilon at gamma poles. Six faults: inverse omitted; G_minus positive;
alpha omitted; exponent a=1 at conductor 4; parity omitted; unit volume doubled.

5.2: global_numeric/global_flint/euler: zeta(2) completed=pi/6; L(1,chi_4)=pi/4 and Lambda=1;
both parities/non-real/composite C; shrinking widths. Six faults: C^z omitted; wrong real parity;
chi conjugated twice; C^-s omitted from Hurwitz reference; sign-half factor 2; xi treated as Lambda.

5.3: bounds/midpoint/taylor/continuation/poles/input_radii/poisson_dilation: Taylor versus independent
Gamma integrals; both L14 branches; near/at poles; trivial zeros. Six faults: wrong -a/s sign;
zero mode retained; dual s instead of 1-s; L14 factor omitted; discarded n tail; input radius dropped.

5.4: functional_equation/goldens/composite: both sides versus external values, finite width and overlap,
W on all 17 goldens. Six faults: no conjugate; inverse W; negative tau; incomplete completion;
-s instead of 1-s; one side copied from the other.

The oracle's faults_51 to faults_54 give six numerical/exact witnesses each. The copying fault also needs
a C call-count/dependency test: replace one independent evaluator by a disjoint sentinel ball and require
the corresponding side alone to change. Equality of equal fake sides is not a certificate.
Link-wrap acb_dirichlet L/zeta/xi/Hurwitz/theta/AFE entry points to abort in the continuation test binary.
Finite character pairing and our finite Gauss sum remain permitted. Compare to FLINT in a separate binary.
The eight real-character goldens are finite W=1 checks, not the unsourced general sign theorem.
The raw (8,7) golden lowers to conductor 4; do not feed a fictitious primitive conductor 8 to integration.
Near zeta s=1 and s=0, check (s-1)Lambda ->1 and s Lambda ->-1 with shrinking nonzero displacements.
At exact poles status/output preservation replaces overlap. For C>1 test s=0,1 and Gamma-factor poles too.

## 8. Decisions for TJO and thin slices

Only D1-D4 above need decisions: acb/status results; inherited caps plus shared work cap; theta splitting
as the primary algorithm; own Taylor quadrature. Apply each when its slice needs it (workflow rule 1).
Suggested order, each ending in a driver result and a Julia ccall fixture using existing layout queries:
In these signatures P=Ptr{Cvoid}, L=Clong; Place is the existing isbits wrapper with one Culong field,
passed by value (place.h), not an integer fabricated by the caller. Obtain it from the place constructors.

1. 5a, about one hour: adf_local_tate_at, standard unramified and ramified vectors and both real parities.
Use local_exact/local_numeric/real and existing local-factor status tests; D1,D2.
Driver: adf tate-local S with PLACE alpha A char CHI. First examples p=2,alpha=1,eta0=1,s=2 ->4/3;
p=2,alpha=i,eta0=chi_4 ->1; infinity e=0,s=2 ->1/pi. No general local-function integration.
Julia: ccall((:adf_local_tate_at,lib),Cint,(P,P,P,Place,P,P,L),z,where,s,v,alpha,eta0,128).
2. 5b: gamma/epsilon, negative local sum, pole/zero certificates and composite alpha; D2.
Driver: adf local-gamma / local-epsilon with the same operands; local_exact/local_numeric/real/composite.
Julia uses the same ABI arguments as 5a with the corresponding symbol.
3. 5c: vector builder and half-plane value, certified Taylor/tail kernel, initially conductor 1 then
primitive characters in the same user-facing call; D3,D4. This can be two hour lanes (zeta, characters).
Driver: adf tate-integral CHI S --bits 64 --prec 128. global_numeric/global_flint/euler/bounds/midpoint.
Julia: ccall((:adf_tate_integral,lib),Cint,(P,P,P,L,L),z,chi,s,64,128).
4. 5d: all-plane continuation and completed wrapper, exact pole geometry, near-pole enclosures,
independence link test and width-floor test; continuation/poles/input_radii/poisson_dilation.
Driver: adf tate-continue / tate-completed CHI S --bits 80 --prec 160; same five Julia arguments as 5c.
5. 5e: caller-computed functional equation command, both guarded calls plus root number, no new C type.
Driver: adf tate-fe CHI S --bits 80 --prec 160 prints both sides and their widths, not a forced equality.
Julia calls completed twice and root_number once; functional_equation/goldens/composite/faults_54.

## 9. Findings and source obligations

No counterexample is asserted against SPEC/PLAN's stated formulas, conventions or goldens. N-D23's
qualification remains necessary: the s=1.125 versus s=1.25 witness prevents arbitrary width on their hull.
The brief's pole list is the TRIVIAL integral's list: odd real integrals have a pole at -1, while odd
real gamma has a pole at 2. These are distinct functions, computed in real tests; rejecting only the
trivial list for every local call would be wrong. Conventions 6.4 already says "trivial factors".
The eight real golden inputs include imprimitive raw (8,7), correctly lowered by the existing character
constructor. This is a brief qualification, not a counterexample to the golden value tau=2i,W=1.

Ground truth on disk: refs/src/flint-3.0.1/arb.rst:6-12 (enclosure), acb.rst:6-17 (rectangles),
:893-902 (Gamma/rgamma); acb_dirichlet.rst:98-100,129-133,524-534,567-569 (reference normalizations).
refs/src/tate-poonen/notes.txt:62-64 states Gamma's poles and absence of zeros, used for the local pole sets.
The transform and local constants here are project definitions and P9-P15 proofs, not quotations from
an external sign convention. T1-T5 are the additional stepwise arguments; do not modify SPEC to adopt them.
[source pending: inherited analysis Definition 1 Haar/product compactness, Fubini/dominated convergence,
holomorphic integration, identity theorem, Fourier uniqueness, Taylor, CRT and unique factorization]
[source pending: universal FLINT Conrey phase/label identification, inherited from api-3c/api-3d]
[source pending: signed primitive quadratic Gauss evaluation for universal W=1; examples are checked only]
[source pending: upper incomplete Gamma ball documentation and vertical-strip Stirling bound, P15]
The last two P15 obligations are not premises of this quadrature or of an absolute-width contract.
No generic tensor Tate API, derivative in s, all-places product object, fast large-conductor transform,
or meromorphic result serialization is designed; none is required for the SPEC 8 acceptance vectors.
