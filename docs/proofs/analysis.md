# Proofs of the analysis conventions

This file implements the conventions of SPEC sections 5 to 8 and the analysis rows of section 9.3.7.
It does not change the specification. Checks are in `proto/analysis_checks.py`.
Numerical checks are evidence for the formulas. The checks that use FLINT balls are interval certificates
for their stated finite cases, not proofs of the general statements
[source pending: FLINT/Arb ball arithmetic enclosure guarantee].
The convention is a project definition here. Its attribution to Tate remains
[source pending: Tate thesis section 2.2, local copy with readable line references].

## Definition 1. Characters, measures, and analytic prerequisites

For every prime p, including 2, let fp_p(x) be the unique element of Z[1/p] in [0,1) with
x - fp_p(x) in Z_p. Let E(t) = exp(2 pi i t). Define

    psi_p(x) = E(fp_p(x)),    psi_inf(x) = E(-x),
    psi(x) = psi_inf(x_inf) product_p psi_p(x_p),
    F f(y) = integral f(x) conj(psi(x y)) dx.

Only finitely many factors of psi differ from 1. The local additive measures have vol(Z_p) = 1;
the real measure is Lebesgue measure. The multiplicative measures are

    d*x_p = (1 - 1/p)^(-1) dx_p / |x_p|_p,    d*x_inf = dx / |x|.

Thus vol*(Z_p^x) = 1. Products of these measures mean restricted products. No measure is assigned to
an uncertain parameter: formulas for parameter balls mean uniform enclosures of pointwise formulas.
Local test functions mean locally constant compactly supported functions. Real test functions below are
finite sums of polynomial-Gaussians, unless the statement explicitly allows all Schwartz functions.

The following named standard theorems are the only analytic/topological imports not proved here.
Their exact forms used here are recorded to separate source work from the algebraic proofs.

- Haar theorem: a locally compact Hausdorff abelian group has a nonzero translation invariant regular
  measure, unique up to a positive scalar. Its compact open sets have finite measure.
  [source pending: Haar existence and uniqueness theorem]
- Product compactness: a product of compact Hausdorff spaces is compact. In particular Zhat is compact.
  [source pending: product compactness theorem]
- Real character theorem: every continuous homomorphism R -> the unit circle is x -> E(c x), c real.
  [source pending: classification of continuous characters of R]
- Integration theorems: an absolutely integrable function on a product admits iterated integrals with
  the same value; a pointwise convergent family bounded in absolute value by one integrable function
  admits passage of the limit under the integral. A locally uniform such majorant for holomorphic
  integrands permits holomorphic differentiation and integration under the integral.
  [source pending: Fubini, dominated convergence, and holomorphic parameter integration]
- Holomorphic identity theorem: two holomorphic functions on a connected open set that agree on a
  set with an interior accumulation point agree everywhere. It also gives uniqueness of meromorphic
  continuation after poles are removed.
  [source pending: identity theorem]
- Fourier uniqueness on a circle: a continuous periodic function with all Fourier coefficients zero
  is zero. Consequently an absolutely convergent series of its Fourier coefficients represents it.
  [source pending: uniqueness of Fourier series, for example via Fejer convergence]
- Taylor theorem with integral remainder: for a C^N function on a real interval containing 0,
  f(x)=sum_(j=0)^(N-1) f^(j)(0)x^j/j! plus
  x^N/(N-1)! integral_0^1 (1-u)^(N-1) f^(N)(ux) du, for integer N>=1.
  [source pending: Taylor theorem with integral remainder]
- Chinese remainder theorem: for pairwise coprime positive integers m_1,...,m_k, reduction is
  a ring isomorphism Z/(product_i m_i)Z -> product_i Z/m_i Z.
  [source pending: Chinese remainder theorem]
- Unique factorization in Z: every positive integer is a unique finite product of nonnegative
  powers of primes; extending exponents to integers gives every positive rational uniquely.
  [source pending: fundamental theorem of arithmetic]

Here Z_p is the inverse limit of Z/p^k Z: its compatibility conditions define a closed subset
of a product of finite compact spaces, so product compactness proves its compactness. Integer
representatives of its successive coordinates prove that Z is dense in Z_p. Scaling proves that
Z[1/p] is dense in Q_p. Elementary finite group orthogonality is proved when used below.
Gamma is initially defined by
Gamma(z) = integral_0^infinity exp(-t) t^(z-1) dt for Re(z) > 0. Complex powers of positive real
numbers use their real logarithm. These imports, not a general adelic Poisson or Tate theorem, are used.

Check: `check_characters`, `check_real_transform`, `check_continuation` (finite instances only).
Used by: SPEC 5, 6, 7, 8, 9.3.7.

## Lemma 2. Additive character, balls, and the quotient

For the characters in Definition 1, psi is a continuous additive character of A, trivial on diagonal Q.
For a rational a and rational N >= 0 the image of a + N Zhat under psi_f is as follows:
N = 0 gives {E(a)}; N = A/B > 0 in lowest positive terms gives
{E(a) E(k/B): 0 <= k < B}. In particular an integer radius gives one phase.
For a real interval I the image of I x (a + N Zhat) is the product of this finite set and E(-I).
The signs and centres may be negative; negative radii are outside the representation contract.
A/Q has fundamental domain [0,1) x Zhat, with boundary (1,z) identified with (0,z-1), and volume 1.

Proof.
1. fp_p(x+y) - fp_p(x) - fp_p(y) lies in Z[1/p] intersect Z_p = Z. This proves local additivity.
   The kernel contains Z_p; local constancy and the restricted product topology prove continuity.
2. For rational q, q - sum_p fp_p(q) is p-integral at every p: all the summands with index other
   than p have denominator prime to p. A rational integral at every prime is an integer. Hence psi(q)=1.
3. psi_f is trivial on Zhat. For N=A/B, the disjoint cosets of A Zhat in N Zhat are
   kN + A Zhat, 0 <= k < B: divide by N and reduce Zhat modulo B. Their phases are E(kA/B).
   Since gcd(A,B)=1 these are precisely the B-th roots. Translating gives the assertion, including a<0.
4. Given x_f, subtract q=sum_p fp_p(x_p). Step 2's local argument gives x_f-q in Zhat.
   Then subtract floor(x_inf-q). Two representatives in [0,1) x Zhat differ by an integer and hence
   are equal. The displayed gluing subtracts the diagonal integer 1.
5. The neighbourhood (-1/2,1/2) x Zhat meets Q only at zero, so Q is discrete.
   The compact set [0,1] x Zhat covers A/Q, by step 4 and product compactness in Definition 1.
   The half-open fundamental domain has product measure 1. Boundaries have real measure zero.
   Translating this domain partitions it into countably many diagonal-Q translates of pieces of
   the original domain. Haar invariance on those pieces proves invariance of the quotient measure.

