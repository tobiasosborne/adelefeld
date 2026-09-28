/* tests/test_cap_local.c: the absolute cap (adelefeld/scaled.h) on local-backend adf_fball
   inputs (finding R1 of docs/reviews/m1/local/review.md).

   Ground truth: docs/proofs/policies.md section 3, Definition 13 (line 253) to Proposition 15
   (line 283), for the cap itself; section 4, Definition 16 (line 305) to Proposition 24
   (line 457), for the local backend, its conversions and the implicit global fallback.
   docs/conventions.md 5.3 ("adf_fball, local backend": raw local data is the stored form,
   "results that leave the context ... are global"); 5.4 ("the absolute cap": the cap block
   gives no backend restriction). include/adelefeld/fball.h, line 16: "Every function of this
   header is defined on sets and accepts inputs of either backend"; include/adelefeld/scaled.h,
   line 160 to 178 (the cap block); include/adelefeld/modctx.h (adf_fball_set_local,
   adf_fball_set_local_enclose, adf_fball_set_global, adf_fball_is_local).

   Finding R1: before this test was written, all five cap functions of src/cap.c refused every
   local input with ADF_UNSUPPORTED, output untouched, whatever the set; neither
   adelefeld/scaled.h nor the local backend of adelefeld/fball.h and adelefeld/modctx.h give
   that restriction (docs/reviews/m1/local/review.md, "R1 - MAJOR"). This file is the red test
   of the repair: run against the refusing code it fails (the red run of the lane report;
   reproduced independently by docs/reviews/m1/local/checks/cap_local.c, local statuses
   8,8,8,8,8), then src/cap.c is changed to remove the refusal.

   Method: a cap function is defined on sets (docs/proofs/policies.md Definition 13: "After a
   tight operation with result c + R Zhat ..."), not on a representation, so a local input and
   the global form of the same value must give results that are the same set. Every scenario
   below builds a local value by converting a global ball into a context, with
   adf_fball_set_local (exact conversion) or adf_fball_set_local_enclose (best enclosure, which
   always succeeds for a ball of positive radius, so any ball converts), applies a cap function,
   and compares against the same function applied to the global form of the same local input(s)
   (adf_fball_set_global): the header status must agree, and the results must be equal as sets
   (adf_fball_equal_set, docs/proofs/precision.md Proposition 3). Contexts of 1, 2 and 64 blocks
   are covered; mixed local/global inputs; two local inputs of different context pointers
   (conventions 5.3, "results that leave the context ... are global"); every permitted aliasing
   of the output with an input; C <= 0; and a local source that becomes an exact result
   (mul_rat_cap with q = 0), which the cap must not touch (Proposition 14.2, CV-47). */

#include <stdio.h>

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

/* The first 64 primes: pairwise coprime by construction, so any prefix is a valid block list
   for adf_modctx_new_blocks (include/adelefeld/modctx.h: "pairwise coprime, 2 <= q_i < 2^64").
   Contexts of 1, 2 and 64 blocks are the prefixes of length 1, 2 and 64. */
static const ulong primes64[64] = {
    2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53,
    59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131,
    137, 139, 149, 151, 157, 163, 167, 173, 179, 181, 191, 193, 197, 199, 211, 223,
    227, 229, 233, 239, 241, 251, 257, 263, 269, 271, 277, 281, 283, 293, 307, 311
};

static void
make_ctx(adf_modctx_struct ** ctx, slong k)
{
    ADF_CHECK(adf_modctx_new_blocks(ctx, primes64, k) == ADF_OK);
}

/* x = the best enclosure, in ctx, of the global ball (A, H, d) (adelefeld/modctx.h
   adf_fball_set_local_enclose): always ADF_OK for H > 0, whatever ctx (policies Proposition 20,
   line 361). */
static void
mklocal_enclose_si(adf_fball_t x, slong A, slong H, slong d, const adf_modctx_struct * ctx)
{
    adf_fball_t g;
    int lost;

    adf_fball_init(g);
    mkball_si(g, A, H, d);
    ADF_CHECK(adf_fball_set_local_enclose(x, &lost, g, ctx) == ADF_OK);
    ADF_CHECK(adf_fball_is_local(x));
    adf_fball_clear(g);
}

