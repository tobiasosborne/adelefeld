/* adelefeld/rfun.h: real test functions, finite sums of P(x) exp(-pi A x^2 + B x + C) (slice 4d).
   Contract: docs/api-4.md sections 1, 2, 5, 6 (slice 4d of section 9); docs/conventions.md 5.12
   (struct, predicate, init; CV-20 no normal form), 9.2 (rfun_v, rterm), 9.4 (template); docs/SPEC.md 7
   (real part, parameters kept as balls); docs/proofs/analysis.md Proposition 5 (closure formulas).
   Statements and decisions of this slice: docs/api-4b.md. Not in this header: the derivative, the
   transform, integrals and norms (slice 4e), the dump forms, adf_ffun and tensors.
   Not to be confused with adelefeld/rfunc.h (the real functions exp, log, ... on arb).

   Common contract (api-4.md section 1). Inputs and outputs are initialized; canonical inputs are
   preconditions except for the raw setter and the predicates (checked under INV). Same-type
   whole-object aliasing is allowed; member aliasing is forbidden. Every status other than ADF_OK leaves
   all outputs untouched: results are built in temporaries and swapped in at the end.
   Every arithmetic or evaluation call first rejects prec > ADF_REAL_PREC_MAX with ADF_LIMIT and works
   at p = max(prec, 2). Statuses of the row "Integrals, Poisson summation" of conventions 3.2 (D1):
   OK, DOMAIN, NOT_DETERMINED, LIMIT. NOT_DETERMINED: a nonfinite intermediate, or Re(A) > 0 of a
   result term not certified after rounding. LIMIT (D1, SPEC 15.4 N-D23): more than ADF_RFUN_TERMS_MAX
   terms or ADF_RFUN_COEFFS_MAX coefficients in an input or a result, more than ADF_RFUN_WORK_MAX work
   units, or an exact integer of more than ADF_RFUN_BITS_MAX bits; checked before any output is written.
   One work unit is one dense polynomial coefficient multiply-add. */
#ifndef ADELEFELD_RFUN_H
#define ADELEFELD_RFUN_H

#include "adelefeld/common.h"
#include "adelefeld/status.h"
#include "adelefeld/rat.h"
#include "adelefeld/idele.h"
#include "adelefeld/text.h"
#include <flint/acb.h>
#include <flint/acb_poly.h>

#ifndef ADF_REAL_PREC_MAX
#define ADF_REAL_PREC_MAX 2097152
#endif

/* The caps of decision D1 (docs/api-4.md section 1; SPEC 15.4 N-D23). */
#define ADF_RFUN_TERMS_MAX ((slong) 65536)
#define ADF_RFUN_COEFFS_MAX ((slong) 65536)
#define ADF_RFUN_WORK_MAX ((slong) 1048576)
#define ADF_RFUN_BITS_MAX ((slong) 1048576)

