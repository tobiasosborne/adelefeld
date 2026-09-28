/* tests/test_fball_local.c: the local backend of adf_fball (work package 1.8, second part).

   Ground truth: docs/proofs/policies.md section 4 (Definition 16, line 305, to Summary 26, line
   541); docs/conventions.md 5.3 and 4.6; include/adelefeld/fball.h and the conversion block of
   include/adelefeld/modctx.h. The oracle for the operations is the global backend: every local
   result is compared, as a set (adf_fball_equal_set) and through its canonical triple, with the
   same operation on the global forms (tests/test_fball.c and tests/test_fball_vectors.c test the
   global backend against the Python reference). The JSON vectors of the Python reference
   tests/ref/adfref/local_ref.py are run by tests/test_fball_local_vectors.c.

   Covered here, besides the vectors: the cases the brief names (A = d = 2 with block 4,
   Proposition 25; (2; 2) in context (4), Proposition 24; the canonical H changing after a sum,
   Summary 26; two context pointers with equal blocks; 1, 2, 64 and 128 blocks); every status with
   the state of the outputs; aliasing; the life cycle of the residue array; operands of thousands
   of bits; and three callers of fball.h (adf_fball_reconstruct, adf_fball_get_str,
   adf_adele_add). */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <flint/ulong_extras.h>

#include <adelefeld.h>

#include "test_runner.h"

/* --------------------------------------------------------------- helpers */

static adf_modctx_struct *
mkctx(const ulong * q, slong k)
{
    adf_modctx_struct * c = NULL;
    ADF_CHECK(adf_modctx_new_blocks(&c, q, k) == ADF_OK);
    return c;
}

static adf_modctx_struct *
mkctx1(ulong q0)
{
    return mkctx(&q0, 1);
}

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

/* x = the local value (d; res) of ctx, written field by field (predicate L of fball.h), without
   any function under test. */
static void
mklocal_raw(adf_fball_t x, const adf_modctx_struct * ctx, const fmpz_t d, const ulong * res)
{
    slong i, k = adf_modctx_nblocks(ctx);

    flint_free(x->res);
    x->res = (ulong *) flint_malloc(k * sizeof(ulong));
    for (i = 0; i < k; i++)
        x->res[i] = res[i];
    fmpz_zero(x->A);
    adf_modctx_get_modulus(x->H, ctx);
    fmpz_set(x->d, d);
    x->backend = ADF_LOCAL;
    x->mctx = ctx;
}

static void
mklocal_si(adf_fball_t x, const adf_modctx_struct * ctx, slong d, const ulong * res)
{
    fmpz_t dd;
    fmpz_init_set_si(dd, d);
    mklocal_raw(x, ctx, dd, res);
    fmpz_clear(dd);
}

/* The canonical triple of x (adf_fball_get_fmpz3) equals (A, H, d). */
static int __attribute__((unused))
triple_is_si(const adf_fball_t x, slong A, slong H, slong d)
{
    fmpz_t a, h, dd;
    int ok;
    fmpz_init(a);
    fmpz_init(h);
    fmpz_init(dd);
    adf_fball_get_fmpz3(a, h, dd, x);
    ok = fmpz_equal_si(a, A) && fmpz_equal_si(h, H) && fmpz_equal_si(dd, d);
    fmpz_clear(a);
    fmpz_clear(h);
    fmpz_clear(dd);
    return ok;
}

/* The stored fields of a global x are (A, H, d). */
static int
fields_are_si(const adf_fball_t x, slong A, slong H, slong d)
{
    return fmpz_equal_si(x->A, A) && fmpz_equal_si(x->H, H) && fmpz_equal_si(x->d, d);
}

/* The CRT lift in [0, K) of the residues of a local x, computed with fmpz_CRT block by block,
   independently of src/modctx.c. */
static void
lift_independent(fmpz_t A, const adf_fball_t x)
{
    fmpz_t M, r, q;
    slong i, k = adf_modctx_nblocks(x->mctx);

    fmpz_init(M);
    fmpz_init(r);
    fmpz_init(q);
    fmpz_zero(A);
    fmpz_one(M);
    for (i = 0; i < k; i++)
    {
        fmpz_set_ui(r, x->res[i]);
        fmpz_set_ui(q, adf_modctx_block(x->mctx, i));
        fmpz_CRT(A, A, M, r, q, 0);
        fmpz_mul(M, M, q);
    }
    fmpz_clear(M);
    fmpz_clear(r);
    fmpz_clear(q);
}

/* g = the global form of the local x, computed from the lift and fmpz gcds, without the
   functions under test: canonical triple of (A0, K, d) by adf_fball_set_fmpz3. */
static void
global_independent(adf_fball_t g, const adf_fball_t x)
{
    fmpz_t A, K;
    fmpz_init(A);
    fmpz_init(K);
    lift_independent(A, x);
    adf_modctx_get_modulus(K, x->mctx);
    ADF_CHECK(adf_fball_set_fmpz3(g, A, K, x->d) == ADF_OK);
    fmpz_clear(A);
    fmpz_clear(K);
}

/* Random residues: a third zero or a multiple of a factor of the block, the rest uniform. */
static void
rand_res(ulong * res, const adf_modctx_struct * ctx, flint_rand_t st)
{
    slong i, k = adf_modctx_nblocks(ctx);
    for (i = 0; i < k; i++)
    {
        ulong q = adf_modctx_block(ctx, i);
        ulong u = n_randint(st, 6);
        if (u == 0)
            res[i] = 0;
        else if (u == 1 && q % 2 == 0)
            res[i] = (2 * n_randint(st, q)) % q;
        else if (u == 2 && q % 3 == 0)
            res[i] = (3 * n_randint(st, q)) % q;
        else
            res[i] = n_randint(st, q);
    }
}

/* Random denominator: small, a product of small primes (often sharing a factor with a block), or
   of `bits` bits. */
static void
rand_den(fmpz_t d, flint_rand_t st, slong bits)
{
    static const ulong small[] = {2, 3, 4, 5, 6, 8, 9, 12, 25, 27, 49, 128};
    ulong u = n_randint(st, 4);
    if (u == 0)
        fmpz_set_ui(d, small[n_randint(st, 12)]);
    else if (u == 1)
    {
        slong j;
        fmpz_one(d);
        for (j = 0; j < 4; j++)
            fmpz_mul_ui(d, d, small[n_randint(st, 12)]);
    }
    else if (u == 2 && bits > 64)
    {
        fmpz_randbits(d, st, bits);
        fmpz_abs(d, d);
        fmpz_add_ui(d, d, 1);
    }
    else
        fmpz_set_ui(d, 1 + n_randint(st, 1000000));
}

/* The contexts of 1, 2, 64 and 128 blocks used by the random tests. */
static adf_modctx_struct *
mkctx_n(slong k)
{
    ulong q[128];
    slong i;
    ulong p = 2;

    if (k == 1)
    {
        q[0] = UWORD(18446744073709551557);      /* 2^64 - 59, prime */
        return mkctx(q, 1);
    }
    if (k == 2)
    {
        q[0] = UWORD(1) << 62;
        q[1] = UWORD(12157665459056928801);      /* 3^40 */
        return mkctx(q, 2);
    }
    /* k = 128: the first 128 primes, every third one squared. k = 64: the first 32 primes, every
       third one squared, then 32 primes above 2^50. */
    for (i = 0; i < k; i++)
    {
        if (k == 64 && i >= 32)
        {
            p = n_nextprime(i == 32 ? UWORD(1) << 50 : q[i - 1], 1);
            q[i] = p;
            continue;
        }
        q[i] = (i % 3 == 0) ? p * p : p;
        p = n_nextprime(p, 1);
    }
    return mkctx(q, k);
}

/* --------------------------------------------------- conversions (modctx.h) */

ADF_TEST(set_local_and_set_global_basic)
{
    adf_modctx_struct * c4 = mkctx1(4);
    adf_fball_t x, y;

    adf_fball_init(x);
    adf_fball_init(y);

    /* 1 + 2 Zhat in the context (4): K/R = 2, c K/R = 2, so (2; 2) (Proposition 19). */
    mkball_si(x, 1, 2, 1);
    ADF_CHECK(!adf_fball_is_local(x));
    ADF_CHECK(adf_fball_context(x) == NULL);
    ADF_CHECK(adf_fball_set_local(y, x, c4) == ADF_OK);
    ADF_CHECK(adf_fball_is_local(y));
    ADF_CHECK(adf_fball_context(y) == c4);
    ADF_CHECK(y->backend == ADF_LOCAL && y->mctx == c4 && y->res != NULL);
    ADF_CHECK(fmpz_is_zero(y->A) && fmpz_equal_si(y->H, 4) && fmpz_equal_si(y->d, 2));
    ADF_CHECK(y->res[0] == 2);
    ADF_CHECK(adf_fball_is_canonical(y));

    /* back to global: the canonical triple (1, 2, 1) (Proposition 24.1) */
    adf_fball_set_global(x, y);
    ADF_CHECK(x->backend == ADF_GLOBAL && x->mctx == NULL && x->res == NULL);
    ADF_CHECK(fields_are_si(x, 1, 2, 1));
    ADF_CHECK(adf_fball_is_canonical(x));

    /* a global input to set_global is copied */
    mkball_si(x, 5, 18, 7);
    adf_fball_set_global(y, x);
    ADF_CHECK(adf_fball_identical(x, y));

    adf_fball_clear(x);
    adf_fball_clear(y);
    adf_modctx_free(c4);
}

