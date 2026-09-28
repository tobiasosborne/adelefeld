/* tests/test_cap.c: the absolute cap on adf_fball (docs/proofs/policies.md section 3,
   Definition 13 to Proposition 15, line 251 to 301), global backend, beyond the vector file.
   The cap on local-backend inputs (finding R1 of docs/reviews/m1/local/review.md) is covered
   separately, in tests/test_cap_local.c.

   Covers: the exact case untouched by the cap (Proposition 14.2, docs/conventions.md 5.4
   "the absolute cap"); the invariant that a capped radius of positive radius divides C
   (Proposition 15.1); idempotence (Proposition 15.2); a sum of two already-capped values
   needs no further cap (Proposition 15.3); aliasing of every permitted combination of
   arguments (docs/adelefeld/fball.h "Aliasing"); C <= 0 gives ADF_DOMAIN with every output
   untouched (docs/api-m1.md "Choices" item 5); operands of thousands of bits. */

#include <stdio.h>
#include <string.h>

#include <adelefeld/scaled.h>

#include "test_runner.h"

/* --------------------------------------------------------------- helpers */

static void
mkball_si(adf_fball_t x, slong A, slong H, slong d)
{
    fmpz_t a, h, dd;
    fmpz_init_set_si(a, A);
    fmpz_init_set_si(h, H);
    fmpz_init_set_si(dd, d);
    ADF_CHECK(adf_fball_set_fmpz3(x, a, h, dd) == ADF_OK);
    fmpz_clear(a);
    fmpz_clear(h);
    fmpz_clear(dd);
}

static int
eqball_si(const adf_fball_t x, slong A, slong H, slong d)
{
    return fmpz_equal_si(x->A, A) && fmpz_equal_si(x->H, H) && fmpz_equal_si(x->d, d);
}

static void
mkrat_si(adf_rat_t q, slong num, slong den)
{
    fmpq_set_si(q->q, num, den);
}

/* ---------------------------------------------------------------- adf_fball_cap */

ADF_TEST(cap_basic)
{
    adf_fball_t x, y;
    adf_rat_t C;

    adf_fball_init(x);
    adf_fball_init(y);
    fmpq_init(C->q);

    /* (0 + 18 Zhat)/1 capped at C = 4: gcd(18, 4) = 2. */
    mkball_si(x, 0, 18, 1);
    mkrat_si(C, 4, 1);
    ADF_CHECK(adf_fball_cap(y, x, C) == ADF_OK);
    ADF_CHECK(adf_fball_is_canonical(y));
    ADF_CHECK(eqball_si(y, 0, 2, 1));

    /* C coarser than the radius: gcd(2, 100) = 2, no change. */
    mkball_si(x, 1, 2, 1);
    mkrat_si(C, 100, 1);
    ADF_CHECK(adf_fball_cap(y, x, C) == ADF_OK);
    ADF_CHECK(eqball_si(y, 1, 2, 1));

    /* A fractional C: (0 + 1 Zhat)/1 capped at C = 1/3: gcd(1, 1/3) = 1/3. */
    mkball_si(x, 0, 1, 1);
    mkrat_si(C, 1, 3);
    ADF_CHECK(adf_fball_cap(y, x, C) == ADF_OK);
    ADF_CHECK(eqball_si(y, 0, 1, 3));

    adf_fball_clear(x);
    adf_fball_clear(y);
    fmpq_clear(C->q);
}

/* Proposition 14.2: an exact result (radius 0) keeps its tag; the cap does not touch it,
   although the literal formula gcd(0, C) = C would widen it. */
