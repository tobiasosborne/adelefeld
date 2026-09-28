/* adelefeld/scaled.h: the precision policies of SPEC 4.4 other than the tight one: the scaled
   residue policy (adf_scaled) and the absolute cap (on adf_fball).

   Contract: docs/conventions.md 0.4, 5.4 (struct, predicate, operations table, adf_scaled_set_context
   of closure finding C5, the cap), 4.6 (shared-pointer rule, closure edit E1), 3.2 (rows "Ring
   arithmetic of adf_scaled", "Conversion between scaled contexts", "Conversion from tight to
   scaled"), 4.1, 4.3; docs/SPEC.md 4.4 ("Context compatibility for scaled values"); docs/proofs/
   policies.md section 2 (Definition 4 to Corollary 12) and section 3 (Definition 13 to Proposition
   15); docs/proofs/precision.md Propositions 5, 6. Reference: tests/ref/adfref/policies.py. Implemented
   in work package 1.7 (docs/PLAN.md section 6).

   Meaning of an adf_scaled value (conventions 5.4): exact = 0: the set s (u + K Zhat) = s u + s K Zhat,
   with s > 0 rational, 0 <= u < K, K the modulus of the borrowed context; exact = 1: the rational s
   (any sign, 0 included), u = 0. Exact values keep their exact tag (policies Definition 4).

   Context rule (conventions 4.6, 5.4; SPEC 4.4; closure E1). A default binary operation (add, sub,
   mul, mul_tight) requires the same context pointer in both inputs, exact inputs included, and
   compares the two inputs, not the old context of the initialised output. Otherwise it returns
   ADF_DOMAIN and leaves the output untouched; the check precedes every write, aliased writes
   included. The result borrows that context. Unary and exact-scalar operations borrow the context of
   their scaled input; an adf_rat scalar has no context. To combine contexts K, K' the caller
   constructs the context lcm(K, K') (adelefeld/modctx.h), converts both operands with
   adf_scaled_set_context (lossless there, policies Corollary 12, line 241) and calls the ordinary
   operation. No function here creates a context.

   Common rules, unless a comment says otherwise: an output may be the same object as an input of the
   same type (conventions 4.1); inputs satisfy the predicate of conventions 5.4; outputs untouched on
   a status other than ADF_OK (conventions 4.3); every result contains the tight result of the same
   operation on the same sets (policies Theorem 3, line 75). */

#ifndef ADELEFELD_SCALED_H
#define ADELEFELD_SCALED_H

#include "adelefeld/common.h"
#include "adelefeld/status.h"
#include "adelefeld/rat.h"
#include "adelefeld/fball.h"
#include "adelefeld/modctx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Layout (conventions 5.4, 12.4), 64-bit: s (fmpq, 16 bytes) at 0, u (fmpz) at 16, mctx at 24,
   exact (int) at 32; size 40, alignment 8.
   Predicate (conventions 5.4): mctx != NULL, exact in {0, 1}, fmpq_is_canonical(s); if exact = 0:
   s > 0 and 0 <= u < K; if exact = 1: u = 0. The data (s, u) are determined by the set (policies
   Lemma 5, line 113). */
typedef struct
{
    fmpq_t s;
    fmpz_t u;
    const adf_modctx_struct * mctx;
    int exact;
} adf_scaled_struct;

typedef adf_scaled_struct adf_scaled_t[1];
typedef adf_scaled_struct * adf_scaled_ptr;
typedef const adf_scaled_struct * adf_scaled_srcptr;

/* ---- life cycle (conventions 2.3) ---- */

/* adf_scaled_init(x, ctx): x = the exact 0 in ctx (conventions 2.3, 5.4). ctx must come from a
   successful constructor and outlive x. Never fails. */
void adf_scaled_init(adf_scaled_t x, const adf_modctx_struct * ctx);

/* adf_scaled_clear(x): releases s and u; never touches the context. */
void adf_scaled_clear(adf_scaled_t x);

/* adf_scaled_set(y, x): y = x, including the context pointer (conventions 2.3). y may be x. */
void adf_scaled_set(adf_scaled_t y, const adf_scaled_t x);

/* adf_scaled_swap(x, y): exchanges contents and context pointers; O(1). */
void adf_scaled_swap(adf_scaled_t x, adf_scaled_t y);

