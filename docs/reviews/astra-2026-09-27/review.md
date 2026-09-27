# Review of the adelfeld design, 2026-09-27

The basic finite-ball arithmetic is sound. Do not freeze the C interface yet: exact rational embedding, capped precision, quotient reduction, function spaces, and the analytic acceptance test need explicit contracts. Several performance ratios currently compare unlike quantities or use an algorithm cost as a lower bound.

Evidence labels used throughout: **[proved here]** means a derivation, counterexample, or design conclusion supported by the argument given here; **[checked by a run]** means a recorded program/header/document inspection, not a mathematical proof; **[from memory]** means an explicitly unverified statement or planning estimate. A recommendation labelled proved here is a consequence of its stated requirements, not a theorem that its engineering tradeoffs are uniquely optimal. Source retrievals are checked by a run; bibliographic links do not imply that an entire work was audited. Replacement text inherits the evidence labels on its finding. This opening verdict is **[proved here]** by the findings below.

## 1. Verdict table

| ID | Location | Severity | Finding |
|---|---|---|---|
| M1 | SPEC 4.2; proto/precision_rules.py | MINOR | Both rules are tight; replace finite sampling as justification with a proof covering zero radii. |
| M2 | SPEC 2, 3, 4.1, 14 | MAJOR | One rational ball represents a basic finite-adelic neighbourhood, not arbitrary finite information or a single p-adic place. |
| M3 | SPEC 2, 4.1, 12; PLAN 4, 5, 1.5 | BLOCKER | An arb cannot hold an exact non-dyadic rational; overlap is not equality of represented sets. |
| M4 | SPEC 4.3 | MINOR | The non-unit conclusion is right, but the stated all-primes argument ignores denominators of the centre. |
| M5 | SPEC 5; PLAN 2.1–2.4, 3.3 | MAJOR | Idele decomposition is right; mixed precision, real rounding, quasi-characters and the forgetful map need qualifications. |
| M6 | SPEC 6; PLAN 3.1 | BLOCKER | Fundamental-domain reduction of a ball can wrap and produce correlated pieces. |
| M7 | SPEC 6; PLAN 3.2 | BLOCKER | A fractional finite radius need not determine one additive-character value; sign must be fixed with the Fourier convention. |
| M8 | SPEC 7; PLAN 4.1, 4.3 | BLOCKER | Finite Fourier support is right, but the Haar factor and sign are missing. |
| M9 | SPEC 7; PLAN 4.2 | MAJOR | Polynomial times the standard Gaussian is a subspace, not all test functions, and lacks the promised closure. |
| M10 | SPEC 7; PLAN 5 | BLOCKER | The same spherical test function gives zero for a ramified twist; continuation and certification are unspecified. |
| M11 | SPEC 8; PLAN 6.2–6.3 | MAJOR | Equation solving confuses exact, modular and local solution sets and lacks certificates/completeness contracts. |
| M12 | SPEC 8; PLAN 6.1 | MAJOR | Rational reconstruction needs explicit bounds and cannot establish a local-to-global principle. |
| M13 | SPEC 2, fact 1 | MINOR | There are continuum many profinite idempotents, and the prime-power argument should be stated. |
| D1 | SPEC 15; PLAN 3–4, 1.3; PERF 4.1 | BLOCKER | Fixed absolute precision is not closed under rational scaling; scale times residue needs proved alignment rules. |
| D2 | SPEC 4.1, 9; PLAN 4–5, 1.4 | MAJOR | Proposed structs do not implement the promised storage forms or denominator model. |
| D3 | SPEC 4.1, 7–9; PLAN 3–4 | MAJOR | Complex function values are not complex adeles of Q; several public result types and lifecycle rules are missing. |
| D4 | SPEC 3; PLAN 7 | MAJOR | Leave extension seams now; a polynomial radius omits the function-field infinity place. |
| P1 | SPEC 11; PLAN 6 | MAJOR | Dependencies and version-one scope disagree; freeze mathematical contracts before the ring milestone. |
| P2 | PLAN acceptance columns; proto/precision_rules.py | MAJOR | Several tests are circular or vacuous and do not check tightness, precision or completeness. |
| P3 | PLAN estimates and house rules | MINOR | Solving and certified integration are under-specified for the quoted sizes; use measurable exits. |
| F1 | PERF 1, 4.3 | MAJOR | Zen 2 multiply throughput is wrong; instruction forms, vector widths and clocks need explicit qualification. |
| F2 | PERF 2 | MAJOR | Space floors mix different state spaces; the ball and denominator counts are incomplete. |
| F3 | PERF 3 | BLOCKER | Several time floors are invalid as universal floors or compare throughput with latency. |
| F4 | bench/baseline.c; PERF 3 | MAJOR | The harness measures two chains and several repeated calls, not a uniform latency benchmark. |
| F5 | PERF 4; PLAN decision 1b | MAJOR | The ratios do not establish an order-of-magnitude opportunity or a gcd-dominated general implementation. |
| F6 | PERF 5 | MAJOR | Add modelled I/O floors and separate algorithm costs for CRT, DFT, reconstruction and vector kernels. |
| A1 | SPEC 13 | MINOR | Hertogh's package exists and is directly relevant; uniqueness and full scope are not established. |
| A2 | SPEC 12; PLAN 7 | MAJOR | Class groups are not certifiable only under GRH; FLINT's number-field support needs a narrower description. |

## 2. Findings in full

### M1 — the precision rules survive review

**Document:** “Both are tight: no smaller ball contains the result. [checked]”.

**Assessment [proved here].** Keep the formulas. Define the rational gcd of all zeros to be zero. For positive rational R, `R Zhat intersect Q = R Z`: a rational integral at every prime is an integer. Consequently containment of rational differences in `R Zhat` is just integer divisibility after dividing by R. Use this fact to prove minimality without a limiting numerical experiment.

For addition put `G=gcd(N,M)`. Bezout after clearing denominators gives `N Zhat + M Zhat = G Zhat`; hence the sum is exactly the displayed ball. This includes N=0 or M=0 and G=0.

For multiplication, with independent `u,v in Zhat`, the difference from ab is

```
(a+Nu)(b+Mv)-ab = aM v + bN u + NM uv.
```

Each coefficient lies in `G Z`, where `G=gcd(aM,bN,NM)`, so every difference lies in `G Zhat`. Conversely the products obtained at `(u,v)=(0,0),(0,1),(1,0),(1,1)` have differences `0,aM,bN,aM+bN+NM`. Any ball containing them contains ab; its translation subgroup must contain `aM,bN,NM`, and thus G. It therefore contains `ab+G Zhat`. If G=0 all three coefficients vanish, the product is the singleton ab, and radius zero is correct. Clearing denominators and using absolute gcds covers rational radii, negative centres, zero centres, and exact inputs. No positivity of centres is assumed. “Tight” means smallest *enclosing ball* for independent choices of inputs; it does not assert preservation of correlations when the same unknown is used twice.

**Correction.** Replace the [checked] justification by the preceding proof and retain the prototype as a regression check. Radius comparison is divisibility, not ordinary numerical order: `R Zhat subset S Zhat` iff `R/S` is an integer, for positive R,S. The analogies with a scalar real radius must not obscure this partial order.

**Checks [checked by a run].** The supplied prototype is rerun in `checks/prototype.txt`. It uses rational centres/radii and signed centres, and does include radius zero. Its four witness pairs already establish the sampled gcd for every input; 6000 repetitions do not prove enclosure on arbitrary profinite points. The algebra above does. Every example in SPEC 4.2 is correct, including `1/3 mod 1/6 = 0 mod 1/6` and exact multiplication by 12 and 1/3.

### M2 — precisely what one finite ball represents

**Document:** “at any finite precision the whole list ... is carried by one rational centre a and one radius N”; “as cheap as a big integer”; “a single prime power (the p-adic case)” (PLAN 4).

**Assessment and evidence [proved here].** This is right for a basic product neighbourhood with local balls at a finite set S and `Z_p` elsewhere. Write its radii as `p^n_p Z_p`, with integer n_p, zero off S. Set `N=product p^n_p`. Choose an integer d supported in S large enough that all d-scaled centres and radii are integral. Approximate each local centre to its required precision by an integer and apply CRT to find A satisfying those congruences. Then a=A/d has the required local residues and is integral off S. The neighbourhood is `a+N Zhat`. This is the finite-adele strong-approximation statement needed here, with an elementary proof for Q.

Conversely every such rational ball is a basic compact open neighbourhood for N>0. Each actual finite adele has a global integer denominator because only finitely many coordinates are nonintegral. This justifies “allowing denominators”. It does not mean its centre is an exact rational value, or that every finite description of a set is one coset. For example the residues {0,1} modulo 3 form two cosets and not one ball. Radius zero extends the representation by rational singletons only.

A radius `p^k` specifies information at *all* primes: it includes an integrality constraint away from p. It is not a Q_p ball. Partial adeles need an explicitly specified set of places and projection semantics. For rational N, say `v_p(N)=0`, not “p does not divide N”. Numeric smallness also misleads: `(2/3)Zhat` and `Zhat` are incomparable. The particular example `(1/6)Zhat` does contain `Zhat` and means `6x in Zhat`.

**Correction [proved here].** Use one rational coset as the basic finite-adelic enclosure for Q, retain separate local/partial types, and promise big-integer-based arithmetic rather than big-integer cost. A rational centre, rational radius, normalisation and gcds require more information and work than one integer.

### M3 — exact points, set equality and overlap are three different contracts

**Document:** “The rational number q sits inside ... `(q ; q)`, exactly”; “Equality ... otherwise overlap test”; “Equality of two inexact adeles is undecidable”.

**Counterexample [proved here].** A finite-precision arb midpoint is dyadic. No dyadic equals 1/3, since `3m=2^k` has no integer solution. With radius zero an arb therefore cannot represent the exact real value 1/3. The proposed struct cannot meet the exact embedding or text-roundtrip contract for arbitrary rational q. A nonzero-radius conversion can enclose q, but is not the exact diagonal point.

For finite balls, set equality is decidable: positive-radius balls are equal iff `N=M` and `(a-b)/N in Z`. They overlap iff `(a-b)/gcd(N,M) in Z`. `a+N Zhat subset b+M Zhat` iff `N/M in Z` and `(a-b)/M in Z`, with the obvious singleton cases for zero radii. A positive-radius ball cannot fit inside a singleton. These statements follow from subgroup sum and membership. Overlap is not transitive: `0 mod 2` overlaps `0 mod 1`, which overlaps `1 mod 2`, but the first and last are disjoint. Equality of the represented real intervals is also decidable from their finite endpoints; equality of unknown points inside them usually cannot be established. Those are different assertions.

**Correction [proved here].** Either add a tagged exact-global-rational variant, with explicit conversion to ball enclosures, or change every exact-embedding promise to enclosure. The former matches the brief better. Provide `equal_set`, `overlaps`, `contains`, and a three-way point-comparison result. Do not overload equality with overlap. An exact rational tag also preserves the correlation between the archimedean and finite components which a Cartesian-product ball loses.

### M4 — the non-unit argument needs one extra finite exceptional set

**Document:** “at every prime not dividing N ... a + Z_p ... includes multiples of p”.

**Counterexample and proof [proved here].** For a=1/2, N=1, the ball at p=2 is `1/2+Z_2`; it contains no multiple of 2. The conclusion still holds: choose any prime outside the numerators/denominators of N and the denominator of a. There `a+N Z_p=Z_p`, so one may choose that coordinate to be zero. Independently choosing all other coordinates gives a finite adele in the ball that is not invertible. Indeed there are infinitely many eligible primes.

**Correction [proved here].** Every positive-radius finite-adelic ball contains non-units, so it cannot certify a denominator invertible in A. Exact nonzero rationals and separately certified ideles can be divisors. A locally nonzero family need not be an idele: `(p)_p` is an adele with no zero coordinate, but its inverse `(1/p)_p` is not an adele. Require units at almost all primes, not just nonzero coordinates.

### M5 — ideles: correct decomposition, qualified precision

**Document:** “`A_f^x = Q_{>0} x Zhat^x`”; “Multiplicative precision never degrades”; “Its characters are `t^s` times a Dirichlet character”.

**Proof [proved here].** An invertible finite adele has valuations n_p zero at almost all p. Set `r=product p^n_p >0` and u=x/r. Then u is a unit at every prime. Uniqueness follows because the only positive rational with all valuations zero is 1. Conversely ru is invertible. This is a topological group decomposition when the rational factor has the discrete topology, not its usual topology as a subset of R. Exact knowledge of r is part of idele precision.

For integer `N>=1`, define `U(N)=ker(Zhat^x -> (Z/NZ)^x)`. The pair `(c,N)`, gcd(c,N)=1, means the unit coset `c U(N)` (any unit lift of c), not the additive ball `c+N Zhat`. Inversion keeps U(N). Independent products at moduli N and M have subgroup `U(N)U(M)=U(gcd(N,M))`: check each prime, where the two congruence kernels are nested. This loses the finer input precision when moduli differ. At fixed N there is no additional *finite multiplicative* precision loss. Arb multiplication/inversion still rounds and can widen the real interval. A “non-zero ball” must mean a ball excluding zero, not merely a nonzero midpoint.