/* Proposition 25: A = d = 2 with the block 4. The set (2 + 4 Zhat)/2 = 1 + 2 Zhat; the
   denominator 2 shares a factor with the block, the value is still exact data, and no residue
   modulo 4 of the ball exists. The conversion multiplies and divides d only as an integer. */
ADF_TEST(prop25_denominator_shares_a_factor_with_the_block)
{
    adf_modctx_struct * c4 = mkctx1(4);
    adf_fball_t x, g, y;
    ulong r2 = 2, r6 = 6 % 4;

    adf_fball_init(x);
    adf_fball_init(g);
    adf_fball_init(y);

    mklocal_si(x, c4, 2, &r2);
    ADF_CHECK(adf_fball_is_canonical(x));
    adf_fball_set_global(g, x);
    ADF_CHECK(fields_are_si(g, 1, 2, 1));
    /* both centres 1 and 3 of P25.3 lie in the set */
    {
        adf_rat_t q;
        adf_rat_init(q);
        adf_rat_set_si(q, 1);
        ADF_CHECK(adf_fball_contains_rat(g, q));
        adf_rat_set_si(q, 3);
        ADF_CHECK(adf_fball_contains_rat(g, q));
        adf_rat_clear(q);
    }
    /* the lift 6 of the residue 2 gives the same local value (Lemma 17.1) */
    mkball_si(y, 6, 4, 2);
    ADF_CHECK(adf_fball_set_local(y, y, c4) == ADF_OK);
    ADF_CHECK(y->res[0] == r6 && fmpz_equal_si(y->d, 2));
    ADF_CHECK(adf_fball_identical(x, y));

    adf_fball_clear(x);
    adf_fball_clear(g);
    adf_fball_clear(y);
    adf_modctx_free(c4);
}

ADF_TEST(set_local_statuses_leave_the_output_untouched)
{
    adf_modctx_struct * c4 = mkctx1(4);
    adf_modctx_struct * c0 = NULL;
    adf_fball_t x, y, keep;
    fmpz_t one;
    ulong r3 = 3;

    adf_fball_init(x);
    adf_fball_init(y);
    adf_fball_init(keep);
    fmpz_init_set_ui(one, 1);
    ADF_CHECK(adf_modctx_new_fmpz(&c0, one) == ADF_OK);
    ADF_CHECK(adf_modctx_nblocks(c0) == 0);

    /* 1 + 3 Zhat is not in the context (4): K/R = 4/3 (Proposition 19) */
    mkball_si(x, 1, 3, 1);
    mkball_si(y, 7, 5, 2);
    adf_fball_set(keep, y);
    ADF_CHECK(adf_fball_set_local(y, x, c4) == ADF_DOMAIN);
    ADF_CHECK(adf_fball_identical(y, keep));

    /* (1 + 2 Zhat)/2 into (3): K/R = 3, c K/R = 3/2 */
    {
        adf_modctx_struct * c3 = mkctx1(3);
        mkball_si(x, 1, 2, 2);
        ADF_CHECK(adf_fball_set_local(y, x, c3) == ADF_DOMAIN);
        ADF_CHECK(adf_fball_identical(y, keep));
        adf_modctx_free(c3);
    }

    /* an exact value is never local (Definition 16) */
    mkball_si(x, 3, 0, 2);
    ADF_CHECK(adf_fball_set_local(y, x, c4) == ADF_DOMAIN);
    ADF_CHECK(adf_fball_identical(y, keep));
    adf_fball_zero(x);
    ADF_CHECK(adf_fball_set_local(y, x, c4) == ADF_DOMAIN);
    ADF_CHECK(adf_fball_identical(y, keep));

    /* a context without blocks: UNSUPPORTED, also when the set would be DOMAIN (3.3: maximum) */
    mkball_si(x, 1, 2, 1);
    ADF_CHECK(adf_fball_set_local(y, x, c0) == ADF_UNSUPPORTED);
    ADF_CHECK(adf_fball_identical(y, keep));
    adf_fball_zero(x);
    ADF_CHECK(adf_fball_set_local(y, x, c0) == ADF_UNSUPPORTED);
    ADF_CHECK(adf_fball_identical(y, keep));

    /* a local output that is untouched keeps its residue array */
    mklocal_si(y, c4, 5, &r3);
    adf_fball_set(keep, y);
    mkball_si(x, 1, 3, 1);
    ADF_CHECK(adf_fball_set_local(y, x, c4) == ADF_DOMAIN);
    ADF_CHECK(adf_fball_identical(y, keep));

    adf_fball_clear(x);
    adf_fball_clear(y);
    adf_fball_clear(keep);
    fmpz_clear(one);
    adf_modctx_free(c4);
    adf_modctx_free(c0);
}

ADF_TEST(set_local_enclose_prop20)
{
    adf_modctx_struct * c4 = mkctx1(4);
    adf_modctx_struct * c0 = NULL;
    adf_fball_t x, y, keep, g;
    fmpz_t one;
    int lost;

    adf_fball_init(x);
    adf_fball_init(y);
    adf_fball_init(keep);
    adf_fball_init(g);
    fmpz_init_set_ui(one, 1);
    ADF_CHECK(adf_modctx_new_fmpz(&c0, one) == ADF_OK);

    /* 1/2 + (1/2) Zhat = (1 + Zhat)/2 in (4): K/R = 8, e = 2, d0 = 8, A0 = 4: (8; 0), exact */
    mkball_si(x, 1, 1, 2);
    lost = 7;
    ADF_CHECK(adf_fball_set_local_enclose(y, &lost, x, c4) == ADF_OK);
    ADF_CHECK(lost == 0);
    ADF_CHECK(adf_fball_is_local(y) && adf_fball_context(y) == c4);
    ADF_CHECK(fmpz_equal_si(y->d, 8) && y->res[0] == 0);
    ADF_CHECK(adf_fball_equal_set(x, y));

    /* 1 + 3 Zhat in (4): K/R = 4/3, n = 4, e = 1, d0 = 4, A0 = 4: (4; 0) = 0 + Zhat, lost */
    mkball_si(x, 1, 3, 1);
    lost = 7;
    ADF_CHECK(adf_fball_set_local_enclose(y, &lost, x, c4) == ADF_OK);
    ADF_CHECK(lost == 1);
    ADF_CHECK(fmpz_equal_si(y->d, 4) && y->res[0] == 0);
    adf_fball_set_global(g, y);
    ADF_CHECK(fields_are_si(g, 0, 1, 1));
    ADF_CHECK(adf_fball_contains(x, y));

    /* lost may be NULL */
    ADF_CHECK(adf_fball_set_local_enclose(y, NULL, x, c4) == ADF_OK);
    ADF_CHECK(fmpz_equal_si(y->d, 4) && y->res[0] == 0);

    /* (1 + 2 Zhat)/2 in (3): K/R = 3, e = 2, d0 = 6, A0 = 3: (6; 0) = 1/2 + (1/2) Zhat, lost;
       its canonical triple is (0, 1, 2), since 1/2 lies in (1/2) Z */
    {
        adf_modctx_struct * c3 = mkctx1(3);
        mkball_si(x, 1, 2, 2);
        ADF_CHECK(adf_fball_set_local_enclose(y, &lost, x, c3) == ADF_OK);
        ADF_CHECK(lost == 1 && fmpz_equal_si(y->d, 6) && y->res[0] == 0);
        adf_fball_set_global(g, y);
        ADF_CHECK(fields_are_si(g, 0, 1, 2));
        adf_modctx_free(c3);
    }

    /* statuses: exact input DOMAIN, no block UNSUPPORTED; y and *lost untouched */
    mkball_si(y, 7, 5, 2);
    adf_fball_set(keep, y);
    lost = 7;
    mkball_si(x, 1, 0, 3);
    ADF_CHECK(adf_fball_set_local_enclose(y, &lost, x, c4) == ADF_DOMAIN);
    ADF_CHECK(lost == 7 && adf_fball_identical(y, keep));
    mkball_si(x, 1, 3, 1);
    ADF_CHECK(adf_fball_set_local_enclose(y, &lost, x, c0) == ADF_UNSUPPORTED);
    ADF_CHECK(lost == 7 && adf_fball_identical(y, keep));
    mkball_si(x, 1, 0, 3);
    ADF_CHECK(adf_fball_set_local_enclose(y, &lost, x, c0) == ADF_UNSUPPORTED);
    ADF_CHECK(lost == 7 && adf_fball_identical(y, keep));

    adf_fball_clear(x);
    adf_fball_clear(y);
    adf_fball_clear(keep);
    adf_fball_clear(g);
    fmpz_clear(one);
    adf_modctx_free(c4);
    adf_modctx_free(c0);
}