/* x = the exact conversion, in ctx, of the ball c + K Zhat, K the modulus of ctx
   (adelefeld/modctx.h adf_fball_set_local): K/K = 1 and c K/K = c are always integers
   (policies Proposition 19, line 347), so this always succeeds, for any ctx and any c. */
static void
mklocal_exact_wide(adf_fball_t x, slong c, const adf_modctx_struct * ctx)
{
    fmpz_t K;
    adf_fball_t g;
    adf_rat_t cc, N;

    fmpz_init(K);
    adf_modctx_get_modulus(K, ctx);
    adf_fball_init(g);
    fmpq_init(cc->q);
    fmpq_init(N->q);
    fmpq_set_si(cc->q, c, 1);
    fmpq_set_fmpz(N->q, K);
    ADF_CHECK(adf_fball_set_center_radius(g, cc, N) == ADF_OK);
    ADF_CHECK(adf_fball_set_local(x, g, ctx) == ADF_OK);
    ADF_CHECK(adf_fball_is_local(x));
    fmpq_clear(cc->q);
    fmpq_clear(N->q);
    adf_fball_clear(g);
    fmpz_clear(K);
}

/* ------------------------------------ 1. local input(s) match the global form of the same value */

/* For contexts of 1, 2 and 64 blocks: each of the five functions, applied to local input(s),
   returns the header status (ADF_OK here, C > 0) and a result equal as a set to the same
   function applied to the global form of the same input(s) (adf_fball_set_global). Several
   source balls per context, so both adf_fball_set_local_enclose (lossy in general) and
   adf_fball_set_local (exact, via mklocal_exact_wide) are exercised. */
ADF_TEST(local_matches_global_of_same_value)
{
    static const slong ks[3] = { 1, 2, 64 };
    /* (A, H, d) of two source balls, and the exact-wide centre used for the third variant. */
    static const slong src1[3] = { 1, 12, 1 };
    static const slong src2[3] = { 5, 20, 3 };
    int ki, variant;

    for (ki = 0; ki < 3; ki++)
    {
        for (variant = 0; variant < 2; variant++)
        {
            adf_modctx_struct *ctx = NULL;
            adf_fball_t xloc, yloc, xg, yg, rloc, rglob;
            adf_rat_t q, C;

            make_ctx(&ctx, ks[ki]);
            adf_fball_init(xloc);
            adf_fball_init(yloc);
            adf_fball_init(xg);
            adf_fball_init(yg);
            adf_fball_init(rloc);
            adf_fball_init(rglob);
            fmpq_init(q->q);
            fmpq_init(C->q);

            if (variant == 0)
            {
                mklocal_enclose_si(xloc, src1[0], src1[1], src1[2], ctx);
                mklocal_enclose_si(yloc, src2[0], src2[1], src2[2], ctx);
            }
            else
            {
                mklocal_exact_wide(xloc, 3, ctx);
                mklocal_exact_wide(yloc, -2, ctx);
            }
            adf_fball_set_global(xg, xloc);
            adf_fball_set_global(yg, yloc);
            mkrat_si(q, 3, 2);
            mkrat_si(C, 7, 1);

            /* adf_fball_cap */
            {
                int s1 = adf_fball_cap(rloc, xloc, C);
                int s2 = adf_fball_cap(rglob, xg, C);
                ADF_CHECK_MSG(s1 == ADF_OK, "k=%ld v=%d cap: local status %d", (long) ks[ki],
                              variant, s1);
                ADF_CHECK(s1 == s2);
                ADF_CHECK(adf_fball_equal_set(rloc, rglob));
            }
            /* adf_fball_add_cap */
            {
                int s1 = adf_fball_add_cap(rloc, xloc, yloc, C);
                int s2 = adf_fball_add_cap(rglob, xg, yg, C);
                ADF_CHECK_MSG(s1 == ADF_OK, "k=%ld v=%d add_cap: local status %d", (long) ks[ki],
                              variant, s1);
                ADF_CHECK(s1 == s2);
                ADF_CHECK(adf_fball_equal_set(rloc, rglob));
            }
            /* adf_fball_sub_cap */
            {
                int s1 = adf_fball_sub_cap(rloc, xloc, yloc, C);
                int s2 = adf_fball_sub_cap(rglob, xg, yg, C);
                ADF_CHECK_MSG(s1 == ADF_OK, "k=%ld v=%d sub_cap: local status %d", (long) ks[ki],
                              variant, s1);
                ADF_CHECK(s1 == s2);
                ADF_CHECK(adf_fball_equal_set(rloc, rglob));
            }
            /* adf_fball_mul_cap */
            {
                int s1 = adf_fball_mul_cap(rloc, xloc, yloc, C);
                int s2 = adf_fball_mul_cap(rglob, xg, yg, C);
                ADF_CHECK_MSG(s1 == ADF_OK, "k=%ld v=%d mul_cap: local status %d", (long) ks[ki],
                              variant, s1);
                ADF_CHECK(s1 == s2);
                ADF_CHECK(adf_fball_equal_set(rloc, rglob));
            }
            /* adf_fball_mul_rat_cap */
            {
                int s1 = adf_fball_mul_rat_cap(rloc, xloc, q, C);
                int s2 = adf_fball_mul_rat_cap(rglob, xg, q, C);
                ADF_CHECK_MSG(s1 == ADF_OK, "k=%ld v=%d mul_rat_cap: local status %d",
                              (long) ks[ki], variant, s1);
                ADF_CHECK(s1 == s2);
                ADF_CHECK(adf_fball_equal_set(rloc, rglob));
            }

            adf_fball_clear(xloc);
            adf_fball_clear(yloc);
            adf_fball_clear(xg);
            adf_fball_clear(yg);
            adf_fball_clear(rloc);
            adf_fball_clear(rglob);
            fmpq_clear(q->q);
            fmpq_clear(C->q);
            adf_modctx_free(ctx);
        }
    }
}

