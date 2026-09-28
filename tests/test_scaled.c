/* tests/test_scaled.c: the scaled-residue policy (work package 1.7), unit tests.

   What is checked here beyond the vector rows of tests/test_scaled_vectors.c (lanes/COMMON-C.md
   rule 2): the shared-context rule of conventions 4.6 and SPEC 4.4 ("Context compatibility for
   scaled values") in all its cases; the loss factor h = gcd(u, v, K) of the default product
   (docs/proofs/policies.md Proposition 10, line 193) against the tight adf_fball_mul of the
   converted operands; the exact tag kept by every operation (policies Definition 4, line 107);
   lost reported exactly when the set changes (conventions 5.4, closure C5); the lossless meeting
   in lcm(K, K') (policies Corollary 12, line 241); canonical data equal for equal sets
   (policies Lemma 5, line 113); moduli of one word and of 4096 bits (context without blocks,
   conventions 5.14); aliasing of every permitted argument combination (conventions 4.1); exact
   zero and zero residues; and random expressions of 30 operations evaluated in the tight and in
   the scaled policy with containment after every step (policies Theorem 3, line 75).

   Values are built through the public API: a scaled value s (u + K Zhat) is the ball s u + s K
   Zhat put through adf_scaled_set_fball, which stores exactly (s, u mod K) (Lemma 5). */

#include <stdio.h>
#include <string.h>

#include <flint/ulong_extras.h>

#include <adelefeld/scaled.h>

#include "test_runner.h"

/* ---- helpers ---- */

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

/* 1 if x is the scaled value s (u + K Zhat) with these data (data, not just the set). */
static int
scaled_has_data(const adf_scaled_t x, const adf_rat_t s, const fmpz_t u)
{
    return !adf_scaled_is_exact(x) && fmpq_equal(x->s, s->q) && fmpz_equal(x->u, u);
}

/* 1 if x is the exact rational q. */
static int
scaled_has_exact(const adf_scaled_t x, const adf_rat_t q)
{
    return adf_scaled_is_exact(x) && fmpq_equal(x->s, q->q) && fmpz_is_zero(x->u);
}

/* The sets of two scaled values, and their containment (first inside second). */
static int
sets_equal(adf_scaled_t x, adf_scaled_t y)
{
    adf_fball_t a, b;
    int r;

    adf_fball_init(a);
    adf_fball_init(b);
    adf_scaled_get_fball(a, x);
    adf_scaled_get_fball(b, y);
    r = adf_fball_equal_set(a, b);
    adf_fball_clear(a);
    adf_fball_clear(b);
    return r;
}

static int
set_contains(adf_scaled_t x, adf_scaled_t y)
{
    adf_fball_t a, b;
    int r;

    adf_fball_init(a);
    adf_fball_init(b);
    adf_scaled_get_fball(a, x);
    adf_scaled_get_fball(b, y);
    r = adf_fball_contains(a, b);
    adf_fball_clear(a);
    adf_fball_clear(b);
    return r;
}

/* 1 if the set of x equals the set of the ball b. */
static int
set_equals_ball(adf_scaled_t x, const adf_fball_t b)
{
    adf_fball_t a;
    int r;

    adf_fball_init(a);
    adf_scaled_get_fball(a, x);
    r = adf_fball_equal_set(a, b);
    adf_fball_clear(a);
    return r;
}

/* ---- 1. life cycle and predicate (conventions 2.3, 5.4) ---- */

ADF_TEST(life_cycle_and_predicate)
{
    adf_modctx_struct * ctx = NULL;
    adf_modctx_struct * ctx2 = NULL;
    adf_scaled_t x, y, z, e;
    adf_rat_t s, q;
    fmpz_t K, u;

    fmpz_init_set_si(K, 12);
    ADF_CHECK(adf_modctx_new_fmpz(&ctx, K) == ADF_OK);
    ADF_CHECK(adf_modctx_new_fmpz(&ctx2, K) == ADF_OK);
    adf_scaled_init(x, ctx);
    adf_scaled_init(y, ctx);
    adf_scaled_init(z, ctx2);
    adf_scaled_init(e, ctx);
    adf_rat_init(s);
    adf_rat_init(q);
    fmpz_init(u);

    /* init: the exact 0 in ctx */
    ADF_CHECK(adf_scaled_is_canonical(x));
    ADF_CHECK(adf_scaled_is_exact(x));
    ADF_CHECK(fmpq_is_zero(x->s) && fmpz_is_zero(x->u));
    ADF_CHECK(adf_scaled_context(x) == ctx);

    /* set copies the data and the context pointer; y may be x */
    rat_si(s, 5, 2);
    fmpz_set_si(u, 7);
    scaled_from(x, s, u, ctx);
    adf_scaled_set(y, x);
    ADF_CHECK(adf_scaled_identical(y, x));
    ADF_CHECK(adf_scaled_context(y) == ctx);
    adf_scaled_set(y, y);
    ADF_CHECK(adf_scaled_identical(y, x));

    /* identical: pointer, tag and data */
    scaled_from(z, s, u, ctx2);
    ADF_CHECK(!adf_scaled_identical(z, x)); /* same set and data, other context pointer */
    rat_si(q, 0, 1);
    adf_scaled_set_rat(e, q, ctx);          /* the exact 0 */
    ADF_CHECK(!adf_scaled_identical(y, e)); /* scaled versus exact */

    /* swap exchanges contents and context pointers; x may be x */
    adf_scaled_swap(x, x);
    ADF_CHECK(adf_scaled_identical(x, y));
    rat_si(s, 3, 1);
    scaled_from(z, s, u, ctx2);
    adf_scaled_swap(x, z);
    ADF_CHECK_MSG(scaled_has_data(x, s, u) && adf_scaled_context(x) == ctx2,
                  "swap moves the value and its context");
    ADF_CHECK(adf_scaled_identical(z, y) && adf_scaled_context(z) == ctx);
    adf_scaled_swap(x, z);
    rat_si(s, 5, 2);
    ADF_CHECK(scaled_has_data(x, s, u) && adf_scaled_context(x) == ctx);

    /* the predicate, on values crafted field by field: it returns 0, never aborts */
    ADF_CHECK(adf_scaled_is_canonical(x));
    x->exact = 2;
    ADF_CHECK(!adf_scaled_is_canonical(x));
    x->exact = 1;
    ADF_CHECK(!adf_scaled_is_canonical(x)); /* exact = 1 needs u = 0 */
    x->exact = 0;
    ADF_CHECK(adf_scaled_is_canonical(x));
    fmpz_set_si(x->u, 12);                  /* u = K: out of range */
    ADF_CHECK(!adf_scaled_is_canonical(x));
    fmpz_set_si(x->u, -1);                  /* u < 0: out of range */
    ADF_CHECK(!adf_scaled_is_canonical(x));
    fmpz_set_si(x->u, 7);
    fmpq_zero(x->s);                        /* inexact needs s > 0 */
    ADF_CHECK(!adf_scaled_is_canonical(x));
    fmpq_set_si(x->s, -1, 2);               /* inexact needs s > 0 */
    ADF_CHECK(!adf_scaled_is_canonical(x));
    fmpq_set(x->s, s->q);
    ADF_CHECK(adf_scaled_is_canonical(x));
    x->mctx = NULL;                         /* a context is required */
    ADF_CHECK(!adf_scaled_is_canonical(x));
    x->mctx = ctx;
    fmpz_set_si(fmpq_numref(x->s), 2);          /* raw 2/4: not canonical data */
    fmpz_set_si(fmpq_denref(x->s), 4);
    ADF_CHECK(!adf_scaled_is_canonical(x));
    fmpq_set(x->s, s->q);
    ADF_CHECK(adf_scaled_is_canonical(x));

    adf_scaled_clear(x);
    adf_scaled_clear(y);
    adf_scaled_clear(z);
    adf_scaled_clear(e);
    adf_rat_clear(s);
    adf_rat_clear(q);
    fmpz_clear(u);
    fmpz_clear(K);
    adf_modctx_free(ctx);
    adf_modctx_free(ctx2);
}

/* ---- 2. set_rat: the exact tag (policies Definition 4) ---- */

ADF_TEST(set_rat_stores_the_exact_rational)
{
    adf_modctx_struct * ctx = NULL;
    adf_scaled_t x;
    adf_rat_t q;
    fmpz_t K;

    fmpz_init_set_si(K, 6);
    ADF_CHECK(adf_modctx_new_fmpz(&ctx, K) == ADF_OK);
    adf_scaled_init(x, ctx);
    adf_rat_init(q);

    ADF_CHECK(scaled_has_exact(x, q)); /* init: the exact 0 */
    rat_si(q, -7, 2);
    adf_scaled_set_rat(x, q, ctx);
    ADF_CHECK(scaled_has_exact(x, q));
    ADF_CHECK(adf_scaled_is_canonical(x));
    ADF_CHECK(adf_scaled_context(x) == ctx);
    fmpq_set_si(q->q, 123456789, 1);
    adf_scaled_set_rat(x, q, ctx);
    ADF_CHECK(scaled_has_exact(x, q));
    rat_si(q, 0, 1);
    adf_scaled_set_rat(x, q, ctx);
    ADF_CHECK(scaled_has_exact(x, q));

    adf_scaled_clear(x);
    adf_rat_clear(q);
    fmpz_clear(K);
    adf_modctx_free(ctx);
}

/* ---- 3. set_fball: loss exactly when the set changes, and the best enclosure
        (policies Proposition 7, line 139; Lemma 6, line 124; closure C5) ---- */

