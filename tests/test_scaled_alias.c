/* tests/test_scaled_alias.c: aliasing of the scaled policy of include/adelefeld/scaled.h
   (lane m1-repair-ctx, finding R1 of reviewer `contexts`).

   Ground truth: conventions 4.1 ("an output may be the same object as an input of the same
   type"), 4.3 (outputs untouched on a status other than ADF_OK), conventions 5.4 and closure
   C5 ("y may alias x"; *lost = 1 exactly when the set changes), docs/proofs/policies.md
   Proposition 11 (line 218) and Proposition 8 (line 159).

   For every function of scaled.h with a status or an output report (`lost`), the aliased call
   must give the same value, the same *lost and the same status as the call on copies.  The
   exhaustive part runs adf_scaled_set_context for K, K' in 1..24, every residue and the scales
   1, 1/2, 3; then 10000 random cases with moduli of up to 4096 bits.  add_rat is called with
   y = x, the binary operations with z = x, z = y and z = x = y, and the cap functions with
   every permitted aliasing.  set_fball takes an input of another type, so only the overwrite of
   the output is checked there. */

#include <stdint.h>
#include <stdio.h>

#include <flint/fmpz.h>
#include <flint/fmpq.h>

#include <adelefeld/scaled.h>

#include "test_runner.h"

/* ---- deterministic generator and helpers ---- */

static uint64_t rng_state = 0x9e3779b97f4a7c15ULL;

static uint64_t
rng_next(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;
    return rng_state;
}

static void
rat_si(adf_rat_t q, slong n, ulong d)
{
    fmpq_set_si(q->q, n, d);
}

/* x = s (u + K Zhat) in ctx, through the ball s u + s K Zhat (policies Lemma 5, line 113). */
static void
scaled_from(adf_scaled_t x, const adf_rat_t s, const fmpz_t u, const adf_modctx_struct * ctx)
{
    adf_rat_t c, r;
    adf_fball_t b;
    fmpz_t K;

    fmpz_init(K);
    adf_modctx_get_modulus(K, ctx);
    adf_rat_init(c);
    adf_rat_init(r);
    adf_fball_init(b);
    fmpq_mul_fmpz(c->q, s->q, u);
    fmpq_mul_fmpz(r->q, s->q, K);
    ADF_CHECK(adf_fball_set_center_radius(b, c, r) == ADF_OK);
    ADF_CHECK(adf_scaled_set_fball(x, NULL, b, ctx) == ADF_OK);
    adf_fball_clear(b);
    adf_rat_clear(c);
    adf_rat_clear(r);
    fmpz_clear(K);
}

/* K = a positive integer with at most `bits` bits (bit 64 gives 4096). */
static void
random_fmpz_bits(fmpz_t K, ulong bits)
{
    ulong words = (bits + 63) / 64;
    ulong i;

    fmpz_zero(K);
    for (i = 0; i < words; i++)
    {
        fmpz_mul_2exp(K, K, 64);
        fmpz_add_ui(K, K, rng_next());
    }
    if (fmpz_sgn(K) < 1)
        fmpz_one(K);
}

/* 1 if a and b are the same data (context pointer, tag, s, u). */
static int
same_data(const adf_scaled_t a, const adf_scaled_t b)
{
    return adf_scaled_identical(a, b);
}

/* The set of x as a global ball. */
static void
set_of(adf_fball_t f, const adf_scaled_t x)
{
    adf_scaled_get_fball(f, x);
}

/* ---- 1. set_context, exhaustive: K, K' in 1..24, every residue, scales 1, 1/2, 3 ---- */

