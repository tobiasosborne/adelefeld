/* adelefeld/idclass.h: adf_idclass, an element of the idele class group A^x / Q^x of Q to finite
   precision: a positive real ball t and a unit coset u'.

   Contract: docs/SPEC.md 5 (the table of types, "Sign preservation", "Operations": "The class of an idele
   has coordinates t = |x_inf| / r and u' = sign(x_inf) u; the sign on the unit is essential. The idele
   class group of Q is R_{>0} x Zhat^x"); docs/conventions.md 5.7 (struct, class predicate, class init
   <1 ; [1 mod 0]>, the result sign check of gate finding G5, the sign rule), 2.2, 2.3, 3.2 (row "Unit
   coset, idele, idele class arithmetic"), 4.1, 4.3, 4.4, 12.4; decisions M0-D1, M1-D4, N-D6. Proofs:
   docs/proofs/ideles.md Proposition 15 (line 366; the class map, its kernel Q^x, the representative
   (t, u'), and "On data", line 393); docs/api-2.md section 2, Statements F (the real kernel with an
   exact rational factor) and G (the set of a class value, product, inverse, the class of an idele
   value). Implemented in src/idclass.c (milestone 2, slice 2, lane i-slice2); tests tests/test_idclass.c.

   Meaning (api-2.md Statement G): by ideles.md P15.1 and P15.2 every class of ideles has exactly one
   representative (t, u') with t > 0 real and u' in Zhat^x, so a class is the pair (t, u'). The value
   (T, u) is the set of classes (t, w) with t in the closed interval T = [m - rho, m + rho] of the real
   ball t and w in the set of the unit coset u (adelefeld/ucoset.h).

   Predicate (is_canonical, conventions 5.7): arb_is_finite(t) and arb_is_positive(t) (every point of
   the ball is > 0; refs/src/flint-3.0.1/arb.rst:606, :639); adf_ucoset_is_canonical(u). Init: the class
   of 1, <1 ; [1]>: t = 1 exact, u = [1] = (1, 0).

   The real kernel is the one of adelefeld/idele.h (api-2.md Statement E, kernel B), with the sign +1:
   end points lo <= t <= hi rounded outwards at p = max(prec, 2) bits, then a ball; ADF_NOT_DETERMINED
   when e(hi) - e(lo) > p (B1), and the output untouched.

   Common rules for every function below, unless its comment says otherwise:
   - Aliasing (conventions 4.1): an output may be the same object as any input of this type; inputs
     may alias each other; an output never aliases a part of an input (x->t, &x->u).
   - Inputs satisfy their predicates; otherwise undefined (conventions 4.4, CV-09). With
     -DADF_CHECK_INVARIANTS every function that reads a value (a class, an idele, a unit coset) checks
     it on entry and aborts, except is_canonical (M1-D2); clear and swap do not read one.
   - A function that returns a status leaves every output untouched on a status other than ADF_OK
     (conventions 4.3): it computes into temporaries and swaps at the end.
   - prec is the real working precision in bits; a prec below 2 is taken as 2 (M1-D4). The unit does
     not depend on prec. A prec above ADF_IDELE_PREC_MAX (adelefeld/idele.h) gives ADF_LIMIT, decided
     from prec alone before any allocation, every output untouched, before every other status. Results
     of operations store the unit in normal form (N-D6, D2-1); constructors keep the modulus as supplied
     (CV-17). */

#ifndef ADELEFELD_IDCLASS_H
#define ADELEFELD_IDCLASS_H

#include <flint/arb.h>

#include "adelefeld/common.h"
#include "adelefeld/status.h"
#include "adelefeld/ucoset.h"
#include "adelefeld/idele.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Layout (conventions 5.7, 12.4), 64-bit: t (arb_struct, 48 bytes) at 0, u (adf_ucoset_struct,
   16 bytes) at 48; size 64, alignment 8. */
typedef struct
{
    arb_t t;
    adf_ucoset_struct u;
} adf_idclass_struct;

typedef adf_idclass_struct adf_idclass_t[1];
typedef adf_idclass_struct * adf_idclass_ptr;
typedef const adf_idclass_struct * adf_idclass_srcptr;

/* ---- life cycle (conventions 2.3) ---- */

/* adf_idclass_init(x): x = the class of 1, <1 ; [1]> (t = 1 exact, u = (1, 0)). Never fails. */
void adf_idclass_init(adf_idclass_t x);

/* adf_idclass_clear(x): releases the memory; afterwards x may only be passed to init. */
void adf_idclass_clear(adf_idclass_t x);

/* adf_idclass_set(y, x): y becomes a copy of x (the ball as it is, the stored unit pair as it is).
   y may be x. */
void adf_idclass_set(adf_idclass_t y, const adf_idclass_t x);

/* adf_idclass_swap(x, y): exchanges the contents; O(1), no allocation. */
void adf_idclass_swap(adf_idclass_t x, adf_idclass_t y);

/* adf_idclass_is_canonical(x): 1 if the predicate above holds, else 0. Never aborts for an initialised
   object whose fields hold any values (decision M1-D2). */
int adf_idclass_is_canonical(const adf_idclass_t x);

/* adf_idclass_identical(x, y): 1 if arb_equal(x->t, y->t) (same midpoint and radius) and
   adf_ucoset_identical(&x->u, &y->u), else 0 (conventions 2.3, CV-02). Not a comparison of sets. */
int adf_idclass_identical(const adf_idclass_t x, const adf_idclass_t y);

