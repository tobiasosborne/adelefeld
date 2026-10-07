# Milestone 3.3 and 3.4: characters and Gauss sums

Design before code. No change to SPEC, the struct, or CV-07, CV-58, CV-60. Decision D1 below is a proposal.
References to analysis mean docs/proofs/analysis.md; ideles means docs/proofs/ideles.md.
The executable oracle is proto/char_checks.py. It imports no adelefeld implementation.
Python-flint 0.8.0 uses FLINT 3.3.1 here. Its exact phases/lowering are cross-checked against a C adapter
whose compiled and runtime FLINT versions must both be 3.0.1, for every character of moduli 1..80.

## 1. Type, meaning, and common contract

Conventions:785-810 fixes the following layout; the lane's C probe measures the stated 64-bit ABI.

```c
typedef struct { ulong q; ulong n; int parity; acb_t s; } adf_char_struct;
typedef adf_char_struct adf_char_t[1];
typedef adf_char_struct *adf_char_ptr;
typedef const adf_char_struct *adf_char_srcptr;
/* sizeof=120, alignof=8; offsets q=0, n=8, parity=16, s=24. No hidden cache or context. */
```

Predicate: q>=1, 1<=n<=q, gcd(n,q)=1; the FLINT character has conductor q, parity equals the stored
0 or 1, and both components of s are finite. Init is (1,1,0,0). A ball S represents the family
{(t,u') -> exp(z log(t)) chi(u') : z in S}, t>0, where log(t) is real. Each member is a character;
an uncertain S is a family, not one homomorphism. Evaluation encloses the union over z and input points.
SPEC:377-384 and CV-58 fix this meaning. The Tate-integral character is a different member, as proved
in analysis P11:454-477; no evaluation below silently conjugates chi.

All value inputs and outputs are initialized. Inputs are canonical, except raw constructor data and
is_canonical. CV-09 requires INV entry checks; is_canonical never aborts on initialized valid pointers.
Whole-object aliasing is allowed for same-type arguments. Member aliasing and overlapping distinct outputs
are forbidden. All status failures leave every output untouched, including flags and allocated-string slots.
Numerical calls first return LIMIT for prec>ADF_REAL_PREC_MAX=2097152, before INV checks or allocations;
p=max(prec,2). Intermediate nonfinite balls give NOT_DETERMINED and are never committed (CV-08).
Characters use conventions:223's character status row; parser and loader statuses use their own rows.

D1 bounds FLINT setup to q<=65536 in status-returning character operations, and direct sums to C<=65536.
The bound is an algorithm limit, not a new predicate. Copy, swap, printing, and simple field getters do not
use it. is_canonical must test the full word-sized predicate, even above this bound; it may be expensive.
Denote by D(q) the cost of FLINT group setup, factorization and the needed discrete logarithms. Do not
call this O(log q). Bounded numerical calls test q before INV, setup, and coset enumeration.

```c
/* Init the principal character, s=0. No allocation except acb initialization; cannot fail. */
void adf_char_init(adf_char_t x);
/* Clear owned s; afterwards only init is permitted. Cannot fail. */
void adf_char_clear(adf_char_t x);
/* Exact copy of all fields; y may be x. Cost: copying s. */
void adf_char_set(adf_char_t y, const adf_char_t x);
/* Exchange all fields; O(1), no allocation; x may be y. */
void adf_char_swap(adf_char_t x, adf_char_t y);
/* Predicate above, including primitive conductor and parity; no resource-status shortcut.
   Validate integer ranges and finite s before calling FLINT. Cost D(q), not bounded by D1. */
int adf_char_is_canonical(const adf_char_t x);
/* Representation identity: integer fields equal and acb_equal(sx,sy). Cost of comparing s. */
int adf_char_identical(const adf_char_t x, const adf_char_t y);
/* Header-inline AND exported functions; bindings allocate using these, not guessed offsets. */
size_t adf_sizeof_char(void);
size_t adf_alignof_char(void);
```