ADF_TEST(set_context_alias_exhaustive)
{
    const slong sc_n[3] = {1, 1, 3};
    const ulong sc_d[3] = {1, 2, 1};
    ulong K1, K2, u;
    long cases = 0, bad_alias = 0, bad_plain = 0;
    long false0 = 0, false1 = 0;

    for (K1 = 1; K1 <= 24; K1++)
        for (K2 = 1; K2 <= 24; K2++)
        {
            adf_modctx_struct *c1 = NULL;
            adf_modctx_struct *c2 = NULL;
            fmpz_t KK;
            int si;

            fmpz_init(KK);
            fmpz_set_ui(KK, K1);
            ADF_CHECK(adf_modctx_new_fmpz(&c1, KK) == ADF_OK);
            fmpz_set_ui(KK, K2);
            ADF_CHECK(adf_modctx_new_fmpz(&c2, KK) == ADF_OK);
            fmpz_clear(KK);

            for (si = 0; si < 3; si++)
                for (u = 0; u < K1; u++)
                {
                    adf_scaled_t x, xa, y;
                    adf_rat_t s;
                    fmpz_t uu;
                    adf_fball_t before, after;
                    int lost_plain = -2, lost_alias = -2, changed;

                    adf_rat_init(s);
                    fmpz_init(uu);
                    adf_scaled_init(x, c1);
                    adf_scaled_init(xa, c1);
                    adf_scaled_init(y, c1);
                    adf_fball_init(before);
                    adf_fball_init(after);
                    rat_si(s, sc_n[si], sc_d[si]);
                    fmpz_set_ui(uu, u);
                    scaled_from(x, s, uu, c1);
                    adf_scaled_set(xa, x);
                    set_of(before, x);

                    ADF_CHECK(adf_scaled_set_context(y, &lost_plain, x, c2) == ADF_OK);
                    ADF_CHECK(adf_scaled_set_context(xa, &lost_alias, xa, c2) == ADF_OK);
                    set_of(after, y);
                    changed = !adf_fball_equal_set(before, after);
                    cases++;
                    if (lost_plain != changed)
                        bad_plain++;
                    if (lost_alias != changed)
                    {
                        bad_alias++;
                        if (changed)
                            false0++;
                        else
                            false1++;
                    }
                    if (!same_data(y, xa) || lost_plain != lost_alias)
                    {
                        bad_alias++;
                    }

                    adf_fball_clear(before);
                    adf_fball_clear(after);
                    adf_scaled_clear(x);
                    adf_scaled_clear(xa);
                    adf_scaled_clear(y);
                    adf_rat_clear(s);
                    fmpz_clear(uu);
                }
            adf_modctx_free(c1);
            adf_modctx_free(c2);
        }

    ADF_CHECK_MSG(cases == 21600, "exhaustive cases %ld", cases);
    ADF_CHECK_MSG(bad_plain == 0, "set_context plain lost wrong in %ld of %ld", bad_plain, cases);
    ADF_CHECK_MSG(bad_alias == 0, "set_context aliased lost wrong in %ld of %ld "
                  "(set changed but lost 0: %ld; set unchanged but lost 1: %ld)",
                  bad_alias, cases, false0, false1);
}

/* ---- 2. set_context, exact input and y = x ---- */

ADF_TEST(set_context_alias_exact)
{
    adf_modctx_struct *c1 = NULL;
    adf_modctx_struct *c2 = NULL;
    adf_scaled_t x, xa, y;
    adf_rat_t s;
    fmpz_t KK;
    int lost_plain = -2, lost_alias = -2;

    fmpz_init(KK);
    fmpz_set_ui(KK, 6);
    ADF_CHECK(adf_modctx_new_fmpz(&c1, KK) == ADF_OK);
    fmpz_set_ui(KK, 4);
    ADF_CHECK(adf_modctx_new_fmpz(&c2, KK) == ADF_OK);
    adf_rat_init(s);
    rat_si(s, -5, 3);
    adf_scaled_init(x, c1);
    adf_scaled_init(xa, c1);
    adf_scaled_init(y, c1);
    adf_scaled_set_rat(x, s, c1);
    adf_scaled_set(xa, x);
    ADF_CHECK(adf_scaled_set_context(y, &lost_plain, x, c2) == ADF_OK);
    ADF_CHECK(adf_scaled_set_context(xa, &lost_alias, xa, c2) == ADF_OK);
    ADF_CHECK(same_data(y, xa));
    ADF_CHECK(lost_plain == 0 && lost_alias == 0);
    adf_scaled_clear(x);
    adf_scaled_clear(xa);
    adf_scaled_clear(y);
    adf_rat_clear(s);
    fmpz_clear(KK);
    adf_modctx_free(c1);
    adf_modctx_free(c2);
}

