/* tests/test_dump_ctx.c: the dump form of values with a context occurrence (lane m1-dump):
   local adf_fball, adf_adele and adf_cadele with a local finite part, adf_scaled; the binding
   rules; the inspectors; adf_modctx_new_from_dump on every body with context occurrences; the
   reference vectors tests/ref/vectors/m1-dump/dump_ref.jsonl and the from_dump records of
   tests/ref/vectors/m1-modctx/modctx.jsonl; adf_scaled_get_str.

   Contract: include/adelefeld/dump.h, modctx.h; docs/conventions.md 10.1, 10.2 (bindings G3,
   inspection C2, adf_modctx_new_from_dump C3), 5.3 (predicate L), 5.4 (scaled), 8.5, 4.3.

   Helpers (brief of lane m1-dump). The functions of scaled.h and the local conversions of
   modctx.h are written by other lanes and are not used: local balls and scaled values are built
   and cleared field by field below, as the layouts of fball.h and scaled.h and the predicates of
   conventions 5.3 and 5.4 describe them. The helpers are:
     local_set(x, ctx, d, res)      a local adf_fball (A = 0, H = K, d, residues; res owned)
     local_identical(x, y)           backend, context pointer, A, H, d and every residue
     local_is_L(x)                   predicate L of conventions 5.3, blocks included
     sc_init(x, ctx), sc_clear(x)    an adf_scaled, exact 0 in ctx (conventions 5.4, init)
     sc_identical(x, y), sc_is_canonical(x)
     ctx_from(K, q, k)               a context with modulus K and blocks q (any K when k = 0) */

#define _DEFAULT_SOURCE

#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/fmpq.h>
#include <flint/arb.h>
#include <flint/acb.h>
#include <flint/ulong_extras.h>

#include <adelefeld.h>

#include "support/jsonl.h"
#include "test_runner.h"

/* ------------------------------------------------------------------ helpers: contexts */

/* A context with modulus K and blocks q[0..k-1]. For k >= 1 the modulus is their product
   (adf_modctx_new_blocks); for k = 0, K = 1 or K >= 2^64 come from adf_modctx_new_fmpz and any
   other K from the dump of a modctx body (conventions 10.1). */
static adf_modctx_struct *
ctx_from(const fmpz_t K, const ulong * q, slong k)
{
    adf_modctx_struct * c = NULL;

    if (k > 0)
        ADF_CHECK(adf_modctx_new_blocks(&c, q, k) == ADF_OK);
    else if (fmpz_is_one(K) || fmpz_bits(K) > 64)
        ADF_CHECK(adf_modctx_new_fmpz(&c, K) == ADF_OK);
    else
    {
        char * h = fmpz_get_str(NULL, 16, K);
        size_t n = strlen(h);
        char * t = flint_malloc(n + 17);
        memcpy(t, "adf1 Q modctx ", 14);
        memcpy(t + 14, h, n);
        memcpy(t + 14 + n, " 0", 2);
        ADF_CHECK(adf_modctx_new_from_dump(&c, t, n + 16, 0, NULL) == ADF_OK);
        flint_free(t);
        flint_free(h);
    }
    return c;
}

/* k distinct primes: small ones first when `small`, else primes near 2^63 and 2^40. */
static void
some_primes(ulong * q, slong k, int small)
{
    slong i;
    ulong p = small ? 1 : (UWORD(1) << 40);

    for (i = 0; i < k; i++)
    {
        if (!small && i % 2 == 1)
            p = n_nextprime((UWORD(1) << 63) + (ulong) i * (UWORD(1) << 50), 1);
        else
            p = n_nextprime(p, 1);
        q[i] = p;
        if (!small && i % 2 == 1)
            p = (UWORD(1) << 40) + (ulong) i * 1000003;
    }
}

/* The largest prime below 2^64 (18446744073709551557 = 2^64 - 59). */
#define BIGP UWORD(18446744073709551557)

/* ------------------------------------------------------------------ helpers: local fball */

/* x (initialised) becomes the local value (d; res) of ctx (conventions 5.3). */
static void
local_set(adf_fball_t x, const adf_modctx_struct * ctx, const fmpz_t d, const ulong * res)
{
    slong k = adf_modctx_nblocks(ctx);

    fmpz_zero(x->A);
    adf_modctx_get_modulus(x->H, ctx);
    fmpz_set(x->d, d);
    flint_free(x->res);
    x->res = flint_malloc((size_t) k * sizeof(ulong));
    memcpy(x->res, res, (size_t) k * sizeof(ulong));
    x->mctx = ctx;
    x->backend = ADF_LOCAL;
}

static void
local_random(adf_fball_t x, const adf_modctx_struct * ctx, flint_rand_t st, flint_bitcnt_t bits)
{
    slong i, k = adf_modctx_nblocks(ctx);
    ulong * res = flint_malloc((size_t) k * sizeof(ulong));
    fmpz_t d;

    fmpz_init(d);
    fmpz_randtest_not_zero(d, st, bits);
    fmpz_abs(d, d);
    for (i = 0; i < k; i++)
        res[i] = n_randlimb(st) % adf_modctx_block(ctx, i);
    local_set(x, ctx, d, res);
    fmpz_clear(d);
    flint_free(res);
}

static int
local_identical(const adf_fball_t x, const adf_fball_t y)
{
    slong i, k;

    if (x->backend != y->backend || x->mctx != y->mctx)
        return 0;
    if (!fmpz_equal(x->A, y->A) || !fmpz_equal(x->H, y->H) || !fmpz_equal(x->d, y->d))
        return 0;
    if (x->backend != ADF_LOCAL)
        return x->res == NULL && y->res == NULL;
    k = adf_modctx_nblocks(x->mctx);
    for (i = 0; i < k; i++)
        if (x->res[i] != y->res[i])
            return 0;
    return 1;
}

/* Predicate L (conventions 5.3, line 429): mctx != NULL, k >= 1, H = K, A = 0, res != NULL,
   d >= 1, 0 <= res[i] < q_i. */
static int
local_is_L(const adf_fball_t x)
{
    fmpz_t K;
    slong i, k;
    int ok;

    if (x->backend != ADF_LOCAL || x->mctx == NULL || x->res == NULL)
        return 0;
    k = adf_modctx_nblocks(x->mctx);
    fmpz_init(K);
    adf_modctx_get_modulus(K, x->mctx);
    ok = k >= 1 && fmpz_equal(K, x->H) && fmpz_is_zero(x->A) && fmpz_sgn(x->d) >= 1;
    for (i = 0; ok && i < k; i++)
        ok = x->res[i] < adf_modctx_block(x->mctx, i);
    fmpz_clear(K);
    return ok;
}

/* ------------------------------------------------------------------ helpers: scaled */

static void
sc_init(adf_scaled_t x, const adf_modctx_struct * ctx)
{
    fmpq_init(x->s);
    fmpz_init(x->u);
    x->mctx = ctx;
    x->exact = 1;
}

static void
sc_clear(adf_scaled_t x)
{
    fmpq_clear(x->s);
    fmpz_clear(x->u);
}

static int
sc_identical(const adf_scaled_t x, const adf_scaled_t y)
{
    return x->mctx == y->mctx && x->exact == y->exact && fmpq_equal(x->s, y->s) && fmpz_equal(x->u, y->u);
}

/* conventions 5.4, line 477. */
static int
sc_is_canonical(const adf_scaled_t x)
{
    fmpz_t K;
    int ok;

    if (x->mctx == NULL || (x->exact != 0 && x->exact != 1) || !fmpq_is_canonical(x->s))
        return 0;
    if (x->exact)
        return fmpz_is_zero(x->u);
    fmpz_init(K);
    adf_modctx_get_modulus(K, x->mctx);
    ok = fmpq_sgn(x->s) > 0 && fmpz_sgn(x->u) >= 0 && fmpz_cmp(x->u, K) < 0;
    fmpz_clear(K);
    return ok;
}

static void
sc_random(adf_scaled_t x, const adf_modctx_struct * ctx, flint_rand_t st, flint_bitcnt_t bits)
{
    fmpz_t K;

    x->mctx = ctx;
    fmpq_randtest(x->s, st, bits);
    if (n_randint(st, 3) == 0)
    {
        x->exact = 1;
        fmpz_zero(x->u);
        return;
    }
    x->exact = 0;
    if (fmpq_is_zero(x->s))
        fmpq_one(x->s);
    fmpq_abs(x->s, x->s);
    fmpz_init(K);
    adf_modctx_get_modulus(K, ctx);
    fmpz_randm(x->u, st, K);
    fmpz_clear(K);
}

/* ------------------------------------------------------------------ helpers: text */

static int
dump_well_formed(const char * t, size_t len)
{
    size_t i;

    if (t == NULL || len < 8 || t[len] != '\0' || memcmp(t, "adf1 Q ", 7) != 0 || t[len - 1] == ' ')
        return 0;
    for (i = 0; i < len; i++)
    {
        if ((unsigned char) t[i] < 0x20 || (unsigned char) t[i] > 0x7e)
            return 0;
        if (t[i] == ' ' && i + 1 < len && t[i + 1] == ' ')
            return 0;
    }
    return 1;
}

/* The expected text " K k q_1 .. q_k" of a context, from its read access. */
static char *
ctx_tokens(const adf_modctx_struct * c)
{
    size_t len;
    char * d = adf_modctx_dump_str(&len, c);
    char * r = flint_malloc(len - 12 + 1);

    memcpy(r, d + 13, len - 13);        /* after "adf1 Q modctx" */
    r[len - 13] = '\0';
    adf_str_free(d);
    return r;
}

/* ------------------------------------------------------------------ local fball */

static void
local_round_trip(const adf_modctx_struct * ctx, flint_rand_t st, flint_bitcnt_t bits)
{
    adf_fball_t x, y;
    size_t len, len2;
    char * t, * t2, * ct;
    const adf_modctx_struct * binds[1];

    adf_fball_init(x);
    adf_fball_init(y);
    local_random(x, ctx, st, bits);
    ADF_CHECK(local_is_L(x));
    t = adf_fball_dump_str(&len, x);
    ADF_CHECK(dump_well_formed(t, len));
    /* the text: "adf1 Q fball l" d, then the context K k q.., then the residues */
    ct = ctx_tokens(ctx);
    {
        char * dh = fmpz_get_str(NULL, 16, x->d);
        size_t nd = strlen(dh), nc = strlen(ct);
        ADF_CHECK(memcmp(t, "adf1 Q fball l ", 15) == 0 && memcmp(t + 15, dh, nd) == 0
                  && memcmp(t + 15 + nd, ct, nc) == 0);
        flint_free(dh);
    }
    flint_free(ct);
    ADF_CHECK(adf_fball_load_str(y, t, len, ctx, NULL) == ADF_OK);
    ADF_CHECK(local_identical(x, y) && local_is_L(y));
    t2 = adf_fball_dump_str(&len2, y);
    ADF_CHECK(len2 == len && memcmp(t, t2, len) == 0);
    adf_str_free(t2);
    adf_fball_zero(y);
    binds[0] = ctx;
    ADF_CHECK(adf_fball_load_str_binds(y, t, len, binds, 1, NULL) == ADF_OK && local_identical(x, y));
    adf_str_free(t);
    adf_fball_clear(x);
    adf_fball_clear(y);
}

