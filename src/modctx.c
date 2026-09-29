/* adelefeld/modctx.c: modulus contexts (work package 1.8, first part).

   A context is the modulus K >= 1 of the local backend together with k >= 0 word blocks
   q_1, ..., q_k, pairwise coprime, 2 <= q_i < 2^64, in the order supplied, with product K when
   k >= 1 (docs/conventions.md 5.14; docs/SPEC.md 4.1, "The modulus data (blocks, reduction
   constants, recombination tree) live in an immutable context"). For each block the context
   also records whether it is a certified prime power and its prime (conventions 5.14). The
   constructors are the modulus families of docs/SPEC.md 15.1 (arbitrary integer, list of
   pairwise coprime blocks, list of prime powers, factorial, power of a primorial).

   Life cycle: docs/conventions.md 4.6 (gate finding G2, closure edit E2). A successful
   constructor allocates and fully initialises an immutable context and writes its pointer to
   *out; on failure *out is untouched and no allocation is retained; a constructor never frees,
   mutates or reuses the context previously pointed to by *out. adf_modctx_free(NULL) does
   nothing. After construction nothing in the context is written again (no mutable field, no
   lazy initialisation), so any number of threads may read one context (conventions 4.5).

   The two conversions (docs/proofs/policies.md Definition 16, line 305; Lemma 17, line 312;
   docs/PERF.md section 4, the rows "Global integer to k residues" and "k residues to the global
   integer"): adf_modctx_reduce writes A mod q_i into res[i], each in [0, q_i), and
   adf_modctx_combine writes the unique integer of [0, K) with those residues. They are used by
   the local backend (conventions 5.3, work package 1.8) and by tests/test_modctx.c; no public
   header declares them (HEADER-FINDING 2 of lane m1-modctx).

   Reduction uses FLINT's fmpz_multi_mod and recombination its fmpz_multi_CRT (the "remainder
   tree" and "recombination tree" of docs/PERF.md section 4): the precomputed forms
   fmpz_multi_mod_precomp and fmpz_multi_CRT_precomp take their precomputation as
   `const fmpz_multi_mod_t` / `const fmpz_multi_CRT_t` (/usr/include/flint/fmpz.h:657, :692),
   so a call only reads the context. **[probed]** on FLINT 3.0.1 (lanes/m1-modctx/flint_probe.c):
   with sign = 0 both return non-negative results, fmpz_multi_mod_precomp gives input mod m_i in
   [0, m_i) also for a negative input, and fmpz_multi_CRT_precomp gives the unique solution of
   [0, product) ; fmpz_multi_CRT_precompute rejects non-coprime moduli (returns 0), which the
   validated pairwise coprime blocks cannot trigger. fmpz_multi_mod_precompute requires
   non-zero moduli ("good: the moduli are good for MOD, none are zero", fmpz.h:680), likewise
   satisfied by the blocks. The recombination needs no inversion and no division (policies
   Lemma 17.3). */

#include "adelefeld/modctx.h"

#include <flint/fmpz.h>
#include <flint/fmpz_vec.h>
#include <flint/nmod.h>
#include <flint/ulong_extras.h>

#include <stdio.h>
#include <string.h>

/* The cap on the block count of a context (HEADER-FINDING 1 of lane m1-modctx). The bound is
   max_items of conventions 8.4 (ADF_TEXT_MAX_ITEMS_DEFAULT, adelefeld/text.h), which bounds the
   block count of a context occurrence in text: a context at the cap can still be dumped and
   loaded. Beyond it a constructor returns ADF_UNSUPPORTED (conventions 3.2, row "Raw context
   constructors": "a valid request that version 1 does not implement") before reading any
   block, since no block list of that length can be checked (the cost of adf_modctx_new_blocks
   is k^2 word gcds, modctx.h). */
#define MODCTX_MAX_BLOCKS ((slong) ADF_TEXT_MAX_ITEMS_DEFAULT)