ADF_TEST(cap_exact_untouched)
{
    adf_fball_t x, y;
    adf_rat_t C;

    adf_fball_init(x);
    adf_fball_init(y);
    fmpq_init(C->q);

    mkball_si(x, 7, 0, 3);
    mkrat_si(C, 1, 5);
    ADF_CHECK(adf_fball_cap(y, x, C) == ADF_OK);
    ADF_CHECK(eqball_si(y, 7, 0, 3));

    /* Exact 0. */
    mkball_si(x, 0, 0, 1);
    mkrat_si(C, 2, 1);
    ADF_CHECK(adf_fball_cap(y, x, C) == ADF_OK);
    ADF_CHECK(eqball_si(y, 0, 0, 1));

    adf_fball_clear(x);
    adf_fball_clear(y);
    fmpq_clear(C->q);
}

/* Proposition 15.1: every capped radius of positive radius divides C. Proposition 15.2: the
   cap is idempotent. Checked over a spread of (radius, C) pairs, including radius and C
   coprime, radius a multiple of C, and C a multiple of radius. */
ADF_TEST(cap_invariant_and_idempotent)
{
    static const slong Hs[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 30 };
    static const slong Cs[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 30 };
    size_t i, j;

    for (i = 0; i < sizeof(Hs) / sizeof(Hs[0]); i++)
    {
        for (j = 0; j < sizeof(Cs) / sizeof(Cs[0]); j++)
        {
            adf_fball_t x, y, z;
            adf_rat_t C;
            fmpq_t Hq, Cq, q;

            adf_fball_init(x);
            adf_fball_init(y);
            adf_fball_init(z);
            fmpq_init(C->q);
            mkball_si(x, 0, Hs[i], 1);
            mkrat_si(C, Cs[j], 1);
            ADF_CHECK(adf_fball_cap(y, x, C) == ADF_OK);

            /* R' | C: C / R' is an integer, checked with fmpq (Proposition 15.1). */
            fmpq_init(Hq);
            fmpq_init(Cq);
            fmpq_init(q);
            fmpq_set_fmpz_frac(Hq, y->H, y->d);
            fmpq_set(Cq, C->q);
            fmpq_div(q, Cq, Hq);
            ADF_CHECK_MSG(fmpz_is_one(fmpq_denref(q)), "H=%ld C=%ld: R' does not divide C",
                          (long) Hs[i], (long) Cs[j]);

            /* Idempotent: capping again changes nothing. */
            ADF_CHECK(adf_fball_cap(z, y, C) == ADF_OK);
            ADF_CHECK(adf_fball_identical(z, y));

            fmpq_clear(Hq);
            fmpq_clear(Cq);
            fmpq_clear(q);
            adf_fball_clear(x);
            adf_fball_clear(y);
            adf_fball_clear(z);
            fmpq_clear(C->q);
        }
    }
}

/* C <= 0 (zero, negative integer, negative fraction) gives ADF_DOMAIN, output untouched. */
ADF_TEST(cap_domain_on_nonpositive_C)
{
    adf_fball_t x, y;
    adf_rat_t C;

    adf_fball_init(x);
    adf_fball_init(y);
    fmpq_init(C->q);

    mkball_si(x, 1, 4, 1);
    mkball_si(y, 3, 5, 2);     /* sentinel value: must stay untouched, already canonical */
    ADF_CHECK(eqball_si(y, 3, 5, 2));

    mkrat_si(C, 0, 1);
    ADF_CHECK(adf_fball_cap(y, x, C) == ADF_DOMAIN);
    ADF_CHECK(eqball_si(y, 3, 5, 2));

    mkrat_si(C, -1, 1);
    ADF_CHECK(adf_fball_cap(y, x, C) == ADF_DOMAIN);
    ADF_CHECK(eqball_si(y, 3, 5, 2));

    mkrat_si(C, -1, 3);
    ADF_CHECK(adf_fball_cap(y, x, C) == ADF_DOMAIN);
    ADF_CHECK(eqball_si(y, 3, 5, 2));

    adf_fball_clear(x);
    adf_fball_clear(y);
    fmpq_clear(C->q);
}