ADF_TEST(local_fball_round_trip_1_2_64_128_blocks)
{
    static const slong ks[] = {1, 2, 64, 128};
    flint_rand_t st;
    size_t j;
    int i, small;

    flint_randinit(st);
    for (j = 0; j < sizeof(ks) / sizeof(ks[0]); j++)
        for (small = 0; small <= 1; small++)
        {
            ulong * q = flint_malloc((size_t) ks[j] * sizeof(ulong));
            adf_modctx_struct * ctx;
            some_primes(q, ks[j], small);
            ctx = ctx_from(NULL, q, ks[j]);
            for (i = 0; i < 20; i++)
                local_round_trip(ctx, st, i % 5 == 0 ? 4096 : 1 + n_randint(st, 100));
            adf_modctx_free(ctx);
            flint_free(q);
        }
    /* one block at the largest prime below 2^64, and prime powers */
    {
        ulong q1 = BIGP, q2[3] = {UWORD(1) << 63, 3486784401u, 5};   /* 2^63, 3^20, 5 */
        adf_modctx_struct * c1 = ctx_from(NULL, &q1, 1), * c2 = ctx_from(NULL, q2, 3);
        for (i = 0; i < 20; i++)
        {
            local_round_trip(c1, st, 1 + n_randint(st, 200));
            local_round_trip(c2, st, 1 + n_randint(st, 200));
        }
        adf_modctx_free(c1);
        adf_modctx_free(c2);
    }
    flint_randclear(st);
}

/* The raw local value of conventions 10.2 (line 1370): (0 + 6 Zhat)/2, no gcd condition. */
ADF_TEST(local_fball_raw_example)
{
    ulong q[2] = {2, 3};
    adf_modctx_struct * c = ctx_from(NULL, q, 2);
    adf_fball_t x;
    size_t len;
    char * t;

    adf_fball_init(x);
    ADF_CHECK(adf_fball_load_str(x, "adf1 Q fball l 2 6 2 2 3 0 0", 28, c, NULL) == ADF_OK);
    ADF_CHECK(local_is_L(x) && fmpz_equal_ui(x->d, 2) && x->res[0] == 0 && x->res[1] == 0 && x->mctx == c);
    t = adf_fball_dump_str(&len, x);
    ADF_CHECK(len == 28 && strcmp(t, "adf1 Q fball l 2 6 2 2 3 0 0") == 0);
    adf_str_free(t);
    /* a residue out of range, d = 0, K not the product, blocks not coprime: DOMAIN */
    ADF_CHECK(adf_fball_load_str(x, "adf1 Q fball l 1 6 2 2 3 2 0", 28, c, NULL) == ADF_DOMAIN);
    ADF_CHECK(adf_fball_load_str(x, "adf1 Q fball l 0 6 2 2 3 0 0", 28, c, NULL) == ADF_DOMAIN);
    ADF_CHECK(adf_fball_load_str(x, "adf1 Q fball l 1 7 2 2 3 0 0", 28, c, NULL) == ADF_DOMAIN);
    ADF_CHECK(adf_fball_load_str(x, "adf1 Q fball l 1 8 2 2 4 0 2", 28, c, NULL) == ADF_DOMAIN);
    ADF_CHECK(adf_fball_load_str(x, "adf1 Q fball l 1 1 0", 20, c, NULL) == ADF_DOMAIN);
    ADF_CHECK(adf_fball_load_str(x, "adf1 Q fball l 1 6 2 2 3 0 -1", 29, c, NULL) == ADF_DOMAIN);
    ADF_CHECK(adf_fball_load_str(x, "adf1 Q fball l -1 6 2 2 3 0 1", 29, c, NULL) == ADF_DOMAIN);
    ADF_CHECK(local_is_L(x) && fmpz_equal_ui(x->d, 2) && x->res[0] == 0 && x->mctx == c);
    adf_fball_clear(x);
    adf_modctx_free(c);
}

ADF_TEST(adele_cadele_local_round_trip)
{
    flint_rand_t st;
    ulong q[3] = {4, 9, 25};
    adf_modctx_struct * ctx;
    int i;

    flint_randinit(st);
    ctx = ctx_from(NULL, q, 3);
    for (i = 0; i < 100; i++)
    {
        adf_adele_t x, y;
        adf_cadele_t z, w;
        size_t len, len2;
        char * t, * t2;
        const adf_modctx_struct * binds[1] = {ctx};

        adf_adele_init(x);
        adf_adele_init(y);
        adf_cadele_init(z);
        adf_cadele_init(w);
        arb_randtest(x->inf, st, 1 + n_randint(st, 300), 20);
        local_random(&x->fin, ctx, st, i % 10 == 0 ? 4096 : 60);
        t = adf_adele_dump_str(&len, x);
        ADF_CHECK(dump_well_formed(t, len));
        ADF_CHECK(adf_adele_load_str_binds(y, t, len, binds, 1, NULL) == ADF_OK);
        ADF_CHECK(arb_equal(x->inf, y->inf) && local_identical(&x->fin, &y->fin) && local_is_L(&y->fin));
        t2 = adf_adele_dump_str(&len2, y);
        ADF_CHECK(len == len2 && memcmp(t, t2, len) == 0);
        adf_str_free(t);
        adf_str_free(t2);

        acb_randtest(z->inf, st, 1 + n_randint(st, 300), 20);
        local_random(&z->fin, ctx, st, 60);
        t = adf_cadele_dump_str(&len, z);
        ADF_CHECK(adf_cadele_load_str(w, t, len, ctx, NULL) == ADF_OK);
        ADF_CHECK(acb_equal(z->inf, w->inf) && local_identical(&z->fin, &w->fin));
        t2 = adf_cadele_dump_str(&len2, w);
        ADF_CHECK(len == len2 && memcmp(t, t2, len) == 0);
        adf_str_free(t);
        adf_str_free(t2);
        adf_adele_clear(x);
        adf_adele_clear(y);
        adf_cadele_clear(z);
        adf_cadele_clear(w);
    }
    adf_modctx_free(ctx);
    flint_randclear(st);
}

/* ------------------------------------------------------------------ scaled */

static void
scaled_round_trip(const adf_modctx_struct * ctx, flint_rand_t st, flint_bitcnt_t bits)
{
    adf_scaled_t x, y;
    size_t len, len2;
    char * t, * t2;

    sc_init(x, ctx);
    sc_init(y, ctx);
    sc_random(x, ctx, st, bits);
    ADF_CHECK(sc_is_canonical(x));
    t = adf_scaled_dump_str(&len, x);
    ADF_CHECK(dump_well_formed(t, len));
    ADF_CHECK(memcmp(t, x->exact ? "adf1 Q scaled x " : "adf1 Q scaled s ", 16) == 0);
    ADF_CHECK(adf_scaled_load_str(y, t, len, ctx, NULL) == ADF_OK);
    ADF_CHECK(sc_identical(x, y) && sc_is_canonical(y));
    t2 = adf_scaled_dump_str(&len2, y);
    ADF_CHECK(len2 == len && memcmp(t, t2, len) == 0);
    adf_str_free(t);
    adf_str_free(t2);
    sc_clear(x);
    sc_clear(y);
}

ADF_TEST(scaled_round_trip_contexts)
{
    flint_rand_t st;
    fmpz_t K;
    ulong q[2] = {8, 9};
    ulong * big = flint_malloc(128 * sizeof(ulong));
    adf_modctx_struct * c[5];
    int i, j;

    flint_randinit(st);
    fmpz_init(K);
    fmpz_one(K);
    c[0] = ctx_from(K, NULL, 0);                  /* K = 1, no block */
    fmpz_set_ui(K, 6);
    c[1] = ctx_from(K, NULL, 0);                  /* K = 6, no block: only a dump makes it */
    fmpz_one(K);
    fmpz_mul_2exp(K, K, 4096);
    fmpz_add_ui(K, K, 1);
    c[2] = ctx_from(K, NULL, 0);                  /* K = 2^4096 + 1 */
    c[3] = ctx_from(NULL, q, 2);
    some_primes(big, 128, 0);
    c[4] = ctx_from(NULL, big, 128);
    for (j = 0; j < 5; j++)
        for (i = 0; i < 40; i++)
            scaled_round_trip(c[j], st, i % 8 == 0 ? 4096 : 1 + n_randint(st, 120));
    for (j = 0; j < 5; j++)
        adf_modctx_free(c[j]);
    fmpz_clear(K);
    flint_free(big);
    flint_randclear(st);
}

static int
scaled_status(const char * s, size_t len, const adf_modctx_struct * ctx, const adf_text_limits_t * lim)
{
    adf_scaled_t x;
    adf_scaled_struct copy;
    int st;

    memset(x, 0, sizeof(x));             /* padding bytes defined for memcmp */
    sc_init(x, (const adf_modctx_struct *) (void *) 0x10);
    fmpq_set_si(x->s, 5, 3);
    fmpz_set_ui(x->u, 2);
    x->exact = 0;
    memcpy(&copy, x, sizeof(copy));
    st = adf_scaled_load_str(x, s, len, ctx, lim);
    if (st != ADF_OK)
        ADF_CHECK_MSG(memcmp(&copy, x, sizeof(copy)) == 0 && fmpz_equal_ui(x->u, 2), "scaled written on %s",
                      adf_status_str(st));
    sc_clear(x);
    return st;
}

#define SS(lit, ctx) scaled_status((lit), sizeof(lit) - 1, (ctx), NULL)

