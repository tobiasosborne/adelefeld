# Slice 5a

This slice implements `adf_local_tate_at` in localfactor.h and localfactor.c. The value is the
meromorphic standard-vector integral of api-5 section 2, not its local functional-equation constant.
The finite part of eta0 is used; its stored exponent is ignored. Raw s and alpha may alias z.
Character-member aliases are forbidden. where may be NULL.

## Value and enclosure

At p, first validate that the primitive conductor is 1 or a power of p by exact word divisions.
The canonical-character precondition supplies primitivity. No new group or character is constructed.
At infinity the primitive conductor can be any admitted value; only parity e is used.

For conductor 1, Proposition 9 step 1 (analysis.md:361-365) integrates each multiplicative shell
of volume 1. It gives sum_(k>=0) alpha^k p^(-ks) in Re(s)>log|alpha|/log p. Summing the series gives
`1/(1-alpha exp(-s log p))`. This identity gives the meromorphic continuation used here.

1. arb_log_ui encloses the positive real log p.
2. Multiply that ball by the entire input rectangle s, negate, and apply acb_exp.
   Thus T encloses exp(-s log p) for every s of the input, including log rounding.
3. Multiply T by the entire alpha rectangle and subtract from 1. The result encloses every denominator.
4. Invert only after the denominator excludes zero. Division then encloses all values.
5. Round outward to max(2,prec) bits and check finiteness before swapping into z.

There is no midpoint substitution that discards an input radius. A zero-containing computed denominator
returns NOT_DETERMINED. It does not prove an exact pole. For exact alpha=1 the call delegates to
adf_local_zeta_factor_at and preserves its result representation, certificate, statuses and precision policy.
Its stable expm1 algorithm remains useful near zero and at large negative real arguments.

For positive conductor exponent, the prescribed vector is eta0^-1 on the units. Each unit value of eta0
is a root of unity, so eta0^-1=conj(eta0). Multiplying the vector by eta0 cancels pointwise. The integrand
is exactly 1 on Z_p^x and zero elsewhere. Its multiplicative measure is 1. The returned acb is exact 1
at every s and every certified nonzero alpha. This is P9:354-365 and conventions 6.5:963-966.
For the non-real character (5,2), the wrong same-character vector has unit average zero; the tests
check that average separately and require the local call to return 1.

G_minus is not evaluated in 5a. P9:367-374 uses it for the transformed vector and gamma, in slice 5b.
A certified sum would combine the negated unit-character phase with -u/p^a modulo 1, evaluate each
combined phase by adf_phase_get_acb, and sum outward. Using that sum as this integral is a different
function. No kernel-sign, conductor-exponent or inverse-Gauss-sum mutation has a production site here.

At infinity P10:403-414 and :424-426 use x^e sign(x)^e=|x|^e and t=pi x^2 on the positive half-line.
The two half-lines give `pi^(-(s+e)/2) Gamma((s+e)/2)`, initially Re(s)>-e. For e=0 the call delegates
without changing s. For e=1 it adds 1 with outward rounding and delegates. An exact odd negative integer
is checked before that rounding, so a very large exact pole cannot become an undecided rounded box.
The inherited Gamma candidate, midpoint refinement and at most 64-factor recurrence enclose the shifted
factor by local-zeta.md Z4/Z6. Every interval multiplication, division, intersection and final rounding
keeps an enclosure. The new function writes z only after an OK status.

Check: test_tate_local reads all 394 rows of tests/ref/vectors/t5-slice1/local.jsonl and all 1861 samples.
Check: exact ramified 1; 4/3 and 1/pi containment; identical trivial-factor delegation; both real parities.
Check: every vector repeats z=s, z=alpha and where=NULL; a separate case permits z=s=alpha.

## Poles

For conductor 1 at p, a pole solves alpha exp(-s log p)=1. Fix any logarithm of nonzero alpha.
The solutions are `(log(alpha)+2 pi i k)/log p`, k in Z. Differentiating the denominator gives
log p at every root, which is nonzero. The numerator 1 does not cancel it. All poles are simple.
For alpha=1 this is the trivial pole lattice. There are no s poles at a ramified prime because its
standard integral is the constant 1.

At infinity put z=(s+e)/2. Gamma has simple poles at nonpositive integers and no zeros; the exponential
prefactor is everywhere nonzero. Hence exactly `s=-e-2k`, k>=0, are poles. Their residues are
`2*(-1)^k*pi^k/k!`. In particular the odd integral has a pole at -1. The odd gamma factor's pole at 2
is a pole of a different function, as api-5 section 9 states.

