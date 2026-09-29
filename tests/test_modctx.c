/* tests/test_modctx.c: modulus contexts (work package 1.8, first part).

   Runs every vector file of tests/ref/vectors/m1-modctx/ completely (tests/README.md): the
   constructors of every modulus family against the Python reference tests/ref/adfref/modctx_ref.py
   (written from docs/conventions.md 5.14 and docs/proofs/policies.md Definition 16 and Lemma 17),
   and the conversion of an integer to residues and back. Then the parts the vectors cannot show:
   the state of *out after every failure, the copying of input arrays, the read access, the
   descriptor, the dump text, random round trips with 1, 2, 64 and 128 blocks, and two threads
   reading one context at the same time (conventions 4.5).

   adf_modctx_reduce and adf_modctx_combine are the two conversions of docs/PERF.md section 4
   ("Global integer to k residues", "k residues to the global integer"). They are exported by
   src/modctx.c for the local backend of work package 1.8 (conventions 5.3) and are declared
   here because no public header declares them. */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <adelefeld/modctx.h>

#include <flint/fmpz.h>
#include <flint/ulong_extras.h>

#include "support/jsonl.h"
#include "test_runner.h"

/* The conversions of src/modctx.c: res[i] = A mod q_i in [0, q_i); A = the unique integer of
   [0, K) with the residues res (docs/proofs/policies.md Definition 16, line 305). */
void adf_modctx_reduce(ulong * res, const fmpz_t A, const adf_modctx_struct * ctx);
void adf_modctx_combine(fmpz_t A, const ulong * res, const adf_modctx_struct * ctx);

/* The prime of the certified prime-power block q_i, 0 when block i is not a certified prime
   power (conventions 5.14 content; src/modctx.c). */
ulong adf_modctx_block_prime(const adf_modctx_struct * ctx, slong i);

/* The cap on the block count of a context (conventions 8.4 max_items; the choice is recorded
   in docs/api-m1.md terms as HEADER-FINDING 1 of lane m1-modctx). */
#define TEST_MAX_BLOCKS 1048576

/* --------------------------------------------------------------- JSON helpers */

/* Read the integer in the JSON value into v (already initialised). */
static int
json_fmpz(fmpz_t v, const jsonl_value * j, jsonl_error_t * err)
{
    const char * text = jsonl_int_text(j, err);
    if (text == NULL)
        return 0;
    return fmpz_set_str(v, text, 10) == 0;
}

/* Read the word-sized integer in the JSON value (0 <= value < 2^64). */
static int
json_ulong(ulong * v, const jsonl_value * j, jsonl_error_t * err)
{
    fmpz_t t;
    int ok;

    fmpz_init(t);
    ok = json_fmpz(t, j, err) && fmpz_abs_fits_ui(t);
    if (ok)
        *v = fmpz_get_ui(t);
    fmpz_clear(t);
    return ok;
}

/* The status name of the JSON value must be the name of `status` (conventions 11.1). */
static int
json_status_is(int status, const jsonl_value * j, jsonl_error_t * err)
{
    size_t len;
    const char * name = jsonl_string(j, &len, err);
    const char * have = adf_status_str(status);

    return name != NULL && strlen(have) == len && memcmp(have, name, len) == 0;
}

/* The ulong array of a JSON array. Returns NULL when the record is malformed; the caller
   frees the array with flint_free. len 0 gives a NULL array, as the constructors accept. */
static ulong *
json_ulong_array(const jsonl_value * j, size_t * len, jsonl_error_t * err)
{
    size_t n, i;
    ulong * a;

    if (!jsonl_is(j, JSONL_ARRAY))
        return NULL;
    n = jsonl_size(j);
    *len = n;
    if (n == 0)
        return NULL;
    a = flint_malloc(n * sizeof(ulong));
    for (i = 0; i < n; i++)
    {
        const jsonl_value * e = jsonl_at(j, i, err);
        if (e == NULL || !json_ulong(a + i, e, err))
        {
            flint_free(a);
            return NULL;
        }
    }
    return a;
}

/* A pointer the constructors must never write: *out must be untouched on every failure. */
static int test_sentinel_object;
#define SENTINEL ((adf_modctx_struct *) &test_sentinel_object)

/* Call the constructor and check the status against the record, that *out is untouched on a
   failure and that the context matches "K" and "blocks" on success. Frees the context. */
static void
check_constructed(adf_modctx_struct ** out, int status, const jsonl_value * rec,
                  jsonl_error_t * err, const char * what)
{
    const jsonl_value * jstatus;

    ADF_CHECK_MSG(jsonl_field(rec, "status", &jstatus, err) == 1, "%s: %s", what,
                  jsonl_error_message(err));
    ADF_CHECK_MSG(json_status_is(status, jstatus, err), "%s: unexpected status %s", what,
                  adf_status_str(status));
    if (status != ADF_OK)
    {
        ADF_CHECK_MSG(*out == SENTINEL, "%s: *out written on failure", what);
        return;
    }
    ADF_CHECK_MSG(*out != SENTINEL && *out != NULL, "%s: *out not written on success", what);
    {
        const jsonl_value * jK, * jblocks;
        fmpz_t K, back;
        size_t nblocks, i;
        ulong * blocks;

        ADF_CHECK_MSG(jsonl_field(rec, "K", &jK, err) == 1, "%s: %s", what,
                      jsonl_error_message(err));
        ADF_CHECK_MSG(jsonl_field(rec, "blocks", &jblocks, err) == 1, "%s: %s", what,
                      jsonl_error_message(err));
        fmpz_init(K);
        fmpz_init(back);
        ADF_CHECK_MSG(json_fmpz(K, jK, err) == 1, "%s: %s", what, jsonl_error_message(err));
        adf_modctx_get_modulus(back, *out);
        ADF_CHECK_MSG(fmpz_equal(back, K), "%s: the modulus differs from the reference", what);
        fmpz_clear(K);
        fmpz_clear(back);
        blocks = json_ulong_array(jblocks, &nblocks, err);
        ADF_CHECK_MSG(blocks != NULL || nblocks == 0, "%s: %s", what, jsonl_error_message(err));
        ADF_CHECK_MSG((size_t) adf_modctx_nblocks(*out) == nblocks, "%s: nblocks %ld != %u",
                      what, (long) adf_modctx_nblocks(*out), (unsigned) nblocks);
        for (i = 0; i < nblocks; i++)
            ADF_CHECK_MSG(adf_modctx_block(*out, (slong) i) == blocks[i],
                          "%s: block %u is %llu, expected %llu", what, (unsigned) i,
                          (unsigned long long) adf_modctx_block(*out, (slong) i),
                          (unsigned long long) blocks[i]);
        flint_free(blocks);
    }
    adf_modctx_free(*out);
    *out = SENTINEL;
}