At each finite prime `|x_p|=p^(-v_p(r))`; the product is `1/r`. Thus the norm `|x_inf|/r` and the product formula for rational q are correct. Dividing by `sign(x_inf) r` gives class coordinates `t=|x_inf|/r` and `u'=sign(x_inf)u`. The sign on the finite unit is essential. The classes are indeed `R_{>0} x Zhat^x`.

**Correction [proved here].** Call `t^s chi(u)` a continuous quasi-character for arbitrary complex s, and a unitary character when `Re(s)=0`. The real positive factor has a unique real logarithm, so `t^s=exp(s log t)` has no branch choice. A continuous character of Zhat^x has finite image and factors through a finite modulus: continuity and a sufficiently small arc around 1 give an open kernel because the circle has no nontrivial subgroup contained in that arc. Include the conductor as well as a presentation modulus. Evaluation on a unit coset is a single value only when its uncertainty subgroup U(N) lies in the character kernel; divisibility of N by the conductor is the standard sufficient condition. Otherwise return an enclosure of the character image or a precision status. “The whole of GL_1” is an informal motivation, not a scope specification.

The forgetful map can return `(rc, rN)` as an additive enclosure, but must say it is an enclosure, not an exact representation of the unit coset. A tighter hull uses `L=lcm(N,2)` and an odd integer c' congruent to c modulo N, returning `(rc',rL)`. At p=2 all units are odd; at every odd prime unrestricted units have differences generating Z_p. These observations and the specified congruences prove that hull tight. Test multiplication compatibility by containment/overlap as appropriate, not equal sets after discarding unit information. Computing a valuation at a specified prime needs divisibility tests, not factorisation of the entire r; enumerating all nontrivial valuations may need factorisation.

### M6 — reduction modulo Q is a set operation

**Document:** “subtract the fractional part of a ... then an integer to bring the real end into `[0,1)`”; “If N is not an integer the class is known too coarsely to reduce”.

**Proof and counterexamples [proved here].** For a point, choose q with finite part x_f-q in Zhat (M2), then subtract the integer floor of x_inf-q. The resulting representative in `[0,1) x Zhat` is unique: any two differ by a rational integral at every prime, hence an integer, and their real coordinates force that integer to be zero. Q is discrete because `(-1/2,1/2) x Zhat` meets Q only in zero. The image of compact `[0,1] x Zhat` covers A/Q, proving compactness. These verify SPEC 2, fact 2. The half-open fundamental domain is not itself a compact product model of the quotient topology; the boundary identifies `(1,z)` with `(0,z-1)`.

For a real interval `[0.9,1.1]` and finite ball `0 mod 2`, reduction gives two pieces: real coordinates near 1 with finite part `0 mod 2`, and real coordinates near 0 with finite part `-1 mod 2`. One ball cannot encode that correlation or be a unique reduced point. Ordinary closed arb intervals also cannot represent a half-open endpoint exactly.

Noninteger N does not prevent reduction mathematically. Write `N=A/B>0` in lowest terms. The ball splits into B cosets

```
a + N Zhat = union_{k=0}^{B-1} (a+kN + A Zhat).
```

This follows since `NZhat/AZhat` has B elements. Each piece now admits the integer-radius procedure. N=1/2 already gives the two cosets `Zhat` and `1/2+Zhat`. Radius zero admits the exact finite-rational procedure too.

**Correction [proved here].** Add an additive quotient enclosure type, preferably with a lift and equivalence semantics, and optionally a finite union of fundamental-domain pieces. A routine restricted to one chart must return a precise “needs split” status on boundary crossings or fractional-radius splitting, with a caller-controlled output-size limit. “Too coarse” describes that API restriction, not a nonexistence theorem. Rational-translation tests must check the represented quotient set and width, not just overlap.

### M7 — fix both signs and the character's precision domain

**Document:** “a complex ball of modulus one computed from x_inf and the fractional part of a”; “sign convention ... quotation from Tate's thesis”.

**Source check [checked by a run].** Tate's original thesis, §2.2, PDF pages 10 and 12 (zero-based pages 9 and 11), defines the real additive map by minus the real coordinate and the p-adic map by its rational p-primary fractional part. The scan explicitly says “Note the minus sign”. His Fourier transform uses the negative exponential of that additive map. Adopt both choices together. This is fixed by [Tate, *Fourier Analysis in Number Fields and Hecke's Zeta-Functions*, §2.2](https://sites.math.rutgers.edu/~alexk/2023S572/Tate1950.pdf), not by a guessed modern convention. The web text of this scan was inspected; the shell download failed DNS, recorded in `checks/retrieval.txt`. No source-on-disk completion is claimed.

**Convention [proved here, following that checked source].** Write `e(t)=exp(2 pi i t)` and `{x_p}_p` for the rational p-primary representative of x_p modulo Z_p. Set

```
psi(x)=e(-x_inf + sum_p {x_p}_p),
hat f(y)=integral_A f(x) conjugate(psi(xy)) dx.
```

The sum has finite support modulo Z. A rational q minus the sum of its p-primary fractional parts is an integer (clear its denominator and use CRT), so psi(q)=1. The real Fourier kernel is `e(+xy)` and the finite kernel on rational centres is `e(-xy)`. Conjugating BOTH choices is an equally valid alternative, but changing just one local sign breaks triviality on Q. A non-even shifted function distinguishes the two Fourier conventions; a Gaussian does not.

For integer N the finite character is constant on `a+N Zhat` and equals e(a), so evaluate `e(a-x_inf)` with certified real-ball error. For `N=A/B` in lowest terms, its finite image is `e(a)` times all B-th roots of unity. In particular `0+(1/2)Zhat` contains 0 and 1/2 and gives both 1 and -1. The centre alone is insufficient. Return an enclosure of the full image (or an explicit finite union), or a status saying the requested single phase is not determined. Exact finite radius zero is allowed. A rectangular acb enclosing an arc does not itself have modulus exactly one; only the true values do.

### M8 — the finite Fourier table needs Haar weights

**Document:** “length D M ... supported on `(1/M) Zhat` and constant modulo D”; “finite Fourier transform of the array”.

**Proof and correction [proved here].** Specify positive integers D,M and zero extension outside the support. Use additive Haar measure with `vol(Z_p)=1` and ordinary Lebesgue measure at infinity. Then `vol(N Zhat)=1/N` for positive rational N, by multiplying the local factors `p^(-v_p(N))`. The annihilator of `N Zhat` under psi_f is `(1/N)Zhat`: the local condition is `v_p(y)+v_p(N)>=0`. Thus support and periodicity interchange exactly as claimed (inclusions; actual support may be smaller).

Put `L=DM`, `f_j=f(j/D)`, and use the M7 convention. The complete array formula is

```
g_k = hat f(k/M) = (1/M) sum_{j=0}^{L-1} f_j exp(-2 pi i j k/L),
0 <= k < L.
```

Each input coset has measure 1/M. The output is zero off `(1/M)Zhat`, constant modulo `D Zhat`. The second transform has factor 1/D, so orthogonality of roots of unity gives `hat(hat f)_j=f_{-j}` because `L/(MD)=1`. The integral is `(1/M)sum f_j`. Plancherel is `(1/M)sum |f_j|^2=(1/D)sum |g_k|^2`. A unitary `1/sqrt(L)` DFT or a plain unnormalised DFT is not this array map unless the weights are converted explicitly. Real Lebesgue measure and finite volume 1 give quotient volume `vol(A/Q)=1` by the domain in M6; this is the normalisation in Poisson summation. Additive and multiplicative measures must not be conflated.

**Poisson for the implemented subclass [proved here].** For `f(x)=phi(x_inf) f_fin(x_f)`, rational support is on `(1/D)Z`, so the left side is `sum_j f_j sum_{k in Z} phi(j/D+Mk)`. Periodise the Schwartz function phi with period M. Its absolutely convergent Fourier series, with the real positive-sign transform, gives `sum_k phi(t+Mk)=(1/M)sum_n hat phi(n/M)e(-nt/M)`. Substituting t=j/D and the array formula turns the left side into `sum_n hat phi(n/M)g_n`, exactly the sum of the global transform over Q. Polynomial-Gaussian decay justifies the exchanges, and finite sums of products follow by linearity. Numerical truncation still needs a quantitative bound.

**Diagnostic [checked by a run].** `checks/math_checks.txt` uses D=2,M=3 and a nonsymmetric complex array: double-transform error `1.56e-15` or less, weighted Plancherel error `8.89e-16` or less. This is floating-point evidence for indexing, not a certified error bound. The orthogonality argument is the proof. Exact cyclotomic arithmetic or acb roots with certified enclosure should be used in acceptance tests.

### M9 — define an implementable function subspace and its operations

**Document:** “The test functions ... f_inf: a polynomial times a Gaussian”; “translation, dilation by an idele”.

**Counterexample [proved here].** Schwartz functions include smooth compactly supported nonzero functions, whereas a finite polynomial times a fixed Gaussian is analytic and cannot have compact support. Also `exp(-pi(x-1)^2)/exp(-pi x^2)=exp(2pi x-pi)` is not a polynomial: translation leaves the fixed-Gaussian polynomial class. Multiplication changes its width; arbitrary dilation changes its width too. Calling Hermite-transform formulas exact does not make transcendental coefficients exact acb values.

**The Gaussian assertion itself [proved here].** Under the chosen positive real Fourier sign, let `G(y)=integral exp(-pi x^2)e(xy) dx`. Integration by parts gives `G'(y)=-2pi y G(y)`; the Gaussian integral gives G(0)=1, hence `G(y)=exp(-pi y^2)`. Multiplication by x corresponds to `(2pi i)^-1` differentiation in y, so a polynomial times this Gaussian transforms into another polynomial times the same Gaussian. This verifies the claim for the fixed-width subspace while leaving the closure defects above.

**Correction [proved here].** Say “version one implements a specified subclass of Schwartz–Bruhat functions”. A useful closed symbolic family at the real place is finite sums `P(x) exp(-pi A x^2+B x+C)` with `Re(A)>0`, with exact symbolic operations where available and certified coefficient/parameter enclosures otherwise. Complete the square to transform each Gaussian and differentiate for each polynomial factor. The positive-real-part branch selects `A^(-1/2)`; exact symbolic identities and numerical evaluation are separate layers. Products and exact real translations/dilations remain in this family. If parameters are uncertain, the object represents a family of functions; error on coefficients alone must not silently stand in for parameter uncertainty.

For finite arrays choose a common support/period before addition or multiplication and charge the potentially large new `DM`. An uncertain adelic translation may move an indicator among different cosets; evaluating it on a ball that crosses a discontinuity must enclose all possible values. Likewise an idele *ball* is a family of dilations, not one exact operation on one exact function. Initially accept exact transformations or prove sufficient input precision for an unambiguous finite permutation. Define whether dilation means `f(ax)` or `f(a^{-1}x)`; use `D_a f(x)=f(ax)`, giving `hat(D_a f)(y)=|a|^{-1} hat f(a^{-1}y)` and `integral D_a f=|a|^{-1} integral f` by substitution.

### M10 — repair the Tate acceptance test before implementing it

**Document:** “Gaussian ... indicator of Zhat ... completed zeta ... the same with a Dirichlet character”; PLAN: “functional equation ... grid in the critical strip”.

**Untwisted calculation [proved here].** Define

```
Z(f,omega,s)=integral_{A^x} f(x) omega(x) |x|^s d^x x,
d^x x_inf=dx/|x|,
d^x x_p=(1-p^-1)^-1 dx_p/|x_p|_p.
```

Thus every `Z_p^x` has multiplicative volume 1. For `f_inf=e^(-pi x^2)`, integration on both signs gives `pi^(-s/2) Gamma(s/2)`. At p, `1_{Z_p}` gives `sum_{k>=0} p^(-ks)=(1-p^-s)^-1`. The product is the claimed completed zeta for `Re(s)>1`. The real factor converges for `Re(s)>0`; the global Euler product needs `Re(s)>1`. Specify the Gaussian's pi and the measure; otherwise constants are undetermined.

**Twisted counterexample [proved here].** At a ramified prime, averaging a nontrivial unit character against `1_{Z_p}` is zero: multiply the variable by a unit on which the character is not 1. The integral equals that nonunit scalar times itself. Modulo 3 the average is `(1+(-1))/2=0`. At infinity the even Gaussian paired with an odd character also integrates to zero. Thus “the same” test function does not return a nonzero completed L-function.

**Correct test [proved here].** For a primitive Dirichlet character chi of conductor C and parity epsilon, choose the global idele quasi-character's finite unit convention so its unramified uniformizer values are `chi(p)`. With the M5 class coordinate u this means `omega_chi=conjugate(chi(u))`, not `chi(u)` if the target is `L(s,chi)` rather than `L(s,conjugate(chi))`. At ramified p use `f_p=omega_p^{-1} 1_{Z_p^x}`; its local integral is 1. At unramified p use `1_{Z_p}`. At infinity use `x^epsilon exp(-pi x^2)` against `sign(x)^epsilon`. Their product is