struct adf_modctx_struct
{
    fmpz_t K;               /* the modulus, >= 1 (conventions 5.14) */
    slong k;                /* the number of word blocks, >= 0 */
    ulong * q;              /* the blocks, in the order supplied; NULL when k = 0 */
    nmod_t * nm;            /* one nmod_t per block (conventions 5.14, nmod.h:212) */
    ulong * pp;             /* per block: its prime when it is a certified prime power, else 0 */
    fmpz_multi_mod_t red;   /* the reduction tree (SPEC 4.1: "reduction constants") */
    fmpz_multi_CRT_t crt;   /* the recombination tree (SPEC 4.1: "recombination tree") */
};

/* --------------------------------------------------------------- helpers */

/* word_pow(r, p, e): *r = p^e. Returns 1 when p^e >= 2^64 (and leaves *r alone), else 0.
   e >= 64 overflows already because p >= 2, so the loop runs at most 63 times. */
static int
word_pow(ulong * r, ulong p, ulong e)
{
    ulong acc = 1;
    ulong i;

    if (e >= 64)
        return 1;
    for (i = 0; i < e; i++)
    {
        if (acc > UWORD_MAX / p)
            return 1;
        acc *= p;
    }
    *r = acc;
    return 0;
}

/* The smallest prime strictly above p >= 1. n_nextprime is used to propose the candidates and
   n_is_prime to certify them (modctx.h names n_is_prime as the primality test of a prime-power
   block; ulong_extras.h:335 and :345). */
static ulong
next_prime(ulong p)
{
    do
    {
        p = n_nextprime(p, 1);
    }
    while (!n_is_prime(p));
    return p;
}

/* v_p(n!), the exponent of p in n!: the sum of the floors n/p^i (Legendre). */
static ulong
valuation_factorial(ulong n, ulong p)
{
    ulong e = 0;
    ulong t;

    for (t = n / p; t > 0; t /= p)
        e += t;
    return e;
}

/* ctx_create(out, q, k, primes, Kstore): the context with the validated blocks q[0..k-1],
   already pairwise coprime and >= 2. primes is NULL when no block is a certified prime power,
   else primes[i] is the prime of the prime-power block q[i] (0 for a block that is not
   certified). Kstore, when not NULL, is the modulus to store (adf_modctx_new_fmpz keeps a
   modulus without blocks); otherwise the modulus is the product of the blocks. Writes *out only
   on success. */
static int
ctx_create(adf_modctx_struct ** out, const ulong * q, slong k, const ulong * primes,
           const fmpz_t Kstore)
{
    adf_modctx_struct * ctx;
    fmpz * moduli;
    slong i;
    int ok;

    ctx = flint_malloc(sizeof(adf_modctx_struct));
    ctx->k = k;
    ctx->q = (k > 0) ? flint_malloc(k * sizeof(ulong)) : NULL;
    ctx->nm = (k > 0) ? flint_malloc(k * sizeof(nmod_t)) : NULL;
    ctx->pp = (k > 0) ? flint_malloc(k * sizeof(ulong)) : NULL;
    fmpz_init(ctx->K);

    if (Kstore != NULL)
        fmpz_set(ctx->K, Kstore);
    else
        fmpz_one(ctx->K);

    moduli = (k > 0) ? _fmpz_vec_init(k) : NULL;
    for (i = 0; i < k; i++)
    {
        ctx->q[i] = q[i];
        nmod_init(ctx->nm + i, q[i]);
        ctx->pp[i] = (primes != NULL) ? primes[i] : 0;
        fmpz_set_ui(moduli + i, q[i]);
        if (Kstore == NULL)
            fmpz_mul_ui(ctx->K, ctx->K, q[i]);
    }

    fmpz_multi_mod_init(ctx->red);
    fmpz_multi_CRT_init(ctx->crt);
    ok = 1;
    if (k >= 1)
    {
        ok = fmpz_multi_mod_precompute(ctx->red, moduli, k);
        ok = ok && fmpz_multi_CRT_precompute(ctx->crt, moduli, k);
    }
    if (k > 0)
        _fmpz_vec_clear(moduli, k);

    if (!ok)
    {
        /* Unreachable for validated blocks: precompute fails only on a zero or non-coprime
           modulus (fmpz.h:645, :680), which the constructors exclude. Reported as
           "valid request that version 1 does not implement" rather than guessed around. */
        fmpz_multi_mod_clear(ctx->red);
        fmpz_multi_CRT_clear(ctx->crt);
        fmpz_clear(ctx->K);
        flint_free(ctx->pp);
        flint_free(ctx->nm);
        flint_free(ctx->q);
        flint_free(ctx);
        return ADF_UNSUPPORTED;
    }

    *out = ctx;
    return ADF_OK;
}