/* --------------------------------------------------------- the vector files */

ADF_TEST(constructors_against_the_reference)
{
    jsonl_error_t err;
    jsonl_file * f = NULL;
    size_t i;

    ADF_CHECK_MSG(jsonl_open("tests/ref/vectors/m1-modctx/ctx_construct.jsonl", &f, &err) == 1,
                  "%s", jsonl_error_message(&err));
    if (f == NULL)
        return;
    for (i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value * rec = jsonl_record(f, i);
        const jsonl_value * jop;
        size_t len;
        const char * op;
        char what[64];
        adf_modctx_struct * out = SENTINEL;

        ADF_CHECK(jsonl_field(rec, "op", &jop, &err) == 1);
        op = jsonl_string(jop, &len, &err);
        ADF_CHECK(op != NULL);
        if (op == NULL)
            continue;
        snprintf(what, sizeof(what), "%s record %u", op, (unsigned) i);

        if (len == strlen("new_blocks") && memcmp(op, "new_blocks", len) == 0)
        {
            const jsonl_value * jq;
            size_t n = 0;
            ulong * q;
            int st;
            ADF_CHECK_MSG(jsonl_field(rec, "q", &jq, &err) == 1, "%s: %s", what,
                          jsonl_error_message(&err));
            q = json_ulong_array(jq, &n, &err);
            ADF_CHECK_MSG(q != NULL || n == 0, "%s: %s", what, jsonl_error_message(&err));
            st = adf_modctx_new_blocks(&out, q, (slong) n);
            check_constructed(&out, st, rec, &err, what);
            flint_free(q);
        }
        else if (len == strlen("new_prime_powers") && memcmp(op, "new_prime_powers", len) == 0)
        {
            const jsonl_value * jp, * je;
            size_t n = 0, ne = 0;
            ulong * p, * e;
            int st;
            ADF_CHECK_MSG(jsonl_field(rec, "p", &jp, &err) == 1, "%s: %s", what,
                          jsonl_error_message(&err));
            ADF_CHECK_MSG(jsonl_field(rec, "e", &je, &err) == 1, "%s: %s", what,
                          jsonl_error_message(&err));
            p = json_ulong_array(jp, &n, &err);
            e = json_ulong_array(je, &ne, &err);
            ADF_CHECK_MSG((p != NULL || n == 0) && ne == n, "%s: %s", what,
                          jsonl_error_message(&err));
            st = adf_modctx_new_prime_powers(&out, p, e, (slong) n);
            check_constructed(&out, st, rec, &err, what);
            flint_free(p);
            flint_free(e);
        }
        else if (len == strlen("new_fmpz") && memcmp(op, "new_fmpz", len) == 0)
        {
            const jsonl_value * jK;
            fmpz_t K;
            int st;
            ADF_CHECK_MSG(jsonl_field(rec, "input_K", &jK, &err) == 1, "%s: %s", what,
                          jsonl_error_message(&err));
            fmpz_init(K);
            ADF_CHECK_MSG(json_fmpz(K, jK, &err) == 1, "%s: %s", what,
                          jsonl_error_message(&err));
            st = adf_modctx_new_fmpz(&out, K);
            check_constructed(&out, st, rec, &err, what);
            fmpz_clear(K);
        }
        else if (len == strlen("new_factorial") && memcmp(op, "new_factorial", len) == 0)
        {
            const jsonl_value * jn;
            ulong n = 0;
            int st;
            ADF_CHECK_MSG(jsonl_field(rec, "n", &jn, &err) == 1, "%s: %s", what,
                          jsonl_error_message(&err));
            ADF_CHECK_MSG(json_ulong(&n, jn, &err) == 1, "%s: %s", what,
                          jsonl_error_message(&err));
            st = adf_modctx_new_factorial(&out, n);
            check_constructed(&out, st, rec, &err, what);
        }
        else if (len == strlen("new_primorial_pow") && memcmp(op, "new_primorial_pow", len) == 0)
        {
            const jsonl_value * jn, * je;
            ulong n = 0, e = 0;
            int st;
            ADF_CHECK_MSG(jsonl_field(rec, "n", &jn, &err) == 1, "%s: %s", what,
                          jsonl_error_message(&err));
            ADF_CHECK_MSG(jsonl_field(rec, "e", &je, &err) == 1, "%s: %s", what,
                          jsonl_error_message(&err));
            ADF_CHECK_MSG(json_ulong(&n, jn, &err) == 1 && json_ulong(&e, je, &err) == 1,
                          "%s: %s", what, jsonl_error_message(&err));
            st = adf_modctx_new_primorial_pow(&out, n, e);
            check_constructed(&out, st, rec, &err, what);
        }
        else
        {
            ADF_CHECK_MSG(0, "%s: unknown op", what);
        }
    }
    jsonl_close(f);
}

