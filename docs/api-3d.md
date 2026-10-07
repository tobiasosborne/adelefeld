# Slice a

Implementation of docs/api-3c.md sections 1-4 and slice a of section 7.
The specification is unchanged. The stored finite character is primitive and s is an owned finite acb.
The represented family is t^z chi(u') for z in s, as SPEC 5 and conventions 5.13 state.
This slice evaluates only the finite character and computes its Gauss sum and root number.
P1 and P2 describe coset images and hulls for later slices. This slice uses P3 for lowering and P4 for summation.

## Lifecycle and predicate

adf_char_init sets (q,n,parity,s)=(1,1,0,0). adf_char_clear clears the two owned arb components of s.
adf_char_set copies every field and s exactly; adf_char_swap exchanges all fields and the acb storage.
Both permit whole-object self-aliasing. No group or character object is retained in the value.
The FLINT memory contract is refs/src/flint-3.0.1/memory.rst:9-24; strings use flint_malloc/flint_free.

adf_char_is_canonical first rejects bad ranges, gcd, parity range and nonfinite s.
For q=1 it tests n=1 and parity=0. Otherwise it builds a group and character, then compares conductor
and parity with the stored fields. It applies no modulus cap. The cost is D(q), including factorization
and discrete logarithms, as api-3c section 1 requires. It is not O(log q).
The declarations used are /usr/include/flint/dirichlet.h:68,90,110,111,114.

HEADER-FINDING: api-3c section 1 says that this predicate never aborts, while section 7 and N-D22
forbid returning false for a resource failure. Its int result has no resource-status alternative.
The group-init failure semantics are source pending. The implementation calls flint_abort if setup
returns 0, so it never reports a mathematical false because of setup failure. This is a fail-stop
choice, not a proof that failure is unreachable. A status-returning predicate is the alternative;
that would change the fixed declaration and is not implemented here.

adf_char_identical tests all three integer fields and acb_equal(s). It is representation identity.
adf_sizeof_char and adf_alignof_char are header-inline and exported through src/inlines.c.
Bindings allocate with these queries. They do not guess the offsets or assume an acb size.
Cost is O(1) for layout/swap, and the size of s for copy/identity. Lifecycle functions have no status.
INV checks canonical value inputs, including clear. is_canonical itself does not invoke an INV macro.

Check: principal initialization, deep heap midpoint copy, self-copy, self-swap, exchanged s,
layout, malformed storage, wrong parity, imprimitive (16,9), and a primitive character above 65536.

## Constructors and accessors

adf_char_set_conrey and adf_char_set_conrey_acb implement api-3c section 2 and P3.1-3.
They check q!=0, gcd(n,q)=1 and finite supplied s before setup. The zero constructor supplies exact s=0.
q=1 accepts every ulong n and stores (1,1,0). For q>1 the reduced label is logged in G(q).
The conductor C is computed there. If C=1 the stored label/parity are (1,0).
Otherwise H(C) and a second character are initialized, dirichlet_char_lower constructs the inducing
primitive character, and its label/parity are read in H. Neither n mod C nor n mod 2 supplies these fields.
The on-disk declarations are /usr/include/flint/dirichlet.h:68,90,110-120,136.
Both groups and characters are cleared before committing the scalar fields and exact copy of s.
Every recoverable failure therefore precedes every output write.

Statuses: OK, DOMAIN for the cheap raw checks, LIMIT for q>65536 after those checks,
UNSUPPORTED when a required group cannot be initialized. The cap precedes every group or logarithm.
The constructor cost is D(q)+D(C), plus copying s. It deliberately repeats setup even when C=q.
An alternative is retaining the original primitive group for the property reads. No cache is stored.

adf_char_set_s exactly copies finite s, with OK or DOMAIN, and never applies D1.
adf_char_get_conductor, adf_char_get_label and adf_char_get_parity are exact field reads.
adf_char_get_s copies s into independent initialized storage. They have no status or setup budget.
adf_char_get_order returns the character's multiplicative order using dirichlet_order_char
(/usr/include/flint/dirichlet.h:112). D1 precedes INV/setup. OK writes order; LIMIT/UNSUPPORTED preserve it.
At C=1 the order is 1. The cost is D(C), apart from this principal branch.
Whole-object aliases are permitted only for the lifecycle calls. Raw/output acb member aliases are forbidden.

Check: all 1966 pairs q<=80, including (16,9)->(8,5), (8,7)->(4,3), principal (8,1),
reduced and unreduced labels, finite/nonfinite s, preserved failure bytes, 65536 accepted and 65537 refused.
A linker wrapper verifies that all three constructor cap rejections invoke zero group setups.
It also injects both lowering setup failures.

## Value text

adf_char_set_str and adf_char_get_str implement api-3c section 2 and conventions 8.5,9,11.3.
The reader is appended to src/text.c and shares its lexical and exact decimal machinery.
The whole input passes precision, length, alphabet, grammar and literal-exponent limits in that order.
The word bound on q precedes semantic checks. A word overflow gives UNSUPPORTED; it cannot wrap to zero.
The unsigned n literal is parsed into fmpz and reduced modulo q, including 2001-bit labels.
q=0 and a nonunit label give DOMAIN. D1 is then checked before decimal-ball conversion and group setup.
The two exact decimal intervals are enclosed by the existing tx_arb_from_real kernel.
The raw acb constructor lowers the pair and commits only after every check succeeds.
No sign condition is imposed on either coordinate of s.

Statuses: OK/PARSE/LIMIT/UNSUPPORTED/DOMAIN, in the design's stage order; all failures preserve x.
The reader cost is input length, integer/decimal conversion and D(q)+D(C).
The printer independently encloses each s coordinate with the unconstrained 9.5 decimal printer.
It emits char(q=q, n=n, s=(re) + (im)*i), allocates with flint_malloc, and reports byte length without NUL.
The caller frees it with adf_str_free. Only the existing print-exponent refusal yields NULL,*len=0.
No modulus cap is applied by printing. Cost is output size and conversion.
Re-reading contains the stored s. Decimal balls need not preserve representation identity (11.3).
Exact fitting dyadic s does round-trip identically in the tests. A general identity test would be false.

Check: all 32 oracle texts, all char.tsv texts, all four exact input endpoints,
canonical lowered fields, non-dyadic decimal s, huge labels, malformed syntax, embedded NUL/high byte,
stage precedence, printer refusal, and read-back containment.

## Integer evaluation

adf_char_chi_phase implements api-3c section 3. It ignores s.
For C=1 every integer, including zero, gives (is_zero,theta)=(0,0).
Otherwise signed fmpz reduction obtains a mod C without narrowing the input.
A nonunit has DIRICHLET_CHI_NULL and commits (1,0); a unit commits (0,k/G->expo).
fmpq_set_ui reduces the rational, whose range is [0,1). Both outputs commit after setup succeeds.
The exponent is the GROUP exponent, not the order returned by get_order.
The declarations/evidence are /usr/include/flint/dirichlet.h:51,139,162 and the design's F1 adapter checks.
The universal FLINT pairing/exponent implementation remains source pending, as api-3c section 3/P3.5 states.

adf_char_chi first checks the precision and conductor caps, then calls the exact phase getter.
It assigns exact zero on the zero branch and otherwise calls adf_phase_get_acb into independent temporary storage.
The phase API encloses E(theta), including exact cardinal roots (psi.h, api-3b numerical phase).
A finite successful temporary is swapped into z. Setup or numerical failure cannot modify z.
Statuses: phase OK/LIMIT/UNSUPPORTED; acb evaluation also propagates phase NOT_DETERMINED.
Cost is D(C), signed integer reduction and one pairing, plus one certified phase evaluation for acb.

Check: 285 primitive pairs C<=40, 40 integers each, negative/zero/multiples/2001-bit values,
F1 at (5,4), a=2, exact cardinal phases at precision 2,53 and the cap, and preserved phase/flag failures.
The primitive pair (65536,5) accepts chi(1), chi(0) and order 16384 at the modulus boundary.
Its complete direct Gauss sum and root number also run at precision 2, with radii and magnitude checks.

## Gauss sum and root number

adf_char_gauss_sum computes tau of the stored primitive character, ignoring s.
The sign is positive, from analysis Lemma 8:295-327 and
refs/src/flint-3.0.1/acb_dirichlet.rst:358-364. It uses no signed quadratic shortcut.
At C=1 it returns exact 1 after both bounds. Otherwise it initializes one group and one character.
For each a in [0,C), the shared exact phase helper obtains chi(a). Nonunits contribute exact zero.
For a unit it adds a/C to the exact phase and reduces modulo 1. Multiplicativity of E identifies
E(theta+a/C) with chi(a) E(+a/C). Calling adf_phase_get_acb once on that combined rational avoids a
second rounded complex multiplication. This is the combined-phase choice fixed in api-3c section 4.

The code adds the certified coordinate endpoints using ARF_PREC_EXACT.
The source for correct arf rounding is refs/src/flint-3.0.1/arf.rst:24-35.
For each coordinate, add l_a<=v_a<=h_a term by term. The sum remains between sum l_a and sum h_a.
These additions introduce no rounding error. P4 bounds each term's radius by (4+2^-27)2^-w.
Here w=min(ADF_REAL_PREC_MAX,max(prec,2)+ceil(log2(C))+8).
Each final midpoint is rounded once at w bits. The radius is the larger exact endpoint distance,
rounded upward to 30 bits and then advanced by one successor, with an exact-zero branch.
General mag conversions may add extra ulps (refs/src/flint-3.0.1/mag.rst:6-17), so this kernel constructs
its mantissa and exponent directly. P4.3 gives radius <7*C*2^-w, hence <=8*C*2^-w.
The implementation also checks this radius bound and finiteness before the only output swap.

adf_char_root_number divides this certified tau by the positive sqrt(C), then rotates by i^-parity.
The formula is analysis Proposition 13:564-580. Enclosing arb sqrt and acb division preserve containment;
the exact rotation exchanges/signs coordinates and introduces no rounding.
The general ball guarantee is refs/src/flint-3.0.1/arb.rst:6-12; acb rectangles are acb.rst:6-18.
The specific operations are arb.rst:951-953 (sqrt), acb.rst:519-523 (division), and
acb.rst:449-451 (division by i).
It certifies both radii <=32*C*2^-w and finiteness before committing W. No sign is assumed for real characters.
The positive-sqrt, division and rotation occur at w bits; their result must pass this explicit certificate.

Statuses for both: precision/conductor LIMIT first; UNSUPPORTED for unavailable setup;
NOT_DETERMINED for numerical certification failure; OK only after the complete enclosure certificate.
Outputs cannot alias chi->s. Every failure leaves the output untouched.
Cost: D(C)+O(C) pairings and phase evaluations, plus exact endpoint additions and final rounding.
Streaming accumulators occupy O(w+log C) bits. Root adds one sqrt/division and exact rotation.
The cap is 65536 for constructors and direct sums, as N-D22 decides.
A later CRT factor algorithm is an alternative; acb_dirichlet.rst:366-367 documents the reference method.
No performance claim for that unimplemented alternative is made here.

Check: 45 oracle Gauss/root pairs through C=64 at 2,53,128 bits; all 17 gauss.tsv rows at 128,
FLINT's default acb_dirichlet_gauss_sum at 256, both P4 radius bounds, |tau|^2=C,
tau(chi)tau(conj chi)=(-1)^e C, |W|=1, W W_conj=1, eight real golden inputs with W=1,
(8,7) yielding exact 2i, principal 1, ignored s, cap certificates and untouched failures.
The inverse label in these finite tests is computed in the test; adf_char_conj is outside slice a.
Injected overwide phase intervals test each Gauss coordinate separately. An overwide positive sqrt interval
tests the root certificate. These are tests of the mandatory final guards, not evidence that the phase API
or arb_sqrt_ui normally produces those overwide intervals.

## User calls and limitations

The commands char, chi and gauss expose these functions through the existing driver parser.
chi accepts an integer after the script separator `with`; the direct CLI also accepts a bare integer argument.
Gauss formats both output balls before writing the line, so printer failure cannot leave a partial result.
The Julia smoke test uses exported layout queries, GC.@preserve, the design's Gauss ccall signature,
and finally blocks that clear every value and free every allocated string.

Check: 17 hand-derived driver lines, direct CLI forms, Julia Gauss/root calls and preserved precision LIMIT.

Not in this slice: conjugation API, unit coset/class/idele evaluation, dump forms, products or CRT summation.
Sources pending: universal FLINT 3.0.1 pairing/exponent implementation and group-init failure semantics.
The signed primitive quadratic Gauss evaluation remains pending in analysis, but is not used by this code.

# Slice b

Implementation of docs/api-3c.md sections 2, 3, 5 (P1-P3) and section 7, slice b.
This slice adds conjugation, unit-coset evaluation and strict lossless character dumps.
It does not change the specification. A unit-coset input has no t coordinate. Its evaluation is chi(u),
independent of the stored s; no t^s factor is applied.

## Unit-coset image and strict certificate

adf_char_eval_ucoset and adf_char_eval_ucoset_strict implement design P1, P2 and CV-07.
Write C for the stored primitive conductor. At N=0, the input is [1] or [-1].
The singleton value is respectively 1 or (-1)^parity. At N>0 put g=gcd(C,N).
The code computes g from N mod C. It never factors N or enumerates lcm(C,N).

Here is the residue argument of P1, stated step by step.

1. A global unit congruent to c modulo N is a unit modulo C and agrees with c modulo g.
2. For a unit a modulo C agreeing with c modulo g, the two congruences x=a mod C and x=c mod N
   are compatible. Solve them by CRT at lcm(C,N). At each prime dividing that lcm, at least one
   supplied residue is a unit. The solution is a unit at that prime. Choose unit coordinates elsewhere.
   It belongs to c U(N) and has residue a modulo C.
3. Thus the exact set is the phases chi(a) for units a modulo C with a=c modulo g.
   It never contains zero. The printed representative c itself can be a nonunit modulo C.
   In particular (C,n,c,N)=(3,2,3,4) gives {1,-1}, although the zero extension has chi(3)=0.
4. Choose a0 from that residue set. Multiplication by a0 identifies the set with
   H={h in (Z/C)^x: h=1 modulo g}. Its character image is chi(a0)chi(H).
   A compatible a0 modulo C extends to a unit lift modulo lcm(C,N) by step 2.
5. If chi(H) is trivial, chi descends through the surjective reduction to (Z/g)^x.
   Primitivity forces g=C. Conversely g=C makes H trivial. Therefore the image is a singleton
   exactly when C divides N, or when N=0. This proves the strict certificate.

The strict call checks precision and C bounds first, then INV, then the singleton criterion.
An ambiguous input gives NOT_DETERMINED before group setup or phase evaluation in a normal build.
INV may construct a group while checking the character predicate. It never performs trig work here.
A singleton delegates to the existing integer evaluator; cardinal phases stay exact.
The default ambiguous call shares one initialized FLINT group and character throughout its residue scan.
It reuses slice a's exact char_phase helper, including the group-exponent convention of F1.

Check: 4972 oracle cosets for all 108 primitive pairs C<=24, plus the asymmetric (27,2,2,9) image;
each exact phase, raw zero-extension flag,
singleton criterion and strict failure bytes; negative and 2001-bit representatives; N=0 and N=360;
F2; huge singleton and ambiguous moduli; C=65536 and C=65537; independence from s.

## The four hull extrema and rounding

For each phase theta in the scanned finite set, its circle distance to t is
min(frac(t-theta),1-frac(t-theta)). Minimize that exact rational separately at
t=0,1/2,1/4,3/4. These four distances are attained. This implements P2 by scanning the finite set
directly, rather than computing its subgroup order. The two methods give the same four distances.

Cosine decreases with circle distance from 0 on [0,1/2]. The real maximum is cos(2 pi d(0)).
Distance from 0 and distance from 1/2 add to 1/2, so the real minimum is -cos(2 pi d(1/2)).
Sine is cosine shifted by 1/4. Hence the imaginary interval is
[-cos(2 pi d(3/4)),cos(2 pi d(1/4))]. Each endpoint is attained by an input phase.
This proves that the resulting rectangle is the exact rectangular hull before numerical rounding.
It is Q4 with r=0. A square would fail the F2 hull, whose imaginary coordinate is exactly zero.

Each cosine uses the real part of adf_phase_get_acb at min(p+32,ADF_REAL_PREC_MAX), p=max(prec,2).
Refine by doubling to the cap until that real ball has width at most 2^-p.
The phase contract is psi.h; its trig source is refs/src/flint-3.0.1/arb.rst:1125-1138.
The upper endpoint is a certified upper bound for that cosine. Clip it to 1, since cosine is at most 1.
Negate the upper bound for each coordinate's lower extremum. The four resulting bounds enclose every value.

Reuse slice a's char_round, the Q1 midpoint/RU30-successor kernel, once per coordinate.
The exact endpoint arithmetic and directed rounding source is refs/src/flint-3.0.1/arf.rst:24-35.
The nonzero radius construction avoids the extra ulps permitted by general mag conversion
(refs/src/flint-3.0.1/mag.rst:6-17). Let epsilon=2^-p and W be the true coordinate width.
The outward cosine error is at most epsilon on each endpoint. Midpoint rounding and the radius
successor satisfy Q1's bound, yielding the design allowance
4 epsilon + 2^-28*(W/2+2 epsilon) for each endpoint. Exact-zero coordinates retain zero radius.
The finite temporary is swapped into z only after all four certificates succeed.

Statuses: OK/LIMIT/UNSUPPORTED/NOT_DETERMINED. LIMIT for precision or C>65536 precedes INV,
allocation and strict ambiguity. Setup failure is UNSUPPORTED. Phase statuses propagate.
Failure to certify numerical width or finiteness gives NOT_DETERMINED. Every failure preserves z.
Output cannot be chi->s. INV checks both the character and unit-coset predicates.
Cost is D(C)+O(C) pairings and four exact distance comparisons per compatible residue,
followed by four phase evaluations and two roundings. Storage is constant in the number of residues,
apart from precision-dependent ball/rational storage. There is no retained context or cache.

Check: all 23 shared oracle hulls, all their phases and all four 60-digit certified extrema,
on every coset at p=2,53,128; both line orientations and the four cardinal roots;
the quantitative endpoint allowance; exact cardinal values at the cap; failure snapshots.
Wrapped checks inject setup failure, phase NOT_DETERMINED/LIMIT and two overwide cosine certificates.
The finer overwide case has width 1.5*2^-53; the required width test rejects it even though the
phase precision was increased. A separate injection checks the shared nonfinite root-result guard.

## Conjugation and the remaining source obligation

adf_char_conj implements the brief's exponent-negation fallback. It keeps conductor and parity,
negates each cyclic log modulo its component order and exactly conjugates s. y=x is allowed.
The local header /usr/include/flint/dirichlet.h:84 states
"s.t. prod generators[k]^log[k] = number". Line 36 stores phi(p^e) in the component's nmod_t.
Lines 114 and 123 declare dirichlet_char_log and _dirichlet_char_exp.
Lines 116-120 show that public dirichlet_char_exp only returns the cached x->n.
After editing logs, the implementation therefore uses _dirichlet_char_exp to rebuild that cached number.
Calling the public inline alone would return the old label.

The group representation proves the inverse-number part without assuming a symmetric pairing:

1. Let n=product_j generators[j]^a_j, where each generator has order d_j.
2. Negating a_j modulo d_j gives a product n' with n n'=1 in (Z/C)^x.
3. Thus n'=n^-1 modulo C. At C=1 the canonical label remains 1.
4. If the label-to-character map is a homomorphism, it takes n^-1 to chi(n)^-1.
   A finite character has roots of unity as values, so inverse and complex conjugate coincide.
5. For a real positive t, conjugating exp(s log(t)) conjugates s because log(t) is real.
   The kernels of chi and its inverse coincide, so conductor and parity are preserved.

Step 4's identification with FLINT remains [source pending: universal FLINT Conrey pairing definition].
refs/src/flint-3.0.1/acb_dirichlet.rst:337-342 names the pairing but does not state symmetry or its
cyclic exponent formula. Its :466 names a "Conrey isomorphism" but supplies no definition of that map.
The header comments do not supply that formula either. This is not marked as a universal proof of P3.
The fallback is tested with exact phases for all 1966 characters through modulus 80 and every unit there.
All 1206 primitive pairs through conductor 80 also match the oracle's inverse labels.

HEADER-FINDING: the design's void conjugation declaration promises extended gcd plus exact acb copy.
The explicitly required source-pending fallback instead needs D(C) setup and discrete logarithms.
It has no D1 cap, since the design forbids one on this void operation. Setup failure has no status slot,
so it fail-stops before any write, as the slice a predicate already does under its conditional finding.
This is a cost/failure-contract discrepancy, not evidence that a valid word-sized setup can fail.
No change to the fixed struct or ABI resolves that missing source obligation here.

Check: inverse labels, preserved parity/conductor, s midpoint and radius conjugation, involution,
self-aliasing, and 74434 exact phase products equalling 1. INV checks both arguments.

## Strict dumps, loading and inspection

The four typed functions implement api-3c section 2 and conventions 10.1:1414, 10.2:1427-1480.
The body is char q n acb, with no contexts and no stored parity token.
The shared dump validator handles the entire body in its existing stage order.
After grammar, limits and q word support, it checks q>=1, 1<=n<=q, gcd=1 and both canonical finite balls.
These cheap semantics precede D1. Label 1 is principal, as dirichlet.h:144-147 states, so q>1,n=1
is also cheaply imprimitive and gives DOMAIN without setup, even above D1. The principal character
has conductor 1: it descends to the one-element group modulo 1, and no smaller positive conductor exists.
A valid pair above 65536 gives LIMIT before group setup; the boundary test uses primitive (65537,3).
It then verifies conductor=q and derives parity. An imprimitive input gives DOMAIN; it is never lowered.
The label n outside the word range gives DOMAIN at the semantic stage, since n<=q already fails.
This is the design's q-only word-support rule; no lexical length cap is substituted for D1.

The loader then validates the binding count. nbinds must be 0. The one-context argument is ignored.
The balls are reconstructed exactly from validated dyadic fields using the existing dp_set_arb kernel.
No arb_load_str is called. Long validated hexadecimal integers are copied before fmpz_set_str parses them.
Every raw byte has passed validation first.
Only the complete temporary is swapped into x. Dumping writes the stored fields and exact ball bits.
The inspector runs the same validator, writes nctx=0 only on OK and never touches descriptors.
It builds no adf_char value. The existing inspector's NULL nctx result remains DOMAIN.

Statuses: OK/PARSE/LIMIT/UNSUPPORTED/DOMAIN, with all outputs preserved on failure.
Cost: input/output size plus D(q) for validation; no working precision. Dump strings are FLINT allocated,
NUL terminated and freed by adf_str_free. Their reported length excludes the NUL.
Round trips are byte-identical when caller limits admit them. Parity is recomputed, rather than trusted.

Check: 30 exact/radius character texts; both imprimitive witnesses (8,1) and (16,9);
all eight existing char golden rows; complete dump/load/dump identity, nctx=0 and untouched descriptors;
truncations, NUL/high bytes, bad literals/balls, q=0, word overflow, D1 boundaries and binding failures.
The test interposes arb_load_str and aborts if it is called.

## User calls

char_unit and char_unit_strict expose the design's slice b driver calls.
char_conj prints the conjugate. Generic dump/load dispatch uses the typed character functions.
The two driver fixtures have 24 hand-derived output lines, including F2 and imprimitive rejection.
Generic character arithmetic remains UNSUPPORTED; products are deferred by the design.
Julia allocates through layout queries and calls the design's adf_char_eval_ucoset ABI.
It checks all cardinal roots, a preserved strict failure, conjugation and lossless dump loading.

Check: tests/driver/char-eval.cmd, tests/driver/char-dump.cmd and tests/julia/char_eval.jl.
Class and idele evaluation, character products and the universal P3 source remain outside this slice.

# Slice c

The four functions implement docs/api-3c.md section 3 and slice c of section 7.
The value remains t^s chi(u'), as SPEC 5:377-384 and conventions 5.13, CV-58 require.
There is no hidden conjugation. The stored s ball describes a family of quasi-characters.

## Class evaluation and the real power

adf_char_eval_idclass and adf_char_eval_idclass_strict call the slice b unit-hull helper.
P1 identifies the complete finite phase set; P2 certifies its four coordinate extrema.
The functions then multiply that hull by a ball enclosing t^s, using independent temporaries.
Strict has the same finite-coset certificate, N=0 or C|N. Radii in t and s are allowed.
It does not decide uniqueness from t, from s, or from the width of a numerical enclosure.

The positive real branch is fixed by the class invariant, conventions 5.7:625-640.
Every point of t is positive, so log(t) is real. For every s=a+bi in the stored rectangle,
exp(s log(t)) has modulus exp(a log(t)) and phase b log(t). Thus the complex power introduces
neither a sign correction nor a conjugation. Those belong to other parts of the formula.

FLINT's contract is on disk. refs/src/flint-3.0.1/arb.rst:6-12 says that an approximate operation
returns a ball containing the exact result for any choice of points in the input balls.
arb.rst:1034-1040 defines arb_pow with integer and half-integer shortcuts and, generally,
"as z = exp(y log x)". acb.rst:637-643 gives the same formula for acb_pow.
acb.rst:6-18 describes independent real and imaginary balls, hence rectangular enclosures.
acb.rst:675-679 fixes the logarithm's principal branch; acb.rst:382-389 gives arg(a+bi)=atan2(b,a).
On the strictly positive real axis that argument is zero. acb.rst:463-468 specifies multiplication
and its reduction to real operations when a component is zero.
acb_pow_arb takes a complex base and a real exponent; it does not mean a real base to a complex exponent.

The implementation follows the design's fixed choice: when Im(s) is exactly zero, use arb_pow
and embed its result with exactly zero imaginary part. Otherwise embed t as a real acb and call acb_pow.
This also preserves the integer-power shortcut: at exact t=2 and s=1 the real path gives exact 2.
The design example therefore gives exact -2 here. The public contract promises enclosure, not general exactness.
s=0 and exact t=1 give exact power 1. No exponent or midpoint is narrowed to a machine floating-point number.

Containment follows stepwise. For each represented input, chi(v) belongs to the P1/P2 unit rectangle.
The FLINT power contains exp(s log(t)) for every represented t and s on this positive axis.
Rectangular multiplication therefore contains each product. Finiteness of the power and final product
is checked before swapping the result into z. The imaginary component stays exactly zero in real line cases.

The rectangle product need not be the rectangular hull of the finite value set.
For the fourth roots, multiplication of their square by R exp(i theta) gives coordinate widths
2R(|cos(theta)|+|sin(theta)|); the rotated four-point set has widths
2R max(|cos(theta)|,|sin(theta)|). The excess is 2R min(|cos(theta)|,|sin(theta)|).
For t=2,s=2+3i, R=4 and theta=3 log(2), this is positive and does not disappear with precision.
This is the design's prescribed interval multiplication, not a failure of enclosure or of P2.

The test endpoint allowance over the high-precision rectangle product is
256*M*2^-p + 2^-24*(W+M*2^-p), M=max(1, endpoint magnitudes), W its coordinate width.
Against the exact point-value hull, add the explicitly computed rectangle overhang.
The 60-decimal intervals are outward-rounded certified endpoints. Radius-bearing rows include
corner/midpoint witnesses and whole-family rectangles per phase. Their four exact hull extrema
are certified by the following finite-candidate argument, implemented in family_hull.py.

1. Put l=log(t), L=log(t_low), H=log(t_high), and phi=2*pi*phase(chi(v)).
   A coordinate is exp(a*l) cos(phi+b*l) or exp(a*l) sin(phi+b*l), with a and b in closed intervals.
   For fixed l,b it is monotone in a, or constant. Thus every extremum occurs at an endpoint of a.
2. For fixed a,l, extrema as b varies occur at an endpoint of b or a cardinal phase of the coordinate.
   This follows by differentiating sine/cosine on the finite phase interval.
3. On an endpoint b!=0, differentiate in l. The real derivative is
   exp(a*l)*(a*cos(phi+b*l)-b*sin(phi+b*l)); its zeros have phase atan2(a,b)+k*pi.
   The imaginary derivative gives phase atan2(-b,a)+k*pi. Evaluate each root in [L,H] and both endpoints.
   For b=0 the coordinate is a constant times exp(a*l), so endpoints suffice.
4. For an interior cardinal phase theta, feasibility is b0*l<=theta-phi<=b1*l when l>=0,
   with b0,b1 exchanged when l<=0. Split [L,H] at zero. The two inequalities cut out a closed interval.
   The coordinate there is +/-exp(a*l), which is monotone or constant. Its extrema occur at interval ends.
5. These cases exhaust the boundary and interior extrema on a compact parameter box.
   An exact rational bound on |phi+b*l|, using certified pi/log endpoints, bounds the finite k enumeration.
   Evaluation uses FLINT at 512 bits. Uncertain feasibility contributes only an outer bound.
   The inner endpoint uses a certified attained witness. Min/max of these dyadic intervals brackets each
   exact hull extremum; outward rounding gives an interval at most two units wide on the 10^-60 grid.
   The generator asserts that width for every continuous-family extremum.

These derivative steps are our own proof. The atan2 contract used to evaluate the stationary phases is
refs/src/flint-3.0.1/arb.rst:1187-1192; exp/log and ball enclosure use the contracts already cited above.

Check: all 2172 class rows, all their finite phases, five exact exponents, three radius-bearing exponents,
positive t balls, exact units +/-1, C|N, ambiguous cosets and F2's nonunit printed representative.
Every value interval is contained; every rectangle endpoint satisfies the stated allowance at p=2,53,128.
Line cases reject a square. Conjugation is checked on the entire witness value set through slice b's conj.

## Idele evaluation and the sign

adf_char_eval_idele and adf_char_eval_idele_strict first call adf_idclass_set_idele.
Ideles P15:368-394 proves that this supplies t=|x_inf|/r and u'=sign(x_inf)u.
The map is multiplicative, has kernel Q^x, and supplies the unique positive representative of each class.
For a diagonal -1 both real and finite signs are negative, so u'=1 and the character value is 1.
For (-2;1*[1]) only the real sign is negative. The odd character (3,2) gives chi(-1)=-1,
so t^1 chi(u')=2*(-1)=-2. The code applies no further sign after conversion.

The conversion is performed at the requested precision before evaluating the temporary class.
At identical precision and input, idele evaluation returns a rectangle identical to an explicit call
to adf_idclass_set_idele followed by the corresponding class evaluator. It may be wider than
evaluation of an exact mathematical norm because conversion rounds the real ball (idclass.h:100-115).
Negating only x_inf multiplies the value set by chi(-1)=(-1)^parity; it is not diagonal negation.

Check: all 128 idele rows, both real signs, content 1,2,1/3,6/5, exact and radius-bearing x_inf,
the explicit-conversion identity, odd/even sign identities, diagonal -1 and the exact design example.

## Statuses, aliasing and cost

All four calls preflight prec>ADF_REAL_PREC_MAX and C>ADF_CHAR_MOD_MAX before INV or allocations.
Either bound gives LIMIT, even when the finite coset is ambiguous or raw storage would fail INV.
Class strict ambiguity gives NOT_DETERMINED before power evaluation.
Setup failures propagate UNSUPPORTED. Phase LIMIT propagates before any dependent power work.
Nonfinite powers or products give NOT_DETERMINED. Idele conversion failure stops class evaluation.
Every failure preserves all output bytes. Each successful path has one final acb swap.

A raw t meeting zero or negative values is rejected by adf_idclass_set_parts with DOMAIN.
Such a class is not an evaluation input. INV aborts on its invalid storage; ordinary evaluation
has no promised status for invalid storage. This is the design's positive-ball distinction in section 3.
Valid idele conversion can fail to preserve positivity at low precision and gives NOT_DETERMINED.
No raw-real evaluator or additional complex branch was introduced.

Output member aliasing with either input is forbidden. INV tests both arb components of z against
both components of chi->s and the class t or idele inf member. No same-type whole-object alias exists
between these heterogeneous inputs and the acb output. Call-local objects are initialized and cleared.

Class cost is the unit call, one power and one multiplication. Idele adds class conversion.
The unit cost remains D(C)+O(C) exact work for an ambiguous default coset and four phase evaluations,
with streaming storage. There is no factorization of N and no enumeration modulo lcm(C,N).
The implementation retains separate base/power/result temporaries and repeats INV predicate setup.
These are avoidable costs, not an optimized algorithm. No new design decision was needed.

Check: strict output snapshots, precision 2,53,cap and cap+1, conductor cap and cap+1,
constructor DOMAIN snapshots, valid conversion failure, injected setup/phase/power/product/conversion failures,
zero dependent calls after conversion failure, all 32 INV children and FLINT allocation balance over 100 cycles.
LeakSanitizer availability and mutation results are reported separately in lanes/c-slice3/report.md.

## User calls and limits of evidence

char_eval C with X and char_eval_strict C with X accept an idele or class text X.
Both print the usual complex ball, including the design example -2. The 21 expected fixture lines
were derived by hand before running the driver. Julia allocates using exported layout queries,
retains every storage owner with GC.@preserve and calls the design's adf_char_eval_idele ABI.

Check: tests/driver/char-class.cmd and tests/julia/char_class.jl.
The six required scratch faults concern sign, exponent conjugation, content, strictness,
one-corner multiplication and early output writes. Their actual exits are in the lane report.
The inherited universal FLINT Conrey pairing/source gaps remain pending. This slice adds no such dependency.
There is no claim of long fuzzing or unbounded mutation coverage.
