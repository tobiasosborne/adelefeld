/* tests/test_adele_lowprec.c: adf_adele and adf_cadele at a working precision below 2.

   The contract is include/adelefeld/adele.h: a prec below 2 is taken as 2 (decision M1-D4,
   docs/SPEC.md 15 row M1-D4; conventions 2.2), the real or complex coordinate is finite, the
   result encloses the exact image, and the finite coordinate does not depend on prec. The
   finding that motivates the file is R3 of docs/reviews/m1/arith/review.md: adf_adele_div_rat
   at prec = 1 returned ADF_OK with a non-finite real part, because the old code converted the
   exact rational q to a ball at prec and divided by it.

   For every operation of the header that takes a prec and every permitted aliasing, the test
   runs the operation at prec in {1, 0, -5, 2, 3, 53} and checks three things:
   - adf_adele_is_canonical / adf_cadele_is_canonical (the real or complex part is finite);
   - the coordinate contains the exact value, computed with fmpq;
   - for prec below 2 the result is adf_adele_identical / adf_cadele_identical to the result at
     prec = 2 (the clamp of M1-D4).

   The q values are 1/3, -1/3, 3, 7/1024 and (2^4096 + 1)/(2^4095 + 1), a rational whose
   numerator has 4097 bits and whose denominator has 4096. The binary operations use real parts
   1/3 and 1/7 (exact at 200 bits, not representable below), so that the result is inexact and
   the clamp is visible. */

#include <flint/arb.h>
#include <flint/acb.h>
#include <flint/fmpq.h>
#include <flint/fmpz.h>

#include <adelefeld/adele.h>

#include "test_runner.h"

/* The precisions, the ones below 2 last, so that the assertion failures of the red run are
   printed before the current code aborts at prec = 0. */
static const slong adf_precs[] = {1, 2, 3, 53, 0, -5};
#define ADF_NPRECS ((slong) (sizeof(adf_precs) / sizeof(adf_precs[0])))

#define ADF_NQ 5

/* --------------------------------------------------------------- the q values */

static void
build_qs(fmpq_t qs[ADF_NQ], adf_rat_t qr[ADF_NQ])
{
    fmpz_t n, d;
    slong i;

    for (i = 0; i < ADF_NQ; i++)
    {
        fmpq_init(qs[i]);
        adf_rat_init(qr[i]);
    }
    fmpq_set_si(qs[0], 1, 3);
    fmpq_set_si(qs[1], -1, 3);
    fmpq_set_si(qs[2], 3, 1);
    fmpq_set_si(qs[3], 7, 1024);
    fmpz_init(n);
    fmpz_init(d);
    fmpz_one(n);
    fmpz_mul_2exp(n, n, 4096);
    fmpz_add_ui(n, n, 1);
    fmpz_one(d);
    fmpz_mul_2exp(d, d, 4095);
    fmpz_add_ui(d, d, 1);
    fmpq_set_fmpz_frac(qs[4], n, d);
    fmpq_canonicalise(qs[4]);
    fmpz_clear(n);
    fmpz_clear(d);
    for (i = 0; i < ADF_NQ; i++)
        ADF_CHECK_MSG(adf_rat_set_fmpq(qr[i], qs[i]) == ADF_OK,
                      "adf_rat_set_fmpq failed on q %ld", (long) i);
}

static void
clear_qs(fmpq_t qs[ADF_NQ], adf_rat_t qr[ADF_NQ])
{
    slong i;

    for (i = 0; i < ADF_NQ; i++)
    {
        fmpq_clear(qs[i]);
        adf_rat_clear(qr[i]);
    }
}

/* --------------------------------------------------------------- inputs */

/* An adele whose real part is the ball of q at 200 bits and whose finite part is the exact 1. */
static void
build_adele(adf_adele_t x, const fmpq_t q)
{
    arb_t r;
    adf_fball_t f;

    arb_init(r);
    adf_fball_init(f);
    arb_set_fmpq(r, q, 200);
    adf_fball_set_si(f, 1);
    ADF_CHECK(adf_adele_set_arb_fball(x, r, f) == ADF_OK);
    arb_clear(r);
    adf_fball_clear(f);
}

/* --------------------------------------------------------------- checks */

static void
check_adele(const adf_adele_t z, const fmpq_t ex, const char *what, slong prec)
{
    ADF_CHECK_MSG(adf_adele_is_canonical(z), "%s at prec %ld: the result is not canonical",
                  what, (long) prec);
    ADF_CHECK_MSG(arb_is_finite(z->inf), "%s at prec %ld: the real part is not finite", what,
                  (long) prec);
    ADF_CHECK_MSG(arb_contains_fmpq(z->inf, ex),
                  "%s at prec %ld: the real part does not contain the exact value", what,
                  (long) prec);
}

