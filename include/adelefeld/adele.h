/* adelefeld/adele.h: adf_adele (real ball ; finite ball) and adf_cadele (complex ball ; finite
   ball).

   Contract: docs/conventions.md 0.4, 5.5 (structs, predicates, init), 2.2 (prec is the last
   argument before a context), 3.2, 4.1, 4.3, 4.4 (non-finite balls); docs/SPEC.md 4.1 (the two
   types; the exact rational is converted "at a requested real precision only when it meets an
   inexact value"); docs/seams.md section 5, R3 (archimedean operations through the place) and R4
   (adf_cadele is special to Q). Implemented in work package 1.3 (docs/PLAN.md section 6).

   Meaning: adf_adele is the set I x F of the ring A = R x A_f, I the real ball inf (arb) and F the
   finite ball fin (adelefeld/fball.h). adf_cadele is the set Z x F of the ring C x A_f, Z the
   complex ball inf (acb). C x A_f contains the adeles; it is not the adele ring of Q nor of Q(i),
   and it is not an algebra over C (SPEC 4.1). The two coordinates are independent.

   Common rules for every function below, unless its comment says otherwise:
   - Aliasing (conventions 4.1): an output may be the same object as any input of the same type;
     inputs may alias each other; an output never aliases a part of an input (&x->fin, x->inf).
   - Inputs satisfy the predicate of conventions 5.5; otherwise undefined (conventions 4.4, CV-09).
   - The finite coordinate follows adelefeld/fball.h exactly (tight rules, backends, the implicit
     global fallback of conventions 4.6); the real or complex coordinate follows arb or acb at the
     working precision prec, in bits (conventions 2.2; arb.h:382). A prec below 2 is taken as 2
     (decision M1-D4: a ball of one bit of an exact non-zero rational may contain 0). The finite
     coordinate does not depend on prec.
   - Enclosure: if the inputs contain the true values, the output contains the true result, in each
     coordinate (PLAN section 1, principle 5).
   - Functions that cannot fail return void (conventions 3.2 row 1). A function that returns a
     status leaves every output untouched on a status other than ADF_OK (conventions 4.3).
   - A result must satisfy conventions 5.5, whose real part must be finite. arb and acb ring
     operations on finite balls at a finite precision give finite balls in FLINT 3.0.1
     [unverified: overflow of the exponent is not considered; fmpz exponents make it practically
     unreachable]. */

#ifndef ADELEFELD_ADELE_H
#define ADELEFELD_ADELE_H

#include <flint/arb.h>
#include <flint/acb.h>

#include "adelefeld/common.h"
#include "adelefeld/status.h"
#include "adelefeld/place.h"
#include "adelefeld/rat.h"
#include "adelefeld/fball.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Layout (conventions 5.5, 12.4, 12.11), 64-bit:
   adf_adele_struct:  inf (arb_struct, 48 bytes) at 0, fin (adf_fball_struct, 48 bytes) at 48;
                      size 96, alignment 8.
   adf_cadele_struct: inf (acb_struct, 96 bytes) at 0, fin at 96; size 144, alignment 8.
   Predicate: arb_is_finite(inf) (arb.h:134), respectively acb_is_finite(inf) (acb.h:801), and fin
   satisfies G or L (conventions 5.2, 5.3). The real ball is never normalised beyond what arb does. */
typedef struct
{
    arb_t inf;
    adf_fball_struct fin;
} adf_adele_struct;

typedef adf_adele_struct adf_adele_t[1];
typedef adf_adele_struct * adf_adele_ptr;
typedef const adf_adele_struct * adf_adele_srcptr;

typedef struct
{
    acb_t inf;
    adf_fball_struct fin;
} adf_cadele_struct;

typedef adf_cadele_struct adf_cadele_t[1];
typedef adf_cadele_struct * adf_cadele_ptr;
typedef const adf_cadele_struct * adf_cadele_srcptr;

/* ======================= adf_adele ======================= */

/* ---- life cycle (conventions 2.3) ---- */

/* adf_adele_init(x): x = (0 ; 0), both coordinates the exact 0 (conventions 5.5). Never fails. */
void adf_adele_init(adf_adele_t x);

/* adf_adele_clear(x): releases both coordinates; never touches a context. */
void adf_adele_clear(adf_adele_t x);

/* adf_adele_set(y, x): y = x, same backend and context pointer in fin. y may be x. */
void adf_adele_set(adf_adele_t y, const adf_adele_t x);

/* adf_adele_swap(x, y): exchanges the contents; O(1), no allocation. */
void adf_adele_swap(adf_adele_t x, adf_adele_t y);