/* Aliasing: y may be x. */
ADF_TEST(cap_aliasing)
{
    adf_fball_t x;
    adf_rat_t C;

    adf_fball_init(x);
    fmpq_init(C->q);

    mkball_si(x, 0, 18, 1);
    mkrat_si(C, 4, 1);
    ADF_CHECK(adf_fball_cap(x, x, C) == ADF_OK);
    ADF_CHECK(eqball_si(x, 0, 2, 1));

    adf_fball_clear(x);
    fmpq_clear(C->q);
}

/* Operands of thousands of bits. */
ADF_TEST(cap_huge_operands)
{
    adf_fball_t x, y;
    adf_rat_t C;
    fmpz_t A, H, d, cn, cd, g;

    fmpz_init(A);
    fmpz_init(H);
    fmpz_init(d);
    fmpz_init(cn);
    fmpz_init(cd);
    fmpz_init(g);

    /* H = 2^4000 * 3, C = 2^4000, gcd = 2^4000. */
    fmpz_one(H);
    fmpz_mul_2exp(H, H, 4000);
    fmpz_mul_ui(H, H, 3);
    fmpz_one(d);
    fmpz_set_si(A, 5);

    adf_fball_init(x);
    adf_fball_init(y);
    ADF_CHECK(adf_fball_set_fmpz3(x, A, H, d) == ADF_OK);

    fmpq_init(C->q);
    fmpz_one(cn);
    fmpz_mul_2exp(cn, cn, 4000);
    fmpq_set_fmpz_frac(C->q, cn, d);

    ADF_CHECK(adf_fball_cap(y, x, C) == ADF_OK);
    ADF_CHECK(adf_fball_is_canonical(y));
    /* Expected radius: gcd(2^4000 * 3, 2^4000) = 2^4000; centre 5 mod that radius, d = 1. */
    fmpz_one(g);
    fmpz_mul_2exp(g, g, 4000);
    ADF_CHECK(fmpz_equal(y->H, g));
    ADF_CHECK(fmpz_equal_si(y->d, 1));
    ADF_CHECK(fmpz_equal_si(y->A, 5));

    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
    fmpz_clear(cn);
    fmpz_clear(cd);
    fmpz_clear(g);
    adf_fball_clear(x);
    adf_fball_clear(y);
    fmpq_clear(C->q);
}

/* ---------------------------------------------------------------- adf_fball_add_cap */

/* The tight sum (a + N Zhat) + (b + M Zhat) = (a + b) + gcd(N, M) Zhat (docs/proofs/
   precision.md Proposition 1), then the cap (Definition 13). */
ADF_TEST(add_cap_basic)
{
    adf_fball_t x, y, z;
    adf_rat_t C;

    adf_fball_init(x);
    adf_fball_init(y);
    adf_fball_init(z);
    fmpq_init(C->q);

    /* (0 + 8 Zhat) + (0 + 12 Zhat) = 0 + gcd(8,12) Zhat = 0 + 4 Zhat; capped at C = 6:
       gcd(4, 6) = 2. */
    mkball_si(x, 0, 8, 1);
    mkball_si(y, 0, 12, 1);
    mkrat_si(C, 6, 1);
    ADF_CHECK(adf_fball_add_cap(z, x, y, C) == ADF_OK);
    ADF_CHECK(adf_fball_is_canonical(z));
    ADF_CHECK(eqball_si(z, 0, 2, 1));

    adf_fball_clear(x);
    adf_fball_clear(y);
    adf_fball_clear(z);
    fmpq_clear(C->q);
}

/* Proposition 14.2: exact + exact is exact; the cap does not touch it. */
ADF_TEST(add_cap_exact_untouched)
{
    adf_fball_t x, y, z;
    adf_rat_t C;

    adf_fball_init(x);
    adf_fball_init(y);
    adf_fball_init(z);
    fmpq_init(C->q);

    mkball_si(x, 2, 0, 3);      /* 2/3 */
    mkball_si(y, 1, 0, 6);      /* 1/6 */
    mkrat_si(C, 1, 100);
    ADF_CHECK(adf_fball_add_cap(z, x, y, C) == ADF_OK);
    ADF_CHECK(eqball_si(z, 5, 0, 6));   /* 2/3 + 1/6 = 5/6, exact */

    adf_fball_clear(x);
    adf_fball_clear(y);
    adf_fball_clear(z);
    fmpq_clear(C->q);
}

