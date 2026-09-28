/* adelefeld/adele.c: adf_adele and adf_cadele, the additive adeles (work package 1.3).

   Meaning (docs/SPEC.md 4.1, docs/conventions.md 5.5): adf_adele is the set I x F of the ring
   A = R x A_f, I the real ball inf (arb) and F the finite ball fin (adelefeld/fball.h);
   adf_cadele is the set Z x F of the ring C x A_f, Z the complex ball inf (acb). The two
   coordinates are independent. The finite coordinate is reached only through the functions of
   fball.h; this file never reads or writes a field of adf_fball_struct, so a local finite part
   (work package 1.8) is passed to those functions unchanged and their result is used as it is.

   Aliasing (docs/conventions.md 4.1): an output may be the same object as any input of the same
   type; an output never aliases a part of an input (&x->fin, x->inf). The arb and acb functions
   used below allow the output to be either input (refs/src/flint-3.0.1/arb.rst:767-805 for add,
   sub, mul; acb.rst:429-463 for add, sub, mul; acb.rst:411 for neg), and every function of
   fball.h allows it as well (fball.h, "Common rules", aliasing). Each function writes the two
   coordinates last.

   State after a status (docs/conventions.md 4.3): adf_adele_set_arb_fball,
   adf_cadele_set_acb_fball, adf_adele_get_arb_at, adf_adele_div_rat and adf_cadele_div_rat
   check everything before the first write, so the output is untouched on ADF_DOMAIN or
   ADF_NOT_UNIT.

   Reading of the header for the functions with an exact rational q (include/adelefeld/adele.h,
   decision M1-D4). FLINT 3.0.1 has no arb_add_fmpq, arb_mul_fmpq or arb_div_fmpq (nm of
   libflint.so has no such symbol, and arb.rst and acb.rst document no such function). The
   reading used here:
   - add_rat converts the exact rational q to a ball at the clamped prec with arb_set_fmpq
     (arb.rst:167) or acb_set_fmpq (acb.rst:128) and adds that ball with arb_add (arb.rst:767)
     or acb_add (acb.rst:429). There is no exact sum of a ball with a rational in FLINT 3.0.1;
     the conversion is an enclosure ("The result of an (approximate) operation ... is a ball
     which contains the result of the mathematically exact operation", arb.rst:9-11), and the
     following operation encloses the exact result, so the composition encloses I + q.
   - mul_rat and div_rat never form a ball of q. With q = n/d in lowest terms they use the
     exact integers: mul_rat is arb_mul_fmpz by n (arb.rst:806) then arb_div_fmpz by d
     (arb.rst:864), and div_rat is arb_mul_fmpz by d then arb_div_fmpz by n; the sign of n is
     carried by the integer, and n = 0 is ADF_NOT_UNIT before. The acb counterparts are
     acb_mul_fmpz (acb.rst:457) and acb_div_fmpz (acb.rst:517). No division by a ball that may
     contain 0 can occur (finding R3 of docs/reviews/m1/arith/review.md).
   The finite coordinate is exact and independent of prec, as the header demands. */

#include <adelefeld/adele.h>

#include <flint/arb.h>
#include <flint/acb.h>
#include <flint/fmpq.h>

/* A prec below 2 is taken as 2 (decision M1-D4, docs/SPEC.md 15.2 row M1-D4; a ball of one bit
   of an exact non-zero rational may contain 0, and arb_div by such a ball gives a non-finite
   result, finding R3 of docs/reviews/m1/arith/review.md). This is the only place the clamp is
   made. */
static slong
adele_prec(slong prec)
{
    return prec < 2 ? 2 : prec;
}

/* ======================= adf_adele ======================= */

/* ---- life cycle (conventions 2.3) ---- */

/* adf_adele_init(x): x = (0 ; 0), both coordinates exact (conventions 5.5). arb_init and
   adf_fball_init never fail and give the exact zero (arb.rst:84; fball.h, adf_fball_init). */

void
adf_adele_init(adf_adele_t x)
{
    arb_init(x->inf);
    adf_fball_init(&x->fin);
}