Exact finite poles are recognized when s=k is an exact real integer and alpha=p^k is exactly represented,
with a bounded exact power. Negative powers can be exact dyadics only when p=2. Otherwise a supplied
ball around that rational is a mixed pole input. Other exact finite pole recognition is optional in the
design. At real places the inherited integer geometry detects exact and mixed poles, with the added
exact odd-integer test before shifting. Closed endpoint contact counts as intersection.

Check: integer finite poles at all six primes, nonzero pole lattices, exact/mixed/adjacent poles for
both real parities, closed boundaries, and the exact odd pole -2^1000+3 at prec=53.

## Status, atomicity and resources

The status row is Characters, Gauss sums, local factors (conventions.md:223), as api-5:35-37 decides.
The accepted D2 precision/conductor limits use its LIMIT status; the Integrals, Poisson row at :225
is used by the global calls. No UNIT_NOT_CERTIFIED, NEEDS_SPLIT or branch-cut status applies here.

- LIMIT: precision above 2097152 first; conductor above 65536 next; inherited Gamma shift cap 64.
- DOMAIN: nonfinite s; nonfinite used alpha; exact alpha=0; invalid prime-power conductor; recognized exact pole.
- NOT_DETERMINED: alpha rectangle contains 0; mixed or undecided poles; nonfinite computed value.
- OK: finite enclosure of the entire admitted input family. No relative or absolute-width promise is added.

At infinity alpha is ignored even if nonfinite. At p certify alpha nonzero even for the ramified constant.
On failure z is untouched byte for byte, and where receives v if supplied. On OK where is untouched.
The working precision is max(2,prec). The new geometric path adds 32 guard bits, clipped to the cap.
The inherited trivial/real path retains its existing max(2,prec)+32 policy to meet exact delegation.
That inherited policy exceeds api-5:33's guard-cap wording at the cap. This is a design reconciliation
finding, not a new silent change to the inherited factor.

Validation costs O(log conductor) word divisions. The unramified path has O(1) log/exp/inverse calls.
The optional pole recognizer computes p^k only if its integer uses at most 2^20 bits. Otherwise it skips
recognition and lets the enclosure certificate decide. No loop depends on Im(s) or a large integer s.
The ramified path assigns exact 1. The real path inherits at most 64 recurrence factors and bounded
refinement. No work cap beyond these inherited bounds is reached by this constant-size slice.
INV additionally runs the existing canonical character predicate, whose group-setup cost is inherited.

The fixture width test is deliberately separate from the API enclosure contract. Each returned coordinate
diameter must be at most `64*sampled_width_lower + 2^(-prec+8)*image_bound`. The lower width is a certified
separation between a centre and sampled endpoint images, not a claim to the full image diameter.
All s/alpha corners and the centre are sampled. Fixed input radii therefore remain in the test.
Point references use 60 decimal digits plus an outward conversion margin from an independent interval.
The real references integrate a Taylor polynomial with tails from zeta_checks Z8; no FLINT Gamma is used
as that reference. Ramified references are the exact cyclotomic constant 1.

Check: byte sentinels on every failure; where on every local failure; precision 2, 53, the cap and one above;
raw nonfinite inputs; invalid conductor; member-alias/forged-place INV aborts; huge and near-pole operands.
Check: tests/driver/tate-local.cmd; tests/julia/tate_local.jl, using constructor-produced Place by value.

## Local sources and findings

refs/src/flint-3.0.1/acb.rst:6-17 defines separate real/imaginary error bounds. Its :893-896 says
"Computes the gamma function" and :898-902 specifies reciprocal Gamma while avoiding division by zero
at its poles. This slice uses Gamma through the inherited evaluator; it does not need reciprocal Gamma.
Its :637-643 defines acb_pow through exp(y log x) in the general case. The implementation uses the
positive real logarithm and exp explicitly, so no complex-log branch cut is introduced.
refs/src/tate-poonen/notes.txt:62-64 supplies Gamma's simple poles and absence of zeros.
The finite and real formulas above are project definitions proved stepwise in P9/P10.

The requested production G_minus faults, and the conductor-a=1 gamma fault, do not apply to the integral
1. They remain meaningful oracle witnesses for 5.1 as a whole and production faults for 5b.
A place outside Q cannot be constructed with place.h; forged handles violate a precondition and abort
under INV. They are not valid DOMAIN requests for this signature. No header change is needed for either point.
The FLINT Conrey pairing identification is closed (api-3c P3.5 to P3.7, from flint-src-3.0.1:dirichlet/
pairing_char.c:23 and char_pow.c:19-20; the conjugate label by exponent negation is proved unconditionally).
No new dependence on it is introduced into numerical evaluation, which reads only conductor and parity.