/* ---- constructors and accessors ---- */

/* adf_idclass_set_parts(x, t, u): x = (t, u), from raw data (conventions 3.2: constructors from raw
   data give OK, DOMAIN; 4.4). u is copied as it is (its modulus as supplied, CV-17).
   Status: ADF_OK, x written; ADF_DOMAIN, x untouched, if t is not finite (arb_is_finite = 0) or not
   positive (arb_is_positive = 0: the ball contains 0 or a negative number).
   u must be canonical (precondition; it is a value of the library). Aliasing: t and u are of other
   types than x and must not be parts of x. Cost: copies. */
int adf_idclass_set_parts(adf_idclass_t x, const arb_t t, const adf_ucoset_t u);

/* adf_idclass_set_idele(c, x, prec): c = the class of the idele value x = (X, r, u): the value
   (T, (sign(X) c0) U(N)) for u = c0 U(N) (ideles.md P15.1, line 370, and "On data", line 393; api-2.md
   Statement G.4): every class of a point of x lies in c. The unit is adf_ucoset_mul(u, [sign(X)]) in
   normal form, exact as a set (the sign of X is one sign, as X excludes 0); T is the norm of x,
   kernel B on lo = RD_p(l_X d / n), hi = RU_p(h_X d / n), r = n/d (Statement F; adf_idele_norm), so
   the factor 1/r is exact before the rounding. The sign on the unit is essential: without it the map
   is not constant on classes (P15.3, line 375).
   The class of the idele of a rational q is <T ; [1]> with T containing 1 (P15.1, kernel Q^x), the
   exact <1 ; [1]> when the real ball of q is exact and q has at most p = max(prec, 2) bits, prec of this
   call (Statement G.5; for a q of more bits T is a ball around 1); the class of q times x equals the class
   of x as a set (Statement G.3).
   Status (SPEC 5: the rule applies to the idele-to-class conversion): ADF_OK, c written;
   ADF_NOT_DETERMINED (B1) if T cannot be certified positive at p bits, c untouched; ADF_LIMIT if
   prec > ADF_IDELE_PREC_MAX, c untouched.
   c and x are of different types. Cost: that of adf_idele_norm and adf_ucoset_mul. */
int adf_idclass_set_idele(adf_idclass_t c, const adf_idele_t x, slong prec);

/* adf_idclass_get_t(t, x): t = the positive real ball x->t, a copy (the coordinate t of the class, special
   to Q, conventions 5.7). t must not be x->t. */
void adf_idclass_get_t(arb_t t, const adf_idclass_t x);

/* adf_idclass_get_unit(u, x): u = the unit coset x->u, a copy of the stored pair (the coordinate u' of
   the class, special to Q, conventions 5.7). u must not be &x->u (conventions 4.1(3)). */
void adf_idclass_get_unit(adf_ucoset_t u, const adf_idclass_t x);

/* adf_idclass_norm(t, x): t = the norm of the classes of x: the norm |.| is trivial on Q^x (ideles.md
   P14.3, line 350) and the norm of the representative (t, u') is t (P14.2 with r = 1, x_inf = t > 0), so
   the result is x->t, a copy, exact. Never fails. t must not be x->t. */
void adf_idclass_norm(arb_t t, const adf_idclass_t x);

/* ---- arithmetic (SPEC 5: componentwise; A^x/Q^x = R_{>0} x Zhat^x as groups, ideles.md P15.1) ---- */

/* adf_idclass_mul(z, x, y, prec): z = (Z, u u') for x = (T, u), y = (T', u'): every product of a class
   of x and a class of y lies in z (api-2.md G.1). The unit is adf_ucoset_mul (normal form; ideles.md
   P10, P11); Z is kernel B on lo = RD_p(l_T l_T'), hi = RU_p(h_T h_T') (Statement E2) with the sign +1.
   Status: ADF_OK, z written; ADF_NOT_DETERMINED (B1), z untouched; ADF_LIMIT if
   prec > ADF_IDELE_PREC_MAX, z untouched. z may be x or y or both.
   Cost: two products at p bits, the kernel and adf_ucoset_mul. */
int adf_idclass_mul(adf_idclass_t z, const adf_idclass_t x, const adf_idclass_t y, slong prec);

/* adf_idclass_inv(y, x, prec): y = (Z, u^-1) for x = (T, u): the inverse of every class of x lies in y
   (api-2.md G.2). The unit is adf_ucoset_inv (normal form; ideles.md P10.2); Z is kernel B on
   lo = RD_p(1/h_T), hi = RU_p(1/l_T) (Statement E3) with the sign +1.
   Status: ADF_OK, y written; ADF_NOT_DETERMINED (B1), y untouched; ADF_LIMIT if
   prec > ADF_IDELE_PREC_MAX, y untouched. y may be x. x * x^-1 contains the
   class of 1. Cost: two divisions at p bits, the kernel and a modular inverse. */
int adf_idclass_inv(adf_idclass_t y, const adf_idclass_t x, slong prec);

/* Layout queries (conventions 12.4, CV-40). Header-inline and exported. */
ADF_INLINE size_t adf_sizeof_idclass(void) { return sizeof(adf_idclass_struct); }
ADF_INLINE size_t adf_alignof_idclass(void) { return ADF_ALIGNOF(adf_idclass_struct); }

#ifdef __cplusplus
}
#endif

#endif /* ADELEFELD_IDCLASS_H */