/* adf_adele_clear(x): releases both coordinates; never touches a context (conventions 2.3,
   4.2; arb_clear, arb.rst:89; fball.h adf_fball_clear). */

void
adf_adele_clear(adf_adele_t x)
{
    arb_clear(x->inf);
    adf_fball_clear(&x->fin);
}

/* adf_adele_set(y, x): y = x, same backend and context pointer in fin; y may be x
   (conventions 2.3; arb_set, arb.rst:137; fball.h adf_fball_set). */

void
adf_adele_set(adf_adele_t y, const adf_adele_t x)
{
    arb_set(y->inf, x->inf);
    adf_fball_set(&y->fin, &x->fin);
}

/* adf_adele_swap(x, y): exchanges the contents, O(1), no allocation (conventions 2.3;
   arb_swap, arb.rst:102; fball.h adf_fball_swap). */

void
adf_adele_swap(adf_adele_t x, adf_adele_t y)
{
    arb_swap(x->inf, y->inf);
    adf_fball_swap(&x->fin, &y->fin);
}

/* adf_adele_is_canonical(x): 1 if conventions 5.5 holds, else 0; never aborts. The predicate
   is arb_is_finite(inf) (arb.rst:606) and fin satisfies G or L (fball.h
   adf_fball_is_canonical). */

int
adf_adele_is_canonical(const adf_adele_t x)
{
    return arb_is_finite(x->inf) && adf_fball_is_canonical(&x->fin);
}

/* adf_adele_identical(x, y): 1 if the real balls are equal (same midpoint and radius;
   arb_equal, arb.rst:623) and the finite parts are identical (conventions 2.1, CV-02; not a
   comparison of points). */

int
adf_adele_identical(const adf_adele_t x, const adf_adele_t y)
{
    return arb_equal(x->inf, y->inf) && adf_fball_identical(&x->fin, &y->fin);
}

/* ---- constructors and conversion of adf_rat ---- */

/* adf_adele_set_rat(y, q, prec): y = (I ; q), I = the ball containing q at prec, and the exact
   finite ball q. "The exact rational ... is converted to an adf_adele at a requested real
   precision only when it meets an inexact value" (docs/SPEC.md 4.1). arb_set_fmpq sets the ball
   containing q, rounded towards zero at prec (arb.rst:167); for a dyadic q that fits in prec
   bits the ball is exact ("Arithmetic operations done on exact input with exactly representable
   output are always guaranteed to produce exact output", arb.rst:25-26). The finite coordinate
   is exact and does not depend on prec. Aliasing: y and q are of different types. Cost: one
   fmpq copy, one arb_set_fmpq, and a copy of q into fin. */

void
adf_adele_set_rat(adf_adele_t y, const adf_rat_t q, slong prec)
{
    fmpq_t t;

    prec = adele_prec(prec);
    fmpq_init(t);
    adf_rat_get_fmpq(t, q);
    arb_set_fmpq(y->inf, t, prec);
    adf_fball_set_rat(&y->fin, q);
    fmpq_clear(t);
}

/* adf_adele_set_si(y, n): y = (n ; n), both exact. arb_set_si is exact (arb.rst:141) and
   fball.h adf_fball_set_si is exact. Cost: constant. */

void
adf_adele_set_si(adf_adele_t y, slong n)
{
    arb_set_si(y->inf, n);
    adf_fball_set_si(&y->fin, n);
}

/* adf_adele_set_arb_fball(y, r, f): y = (r ; f), copies of both.
   Status: ADF_OK, y written; ADF_DOMAIN if r is not finite (arb_is_finite(r) = 0; conventions
   4.4 "Non-finite real or complex ball"), y untouched (conventions 4.3). The check precedes
   every write. The finite part f satisfies G or L by precondition and is copied by
   adf_fball_set. Aliasing: r and f are of other types than y and are not parts of y.
   Cost: two copies. */

int
adf_adele_set_arb_fball(adf_adele_t y, const arb_t r, const adf_fball_t f)
{
    if (!arb_is_finite(r))
        return ADF_DOMAIN;

    arb_set(y->inf, r);
    adf_fball_set(&y->fin, f);
    return ADF_OK;
}