# Slice 5b

This slice implements `adf_local_gamma_at` and `adf_local_epsilon_at` in localfactor.h and localfactor.c
(design api-5.md section 2, lines 48-119; decisions D1, D2). gamma is the factor of the fixed local functional
equation `Z_v(F f, eta^-1, 1-s) = gamma_v(s, eta) Z_v(f, eta, s)`; epsilon is its simplified entire normalisation.
The finite part of eta0 is used; its stored exponent is ignored. The pair (alpha, eta0), the conductor
validation, the alpha checks, the status order, where, aliasing and atomicity are those of slice 5a above.

## Values

At p with conductor exponent a (conventions 6.4; P9:344-351), T = exp(-s log p) with the real logarithm:

    a = 0: gamma_p = (1 - alpha T)/(1 - alpha^-1 p^(s-1)),    epsilon_p = 1;
    a > 0: gamma_p = alpha^a T^a G_minus(eta0^-1),           epsilon_p = gamma_p,
           G_minus(eta0^-1) = sum_(u mod p^a, p not | u) eta0(u)^-1 E(-u/p^a)   (the NEGATIVE kernel).

At infinity, e = eta0 parity, alpha ignored even if nonfinite (P10:406-410; api-5.md:74-78):

    gamma_inf = i^e pi^(s-1/2) Gamma((1-s+e)/2) rgamma((s+e)/2),    epsilon_inf = i^e.

## Steps and why each keeps the enclosure

1. G_minus is a certified finite sum (api-5.md:66-69). For each unit u of 1..p^a-1, adf_char_chi_phase gives the
   exact phase theta of eta0(u) in [0, 1). The code negates it, adds -u/p^a, and reduces the rational into [0, 1)
   exactly. adf_phase_get_acb encloses E of that phase. acb_add sums the enclosures with outward rounding
   (arb.rst:6-12). The positive Gauss sum is not called. The working precision is prec + 32 + bits(p^a), clipped
   to the cap. A setup failure of the character (UNSUPPORTED) gives NOT_DETERMINED (api-5.md:37-38).
2. Ramified gamma: acb_pow_ui encloses alpha^a on the alpha rectangle. arb_log_ui and acb_exp enclose
   exp(-a s log p) on the s rectangle (acb.rst:658-660). The product of enclosures encloses the product.
   The function is entire, so there is no pole test.
3. Unramified gamma: multiplying numerator and denominator by alpha gives alpha (1 - alpha T)/(alpha - p^(s-1)).
   This is valid because alpha != 0 is certified for every point of the rectangle first. Numerator and
   denominator are enclosed directly from the input rectangles; no midpoint substitution drops a radius.
   The quotient is formed only when the denominator enclosure excludes 0; then acb_div encloses every value.
   A denominator enclosure that contains 0 is NOT_DETERMINED and never DOMAIN. The optional midpoint
   refinement of api-5.md:91-92 is not used.
4. Real gamma. With w = (1-s+e)/2 and z = (s+e)/2 one has -w + z = s - 1/2, so
   gamma_inf = i^e [pi^-w Gamma(w)] [pi^z rgamma(z)]. The first factor is adf_local_zeta_factor_at at 1-s+e.
   Its integer geometry, Gamma certificate, 64-factor recurrence and midpoint refinement are inherited
   (local-zeta.md Z4). The second factor is entire. Its rgamma(z) is computed as
   z (z+1) ... (z+n-1) rgamma(z+n), with n chosen as in Z4 so that min Re(z+n) >= 1. This is the identity of
   entire functions that comes from Gamma(z+n) = z ... (z+n-1) Gamma(z), notes.txt:61 iterated. A product of
   ball factors encloses the product. acb_rgamma encloses rgamma at z+n (acb.rst:898-902). No division
   occurs, and in particular no division by a nonfinite Gamma. If n > 64, acb_rgamma(z) is used directly,
   which is also an enclosure; rgamma is entire, so there is no LIMIT. The shift is needed for tightness, not
   for soundness: for z = -2 +/- 2^-12, FLINT 3.0.1's direct acb_rgamma has radius 3.8e-2 against a variation
   of about 7e-4 (lanes/t5-slice3/redgreen.md item 3).
