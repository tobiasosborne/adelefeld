# Milestone 4: test functions and Poisson summation

Design before code. SPEC 7, conventions 5.11/5.12 and CV-20 remain fixed. No normal form is introduced.
Here analysis means docs/proofs/analysis.md. The oracle is proto/functions4_checks.py.
Its phases and indexing are exact; its finite transform uses certified acb arithmetic. Real closure is
symbolic. Its 60-digit numerical comparisons allow 1e-45 times max(1, absolute reference value).
Those comparisons are not ball certificates. No adelefeld implementation is imported.

## 1. Values and common contract

The fixed layouts, with the usual one-element-array typedefs and ptr/srcptr aliases, are:
```c
typedef struct { ulong D, M; acb_ptr f; } adf_ffun_struct;
typedef struct { acb_poly_t P; acb_t A, B, C; } adf_rterm_struct;
typedef struct { slong len; adf_rterm_struct *term; } adf_rfun_struct;
```
An ffun has D,M>=1, L=DM<2^62, L live finite acb entries, and owned non-NULL storage.
Its members are functions zero off (1/D) Zhat, taking an independently chosen value in f[j] on
j/D+M Zhat. An rfun has len>=0, live owned term storage when len>0, finite normalized polynomials,
finite A,B,C and a certified strictly positive real part of every A. Each member chooses parameters
once, then evaluates sum P(x) exp(-pi A x^2+B x+C) on real x. Zero polynomials and term order persist.
Operations enclose every resulting member; loss of dependencies between parameter balls can enlarge the family.
Pair (phi,f) means (x_inf,x_f) -> phi(x_inf) f(x_f). Finite lists of pairs mean sums, with no new adf_afun
owning type (D2): pair calls suffice; callers add results and distribute products. Correlations are not stored.

Inputs/outputs are initialized; canonical inputs are preconditions except raw setters and predicates.
Same-type whole-object aliasing is allowed; member aliasing and overlapping distinct outputs are forbidden.
Every status failure leaves all outputs untouched, including counts. Build temporaries and commit together.
INV checks entry predicates, except predicates themselves, which inspect initialized readable storage.
Every arithmetic/evaluation call first rejects prec>ADF_REAL_PREC_MAX=2097152 with LIMIT; p=max(prec,2).
Nonfinite intermediates or failure to certify a required output sign give NOT_DETERMINED, never DOMAIN.
Arithmetic, evaluation, transforms and integrals are assigned to conventions 3.2's "Integrals, Poisson
summation" row by proposal D1: OK, DOMAIN, NOT_DETERMINED, LIMIT. That row does not currently explicitly
name test-function algebra. Text/load calls use their existing separate rows, without new statuses.

D1 proposes operation caps: array length 2^20, total terms and coefficients each 2^16, and 2^20 charged
work units per call, including retries; direct Fourier L^2<=2^20. Charge one unit per dense polynomial
coefficient multiply-add, phase term (evaluation/product/addition together), refinement or unit-residue
index, lattice term, and tail-ratio iteration; allocation is bounded separately. Each exact integer has at most
2^20 bits; check projected products, shifts, lcm and allocations before forming them. Also require counts
to fit slong and bytes to fit SIZE_MAX. These are algorithm limits, not stronger canonical predicates.
Predicates/copy/printing traverse any valid allocated input; they do not reject merely because D1 is exceeded.
Text keeps conventions 8.4's caller limits and validation order. Preflight arithmetic sizes before INV
or allocating outputs. Raw setters below use D1's row, not the generic raw-constructor row.

```c
/* Init zero: ffun (1,1,[0]); rfun len=0, term=NULL. Own storage; no status. */
void adf_ffun_init(adf_ffun_t x);
void adf_rfun_init(adf_rfun_t x);
/* Release each initialized entry/term and owned storage; afterwards only init is permitted. */
void adf_ffun_clear(adf_ffun_t x);
void adf_rfun_clear(adf_rfun_t x);
/* Exact deep copies, cost input size; self-copy allowed, no numerical rounding. */
void adf_ffun_set(adf_ffun_t y, const adf_ffun_t x);
void adf_rfun_set(adf_rfun_t y, const adf_rfun_t x);
/* O(1) exchange; self-swap allowed. */
void adf_ffun_swap(adf_ffun_t x, adf_ffun_t y);
void adf_rfun_swap(adf_rfun_t x, adf_rfun_t y);
/* Test the full predicates above without aborting; live array capacity is a caller precondition. */
int adf_ffun_is_canonical(const adf_ffun_t x);
int adf_rfun_is_canonical(const adf_rfun_t x);
/* Representation identity, O(input size): same layout/order and acb_equal fields, not function equality. */
int adf_ffun_identical(const adf_ffun_t x, const adf_ffun_t y);
int adf_rfun_identical(const adf_rfun_t x, const adf_rfun_t y);
/* Header-inline and exported ABI queries. Bindings allocate using these, never guessed offsets. */
size_t adf_sizeof_ffun(void); size_t adf_alignof_ffun(void);
size_t adf_sizeof_rterm(void); size_t adf_alignof_rterm(void);
size_t adf_sizeof_rfun(void); size_t adf_alignof_rfun(void);
/* Raw deep-copy setters; DOMAIN for invalid shape, nonfinite fields, or Re(A) not positive;
   LIMIT for D1. Terms must have normalized P. Cost total entries; no prec, no rounding. */
int adf_ffun_set_acb_vec(adf_ffun_t y, ulong D, ulong M, acb_srcptr f, slong n);
int adf_rfun_set_terms(adf_rfun_t y, const adf_rterm_struct *terms, slong n);
```