```
pi^(-(s+epsilon)/2) Gamma((s+epsilon)/2) L(s,chi).
```

Multiplying by `C^((s+epsilon)/2)` gives the commonly completed `Lambda(s,chi)`. State explicitly whether this scalar is part of the test function or the returned completion. Imprimitive characters need separate missing Euler factors; presentation modulus is not conductor. This sign/conjugation claim follows directly by evaluating u at an idele with one p-uniformizer: at conductor primes its unit coordinate is p^-1.

**Certification contract [proved here].** A numerical integral or Euler product is not automatically certified by using acb internally. Prove truncation bounds and quadrature bounds. In the critical strip evaluate a meromorphic continuation, not the divergent defining global integral; reject balls meeting a pole or return a separately specified meromorphic object. A theta-splitting/Poisson construction with explicit Gaussian tails supplies an independent route. The functional equation must include the conductor, parity, conjugate character and root number. With `tau(chi)=sum_{a mod C} chi(a)e(a/C)`, use `Lambda(s,chi)=tau(chi)/(i^epsilon sqrt(C)) Lambda(1-s,conjugate(chi))`. Its phase must be derived in the analytic proof using M7, not inferred from a symmetric Gaussian test. **[from memory]** This last standard functional-equation formula is stated here as the proposed target; its full Gauss-sum derivation is a required proof work package, not a claim that this review has proved analytic continuation.

**Library target check [checked by a run].** FLINT exposes `acb_dirichlet_zeta`, `acb_dirichlet_l`, and the differently normalised `acb_dirichlet_xi`. At s=3 the probe reports `xi overlaps completed_zeta=0` and `xi overlaps 3*completed_zeta=1`; the desired completed zeta here is `zeta(3)/(2 pi)`. Build the reference completion explicitly; do not mistake xi for the target.

Comparison with `acb_dirichlet` tests the new integral path only if that path has not itself called the same completed-L evaluator. Require certified ball overlap and a radius target at increasing precision, including non-real characters and both parities. Infinite-width output must fail. **[proved here]** These requirements follow from the counterexamples and the meaning of certification.

### M11 — give equation solvers honest domains and outputs

**Document:** “Otherwise a coset of solutions or none”; “Hermite and Smith forms modulo N (`fmpz_mat`)”; “Newton and Hensel are the same iteration”.

**Proof and qualification [proved here].** In any ring the exact solutions of `ax=b`, if x0 exists, are `x0+ann(a)`. With a an idele the annihilator is zero and `a^-1 b` is the unique solution. The first row is algebraically correct, but the proposed types cannot represent an arbitrary annihilator or exact a. Non-idele need not mean several solutions: `(p)_p` from M4 is a non-zero-divisor, so `ax=a` has just x=1 while `ax=1` has none in A_f. Distinguish existential choices of ball coefficients, equations for all represented coefficients, and enclosure of solutions for fixed but unknown coefficients.

Modulo N, `Ax=b` is a finite module problem. One valid factorisation-free route solves the integer lattice problem `Ax-Ny=b` by HNF with transformations. Given integer SNF `UAV=S`, the ith congruence is solvable iff `gcd(S_ii,N)` divides `(Ub)_i`; zero rows impose the corresponding zero condition modulo N. Recover x via V and return generators and relations for the kernel. Merely calling SNF to get diagonal entries does not produce x or kernel generators. **[checked by a run]** Installed FLINT 3.0.1 exposes `fmpz_mat_hnf_transform` but its `fmpz_mat_snf` declaration returns the diagonal matrix, not U,V; “modular HNF” routines still compute integer HNF with auxiliary bounds. See the header evidence in `checks/environment.txt`.

