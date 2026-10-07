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