/* Proposition 15.3: the sum of two already-capped values needs no further cap. */
ADF_TEST(add_cap_of_two_capped_needs_no_cap)
{
    adf_fball_t x, y, z, cx, cy;
    adf_rat_t C;

    adf_fball_init(x);
    adf_fball_init(y);
    adf_fball_init(z);
    adf_fball_init(cx);
    adf_fball_init(cy);
    fmpq_init(C->q);

    mkball_si(x, 0, 7, 1);
    mkball_si(y, 0, 20, 1);
    mkrat_si(C, 12, 1);
    ADF_CHECK(adf_fball_cap(cx, x, C) == ADF_OK);   /* gcd(7,12)=1 */
    ADF_CHECK(adf_fball_cap(cy, y, C) == ADF_OK);   /* gcd(20,12)=4 */
    ADF_CHECK(adf_fball_add_cap(z, cx, cy, C) == ADF_OK);
    /* tight sum of the capped radii: gcd(1, 4) = 1, already dividing C = 12. */
    ADF_CHECK(eqball_si(z, 0, 1, 1));

    adf_fball_clear(x);
    adf_fball_clear(y);
    adf_fball_clear(z);
    adf_fball_clear(cx);
    adf_fball_clear(cy);
    fmpq_clear(C->q);
}

/* C <= 0: ADF_DOMAIN, output untouched. */
ADF_TEST(add_cap_domain_on_nonpositive_C)
{
    adf_fball_t x, y, z;
    adf_rat_t C;

    adf_fball_init(x);
    adf_fball_init(y);
    adf_fball_init(z);
    fmpq_init(C->q);

    mkball_si(x, 0, 8, 1);
    mkball_si(y, 0, 12, 1);
    mkball_si(z, 1, 3, 1);      /* sentinel, canonical */
    mkrat_si(C, 0, 1);
    ADF_CHECK(adf_fball_add_cap(z, x, y, C) == ADF_DOMAIN);
    ADF_CHECK(eqball_si(z, 1, 3, 1));

    adf_fball_clear(x);
    adf_fball_clear(y);
    adf_fball_clear(z);
    fmpq_clear(C->q);
}

/* Aliasing: z may be x, y, or both. */
ADF_TEST(add_cap_aliasing)
{
    adf_fball_t x, y;
    adf_rat_t C;

    adf_fball_init(x);
    adf_fball_init(y);
    fmpq_init(C->q);
    mkrat_si(C, 6, 1);

    mkball_si(x, 0, 8, 1);
    mkball_si(y, 0, 12, 1);
    ADF_CHECK(adf_fball_add_cap(x, x, y, C) == ADF_OK);
    ADF_CHECK(eqball_si(x, 0, 2, 1));

    mkball_si(x, 0, 8, 1);
    mkball_si(y, 0, 12, 1);
    ADF_CHECK(adf_fball_add_cap(y, x, y, C) == ADF_OK);
    ADF_CHECK(eqball_si(y, 0, 2, 1));

    mkball_si(x, 0, 8, 1);
    ADF_CHECK(adf_fball_add_cap(x, x, x, C) == ADF_OK);
    /* x + x = 0 + gcd(8,8) Zhat = 0 + 8 Zhat, capped at 6: gcd(8,6) = 2. */
    ADF_CHECK(eqball_si(x, 0, 2, 1));

    adf_fball_clear(x);
    adf_fball_clear(y);
    fmpq_clear(C->q);
}

/* ---------------------------------------------------------------- adf_fball_sub_cap */

/* z = x - y = x + (-y) as sets (docs/proofs/precision.md Proposition 1, with negation), then
   the cap. */