## 2. Text and dump declarations

Conventions 9.4 already fixes ffun(D=D, M=M; z(f_0), ...) and rfun(term(P=[...], A=, B=, C=), ...).
Conventions 10.1:1412-1420 already defines both dump bodies: ffun D M followed by DM acb groups;
rfun len followed by each polynomial length, its coefficients, A,B,C. Integers are hexadecimal, acb is
eight hexadecimal integers, and the prefix is adf1 Q. There is no format decision to make.
Readers trim only exact trailing polynomial zeros; loaders reject them. Neither combines or sorts terms.
```c
/* Value readers: full grammar then limits then domain, conventions 8.5/9.3; OK commits.
   PARSE, LIMIT, UNSUPPORTED, DOMAIN, NOT_DETERMINED preserve x. Last status only when rounding
   an exactly positive decimal Re(A) fails to preserve its sign. O(text + represented entries). */
int adf_ffun_set_str(adf_ffun_t x, const char *s, size_t len, slong prec, const adf_text_limits_t *lim);
int adf_rfun_set_str(adf_rfun_t x, const char *s, size_t len, slong prec, const adf_text_limits_t *lim);
/* Canonical value text; digits/text ownership/refusal rules of text.h. Caller adf_str_free.
   *len excludes NUL. NULL and len=0 on exponent/printing-work refusal. Constrain Re(A)>0.
   Cost printed size plus decimal conversion; all 37 goldens use conventions 11.3, not bit identity. */
char *adf_ffun_get_str(size_t *len, const adf_ffun_t x, slong digits);
char *adf_rfun_get_str(size_t *len, const adf_rfun_t x, slong digits);
/* Strict lossless loaders; validate all tokens before FLINT. No normalization; no prec.
   OK or PARSE/LIMIT/UNSUPPORTED/DOMAIN; outputs untouched on failure; cost text size.
   These bodies have zero context occurrences; ctx is ignored by the convenience entry point. */
int adf_ffun_load_str(adf_ffun_t x, const char *s, size_t n, const adf_modctx_struct *ctx,
                      const adf_text_limits_t *lim);
int adf_rfun_load_str(adf_rfun_t x, const char *s, size_t n, const adf_modctx_struct *ctx,
                      const adf_text_limits_t *lim);
/* Same load; require nbinds=0 after full validation. binds is then ignored. */
int adf_ffun_load_str_binds(adf_ffun_t x, const char *s, size_t n,
    const adf_modctx_struct *const *binds, size_t nbinds, const adf_text_limits_t *lim);
int adf_rfun_load_str_binds(adf_rfun_t x, const char *s, size_t n,
    const adf_modctx_struct *const *binds, size_t nbinds, const adf_text_limits_t *lim);
/* Allocate exact dump, preserving term order and all ball bits. *len excludes NUL; adf_str_free.
   No status, cost output size. Dumping then loading is identical, including retained zero terms. */
char *adf_ffun_dump_str(size_t *len, const adf_ffun_t x);
char *adf_rfun_dump_str(size_t *len, const adf_rfun_t x);
/* Validate as load, then write nctx=0; descs untouched. Same statuses and cost; no contexts built. */
int adf_ffun_dump_inspect(size_t *nctx, adf_ctx_desc_t *descs, const char *s, size_t n,
                          const adf_text_limits_t *lim);
int adf_rfun_dump_inspect(size_t *nctx, adf_ctx_desc_t *descs, const char *s, size_t n,
                          const adf_text_limits_t *lim);
```

## 3. Finite algebra (4.1), and statements F1 to F3 to add to analysis

F1. Refinement from (D,M) to (D2,M2), with D|D2 and M|M2, puts r=D2/D and
g[k]=0 if r does not divide k, otherwise g[k]=f[(k/r) mod DM].
1. k/D2 lies in (1/D) Zhat iff k/r is an integer: a rational integral at every prime is an integer.
2. In that case reducing k/r modulo DM selects its old coset. M2 Zhat is contained in M Zhat.
3. Thus both zero extension and repetition preserve each exact function. Repeated uncertain entries
can lose correlations, so the stored output family encloses the old family. Common refinement is
D2=lcm(Da,Db), M2=lcm(Ma,Mb). Add/multiply entries there; cost O(D2 M2), charged even for zeros.