## 2. Construction, access, conjugation, and text

The header /usr/include/flint/dirichlet.h declares group_init at 68, char_log at 114,
conductor_char at 111, parity_char at 110, order_char at 112, char_lower at 136. Its char struct at
83-84 holds n and logs; char_exp at 116-120 returns x->n. Relevant declarations, quoted with line wrapping:

```c
int dirichlet_group_init(dirichlet_group_t G, ulong q);
void dirichlet_char_log(dirichlet_char_t x, const dirichlet_group_t G, ulong m);
ulong dirichlet_conductor_char(const dirichlet_group_t G, const dirichlet_char_t x);
int dirichlet_parity_char(const dirichlet_group_t G, const dirichlet_char_t x);
ulong dirichlet_order_char(const dirichlet_group_t G, const dirichlet_char_t x);
void dirichlet_char_lower(dirichlet_char_t y, const dirichlet_group_t H,
                          const dirichlet_char_t x, const dirichlet_group_t G);
ulong dirichlet_chi(const dirichlet_group_t G, const dirichlet_char_t chi, ulong n); /* :162 */
```

The lowering sequence is: initialize G(q),
log the reduced n into x; compute C; initialize H(C), initialize y in H, lower(y,H,x,G), and read
char_exp(H,y). Recompute parity/order in H, release both groups and characters on every exit.
C=1 is stored as n=1, parity=0, order=1, without asking for a discrete logarithm modulo 1.
Never use n mod C. P3 proves the exact matching oracle and explains the conjugation source gap.

```c
/* Set s=0 and the primitive character inducing (q,n). Reduce n modulo q; q=1 accepts
   every n, including 0, and stores (1,1). q=0 or gcd(n,q)>1: DOMAIN; in particular
   n=0 is DOMAIN for q>1. D1: q>65536 gives LIMIT before setup, after raw domain checks.
   OK commits all fields. Cost D(q)+D(C). A failed FLINT setup gives UNSUPPORTED (D1).
   q>=2^64 cannot be passed through ulong: callers must range-check before casting. */
int adf_char_set_conrey(adf_char_t x, ulong q, ulong n);
/* Same lowering, copying the supplied finite s exactly; no precision or rounding.
   Nonfinite s: DOMAIN before setup. s must not alias x->s. Other statuses/cost as above. */
int adf_char_set_conrey_acb(adf_char_t x, ulong q, ulong n, const acb_t s);
/* Replace s by a finite exact copy. OK or DOMAIN for nonfinite s; no q setup or D1 limit.
   The raw s must not be a member of x. Cost: copy. */
int adf_char_set_s(adf_char_t x, const acb_t s);
/* Exact scalar field reads; no allocation or status. */
ulong adf_char_get_conductor(const adf_char_t x);
ulong adf_char_get_label(const adf_char_t x);
int adf_char_get_parity(const adf_char_t x);
/* Copy s into an independent acb. No status; cost: copy. */
void adf_char_get_s(acb_t s, const adf_char_t x);
/* Write multiplicative order of chi, not G->expo. OK; LIMIT for D1;
   UNSUPPORTED for unavailable FLINT setup. Cost D(q); order untouched on failure. */
int adf_char_get_order(ulong *order, const adf_char_t x);
/* Pointwise complex conjugate of the family: (q,n^-1 mod q,parity,conj(s)).
   q=1 retains n=1. y may be x. No status; extended gcd and an exact acb conjugation.
   Conjugation preserves conductor: the kernels of chi and conj(chi) coincide. See P3. */
void adf_char_conj(adf_char_t y, const adf_char_t x);
```

Product is deferred: neither PLAN row requires it. Its eventual algorithm lifts both finite characters to
lcm(C1,C2), multiplies there, lowers, and adds the s balls. The lcm can exceed a word even when both inputs
fit; overflow and setup budgets deserve their own callable slice. No adf_char_mul is promised here.