ADF_TEST(set_fball_loss_exactly_when_the_set_changes)
{
    adf_modctx_struct * ctx = NULL;
    adf_scaled_t x, cand;
    adf_fball_t b;
    adf_rat_t c, r, s;
    fmpz_t K, u;
    int iK, ic, iR, iu, in, im;
    int lost;

    for (iK = 1; iK <= 4; iK++)
    {
        fmpz_init_set_si(K, iK);
        ADF_CHECK(adf_modctx_new_fmpz(&ctx, K) == ADF_OK);
        for (ic = -3; ic <= 3; ic++)
        {
            for (iR = 0; iR <= 6; iR++)
            {
                adf_rat_init(c);
                adf_rat_init(r);
                adf_rat_init(s);
                fmpz_init(u);
                adf_scaled_init(x, ctx);
                adf_scaled_init(cand, ctx);
                adf_fball_init(b);
                rat_si(c, ic, (iR % 2) ? 2 : 1);   /* centres ic or ic/2 */
                rat_si(r, iR, (ic % 2) ? 2 : 1);   /* radii iR or iR/2 */
                ADF_CHECK(adf_fball_set_center_radius(b, c, r) == ADF_OK);
                lost = -1;
                ADF_CHECK(adf_scaled_set_fball(x, &lost, b, ctx) == ADF_OK);
                ADF_CHECK(adf_scaled_is_canonical(x));
                /* lost is 1 exactly when the set changes */
                ADF_CHECK_MSG(lost == (set_equals_ball(x, b) ? 0 : 1),
                              "K = %d, c = %d, R = %d: lost = %d", iK, ic, iR, lost);
                /* the result contains the ball */
                {
                    adf_fball_t fx;
                    adf_fball_init(fx);
                    adf_scaled_get_fball(fx, x);
                    ADF_CHECK(adf_fball_contains(b, fx));
                    adf_fball_clear(fx);
                }
                /* R = 0: the exact value c, lost = 0 (closure C5) */
                if (iR == 0)
                {
                    ADF_CHECK(lost == 0);
                    ADF_CHECK(scaled_has_exact(x, c));
                }
                /* best enclosure: every scaled value of this context containing b contains x */
                for (im = 1; im <= 6; im++)
                {
                    for (in = 1; in <= 6; in++)
                    {
                        fmpq_set_si(s->q, in, im);
                        if (!fmpq_is_canonical(s->q))
                            continue;
                        for (iu = 0; iu < iK; iu++)
                        {
                            adf_fball_t fx, cb;
                            fmpz_set_si(u, iu);
                            scaled_from(cand, s, u, ctx);
                            adf_fball_init(fx);
                            adf_fball_init(cb);
                            adf_scaled_get_fball(fx, x);
                            adf_scaled_get_fball(cb, cand);
                            if (adf_fball_contains(b, cb))
                                ADF_CHECK_MSG(adf_fball_contains(fx, cb),
                                              "best: K = %d, c = %d, R = %d, s = %d/%d, u = %d",
                                              iK, ic, iR, in, im, iu);
                            adf_fball_clear(fx);
                            adf_fball_clear(cb);
                        }
                    }
                }
                adf_fball_clear(b);
                adf_scaled_clear(x);
                adf_scaled_clear(cand);
                adf_rat_clear(c);
                adf_rat_clear(r);
                adf_rat_clear(s);
                fmpz_clear(u);
            }
        }
        fmpz_clear(K);
        adf_modctx_free(ctx);
        ctx = NULL;
    }
}

/* ---- 4. set_context: the C5 examples, loss exactly when the set changes, K | K' is
        lossless, and the best enclosure (policies Proposition 11, line 218) ---- */

ADF_TEST(set_context_examples)
{
    adf_modctx_struct * ctx2 = NULL;
    adf_modctx_struct * ctx4 = NULL;
    adf_scaled_t x, y;
    adf_rat_t s, s2;
    fmpz_t K2, K4, u, u2;
    int lost;

    fmpz_init_set_si(K2, 2);
    fmpz_init_set_si(K4, 4);
    ADF_CHECK(adf_modctx_new_fmpz(&ctx2, K2) == ADF_OK);
    ADF_CHECK(adf_modctx_new_fmpz(&ctx4, K4) == ADF_OK);
    adf_scaled_init(x, ctx4);
    adf_scaled_init(y, ctx2);
    adf_rat_init(s);
    adf_rat_init(s2);
    fmpz_init(u);
    fmpz_init(u2);

    /* 1 + 4 Zhat to context 2 gives 1 + 2 Zhat, the set changes (policies line 235) */
    rat_si(s, 1, 1);
    fmpz_set_si(u, 1);
    scaled_from(x, s, u, ctx4);
    lost = -1;
    ADF_CHECK(adf_scaled_set_context(y, &lost, x, ctx2) == ADF_OK);
    ADF_CHECK(lost == 1);
    fmpz_set_si(u2, 1);
    ADF_CHECK_MSG(scaled_has_data(y, s, u2), "1 + 4 Zhat to context 2 is 1 + 2 Zhat");
    ADF_CHECK(adf_scaled_context(y) == ctx2);

    /* 1 + 2 Zhat to context 4 gives (1/2)(2 + 4 Zhat), the same set (policies line 235) */
    scaled_from(x, s, u, ctx2); /* x = 1 + 2 Zhat */
    lost = -1;
    ADF_CHECK(adf_scaled_set_context(y, &lost, x, ctx4) == ADF_OK);
    ADF_CHECK_MSG(lost == 0, "1 + 2 Zhat to context 4 changes no set");
    rat_si(s2, 1, 2);
    fmpz_set_si(u2, 2);
    ADF_CHECK_MSG(scaled_has_data(y, s2, u2), "1 + 2 Zhat to context 4 is (1/2)(2 + 4 Zhat)");
    ADF_CHECK(sets_equal(x, y));

    /* an exact value keeps its rational, exact = 1 and u = 0, with lost = 0 (closure C5) */
    rat_si(s2, -3, 4);
    adf_scaled_set_rat(x, s2, ctx2);
    lost = -1;
    ADF_CHECK(adf_scaled_set_context(y, &lost, x, ctx4) == ADF_OK);
    ADF_CHECK(lost == 0);
    ADF_CHECK(scaled_has_exact(y, s2));
    ADF_CHECK(adf_scaled_context(y) == ctx4);

    /* y may alias x */
    rat_si(s, 5, 3);
    fmpz_set_si(u, 1);
    scaled_from(x, s, u, ctx2);
    ADF_CHECK(adf_scaled_set_context(x, NULL, x, ctx2) == ADF_OK);
    ADF_CHECK(scaled_has_data(x, s, u));

    adf_scaled_clear(x);
    adf_scaled_clear(y);
    adf_rat_clear(s);
    adf_rat_clear(s2);
    fmpz_clear(u);
    fmpz_clear(u2);
    fmpz_clear(K2);
    fmpz_clear(K4);
    adf_modctx_free(ctx2);
    adf_modctx_free(ctx4);
}

ADF_TEST(set_context_loss_grid_and_best)
{
    adf_modctx_struct * ctxa = NULL;
    adf_modctx_struct * ctxb = NULL;
    adf_scaled_t x, y, cand;
    adf_rat_t s;
    fmpz_t Ka, Kb, u;
    int iK, iK2, iu, in, im, jn;
    int lost;

    for (iK = 1; iK <= 6; iK++)
    {
        fmpz_init_set_si(Ka, iK);
        ADF_CHECK(adf_modctx_new_fmpz(&ctxa, Ka) == ADF_OK);
        for (iK2 = 1; iK2 <= 6; iK2++)
        {
            fmpz_init_set_si(Kb, iK2);
            ADF_CHECK(adf_modctx_new_fmpz(&ctxb, Kb) == ADF_OK);
            for (iu = 0; iu < iK; iu++)
            {
                adf_scaled_init(x, ctxa);
                adf_scaled_init(y, ctxb);
                adf_scaled_init(cand, ctxb);
                adf_rat_init(s);
                fmpz_init(u);
                rat_si(s, 3, 2);
                fmpz_set_si(u, iu);
                scaled_from(x, s, u, ctxa);
                lost = -1;
                ADF_CHECK(adf_scaled_set_context(y, &lost, x, ctxb) == ADF_OK);
                ADF_CHECK(adf_scaled_is_canonical(y));
                /* lost is 1 exactly when the set changes */
                ADF_CHECK_MSG(lost == (sets_equal(x, y) ? 0 : 1),
                              "K = %d -> %d, u = %d: lost = %d", iK, iK2, iu, lost);
                /* K divides K' is lossless (policies Proposition 11(2), line 223) */
                if (iK2 % iK == 0)
                    ADF_CHECK_MSG(lost == 0, "K = %d divides %d must be lossless", iK, iK2);
                /* the result contains the input */
                ADF_CHECK(set_contains(x, y));
                /* best enclosure among the scaled values of context K' */
                for (im = 1; im <= 6; im++)
                {
                    for (jn = 1; jn <= 6; jn++)
                    {
                        fmpq_set_si(s->q, jn, im);
                        if (!fmpq_is_canonical(s->q))
                            continue;
                        for (in = 0; in < iK2; in++)
                        {
                            fmpz_set_si(u, in);
                            scaled_from(cand, s, u, ctxb);
                            if (set_contains(cand, x))
                                ADF_CHECK_MSG(set_contains(cand, y),
                                              "best: %d -> %d, u = %d, s = %d/%d, u' = %d",
                                              iK, iK2, iu, jn, im, in);
                        }
                    }
                }
                adf_scaled_clear(x);
                adf_scaled_clear(y);
                adf_scaled_clear(cand);
                adf_rat_clear(s);
                fmpz_clear(u);
            }
            fmpz_clear(Kb);
            adf_modctx_free(ctxb);
            ctxb = NULL;
        }
        fmpz_clear(Ka);
        adf_modctx_free(ctxa);
        ctxa = NULL;
    }
}