No step distinguishes p=2. For radius zero splitting is unnecessary.

Check: `check_characters`.
Used by: SPEC 6; SPEC 7 quotient volume; PLAN 3.2.

## Proposition 3. Local annihilators and self-dual measures

For every prime p, integer k (positive, zero, or negative), and y in Q_p,

    integral_(p^k Z_p) conj(psi_p(x y)) dx = p^(-k) 1_(p^(-k) Z_p)(y).

For rational N>0 the annihilator of N Zhat is (1/N) Zhat and vol(N Zhat)=1/N.
Every continuous additive character of Q_p is x -> psi_p(x y) for a unique y in Q_p.
Every continuous character of A is x -> psi(x y) for a unique y in A.
The chosen measures are self-dual: F(F f)(x)=f(-x) for local test functions and for the implemented
real and adelic class. This is also the normalization of self-dual measure for all Schwartz functions.

Proof.
1. Splitting Z_p into p residue classes gives vol(p Z_p)=1/p. Scaling and iteration give p^(-k).
   The character on p^k Z_p is trivial exactly when v_p(y)+k>=0. If not, translating the integral
   by an element with nontrivial character multiplies it by a scalar other than 1, so it is zero.
2. Products give the finite annihilator. Multiplying p^(-v_p(N)) gives 1/N by prime factorization.
3. A continuous character is trivial on some p^k Z_p. Indeed its image there can be forced into
   an arc around 1 containing no nontrivial subgroup of the circle. To see the latter fact, powers
   of any nonidentity point eventually leave the arc of arguments (-1/4,1/4).
   After scaling, suppose the character is trivial on Z_p. Its value at p^(-m) is E(b_m/p^m),
   with b_(m+1)=b_m mod p^m. These compatible residues define y in Z_p. Additivity proves equality
   on Z[1/p], and density proves equality on Q_p. Scaling back allows arbitrary y in Q_p.
   If two y's give the same character, step 1 at every k shows their difference is zero.
4. A character of A restricted to R has the form in Definition 1's real character theorem.
   Continuity on Zhat forces trivial restriction on Z_p for all but finitely many p, by the same
   small-arc argument. Step 3 therefore gives y_p in Z_p outside a finite set. The product formula
   for the character holds first on points with finitely many nonzero coordinates, then by density.
5. For 1_(a+p^k Z_p), step 1 gives transform p^(-k) conj(psi_p(a y)) 1_(p^(-k) Z_p)(y).
   A second application gives 1_(-a+p^k Z_p). Every local test function is a finite linear
   combination of these indicators. This proves local inversion, also at p=2 without alteration.
6. Real inversion on the implemented class follows from Proposition 5 below, including its
   polynomial differentiation rule. For all Schwartz functions one can insert exp(-pi epsilon y^2)
   into the double transform, use the Gaussian integral of Proposition 5 and Fubini, and obtain
   convolution with epsilon^(-1/2) exp(-pi x^2/epsilon) evaluated at -x. Splitting the convolution
   at |x|=delta and using continuity and Gaussian decay proves convergence to f(-x).
   Dominated convergence applies to the undamped inverse integral because the transform of a
   Schwartz function is integrable: repeated integration by parts bounds it by const/(1+y^2).
7. Tensor products and Fubini give adelic inversion. A positive rescaling c of any one measure
   changes the second transform by c^2. Thus inversion fixes its scalar to c=1.

Check: `check_local_fourier`, `check_finite_fourier`, `check_real_transform`.
Used by: SPEC 6, 7, 9.3.7 local constants.

## Proposition 4. The weighted finite transform

Let D,M be positive integers, L=DM, and let f be zero off (1/D) Zhat and constant modulo M Zhat.
Write f_j=f(j/D), for 0<=j<L. Its transform g is zero off (1/M) Zhat, is constant modulo D Zhat,
and satisfies all three formulas of SPEC 7:

    g_k = g(k/M) = (1/M) sum_(j=0)^(L-1) f_j E(-j k/L),
    integral f = (1/M) sum_j f_j,
    integral |f|^2 = (1/M) sum_j |f_j|^2 = (1/D) sum_k |g_k|^2.

The second transform uses the weight 1/D and returns f(-x). These are support and periodicity
inclusions, not minimality claims. The zero function and D=M=1 are included.

Proof.
1. Apply Proposition 3 to each coset j/D + M Zhat, whose volume is 1/M. Its transform is
   (1/M) E(-j k/L) at k/M, vanishes off (1/M) Zhat, and is D Zhat-periodic.
2. Integrating the step function directly proves the second formula.
3. The geometric sum sum_(k=0)^(L-1) E(h k/L) is L if L divides h and otherwise zero:
   multiply by 1-E(h/L) and telescope. Substitute this sum in the second transform to get
   (L/(DM)) f_(-j)=f_(-j). Substitute it in sum |g_k|^2 to obtain the third formula.

Check: `check_finite_fourier`, `check_local_fourier`.
Used by: SPEC 7; PLAN 4.1, 4.2, 4.4.

## Proposition 5. Real polynomial-Gaussians and their transform

Let P(x)=sum_(j=0)^d p_j x^j be any complex polynomial (zero allowed), A,B,C complex with Re(A)>0.
Let h be nonzero real and a real. Put phi(x)=P(x) exp(-pi A x^2+B x+C). The class of finite sums
of these terms is closed under product, translation T_a phi(x)=phi(x-a), dilation phi(hx), and F.
For translation the new parameters are A, B+2 pi A a, C-Ba-pi A a^2 and polynomial P(x-a).
For dilation they are Ah^2, Bh, C and P(hx). Products multiply polynomials and add A,B,C.
Dilation by zero is excluded: a nonzero constant is not a Schwartz function.
For real y define z=B+2 pi i y and polynomials in z by

    H_0(z)=1,    H_(j+1)(z)=H_j'(z) + z H_j(z)/(2 pi A).
    F phi(y)=A^(-1/2) exp(C+z^2/(4 pi A)) sum_j p_j H_j(z).

The square root is the branch positive for A>0, with argument of A in (-pi/2,pi/2).
Equivalently the new Gaussian parameters are 1/A, i B/A, C+B^2/(4 pi A), with prefactor A^(-1/2).
In particular F(exp(-pi x^2))=exp(-pi y^2) and F(x exp(-pi x^2))=i y exp(-pi y^2).