/* adf_adele_is_canonical(x): 1 if conventions 5.5 holds, else 0; never aborts. */
int adf_adele_is_canonical(const adf_adele_t x);

/* adf_adele_identical(x, y): 1 if arb_equal(x->inf, y->inf) (same midpoint and radius, arb.h) and
   adf_fball_identical(&x->fin, &y->fin), else 0 (conventions 2.1, CV-02). Not a comparison of
   points. */
int adf_adele_identical(const adf_adele_t x, const adf_adele_t y);

/* ---- constructors and conversion of adf_rat ---- */

/* adf_adele_set_rat(y, q, prec): y = (I ; q), I = arb_set_fmpq(q, prec) (arb.h), a real ball that
   contains q, and the exact finite ball q (SPEC 4.1: the exact rational is converted at a requested
   real precision). Enclosure: q lies in both coordinates. When q is a dyadic number whose odd
   mantissa has at most prec bits the real ball is exact. Cost: one arb_set_fmpq at prec. */
void adf_adele_set_rat(adf_adele_t y, const adf_rat_t q, slong prec);

/* adf_adele_set_si(y, n): y = (n ; n), both coordinates exact (an slong fits an arb exactly). */
void adf_adele_set_si(adf_adele_t y, slong n);

/* adf_adele_set_arb_fball(y, r, f): y = (r ; f), copies of both.
   Convention: constructor from raw data (conventions 3.2: OK, DOMAIN; 4.4: non-finite ball).
   Status: ADF_OK, y written; ADF_DOMAIN if r is not finite (arb_is_finite(r) = 0), y untouched.
   f must satisfy G or L (precondition). Aliasing: r and f are of other types than y and must not
   be parts of y. Cost: two copies. */
int adf_adele_set_arb_fball(adf_adele_t y, const arb_t r, const adf_fball_t f);

/* ---- projections to the places (seams R3) ---- */

/* adf_adele_get_real(r, x): r = the real coordinate x->inf, a copy. A convenience special to Q
   (conventions 5.5, seams R3); the place-based form is adf_adele_get_arb_at. */
void adf_adele_get_real(arb_t r, const adf_adele_t x);

/* adf_adele_get_arb_at(r, x, v): r = the coordinate of x at the archimedean place v.
   Status: ADF_OK, r written; ADF_DOMAIN if v is a finite place, r untouched (the coordinate at a
   prime is a ball of Q_p, adf_lball, milestone 1F; conventions 3.1: reported with the place). */
int adf_adele_get_arb_at(arb_t r, const adf_adele_t x, adf_place_t v);

/* adf_adele_get_fin(f, x): f = the finite coordinate x->fin, a copy (same backend and context
   pointer). f must not be &x->fin (conventions 4.1(3)). */
void adf_adele_get_fin(adf_fball_t f, const adf_adele_t x);

/* ---- arithmetic (SPEC 4.3 at the finite part; arb at the real part; void) ---- */

/* adf_adele_add(z, x, y, prec): z = (arb_add(I, J, prec) ; adf_fball_add(F, G)).
   adf_adele_sub, adf_adele_mul: likewise with arb_sub, arb_mul and adf_fball_sub, adf_fball_mul.
   adf_adele_neg(y, x): y = (-I ; -F), exact in both coordinates (no prec).
   Enclosure: each coordinate encloses the result set of its operation (arb.h:382, :395;
   precision.md Propositions 1 and 2, lines 27 and 34). Cost: one arb operation at prec plus the
   finite operation of fball.h. */
void adf_adele_add(adf_adele_t z, const adf_adele_t x, const adf_adele_t y, slong prec);
void adf_adele_sub(adf_adele_t z, const adf_adele_t x, const adf_adele_t y, slong prec);
void adf_adele_mul(adf_adele_t z, const adf_adele_t x, const adf_adele_t y, slong prec);
void adf_adele_neg(adf_adele_t y, const adf_adele_t x);

/* adf_adele_add_rat(z, x, q, prec): z = x + q, q an exact rational at every place:
   (an enclosure of I + q at prec ; F + q), the finite part by adf_fball_add with the exact ball q
   (precision.md Proposition 1 with radius 0). SPEC 4.1: q meets an inexact value, so it is
   converted at prec only in the real coordinate. */
void adf_adele_add_rat(adf_adele_t z, const adf_adele_t x, const adf_rat_t q, slong prec);

/* adf_adele_mul_rat(z, x, q, prec): z = q x = (an enclosure of q I at prec ; adf_fball_mul_rat(F, q))
   (precision.md Proposition 6(2), line 106). FLINT 3.0.1 has no arb_add_fmpq, arb_mul_fmpq or
   arb_div_fmpq; with q = n/d in lowest terms the real part of mul_rat and div_rat is computed with
   the exact integers n and d (arb_mul_fmpz, arb_div_fmpz), never by dividing by a ball of q, so
   that no division by a ball containing 0 can occur (review of milestone 1, arith R3). */