/* adf_scaled_is_canonical(x): 1 if the predicate above holds, else 0; never aborts for an initialised
   object whose mctx is NULL or a live context (decision M1-D2) (a NULL mctx
   gives 0). */
int adf_scaled_is_canonical(const adf_scaled_t x);

/* adf_scaled_identical(x, y): 1 if same context pointer, same exact tag, equal s and u, else 0. */
int adf_scaled_identical(const adf_scaled_t x, const adf_scaled_t y);

/* adf_scaled_context(x): the borrowed context, never NULL. adf_scaled_is_exact(x): the exact tag. */
const adf_modctx_struct * adf_scaled_context(const adf_scaled_t x);
int adf_scaled_is_exact(const adf_scaled_t x);

/* ---- conversions ---- */

/* adf_scaled_set_rat(y, q, ctx): y = the exact value q in ctx: exact = 1, s = q, u = 0. Never fails. */
void adf_scaled_set_rat(adf_scaled_t y, const adf_rat_t q, const adf_modctx_struct * ctx);

/* adf_scaled_set_fball(y, lost, x, ctx): conversion from a tight ball x = c + R Zhat into ctx of
   modulus K (conventions 5.4 row "conversion from a tight ball"; policies Proposition 7, line 139,
   and Lemma 6, line 124). R > 0: s* = gcd(c, R/K), u* = (c/s*) mod K, the best enclosure of x among
   the scaled values of ctx; *lost = 1 exactly when c K/R is not an integer (the set changes).
   R = 0: exact = 1, s = c, u = 0 in ctx, *lost = 0 (closure C5). lost may be NULL. y borrows ctx.
   Status: ADF_OK always (conventions 3.2 row "Conversion from tight to scaled"). The int return
   matches adf_scaled_set_context of closure C5. Word blocks are not needed. Cost: one gcd of
   rationals and one reduction modulo K. */
int adf_scaled_set_fball(adf_scaled_t y, int * lost, const adf_fball_t x, const adf_modctx_struct * ctx);

/* adf_scaled_set_context(y, lost, x, ctx): closure finding C5, verbatim in conventions 5.4:
   "With initialized y and a successfully constructed ctx, this returns ADF_OK and stores the best
   enclosure in ctx. If lost is non-NULL, write *lost=1 exactly when the set changes. Word blocks are
   optional. Exact input keeps its rational s, exact=1 and u=0, with *lost=0 if requested. y may
   alias x; it borrows ctx." Formula (policies Proposition 11, line 218): for x = s (u + K Zhat) and
   ctx of modulus K', s' = s gcd(u, K/K'), u' = (s u/s') mod K'; exact (lost = 0) exactly when
   u K'/K is an integer, always when K divides K'. Status: ADF_OK always. */
int adf_scaled_set_context(adf_scaled_t y, int * lost, const adf_scaled_t x, const adf_modctx_struct * ctx);

/* adf_scaled_get_fball(y, x): y = the set of x as a global canonical adf_fball: s u + s K Zhat, or
   the exact s (conventions 9.4 row "scaled value"). Exact as a set; never fails. */
void adf_scaled_get_fball(adf_fball_t y, const adf_scaled_t x);

/* ---- arithmetic at one context (conventions 5.4 operations table) ---- */

/* adf_scaled_add(z, x, y): both scaled: with g = gcd(s, t), A = s/g, B = t/g,
   z = g ((A u + B v) mod K + K Zhat), tight (precision.md Proposition 5(1), line 94; SPEC 4.4).
   One exact operand q: z = x + q by policies Proposition 8 (line 159), the best scaled enclosure of
   the tight sum. Both exact: the exact sum, exact = 1 (policies Definition 4).
   Status: ADF_OK; ADF_DOMAIN if x and y have different context pointers (z untouched).
   Cost: a gcd of rationals and a reduction modulo K. */
int adf_scaled_add(adf_scaled_t z, const adf_scaled_t x, const adf_scaled_t y);

/* adf_scaled_sub(z, x, y): z = x + (-y), with adf_scaled_neg; statuses as adf_scaled_add. */
int adf_scaled_sub(adf_scaled_t z, const adf_scaled_t x, const adf_scaled_t y);

/* adf_scaled_mul(z, x, y): the default product (DECISION CV-48, D5): both scaled:
   z = s t ((u v mod K) + K Zhat), an enclosure of the tight product that loses the factor
   h = gcd(u, v, K) (precision.md Proposition 5(2); policies Proposition 10.2, line 193). One exact
   operand q: q x by policies Proposition 9 (line 184), exact as a set; q = 0 gives the exact 0.
   Both exact: the exact product. Status: ADF_OK; ADF_DOMAIN on different context pointers. */