/* ---- projections to the places (seams R3) ---- */

/* adf_adele_get_real(r, x): r = x->inf, a copy (conventions 5.5; arb_set, arb.rst:137). The
   place-based form is adf_adele_get_arb_at (docs/seams.md R3). */

void
adf_adele_get_real(arb_t r, const adf_adele_t x)
{
    arb_set(r, x->inf);
}

/* adf_adele_get_arb_at(r, x, v): r = the coordinate of x at the archimedean place v.
   Status: ADF_OK, r written; ADF_DOMAIN if v is a finite place, r untouched (the coordinate at
   a prime is an adf_lball, milestone 1F; docs/conventions.md 3.1). The check precedes the write
   (conventions 4.3). */

int
adf_adele_get_arb_at(arb_t r, const adf_adele_t x, adf_place_t v)
{
    if (!adf_place_is_archimedean(v))
        return ADF_DOMAIN;

    arb_set(r, x->inf);
    return ADF_OK;
}

/* adf_adele_get_fin(f, x): f = x->fin, a copy, same backend and context pointer (conventions
   5.5; fball.h adf_fball_set). f must not be &x->fin (conventions 4.1(3)). */

void
adf_adele_get_fin(adf_fball_t f, const adf_adele_t x)
{
    adf_fball_set(f, &x->fin);
}

/* ---- arithmetic (docs/SPEC.md 4.3 at the finite part; arb at the real part; void) ---- */

/* adf_adele_add(z, x, y, prec): z = (arb_add(I, J, prec) ; adf_fball_add(F, G)). arb_add sets
   z to a ball containing the pointwise sum, rounded to prec bits (arb.rst:767); the finite sum
   is the tight set sum (docs/proofs/precision.md Proposition 1, line 27). Both allow the output
   to be either input. Aliasing: z may be x, y or both. Cost: one arb_add plus the finite add. */

void
adf_adele_add(adf_adele_t z, const adf_adele_t x, const adf_adele_t y, slong prec)
{
    prec = adele_prec(prec);
    arb_add(z->inf, x->inf, y->inf, prec);
    adf_fball_add(&z->fin, &x->fin, &y->fin);
}

/* adf_adele_sub(z, x, y, prec): z = (arb_sub(I, J, prec) ; adf_fball_sub(F, G)). arb_sub sets a
   ball containing the difference (arb.rst:785); the finite difference is the tight set
   difference (precision.md Proposition 1 with negation). Aliasing: as add. */

void
adf_adele_sub(adf_adele_t z, const adf_adele_t x, const adf_adele_t y, slong prec)
{
    prec = adele_prec(prec);
    arb_sub(z->inf, x->inf, y->inf, prec);
    adf_fball_sub(&z->fin, &x->fin, &y->fin);
}

/* adf_adele_mul(z, x, y, prec): z = (arb_mul(I, J, prec) ; adf_fball_mul(F, G)). arb_mul sets a
   ball containing the product (arb.rst:798); the finite product is the tight set product
   (precision.md Proposition 2, line 34). Aliasing: as add. */

void
adf_adele_mul(adf_adele_t z, const adf_adele_t x, const adf_adele_t y, slong prec)
{
    prec = adele_prec(prec);
    arb_mul(z->inf, x->inf, y->inf, prec);
    adf_fball_mul(&z->fin, &x->fin, &y->fin);
}

/* adf_adele_neg(y, x): y = (-I ; -F), exact in both coordinates, no prec. arb_neg is exact
   (arb.rst:729); the finite negation is exact (fball.h adf_fball_neg). Aliasing: y may be x. */

void
adf_adele_neg(adf_adele_t y, const adf_adele_t x)
{
    arb_neg(y->inf, x->inf);
    adf_fball_neg(&y->fin, &x->fin);
}