F2. Translation by q has D2=lcm(D,den(q)), M2=M. Refine first; put b=D2 q in Z and read g[k-b].
1. x-q at x=k/D2 has index k-b. Periods are unchanged, so indices are reduced modulo D2 M.
2. Outside (1/D2) Zhat neither x nor x-q lies in the old support. Hence this is also correct off-grid.
3. If den(q)|D this is just a permutation. A delta at index 1 with (D,M)=(2,3), translated by 1/2,
moves to index 2, not 0. Negating the argument reads f[-k mod L], without conjugating values.

F3. For q=s/t reduced, s!=0,t>0, choose D2=|s|D, M2=tM (intentionally not minimal).
The new entry is zero unless t|k; otherwise it is f[sign(s) k/t mod L].
1. q k/D2=sign(s) k/(tD) belongs to old support iff t|k; multiplying the new period by q gives sM.
2. If qx is in old support, x is in (t/(|s|D)) Zhat, a subset of (1/D2) Zhat. This proves the formula.
3. For idele finite part r u, r=s/t>0, enumerate R={v mod L: gcd(v,L)=1, v=c mod gcd(N,L)}
for u=c U(N), N>0; for N=0 use the exact residue c mod L. These are precisely its images:
compatible residues lift modulo lcm(N,L), with unit conditions at every prime; CRT gives a unit lift.
4. A singleton R fixes all array indices, so replace the surviving entry by f[v k/t mod L].
Non-singleton R returns NOT_DETERMINED, even if this particular f happens to be invariant. This is an
index certificate, not an equality solver for ball-valued functions. Example L=6,c=1,N=3 has R={1};
N=1 has R={1,5}. The sufficient shortcut L|N must not replace the exact singleton test.
Generic dilation balls must first pass the existing adele-to-idele certification, then this test;
conversion statuses belong to that conversion call. No sampling of an uncertified scalar is allowed.

```c
/* Refine as F1; DOMAIN unless D2,M2>=1 and divisibility holds; LIMIT for D1; OK exact copies/zeros.
   Cost O(D2 M2), including zero entries. */
int adf_ffun_refine(adf_ffun_t y, const adf_ffun_t x, ulong D2, ulong M2);
/* Common F1 refinement, then acb add/mul at p. OK, LIMIT, NOT_DETERMINED. O(lcm(D)*lcm(M)). */
int adf_ffun_add(adf_ffun_t z, const adf_ffun_t x, const adf_ffun_t y, slong prec);
int adf_ffun_mul(adf_ffun_t z, const adf_ffun_t x, const adf_ffun_t y, slong prec);
/* F2 translation and argument reflection. Exact copies; OK or LIMIT; same-object alias allowed. */
int adf_ffun_translate_rat(adf_ffun_t y, const adf_ffun_t x, const adf_rat_t q);
int adf_ffun_reflect(adf_ffun_t y, const adf_ffun_t x);
/* F3; exact copies, cost new array length. DOMAIN for q=0, LIMIT for caps, otherwise OK. */
int adf_ffun_dilate_rat(adf_ffun_t y, const adf_ffun_t x, const adf_rat_t q);
/* Finite part only, ignores a.inf; F3 certificate, OK/NOT_DETERMINED/LIMIT, O(L+new length). */
int adf_ffun_dilate_idele(adf_ffun_t y, const adf_ffun_t x, const adf_idele_t a);
/* Pointwise conjugate, exact, O(L); OK or LIMIT. Needed for inner products of tensor sums. */
int adf_ffun_conj(adf_ffun_t y, const adf_ffun_t x);
```

## 4. Finite transform (4.2) and error statement F4

Use analysis P4: output layout is (M,D), g[k]=(1/M) sum_j f[j] E(-jk/L).
Call adf_phase_get_acb with exact angle (-jk mod L)/L reduced into [0,1); multiply indices in fmpz
or overflow-safe modular arithmetic. E(theta) is the phase evaluator; it never chooses the Fourier sign.
The next transform uses 1/D and equals reflection, not identity. Integral and norm appear in section 6.
Source for the character convention: refs/src/tate-poonen/notes.txt:693-700,733-740. The project
conjugates that character (conventions 6.1), giving the negative finite and positive real kernels.