/* ---- 5. the shared-context rule (conventions 4.6, closure E1; SPEC 4.4) ---- */

typedef int (*bin_op)(adf_scaled_t, const adf_scaled_t, const adf_scaled_t);

static void
context_rule_for(const char * name, bin_op op)
{
    adf_modctx_struct * ctxa = NULL;
    adf_modctx_struct * ctxb = NULL; /* same modulus as ctxa, different pointer */
    adf_modctx_struct * ctxc = NULL; /* different modulus */
    adf_scaled_t x, y, z, snap;
    adf_rat_t s;
    fmpz_t K, L, u;

    fmpz_init_set_si(K, 6);
    fmpz_init_set_si(L, 15);
    ADF_CHECK(adf_modctx_new_fmpz(&ctxa, K) == ADF_OK);
    ADF_CHECK(adf_modctx_new_fmpz(&ctxb, K) == ADF_OK);
    ADF_CHECK(adf_modctx_new_fmpz(&ctxc, L) == ADF_OK);
    adf_scaled_init(x, ctxa);
    adf_scaled_init(y, ctxb);
    adf_scaled_init(z, ctxc);
    adf_scaled_init(snap, ctxc);
    adf_rat_init(s);
    fmpz_init(u);

    rat_si(s, 5, 3);
    fmpz_set_si(u, 2);
    scaled_from(x, s, u, ctxa);
    fmpz_set_si(u, 4);
    scaled_from(y, s, u, ctxb);

    /* different context pointers: ADF_DOMAIN, the output untouched */
    rat_si(s, -1, 1);
    adf_scaled_set_rat(z, s, ctxc);
    adf_scaled_set(snap, z);
    ADF_CHECK_MSG(op(z, x, y) == ADF_DOMAIN, "%s: different pointers", name);
    ADF_CHECK_MSG(adf_scaled_identical(z, snap), "%s: output untouched on DOMAIN", name);
    ADF_CHECK(adf_scaled_context(z) == ctxc);

    /* the same modulus at two pointers is still a mismatch */
    ADF_CHECK_MSG(op(z, x, y) == ADF_DOMAIN, "%s: equal moduli, other pointers", name);

    /* exact inputs follow the same rule */
    rat_si(s, 7, 2);
    adf_scaled_set_rat(x, s, ctxa);   /* exact */
    adf_scaled_set_rat(y, s, ctxb);   /* exact */
    ADF_CHECK_MSG(op(z, x, y) == ADF_DOMAIN, "%s: two exact operands", name);
    fmpz_set_si(u, 2);
    scaled_from(y, s, u, ctxb);       /* scaled again */
    adf_scaled_set_rat(x, s, ctxa);   /* exact */
    ADF_CHECK_MSG(op(z, x, y) == ADF_DOMAIN, "%s: one exact operand", name);
    scaled_from(x, s, u, ctxa);

    /* an aliased output with mismatching inputs: the aliased input is untouched */
    adf_scaled_set(snap, x);
    ADF_CHECK_MSG(op(x, x, y) == ADF_DOMAIN, "%s: output aliases the first input", name);
    ADF_CHECK(adf_scaled_identical(x, snap));
    adf_scaled_set(snap, y);
    ADF_CHECK_MSG(op(y, x, y) == ADF_DOMAIN, "%s: output aliases the second input", name);
    ADF_CHECK(adf_scaled_identical(y, snap));

    /* the old context of the output is never compared */
    fmpz_set_si(u, 4);
    scaled_from(y, s, u, ctxa);       /* same pointer as x now */
    ADF_CHECK_MSG(op(z, x, y) == ADF_OK, "%s: shared inputs, output in a third context", name);
    ADF_CHECK_MSG(adf_scaled_context(z) == ctxa, "%s: the result borrows the input context", name);
    ADF_CHECK(adf_scaled_is_canonical(z));

    adf_scaled_clear(x);
    adf_scaled_clear(y);
    adf_scaled_clear(z);
    adf_scaled_clear(snap);
    adf_rat_clear(s);
    fmpz_clear(u);
    fmpz_clear(K);
    fmpz_clear(L);
    adf_modctx_free(ctxa);
    adf_modctx_free(ctxb);
    adf_modctx_free(ctxc);
}

ADF_TEST(context_rule_add)
{
    context_rule_for("add", adf_scaled_add);
}

ADF_TEST(context_rule_sub)
{
    context_rule_for("sub", adf_scaled_sub);
}

ADF_TEST(context_rule_mul)
{
    context_rule_for("mul", adf_scaled_mul);
}

ADF_TEST(context_rule_mul_tight)
{
    context_rule_for("mul_tight", adf_scaled_mul_tight);
}

/* ---- 6. exact values keep the tag through every operation (policies Definition 4) ---- */

ADF_TEST(exact_values_keep_the_tag)
{
    adf_modctx_struct * ctx = NULL;
    adf_scaled_t x, y, z;
    adf_rat_t a, b, want;
    fmpz_t K;

    fmpz_init_set_si(K, 9);
    ADF_CHECK(adf_modctx_new_fmpz(&ctx, K) == ADF_OK);
    adf_scaled_init(x, ctx);
    adf_scaled_init(y, ctx);
    adf_scaled_init(z, ctx);
    adf_rat_init(a);
    adf_rat_init(b);
    adf_rat_init(want);

    rat_si(a, -3, 4);
    rat_si(b, 5, 6);
    adf_scaled_set_rat(x, a, ctx);
    adf_scaled_set_rat(y, b, ctx);

    ADF_CHECK(adf_scaled_add(z, x, y) == ADF_OK);
    adf_rat_add(want, a, b);
    ADF_CHECK_MSG(scaled_has_exact(z, want), "exact + exact stays exact");
    ADF_CHECK(adf_scaled_sub(z, x, y) == ADF_OK);
    adf_rat_sub(want, a, b);
    ADF_CHECK_MSG(scaled_has_exact(z, want), "exact - exact stays exact");
    ADF_CHECK(adf_scaled_mul(z, x, y) == ADF_OK);
    adf_rat_mul(want, a, b);
    ADF_CHECK_MSG(scaled_has_exact(z, want), "exact * exact stays exact");
    ADF_CHECK(adf_scaled_mul_tight(z, x, y) == ADF_OK);
    ADF_CHECK_MSG(scaled_has_exact(z, want), "exact *tight exact stays exact");
    adf_scaled_neg(z, x);
    adf_rat_neg(want, a);
    ADF_CHECK_MSG(scaled_has_exact(z, want), "-exact stays exact");
    adf_scaled_mul_rat(z, x, b);
    adf_rat_mul(want, a, b);
    ADF_CHECK_MSG(scaled_has_exact(z, want), "q * exact stays exact");
    adf_scaled_add_rat(z, NULL, x, b);
    adf_rat_add(want, a, b);
    ADF_CHECK_MSG(scaled_has_exact(z, want), "exact + q stays exact");
    ADF_CHECK(adf_scaled_set_context(z, NULL, x, ctx) == ADF_OK);
    ADF_CHECK_MSG(scaled_has_exact(z, a), "a converted exact value stays exact");

    adf_scaled_clear(x);
    adf_scaled_clear(y);
    adf_scaled_clear(z);
    adf_rat_clear(a);
    adf_rat_clear(b);
    adf_rat_clear(want);
    fmpz_clear(K);
    adf_modctx_free(ctx);
}

/* ---- 7. the literal rows of SPEC 4.3 and the example of policies Proposition 10 ---- */