ADF_TEST(conversions_aliasing_and_other_contexts)
{
    ulong q49[2] = {4, 9};
    adf_modctx_struct * c = mkctx(q49, 2);
    adf_modctx_struct * c2 = mkctx(q49, 2);
    adf_modctx_struct * c36 = mkctx1(36);
    adf_fball_t x, y;
    int lost;

    adf_fball_init(x);
    adf_fball_init(y);

    /* (5 + 36 Zhat)/7 into (4, 9) in place */
    mkball_si(x, 5, 36, 7);
    ADF_CHECK(adf_fball_set_local(x, x, c) == ADF_OK);
    ADF_CHECK(adf_fball_context(x) == c && fmpz_equal_si(x->d, 7));
    ADF_CHECK(x->res[0] == 1 && x->res[1] == 5);
    /* a local value into another context pointer with the same blocks: the same data */
    ADF_CHECK(adf_fball_set_local(y, x, c2) == ADF_OK);
    ADF_CHECK(adf_fball_context(y) == c2 && fmpz_equal_si(y->d, 7));
    ADF_CHECK(y->res[0] == 1 && y->res[1] == 5);
    ADF_CHECK(!adf_fball_identical(x, y));
    ADF_CHECK(adf_fball_equal_set(x, y));
    /* into the one-block context (36) with the same modulus */
    ADF_CHECK(adf_fball_set_local(y, x, c36) == ADF_OK);
    ADF_CHECK(adf_fball_context(y) == c36 && fmpz_equal_si(y->d, 7) && y->res[0] == 5);
    /* in place, local to local */
    ADF_CHECK(adf_fball_set_local(y, y, c) == ADF_OK);
    ADF_CHECK(adf_fball_identical(x, y));
    /* the same context: a copy */
    ADF_CHECK(adf_fball_set_local(y, x, c) == ADF_OK);
    ADF_CHECK(adf_fball_identical(x, y));
    /* enclose in place, local input: nothing lost */
    lost = 7;
    ADF_CHECK(adf_fball_set_local_enclose(y, &lost, y, c36) == ADF_OK);
    ADF_CHECK(lost == 0 && adf_fball_context(y) == c36 && y->res[0] == 5);
    /* set_global in place */
    adf_fball_set_global(x, x);
    ADF_CHECK(!adf_fball_is_local(x) && fields_are_si(x, 5, 36, 7) && x->res == NULL);
    /* a local value into a context where its set does not live: DOMAIN, and via enclose */
    {
        adf_modctx_struct * c8 = mkctx1(8);
        adf_fball_t keep;
        adf_fball_init(keep);
        adf_fball_set(keep, y);
        ADF_CHECK(adf_fball_set_local(y, y, c8) == ADF_DOMAIN);
        ADF_CHECK(adf_fball_identical(y, keep));
        ADF_CHECK(adf_fball_set_local_enclose(y, &lost, y, c8) == ADF_OK);
        ADF_CHECK(lost == 1 && adf_fball_context(y) == c8);
        ADF_CHECK(adf_fball_contains(keep, y));
        adf_fball_clear(y);             /* before its context is freed */
        adf_fball_init(y);
        adf_fball_clear(keep);
        adf_modctx_free(c8);
    }

    adf_fball_clear(x);
    adf_fball_clear(y);
    adf_modctx_free(c);
    adf_modctx_free(c2);
    adf_modctx_free(c36);
}

/* Lemma 17 (both directions keep the set, denominators included) on 1, 2, 64 and 128 blocks,
   with denominators of up to 4096 bits. */
ADF_TEST(round_trips_on_1_2_64_128_blocks)
{
    static const slong ks[4] = {1, 2, 64, 128};
    flint_rand_t st;
    slong t, j;

    flint_randinit(st);
    for (j = 0; j < 4; j++)
    {
        adf_modctx_struct * c = mkctx_n(ks[j]);
        slong k = adf_modctx_nblocks(c);
        ulong * res = (ulong *) flint_malloc(k * sizeof(ulong));
        adf_fball_t x, g, gi, y;
        fmpz_t d, K, t1, t2;

        ADF_CHECK(k == ks[j]);
        adf_fball_init(x);
        adf_fball_init(g);
        adf_fball_init(gi);
        adf_fball_init(y);
        fmpz_init(d);
        fmpz_init(K);
        fmpz_init(t1);
        fmpz_init(t2);
        adf_modctx_get_modulus(K, c);

        for (t = 0; t < 60; t++)
        {
            /* local -> global -> local */
            rand_res(res, c, st);
            rand_den(d, st, t % 5 == 0 ? 4096 : 64);
            mklocal_raw(x, c, d, res);
            ADF_CHECK(adf_fball_is_canonical(x));
            adf_fball_set_global(g, x);
            global_independent(gi, x);
            ADF_CHECK(adf_fball_is_canonical(g));
            ADF_CHECK(adf_fball_identical(g, gi));
            ADF_CHECK(adf_fball_set_local(y, g, c) == ADF_OK);
            ADF_CHECK(adf_fball_identical(x, y));

            /* global -> local or DOMAIN, decided independently by Proposition 19 */
            fmpz_randtest_not_zero(t1, st, t % 7 == 0 ? 4096 : 100);
            fmpz_abs(t1, t1);                         /* H */
            fmpz_randtest(t2, st, 100);               /* A */
            ADF_CHECK(adf_fball_set_fmpz3(g, t2, t1, d) == ADF_OK);
            {
                int st1 = adf_fball_set_local(y, g, c);
                /* K/R = K d/H and c K/R = A K/H (canonical A, H, d of g) */
                fmpz_t u, v;
                int in;
                fmpz_init(u);
                fmpz_init(v);
                fmpz_mul(u, K, g->d);
                fmpz_mul(v, K, g->A);
                in = fmpz_divisible(u, g->H) && fmpz_divisible(v, g->H);
                ADF_CHECK(st1 == (in ? ADF_OK : ADF_DOMAIN));
                if (in)
                {
                    adf_fball_set_global(gi, y);
                    ADF_CHECK(adf_fball_identical(gi, g));
                    fmpz_divexact(u, u, g->H);
                    ADF_CHECK(fmpz_equal(y->d, u));
                }
                else
                {
                    int lost;
                    ADF_CHECK(adf_fball_set_local_enclose(y, &lost, g, c) == ADF_OK);
                    ADF_CHECK(lost == 1);
                    ADF_CHECK(adf_fball_contains(g, y));
                }
                fmpz_clear(u);
                fmpz_clear(v);
            }
        }
        adf_fball_clear(x);
        adf_fball_clear(g);
        adf_fball_clear(gi);
        adf_fball_clear(y);
        fmpz_clear(d);
        fmpz_clear(K);
        fmpz_clear(t1);
        fmpz_clear(t2);
        flint_free(res);
        adf_modctx_free(c);
    }
    flint_randclear(st);
}

/* ------------------------------------------------ life cycle (fball.h) */

ADF_TEST(life_cycle_of_the_residue_array)
{
    ulong q3[3] = {4, 9, 5};
    ulong r3[3] = {3, 7, 2};
    ulong r1 = 5;
    adf_modctx_struct * ca = mkctx1(7);
    adf_modctx_struct * cb = mkctx(q3, 3);
    adf_fball_t x, y, z;
    adf_rat_t q;
    fmpz_t n;

    adf_fball_init(x);
    adf_fball_init(y);
    adf_fball_init(z);
    adf_rat_init(q);
    fmpz_init_set_si(n, -4);

    /* set: global output <- local input: same backend, context, fields, a new array */
    mklocal_si(x, cb, 6, r3);
    adf_fball_set(y, x);
    ADF_CHECK(adf_fball_identical(x, y));
    ADF_CHECK(y->res != NULL && y->res != x->res);
    /* set: a local output of another context (1 block) <- a local input of 3 blocks */
    mklocal_si(z, ca, 1, &r1);
    adf_fball_set(z, x);
    ADF_CHECK(adf_fball_identical(z, x) && z->res != x->res);
    /* set in place */
    adf_fball_set(z, z);
    ADF_CHECK(adf_fball_identical(z, x));
    /* set: local output <- global input: the array is released */
    mkball_si(y, 5, 12, 7);
    adf_fball_set(z, y);
    ADF_CHECK(adf_fball_identical(z, y) && z->res == NULL && z->mctx == NULL);
    /* set: local output of 3 blocks <- local input of 1 block */
    mklocal_si(y, ca, 3, &r1);
    mklocal_si(z, cb, 6, r3);
    adf_fball_set(z, y);
    ADF_CHECK(adf_fball_identical(z, y));

    /* swap: contents and context pointers */
    mklocal_si(x, cb, 6, r3);
    mkball_si(y, 1, 2, 3);
    adf_fball_swap(x, y);
    ADF_CHECK(adf_fball_is_local(y) && adf_fball_context(y) == cb && y->res[2] == 2);
    ADF_CHECK(!adf_fball_is_local(x) && fields_are_si(x, 1, 2, 3) && x->res == NULL);
    adf_fball_swap(x, y);
    ADF_CHECK(adf_fball_is_local(x) && fields_are_si(y, 1, 2, 3));

    /* every global constructor on a local output releases the array and gives predicate G */
    mklocal_si(x, cb, 6, r3);
    adf_fball_zero(x);
    ADF_CHECK(x->res == NULL && adf_fball_is_canonical(x) && fields_are_si(x, 0, 0, 1));
    mklocal_si(x, cb, 6, r3);
    adf_fball_one(x);
    ADF_CHECK(x->res == NULL && adf_fball_is_canonical(x) && fields_are_si(x, 1, 0, 1));
    mklocal_si(x, cb, 6, r3);
    adf_fball_set_si(x, -3);
    ADF_CHECK(x->res == NULL && adf_fball_is_canonical(x) && fields_are_si(x, -3, 0, 1));
    mklocal_si(x, cb, 6, r3);
    adf_fball_set_fmpz(x, n);
    ADF_CHECK(x->res == NULL && adf_fball_is_canonical(x) && fields_are_si(x, -4, 0, 1));
    mklocal_si(x, cb, 6, r3);
    adf_rat_set_si(q, 5);
    adf_fball_set_rat(x, q);
    ADF_CHECK(x->res == NULL && adf_fball_is_canonical(x) && fields_are_si(x, 5, 0, 1));
    mklocal_si(x, cb, 6, r3);
    mkball_si(x, 2, 4, 2);
    ADF_CHECK(x->res == NULL && adf_fball_is_canonical(x) && fields_are_si(x, 1, 2, 1));
    mklocal_si(x, cb, 6, r3);
    ADF_CHECK(adf_fball_set_center_radius(x, q, q) == ADF_OK);
    ADF_CHECK(x->res == NULL && adf_fball_is_canonical(x) && fields_are_si(x, 0, 5, 1));

    /* canonicalise of a local value: OK, nothing changes (raw data is the stored form) */
    mklocal_si(x, cb, 6, r3);
    adf_fball_set(y, x);
    ADF_CHECK(adf_fball_canonicalise(x) == ADF_OK);
    ADF_CHECK(adf_fball_identical(x, y));

    adf_fball_clear(x);
    adf_fball_clear(y);
    adf_fball_clear(z);
    adf_rat_clear(q);
    fmpz_clear(n);
    adf_modctx_free(ca);
    adf_modctx_free(cb);
}

