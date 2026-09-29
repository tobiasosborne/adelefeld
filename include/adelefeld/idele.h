/* adelefeld/idele.h: adf_idele, an idele of Q to finite precision: a real ball that excludes 0, an
   exact positive rational (the content) and a unit coset.

   Contract: docs/SPEC.md 5 ("Decomposition", the table of types, "Sign preservation"); docs/conventions.md
   5.7 (struct, predicate, init, the idele of a rational, the name "content", the result sign check of
   gate finding G5), 2.2, 2.3, 3.2 (row "Unit coset, idele, idele class arithmetic"), 4.1, 4.3, 4.4;
   decisions M0-D1, M1-D4, and D2-1, D2-2 of lane i-slice1's brief (results of operations stored in normal
   form; the real kernel works by end points and returns NOT_DETERMINED when it cannot certify a result
   free of 0). Proofs: docs/proofs/ideles.md Proposition 3 (line 67), Propositions 10 and 11 (lines 195,
   213); docs/api-2.md 1.3, Statements D (the set of a value, product, inverse, the idele of a rational)
   and E (the real kernel). Implemented in src/idele.c (milestone 2, slice 1); tests tests/test_idele.c.
   Slice 2 (lane i-slice2) adds mul_rat, valuation_at, abs_at, abs_inf and norm (ideles.md P14, line 343;
   api-2.md 2.3, 2.4, Statements F, H, I); tests tests/test_idele_maps.c. The class map is in
   adelefeld/idclass.h.

   Meaning (api-2.md Statement D): the value (X, r, u) is the set of ideles (xi, r w) with xi in the
   closed interval X = [m - rho, m + rho] of the real ball inf and w in the set of the unit coset u
   (adelefeld/ucoset.h). Every finite idele is r w for exactly one positive rational r and one unit w of
   Zhat (ideles.md P3), so the content r and the unit are exact data; only the real part is a ball.

   Predicate (is_canonical, conventions 5.7): arb_is_finite(inf) and arb_is_nonzero(inf) (the ball
   excludes 0; refs/src/flint-3.0.1/arb.rst:597, :606); fmpq_is_canonical(r) and r > 0;
   adf_ucoset_is_canonical(u). Init: the exact idele 1: inf = 1 exact, r = 1, u = [1].

   The real kernel of mul and inv (api-2.md Statement E; SPEC 5 "Sign preservation", G5): it does NOT
   call arb_mul or arb_inv and then test the sign; the ball product of arb can contain 0 when the
   product set does not (x = y = 1 +/- (1 - 2^-30), SPEC 5). It computes rounded end points of the
   absolute value, lo <= |result| <= hi, with directed rounding at p = max(prec, 2) bits (E1 to E3),
   and then a ball from them (kernel B, E5):
     B1  e(hi) - e(lo) > p          -> ADF_NOT_DETERMINED, output untouched
     B2  lo = hi                    -> the exact ball sign * lo
     B3  m = RN_p((lo + hi)/2), rho >= max(hi - m, m - lo); if m > rho -> [sign m +/- rho]
     B4  otherwise rho' >= (hi - lo)/2, m' = lo + rho' exactly -> [sign m' +/- rho'] (lower end lo)
   e(t) is FLINT's exponent ARF_EXP: 2^(e(t)-1) <= |t| < 2^e(t). The result contains every product
   (every inverse), excludes 0, and its midpoint has at most 2 p + 30 bits. The status is a function of
   the inputs and p (E6): OK when hi < 2^(p-1) lo, NOT_DETERMINED when hi >= 2^(p+1) lo; in between it
   is decided by the exponents as above. Exact inputs whose exact result fits in p bits give an exact
   result.

   Common rules for every function below, unless its comment says otherwise:
   - Aliasing (conventions 4.1): an output may be the same object as any input of this type; inputs
     may alias each other; an output never aliases a part of an input (&x->u, x->inf, x->r).
   - Inputs satisfy the predicate; otherwise undefined (conventions 4.4, CV-09). With
     -DADF_CHECK_INVARIANTS every function that reads a value (an idele, a unit coset, a rational)
     checks it on entry and aborts, except is_canonical (M1-D2); clear and swap do not read one.
   - A function that returns a status leaves every output untouched on a status other than ADF_OK
     (conventions 4.3): it computes into temporaries and swaps at the end.
   - prec is the real working precision in bits; a prec below 2 is taken as 2 (M1-D4). The finite
     part (content and unit) does not depend on prec.
   - A prec above ADF_IDELE_PREC_MAX gives ADF_LIMIT, decided from prec alone before any allocation,
     every output untouched; it takes precedence over every other status of the function (the maximum
     of conventions 3.3). Decision of the orchestrator, 2026-09-30, after lane i-review1 (a prec of
     LONG_MAX made FLINT try to allocate 2^63 bits in inv and set_rat). The same rule holds for every
     function of adelefeld/idclass.h that takes a prec. */