ADF_TEST(literal_rows_of_the_specification)
{
    adf_modctx_struct * ctx36 = NULL;
    adf_modctx_struct * ctx4 = NULL;
    adf_scaled_t x, y, z;
    adf_rat_t s;
    fmpz_t K36, K4, u;

    fmpz_init_set_si(K36, 36);
    fmpz_init_set_si(K4, 4);
    ADF_CHECK(adf_modctx_new_fmpz(&ctx36, K36) == ADF_OK);
    ADF_CHECK(adf_modctx_new_fmpz(&ctx4, K4) == ADF_OK);
    adf_scaled_init(x, ctx36);
    adf_scaled_init(y, ctx36);
    adf_scaled_init(z, ctx36);
    adf_rat_init(s);
    fmpz_init(u);

    /* (3 mod 12) + (5 mod 18) = 2 mod 6 (SPEC 4.3): as scaled values of context 36,
       (1/3)(9 + 36 Zhat) + (1/2)(10 + 36 Zhat) = 2 + 6 Zhat (tight, precision P5(1)) */
    rat_si(s, 1, 3);
    fmpz_set_si(u, 9);
    scaled_from(x, s, u, ctx36);
    rat_si(s, 1, 2);
    fmpz_set_si(u, 10);
    scaled_from(y, s, u, ctx36);
    ADF_CHECK(adf_scaled_add(z, x, y) == ADF_OK);
    {
        adf_fball_t want, got;
        adf_rat_t c, r;
        adf_fball_init(want);
        adf_fball_init(got);
        adf_rat_init(c);
        adf_rat_init(r);
        rat_si(c, 2, 1);
        rat_si(r, 6, 1);
        ADF_CHECK(adf_fball_set_center_radius(want, c, r) == ADF_OK);
        adf_scaled_get_fball(got, z);
        ADF_CHECK_MSG(adf_fball_equal_set(got, want), "(3 mod 12) + (5 mod 18) = 2 mod 6");
        /* (3 mod 12) * (5 mod 18) = 3 mod 6 (SPEC 4.3); here h = gcd(9, 10, 36) = 1, so the
           default product and the tight product both give the row */
        ADF_CHECK(adf_scaled_mul(z, x, y) == ADF_OK);
        rat_si(c, 3, 1);
        rat_si(r, 6, 1);
        ADF_CHECK(adf_fball_set_center_radius(want, c, r) == ADF_OK);
        adf_scaled_get_fball(got, z);
        ADF_CHECK_MSG(adf_fball_equal_set(got, want), "(3 mod 12) * (5 mod 18) = 3 mod 6");
        ADF_CHECK(adf_scaled_mul_tight(z, x, y) == ADF_OK);
        adf_scaled_get_fball(got, z);
        ADF_CHECK(adf_fball_equal_set(got, want));
        adf_fball_clear(want);
        adf_fball_clear(got);
        adf_rat_clear(c);
        adf_rat_clear(r);
    }

    /* 12 * (5 mod 18) = 60 mod 216 (SPEC 4.3, adf_scaled_mul_rat) */
    rat_si(s, 1, 2);
    fmpz_set_si(u, 10);
    scaled_from(x, s, u, ctx36);
    {
        adf_rat_t q;
        adf_fball_t want, got;
        adf_rat_t c, r;
        adf_rat_init(q);
        adf_fball_init(want);
        adf_fball_init(got);
        adf_rat_init(c);
        adf_rat_init(r);
        fmpq_set_si(q->q, 12, 1);
        adf_scaled_mul_rat(x, x, q); /* x = 60 + 216 Zhat */
        rat_si(c, 60, 1);
        rat_si(r, 216, 1);
        ADF_CHECK(adf_fball_set_center_radius(want, c, r) == ADF_OK);
        adf_scaled_get_fball(got, x);
        ADF_CHECK_MSG(adf_fball_equal_set(got, want), "12 * (5 mod 18) = 60 mod 216");
        adf_rat_clear(q);
        adf_fball_clear(want);
        adf_fball_clear(got);
        adf_rat_clear(c);
        adf_rat_clear(r);
    }

    /* the example of policies Proposition 10 (line 212): K = 4, x = y = 2 + 4 Zhat.
       The default product is 0 + 4 Zhat (it loses h = 2); the tight product is 4 + 8 Zhat. */
    rat_si(s, 1, 1);
    fmpz_set_si(u, 2);
    scaled_from(x, s, u, ctx4);
    scaled_from(y, s, u, ctx4);
    {
        adf_fball_t want, got;
        adf_rat_t c, r;
        adf_fball_init(want);
        adf_fball_init(got);
        adf_rat_init(c);
        adf_rat_init(r);
        ADF_CHECK(adf_scaled_mul(z, x, y) == ADF_OK);
        rat_si(c, 0, 1);
        rat_si(r, 4, 1);
        ADF_CHECK(adf_fball_set_center_radius(want, c, r) == ADF_OK);
        adf_scaled_get_fball(got, z);
        ADF_CHECK_MSG(adf_fball_equal_set(got, want), "(2 mod 4) * (2 mod 4) default = 0 mod 4");
        ADF_CHECK(adf_scaled_mul_tight(z, x, y) == ADF_OK);
        rat_si(c, 4, 1);
        rat_si(r, 8, 1);
        ADF_CHECK(adf_fball_set_center_radius(want, c, r) == ADF_OK);
        adf_scaled_get_fball(got, z);
        ADF_CHECK_MSG(adf_fball_equal_set(got, want), "(2 mod 4) * (2 mod 4) tight = 4 mod 8");
        adf_fball_clear(want);
        adf_fball_clear(got);
        adf_rat_clear(c);
        adf_rat_clear(r);
    }

    adf_scaled_clear(x);
    adf_scaled_clear(y);
    adf_scaled_clear(z);
    adf_rat_clear(s);
    fmpz_clear(u);
    fmpz_clear(K36);
    fmpz_clear(K4);
    adf_modctx_free(ctx36);
    adf_modctx_free(ctx4);
}

/* ---- 8. the default product loses the factor h = gcd(u, v, K); mul_tight is finer and both
        contain the tight product of the converted operands (policies Proposition 10) ---- */

ADF_TEST(product_loses_the_factor_h)
{
    static const int triples[][3] = {
        /* K, u, v: h = gcd(u, v, K) */
        {4, 2, 2}, {4, 0, 0}, {12, 6, 4}, {12, 0, 5}, {6, 3, 2}, {9, 6, 6},
        {8, 4, 6}, {12, 8, 9}, {2, 0, 1}, {6, 4, 3}, {5, 3, 4}, {7, 0, 0}
    };
    adf_modctx_struct * ctx = NULL;
    adf_scaled_t x, y, zd, zt;
    adf_rat_t s, t;
    fmpz_t K, u, v;
    size_t i;

    adf_rat_init(s);
    adf_rat_init(t);
    fmpz_init(K);
    fmpz_init(u);
    fmpz_init(v);
    for (i = 0; i < sizeof(triples) / sizeof(triples[0]); i++)
    {
        ulong h;
        adf_fball_t bx, by, tight, fd, ft;

        fmpz_set_si(K, triples[i][0]);
        ADF_CHECK(adf_modctx_new_fmpz(&ctx, K) == ADF_OK);
        adf_scaled_init(x, ctx);
        adf_scaled_init(y, ctx);
        adf_scaled_init(zd, ctx);
        adf_scaled_init(zt, ctx);
        rat_si(s, 5, 2);
        rat_si(t, 7, 3);
        fmpz_set_si(u, triples[i][1]);
        fmpz_set_si(v, triples[i][2]);
        scaled_from(x, s, u, ctx);
        scaled_from(y, t, v, ctx);
        ADF_CHECK(adf_scaled_mul(zd, x, y) == ADF_OK);
        ADF_CHECK(adf_scaled_mul_tight(zt, x, y) == ADF_OK);

        h = n_gcd((ulong) triples[i][0], (ulong) triples[i][1]);
        h = n_gcd(h, (ulong) triples[i][2]);

        adf_fball_init(bx);
        adf_fball_init(by);
        adf_fball_init(tight);
        adf_fball_init(fd);
        adf_fball_init(ft);
        adf_scaled_get_fball(bx, x);
        adf_scaled_get_fball(by, y);
        adf_fball_mul(tight, bx, by);
        adf_scaled_get_fball(fd, zd);
        adf_scaled_get_fball(ft, zt);
        ADF_CHECK_MSG(adf_fball_contains(tight, fd),
                      "default product contains the tight product, K = %d u = %d v = %d",
                      triples[i][0], triples[i][1], triples[i][2]);
        ADF_CHECK_MSG(adf_fball_contains(tight, ft),
                      "mul_tight contains the tight product, K = %d u = %d v = %d",
                      triples[i][0], triples[i][1], triples[i][2]);
        ADF_CHECK_MSG(adf_fball_equal_set(tight, ft),
                      "mul_tight equals the tight product, K = %d u = %d v = %d",
                      triples[i][0], triples[i][1], triples[i][2]);
        /* mul_tight is finer than the default product, strictly when h > 1 */
        ADF_CHECK(adf_fball_contains(ft, fd));
        if (h > 1)
        {
            ADF_CHECK_MSG(!adf_fball_contains(fd, ft),
                          "loss factor h = %lu: default is strictly coarser, K = %d", h,
                          triples[i][0]);
            ADF_CHECK_MSG(!adf_scaled_identical(zd, zt), "h = %lu: the data differ", h);
        }
        else
        {
            ADF_CHECK_MSG(adf_fball_equal_set(fd, ft), "h = 1: the default product is tight, "
                          "K = %d", triples[i][0]);
            ADF_CHECK_MSG(adf_scaled_identical(zd, zt), "h = 1: the data agree");
        }
        adf_fball_clear(bx);
        adf_fball_clear(by);
        adf_fball_clear(tight);
        adf_fball_clear(fd);
        adf_fball_clear(ft);
        adf_scaled_clear(x);
        adf_scaled_clear(y);
        adf_scaled_clear(zd);
        adf_scaled_clear(zt);
        adf_modctx_free(ctx);
        ctx = NULL;
    }
    adf_rat_clear(s);
    adf_rat_clear(t);
    fmpz_clear(K);
    fmpz_clear(u);
    fmpz_clear(v);
}

/* ---- 9. negation and the exact scalar product are exact as sets (policies P9) ---- */

ADF_TEST(neg_and_mul_rat_are_exact_as_sets)
{
    adf_modctx_struct * ctx = NULL;
    adf_scaled_t x, y;
    adf_rat_t s, q;
    fmpz_t K, u;
    int iu, iq;

    fmpz_init_set_si(K, 6);
    ADF_CHECK(adf_modctx_new_fmpz(&ctx, K) == ADF_OK);
    adf_scaled_init(x, ctx);
    adf_scaled_init(y, ctx);
    adf_rat_init(s);
    adf_rat_init(q);
    fmpz_init(u);

    for (iu = 0; iu < 6; iu++)
    {
        adf_fball_t bx, by, want;

        rat_si(s, 3, 2);
        fmpz_set_si(u, iu);
        scaled_from(x, s, u, ctx);
        adf_scaled_neg(y, x);
        ADF_CHECK(adf_scaled_is_canonical(y));
        adf_fball_init(bx);
        adf_fball_init(by);
        adf_fball_init(want);
        adf_scaled_get_fball(bx, x);
        adf_fball_neg(want, bx);
        adf_scaled_get_fball(by, y);
        ADF_CHECK_MSG(adf_fball_equal_set(want, by), "-x is exact as a set, u = %d", iu);
        adf_fball_clear(bx);
        adf_fball_clear(by);
        adf_fball_clear(want);
        for (iq = -3; iq <= 3; iq++)
        {
            rat_si(q, iq, (iq % 2 == 0) ? 2 : 1);
            adf_scaled_mul_rat(y, x, q);
            ADF_CHECK(adf_scaled_is_canonical(y));
            ADF_CHECK(adf_scaled_context(y) == ctx);
            adf_fball_init(bx);
            adf_fball_init(by);
            adf_fball_init(want);
            adf_scaled_get_fball(bx, x);
            adf_fball_mul_rat(want, bx, q);
            adf_scaled_get_fball(by, y);
            ADF_CHECK_MSG(adf_fball_equal_set(want, by), "q * x is exact as a set, q = %d", iq);
            if (iq == 0)
                ADF_CHECK(adf_scaled_is_exact(y)); /* q = 0 gives the exact 0 */
            adf_fball_clear(bx);
            adf_fball_clear(by);
            adf_fball_clear(want);
        }
    }
    adf_scaled_clear(x);
    adf_scaled_clear(y);
    adf_rat_clear(s);
    adf_rat_clear(q);
    fmpz_clear(u);
    fmpz_clear(K);
    adf_modctx_free(ctx);
}