#ifdef __cplusplus
extern "C" {
#endif

/* One term P(x) exp(-pi A x^2 + B x + C) and the sum of len terms (conventions 5.12). */
typedef struct
{
    acb_poly_t P;
    acb_t A, B, C;
} adf_rterm_struct;

typedef adf_rterm_struct adf_rterm_t[1];
typedef adf_rterm_struct * adf_rterm_ptr;
typedef const adf_rterm_struct * adf_rterm_srcptr;

typedef struct
{
    slong len;
    adf_rterm_struct * term;
} adf_rfun_struct;

typedef adf_rfun_struct adf_rfun_t[1];
typedef adf_rfun_struct * adf_rfun_ptr;
typedef const adf_rfun_struct * adf_rfun_srcptr;

/* Init: len = 0, term = NULL, the zero function (conventions 5.12). No status, O(1). */
void adf_rfun_init(adf_rfun_t x);

/* Release every term (acb_poly_clear, acb_clear) and the array (flint_free); afterwards only init
   is permitted. O(total size). */
void adf_rfun_clear(adf_rfun_t x);

/* Exact deep copy, no rounding; y may equal x. Cost: input size. */
void adf_rfun_set(adf_rfun_t y, const adf_rfun_t x);

/* O(1) exchange; x may equal y. */
void adf_rfun_swap(adf_rfun_t x, adf_rfun_t y);

/* 1 exactly for the predicate of conventions 5.12: len >= 0; term != NULL when len > 0; every P
   normalized (no exact-zero last coefficient) with finite coefficients; A, B, C finite;
   arb_is_positive(Re A). Never aborts for initialized readable storage. O(total size). */
int adf_rfun_is_canonical(const adf_rfun_t x);

/* 1 iff same len and, term by term, acb_poly_equal(P) and acb_equal(A, B, C): representation
   identity, not function equality. No status, O(input size). */
int adf_rfun_identical(const adf_rfun_t x, const adf_rfun_t y);

/* Raw deep-copy setter (api-4.md section 1). LIMIT first: n > ADF_RFUN_TERMS_MAX, decided before any
   term is read, or more than ADF_RFUN_COEFFS_MAX coefficients in all; DOMAIN: n < 0, terms == NULL with
   n > 0, a P that is not normalized or has a nonfinite coefficient, a nonfinite A, B or C, or Re(A) not
   certified positive. No prec, no rounding. terms must not overlap y's storage. Cost: total entries. */
int adf_rfun_set_terms(adf_rfun_t y, const adf_rterm_struct * terms, slong n);

/* Value reader (api-4.md section 2; conventions 8.5, 9.2 rfun_v, 9.3): stages 1 to 7 in order. Stage 4
   compares the number of terms and the length of every coefficient list with lim->max_items; stage 6
   (DOMAIN): the exact decimal interval of Re(A) is not in (0, infinity); stage 7 (NOT_DETERMINED): the
   enclosure of a positive Re(A) at prec is not certified positive. Exact trailing zero coefficients are
   removed (conventions 5.12); terms are neither combined nor sorted. Every status other than OK leaves
   x untouched. O(text + represented entries). */
int adf_rfun_set_str(adf_rfun_t x, const char * s, size_t len, slong prec, const adf_text_limits_t * lim);

/* Canonical value text rfun(term(P=[z(c_0), ...], A=z(A), B=z(B), C=z(C)), ...) (conventions 9.4),
   Re(A) printed with the constraint "positive" of conventions 9.5. Rules of the printers of text.h:
   caller frees with adf_str_free; NULL and *len = 0 on the exponent bound M1-D6 or the work bound N-D11.
   Cost: printed size plus decimal conversion. */
char * adf_rfun_get_str(size_t * len, const adf_rfun_t x, slong digits);

/* z = x + y: the terms of x, then those of y (api-4.md section 5). Exact copies, no rounding. OK or
   LIMIT. Cost: total size. */
int adf_rfun_add(adf_rfun_t z, const adf_rfun_t x, const adf_rfun_t y, slong prec);

/* z = x * y: for i over x, then j over y (lexicographic), the term P_i Q_j, A_i + A'_j, B_i + B'_j,
   C_i + C'_j (analysis Proposition 5). OK, LIMIT, NOT_DETERMINED. Cost: sum of dense products. */
int adf_rfun_mul(adf_rfun_t z, const adf_rfun_t x, const adf_rfun_t y, slong prec);

/* y(x) = x(t - q): P(t - q), A, B + 2 pi A q, C - B q - pi A q^2 (analysis Proposition 5). q = n/d is
   applied as multiplication by n then division by d. OK, LIMIT, NOT_DETERMINED. O(sum (deg+1)^2). */
int adf_rfun_translate_rat(adf_rfun_t y, const adf_rfun_t x, const adf_rat_t q, slong prec);

/* y(t) = x(h t): P(h t), A h^2, B h, C (analysis Proposition 5). DOMAIN for h = 0, also for the zero
   function. OK, DOMAIN, LIMIT, NOT_DETERMINED. O(total coefficients). */
int adf_rfun_dilate_rat(adf_rfun_t y, const adf_rfun_t x, const adf_rat_t h, slong prec);

/* As dilate_rat with h = the real component a->inf of the idele (a ball that excludes 0), h^2 by
   interval squaring. Only the real component is used. OK, LIMIT, NOT_DETERMINED. */
int adf_rfun_dilate_idele(adf_rfun_t y, const adf_rfun_t x, const adf_idele_t a, slong prec);

/* y(t) = x(-t): odd coefficients and B change sign, exactly. OK or LIMIT. O(total size). */
int adf_rfun_reflect(adf_rfun_t y, const adf_rfun_t x);

/* y = conj(x) pointwise on real t: every coefficient and A, B, C conjugated, exactly. OK or LIMIT. */
int adf_rfun_conj(adf_rfun_t y, const adf_rfun_t x);

/* z encloses phi(t) for every real t in the ball x and every member of phi (api-4.md section 6):
   Horner, exponential and term addition in acb. DOMAIN for a nonfinite x; OK, LIMIT, NOT_DETERMINED.
   Cost: total coefficients acb operations plus one exp per term. */
int adf_rfun_eval(acb_t z, const adf_rfun_t phi, const arb_t x, slong prec);

/* Layout queries (api-4.md section 1; conventions 12.4). Header-inline and exported; O(1). */
ADF_INLINE size_t adf_sizeof_rterm(void) { return sizeof(adf_rterm_struct); }
ADF_INLINE size_t adf_alignof_rterm(void) { return ADF_ALIGNOF(adf_rterm_struct); }
ADF_INLINE size_t adf_sizeof_rfun(void) { return sizeof(adf_rfun_struct); }
ADF_INLINE size_t adf_alignof_rfun(void) { return ADF_ALIGNOF(adf_rfun_struct); }

#ifdef __cplusplus
}
#endif

#endif /* ADELEFELD_RFUN_H */