ADF_TEST(scaled_statuses)
{
    ulong q[2] = {2, 3}, q6 = 6;
    fmpz_t K;
    adf_modctx_struct * c23 = ctx_from(NULL, q, 2), * c6 = ctx_from(NULL, &q6, 1), * c1;
    adf_text_limits_t lim;

    fmpz_init_set_ui(K, 1);
    c1 = ctx_from(K, NULL, 0);
    ADF_CHECK(SS("adf1 Q scaled x 0 1 6 2 2 3", c23) == ADF_OK);
    ADF_CHECK(SS("adf1 Q scaled s 1 2 5 6 2 2 3", c23) == ADF_OK);
    ADF_CHECK(SS("adf1 Q scaled x 1 2 1 0", c1) == ADF_OK);
    ADF_CHECK(SS("adf1 Q scaled x -7 2 1 0", c1) == ADF_OK);
    ADF_CHECK(SS("adf1 Q scaled s 1 2 6 6 2 2 3", c23) == ADF_DOMAIN);   /* u = K */
    ADF_CHECK(SS("adf1 Q scaled s 0 1 0 6 1 6", c6) == ADF_DOMAIN);      /* s = 0 */
    ADF_CHECK(SS("adf1 Q scaled s -1 2 1 6 1 6", c6) == ADF_DOMAIN);     /* s < 0 */
    ADF_CHECK(SS("adf1 Q scaled s 1 2 -1 6 1 6", c6) == ADF_DOMAIN);     /* u < 0 */
    ADF_CHECK(SS("adf1 Q scaled s 2 4 1 6 1 6", c6) == ADF_DOMAIN);      /* s not canonical */
    ADF_CHECK(SS("adf1 Q scaled x 2 4 1 0", c1) == ADF_DOMAIN);
    ADF_CHECK(SS("adf1 Q scaled x 1 0 1 0", c1) == ADF_DOMAIN);
    ADF_CHECK(SS("adf1 Q scaled x 1 2 0 0", c1) == ADF_DOMAIN);          /* K = 0 */
    ADF_CHECK(SS("adf1 Q scaled x 1 2 6 2 2 2", c23) == ADF_DOMAIN);     /* blocks not coprime */
    ADF_CHECK(SS("adf1 Q scaled x 1 2 7 2 2 3", c23) == ADF_DOMAIN);     /* K not the product */
    ADF_CHECK(SS("adf1 Q scaled x 1 2 5 6 1 0", c1) == ADF_PARSE);
    ADF_CHECK(SS("adf1 Q scaled y 1 2 1 0", c1) == ADF_PARSE);
    ADF_CHECK(SS("adf1 Q scaled s 1 2 1 0", c1) == ADF_PARSE);
    ADF_CHECK(SS("adf1 Q scaled x 1 2 1 0 ", c1) == ADF_PARSE);
    ADF_CHECK(SS("adf1 Q scaled x 1 2 1 1", c1) == ADF_PARSE);
    ADF_CHECK(SS("adf1 Q scaled x 1 2 01 0", c1) == ADF_PARSE);
    ADF_CHECK(SS("adf2 Q scaled x 1 2 1 0", c1) == ADF_UNSUPPORTED);
    ADF_CHECK(SS("adf1 K scaled x 1 2 1 0", c1) == ADF_UNSUPPORTED);
    /* bindings: the modulus and the ordered blocks must match */
    ADF_CHECK(SS("adf1 Q scaled x 1 2 6 2 2 3", c6) == ADF_DOMAIN);
    ADF_CHECK(SS("adf1 Q scaled x 1 2 6 2 3 2", c23) == ADF_DOMAIN);
    ADF_CHECK(SS("adf1 Q scaled x 1 2 1 0", c6) == ADF_DOMAIN);
    ADF_CHECK(SS("adf1 Q scaled x 1 2 1 0", NULL) == ADF_DOMAIN);
    /* stage 4: the block count; before stage 6 */
    adf_text_limits_default(&lim);
    lim.max_items = 1;
    ADF_CHECK(scaled_status("adf1 Q scaled x 2 4 6 2 2 3", 27, c23, &lim) == ADF_LIMIT);
    lim.max_items = -1;
    ADF_CHECK(scaled_status("adf1 Q scaled x 1 2 1 0", 23, c1, &lim) == ADF_LIMIT);
    fmpz_clear(K);
    adf_modctx_free(c23);
    adf_modctx_free(c6);
    adf_modctx_free(c1);
}

/* conventions 9.4 row "scaled value" (line 1190): the finite ball s u + s K Zhat, or q(s). */
ADF_TEST(scaled_get_str)
{
    ulong q[2] = {2, 3};
    adf_modctx_struct * c = ctx_from(NULL, q, 2);
    adf_scaled_t x;
    size_t len;
    char * t;
    flint_rand_t st;
    int i;

    sc_init(x, c);
    t = adf_scaled_get_str(&len, x);
    ADF_CHECK(strcmp(t, "(* ; 0)") == 0 && len == 7);
    adf_str_free(t);
    fmpq_set_si(x->s, -7, 3);
    t = adf_scaled_get_str(&len, x);
    ADF_CHECK(strcmp(t, "(* ; -7/3)") == 0 && len == 10);
    adf_str_free(t);
    x->exact = 0;
    fmpq_set_si(x->s, 1, 2);
    fmpz_set_ui(x->u, 5);                    /* 5/2 + 3 Zhat */
    t = adf_scaled_get_str(&len, x);
    ADF_CHECK(strcmp(t, "(* ; 5/2 mod 3)") == 0 && len == 15);
    adf_str_free(t);
    fmpq_set_si(x->s, 4, 1);
    fmpz_set_ui(x->u, 0);                    /* 0 + 24 Zhat */
    t = adf_scaled_get_str(&len, x);
    ADF_CHECK(strcmp(t, "(* ; 0 mod 24)") == 0);
    adf_str_free(t);
    /* random: the text parses to the ball with centre s u and radius s K */
    flint_randinit(st);
    for (i = 0; i < 200; i++)
    {
        adf_fball_t f, g;
        adf_rat_t cc, N;
        sc_random(x, c, st, 1 + n_randint(st, 200));
        t = adf_scaled_get_str(&len, x);
        adf_fball_init(f);
        adf_fball_init(g);
        adf_rat_init(cc);
        adf_rat_init(N);
        ADF_CHECK(adf_fball_set_str(f, t, len, NULL) == ADF_OK);
        if (x->exact)
        {
            fmpq_set(cc->q, x->s);
            adf_fball_set_rat(g, cc);
        }
        else
        {
            fmpq_mul_fmpz(cc->q, x->s, x->u);
            fmpq_mul_ui(N->q, x->s, 6);
            ADF_CHECK(adf_fball_set_center_radius(g, cc, N) == ADF_OK);
        }
        ADF_CHECK(adf_fball_identical(f, g));
        adf_str_free(t);
        adf_fball_clear(f);
        adf_fball_clear(g);
        adf_rat_clear(cc);
        adf_rat_clear(N);
    }
    flint_randclear(st);
    sc_clear(x);
    adf_modctx_free(c);
}

/* ------------------------------------------------------------------ bindings */

static int
fball_binds_status(const char * s, size_t len, const adf_modctx_struct * const * b, size_t nb)
{
    adf_fball_t x;
    adf_fball_struct copy;
    int st;

    memset(x, 0, sizeof(x));             /* padding bytes defined for memcmp */
    adf_fball_init(x);
    fmpz_set_si(x->A, 5);
    fmpz_set_si(x->H, 18);
    memcpy(&copy, x, sizeof(copy));
    st = adf_fball_load_str_binds(x, s, len, b, nb, NULL);
    if (st != ADF_OK)
        ADF_CHECK(memcmp(&copy, x, sizeof(copy)) == 0 && fmpz_equal_si(x->A, 5));
    adf_fball_clear(x);
    return st;
}

#define FB(lit, b, nb) fball_binds_status((lit), sizeof(lit) - 1, (b), (nb))