int adf_scaled_mul(adf_scaled_t z, const adf_scaled_t x, const adf_scaled_t y);

/* adf_scaled_mul_tight(z, x, y): both scaled: z = (s t h) (((u v / h) mod K) + K Zhat),
   h = gcd(u, v, K), equal to the tight product (policies Proposition 10.3). Exact operands as in
   adf_scaled_mul. The shared-pointer check applies, exact operands included (closure E1).
   Status: ADF_OK; ADF_DOMAIN on different context pointers. Cost: an integer gcd at the size of K
   and an exact division. */
int adf_scaled_mul_tight(adf_scaled_t z, const adf_scaled_t x, const adf_scaled_t y);

/* adf_scaled_neg(y, x): y = -x: s ((-u) mod K + K Zhat), or the exact -s; exact as a set
   (tests/ref/adfref/policies.py scaled_neg; Proposition 9 with q = -1). Borrows x's context. */
void adf_scaled_neg(adf_scaled_t y, const adf_scaled_t x);

/* adf_scaled_mul_rat(y, x, q): y = q x: q != 0: |q| s ((sign(q) u mod K) + K Zhat), exact as a set;
   q = 0: the exact 0 (policies Proposition 9, line 184). Borrows x's context. */
void adf_scaled_mul_rat(adf_scaled_t y, const adf_scaled_t x, const adf_rat_t q);

/* adf_scaled_add_rat(y, lost, x, q): y = x + q (policies Proposition 8, line 159): q = 0: y = x;
   else g = gcd(s, q), y = g ((q/g + (s/g) u) mod K + K Zhat), the best scaled enclosure of the tight
   sum; *lost = 1 exactly when s does not divide q. x exact: the exact sum, *lost = 0. lost may be
   NULL. Borrows x's context. */
void adf_scaled_add_rat(adf_scaled_t y, int * lost, const adf_scaled_t x, const adf_rat_t q);

/* Layout queries (conventions 12.4, CV-40). Header-inline and exported. */
ADF_INLINE size_t adf_sizeof_scaled(void) { return sizeof(adf_scaled_struct); }
ADF_INLINE size_t adf_alignof_scaled(void) { return ADF_ALIGNOF(adf_scaled_struct); }

/* ---- the absolute cap (SPEC 4.4 item 3; conventions 5.4, CV-23, CV-47) ----
   Values are plain adf_fball; the cap C, a positive rational, is an argument. After the tight
   operation with result c + R Zhat, the radius R > 0 becomes gcd(R, C) (policies Definition 13,
   line 253; Proposition 14.1, line 258: an enclosure, the best among radii dividing C, P14.3). An
   exact result (R = 0) keeps its tag; the cap never touches it (CV-47, D2; P14.2). Aliasing as in
   adelefeld/fball.h. Status: ADF_OK; ADF_DOMAIN if C <= 0, output untouched (conventions 3.2 has no
   row for capped operations; the choice is recorded in docs/api-m1.md, section "Choices").
   Cost: the tight operation plus one gcd of rationals. */

/* adf_fball_cap(y, x, C): y = x with its radius capped. */
int adf_fball_cap(adf_fball_t y, const adf_fball_t x, const adf_rat_t C);

/* adf_fball_add_cap, adf_fball_sub_cap, adf_fball_mul_cap: the tight operation of fball.h, then the
   cap. adf_fball_mul_rat_cap: adf_fball_mul_rat, then the cap (the cap acts on the product of an
   exact scalar with a ball, P14.2). */
int adf_fball_add_cap(adf_fball_t z, const adf_fball_t x, const adf_fball_t y, const adf_rat_t C);
int adf_fball_sub_cap(adf_fball_t z, const adf_fball_t x, const adf_fball_t y, const adf_rat_t C);
int adf_fball_mul_cap(adf_fball_t z, const adf_fball_t x, const adf_fball_t y, const adf_rat_t C);
int adf_fball_mul_rat_cap(adf_fball_t y, const adf_fball_t x, const adf_rat_t q, const adf_rat_t C);

#ifdef __cplusplus
}
#endif

#endif /* ADELEFELD_SCALED_H */
