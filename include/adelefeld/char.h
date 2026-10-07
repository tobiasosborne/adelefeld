/* Slice a: primitive finite character chi and the finite complex ball family t^s chi.
   docs/api-3c.md 1-4, P3/P4; conventions 5.13 (CV-58), 6.4 (CV-60); SPEC 5.
   Initialized canonical inputs and initialized outputs are required, except raw constructors
   and is_canonical. Same-type whole-object aliasing is allowed. Member aliasing is forbidden.
   Every status failure preserves every output. No global state or retained FLINT group.
   Numerical p=max(prec,2); precision LIMIT precedes INV and allocation. */
#ifndef ADELEFELD_CHAR_H
#define ADELEFELD_CHAR_H

#include "adelefeld/text.h"
#include "adelefeld/psi.h"

#define ADF_CHAR_MOD_MAX 65536

#ifdef __cplusplus
extern "C" {
#endif

typedef struct { ulong q; ulong n; int parity; acb_t s; } adf_char_struct;
typedef adf_char_struct adf_char_t[1];
typedef adf_char_struct *adf_char_ptr;
typedef const adf_char_struct *adf_char_srcptr;

/* Principal (1,1,0,0); clear owns s. No status; acb initialization/clearing cost.
   Sources: api-3c 1; conventions 5.13. Only init is permitted after clear. */
void adf_char_init(adf_char_t x);
void adf_char_clear(adf_char_t x);
/* Exact copy; exchange all fields. Self-alias allowed. No status; copy cost / O(1).
   Sources: api-3c 1; conventions 4.2. */
void adf_char_set(adf_char_t y, const adf_char_t x);
void adf_char_swap(adf_char_t x, adf_char_t y);
/* Predicate: q>=1, 1<=n<=q, gcd=1, conductor=q, correct parity, finite s.
   Full word-sized predicate; never false merely for q above D1. Cost D(q), unbounded.
   Identity compares fields and acb_equal(s). Sources: api-3c 1; conventions 5.13. */
int adf_char_is_canonical(const adf_char_t x);
int adf_char_identical(const adf_char_t x, const adf_char_t y);
/* Binding layout, header-inline and exported; no status or allocation; O(1). */
ADF_INLINE size_t adf_sizeof_char(void) { return sizeof(adf_char_struct); }
ADF_INLINE size_t adf_alignof_char(void) { return ADF_ALIGNOF(adf_char_struct); }

/* Lower inducing (q,n) using FLINT char_lower, never n mod conductor (P3).
   Reduce n modulo q; q=1 accepts all n. Set s=0 or exactly copy finite supplied s.
   DOMAIN: q=0, gcd>1, nonfinite s. Then LIMIT: q>ADF_CHAR_MOD_MAX before setup.
   UNSUPPORTED: unavailable group setup; OK commits. Cost D(q)+D(C).
   Raw s must not alias x->s. Sources: api-3c 2/P3; dirichlet.h:68,90,110-136. */
int adf_char_set_conrey(adf_char_t x, ulong q, ulong n);
int adf_char_set_conrey_acb(adf_char_t x, ulong q, ulong n, const acb_t s);
/* Replace s by exact finite copy: OK/DOMAIN. Independent raw s; no D1/setup.
   Source: api-3c 2. Cost copy; failure preserves x. */
int adf_char_set_s(adf_char_t x, const acb_t s);
/* Exact scalar/copy reads; independent s output. No status/allocation except acb copy.
   Sources: api-3c 2; conventions 5.13. Cost O(1), or copying s. */
ulong adf_char_get_conductor(const adf_char_t x);
ulong adf_char_get_label(const adf_char_t x);
int adf_char_get_parity(const adf_char_t x);
void adf_char_get_s(acb_t s, const adf_char_t x);
/* Multiplicative character order, distinct from group exponent. OK/LIMIT/UNSUPPORTED.
   D1 before INV/setup; order untouched on failure. Source: api-3c 2. Cost D(q). */
int adf_char_get_order(ulong *order, const adf_char_t x);

/* Value form char(q=q, n=n, s=z(s)); lexical stages 8.5, q word bound then semantics/D1.
   Arbitrarily long unsigned n is reduced with fmpz. Lower the pair. No s sign condition.
   OK/PARSE/LIMIT/UNSUPPORTED/DOMAIN; precision cap first. Cost text conversion+D(q)+D(C).
   Source: api-3c 2; conventions 9,11.3. No member aliasing; failure preserves x. */
int adf_char_set_str(adf_char_t x, const char *s, size_t len, slong prec,
                     const adf_text_limits_t *lim);
/* Canonical value text, unconstrained decimal enclosure of both s coordinates (9.5).
   digits 1..10^6; flint-allocated result/free with adf_str_free. Byte length excludes NUL.
   NULL,*len=0 for ADF_PRINT_EXP_MAX refusal. Cost output conversion; no D1 limit.
   Re-reading encloses s; it need not be identical. Source: api-3c 2; conventions 11.3. */
char *adf_char_get_str(size_t *len, const adf_char_t x, slong digits);

/* chi(a), ignoring s. OK writes (zero=1,theta=0) for a nonunit, else (0,phase in [0,1)).
   C=1: (0,0) for all integers. Signed arbitrary fmpz reduction. LIMIT for D1 before INV;
   UNSUPPORTED for setup. Both outputs commit together. No member/output aliasing.
   Sources: api-3c 3/P3; dirichlet.h:51,139,162. Cost D(C)+reduction+one pairing. */
int adf_char_chi_phase(int *is_zero, fmpq_t theta, const adf_char_t chi, const fmpz_t a);
/* Same acb value through adf_phase_get_acb, zero exact. Precision cap first.
   OK/LIMIT/UNSUPPORTED/NOT_DETERMINED. Output independent of chi->s; failure preserves z.
   Source: api-3c 3; psi.h phase contract. Cost phase getter plus one phase evaluation. */
int adf_char_chi(acb_t z, const adf_char_t chi, const fmpz_t a, slong prec);
/* tau=sum chi(a) E(+a/C), stored primitive conductor, independent of s. C=1 gives 1.
   Bounds on precision/C before INV/setup; OK/LIMIT/UNSUPPORTED/NOT_DETERMINED.
   P4: exact dyadic endpoint accumulation, one rounding, radii <=8*C*2^-w,
   w=min(ADF_REAL_PREC_MAX,max(prec,2)+ceil(log2(C))+8). No output/member aliasing.
   Sources: api-3c 4/P4; analysis L8:295; refs/src/flint-3.0.1/acb_dirichlet.rst:358-378.
   Cost D(C)+O(C) pairings and phase calls; streaming temporaries O(w+log C) bits. */
int adf_char_gauss_sum(acb_t tau, const adf_char_t chi, slong prec);
/* W=tau/(i^parity sqrt(C)), positive sqrt. Same statuses/commit/aliasing/bounds.
   Certify coordinate radii <=32*C*2^-w, else NOT_DETERMINED. No real-character shortcut.
   Sources: api-3c 4/P4; analysis P13:564. Cost Gauss plus sqrt/division/exact rotation. */
int adf_char_root_number(acb_t W, const adf_char_t chi, slong prec);

#ifdef __cplusplus
}
#endif
#endif /* ADELEFELD_CHAR_H */