ADF_TEST(binding_rules)
{
    ulong q23[2] = {2, 3}, q32[2] = {3, 2}, q6 = 6;
    adf_modctx_struct * c23 = ctx_from(NULL, q23, 2), * c32 = ctx_from(NULL, q32, 2);
    adf_modctx_struct * c6 = ctx_from(NULL, &q6, 1), * c23b = ctx_from(NULL, q23, 2);
    const adf_modctx_struct * b1[1], * b2[2], * bn[1] = {NULL};
    const char * text = "adf1 Q fball l 1 6 2 2 3 0 2";

    b1[0] = c23;
    b2[0] = c23;
    b2[1] = c23;
    ADF_CHECK(FB("adf1 Q fball l 1 6 2 2 3 0 2", b1, 1) == ADF_OK);
    /* the count of bindings must be the number of occurrences */
    ADF_CHECK(FB("adf1 Q fball l 1 6 2 2 3 0 2", b1, 0) == ADF_DOMAIN);
    ADF_CHECK(FB("adf1 Q fball l 1 6 2 2 3 0 2", b2, 2) == ADF_DOMAIN);
    ADF_CHECK(FB("adf1 Q fball l 1 6 2 2 3 0 2", NULL, 1) == ADF_DOMAIN);
    ADF_CHECK(FB("adf1 Q fball l 1 6 2 2 3 0 2", NULL, 0) == ADF_DOMAIN);
    ADF_CHECK(FB("adf1 Q fball g 2 6 1", b1, 1) == ADF_DOMAIN);        /* no occurrence, one binding */
    ADF_CHECK(FB("adf1 Q fball g 2 6 1", NULL, 0) == ADF_OK);
    ADF_CHECK(FB("adf1 Q fball g 2 6 1", b1, 0) == ADF_OK);
    /* a NULL binding */
    ADF_CHECK(FB("adf1 Q fball l 1 6 2 2 3 0 2", bn, 1) == ADF_DOMAIN);
    /* a binding with the same modulus and other blocks, or the blocks in another order */
    b1[0] = c6;
    ADF_CHECK(FB("adf1 Q fball l 1 6 2 2 3 0 2", b1, 1) == ADF_DOMAIN);
    b1[0] = c32;
    ADF_CHECK(FB("adf1 Q fball l 1 6 2 2 3 0 2", b1, 1) == ADF_DOMAIN);
    ADF_CHECK(FB("adf1 Q fball l 1 6 2 3 2 2 0", b1, 1) == ADF_OK);
    /* a binding that matches at another pointer: accepted; the value borrows that pointer */
    {
        adf_fball_t x, y;
        const adf_modctx_struct * bb[1];
        size_t len = strlen(text);
        adf_fball_init(x);
        adf_fball_init(y);
        bb[0] = c23;
        ADF_CHECK(adf_fball_load_str_binds(x, text, len, bb, 1, NULL) == ADF_OK && x->mctx == c23);
        bb[0] = c23b;
        ADF_CHECK(adf_fball_load_str_binds(y, text, len, bb, 1, NULL) == ADF_OK && y->mctx == c23b);
        ADF_CHECK(!local_identical(x, y));             /* identical() needs the original pointer */
        ADF_CHECK(fmpz_equal(x->d, y->d) && x->res[0] == y->res[0] && x->res[1] == y->res[1]);
        /* loading over a local value releases its residues (valgrind, ASan) */
        ADF_CHECK(adf_fball_load_str_binds(y, text, len, bb, 1, NULL) == ADF_OK);
        ADF_CHECK(adf_fball_load_str(y, "adf1 Q fball g 2 6 1", 20, NULL, NULL) == ADF_OK
                  && y->backend == ADF_GLOBAL && y->res == NULL && y->mctx == NULL);
        adf_fball_clear(x);
        adf_fball_clear(y);
    }
    /* the one-context form: any ctx for a dump without occurrences, NULL included */
    {
        adf_fball_t x;
        adf_fball_init(x);
        ADF_CHECK(adf_fball_load_str(x, "adf1 Q fball g 2 6 1", 20, c6, NULL) == ADF_OK);
        ADF_CHECK(adf_fball_load_str(x, "adf1 Q fball l 1 6 2 2 3 0 2", 28, NULL, NULL) == ADF_DOMAIN);
        ADF_CHECK(adf_fball_load_str(x, "adf1 Q fball l 1 6 2 2 3 0 2", 28, c6, NULL) == ADF_DOMAIN);
        ADF_CHECK(x->backend == ADF_GLOBAL && fmpz_equal_ui(x->A, 2));
        adf_fball_clear(x);
    }
    /* adele and cadele: the same rules */
    {
        adf_adele_t a;
        adf_cadele_t z;
        const adf_modctx_struct * bb[1] = {c23};
        adf_adele_init(a);
        adf_cadele_init(z);
        ADF_CHECK(adf_adele_load_str_binds(a, "adf1 Q adele 1 1 0 0 0 l 1 6 2 2 3 0 2", 38, bb, 1, NULL) == ADF_OK);
        ADF_CHECK(adf_adele_load_str_binds(a, "adf1 Q adele 1 1 0 0 0 l 1 6 2 2 3 0 2", 38, bb, 0, NULL)
                  == ADF_DOMAIN);
        ADF_CHECK(adf_adele_load_str(a, "adf1 Q adele 1 1 0 0 0 l 1 6 2 2 3 0 2", 38, c32, NULL) == ADF_DOMAIN);
        ADF_CHECK(adf_cadele_load_str(z, "adf1 Q cadele 1 1 0 0 0 1 0 0 0 l 1 6 2 2 3 0 2", 47, c23, NULL)
                  == ADF_OK);
        ADF_CHECK(adf_cadele_load_str_binds(z, "adf1 Q cadele 1 1 0 0 0 1 0 0 0 l 1 6 2 2 3 0 2", 47, NULL, 1,
                                            NULL) == ADF_DOMAIN);
        ADF_CHECK(local_is_L(&a->fin) && local_is_L(&z->fin));
        adf_adele_clear(a);
        adf_cadele_clear(z);
    }
    /* adf_rat: only nbinds = 0 matches (docs/api-m1.md, Choices 9) */
    {
        adf_rat_t r;
        const adf_modctx_struct * bb[1] = {c23};
        adf_rat_init(r);
        ADF_CHECK(adf_rat_load_str_binds(r, "adf1 Q rat 7 3", 14, bb, 1, NULL) == ADF_DOMAIN);
        ADF_CHECK(adf_rat_load_str_binds(r, "adf1 Q rat 7 3", 14, bb, 0, NULL) == ADF_OK);
        ADF_CHECK(adf_rat_load_str(r, "adf1 Q rat 7 3", 14, c23, NULL) == ADF_OK);
        adf_rat_clear(r);
    }
    /* scaled with binds */
    {
        adf_scaled_t x;
        const adf_modctx_struct * bb[2] = {c23, c23};
        sc_init(x, c6);
        ADF_CHECK(adf_scaled_load_str_binds(x, "adf1 Q scaled s 1 2 5 6 2 2 3", 29, bb, 2, NULL) == ADF_DOMAIN);
        ADF_CHECK(x->mctx == c6 && x->exact == 1);
        ADF_CHECK(adf_scaled_load_str_binds(x, "adf1 Q scaled s 1 2 5 6 2 2 3", 29, bb, 1, NULL) == ADF_OK);
        ADF_CHECK(x->mctx == c23 && x->exact == 0 && fmpz_equal_ui(x->u, 5) && sc_is_canonical(x));
        sc_clear(x);
    }
    adf_modctx_free(c23);
    adf_modctx_free(c32);
    adf_modctx_free(c6);
    adf_modctx_free(c23b);
}

/* ------------------------------------------------------------------ inspection (closure C2) */

ADF_TEST(inspection_capacity)
{
    const char * t = "adf1 Q fball l 1 6 2 2 3 0 2";
    size_t len = strlen(t), n;
    adf_ctx_desc_t d[3];
    ulong q23[2] = {2, 3};
    adf_modctx_struct * c = ctx_from(NULL, q23, 2);
    int i;

    for (i = 0; i < 3; i++)
        adf_ctx_desc_init(&d[i]);
    /* descriptors with old contents, to be released on OK */
    fmpz_set_ui(d[0].K, 99);
    d[0].k = 1;
    d[0].q = flint_malloc(sizeof(ulong));
    d[0].q[0] = 99;
    fmpz_set_ui(d[1].K, 98);
    /* descs = NULL: the incoming *nctx is ignored */
    n = 12345;
    ADF_CHECK(adf_fball_dump_inspect(&n, NULL, t, len, NULL) == ADF_OK && n == 1);
    /* too little capacity: LIMIT, *nctx and every descriptor untouched */
    n = 0;
    ADF_CHECK(adf_fball_dump_inspect(&n, d, t, len, NULL) == ADF_LIMIT && n == 0);
    ADF_CHECK(fmpz_equal_ui(d[0].K, 99) && d[0].k == 1 && d[0].q[0] == 99);
    /* an invalid dump with too little capacity: the validation comes first */
    n = 0;
    ADF_CHECK(adf_fball_dump_inspect(&n, d, "adf1 Q fball l 0 6 2 2 3 0 2", 28, NULL) == ADF_DOMAIN && n == 0);
    /* exact capacity: the first descriptor replaced, the count written */
    n = 1;
    ADF_CHECK(adf_fball_dump_inspect(&n, d, t, len, NULL) == ADF_OK && n == 1);
    ADF_CHECK(fmpz_equal_ui(d[0].K, 6) && d[0].k == 2 && d[0].q[0] == 2 && d[0].q[1] == 3);
    ADF_CHECK(adf_modctx_matches_desc(c, &d[0]));
    ADF_CHECK(fmpz_equal_ui(d[1].K, 98) && d[1].k == 0 && d[1].q == NULL);
    /* more capacity: the rest untouched */
    n = 3;
    ADF_CHECK(adf_adele_dump_inspect(&n, d, "adf1 Q adele 1 1 0 0 0 l 1 6 2 3 2 2 0", 38, NULL) == ADF_OK
              && n == 1);
    ADF_CHECK(fmpz_equal_ui(d[0].K, 6) && d[0].k == 2 && d[0].q[0] == 3 && d[0].q[1] == 2);
    ADF_CHECK(fmpz_equal_ui(d[1].K, 98) && d[1].q == NULL);
    /* scaled with k = 0: the descriptor has no block array */
    n = 1;
    ADF_CHECK(adf_scaled_dump_inspect(&n, d, "adf1 Q scaled x 1 2 1000000000000000000000 0", 44, NULL) == ADF_OK
              && n == 1);
    ADF_CHECK(d[0].k == 0 && d[0].q == NULL && fmpz_bits(d[0].K) == 85);
    /* a global body: 0 occurrences with capacity 0 */
    n = 0;
    ADF_CHECK(adf_cadele_dump_inspect(&n, d, "adf1 Q cadele 1 1 0 0 0 1 0 0 0 g 0 0 1", 39, NULL) == ADF_OK
              && n == 0);
    /* failures leave all outputs untouched */
    n = 3;
    ADF_CHECK(adf_scaled_dump_inspect(&n, d, "adf1 Q scaled x 1 2 1 0 ", 24, NULL) == ADF_PARSE && n == 3);
    ADF_CHECK(adf_scaled_dump_inspect(&n, NULL, "adf2 Q scaled x 1 2 1 0", 23, NULL) == ADF_UNSUPPORTED && n == 3);
    ADF_CHECK(d[0].k == 0 && fmpz_bits(d[0].K) == 85);
    for (i = 0; i < 3; i++)
        adf_ctx_desc_clear(&d[i]);
    adf_modctx_free(c);
}

/* ------------------------------------------------------------------ adf_modctx_new_from_dump */

/* The context at occurrence occ of the text, compared with (K, q, k); ADF_OK expected. */
static void
from_dump_is(const char * t, size_t occ, ulong K, const ulong * q, slong k)
{
    adf_modctx_struct * c = NULL;
    int st = adf_modctx_new_from_dump(&c, t, strlen(t), occ, NULL);
    slong i;

    ADF_CHECK_MSG(st == ADF_OK, "\"%s\" occurrence %zu: %s", t, occ, adf_status_str(st));
    if (st != ADF_OK)
        return;
    {
        fmpz_t Kc;
        fmpz_init(Kc);
        adf_modctx_get_modulus(Kc, c);
        ADF_CHECK(fmpz_equal_ui(Kc, K) && adf_modctx_nblocks(c) == k);
        fmpz_clear(Kc);
    }
    for (i = 0; i < k && i < adf_modctx_nblocks(c); i++)
        ADF_CHECK(adf_modctx_block(c, i) == q[i]);
    adf_modctx_free(c);
}

static int
from_dump_status(const char * t, size_t len, size_t occ, const adf_text_limits_t * lim)
{
    adf_modctx_struct * sentinel = (adf_modctx_struct *) (void *) 0x20, * c = sentinel;
    int st = adf_modctx_new_from_dump(&c, t, len, occ, lim);

    if (st != ADF_OK)
        ADF_CHECK(c == sentinel);
    else
        adf_modctx_free(c);
    return st;
}

#define FD(lit, occ) from_dump_status((lit), sizeof(lit) - 1, (occ), NULL)
#define FDL(lit, occ, lim) from_dump_status((lit), sizeof(lit) - 1, (occ), (lim))