static void
check_cadele(const adf_cadele_t z, const fmpq_t ex, const char *what, slong prec)
{
    ADF_CHECK_MSG(adf_cadele_is_canonical(z), "%s at prec %ld: the result is not canonical",
                  what, (long) prec);
    ADF_CHECK_MSG(acb_is_finite(z->inf), "%s at prec %ld: the complex part is not finite", what,
                  (long) prec);
    ADF_CHECK_MSG(acb_contains_fmpq(z->inf, ex),
                  "%s at prec %ld: the complex part does not contain the exact value", what,
                  (long) prec);
}

/* --------------------------------------------------------------- operation shapes */

typedef void (*adele_rat_fn)(adf_adele_t, const adf_adele_t, const adf_rat_t, slong);
typedef void (*adele_bin_fn)(adf_adele_t, const adf_adele_t, const adf_adele_t, slong);
typedef void (*cadele_rat_fn)(adf_cadele_t, const adf_cadele_t, const adf_rat_t, slong);
typedef void (*cadele_bin_fn)(adf_cadele_t, const adf_cadele_t, const adf_cadele_t, slong);
typedef void (*exact_fn)(fmpq_t, const fmpq_t, const fmpq_t);

/* The status of adf_adele_div_rat must be OK for the q values of this file (none is zero). */
static void
adele_div_rat_ok(adf_adele_t z, const adf_adele_t x, const adf_rat_t q, slong prec)
{
    ADF_CHECK_MSG(adf_adele_div_rat(z, x, q, prec) == ADF_OK,
                  "adf_adele_div_rat returned a status other than ADF_OK at prec %ld",
                  (long) prec);
}

static void
cadele_div_rat_ok(adf_cadele_t z, const adf_cadele_t x, const adf_rat_t q, slong prec)
{
    ADF_CHECK_MSG(adf_cadele_div_rat(z, x, q, prec) == ADF_OK,
                  "adf_cadele_div_rat returned a status other than ADF_OK at prec %ld",
                  (long) prec);
}

/* --------------------------------------------------------------- adf_adele: rat operations */

/* y = x op q, for q in the five values. Checks the enclosure and canonicality, the aliasing
   z = x, and that a prec below 2 gives the same value as prec = 2. */
static void
run_adele_rat(adele_rat_fn op, exact_fn exfn, const char *name)
{
    adf_adele_t x, z, ref, ref2;
    fmpq_t xre, ex, qs[ADF_NQ];
    adf_rat_t qr[ADF_NQ];
    slong i, pi;

    adf_adele_init(x);
    adf_adele_init(z);
    adf_adele_init(ref);
    adf_adele_init(ref2);
    fmpq_init(xre);
    fmpq_init(ex);
    fmpq_set_si(xre, 1, 3);
    build_adele(x, xre);
    build_qs(qs, qr);

    for (i = 0; i < ADF_NQ; i++)
    {
        exfn(ex, xre, qs[i]);
        for (pi = 0; pi < ADF_NPRECS; pi++)
        {
            slong prec = adf_precs[pi];

            op(ref, x, qr[i], prec);
            check_adele(ref, ex, name, prec);

            adf_adele_set(z, x);
            op(z, z, qr[i], prec);
            ADF_CHECK_MSG(adf_adele_identical(z, ref),
                          "%s at prec %ld: the result aliasing the input differs", name,
                          (long) prec);

            if (prec < 2)
            {
                op(ref2, x, qr[i], 2);
                ADF_CHECK_MSG(adf_adele_identical(ref, ref2),
                              "%s at prec %ld: the result differs from the result at prec 2",
                              name, (long) prec);
            }
        }
    }

    adf_adele_clear(x);
    adf_adele_clear(z);
    adf_adele_clear(ref);
    adf_adele_clear(ref2);
    fmpq_clear(xre);
    fmpq_clear(ex);
    clear_qs(qs, qr);
}

ADF_TEST(adele_add_rat_below_prec_two)
{
    run_adele_rat(adf_adele_add_rat, fmpq_add, "adf_adele_add_rat");
}

ADF_TEST(adele_mul_rat_below_prec_two)
{
    run_adele_rat(adf_adele_mul_rat, fmpq_mul, "adf_adele_mul_rat");
}

ADF_TEST(adele_div_rat_below_prec_two)
{
    run_adele_rat(adele_div_rat_ok, fmpq_div, "adf_adele_div_rat");
}

/* --------------------------------------------------------------- adf_adele: binary operations */

