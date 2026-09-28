/* tests/fuzz/fuzz_dump.c: the fuzzer of the dump form (src/dump.c and adf_modctx_new_from_dump
   of src/modctx.c; work package 1.4, lane m1-dump; include/adelefeld/dump.h).

   libFuzzer calls LLVMFuzzerTestOneInput with arbitrary bytes. The bytes go, without a
   terminator (a copy of exactly size bytes, so that a read past the end is caught), to the five
   inspectors, to adf_modctx_new_from_dump, and to the five loaders, each with a small fixed set of
   contexts (none, NULL, {2, 3}, {6}, K = 1 without blocks) and with one binding per occurrence
   made from the text itself by adf_modctx_new_from_dump. The address and undefined-behaviour
   sanitizers of `make fuzz` watch every read and every allocation.

   src/dump.c and src/modctx.c are included into this file, so that they are compiled with the
   sanitizers and the coverage flags of `make fuzz`; the definitions here take the place of the
   archive members dump.o and modctx.o. No FLINT load function sees the input: the loaders do not
   call one, and neither does this file.

   What is asserted (a violation aborts, and libFuzzer keeps the input as a crash):
   - every status is a loader status (conventions 3.2: OK, PARSE, LIMIT, UNSUPPORTED, DOMAIN);
   - the inspector of a type returns the status of the whole validation; the loader returns the
     same status for any binding when it is not OK; when it is OK, the loader with the bindings
     made from the text returns OK, and with a fixed context OK exactly when that context matches
     every occurrence (conventions 10.2);
   - adf_modctx_new_from_dump at occurrence i < n returns OK and a context that matches the
     descriptor of the inspector; at n it returns DOMAIN; on a text the inspector refuses it
     returns the same status (or DOMAIN where the text has no occurrence);
   - on a status other than OK the output value, *nctx, the descriptors and *out are untouched
     (conventions 4.3, closure C2);
   - on OK the value satisfies its predicate (conventions 5), and dumping it gives the input byte
     for byte (CV-38); adf_scaled_get_str gives a value text that adf_fball_set_str reads. */

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../../src/dump.c"
#include "../../src/modctx.c"

#define REQUIRE(cond) do { if (!(cond)) abort(); } while (0)

static int
loader_status(int st)
{
    return st == ADF_OK || st == ADF_PARSE || st == ADF_LIMIT || st == ADF_UNSUPPORTED || st == ADF_DOMAIN;
}

/* The fixed contexts, made once. */
static adf_modctx_struct * fx[3];
static int fx_ready = 0;

static void
fixed_contexts(void)
{
    ulong q23[2] = {2, 3}, q6 = 6;
    fmpz_t one;

    if (fx_ready)
        return;
    REQUIRE(adf_modctx_new_blocks(&fx[0], q23, 2) == ADF_OK);
    REQUIRE(adf_modctx_new_blocks(&fx[1], &q6, 1) == ADF_OK);
    fmpz_init_set_ui(one, 1);
    REQUIRE(adf_modctx_new_fmpz(&fx[2], one) == ADF_OK);
    fmpz_clear(one);
    fx_ready = 1;
}

/* Predicate L (conventions 5.3). */
static int
local_ok(const adf_fball_struct * f)
{
    fmpz_t K;
    slong i, k;
    int ok;

    if (f->mctx == NULL || f->res == NULL)
        return 0;
    k = adf_modctx_nblocks(f->mctx);
    fmpz_init(K);
    adf_modctx_get_modulus(K, f->mctx);
    ok = k >= 1 && fmpz_equal(K, f->H) && fmpz_is_zero(f->A) && fmpz_sgn(f->d) >= 1;
    for (i = 0; ok && i < k; i++)
        ok = f->res[i] < adf_modctx_block(f->mctx, i);
    fmpz_clear(K);
    return ok;
}

static int
fball_ok(const adf_fball_struct * f)
{
    return f->backend == ADF_GLOBAL ? adf_fball_is_canonical(f) : (f->backend == ADF_LOCAL && local_ok(f));
}

static void
same_bytes(const char * t, size_t tl, const char * s, size_t n)
{
    REQUIRE(t != NULL && tl == n && t[tl] == '\0' && memcmp(t, s, n) == 0);
    flint_free((void *) t);
}