Proof.
1. Substitution proves the closure formulas. Re(Ah^2)>0 and Re(A+A')>0 give the required domain.
2. Squaring integral exp(-pi x^2) dx, using Fubini and polar coordinates, gives
   integral_0^infinity 2 pi r exp(-pi r^2) dr=1. The integral is positive, hence equals 1.
3. For A>0 and real z, completing the real square gives
   integral exp(-pi A x^2+z x) dx=A^(-1/2) exp(z^2/(4 pi A)).
   Both sides are entire in z and holomorphic in Re(A)>0: on compact parameter sets, a polynomial
   times exp(-c x^2+b|x|) is an integrable majorant. Twice applying the identity theorem in
   Definition 1 extends the formula to complex z and complex A in this half-plane.
4. Differentiation j times with respect to z inserts x^j. The product rule gives the stated
   recurrence. Substituting z=B+2 pi i y gives the positive Fourier sign, with all constants shown.
5. For a pure Gaussian a second transform has amplitude
   A^(-1/2)(1/A)^(-1/2)=1, exponent -pi A x^2-Bx+C. The chosen square roots make this equality
   exact. For polynomial factors use F(x phi)=(2 pi i)^(-1)(F phi)' and
   F(phi')=-2 pi i y F(phi), proved by differentiation and integration by parts with zero boundary
   terms. Induction gives F^2(x^j phi)(x)=(-x)^j phi(-x).
6. Substitution x -> hx gives F(D_h phi)(y)=|h|^(-1) F phi(y/h), including negative h.
   The same change of variables locally gives |a|^(-1) for dilation by any exact idele a.

Check: `check_real_transform`, `check_real_closure`.
Used by: SPEC 7; PLAN 4.3, 4.5.

## Lemma 6. A computable Gaussian series bound

Let c>0, j a nonnegative integer, and T>=0 real. Define K=floor(T)+1 and

    rho_j(c,K)=exp(j/K-c(2K+1)).

If rho_j(c,K)<=1/2, set S_j(c,T)=K^j exp(-c K^2)/(1-rho_j(c,K)). Otherwise increment K to
the first K0 with rho_j(c,K0)<=1/2, add the explicit terms for floor(T)+1<=n<K0, and use that
geometric bound at K0. Then sum_(n>T) n^j exp(-c n^2) <= S_j(c,T). This terminates for every c>0.
For certified balls, compare an upper bound for rho with 1/2; increase K if the comparison is undecided.
Similarly, define S_j(alpha,beta,T) for alpha>0 and beta>=0 by the same rule with

    rho_j(alpha,beta,K)=exp(j/K-alpha(2K+1)+beta),
    first term K^j exp(-alpha K^2+beta K).

It bounds sum_(n>T) n^j exp(-alpha n^2+beta n).
For phi as in Proposition 5, a real, h real and nonzero, expand

    P(a+hn)=sum_j q_j n^j,
    A'=Ah^2, B'=h(B-2 pi A a), C'=C+Ba-pi A a^2,
    alpha=pi Re(A'), beta=|Re(B')|.

A proved bound for the two-sided lattice tail, including negative n, is

    sum_(|n|>T) |phi(a+hn)| <= B_phi(a,h,T),
    B_phi(a,h,T)=2 exp(Re(C')) sum_j |q_j| S_j(alpha,beta,T).

The same bound holds for |sum_(|n|>T) phi(a+hn)|. T need not be integral. The zero polynomial gives
zero. Finite sums are bounded by summing these bounds. The former bound with
Kappa=exp(Re(C')+beta^2/(2 alpha)) and S_j(alpha/2,T) remains valid as a looser corollary.

Proof.
1. For n>=K the ratio of successive terms n^j exp(-alpha n^2+beta n) is at most
   exp(j/n-alpha(2n+1)+beta), since log(1+1/n)<=1/n. This upper bound decreases in n.
   Thus all later ratios are at most rho_j(alpha,beta,K). Once rho<=1/2, a geometric sum bounds
   the remaining tail. Its exponent tends to minus infinity, so the search terminates. Taking
   beta=0 gives the rule for S_j(c,T).
2. The expansion above gives |phi(a+hn)| <= exp(Re(C')) sum_j |q_j| |n|^j
   exp(-alpha n^2+beta |n|): use Re(B')n<=beta |n| for either sign of n.
3. Sum the last bound separately for positive and negative n by step 1. This proves the bound on
   the sum of absolute values and hence on the absolute value of the sum. It proves convergence.
   Finally beta |n| <= alpha n^2/2+beta^2/(2 alpha), by completing the square. Applying the
   first series bound with c=alpha/2 proves the older corollary.

Check: `check_tail_bounds`.
Used by: SPEC 7 Poisson tails; SPEC 9.3.7 theta; PLAN 4.5.

## Proposition 7. Poisson summation over Q, and theta series

Let f(x)=phi(x_inf) f_fin(x_f), with phi as in Proposition 5 and f_fin as in Proposition 4.
Finite sums of such tensors are allowed. Then both sums below converge absolutely and

    sum_(q in Q) f(q) = sum_(q in Q) F f(q).

More explicitly, with g=F f_fin and F_real phi denoted by phi_hat,

    sum_j f_j sum_(n in Z) phi(j/D+Mn)
      = sum_(n in Z) g_(n mod DM) phi_hat(n/M).

For an exact idele x, Theta_f(x)=sum_q f(qx) obeys
Theta_f(x)=|x|^(-1) Theta_(F f)(1/x). The term q=0 is included.
Bounds for truncating the left side at |n|<=T are sum_j |f_j| B_phi(j/D,M,T).
A bound for the right side is max_k |g_k| B_phi_hat(0,1/M,T).

Proof.
1. A rational lies in (1/D) Zhat exactly when it lies in (1/D) Z. This follows by clearing its
   denominator: a rational integral at every prime is an integer. Splitting n modulo DM gives
   the left expression. Lemma 6 proves absolute convergence, also for every differentiated series.
2. Periodize phi with period M. The coefficient of E(-n t/M) is
   (1/M) integral_R phi(t) E(n t/M) dt = phi_hat(n/M)/M.
   Proposition 5 and Lemma 6 give absolute convergence of its Fourier series. Fourier uniqueness
   from Definition 1 identifies this series with the periodization.
3. Evaluate at j/D and sum over j. Proposition 4 gives the right expression and its sign.
   Lemma 6 justifies the exchanges and proves both stated truncation bounds.
4. Apply the result to D_x f and the idele dilation formula of Proposition 5. Its finite factor is
   again compactly supported and locally constant, hence has some finite array: clear the finitely
   many negative valuations of support and choose an integer period contained in all local periods.
   This gives the theta identity with no extra constant, consistently with Lemma 2's volume 1.

Check: `check_poisson`.
Used by: SPEC 7, 8, 9.3.7 theta; PLAN 4.5, 5.3.

## Lemma 8. Primitive characters and signed Gauss sums

Let chi be a primitive Dirichlet character of conductor C>=1. For C>1 extend chi by zero on
nonunits; for C=1 set chi(n)=1 for every integer n, including zero. Define e in {0,1} by
chi(-1)=(-1)^e. The additive character in the following definition has a positive finite sign:

Primitive of conductor C means that chi is a character modulo C and, for every divisor C'<C of C,
there is a unit u modulo C with u=1 mod C' and chi(u)!=1. For C=1 this condition is empty.

    tau(chi)=sum_(a mod C) chi(a) E(a/C).

Then, for every integer m, including zero and negative integers,

    sum_(a mod C) chi(a) E(m a/C)=conj(chi(m)) tau(chi),
    |tau(chi)|=sqrt(C),    tau(chi) tau(conj(chi))=(-1)^e C.

For C=1 these assertions give tau=1 and e=0. For a primitive nontrivial character at 2 the
conductor exponent is at least 2; there is no nontrivial character of conductor 2. No odd-prime
assumption is needed in these formulas.

Proof.
1. For gcd(m,C)=1 substitute b=ma. The resulting coefficient is chi(m)^(-1)=conj(chi(m)).
2. Suppose d=gcd(m,C)>1. Then C'=C/d is a proper divisor. By primitivity choose a unit
   u=1 mod C' with chi(u)!=1. Since d divides m and C' divides u-1, C divides m(u-1).
   Thus E(mua/C)=E(ma/C). Substitution a -> ua permutes the residues and makes the sum equal
   to conj(chi(u)) times itself, so it vanishes. This includes m=0 for C>1, when C'=1.
   For C=1 direct evaluation gives the one-term sum 1.
3. Expand the sum over m of the squared absolute values of these finite sums. The geometric
   orthogonality calculation in Proposition 4 gives C sum_a |chi(a)|^2=C phi(C).
   Steps 1 and 2 give phi(C)|tau|^2 on the other side. Divide by phi(C)>0.
4. Conjugating tau(chi) and substituting a -> -a gives conj(tau(chi))=(-1)^e tau(conj(chi)).
   Combine with step 3. The assertion about conductor 2 follows because its unit group has one element.

Check: `check_gauss_sums`.
Used by: SPEC 8, 9.3.7 Gauss sums and local constants; PLAN 3.4, 5.4.

## Proposition 9. Local Tate integrals and local gamma factors

Fix any prime p, including 2. Let eta:Q_p^x -> C^x be a continuous quasi-character, let
alpha=eta(p)!=0, and let eta_0 be its unit restriction. This restriction has finite image:
compactness makes its modulus 1 and the small-arc argument of Proposition 3 gives an open kernel.
Let a>=0 be its conductor exponent. Thus a=0 means eta_0=1; a>0 means exact conductor p^a.
For any local test function define Z_p(f,eta,s)=integral f(x) eta(x)|x|_p^s d*x.
Its meromorphic continuation satisfies

    Z_p(F f,eta^(-1),1-s)=gamma_p(s,eta) Z_p(f,eta,s).

With exactly the negative finite kernel of Definition 1, the values are

    a=0: gamma_p(s,eta)=(1-alpha p^(-s))/(1-alpha^(-1) p^(s-1)),
    a>0: G_minus(eta_0^(-1))=sum_(u mod p^a, p does not divide u)
                                    eta_0(u)^(-1) E(-u/p^a),
         gamma_p(s,eta)=alpha^a p^(-a s) G_minus(eta_0^(-1)).

The associated normalized local L-factor is (1-alpha p^(-s))^(-1) for a=0 and 1 for a>0.
Defining epsilon=gamma L_p(s,eta)/L_p(1-s,eta^(-1)), epsilon is 1 in the first case and gamma
in the second. Gamma is meromorphic; a pole is not a finite complex value.
The integrals of the test vectors are

    Z_p(1_Z_p,eta,s)=(1-alpha p^(-s))^(-1)             if a=0,
    Z_p(eta_0^(-1) 1_(Z_p^x),eta,s)=1                if a>0,
    Z_p(1_Z_p,eta,s)=0                               if a>0.

The first and third defining integrals are absolutely convergent when
Re(s)>log(|alpha|)/log(p). The unit-supported vector is integrable for every s.
The formulas are uniform at p=2; the ramified case there requires a>=2.

Proof.
1. Each shell p^k Z_p^x has multiplicative measure 1. The unramified test gives
   sum_(k>=0) alpha^k p^(-ks), the displayed geometric series in its convergence domain.
   A nontrivial unit character averages to zero: translate by a unit on which it is not 1.
   This proves the naive zero result on every shell. The inverse-character vector has integrand 1
   on the units and zero elsewhere, hence has integral 1.
2. For the ramified vector write its additive integral as a sum of p^a residue classes.
   The transform vanishes unless y in p^(-a) Z_p by its period. For y=p^(-a)v with v a unit,

       F f(y)=p^(-a) eta_0(v) G_minus(eta_0^(-1)).

   If v is a nonunit the sum vanishes by Lemma 8, applied to eta_0^(-1) modulo p^a.
   Thus its exact possible support is the shell v_p(y)=-a. Integrating there against eta^(-1)
   gives p^(-a) alpha^a p^(a(1-s)) G_minus, the stated gamma.
3. In the unramified case Proposition 3 gives F(1_Z_p)=1_Z_p. The ratio of the two geometric
   integrals is the stated gamma.
4. These ratios hold for all test functions, not merely the displayed vectors. Here is the
   uniqueness argument. In the strip

       log(|alpha|)/log(p) < Re(s) < 1+log(|alpha|)/log(p),

   both distributions T(f)=Z_p(F f,eta^(-1),1-s) and Z(f)=Z_p(f,eta,s) exist. Changing variables
   gives T(D_b f)=eta(b)^(-1)|b|_p^(-s) T(f), and the same law for Z.
   On Q_p^x any two such distributions are proportional: on a small multiplicative coset
   b(1+p^k Z_p), k>=max(1,a), their values are fixed by their value on 1+p^k Z_p and this law.
   Refining k multiplies both by the same index, by finite additivity. These indicators span all
   test functions supported away from zero. The proportionality constant is computed by the
   unit-supported vector for a>0. For a=0 it can also be computed by 1_Z_p after the next step.
5. The difference after this proportionality is supported at zero. For a locally constant f it
   therefore equals c f(0): subtract f(0) times any small ball indicator. Dilation leaves f(0)
   fixed, but its required multiplier for b=p has absolute value p^Re(s)/|alpha|>1 in the strip.
   Hence c=0. This also shows the ratio from the unramified 1_Z_p is the same constant.
6. For any local test function, sufficiently deep shells have its constant value f(0).
   Its Mellin integral is a finite sum plus a geometric series, or zero unit averages there.
   Thus it continues meromorphically. The identity theorem extends steps 4 and 5 to all s.

Check: `check_local_integrals`, `check_local_gamma`.
Used by: SPEC 8, 9.3.7 local zeta factor and local constants; PLAN 5.1.

## Proposition 10. The real Tate integral and gamma factor, both parities

Let e be 0 or 1 and eta(x)=sign(x)^e on R^x. For phi_e(x)=x^e exp(-pi x^2),

    Z_inf(phi_e,eta,s)=pi^(-(s+e)/2) Gamma((s+e)/2),    Re(s)>-e.
    gamma_inf(s,eta)=i^e pi^(s-1/2) Gamma((1-s+e)/2)/Gamma((s+e)/2).

For every real Schwartz f, Z_inf(F f,eta^(-1),1-s)=gamma_inf(s,eta) Z_inf(f,eta,s)
by meromorphic continuation. For eta(x)=sign(x)^e |x|^w, w complex, replace s by s+w.
Set L_inf(s,eta)=pi^(-(s+e)/2) Gamma((s+e)/2). Then epsilon_inf=i^e in the normalization
of Proposition 9. An even Gaussian with odd eta gives zero in Re(s)>0.
The trivial real L-factor has simple poles at s=0,-2,-4,... . Its residue at s=-2k is
2 (-1)^k pi^k/k!, for k>=0. The finite trivial factor has simple poles at
s=2 pi i k/log(p), k any integer, with residue 1/log(p).

Proof.
1. The two signs of x contribute equally for phi_e: x^e sign(x)^e=|x|^e.
   Substitute t=pi x^2 on the positive half-line. This gives the Gamma integral and its domain.
   Oddness gives zero for the naive even vector against odd eta.
2. To prove the equation for all f, first take 0<Re(s)<1 and insert exp(-epsilon |y|) in the
   y integral defining Z_inf(F f,eta,1-s). Absolute convergence permits Fubini. Its kernel is

       Gamma(1-s) [(epsilon-2 pi i x)^(s-1)
                    +(-1)^e (epsilon+2 pi i x)^(s-1)].

   This follows by the Gamma integral, scaling for a positive real Laplace parameter, and the
   identity theorem for parameters with positive real part. The complex powers use arguments
   in (-pi/2,pi/2). As epsilon decreases to zero the kernel tends to
   K_e(s) sign(x)^e |x|^(s-1), with

       K_0(s)=2 Gamma(1-s)(2 pi)^(s-1) sin(pi s/2),
       K_1(s)=2 i Gamma(1-s)(2 pi)^(s-1) cos(pi s/2).

   Its modulus is bounded by a constant depending on s times |x|^(Re(s)-1), uniformly in
   epsilon>0. Integrate this against |f(x)| and use dominated convergence from Definition 1.
   On the y side F f is Schwartz by integration by parts, so dominated convergence applies there too.
3. Evaluate the resulting scalar K_e(s) on phi_e. Proposition 5 gives F phi_e=i^e phi_e;
   step 1 then gives exactly the displayed Gamma ratio. This avoids importing reflection or
   duplication formulas for Gamma.
4. Taylor expansion of f at zero and subtraction of finitely many Taylor terms from its Mellin
   integral over (0,1) continues it to successive half-planes. The remainders are bounded by
   const |x|^N using Taylor's integral remainder. Terms at infinity converge everywhere because
   f is Schwartz. Thus the equation extends meromorphically by the identity theorem.
5. Integration by parts gives Gamma(z+1)=z Gamma(z). Gamma has residue 1 at zero by its defining
   integral split at 1 and exp(-t)=1+O(t). Iterating gives residue (-1)^k/k! at z=-k and no
   other poles. The variable z=s/2 and prefactor pi^(-s/2) give the stated real residues.
   Differentiating 1-exp(-s log p) at each zero gives the finite residues.

Check: `check_real_local`, `check_local_gamma`, `check_poles`.
Used by: SPEC 8, 9.3.7 local constants and local zeta factor; PLAN 5.1.

## Proposition 11. Global test vectors and the conjugate idele character

Let chi,C,e be as in Lemma 8. For an idele with x_f=r u, r positive rational and u in Zhat^x,
put t=|x_inf|/r and u'=sign(x_inf)u. Define omega_chi(x)=conj(chi(u')).
At p not dividing C its uniformizer value is chi(p). At p dividing C its unit restriction is
eta_p(u)=conj(chi_p(u)), where chi=product_(p|C) chi_p is the CRT factorization.
More explicitly its uniformizer value at such p is
alpha_p=product_(l|C, l!=p) chi_l(p). Its real restriction is sign(x)^e.
Let f_chi have real part phi_e, local part eta_p^(-1) 1_(Z_p^x) at ramified primes, and
1_Z_p elsewhere. For Re(s)>1 the defining global integral is absolutely convergent and

    I_chi(s)=Z(f_chi,omega_chi,s)
            =pi^(-(s+e)/2) Gamma((s+e)/2) L(s,chi),
    Lambda(s,chi)=C^((s+e)/2) I_chi(s).

Here L(s,chi)=sum_(n>=1) chi(n)n^(-s) in that half-plane. C=1 is zeta. For C>1, replacing
all ramified vectors by 1_Z_p gives zero where the integral converges.

Proof.
1. The decomposition r=product_p p^(v_p(x_p)) and u=x_f/r is unique. Multiplication by a
   nonzero rational q leaves t and u' unchanged: its positive scale is |q| and its unit part is
   sign(q). This proves that omega_chi is a character of the idele class group.
2. For an idele equal to p at p and 1 elsewhere, r=p. At each conductor prime other than p its
   unit coordinate is p^(-1); at p it is 1. Conjugating chi of these coordinates gives the
   asserted alpha_p, and gives chi(p) when p does not divide C. Using chi(u') without conjugation
   would instead give the conjugate Euler factors.
3. Propositions 9 and 10 evaluate the local factors. Their absolute integrals have unit factors
   at ramified primes and geometric factors (1-p^(-Re(s)))^(-1) elsewhere. The product converges
   for Re(s)>1: expand finite prime products by unique factorization, bound by sum n^(-Re(s)),
   and bound the latter tail by the integral of x^(-Re(s)). Fubini then gives the global product.
4. The same expansion without absolute values gives the Dirichlet series L(s,chi). This proves
   the formula and the factor C^((s+e)/2). The zero assertion follows from Proposition 9.
   An independent reference expression is obtained by splitting this series into residue classes:

       L(s,chi)=C^(-s) sum_(a=1)^C chi(a) zeta(s,a/C),
       zeta(s,u)=sum_(m>=0) (m+u)^(-s),    u>0, Re(s)>1.

   The identity theorem preserves this identity wherever both sides continue. The checks use
   mpmath's Hurwitz-zeta evaluation as a numerical reference, not as an import into the proof of
   continuation. No precision certification of that implementation is claimed here.

Check: `check_global_integral`, `check_idele_character`.
Used by: SPEC 5, 8; PLAN 3.3, 5.1, 5.2.

## Proposition 12. Splitting the adelic integral at norm 1

Let chi be a primitive character as in Lemma 8 and let f be any finite sum of implemented tensors.
Use the probability measure du on Zhat^x (each finite unit quotient is uniformly weighted), and set

    H_f,chi(t)=integral_(Zhat^x) Theta_f((t,u)) conj(chi(u)) du,    t>0,
    delta=1 if C=1, otherwise 0,
    a=delta f(0),    b=delta F f(0).

Then Z(f,omega_chi,s), initially for Re(s)>1, continues meromorphically as

    J_f,chi(s) = integral_1^infinity (H_f,chi(t)-a) t^s dt/t
              + integral_1^infinity (H_(F f),conj(chi)(t)-b) t^(1-s) dt/t
              + b/(s-1) - a/s.

Both integrals are entire functions of s and rapidly convergent. The only possible poles are simple:
residue b at 1 and residue -a at 0. If the corresponding coefficient is zero the singularity is removable.
Furthermore J_f,chi(s)=J_(F f),conj(chi)(1-s).

Proof.
1. Decompose x_f=r u and split the real integral into its positive and negative half-lines.
   With t=|x_inf|/r, set q=sign(x_inf)r in Q^x and replace u by sign(x_inf)u.
   Each local valuation shell has multiplicative measure 1 and dx_inf/|x_inf|=dt/t.
   Thus unfolding gives integral_0^infinity (H_f,chi(t)-a)t^s dt/t, without a factor 2.
   The rational sum itself contains both signs. To justify unfolding, consider one tensor, let
   F0=max_j |f_j|, and use the Gaussian majorant of Lemma 6 step 3. Uniformly in u,

       sum_(q != 0) |f(q(t,u))|
       <= 2 F0 Kappa sum_j |p_j| sum_(n>=1) (t n/D)^j
                            exp(-alpha t^2 n^2/(2 D^2)),

   where alpha=pi Re(A), beta=|Re(B)|, and Kappa=exp(Re(C)+beta^2/(2 alpha)). For each j,
   g_j(x)=x^j exp(-alpha x^2/2) is nonnegative, rises to one maximum m_j, then falls, and has
   finite integral I_j on [0,infinity). A sum of g_j over points spaced t/D is at most
   m_j+(D/t) I_j: compare the terms on each side of the maximum with their adjacent intervals.
   Hence the displayed sum is at most c1+c2/t for t>0, for constants independent of u.
   Step 4 gives rapid decay for t>=1. The integral of (c1+c2/t)t^(Re(s)-1) over (0,1)
   is finite for Re(s)>1; the rapid bound handles (1,infinity). Apply Fubini first to the
   absolute values, then to the original integrand. Finite sums of tensors follow by addition.
2. Proposition 7, then u -> u^(-1), gives

       H_f,chi(t)=t^(-1) H_(F f),conj(chi)(1/t).

   Haar probability on units is invariant under inversion, as is seen on every finite quotient.
   The character average of a constant is delta by finite group orthogonality.
3. Split the unfolded integral at t=1. In its lower part substitute t -> 1/t and use step 2.
   The decaying part becomes the second integral in the statement. The two remaining elementary
   integrals are integral_0^1 b t^(s-2)dt=b/(s-1) and
   -integral_0^1 a t^(s-1)dt=-a/s. This proves the signs of both pole terms.
4. Uniform decay can be checked before unit averaging. If f_fin has support (1/D) Zhat and
   F0=max_j |f_j|, then q=n/D. For t>=1, putting alpha=pi Re(A), beta=|Re(B)| gives

       |H_f,chi(t)-a|
       <= 2 F0 exp(Re(C)+beta^2/(2 alpha))
          sum_j |p_j| D^(-j) sum_(n>=1) n^j t^j exp(-alpha n^2 t^2/(2 D^2)).

   This bound also holds for nontrivial chi because it bounds the nonzero rational terms before
   averaging. Here C in the exponential is the Gaussian parameter, not the character conductor.
   For n,t>=1, n^2 t^2 >= (n^2+t^2)/2. Split the exponential accordingly: the n-series is
   summable by Lemma 6, and t^j exp(-alpha t^2/(4D^2)) decreases faster than every power.
   Lemma 14 and Proposition 15 below give explicit integral majorants, also on compact s sets.
   Holomorphic parameter integration from Definition 1 makes the two integrals entire.
5. Apply the same formula to F f and conjugate chi at 1-s. Double transform is reflection by
   Proposition 3, and Theta_(f(-.))=Theta_f by q -> -q. The two integrals and pole terms interchange.

Check: `check_poisson`, `check_continuation`, `check_global_integral`.
Used by: SPEC 8, 9.3.7 theta; PLAN 5.3, 5.4.

## Proposition 13. Primitive theta identity and the completed functional equation

For chi,C,e as in Lemma 8 and real t>0 define

    Theta_chi(t)=sum_(n in Z) chi(n) n^e exp(-pi n^2 t/C),
    W_chi=tau(chi)/(i^e sqrt(C)).

At n=0 the summand is 1 for C=1 and zero for C>1. Then

    Theta_chi(t)=W_chi t^(-e-1/2) Theta_conj(chi)(1/t),
    Lambda(s,chi)=W_chi Lambda(1-s,conj(chi)).

W_chi W_conj(chi)=1 and |W_chi|=1. The completed L-function is entire for C>1. For C=1 its
only poles are simple, with residue -1 at s=0 and +1 at s=1.
The root number uses the positive finite Gauss sum in Lemma 8, despite the negative Fourier kernel.

For real primitive chi, the identities above imply W_chi^2 = 1; they do not select its sign. The further
assertion W_chi = 1 is
[source pending: a local source or proof of the signed primitive quadratic Gauss sum evaluation].
Until supplied, compute W_chi by the finite Gauss-sum formula, including for real characters.
The real-character golden vectors check examples, not the universal signed evaluation.

Proof.
1. Periodize x^e exp(-pi t x^2/C) modulo C as in Proposition 7 and sum over residues weighted
   by chi. Proposition 5 gives its positive real transform

       i^e (C/t)^(e+1/2) y^e exp(-pi C y^2/t).

   Lemma 8 gives the finite coefficient sum_a chi(a) E(-ma/C)=(-1)^e conj(chi(m)) tau(chi).
   Multiply the periodization weight 1/C and y^e=(m/C)^e. The constant is
   i^e (-1)^e tau(chi)/sqrt(C)=tau(chi)/(i^e sqrt(C)). This proves the theta identity.
2. In Re(s)>1 termwise integration gives

       Lambda(s,chi)=(1/2) integral_0^infinity
                        (Theta_chi(t)-delta) t^((s+e)/2) dt/t,

   where delta is as in Proposition 12. Each positive n contributes its Gamma integral and
   the negative n contributes the same value because chi(-1)(-1)^e=1.
3. Split at t=1 and use step 1. With z=(s+e)/2, z'=(1-s+e)/2, and
   V_chi(t)=sum_(n>=1) chi(n)n^e exp(-pi n^2 t/C), obtain the directly computable formula

       Lambda(s,chi)=integral_1^infinity V_chi(t) t^(z-1) dt
                    +W_chi integral_1^infinity V_conj(chi)(t) t^(z'-1) dt
                    +delta [1/(s-1)-1/s].

   At C=1, e=0, W=1, the constant terms are (1/2)/(z-1/2) and -(1/2)/z,
   which are precisely the two displayed pole terms. Proposition 15 step 1 gives a majorant
   integrable locally uniformly in z. Holomorphic parameter integration from Definition 1
   makes both integrals entire.
4. Lemma 8 gives W_chi W_conj(chi)=1. Exchange z and z' in step 3 to prove the functional
   equation everywhere by continuation. The pole residues follow from the explicit rational terms.
5. This is also Proposition 12's splitting for the prescribed adelic vectors. In fact
   H_f,chi(u)=u^e sum_n chi(n)n^e exp(-pi n^2 u^2).
   Set t=C u^2 to get step 2 after multiplying I_chi by C^((s+e)/2).
   Equivalently Proposition 4 gives the finite transform
   (tau(chi)/C) conj(chi(-k)) at k/C. Combining with F phi_e=i^e phi_e gives
   H_(F f),conj(chi)(u)=i^(-e) tau(chi) H_f_conj(chi),conj(chi)(u/C)/C.
   Consequently the uncompleted equation is I_chi(s)=i^(-e) tau(chi) C^(-s) I_conj(chi)(1-s).
   Completing both sides gives exactly W_chi, including the conductor and parity factors.

Check: `check_theta`, `check_functional_equation`, `check_continuation`.
Used by: SPEC 8, 9.3.7 Gauss sums and theta; PLAN 5.3, 5.4.

## Lemma 14. An exponential integral bound for every real power

For b>0, real r, and R>=1, let J(r,b,R)=integral_R^infinity t^r exp(-b t)dt and rplus=max(r,0).
If b>=2 rplus/R then

    J(r,b,R) <= R^r exp(-b R)/(b-rplus/R)
             <= 2 R^r exp(-b R)/b = J_bound(r,b,R).

Otherwise r>0. Put R0=2 rplus/b>R and t*=min(max(r/b,R),R0). Then

    J(r,b,R) <= (R0-R) (t*)^r exp(-b t*) + 2 R0^r exp(-b R0)/b = J_bound(r,b,R).

For r<=0 the first case always applies. Both branches are finite and cover equality at the threshold.

Proof.
1. Write t=R+u. If r>=0, log(1+u/R)<=u/R bounds t^r by R^r exp(ru/R).
   If r<0, t^r<=R^r. In both cases integrate the resulting exponential in u>=0.
2. The derivative of r log t-bt is r/t-b. Since r>0 in the second branch, the integrand rises
   up to r/b and falls thereafter. Its maximum on [R,R0] is at t*. The interval integral is
   bounded by its length times this maximum. At R0, b-rplus/R0=b/2, so step 1 supplies the
   remaining term. In the first branch, b-rplus/R>=b/2, proving its second inequality.

Check: `check_tail_bounds`.
Used by: SPEC 8 continuation; PLAN 5.3.

## Proposition 15. Explicit truncation and quadrature certificates

For Proposition 13 let a0=pi/C, e in {0,1}, z any complex number, r=Re(z)-1, N>=0 an integer,
and R>=1. Approximate integral_1^infinity V_chi(t)t^(z-1)dt by integrating only 1<=n<=N
and 1<=t<=R. The two omitted regions have absolute bounds

    E_sum <= exp(a0) S_e(a0,N) J_bound(r,a0,1),
    E_integral <= exp(a0) S_e(a0,0) J_bound(r,a0,R).

Use the two branches of Lemma 14 for J_bound.
The same formulas apply to conjugate chi and z'. Add the errors with multiplier |W_chi|=1.
For a uniform pointwise theta bound at any t>=1, the omitted positive terms are bounded by
S_e(a0 t,N); for the full two-sided theta multiply by 2. All bounds tend to zero as the
corresponding cutoffs tend to infinity. Error estimates are absolute, also near the poles.
They do not depend on Im(s). On a fixed vertical strip the Gamma factor of Lambda has the
exponential factor exp(-pi |Im(s)|/4), up to powers of |Im(s)|
[source pending: a vertical-strip Stirling bound for Gamma]. Away from zeros, relative accuracy
can therefore need roughly 0.34 |Im(s)| extra decimal digits; near zeros it can need more.

For a composite midpoint rule on [1,R] with K>=1 equal subintervals, h=(R-1)/K, put

    U_l=max(1,R^(Re(z)-1-l)),    l=0,1,2,
    M2=sum_(n=1)^N n^e exp(-a0 n^2)
          [a0^2 n^4 U_0 + 2 a0 n^2 |z-1| U_1 + |(z-1)(z-2)| U_2].

Then its absolute quadrature error on the finite integrand is at most (R-1)h^2 M2/24.
If each midpoint value is enclosed with absolute error epsilon_k, add h sum_k epsilon_k.
R=1 gives a zero interval and zero quadrature error. Terms with gcd(n,C)>1 vanish and may be
omitted, but keeping them in the majorant is valid. The midpoint rule has order h^2 and is a
fallback. The main method integrates each term exactly:

    integral_1^R exp(-a0 n^2 t)t^(z-1)dt
      = (a0 n^2)^(-z) [Gamma(z,a0 n^2)-Gamma(z,a0 n^2 R)],

where Gamma(z,x)=integral_x^infinity exp(-u)u^(z-1)du is the upper incomplete Gamma function.
For R=infinity, the second Gamma term and E_integral vanish. Ball evaluation of this function
is [source pending: FLINT documentation for upper incomplete Gamma enclosure].

For the general adelic bound of Proposition 12 put c=alpha/(2D^2) and
K_j=2F0 exp(Re(C)+beta^2/(2alpha)) |p_j|D^(-j). For a Mellin exponent v (s or 1-s), put
r_j=Re(v)+j-1. Omitting |n|>N in the unit-averaged nonconstant theta integral costs at most

    sum_j K_j exp(c) S_j(c,N) J_bound(r_j,c,1).

Omitting t>R costs at most sum_j K_j exp(c) S_j(c,0) J_bound(r_j,c,R).
These bounds also cover a finite sum of tensors by addition.

Proof.
1. For n>=1 and t>=1, (n^2-1)(t-1)>=0, so n^2 t>=n^2+t-1. Thus
   exp(-a0 n^2 t)<=exp(a0) exp(-a0 n^2) exp(-a0 t). Lemmas 6 and 14 bound the two
   omitted regions. Overcounting their intersection is harmless. The same majorant on compact
   z sets proves locally uniform convergence of the integrals in Proposition 13. For the
   general adelic estimate, (n^2-1)(t^2-1)>=0 gives n^2 t^2>=n^2+t^2-1>=n^2+t-1.
   Substitute c for a0, include
   t^j in the power r_j, and apply the same two bounds. Each term tends to zero as its
   cutoff grows. For exact term integration, put u=a0 n^2 t. Then dt=du/(a0 n^2) and
   t^(z-1)dt=(a0 n^2)^(-z)u^(z-1)du, which gives the upper incomplete Gamma difference.
2. Differentiate the finite integrand twice. The nth term, without chi(n), is

       n^e exp(-a0 n^2 t)
       [a0^2 n^4 t^(z-1)-2a0 n^2(z-1)t^(z-2)+(z-1)(z-2)t^(z-3)].

   On [1,R] each power is bounded by the corresponding U_l and exp(-a0 n^2 t)<=exp(-a0 n^2).
   This proves the M2 bound, for complex z as well as real z.
3. Taylor's integral remainder about a midpoint bounds the remainder by M2 |t-midpoint|^2/2.
   The linear term integrates to zero. Integrating this majorant gives M2 h^3/24 per interval.
   Summing proves the quadrature bound; the evaluation-error bound follows by the triangle inequality.
4. For Proposition 12 use the inequality in step 1 termwise. To certify its finite-interval
   quadrature, unit averages reduce to a finite quotient and hence a
   finite sum of polynomial-Gaussians phi(t n/D). Their second derivatives follow by the product
   rule; polynomial and exponential absolute bounds on [1,R] give M2 exactly as in steps 2 and 3.
5. On parameter balls, replace each real exponent and coefficient by an outward upper bound,
   and require a strictly positive lower bound for Re(A) or a0. Propagate the root-number and
   rational pole-term errors by ball arithmetic. A ball meeting a nonremovable pole returns a pole
   status or a meromorphic object, not a finite value. Increasing N,R,K and evaluation precision
   makes these explicit bounds arbitrarily small away from the poles. The midpoint bound is
   conservative and need not be the final fast quadrature algorithm used in milestone 5.

Check: `check_tail_bounds`, `check_quadrature_bound`, `check_continuation`, `check_continuation_bounds`.
Used by: SPEC 7, 8, 9.3.7 theta; PLAN 4.5, 5.3, 5.4.

## Statement register

Here AT denotes the integration, holomorphic, Taylor, and Fourier uniqueness theorems in Definition 1.
HT denotes its Haar, product compactness, and real character theorems. No statement is open as a
mathematical formula; the named source dependencies remain open for the sources gate.
The elementary CRT and unique factorization imports are also dependencies where explicitly used.
The status column lists new imports in that argument; dependencies on earlier statements are inherited.

| Number | Content | Status | Check |
|---|---|---|---|
| 1 | Conventions and explicit imported theorems | proved modulo AT and HT | check_characters |
| 2 | Character, fractional balls, quotient | proved modulo HT | check_characters |
| 3 | Annihilators and self-dual measure | proved modulo AT and HT | check_local_fourier |
| 4 | Weighted array formulas and reflection | proved here | check_finite_fourier |
| 5 | Polynomial-Gaussian closure and positive transform | proved modulo AT | check_real_transform |
| 6 | Lattice Gaussian tail algorithm | proved here | check_tail_bounds |
| 7 | Adelically normalized Poisson and theta | proved modulo AT | check_poisson |
| 8 | Signed primitive Gauss identities | proved modulo CRT | check_gauss_sums |
| 9 | Finite local integrals and gamma | proved modulo Haar and identity theorems | check_local_gamma |
| 10 | Real integrals, parities, gamma and poles | proved modulo AT | check_real_local |
| 11 | Global integral and idele character | proved modulo AT, HT, CRT and factorization | check_global_integral |
| 12 | Norm splitting and pole residues | proved modulo AT and Haar theorem | check_continuation |
| 13 | Completed primitive functional equation | proved modulo AT | check_functional_equation |
| 14 | Exponential integral tail for any real power | proved here | check_tail_bounds |
| 15 | Series, integral and quadrature errors | proved modulo Taylor theorem | check_quadrature_bound |

## Review record

Date: 2026-09-27. Reviewer: Claude fable. Review: `docs/reviews/m0-proofs/analysis-review.md`.
Verdicts: 9 VALID, 6 MINOR, 0 INVALID.

- R1: Lemma 6 now bounds the sum of absolute values with a ratio cutoff of 1/2; the old bound is a corollary.
- R2: Lemma 8 defines primitive conductor and uses that definition in the zero Gauss-sum case.
- R3: Proposition 12 now estimates the small-norm sum before unfolding and cites existing bounds.
- R4: Proposition 13 cites Proposition 15 and states the locally uniform convergence needed for entire integrals.
- R5: Lemma 14 chooses its split point and bounds the finite interval by the true maximum.
- R6: Proposition 15 uses the sharper split, exact incomplete Gamma integrals, and corrected omission rules.
- R7: The checks kill all three surviving mutants and cover the cases listed as untested in the review.
- E4: the gate closure check (`docs/reviews/m0-gate/closure.md`, 2026-09-28) added the scope note before the
  proof of Proposition 13: for real primitive chi the identities give W_chi^2 = 1 and do not select its sign;
  W_chi = 1 is [source pending: a local source or proof of the signed primitive quadratic Gauss sum
  evaluation]; W_chi is computed by the finite Gauss-sum formula until it is supplied.