#ifndef ADELEFELD_IDELE_H
#define ADELEFELD_IDELE_H

#include <flint/arb.h>

#include "adelefeld/common.h"
#include "adelefeld/status.h"
#include "adelefeld/rat.h"
#include "adelefeld/place.h"
#include "adelefeld/ucoset.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The largest working precision of the functions of this header and of adelefeld/idclass.h, 2^21 bits
   (the value of ADF_ROOTS_REAL_PREC_MAX of adelefeld/roots.h). Above it: ADF_LIMIT (the common rules
   above). */
#define ADF_IDELE_PREC_MAX 2097152

/* Layout (conventions 5.7, 12.4, 12.11), 64-bit: inf (arb_struct, 48 bytes) at 0, r (fmpq, 16 bytes)
   at 48, u (adf_ucoset_struct, 16 bytes) at 64; size 80, alignment 8. The field name r is not part of
   the contract (conventions 5.7); its accessor is adf_idele_content. */
typedef struct
{
    arb_t inf;
    fmpq_t r;
    adf_ucoset_struct u;
} adf_idele_struct;

typedef adf_idele_struct adf_idele_t[1];
typedef adf_idele_struct * adf_idele_ptr;
typedef const adf_idele_struct * adf_idele_srcptr;

/* ---- life cycle (conventions 2.3) ---- */

/* adf_idele_init(x): x = the exact idele 1 (inf = 1 exact, r = 1, u = [1]). Never fails. */
void adf_idele_init(adf_idele_t x);

/* adf_idele_clear(x): releases the memory; afterwards x may only be passed to init. */
void adf_idele_clear(adf_idele_t x);

/* adf_idele_set(y, x): y becomes a copy of x (the real ball as it is, the stored unit pair as it is).
   y may be x. */
void adf_idele_set(adf_idele_t y, const adf_idele_t x);

/* adf_idele_swap(x, y): exchanges the contents; O(1), no allocation. */
void adf_idele_swap(adf_idele_t x, adf_idele_t y);

/* adf_idele_is_canonical(x): 1 if the predicate above holds, else 0. Never aborts for an initialised
   object whose fields hold any values (decision M1-D2); a zero denominator of r gives 0. */
int adf_idele_is_canonical(const adf_idele_t x);

/* adf_idele_identical(x, y): 1 if arb_equal(x->inf, y->inf) (same midpoint and radius), the contents
   are equal and adf_ucoset_identical(&x->u, &y->u), else 0 (conventions 2.3, CV-02). Not a comparison
   of points or of sets. */
int adf_idele_identical(const adf_idele_t x, const adf_idele_t y);

/* ---- constructors ---- */

/* adf_idele_set_parts(x, inf, r, u): x = (inf, r, u), from raw data (conventions 3.2: constructors
   from raw data give OK, DOMAIN; 4.4).
   r may be any fmpq with a non-zero denominator; it is stored canonicalised (6/4 becomes 3/2). u is
   copied as it is: its modulus is kept as supplied (CV-17, D2-1).
   Status: ADF_OK, x written; ADF_DOMAIN, x untouched, if inf is not finite (arb_is_finite = 0), or inf
   contains 0 (arb_is_nonzero = 0), or the denominator of r is 0, or r <= 0.
   u must be canonical (precondition; it is a value of the library). Aliasing: inf, r and u are of
   other types than x and must not be parts of x. Cost: copies and one canonicalisation. */
int adf_idele_set_parts(adf_idele_t x, const arb_t inf, const fmpq_t r, const adf_ucoset_t u);