/* ---- 3. set_context, 10000 random cases with moduli up to 4096 bits ---- */

ADF_TEST(set_context_alias_random)
{
    long i, cases = 0, bad_alias = 0, bad_plain = 0;

    for (i = 0; i < 10000; i++)
    {
        adf_modctx_struct *c1 = NULL;
        adf_modctx_struct *c2 = NULL;
        adf_scaled_t x, xa, y;
        adf_rat_t s;
        fmpz_t K1, K2, u, r;
        adf_fball_t before, after;
        int lost_plain = -2, lost_alias = -2, changed;

        fmpz_init(K1);
        fmpz_init(K2);
        fmpz_init(u);
        fmpz_init(r);
        random_fmpz_bits(K1, (ulong) (1 + rng_next() % 4096));
        random_fmpz_bits(K2, (ulong) (1 + rng_next() % 4096));
        ADF_CHECK(adf_modctx_new_fmpz(&c1, K1) == ADF_OK);
        ADF_CHECK(adf_modctx_new_fmpz(&c2, K2) == ADF_OK);
        adf_rat_init(s);
        if (rng_next() % 4 == 0)
        {
            /* an exact input */
            rat_si(s, (slong) (rng_next() % 200) - 100, 3);
            adf_scaled_init(x, c1);
            adf_scaled_init(xa, c1);
            adf_scaled_init(y, c1);
            adf_scaled_set_rat(x, s, c1);
        }
        else
        {
            random_fmpz_bits(r, (ulong) (1 + rng_next() % 4096));
            fmpz_fdiv_r(u, r, K1);
            rat_si(s, (slong) (1 + rng_next() % 50), (ulong) (1 + rng_next() % 50));
            adf_scaled_init(x, c1);
            adf_scaled_init(xa, c1);
            adf_scaled_init(y, c1);
            scaled_from(x, s, u, c1);
        }
        adf_scaled_set(xa, x);
        adf_fball_init(before);
        adf_fball_init(after);
        set_of(before, x);
        ADF_CHECK(adf_scaled_set_context(y, &lost_plain, x, c2) == ADF_OK);
        ADF_CHECK(adf_scaled_set_context(xa, &lost_alias, xa, c2) == ADF_OK);
        set_of(after, y);
        changed = !adf_fball_equal_set(before, after);
        cases++;
        if (lost_plain != changed)
            bad_plain++;
        if (lost_alias != changed || !same_data(y, xa) || lost_plain != lost_alias)
            bad_alias++;
        adf_fball_clear(before);
        adf_fball_clear(after);
        adf_scaled_clear(x);
        adf_scaled_clear(xa);
        adf_scaled_clear(y);
        adf_rat_clear(s);
        fmpz_clear(K1);
        fmpz_clear(K2);
        fmpz_clear(u);
        fmpz_clear(r);
        adf_modctx_free(c1);
        adf_modctx_free(c2);
    }
    ADF_CHECK_MSG(bad_plain == 0, "random plain lost wrong in %ld of %ld", bad_plain, cases);
    ADF_CHECK_MSG(bad_alias == 0, "random aliased lost wrong in %ld of %ld", bad_alias, cases);
}

/* ---- 4. add_rat with y = x ---- */

