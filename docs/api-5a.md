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

Check: test_tate_local reads all 394 rows of tests/ref/vectors/t-slice1/local.jsonl and all 1861 samples.
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
[source pending: universal FLINT Conrey pairing identification, inherited from api-3d:283-290]
No new dependence on it is introduced into numerical evaluation, which reads only conductor and parity.