/* adf_adele_add_rat(z, x, q, prec): z = x + q, q exact at every place:
   (I + q at prec ; F + q). The finite part is adf_fball_add with the exact ball q, radius 0
   (precision.md Proposition 1 with radius 0). The real part is the reading in the file comment
   above: q is read into a temporary ball at prec (arb_set_fmpq, arb.rst:167) and added with
   arb_add (arb.rst:767). Aliasing: z may be x. Cost: one fmpq copy, one arb_set_fmpq, one
   arb_add, one finite set and add. */

void
adf_adele_add_rat(adf_adele_t z, const adf_adele_t x, const adf_rat_t q, slong prec)
{
    fmpq_t tq;
    arb_t t;
    adf_fball_t fq;

    prec = adele_prec(prec);
    fmpq_init(tq);
    arb_init(t);
    adf_fball_init(fq);

    adf_rat_get_fmpq(tq, q);
    arb_set_fmpq(t, tq, prec);
    adf_fball_set_rat(fq, q);

    arb_add(z->inf, x->inf, t, prec);
    adf_fball_add(&z->fin, &x->fin, fq);

    fmpq_clear(tq);
    arb_clear(t);
    adf_fball_clear(fq);
}

/* adf_adele_mul_rat(z, x, q, prec): z = q x = (I q at prec ; q F). The finite part is the exact
   scaling by the rational q (precision.md Proposition 6(2), line 106; fball.h
   adf_fball_mul_rat). The real part is q = n/d in lowest terms applied as the exact integers:
   arb_mul_fmpz by n then arb_div_fmpz by d (the reading of the header in the file comment
   above). Aliasing: z may be x. Cost: one fmpq copy, one arb_mul_fmpz, one arb_div_fmpz, one
   finite scaling. */

void
adf_adele_mul_rat(adf_adele_t z, const adf_adele_t x, const adf_rat_t q, slong prec)
{
    fmpq_t tq;

    prec = adele_prec(prec);
    fmpq_init(tq);

    adf_rat_get_fmpq(tq, q);
    arb_mul_fmpz(z->inf, x->inf, fmpq_numref(tq), prec);
    arb_div_fmpz(z->inf, z->inf, fmpq_denref(tq), prec);
    adf_fball_mul_rat(&z->fin, &x->fin, q);

    fmpq_clear(tq);
}

/* adf_adele_div_rat(z, x, q, prec): z = x / q (docs/SPEC.md 4.5: one divides by an exact
   non-zero rational). Status: ADF_OK, z written; ADF_NOT_UNIT if q = 0, z untouched
   (conventions 3.2, 4.3). The finite division is computed into a temporary first, so that a
   non-ADF_OK status from adf_fball_div_rat leaves z untouched; the real part is q = n/d in
   lowest terms applied as the exact integers: arb_mul_fmpz by d then arb_div_fmpz by n, with
   the sign of n carried by the integer (the reading of the header in the file comment above).
   Aliasing: z may be x. Cost: one fmpq copy, one arb_mul_fmpz, one arb_div_fmpz, one finite
   division. */

int
adf_adele_div_rat(adf_adele_t z, const adf_adele_t x, const adf_rat_t q, slong prec)
{
    fmpq_t tq;
    adf_fball_t fq;
    int st;

    if (adf_rat_is_zero(q))
        return ADF_NOT_UNIT;

    prec = adele_prec(prec);
    fmpq_init(tq);
    adf_fball_init(fq);

    adf_rat_get_fmpq(tq, q);
    st = adf_fball_div_rat(fq, &x->fin, q);
    if (st == ADF_OK)
    {
        arb_mul_fmpz(z->inf, x->inf, fmpq_denref(tq), prec);
        arb_div_fmpz(z->inf, z->inf, fmpq_numref(tq), prec);
        adf_fball_swap(&z->fin, fq);
    }

    fmpq_clear(tq);
    adf_fball_clear(fq);
    return st;
}

/* ======================= adf_cadele (special to Q, seams R4) ======================= */

/* Life cycle (conventions 2.3, 5.5). acb_init gives 0 + 0 i (acb.rst:58); acb_clear
   (acb.rst:62), acb_swap (acb.rst:138) and acb_set (acb.rst:104) are the acb counterparts of
   the arb functions above. */