ADF_TEST(is_canonical_predicate_L)
{
    ulong q2[2] = {4, 9};
    ulong r[2] = {3, 8};
    adf_modctx_struct * c = mkctx(q2, 2);
    adf_fball_t x;

    adf_fball_init(x);
    mklocal_si(x, c, 6, r);
    ADF_CHECK(adf_fball_is_canonical(x));

    /* raw data: no gcd condition (CV-55); here gcd(A, K, d) = gcd(18, 36, 6) = 6 */
    r[0] = 2;
    r[1] = 0;
    mklocal_si(x, c, 6, r);
    ADF_CHECK(adf_fball_is_canonical(x));

    /* a residue at or above its block */
    x->res[0] = 4;
    ADF_CHECK(!adf_fball_is_canonical(x));
    x->res[0] = 3;
    x->res[1] = 9;
    ADF_CHECK(!adf_fball_is_canonical(x));
    x->res[1] = UWORD_MAX;
    ADF_CHECK(!adf_fball_is_canonical(x));
    x->res[1] = 8;
    ADF_CHECK(adf_fball_is_canonical(x));

    /* H != K */
    fmpz_set_si(x->H, 18);
    ADF_CHECK(!adf_fball_is_canonical(x));
    fmpz_set_si(x->H, 72);
    ADF_CHECK(!adf_fball_is_canonical(x));
    fmpz_set_si(x->H, 36);
    ADF_CHECK(adf_fball_is_canonical(x));

    /* A != 0, d < 1 */
    fmpz_set_si(x->A, 1);
    ADF_CHECK(!adf_fball_is_canonical(x));
    fmpz_set_si(x->A, 0);
    fmpz_set_si(x->d, 0);
    ADF_CHECK(!adf_fball_is_canonical(x));
    fmpz_set_si(x->d, -6);
    ADF_CHECK(!adf_fball_is_canonical(x));
    fmpz_set_si(x->d, 1);
    ADF_CHECK(adf_fball_is_canonical(x));

    /* a backend tag that is neither global nor local */
    x->backend = 2;
    ADF_CHECK(!adf_fball_is_canonical(x));
    x->backend = ADF_LOCAL;
    ADF_CHECK(adf_fball_is_canonical(x));

    adf_fball_clear(x);
    adf_modctx_free(c);
}

ADF_TEST(identical_compares_raw_data)
{
    ulong q2[2] = {4, 9};
    ulong r[2] = {3, 8};
    adf_modctx_struct * c = mkctx(q2, 2);
    adf_modctx_struct * c2 = mkctx(q2, 2);
    adf_fball_t x, y, g;

    adf_fball_init(x);
    adf_fball_init(y);
    adf_fball_init(g);
    mklocal_si(x, c, 6, r);
    mklocal_si(y, c, 6, r);
    ADF_CHECK(adf_fball_identical(x, y));
    y->res[1] = 7;
    ADF_CHECK(!adf_fball_identical(x, y));
    y->res[1] = 8;
    y->res[0] = 1;
    ADF_CHECK(!adf_fball_identical(x, y));
    y->res[0] = 3;
    fmpz_set_si(y->d, 12);
    ADF_CHECK(!adf_fball_identical(x, y));
    /* equal blocks, another context pointer */
    mklocal_si(y, c2, 6, r);
    ADF_CHECK(!adf_fball_identical(x, y));
    ADF_CHECK(adf_fball_equal_set(x, y));
    /* a local value and its global form: the same set, not identical */
    adf_fball_set_global(g, x);
    ADF_CHECK(!adf_fball_identical(x, g) && !adf_fball_identical(g, x));
    ADF_CHECK(adf_fball_equal_set(x, g) && adf_fball_equal_set(g, x));

    adf_fball_clear(x);
    adf_fball_clear(y);
    adf_fball_clear(g);
    adf_modctx_free(c);
    adf_modctx_free(c2);
}

/* -------------------------------------------------------- accessors */

/* Proposition 24: (2; 2) in (4). The set 1 + 2 Zhat stays a local value of (4); the canonical
   triple (1, 2, 1) has the modulus 2 and is what every accessor reports. */
ADF_TEST(prop24_cancellation_keeps_the_set_not_the_triple)
{
    adf_modctx_struct * c4 = mkctx1(4);
    adf_fball_t x, g;
    adf_rat_t r;
    fmpz_t d;
    adf_place_t v;
    slong e = 99;
    ulong r2 = 2;

    adf_fball_init(x);
    adf_fball_init(g);
    adf_rat_init(r);
    fmpz_init(d);

    mklocal_si(x, c4, 2, &r2);
    mkball_si(g, 1, 2, 1);
    ADF_CHECK(adf_fball_is_canonical(x));
    ADF_CHECK(triple_is_si(x, 1, 2, 1));
    ADF_CHECK(adf_fball_equal_set(x, g) && !adf_fball_identical(x, g));
    adf_fball_get_center(r, x);
    ADF_CHECK(fmpq_equal_si(r->q, 1));
    adf_fball_get_radius(r, x);
    ADF_CHECK(fmpq_equal_si(r->q, 2));
    adf_fball_get_den(d, x);
    ADF_CHECK(fmpz_equal_si(d, 1));
    ADF_CHECK(!adf_fball_is_exact(x));
    adf_fball_haar_volume(r, x);
    ADF_CHECK(fmpz_equal_si(fmpq_numref(r->q), 1) && fmpz_equal_si(fmpq_denref(r->q), 2));
    ADF_CHECK(adf_place_prime(&v, 2) == ADF_OK);
    ADF_CHECK(adf_fball_prec_at(&e, x, v) == ADF_OK && e == 1);
    ADF_CHECK(adf_place_prime(&v, 3) == ADF_OK);
    ADF_CHECK(adf_fball_prec_at(&e, x, v) == ADF_OK && e == 0);
    ADF_CHECK(adf_fball_prec_at(&e, x, adf_place_inf()) == ADF_DOMAIN && e == 0);
    /* the storage stays raw: nothing above changed x */
    ADF_CHECK(adf_fball_is_local(x) && fmpz_equal_si(x->d, 2) && x->res[0] == 2);

    adf_fball_clear(x);
    adf_fball_clear(g);
    adf_rat_clear(r);
    fmpz_clear(d);
    adf_modctx_free(c4);
}

/* Every accessor of a local value equals the accessor of its global form, on random values. */
ADF_TEST(accessors_agree_with_the_global_form)
{
    static const slong ks[4] = {1, 2, 64, 128};
    static const ulong ps[5] = {2, 3, 5, 7, 1000003};
    flint_rand_t st;
    slong j, t;

    flint_randinit(st);
    for (j = 0; j < 4; j++)
    {
        adf_modctx_struct * c = mkctx_n(ks[j]);
        slong k = adf_modctx_nblocks(c), i;
        ulong * res = (ulong *) flint_malloc(k * sizeof(ulong));
        adf_fball_t x, g;
        adf_rat_t r1, r2;
        fmpz_t d, A1, H1, d1;

        adf_fball_init(x);
        adf_fball_init(g);
        adf_rat_init(r1);
        adf_rat_init(r2);
        fmpz_init(d);
        fmpz_init(A1);
        fmpz_init(H1);
        fmpz_init(d1);
        for (t = 0; t < 40; t++)
        {
            rand_res(res, c, st);
            rand_den(d, st, t % 4 == 0 ? 4096 : 64);
            mklocal_raw(x, c, d, res);
            global_independent(g, x);
            adf_fball_get_fmpz3(A1, H1, d1, x);
            ADF_CHECK(fmpz_equal(A1, g->A) && fmpz_equal(H1, g->H) && fmpz_equal(d1, g->d));
            adf_fball_get_center(r1, x);
            adf_fball_get_center(r2, g);
            ADF_CHECK(adf_rat_equal(r1, r2));
            adf_fball_get_radius(r1, x);
            adf_fball_get_radius(r2, g);
            ADF_CHECK(adf_rat_equal(r1, r2));
            adf_fball_get_den(d1, x);
            ADF_CHECK(fmpz_equal(d1, g->d));
            adf_fball_haar_volume(r1, x);
            adf_fball_haar_volume(r2, g);
            ADF_CHECK(adf_rat_equal(r1, r2));
            ADF_CHECK(!adf_fball_is_exact(x));
            for (i = 0; i < 5; i++)
            {
                adf_place_t v;
                slong e1 = 0, e2 = 1;
                ADF_CHECK(adf_place_prime(&v, ps[i]) == ADF_OK);
                ADF_CHECK(adf_fball_prec_at(&e1, x, v) == ADF_OK);
                ADF_CHECK(adf_fball_prec_at(&e2, g, v) == ADF_OK);
                ADF_CHECK(e1 == e2);
            }
            /* the raw storage is untouched and still satisfies L */
            ADF_CHECK(adf_fball_is_canonical(x) && adf_fball_is_local(x) && fmpz_equal(x->d, d));
        }
        adf_fball_clear(x);
        adf_fball_clear(g);
        adf_rat_clear(r1);
        adf_rat_clear(r2);
        fmpz_clear(d);
        fmpz_clear(A1);
        fmpz_clear(H1);
        fmpz_clear(d1);
        flint_free(res);
        adf_modctx_free(c);
    }
    flint_randclear(st);
}