```c
/* text.h: read char(q=q, n=n, s=z(s)), conventions:1264. Apply parser stages 8.5;
   prec cap first, then length/alphabet/grammar/literal limits, word bound on q, semantics.
   q>=2^64: UNSUPPORTED, not a wrapped ulong; arbitrarily long n is reduced as fmpz modulo q.
   Semantics and D1 as constructors; preserve the parser's stage precedence. Lower (q,n).
   OK/PARSE/LIMIT/UNSUPPORTED/DOMAIN; no sign condition on s. Every failure preserves x.
   Cost text length, decimal conversion and D(q)+D(C); no member aliasing. */
int adf_char_set_str(adf_char_t x, const char *s, size_t len, slong prec,
                     const adf_text_limits_t *lim);
/* Canonical value text, conventions 9.5 independently on re(s),im(s). digits 1..10^6.
   flint-allocated result, byte length excluding NUL; free with adf_str_free. NULL and
   *len=0 only for the existing ADF_PRINT_EXP_MAX refusal. Cost output size/conversion.
   Parsed s encloses the input; no C print-read-print identity claim (11.3:1530-1534). */
char *adf_char_get_str(size_t *len, const adf_char_t x, slong digits);
/* dump.h: STRICT lossless body already defined at conventions:1414: char h h acb (q n s).
   Prefix adf1 Q. Recompute parity; reject imprimitive input rather than lower it.
   Validate all tokens and ball fields before any FLINT loader (10.2:1430-1446).
   OK/PARSE/LIMIT/UNSUPPORTED/DOMAIN; word bound at stage 5. D1 follows cheap semantic
   validation and precedes any FLINT setup; it is not a limit on the lexical integer length.
   No contexts occur: ctx ignored; binds form requires nbinds=0, else DOMAIN.
   Cost input size and D(q); output unchanged on failure. No working precision. */
int adf_char_load_str(adf_char_t x, const char *s, size_t len,
                      const adf_modctx_struct *ctx, const adf_text_limits_t *lim);
int adf_char_load_str_binds(adf_char_t x, const char *s, size_t len,
                            const adf_modctx_struct *const *binds, size_t nbinds,
                            const adf_text_limits_t *lim);
/* Exact stored bits; allocated string/free/length as get_str. No failure, cost output size.
   Round trip is identical when the reader's limits admit it; parity is a derived field. */
char *adf_char_dump_str(size_t *len, const adf_char_t x);
/* Validate the same strict body, write nctx=0 on OK; descs unchanged. With descs=NULL
   ignore incoming nctx, otherwise treat it as capacity. Loader statuses/cost, no value built. */
int adf_char_dump_inspect(size_t *nctx, adf_ctx_desc_t *descs, const char *s, size_t len,
                          const adf_text_limits_t *lim);
```

No dump-body decision is needed. The existing generic dump validator marks char as not implemented
(src/dump.c:1102 onward); slice c must add semantic validation, not merely expose typed entry points.

## 3. Evaluation

Use zero plus a rational phase, not a new owning type. /usr/include/flint/dirichlet.h:139 defines
DIRICHLET_CHI_NULL; :162 declares dirichlet_chi. Header :51 calls G->expo the group exponent.
Observed and tested on every adapter row: its non-null k means E(k/G->expo), NOT E(k/ord(chi)).
Normalize that rational into [0,1). If an order-based integer is wanted, k*ord(chi)/G->expo is integral.
The oracle's chi_exponent returns this rescaled integer; no machine-word product is needed in C.
The full exponent convention remains [source pending: FLINT 3.0.1 dirichlet_chi source under refs/].