F4. Certified direct summation, including uncertain coefficients.
1. Enclose each input rectangle by disk (m_j,r_j), r_j=sqrt(rad_re^2+rad_im^2), rounded upward.
Let the evaluated phase have midpoint e_j and disk error eta_j; |E|=1 implies |e_j|<=1+eta_j.
psi.h bounds eta_j by sqrt(2)(4+2^-27)2^-w at phase precision w for a singleton input angle.
2. The exact product differs from m_j e_j by at most r_j+(|m_j|+r_j)eta_j.
3. If delta_j bounds rounding of the midpoint multiplication/addition, a sum accumulator satisfies
R_k <= (sum_j [r_j+(|m_j|+r_j)eta_j+delta_j])/M + delta_div.
Each delta is bounded by directed arf rounding, or already included by the acb operation.
4. Apply the triangle inequality inductively to all L terms. Final rectangular rounding adds its
own outward error; acb handles it. Never report O(2^-prec) without the coefficients and L factors.
Input radii impose a floor. At exact inputs increasing precision must reduce numerical error.
```c
/* Weighted direct transform above; O(L^2) acb products/additions and phase evaluations, O(L) storage.
   OK encloses every transform; LIMIT above D1 including L^2, NOT_DETERMINED on phase failure.
   No hidden unweighted FFT. p is working precision, not a guarantee of absolute output width. */
int adf_ffun_fourier(adf_ffun_t y, const adf_ffun_t x, slong prec);
```

## 5. Real algebra and transform (4.3), statements R1 and R2