/* ------------------------------------------------- 2. mixed local and global inputs */

/* One local input, one global input, in both argument positions where the operation is not
   symmetric (sub_cap). Matches the same computation on the global form of the local input. */
ADF_TEST(mixed_local_global_inputs)
{
    adf_modctx_struct *ctx = NULL;
    adf_fball_t xloc, xg, yglob, rloc, rglob;
    adf_rat_t C;

    make_ctx(&ctx, 2);
    adf_fball_init(xloc);
    adf_fball_init(xg);
    adf_fball_init(yglob);
    adf_fball_init(rloc);
    adf_fball_init(rglob);
    fmpq_init(C->q);

    mklocal_enclose_si(xloc, 2, 9, 1, ctx);
    adf_fball_set_global(xg, xloc);
    mkball_si(yglob, 1, 5, 2);
    mkrat_si(C, 4, 1);

    /* add_cap(local, global) */
    ADF_CHECK(adf_fball_add_cap(rloc, xloc, yglob, C) == ADF_OK);
    ADF_CHECK(adf_fball_add_cap(rglob, xg, yglob, C) == ADF_OK);
    ADF_CHECK(adf_fball_equal_set(rloc, rglob));

    /* sub_cap(local, global) */
    ADF_CHECK(adf_fball_sub_cap(rloc, xloc, yglob, C) == ADF_OK);
    ADF_CHECK(adf_fball_sub_cap(rglob, xg, yglob, C) == ADF_OK);
    ADF_CHECK(adf_fball_equal_set(rloc, rglob));

    /* sub_cap(global, local): the local input in the second position. */
    ADF_CHECK(adf_fball_sub_cap(rloc, yglob, xloc, C) == ADF_OK);
    ADF_CHECK(adf_fball_sub_cap(rglob, yglob, xg, C) == ADF_OK);
    ADF_CHECK(adf_fball_equal_set(rloc, rglob));

    /* mul_cap(local, global) */
    ADF_CHECK(adf_fball_mul_cap(rloc, xloc, yglob, C) == ADF_OK);
    ADF_CHECK(adf_fball_mul_cap(rglob, xg, yglob, C) == ADF_OK);
    ADF_CHECK(adf_fball_equal_set(rloc, rglob));

    adf_fball_clear(xloc);
    adf_fball_clear(xg);
    adf_fball_clear(yglob);
    adf_fball_clear(rloc);
    adf_fball_clear(rglob);
    fmpq_clear(C->q);
    adf_modctx_free(ctx);
}