```c
/* chi(a), ignoring s: OK writes is_zero=1 and theta=0 for a non-unit; otherwise
   is_zero=0 and theta in [0,1) with chi(a)=E(theta). theta=0 alone means 1, never zero.
   At C=1 all integers, including 0, give (0,0). Reduce arbitrary signed fmpz a modulo C.
   LIMIT for D1; UNSUPPORTED for FLINT setup. Exact work, cost D(C)+reduction+one pairing.
   Both outputs commit together; different outputs and input members never alias. */
int adf_char_chi_phase(int *is_zero, fmpq_t theta, const adf_char_t chi, const fmpz_t a);
/* Same value in acb: zero branch exact; otherwise adf_phase_get_acb(theta,prec).
   OK/LIMIT/UNSUPPORTED and NOT_DETERMINED from phase certification; precision cap first.
   Cost previous call plus one phase evaluation. Output cannot alias chi->s. */
int adf_char_chi(acb_t z, const adf_char_t chi, const fmpz_t a, slong prec);
/* chi on the entire unit coset, ignoring s. P1/P2 give its rectangular hull; construct it
   with Q4's numerical allowance below. Default returns OK even for a multiple-value image.
   Strict requires N=0 or C|N; otherwise NOT_DETERMINED, without a write or trig work.
   Both: LIMIT for prec or D1 before ambiguity; UNSUPPORTED for FLINT setup;
   NOT_DETERMINED for a failed numerical certificate. Cost D(C)+O(C) exact pairings
   in the ambiguous default case, four phase evaluations, O(1) streaming storage.
   N is fmpz: compute gcd(N mod C,C), never factor N or enumerate lcm(C,N). */
int adf_char_eval_ucoset(acb_t z, const adf_char_t chi, const adf_ucoset_t u, slong prec);
int adf_char_eval_ucoset_strict(acb_t z, const adf_char_t chi, const adf_ucoset_t u, slong prec);
/* Enclose {exp(s log(t)) chi(v): s in chi->s, t in x->t, v in x->u}.
   Form the unit hull above, multiply by the certified ball for t^s. Strict has exactly
   the same finite-coset certificate; it allows uncertainty in t and s. All statuses above.
   Cost unit call plus one power and multiplication. No alias with either input's members. */
int adf_char_eval_idclass(acb_t z, const adf_char_t chi, const adf_idclass_t x, slong prec);
int adf_char_eval_idclass_strict(acb_t z, const adf_char_t chi, const adf_idclass_t x, slong prec);
/* Through adf_idclass_set_idele, then the preceding call, into temporaries. The map supplies
   t=|x_inf|/r and u'=sign(x_inf)u (ideles P15:368-394); do not apply the sign twice.
   Statuses above plus NOT_DETERMINED from class conversion; same untouched-output rule.
   Preflight precision and C before conversion; conversion failure stops dependent work.
   Cost conversion plus class evaluation; enclosure may widen during conversion. */
int adf_char_eval_idele(acb_t z, const adf_char_t chi, const adf_idele_t x, slong prec);
int adf_char_eval_idele_strict(acb_t z, const adf_char_t chi, const adf_idele_t x, slong prec);
```

P2 gives four exact distances. Reuse the Q4 r=0 algorithm of api-3b.md and its certified cosine/rounding
kernel. Alternatively obtain each cosine as the real component of adf_phase_get_acb(distance), refining
until its width<=2^-p, then apply that same kernel. For true coordinate width W, endpoint excess is at most
4*2^-p + 2^-28*(W/2+2*2^-p), as in psi.h. Enclosure alone is insufficient; the unit square fails line cases.
Singleton evaluation uses adf_phase_get_acb directly, preserving its exact cardinal phases.