ADF_TEST(conversion_against_the_reference)
{
    jsonl_error_t err;
    jsonl_file * f = NULL;
    size_t i;

    ADF_CHECK_MSG(jsonl_open("tests/ref/vectors/m1-modctx/ctx_roundtrip.jsonl", &f, &err) == 1,
                  "%s", jsonl_error_message(&err));
    if (f == NULL)
        return;
    for (i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value * rec = jsonl_record(f, i);
        const jsonl_value * jblocks, * ja, * jres, * jback;
        size_t n, nr, j;
        ulong * blocks, * res;
        fmpz_t a, back, want;

        ADF_CHECK(jsonl_field(rec, "blocks", &jblocks, &err) == 1);
        ADF_CHECK(jsonl_field(rec, "a", &ja, &err) == 1);
        ADF_CHECK(jsonl_field(rec, "residues", &jres, &err) == 1);
        ADF_CHECK(jsonl_field(rec, "combined", &jback, &err) == 1);
        blocks = json_ulong_array(jblocks, &n, &err);
        ADF_CHECK_MSG(blocks != NULL, "record %u: %s", (unsigned) i, jsonl_error_message(&err));
        if (blocks == NULL)
            continue;
        res = json_ulong_array(jres, &nr, &err);
        ADF_CHECK_MSG(res != NULL && nr == n, "record %u: %s", (unsigned) i,
                      jsonl_error_message(&err));
        fmpz_init(a);
        fmpz_init(back);
        fmpz_init(want);
        ADF_CHECK(json_fmpz(a, ja, &err) == 1);
        ADF_CHECK(json_fmpz(back, jback, &err) == 1);
        {
            adf_modctx_struct * ctx = NULL;
            ulong * out = flint_malloc(n * sizeof(ulong));

            ADF_CHECK_MSG(adf_modctx_new_blocks(&ctx, blocks, (slong) n) == ADF_OK,
                          "record %u: blocks not accepted", (unsigned) i);
            adf_modctx_reduce(out, a, ctx);
            for (j = 0; j < n; j++)
                ADF_CHECK_MSG(out[j] == res[j], "record %u: residue %u is %llu, expected %llu",
                              (unsigned) i, (unsigned) j, (unsigned long long) out[j],
                              (unsigned long long) res[j]);
            /* an independent oracle for the residues: fmpz_fdiv_ui */
            for (j = 0; j < n; j++)
                ADF_CHECK(out[j] == fmpz_fdiv_ui(a, blocks[j]));
            adf_modctx_combine(want, out, ctx);
            ADF_CHECK_MSG(fmpz_equal(want, back), "record %u: combined value differs",
                          (unsigned) i);
            /* an independent oracle for the recombination: a mod K in [0, K) */
            {
                fmpz_t K;
                fmpz_init(K);
                adf_modctx_get_modulus(K, ctx);
                fmpz_mod(want, a, K);
                ADF_CHECK(fmpz_equal(want, back));
                ADF_CHECK(fmpz_sgn(back) >= 0 && fmpz_cmp(back, K) < 0);
                fmpz_clear(K);
            }
            flint_free(out);
            adf_modctx_free(ctx);
        }
        fmpz_clear(a);
        fmpz_clear(back);
        fmpz_clear(want);
        flint_free(blocks);
        flint_free(res);
    }
    jsonl_close(f);
}

/* ------------------------------------------------- the states of the outputs */

ADF_TEST(block_list_failures_leave_out_untouched)
{
    adf_modctx_struct * out = SENTINEL;
    ulong q0[] = {0};
    ulong q1[] = {1};
    ulong q01[] = {2, 0};
    ulong q11[] = {2, 1};
    ulong qnc[] = {4, 6};
    ulong qnc3[] = {2, 9, 15};
    ulong qdup[] = {65537, 65537};

    ADF_CHECK(adf_modctx_new_blocks(&out, q0, 1) == ADF_DOMAIN);
    ADF_CHECK(out == SENTINEL);
    ADF_CHECK(adf_modctx_new_blocks(&out, q1, 1) == ADF_DOMAIN);
    ADF_CHECK(out == SENTINEL);
    ADF_CHECK(adf_modctx_new_blocks(&out, q01, 2) == ADF_DOMAIN);
    ADF_CHECK(adf_modctx_new_blocks(&out, q11, 2) == ADF_DOMAIN);
    ADF_CHECK(out == SENTINEL);
    ADF_CHECK(adf_modctx_new_blocks(&out, qnc, 2) == ADF_DOMAIN);
    ADF_CHECK(out == SENTINEL);
    ADF_CHECK(adf_modctx_new_blocks(&out, qnc3, 3) == ADF_DOMAIN);
    ADF_CHECK(adf_modctx_new_blocks(&out, qdup, 2) == ADF_DOMAIN);
    ADF_CHECK(out == SENTINEL);
    /* k < 0 */
    ADF_CHECK(adf_modctx_new_blocks(&out, q0, -1) == ADF_DOMAIN);
    ADF_CHECK(out == SENTINEL);
    /* a block list that does not exist (q may be NULL only when k = 0) */
    ADF_CHECK(adf_modctx_new_blocks(&out, NULL, 1) == ADF_DOMAIN);
    ADF_CHECK(out == SENTINEL);
    /* k = 0 with q = NULL is K = 1 without blocks */
    {
        adf_modctx_struct * ctx = NULL;
        fmpz_t K;
        ADF_CHECK(adf_modctx_new_blocks(&ctx, NULL, 0) == ADF_OK);
        ADF_CHECK(adf_modctx_nblocks(ctx) == 0);
        fmpz_init(K);
        adf_modctx_get_modulus(K, ctx);
        ADF_CHECK(fmpz_cmp_ui(K, 1) == 0);
        fmpz_clear(K);
        adf_modctx_free(ctx);
    }
}

ADF_TEST(too_many_blocks_are_refused_before_the_list_is_read)
{
    adf_modctx_struct * out = SENTINEL;

    /* The count check runs first, so a block list that cannot exist is never read. */
    ADF_CHECK(adf_modctx_new_blocks(&out, NULL, TEST_MAX_BLOCKS + 1) == ADF_UNSUPPORTED);
    ADF_CHECK(out == SENTINEL);
    ADF_CHECK(adf_modctx_new_prime_powers(&out, NULL, NULL, TEST_MAX_BLOCKS + 1) ==
              ADF_UNSUPPORTED);
    ADF_CHECK(out == SENTINEL);
    /* The boundary itself (k = TEST_MAX_BLOCKS accepted) is not run here: the documented cost
       of the constructor is k^2 word gcds (modctx.h), about 10^12 at the cap. */
}