ADF_TEST(new_from_dump_every_body)
{
    ulong q23[2] = {2, 3}, q32[2] = {3, 2}, q2 = 2, q3 = 3, q6 = 6;
    adf_text_limits_t lim;

    /* bodies with an occurrence */
    from_dump_is("adf1 Q modctx 6 2 2 3", 0, 6, q23, 2);
    from_dump_is("adf1 Q fball l 1 6 2 2 3 0 2", 0, 6, q23, 2);
    from_dump_is("adf1 Q fball l 1 6 2 3 2 2 0", 0, 6, q32, 2);
    from_dump_is("adf1 Q scaled s 1 2 5 6 2 2 3", 0, 6, q23, 2);
    from_dump_is("adf1 Q scaled x 1 2 6 0", 0, 6, NULL, 0);
    from_dump_is("adf1 Q adele 1 1 0 0 0 l 1 6 1 6 5", 0, 6, &q6, 1);
    from_dump_is("adf1 Q cadele 1 1 0 0 0 1 0 0 0 l 1 6 1 6 5", 0, 6, &q6, 1);
    from_dump_is("adf1 Q qclass lift 1 1 -1 0 0 l 1 6 2 2 3 0 2", 0, 6, q23, 2);
    /* the witness of gate finding G3: two pieces, two contexts */
    from_dump_is("adf1 Q qclass pieces 2 1 1 -2 0 0 l 1 2 1 2 0 1 3 -2 0 0 l 1 3 1 3 0", 0, 2, &q2, 1);
    from_dump_is("adf1 Q qclass pieces 2 1 1 -2 0 0 l 1 2 1 2 0 1 3 -2 0 0 l 1 3 1 3 0", 1, 3, &q3, 1);
    ADF_CHECK(FD("adf1 Q qclass pieces 2 1 1 -2 0 0 l 1 2 1 2 0 1 3 -2 0 0 l 1 3 1 3 0", 2) == ADF_DOMAIN);
    /* a global piece between two local ones: occurrences count local balls only */
    from_dump_is("adf1 Q qclass pieces 3 1 1 -3 0 0 l 1 2 1 2 0 1 1 -2 0 0 g 0 1 1 1 3 -2 0 0 l 1 3 1 3 0", 1,
                 3, &q3, 1);
    ADF_CHECK(FD("adf1 Q modctx 6 2 2 3", 1) == ADF_DOMAIN);
    ADF_CHECK(FD("adf1 Q fball l 1 6 2 2 3 0 2", 1) == ADF_DOMAIN);
    ADF_CHECK(FD("adf1 Q fball l 1 6 2 2 3 0 2", (size_t) -1) == ADF_DOMAIN);
    /* bodies without an occurrence: valid or not at stage 6, DOMAIN */
    ADF_CHECK(FD("adf1 Q fball g 1 6 1", 0) == ADF_DOMAIN);
    ADF_CHECK(FD("adf1 Q rat 7 3", 0) == ADF_DOMAIN);
    ADF_CHECK(FD("adf1 Q rat 2 4", 0) == ADF_DOMAIN);
    ADF_CHECK(FD("adf1 Q ucoset 5 6", 0) == ADF_DOMAIN);
    ADF_CHECK(FD("adf1 Q ucoset 0 1", 0) == ADF_DOMAIN);
    ADF_CHECK(FD("adf1 Q idele 1 5 -1 1 -1e 3 2 5 24", 0) == ADF_DOMAIN);
    ADF_CHECK(FD("adf1 Q idclass 5 -2 0 0 5 24", 0) == ADF_DOMAIN);
    ADF_CHECK(FD("adf1 Q lball 5 b 3 0 4", 0) == ADF_DOMAIN);
    ADF_CHECK(FD("adf1 Q sball r 3 -1 0 0 2 3 x 1 1 0 5 b 3 0 4", 0) == ADF_DOMAIN);
    ADF_CHECK(FD("adf1 Q ffun 1 1 0 0 0 0 0 0 0 0", 0) == ADF_DOMAIN);
    ADF_CHECK(FD("adf1 Q rfun 0", 0) == ADF_DOMAIN);
    ADF_CHECK(FD("adf1 Q char 5 2 0 0 0 0 0 0 0 0", 0) == ADF_DOMAIN);
    /* and their earlier stages */
    ADF_CHECK(FD("adf1 Q lball 5 b 3 0 186a1", 0) == ADF_LIMIT);
    ADF_CHECK(FD("adf1 Q lball 5 x 1 3 -186a1", 0) == ADF_LIMIT);
    ADF_CHECK(FD("adf1 Q lball 10000000000000000 b 1 0 1", 0) == ADF_UNSUPPORTED);
    ADF_CHECK(FD("adf1 Q lball -10000000000000000 b 1 0 1", 0) == ADF_DOMAIN);
    ADF_CHECK(FD("adf1 Q lball 5 y 3 0 4", 0) == ADF_PARSE);
    ADF_CHECK(FD("adf1 Q char 10000000000000000 1 0 0 0 0 0 0 0 0", 0) == ADF_UNSUPPORTED);
    ADF_CHECK(FD("adf1 Q sball r 1 0 0 0 1 5 b 3 0 4 5 b 3 0 4", 0) == ADF_PARSE);
    ADF_CHECK(FD("adf1 Q ffun 1 2 1 0 0 0 0 0 0 0", 0) == ADF_PARSE);
    ADF_CHECK(FD("adf1 Q ffun -1 1", 0) == ADF_PARSE);
    ADF_CHECK(FD("adf1 Q ffun 0 1", 0) == ADF_DOMAIN);
    ADF_CHECK(FD("adf1 Q rfun 1 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0", 0) == ADF_PARSE);
    ADF_CHECK(FD("adf1 Q ucoset 5", 0) == ADF_PARSE);
    ADF_CHECK(FD("adf1 Q nothing 5 6", 0) == ADF_PARSE);
    /* the stages of qclass */
    ADF_CHECK(FD("adf1 Q qclass pieces 0", 0) == ADF_DOMAIN);
    ADF_CHECK(FD("adf1 Q qclass pieces 2 1 f -4 1 -4 g 0 2 1 1 1 -4 1 -4 g 1 2 1", 0) == ADF_DOMAIN);
    ADF_CHECK(FD("adf1 Q qclass pieces 1 1 3 -1 0 0 g 0 1 1", 0) == ADF_DOMAIN);
    ADF_CHECK(FD("adf1 Q qclass pieces 1 1 1 -1 0 0 g 1 2 2", 0) == ADF_DOMAIN);
    ADF_CHECK(FD("adf1 Q qclass pieces 1 1 1 -1 0 0 l 2 6 2 2 3 1 1", 0) == ADF_DOMAIN);
    from_dump_is("adf1 Q qclass pieces 1 1 1 -1 0 0 l 2 6 2 2 3 0 0", 0, 6, q23, 2);
    ADF_CHECK(FD("adf1 Q qclass pieces 1 1 1 -100001 0 0 g 0 1 1", 0) == ADF_LIMIT);
    ADF_CHECK(FD("adf1 Q qclass pieces 1 1 1 -100000 0 0 g 0 1 1", 0) == ADF_DOMAIN);   /* d = 1, mid ok? */
    ADF_CHECK(FD("adf1 Q qclass lift 2 1 0 0 0 1 0 0 0 g 0 0 1", 0) == ADF_DOMAIN);
    ADF_CHECK(FD("adf1 Q qclass lift 1 2 0 0 0 g 0 0 1", 0) == ADF_DOMAIN);
    ADF_CHECK(FD("adf1 Q qclass lift 1 1 0 0 0 g 0 0 1 1", 0) == ADF_PARSE);
    /* limits: the block count of every occurrence before any stage-6 check */
    adf_text_limits_default(&lim);
    lim.max_items = 1;
    ADF_CHECK(FDL("adf1 Q qclass pieces 1 1 3 -1 0 0 l 1 6 2 2 3 0 2", 0, &lim) == ADF_LIMIT);
    /* two pieces > max_items = 1 */
    ADF_CHECK(FDL("adf1 Q qclass pieces 2 1 1 -2 0 0 l 1 2 1 2 0 1 3 -2 0 0 l 1 3 1 3 0", 1, &lim) == ADF_LIMIT);
    lim.max_items = 0;
    ADF_CHECK(FDL("adf1 Q qclass lift 1 1 0 0 0 g 0 0 1", 0, &lim) == ADF_LIMIT);
    ADF_CHECK(FDL("adf1 Q modctx 6 0", 0, &lim) == ADF_OK);
    ADF_CHECK(FDL("adf1 Q sball n 1 5 b 3 0 4", 0, &lim) == ADF_LIMIT);
    ADF_CHECK(FDL("adf1 Q ffun 1 1 0 0 0 0 0 0 0 0", 0, &lim) == ADF_LIMIT);
    ADF_CHECK(FDL("adf1 Q rfun 0", 0, &lim) == ADF_DOMAIN);
    lim.max_prec = 3;
    lim.max_items = 5;
    ADF_CHECK(FDL("adf1 Q lball 5 b 3 0 4", 0, &lim) == ADF_LIMIT);
    ADF_CHECK(FDL("adf1 Q lball 5 b 3 -4 3", 0, &lim) == ADF_LIMIT);
    ADF_CHECK(FDL("adf1 Q lball 5 b 3 0 3", 0, &lim) == ADF_DOMAIN);
    /* the header, and the out pointer */
    ADF_CHECK(FD("adf2 Q qclass lift 1 1 0 0 0 g 0 0 1", 0) == ADF_UNSUPPORTED);
    ADF_CHECK(FD("adf1 Q", 0) == ADF_PARSE);
    ADF_CHECK(adf_modctx_new_from_dump(NULL, "adf1 Q modctx 6 1 6", 19, 0, NULL) == ADF_DOMAIN);
}

/* ------------------------------------------------------------------ the reference vectors */

static int
status_of_name(const char * name)
{
    int s;
    for (s = 0; s < ADF_STATUS_COUNT; s++)
        if (strcmp(adf_status_str(s), name) == 0)
            return s;
    return -1;
}

static int
json_size(size_t * v, const jsonl_value * j, jsonl_error_t * err)
{
    fmpz_t t;
    const char * text = jsonl_int_text(j, err);
    int ok;

    if (text == NULL)
        return 0;
    fmpz_init(t);
    ok = fmpz_set_str(t, text, 10) == 0 && fmpz_sgn(t) >= 0;
    if (ok)
        *v = (size_t) fmpz_get_ui(t);
    fmpz_clear(t);
    return ok;
}

/* The body keyword of a text: its third space-separated token. */
static int
body_is(const char * t, size_t len, const char * kw)
{
    size_t i = 0, sp = 0, n = strlen(kw);

    while (i < len && sp < 2)
        if (t[i++] == ' ')
            sp++;
    return sp == 2 && i + n <= len && memcmp(t + i, kw, n) == 0 && (i + n == len || t[i + n] == ' ');
}