/* ---------------------------------------------------------- construction */

int
adf_modctx_new_blocks(adf_modctx_struct ** out, const ulong * q, slong k)
{
    slong i, j;

    if (k < 0)
        return ADF_DOMAIN;
    if (k > MODCTX_MAX_BLOCKS)
        return ADF_UNSUPPORTED;                 /* HEADER-FINDING 1 */
    if (k > 0 && q == NULL)
        return ADF_DOMAIN;                      /* q may be NULL only when k = 0 (modctx.h) */
    for (i = 0; i < k; i++)
        if (q[i] < 2)
            return ADF_DOMAIN;
    for (i = 0; i < k; i++)
        for (j = 0; j < i; j++)
            if (n_gcd(q[i], q[j]) != 1)
                return ADF_DOMAIN;
    return ctx_create(out, q, k, NULL, NULL);
}

int
adf_modctx_new_prime_powers(adf_modctx_struct ** out, const ulong * p, const ulong * e,
                            slong k)
{
    ulong * q = NULL;
    ulong * primes = NULL;
    slong i, j;
    int status;

    if (k < 0)
        return ADF_DOMAIN;
    if (k > MODCTX_MAX_BLOCKS)
        return ADF_UNSUPPORTED;                 /* HEADER-FINDING 1 */
    if (k > 0 && (p == NULL || e == NULL))
        return ADF_DOMAIN;
    if (k > 0)
    {
        q = flint_malloc(k * sizeof(ulong));
        primes = flint_malloc(k * sizeof(ulong));
    }
    for (i = 0; i < k; i++)
    {
        if (e[i] == 0)
        {
            status = ADF_DOMAIN;
            goto fail;
        }
        if (!n_is_prime(p[i]))
        {
            status = ADF_DOMAIN;
            goto fail;
        }
        for (j = 0; j < i; j++)
            if (p[j] == p[i])
            {
                status = ADF_DOMAIN;
                goto fail;
            }
        if (word_pow(q + i, p[i], e[i]))
        {
            status = ADF_UNSUPPORTED;
            goto fail;
        }
        primes[i] = p[i];
    }
    status = ctx_create(out, q, k, primes, NULL);
fail:
    flint_free(q);
    flint_free(primes);
    return status;
}

int
adf_modctx_new_fmpz(adf_modctx_struct ** out, const fmpz_t K)
{
    ulong q[1];

    if (fmpz_sgn(K) < 1)
        return ADF_DOMAIN;
    if (fmpz_cmp_ui(K, 2) >= 0 && fmpz_abs_fits_ui(K))
    {
        q[0] = fmpz_get_ui(K);
        return ctx_create(out, q, 1, NULL, K);
    }
    /* K = 1, or K >= 2^64: no block, the modulus is kept (modctx.h, new_fmpz) */
    return ctx_create(out, NULL, 0, NULL, K);
}