ADF_TEST(prime_power_failures_leave_out_untouched)
{
    adf_modctx_struct * out = SENTINEL;
    ulong p[] = {2};
    ulong p2[] = {2, 2};
    ulong p3[] = {2, 3};
    ulong pcomp[] = {4};
    ulong pcomp2[] = {2, 4};
    ulong e0[] = {0};
    ulong e1[] = {1};
    ulong e2[] = {2};
    ulong e64[] = {64};
    ulong e12[] = {1, 2};
    ulong ebig[] = {2, 41};          /* 2^2 fits, 3^41 >= 2^64 */

    ADF_CHECK(adf_modctx_new_prime_powers(&out, p, e0, 1) == ADF_DOMAIN);
    ADF_CHECK(out == SENTINEL);
    ADF_CHECK(adf_modctx_new_prime_powers(&out, pcomp, e2, 1) == ADF_DOMAIN);
    ADF_CHECK(out == SENTINEL);
    ADF_CHECK(adf_modctx_new_prime_powers(&out, pcomp2, e12, 2) == ADF_DOMAIN);
    ADF_CHECK(out == SENTINEL);
    ADF_CHECK(adf_modctx_new_prime_powers(&out, p2, e12, 2) == ADF_DOMAIN);
    ADF_CHECK(out == SENTINEL);
    ADF_CHECK(adf_modctx_new_prime_powers(&out, p3, ebig, 2) == ADF_UNSUPPORTED);
    ADF_CHECK(out == SENTINEL);
    ADF_CHECK(adf_modctx_new_prime_powers(&out, p, e64, 1) == ADF_UNSUPPORTED);
    ADF_CHECK(out == SENTINEL);
    ADF_CHECK(adf_modctx_new_prime_powers(&out, p, e1, -1) == ADF_DOMAIN);
    ADF_CHECK(out == SENTINEL);
    ADF_CHECK(adf_modctx_new_prime_powers(&out, NULL, e1, 1) == ADF_DOMAIN);
    ADF_CHECK(out == SENTINEL);
    ADF_CHECK(adf_modctx_new_prime_powers(&out, p, NULL, 1) == ADF_DOMAIN);
    ADF_CHECK(out == SENTINEL);
    /* 2^1 * 3^2 is accepted: the order supplied is kept */
    {
        adf_modctx_struct * ctx = NULL;
        ADF_CHECK(adf_modctx_new_prime_powers(&out, p3, e12, 2) == ADF_OK);
        ctx = out;
        out = SENTINEL;
        ADF_CHECK(adf_modctx_block(ctx, 0) == 2 && adf_modctx_block(ctx, 1) == 9);
        adf_modctx_free(ctx);
    }
}

ADF_TEST(other_family_failures_leave_out_untouched)
{
    adf_modctx_struct * out = SENTINEL;
    fmpz_t K;

    fmpz_init(K);
    fmpz_set_ui(K, 0);
    ADF_CHECK(adf_modctx_new_fmpz(&out, K) == ADF_DOMAIN);
    ADF_CHECK(out == SENTINEL);
    fmpz_set_si(K, -5);
    ADF_CHECK(adf_modctx_new_fmpz(&out, K) == ADF_DOMAIN);
    ADF_CHECK(out == SENTINEL);
    fmpz_set_str(K, "-123456789012345678901234567890", 10);
    ADF_CHECK(adf_modctx_new_fmpz(&out, K) == ADF_DOMAIN);
    ADF_CHECK(out == SENTINEL);
    fmpz_clear(K);

    /* 66! has the prime power 2^64; every n >= 66 is refused */
    ADF_CHECK(adf_modctx_new_factorial(&out, 66) == ADF_UNSUPPORTED);
    ADF_CHECK(out == SENTINEL);
    ADF_CHECK(adf_modctx_new_factorial(&out, 1000) == ADF_UNSUPPORTED);
    ADF_CHECK(out == SENTINEL);

    /* primorial powers: 3^63 and 2^64 leave a word */
    ADF_CHECK(adf_modctx_new_primorial_pow(&out, 3, 63) == ADF_UNSUPPORTED);
    ADF_CHECK(out == SENTINEL);
    ADF_CHECK(adf_modctx_new_primorial_pow(&out, 2, 64) == ADF_UNSUPPORTED);
    ADF_CHECK(out == SENTINEL);
    ADF_CHECK(adf_modctx_new_primorial_pow(&out, 100, 64) == ADF_UNSUPPORTED);
    ADF_CHECK(out == SENTINEL);
}

/* ---------------------------------------------------------- read access */

ADF_TEST(read_access_and_ownership)
{
    ulong qmut[] = {5, 2};
    adf_modctx_struct * ctx = NULL;
    adf_modctx_struct * other = NULL;
    fmpz_t K;

    /* the order supplied is kept */
    ADF_CHECK(adf_modctx_new_blocks(&ctx, qmut, 2) == ADF_OK);
    fmpz_init(K);
    adf_modctx_get_modulus(K, ctx);
    ADF_CHECK(fmpz_cmp_ui(K, 10) == 0);
    ADF_CHECK(adf_modctx_nblocks(ctx) == 2);
    ADF_CHECK(adf_modctx_block(ctx, 0) == 5);
    ADF_CHECK(adf_modctx_block(ctx, 1) == 2);

    /* the array is copied (conventions 4.2): changing it changes nothing */
    qmut[0] = 7;
    qmut[1] = 11;
    ADF_CHECK(adf_modctx_block(ctx, 0) == 5);
    ADF_CHECK(adf_modctx_block(ctx, 1) == 2);

    /* a constructor never frees, mutates or reuses the previous *out (closure E2) */
    other = ctx;
    {
        ulong q2[] = {3};
        adf_modctx_struct ** outp = &other;
        ADF_CHECK(adf_modctx_new_blocks(outp, q2, 1) == ADF_OK);
        ADF_CHECK(other != ctx);
        ADF_CHECK(adf_modctx_block(ctx, 0) == 5);   /* the old context is intact */
        adf_modctx_get_modulus(K, ctx);
        ADF_CHECK(fmpz_cmp_ui(K, 10) == 0);
        adf_modctx_free(other);
        other = NULL;
        /* on failure the slot keeps whatever it held */
        other = ctx;
        ADF_CHECK(adf_modctx_new_blocks(outp, NULL, 1) == ADF_DOMAIN);
        ADF_CHECK(other == ctx);
    }
    fmpz_clear(K);
    adf_modctx_free(ctx);
    /* free(NULL) does nothing (conventions 4.6) */
    adf_modctx_free(NULL);
}