typedef struct
{
    fmpz_t K;
    slong k;
    ulong * q;
} ref_ctx;

/* The typed checks of one record: inspection against the reference occurrences, then load with
   one binding per occurrence (contexts made from the reference), dump, the same bytes; or the
   reference status with the output untouched. */
static void
typed_check(const char * t, size_t len, const adf_text_limits_t * lim, int status, ref_ctx * occ, size_t nocc)
{
    adf_modctx_struct * ctxs[4] = {NULL, NULL, NULL, NULL};
    const adf_modctx_struct * binds[4];
    adf_ctx_desc_t d[4];
    size_t i, n, dl;
    int st;
    char * again = NULL;

    ADF_CHECK(nocc <= 1);
    for (i = 0; i < 4; i++)
        adf_ctx_desc_init(&d[i]);
    for (i = 0; i < nocc; i++)
    {
        ctxs[i] = ctx_from(occ[i].K, occ[i].q, occ[i].k);
        binds[i] = ctxs[i];
    }
    n = 4;
    if (body_is(t, len, "rat"))
    {
        adf_rat_t x;
        adf_rat_init(x);
        st = adf_rat_dump_inspect(&n, d, t, len, lim);
        ADF_CHECK(adf_rat_load_str_binds(x, t, len, binds, nocc, lim) == st);
        if (st == ADF_OK)
            again = adf_rat_dump_str(&dl, x);
        else
            ADF_CHECK(fmpz_is_zero(fmpq_numref(x->q)) && fmpz_is_one(fmpq_denref(x->q)));
        adf_rat_clear(x);
    }
    else if (body_is(t, len, "fball"))
    {
        adf_fball_t x;
        adf_fball_init(x);
        st = adf_fball_dump_inspect(&n, d, t, len, lim);
        ADF_CHECK(adf_fball_load_str_binds(x, t, len, binds, nocc, lim) == st);
        if (st == ADF_OK)
        {
            ADF_CHECK(x->backend == ADF_GLOBAL ? adf_fball_is_canonical(x) : local_is_L(x));
            again = adf_fball_dump_str(&dl, x);
        }
        else
            ADF_CHECK(x->backend == ADF_GLOBAL && fmpz_is_zero(x->A) && fmpz_is_one(x->d));
        adf_fball_clear(x);
    }
    else if (body_is(t, len, "scaled"))
    {
        adf_scaled_t x;
        sc_init(x, (const adf_modctx_struct *) (void *) 0x30);
        st = adf_scaled_dump_inspect(&n, d, t, len, lim);
        ADF_CHECK(adf_scaled_load_str_binds(x, t, len, binds, nocc, lim) == st);
        if (st == ADF_OK)
        {
            ADF_CHECK(sc_is_canonical(x) && x->mctx == ctxs[0]);
            again = adf_scaled_dump_str(&dl, x);
        }
        else
            ADF_CHECK(x->mctx == (const adf_modctx_struct *) (void *) 0x30 && x->exact == 1);
        sc_clear(x);
    }
    else if (body_is(t, len, "adele"))
    {
        adf_adele_t x;
        adf_adele_init(x);
        st = adf_adele_dump_inspect(&n, d, t, len, lim);
        ADF_CHECK(adf_adele_load_str_binds(x, t, len, binds, nocc, lim) == st);
        if (st == ADF_OK)
        {
            ADF_CHECK(arb_is_finite(x->inf));
            again = adf_adele_dump_str(&dl, x);
        }
        else
            ADF_CHECK(arb_is_zero(x->inf) && x->fin.backend == ADF_GLOBAL);
        adf_adele_clear(x);
    }
    else
    {
        adf_cadele_t x;
        adf_cadele_init(x);
        st = adf_cadele_dump_inspect(&n, d, t, len, lim);
        ADF_CHECK(adf_cadele_load_str_binds(x, t, len, binds, nocc, lim) == st);
        if (st == ADF_OK)
            again = adf_cadele_dump_str(&dl, x);
        else
            ADF_CHECK(acb_is_zero(x->inf) && x->fin.backend == ADF_GLOBAL);
        adf_cadele_clear(x);
    }
    ADF_CHECK_MSG(st == status, "\"%s\": status %s, reference %s", t, adf_status_str(st), adf_status_str(status));
    if (st == ADF_OK)
    {
        ADF_CHECK(n == nocc);
        for (i = 0; i < nocc && i < n; i++)
            ADF_CHECK(adf_modctx_matches_desc(ctxs[i], &d[i]));
        ADF_CHECK_MSG(again != NULL && dl == len && memcmp(again, t, len) == 0, "\"%s\" dumped again as \"%s\"",
                      t, again);
    }
    else
        ADF_CHECK(n == 4);
    adf_str_free(again);
    for (i = 0; i < 4; i++)
        adf_ctx_desc_clear(&d[i]);
    for (i = 0; i < nocc; i++)
        adf_modctx_free(ctxs[i]);
}

ADF_TEST(reference_vectors)
{
    jsonl_error_t err;
    jsonl_file * f = NULL;
    size_t i, n_typed = 0, n_occ = 0;

    ADF_CHECK_MSG(jsonl_open("tests/ref/vectors/m1-dump/dump_ref.jsonl", &f, &err) == 1, "%s",
                  jsonl_error_message(&err));
    if (f == NULL)
        return;
    for (i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value * rec = jsonl_record(f, i), * jt, * jr, * jc, * jmi, * jml;
        const char * t, * rt;
        size_t len, rlen, mi = 0, ml = 0, nocc = 0, j;
        adf_text_limits_t lim;
        int status;
        ref_ctx occ[8];

        ADF_CHECK(jsonl_field(rec, "text", &jt, &err) && jsonl_field(rec, "roundtrip", &jr, &err)
                  && jsonl_field(rec, "contexts", &jc, &err) && jsonl_field(rec, "max_items", &jmi, &err)
                  && jsonl_field(rec, "max_len", &jml, &err));
        t = jsonl_string(jt, &len, &err);
        rt = jsonl_string(jr, &rlen, &err);
        ADF_CHECK(t != NULL && rt != NULL && json_size(&mi, jmi, &err) && json_size(&ml, jml, &err));
        if (t == NULL || rt == NULL)
            continue;
        adf_text_limits_default(&lim);
        lim.max_items = (slong) mi;
        lim.max_len = ml;
        status = rt[0] == '!' ? status_of_name(rt + 1) : ADF_OK;
        ADF_CHECK(status >= 0);
        if (status == ADF_OK)
            ADF_CHECK(rlen == len && memcmp(rt, t, len) == 0);
        /* the context occurrences, or the status */
        if (jsonl_is(jc, JSONL_STR))
        {
            const char * cs = jsonl_string(jc, NULL, &err);
            int cst = status_of_name(cs + 1);
            ADF_CHECK_MSG(from_dump_status(t, len, 0, &lim) == cst, "\"%s\": from_dump", t);
        }
        else
        {
            nocc = jsonl_size(jc);
            ADF_CHECK(nocc <= 8);
            for (j = 0; j < nocc && j < 8; j++)
            {
                const jsonl_value * e = jsonl_at(jc, j, &err);
                const jsonl_value * jK = jsonl_at(e, 0, &err), * jq = jsonl_at(e, 1, &err);
                size_t m;
                fmpz_init(occ[j].K);
                ADF_CHECK(fmpz_set_str(occ[j].K, jsonl_int_text(jK, &err), 10) == 0);
                occ[j].k = (slong) jsonl_size(jq);
                occ[j].q = occ[j].k ? flint_malloc((size_t) occ[j].k * sizeof(ulong)) : NULL;
                for (m = 0; m < (size_t) occ[j].k; m++)
                {
                    fmpz_t v;
                    fmpz_init(v);
                    ADF_CHECK(fmpz_set_str(v, jsonl_int_text(jsonl_at(jq, m, &err), &err), 10) == 0);
                    occ[j].q[m] = fmpz_get_ui(v);
                    fmpz_clear(v);
                }
                /* adf_modctx_new_from_dump at this occurrence */
                {
                    adf_modctx_struct * c = NULL;
                    adf_ctx_desc_t d;
                    ADF_CHECK_MSG(adf_modctx_new_from_dump(&c, t, len, j, &lim) == ADF_OK, "\"%s\" occ %zu", t, j);
                    adf_ctx_desc_init(&d);
                    fmpz_set(d.K, occ[j].K);
                    d.k = occ[j].k;
                    d.q = occ[j].q;
                    ADF_CHECK(c != NULL && adf_modctx_matches_desc(c, &d));
                    d.q = NULL;
                    adf_ctx_desc_clear(&d);
                    adf_modctx_free(c);
                }
                n_occ++;
            }
            ADF_CHECK(from_dump_status(t, len, nocc, &lim) == ADF_DOMAIN);
        }
        if (body_is(t, len, "rat") || body_is(t, len, "fball") || body_is(t, len, "scaled")
            || body_is(t, len, "adele") || body_is(t, len, "cadele"))
        {
            typed_check(t, len, &lim, status, occ, status == ADF_OK ? nocc : 0);
            n_typed++;
        }
        for (j = 0; j < nocc && j < 8; j++)
        {
            fmpz_clear(occ[j].K);
            flint_free(occ[j].q);
        }
    }
    ADF_CHECK(jsonl_count(f) == 2180);
    ADF_CHECK(n_typed > 1000 && n_occ > 500);
    jsonl_close(f);
}

/* The from_dump records of tests/ref/vectors/m1-modctx/modctx.jsonl (lane m1-modctx-b), run
   completely. One record, "adf1 Q fball g 1 6 1" with status PARSE, was written when the
   function handled the body modctx only (lanes/m1-modctx-b/report.md, finding 2). The reference
   proto/text_grammar.py modctx_new_from_dump (lines 1510-1521) gives DOMAIN for it: a valid
   dump without a context occurrence. That record is checked against the reference; the
   disagreement is a finding of this lane (lanes/m1-dump/report.md). */