enum { F_RAT, F_FBALL, F_SCALED, F_ADELE, F_CADELE };

/* Load with bindings b[0..nb-1] (or the one-context form with ctx when nb == SIZE_MAX); check the
   output against the expected status; on OK dump and compare. Returns the status. */
static int
load_one(int ty, const char * s, size_t n, const adf_text_limits_t * lim, const adf_modctx_struct * const * b,
         size_t nb, const adf_modctx_struct * ctx)
{
    int st;
    size_t tl;

    switch (ty)
    {
        case F_RAT:
        {
            adf_rat_t x;
            adf_rat_struct copy;
            adf_rat_init(x);
            fmpq_set_si(x->q, 12345, 7);
            memcpy(&copy, x, sizeof(copy));
            st = nb == SIZE_MAX ? adf_rat_load_str(x, s, n, ctx, lim) : adf_rat_load_str_binds(x, s, n, b, nb, lim);
            if (st != ADF_OK)
                REQUIRE(memcmp(&copy, x, sizeof(copy)) == 0 && fmpz_equal_si(fmpq_numref(x->q), 12345));
            else
            {
                REQUIRE(adf_rat_is_canonical(x));
                same_bytes(adf_rat_dump_str(&tl, x), tl, s, n);
            }
            adf_rat_clear(x);
            break;
        }
        case F_FBALL:
        {
            adf_fball_t x;
            adf_fball_struct copy;
            adf_fball_init(x);
            fmpz_set_si(x->A, 5);
            fmpz_set_si(x->H, 18);
            memcpy(&copy, x, sizeof(copy));
            st = nb == SIZE_MAX ? adf_fball_load_str(x, s, n, ctx, lim)
                                : adf_fball_load_str_binds(x, s, n, b, nb, lim);
            if (st != ADF_OK)
                REQUIRE(memcmp(&copy, x, sizeof(copy)) == 0 && fmpz_equal_si(x->A, 5) && fmpz_equal_si(x->H, 18));
            else
            {
                REQUIRE(fball_ok(x));
                same_bytes(adf_fball_dump_str(&tl, x), tl, s, n);
            }
            adf_fball_clear(x);
            break;
        }
        case F_SCALED:
        {
            adf_scaled_t x;
            adf_scaled_struct copy;
            fmpq_init(x->s);
            fmpz_init(x->u);
            x->mctx = (const adf_modctx_struct *) (void *) 0x10;
            x->exact = 1;
            fmpq_set_si(x->s, -3, 5);
            memcpy(&copy, x, sizeof(copy));
            st = nb == SIZE_MAX ? adf_scaled_load_str(x, s, n, ctx, lim)
                                : adf_scaled_load_str_binds(x, s, n, b, nb, lim);
            if (st != ADF_OK)
                REQUIRE(memcmp(&copy, x, sizeof(copy)) == 0 && fmpz_equal_si(fmpq_numref(x->s), -3));
            else
            {
                fmpz_t K;
                char * v;
                adf_fball_t f;
                fmpz_init(K);
                adf_modctx_get_modulus(K, x->mctx);
                REQUIRE(fmpq_is_canonical(x->s) && (x->exact == 0 || x->exact == 1));
                if (x->exact)
                    REQUIRE(fmpz_is_zero(x->u));
                else
                    REQUIRE(fmpq_sgn(x->s) > 0 && fmpz_sgn(x->u) >= 0 && fmpz_cmp(x->u, K) < 0);
                fmpz_clear(K);
                same_bytes(adf_scaled_dump_str(&tl, x), tl, s, n);
                v = adf_scaled_get_str(&tl, x);
                adf_fball_init(f);
                REQUIRE(v != NULL && v[tl] == '\0' && adf_fball_set_str(f, v, tl, NULL) == ADF_OK);
                adf_fball_clear(f);
                flint_free(v);
            }
            fmpq_clear(x->s);
            fmpz_clear(x->u);
            break;
        }
        case F_ADELE:
        {
            adf_adele_t x;
            adf_adele_struct copy;
            adf_adele_init(x);
            arb_set_si(x->inf, 7);
            memcpy(&copy, x, sizeof(copy));
            st = nb == SIZE_MAX ? adf_adele_load_str(x, s, n, ctx, lim)
                                : adf_adele_load_str_binds(x, s, n, b, nb, lim);
            if (st != ADF_OK)
                REQUIRE(memcmp(&copy, x, sizeof(copy)) == 0 && arf_equal_si(arb_midref(x->inf), 7));
            else
            {
                REQUIRE(arb_is_finite(x->inf) && fball_ok(&x->fin));
                same_bytes(adf_adele_dump_str(&tl, x), tl, s, n);
            }
            adf_adele_clear(x);
            break;
        }
        default:
        {
            adf_cadele_t x;
            adf_cadele_struct copy;
            adf_cadele_init(x);
            acb_set_si(x->inf, 7);
            memcpy(&copy, x, sizeof(copy));
            st = nb == SIZE_MAX ? adf_cadele_load_str(x, s, n, ctx, lim)
                                : adf_cadele_load_str_binds(x, s, n, b, nb, lim);
            if (st != ADF_OK)
                REQUIRE(memcmp(&copy, x, sizeof(copy)) == 0 && arf_equal_si(arb_midref(acb_realref(x->inf)), 7));
            else
            {
                REQUIRE(acb_is_finite(x->inf) && fball_ok(&x->fin));
                same_bytes(adf_cadele_dump_str(&tl, x), tl, s, n);
            }
            adf_cadele_clear(x);
            break;
        }
    }
    REQUIRE(loader_status(st));
    return st;
}