ADF_TEST(add_rat_alias)
{
    adf_modctx_struct * ctx = NULL;
    adf_scaled_t x, xa, y;
    adf_rat_t s, q;
    fmpz_t K, u;
    long bad = 0, cases = 0;
    const slong qn[6] = {0, 1, -1, 3, 1, 7};
    const ulong qd[6] = {1, 1, 2, 1, 2, 3};
    const ulong us[5] = {0, 1, 2, 3, 7};
    int i, j;

    fmpz_init_set_ui(K, 12);
    fmpz_init(u);
    ADF_CHECK(adf_modctx_new_fmpz(&ctx, K) == ADF_OK);
    adf_rat_init(s);
    adf_rat_init(q);
    adf_scaled_init(y, ctx);
    for (i = 0; i < 5; i++)
        for (j = 0; j < 6; j++)
        {
            int lost_plain = -2, lost_alias = -2;

            rat_si(s, 5, 2);
            fmpz_set_ui(u, us[i]);
            adf_scaled_init(x, ctx);
            adf_scaled_init(xa, ctx);
            scaled_from(x, s, u, ctx);
            adf_scaled_set(xa, x);
            rat_si(q, qn[j], qd[j]);
            adf_scaled_add_rat(y, &lost_plain, x, q);
            adf_scaled_add_rat(xa, &lost_alias, xa, q);
            cases++;
            if (!same_data(y, xa) || lost_plain != lost_alias)
                bad++;
            adf_scaled_clear(x);
            adf_scaled_clear(xa);
        }
    /* an exact x */
    {
        int lost_plain = -2, lost_alias = -2;
        adf_scaled_init(x, ctx);
        adf_scaled_init(xa, ctx);
        adf_scaled_set_rat(x, s, ctx);
        adf_scaled_set(xa, x);
        rat_si(q, 1, 2);
        adf_scaled_add_rat(y, &lost_plain, x, q);
        adf_scaled_add_rat(xa, &lost_alias, xa, q);
        cases++;
        if (!same_data(y, xa) || lost_plain != lost_alias || lost_alias != 0)
            bad++;
        adf_scaled_clear(x);
        adf_scaled_clear(xa);
    }
    ADF_CHECK_MSG(bad == 0, "add_rat y = x differs in %ld of %ld", bad, cases);
    adf_scaled_clear(y);
    adf_rat_clear(s);
    adf_rat_clear(q);
    fmpz_clear(K);
    fmpz_clear(u);
    adf_modctx_free(ctx);
}

/* ---- 5. the four binary operations, z = x, z = y, z = x = y; OK and DOMAIN ---- */

static void
check_binary_alias(adf_modctx_struct * c1, adf_scaled_t x, adf_scaled_t y,
                   int (*op)(adf_scaled_t, const adf_scaled_t, const adf_scaled_t),
                   long * bad, long * cases)
{
    adf_scaled_t z, xx, yy;
    int st_plain;

    adf_scaled_init(z, c1);
    adf_scaled_init(xx, c1);
    adf_scaled_init(yy, c1);
    adf_scaled_set(xx, x);
    adf_scaled_set(yy, y);

    /* the plain call */
    st_plain = op(z, xx, yy);
    (*cases)++;

    /* z = x */
    {
        adf_scaled_t a;
        adf_scaled_init(a, c1);
        adf_scaled_set(a, x);
        if (op(a, a, y) != st_plain || (st_plain == ADF_OK && !same_data(a, z)))
            (*bad)++;
        adf_scaled_clear(a);
    }
    /* z = y */
    {
        adf_scaled_t b;
        adf_scaled_init(b, c1);
        adf_scaled_set(b, y);
        if (op(b, x, b) != st_plain || (st_plain == ADF_OK && !same_data(b, z)))
            (*bad)++;
        adf_scaled_clear(b);
    }
    /* z = x = y: both inputs equal x */
    {
        adf_scaled_t a;
        adf_scaled_t q, zz;
        int st_both;
        adf_scaled_init(a, c1);
        adf_scaled_init(q, c1);
        adf_scaled_init(zz, c1);
        adf_scaled_set(a, x);
        adf_scaled_set(q, x);
        st_both = op(zz, q, q);
        if (op(a, a, a) != st_both || (st_both == ADF_OK && !same_data(a, zz)))
            (*bad)++;
        adf_scaled_clear(a);
        adf_scaled_clear(q);
        adf_scaled_clear(zz);
    }
    adf_scaled_clear(z);
    adf_scaled_clear(xx);
    adf_scaled_clear(yy);
}