ADF_TEST(sub_cap_basic)
{
    adf_fball_t x, y, z;
    adf_rat_t C;

    adf_fball_init(x);
    adf_fball_init(y);
    adf_fball_init(z);
    fmpq_init(C->q);

    /* (1 + 8 Zhat) - (0 + 12 Zhat) = 1 + gcd(8,12) Zhat = 1 + 4 Zhat; capped at C = 6:
       gcd(4, 6) = 2, centre 1 reduced mod 2 stays 1. */
    mkball_si(x, 1, 8, 1);
    mkball_si(y, 0, 12, 1);
    mkrat_si(C, 6, 1);
    ADF_CHECK(adf_fball_sub_cap(z, x, y, C) == ADF_OK);
    ADF_CHECK(adf_fball_is_canonical(z));
    ADF_CHECK(eqball_si(z, 1, 2, 1));

    adf_fball_clear(x);
    adf_fball_clear(y);
    adf_fball_clear(z);
    fmpq_clear(C->q);
}

ADF_TEST(sub_cap_exact_untouched)
{
    adf_fball_t x, y, z;
    adf_rat_t C;

    adf_fball_init(x);
    adf_fball_init(y);
    adf_fball_init(z);
    fmpq_init(C->q);

    mkball_si(x, 5, 0, 6);      /* 5/6 */
    mkball_si(y, 1, 0, 6);      /* 1/6 */
    mkrat_si(C, 1, 100);
    ADF_CHECK(adf_fball_sub_cap(z, x, y, C) == ADF_OK);
    ADF_CHECK(eqball_si(z, 2, 0, 3));    /* 5/6 - 1/6 = 2/3, exact */

    adf_fball_clear(x);
    adf_fball_clear(y);
    adf_fball_clear(z);
    fmpq_clear(C->q);
}

ADF_TEST(sub_cap_domain_on_nonpositive_C)
{
    adf_fball_t x, y, z;
    adf_rat_t C;

    adf_fball_init(x);
    adf_fball_init(y);
    adf_fball_init(z);
    fmpq_init(C->q);

    mkball_si(x, 0, 8, 1);
    mkball_si(y, 0, 12, 1);
    mkball_si(z, 1, 3, 1);
    mkrat_si(C, -3, 1);
    ADF_CHECK(adf_fball_sub_cap(z, x, y, C) == ADF_DOMAIN);
    ADF_CHECK(eqball_si(z, 1, 3, 1));

    adf_fball_clear(x);
    adf_fball_clear(y);
    adf_fball_clear(z);
    fmpq_clear(C->q);
}

/* Aliasing: z may be x, y, or both. x - x = the exact 0, untouched by the cap even when z
   aliases x. */
ADF_TEST(sub_cap_aliasing)
{
    adf_fball_t x, y;
    adf_rat_t C;

    adf_fball_init(x);
    adf_fball_init(y);
    fmpq_init(C->q);
    mkrat_si(C, 6, 1);

    mkball_si(x, 1, 8, 1);
    mkball_si(y, 0, 12, 1);
    ADF_CHECK(adf_fball_sub_cap(x, x, y, C) == ADF_OK);
    ADF_CHECK(eqball_si(x, 1, 2, 1));

    mkball_si(x, 1, 8, 1);
    mkball_si(y, 0, 12, 1);
    ADF_CHECK(adf_fball_sub_cap(y, x, y, C) == ADF_OK);
    ADF_CHECK(eqball_si(y, 1, 2, 1));

    /* x - x as sets, not as a dependent difference: (a - a) + gcd(N, N) Zhat = 0 + N Zhat
       (docs/proofs/precision.md Proposition 1 applies to independent representatives, exactly
       as adf_fball_sub(z, x, x) does; the library does not track this correlation). Capped at
       C = 6: gcd(8, 6) = 2. */
    mkball_si(x, 1, 8, 1);
    ADF_CHECK(adf_fball_sub_cap(x, x, x, C) == ADF_OK);
    ADF_CHECK(eqball_si(x, 0, 2, 1));

    adf_fball_clear(x);
    adf_fball_clear(y);
    fmpq_clear(C->q);
}