int
adf_modctx_new_factorial(adf_modctx_struct ** out, ulong n)
{
    ulong * q = NULL;
    ulong * primes = NULL;
    ulong p, e, r;
    slong k = 0;
    int status;

    /* n < 2 gives K = 1 without blocks (modctx.h). */
    if (n < 2)
        return ctx_create(out, NULL, 0, NULL, NULL);

    /* The smallest prime power of n! is 2^v_2(n!); if it leaves a word, some prime power of n!
       does, which is ADF_UNSUPPORTED (modctx.h, new_factorial). v_2(n!) >= 64 holds for every
       n >= 66 (v_2 is non-decreasing, v_2(65!) = 63), so this check also bounds the loop
       below. */
    if (word_pow(&r, 2, valuation_factorial(n, 2)))
        return ADF_UNSUPPORTED;

    /* n <= 65 here: at most the 18 primes below it. */
    for (p = 2; p <= n; p = next_prime(p))
        k++;
    q = flint_malloc(k * sizeof(ulong));
    primes = flint_malloc(k * sizeof(ulong));
    k = 0;
    for (p = 2; p <= n; p = next_prime(p))
    {
        e = valuation_factorial(n, p);
        if (word_pow(q + k, p, e))
        {
            status = ADF_UNSUPPORTED;
            goto fail;
        }
        primes[k] = p;
        k++;
    }
    status = ctx_create(out, q, k, primes, NULL);
fail:
    flint_free(q);
    flint_free(primes);
    return status;
}

int
adf_modctx_new_primorial_pow(adf_modctx_struct ** out, ulong n, ulong e)
{
    ulong * q = NULL;
    ulong * primes = NULL;
    ulong alloc = 0;
    ulong p;
    slong k = 0;
    int status;

    if (n < 2 || e == 0)
        return ctx_create(out, NULL, 0, NULL, NULL);   /* K = 1 without blocks (modctx.h) */

    /* The blocks are p^e for the primes p <= n, in increasing order of p. Two things stop the
       enumeration, both ADF_UNSUPPORTED: a prime power p^e >= 2^64 (modctx.h), and more than
       MODCTX_MAX_BLOCKS primes below n (HEADER-FINDING 1). The second stop bounds the work for
       every n, so p never approaches the overflow of the word. */
    for (p = 2; p <= n; p = next_prime(p))
    {
        if (k == MODCTX_MAX_BLOCKS)
        {
            status = ADF_UNSUPPORTED;
            goto fail;
        }
        if ((ulong) k == alloc)
        {
            alloc = (alloc == 0) ? 16 : 2 * alloc;
            q = flint_realloc(q, alloc * sizeof(ulong));
            primes = flint_realloc(primes, alloc * sizeof(ulong));
        }
        if (word_pow(q + k, p, e))
        {
            status = ADF_UNSUPPORTED;
            goto fail;
        }
        primes[k] = p;
        k++;
    }
    status = ctx_create(out, q, k, primes, NULL);
fail:
    flint_free(q);
    flint_free(primes);
    return status;
}

void
adf_modctx_free(adf_modctx_struct * ctx)
{
    if (ctx == NULL)
        return;
    fmpz_multi_mod_clear(ctx->red);
    fmpz_multi_CRT_clear(ctx->crt);
    fmpz_clear(ctx->K);
    flint_free(ctx->pp);
    flint_free(ctx->nm);
    flint_free(ctx->q);
    flint_free(ctx);
}

/* --------------------------------------------------------- read access */

void
adf_modctx_get_modulus(fmpz_t K, const adf_modctx_struct * ctx)
{
    fmpz_set(K, ctx->K);
}

slong
adf_modctx_nblocks(const adf_modctx_struct * ctx)
{
    return ctx->k;
}

ulong
adf_modctx_block(const adf_modctx_struct * ctx, slong i)
{
    return ctx->q[i];
}