ADF_TEST(binary_ops_alias)
{
    adf_modctx_struct * ctx = NULL;
    adf_scaled_t x, y;
    adf_rat_t s, t;
    fmpz_t K, u, v;
    long bad = 0, cases = 0;
    int i;

    fmpz_init_set_ui(K, 30);
    ADF_CHECK(adf_modctx_new_fmpz(&ctx, K) == ADF_OK);
    adf_rat_init(s);
    adf_rat_init(t);
    fmpz_init(u);
    fmpz_init(v);
    adf_scaled_init(x, ctx);
    adf_scaled_init(y, ctx);
    for (i = 0; i < 20; i++)
    {
        rat_si(s, (slong) (1 + rng_next() % 20), (ulong) (1 + rng_next() % 20));
        rat_si(t, (slong) (1 + rng_next() % 20), (ulong) (1 + rng_next() % 20));
        fmpz_set_ui(u, rng_next() % 30);
        fmpz_set_ui(v, rng_next() % 30);
        adf_scaled_clear(x);
        adf_scaled_clear(y);
        adf_scaled_init(x, ctx);
        adf_scaled_init(y, ctx);
        scaled_from(x, s, u, ctx);
        scaled_from(y, t, v, ctx);
        check_binary_alias(ctx, x, y, adf_scaled_add, &bad, &cases);
        check_binary_alias(ctx, x, y, adf_scaled_sub, &bad, &cases);
        check_binary_alias(ctx, x, y, adf_scaled_mul, &bad, &cases);
        check_binary_alias(ctx, x, y, adf_scaled_mul_tight, &bad, &cases);
    }
    ADF_CHECK_MSG(bad == 0, "binary aliasing differs in %ld of %ld", bad, cases);
    adf_scaled_clear(x);
    adf_scaled_clear(y);
    adf_rat_clear(s);
    adf_rat_clear(t);
    fmpz_clear(K);
    fmpz_clear(u);
    fmpz_clear(v);
    adf_modctx_free(ctx);
}

/* ---- 6. DOMAIN with an aliased output: the output is untouched ---- */

ADF_TEST(binary_domain_alias_untouched)
{
    adf_modctx_struct * c1 = NULL;
    adf_modctx_struct * c2 = NULL;
    adf_scaled_t x, y, xa, yb;
    adf_rat_t s;
    fmpz_t K, u;

    fmpz_init_set_ui(K, 12);
    ADF_CHECK(adf_modctx_new_fmpz(&c1, K) == ADF_OK);
    fmpz_set_ui(K, 5);
    ADF_CHECK(adf_modctx_new_fmpz(&c2, K) == ADF_OK);
    adf_rat_init(s);
    rat_si(s, 1, 1);
    fmpz_init(u);
    fmpz_set_ui(u, 2);
    adf_scaled_init(x, c1);
    adf_scaled_init(xa, c1);
    adf_scaled_init(y, c2);
    adf_scaled_init(yb, c2);
    scaled_from(x, s, u, c1);
    fmpz_set_ui(u, 3);
    scaled_from(y, s, u, c2);
    adf_scaled_set(xa, x);
    adf_scaled_set(yb, y);

    ADF_CHECK(adf_scaled_add(xa, xa, y) == ADF_DOMAIN);
    ADF_CHECK(same_data(xa, x));
    ADF_CHECK(adf_scaled_mul(yb, x, yb) == ADF_DOMAIN);
    ADF_CHECK(same_data(yb, y));

    adf_scaled_clear(x);
    adf_scaled_clear(xa);
    adf_scaled_clear(y);
    adf_scaled_clear(yb);
    adf_rat_clear(s);
    fmpz_clear(K);
    fmpz_clear(u);
    adf_modctx_free(c1);
    adf_modctx_free(c2);
}