/* ------------------------------------------- 3. two local inputs of different contexts */

/* Two different context pointers give a global result (conventions 5.3: "results that leave
   the context ... are global", the fallback of 4.6, gate finding G1), whatever the moduli. */
ADF_TEST(local_inputs_different_contexts)
{
    adf_modctx_struct *ctx1 = NULL, *ctx2 = NULL;
    adf_fball_t xloc, yloc, xg, yg, rloc, rglob;
    adf_rat_t C;

    make_ctx(&ctx1, 1);
    make_ctx(&ctx2, 2);
    adf_fball_init(xloc);
    adf_fball_init(yloc);
    adf_fball_init(xg);
    adf_fball_init(yg);
    adf_fball_init(rloc);
    adf_fball_init(rglob);
    fmpq_init(C->q);

    mklocal_enclose_si(xloc, 1, 7, 1, ctx1);
    mklocal_enclose_si(yloc, 2, 11, 1, ctx2);
    adf_fball_set_global(xg, xloc);
    adf_fball_set_global(yg, yloc);
    mkrat_si(C, 5, 1);

    ADF_CHECK(adf_fball_add_cap(rloc, xloc, yloc, C) == ADF_OK);
    ADF_CHECK(adf_fball_add_cap(rglob, xg, yg, C) == ADF_OK);
    ADF_CHECK(adf_fball_equal_set(rloc, rglob));

    ADF_CHECK(adf_fball_sub_cap(rloc, xloc, yloc, C) == ADF_OK);
    ADF_CHECK(adf_fball_sub_cap(rglob, xg, yg, C) == ADF_OK);
    ADF_CHECK(adf_fball_equal_set(rloc, rglob));

    ADF_CHECK(adf_fball_mul_cap(rloc, xloc, yloc, C) == ADF_OK);
    ADF_CHECK(adf_fball_mul_cap(rglob, xg, yg, C) == ADF_OK);
    ADF_CHECK(adf_fball_equal_set(rloc, rglob));

    adf_fball_clear(xloc);
    adf_fball_clear(yloc);
    adf_fball_clear(xg);
    adf_fball_clear(yg);
    adf_fball_clear(rloc);
    adf_fball_clear(rglob);
    fmpq_clear(C->q);
    adf_modctx_free(ctx1);
    adf_modctx_free(ctx2);
}

/* ------------------------------------------------- 4. output aliasing each input, local */

/* Every permitted aliasing of the output with a local input (adelefeld/fball.h "Aliasing";
   the cap block of scaled.h: "Aliasing as in adelefeld/fball.h"). The expected result is
   computed first, from the global form of the input(s), before the aliasing call mutates the
   local input in place. */