void adf_adele_mul_rat(adf_adele_t z, const adf_adele_t x, const adf_rat_t q, slong prec);

/* adf_adele_div_rat(z, x, q, prec): z = x / q (SPEC 4.5: one divides by an exact non-zero
   rational). Convention: conventions 3.2 ("scaling by the inverse of an exact rational").
   Status: ADF_OK, z written; ADF_NOT_UNIT if q = 0, z untouched. Division by an adf_adele is not
   defined (SPEC 4.5) and is not offered. */
int adf_adele_div_rat(adf_adele_t z, const adf_adele_t x, const adf_rat_t q, slong prec);

/* Layout queries (conventions 12.4, CV-40). Header-inline and exported. */
ADF_INLINE size_t adf_sizeof_adele(void) { return sizeof(adf_adele_struct); }
ADF_INLINE size_t adf_alignof_adele(void) { return ADF_ALIGNOF(adf_adele_struct); }

/* ======================= adf_cadele (special to Q, seams R4) ======================= */

/* Life cycle as for adf_adele (conventions 2.3, 5.5): init gives (0 ; 0); identical compares
   acb_equal of the complex parts and adf_fball_identical of the finite parts. */
void adf_cadele_init(adf_cadele_t x);
void adf_cadele_clear(adf_cadele_t x);
void adf_cadele_set(adf_cadele_t y, const adf_cadele_t x);
void adf_cadele_swap(adf_cadele_t x, adf_cadele_t y);
int adf_cadele_is_canonical(const adf_cadele_t x);
int adf_cadele_identical(const adf_cadele_t x, const adf_cadele_t y);

/* adf_cadele_set_rat(y, q, prec): y = (acb_set_fmpq(q, prec) ; q) (acb.h), imaginary part exact 0. */
void adf_cadele_set_rat(adf_cadele_t y, const adf_rat_t q, slong prec);

/* adf_cadele_set_adele(y, x): y = the image of x under A -> C x A_f, (inf + 0 i ; fin), exact. */
void adf_cadele_set_adele(adf_cadele_t y, const adf_adele_t x);

/* adf_cadele_set_acb_fball(y, z, f): y = (z ; f).
   Status: ADF_OK; ADF_DOMAIN if z is not finite (acb_is_finite, acb.h:801), y untouched
   (conventions 3.2, 4.4). */
int adf_cadele_set_acb_fball(adf_cadele_t y, const acb_t z, const adf_fball_t f);

/* adf_cadele_get_complex(z, x): z = x->inf. adf_cadele_get_fin(f, x): f = x->fin (copies). */
void adf_cadele_get_complex(acb_t z, const adf_cadele_t x);
void adf_cadele_get_fin(adf_fball_t f, const adf_cadele_t x);

/* Arithmetic of the ring C x A_f: acb_add, acb_sub, acb_mul, acb_neg at prec in the first
   coordinate, the tight finite operations in the second; mul_rat and add_rat with an exact
   rational as for adf_adele; div_rat returns ADF_NOT_UNIT for q = 0 with z untouched. SPEC 4.1:
   (i ; 0) squares to (-1 ; 0), which is not the -1 of the ring. */
void adf_cadele_add(adf_cadele_t z, const adf_cadele_t x, const adf_cadele_t y, slong prec);
void adf_cadele_sub(adf_cadele_t z, const adf_cadele_t x, const adf_cadele_t y, slong prec);
void adf_cadele_mul(adf_cadele_t z, const adf_cadele_t x, const adf_cadele_t y, slong prec);
void adf_cadele_neg(adf_cadele_t y, const adf_cadele_t x);
void adf_cadele_add_rat(adf_cadele_t z, const adf_cadele_t x, const adf_rat_t q, slong prec);
void adf_cadele_mul_rat(adf_cadele_t z, const adf_cadele_t x, const adf_rat_t q, slong prec);
int adf_cadele_div_rat(adf_cadele_t z, const adf_cadele_t x, const adf_rat_t q, slong prec);

/* Layout queries (conventions 12.4, CV-40). Header-inline and exported. */
ADF_INLINE size_t adf_sizeof_cadele(void) { return sizeof(adf_cadele_struct); }
ADF_INLINE size_t adf_alignof_cadele(void) { return ADF_ALIGNOF(adf_cadele_struct); }

#ifdef __cplusplus
}
#endif

#endif /* ADELEFELD_ADELE_H */