/* adf_idele_set_rat(x, q, prec): x = the idele of the exact rational q (conventions 5.7): a real ball
   that contains q and excludes 0, the content |q| and the exact unit [sign(q)] (ideles.md P3.4, line 80;
   M0-D1; api-2.md D.3). The real ball is kernel B on lo = RD_p(|q|), hi = RU_p(|q|) (E4), so it is
   the exact q when q is a dyadic number of at most p bits, and NOT_DETERMINED cannot occur.
   Status (conventions 3.2, NOT_UNIT for an exact zero input): ADF_OK, x written; ADF_NOT_UNIT if
   q = 0, x untouched; ADF_LIMIT if prec > ADF_IDELE_PREC_MAX (also for q = 0), x untouched.
   Aliasing: q is an adf_rat and not part of x. Cost: two divisions at p bits. */
int adf_idele_set_rat(adf_idele_t x, const adf_rat_t q, slong prec);

/* ---- arithmetic (SPEC 5: componentwise) ---- */

/* adf_idele_mul(z, x, y, prec): z = (Z, r s, u u') for x = (X, r, u), y = (Y, s, u'): every product of
   a point of x and a point of y lies in z (api-2.md D.1). The content r s is exact; the unit is
   adf_ucoset_mul (normal form; ideles.md P10, P11); Z is kernel B on the end points of E2, with the
   sign sign(X) sign(Y). The finite part is the product set itself (D.4).
   Status (conventions 3.2): ADF_OK, z written; ADF_NOT_DETERMINED (B1: e(hi) - e(lo) > p) if the real
   part cannot be certified free of 0 at p bits, z untouched; ADF_LIMIT if prec > ADF_IDELE_PREC_MAX,
   z untouched. Never ADF_NOT_UNIT.
   z may be x or y or both. Cost: two products at p bits for the end points, the kernel, a product of
   rationals and adf_ucoset_mul. */
int adf_idele_mul(adf_idele_t z, const adf_idele_t x, const adf_idele_t y, slong prec);

/* adf_idele_inv(y, x, prec): y = (Z, 1/r, u^-1) for x = (X, r, u): the inverse of every point of x
   lies in y (api-2.md D.2). The unit is adf_ucoset_inv (normal form; ideles.md P10.2); Z is kernel B
   on the end points of E3, with the sign of X.
   Status: ADF_OK, y written; ADF_NOT_DETERMINED (B1) if the real part cannot be certified free of 0 at
   p bits, y untouched; ADF_LIMIT if prec > ADF_IDELE_PREC_MAX, y untouched. y may be x. x * x^-1
   contains the idele 1 (its unit is U(N'), N' the normal modulus, which contains 1; its content is 1;
   its real ball contains 1). Cost: two divisions at p bits, the kernel, a rational inverse and a
   modular inverse. */
int adf_idele_inv(adf_idele_t y, const adf_idele_t x, slong prec);

/* adf_idele_mul_rat(z, x, q, prec): z = (Z, r |q|, u [sign(q)]) for x = (X, r, u) and an exact rational
   q != 0 (slice 2; api-2.md 2.3, Statement H): every product of a point of x with the diagonal q lies
   in z. The content r |q| is exact; the unit is adf_ucoset_mul(u, [sign(q)]), (sign(q) c) U(N) in normal
   form; Z is kernel B on lo = RD_p(l_X |n| / d), hi = RU_p(h_X |n| / d) with q = n/d (Statement F;
   the integers of q are used, never a ball of q, M1-D4), with the sign sign(X) sign(q).
   Status: ADF_OK, z written; ADF_NOT_UNIT if q = 0 (conventions 3.2: an exact zero input), z untouched;
   ADF_NOT_DETERMINED (B1) if the real part cannot be certified free of 0 at p bits, z untouched;
   ADF_LIMIT if prec > ADF_IDELE_PREC_MAX (also for q = 0), z untouched.
   z may be x. q is an adf_rat and not part of z. The class of the result equals the class of x
   (ideles.md P15, kernel Q^x; Statement G.3). Cost: two exact products and two divisions at p bits, the
   kernel, a product of rationals and adf_ucoset_mul. */
int adf_idele_mul_rat(adf_idele_t z, const adf_idele_t x, const adf_rat_t q, slong prec);