ADF_TEST(fmpz_family_edges)
{
    struct { const char * k; int blocks; } cases[] = {
        {"1", 0}, {"2", 1}, {"7", 1}, {"18446744073709551615", 1},
        {"18446744073709551616", 0}, {"340282366920938463463374607431768211457", 0}
    };
    size_t i;

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
    {
        adf_modctx_struct * ctx = NULL;
        fmpz_t K, back;
        fmpz_init(K);
        fmpz_init(back);
        ADF_CHECK(fmpz_set_str(K, cases[i].k, 10) == 0);
        ADF_CHECK(adf_modctx_new_fmpz(&ctx, K) == ADF_OK);
        ADF_CHECK(adf_modctx_nblocks(ctx) == cases[i].blocks);
        adf_modctx_get_modulus(back, ctx);
        /* the modulus is kept even when there is no block (modctx.h, new_fmpz) */
        ADF_CHECK_MSG(fmpz_equal(back, K), "new_fmpz(%s) kept the wrong modulus", cases[i].k);
        if (cases[i].blocks == 1)
            ADF_CHECK(adf_modctx_block(ctx, 0) == fmpz_get_ui(K));
        fmpz_clear(K);
        fmpz_clear(back);
        adf_modctx_free(ctx);
    }
}

ADF_TEST(factorial_and_primorial_are_the_products_of_their_blocks)
{
    /* 65! is the largest accepted factorial; K must equal n! exactly and the blocks must be
       the prime powers p^v_p(n!) in increasing order of the prime (conventions 5.14, CV-21),
       computed here again from Legendre's formula */
    ulong n;
    for (n = 0; n <= 65; n++)
    {
        adf_modctx_struct * ctx = NULL;
        fmpz_t K, fac, prod, q, pw;
        slong i;
        ulong p, e, t;
        ADF_CHECK(adf_modctx_new_factorial(&ctx, n) == ADF_OK);
        fmpz_init(K);
        fmpz_init(fac);
        fmpz_init(prod);
        fmpz_init(q);
        fmpz_init(pw);
        adf_modctx_get_modulus(K, ctx);
        fmpz_fac_ui(fac, n);
        ADF_CHECK_MSG(fmpz_equal(K, fac), "the modulus of %llu! is wrong",
                      (unsigned long long) n);
        fmpz_set_ui(prod, 1);
        i = 0;
        for (p = 2; p <= n; p++)
        {
            if (!n_is_prime(p))
                continue;
            e = 0;
            for (t = n / p; t > 0; t /= p)
                e += t;
            fmpz_set_ui(pw, 1);
            for (t = 0; t < e; t++)
                fmpz_mul_ui(pw, pw, p);
            ADF_CHECK_MSG(i < adf_modctx_nblocks(ctx), "%llu!: too few blocks",
                          (unsigned long long) n);
            fmpz_set_ui(q, adf_modctx_block(ctx, i));
            ADF_CHECK_MSG(fmpz_equal(q, pw), "%llu!: block %ld is not %llu^%llu",
                          (unsigned long long) n, (long) i, (unsigned long long) p,
                          (unsigned long long) e);
            fmpz_mul(prod, prod, pw);
            i++;
        }
        ADF_CHECK(i == adf_modctx_nblocks(ctx));
        ADF_CHECK(fmpz_equal(prod, K));
        fmpz_clear(K);
        fmpz_clear(fac);
        fmpz_clear(prod);
        fmpz_clear(q);
        fmpz_clear(pw);
        adf_modctx_free(ctx);
    }
    /* (product of the primes p <= 10)^2: the blocks are 4, 9, 25, 49 */
    {
        adf_modctx_struct * ctx = NULL;
        ADF_CHECK(adf_modctx_new_primorial_pow(&ctx, 10, 2) == ADF_OK);
        ADF_CHECK(adf_modctx_nblocks(ctx) == 4);
        ADF_CHECK(adf_modctx_block(ctx, 0) == 4);
        ADF_CHECK(adf_modctx_block(ctx, 1) == 9);
        ADF_CHECK(adf_modctx_block(ctx, 2) == 25);
        ADF_CHECK(adf_modctx_block(ctx, 3) == 49);
        adf_modctx_free(ctx);
    }
    /* n < 2 and e = 0 give K = 1 without blocks */
    {
        adf_modctx_struct * ctx = NULL;
        fmpz_t K;
        fmpz_init(K);
        ADF_CHECK(adf_modctx_new_primorial_pow(&ctx, 1, 5) == ADF_OK);
        adf_modctx_get_modulus(K, ctx);
        ADF_CHECK(fmpz_cmp_ui(K, 1) == 0 && adf_modctx_nblocks(ctx) == 0);
        adf_modctx_free(ctx);
        ctx = NULL;
        ADF_CHECK(adf_modctx_new_primorial_pow(&ctx, 100, 0) == ADF_OK);
        adf_modctx_get_modulus(K, ctx);
        ADF_CHECK(fmpz_cmp_ui(K, 1) == 0 && adf_modctx_nblocks(ctx) == 0);
        adf_modctx_free(ctx);
        ctx = NULL;
        ADF_CHECK(adf_modctx_new_factorial(&ctx, 0) == ADF_OK);
        adf_modctx_get_modulus(K, ctx);
        ADF_CHECK(fmpz_cmp_ui(K, 1) == 0 && adf_modctx_nblocks(ctx) == 0);
        adf_modctx_free(ctx);
        ctx = NULL;
        ADF_CHECK(adf_modctx_new_factorial(&ctx, 1) == ADF_OK);
        adf_modctx_get_modulus(K, ctx);
        ADF_CHECK(fmpz_cmp_ui(K, 1) == 0 && adf_modctx_nblocks(ctx) == 0);
        fmpz_clear(K);
        adf_modctx_free(ctx);
    }
}

/* ---------------------------------------------------------- the descriptor */