/* ---- 7. neg and mul_rat with y = x ---- */

ADF_TEST(neg_mul_rat_alias)
{
    adf_modctx_struct * ctx = NULL;
    adf_scaled_t x, xa, z;
    adf_rat_t q;
    fmpz_t K, u;
    long bad = 0, cases = 0;
    int i;

    fmpz_init_set_ui(K, 36);
    ADF_CHECK(adf_modctx_new_fmpz(&ctx, K) == ADF_OK);
    adf_rat_init(q);
    fmpz_init(u);
    adf_scaled_init(z, ctx);
    for (i = 0; i < 10; i++)
    {
        adf_rat_t s;
        adf_rat_init(s);
        rat_si(s, (slong) (1 + rng_next() % 20), (ulong) (1 + rng_next() % 20));
        fmpz_set_ui(u, rng_next() % 36);
        adf_scaled_init(x, ctx);
        adf_scaled_init(xa, ctx);
        scaled_from(x, s, u, ctx);
        adf_scaled_set(xa, x);

        adf_scaled_neg(z, x);
        adf_scaled_neg(xa, xa);
        cases++;
        if (!same_data(z, xa))
            bad++;

        adf_scaled_set(xa, x);
        rat_si(q, (slong) (rng_next() % 21) - 10, (ulong) (1 + rng_next() % 9));
        adf_scaled_mul_rat(z, x, q);
        adf_scaled_mul_rat(xa, xa, q);
        cases++;
        if (!same_data(z, xa))
            bad++;
        adf_scaled_clear(x);
        adf_scaled_clear(xa);
        adf_rat_clear(s);
    }
    ADF_CHECK_MSG(bad == 0, "neg/mul_rat y = x differs in %ld of %ld", bad, cases);
    adf_scaled_clear(z);
    adf_rat_clear(q);
    fmpz_clear(K);
    fmpz_clear(u);
    adf_modctx_free(ctx);
}

/* ---- 8. set_fball writes its output whatever it held before; value and *lost ---- */

ADF_TEST(set_fball_overwrite_and_lost)
{
    adf_modctx_struct * ctx = NULL;
    adf_scaled_t a, b;
    adf_rat_t c, r, junk;
    adf_fball_t f;
    fmpz_t K;
    int lost_a = -2, lost_b = -2;

    fmpz_init_set_ui(K, 12);
    ADF_CHECK(adf_modctx_new_fmpz(&ctx, K) == ADF_OK);
    adf_rat_init(c);
    adf_rat_init(r);
    adf_rat_init(junk);
    adf_fball_init(f);
    adf_scaled_init(a, ctx);
    adf_scaled_init(b, ctx);
    rat_si(c, 5, 4);
    rat_si(r, 9, 2);
    ADF_CHECK(adf_fball_set_center_radius(f, c, r) == ADF_OK);
    rat_si(junk, -7, 5);
    adf_scaled_set_rat(b, junk, ctx); /* b holds something else */
    ADF_CHECK(adf_scaled_set_fball(a, &lost_a, f, ctx) == ADF_OK);
    ADF_CHECK(adf_scaled_set_fball(b, &lost_b, f, ctx) == ADF_OK);
    ADF_CHECK(same_data(a, b));
    ADF_CHECK(lost_a == lost_b);
    adf_scaled_clear(a);
    adf_scaled_clear(b);
    adf_rat_clear(c);
    adf_rat_clear(r);
    adf_rat_clear(junk);
    adf_fball_clear(f);
    fmpz_clear(K);
    adf_modctx_free(ctx);
}

/* ---- 9. the cap functions, every permitted aliasing (scaled.h, the cap block) ---- */

static void
ball_from(adf_fball_t f, const adf_rat_t c, const adf_rat_t r)
{
    ADF_CHECK(adf_fball_set_center_radius(f, c, r) == ADF_OK);
}

