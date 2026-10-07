/* adelefeld/ffun.h: finite test functions, slice 4a.
   Contract: docs/api-4.md sections 1-4, F1-F4; docs/conventions.md 5.11, 6.3 (CV-20).
   Initialized canonical inputs are required except raw setters and the predicate. Storage is owned.
   f[j] is the value on j/D + M Zhat; zero outside (1/D) Zhat. No normal form is imposed.
   Whole-object aliasing is allowed. Member aliasing is forbidden. Failure preserves every output.
   Precision and size preflight precede debug entry checks and output allocation. */
#ifndef ADELEFELD_FFUN_H
#define ADELEFELD_FFUN_H
#include "adelefeld/common.h"
#include "adelefeld/status.h"
#include "adelefeld/text.h"
#include <flint/acb.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct { ulong D, M; acb_ptr f; } adf_ffun_struct;
typedef adf_ffun_struct adf_ffun_t[1];
typedef adf_ffun_struct *adf_ffun_ptr;
typedef const adf_ffun_struct *adf_ffun_srcptr;
#define ADF_FFUN_ITEMS_MAX ((ulong) 1048576)
#define ADF_FFUN_BITS_MAX ((slong) 1048576)
#ifndef ADF_REAL_PREC_MAX
#define ADF_REAL_PREC_MAX 2097152
#endif

/* Init (1,1,[0]); clear releases DM live entries and owned storage. No status.
   Init O(1); clear O(DM). After clear only init is permitted. */
void adf_ffun_init(adf_ffun_t x);
void adf_ffun_clear(adf_ffun_t x);
/* Exact deep copy, including radii, O(DM); self-copy permitted. Never fails. */
void adf_ffun_set(adf_ffun_t y, const adf_ffun_t x);
/* O(1) exchange; self-swap permitted. Never fails. */
void adf_ffun_swap(adf_ffun_t x, adf_ffun_t y);
/* Full predicate: D,M>=1, DM<2^62, non-NULL owned storage, finite entries.
   Live capacity for DM entries is a caller precondition. No cap restriction, O(DM), never aborts. */
int adf_ffun_is_canonical(const adf_ffun_t x);
/* Representation identity: same D,M and acb_equal entries, O(DM). No equality of functions. */
int adf_ffun_identical(const adf_ffun_t x, const adf_ffun_t y);
/* Header-inline and exported ABI queries; O(1), no status. */
ADF_INLINE size_t adf_sizeof_ffun(void) { return sizeof(adf_ffun_struct); }
ADF_INLINE size_t adf_alignof_ffun(void) { return ADF_ALIGNOF(adf_ffun_struct); }
/* Raw exact deep copy. DOMAIN for invalid shape, NULL storage or nonfinite fields;
   LIMIT for D1 length/byte caps before allocation. OK commits. Cost O(DM), no rounding. */
int adf_ffun_set_acb_vec(adf_ffun_t y, ulong D, ulong M, acb_srcptr f, slong n);
/* Full grammar, then caller text limits, then domain (conventions 8.5/9.3).
   OK commits; PARSE, LIMIT, UNSUPPORTED, DOMAIN, NOT_DETERMINED preserve x.
   O(text + entries); values rounded outward at max(prec,2). No bare scalar entries. */
int adf_ffun_set_str(adf_ffun_t x, const char *s, size_t len, slong prec,
                     const adf_text_limits_t *lim);
/* Canonical value text, decimal rules and refusal of text.h. Caller adf_str_free.
   *len excludes NUL; NULL and *len=0 on refusal. O(printed size + decimal conversion). */
char *adf_ffun_get_str(size_t *len, const adf_ffun_t x, slong digits);
/* F1: DOMAIN unless D2,M2>=1 and divisibility holds. LIMIT for D1; OK copies and zeros.
   Cost O(D2 M2), including zeros. Repetition of balls loses correlations. */
int adf_ffun_refine(adf_ffun_t y, const adf_ffun_t x, ulong D2, ulong M2);
/* F1 common refinement by lcm(D),lcm(M), then acb addition at max(prec,2).
   OK, LIMIT, NOT_DETERMINED; O(lcm(D) lcm(M)); output aliases either or both inputs. */
int adf_ffun_add(adf_ffun_t z, const adf_ffun_t x, const adf_ffun_t y, slong prec);
/* F4, analysis P4: (M,D), g[k]=(1/M) sum_j f[j] E(-jk/(DM)).
   Direct certified acb summation; every product, addition and division rounds outward.
   OK encloses all members; LIMIT for D1 (also (DM)^2<=2^20); NOT_DETERMINED for phase
   or nonfinite result failure. O((DM)^2) operations, O(DM) storage. */
int adf_ffun_fourier(adf_ffun_t y, const adf_ffun_t x, slong prec);
/* Slice 4b, docs/api-4.md section 3, F1-F3. Common lifecycle/alias rules above apply. */
#include "adelefeld/idele.h"
/* Common F1 refinement, then acb multiplication at max(prec,2).
   OK, LIMIT, NOT_DETERMINED. O(lcm(D)*lcm(M)); repeated balls lose correlations. */
int adf_ffun_mul(adf_ffun_t z, const adf_ffun_t x, const adf_ffun_t y, slong prec);
/* F2: f(x-q). Refine D by den(q), then read k-D2*q modulo D2*M.
   Exact copies; OK or LIMIT; O(D2*M). */
int adf_ffun_translate_rat(adf_ffun_t y, const adf_ffun_t x, const adf_rat_t q);
/* F2: f(-x), reading -k modulo DM. Exact copies; OK or LIMIT; O(DM). */
int adf_ffun_reflect(adf_ffun_t y, const adf_ffun_t x);
/* F3: f(q*x), q=s/t reduced. Layout (|s|D,tM); zero unless t divides k, then
   f[sign(s)*k/t mod DM]. Exact copies, O(new length). DOMAIN for q=0; LIMIT for D1; otherwise OK. */
int adf_ffun_dilate_rat(adf_ffun_t y, const adf_ffun_t x, const adf_rat_t q);
/* Finite part only, ignores a.inf. F3 singleton unit-image certificate modulo DM;
   non-singleton gives NOT_DETERMINED even for invariant arrays. Content is rational dilation.
   OK/NOT_DETERMINED/LIMIT, O(DM+new length). D1 charges enumeration and copied cells together. */
int adf_ffun_dilate_idele(adf_ffun_t y, const adf_ffun_t x, const adf_idele_t a);
/* Exact pointwise complex conjugate. OK or LIMIT; O(DM). */
int adf_ffun_conj(adf_ffun_t y, const adf_ffun_t x);
#ifdef __cplusplus
}
#endif
#endif /* ADELEFELD_FFUN_H */