Polynomial roots over A must satisfy the real equation and every local equation, with the restricted-product condition retained. For a polynomial over Q, roots at primes outside its finitely many bad coefficients/leading coefficient are integral, so that condition is automatic there. A modular root need not lift to a Q_p root: 1 is a root of `x^2+1 mod 2`, but there is no root modulo 4. Simple-root Hensel lifting has a proved uniqueness condition; multiple roots need branching and a different certificate. Newton's rational update `x-f(x)/f'(x)` is shared, but archimedean convergence and p-adic lifting criteria are not interchangeable. “Roots at all primes” may require global arithmetic, but density information alone does not decide every exceptional prime or produce roots. No universal equivalence with a splitting-density computation should be promised.

**Correction [proved here].** Separate finite-module solvers, simple-root local lifting, singular-root enumeration and real-root isolation. Return a particular solution plus a kernel description for linear congruences, and disjoint certified isolating balls/branches with a completeness status for roots. Empty, incomplete and too-coarse results must differ. Tests must verify equations, certificates and completeness independently. `arb_fmpz_poly_complex_roots` is available locally; converting its output to complete real-root certificates is its own contract, not simply “roots by arb”.

### M12 — full adelic reconstruction is not ordinary modular reconstruction

**Document:** “Rational solutions from adelic ones | local-to-global | rational reconstruction”; “a rational number of bounded height is determined ... lattice reduction recovers it”.

**Key proof [proved here].** For the actual proposed ball,

```
(a+N Zhat) intersect Q = a+N Z,       N>0.
```

This is M1's intersection fact. Intersect that arithmetic progression with the real interval I: enumerate integers k for which `a+Nk in I`. If width(I)<N there is at most one, with no height bound needed. If there are several, height restrictions may filter them; if there are none, there is no diagonal rational in the ball. For N=0 the only candidate is a and one tests its real membership. This elementary exact-interval task is the first reconstruction routine the full-adele type needs.

Ordinary modular reconstruction has a different input: `n/d == c mod m`, with `gcd(d,m)=1`, at only the primes dividing m. For example `1/5 == 5 mod 6`, but `1/5` is not in `5+6 Zhat`, since `(1/5-5)/6=-4/5` is not integral. The constraints at the other primes cannot be discarded silently. Partial local data must use a separate type.

For the modular problem, bounds `|n|<=A`, `0<d<=B`, coprime n,d and `2AB<m` ensure uniqueness: two candidates give the multiple of m `n1*d2-n2*d1`, of absolute value at most 2AB, hence zero. An interval of width less than `1/B^2` independently separates reduced rationals with denominators at most B. These are sufficient criteria, not necessary ones; coarser data may still have a unique candidate. Distinguish “no candidate”, “multiple candidates” and “uniqueness not certified by this algorithm”. Extended Euclid/continued fractions suffice for standard bounded modular reconstruction; a general LLL dependency is unnecessary for this first case.

**Explicit failure of a local-to-global inference [proved here].** The monic polynomial `(x^2-13)(x^2-17)(x^2-221)` has no rational root, since none of 13,17,221 is a rational square. It has a real root and a root in every Q_p. At an odd p other than 13,17, either 13 or 17 is a nonzero square modulo p, or both are nonsquares and their product 221 is a square; the chosen simple root lifts. At 13 use 17 congruent to 4; at 17 use 13 congruent to 8^2. At 2 use 17 congruent to 1 modulo 8: an odd root modulo 2^n can be adjusted by 2^(n-1) to choose the next square bit for n>=3, giving a convergent 2-adic square root. Simple-root lifting at odd p follows by adjusting x to x+t p^n and solving the next linear congruence using the unit derivative 2x. All roots used are integral locally. Thus one obtains an adelic root with no diagonal rational root.

**Correction [proved here].** Rename the row “reconstruction of a bounded rational candidate”. Verify the candidate in every original equation exactly. Local solvability is not a proof of a rational solution; reconstruction only finds a rational when the specified bounds/data identify one. This distinction fixes both the scope and the acceptance tests.

### M13 — idempotents and the ring assertions

**Document:** “two solutions at every prime independently, so infinitely many altogether”; “modulo N ... 2^k”.

**Proof [proved here].** In the integral domain Z_p, `x(x-1)=0` implies x=0 or 1. Choices at the countably many primes are unconstrained, giving `2^aleph_0` idempotents of Zhat. An idempotent other than 0 and 1 has a complementary nonzero idempotent and their product is zero, proving the claimed zero divisors. Modulo `p^e`, consecutive integers are coprime, so `p^e | x(x-1)` forces x=0 or 1 modulo `p^e`; CRT gives `2^omega(N)` for positive integer N, including one class for N=1. The eight listed residues modulo 30 are correct. Say “continuum many” and restrict the modular count to positive integer moduli.

### D1 — choose an actual capped policy, not an analogy

**Document:** “every value is a residue modulo N times a rational scale ... widened to the context's radius”; “the product is one modular multiplication”; “padic ... caps the precision in its context”.

**Counterexample [proved here].** Set context modulus K=2 and start from `1+2 Zhat`. Multiplication by exact 1/2 has tight result `1/2+Zhat`. Replacing its radius by 2 gives `1/2+2 Zhat`, which excludes 3/2. This is a narrowing, not an enclosure. The check reports `contains = False`. Arbitrary rational scaling is not closed at fixed absolute radius.

There are three coherent designs; do not mix their contracts. **[proved here]**

1. A fixed finite ring `Z/KZ` with integral values supports add/mul at fixed K; exact division is allowed only by invertible residue classes. This is L0, not all of A_f.
2. An absolute cap C stores each value's effective rational radius R. After a tight operation replace R by `gcd(R,C)`. The containing-ball criterion proves soundness. Coarser input precision remains coarser. Per-value radii remain necessary.
3. A scaled fixed-residue representation `s(u+K Zhat)`, with positive rational s, integer `0<=u<K`, and integer `K>=1`, has effective radius sK. For `x=s(u+K Zhat)`, `y=t(v+K Zhat)`, set `g=gcd(s,t)`, `A=s/g`, `B=t/g`, both integers. Define

```
x+y -> g ((Au+Bv mod K)+K Zhat),
xy  -> st ((uv mod K)+K Zhat).
```

The sum is tight. The product's tight radius is `stK*gcd(u,v,K)`, which is a multiple of stK, proving enclosure. A product can lose genuine precision (e.g. u=v=0, K>1) but never invent it. Exact scalar multiplication changes s and, for negative scalars, the sign of u; exact zero gets an explicit exact-zero branch. Exact scalar addition needs scale alignment and may widen. Operations across different K need an explicit common target and conversion contract.

**Design correction [proved here].** Scaled residues are a sound optional policy with these rules, useful when scale alignment is cheap. They are not a fixed absolute cap, nor always one modular multiplication in total: rational scale multiplication/normalisation and addition's rational gcd cost work. Conversion from an arbitrary tight ball may lose precision. Exact preservation would require `s=R/K` and `a/s in Z`; for `1+2 Zhat`, K=3, that fails. A sound positive-radius conversion uses `s=gcd(a,R/K)`, sets u=a/s mod K, and proves `R/(sK)` is integral. Exact values should retain their separate tag. State conversions' loss, and compare policies from the same mathematical inputs, not from already differently widened inputs.

**Checks [checked by a run].** `checks/math_checks.py` passed 20,000 random scaled addition/product enclosure comparisons against the tight formulas, and the fixed-absolute-radius counterexample failed as expected. The installed `padic_struct` contains `u,v,N`; N belongs to each element. `padic_ctx_struct` contains p and a cached power range min/max, not the precision cap of every value. This was inspected in `/usr/include/flint/padic.h:32` and saved in `checks/environment.txt`. Remove the incorrect claim about FLINT's context. The correspondence is with precision *policies*, not with that struct layout.

### D2 — keep global/local arithmetic, redesign their storage contract

**Document:** “a and N as FLINT integers over a common denominator”; PLAN: `fmpq_t centre; fmpq_t radius;`; “a value records which form it is in”; PLAN puts `storage` in the context.

**Assessment [proved here].** The semantic split is useful, but these are two distinct layouts and neither proposed struct has a residue vector or storage tag. A tight result changes its radius, so a context whose product of q_i is permanently N cannot describe all tight results. Factorisation of N alone is not enough to convert rational centres: `1/2+Zhat` has N=1 but nonintegral data at 2. Also one prime power can exceed 2^32 and cannot be split into coprime powers of that same prime for CRT.

**Concrete replacement design [proved here].** Make the semantic ball independent of the kernel representation. A useful global integer layout is `(A,H,d)` representing `(A+H Zhat)/d`, with `H>=0`, `d>0`. For H>0 reduce `0<=A<H` and cancel `gcd(A,H,d)`; for H=0 use a canonical exact rational. It has one denominator, unlike two independently reduced fmpq values. Alternatively retain the two-fmpq layout for simplicity, but say so and benchmark its rational normalisation costs.

The local form can store that same denominator d, integral H, pairwise coprime factor blocks q_i with product H, and residues of A modulo q_i. The denominator scaling is applied *after* CRT, never by trying to invert a denominator divisible by q_i. Cache the factorisation, CRT/remainder tree and reduction constants in an immutable modulus context; store per-value scale/radius and an explicit backend tag. Exact H=0 is not a modular residue. Permit big prime-power blocks to use an fmpz backend. Generic coprime blocks work for CRT and ring arithmetic even if their prime factorisations are unknown; certified prime powers are needed for algorithms that name primes.

Changing moduli requires rebuilding/reusing a suitable context or falling back to global storage. Operations must specify output aliasing, context compatibility, ownership of arrays and scratch space, and failure output state. A structure-of-arrays batch API is a better SIMD seam than assuming every individual object has a useful short vector. Factorisation can be a supplied certificate/cache, never an implicit cost hidden in ordinary global ring arithmetic.

**FLINT check [checked by a run].** Embedded `fmpq_t`, `fmpz_t`, `arb_t`, `acb_t` (arrays of one) are legal C and initialise/clear with their FLINT routines. `fmpq` is two fmpz components, not an mpq or shared-denominator handle. `fmpz` is tagged small-integer/heap data; do not copy owning structs by assignment or assume it is a raw limb. `nmod_t` is a modulus/reduction descriptor passed by value; the residues are `ulong`. `fmpz_mod_ctx_t` is an array-of-one context and the residue is an `fmpz_t`; it is not a numeric `fmpz_mod` value. CRT entry points are present for both fmpz moduli and word-residue combination trees (`fmpz_multi_CRT_precompute`, `fmpz_multi_CRT_ui`, `fmpz_multi_mod_ui`). Choose the matching API and own its scratch context. `checks/flint_probe.txt` confirms the proposed all-small tight ball occupies 32 bytes and adele 80 bytes; those are x2 of a 16-byte two-integer layout and x80/29 of PERF's restricted 29-byte model respectively, not equivalent representations.

Canonical value printing and physical storage serialization are different promises: SPEC requires the forms to print identically and also roundtrip the storage form. **[proved here]** Give `print_value` a canonical semantic output and a separate versioned dump containing backend/context metadata if retaining the physical form is required. For real balls use lossless dyadic serialization; FLINT 3.0.1 already has `arb_dump_str`/`arb_load_str`. A friendly decimal `+/-` string is an enclosure input, not automatically a lossless dump.

### D3 — complete the type inventory before freezing the header

**Document:** “`adf_adele_c` ... with an acb ball ... complex-valued functions need it”; PLAN lists only the core structs.

**Counterexample [proved here].** A function `f:A_Q -> C` has an ordinary Q-adele as its argument and a complex value as its result. Replacing R by C gives the different ring `C x A_Q,f`; it is not the adele ring of Q. Nor is it the adele ring of a number field with a complex embedding, whose finite completions also change. Keep the user's desired complex facility, but name its mathematical domain explicitly and keep complex function values separate.

**Correction [proved here].** Add or reserve public contracts for: exact global scalars; finite and partial adelic balls with explicit place sets; local balls; finite unit cosets; additive quotient enclosures and finite unions; finite test-function arrays with D,M and acb coefficient precision; archimedean test-function expressions; character/conductor contexts; matrix/kernel solution descriptions; root certificates/completeness; and certified integral/meromorphic results. These can be opaque handles or small explicit structs in version one, not a universal abstract hierarchy. Refuse ambiguous implicit conversions.

Separate working real precision from finite precision and from the immutable base-field/modulus identity. Allow operations to request real working precision explicitly or via an immutable operation context. Specify init/clear, deep-copy/set, swap, in-place aliasing, parser size limits, failed-operation output state, invalid NaN/infinite-ball policy and thread safety. No hidden global state does not by itself supply these contracts. `ADF_NOT_UNIT` should distinguish “proved non-unit” from “enclosure does not certify a unit”; modular inversion failure is another domain. “Overlap”, “needs split”, “not unique”, “unsupported backend”, and “resource limit” are not interchangeable errors. These distinctions are forced by M3, M6, M10 and M11.

### D4 — the extension seam must precede the second implementation

**Document:** SPEC: “interfaces ... abstract global field”; PLAN 7.1: “Extract ... from milestones 1–3”; PLAN 7.2: “radius is a polynomial”.

**Assessment [proved here].** Q-specific kernels first is sensible; promising that public Q structs need no rewrite while hard-coding one real ball and one rational scale is not. Decide the semantic seams before the public header, and test them on paper on one non-principal ideal and on the infinity place of F_q(T). Implement the second backend later. Do not build an abstraction for every restricted product, algebraic group and higher adele in milestone 0.

For number fields, a basic finite ball is `a+I Ohat`, with a in K and a fractional ideal I. The Q proof generalises with ideal sum and product: add radius `I+J`, product hull radius `aJ+bI+IJ`. A non-principal ideal has no single scalar “scale”; archimedean components are a vector of real and complex places with fixed embeddings. Finite ideles map to fractional ideals, and a positive rational generator is special to Q. The class group measures the obstruction to a principal-ideal decomposition; global units complicate reduction. The additive character uses trace and its annihilators involve the different, so copying the Q Fourier radius rule literally is wrong. **[from memory]** These ideal/trace statements are the standard extension target; implement them only after their own source/proof package. The elementary fact that a non-principal ideal lacks a scalar generator already proves the proposed rational-scale API is too restrictive.

For F_q(T), the affine primes can use monic polynomial moduli only after a distinguished infinity place is stored separately as `F_q((1/T))`. If all places are treated uniformly, a precision radius is a divisor with separate integer exponents, not a single polynomial. Multiplication by T has valuation +1 at T and -1 at infinity, so “polynomial radius at every place” loses required data. Residue fields have q^degree elements, there is no ordered positive scale, and the analogue of the idele norm has discrete image rather than all positive reals. Constants `F_q^x` are the kernel of principal-divisor valuation data, so the Q decomposition cannot be copied unchanged. **[proved here]** These examples suffice to reject the proposed universal radius/sign fields.

The elementary restricted-product definition should require compact open **subrings/subgroups**, not arbitrary compact open “pieces”, to guarantee a ring/group, and specify its restricted-product topology. For example, take infinitely many copies of the finite discrete ring Z/3Z and the compact open subset {1}; the restricted set contains the all-ones family but not its sum with itself. **[proved here]** Thus “pieces” alone does not ensure closure. G(Z_p) needs an integral model, at least away from finitely many primes. Infinite arbitrary place subsets need a computable membership/description contract; `A_S` is ambiguous unless the convention says “over S” or “away from S”. Finite-dimensional algebras/groups can motivate later interfaces, but higher adeles are not just another locally compact restricted-product instance. **[from memory]** That last higher-adelic warning is a scope caution, not a claim that an implementation has been surveyed.

### P1 — reorder by contracts and dependencies

**Document:** SPEC 11 makes milestone 6 the second field; PLAN makes milestone 6 solving and milestone 7 the second field; SPEC 1 says version one ends at Tate, while SPEC 8 also promises solvers.

**Correction [proved here].** Publish one release scope and one milestone map. Before 1.1: resolve M3/D1/D2 invariants, prove membership/containment and precision rules, freeze character/Fourier/measure conventions, and decide the exact scope of analytic certification. Then build the global tight finite ball and a small CLI with lossless output. Add exact scalar/real enclosures; then one capped policy, then the local backend only when its conversion invariants are tested. Ideles precede division and idele characters. Additive quotient representation precedes fundamental-domain reduction. Finite functions and their weighted DFT precede the more expensive archimedean expression and integral layers. Basic rational intersection/reconstruction can be a small earlier slice; certified singular roots and module solvers should not quietly ride inside the Tate deadline.

The dependencies follow from the counterexamples above: an early backend split without a radius contract implements the wrong semantics twice; a completion test without measure conventions is ambiguous; a public header with only one real field cannot later acquire function-field infinity by a flag. The second-field implementation can remain late, but an explicit compatibility sketch is a pre-milestone-1 document.

### P2 — replace tests that can pass without testing the claim

**Document:** “group axioms on random values”; “both sides overlap”; “against fmpz_mat”; “recovers random rationals ... refuses ... too coarse”; “C against the reference”.

**Defects and replacements [proved here].** Use independent properties with precision targets:

| Existing test | Why insufficient | Required replacement/addition |
|---|---|---|
| Prototype sampled gcd | Samples verify witnesses but not universal enclosure | M1 algebraic proof; exhaustive small local quotients plus random rational denominators and zero radii |
| C against identical Python formulas | The same incorrect formula can agree | Four-witness minimality, direct finite enumeration, local valuation formula `min(v(a)+v(M),v(b)+v(N),v(N)+v(M))`, including infinity for zero |
| Only SPEC examples | Misses huge common denominators and edge cases | Signed centres, non-coprime denominators, exact zero/nonzero, N=1, fractional/incomparable radii, normalization invariance, aliasing |
| Group axioms on idele balls | `B*B^-1` with independent choices gives a neighbourhood of 1, not necessarily {1}; real rounding breaks set-level exact associativity | Exact finite coset identities at fixed modulus; mixed-modulus gcd; pointwise containment of 1; real accuracy and monotonicity tests |
| Product formula | Mostly rechecks the stored r against itself | Negative rationals, separate local valuations, finite support, sign in class reduction, and exact norm identity before real rounding |
| psi(q)=1 | Constant-1 implementation passes | Nontrivial phase at `(0;1/3)`; mixed real/finite rational cancellation; finite fractional-radius image; additivity and width target |
| Double Fourier/reflection, Gaussian Poisson | Both signs and wrong paired normalisations may pass special symmetric cases | Delta cosets at D != M, exact weighted formula, non-even translates, explicit support/period, Haar integrals, weighted Plancherel |
| Poisson overlap | Whole-plane balls pass; truncation might be uncertified | Independent truncations with proved tail bounds, width target, precision convergence; do not compare a value with a result constructed from it |
| Tate equals FLINT | Tautological if using the same evaluator; ramified test may be zero | Independent certified theta/integral path, both parities, non-real primitive character, conductor scaling, poles and continuation domain |
| Solver against same fmpz_mat routine | Repeating the implementation is not a certificate | Verify `Ax=b mod N`, kernel generators and completeness on enumerated small composites; verify transformation matrices |
| Roots against mod-p factorisation | Says nothing about liftability or singular completeness | `x^2+1` at 2; `x^2` repeated roots; simple Hensel uniqueness; real nonexistence; exhaustive p^k branches |
| Reconstruction “too coarse” rejection | Sufficient uniqueness bound is not necessary; full-adeles differ from modular data | Full-ball progression intersection, modular uniqueness boundary `2AB=m`, ambiguous/no candidate cases, denominator conflict at a prime outside m |
| 10^6 round trips | Can reproduce a consistently wrong interpretation; random data may omit rare tags | Golden grammar vectors, exact 1/3 tag, dyadic radius dump, storage-independent canonical form, malformed/huge input limits, coverage-guided fuzzing |

For policies, use the same expression DAG and exact input enclosures; after each operation compare the tight enclosure with the capped enclosure by the exact containment criterion. Test repeated operations, exact constants, scale changes and mixed contexts. Directly test the intentional precision-loss cases and that no conversion invents precision. Correlation-sensitive identities (`x-x`, `x/x`) need an explicit dependency policy; ordinary independent-ball evaluation need only enclose the true result. Local/global round trips should preserve the represented set, including denominators, and any cached form should be invalidated consistently. Property tests must include signed/zero/large tagged FLINT states and invalid idele constructors.

### P3 — sizes and completion criteria

**Document:** milestone 5 “M”, solving “M”; “a failing test precedes every change”.

**Assessment [from memory].** A few days for each of independent certified global integration, conductor/parity/root-number handling, continuation, module solution certificates and singular-root completeness is optimistic unless most of it is delegated to existing routines, in which case the acceptance tests must say what new code they validate. I cannot give a defensible calendar estimate without an implementer and an agreed narrower scope.

**Replacement [proved here].** Estimate separately the proof/reference work, implementation, adversarial testing and performance measurement. Mark analytic continuation and singular lifting as research/validation gates with explicit exits before assigning dates. Require a failing regression for a defect fix and a defining test for a new behaviour; do not require artificial failing tests for citation corrections or purely explanatory changes. Keep mutation testing for arithmetic branches and parser fuzzing, but measure coverage and surviving mutations instead of treating one million random objects as a quality theorem.

### F1 — correct the hardware model and its limits

**Document:** “mul ... latency 3 cycles, one per 2 cycles”; “Cycle counters ... not available”; “2 loads and 1 store per cycle”.

**Checks [checked by a run].** `checks/environment.txt` identifies the current WSL2 guest as a 3970X with AVX2, FMA, BMI2 and ADX, and no AVX-512 flag; its reported L1d/L2 totals divided by 32 give 32 KiB/512 KiB per core. This is guest-visible information, not a physical-machine audit. The saved compiler version is GCC 13.3.0. AMD lists 3.7 GHz base and up to 4.5 GHz boost for the 3970X; this is not a measured interval containing the clock during each benchmark. See [AMD's processor announcement](https://ir.amd.com/news-events/press-releases/detail/913/amd-introduces-worlds-fastest-high-end-desktop-processors-with-3rd-gen-ryzen-threadripper-family-delivering-unmatched-performance-with-no-compromises).

The inspected Zen 2 instruction-form data give:

| Instruction | Dependency latency | Reciprocal throughput | Evidence |
|---|---|---|---|
| `mul r64` | 3 cycles to low RAX, 4 to high RDX | 1 cycle/instruction | [uops.info MUL, Zen 2](https://www.uops.info/html-instr/MUL_R64.html#ZEN2) |
| `add r64,r64` | 1 cycle | 0.25 cycles/instruction | [uops.info ADD, Zen 2](https://uops.info/html-instr/ADD_03_R64_R64.html#ZEN2) |
| `adc r64,r64` | 1 cycle, including carry-to-carry | measured about 0.33–0.35 cycles/instruction; documentation model 0.25 | [uops.info ADC, Zen 2](https://uops.info/html-instr/ADC_13_R64_R64.html#ZEN2) |
| `vpmuludq ymm,ymm,ymm` | 3 cycles | 1 cycle/instruction | [uops.info VPMULUDQ, Zen 2](https://uops.info/html-instr/VPMULUDQ_YMM_YMM_YMM.html#ZEN2) |

The two-cycle multiply throughput in PERF matches the Zen+ measurement on that page, not Zen 2. Do not treat measured reciprocal throughput as a proved execution-port ceiling. The ADD data independently confirm one-cycle latency and measured/documented 0.25-cycle reciprocal throughput for the register form. Throughput is not the latency of a dependent carry chain. Specify register forms; memory operands, implicit flags, low/high halves and MULX are different dependency graphs.

`checks/flint_probe.txt` successfully executes RDTSCP. Thus “cycle counters unavailable” is too broad: distinguish unavailable PMU core-cycle events from an accessible invariant/virtualised timestamp counter. TSC ticks are not automatically actual core cycles. No frequency calibration is claimed here. Nominal GHz conversions should be labelled conditional estimates, not verified upper/lower confidence bounds. **[proved here]** A clock outside the assumed interval invalidates that interval of ratios even if the arithmetic conversion is right.

**AVX2 correction [checked by a run, source inspection].** VPMULUDQ multiplies the four even-numbered unsigned 32-bit lanes and returns four 64-bit products. Eight packed 32-bit inputs require two multiplies plus rearrangement. AVX2 has no single packed full 64x64-to-128 multiply, but multi-instruction partial-product implementations are possible. [Intel's instruction-set manual, PMULUDQ/VPMULUDQ](https://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-vol-2b-manual.pdf) fixes the semantics; the uops source above fixes the selected throughput model. “Must use moduli below 2^32” is too strong. It is a reasonable first backend restriction. Floating-point/FMA reduction near 2^50 requires an explicit input range, rounding-mode and error proof; the modulus threshold alone proves nothing.

**Memory-model correction [proved here under stated assumptions].** Keep a conditional peak model of two 32-byte loads and one 32-byte store per cycle, aligned L1-resident arrays, adequate independent instructions, no cache-line splits. This gives read capacity 64 bytes/cycle and write capacity 32 bytes/cycle. PERF's counts instead assume 8-byte accesses. Both are useful models if named; they are not the same lower bound. The exact Zen 2 load/store port details have not been independently microbenchmarked here and remain **[from memory]** hardware assumptions. DRAM bandwidth, cache misses and write allocation need separate measured/calibrated models; a cacheline size is not bandwidth.

### F2 — count states for the same object that is measured

**Document:** “Counting states (PROVED)”; “Real ball ... about 168 bits”; “same with a denominator d ... add log2 d”; “528 bytes ... x1.08”.

**Derivations [proved here].** For a *fixed known integer* N there are exactly N integral-centre cosets, so fixed-length storage needs `ceil(log2 N)` bits. For radii `1<=N<=B` and integral reduced centres there are `sum N=B(B+1)/2` states. The information floor is `ceil(log2(B(B+1)/2))`, about `2 log2 B-1`, not literally `2 log2 N` for each object. With B=2^62-1, 123 bits require 16 bytes when rounded to whole bytes. Do not infer that an 8-byte machine word is an information floor; it is an alignment/kernel design choice. There is no uniform finite information floor for unbounded exact rationals.

If d is fixed and shared, centres on the grid `(1/d)Z` modulo integer N give Nd states, so adding log2 d is correct *for that model*. If d is an additional variable reduced denominator bounded by D, the count is `N sum_{d=1}^D phi(d)`; one must encode its identity too. If the radius is fractional, bound and count its numerator/denominator as well, or use an explicit `(A,H,d)` parameter family. Without those restrictions the fractional-radius row is not a derived floor.

A general arb at working precision 128 still has an unbounded exponent and a separately represented nonnegative radius. The 32-bit exponent/8-bit radius model changes that state space. If it permits exactly `2^127` normalised mantissas, two signs, `2^32` exponents and `2^8` radius codes, all independent with no special states, it has `2^168` encodings. That is a restricted **MODEL** information count, not a PROVED bound for arbitrary arb balls, and special zeros/infinities need additional conventions. Eight radius bits cannot encode all arb radii or prove a useful accuracy contract. Comparing 48-byte arb storage against 21 bytes can describe a deliberate feature/space tradeoff; it is not pure overhead.

The 29-byte small-adele model uses *fixed integral scale and a shared word modulus*. A per-value scaled residue has two potentially unbounded scale integers plus a residue; the generic tight object has different states again. Compare the 56-byte layout only with the matching restricted ball model; compare own-radius layouts with the own-radius count; charge shared context/CRT tables as `context_bytes / batch_size`. An object of size <=64 can straddle cache lines unless appropriately aligned; “fits in one cache line” is not implied by sizeof alone.

**Numerical checks [checked by a run].** `checks/math_checks.txt` gives `log2(20!)=61.07738392090622` and 65 bits for separately packed prime-power fields. Those rows are right: a word is x64/61.077 = 1.048 of the information count; 8x32 bits is x4.191; 8x64 bits is x8.383. The 32-byte hand-packed adele is x32/29 = 1.103 only under the restricted model. The tabulated x1.93, x2.76 and x2.21 divisions by 29 are arithmetically right, but the compared features differ. The own-radius 37-byte comparison is likewise a restricted layout model, not an arb information theorem.

`528/512=1.03125`, not 1.08. The local allocation probe reports 520 usable bytes for a 512-byte request, consistent with a 528-byte chunk including allocator overhead, but usable size alone does not measure total allocator metadata or pooled GMP headers. Count the fmpz handle, mpz struct/pool, actual allocated limb capacity and allocator effects separately. The stated `max(32,round16(n+8))` is a small-allocation glibc chunk model under 64-bit alignment assumptions; it is not a universal formula for mmap allocations, debugging allocators or resident memory. The ratio tends to one only for one large payload with bounded additive overhead, not for a whole application retaining multiple temporaries and contexts. **[proved here]** These qualifications follow from what is and is not included in the numerator.

### F3 — audit every time floor

**Document:** PERF 3's nine rows and “Best-known reference lines”.

**Rule [proved here].** A lower bound is for a specified computational problem in a specified model, not for an algorithm one hopes the optimum uses. A mandatory instruction on an identified dependency path can bound that implementation's latency; an instruction count times reciprocal throughput bounds its resource demand; a worst-case input-reading argument is not automatically a lower bound for a favourable fixed input. An enclosure API without a precision requirement can return a constant enormous enclosure, so precision must be part of the timed problem.

| PERF row | Verdict and corrected kind/model | Matching measurement and possible stronger bound |
|---|---|---|
| 62-bit `nmod_add`, 1-cycle add | Valid weak **MODEL latency** floor for the selected register-chain implementation requiring an integer add. Not a theorem about all modular addition implementations. | The benchmark is a dependency chain, so the kind matches. Conditional 3.7–4.5 GHz gives x2.74–x3.33. An instruction-DAG bound including its conditional correction is stronger, but needs the compiled code and flag/CMOV latencies. |
| 62-bit `nmod_mul`, 3-cycle mul | A weak **MODEL latency** floor for a kernel required to use that multiply on its path; full MUL's high half has 4-cycle latency. It is not a universal multiplication theorem. | The chain kind matches. A required high-half multiply gives 4 cycles and conditional x4.62–x5.61. The emitted kernel has a much longer provable path described below. |
| Word gcd, “PROVED ... 1 cycle” | Invalid kind/unit. Reading O(1) words gives a **PROVED word-probe** lower bound under an explicit input-access model, not one clock cycle. Inputs here are registers. | No justified numeric cycle floor is supplied for this mixed gcd/input-generation chain. Withdraw x340–x410 as an optimality claim; a trivial model rung conveys no useful headroom. |
| 4096-bit add, 64-cycle carry chain | Valid **MODEL latency** only if restricted to a scalar ripple-carry algorithm. Carry-lookahead and parallel chunks defeat it as a general lower bound. | Repeated fixed-operand out-of-line calls are not a chained-add latency test. In a compulsory dense-I/O rate model the 1024-byte input and 520-byte full sum require at least max(16,17)=17 cycles with vector accesses. Scalar-only accesses instead require at least 65 stores for the full 4097-bit sum. |
| 4096-bit mul, 128 stores | Valid **MODEL throughput** only for scalar stores to a fully materialised 128-limb result. AVX2 stores reduce the store-count floor. | Under the vector I/O model, max(1024/64,1024/32)=32 cycles. Under scalar-only stores, 128 cycles is valid and yields the quoted conditional x45–x55. Neither says the arithmetic can approach the I/O bound. |
| 8192/4096 reduction, 192 reads/2 | Valid **MODEL throughput** only with compulsory scalar reads of numerator and modulus. | With vector accesses, 1536 input bytes/64=24 cycles, output <=512/32=16 cycles. A reused modulus/precomputation changes the compulsory bytes. Fixed operands in the harness do not measure a fresh-input latency. |
| 4096-bit gcd, 128 reads/2 | Same restriction: scalar compulsory-input model, not a gcd computational lower bound. Output may be one word. | Vector-input rung is 1024/64=16 cycles, under compulsory full reads. Gcd can return early on special input families. No stronger general numerical floor is established here; a chosen Euclidean/half-gcd algorithm's work is not a problem floor. |
| 128-bit arb add, 2-cycle carry chain | Unsupported as a floor for the actual ball problem. It assumes an algorithm and excludes exponent alignment, output accuracy, radius and special cases. | The harness is a repeated fixed-input call-rate test, not a carry dependency chain. Replace the numeric ratio by “floor not yet specified”; first define required accuracy, exponent/radius ranges, exact/special inputs, output materialisation and benchmark mode. |
| 128-bit arb mul, 3 multiplies x 2 cycles | Invalid for the stated problem, and based on wrong Zen 2 throughput. Three limb products describe one full-precision two-limb algorithm, not a lower bound for an arbitrary rounded ball product. | Correcting throughput gives three cycles of multiply-resource demand *if that algorithm is required*; latency includes the dependency graph. Neither justifies the quoted x13–x16 for this benchmark. No replacement numeric optimality ratio until the accuracy/model contract is fixed. |

All formulas and conclusions in this table are **[proved here]** conditional on the named models; the supplied timing numerators are **[checked by a run]** from inspection of the supplied baseline file, not a rerun of that performance experiment. To avoid laundering questionable assumptions, the vector I/O figures above are replacement *floors for new matched benchmarks*, not newly certified ratios for the old harness. Even a valid weaker floor can yield a large ratio without indicating a reachable improvement. A scalar-restricted ratio should not be compared with a vector-permitted one.

**Tighter bound for the actual small modular kernel [checked by a run + proved here].** Compiling the unchanged baseline with the installed GCC `-O2` produced `checks/baseline_O2.s`. Its mul loop has a `mulq`, then a second `mulq` using the first high half, then low-half add/high-half adc, increment, a dependent `imul`, and subtraction before two corrections. For this calculation additionally assume one-cycle integer add/sub and three-cycle immediate IMUL latency; the exact immediate IMUL form was not independently verified and is a **[from memory]** model parameter. Under the latency model above, the partial path alone costs at least

```
4 + max(4,3+1) + 1 + 1 + 3 + 1 = 14 cycles.
```

The loop's remaining corrections may increase this. This is a code-specific **MODEL latency** lower bound; it would put the supplied 4.99 ns at conditional x1.32–x1.60, not x6–x7.5. It is not a lower bound on alternative modular algorithms, and it illustrates why a one-multiply floor is poor evidence of library overhead. A frozen compiler/FLINT version and dependency-path specification belong in that row.

**Algorithm references.** **[checked by a run]** Harvey and van der Hoeven's [*Integer multiplication in time O(n log n)*](https://annals.math.princeton.edu/2021/193-2/p04) exists (2021, pp. 563–617). It is an asymptotic upper bound, not a latency prediction at 4096 bits. **[from memory]** Fast division via reciprocal/Newton and divide-and-conquer gcd give standard reference costs `O(M(n))` and `O(M(n) log n)` respectively under customary regularity assumptions on multiplication cost M(n). These should receive primary-source citations before being called verified best-known claims; this review does not invent one. “Quasi-linear” is a family of growth rates, not an exact n log n gcd assertion.

“Montgomery or Barrett ... about three multiplies, about 9 cycles ... best-known” is too vague. **[proved here]** An operation count for one reduction scheme is an algorithm reference; it is neither proved optimal nor a universal latency. It depends on odd modulus for ordinary Montgomery, representation conversion, input ranges, available high-half instructions, constant modulus specialisation and reduction corrections. Call it an example algorithm cost and show the actual graph. The inspected kernel's chained multiplications already disprove interpreting nine cycles as its complete cost.

### F4 — name what the baseline actually measures

**Document:** “minimum of 15 trials”; row labels “dependent chain”, “fresh inputs”, sizes 4096 and 8192.

**Inspection [checked by a run].** The two nmod loops are output-to-input chains. Word gcd includes `acc += gcd(u,w)` and arithmetic generating the next inputs, coupled through acc; it is not isolated gcd latency or independent gcd throughput. fmpz and arb calls repeatedly read the same A,B or X,Y and overwrite the same destination, with no mathematical output-to-input dependency. Their result is elapsed steady-state call cost for those operands, including dispatch and potentially overlapping hardware work. Large gcd uses 9 trials, not 15. The parsed seed is explicitly unused. `flint_randinit` is called without applying it. The loop's memory compiler barrier is not a hardware fence and does not turn independent arithmetic into a latency chain.

The C probe reproduces the random setup: actual A,B,M lengths are 4096 bits, while A*B has 8191 bits. That distinction does not change its 128-limb storage, but the size should be measured, not assumed. The initial b is in range for this seed; generally `n_randint(st,p)|1` can yield p if the drawn value is p-1, violating reduced-residue assumptions. Fix odd-constant selection before parameterising seeds. Repeated operations warm/resize their outputs, so the minima largely omit first-allocation cost; heap-growth costs are a separate benchmark. The source does not itself establish CPU affinity; the command in the comment must be logged with each run. No compiled disassembly, loaded FLINT build flags, governor/host-load state or per-run clock calibration was saved with the original results.

**Correction [proved here].** Give each benchmark a workload contract: latency chain, independent batch throughput, or API call rate. For chain tests bound values to prevent bit-length/precision drift. For batches use distinct outputs, consume checksums outside timing, initialise several reproducible operand families, record actual sizes, warmup/allocation policy, affinity, compiler/library build and clock source. Preserve min/median/max but do not equate a minimum with absence of bias. Use separate hot-L1 and streaming cases, and explicit integral/rational/scaled balls before drawing adelfeld design conclusions. Match each numerator to a floor of the same mode. Keep the original file as a historical primitive reference, with its limitations stated.

### F5 — rewrite the design conclusions as hypotheses to test

**Document:** PERF 4: “gcd ... dominated by bookkeeping”; “gain an order of magnitude”; “Shared radius halves the space and removes the radius arithmetic”.

**Assessment [proved here].** The quoted gcd/multiply ratios (about 18 word-size and 10 large-size) are arithmetic ratios of different primitive workloads, not lower-bound-normalised evidence about a completed ball multiply. The three-argument radius gcd may take multiple gcds and products at increased bit sizes; rational normalisation may add work. Conversely structured cases can be cheap: at fixed integral N, `gcd(aN,bN,N^2)=N gcd(a,b,N)`, and known unit residues make it N immediately. Exact scalar multiplication is just scale/radius arithmetic. Local valuations and supplied factorizations change the cost model. Measure the full kernel on typical as well as adversarial radius patterns before claiming dominance.

The proposed scaled policy saves some per-product gcds but still multiplies/reduces rational scales and aligns them for addition (D1). “One modular multiplication” describes its residue component only. Shared radius halves a two-integer finite payload only when scale and modulus really are shared; it does not halve the whole adele or a two-fmpq fractional ball. Cache/layout context overhead must be amortised.

Large integer addition being within x2 of a scalar ripple floor does not prove closeness to the unrestricted optimum; the arb ratios are not sound floors (F3). Therefore a new fixed-precision ball kernel cannot be justified as an expected tenfold improvement by these tables. Keep arb initially; only open that project after a profile of complete operations, a range/accuracy contract for the specialised type, and a matched benchmark against a derived floor. SIMD below 2^32 is a candidate first kernel, not an architectural necessity. This is the constructive replacement for all four conclusions in PERF 4.

### F6 — usable replacement rows for the missing operations

**Document:** “Rows still to be derived” lists CRT, DFT, reconstruction, text, vector kernels and idele-class product.

**Common model [proved here].** Let `b=ceil(n/8)` for an n-bit modulus, k the number of pairwise coprime moduli, `L=DM`, and W the materialised bytes per complex coefficient at the required accuracy. Precomputation is excluded only when a separate setup cost and reuse count are reported. Distinct dense arrays begin in L1, all declared input bytes must be read, all output bytes must be written, and no result is cached from a previous call. With the conditional peak model in F1, an I/O throughput lower bound is

```
T_IO >= max(B_read/64, B_write/32) cycles per operation.
```

Integer instruction counts can be rounded up when each operation must issue complete accesses independently. Reading and writing overlap, so add neither rates nor their reciprocals. For streaming buffers also require `T >= B_traffic / beta`, where beta is a stated peak/assumed sustainable bandwidth in bytes/cycle, including compulsory write allocation if applicable. Hardware models and bit-probe theorems have different units.

| Operation and assumptions | Valid floor and kind | Algorithm reference, explicitly not a floor |
|---|---|---|
| Global integer to k residues, `q_i<2^32`, product has n bits; moduli/tree reused | **PROVED:** Ω(n) bit access in worst case for an explicit conversion, also Ω(k) residue outputs. **MODEL:** `max(b/64,4k/32)` cycles for packed output payload; charge tree/constants separately. | Remainder tree: `O(M(n) log k)` bit operations with usual regularity assumptions; direct k divisions may be better for small k. |
| k packed residues to one canonical integer modulo their product | **PROVED:** Ω(n) bits to distinguish/reconstruct all residue states. **MODEL:** `max(4k/64,b/32)` cycles for payload. | Precomputed product/CRT tree: `O(M(n) log k)` bit operations, plus setup/storage; denominator scaling/normalisation is a separate cost. |
| Dense finite Fourier transform, L coefficients, specified absolute accuracy, materialised W-byte values | **PROVED:** Ω(L) coefficient accesses/outputs in the explicit dense model. **MODEL:** at least `max(LW/64,LW/32)=LW/32` cycles for one compulsory input/output pass; transforms may need many passes. | Direct DFT O(L²) coefficient operations; FFT/Bluestein O(L log L) operations with an adequate convolution length and certified roots/error budget. Haar 1/M scaling must be included. |
| Bounded modular rational reconstruction, n-bit c,m, nontrivial bounds, full decision/certificate required | **PROVED:** Ω(n) bit access in worst case, not a quadratic gcd floor. **MODEL:** `max((2b+B_bounds)/64,B_out/32)` for the explicitly compulsory-input interface. | Classical continued fractions/extended Euclid; fast half-gcd `O(M(n) log n)` bit reference. Output verification is additional. This is not the full-adele routine. |
| Rational candidate in a full ball `(I;a mod N)` | **PROVED:** read the exact endpoint/scalar encoding and write the selected candidate or decision in the explicit-input model. **MODEL:** use its actual read/write byte counts above. | Compute integer lower/upper bounds on `(I-a)/N`, with exact endpoint inclusion; no general lattice reduction is required (M12). Cost is dominated by rational arithmetic/division at the input sizes. |
| Packed AVX2 modular product, k residues each <q_i<2^32, selected algorithm uses native full 32x32 products | **MODEL throughput:** at least `ceil(k/4)` VPMULUDQ instructions, at one/cycle on the F1 model; payload I/O floor k/8 cycles, so the product-only resource rung is at least k/4. For a dependent four-residue vector chain, at least 3 cycles for its initial multiply, plus reduction's actual dependency path. | Add the concrete reduction, shuffles, compare/correction and constant loads before quoting a schedule. One native multiply is not an entire modular multiply. |
| Vector modular add on the same packed data | **MODEL:** payload I/O floor k/8 cycles; also bound the chosen vector-add/correction resource counts once specified. | Packed 32-bit sums need overflow handling; a four-lane 64-bit representation avoids overflow for q<2^32 but doubles payload bytes. |
| Exact text dump or parse of B bytes, explicit buffer | **PROVED:** Ω(B) emitted/read bytes for worst-case full output/validation. **MODEL:** B divided by output/input bandwidth (and write traffic); SIMD scanning is permitted. | Decimal conversion has its own big-integer cost; a fixed digit-count floor is not a division-per-digit theorem. Measure parsing and formatting separately from system I/O. |
| Idele class product at shared N | **PROVED:** at least the input/output work for the requested residue and positive-real accuracy. **MODEL:** combine the residue kernel and real-ball DAG/resource counts; no numeric universal arb floor yet. | One unit-residue modular product plus real-ball product at shared N; mixed N additionally needs modulus reconciliation. No rational r scale exists in the reduced class, unlike general ideles. |

The bit-access and I/O bounds in the table are **[proved here]** under the explicit models. The algorithm orders for product/remainder trees follow by summing per-level multiplication/reduction costs across at most log k levels, **[proved here]** assuming an O(M(n)) reduction primitive and superlinear regular multiplication cost. The fast division/half-gcd primitive bounds are **[from memory]**, as disclosed in F3. FFT's operation-count reference is **[proved here]** for radix-two transforms by the recurrence `T(L)=2T(L/2)+O(L)`; arbitrary L uses the identity expressing `jk` as a difference of squares to reduce DFT to convolution padded to a power of two at least 2L-1. Numerical bit cost is not determined by L alone: working precision, input magnitude, root-generation, absolute/relative tolerance and cancellation must be charged. None of these references proves optimality.

**A stronger conditional Fourier lower bound [proved here].** For the *unnormalised exact* L-point DFT in a linear arithmetic circuit with operations `z=alpha*x+beta*y`, `|alpha|,|beta|<=B` for a fixed B>=1, let Delta be the largest absolute determinant formed from any L currently available linear forms. Initially Delta=1. Adding one form multiplies its possible maximum by at most 2B by determinant multilinearity. The DFT matrix F has `F* F=L I`, so `|det F|=L^(L/2)`. Thus at least `L log L/(2 log(2B))` steps are required. This is a **PROVED bounded-coefficient-model** bound, not a hardware floor or an unrestricted bit-complexity theorem. It does NOT automatically apply to the actual Haar-normalised `(1/M)F`, whose determinant magnitude is `(D/M)^(L/2)` and may be <=1. Use the weak explicit-output floor for that row unless an appropriate stronger theorem is proved. There is no justified blanket PROVED “L log L FFT floor” for this project.

**AVX2 details [proved here from instruction semantics in F1].** A YMM has eight packed 32-bit lanes but only four full 32x32 products per VPMULUDQ. Two shifted multiplies cover eight inputs. With four 64-bit residue slots, payload reads are 16k bytes and writes 8k, so the I/O rung becomes k/4 cycles, matching the product-only rung. If every residue has its own modulus and reciprocal loaded from memory, include those bytes. Signed 32-bit comparisons are insufficient near 2^32; zero-extended 64-bit lanes have sums below 2^33 and safe signed 64-bit comparisons. General Barrett reduction of a 64-bit product can require a high 64-bit product with a reciprocal, which AVX2 must synthesise; small-modulus restrictions or floating-point/FMA alternatives need their own proofs. Power-of-two moduli, constant operands and special moduli may avoid the assumed generic multiply/reduction path entirely. Hence the k/4 bound is explicitly for the selected kernel family, not every possible modulus family.

**Example [proved here, not a timing measurement].** For n=4096 and k=128 packed residues, both CRT directions have a 16-cycle payload I/O floor. This says only that a conversion cannot beat its required traffic in this model. It does not promise a conversion remotely close to 16 cycles. Report setup and amortised conversion ratios separately; no measurements of these new kernels exist yet.

### A1 — Hertogh is a real, relevant source; inspect the limits of the comparison

**Document:** “The one implementation known to us is M. Hertogh's Sage package ... (2021) ... precision rules”.

**Checked by a run (primary-source inspection).** The author's [repository](https://github.com/mathehertogh/adeles) identifies Mathé Hertogh's *Computing with adèles and idèles*, Leiden master's thesis, 2021, and lists adeles/ideles over number fields, multiplicative p-adics, ray class groups and additional applications. The repository includes a thesis PDF and links to its [Leiden record](https://hdl.handle.net/1887/3249353). The record/PDF retrieval did not succeed through the available raw endpoints, so I do not claim a complete thesis reading. The package's authored [profinite-number documentation](https://mathehertogh.github.io/adeles/profinite_number.html) specifies smallest represented subsets for addition and multiplication, uses profinite numerator plus integer denominator, and gives arithmetic examples consistent with SPEC 4.2. Its [idele documentation](https://mathehertogh.github.io/adeles/idele.html) and [multiplicative local documentation](https://mathehertogh.github.io/adeles/multiplicative_padic.html) confirm distinct multiplicative precision data. This supports the attribution but is not a code audit or a proof of all its general-field guarantees.

**Correction [proved here].** Say “a directly relevant implementation is ...”, describe the inspected modules, pin a commit and finish fetching the thesis before claiming detailed parity. The phrase “one implementation known to us” is appropriately limited if it reports the authors' knowledge; do not promote it to “the only implementation”. Avoid copying its [overlap-based equality convention](https://mathehertogh.github.io/adeles/profinite_integer.html), which its own examples explicitly show is nontransitive. M3 gives the reason. No exhaustive prior-art survey was done here, and no additional implementation is asserted from memory. The named parent-project survey was not supplied among the requested inputs and is not evidence until inspected.

### A2 — certification and available number-field software

**Document:** “Number fields bring ideals, class groups and units, which FLINT does not have”; “Class-group computations are certified only under the generalised Riemann hypothesis”.

**Checked by a run.** Installed FLINT 3.0.1 includes `nf.h`, `nf_elem.h`, `qadic.h` and `qfb.h`; it does have number-field element arithmetic, unramified local extension arithmetic and specialised quadratic-form infrastructure. The inspected headers do not supply a general maximal-order/fractional-ideal/class-group/unit backend sufficient for the proposed arbitrary-number-field adeles. Phrase the missing capability at that level; do not say FLINT has no number-field support or imply qadic covers all ramified extensions.

PARI's primary [bnfcertify documentation](https://pari.math.u-bordeaux.fr/dochtml/ref-stable/General_number_fields.html#bnfcertify) explicitly describes removing the GRH assumption from a `bnfinit` result when full certification succeeds. Its `bnfinit` documentation separately states the default conditional guarantee. Therefore the blanket “only under GRH” statement is wrong. A successful reduced/flagged certification has a weaker guarantee and is not the same as certifying the full class group and units.

**Correction [proved here from that checked counterexample].** State: some fast class-group/unit computations are conditional unless followed by successful unconditional certification; record which guarantee was obtained. Unconditional algorithms/certification exist, with potentially substantial extra cost. This is a requirement on future backend provenance, not a reason to declare all class groups inherently uncertifiable. Do not promise a future dependency or its guarantees without choosing and testing it.

## 3. What is right and should be kept

**[proved here]** Keep the finite ball `a+N Zhat` as Q's basic compact-open enclosure, rational radii and radius zero for rational finite singletons, the exact gcd precision rules, and factorisation-free global ring arithmetic. Keep the separate idele topology and exact valuation scale: Q's finite-idele and idele-class decompositions are unusually simple and useful. Keep self-dual Fourier/Poisson identities as tests once their weights, phases and width targets are fixed.

**[proved here]** Keep enclosure as correctness and tightness as quality, explicit contexts, FLINT init/clear conventions, a slow reference and adversarial tests, lossless serialization, an independently specified local backend and the distinction between PROVED, MODEL and algorithm upper bounds. The current drafts expose assumptions clearly enough to repair them before they harden into an ABI. **[checked by a run]** The existing prototype's examples and all 6000 random precision checks passed; the design does not need new arithmetic formulas, it needs the surrounding contracts made precise.

## 4. Proposed replacement texts

The following are literal replacement paragraphs/rows, grouped by finding. **[proved here]** marks deductions and contracts justified above; **[checked by a run]** marks implementation/source facts; **[from memory]** marks an external theorem still needing its own proof/source. These labels apply to the immediately following quoted replacement as well. Where several old statements conflict, the specified paragraph replaces all of them rather than leaving a second incompatible promise elsewhere.

### M2 — SPEC 2 representation claim; SPEC 3 partial-place row; PLAN 4 modulus families

**[proved here]**

> A basic finite-adelic enclosure over Q is a compact open coset `a+N Zhat`, with rational a and positive rational N. It specifies one local ball at each prime and equals Z_p outside a finite exceptional set. Every basic product neighbourhood has this form by clearing denominators and applying CRT. Arbitrary finite unions or correlated information need not be one ball. Radius zero is an additional exact-rational singleton case. The representation uses rational/integer arithmetic without factorisation; it is not a claim that an operation costs the same as one big-integer operation. A radius `p^k` still constrains every other prime; a single Q_p ball and partial adeles use explicit place-set types with their own projection semantics.

### M3 — SPEC 4.1 equality column and exact embedding; PLAN 1.5

**[proved here]**

> Exact global rationals are a tagged value storing one canonical fmpq. Their diagonal embedding remains exact until explicitly converted to a product ball at requested real precision. A ball-valued adele has an arb enclosure and a finite ball, and need not preserve the correlation of a diagonal rational. Provide separate `equal_set`, `overlaps`, and `contains` predicates; overlap is not equality. Equality of represented balls is decidable, while equality of unspecified points within overlapping balls is generally unresolved. For finite positive radii, set equality requires equal radii and integral `(a-b)/N`; overlap requires integral `(a-b)/gcd(N,M)`; containment requires integral `N/M` and `(a-b)/M`. Handle singleton cases separately. The embedding test checks exact rational-tag arithmetic and certified containment after conversion, including 1/3 and negative rationals.

### M5 — SPEC 5 precision and character paragraphs

**[proved here]**

> A finite idele is uniquely `r u` with positive rational r and `u in Zhat^x`; the scale r is exact and its factor is discrete. Finite unit precision is a coset of `U(N)=ker(Zhat^x -> (Z/NZ)^x)`, for integer N>=1. At a common N, product and inversion preserve this finite precision; at moduli N,M the product uses gcd(N,M), and real-ball error still propagates. An idele's real ball must exclude zero. Its class coordinates are `t=|x_inf|/r` and `u'=sign(x_inf)u`. Continuous quasi-characters are `t^s chi(u')`; unitary characters require Re(s)=0. Character evaluation requires the unit uncertainty subgroup to lie in its kernel, or returns an image enclosure/precision status; carry the conductor explicitly. The idele-to-adele operation returns an additive enclosure of the represented unit coset, not an exact set conversion, and states whether it uses the simple radius rN or the tighter odd-unit hull of radius `r lcm(N,2)`.

### M6 — SPEC 6 reduction paragraph; PLAN 3.1

**[proved here]**

> Reduction modulo Q returns an enclosure in the additive quotient. Exact points have a unique representative in `[0,1) x Zhat`, with boundary identification `(1,z)~(0,z-1)`. A ball crossing an integer after the rational shift may require several correlated chart pieces. For `N=A/B>0` in lowest terms, split the finite ball into the B cosets `a+kN+A Zhat`, 0<=k<B, before chart reduction, or retain an unsplit quotient lift. Radius zero is supported. A single-chart API returns `NEEDS_SPLIT` when necessary, and a materialising API accepts an output-size limit. Test quotient-set equivalence under rational translations, boundary crossings, fractional radii and enclosure widths.

### M7 — SPEC 6 character paragraph; PLAN 3.2

**[proved here; source choice checked by a run]**

> Use Tate's §2.2 convention: `psi(x)=exp(2 pi i(-x_inf+sum_p {x_p}_p))`, and define Fourier transform with `conjugate(psi(xy))`. Thus psi is trivial on diagonal Q, the real Fourier kernel has positive sign and the finite kernel negative sign. For integer positive N, the finite phase of `a+N Zhat` is exp(2 pi i a); for `N=A/B` in lowest terms its image is that phase times the B-th roots of unity. Character evaluation encloses the full image, or reports that a single phase is not determined. Exact finite singletons are allowed. An acb rectangle encloses unit-modulus values but need not consist of them. Tests include a nontrivial phase and fractional-radius ambiguity, not only psi(q)=1. Source: Tate, *Fourier Analysis in Number Fields and Hecke's Zeta-Functions*, §2.2.

### M8 — SPEC 7 finite-function and Haar paragraphs

**[proved here]**

> A finite test function uses positive integers D,M, is zero outside `(1/D)Zhat`, and is constant on cosets of `M Zhat`. Store `f_j=f(j/D)`, 0<=j<L=DM. Normalise additive Haar measure by vol(Z_p)=1 and Lebesgue measure at infinity. With the selected Fourier convention, `hat f(k/M)=(1/M)sum_{j=0}^{L-1} f_j exp(-2 pi i jk/L)`. The transform is supported in `(1/M)Zhat` and constant modulo `D Zhat`; its second transform uses the factor 1/D and is reflection. Its integral is `(1/M)sum f_j`, and its squared L2 norm is `(1/M)sum |f_j|^2`. The output squared norm uses 1/D. The global measure has vol(A/Q)=1, giving Poisson summation with no further scalar. These weights are part of the API and acceptance tests.

### M9 — SPEC 7 real-function paragraph and operations; PLAN 4.2

**[proved here]**

> Version one implements a specified subclass of Schwartz–Bruhat functions, not their whole space. The real part is a finite sum of `P(x) exp(-pi A x^2+B x+C)` with Re(A)>0, with symbolic transformation identities and certified numerical evaluation. Coefficient and parameter uncertainty is explicit. Translation, multiplication and dilation must retain this representation or return a documented enclosure in a larger function class. Define `D_a f(x)=f(ax)`, so `hat(D_a f)(y)=|a|^-1 hat f(a^-1 y)`. Initially accept exact transformations or sufficient certified finite precision for an unambiguous array operation. Evaluation on a ball crossing a finite-function discontinuity encloses every possible value. Charge support/period enlargement and specify memory limits.

### M10 — SPEC 7 acceptance test; PLAN 5.1–5.4

**[proved here; functional-equation target from memory pending the stated proof package]**

> Define `Z(f,omega,s)=integral_{A^x} f(x)omega(x)|x|^s d^x x`, with real measure dx/|x| and local measures giving Z_p^x volume 1. For Re(s)>1, `exp(-pi x^2) 1_Zhat` with the trivial character gives `pi^(-s/2)Gamma(s/2)zeta(s)`. For a primitive Dirichlet chi of conductor C and parity epsilon, fix omega's unramified uniformizer values to chi(p); use the inverse local character on Z_p^x at ramified primes, `1_Z_p` elsewhere, and `x^epsilon exp(-pi x^2)` at infinity. Multiply the result by `C^((s+epsilon)/2)` when returning `Lambda(s,chi)=(C/pi)^((s+epsilon)/2)Gamma((s+epsilon)/2)L(s,chi)`. A separate certified continuation algorithm handles other s and states its pole behaviour. Before implementation, prove its truncation/quadrature errors and the functional equation with the chosen Gauss-sum phase, conductor and conjugate character. Acceptance requires an independent numerical path, finite error targets improving with precision, both parities and a non-real primitive character, comparison with explicitly completed FLINT zeta/L values (not an unadjusted xi value), and explicit pole/domain tests. Returning FLINT's completed-L evaluation directly does not test an integral implementation.

### M11 — SPEC 8 linear/module/root rows; PLAN 6.2–6.3

**[proved here; FLINT API facts checked by a run]**

> For exact coefficients, `ax=b` has either no solution or a coset of ann(a); an idele coefficient gives the unique solution a^-1 b. Version one exposes division by certified ideles and separately exposes finite-module solvers `Ax=b mod N`. A finite-module solver returns a particular solution and a complete kernel description with verifiable transformations, or a no-solution certificate. Integer HNF/SNF primitives do not alone constitute that solver. Local polynomial solving initially certifies simple-root Hensel branches at specified primes; singular branches and complete real-root isolation have separate algorithms, certificates and completion statuses. Modular roots are not automatically local roots. Every result states whether it is complete and what exact or ball-coefficient problem it solves.

### M12 — SPEC 8 reconstruction row and paragraph; PLAN 6.1

**[proved here]**

> Full-adelic rational reconstruction intersects the real interval I with the progression `a+N Z`, because `(a+N Zhat) intersect Q=a+N Z`; width(I)<N gives at most one candidate for N>0. Radius zero has the single candidate a. A separate partial-modular reconstruction API takes c mod m and bounds |n|<=A, 0<d<=B, gcd(n,d)=gcd(d,m)=1; `2AB<m` is a sufficient uniqueness condition. It returns a verified candidate, no candidate, multiple candidates, or an explicit failure to certify uniqueness. Coarser bounds do not force failure in every case. Reconstruction never asserts a local-to-global principle: any recovered rational must be checked in the original equations exactly.

### D1 — SPEC 15; PLAN 4 capped struct comments; PERF 4.1

**[proved here; padic distinction checked by a run]**

> Tight balls use their exact rational radii and the proved gcd rules. The optional scaled-residue policy has one integer modulus K>=1 per modulus context and stores `s(u+K Zhat)` with rational s>0 and 0<=u<K; its effective radius is sK, not K. For scales s,t, let g=gcd(s,t), A=s/g, B=t/g. Addition returns scale g and residue Au+Bv modulo K; multiplication returns scale st and residue uv modulo K. Addition is tight; multiplication encloses the tight radius `stK gcd(u,v,K)`. Exact zero/scalars and mixed-context conversions have separate proved rules. Rational scale costs are included in benchmarks. A fixed absolute cap C is a different optional policy: round effective radius R to gcd(R,C) and retain it per value. No policy may replace a coarser radius by C. FLINT padic stores precision N per value; its context caches prime powers.

### D2 — SPEC 4.1 storage; PLAN 4 structs and PLAN 5 serialization

**[proved here; FLINT names checked by a run]**

> The canonical global finite-ball payload is `(A,H,d)` with d>0, H>=0, representing `(A+H Zhat)/d`; for H>0 impose 0<=A<H and gcd(A,H,d)=1, and for H=0 use a canonical exact rational. A local backend stores the same d and residues of A modulo pairwise coprime blocks whose product is H. It owns or references a checked immutable modulus context with reduction data, CRT/remainder trees and declared lifetime; large blocks have a scalar fallback. The value carries its backend and effective radius, since tight operations can change H. Denominators are never inverted modulo non-coprime blocks. Use ulong residues with nmod_t, fmpz residues with fmpz_mod_ctx_t, and matching FLINT CRT APIs. Define deep-copy, aliasing, scratch and failure-state rules. Canonical value text is backend independent; a separate versioned physical dump may preserve backend/context metadata and uses lossless arb dyadic serialization.

### D3 — SPEC 4.1 complex type and PLAN type inventory

**[proved here]**

> Complex-valued functions on A_Q return acb enclosures and take ordinary Q-adeles as arguments. Any optional type with complex archimedean coordinate and rational finite part is explicitly the different ring `C x A_Q,f`, not A_Q or a number-field adele ring. Before freezing the header, define contracts for exact scalars, finite/partial/local balls, unit cosets, additive quotient enclosures/unions, function arrays and symbolic archimedean functions, character/conductor contexts, solver certificates and meromorphic/integral results. Context identity, precision requests, ownership, aliasing, parser limits, invalid values and thread safety are explicit. Failure codes distinguish inability to certify a unit, proved non-unit, ambiguity, required splitting and resource limits.

### D4 — SPEC 3 abstraction paragraph; PLAN 7

**[proved here; general number-field formulas remain a later proof/source gate]**

> Implement Q directly behind documented semantic seams for exact scalars, places, precision subgroups, local conversion, archimedean components and character/measure conventions. Before milestone 1, test this interface on paper against a non-principal ideal and F_q(T)'s infinity place. Do not freeze a universal public radius as a rational number or one real coordinate. For F_q(T), polynomial moduli describe affine places only; store the F_q((1/T)) infinity component separately, or use a divisor radius covering all places. Number-field support later needs ideal radii, an embedding-indexed archimedean vector and an independently verified class/idele reduction backend. Restricted products are formed relative to compact open subrings/subgroups with a specified topology and computable place-set convention. Higher adeles remain a separate out-of-scope construction. Extract reusable implementation interfaces after the Q prototype, without promising unchanged concrete structs.

### P1 — replace the milestone map and add a pre-milestone gate

**[proved here]**

> Use one version-one milestone map in SPEC and PLAN. Milestone 0 includes sources/proofs, exact-value and set semantics, precision policies, storage/conversion invariants, character/measure conventions, matched benchmark definitions and a second-field compatibility sketch. Milestone 1 first delivers global tight finite balls, exact scalars/real enclosures and a small CLI with lossless text; then adds the proved scaled policy and independently tested local storage. Subsequent slices deliver ideles/division, additive quotients/characters, finite and real test functions with weighted transforms, and certified Tate integrals/continuation. Basic full-ball rational intersection may ship earlier. Module solvers and singular-root completeness have separately accepted scope and certificates. The second field is a later implementation milestone, not the first time the public representation is checked for compatibility.

### P2 — replace the generic acceptance-test paragraph

**[proved here]**

> Acceptance tests distinguish enclosure, smallest-ball tightness, finite coset equality and numerical accuracy. Use direct small local enumeration and minimality witnesses in addition to a formula reference. Exercise rational/zero radii, negative centres, shared factors, scale alignment, aliases and both backends. Compare tight and capped evaluation from the same initial enclosures after every operation, including conversions. Quotient tests include wrapping and fractional radii; character tests include nontrivial phases; DFT tests use D!=M and nonsymmetric delta cosets with Haar weights. Poisson/integral tests require independent algorithms, certified tails and finite width targets. Solvers verify equations, certificates and small-case completeness; reconstruction separates full and partial adelic data. Fuzzing supplements golden semantic and invalid-input cases. Overlap with an unbounded ball and agreement with the same underlying routine do not count as acceptance.

### F1 — replace the affected hardware rows

**[checked by a run for instruction/source facts; model assumptions as labelled]**

| Quantity | Replacement value/qualification | Kind/source |
|---|---|---|
| `mul r64`, Zen 2 | 3 cycles to RAX, 4 to RDX; reciprocal throughput 1 cycle/instruction | MODEL parameters from uops.info's Zen 2 measurements/documentation; specify operand path |
| `add r64,r64`, Zen 2 | Latency 1, reciprocal throughput 0.25 | Checked uops.info register-form data; chain latency and throughput differ |
| `adc r64,r64`, Zen 2 | Carry latency 1; documented reciprocal throughput 0.25, measured about 0.33–0.35 | MODEL ceiling versus measured instruction behaviour; neither is gcd cost |
| `vpmuludq ymm` | Four unsigned 32x32-to-64 products, latency 3, reciprocal throughput 1 | MODEL for chosen instruction form, not a whole modular kernel |
| Clock | 3.7 GHz base, up to 4.5 GHz boost; actual benchmark clock uncalibrated | Vendor specification; ns/cycle ratios using this interval are conditional |
| Counters | RDTSCP works in the guest; PMU core-cycle access and effective frequency require separate verification | Checked locally; TSC ticks are not automatically core cycles |
| Load/store capacity | Conditional aligned L1 model: 64 bytes/cycle read, 32 bytes/cycle write; scalar-only access is a different model | MODEL assumptions; streaming beta measured/reported separately |

### F2 — replace the space-floor introduction and affected rows

**[proved here]**

> Count only the states admitted by the measured type. A fixed integer modulus N needs ceil(log2 N) centre bits. Integral centres and radii 1<=N<=B need ceil(log2(B(B+1)/2)) bits. A fixed shared denominator d adds log2 d bits to the centre count; a variable denominator requires its own state count and bounds. Arbitrary rational radii and exact rationals have no size-independent finite floor. The 168-bit real-ball and 29-byte adele figures are restricted MODEL layouts with bounded exponent/radius codes and shared integral finite scale, not information floors for general arb or scaled rational balls. Include tags, alignment, allocated capacity, shared contexts and temporaries in measured sizes. A 528-byte chunk over a 512-byte payload is x1.03125; count any separate GMP/fmpz overhead explicitly.

### F3 — replace the time-floor table's model column and ratio policy

**[proved here under the stated models]**

> Every time row specifies problem accuracy, input family, cached data, output materialisation, computational restrictions and measurement mode. Report a numerical ratio only when its numerator matches those assumptions. Algorithm costs are upper-bound references and never appear in the floor denominator. For the historical baseline retain the two word dependency-chain rows as conditional implementation-model comparisons; mark the word-gcd and arb optimality ratios unestablished. Do not call a ripple-carry dependency a universal addition floor. The following I/O bounds are for new matched compulsory-I/O throughput tests, not calibrated latency ratios for the historical file.

| Operation | Replacement floor entry |
|---|---|
| Word add chain | MODEL: required add latency >=1 cycle; stronger bound requires actual correction DAG |
| Word multiply chain | MODEL: required high-half MUL >=4 cycles; inspected implementation's partial dependency path >=14 under the checked MUL/ADC and separately stated ADD/SUB/IMUL latency assumptions |
| Word gcd | PROVED: only a word-probe bound here; no numeric core-cycle floor established for the current mixed chain |
| 4096-bit full add | MODEL: compulsory vector-I/O >=17 cycles; 64-cycle carry path applies only to a ripple algorithm |
| 4096x4096-bit full multiply | MODEL: vector payload I/O >=32 cycles, or >=128 with scalar stores explicitly required |
| 8192/4096 reduction | MODEL: vector payload I/O >=24 cycles with numerator and modulus compulsory; scalar reads give 96 under a different model |
| 4096-bit gcd | MODEL: vector compulsory-input rung >=16 cycles; scalar reads give 64; neither proves arithmetic optimality |
| 128-bit arb add/multiply | Floor/ratio pending specified exponent/radius/accuracy contract and matched benchmark; no three-multiply or two-carry theorem asserted |

### F4 — replace PERF's measurement-method paragraph and benchmark acceptance rule

**[proved here; inspection facts checked by a run]**

> The dated baseline contains nmod dependency chains, a word-gcd chain mixed with input generation, and fixed-operand repeated fmpz/arb call rates. Large gcd has 9 trials; other rows have 15. Preserve those measurements as primitive references with conditional clocks and their exact workload labels. New benchmarks must apply/log their seed, record actual bit lengths and affinity, consume results, separate warm and allocating calls, and specify latency, independent throughput or API call rate. Include operand families and matched real/finite precision requirements. Compile and inspect the timed loop when using a dependency-path floor. A compiler memory barrier alone is not a hardware latency measurement.

### F5 — replace PERF 4 design conclusions

**[proved here]**

> The primitive measurements motivate hypotheses, not speedup promises. Benchmark complete tight and scaled-ball kernels before deciding whether gcds, rational normalization, real arithmetic or conversions dominate; include structured radii and exact scalars. A scaled residue product still pays for its rational scale. Shared modulus saves the radius payload only when the scale/precision semantics permit it. Moduli below 2^32 are a useful first AVX2 backend restriction, not a hardware necessity. Retain arb initially; a specialised real-ball kernel requires a separately specified accuracy/range contract, an end-to-end profile and matched lower-bound ratios. No order-of-magnitude improvement is inferred from the current arb rows.

### F6 — replace PERF 5's placeholder paragraph

**[proved here for floors; algorithm references qualified in F6 above]**

> Add separate setup and operation rows. Let b=ceil(n/8), k be the number of pairwise coprime 32-bit residue moduli, L=DM and W bytes per materialised complex coefficient. Under compulsory dense L1 I/O at 64 read bytes and 32 write bytes/cycle, global-to-local conversion has payload floor max(b/64,4k/32), reverse CRT max(4k/64,b/32), and dense finite DFT LW/32. Bounded rational reconstruction uses max(total input bytes/64,total output bytes/32), alongside an explicit-input worst-case linear bit-access bound. A selected native-product AVX2 modular multiplication kernel requires at least ceil(k/4) VPMULUDQ instructions before reduction; its dependency latency and reduction cost are separate. Text I/O has a linear output/input-byte bound. Idele-class product combines real-ball and unit-residue kernels at shared modulus. Product/remainder-tree, FFT and half-gcd costs are algorithm references, not floors; list their assumptions and primary sources separately. Streaming rows add actual traffic/bandwidth and setup costs are amortised only with a stated reuse count.

### A2 — replace SPEC 12's two unverified bullets and PLAN's backend claim

**[checked by a run]**

> FLINT 3.0.1 supplies number-field element arithmetic and some specialised local/quadratic infrastructure; arbitrary-number-field adelic support still needs a chosen maximal-order, fractional-ideal, class-group and unit backend. Some fast class-group computations give conditional results by default; unconditional certification is possible, for example via successful full PARI bnfcertify. Record the exact guarantee and extra cost instead of saying certification is possible only under GRH. Future backend selection remains an explicit work package.

## What this changes in the plan

The ordering below is **[proved here]** as a recommendation from the demonstrated dependencies and counterexamples; calendar effort remains **[from memory]** until these scopes are fixed.

1. Reconcile SPEC and PLAN's version-one scope and milestone numbering. Separate optional solving deliverables from the Tate acceptance gate.
2. Approve the mathematical contract for basic finite balls, rational singletons, exact diagonal values, set equality, overlap and containment. Adopt M1's proof and the progression identity for full-ball rational reconstruction.
3. Choose one explicit capped policy. Specify its scale alignment, exact-scalar and conversion rules; turn the fixed-cap counterexample and the intentional-loss examples into mandatory tests.
4. Freeze the denominator/storage invariants and lifecycle/context contracts, including large local blocks, changing tight moduli, backend-independent value text and lossless physical dumps if required. Remove unsupported ABI promises.
5. Add missing public result domains: partial places, finite units, additive quotient enclosures with splitting, function/character metadata and solver/integral certificates. Resolve what the optional complex-coordinate ring means.
6. Fix Tate's paired character/Fourier signs, additive and multiplicative Haar measures, finite DFT weights, conductor/parity conventions and twisted local test vectors. Put sources/proofs on disk with reproducible identifiers; this review has not completed the failed full-thesis downloads.
7. Split function support/period algebra, Gaussian-family closure, certified Poisson tails, numerical integration and meromorphic continuation into independently testable work packages. Define width targets and pole behaviour before an implementation is called certified.
8. Replace circular or overlap-only acceptance tests with P2's semantic, precision and completeness checks. Keep the existing prototype as a regression tool and add the proof-backed edge cases saved with this review.
9. Replace PERF's invalid ratios, document scalar versus vector and latency versus throughput models, repair/parameterise the harness, and add F6's missing floor rows. Defer a bespoke real-ball kernel until complete-operation evidence warrants it.
10. Finish the directly relevant Hertogh source comparison and correct the GRH/backend claims. Record what was actually read, what was run and what remains from memory.
11. Write the short second-field compatibility sketch for a non-principal ideal and F_q(T)'s infinity place. Implement Q first behind those seams; implement the second field later.
12. Start milestone 1 with the global tight finite-ball slice and exact scalar/enclosure conversion. Bring in scaled and local kernels only after their mathematical invariants and acceptance tests are in place.