/* ------------------------------------------------------- predicates */

ADF_TEST(predicates_through_the_canonical_triple)
{
    ulong q2[2] = {4, 9};
    adf_modctx_struct * c = mkctx(q2, 2);
    adf_modctx_struct * c2 = mkctx(q2, 2);
    adf_fball_t x, y, g, h;
    adf_rat_t q;
    ulong r[2];

    adf_fball_init(x);
    adf_fball_init(y);
    adf_fball_init(g);
    adf_fball_init(h);
    adf_rat_init(q);

    /* x = (5 + 36 Zhat)/6 in (4, 9), y = (5 + 36 Zhat)/3 in another context with the same blocks;
       the global forms are the oracle */
    r[0] = 1;
    r[1] = 5;
    mklocal_si(x, c, 6, r);
    mklocal_si(y, c2, 3, r);
    adf_fball_set_global(g, x);
    adf_fball_set_global(h, y);
    ADF_CHECK(adf_fball_equal_set(x, y) == adf_fball_equal_set(g, h));
    ADF_CHECK(adf_fball_overlaps(x, y) == adf_fball_overlaps(g, h));
    ADF_CHECK(adf_fball_contains(x, y) == adf_fball_contains(g, h));
    ADF_CHECK(adf_fball_contains(y, x) == adf_fball_contains(h, g));
    ADF_CHECK(adf_fball_compare(x, y) == adf_fball_compare(g, h));

    /* (5 + 36 Zhat)/6 = 5/6 + 6 Zhat contains 5/6 + 12 and not 1/6 */
    fmpq_set_si(q->q, 5 + 72, 6);
    ADF_CHECK(adf_fball_contains_rat(x, q));
    fmpq_set_si(q->q, 1, 6);
    ADF_CHECK(!adf_fball_contains_rat(x, q));

    /* 5/6 + 6 Zhat is inside (10 + 36 Zhat)/12 = 5/6 + 3 Zhat, not conversely */
    r[0] = 10 % 4;
    r[1] = 10 % 9;
    mklocal_si(y, c, 12, r);
    ADF_CHECK(adf_fball_contains(x, y));
    ADF_CHECK(!adf_fball_contains(y, x));
    ADF_CHECK(adf_fball_overlaps(x, y) && adf_fball_overlaps(y, x));
    ADF_CHECK(adf_fball_compare(x, y) == ADF_CMP_UNDECIDED);
    ADF_CHECK(!adf_fball_equal_set(x, y));

    /* disjoint: 5/6 + 6 Zhat and 1/6 + 6 Zhat */
    r[0] = 1;
    r[1] = 1;
    mklocal_si(y, c, 6, r);
    ADF_CHECK(!adf_fball_overlaps(x, y));
    ADF_CHECK(adf_fball_compare(x, y) == ADF_CMP_DIFFERENT);
    ADF_CHECK(adf_fball_compare(y, x) == ADF_CMP_DIFFERENT);

    /* a local value against exact global points */
    mkball_si(g, 5 + 36, 0, 6);
    ADF_CHECK(adf_fball_contains(g, x) && !adf_fball_contains(x, g));
    ADF_CHECK(adf_fball_overlaps(x, g) && adf_fball_compare(x, g) == ADF_CMP_UNDECIDED);
    mkball_si(g, 1, 0, 6);
    ADF_CHECK(!adf_fball_overlaps(g, x) && adf_fball_compare(g, x) == ADF_CMP_DIFFERENT);

    adf_fball_clear(x);
    adf_fball_clear(y);
    adf_fball_clear(g);
    adf_fball_clear(h);
    adf_rat_clear(q);
    adf_modctx_free(c);
    adf_modctx_free(c2);
}

/* ------------------------------------------------------- arithmetic */

/* Summary 26: (1 + 2 Zhat)/2 + (1 + 2 Zhat)/2 has the raw form (2; 0) in (2) and the canonical
   triple (0, 1, 1): the canonical H goes from 2 to 1, the value stays local. */
ADF_TEST(canonical_H_changes_after_a_sum)
{
    adf_modctx_struct * c2 = mkctx1(2);
    adf_fball_t x, z;
    ulong r1 = 1;

    adf_fball_init(x);
    adf_fball_init(z);
    mklocal_si(x, c2, 2, &r1);
    ADF_CHECK(triple_is_si(x, 1, 2, 2));
    adf_fball_add(z, x, x);
    ADF_CHECK(adf_fball_is_local(z) && adf_fball_context(z) == c2);
    ADF_CHECK(fmpz_equal_si(z->d, 2) && z->res[0] == 0 && fmpz_equal_si(z->H, 2));
    ADF_CHECK(adf_fball_is_canonical(z));
    ADF_CHECK(triple_is_si(z, 0, 1, 1));
    /* in place */
    adf_fball_add(x, x, x);
    ADF_CHECK(adf_fball_identical(x, z));

    adf_fball_clear(x);
    adf_fball_clear(z);
    adf_modctx_free(c2);
}

ADF_TEST(products_of_proposition_22)
{
    adf_modctx_struct * c4 = mkctx1(4);
    adf_modctx_struct * c6 = mkctx1(6);
    adf_fball_t x, y, z;
    ulong r;

    adf_fball_init(x);
    adf_fball_init(y);
    adf_fball_init(z);

    /* (4), x = y = 2 + 4 Zhat: h = 2 does not divide d e = 1: the tight 4 + 8 Zhat, global */
    r = 2;
    mklocal_si(x, c4, 1, &r);
    adf_fball_mul(z, x, x);
    ADF_CHECK(!adf_fball_is_local(z) && z->res == NULL && fields_are_si(z, 4, 8, 1));
    /* (2; 2) squared: h = 2 divides d e = 4: local (2; 2) = 1 + 2 Zhat */
    mklocal_si(x, c4, 2, &r);
    adf_fball_mul(z, x, x);
    ADF_CHECK(adf_fball_is_local(z) && fmpz_equal_si(z->d, 2) && z->res[0] == 2);
    ADF_CHECK(triple_is_si(z, 1, 2, 1));
    /* Summary 26: (1 + 6 Zhat)/2 times (2 + 6 Zhat)/3, h = 1: raw (6; 2), canonical (1, 3, 3) */
    r = 1;
    mklocal_si(x, c6, 2, &r);
    r = 2;
    mklocal_si(y, c6, 3, &r);
    adf_fball_mul(z, x, y);
    ADF_CHECK(adf_fball_is_local(z) && fmpz_equal_si(z->d, 6) && z->res[0] == 2);
    ADF_CHECK(triple_is_si(z, 1, 3, 3));

    adf_fball_clear(x);
    adf_fball_clear(y);
    adf_fball_clear(z);
    adf_modctx_free(c4);
    adf_modctx_free(c6);
}