ADF_TEST(descriptors_and_matching)
{
    adf_ctx_desc_t d;
    adf_modctx_struct * ctx = NULL;
    ulong q[] = {6, 35, 11};

    adf_ctx_desc_init(&d);
    ADF_CHECK(fmpz_cmp_ui(d.K, 1) == 0);
    ADF_CHECK(d.k == 0);
    ADF_CHECK(d.q == NULL);
    /* the init descriptor matches the context K = 1 without blocks */
    {
        adf_modctx_struct * one = NULL;
        ADF_CHECK(adf_modctx_new_blocks(&one, NULL, 0) == ADF_OK);
        ADF_CHECK(adf_modctx_matches_desc(one, &d) == 1);
        adf_modctx_free(one);
    }

    ADF_CHECK(adf_modctx_new_blocks(&ctx, q, 3) == ADF_OK);
    ADF_CHECK(adf_modctx_matches_desc(ctx, &d) == 0);   /* K = 2310 != 1 */
    fmpz_set_ui(d.K, 2310);
    ADF_CHECK(adf_modctx_matches_desc(ctx, &d) == 0);   /* k = 0 != 3 */
    d.k = 3;
    d.q = flint_malloc(3 * sizeof(ulong));
    d.q[0] = 6;
    d.q[1] = 35;
    d.q[2] = 11;
    ADF_CHECK(adf_modctx_matches_desc(ctx, &d) == 1);
    /* the order of the blocks matters */
    d.q[0] = 11;
    d.q[2] = 6;
    ADF_CHECK(adf_modctx_matches_desc(ctx, &d) == 0);
    d.q[0] = 6;
    d.q[2] = 11;
    ADF_CHECK(adf_modctx_matches_desc(ctx, &d) == 1);
    /* so does the modulus */
    fmpz_set_ui(d.K, 30);
    ADF_CHECK(adf_modctx_matches_desc(ctx, &d) == 0);

    adf_ctx_desc_clear(&d);
    adf_modctx_free(ctx);
}

/* ---------------------------------------------------------- the dump text */

ADF_TEST(dump_text)
{
    struct { ulong q[3]; slong k; const char * text; } cases[] = {
        {{0, 0, 0}, 0, "adf1 Q modctx 1 0"},
        {{2, 0, 0}, 1, "adf1 Q modctx 2 1 2"},
        {{6, 35, 11}, 3, "adf1 Q modctx 906 3 6 23 b"},
        {{65537, 255, 2}, 3, "adf1 Q modctx 1fe01fe 3 10001 ff 2"}
    };
    size_t i;

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
    {
        adf_modctx_struct * ctx = NULL;
        char * s;
        size_t len = 0;

        ADF_CHECK(adf_modctx_new_blocks(&ctx, cases[i].q, cases[i].k) == ADF_OK);
        s = adf_modctx_dump_str(&len, ctx);
        ADF_CHECK_MSG(s != NULL, "dump_str returned NULL");
        ADF_CHECK_MSG(len == strlen(cases[i].text), "dump %u: length %u, expected %u",
                      (unsigned) i, (unsigned) len, (unsigned) strlen(cases[i].text));
        ADF_CHECK_MSG(s != NULL && len == strlen(cases[i].text) &&
                      memcmp(s, cases[i].text, len) == 0,
                      "dump %u: \"%s\" != \"%s\"", (unsigned) i, s == NULL ? "" : s,
                      cases[i].text);
        ADF_CHECK(s[len] == 0);
        flint_free(s);   /* adf_str_free is this flint_free (common.h, conventions 8.1) */
        adf_modctx_free(ctx);
    }
    /* a context without blocks but with a big modulus keeps the modulus in the text */
    {
        adf_modctx_struct * ctx = NULL;
        fmpz_t K;
        char * s;
        size_t len = 0;
        fmpz_init(K);
        fmpz_set_str(K, "340282366920938463463374607431768211457", 10);   /* 2^128 + 1 */
        ADF_CHECK(adf_modctx_new_fmpz(&ctx, K) == ADF_OK);
        s = adf_modctx_dump_str(&len, ctx);
        ADF_CHECK_MSG(s != NULL && len == strlen("adf1 Q modctx 100000000000000000000000000000001 0") &&
                      memcmp(s, "adf1 Q modctx 100000000000000000000000000000001 0", len) == 0,
                      "big-modulus dump: \"%s\"", s == NULL ? "" : s);
        ADF_CHECK(s != NULL && s[len] == 0);
        flint_free(s);
        fmpz_clear(K);
        adf_modctx_free(ctx);
    }
}

/* ------------------------------------------------- random round trips */

/* Build a context of `count` pairwise coprime word blocks from distinct primes and round-trip
   random integers through reduce and combine (docs/proofs/policies.md Lemma 17). */
static void
roundtrip_with_blocks(slong count, flint_rand_t state)
{
    ulong * q = flint_malloc((count > 0 ? count : 1) * sizeof(ulong));
    ulong * res = flint_malloc((count > 0 ? count : 1) * sizeof(ulong));
    adf_modctx_struct * ctx = NULL;
    fmpz_t K, a, back, m;
    slong i, round;

    for (i = 0; i < count; i++)
    {
        static ulong p = 3;
        while (!n_is_prime(p))
            p += 2;
        q[i] = p;
        p += 2;
    }
    ADF_CHECK(adf_modctx_new_blocks(&ctx, q, count) == ADF_OK);
    fmpz_init(K);
    fmpz_init(a);
    fmpz_init(back);
    fmpz_init(m);
    adf_modctx_get_modulus(K, ctx);

    for (round = 0; round < 50; round++)
    {
        /* uniform in [0, K), then two values beyond the modulus */
        fmpz_randm(a, state, K);
        adf_modctx_reduce(res, a, ctx);
        adf_modctx_combine(back, res, ctx);
        ADF_CHECK(fmpz_equal(back, a));
        for (i = 0; i < count; i++)
            ADF_CHECK(res[i] == fmpz_fdiv_ui(a, q[i]));

        fmpz_mul_ui(a, K, (ulong) (round + 1));
        fmpz_add_ui(a, a, (ulong) round);
        adf_modctx_reduce(res, a, ctx);
        adf_modctx_combine(back, res, ctx);
        fmpz_mod(m, a, K);
        ADF_CHECK(fmpz_equal(back, m));

        fmpz_neg(a, a);
        adf_modctx_reduce(res, a, ctx);
        adf_modctx_combine(back, res, ctx);
        fmpz_mod(m, a, K);
        ADF_CHECK(fmpz_equal(back, m));
        for (i = 0; i < count; i++)
            ADF_CHECK(res[i] == fmpz_fdiv_ui(a, q[i]));
    }
    fmpz_clear(K);
    fmpz_clear(a);
    fmpz_clear(back);
    fmpz_clear(m);
    adf_modctx_free(ctx);
    flint_free(q);
    flint_free(res);
}