ADF_TEST(modctx_vectors_from_dump)
{
    jsonl_error_t err;
    jsonl_file * f = NULL;
    size_t i, n = 0;

    ADF_CHECK_MSG(jsonl_open("tests/ref/vectors/m1-modctx/modctx.jsonl", &f, &err) == 1, "%s",
                  jsonl_error_message(&err));
    if (f == NULL)
        return;
    for (i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value * rec = jsonl_record(f, i), * jop, * jt, * jo, * jmi, * jml, * js;
        const char * op, * t;
        size_t oplen, len, occ = 0, mi = 0, ml = 0;
        adf_text_limits_t lim;

        ADF_CHECK(jsonl_field(rec, "op", &jop, &err));
        op = jsonl_string(jop, &oplen, &err);
        if (op == NULL || strcmp(op, "from_dump") != 0)
            continue;
        ADF_CHECK(jsonl_field(rec, "text", &jt, &err) && jsonl_field(rec, "occurrence", &jo, &err)
                  && jsonl_field(rec, "max_items", &jmi, &err) && jsonl_field(rec, "max_len", &jml, &err));
        t = jsonl_string(jt, &len, &err);
        ADF_CHECK(json_size(&occ, jo, &err) && json_size(&mi, jmi, &err) && json_size(&ml, jml, &err));
        adf_text_limits_default(&lim);
        lim.max_items = (slong) mi;
        lim.max_len = ml;
        if (jsonl_field(rec, "status", &js, &err))
        {
            const char * sn = jsonl_string(js, NULL, &err);
            int want = status_of_name(sn);
            if (len == 20 && memcmp(t, "adf1 Q fball g 1 6 1", 20) == 0)
                want = ADF_DOMAIN;     /* the reference; see the comment above */
            ADF_CHECK_MSG(from_dump_status(t, len, occ, &lim) == want, "\"%s\"", t);
        }
        else
        {
            adf_modctx_struct * c = NULL;
            ADF_CHECK_MSG(adf_modctx_new_from_dump(&c, t, len, occ, &lim) == ADF_OK, "\"%s\"", t);
            if (c != NULL)
            {
                size_t dl;
                char * d = adf_modctx_dump_str(&dl, c);
                ADF_CHECK(dl == len && memcmp(d, t, len) == 0);   /* every OK record is a modctx dump */
                adf_str_free(d);
                adf_modctx_free(c);
            }
        }
        n++;
    }
    ADF_CHECK(n == 29);
    jsonl_close(f);
}

/* ------------------------------------------------------------------ hostile input with contexts */