static int
inspect(int ty, size_t * nctx, adf_ctx_desc_t * d, const char * s, size_t n, const adf_text_limits_t * lim)
{
    switch (ty)
    {
        case F_RAT: return adf_rat_dump_inspect(nctx, d, s, n, lim);
        case F_FBALL: return adf_fball_dump_inspect(nctx, d, s, n, lim);
        case F_SCALED: return adf_scaled_dump_inspect(nctx, d, s, n, lim);
        case F_ADELE: return adf_adele_dump_inspect(nctx, d, s, n, lim);
        default: return adf_cadele_dump_inspect(nctx, d, s, n, lim);
    }
}

static void
fuzz_type(int ty, const char * s, size_t n, const adf_text_limits_t * lim)
{
    adf_ctx_desc_t d[2];
    size_t nctx = 2, cnt = 777, i;
    adf_modctx_struct * own[2] = {NULL, NULL};
    const adf_modctx_struct * b[2] = {NULL, NULL};
    int ist, st, j;

    adf_ctx_desc_init(&d[0]);
    adf_ctx_desc_init(&d[1]);
    fmpz_set_ui(d[1].K, 55);
    ist = inspect(ty, &nctx, d, s, n, lim);
    REQUIRE(loader_status(ist));
    REQUIRE(inspect(ty, &cnt, NULL, s, n, lim) == ist);
    if (ist != ADF_OK)
    {
        REQUIRE(nctx == 2 && cnt == 777 && fmpz_is_one(d[0].K) && d[0].q == NULL && fmpz_equal_ui(d[1].K, 55));
        for (j = 0; j < 3; j++)
            REQUIRE(load_one(ty, s, n, lim, NULL, SIZE_MAX, fx[j]) == ist);
        REQUIRE(load_one(ty, s, n, lim, NULL, SIZE_MAX, NULL) == ist);
        REQUIRE(load_one(ty, s, n, lim, NULL, 0, NULL) == ist);
    }
    else
    {
        REQUIRE(nctx == cnt && nctx <= 1 && fmpz_equal_ui(d[1].K, 55));
        /* one binding per occurrence, made from the text */
        for (i = 0; i < nctx; i++)
        {
            adf_modctx_struct * sentinel = (adf_modctx_struct *) (void *) 0x20, * c = sentinel;
            REQUIRE(adf_modctx_new_from_dump(&c, s, n, i, lim) == ADF_OK && c != sentinel);
            REQUIRE(adf_modctx_matches_desc(c, &d[i]));
            own[i] = c;
            b[i] = c;
        }
        {
            adf_modctx_struct * sentinel = (adf_modctx_struct *) (void *) 0x20, * c = sentinel;
            REQUIRE(adf_modctx_new_from_dump(&c, s, n, nctx, lim) == ADF_DOMAIN && c == sentinel);
        }
        REQUIRE(load_one(ty, s, n, lim, b, nctx, NULL) == ADF_OK);
        REQUIRE(load_one(ty, s, n, lim, b, nctx + 1, NULL) == ADF_DOMAIN);
        /* the fixed contexts: OK exactly when they match every occurrence */
        for (j = 0; j < 3; j++)
        {
            int match = nctx == 0 || adf_modctx_matches_desc(fx[j], &d[0]);
            st = load_one(ty, s, n, lim, NULL, SIZE_MAX, fx[j]);
            REQUIRE(st == (match ? ADF_OK : ADF_DOMAIN));
        }
        REQUIRE(load_one(ty, s, n, lim, NULL, SIZE_MAX, NULL) == (nctx == 0 ? ADF_OK : ADF_DOMAIN));
        for (i = 0; i < nctx; i++)
            adf_modctx_free(own[i]);
    }
    adf_ctx_desc_clear(&d[0]);
    adf_ctx_desc_clear(&d[1]);
}