For t^s, use arb_pow when Im(s) is exactly zero, and acb_pow with the positive-real t otherwise.
There is no arb_pow_arb in FLINT 3.0.1; source: refs/src/flint-3.0.1/arb.rst:1034-1039 and
acb.rst:637-643. Embed a real result with exactly zero imaginary part. s=0 and t=1 permit exact 1.
A canonical idclass has t strictly positive. A t ball containing zero is not an evaluation input:
adf_idclass_set_parts rejects it with DOMAIN, including mixed-sign balls, as a constructor invariant error.
No evaluation branch takes a logarithm of such a raw ball. INV aborts on that precondition violation;
ordinary evaluation has no promised status for invalid storage. A class conversion unable to certify
positivity returns NOT_DETERMINED. There is no new public raw-t evaluator or arbitrary complex branch.
Independent failures combine by maximum: check all preflight bounds before strict ambiguity; after that,
all numerical failures are NOT_DETERMINED except propagated phase LIMIT, which takes precedence.

## 4. Gauss sum and root number

CV-60 and analysis L8:297-326 fix tau=sum_(a mod C) chi(a) E(+a/C), with chi=1 everywhere for C=1.
refs/src/flint-3.0.1/acb_dirichlet.rst:358-362 defines the same positive sign; :364 is the direct sum,
:366-367 the CRT factor method, :369-370 assumes real AND primitive for order2, :372-376 warns that
theta assumes primitive and may abort, :378 selects methods automatically. Use the default as a test
reference at these small conductors, not the theta entry point. No helper named just "Fourier kernel".

```c
/* Enclose tau of the STORED primitive chi; ignore s. OK writes tau; prec and C bounds
   give LIMIT before setup; UNSUPPORTED for FLINT setup; NOT_DETERMINED for failed phase
   certificates. C=1 returns exact 1 after bounds. All failures preserve tau.
   Algorithm and radius certificate P4 below. O(C) phase evaluations and additions,
   plus D(C) and O(C) pairings; streaming temporaries. tau must not alias chi->s. */
int adf_char_gauss_sum(acb_t tau, const adf_char_t chi, slong prec);
/* W=tau/(i^parity sqrt(C)), positive sqrt. Independent of s. Same statuses, bounds,
   aliasing and commit rule; cost Gauss sum plus sqrt, division and exact rotation.
   Require both output coordinate radii <=32*C*2^-w with w from P4; otherwise
   NOT_DETERMINED. No shortcut W=1 for real characters while its source is pending. */
int adf_char_root_number(acb_t W, const adf_char_t chi, slong prec);
```

At w=min(ADF_REAL_PREC_MAX,p+ceil(log2(C))+8), combine the two exact phases theta_chi+a/C modulo 1
and call adf_phase_get_acb once per unit a; non-units contribute exact zero. Combining angles avoids
an extra rounded product and still uses the phase API for both chi and E(+a/C). The integer phase getter
must share one initialized FLINT group within this loop; do not rebuild it C times. Accumulate coordinate
endpoints as exact dyadics, then round once as in P4. This is a direct C-term sum, not a Gauss shortcut.
The later CRT factor method replaces C-sized summation by local prime-power sums and factors; it can
benefit composite conductors and would justify a different cap. No unproved speed claim at prime C.

## 5. Statements to add to analysis.md

P1. Exact image of a unit coset under a primitive character of conductor C, order o.
1. If N=0, c=+/-1 and the only phase is chi(c). Otherwise put g=gcd(C,N). The possible residues
   modulo C are V={a mod C: gcd(a,C)=1, a=c mod g}. Any global unit in the coset belongs to V.
2. Conversely, for a in V solve x=a mod C, x=c mod N. Compatibility is precisely agreement mod g.
   At each prime dividing lcm(C,N), one of these residues is a unit, so the solution is a unit there.
   Choose unit coordinates elsewhere. This constructs a global unit in c U(N) with residue a.
3. Choose a0 in V. It exists by the same local construction, choosing missing prime coordinates to be 1.
   Multiplication by a0 identifies V with H={h in (Z/C)^x: h=1 mod g}. Thus chi(V)=chi(a0)chi(H).
   The displayed integer c need not be a unit modulo C; chi(c) cannot replace chi(a0).