ADF_TEST(scalars_of_proposition_23)
{
    ulong q2[2] = {4, 9};
    ulong r[2] = {3, 7};
    adf_modctx_struct * c = mkctx(q2, 2);
    adf_fball_t x, z, keep, g, e;
    adf_rat_t q;

    adf_fball_init(x);
    adf_fball_init(z);
    adf_fball_init(keep);
    adf_fball_init(g);
    adf_fball_init(e);
    adf_rat_init(q);

    mklocal_si(x, c, 6, r);
    adf_fball_set_global(g, x);

    /* q = -2/5: |m| = 2 divides d = 6: local (15; -r mod q) */
    fmpq_set_si(q->q, -2, 5);
    adf_fball_mul_rat(z, x, q);
    ADF_CHECK(adf_fball_is_local(z) && adf_fball_context(z) == c && fmpz_equal_si(z->d, 15));
    ADF_CHECK(adf_fball_is_local(z) && z->res[0] == 1 && z->res[1] == 2);
    adf_fball_mul_rat(e, g, q);
    ADF_CHECK(adf_fball_equal_set(z, e));
    /* q = 4: 4 does not divide 6: global */
    fmpq_set_si(q->q, 4, 1);
    adf_fball_mul_rat(z, x, q);
    adf_fball_mul_rat(e, g, q);
    ADF_CHECK(!adf_fball_is_local(z) && adf_fball_identical(z, e));
    /* q = 0: the exact 0, global */
    fmpq_zero(q->q);
    adf_fball_mul_rat(z, x, q);
    ADF_CHECK(!adf_fball_is_local(z) && z->res == NULL && fields_are_si(z, 0, 0, 1));

    /* div_rat by 3/2 = mul_rat by 2/3: 2 divides 6: local (9; r) */
    fmpq_set_si(q->q, 3, 2);
    ADF_CHECK(adf_fball_div_rat(z, x, q) == ADF_OK);
    ADF_CHECK(adf_fball_is_local(z) && fmpz_equal_si(z->d, 9) && z->res[0] == 3 && z->res[1] == 7);
    /* div_rat by 1/4 = mul_rat by 4: global */
    fmpq_set_si(q->q, 1, 4);
    ADF_CHECK(adf_fball_div_rat(z, x, q) == ADF_OK);
    fmpq_set_si(q->q, 4, 1);
    adf_fball_mul_rat(e, g, q);
    ADF_CHECK(!adf_fball_is_local(z) && adf_fball_identical(z, e));
    /* div_rat by 0: NOT_UNIT, a local output untouched */
    adf_fball_set(z, x);
    adf_fball_set(keep, z);
    fmpq_zero(q->q);
    ADF_CHECK(adf_fball_div_rat(z, x, q) == ADF_NOT_UNIT);
    ADF_CHECK(adf_fball_identical(z, keep));
    ADF_CHECK(adf_fball_div_rat(z, z, q) == ADF_NOT_UNIT);
    ADF_CHECK(adf_fball_identical(z, keep));
    /* in place */
    fmpq_set_si(q->q, -1, 1);
    adf_fball_mul_rat(z, z, q);
    adf_fball_neg(e, x);
    ADF_CHECK(adf_fball_identical(z, e));
    ADF_CHECK(adf_fball_div_rat(z, z, q) == ADF_OK);
    ADF_CHECK(adf_fball_identical(z, x));

    adf_fball_clear(x);
    adf_fball_clear(z);
    adf_fball_clear(keep);
    adf_fball_clear(g);
    adf_fball_clear(e);
    adf_rat_clear(q);
    adf_modctx_free(c);
}

/* Two context pointers with equal blocks, and mixed backends: global results (conventions 4.6,
   5.3; gate finding G1). */
ADF_TEST(different_context_pointers_give_global_results)
{
    ulong q2[2] = {4, 9};
    ulong r[2] = {3, 7}, s[2] = {2, 4};
    adf_modctx_struct * c = mkctx(q2, 2);
    adf_modctx_struct * c2 = mkctx(q2, 2);
    adf_fball_t x, y, g, h, z, e;

    adf_fball_init(x);
    adf_fball_init(y);
    adf_fball_init(g);
    adf_fball_init(h);
    adf_fball_init(z);
    adf_fball_init(e);
    mklocal_si(x, c, 6, r);
    mklocal_si(y, c2, 10, s);
    adf_fball_set_global(g, x);
    adf_fball_set_global(h, y);

    adf_fball_add(z, x, y);
    adf_fball_add(e, g, h);
    ADF_CHECK(!adf_fball_is_local(z) && adf_fball_identical(z, e));
    adf_fball_sub(z, x, y);
    adf_fball_sub(e, g, h);
    ADF_CHECK(!adf_fball_is_local(z) && adf_fball_identical(z, e));
    adf_fball_mul(z, x, y);
    adf_fball_mul(e, g, h);
    ADF_CHECK(!adf_fball_is_local(z) && adf_fball_identical(z, e));

    /* mixed: local and global, both orders */
    adf_fball_add(z, x, h);
    adf_fball_add(e, g, h);
    ADF_CHECK(!adf_fball_is_local(z) && adf_fball_identical(z, e));
    adf_fball_mul(z, h, x);
    adf_fball_mul(e, h, g);
    ADF_CHECK(!adf_fball_is_local(z) && adf_fball_identical(z, e));
    adf_fball_sub(z, h, x);
    adf_fball_sub(e, h, g);
    ADF_CHECK(!adf_fball_is_local(z) && adf_fball_identical(z, e));

    /* the same pointer: local */
    mklocal_si(y, c, 10, s);
    adf_fball_add(z, x, y);
    ADF_CHECK(adf_fball_is_local(z) && adf_fball_context(z) == c && fmpz_equal_si(z->d, 30));

    adf_fball_clear(x);
    adf_fball_clear(y);
    adf_fball_clear(g);
    adf_fball_clear(h);
    adf_fball_clear(z);
    adf_fball_clear(e);
    adf_modctx_free(c);
    adf_modctx_free(c2);
}

typedef void (*binop_t)(adf_fball_t, const adf_fball_t, const adf_fball_t);

/* h = gcd(A, B, K) (Proposition 22.1), from independent lifts. */
static void
h_independent(fmpz_t h, const adf_fball_t x, const adf_fball_t y)
{
    fmpz_t A, B, K;
    fmpz_init(A);
    fmpz_init(B);
    fmpz_init(K);
    lift_independent(A, x);
    lift_independent(B, y);
    adf_modctx_get_modulus(K, x->mctx);
    fmpz_gcd3(h, A, B, K);
    fmpz_clear(A);
    fmpz_clear(B);
    fmpz_clear(K);
}

/* Whether the tight product of x and y stays local: h | d e (Proposition 22.3). */
static int
product_stays_local(const adf_fball_t x, const adf_fball_t y)
{
    fmpz_t h, de;
    int r;
    fmpz_init(h);
    fmpz_init(de);
    h_independent(h, x, y);
    fmpz_mul(de, x->d, y->d);
    r = fmpz_divisible(de, h);
    fmpz_clear(h);
    fmpz_clear(de);
    return r;
}

/* Every local operation against the same operation on the global forms, on 1, 2, 64 and 128
   blocks, with every permitted aliasing (conventions 4.1): (z, x, y), (x, x, y), (y, x, y),
   (x, x, x). The expected backend: add, sub, neg local; mul local exactly when h | d e
   (Proposition 22.3); mul_rat local exactly when |m| divides d (Proposition 23). */
ADF_TEST(operations_against_the_global_forms)
{
    static const slong ks[4] = {1, 2, 64, 128};
    static const binop_t ops[3] = {adf_fball_add, adf_fball_sub, adf_fball_mul};
    static const slong ms[6] = {1, -1, 2, -3, 12, 7};
    static const ulong ns[6] = {1, 1, 5, 1, 7, 2};
    flint_rand_t st;
    slong j, t, o, a, i;
    slong nlocal_mul = 0, nglobal_mul = 0;

    flint_randinit(st);
    for (j = 0; j < 4; j++)
    {
        adf_modctx_struct * c = mkctx_n(ks[j]);
        slong k = adf_modctx_nblocks(c);
        ulong * res = (ulong *) flint_malloc(k * sizeof(ulong));
        adf_fball_t x, y, g, h, z, e, x2, y2;
        fmpz_t d;
        adf_rat_t q;

        adf_fball_init(x);
        adf_fball_init(y);
        adf_fball_init(g);
        adf_fball_init(h);
        adf_fball_init(z);
        adf_fball_init(e);
        adf_fball_init(x2);
        adf_fball_init(y2);
        fmpz_init(d);
        adf_rat_init(q);
        for (t = 0; t < 30; t++)
        {
            rand_res(res, c, st);
            rand_den(d, st, t % 6 == 0 ? 4096 : 64);
            mklocal_raw(x, c, d, res);
            if (t % 3 == 0)
            {
                /* y shares factors with x blockwise: s_i = 2 r_i or 3 r_i mod q_i */
                for (i = 0; i < k; i++)
                {
                    ulong qi = adf_modctx_block(c, i);
                    res[i] = n_mulmod2_preinv(res[i], 2 + (ulong) (t % 2), qi, n_preinvert_limb(qi));
                }
            }
            else
                rand_res(res, c, st);
            rand_den(d, st, 64);
            mklocal_raw(y, c, d, res);
            adf_fball_set_global(g, x);
            adf_fball_set_global(h, y);

            for (o = 0; o < 3; o++)
            {
                for (a = 0; a < 4; a++)
                {
                    int want_local;
                    adf_fball_set(x2, x);
                    adf_fball_set(y2, y);
                    if (a == 0)
                    {
                        ops[o](z, x2, y2);
                        ops[o](e, g, h);
                        want_local = (o < 2) || product_stays_local(x, y);
                    }
                    else if (a == 1)
                    {
                        ops[o](x2, x2, y2);
                        adf_fball_swap(z, x2);
                        ops[o](e, g, h);
                        want_local = (o < 2) || product_stays_local(x, y);
                    }
                    else if (a == 2)
                    {
                        ops[o](y2, x2, y2);
                        adf_fball_swap(z, y2);
                        ops[o](e, g, h);
                        want_local = (o < 2) || product_stays_local(x, y);
                    }
                    else
                    {
                        ops[o](x2, x2, x2);
                        adf_fball_swap(z, x2);
                        ops[o](e, g, g);
                        want_local = (o < 2) || product_stays_local(x, x);
                    }
                    ADF_CHECK(adf_fball_is_canonical(z));
                    ADF_CHECK(adf_fball_equal_set(z, e));
                    ADF_CHECK_MSG(adf_fball_is_local(z) == want_local, "op %ld aliasing %ld blocks %ld",
                                  (long) o, (long) a, (long) k);
                    if (adf_fball_is_local(z))
                        ADF_CHECK(adf_fball_context(z) == c);
                    else
                        ADF_CHECK(adf_fball_identical(z, e));
                    if (o == 2 && a == 0)
                    {
                        if (adf_fball_is_local(z))
                            nlocal_mul++;
                        else
                            nglobal_mul++;
                    }
                }
            }

            /* negation, in place and not */
            adf_fball_neg(z, x);
            adf_fball_neg(e, g);
            ADF_CHECK(adf_fball_is_local(z) && adf_fball_equal_set(z, e));
            adf_fball_set(x2, x);
            adf_fball_neg(x2, x2);
            ADF_CHECK(adf_fball_identical(x2, z));

            /* exact scalars */
            for (i = 0; i < 6; i++)
            {
                fmpq_set_si(q->q, ms[i], ns[i]);
                adf_fball_mul_rat(z, x, q);
                adf_fball_mul_rat(e, g, q);
                ADF_CHECK(adf_fball_equal_set(z, e));
                ADF_CHECK(adf_fball_is_local(z) == fmpz_divisible_si(x->d, ms[i] < 0 ? -ms[i] : ms[i]));
                ADF_CHECK(adf_fball_is_canonical(z));
            }
        }
        adf_fball_clear(x);
        adf_fball_clear(y);
        adf_fball_clear(g);
        adf_fball_clear(h);
        adf_fball_clear(z);
        adf_fball_clear(e);
        adf_fball_clear(x2);
        adf_fball_clear(y2);
        fmpz_clear(d);
        adf_rat_clear(q);
        flint_free(res);
        adf_modctx_free(c);
    }
    /* both branches of the product rule were reached */
    ADF_CHECK(nlocal_mul > 0 && nglobal_mul > 0);
    flint_randclear(st);
}

