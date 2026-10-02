# Proof review notes

These notes concern the claims as written, not a replacement specification.

## F10

The first omitted odd degree is 2 floor(C/2)+1, and the first omitted even degree is
2 ceildiv(C,2). Both are at least C. Alternation cannot lower the valuation of a sum below
the smallest term valuation. There is no missing factor of two in the counts.

When a nonconstant odd sum is needed, K>w, so C>=2 and its top odd degree is at least 1.
When a nonconstant even sum is needed, K>2w-v_p(2), so C>=3 and its top even degree is at least 2.
Thus the decrement to select a parity in src/lfunc.c:711 cannot produce a negative degree on this path.

## F11

Interpret its invariant as the state after the step indexed k and before modular reduction.
At that point F=L!/(k-1)! and A is the stated polynomial. The induction is valid with the
zero coefficients included. At the last step the polynomial equals L! times the finite series.

Each finite-series term is p-integral. Therefore every summand of the numerator has valuation >=D.
The residue representative is divisible by p^D because W=K+D>D. The residue of F divided by p^D
is a unit modulo p^K. Replacing a rational input by a representative modulo p^W changes this integer
polynomial by an element of p^W Z_p. Division by p^D leaves precision K. No divisibility gap found.

## F12

For odd functions h(t)=f(t)-t has only degrees >=3. Factoring u^k-t^k gives a difference valuation
at least v_p(u-t)+(k-1)c-v_p(k!). For k>=3 this excess is positive on the stated domain.
Thus h contracts by at least one digit. For b in f(a)+p^M Z_p the displayed iteration stays in B.
Its successive differences tend to zero. The limit satisfies t=b-h(t), hence f(t)=b.
The onto claim does not follow from isometry alone, but the supplied contraction supplies the needed step.

For a centred even ball, the quadratic term at t=p^M has valuation 2M-v_p(2).
For k>=4 even, its competitors have strictly larger valuation. At odd p,
(k-1)/(p-1)<k-2 for k>=4; at 2, v_2(k!)-1<=k-2<2(k-2).
The values at 0 and p^M therefore differ with exactly the claimed valuation. The hull is smallest.
For 4 Z_2 and 8 Z_2 this gives E=3 and E=5, respectively. The enumeration checks these cases.

## F13

The first status sentence in step 1 is false without a domain qualification. It says LONG_MIN
returns LIMIT unless exact zero ignores it. Exact 2 at p=2 is neither zero nor in the domain;
the implementation returns DOMAIN. The ball 2 Z_2 returns NOT_DETERMINED at the same request.
The introductory check order in F13 and the code both give the domain check precedence.
The erroneous sentence should be restricted to inputs that pass the input-bound and domain checks.
This does not refute the specified status order. f13.in reproduces both counterexamples for all four functions.

The arithmetic bounds themselves withstand the review. After |v|,|M|<=2^60, doubling either
quantity and subtracting at most one remains within signed 64-bit range. The centred expression is
only relevant for M>=c. K is bounded before any sum count; K bits(p)<=2^26 implies K<=2^25.
C<=2K, L<C and D<=L imply W<3K. The products with p-1 are formed as fmpz integers.
The implementation compares N before doing arithmetic with the bounded K. No signed overflow found.

## F14

The local dispatch copies the selected component and keeps the input alive through the final swap.
On failure it reports the requested prime. On success where is left untouched.
The real wrapper checks the precision before place membership, checks membership before the complex tag,
clamps working precision below two, and tests finiteness before changing the output.

Ground truth for the external arb behavior is on disk:
refs/src/flint-3.0.1/arb.rst:6-12 states the enclosure convention; lines 1209-1219 describe sinh/cosh.
The wording at lines 9-11 is: "contains the result of the (mathematically exact) operation
applied to any choice of points in the input balls."
No stronger claim about arb, or smallest real intervals, was used.

## Test review

Read tests/test_lfunc_trig.c, tests/julia/lfunc_trig.jl, proto/lfunc_trig_checks.py,
the added tests in tests/test_rfunc_prime.c, and lanes/f-slice7/faults.py.
The exact fixture assertions compare residues and exponents, not overlap alone.
The enumeration requires a nonempty grid and a witness modulo p^(E+1).
For the hyperbolic identity on uncertain balls, the reverse containment only checks compatibility
with a wider interval-arithmetic enclosure. It is not itself an enclosure proof; the separate
point-oracle tests supply that evidence. No assertion that is identically true was found.
No repository test suite or lane oracle was executed by this review.