void
adf_cadele_init(adf_cadele_t x)
{
    acb_init(x->inf);
    adf_fball_init(&x->fin);
}

void
adf_cadele_clear(adf_cadele_t x)
{
    acb_clear(x->inf);
    adf_fball_clear(&x->fin);
}

void
adf_cadele_set(adf_cadele_t y, const adf_cadele_t x)
{
    acb_set(y->inf, x->inf);
    adf_fball_set(&y->fin, &x->fin);
}

void
adf_cadele_swap(adf_cadele_t x, adf_cadele_t y)
{
    acb_swap(x->inf, y->inf);
    adf_fball_swap(&x->fin, &y->fin);
}

/* adf_cadele_is_canonical: acb_is_finite(inf) (acb.rst:224) and fin satisfies G or L. */

int
adf_cadele_is_canonical(const adf_cadele_t x)
{
    return acb_is_finite(x->inf) && adf_fball_is_canonical(&x->fin);
}

/* adf_cadele_identical: acb_equal compares real and imaginary parts (acb.rst:240) and
   adf_fball_identical the finite parts. */

int
adf_cadele_identical(const adf_cadele_t x, const adf_cadele_t y)
{
    return acb_equal(x->inf, y->inf) && adf_fball_identical(&x->fin, &y->fin);
}

/* adf_cadele_set_rat(y, q, prec): y = (the ball of q at prec ; the exact q), imaginary part
   exact 0. acb_set_fmpq sets z to the rational x rounded to prec bits (acb.rst:128) with a zero
   imaginary part. The finite coordinate is exact and does not depend on prec. Aliasing: y and q
   are of different types. Cost: one fmpq copy, one acb_set_fmpq, one finite copy. */

void
adf_cadele_set_rat(adf_cadele_t y, const adf_rat_t q, slong prec)
{
    fmpq_t t;

    prec = adele_prec(prec);
    fmpq_init(t);
    adf_rat_get_fmpq(t, q);
    acb_set_fmpq(y->inf, t, prec);
    adf_fball_set_rat(&y->fin, q);
    fmpq_clear(t);
}

/* adf_cadele_set_adele(y, x): y = the image of x under A -> C x A_f, (inf + 0 i ; fin), exact.
   acb_set_arb sets the real part to an arb and the imaginary part to zero (acb.rst:114); the
   finite part is a copy. Aliasing: y and x are of different types. Cost: one copy each. */

void
adf_cadele_set_adele(adf_cadele_t y, const adf_adele_t x)
{
    acb_set_arb(y->inf, x->inf);
    adf_fball_set(&y->fin, &x->fin);
}

/* adf_cadele_set_acb_fball(y, z, f): y = (z ; f).
   Status: ADF_OK, y written; ADF_DOMAIN if z is not finite (acb_is_finite, acb.rst:224; the
   conventions 4.4 row "Non-finite real or complex ball"), y untouched (conventions 4.3). The
   check precedes every write. Aliasing: z and f are of other types than y and are not parts of
   y. Cost: two copies. */

int
adf_cadele_set_acb_fball(adf_cadele_t y, const acb_t z, const adf_fball_t f)
{
    if (!acb_is_finite(z))
        return ADF_DOMAIN;

    acb_set(y->inf, z);
    adf_fball_set(&y->fin, f);
    return ADF_OK;
}

/* adf_cadele_get_complex(z, x): z = x->inf (acb.rst:104). adf_cadele_get_fin(f, x): f = x->fin
   (fball.h adf_fball_set). z must not be &x->inf and f must not be &x->fin (conventions
   4.1(3)). */

void
adf_cadele_get_complex(acb_t z, const adf_cadele_t x)
{
    acb_set(z, x->inf);
}

void
adf_cadele_get_fin(adf_fball_t f, const adf_cadele_t x)
{
    adf_fball_set(f, &x->fin);
}

/* Arithmetic of the ring C x A_f: acb_add, acb_sub, acb_mul, acb_neg at prec in the first
   coordinate (acb.rst:429, :441, :463, :411), the tight finite operations in the second
   (fball.h). Aliasing as for adf_adele. If x and y are the same pointer, acb_mul uses the
   squaring formula (acb.rst:463-469), which is the correct reading of z = x^2. Cost: one acb
   operation plus the finite operation. */