/* ----------------------------------------------------------- callers */

/* src/recon.c reads the ball through adf_fball_get_fmpz3; a local ball gives the answer of its
   global form. */
ADF_TEST(caller_reconstruct_with_a_local_ball)
{
    ulong q2[2] = {4, 9};
    ulong r[2];
    adf_modctx_struct * c = mkctx(q2, 2);
    adf_fball_t x, g;
    adf_rat_t lo, hi, q1, q2r;
    int s1, s2;

    adf_fball_init(x);
    adf_fball_init(g);
    adf_rat_init(lo);
    adf_rat_init(hi);
    adf_rat_init(q1);
    adf_rat_init(q2r);

    /* (10 + 36 Zhat)/4 = 5/2 + 9 Zhat: raw d = 4, canonical (5, 18, 2); [0, 5] holds 5/2 only */
    r[0] = 10 % 4;
    r[1] = 10 % 9;
    mklocal_si(x, c, 4, r);
    adf_fball_set_global(g, x);
    ADF_CHECK(fields_are_si(g, 5, 18, 2));
    adf_rat_set_si(lo, 0);
    adf_rat_set_si(hi, 5);
    s1 = adf_fball_reconstruct(q1, x, lo, hi);
    s2 = adf_fball_reconstruct(q2r, g, lo, hi);
    ADF_CHECK(s1 == ADF_OK && s2 == ADF_OK);
    ADF_CHECK(adf_rat_equal(q1, q2r));
    ADF_CHECK(fmpz_equal_si(fmpq_numref(q1->q), 5) && fmpz_equal_si(fmpq_denref(q1->q), 2));
    /* [0, 20]: 5/2 and 23/2: NOT_UNIQUE for both */
    adf_rat_set_si(hi, 20);
    s1 = adf_fball_reconstruct(q1, x, lo, hi);
    s2 = adf_fball_reconstruct(q2r, g, lo, hi);
    ADF_CHECK(s1 == s2 && s1 == ADF_NOT_UNIQUE);
    /* [3, 11]: none */
    adf_rat_set_si(lo, 3);
    adf_rat_set_si(hi, 11);
    s1 = adf_fball_reconstruct(q1, x, lo, hi);
    s2 = adf_fball_reconstruct(q2r, g, lo, hi);
    ADF_CHECK(s1 == s2 && s1 == ADF_NO_SOLUTION);

    adf_fball_clear(x);
    adf_fball_clear(g);
    adf_rat_clear(lo);
    adf_rat_clear(hi);
    adf_rat_clear(q1);
    adf_rat_clear(q2r);
    adf_modctx_free(c);
}

/* src/text.c prints the canonical triple through adf_fball_get_fmpz3 (conventions 5.3: the value
   form prints the canonical triple and does not show the backend). */
ADF_TEST(caller_printer_with_a_local_ball)
{
    adf_modctx_struct * c4 = mkctx1(4);
    adf_fball_t x, g;
    char * s1;
    char * s2;
    size_t n1, n2;
    ulong r2 = 2;

    adf_fball_init(x);
    adf_fball_init(g);
    mklocal_si(x, c4, 2, &r2);
    adf_fball_set_global(g, x);
    s1 = adf_fball_get_str(&n1, x);
    s2 = adf_fball_get_str(&n2, g);
    ADF_CHECK(s1 != NULL && s2 != NULL);
    ADF_CHECK(n1 == n2 && strcmp(s1, s2) == 0);
    ADF_CHECK_MSG(strcmp(s1, "(* ; 1 mod 2)") == 0, "printed \"%s\"", s1);
    adf_str_free(s1);
    adf_str_free(s2);

    adf_fball_clear(x);
    adf_fball_clear(g);
    adf_modctx_free(c4);
}

/* src/adele.c never reads a field of adf_fball: a local finite part goes through set, add,
   is_canonical and identical. */
ADF_TEST(caller_adele_add_with_local_finite_parts)
{
    ulong q2[2] = {4, 9};
    ulong r[2] = {3, 7}, s[2] = {2, 4};
    adf_modctx_struct * c = mkctx(q2, 2);
    adf_fball_t f1, f2, e, fz;
    adf_adele_t x, y, z;
    arb_t a;

    adf_fball_init(f1);
    adf_fball_init(f2);
    adf_fball_init(e);
    adf_fball_init(fz);
    adf_adele_init(x);
    adf_adele_init(y);
    adf_adele_init(z);
    arb_init(a);

    mklocal_si(f1, c, 6, r);
    mklocal_si(f2, c, 10, s);
    arb_set_si(a, 1);
    ADF_CHECK(adf_adele_set_arb_fball(x, a, f1) == ADF_OK);
    arb_set_si(a, 2);
    ADF_CHECK(adf_adele_set_arb_fball(y, a, f2) == ADF_OK);
    ADF_CHECK(adf_adele_is_canonical(x) && adf_adele_is_canonical(y));
    adf_adele_add(z, x, y, 64);
    ADF_CHECK(adf_adele_is_canonical(z));
    adf_adele_get_fin(fz, z);
    ADF_CHECK(adf_fball_is_local(fz) && adf_fball_context(fz) == c && fmpz_equal_si(fz->d, 30));
    adf_fball_add(e, f1, f2);
    ADF_CHECK(adf_fball_identical(fz, e));
    /* in place, and a copy of the adele keeps the context */
    adf_adele_add(x, x, x, 64);
    adf_adele_get_fin(fz, x);
    adf_fball_add(e, f1, f1);
    ADF_CHECK(adf_fball_identical(fz, e));
    adf_adele_set(y, x);
    ADF_CHECK(adf_adele_identical(x, y));

    adf_adele_clear(x);
    adf_adele_clear(y);
    adf_adele_clear(z);
    adf_fball_clear(f1);
    adf_fball_clear(f2);
    adf_fball_clear(e);
    adf_fball_clear(fz);
    arb_clear(a);
    adf_modctx_free(c);
}

/* ------------------------------------------- no leak of the residue array */

/* Counting memory functions for FLINT (refs/src/flint-3.0.1/memory.rst:16-21: the user may
   replace the functions behind flint_malloc, flint_realloc, flint_calloc, flint_free). They
   count the blocks that are live; a residue array that is never freed shows as growth. */
static long g_live = 0;

static void *
cnt_malloc(size_t n)
{
    g_live++;
    return malloc(n);
}

static void *
cnt_calloc(size_t n, size_t m)
{
    g_live++;
    return calloc(n, m);
}

static void *
cnt_realloc(void * p, size_t n)
{
    if (p == NULL)
        g_live++;
    return realloc(p, n);
}

static void
cnt_free(void * p)
{
    if (p != NULL)
        g_live--;
    free(p);
}

/* 1000 rounds of every operation that writes a residue array or releases one; the number of live
   blocks may not grow by more than a bounded cache (conventions 4.2: a value owns its array;
   init, set, swap, clear and every change of backend must not leak). */