/* adf_modctx_new_from_dump at every occurrence index up to 4, any body: statuses of a loader,
   *out untouched on failure; once an index gives DOMAIN after OKs, every later index does. */
static void
from_dump_all(const char * p, size_t size, const adf_text_limits_t * lim)
{
    size_t occ;
    int prev = ADF_OK;

    for (occ = 0; occ < 4; occ++)
    {
        adf_modctx_struct * sentinel = (adf_modctx_struct *) (void *) 0x30, * c = sentinel;
        int st = adf_modctx_new_from_dump(&c, p, size, occ, lim);
        REQUIRE(loader_status(st));
        if (occ > 0 && prev != ADF_OK)
            REQUIRE(st == prev);
        prev = st;
        if (st == ADF_OK)
        {
            size_t tl;
            char * t = adf_modctx_dump_str(&tl, c);
            REQUIRE(t != NULL && t[tl] == '\0' && c != sentinel);
            flint_free(t);
            adf_modctx_free(c);
        }
        else
            REQUIRE(c == sentinel);
    }
}

int LLVMFuzzerTestOneInput(const uint8_t * data, size_t size);

int
LLVMFuzzerTestOneInput(const uint8_t * data, size_t size)
{
    char * s = (char *) malloc(size > 0 ? size : 1);
    const char * p = size > 0 ? s : NULL;
    adf_text_limits_t lim;
    int ty;

    fixed_contexts();
    if (size > 0)
        memcpy(s, data, size);
    adf_text_limits_default(&lim);
    from_dump_all(p, size, NULL);
    from_dump_all(p, size, &lim);
    lim.max_items = 1;
    lim.max_prec = 3;
    from_dump_all(p, size, &lim);
    adf_text_limits_default(&lim);
    /* capacity 0: LIMIT exactly when the text is valid with an occurrence (closure C2) */
    for (ty = F_RAT; ty <= F_CADELE; ty++)
    {
        size_t n0 = 0, n1 = 5;
        adf_ctx_desc_t d;
        int a, b2;
        adf_ctx_desc_init(&d);
        a = inspect(ty, &n0, &d, p, size, &lim);
        b2 = inspect(ty, &n1, NULL, p, size, &lim);
        REQUIRE(a == (b2 == ADF_OK && n1 > 0 ? ADF_LIMIT : b2) && n0 == 0 && d.q == NULL && fmpz_is_one(d.K));
        adf_ctx_desc_clear(&d);
        REQUIRE(inspect(ty, NULL, NULL, p, size, &lim) == ADF_DOMAIN);
    }
    for (ty = F_RAT; ty <= F_CADELE; ty++)
        fuzz_type(ty, p, size, &lim);
    /* a small count limit (stage 4), then a length limit one byte short (stage 1) */
    lim.max_items = 1;
    for (ty = F_RAT; ty <= F_CADELE; ty++)
        fuzz_type(ty, p, size, &lim);
    if (size > 0)
    {
        adf_text_limits_default(&lim);
        lim.max_len = size - 1;
        for (ty = F_RAT; ty <= F_CADELE; ty++)
            fuzz_type(ty, p, size, &lim);
    }
    free(s);
    return 0;
}