/* ---- 10. add_rat: the best enclosure of the tight sum; lost exactly when s does not
         divide q (policies Proposition 8, line 159) ---- */

ADF_TEST(add_rat_lost_and_best)
{
    adf_modctx_struct * ctx = NULL;
    adf_scaled_t x, y;
    adf_rat_t s, q, g;
    fmpz_t K, u;
    int is, iq, iu, im, jn, ju;

    fmpz_init_set_si(K, 4);
    ADF_CHECK(adf_modctx_new_fmpz(&ctx, K) == ADF_OK);
    adf_scaled_init(x, ctx);
    adf_scaled_init(y, ctx);
    adf_rat_init(s);
    adf_rat_init(q);
    adf_rat_init(g);
    fmpz_init(u);

    for (is = 1; is <= 4; is++)
    {
        for (iu = 0; iu < 4; iu++)
        {
            rat_si(s, is, 2);
            fmpz_set_si(u, iu);
            scaled_from(x, s, u, ctx);
            for (iq = -3; iq <= 3; iq++)
            {
                int lost = -1, divides;
                adf_fball_t tight, got, bx, bq;

                rat_si(q, iq, (iq % 2 == 0) ? 2 : 1);
                adf_scaled_add_rat(y, &lost, x, q);
                ADF_CHECK(adf_scaled_is_canonical(y));
                ADF_CHECK(adf_scaled_context(y) == ctx);
                /* s divides q is the statement: q/s is an integer */
                fmpq_div(g->q, q->q, s->q);
                divides = fmpz_is_one(fmpq_denref(g->q));
                ADF_CHECK_MSG(lost == (divides ? 0 : 1),
                              "lost = %d, s = %d/2, q row %d: s | q is %d", lost, is, iq,
                              divides);
                /* the result contains the tight sum, and equals it when nothing is lost */
                adf_fball_init(tight);
                adf_fball_init(got);
                adf_fball_init(bx);
                adf_fball_init(bq);
                adf_scaled_get_fball(bx, x);
                adf_fball_set_rat(bq, q);
                adf_fball_add(tight, bx, bq);
                adf_scaled_get_fball(got, y);
                ADF_CHECK(adf_fball_contains(tight, got));
                ADF_CHECK_MSG((lost == 0) == (adf_fball_equal_set(tight, got) != 0),
                              "set change and lost agree, s = %d/2, q row %d", is, iq);
                /* best among the scaled values of this context (scales n/m <= 4) */
                for (im = 1; im <= 4; im++)
                {
                    for (jn = 1; jn <= 4; jn++)
                    {
                        adf_scaled_t cand;
                        adf_scaled_init(cand, ctx);
                        fmpq_set_si(s->q, jn, im);
                        if (!fmpq_is_canonical(s->q))
                        {
                            adf_scaled_clear(cand);
                            continue;
                        }
                        for (ju = 0; ju < 4; ju++)
                        {
                            adf_fball_t cb;
                            fmpz_set_si(u, ju);
                            scaled_from(cand, s, u, ctx);
                            adf_fball_init(cb);
                            adf_scaled_get_fball(cb, cand);
                            if (adf_fball_contains(tight, cb))
                                ADF_CHECK(adf_fball_contains(got, cb));
                            adf_fball_clear(cb);
                        }
                        adf_scaled_clear(cand);
                    }
                }
                rat_si(s, is, 2); /* restore s for the next round */
                adf_fball_clear(tight);
                adf_fball_clear(got);
                adf_fball_clear(bx);
                adf_fball_clear(bq);
            }
        }
    }
    /* q = 0: y = x exactly (data included), lost = 0 */
    {
        int lost = -1;
        rat_si(s, 5, 2);
        fmpz_set_si(u, 3);
        scaled_from(x, s, u, ctx);
        rat_si(q, 0, 1);
        adf_scaled_add_rat(y, &lost, x, q);
        ADF_CHECK(lost == 0);
        ADF_CHECK(adf_scaled_identical(y, x));
    }
    adf_scaled_clear(x);
    adf_scaled_clear(y);
    adf_rat_clear(s);
    adf_rat_clear(q);
    adf_rat_clear(g);
    fmpz_clear(u);
    fmpz_clear(K);
    adf_modctx_free(ctx);
}

/* ---- 11. two contexts meet losslessly in lcm(K, K') (policies Corollary 12, line 241) ---- */

ADF_TEST(lcm_contexts_meet_losslessly)
{
    static const int pairs[][2] = {{2, 3}, {4, 6}, {6, 15}, {8, 12}, {5, 7}, {9, 6}, {12, 1}};
    adf_rat_t s;
    fmpz_t Ka, Kb, Klcm;
    size_t i;
    int trial;

    adf_rat_init(s);
    fmpz_init(Ka);
    fmpz_init(Kb);
    fmpz_init(Klcm);
    for (i = 0; i < sizeof(pairs) / sizeof(pairs[0]); i++)
    {
        for (trial = 0; trial < 3; trial++)
        {
            adf_modctx_struct * ctxa = NULL;
            adf_modctx_struct * ctxb = NULL;
            adf_modctx_struct * ctxl = NULL;
            adf_scaled_t x, y, xl, yl, sum, prod, ptight;
            fmpz_t ua, ub;
            int lost1 = -1, lost2 = -1;

            fmpz_set_si(Ka, pairs[i][0]);
            fmpz_set_si(Kb, pairs[i][1]);
            fmpz_lcm(Klcm, Ka, Kb);
            ADF_CHECK(adf_modctx_new_fmpz(&ctxa, Ka) == ADF_OK);
            ADF_CHECK(adf_modctx_new_fmpz(&ctxb, Kb) == ADF_OK);
            ADF_CHECK(adf_modctx_new_fmpz(&ctxl, Klcm) == ADF_OK);
            adf_scaled_init(x, ctxa);
            adf_scaled_init(y, ctxb);
            adf_scaled_init(xl, ctxl);
            adf_scaled_init(yl, ctxl);
            adf_scaled_init(sum, ctxl);
            adf_scaled_init(prod, ctxl);
            adf_scaled_init(ptight, ctxl);
            fmpz_init(ua);
            fmpz_init(ub);
            fmpz_set_si(ua, (trial + 1) * 2);
            fmpz_set_si(ub, trial + 1);
            rat_si(s, 3, 2);
            scaled_from(x, s, ua, ctxa);
            rat_si(s, 5, 3);
            scaled_from(y, s, ub, ctxb);

            /* both conversions into lcm(K, K') are lossless */
            ADF_CHECK(adf_scaled_set_context(xl, &lost1, x, ctxl) == ADF_OK);
            ADF_CHECK(adf_scaled_set_context(yl, &lost2, y, ctxl) == ADF_OK);
            ADF_CHECK_MSG(lost1 == 0, "K = %d to lcm is lossless", pairs[i][0]);
            ADF_CHECK_MSG(lost2 == 0, "K' = %d to lcm is lossless", pairs[i][1]);
            ADF_CHECK(sets_equal(x, xl) && sets_equal(y, yl));
            /* the ordinary operation then works and is the tight one (Corollary 12) */
            ADF_CHECK(adf_scaled_add(sum, xl, yl) == ADF_OK);
            ADF_CHECK(adf_scaled_mul(prod, xl, yl) == ADF_OK);
            ADF_CHECK(adf_scaled_mul_tight(ptight, xl, yl) == ADF_OK);
            {
                adf_fball_t bx, by, tsum, tprod, fsum, fprod, fpt;
                adf_fball_init(bx);
                adf_fball_init(by);
                adf_fball_init(tsum);
                adf_fball_init(tprod);
                adf_fball_init(fsum);
                adf_fball_init(fprod);
                adf_fball_init(fpt);
                adf_scaled_get_fball(bx, x);
                adf_scaled_get_fball(by, y);
                adf_fball_add(tsum, bx, by);
                adf_fball_mul(tprod, bx, by);
                adf_scaled_get_fball(fsum, sum);
                adf_scaled_get_fball(fprod, prod);
                adf_scaled_get_fball(fpt, ptight);
                ADF_CHECK_MSG(adf_fball_equal_set(tsum, fsum),
                              "the scaled sum is the tight sum, K = %d K' = %d", pairs[i][0],
                              pairs[i][1]);
                ADF_CHECK(adf_fball_contains(tprod, fprod));
                ADF_CHECK_MSG(adf_fball_equal_set(tprod, fpt),
                              "mul_tight equals the tight product, K = %d K' = %d",
                              pairs[i][0], pairs[i][1]);
                adf_fball_clear(bx);
                adf_fball_clear(by);
                adf_fball_clear(tsum);
                adf_fball_clear(tprod);
                adf_fball_clear(fsum);
                adf_fball_clear(fprod);
                adf_fball_clear(fpt);
            }
            adf_scaled_clear(x);
            adf_scaled_clear(y);
            adf_scaled_clear(xl);
            adf_scaled_clear(yl);
            adf_scaled_clear(sum);
            adf_scaled_clear(prod);
            adf_scaled_clear(ptight);
            fmpz_clear(ua);
            fmpz_clear(ub);
            adf_modctx_free(ctxa);
            adf_modctx_free(ctxb);
            adf_modctx_free(ctxl);
        }
    }
    adf_rat_clear(s);
    fmpz_clear(Ka);
    fmpz_clear(Kb);
    fmpz_clear(Klcm);
}