5. Exact integers at infinity are tested first on the original dyadic number (arf_bot gives the parity), so a
   large s is never rounded. A pole is DOMAIN. A zero returns the exact 0: the value there is 0 by the pole sets
   below, since Gamma((1-s+e)/2) is finite.
6. Every result is rounded outward to max(2, prec) bits and checked finite before it is swapped into z.
   epsilon is assigned exactly (1, i) in the unramified and real branches; in the ramified branch it is
   gamma's step 2.

## Pole and zero sets

- Unramified gamma. The denominator alpha - p^(s-1) vanishes iff alpha^-1 p^(s-1) = 1. Fix a logarithm of
  alpha; then the roots are s = 1 + (log alpha + 2 pi i k)/log p. The s-derivative of the denominator is
  -p^(s-1) log p = -alpha log p != 0 at a root. At a root p^-s = 1/(p alpha), so the numerator is 1 - 1/p != 0.
  Hence every root is a simple pole and nothing cancels. The integral's poles alpha T = 1 are the zeros of
  the numerator, where gamma = 0 is a value. Both conditions together would give alpha T alpha^-1 p^(s-1) = 1,
  which is impossible since that product equals 1/p (api-5.md:86).
- Ramified gamma and every epsilon have no s poles.
- Real gamma. pi^(s-1/2) and i^e have no zeros or poles. Gamma has simple poles at 0, -1, -2, ... and no zeros
  (notes.txt:62-64), and rgamma is entire with zeros exactly there. Hence the poles are w = -k, that is
  s = 1+e+2k, and the zeros are z = -k, that is s = -e-2k, for k >= 0. At such a pole z = (1+2e+2k)/2 is not a
  nonpositive integer, so rgamma(z) != 0. At such a zero w = (1+2e+2k)/2 > 0, so Gamma(w) is finite and nonzero.
  The two sets are disjoint because 1+2e+2(k+j) is odd. All poles and zeros are simple. The integral's
  poles -e-2k are gamma's zeros. The trivial list 0, -2, ... is not gamma's pole list (api-5.md:391-393).
- Recognition. Finite exact poles: s = k is an exact integer and alpha = p^(k-1) is exactly represented.
  This is slice 5a's recognizer applied to the exact s - 1, with the same 2^20-bit power bound; negative
  powers are dyadic only for p = 2. Examples are (alpha, s) = (1, 1) and (1/2, 0) at p = 2; (1, 0) is a zero.
  Real exact poles and zeros are recognized for every exact integer. A ball meeting a pole, with closed
  contact, is NOT_DETERMINED: at p because the enclosed denominator then contains 0; at infinity through the
  inherited geometry on 1-s+e.

## Statuses, cost and caps

Row: Characters, Gauss sums, local factors (conventions.md:223), as for slice 5a.

- LIMIT: precision above 2097152 first; then conductor above 65536; for real gamma the inherited shift cap 64
  of the numerator.
- DOMAIN: nonfinite s; at p, nonfinite alpha, exact alpha = 0, or a conductor that is not 1 or a power of p;
  for gamma, a recognized exact pole.
- NOT_DETERMINED: alpha rectangle contains 0; for gamma, a mixed or undecided pole; a character setup failure;
  a nonfinite computed value.
- OK: a finite enclosure on the whole input rectangles. No accuracy promise beyond the fixture's width test.

Failure leaves z untouched byte for byte and writes v to where if where is not NULL. z may alias raw s and
alpha. Unramified gamma costs one log, two exponentials and one division. Ramified gamma and epsilon cost
p^a - p^(a-1) phase calls and phase evaluations, plus one power and one exponential. Each phase call repeats
the character's group setup D(p^a) (char.h:79); that cost is avoidable. Real gamma costs the inherited
local-zeta call, at most 64 products, one rgamma and one exponential. Real and unramified epsilon are O(1).
The new paths add 32 guard bits, plus bits(p^a) for G_minus, clipped to the cap. The real numerator inherits
the existing max(2, prec) + 32 policy, the same reconciliation finding as in slice 5a above.

## Decisions where the design is silent

- The unramified denominator is enclosed as alpha - p^(s-1), not as 1 - alpha^-1 p^(s-1). The two have the
  same zero set for alpha != 0, and this form avoids an inversion of the alpha rectangle.
- The real numerator delegates to the local zeta factor at 1-s+e, as slice 5a delegates at s+e.
- rgamma is computed with the shifted product (step 4). The design names rgamma only.
- Exact real zeros return the exact 0, which is a tight ball, for every size of s.
- The driver names are tate_gamma and tate_epsilon (from the brief; the design writes local-gamma/local-epsilon).
  The line prefix `continuation S=` is that of tate_local.