/* ---- valuations, absolute values, norm (PLAN 2.2; ideles.md P14, line 343; api-2.md 2.4, Statement I) ---- */

/* adf_idele_valuation_at(v, x, w): *v = v_p(x_p) = v_p(r) for the prime p of the place w (ideles.md
   P14.1, line 347), the same integer for every point of x. Computed by the number of times p divides the
   numerator and the denominator of r (P14.4, line 351; fmpz_remove, refs/src/flint-3.0.1/fmpz.rst:1142);
   r is never factored. |v| is at most the bit length of r, so it fits in an slong.
   Status: ADF_OK, *v written; ADF_DOMAIN if w is the archimedean place (an idele has no valuation there),
   *v untouched. Cost: two removals of p. */
int adf_idele_valuation_at(slong * v, const adf_idele_t x, adf_place_t w);

/* adf_idele_abs_at(a, x, w): a = |x_p|_p = p^(-v_p(r)) for the prime p of the place w, an exact rational
   (ideles.md P14.1, line 347), the same for every point of x. p^|v| divides the numerator or the
   denominator of r, so a is no larger than r in bits; no limit is needed.
   Status: ADF_OK, a written; ADF_DOMAIN if w is the archimedean place (use adf_idele_abs_inf), a
   untouched. a is an adf_rat and not part of x. Cost: two removals of p and a power of p. */
int adf_idele_abs_at(adf_rat_t a, const adf_idele_t x, adf_place_t w);

/* adf_idele_abs_inf(a, x): a = |x_inf|, the ball [|m| +/- rho] for x->inf = [m +/- rho], exactly (no
   rounding: the midpoint is negated if negative and the radius is copied). As x->inf excludes 0, a is the
   set {|xi| : xi in x->inf} and is positive. Never fails. a must not be x->inf (conventions 4.1(3)). */
void adf_idele_abs_inf(arb_t a, const adf_idele_t x);

/* adf_idele_norm(t, x, prec): t = a positive real ball that contains the norm |xi| * product_p |x_p|_p
   = |xi| / r of every point (xi, r w) of x (ideles.md P14.2, line 348). The finite factor 1/r is exact:
   it is formed from the integers of r and meets the real ball only in the last step, so each end point is
   rounded once (PLAN 2.2, "norm exact before rounding"): t is kernel B on lo = RD_p(l_X d / n),
   hi = RU_p(h_X d / n), r = n/d (Statement F), with the sign +1. It is the t of the class of x
   (adf_idclass_set_idele). For the idele of a rational q (adf_idele_set_rat) t contains 1 (the product
   formula, P14.3, line 350), and t is the exact 1 when the real ball of q is exact (q a dyadic number of at
   most p bits).
   Status: ADF_OK, t written; ADF_NOT_DETERMINED (B1) if the end points are more than p binades apart,
   t untouched; ADF_LIMIT if prec > ADF_IDELE_PREC_MAX, t untouched. t must not be x->inf.
   Cost: two exact products, two divisions at p bits and the kernel. */
int adf_idele_norm(arb_t t, const adf_idele_t x, slong prec);

/* ---- accessors ---- */

/* adf_idele_get_real(r, x): r = the real ball x->inf, a copy. A convenience special to Q (seams R3). */
void adf_idele_get_real(arb_t r, const adf_idele_t x);

/* adf_idele_content(r, x): r = the content of x, the exact positive rational of the decomposition
   x_f = r u (conventions 5.7, seams R5: the name "content", never "scale"), a canonical fmpq. */
void adf_idele_content(fmpq_t r, const adf_idele_t x);

/* adf_idele_get_unit(u, x): u = the unit coset of x, a copy of the stored pair. u must not be &x->u
   (conventions 4.1(3)). */
void adf_idele_get_unit(adf_ucoset_t u, const adf_idele_t x);

/* Layout queries (conventions 12.4, CV-40). Header-inline and exported. */
ADF_INLINE size_t adf_sizeof_idele(void) { return sizeof(adf_idele_struct); }
ADF_INLINE size_t adf_alignof_idele(void) { return ADF_ALIGNOF(adf_idele_struct); }

#ifdef __cplusplus
}
#endif

#endif /* ADELEFELD_IDELE_H */