ADF_TEST(random_round_trips_for_1_2_64_128_blocks)
{
    flint_rand_t state;

    flint_randinit(state);
    flint_randseed(state, 20260928, 1);
    roundtrip_with_blocks(1, state);
    roundtrip_with_blocks(2, state);
    roundtrip_with_blocks(64, state);
    roundtrip_with_blocks(128, state);
    flint_randclear(state);
}

ADF_TEST(conversion_without_blocks)
{
    adf_modctx_struct * ctx = NULL;
    fmpz_t a, back;

    ADF_CHECK(adf_modctx_new_blocks(&ctx, NULL, 0) == ADF_OK);
    fmpz_init(a);
    fmpz_init(back);
    fmpz_set_str(a, "12345678901234567890123456789", 10);
    adf_modctx_reduce(NULL, a, ctx);              /* k = 0: nothing to write */
    fmpz_set_ui(back, 1);                         /* any residue input is ignored */
    adf_modctx_combine(back, NULL, ctx);
    ADF_CHECK(fmpz_is_zero(back));                /* K = 1: only the integer 0 */
    fmpz_clear(a);
    fmpz_clear(back);
    adf_modctx_free(ctx);
}

/* ------------------------------------------------- two threads, one context */

typedef struct
{
    const adf_modctx_struct * ctx;
    ulong expect_k;
    ulong acc;
    int failed;          /* set by the thread; the assertions run in the main thread */
} read_arg;

static void *
thread_read_context(void * p)
{
    read_arg * arg = (read_arg *) p;
    fmpz_t K, a, back;
    ulong res[4];
    int iter;

    fmpz_init(K);
    fmpz_init(a);
    fmpz_init(back);
    for (iter = 0; iter < 500; iter++)
    {
        slong i;
        adf_modctx_get_modulus(K, arg->ctx);
        if (fmpz_sgn(K) <= 0 || (ulong) adf_modctx_nblocks(arg->ctx) != arg->expect_k)
            arg->failed = 1;
        for (i = 0; i < adf_modctx_nblocks(arg->ctx); i++)
            arg->acc = arg->acc * 31 + adf_modctx_block(arg->ctx, i);
        fmpz_set_si(a, iter);
        adf_modctx_reduce(res, a, arg->ctx);
        adf_modctx_combine(back, res, arg->ctx);
        fmpz_mod(a, a, K);
        if (!fmpz_equal(back, a))
            arg->failed = 1;
    }
    fmpz_clear(K);
    fmpz_clear(a);
    fmpz_clear(back);
    return NULL;
}

ADF_TEST(two_threads_read_one_context)
{
    ulong q[] = {2, 3, 5, 7};
    adf_modctx_struct * ctx = NULL;
    pthread_t t1, t2;
    read_arg a1, a2;

    ADF_CHECK(adf_modctx_new_blocks(&ctx, q, 4) == ADF_OK);
    a1.ctx = ctx;
    a1.expect_k = 4;
    a1.acc = 0;
    a1.failed = 0;
    a2 = a1;
    ADF_CHECK(pthread_create(&t1, NULL, thread_read_context, &a1) == 0);
    ADF_CHECK(pthread_create(&t2, NULL, thread_read_context, &a2) == 0);
    ADF_CHECK(pthread_join(t1, NULL) == 0);
    ADF_CHECK(pthread_join(t2, NULL) == 0);
    /* both threads read the same immutable data and produced the same checksum */
    ADF_CHECK(a1.failed == 0 && a2.failed == 0);
    ADF_CHECK(a1.acc == a2.acc);
    adf_modctx_free(ctx);
}

/* ------------------------------------- the prime-power record (conventions 5.14) */

ADF_TEST(prime_power_blocks_record_their_prime)
{
    /* conventions 5.14: for each block the context records whether it is a certified prime
       power and its prime. The prime-power families certify by construction; the block list
       and the fmpz family do not certify anything and record 0. */
    {
        ulong p[] = {3, 2};
        ulong e[] = {1, 1};
        adf_modctx_struct * ctx = NULL;
        ADF_CHECK(adf_modctx_new_prime_powers(&ctx, p, e, 2) == ADF_OK);
        ADF_CHECK(adf_modctx_block_prime(ctx, 0) == 3);
        ADF_CHECK(adf_modctx_block_prime(ctx, 1) == 2);
        adf_modctx_free(ctx);
    }
    {
        ulong q[] = {6, 35, 11};
        adf_modctx_struct * ctx = NULL;
        ADF_CHECK(adf_modctx_new_blocks(&ctx, q, 3) == ADF_OK);
        ADF_CHECK(adf_modctx_block_prime(ctx, 0) == 0);
        ADF_CHECK(adf_modctx_block_prime(ctx, 1) == 0);
        ADF_CHECK(adf_modctx_block_prime(ctx, 2) == 0);
        adf_modctx_free(ctx);
    }
    {
        adf_modctx_struct * ctx = NULL;
        fmpz_t K;
        fmpz_init_set_ui(K, 7);
        ADF_CHECK(adf_modctx_new_fmpz(&ctx, K) == ADF_OK);
        ADF_CHECK(adf_modctx_block_prime(ctx, 0) == 0);
        fmpz_clear(K);
        adf_modctx_free(ctx);
    }
    {
        /* 10! = 2^8 3^4 5^2 7: the primes are certified in the order of the blocks */
        adf_modctx_struct * ctx = NULL;
        ADF_CHECK(adf_modctx_new_factorial(&ctx, 10) == ADF_OK);
        ADF_CHECK(adf_modctx_nblocks(ctx) == 4);
        ADF_CHECK(adf_modctx_block_prime(ctx, 0) == 2);
        ADF_CHECK(adf_modctx_block_prime(ctx, 1) == 3);
        ADF_CHECK(adf_modctx_block_prime(ctx, 2) == 5);
        ADF_CHECK(adf_modctx_block_prime(ctx, 3) == 7);
        adf_modctx_free(ctx);
    }
    {
        /* (product of the primes p <= 13)^3: the blocks are 8, 27, 125, 343, 1331, 2197 */
        adf_modctx_struct * ctx = NULL;
        ADF_CHECK(adf_modctx_new_primorial_pow(&ctx, 13, 3) == ADF_OK);
        ADF_CHECK(adf_modctx_nblocks(ctx) == 6);
        ADF_CHECK(adf_modctx_block_prime(ctx, 0) == 2);
        ADF_CHECK(adf_modctx_block_prime(ctx, 1) == 3);
        ADF_CHECK(adf_modctx_block_prime(ctx, 2) == 5);
        ADF_CHECK(adf_modctx_block_prime(ctx, 3) == 7);
        ADF_CHECK(adf_modctx_block_prime(ctx, 4) == 11);
        ADF_CHECK(adf_modctx_block_prime(ctx, 5) == 13);
        adf_modctx_free(ctx);
    }
}