Analysis P5 gives translation: P(x-q), A, B+2 pi A q, C-Bq-pi A q^2; dilation by h!=0:
P(hx), Ah^2, Bh, C; product: multiply P and add A,B,C. Sum concatenates terms, left before right;
product orders pairs lexicographically. Derivative uses P'+(B-2 pi A x)P with unchanged A,B,C.
Conjugation conjugates all coefficients and parameters. Polynomial work uses acb; remove only exact
trailing zeros. Translation/dilation retain parameter balls, not a fitted polynomial expansion.
For rational q,h multiply by their exact integers and divide by their exact denominators before rounding.
For an idele use h=a.inf with arb interval squaring (not two independent signed copies), and r,u in F3.
The real output encloses all h in that interval. If rounding loses Re(A')>0, return NOT_DETERMINED.

R1. Explicit transform polynomial, for the positive real kernel, from P5.
1. Put H_j(z)=j! sum_(r=0)^floor(j/2) z^(j-2r)/[r!(j-2r)!(4 pi A)^r (2 pi A)^(j-2r)].
Differentiating exp(z^2/(4 pi A)) gives this formula: its Taylor quotient at z+t is
exp(zt/(2 pi A)) exp(t^2/(4 pi A)); collect the coefficient of t^j and multiply by j!.
2. The new parameters are A'=1/A, B'=iB/A, C'=C+B^2/(4 pi A), and the new polynomial is
Q(y)=A^(-1/2) sum_j p_j H_j(B+2 pi i y). Expand by polynomial arithmetic; its degree is at most deg P.
Compute H_0=1, H_(j+1)=H_j'+z H_j/(2 pi A), costing O((deg P+1)^2) scalar operations.
The amplitude belongs in Q; A',B',C' remain separate balls. In particular F(x exp(-pi x^2))=iy exp(-pi y^2).
3. The shift q=1/3 of exp(-pi x^2) has B=2 pi/3,C=-pi/9 and transform exp(-pi y^2) E(y/3).
At y=1/4 its imaginary part is positive. The opposite convention gives its conjugate.

R2. Square root branch, fixed by P5, not a new choice for TJO.
1. For A=a+ib with a>0, set u=sqrt((|A|+a)/2)>0 and v=b/(2u). Then (u+iv)^2=A.
2. Its real part never vanishes on the right half-plane. The other root has negative real part;
continuity from positive real A selects this root uniquely. Local differentiation gives derivative 1/(2 sqrt(A)).
3. Re(1/A)=a/|A|^2>0 and arg(A) is between -pi/2 and pi/2. Hence the chosen roots of A and 1/A
are reciprocal. This fixes the amplitude of F^2 to 1 and yields reflection by P5.
4. acb_sqrt's documented formula implements step 1 (refs/src/flint-3.0.1/acb.rst:590-615).
Use it or acb_rsqrt_analytic with analytic=1. No branch cut meets a canonical A box; certificate
failure at working precision is NOT_DETERMINED. Re(1/A)>0 still needs the output-ball predicate check.

```c
/* Sum concatenates; product takes ordered term pairs. OK/LIMIT/NOT_DETERMINED. Costs respectively
   total size and sum of dense polynomial product costs. Neither merges equal-looking parameters. */
int adf_rfun_add(adf_rfun_t z, const adf_rfun_t x, const adf_rfun_t y, slong prec);
int adf_rfun_mul(adf_rfun_t z, const adf_rfun_t x, const adf_rfun_t y, slong prec);
/* P5 formulas above. Rational shift/dilation: O(sum(degree+1)^2)/O(total coefficients).
   OK/LIMIT/NOT_DETERMINED; dilation additionally DOMAIN for h=0, including the zero function. */
int adf_rfun_translate_rat(adf_rfun_t y, const adf_rfun_t x, const adf_rat_t q, slong prec);
int adf_rfun_dilate_rat(adf_rfun_t y, const adf_rfun_t x, const adf_rat_t h, slong prec);
/* Only the real component of the certified idele is used. Same dilation cost and statuses except DOMAIN. */
int adf_rfun_dilate_idele(adf_rfun_t y, const adf_rfun_t x, const adf_idele_t a, slong prec);
/* Argument reflection: exact odd-coefficient and B sign changes. Conjugate: exact field conjugation.
   O(total size), OK or LIMIT. Derivative uses the polynomial formula above, same cost, numerical statuses. */
int adf_rfun_reflect(adf_rfun_t y, const adf_rfun_t x);
int adf_rfun_conj(adf_rfun_t y, const adf_rfun_t x);
int adf_rfun_derivative(adf_rfun_t y, const adf_rfun_t x, slong prec);
/* R1/R2 term by term, preserving order and zero terms. O(sum(degree+1)^2).
   OK/LIMIT/NOT_DETERMINED, including failure to certify positive Re(1/A). */
int adf_rfun_fourier(adf_rfun_t y, const adf_rfun_t x, slong prec);
```

## 6. Evaluation and additive integrals (4.4), statement E1

E1. Evaluate a finite ball (A+H Zhat)/d, H>=0,d>0, using its exact global triple (CRT if local).
1. Clear denominators against j/D+M Zhat. The intersection is nonempty iff
gcd(HD,MdD) divides AD-jd. Enumerate these j in [0,L). H=0 uses gcd(0,MdD)=MdD.
Indeed two integer ideal cosets meet iff their centre difference is divisible by the gcd: Bezout
constructs a meeting point; divisibility is necessary because every difference is in the sum ideal.
2. The whole ball lies in (1/D) Zhat iff d|DA and d|DH. Otherwise include the zero value for points
outside support. Take the rectangular hull of all selected f[j] and this zero, using acb_union.
This encloses the full image, not a sample. Empty index set gives exactly zero.
3. For (D,M)=(2,3), f=[1,2,3,4,5,6], ball Zhat meets {0,2,4}, hence values {1,3,5}.
The ball (1/4) Zhat meets all six indices and points outside support; its hull must contain 0 and 6.
The exact point 1/5 is outside support and evaluates to 0.
4. For a partial sball define evaluation explicitly on all adelic completions of its tuples (D2).
At each supplied p, keep j iff v_p(j/D-a_p)>=min(e_p,v_p(M)); for an exact local point replace
min by v_p(M). Missing primes impose no restrictions. Local independence proves sufficiency.
Always include zero: some absent prime can put a completion outside support. No implicit Z_p is supplied.
5. With REAL arch, evaluate phi on that arb by Horner, exponential and term addition. With NONE,
bound the whole real axis: alpha=pi lower(Re A)>0, beta=upper(|Re B|), gamma=upper(Re C), and
R=exp(gamma+beta^2/(2alpha)) sum_j upper(|p_j|) T_j, T_0=1, T_j=(j/(alpha e))^(j/2) for j>0.
Completing the square bounds the exponent by gamma+beta^2/(2alpha)-alpha x^2/2;
maximizing |x|^j exp(-alpha x^2/2) gives T_j. Sum R over terms; use [-R,R]+i[-R,R].
COMPLEX arch is DOMAIN: this interface requires REAL or NONE, with no complex-to-real coercion.

The finite Haar integral is sum f[j]/M; its squared L2 norm is sum |f[j]|^2/M (analysis P4).
For one real term the integral is A^(-1/2) exp(C+B^2/(4 pi A)) sum_j p_j H_j(B), with R1's H_j.
Sum over terms. Tensor integral multiplies the two integrals. Real norm2 integrates phi*conj(phi),
including every cross term, and rounds its intersection with [0,infinity) outward; tensor norm2 multiplies.
For sums of tensors use the pairwise inner products, with finite common refinement; summing individual
norms is wrong. A ball norm is a uniform enclosure, not a norm of the midpoint function.
```c
/* E1 hull on a finite ball, O(L) exact divisibility tests plus global-triple cost.
   Real evaluation: O(total coefficients) acb operations plus one exp per term.
   OK/LIMIT/NOT_DETERMINED; raw real input must be finite, otherwise DOMAIN. */
int adf_ffun_eval(acb_t z, const adf_ffun_t f, const adf_fball_t x, slong prec);
int adf_rfun_eval(acb_t z, const adf_rfun_t phi, const arb_t x, slong prec);
/* Multiply real and finite enclosures; sum component costs. Same statuses, no member aliasing. */
int adf_tensor_eval(acb_t z, const adf_rfun_t phi, const adf_ffun_t f, const adf_adele_t x, slong prec);
/* E1 steps 4/5; O(L * number of supplied primes) valuation tests plus real evaluation/bound.
   OK/LIMIT/NOT_DETERMINED; DOMAIN for COMPLEX arch. No factorization of unspecified primes. */
int adf_tensor_eval_sball(acb_t z, const adf_rfun_t phi, const adf_ffun_t f,
                          const adf_sball_t x, slong prec);
/* Additive integrals and squared L2 norms above. OK/LIMIT/NOT_DETERMINED, no DOMAIN on canonical
   input. Finite cost O(L); real integral R1 cost; real norm includes O(len^2) ordered products.
   Clip nonnegative norm interval after enclosure; a provably empty clip is an internal defect. */
int adf_ffun_integral(acb_t z, const adf_ffun_t f, slong prec);
int adf_ffun_norm2(arb_t z, const adf_ffun_t f, slong prec);
int adf_rfun_integral(acb_t z, const adf_rfun_t phi, slong prec);
int adf_rfun_norm2(arb_t z, const adf_rfun_t phi, slong prec);
int adf_tensor_integral(acb_t z, const adf_rfun_t phi, const adf_ffun_t f, slong prec);
int adf_tensor_norm2(arb_t z, const adf_rfun_t phi, const adf_ffun_t f, slong prec);
```

## 7. Poisson (4.5), statement P1 and milestone 5 consumer

Analysis P7 identifies the two absolutely convergent sums for a tensor:
left=sum_j f[j] sum_n phi(j/D+Mn); right=sum_n hat_f[n mod L] hat_phi(n/M), with n in Z.
The rational support is contained in (1/D) Z, not necessarily equal to it: zero entries remove classes.
Transform the two factors, then compute each displayed sum independently, including n=0.
Never derive the second enclosure from the first or intersect them to hide a bad truncation.

P1. Explicit truncation algorithm for uniform parameter balls.
1. For each term and lattice (a,h), expand P(a+hn)=sum q_j n^j and set
alpha=pi lower(Re(Ah^2))>0, beta=upper(|Re(h(B-2 pi A a))|), gamma=upper(Re(C+Ba-pi A a^2)).
Lemma 6 gives B(a,h,N)=2 exp(gamma) sum upper(|q_j|) S_j(alpha,beta,N).
Evaluate its prefix and ratio bound outward; rho<=1/2 must be certified. Charge prefix iterations too.
2. A sufficient start for its geometric regime is integer K>=1 with
2 alpha K >= j+beta+log(2); then j/K-alpha(2K+1)+beta<=-log(2).
This is a preflight upper bound on prefix work, not permission to omit the prefix for smaller N.
3. Left tail E_L=sum_j upper(|f[j]|) sum_terms B(j/D,M,N_L).
Right tail E_R=max_k upper(|hat_f[k]|) sum_hat_terms B(0,1/M,N_R).
For target epsilon=2^-bits, search N_L and N_R independently through 0,1,2,4,... until each E<=epsilon/8.
These explicit integers and the tested inequalities are the truncation certificate. Check D1 before each step.
4. Add the tail to both coordinate radii of each independently computed acb partial sum. Require
each final coordinate diameter <=epsilon; increase working precision geometrically from p to the cap
if arithmetic prevents this. Include transformed parameter uncertainty and every operation's rounding.
5. Canonical input already has lower(Re A)>0. Very small positive bounds may exhaust work: LIMIT.
Losing positivity on a computed transform, or failing width at the precision cap, is NOT_DETERMINED.
Raw nonpositive A is DOMAIN in construction. Fixed input radii can prevent any requested small width;
more precision cannot remove them. Do not return OK with an unmet width target.
```c
/* Independent P1 sums, both enclosures and cutoffs committed atomically. bits in [0,2^21]; otherwise
   DOMAIN (after the prec cap). OK certifies coordinate widths <=2^-bits and valid tails; D1 gives
   LIMIT, certificate/width failure gives NOT_DETERMINED. Cost L^2 plus real transforms and
   L(2NL+1)+2NR+1 real evaluations; charge polynomial sizes, tail searches and precision retries.
   NL/NR count lattice indices, not all terms; outputs must be distinct. */
int adf_tensor_poisson(acb_t left, acb_t right, ulong *NL, ulong *NR,
    const adf_rfun_t phi, const adf_ffun_t f, slong bits, slong prec);
```
For finite sums call per pair with epsilon divided by the number of pairs, then add outward and check
the final width. A zero-length list is exactly zero. A caller needing theta at a certified idele first
dilates both factors into temporaries; commit neither factor if either fails. Fourier scaling uses
|a|=|a_inf|/r, not just |a_inf|. Rational diagonal q has adelic norm 1.

Milestone 5 calls the factor transforms, evaluation, derivative, integral and Poisson/tail kernels.
The zeta vector is D=M=1,f[0]=1 and P=[1],A=1,B=C=0. For primitive chi of conductor C and parity e,
take D=1,M=C,f[j]=chi(j), extended by zero on nonunits, and P=x^e,A=1,B=C_real=0.
This gives 1_(Z_p) off C and chi_p 1_(Z_p^x) at ramified primes; against omega=conj(chi(u')) these
are the inverse local unit characters of conventions 6.5 and analysis P9-P11. Additive integrals here
are not multiplicative Tate integrals. P12's finite unit averages and L14/P15's Mellin truncation and
quadrature use these factors and derivatives; continuation, pole objects and local L factors belong to 5.

## 8. Acceptance and faults

All C slices add INV, alias/self-alias tests, deep-copy/clear checks and failure snapshots. Precision cap+1
must fail before invalid INV inputs or allocation. Test zero, len=0, zero P retained, and exact trailing
zeros versus balls containing zero. Exercise every cap at/below/above its boundary without running huge loops.
The oracle is finite evidence for formulas, not a C status, allocation, parser, or ABI test.

| Calls | Required acceptance, beyond the common tests; oracle groups |
|---|---|
| init/clear/set/swap/predicates/layout | Exact init fields, independent copies, exported query alignment; C only |
| raw setters | Bad dimensions/length, nonfinite entries, A touching 0 rejected without a write; C only |
| text/load/dump/inspect | All 18 ffun and 19 rfun rows; conventions 11.3; dump identity and nctx=0 |
| refine/add/mul | Exact rational point oracle, zero extension and repeats, (2,3)+(3,2) -> (6,6); finite_algebra |
| translate/reflect/dilate_rat | Every new cell, positive/negative scalars and shifts with new denominators |
| dilate_idele | Exact unit-image sets against unit lifts through lcm; singleton at (L,c,N)=(6,1,3) |
| ffun_fourier | Nonsymmetric complex arrays, delta at index 1, D!=M, F^2=reflection, weighted Parseval |
| finite numerical error | Radius decreases at 64/128/212 bits on exact data; F4 plus uncertain input corners |
| rfun add/mul/translate/dilate | Exact expanded polynomial/exponent identities; real_closure; C sum order |
| rfun derivative/conj/reflect | Polynomial identity, conjugate integral norm, F^2 and negative dilation |
| rfun_fourier/integral | Direct independent real quadrature, shifted Gaussian, complex A, both root quadrants |
| finite/tensor eval | Complete E1 index sets, outside-support zero, exact outside point, partial missing places |
| real eval/norm2 | Real-ball samples enclosed, derivative cross terms, nonnegative norm enclosing quadrature |
| tensor integral/norm2 | Products of factor results, and sum cross terms rather than sum of norms; C tests |
| poisson | Separate cutoffs, both sums with tails, coordinate width target, three-precision ball tests |
| tail/domain/limits | Two-sided tails, shifted/non-even P, negative lattice spacing, A=10^-8 hits prefix budget |

The golden files are text vectors, not Fourier output tables. golden_transforms derives numerical tests
from their six valid finite inputs, including coefficient uncertainty. No new expected output is inserted
into the fixed files. Numeric direct quadrature has the declared 1e-45 margin; production tests must use
certified reference balls or independently bounded quadrature, not treat that margin as a proof.

Six faults per work package, with a distinguishing input or invariant:

4.1: max for lcm(D); max for lcm(M), both (2,3)/(3,2); reverse shift of delta_1 by 1/2;
fill refinement holes; use q^-1 instead of q; accept U(1) at L=6.
4.2: positive finite sign; weight 1/D instead of 1/M; drop reflection; retain (D,M); use jk/D as phase;
use 1/M instead of 1/D in Parseval. Delta_1 at (2,3) distinguishes all six.
4.3: negative real kernel on shift 1/3; omit i in the odd Gaussian; omit -pi A q^2 in C;
lose sign of h in B; multiply A in products; omit A^-1/2 at A=4.
4.4: sample one finite point; omit outside zero; accept 1/5 in support at D=2; default absent primes
to Z_p; weight integral by 1/D; square sum instead of sum of squares.
4.5: drop factor 2 in L6; reverse only real transform sign; omit q=0; use right spacing 1/D;
bound omitted n>N by S(N+1); drop beta from shifted-tail bound.

These are mathematical witnesses, not a claim that C mutants were executed. After implementation, run
bounded mutations on the changed files and check transactions separately. No whole-source mutation sweep.

## 9. Decisions for TJO and thin slices

| Decision | Proposed choice; alternative |
|---|---|
| D1 | Stated caps and test-function algebra in the integrals/Poisson status row; measured larger caps later |
| D2 | Paired calls and caller-owned lists, no adf_afun lifecycle/text; owning tensor-sum type later if needed |
| D3 | sball evaluation encloses all adelic completions, including NONE real bound; require complete input instead |

D3 is confined to the new evaluator; it does not change sball's tuple meaning. The square-root branch
is already fixed by analysis P5. Dump bodies and no normal form are already fixed, not approval questions.
Ask decisions when their slice starts (workflow rule 1); none authorizes changes to SPEC here.

Each slice ends in the driver and a Julia ccall, with tests first. Symbols below use initialized pointers
allocated via layout queries. P=Ptr{Cvoid}; lib=libadelefeld. Every slice includes its C tests and exports.

1. 4a, about one hour: ffun lifecycle/setter, text, sum and direct transform (4.1 plus the minimum 4.2).
   D1/D2. Commands adf ffun, adf ffun_add F G, adf ffun_fourier F; goldens, golden_transforms,
   finite_algebra/finite_transform/precision. Julia ccall((:adf_ffun_fourier,lib),Cint,(P,P,Clong),g,f,128).
2. 4b: remaining 4.1 refinement/product/translation/reflection/rational and idele dilation, finite conjugate.
   D1; finite_algebra/idele_indices/faults_41. adf ffun_dilate F 2/3 and adf ffun_dilate_idele F A.
   Julia ccall((:adf_ffun_dilate_rat,lib),Cint,(P,P,P),g,f,q). Reject ambiguous units without output writes.
3. 4c: finish 4.2 certification, finite integral/norm2, strict dump/load/inspect. No new decisions.
   finite_transform/golden_transforms/precision/faults_42. adf ffun_norm2 F, adf ffun_dump F.
   Julia ccall((:adf_ffun_norm2,lib),Cint,(P,P,Clong),z,f,128). Verify F4 on uncertain coefficients.
4. 4d: rfun lifecycle/text/setters/add/product/translation/dilation/conjugate/reflection, real evaluation.
   D1/D2; goldens/real_closure. adf rfun_translate R 1/3 and adf rfun_mul R S.
   Julia ccall((:adf_rfun_translate_rat,lib),Cint,(P,P,P,Clong),out,r,q,128).
5. 4e: real transform, derivative and closed integrals/norms. No new decisions; R1/R2 first.
   real_transform/integral_norm/faults_43. adf rfun_fourier R and adf rfun_integral R.
   Julia ccall((:adf_rfun_fourier,lib),Cint,(P,P,Clong),out,r,128).
6. 4f: 4.4 finite hull, tensor evaluation/integral/norm2 and partial completion evaluator; D3.
   evaluation/faults_44. adf tensor_eval R F X and adf tensor_eval_sball R F S.
   Julia ccall((:adf_tensor_eval,lib),Cint,(P,P,P,P,Clong),z,r,f,x,128).
7. 4g: 4.5 bounded tail kernel, independent Poisson sums, width retries and reported cutoffs; D1.
   tails/poisson/certified_poisson/faults_45. adf poisson R F --bits 80 --prec 144 prints both balls and cutoffs.
   Julia ccall((:adf_tensor_poisson,lib),Cint,(P,P,P,P,P,P,Clong,Clong),l,r,nl,nr,phi,f,80,144).

Adversarial review after a slice/group uses an independent oracle (workflow rule 3). Review this design
before 4a, especially F3, the correlation loss in F1, E1's outside-support case and the charged tail prefix.
Do not extend an hour lane into implementing every call in this map.

## 10. Findings and source obligations

SPEC/PLAN: no counterexample found to the named mathematical formulas or exact-input acceptance tests.
Conventions: the status table omits an explicit row for test-function algebra. D1 resolves the interface
gap; for example the valid (D,M)=(1025,1) needs LIMIT for a direct transform even though generic ring
arithmetic has no status. The fixed golden/struct predicates need no changes. All 37 golden rows agree
with the existing exact text oracle; that is not evidence for an unwritten C reader.

Proof qualification: analysis P15:726-728 must not be read as arbitrary final width on fixed parameter
balls. With phi=c exp(-pi x^2), c in [1,2], f=1_Zhat, the Poisson value interval has width
sum_n exp(-pi n^2)=1.08643481121330801... for every arithmetic precision. Additive integral width is 1.
The oracle computes this witness. The tail and discretization bounds can tend to zero; the input width
cannot. No change to P15's exact-parameter formulas is requested. No counterexample to the goldens was found.
Brief qualification: rational nonzero support is a subset of the stated lattice classes (delta_1 at (2,3)),
not all of (1/D) Z. E1 and the Poisson sum retain zero entries without asserting support minimality.

Source obligations inherited from analysis Definition 1: Haar/product compactness, Fubini/dominated
convergence/holomorphic parameter integration, identity theorem, Fourier uniqueness, CRT and unique
factorization. [source pending: local line references for those analytic imports as listed in analysis:30-66]
Our added finite-index, error and branch arguments are written out above. The ball enclosure source is
on disk: refs/src/flint-3.0.1/arb.rst:6-12 says results contain the exact operation on any input points;
acb.rst:6-12 specifies rectangles and :271-273 specifies union. The character source is cited in section 4.
Milestone 5's incomplete-Gamma and vertical-strip sources remain its obligations (analysis P15), not a
reason to replace this milestone's elementary Gaussian-series certificate with a library L-value.