ADF_TEST(local_output_aliases_each_input)
{
    adf_modctx_struct *ctx = NULL;
    adf_rat_t C, q;

    make_ctx(&ctx, 2);
    fmpq_init(C->q);
    fmpq_init(q->q);
    mkrat_si(C, 4, 1);
    mkrat_si(q, 3, 1);

    /* cap: y aliases x. */
    {
        adf_fball_t x, xg, expect;

        adf_fball_init(x);
        adf_fball_init(xg);
        adf_fball_init(expect);
        mklocal_enclose_si(x, 3, 10, 1, ctx);
        adf_fball_set_global(xg, x);
        ADF_CHECK(adf_fball_cap(expect, xg, C) == ADF_OK);
        ADF_CHECK(adf_fball_cap(x, x, C) == ADF_OK);
        ADF_CHECK(adf_fball_equal_set(x, expect));
        adf_fball_clear(x);
        adf_fball_clear(xg);
        adf_fball_clear(expect);
    }

    /* add_cap: z aliases x, z aliases y, z aliases both. */
    {
        adf_fball_t x, y, xg, yg, expect, z;

        adf_fball_init(x);
        adf_fball_init(y);
        adf_fball_init(xg);
        adf_fball_init(yg);
        adf_fball_init(expect);
        adf_fball_init(z);

        mklocal_enclose_si(x, 1, 8, 1, ctx);
        mklocal_enclose_si(y, 0, 12, 1, ctx);
        adf_fball_set_global(xg, x);
        adf_fball_set_global(yg, y);
        ADF_CHECK(adf_fball_add_cap(expect, xg, yg, C) == ADF_OK);

        adf_fball_set(z, x);
        ADF_CHECK(adf_fball_add_cap(z, z, y, C) == ADF_OK);
        ADF_CHECK(adf_fball_equal_set(z, expect));

        mklocal_enclose_si(x, 1, 8, 1, ctx);
        mklocal_enclose_si(y, 0, 12, 1, ctx);
        adf_fball_set(z, y);
        ADF_CHECK(adf_fball_add_cap(z, x, z, C) == ADF_OK);
        ADF_CHECK(adf_fball_equal_set(z, expect));

        mklocal_enclose_si(x, 1, 8, 1, ctx);
        adf_fball_set_global(xg, x);
        ADF_CHECK(adf_fball_add_cap(expect, xg, xg, C) == ADF_OK);
        ADF_CHECK(adf_fball_add_cap(x, x, x, C) == ADF_OK);
        ADF_CHECK(adf_fball_equal_set(x, expect));

        adf_fball_clear(x);
        adf_fball_clear(y);
        adf_fball_clear(xg);
        adf_fball_clear(yg);
        adf_fball_clear(expect);
        adf_fball_clear(z);
    }

    /* mul_cap: z aliases x, z aliases y. */
    {
        adf_fball_t x, y, xg, yg, expect;

        adf_fball_init(x);
        adf_fball_init(y);
        adf_fball_init(xg);
        adf_fball_init(yg);
        adf_fball_init(expect);

        mklocal_enclose_si(x, 1, 8, 1, ctx);
        mklocal_enclose_si(y, 0, 12, 1, ctx);
        adf_fball_set_global(xg, x);
        adf_fball_set_global(yg, y);
        ADF_CHECK(adf_fball_mul_cap(expect, xg, yg, C) == ADF_OK);
        ADF_CHECK(adf_fball_mul_cap(x, x, y, C) == ADF_OK);
        ADF_CHECK(adf_fball_equal_set(x, expect));

        mklocal_enclose_si(x, 1, 8, 1, ctx);
        mklocal_enclose_si(y, 0, 12, 1, ctx);
        ADF_CHECK(adf_fball_mul_cap(y, x, y, C) == ADF_OK);
        ADF_CHECK(adf_fball_equal_set(y, expect));

        adf_fball_clear(x);
        adf_fball_clear(y);
        adf_fball_clear(xg);
        adf_fball_clear(yg);
        adf_fball_clear(expect);
    }

    /* mul_rat_cap: y aliases x. */
    {
        adf_fball_t x, xg, expect;

        adf_fball_init(x);
        adf_fball_init(xg);
        adf_fball_init(expect);
        mklocal_enclose_si(x, 1, 8, 1, ctx);
        adf_fball_set_global(xg, x);
        ADF_CHECK(adf_fball_mul_rat_cap(expect, xg, q, C) == ADF_OK);
        ADF_CHECK(adf_fball_mul_rat_cap(x, x, q, C) == ADF_OK);
        ADF_CHECK(adf_fball_equal_set(x, expect));
        adf_fball_clear(x);
        adf_fball_clear(xg);
        adf_fball_clear(expect);
    }

    fmpq_clear(C->q);
    fmpq_clear(q->q);
    adf_modctx_free(ctx);
}

/* ------------------------------------------- 5. C <= 0 with a local input: ADF_DOMAIN, untouched */