4. Write chi(h)=E(k_h/o). Under multiplication the k_h form an additive subgroup of Z/o.
   Set d=gcd(o, all k_h), m=o/d. Bezout expresses d as an integer combination, so the subgroup
   contains d and all its multiples; every k_h is a multiple of d. Its image is exactly the m-th roots.
   Therefore the phase set is {b+j/m mod 1: 0<=j<m}, b=phase(chi(a0)). Scan C residues to compute it.
5. If C|N, H={1}. If m=1, chi factors through (Z/g)^x, since reduction onto that group is surjective
   by the construction in step 2 and its kernel is H. Primitivity forces g=C. Thus singleton iff C|N
   for N>0; N=0 is already singleton. This proves the SPEC and strict criteria, including even N.

P2. Hull extrema, without enumerating roots. For b,m of P1 put d(t)=dist(m(t-b),Z)/m.
1. Among the equally spaced roots the closest phase to t has circle distance d(t), in [0,1/2].
2. Cosine decreases with distance from 0. The real maximum is cos(2 pi d(0)). On the circle,
   dist(v,Z)=1/2-dist(v-1/2,Z); hence the minimum is -cos(2 pi d(1/2)).
3. Sine is cosine shifted by 1/4. The imaginary interval is
   [-cos(2 pi d(3/4)),cos(2 pi d(1/4))]. All extrema are attained by roots. This is Q4 with r=0;
   using only endpoints of a chosen list of angles can miss a root attaining an extremum.

P3. Lowering a label, and conjugation.
1. Reduction (Z/q)^x -> (Z/f)^x is surjective for f|q: lift separately at each prime and use CRT.
   A character descends iff it is trivial on the kernel. The smallest such f is its conductor.
2. Once descent holds, define chi_f(a)=chi_q(a lift). Two lifts differ by a kernel element,
   so this is well-defined, multiplicative and unique. Its values on units modulo q determine it.
3. Enumerate labels at f and match their exact phases on those units. The unique matching label
   is the primitive label. Matching non-units would be wrong because the zero extension changes.
   Oracle lower uses this procedure, independent of FLINT conductor and lower algorithms.
4. For the Conrey pairing in cyclic coordinates, B(n,a)=sum_j log_j(n)log_j(a)/d_j modulo 1.
   Inversion negates log_j(n) modulo d_j, hence B(n^-1,a)=-B(n,a). This proves the inverse-label
   rule for this pairing, for any choice of cyclic generators; conductor and parity stay unchanged.
   Taking conjugates of exp(s log(t)) also conjugates s, since log(t) is real.
5. Step 4's identification with FLINT is [source pending: dirichlet pairing implementation under refs/].
   Header:84 specifies the logs of a label but does not state this bilinear formula. The oracle checks
   the inverse-label rule for all primitive C<=40; this is finite evidence, not a replacement source.
   Do not mark the universal FLINT identification proved until that source has been supplied.

P4. Certified direct-sum bound; the C additions introduce no unaccounted rounding.
1. At phase precision w, psi.h's singleton bound gives endpoint excess A*2^-w, A=4+2^-27.
   Each coordinate radius is <=A*2^-w. Let l_a,h_a be its exact dyadic endpoints. The true sum
   lies between sum l_a and sum h_a, by adding interval inequalities. Width is <=2*C*A*2^-w.
2. Sum these endpoints exactly (arf exact addition or dyadic integer alignment). The recurrence
   for endpoint error is e_j<=e_(j-1)+A*2^-w+delta_j; here delta_j=0 for all C additions.
   Storage is O(w+log C) bits per accumulator, because all phase endpoints are bounded by 1+A*2^-w.