static void
run_adele_bin(adele_bin_fn op, exact_fn exfn, const char *name)
{
    adf_adele_t x, y, z, ref, ref2;
    fmpq_t xre, yre, ex;
    slong pi;

    adf_adele_init(x);
    adf_adele_init(y);
    adf_adele_init(z);
    adf_adele_init(ref);
    adf_adele_init(ref2);
    fmpq_init(xre);
    fmpq_init(yre);
    fmpq_init(ex);
    fmpq_set_si(xre, 1, 3);
    fmpq_set_si(yre, 1, 7);
    build_adele(x, xre);
    build_adele(y, yre);
    exfn(ex, xre, yre);

    for (pi = 0; pi < ADF_NPRECS; pi++)
    {
        slong prec = adf_precs[pi];

        op(ref, x, y, prec);
        check_adele(ref, ex, name, prec);

        /* z = x. */
        adf_adele_set(z, x);
        op(z, z, y, prec);
        ADF_CHECK_MSG(adf_adele_identical(z, ref),
                      "%s at prec %ld: the result aliasing the first input differs", name,
                      (long) prec);

        /* z = y. */
        adf_adele_set(z, y);
        op(z, x, z, prec);
        ADF_CHECK_MSG(adf_adele_identical(z, ref),
                      "%s at prec %ld: the result aliasing the second input differs", name,
                      (long) prec);

        if (prec < 2)
        {
            op(ref2, x, y, 2);
            ADF_CHECK_MSG(adf_adele_identical(ref, ref2),
                          "%s at prec %ld: the result differs from the result at prec 2", name,
                          (long) prec);
        }
    }

    adf_adele_clear(x);
    adf_adele_clear(y);
    adf_adele_clear(z);
    adf_adele_clear(ref);
    adf_adele_clear(ref2);
    fmpq_clear(xre);
    fmpq_clear(yre);
    fmpq_clear(ex);
}

ADF_TEST(adele_add_below_prec_two)
{
    run_adele_bin(adf_adele_add, fmpq_add, "adf_adele_add");
}

ADF_TEST(adele_sub_below_prec_two)
{
    run_adele_bin(adf_adele_sub, fmpq_sub, "adf_adele_sub");
}

ADF_TEST(adele_mul_below_prec_two)
{
    run_adele_bin(adf_adele_mul, fmpq_mul, "adf_adele_mul");
}

/* --------------------------------------------------------------- adf_adele: set_rat */

ADF_TEST(adele_set_rat_below_prec_two)
{
    adf_adele_t z, ref2;
    fmpq_t qs[ADF_NQ];
    adf_rat_t qr[ADF_NQ];
    slong i, pi;

    adf_adele_init(z);
    adf_adele_init(ref2);
    build_qs(qs, qr);

    for (i = 0; i < ADF_NQ; i++)
    {
        for (pi = 0; pi < ADF_NPRECS; pi++)
        {
            slong prec = adf_precs[pi];

            adf_adele_set_rat(z, qr[i], prec);
            check_adele(z, qs[i], "adf_adele_set_rat", prec);
            if (prec < 2)
            {
                adf_adele_set_rat(ref2, qr[i], 2);
                ADF_CHECK_MSG(adf_adele_identical(z, ref2),
                              "adf_adele_set_rat at prec %ld: the result differs from the result "
                              "at prec 2", (long) prec);
            }
        }
    }

    adf_adele_clear(z);
    adf_adele_clear(ref2);
    clear_qs(qs, qr);
}

/* --------------------------------------------------------------- adf_cadele: rat operations */

static void
run_cadele_rat(cadele_rat_fn op, exact_fn exfn, const char *name)
{
    adf_cadele_t x, z, ref, ref2;
    fmpq_t xre, ex, qs[ADF_NQ];
    adf_rat_t qr[ADF_NQ];
    slong i, pi;

    adf_cadele_init(x);
    adf_cadele_init(z);
    adf_cadele_init(ref);
    adf_cadele_init(ref2);
    fmpq_init(xre);
    fmpq_init(ex);
    fmpq_set_si(xre, 1, 3);
    {
        adf_adele_t a;
        adf_adele_init(a);
        build_adele(a, xre);
        adf_cadele_set_adele(x, a);
        adf_adele_clear(a);
    }
    build_qs(qs, qr);

    for (i = 0; i < ADF_NQ; i++)
    {
        exfn(ex, xre, qs[i]);
        for (pi = 0; pi < ADF_NPRECS; pi++)
        {
            slong prec = adf_precs[pi];

            op(ref, x, qr[i], prec);
            check_cadele(ref, ex, name, prec);

            adf_cadele_set(z, x);
            op(z, z, qr[i], prec);
            ADF_CHECK_MSG(adf_cadele_identical(z, ref),
                          "%s at prec %ld: the result aliasing the input differs", name,
                          (long) prec);

            if (prec < 2)
            {
                op(ref2, x, qr[i], 2);
                ADF_CHECK_MSG(adf_cadele_identical(ref, ref2),
                              "%s at prec %ld: the result differs from the result at prec 2",
                              name, (long) prec);
            }
        }
    }

    adf_cadele_clear(x);
    adf_cadele_clear(z);
    adf_cadele_clear(ref);
    adf_cadele_clear(ref2);
    fmpq_clear(xre);
    fmpq_clear(ex);
    clear_qs(qs, qr);
}