ADF_TEST(cap_alias)
{
    adf_fball_t x, y, xa, ya, z;
    adf_rat_t c, r, d, q, C;
    long cases = 0;
    int i;

    adf_rat_init(c);
    adf_rat_init(r);
    adf_rat_init(d);
    adf_rat_init(q);
    adf_rat_init(C);
    adf_fball_init(x);
    adf_fball_init(y);
    adf_fball_init(xa);
    adf_fball_init(ya);
    adf_fball_init(z);
    rat_si(C, 3, 2);
    for (i = 0; i < 12; i++)
    {
        int st;
        rat_si(c, (slong) (rng_next() % 20) - 10, (ulong) (1 + rng_next() % 9));
        rat_si(r, (slong) (1 + rng_next() % 20), (ulong) (1 + rng_next() % 9));
        rat_si(d, (slong) (1 + rng_next() % 20), (ulong) (1 + rng_next() % 9));
        ball_from(x, c, r);
        ball_from(y, c, d);
        adf_fball_set(xa, x);
        adf_fball_set(ya, y);

        /* adf_fball_cap: y = x */
        st = adf_fball_cap(z, x, C);
        ADF_CHECK(st == ADF_OK);
        ADF_CHECK(adf_fball_cap(xa, xa, C) == st);
        ADF_CHECK(adf_fball_equal_set(xa, z));
        cases++;

        /* add_cap: z = x, z = y, z = x = y */
        st = adf_fball_add_cap(z, x, y, C);
        adf_fball_set(xa, x);
        ADF_CHECK(adf_fball_add_cap(xa, xa, y, C) == st);
        ADF_CHECK(adf_fball_equal_set(xa, z));
        adf_fball_set(ya, y);
        ADF_CHECK(adf_fball_add_cap(ya, x, ya, C) == st);
        ADF_CHECK(adf_fball_equal_set(ya, z));
        adf_fball_set(xa, x);
        ADF_CHECK(adf_fball_add_cap(xa, xa, xa, C) == adf_fball_add_cap(z, x, x, C));
        cases++;

        /* sub_cap: z = x, z = y */
        st = adf_fball_sub_cap(z, x, y, C);
        adf_fball_set(xa, x);
        ADF_CHECK(adf_fball_sub_cap(xa, xa, y, C) == st);
        ADF_CHECK(adf_fball_equal_set(xa, z));
        adf_fball_set(ya, y);
        ADF_CHECK(adf_fball_sub_cap(ya, x, ya, C) == st);
        ADF_CHECK(adf_fball_equal_set(ya, z));
        cases++;

        /* mul_cap: z = x, z = y, z = x = y */
        st = adf_fball_mul_cap(z, x, y, C);
        adf_fball_set(xa, x);
        ADF_CHECK(adf_fball_mul_cap(xa, xa, y, C) == st);
        ADF_CHECK(adf_fball_equal_set(xa, z));
        adf_fball_set(ya, y);
        ADF_CHECK(adf_fball_mul_cap(ya, x, ya, C) == st);
        ADF_CHECK(adf_fball_equal_set(ya, z));
        adf_fball_set(xa, x);
        ADF_CHECK(adf_fball_mul_cap(xa, xa, xa, C) == adf_fball_mul_cap(z, x, x, C));
        cases++;

        /* mul_rat_cap: y = x */
        rat_si(q, (slong) (rng_next() % 10) - 5, (ulong) (1 + rng_next() % 5));
        st = adf_fball_mul_rat_cap(z, x, q, C);
        adf_fball_set(xa, x);
        ADF_CHECK(adf_fball_mul_rat_cap(xa, xa, q, C) == st);
        ADF_CHECK(adf_fball_equal_set(xa, z));
        cases++;
    }
    ADF_CHECK_MSG(cases == 60, "cap cases %ld", cases);
    adf_rat_clear(c);
    adf_rat_clear(r);
    adf_rat_clear(d);
    adf_rat_clear(q);
    adf_rat_clear(C);
    adf_fball_clear(x);
    adf_fball_clear(y);
    adf_fball_clear(xa);
    adf_fball_clear(ya);
    adf_fball_clear(z);
}