/* ---------------------------------------------------------------- adf_fball_mul_cap */

/* z = a b + gcd(a M, b N, N M) Zhat (docs/proofs/precision.md Proposition 2), then the cap. */
ADF_TEST(mul_cap_basic)
{
    adf_fball_t x, y, z;
    adf_rat_t C;

    adf_fball_init(x);
    adf_fball_init(y);
    adf_fball_init(z);
    fmpq_init(C->q);

    /* a=0,N=8; b=0,M=12: a b = 0, gcd(a M, b N, N M) = gcd(0,0,96) = 96; capped at C = 10:
       gcd(96, 10) = 2. */
    mkball_si(x, 0, 8, 1);
    mkball_si(y, 0, 12, 1);
    mkrat_si(C, 10, 1);
    ADF_CHECK(adf_fball_mul_cap(z, x, y, C) == ADF_OK);
    ADF_CHECK(adf_fball_is_canonical(z));
    ADF_CHECK(eqball_si(z, 0, 2, 1));

    adf_fball_clear(x);
    adf_fball_clear(y);
    adf_fball_clear(z);
    fmpq_clear(C->q);
}

ADF_TEST(mul_cap_exact_untouched)
{
    adf_fball_t x, y, z;
    adf_rat_t C;

    adf_fball_init(x);
    adf_fball_init(y);
    adf_fball_init(z);
    fmpq_init(C->q);

    mkball_si(x, 3, 0, 4);      /* 3/4 */
    mkball_si(y, 2, 0, 3);      /* 2/3 */
    mkrat_si(C, 1, 100);
    ADF_CHECK(adf_fball_mul_cap(z, x, y, C) == ADF_OK);
    ADF_CHECK(eqball_si(z, 1, 0, 2));    /* 3/4 * 2/3 = 1/2, exact */

    adf_fball_clear(x);
    adf_fball_clear(y);
    adf_fball_clear(z);
    fmpq_clear(C->q);
}

ADF_TEST(mul_cap_domain_on_nonpositive_C)
{
    adf_fball_t x, y, z;
    adf_rat_t C;

    adf_fball_init(x);
    adf_fball_init(y);
    adf_fball_init(z);
    fmpq_init(C->q);

    mkball_si(x, 0, 8, 1);
    mkball_si(y, 0, 12, 1);
    mkball_si(z, 1, 3, 1);
    mkrat_si(C, 0, 1);
    ADF_CHECK(adf_fball_mul_cap(z, x, y, C) == ADF_DOMAIN);
    ADF_CHECK(eqball_si(z, 1, 3, 1));

    adf_fball_clear(x);
    adf_fball_clear(y);
    adf_fball_clear(z);
    fmpq_clear(C->q);
}

/* Aliasing: z may be x, y, or both. */
ADF_TEST(mul_cap_aliasing)
{
    adf_fball_t x, y;
    adf_rat_t C;

    adf_fball_init(x);
    adf_fball_init(y);
    fmpq_init(C->q);
    mkrat_si(C, 10, 1);

    mkball_si(x, 0, 8, 1);
    mkball_si(y, 0, 12, 1);
    ADF_CHECK(adf_fball_mul_cap(x, x, y, C) == ADF_OK);
    ADF_CHECK(eqball_si(x, 0, 2, 1));

    mkball_si(x, 0, 8, 1);
    mkball_si(y, 0, 12, 1);
    ADF_CHECK(adf_fball_mul_cap(y, x, y, C) == ADF_OK);
    ADF_CHECK(eqball_si(y, 0, 2, 1));

    /* x * x: a b = 0, gcd(a M, b N, N M) = gcd(0, 0, 64) = 64; capped at 10: gcd(64,10) = 2. */
    mkball_si(x, 0, 8, 1);
    ADF_CHECK(adf_fball_mul_cap(x, x, x, C) == ADF_OK);
    ADF_CHECK(eqball_si(x, 0, 2, 1));

    adf_fball_clear(x);
    adf_fball_clear(y);
    fmpq_clear(C->q);
}