char *
adf_modctx_dump_str(size_t * len, const adf_modctx_struct * ctx)
{
    /* "adf1 Q modctx K k q_1 ... q_k", every integer the h token of conventions 10.1:
       lower-case hexadecimal without leading zeros. Allocated with flint_malloc, NUL at
       s[len], freed with adf_str_free (conventions 8.1). */
    static const char prefix[] = "adf1 Q modctx ";
    char kbuf[2 + 8 * sizeof(slong)];
    char qbuf[2 + 8 * sizeof(ulong)];
    char * Kstr;
    char * s;
    size_t total, pos, kl, ql;
    slong i;

    Kstr = fmpz_get_str(NULL, 16, ctx->K);
    kl = (size_t) snprintf(kbuf, sizeof(kbuf), "%llx", (unsigned long long) ctx->k);

    total = strlen(prefix) + strlen(Kstr) + 1 + kl;
    for (i = 0; i < ctx->k; i++)
    {
        ql = (size_t) snprintf(qbuf, sizeof(qbuf), "%llx", (unsigned long long) ctx->q[i]);
        total += 1 + ql;
    }

    s = flint_malloc(total + 1);
    pos = 0;
    memcpy(s + pos, prefix, strlen(prefix) - 1);       /* without the trailing NUL */
    pos += strlen(prefix) - 1;
    s[pos++] = ' ';
    memcpy(s + pos, Kstr, strlen(Kstr));
    pos += strlen(Kstr);
    s[pos++] = ' ';
    memcpy(s + pos, kbuf, kl);
    pos += kl;
    for (i = 0; i < ctx->k; i++)
    {
        ql = (size_t) snprintf(qbuf, sizeof(qbuf), "%llx", (unsigned long long) ctx->q[i]);
        s[pos++] = ' ';
        memcpy(s + pos, qbuf, ql);
        pos += ql;
    }
    s[pos] = 0;
    flint_free(Kstr);
    *len = total;
    return s;
}

/* --------------------------------------------------------- descriptors */

void
adf_ctx_desc_init(adf_ctx_desc_t * d)
{
    fmpz_init_set_ui(d->K, 1);
    d->k = 0;
    d->q = NULL;
}

void
adf_ctx_desc_clear(adf_ctx_desc_t * d)
{
    fmpz_clear(d->K);
    flint_free(d->q);
    d->q = NULL;
    d->k = 0;
}

int
adf_modctx_matches_desc(const adf_modctx_struct * ctx, const adf_ctx_desc_t * d)
{
    slong i;

    if (!fmpz_equal(ctx->K, d->K) || ctx->k != d->k)
        return 0;
    if (d->k > 0 && d->q == NULL)
        return 0;
    for (i = 0; i < ctx->k; i++)
        if (ctx->q[i] != d->q[i])
            return 0;
    return 1;
}

/* ------------------------------------------- the two conversions (internal) */

void
adf_modctx_reduce(ulong * res, const fmpz_t A, const adf_modctx_struct * ctx)
{
    fmpz * out;
    slong i;

    if (ctx->k == 0)
        return;
    out = _fmpz_vec_init(ctx->k);
    fmpz_multi_mod_precomp(out, ctx->red, A, 0);
    for (i = 0; i < ctx->k; i++)
        res[i] = fmpz_get_ui(out + i);
    _fmpz_vec_clear(out, ctx->k);
}

void
adf_modctx_combine(fmpz_t A, const ulong * res, const adf_modctx_struct * ctx)
{
    fmpz * in;
    slong i;

    if (ctx->k == 0)
    {
        fmpz_zero(A);          /* K = 1: the only integer of [0, 1) */
        return;
    }
    in = _fmpz_vec_init(ctx->k);
    for (i = 0; i < ctx->k; i++)
        fmpz_set_ui(in + i, res[i]);
    fmpz_multi_CRT_precomp(A, ctx->crt, in, 0);
    _fmpz_vec_clear(in, ctx->k);
}

/* ------------------------------------- the prime-power record (internal) */

ulong
adf_modctx_block_prime(const adf_modctx_struct * ctx, slong i)
{
    /* The prime of the certified prime-power block q_i, 0 when block i is not a certified
       prime power (conventions 5.14 content: "for each block whether it is a certified prime
       power and its prime"). Exported with adf_modctx_reduce and adf_modctx_combine for the
       operations that name a prime (SPEC 4.1) and for tests/test_modctx.c; no public header
       declares it (HEADER-FINDING 2 of lane m1-modctx). */
    return ctx->pp[i];
}