3. Round the final midpoint to w bits. Its rounding error eta<=2*C*2^-w, since its magnitude
   is at most C*(1+A*2^-w) and the chosen w>=10. Let R=halfwidth+eta.
   The Q1 radius kernel, upward 30-bit rounding plus its successor, gives rho<=(1+2^-28)*R.
   Consequently each output radius is <7*C*2^-w, hence <=8*C*2^-w. Use the exact-zero branch
   when R=0. The bound holds on [-C,C]; Q1's midpoint-error argument is translation independent.
4. Dividing the ball by the positive certified sqrt(C), then rotating by i^-e, encloses W.
   Testing its stated radius bound is a certificate, not an assumption about FLINT rounding.
   L8 gives |tau|^2=C and tau(chi)tau(conj chi)=(-1)^e C; division by i^(2e)C gives W W_conj=1.
   No general proof W=1 for real primitive chi follows. analysis:580-584 leaves that source pending.

## 6. Acceptance tests and planted faults

| Calls | Required rejection criteria; oracle groups |
|---|---|
| lifecycle/layout | principal fields, deep s copy, alias, swaps, allocation balance; ABI/offsets; flint_c |
| is_canonical | reject q=0, n=0 at q>1, imprimitive (16,9), wrong parity, nonfinite s; do not reject solely by D1 |
| constructors/accessors | (16,9)->(8,5,0,2), (8,7)->(4,3,1,2), (1,0) accepted; constructors/lowering |
| conj, get_order | conjugate s, involution, inverse labels, order 2 vs group exponent 4 at (5,4); characters |
| text/dump/inspect | char.tsv, CHAR dispatch, 11.3 decimal rules, exact dump bits, nctx=0; new C tests |
| chi_phase, chi | primitive C<=40, signed a, zero flag, homomorphism; characters |
| unit evaluators | equality of exact phase sets with lifts modulo lcm(C,N), singleton iff C|N or N=0; cosets |
| unit hull | four extrema, endpoint excess, line cases, reject universal square; hulls |
| strict | failure snapshots, uncertain t,s allowed when unit value fixed; faults_33, new C tests |
| class/idele | positive t, complex s, negative sign once, diagonal -1 gives 1; evaluation |
| Gauss/root | P4, FLINT overlap, 17 goldens, prec=128 radius<2^-60; gauss/sum_bound/flint_c/goldens |

For every numerical function test prec cap+1 before invalid INV input, prec<2, alias prohibition, every
reachable failure sentinel, and bounds before allocation. Test q/C=65536 and 65537 for D1 with no large
loop on rejection. Constructor test at the cap uses (65536,1), which lowers to principal. Round-trip
Gauss values must ignore s. Gauss identities test magnitude, conjugate product and W product separately.
Constructor tests must snapshot outputs on all errors, including nonfinite set_s. Dump tests must reject
imprimitive input without lowering. Class conversion failure must preserve the evaluation output.
The eight nonprincipal real-character INPUT rows of gauss.tsv test W=1 after lowering; they include (8,7).

Six faults per work package to plant in the implementation, each with an independent expected result:
3.3: n mod C lowering (16,9); parity=n%2 (5,2); zero returned as phase 0 at a=5;
strict writes on (5,2,[1 mod 1]); hull from first/last phase only (fourth roots, misses -1);
drop sign(x_inf) on (-2;1*[1]) with (3,2), s=1 (expected -2).
3.4: negative finite sign at (3,2); conjugate chi at (5,2); omit C=1 term;
omit i^e in W at (3,2); use input q=8 instead of stored C=4 for (8,7);
divide raw exponent by character order at (5,4), a=2 (wrong 1 instead of -1).
Oracle fault witnesses reject these mathematical mistakes; C output transactions and rounding still need
implementation tests. Do not count a proposed C mutation as killed by a Python reference model.

## 7. Decisions for TJO and thin slices

| Decision | Recommendation and alternative |
|---|---|
| D1 setup/work limit and row | Recommend 65536 and row 223; alternative: larger measured cap. |