/* ------------------------------------------------------------ adf_fball_mul_rat_cap */

/* y = q a + |q| N Zhat for q != 0 (docs/adelefeld/fball.h adf_fball_mul_rat), then the cap
   (the cap acts on the product of an exact scalar with a ball, Proposition 14.2). */
ADF_TEST(mul_rat_cap_basic)
{
    adf_fball_t x, y;
    adf_rat_t q, C;

    adf_fball_init(x);
    adf_fball_init(y);
    fmpq_init(q->q);
    fmpq_init(C->q);

    /* 3 * (0 + 8 Zhat) = 0 + 24 Zhat; capped at C = 10: gcd(24, 10) = 2. */
    mkball_si(x, 0, 8, 1);
    mkrat_si(q, 3, 1);
    mkrat_si(C, 10, 1);
    ADF_CHECK(adf_fball_mul_rat_cap(y, x, q, C) == ADF_OK);
    ADF_CHECK(adf_fball_is_canonical(y));
    ADF_CHECK(eqball_si(y, 0, 2, 1));

    adf_fball_clear(x);
    adf_fball_clear(y);
    fmpq_clear(q->q);
    fmpq_clear(C->q);
}

/* q = 0 gives the exact 0, untouched by the cap. An exact x times any q stays exact, also
   untouched (Proposition 14.2, CV-47). */
ADF_TEST(mul_rat_cap_exact_untouched)
{
    adf_fball_t x, y;
    adf_rat_t q, C;

    adf_fball_init(x);
    adf_fball_init(y);
    fmpq_init(q->q);
    fmpq_init(C->q);

    mkball_si(x, 0, 8, 1);
    mkrat_si(q, 0, 1);
    mkrat_si(C, 1, 100);
    ADF_CHECK(adf_fball_mul_rat_cap(y, x, q, C) == ADF_OK);
    ADF_CHECK(eqball_si(y, 0, 0, 1));

    mkball_si(x, 3, 0, 4);      /* 3/4, exact */
    mkrat_si(q, -2, 3);
    mkrat_si(C, 1, 1000);
    ADF_CHECK(adf_fball_mul_rat_cap(y, x, q, C) == ADF_OK);
    ADF_CHECK(eqball_si(y, -1, 0, 2));   /* 3/4 * (-2/3) = -1/2, exact */

    adf_fball_clear(x);
    adf_fball_clear(y);
    fmpq_clear(q->q);
    fmpq_clear(C->q);
}

ADF_TEST(mul_rat_cap_domain_on_nonpositive_C)
{
    adf_fball_t x, y;
    adf_rat_t q, C;

    adf_fball_init(x);
    adf_fball_init(y);
    fmpq_init(q->q);
    fmpq_init(C->q);

    mkball_si(x, 0, 8, 1);
    mkball_si(y, 1, 3, 1);
    mkrat_si(q, 3, 1);
    mkrat_si(C, 0, 1);
    ADF_CHECK(adf_fball_mul_rat_cap(y, x, q, C) == ADF_DOMAIN);
    ADF_CHECK(eqball_si(y, 1, 3, 1));

    adf_fball_clear(x);
    adf_fball_clear(y);
    fmpq_clear(q->q);
    fmpq_clear(C->q);
}

/* Aliasing: y may be x. */
ADF_TEST(mul_rat_cap_aliasing)
{
    adf_fball_t x;
    adf_rat_t q, C;

    adf_fball_init(x);
    fmpq_init(q->q);
    fmpq_init(C->q);

    mkball_si(x, 0, 8, 1);
    mkrat_si(q, 3, 1);
    mkrat_si(C, 10, 1);
    ADF_CHECK(adf_fball_mul_rat_cap(x, x, q, C) == ADF_OK);
    ADF_CHECK(eqball_si(x, 0, 2, 1));

    adf_fball_clear(x);
    fmpq_clear(q->q);
    fmpq_clear(C->q);
}