Check: test_tate_gamma reads all 721 rows of tests/ref/vectors/t5-slice3/gamma.jsonl and all 1950 samples.
These cover p = 2, 3, 5, 7, 13, six alphas, conductors p and p^2 (4, 8, 16 at p = 2), both real parities,
gamma's poles, the integral's poles and the real zeros. Every call repeats z = s, z = alpha and where = NULL.
Check: the exact quotients -3/4, 18/5 and (10 - 15i)/13 and the zero at (1, 0). G_minus against conj(eta0) with
E(-u/q), against (-1)^e tau(conj eta0), and not overlapping the positive kernel or eta0 in place of eta0^-1
(non-real (5,2), (13,2), (16,5), (9,2), (25,2)).
Check: epsilon exactly 1 unramified and exactly i^e at infinity, including at gamma's poles. Real s = 1/2 gives
1 and i. Exact zeros give exact 0, including -2^1000 and -2^1000 - 3. Exact poles give DOMAIN, including
2^1000 + 1 and 2^1000.
Check: the duality gamma(alpha, eta0, s) gamma(alpha^-1, eta0^-1, 1-s) = eta0(-1) on at least 120 finite
cases, with its hand derivation in the test; precision 2, 53, the cap and one above; the conductor cap;
INV aborts.
Check: tests/driver/tate-gamma.cmd; tests/julia/tate_gamma.jl.

## Closure of m5-review1

F1 is closed in the shared real factor. For z = (s+e)/2, choose n from the outward lower real endpoint
so that Re(z+n) >= 1. When 0 < n <= ADF_LOCAL_ZETA_SHIFT_MAX, form the recurrence product as a loop
of ball multiplications and intersect direct Gamma(z) with Gamma(z+n)/product_(j=0..n-1)(z+j).
Gamma(z+1) = z Gamma(z) is refs/src/tate-poonen/notes.txt:61; acb_gamma is
refs/src/flint-3.0.1/acb.rst:893-896. A product containing zero cannot certify division. A finite direct
candidate is retained in that case. The existing exact and mixed pole tests still run first.
Above the shift cap a finite direct value remains usable; a failed direct value still gives LIMIT.

The recurrence alone does not meet this fixture target on FLINT 3.0.1. The midpoint certificate is also
sharpened by the following bound. No input radius is discarded.

1. On the pole-free rectangle, differentiate L(s) = pi^(-s/2) Gamma(s/2). The product and chain rules give
   L'(s) = L(s) (digamma(s/2) - log(pi))/2. Digamma is Gamma'/Gamma by
   refs/src/flint-3.0.1/acb.rst:916-918.
2. The candidate ball encloses L on the entire rectangle. Its magnitude, multiplied by the magnitude
   of the second factor and divided by 2, bounds |L'| there. Every magnitude operation rounds outward.
3. The smaller of this bound and Z6's bound is still an upper bound. If digamma is nonfinite, Z6 remains.
4. Integrate L' along the segment from the midpoint to any point of the convex rectangle. Its length is
   at most the Euclidean input radius R. Enlarging the midpoint value by R times this bound encloses L.
   Intersection with the recurrence candidate and outward rounding preserve enclosure.

The real fixture width remains `64*sampled_width_lower + 2^(-prec+8)*image_bound` per coordinate.
It applies to the two review inputs eta0 = (1,1) and (5,4), alpha = 1, prec = 160, and
S = -3/4 +/- 2^-12 + i (0 +/- 2^-12). Both characters are even; here z = s/2 is centred at -3/8.
The certified sampled lower width is 0.00010116010017604849 and image_bound is 20.
The centre and all four corners are included. This remains a fixture target, not an API width promise.

Check: `test_tate_local` appends `review_real_width`; its 60 checks pass after four width checks failed
on the unchanged implementation. All 394 slice 5a vectors and 1861 samples still run.
Check: `test_tate_gamma` still runs all 721 slice 5b vectors and 1950 samples.
Check: `lanes/m5-review1/extra_checks.py real`, linked to the repaired archive, exits 0 with
2594 checks, 160 inputs, 480 calls and 0 failures.

The bounded mutation run exposed a missing inclusive character-cap test. `test_tate_gamma` appends
`inclusive_character_cap`: the primitive even character (65536,5) at infinity and s = 1/2 has gamma
and epsilon containing 1. Changing gamma's conductor test from `>` to `>=` fails this test; the live
code passes it. No conductor limit or header changed.