ADF_TEST(no_growth_of_live_blocks)
{
    void *(*om)(size_t);
    void *(*oc)(size_t, size_t);
    void *(*orl)(void *, size_t);
    void (*of)(void *);
    ulong q3[3] = {4, 9, 5};
    adf_modctx_struct * c = mkctx(q3, 3);
    adf_modctx_struct * c2 = mkctx(q3, 3);
    adf_fball_t g, x, y, z, w;
    adf_rat_t q;
    long before = 0, round;
    int lost;

    adf_fball_init(g);
    adf_fball_init(x);
    adf_fball_init(y);
    adf_fball_init(z);
    adf_fball_init(w);
    adf_rat_init(q);
    mkball_si(g, 7, 180, 3);                       /* (7 + 180 Zhat)/3: in (4, 9, 5) as (3; 7) */
    mkball_si(w, 1, 2, 1);

    __flint_get_memory_functions(&om, &oc, &orl, &of);
    __flint_set_memory_functions(cnt_malloc, cnt_calloc, cnt_realloc, cnt_free);
    for (round = 0; round < 1001; round++)
    {
        if (round == 1)
            before = g_live;
        ADF_CHECK(adf_fball_set_local(x, g, c) == ADF_OK);        /* global -> local */
        ADF_CHECK(adf_fball_set_local(x, x, c) == ADF_OK);        /* local -> local, in place */
        ADF_CHECK(adf_fball_set_local(y, x, c2) == ADF_OK);       /* another context */
        ADF_CHECK(adf_fball_set_local_enclose(z, &lost, w, c) == ADF_OK);
        ADF_CHECK(adf_fball_set_local_enclose(z, &lost, z, c) == ADF_OK);
        adf_fball_set(z, y);                                       /* local <- local */
        adf_fball_add(z, x, x);
        adf_fball_mul(z, x, x);
        adf_fball_neg(z, z);
        fmpq_set_si(q->q, 1, 3);
        adf_fball_mul_rat(z, z, q);
        adf_fball_add(z, x, y);                                    /* two contexts: global */
        adf_fball_set(z, x);
        adf_fball_swap(z, y);
        adf_fball_set_global(y, y);                                /* local -> global */
        adf_fball_set_global(x, x);
        adf_fball_set(x, g);                                       /* global over local */
        adf_fball_zero(z);                                         /* constructor over local */
    }
    __flint_set_memory_functions(om, oc, orl, of);
    ADF_CHECK_MSG(g_live - before < 100, "live blocks grew by %ld in 1000 rounds", g_live - before);

    adf_fball_clear(g);
    adf_fball_clear(x);
    adf_fball_clear(y);
    adf_fball_clear(z);
    adf_fball_clear(w);
    adf_rat_clear(q);
    adf_modctx_free(c);
    adf_modctx_free(c2);
}

/* -------------------------------- tests added after the mutation run of src/fball.c */

/* An output that is local in a context of fewer blocks than the inputs: its residue array must be
   resized before the residues are written (src/fball.c fb_local_res). Under the sanitizer an
   array that is not resized is a heap overflow; without it, 127 words past a one-word block. */
ADF_TEST(local_output_of_another_context_is_resized)
{
    adf_modctx_struct * c1 = mkctx1(7);
    adf_modctx_struct * c = mkctx_n(128);
    slong k = adf_modctx_nblocks(c);
    ulong * res = (ulong *) flint_malloc(k * sizeof(ulong));
    ulong r1 = 3;
    adf_fball_t x, y, z, e, g, h;
    adf_rat_t q;
    flint_rand_t st;
    int op;

    flint_randinit(st);
    adf_fball_init(x);
    adf_fball_init(y);
    adf_fball_init(z);
    adf_fball_init(e);
    adf_fball_init(g);
    adf_fball_init(h);
    adf_rat_init(q);
    rand_res(res, c, st);
    mklocal_si(x, c, 6, res);
    rand_res(res, c, st);
    mklocal_si(y, c, 10, res);
    adf_fball_set_global(g, x);
    adf_fball_set_global(h, y);
    for (op = 0; op < 5; op++)
    {
        mklocal_si(z, c1, 1, &r1);
        if (op == 0)
        {
            adf_fball_add(z, x, y);
            adf_fball_add(e, g, h);
        }
        else if (op == 1)
        {
            adf_fball_sub(z, x, y);
            adf_fball_sub(e, g, h);
        }
        else if (op == 2)
        {
            adf_fball_neg(z, x);
            adf_fball_neg(e, g);
        }
        else if (op == 3)
        {
            fmpq_set_si(q->q, -3, 7);
            adf_fball_mul_rat(z, x, q);
            adf_fball_mul_rat(e, g, q);
        }
        else
        {
            adf_fball_mul(z, x, y);
            adf_fball_mul(e, g, h);
        }
        ADF_CHECK(adf_fball_equal_set(z, e));
        ADF_CHECK(adf_fball_is_canonical(z));
        if (op < 4)
            ADF_CHECK(adf_fball_is_local(z) && adf_fball_context(z) == c);
    }
    adf_fball_clear(x);
    adf_fball_clear(y);
    adf_fball_clear(z);
    adf_fball_clear(e);
    adf_fball_clear(g);
    adf_fball_clear(h);
    adf_rat_clear(q);
    flint_free(res);
    flint_randclear(st);
    adf_modctx_free(c1);
    adf_modctx_free(c);
}

/* Predicate L needs k >= 1 (fball.h): a value that points to a context without blocks is not L,
   even with H = K = 1, A = 0 and d = 1. */
ADF_TEST(is_canonical_rejects_a_context_without_blocks)
{
    adf_modctx_struct * c0 = NULL;
    adf_fball_t x;
    fmpz_t one;

    fmpz_init_set_ui(one, 1);
    ADF_CHECK(adf_modctx_new_fmpz(&c0, one) == ADF_OK);
    ADF_CHECK(adf_modctx_nblocks(c0) == 0);
    adf_fball_init(x);
    x->res = (ulong *) flint_malloc(sizeof(ulong));
    x->res[0] = 0;
    fmpz_zero(x->A);
    fmpz_one(x->H);
    fmpz_one(x->d);
    x->backend = ADF_LOCAL;
    x->mctx = c0;
    ADF_CHECK(!adf_fball_is_canonical(x));
    adf_fball_clear(x);
    fmpz_clear(one);
    adf_modctx_free(c0);
}

/* The global constructors over a global value that holds other data (the constructors write
   every field; src/fball.c of lane m1-fball, kept unchanged). */
ADF_TEST(constructors_over_a_value_with_other_data)
{
    adf_fball_t x;
    fmpz_t n;

    adf_fball_init(x);
    fmpz_init_set_si(n, -9);
    mkball_si(x, 5, 12, 7);
    adf_fball_zero(x);
    ADF_CHECK(fields_are_si(x, 0, 0, 1) && adf_fball_is_canonical(x));
    mkball_si(x, 5, 12, 7);
    adf_fball_one(x);
    ADF_CHECK(fields_are_si(x, 1, 0, 1) && adf_fball_is_canonical(x));
    mkball_si(x, 5, 12, 7);
    adf_fball_set_si(x, 4);
    ADF_CHECK(fields_are_si(x, 4, 0, 1) && adf_fball_is_canonical(x));
    mkball_si(x, 5, 12, 7);
    adf_fball_set_fmpz(x, n);
    ADF_CHECK(fields_are_si(x, -9, 0, 1) && adf_fball_is_canonical(x));
    adf_fball_clear(x);
    fmpz_clear(n);
}

/* As no_growth_of_live_blocks, for the functions of src/fball.c that release a residue array:
   clear, set (global over local), one, set_si, set_fmpz, set_rat, and a global result over a
   local output. */
ADF_TEST(no_growth_of_live_blocks_in_the_life_cycle)
{
    void *(*om)(size_t);
    void *(*oc)(size_t, size_t);
    void *(*orl)(void *, size_t);
    void (*of)(void *);
    ulong q3[3] = {4, 9, 5};
    ulong r3[3] = {3, 7, 2};
    adf_modctx_struct * c = mkctx(q3, 3);
    adf_fball_t g, x;
    adf_rat_t q;
    fmpz_t n;
    long before = 0, round;

    adf_fball_init(g);
    adf_rat_init(q);
    fmpz_init_set_si(n, 11);
    mkball_si(g, 1, 2, 3);
    adf_rat_set_si(q, 2);

    __flint_get_memory_functions(&om, &oc, &orl, &of);
    __flint_set_memory_functions(cnt_malloc, cnt_calloc, cnt_realloc, cnt_free);
    for (round = 0; round < 1001; round++)
    {
        if (round == 1)
            before = g_live;
        adf_fball_init(x);
        mklocal_si(x, c, 6, r3);
        adf_fball_set(x, g);
        mklocal_si(x, c, 6, r3);
        adf_fball_one(x);
        mklocal_si(x, c, 6, r3);
        adf_fball_set_si(x, 3);
        mklocal_si(x, c, 6, r3);
        adf_fball_set_fmpz(x, n);
        mklocal_si(x, c, 6, r3);
        adf_fball_set_rat(x, q);
        mklocal_si(x, c, 6, r3);
        adf_fball_add(x, g, g);
        mklocal_si(x, c, 6, r3);
        adf_fball_clear(x);
    }
    __flint_set_memory_functions(om, oc, orl, of);
    ADF_CHECK_MSG(g_live - before < 100, "live blocks grew by %ld in 1000 rounds", g_live - before);

    adf_fball_clear(g);
    adf_rat_clear(q);
    fmpz_clear(n);
    adf_modctx_free(c);
}