void
adf_cadele_add(adf_cadele_t z, const adf_cadele_t x, const adf_cadele_t y, slong prec)
{
    prec = adele_prec(prec);
    acb_add(z->inf, x->inf, y->inf, prec);
    adf_fball_add(&z->fin, &x->fin, &y->fin);
}

void
adf_cadele_sub(adf_cadele_t z, const adf_cadele_t x, const adf_cadele_t y, slong prec)
{
    prec = adele_prec(prec);
    acb_sub(z->inf, x->inf, y->inf, prec);
    adf_fball_sub(&z->fin, &x->fin, &y->fin);
}

void
adf_cadele_mul(adf_cadele_t z, const adf_cadele_t x, const adf_cadele_t y, slong prec)
{
    prec = adele_prec(prec);
    acb_mul(z->inf, x->inf, y->inf, prec);
    adf_fball_mul(&z->fin, &x->fin, &y->fin);
}

void
adf_cadele_neg(adf_cadele_t y, const adf_cadele_t x)
{
    acb_neg(y->inf, x->inf);
    adf_fball_neg(&y->fin, &x->fin);
}

/* adf_cadele_add_rat: as adf_adele_add_rat, with acb_add (acb.rst:429). */

void
adf_cadele_add_rat(adf_cadele_t z, const adf_cadele_t x, const adf_rat_t q, slong prec)
{
    fmpq_t tq;
    acb_t t;
    adf_fball_t fq;

    prec = adele_prec(prec);
    fmpq_init(tq);
    acb_init(t);
    adf_fball_init(fq);

    adf_rat_get_fmpq(tq, q);
    acb_set_fmpq(t, tq, prec);
    adf_fball_set_rat(fq, q);

    acb_add(z->inf, x->inf, t, prec);
    adf_fball_add(&z->fin, &x->fin, fq);

    fmpq_clear(tq);
    acb_clear(t);
    adf_fball_clear(fq);
}

/* adf_cadele_mul_rat: as adf_adele_mul_rat, with acb_mul_fmpz by n and acb_div_fmpz by d
   (acb.rst:457, :517). */

void
adf_cadele_mul_rat(adf_cadele_t z, const adf_cadele_t x, const adf_rat_t q, slong prec)
{
    fmpq_t tq;

    prec = adele_prec(prec);
    fmpq_init(tq);

    adf_rat_get_fmpq(tq, q);
    acb_mul_fmpz(z->inf, x->inf, fmpq_numref(tq), prec);
    acb_div_fmpz(z->inf, z->inf, fmpq_denref(tq), prec);
    adf_fball_mul_rat(&z->fin, &x->fin, q);

    fmpq_clear(tq);
}

/* adf_cadele_div_rat: as adf_adele_div_rat, with acb_mul_fmpz by d and acb_div_fmpz by n
   (acb.rst:457, :517). Status: ADF_OK, z written; ADF_NOT_UNIT if q = 0, z untouched
   (docs/SPEC.md 4.5; conventions 3.2, 4.3). */

int
adf_cadele_div_rat(adf_cadele_t z, const adf_cadele_t x, const adf_rat_t q, slong prec)
{
    fmpq_t tq;
    adf_fball_t fq;
    int st;

    if (adf_rat_is_zero(q))
        return ADF_NOT_UNIT;

    prec = adele_prec(prec);
    fmpq_init(tq);
    adf_fball_init(fq);

    adf_rat_get_fmpq(tq, q);
    st = adf_fball_div_rat(fq, &x->fin, q);
    if (st == ADF_OK)
    {
        acb_mul_fmpz(z->inf, x->inf, fmpq_denref(tq), prec);
        acb_div_fmpz(z->inf, z->inf, fmpq_numref(tq), prec);
        adf_fball_swap(&z->fin, fq);
    }

    fmpq_clear(tq);
    adf_fball_clear(fq);
    return st;
}