/* ------------------------------------------------- boundaries of the cap */

ADF_TEST(at_the_cap_the_arrays_are_still_read)
{
    adf_modctx_struct * out = SENTINEL;

    /* k = TEST_MAX_BLOCKS has not exceeded the cap, so the missing arrays are reported
       (modctx.h: "q is read only (it may be NULL when k = 0)"). One above the cap the count
       is refused first (too_many_blocks_are_refused_before_the_list_is_read). */
    ADF_CHECK(adf_modctx_new_blocks(&out, NULL, TEST_MAX_BLOCKS) == ADF_DOMAIN);
    ADF_CHECK(out == SENTINEL);
    ADF_CHECK(adf_modctx_new_prime_powers(&out, NULL, NULL, TEST_MAX_BLOCKS) == ADF_DOMAIN);
    ADF_CHECK(out == SENTINEL);
}

ADF_TEST(a_prime_block_just_below_2_64)
{
    /* 2^64 - 59 is prime (n_is_prime); p^1 fits the word, p^2 does not. */
    ulong p[] = {UWORD(18446744073709551557)};
    ulong e1[] = {1};
    ulong e2[] = {2};
    adf_modctx_struct * out = SENTINEL;

    ADF_CHECK(adf_modctx_new_prime_powers(&out, p, e1, 1) == ADF_OK);
    ADF_CHECK(out != SENTINEL && adf_modctx_block(out, 0) == p[0]);
    ADF_CHECK(adf_modctx_block_prime(out, 0) == p[0]);
    adf_modctx_free(out);
    out = SENTINEL;
    ADF_CHECK(adf_modctx_new_prime_powers(&out, p, e2, 1) == ADF_UNSUPPORTED);
    ADF_CHECK(out == SENTINEL);
}

/* ------------------------------------------------- descriptors at the edges */

ADF_TEST(descriptor_cleared_state_null_blocks_and_block_zero)
{
    adf_ctx_desc_t d;
    adf_modctx_struct * ctx = NULL;
    ulong q[] = {2, 3, 5};

    /* a descriptor that claims blocks without an array never matches: at the same block
       count as the context (so that the guard is reached), and at a different one */
    adf_ctx_desc_init(&d);
    ADF_CHECK(adf_modctx_new_blocks(&ctx, q, 3) == ADF_OK);
    fmpz_set_ui(d.K, 30);
    d.k = 3;
    ADF_CHECK(adf_modctx_matches_desc(ctx, &d) == 0);
    d.k = 1;
    ADF_CHECK(adf_modctx_matches_desc(ctx, &d) == 0);
    d.k = 2;
    ADF_CHECK(adf_modctx_matches_desc(ctx, &d) == 0);
    d.k = 0;
    {
        /* the same guard with a one-block context */
        adf_modctx_struct * one = NULL;
        ulong q1[] = {2};
        ADF_CHECK(adf_modctx_new_blocks(&one, q1, 1) == ADF_OK);
        fmpz_set_ui(d.K, 2);
        d.k = 1;
        ADF_CHECK(adf_modctx_matches_desc(one, &d) == 0);
        fmpz_set_ui(d.K, 30);
        adf_modctx_free(one);
    }

    /* block zero is compared like every other block */
    d.k = 3;
    d.q = flint_malloc(3 * sizeof(ulong));
    d.q[0] = 2;
    d.q[1] = 3;
    d.q[2] = 5;
    ADF_CHECK(adf_modctx_matches_desc(ctx, &d) == 1);
    d.q[0] = 4;                       /* only the first block differs */
    ADF_CHECK(adf_modctx_matches_desc(ctx, &d) == 0);
    d.q[0] = 2;
    ADF_CHECK(adf_modctx_matches_desc(ctx, &d) == 1);

    /* clear releases the owned array and leaves the released state (k = 0, q = NULL) */
    adf_ctx_desc_clear(&d);
    ADF_CHECK(d.k == 0);
    ADF_CHECK(d.q == NULL);

    adf_modctx_free(ctx);
}

/* ------------------------------------------------- the dump of many blocks */

ADF_TEST(dump_of_256_blocks)
{
    /* k = 256 writes its count as the three-digit hexadecimal "100" (conventions 10.1). */
    ulong q[256];
    ulong p = 3;
    char expected[8192];
    size_t pos = 0;
    int n;
    slong i;
    adf_modctx_struct * ctx = NULL;
    char * s;
    size_t len = 0;

    for (i = 0; i < 256; i++)
    {
        while (!n_is_prime(p))
            p += 2;
        q[i] = p;
        p += 2;
    }
    ADF_CHECK(adf_modctx_new_blocks(&ctx, q, 256) == ADF_OK);

    n = snprintf(expected + pos, sizeof(expected) - pos, "adf1 Q modctx ");
    ADF_CHECK(n > 0);
    pos += (size_t) n;
    {
        fmpz_t K;
        char * Kstr;
        fmpz_init(K);
        adf_modctx_get_modulus(K, ctx);
        Kstr = fmpz_get_str(NULL, 16, K);
        n = snprintf(expected + pos, sizeof(expected) - pos, "%s 100", Kstr);
        ADF_CHECK(n > 0);
        pos += (size_t) n;
        flint_free(Kstr);
        fmpz_clear(K);
    }
    for (i = 0; i < 256; i++)
    {
        n = snprintf(expected + pos, sizeof(expected) - pos, " %llx",
                     (unsigned long long) q[i]);
        ADF_CHECK(n > 0);
        pos += (size_t) n;
    }

    s = adf_modctx_dump_str(&len, ctx);
    ADF_CHECK_MSG(s != NULL && len == pos && memcmp(s, expected, pos) == 0,
                  "256-block dump: length %u, expected %u", (unsigned) len,
                  (unsigned) pos);
    ADF_CHECK(s != NULL && s[len] == 0);
    flint_free(s);
    adf_modctx_free(ctx);
}