ADF_TEST(local_domain_on_nonpositive_C)
{
    adf_modctx_struct *ctx = NULL;
    adf_fball_t xloc, yloc, out;
    adf_rat_t q, C;

    make_ctx(&ctx, 2);
    adf_fball_init(xloc);
    adf_fball_init(yloc);
    adf_fball_init(out);
    fmpq_init(q->q);
    fmpq_init(C->q);

    mklocal_enclose_si(xloc, 1, 8, 1, ctx);
    mklocal_enclose_si(yloc, 0, 12, 1, ctx);
    mkrat_si(q, 3, 1);

    mkrat_si(C, 0, 1);
    mkball_si(out, 3, 5, 2);
    ADF_CHECK(adf_fball_cap(out, xloc, C) == ADF_DOMAIN);
    ADF_CHECK(eqball_si(out, 3, 5, 2));

    mkball_si(out, 3, 5, 2);
    ADF_CHECK(adf_fball_add_cap(out, xloc, yloc, C) == ADF_DOMAIN);
    ADF_CHECK(eqball_si(out, 3, 5, 2));

    mkball_si(out, 3, 5, 2);
    ADF_CHECK(adf_fball_sub_cap(out, xloc, yloc, C) == ADF_DOMAIN);
    ADF_CHECK(eqball_si(out, 3, 5, 2));

    mkball_si(out, 3, 5, 2);
    ADF_CHECK(adf_fball_mul_cap(out, xloc, yloc, C) == ADF_DOMAIN);
    ADF_CHECK(eqball_si(out, 3, 5, 2));

    mkball_si(out, 3, 5, 2);
    ADF_CHECK(adf_fball_mul_rat_cap(out, xloc, q, C) == ADF_DOMAIN);
    ADF_CHECK(eqball_si(out, 3, 5, 2));

    /* A negative fraction, in one function, as a second check that the sign test does not
       change with the backend. */
    mkrat_si(C, -1, 3);
    mkball_si(out, 3, 5, 2);
    ADF_CHECK(adf_fball_cap(out, xloc, C) == ADF_DOMAIN);
    ADF_CHECK(eqball_si(out, 3, 5, 2));

    adf_fball_clear(xloc);
    adf_fball_clear(yloc);
    adf_fball_clear(out);
    fmpq_clear(q->q);
    fmpq_clear(C->q);
    adf_modctx_free(ctx);
}

/* ------------------------------------------- 6. a local source producing an exact result */

/* mul_rat_cap with q = 0 gives the exact 0 (adelefeld/fball.h adf_fball_mul_rat), whatever the
   backend of x; the cap does not touch an exact result (Proposition 14.2, CV-47). Matches the
   same computation on the global form of the local input. */
ADF_TEST(local_source_exact_result_untouched)
{
    adf_modctx_struct *ctx = NULL;
    adf_fball_t xloc, xg, rloc, rglob;
    adf_rat_t q, C;

    make_ctx(&ctx, 2);
    adf_fball_init(xloc);
    adf_fball_init(xg);
    adf_fball_init(rloc);
    adf_fball_init(rglob);
    fmpq_init(q->q);
    fmpq_init(C->q);

    mklocal_enclose_si(xloc, 1, 8, 1, ctx);
    adf_fball_set_global(xg, xloc);
    mkrat_si(q, 0, 1);
    mkrat_si(C, 1, 100);

    ADF_CHECK(adf_fball_mul_rat_cap(rloc, xloc, q, C) == ADF_OK);
    ADF_CHECK(adf_fball_mul_rat_cap(rglob, xg, q, C) == ADF_OK);
    ADF_CHECK(adf_fball_is_exact(rloc));
    ADF_CHECK(eqball_si(rloc, 0, 0, 1));
    ADF_CHECK(adf_fball_equal_set(rloc, rglob));

    adf_fball_clear(xloc);
    adf_fball_clear(xg);
    adf_fball_clear(rloc);
    adf_fball_clear(rglob);
    fmpq_clear(q->q);
    fmpq_clear(C->q);
    adf_modctx_free(ctx);
}