D1 adds LIMIT/UNSUPPORTED to raw character construction, beyond generic raw row 202, by placing it
explicitly in character row 223. It bounds expensive work before setup. Unbounded discrete logarithms
are not an hour-slice promise. Conjugation waits for the pairing source; the dump syntax is fixed.

Fixed choices, not questions: stored primitive type CV-58, positive tau and root formula CV-60,
coarse-coset default/strict CV-07, canonical dump body conventions:1414. Source acquisition is required
work, not a decision to treat a probe as a proof. D1 also needs the full-word is_canonical setup failure
semantics checked from the missing FLINT source; it must never turn a resource failure into predicate false.

Slice a (about one hour): type/lifecycle, constructors and getters, text, integer phase/chi, Gauss and root,
using the existing phase API and text helpers. D1 first. Oracles: constructors, lowering, characters,
flint_c, gauss, goldens. Calls: adf char 'char(q=5, n=2, s=(0) + (0)*i)' prints canonical text;
adf chi 'char(q=5, n=2, s=(0) + (0)*i)' 2 gives i; adf gauss with that text prints e, tau, W.
Julia, after allocating with layout queries and init:
ccall((:adf_char_gauss_sum,libadf),Cint,(Ptr{Cvoid},Ptr{Cvoid},Clong),tau,chi,128).
This is the first user-visible slice; no unit-coset algorithm or product is needed to check the goldens.

Slice b (about one hour): conjugation, unit-coset default/strict and Q4 hull, strict dump/load/inspect;
D1 and the pairing source. Oracles: cosets, hulls, characters, faults_33; add C dump/text tests.
adf char_unit 'char(q=5, n=2, s=(0) + (0)*i)' '[1 mod 1]' encloses the four cardinal roots;
char_unit_strict returns NOT_DETERMINED. Julia:
ccall((:adf_char_eval_ucoset,libadf),Cint,(Ptr{Cvoid},Ptr{Cvoid},Ptr{Cvoid},Clong),z,chi,u,128).

Slice c (about one hour): class and idele default/strict; no new decisions. Oracles: evaluation, cosets;
C tests add status composition, positive-ball failure and complex-s enclosure. Calls:
adf char_eval 'char(q=3, n=2, s=(1) + (0)*i)' '(-2 ; 1 * [1])' gives -2;
adf char_eval_strict with the same arguments also succeeds. Julia:
ccall((:adf_char_eval_idele,libadf),Cint,(Ptr{Cvoid},Ptr{Cvoid},Ptr{Cvoid},Clong),z,chi,x,128).
Every slice adds its driver commands, Julia call, exports, failure snapshots, and bounded mutation run.

## 8. Findings and source gaps

Golden normalization check, not a defect: lower(8,7)=(4,3,1,2). Conventions:929-930 lists real-character
golden INPUTS; it does not say every input pair is primitive. The golden and generator correctly lower
this pair and give tau=2i, W=1. Do not change it to a Gauss sum modulo 8. All 17 numeric rows passed.

F1, brief's raw-exponent wording needs a distinction: at (5,4), a=2, dirichlet_chi gives 2, with group
exponent 4 and character order 2. Its phase is 1/2, not 0. Thus its k cannot be divided by the character
order returned by get_order. Oracle chi_exponent rescales; production uses k/G->expo.

F2, brief's chi(c) coset formula: (C,n,c,N)=(3,2,3,4) has phases {0,1/2}, although chi(3)=0.
A compatible unit lift a0 is necessary (P1). SPEC's coset meaning and its singleton test are unaffected.

No counterexample was found to the named SPEC, PLAN, conventions, proofs, or numerical goldens.
P3's universal FLINT pairing identification and full-word group-init failure semantics remain source pending.
The signed real quadratic Gauss evaluation remains pending as already recorded at analysis:580-584;
it is not used for the implementation. The on-disk acb_dirichlet documentation supplies the Gauss sign,
reference-function assumptions and the later factor-method description; no claim is sourced from memory.