/* ---- 12. canonical data equal for equal sets from different inputs (policies Lemma 5) ---- */

ADF_TEST(equal_sets_equal_data)
{
    adf_modctx_struct * ctx = NULL;
    adf_scaled_t x, y, z;
    adf_rat_t q;
    fmpz_t K, A, H, d;
    adf_fball_t b1, b2;

    fmpz_init_set_si(K, 6);
    ADF_CHECK(adf_modctx_new_fmpz(&ctx, K) == ADF_OK);
    adf_scaled_init(x, ctx);
    adf_scaled_init(y, ctx);
    adf_scaled_init(z, ctx);
    adf_rat_init(q);
    adf_fball_init(b1);
    adf_fball_init(b2);

    /* the same set 3 + 6 Zhat from the triples (3, 6, 1) and (6, 12, 2) */
    fmpz_init_set_si(A, 3);
    fmpz_init_set_si(H, 6);
    fmpz_init_set_si(d, 1);
    ADF_CHECK(adf_fball_set_fmpz3(b1, A, H, d) == ADF_OK);
    fmpz_set_si(A, 6);
    fmpz_set_si(H, 12);
    fmpz_set_si(d, 2);
    ADF_CHECK(adf_fball_set_fmpz3(b2, A, H, d) == ADF_OK);
    ADF_CHECK(adf_scaled_set_fball(x, NULL, b1, ctx) == ADF_OK);
    ADF_CHECK(adf_scaled_set_fball(y, NULL, b2, ctx) == ADF_OK);
    ADF_CHECK_MSG(adf_scaled_identical(x, y), "equal sets give equal (s, u)");

    /* and from a lossless round trip through a larger context */
    {
        adf_modctx_struct * ctx12 = NULL;
        adf_scaled_t tmp;
        fmpz_t K12;

        fmpz_init_set_si(K12, 12);
        ADF_CHECK(adf_modctx_new_fmpz(&ctx12, K12) == ADF_OK);
        adf_scaled_init(tmp, ctx12);
        ADF_CHECK(adf_scaled_set_context(tmp, NULL, x, ctx12) == ADF_OK);
        ADF_CHECK(adf_scaled_set_context(z, NULL, tmp, ctx) == ADF_OK);
        ADF_CHECK_MSG(adf_scaled_identical(x, z),
                      "a lossless round trip returns the canonical data");
        adf_scaled_clear(tmp);
        fmpz_clear(K12);
        adf_modctx_free(ctx12);
    }
    /* and from add_rat with q = 0 */
    rat_si(q, 0, 1);
    adf_scaled_add_rat(z, NULL, x, q);
    ADF_CHECK(adf_scaled_identical(x, z));

    adf_scaled_clear(x);
    adf_scaled_clear(y);
    adf_scaled_clear(z);
    adf_rat_clear(q);
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
    fmpz_clear(K);
    adf_fball_clear(b1);
    adf_fball_clear(b2);
    adf_modctx_free(ctx);
}

/* ---- 13. aliasing of every permitted combination (conventions 4.1) ---- */

static void
aliasing_for(const char * name, bin_op op)
{
    adf_modctx_struct * ctx = NULL;
    adf_scaled_t x, y, z, want;
    adf_rat_t s;
    fmpz_t K, u;

    fmpz_init_set_si(K, 6);
    ADF_CHECK(adf_modctx_new_fmpz(&ctx, K) == ADF_OK);
    adf_scaled_init(x, ctx);
    adf_scaled_init(y, ctx);
    adf_scaled_init(z, ctx);
    adf_scaled_init(want, ctx);
    adf_rat_init(s);
    fmpz_init(u);

    rat_si(s, 5, 3);
    fmpz_set_si(u, 2);
    scaled_from(x, s, u, ctx);
    rat_si(s, 7, 4);
    fmpz_set_si(u, 5);
    scaled_from(y, s, u, ctx);

    ADF_CHECK(op(z, x, y) == ADF_OK);
    adf_scaled_set(want, z);
    ADF_CHECK_MSG(op(x, x, y) == ADF_OK && adf_scaled_identical(x, want),
                  "%s: output = first input", name);
    rat_si(s, 5, 3);
    fmpz_set_si(u, 2);
    scaled_from(x, s, u, ctx);
    ADF_CHECK_MSG(op(y, x, y) == ADF_OK && adf_scaled_identical(y, want),
                  "%s: output = second input", name);
    rat_si(s, 7, 4);
    fmpz_set_si(u, 5);
    scaled_from(y, s, u, ctx);
    ADF_CHECK(op(z, x, x) == ADF_OK);
    adf_scaled_set(want, z);
    ADF_CHECK_MSG(op(x, x, x) == ADF_OK && adf_scaled_identical(x, want),
                  "%s: output = both inputs", name);

    adf_scaled_clear(x);
    adf_scaled_clear(y);
    adf_scaled_clear(z);
    adf_scaled_clear(want);
    adf_rat_clear(s);
    fmpz_clear(u);
    fmpz_clear(K);
    adf_modctx_free(ctx);
}

ADF_TEST(aliasing_binary_ops)
{
    aliasing_for("add", adf_scaled_add);
    aliasing_for("sub", adf_scaled_sub);
    aliasing_for("mul", adf_scaled_mul);
    aliasing_for("mul_tight", adf_scaled_mul_tight);
}

ADF_TEST(aliasing_unary_and_scalar_ops)
{
    adf_modctx_struct * ctx = NULL;
    adf_modctx_struct * ctx2 = NULL;
    adf_scaled_t x, z, snap;
    adf_rat_t s, q;
    fmpz_t K, K2, u;

    fmpz_init_set_si(K, 6);
    fmpz_init_set_si(K2, 4);
    ADF_CHECK(adf_modctx_new_fmpz(&ctx, K) == ADF_OK);
    ADF_CHECK(adf_modctx_new_fmpz(&ctx2, K2) == ADF_OK);
    adf_scaled_init(x, ctx);
    adf_scaled_init(z, ctx);
    adf_scaled_init(snap, ctx);
    adf_rat_init(s);
    adf_rat_init(q);
    fmpz_init(u);

    rat_si(s, 5, 3);
    fmpz_set_si(u, 2);
    scaled_from(x, s, u, ctx);

    /* neg with y = x */
    adf_scaled_neg(z, x);
    adf_scaled_set(snap, z);
    adf_scaled_neg(x, x);
    ADF_CHECK(adf_scaled_identical(x, snap));
    adf_scaled_neg(x, x); /* back */

    /* mul_rat with y = x */
    rat_si(q, -2, 3);
    adf_scaled_mul_rat(z, x, q);
    adf_scaled_set(snap, z);
    adf_scaled_mul_rat(x, x, q);
    ADF_CHECK(adf_scaled_identical(x, snap));
    rat_si(s, 5, 3);
    fmpz_set_si(u, 2);
    scaled_from(x, s, u, ctx);

    /* add_rat with y = x, lossless and lossy */
    rat_si(q, 3, 1);
    adf_scaled_add_rat(z, NULL, x, q);
    adf_scaled_set(snap, z);
    adf_scaled_add_rat(x, NULL, x, q);
    ADF_CHECK(adf_scaled_identical(x, snap));
    rat_si(s, 5, 3);
    fmpz_set_si(u, 2);
    scaled_from(x, s, u, ctx);
    rat_si(q, 1, 2);
    adf_scaled_add_rat(z, NULL, x, q);
    adf_scaled_set(snap, z);
    adf_scaled_add_rat(x, NULL, x, q);
    ADF_CHECK(adf_scaled_identical(x, snap));

    /* set_context with y = x, in place and across contexts */
    ADF_CHECK(adf_scaled_set_context(x, NULL, x, ctx) == ADF_OK);
    adf_scaled_set(snap, x);
    ADF_CHECK(adf_scaled_set_context(x, NULL, x, ctx2) == ADF_OK);
    ADF_CHECK(adf_scaled_context(x) == ctx2);
    ADF_CHECK(set_contains(snap, x));
    ADF_CHECK(adf_scaled_set_context(x, NULL, x, ctx) == ADF_OK);
    ADF_CHECK(set_contains(snap, x));

    /* swap with x = x */
    adf_scaled_set(snap, x);
    adf_scaled_swap(x, x);
    ADF_CHECK(adf_scaled_identical(x, snap));

    adf_scaled_clear(x);
    adf_scaled_clear(z);
    adf_scaled_clear(snap);
    adf_rat_clear(s);
    adf_rat_clear(q);
    fmpz_clear(u);
    fmpz_clear(K);
    fmpz_clear(K2);
    adf_modctx_free(ctx);
    adf_modctx_free(ctx2);
}

/* ---- 14. moduli of one word and of 4096 bits (context without blocks, conventions 5.14) ---- */