ADF_TEST(local_dump_at_the_end_of_a_page)
{
    static const char * texts[] = {
        "adf1 Q fball l 1 6 2 2 3 0 2", "adf1 Q scaled s 1 2 5 6 2 2 3",
        "adf1 Q qclass pieces 2 1 1 -2 0 0 l 1 2 1 2 0 1 3 -2 0 0 l 1 3 1 3 0",
        "adf1 Q adele 1 1 0 0 0 l 1 6 2 2 3 0 2", "adf1 Q ffun 1 1 0 0 0 0 0 0 0 0",
        "adf1 Q rfun 1 1 1 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0",
        "adf1 Q sball c 1 0 0 0 0 0 0 0 1 5 b 3 0 4", "adf1 Q lball 5 x 1 3 0", "adf1 Q char 5 2 0 0 0 0 0 0 0 0"};
    ulong q23[2] = {2, 3};
    adf_modctx_struct * c = ctx_from(NULL, q23, 2);
    long pg = sysconf(_SC_PAGESIZE);
    char * base;
    size_t i, j;

    ADF_CHECK(pg > 0);
    base = mmap(NULL, (size_t) pg * 2, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    ADF_CHECK(base != MAP_FAILED);
    if (pg <= 0 || base == MAP_FAILED)
        return;
    ADF_CHECK(mprotect(base + pg, (size_t) pg, PROT_NONE) == 0);
    for (i = 0; i < sizeof(texts) / sizeof(texts[0]); i++)
    {
        size_t n = strlen(texts[i]);
        for (j = 0; j <= n; j++)
        {
            char * p = base + pg - j;
            char * h = flint_malloc(j > 0 ? j : 1);           /* a heap block of exact size */
            adf_fball_t x;
            adf_scaled_t y;
            size_t k;
            memcpy(p, texts[i], j);
            memcpy(h, texts[i], j);
            adf_fball_init(x);
            sc_init(y, c);
            (void) adf_fball_load_str(x, p, j, c, NULL);
            (void) adf_scaled_load_str(y, p, j, c, NULL);
            (void) adf_fball_load_str(x, h, j, c, NULL);
            k = 2;
            (void) adf_fball_dump_inspect(&k, NULL, p, j, NULL);
            (void) from_dump_status(p, j, 0, NULL);
            (void) from_dump_status(p, j, 1, NULL);
            (void) from_dump_status(h, j, 0, NULL);
            if (j == n && i == 0)
                ADF_CHECK(adf_fball_load_str(x, p, j, c, NULL) == ADF_OK && local_is_L(x));
            adf_fball_clear(x);
            sc_clear(y);
            flint_free(h);
        }
    }
    munmap(base, (size_t) pg * 2);
    adf_modctx_free(c);
}

/* ------------------------------------------------------------------ edges of the stages */

/* The boundaries of stage 4 and stage 5 on the bodies that only adf_modctx_new_from_dump reads,
   the count that wraps a word, the order keys of qclass pieces (conventions 5.10, CV-24: by the
   lower end, then the upper end, then H, then A; proto lines 1358-1368), and the inspector
   without *nctx. A trailing local piece gives the pieces dumps one occurrence, so that a valid
   text (OK at occurrence 0) and an invalid one (DOMAIN) are told apart. */
ADF_TEST(stage_edges_and_piece_order)
{
    static const ulong two[1] = {2};
    adf_text_limits_t lim;
    size_t n;

    adf_text_limits_default(&lim);
    /* max_prec: |v|, |N| <= max_prec passes stage 4, one more does not; a negative limit fails
       every ball */
    lim.max_prec = 0;
    ADF_CHECK(FDL("adf1 Q lball 5 b 0 0 0", 0, &lim) == ADF_DOMAIN);
    ADF_CHECK(FDL("adf1 Q lball 5 b 0 0 1", 0, &lim) == ADF_LIMIT);
    ADF_CHECK(FDL("adf1 Q lball 5 x 1 1 0", 0, &lim) == ADF_DOMAIN);
    ADF_CHECK(FDL("adf1 Q lball 5 x 1 1 -1", 0, &lim) == ADF_LIMIT);
    ADF_CHECK(FDL("adf1 Q lball 5 b 1 -1 0", 0, &lim) == ADF_LIMIT);
    ADF_CHECK(FDL("adf1 Q lball 5 b 0 0 10000000000000000", 0, &lim) == ADF_LIMIT);
    lim.max_prec = -1;
    ADF_CHECK(FDL("adf1 Q lball 5 b 0 0 0", 0, &lim) == ADF_LIMIT);
    adf_text_limits_default(&lim);
    lim.max_prec = WORD_MAX;
    ADF_CHECK(FDL("adf1 Q lball 5 b 0 0 7fffffffffffffff", 0, &lim) == ADF_DOMAIN);
    ADF_CHECK(FDL("adf1 Q lball 5 b 0 0 8000000000000000", 0, &lim) == ADF_LIMIT);
    /* the word restriction: 2^64 - 1 is a word, 2^64 is not */
    ADF_CHECK(FD("adf1 Q lball ffffffffffffffff b 1 0 1", 0) == ADF_DOMAIN);
    ADF_CHECK(FD("adf1 Q lball 10000000000000000 x 1 1 0", 0) == ADF_UNSUPPORTED);
    /* (the reference gives UNSUPPORTED here: its search for the primitive character refuses moduli
       above 10^5, a limit of the reference, proto/text_grammar.py lines 18-19; conventions 8.5
       item 5 makes only q >= 2^64 UNSUPPORTED, and a char body has no occurrence) */
    ADF_CHECK(FD("adf1 Q char ffffffffffffffff 1 0 0 0 0 0 0 0 0", 0) == ADF_DOMAIN);
    ADF_CHECK(FD("adf1 Q sball n 1 10000000000000000 b 1 0 1", 0) == ADF_UNSUPPORTED);
    /* a count of 2^64 + 1 does not wrap to 1 */
    ADF_CHECK(FD("adf1 Q adele 10000000000000001 1 0 0 0 g 0 0 1", 0) == ADF_PARSE);
    ADF_CHECK(FD("adf1 Q modctx 6 10000000000000001 6", 0) == ADF_PARSE);
    ADF_CHECK(FD("adf1 Q qclass pieces 10000000000000001 1 1 -1 0 0 g 0 1 1", 0) == ADF_PARSE);
    {
        adf_adele_t a;
        adf_adele_init(a);
        ADF_CHECK(adf_adele_load_str(a, "adf1 Q adele 10000000000000001 1 0 0 0 g 0 0 1", 46, NULL, NULL)
                  == ADF_PARSE);
        adf_adele_clear(a);
    }
    /* ffun: 8 D M against the tokens left, D M against max_items */
    ADF_CHECK(FD("adf1 Q ffun 1 1 0 0 0 0 0 0 0", 0) == ADF_PARSE);
    ADF_CHECK(FD("adf1 Q ffun 10000000000000000 0", 0) == ADF_DOMAIN);
    adf_text_limits_default(&lim);
    lim.max_items = 1;
    ADF_CHECK(FDL("adf1 Q ffun 1 1 0 0 0 0 0 0 0 0", 0, &lim) == ADF_DOMAIN);
    ADF_CHECK(FDL("adf1 Q rfun 1 1 1 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0", 0, &lim)
              == ADF_DOMAIN);
    ADF_CHECK(FDL("adf1 Q rfun 1 2 1 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 "
                  "0 0", 0, &lim) == ADF_LIMIT);
    ADF_CHECK(FDL("adf1 Q sball n 1 5 b 3 0 4", 0, &lim) == ADF_DOMAIN);
    ADF_CHECK(FDL("adf1 Q sball n 2 5 b 3 0 4 7 b 3 0 4", 0, &lim) == ADF_LIMIT);
    lim.max_items = 0;
    ADF_CHECK(FDL("adf1 Q rfun 1 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0", 0, &lim) == ADF_LIMIT);
    /* qclass: the midpoint at the ends 0 and 1 is inside [0, 1] */
    from_dump_is("adf1 Q qclass pieces 1 1 0 0 0 0 l 1 2 1 2 0", 0, 2, two, 1);
    from_dump_is("adf1 Q qclass pieces 1 1 1 0 0 0 l 1 2 1 2 0", 0, 2, two, 1);
    ADF_CHECK(FD("adf1 Q qclass pieces 1 1 -1 -1 0 0 l 1 2 1 2 0", 0) == ADF_DOMAIN);
    ADF_CHECK(FD("adf1 Q qclass pieces 1 1 3 -1 0 0 l 1 2 1 2 0", 0) == ADF_DOMAIN);
    /* two equal pieces */
    ADF_CHECK(FD("adf1 Q qclass pieces 2 1 1 -1 0 0 l 1 2 1 2 0 1 1 -1 0 0 l 1 2 1 2 0", 0) == ADF_DOMAIN);
    /* the lower end decides first: D1 = 1/2 +/- 1/2 before D2 = 1/4 */
    ADF_CHECK(FD("adf1 Q qclass pieces 3 1 1 -1 1 -1 g 0 1 1 1 1 -2 0 0 g 0 1 1 1 1 0 0 0 l 1 2 1 2 0", 0)
              == ADF_OK);
    ADF_CHECK(FD("adf1 Q qclass pieces 3 1 1 -2 0 0 g 0 1 1 1 1 -1 1 -1 g 0 1 1 1 1 0 0 0 l 1 2 1 2 0", 0)
              == ADF_DOMAIN);
    /* equal lower ends: the upper end */
    ADF_CHECK(FD("adf1 Q qclass pieces 3 1 1 -1 0 0 g 0 1 1 1 3 -2 1 -2 g 0 1 1 1 1 0 0 0 l 1 2 1 2 0", 0)
              == ADF_OK);
    ADF_CHECK(FD("adf1 Q qclass pieces 3 1 3 -2 1 -2 g 0 1 1 1 1 -1 0 0 g 0 1 1 1 1 0 0 0 l 1 2 1 2 0", 0)
              == ADF_DOMAIN);
    /* equal real parts: H */
    ADF_CHECK(FD("adf1 Q qclass pieces 3 1 1 -1 0 0 g 0 1 1 1 1 -1 0 0 g 1 2 1 1 1 0 0 0 l 1 2 1 2 0", 0)
              == ADF_OK);
    ADF_CHECK(FD("adf1 Q qclass pieces 3 1 1 -1 0 0 g 1 2 1 1 1 -1 0 0 g 0 1 1 1 1 0 0 0 l 1 2 1 2 0", 0)
              == ADF_DOMAIN);
    /* equal real parts and H: A */
    ADF_CHECK(FD("adf1 Q qclass pieces 3 1 1 -1 0 0 g 1 3 1 1 1 -1 0 0 g 2 3 1 1 1 0 0 0 l 1 2 1 2 0", 0)
              == ADF_OK);
    ADF_CHECK(FD("adf1 Q qclass pieces 3 1 1 -1 0 0 g 2 3 1 1 1 -1 0 0 g 1 3 1 1 1 0 0 0 l 1 2 1 2 0", 0)
              == ADF_DOMAIN);
    /* the key of a local piece is its canonical triple: (0 + 6 Zhat)/2 is (0, 3, 1), H = 3 */
    ADF_CHECK(FD("adf1 Q qclass pieces 2 1 1 -1 0 0 g 1 2 1 1 1 -1 0 0 l 2 6 2 2 3 0 0", 0) == ADF_OK);
    ADF_CHECK(FD("adf1 Q qclass pieces 2 1 1 -1 0 0 l 2 6 2 2 3 0 0 1 1 -1 0 0 g 1 2 1", 0) == ADF_DOMAIN);
    /* (2 + 6 Zhat)/2 = (1 + 3 Zhat): the residues (2 mod 2, 2 mod 3) = (0, 2), key (1/2, 1/2, 3, 1) */
    ADF_CHECK(FD("adf1 Q qclass pieces 2 1 1 -1 0 0 g 0 3 1 1 1 -1 0 0 l 2 6 2 2 3 0 2", 0) == ADF_OK);
    ADF_CHECK(FD("adf1 Q qclass pieces 2 1 1 -1 0 0 g 2 3 1 1 1 -1 0 0 l 2 6 2 2 3 0 2", 0) == ADF_DOMAIN);
    /* the inspectors need *nctx */
    ADF_CHECK(adf_rat_dump_inspect(NULL, NULL, "adf1 Q rat 7 3", 14, NULL) == ADF_DOMAIN);
    ADF_CHECK(adf_scaled_dump_inspect(NULL, NULL, "adf1 Q scaled x 1 2 1 0", 23, NULL) == ADF_DOMAIN);
    n = 3;
    ADF_CHECK(adf_scaled_dump_inspect(&n, NULL, "adf1 Q scaled x 1 2 1 0", 23, NULL) == ADF_OK && n == 1);
    /* the grammar comes before the limits, over the whole text: a syntax fault after a count or a
       precision over its limit is PARSE */
    adf_text_limits_default(&lim);
    lim.max_items = 1;
    ADF_CHECK(FDL("adf1 Q sball n 2 5 b 3 0 4 7 y 3 0 4", 0, &lim) == ADF_PARSE);
    ADF_CHECK(FDL("adf1 Q modctx 6 2 2 3 5", 0, &lim) == ADF_PARSE);
    ADF_CHECK(FDL("adf1 Q qclass pieces 2 1 1 -2 0 0 g 0 1 1 1 3 -2 0 0 g 0 1 1 7", 0, &lim) == ADF_PARSE);
    ADF_CHECK(FD("adf1 Q lball 5 b 3 0 186a1 7", 0) == ADF_PARSE);
    ADF_CHECK(FD("adf1 Q sball n 2 5 b 3 0 186a1 5 y 3 0 4", 0) == ADF_PARSE);
    /* a fault in the last token of a ball of an rfun, the last token of the text */
    ADF_CHECK(FD("adf1 Q rfun 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 g", 0) == ADF_PARSE);
    ADF_CHECK(FD("adf1 Q idclass 5 -2 0 g 5 24", 0) == ADF_PARSE);
    ADF_CHECK(FD("adf1 Q char 5 2 0 0 0 0 0 0 0 g", 0) == ADF_PARSE);
    ADF_CHECK(FD("adf1 Q ffun 1 1 0 0 0 0 0 0 0 g", 0) == ADF_PARSE);
    ADF_CHECK(FD("adf1 Q sball c 1 0 0 0 0 0 0 g 0", 0) == ADF_PARSE);
    ADF_CHECK(FD("adf1 Q idele 1 1 0 0 g 1 1 1 0", 0) == ADF_PARSE);
    lim.max_items = 0;
    ADF_CHECK(FDL("adf1 Q ffun 1 1 0 0 0 0 0 0 0 g", 0, &lim) == ADF_PARSE);
    /* ffun: 8 D M above the tokens left is a grammar fault, also when D M does not fit a word */
    ADF_CHECK(FD("adf1 Q ffun 10000000000000000 1", 0) == ADF_PARSE);
    ADF_CHECK(FD("adf1 Q ffun 1 10000000000000000", 0) == ADF_PARSE);
    ADF_CHECK(FD("adf1 Q ffun 1 1", 0) == ADF_PARSE);
}

/* ------------------------------------------------------------------ binding counts */

/* A binding count other than the number of occurrences is DOMAIN, whatever the first binding
   (conventions 10.2), SIZE_MAX included: it is a count, not the one-context form. */
ADF_TEST(binding_counts_of_every_type)
{
    ulong q23[2] = {2, 3};
    adf_modctx_struct * c = ctx_from(NULL, q23, 2);
    const adf_modctx_struct * b[2];
    adf_rat_t r;
    adf_fball_t f;
    adf_scaled_t sc;
    adf_adele_t a;
    adf_cadele_t z;

    b[0] = c;
    b[1] = c;
    adf_rat_init(r);
    adf_fball_init(f);
    sc_init(sc, c);
    adf_adele_init(a);
    adf_cadele_init(z);
    ADF_CHECK(adf_rat_load_str_binds(r, "adf1 Q rat 7 3", 14, b, SIZE_MAX, NULL) == ADF_DOMAIN);
    ADF_CHECK(adf_fball_load_str_binds(f, "adf1 Q fball g 2 6 1", 20, b, SIZE_MAX, NULL) == ADF_DOMAIN);
    ADF_CHECK(adf_fball_load_str_binds(f, "adf1 Q fball l 1 6 2 2 3 0 2", 28, b, SIZE_MAX, NULL) == ADF_DOMAIN);
    ADF_CHECK(adf_fball_load_str_binds(f, "adf1 Q fball l 1 6 2 2 3 0 2", 28, b, 1, NULL) == ADF_OK);
    ADF_CHECK(adf_scaled_load_str_binds(sc, "adf1 Q scaled x 1 2 6 2 2 3", 27, b, 0, NULL) == ADF_DOMAIN);
    ADF_CHECK(adf_scaled_load_str_binds(sc, "adf1 Q scaled x 1 2 6 2 2 3", 27, b, SIZE_MAX, NULL) == ADF_DOMAIN);
    ADF_CHECK(sc->exact == 1 && fmpq_is_zero(sc->s));
    ADF_CHECK(adf_scaled_load_str_binds(sc, "adf1 Q scaled x 1 2 6 2 2 3", 27, b, 1, NULL) == ADF_OK);
    ADF_CHECK(adf_adele_load_str_binds(a, "adf1 Q adele 1 1 0 0 0 l 1 6 2 2 3 0 2", 38, b, 2, NULL) == ADF_DOMAIN);
    ADF_CHECK(adf_adele_load_str_binds(a, "adf1 Q adele 1 1 0 0 0 l 1 6 2 2 3 0 2", 38, b, SIZE_MAX, NULL)
              == ADF_DOMAIN);
    ADF_CHECK(adf_adele_load_str_binds(a, "adf1 Q adele 1 1 0 0 0 g 0 0 1", 30, b, 1, NULL) == ADF_DOMAIN);
    ADF_CHECK(adf_cadele_load_str_binds(z, "adf1 Q cadele 1 1 0 0 0 1 0 0 0 l 1 6 2 2 3 0 2", 47, b, 0, NULL)
              == ADF_DOMAIN);
    ADF_CHECK(adf_cadele_load_str_binds(z, "adf1 Q cadele 1 1 0 0 0 1 0 0 0 g 0 0 1", 39, b, 1, NULL) == ADF_DOMAIN);
    ADF_CHECK(adf_cadele_load_str_binds(z, "adf1 Q cadele 1 1 0 0 0 1 0 0 0 g 0 0 1", 39, b, SIZE_MAX, NULL)
              == ADF_DOMAIN);
    ADF_CHECK(adf_cadele_load_str_binds(z, "adf1 Q cadele 1 1 0 0 0 1 0 0 0 l 1 6 2 2 3 0 2", 47, b, 1, NULL)
              == ADF_OK);
    ADF_CHECK(local_is_L(&z->fin) && z->fin.mctx == c && f->mctx == c && sc->mctx == c);
    ADF_CHECK(arb_is_zero(a->inf) && a->fin.backend == ADF_GLOBAL);
    adf_rat_clear(r);
    adf_fball_clear(f);
    sc_clear(sc);
    adf_adele_clear(a);
    adf_cadele_clear(z);
    adf_modctx_free(c);
}