ADF_TEST(cadele_add_rat_below_prec_two)
{
    run_cadele_rat(adf_cadele_add_rat, fmpq_add, "adf_cadele_add_rat");
}

ADF_TEST(cadele_mul_rat_below_prec_two)
{
    run_cadele_rat(adf_cadele_mul_rat, fmpq_mul, "adf_cadele_mul_rat");
}

ADF_TEST(cadele_div_rat_below_prec_two)
{
    run_cadele_rat(cadele_div_rat_ok, fmpq_div, "adf_cadele_div_rat");
}

/* --------------------------------------------------------------- adf_cadele: binary operations */

static void
run_cadele_bin(cadele_bin_fn op, exact_fn exfn, const char *name)
{
    adf_cadele_t x, y, z, ref, ref2;
    fmpq_t xre, yre, ex;
    slong pi;

    adf_cadele_init(x);
    adf_cadele_init(y);
    adf_cadele_init(z);
    adf_cadele_init(ref);
    adf_cadele_init(ref2);
    fmpq_init(xre);
    fmpq_init(yre);
    fmpq_init(ex);
    fmpq_set_si(xre, 1, 3);
    fmpq_set_si(yre, 1, 7);
    {
        adf_adele_t a, b;
        adf_adele_init(a);
        adf_adele_init(b);
        build_adele(a, xre);
        build_adele(b, yre);
        adf_cadele_set_adele(x, a);
        adf_cadele_set_adele(y, b);
        adf_adele_clear(a);
        adf_adele_clear(b);
    }
    exfn(ex, xre, yre);

    for (pi = 0; pi < ADF_NPRECS; pi++)
    {
        slong prec = adf_precs[pi];

        op(ref, x, y, prec);
        check_cadele(ref, ex, name, prec);

        adf_cadele_set(z, x);
        op(z, z, y, prec);
        ADF_CHECK_MSG(adf_cadele_identical(z, ref),
                      "%s at prec %ld: the result aliasing the first input differs", name,
                      (long) prec);

        adf_cadele_set(z, y);
        op(z, x, z, prec);
        ADF_CHECK_MSG(adf_cadele_identical(z, ref),
                      "%s at prec %ld: the result aliasing the second input differs", name,
                      (long) prec);

        if (prec < 2)
        {
            op(ref2, x, y, 2);
            ADF_CHECK_MSG(adf_cadele_identical(ref, ref2),
                          "%s at prec %ld: the result differs from the result at prec 2", name,
                          (long) prec);
        }
    }

    adf_cadele_clear(x);
    adf_cadele_clear(y);
    adf_cadele_clear(z);
    adf_cadele_clear(ref);
    adf_cadele_clear(ref2);
    fmpq_clear(xre);
    fmpq_clear(yre);
    fmpq_clear(ex);
}

ADF_TEST(cadele_add_below_prec_two)
{
    run_cadele_bin(adf_cadele_add, fmpq_add, "adf_cadele_add");
}

ADF_TEST(cadele_sub_below_prec_two)
{
    run_cadele_bin(adf_cadele_sub, fmpq_sub, "adf_cadele_sub");
}

ADF_TEST(cadele_mul_below_prec_two)
{
    run_cadele_bin(adf_cadele_mul, fmpq_mul, "adf_cadele_mul");
}

/* --------------------------------------------------------------- adf_cadele: set_rat */

ADF_TEST(cadele_set_rat_below_prec_two)
{
    adf_cadele_t z, ref2;
    fmpq_t qs[ADF_NQ];
    adf_rat_t qr[ADF_NQ];
    slong i, pi;

    adf_cadele_init(z);
    adf_cadele_init(ref2);
    build_qs(qs, qr);

    for (i = 0; i < ADF_NQ; i++)
    {
        for (pi = 0; pi < ADF_NPRECS; pi++)
        {
            slong prec = adf_precs[pi];

            adf_cadele_set_rat(z, qr[i], prec);
            check_cadele(z, qs[i], "adf_cadele_set_rat", prec);
            if (prec < 2)
            {
                adf_cadele_set_rat(ref2, qr[i], 2);
                ADF_CHECK_MSG(adf_cadele_identical(z, ref2),
                              "adf_cadele_set_rat at prec %ld: the result differs from the "
                              "result at prec 2", (long) prec);
            }
        }
    }

    adf_cadele_clear(z);
    adf_cadele_clear(ref2);
    clear_qs(qs, qr);
}