ADF_TEST(one_word_and_4096_bit_moduli)
{
    adf_modctx_struct * ctx1 = NULL;
    adf_modctx_struct * ctx2 = NULL;
    adf_scaled_t x, y, z, x2;
    adf_rat_t s, t;
    fmpz_t K1, K2, u, v;
    int trial;

    /* K1 = 2^61 + 3 (one word), K2 = 2^4095 + 19 (4096 bits) */
    fmpz_init(K1);
    fmpz_init(K2);
    fmpz_one(K1);
    fmpz_mul_2exp(K1, K1, 61);
    fmpz_add_ui(K1, K1, 3);
    fmpz_one(K2);
    fmpz_mul_2exp(K2, K2, 4095);
    fmpz_add_ui(K2, K2, 19);
    ADF_CHECK(adf_modctx_new_fmpz(&ctx1, K1) == ADF_OK);
    ADF_CHECK(adf_modctx_new_fmpz(&ctx2, K2) == ADF_OK);
    adf_scaled_init(x, ctx1);
    adf_scaled_init(y, ctx1);
    adf_scaled_init(z, ctx1);
    adf_scaled_init(x2, ctx2);
    adf_rat_init(s);
    adf_rat_init(t);
    fmpz_init(u);
    fmpz_init(v);

    for (trial = 0; trial < 3; trial++)
    {
        adf_fball_t bx, by, tight, ft;

        /* residues of the full size of the modulus, scales of 2000 resp. 1500 bits */
        fmpz_one(u);
        fmpz_mul_2exp(u, u, 61);
        fmpz_sub_ui(u, u, 100 + (ulong) trial);
        fmpz_mod(u, u, K1);
        fmpz_one(v);
        fmpz_mul_2exp(v, v, 4095);
        fmpz_add_ui(v, v, 12345 + (ulong) trial);
        fmpz_mod(v, v, K2);
        fmpz_one(fmpq_numref(s->q));
        fmpz_mul_2exp(fmpq_numref(s->q), fmpq_numref(s->q), 2000);
        fmpz_set_si(fmpq_denref(s->q), 3);
        fmpq_canonicalise(s->q);
        fmpz_one(fmpq_numref(t->q));
        fmpz_mul_2exp(fmpq_numref(t->q), fmpq_numref(t->q), 1500);
        fmpz_set_si(fmpq_denref(t->q), 5);
        fmpq_canonicalise(t->q);

        scaled_from(x, s, u, ctx1);
        scaled_from(y, t, u, ctx1);
        ADF_CHECK(adf_scaled_is_canonical(x));
        ADF_CHECK(adf_scaled_add(z, x, y) == ADF_OK);
        ADF_CHECK(adf_scaled_is_canonical(z));
        ADF_CHECK(adf_scaled_mul(z, x, y) == ADF_OK);
        ADF_CHECK(adf_scaled_is_canonical(z));
        ADF_CHECK(adf_scaled_mul_tight(z, x, y) == ADF_OK);
        ADF_CHECK(adf_scaled_is_canonical(z));
        /* the tight variant equals the tight product of the sets */
        adf_fball_init(bx);
        adf_fball_init(by);
        adf_fball_init(tight);
        adf_fball_init(ft);
        adf_scaled_get_fball(bx, x);
        adf_scaled_get_fball(by, y);
        adf_fball_mul(tight, bx, by);
        adf_scaled_get_fball(ft, z);
        ADF_CHECK(adf_fball_equal_set(tight, ft));
        adf_fball_clear(bx);
        adf_fball_clear(by);
        adf_fball_clear(tight);
        adf_fball_clear(ft);

        /* one value in the 4096-bit context; conversion to a multiple of K is lossless */
        scaled_from(x2, s, v, ctx2);
        ADF_CHECK(adf_scaled_is_canonical(x2));
        {
            adf_modctx_struct * ctx3 = NULL;
            adf_scaled_t y2;
            fmpz_t K3;
            int lost = -1;

            fmpz_init(K3);
            fmpz_mul_ui(K3, K2, 2); /* K divides 2 K */
            ADF_CHECK(adf_modctx_new_fmpz(&ctx3, K3) == ADF_OK);
            adf_scaled_init(y2, ctx3);
            ADF_CHECK(adf_scaled_set_context(y2, &lost, x2, ctx3) == ADF_OK);
            ADF_CHECK_MSG(lost == 0, "K divides K' is lossless at 4096 bits");
            ADF_CHECK(sets_equal(x2, y2));
            adf_scaled_clear(y2);
            fmpz_clear(K3);
            adf_modctx_free(ctx3);
        }
    }
    adf_scaled_clear(x);
    adf_scaled_clear(y);
    adf_scaled_clear(z);
    adf_scaled_clear(x2);
    adf_rat_clear(s);
    adf_rat_clear(t);
    fmpz_clear(u);
    fmpz_clear(v);
    fmpz_clear(K1);
    fmpz_clear(K2);
    adf_modctx_free(ctx1);
    adf_modctx_free(ctx2);
}

/* ---- 15. exact zero and zero residues through the operations ---- */

ADF_TEST(exact_zero_and_zero_residues)
{
    adf_modctx_struct * ctx = NULL;
    adf_scaled_t x, y, z, zero;
    adf_rat_t s, q;
    fmpz_t K, u;

    fmpz_init_set_si(K, 6);
    ADF_CHECK(adf_modctx_new_fmpz(&ctx, K) == ADF_OK);
    adf_scaled_init(x, ctx);
    adf_scaled_init(y, ctx);
    adf_scaled_init(z, ctx);
    adf_scaled_init(zero, ctx);
    adf_rat_init(s);
    adf_rat_init(q);
    fmpz_init(u);

    rat_si(s, 5, 3);
    fmpz_set_si(u, 0);       /* the residue 0: the set s K Zhat */
    scaled_from(x, s, u, ctx);
    rat_si(q, 0, 1);
    adf_scaled_set_rat(zero, q, ctx); /* the exact 0 */

    ADF_CHECK(adf_scaled_add(z, x, zero) == ADF_OK);
    ADF_CHECK_MSG(sets_equal(z, x), "x + 0 = x as a set");
    ADF_CHECK(adf_scaled_add(z, zero, x) == ADF_OK);
    ADF_CHECK(sets_equal(z, x));
    ADF_CHECK(adf_scaled_add(z, zero, zero) == ADF_OK);
    ADF_CHECK(adf_scaled_is_exact(z));
    ADF_CHECK(adf_scaled_mul(z, x, zero) == ADF_OK);
    ADF_CHECK_MSG(adf_scaled_is_exact(z), "x * 0 is the exact 0");
    ADF_CHECK(adf_scaled_mul(z, zero, zero) == ADF_OK);
    ADF_CHECK(adf_scaled_is_exact(z));
    adf_scaled_mul_rat(z, x, q);
    ADF_CHECK(adf_scaled_is_exact(z));
    adf_scaled_add_rat(z, NULL, x, q);
    ADF_CHECK(sets_equal(z, x));
    /* u = v = 0 in the product: the default product loses the full factor K */
    ADF_CHECK(adf_scaled_mul(z, x, x) == ADF_OK);
    ADF_CHECK(adf_scaled_mul_tight(y, x, x) == ADF_OK);
    ADF_CHECK_MSG(!sets_equal(z, y), "u = v = 0 loses the factor K");
    ADF_CHECK(set_contains(y, z));

    adf_scaled_clear(x);
    adf_scaled_clear(y);
    adf_scaled_clear(z);
    adf_scaled_clear(zero);
    adf_rat_clear(s);
    adf_rat_clear(q);
    fmpz_clear(u);
    fmpz_clear(K);
    adf_modctx_free(ctx);
}

/* ---- 16. random expressions of 30 operations in both policies, containment after every
         step (policies Theorem 3, line 75) ---- */

ADF_TEST(tight_and_scaled_policies_containment)
{
    flint_rand_t state;
    int expr, step;

    flint_randinit(state);
    flint_randseed(state, 20260928UL, 0UL);
    for (expr = 0; expr < 3; expr++)
    {
        adf_modctx_struct * ctx = NULL;
        adf_scaled_t acc, leaf, tmp;
        adf_fball_t facc, fleaf, ftmp, fconv;
        adf_rat_t s, q;
        fmpz_t K, u;

        fmpz_init_set_si(K, 6 + expr);
        ADF_CHECK(adf_modctx_new_fmpz(&ctx, K) == ADF_OK);
        adf_scaled_init(acc, ctx);
        adf_scaled_init(leaf, ctx);
        adf_scaled_init(tmp, ctx);
        adf_fball_init(facc);
        adf_fball_init(fleaf);
        adf_fball_init(ftmp);
        adf_fball_init(fconv);
        adf_rat_init(s);
        adf_rat_init(q);
        fmpz_init(u);

        for (step = 0; step < 30; step++)
        {
            int op = (int) n_randint(state, 7);
            long num = (long) n_randint(state, 21) - 10;
            ulong den = 1 + n_randint(state, 4);

            /* the leaf: the scaled value s (u + K Zhat) and the same set as a tight ball */
            rat_si(s, 1 + (long) n_randint(state, 6), den);
            fmpz_randm(u, state, K);
            scaled_from(leaf, s, u, ctx);
            {
                adf_rat_t c, r;
                fmpz_t Kf;
                adf_rat_init(c);
                adf_rat_init(r);
                fmpz_init(Kf);
                adf_modctx_get_modulus(Kf, ctx);
                fmpq_mul_fmpz(c->q, s->q, u);
                fmpq_mul_fmpz(r->q, s->q, Kf);
                ADF_CHECK(adf_fball_set_center_radius(fleaf, c, r) == ADF_OK);
                adf_rat_clear(c);
                adf_rat_clear(r);
                fmpz_clear(Kf);
            }
            rat_si(q, num, den);

            if (step == 0)
            {
                adf_scaled_set(acc, leaf);
                adf_fball_set(facc, fleaf);
            }
            else if (op == 0)
            {
                adf_scaled_add(tmp, acc, leaf);
                adf_scaled_set(acc, tmp);
                adf_fball_add(ftmp, facc, fleaf);
                adf_fball_set(facc, ftmp);
            }
            else if (op == 1)
            {
                adf_scaled_sub(tmp, acc, leaf);
                adf_scaled_set(acc, tmp);
                adf_fball_sub(ftmp, facc, fleaf);
                adf_fball_set(facc, ftmp);
            }
            else if (op == 2)
            {
                adf_scaled_mul(tmp, acc, leaf);
                adf_scaled_set(acc, tmp);
                adf_fball_mul(ftmp, facc, fleaf);
                adf_fball_set(facc, ftmp);
            }
            else if (op == 3)
            {
                adf_scaled_mul_tight(tmp, acc, leaf);
                adf_scaled_set(acc, tmp);
                adf_fball_mul(ftmp, facc, fleaf);
                adf_fball_set(facc, ftmp);
            }
            else if (op == 4)
            {
                adf_scaled_neg(tmp, acc);
                adf_scaled_set(acc, tmp);
                adf_fball_neg(ftmp, facc);
                adf_fball_set(facc, ftmp);
            }
            else if (op == 5)
            {
                adf_scaled_mul_rat(acc, acc, q);
                adf_fball_mul_rat(facc, facc, q);
            }
            else
            {
                adf_scaled_add_rat(acc, NULL, acc, q);
                adf_fball_set_rat(ftmp, q);
                adf_fball_add(facc, facc, ftmp);
            }
            ADF_CHECK(adf_scaled_is_canonical(acc));
            /* containment after every step: the scaled result contains the tight result */
            adf_scaled_get_fball(fconv, acc);
            ADF_CHECK_MSG(adf_fball_contains(facc, fconv),
                          "expression %d step %d: the scaled result contains the tight one",
                          expr, step);
        }
        adf_scaled_clear(acc);
        adf_scaled_clear(leaf);
        adf_scaled_clear(tmp);
        adf_fball_clear(facc);
        adf_fball_clear(fleaf);
        adf_fball_clear(ftmp);
        adf_fball_clear(fconv);
        adf_rat_clear(s);
        adf_rat_clear(q);
        fmpz_clear(u);
        fmpz_clear(K);
        adf_modctx_free(ctx);
    }
    flint_randclear(state);
}

/* ---- 17. a local adf_fball input is converted as its set (scaled.h: "ADF_OK always";
        fball.h admits both backends). The result is identical to the conversion of the
        global form of the same set, lost included, in the context of the value and in
        another one. Until work package 1.8 existed the function refused local inputs. ---- */

ADF_TEST(set_fball_converts_a_local_value_as_its_set)
{
    adf_modctx_struct * ctx = NULL;
    adf_modctx_struct * other = NULL;
    ulong q[2] = {4, 3};
    ulong q2[1] = {5};
    adf_fball_t g, x;
    adf_scaled_t y, e, snap;
    adf_rat_t c, r;
    int lost, elost, k;
    static const slong cn[4] = {1, 5, -7, 2};
    static const slong cd[4] = {2, 6, 1, 2};
    static const slong rn[4] = {6, 2, 12, 1};   /* radii 6, 2/1, 12, 1/... chosen so that the */
    static const slong rd[4] = {1, 1, 1, 1};   /* ball lives in the context of modulus 12     */

    ADF_CHECK(adf_modctx_new_blocks(&ctx, q, 2) == ADF_OK);
    ADF_CHECK(adf_modctx_new_blocks(&other, q2, 1) == ADF_OK);
    adf_fball_init(g);
    adf_fball_init(x);
    adf_rat_init(c);
    adf_rat_init(r);
    for (k = 0; k < 4; k++)
    {
        const adf_modctx_struct * target = (k % 2 == 0) ? ctx : other;

        adf_rat_set_si(c, cn[k]);
        adf_rat_set_si(r, cd[k]);
        ADF_CHECK(adf_rat_div(c, c, r) == ADF_OK);
        adf_rat_set_si(r, rn[k]);
        (void) rd;
        ADF_CHECK(adf_fball_set_center_radius(g, c, r) == ADF_OK);
        ADF_CHECK(adf_fball_set_local_enclose(x, NULL, g, ctx) == ADF_OK);
        ADF_CHECK(adf_fball_is_local(x));
        adf_fball_set_global(g, x);              /* the same set, global */
        adf_scaled_init(y, target);
        adf_scaled_init(e, target);
        adf_scaled_init(snap, target);
        lost = -1;
        elost = -2;
        ADF_CHECK(adf_scaled_set_fball(e, &elost, g, target) == ADF_OK);
        ADF_CHECK(adf_scaled_set_fball(y, &lost, x, target) == ADF_OK);
        ADF_CHECK(!adf_scaled_identical(y, snap) || adf_scaled_identical(e, snap));
        ADF_CHECK(adf_scaled_identical(y, e));
        ADF_CHECK(lost == elost);
        ADF_CHECK(adf_scaled_is_canonical(y));
        adf_scaled_clear(y);
        adf_scaled_clear(e);
        adf_scaled_clear(snap);
    }
    adf_rat_clear(c);
    adf_rat_clear(r);
    adf_fball_clear(g);
    adf_fball_clear(x);
    adf_modctx_free(ctx);
    adf_modctx_free(other);
}

/* ---- 18. every exact result stores the residue u = 0 (conventions 5.4: exact = 1 needs
        u = 0), also when the output held a scaled value before ---- */

ADF_TEST(exact_results_store_a_zero_residue)
{
    adf_modctx_struct * ctx = NULL;
    adf_modctx_struct * ctx2 = NULL;
    adf_scaled_t x, y;
    adf_rat_t s, q, c, r;
    fmpz_t K, K2, u;
    adf_fball_t b;

    fmpz_init_set_si(K, 6);
    fmpz_init_set_si(K2, 4);
    ADF_CHECK(adf_modctx_new_fmpz(&ctx, K) == ADF_OK);
    ADF_CHECK(adf_modctx_new_fmpz(&ctx2, K2) == ADF_OK);
    adf_scaled_init(x, ctx);
    adf_scaled_init(y, ctx);
    adf_rat_init(s);
    adf_rat_init(q);
    adf_rat_init(c);
    adf_rat_init(r);
    fmpz_init(u);
    adf_fball_init(b);

    /* every call below writes into a y that holds (5/3)(4 + 6 Zhat), so u = 4 is stale data
       unless the function stores u = 0 */
    rat_si(s, 5, 3);
    fmpz_set_si(u, 4);

    /* set_rat */
    scaled_from(y, s, u, ctx);
    rat_si(q, -7, 2);
    adf_scaled_set_rat(y, q, ctx);
    ADF_CHECK_MSG(adf_scaled_is_canonical(y), "set_rat: exact = 1 needs u = 0");
    ADF_CHECK(fmpz_is_zero(y->u));

    /* set_fball of an exact ball (R = 0, closure C5) */
    scaled_from(y, s, u, ctx);
    rat_si(c, 3, 4);
    rat_si(r, 0, 1);
    ADF_CHECK(adf_fball_set_center_radius(b, c, r) == ADF_OK);
    ADF_CHECK(adf_scaled_set_fball(y, NULL, b, ctx) == ADF_OK);
    ADF_CHECK_MSG(adf_scaled_is_canonical(y), "set_fball R = 0: exact = 1 needs u = 0");
    ADF_CHECK(fmpz_is_zero(y->u));

    /* mul_rat with q = 0: the exact 0 */
    scaled_from(y, s, u, ctx);
    rat_si(q, 0, 1);
    adf_scaled_mul_rat(y, y, q);
    ADF_CHECK_MSG(adf_scaled_is_canonical(y), "mul_rat q = 0: exact = 1 needs u = 0");
    ADF_CHECK(fmpz_is_zero(y->u));

    /* mul_rat with an exact x: the exact product */
    scaled_from(y, s, u, ctx);
    rat_si(q, -7, 2);
    adf_scaled_set_rat(x, q, ctx);
    rat_si(q, 3, 5);
    adf_scaled_mul_rat(y, x, q);
    ADF_CHECK_MSG(adf_scaled_is_canonical(y), "mul_rat of an exact x: exact = 1 needs u = 0");
    ADF_CHECK(fmpz_is_zero(y->u));

    /* neg of an exact value */
    scaled_from(y, s, u, ctx);
    adf_scaled_neg(y, x);
    ADF_CHECK_MSG(adf_scaled_is_canonical(y), "neg of an exact x: exact = 1 needs u = 0");
    ADF_CHECK(fmpz_is_zero(y->u));

    /* set_context of an exact value */
    scaled_from(y, s, u, ctx);
    ADF_CHECK(adf_scaled_set_context(y, NULL, x, ctx2) == ADF_OK);
    ADF_CHECK_MSG(adf_scaled_is_canonical(y), "set_context of an exact x: exact = 1 needs u = 0");
    ADF_CHECK(fmpz_is_zero(y->u));

    adf_scaled_clear(x);
    adf_scaled_clear(y);
    adf_rat_clear(s);
    adf_rat_clear(q);
    adf_rat_clear(c);
    adf_rat_clear(r);
    fmpz_clear(u);
    fmpz_clear(K);
    fmpz_clear(K2);
    adf_fball_clear(b);
    adf_modctx_free(ctx);
    adf_modctx_free(ctx2);
}
