/* src/modctx.c: modulus contexts (constructors, free, read access), context descriptors of
   dump occurrences, and the reduction/recombination kernels of the local backend.

   Contract: docs/conventions.md 4.6 (lifetime and construction; closure E2), 5.14 (contents
   and constructors), 5.3 (local backend), 10.1 and 10.2 (descriptors and inspection; closure
   C2, C3), 12.4 and 12.10 (opaque type, 64-bit); docs/SPEC.md 4.1, 15; docs/PLAN.md section 4
   ("Contexts"); docs/proofs/policies.md Definition 16 to Lemma 18 (blocks, residues,
   recombination). Statuses: docs/conventions.md 3.1, 3.2.

   Ground truth for the FLINT calls. fmpz_multi_mod_t and fmpz_multi_CRT_t and their precompute
   and precomp functions are declared in the installed header
   `/usr/include/flint/fmpz.h:625-692`; the precomp functions take the program by `const`, so
   they write no state of the context, which is what makes a context safe to share between
   threads. The FLINT 3.0.1 documentation of these functions is not on disk under refs():
   [source pending: refs/src/flint-3.0.1/fmpz.rst, "multi mod" and "multi CRT"]. n_is_prime
   and n_nextprime are refs/src/flint-3.0.1/ulong_extras.rst:833 and :688 and
   /usr/include/flint/ulong_extras.h:335 and :345.

   A context is immutable after construction and has no mutable field and no lazy
   initialisation (docs/conventions.md 4.5, 4.6; SPEC.md 10.1). Everything a reduction needs at
   run time is built once, in the constructor: the blocks, their product K, and the fmpz_comb
   tables. The per-call scratch of fmpz_comb (`fmpz_comb_temp_t`) is local to the kernel, so a
   context may be read by any number of threads. */

#include <string.h>

#include <flint/fmpz.h>
#include <flint/fmpz_vec.h>
#include <flint/nmod.h>
#include <flint/ulong_extras.h>

#include "adelefeld/modctx.h"

/* The internal kernels are not part of the public interface and must not appear in the
   dynamic symbol table (tests/test_exports.sh of lane m1-common compares it with the public
   headers). Hidden visibility keeps them linkable from the test and the benchmark without
   exporting them. */
#if defined(__GNUC__) || defined(__clang__)
#define ADF_HIDDEN __attribute__((visibility("hidden")))
#else
#define ADF_HIDDEN
#endif

/* Contents of a context (docs/conventions.md 5.14): K >= 1; k >= 0 blocks q[0..k-1], pairwise
   coprime, 2 <= q[i] < 2^64, in the order supplied, with product K when k >= 1; and the
   recombination tables. Not part of the public interface. */
struct adf_modctx_struct
{
    fmpz_t K;
    slong k;
    ulong * q;
    fmpz_multi_mod_t mod_P;            /* initialised exactly when k >= 1 */
    fmpz_multi_CRT_t crt_P;            /* initialised exactly when k >= 1 */
};

/* ------------------------------------------------------------------ small helpers */

static int
adf_is_hexdigit(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
}

static int
adf_is_hexnonzero(char c)
{
    return (c >= '1' && c <= '9') || (c >= 'a' && c <= 'f');
}

static int
adf_is_upper(char c)
{
    return c >= 'A' && c <= 'Z';
}

/* The length of the lower-case hexadecimal spelling of v (no leading zeros; "0" for 0). */
static size_t
adf_hex_len(ulong v)
{
    size_t n = 1;
    while (v >= 16)
    {
        v >>= 4;
        n++;
    }
    return n;
}

/* Write the lower-case hexadecimal spelling of v into buf (at least adf_hex_len(v) bytes);
   returns the length. No leading zeros, "0" for 0. */
static size_t
adf_hex_write(char * buf, ulong v)
{
    size_t n = adf_hex_len(v);
    size_t i = n;
    buf[n] = '\0';
    do
    {
        unsigned d = (unsigned) (v & 15);
        buf[--i] = (char) (d < 10 ? '0' + d : 'a' + (d - 10));
        v >>= 4;
    } while (i > 0);
    return n;
}

/* p^e, if it is below 2^64; returns 1 and writes *out, else 0. Assumes p >= 2, e >= 1. */
static int
adf_power_fits_word(ulong p, ulong e, ulong * out)
{
    ulong r = 1;
    ulong i;

    if (e >= 64)
        return 0;                      /* p >= 2, so p^e >= 2^64 */
    for (i = 0; i < e; i++)
    {
        if (r > (ulong) -1 / p)
            return 0;
        r *= p;
    }
    *out = r;
    return 1;
}

/* Allocate and fully initialise a context from already validated data. K is copied; the
   blocks q[0..k-1] are copied; the comb tables are built. The caller has checked the
   predicate of 5.14 (for k >= 1: pairwise coprime, 2 <= q[i] < 2^64, product = K). */
static adf_modctx_struct *
adf_modctx_alloc(const fmpz_t K, const ulong * q, slong k)
{
    adf_modctx_struct * ctx = flint_malloc(sizeof(adf_modctx_struct));

    fmpz_init_set(ctx->K, K);
    ctx->k = k;
    if (k > 0)
    {
        fmpz * mods;
        slong i;

        ctx->q = flint_malloc((size_t) k * sizeof(ulong));
        memcpy(ctx->q, q, (size_t) k * sizeof(ulong));
        mods = _fmpz_vec_init(k);
        for (i = 0; i < k; i++)
            fmpz_set_ui(&mods[i], q[i]);
        /* The reduction and recombination programs are built once, here.  The `_precomp`
           calls take a `const` program and write nothing into it, so a context is immutable
           after construction and two threads may reduce against one context
           (docs/conventions.md 4.5; fmpz.h:656 and :691). */
        fmpz_multi_mod_init(ctx->mod_P);
        fmpz_multi_mod_precompute(ctx->mod_P, mods, k);
        fmpz_multi_CRT_init(ctx->crt_P);
        fmpz_multi_CRT_precompute(ctx->crt_P, mods, k);
        _fmpz_vec_clear(mods, k);
    }
    else
    {
        ctx->q = NULL;
        fmpz_multi_mod_init(ctx->mod_P);
        fmpz_multi_CRT_init(ctx->crt_P);
    }
    return ctx;
}

/* Allocate from the blocks alone: K is their product. */
static adf_modctx_struct *
adf_modctx_alloc_from_blocks(const ulong * q, slong k)
{
    adf_modctx_struct * ctx;
    fmpz_t K;
    slong i;

    fmpz_init_set_ui(K, 1);
    for (i = 0; i < k; i++)
        fmpz_mul_ui(K, K, q[i]);
    ctx = adf_modctx_alloc(K, q, k);
    fmpz_clear(K);
    return ctx;
}

/* ------------------------------------------------------------------ constructors */

/* docs/conventions.md 5.14, table row "adf_modctx_new_blocks": the context with the blocks as
   supplied, K = their product; k = 0 gives K = 1. DOMAIN unless k >= 0, every q[i] >= 2 and the
   blocks pairwise coprime. The blocks are copied. */
int
adf_modctx_new_blocks(adf_modctx_struct ** out, const ulong * q, slong k)
{
    slong i, j;

    if (out == NULL)
        return ADF_DOMAIN;
    if (k < 0)
        return ADF_DOMAIN;
    if (k == 0)
    {
        *out = adf_modctx_alloc_from_blocks(NULL, 0);
        return ADF_OK;
    }
    if (q == NULL)
        return ADF_DOMAIN;
    for (i = 0; i < k; i++)
        if (q[i] < 2)
            return ADF_DOMAIN;
    for (i = 0; i < k; i++)
        for (j = 0; j < i; j++)
            if (n_gcd(q[i], q[j]) != 1)
                return ADF_DOMAIN;
    *out = adf_modctx_alloc_from_blocks(q, k);
    return ADF_OK;
}

/* docs/conventions.md 5.14, table row "adf_modctx_new_prime_powers": the blocks p[i]^e[i] in
   the order supplied; DOMAIN if k < 0, a non-prime p[i], a repeated prime, or e[i] = 0;
   UNSUPPORTED if a power reaches 2^64. The argument checks of the DOMAIN kind are done before
   the power computation, so a composite prime wins over an oversized power. */
int
adf_modctx_new_prime_powers(adf_modctx_struct ** out, const ulong * p, const ulong * e, slong k)
{
    ulong * blocks;
    slong i, j;
    int ok = 1;

    if (out == NULL)
        return ADF_DOMAIN;
    if (k < 0)
        return ADF_DOMAIN;
    if (k == 0)
    {
        *out = adf_modctx_alloc_from_blocks(NULL, 0);
        return ADF_OK;
    }
    if (p == NULL || e == NULL)
        return ADF_DOMAIN;
    for (i = 0; i < k; i++)
        if (e[i] == 0 || !n_is_prime(p[i]))
            return ADF_DOMAIN;
    for (i = 0; i < k; i++)
        for (j = 0; j < i; j++)
            if (p[i] == p[j])
                return ADF_DOMAIN;
    blocks = flint_malloc((size_t) k * sizeof(ulong));
    for (i = 0; i < k; i++)
    {
        if (!adf_power_fits_word(p[i], e[i], &blocks[i]))
        {
            ok = 0;
            break;
        }
    }
    if (!ok)
    {
        flint_free(blocks);
        return ADF_UNSUPPORTED;
    }
    *out = adf_modctx_alloc_from_blocks(blocks, k);
    flint_free(blocks);
    return ADF_OK;
}

/* docs/conventions.md 5.14, table row "adf_modctx_new_fmpz": one block K for
   2 <= K < 2^64; no block for K = 1 or K >= 2^64; DOMAIN for K < 1. */
int
adf_modctx_new_fmpz(adf_modctx_struct ** out, const fmpz_t K)
{
    if (out == NULL)
        return ADF_DOMAIN;
    if (fmpz_sgn(K) < 1)
        return ADF_DOMAIN;
    if (fmpz_cmp_ui(K, 2) >= 0 && fmpz_bits(K) <= 64)
    {
        ulong q = fmpz_get_ui(K);
        *out = adf_modctx_alloc_from_blocks(&q, 1);
    }
    else
    {
        *out = adf_modctx_alloc(K, NULL, 0);
    }
    return ADF_OK;
}

/* The blocks of n! as prime powers, in increasing order of the prime (conventions 5.14,
   CV-21). Returns the number of blocks or -1 for UNSUPPORTED. Writes into blocks, which must
   hold room for the primes up to n; the caller knows n <= 65 when this succeeds. */
static slong
adf_factorial_blocks(ulong n, ulong * blocks)
{
    ulong p;
    slong k = 0;

    for (p = 2; p <= n; p++)
    {
        ulong e = 0, pk = p;
        if (!n_is_prime(p))
            continue;
        while (pk <= n)
        {
            e += n / pk;
            if (pk > n / p)
                break;
            pk *= p;
        }
        if (!adf_power_fits_word(p, e, &blocks[k]))
            return -1;
        k++;
    }
    return k;
}

/* docs/conventions.md 5.14, CV-21: K = n!, blocks the prime powers of n! in increasing order
   of the prime. n = 0 and n = 1 give K = 1 without blocks. UNSUPPORTED if a block reaches
   2^64; for n >= 66 this already happens at the prime 2 (v_2(66!) = 64), so the loop over the
   primes runs only for n <= 65. */
int
adf_modctx_new_factorial(adf_modctx_struct ** out, ulong n)
{
    ulong * blocks;
    slong k;

    if (out == NULL)
        return ADF_DOMAIN;
    if (n < 2)
    {
        *out = adf_modctx_alloc_from_blocks(NULL, 0);
        return ADF_OK;
    }
    /* For n >= 66 the first prime power is 2^64 or larger; check it before allocating a
       buffer whose size would follow n. */
    if (n >= 66)
        return ADF_UNSUPPORTED;
    blocks = flint_malloc((size_t) n * sizeof(ulong));
    k = adf_factorial_blocks(n, blocks);
    if (k < 0)
    {
        flint_free(blocks);
        return ADF_UNSUPPORTED;
    }
    *out = adf_modctx_alloc_from_blocks(blocks, k);
    flint_free(blocks);
    return ADF_OK;
}

/* docs/conventions.md 5.14, "powers of a primorial"; docs/PLAN.md section 4. K = (product of
   the primes p <= n)^e; the blocks p^e in increasing order of p. n < 2 or e = 0 gives K = 1.
   UNSUPPORTED if some p^e reaches 2^64. */
int
adf_modctx_new_primorial_pow(adf_modctx_struct ** out, ulong n, ulong e)
{
    ulong * blocks;
    ulong p;
    slong k = 0;

    if (out == NULL)
        return ADF_DOMAIN;
    if (n < 2 || e == 0)
    {
        *out = adf_modctx_alloc_from_blocks(NULL, 0);
        return ADF_OK;
    }
    if (e >= 64)
        return ADF_UNSUPPORTED;            /* the first block 2^e already reaches 2^64 */
    /* e in 1..63. The number of blocks is pi(n); n is the caller's choice and can be large
       for e = 1, where no p^e overflows. Version 1 has no LIMIT status for a raw context
       constructor; the unbounded work for a huge n and e = 1 is recorded in the lane
       report as a finding. */
    {
        ulong pi = n_prime_pi(n);
        blocks = flint_malloc((size_t) pi * sizeof(ulong));
        for (p = 2; p <= n; p = n_nextprime(p, 0))
        {
            if (!adf_power_fits_word(p, e, &blocks[k]))
            {
                flint_free(blocks);
                return ADF_UNSUPPORTED;
            }
            k++;
        }
    }
    *out = adf_modctx_alloc_from_blocks(blocks, k);
    flint_free(blocks);
    return ADF_OK;
}

/* ------------------------------------------------------------------ release and read access */

/* docs/conventions.md 4.6: releases a context made by a constructor; ctx = NULL does
   nothing. */
void
adf_modctx_free(adf_modctx_struct * ctx)
{
    if (ctx == NULL)
        return;
    fmpz_multi_mod_clear(ctx->mod_P);
    fmpz_multi_CRT_clear(ctx->crt_P);
    flint_free(ctx->q);
    fmpz_clear(ctx->K);
    flint_free(ctx);
}

/* docs/conventions.md 5.14: K = the modulus. */
void
adf_modctx_get_modulus(fmpz_t K, const adf_modctx_struct * ctx)
{
    fmpz_set(K, ctx->K);
}

/* docs/conventions.md 5.14: the number of word blocks. */
slong
adf_modctx_nblocks(const adf_modctx_struct * ctx)
{
    return ctx->k;
}

/* docs/conventions.md 5.14: the block q_i; 0 <= i < k is a precondition. */
ulong
adf_modctx_block(const adf_modctx_struct * ctx, slong i)
{
    return ctx->q[i];
}

/* docs/conventions.md 10.1, body "modctx"; 8.1 for the (pointer, length) output:
   "adf1 Q modctx K k q_1 ... q_k", lower-case hexadecimal, one space between tokens. */
char *
adf_modctx_dump_str(size_t * len, const adf_modctx_struct * ctx)
{
    char * Khex;
    char * buf;
    char tmp[32];
    size_t nK, nk, total, pos, i;

    Khex = fmpz_get_str(NULL, 16, ctx->K);
    nK = strlen(Khex);
    nk = adf_hex_len((ulong) ctx->k);
    total = 13;                           /* "adf1 Q modctx" */
    total += 1 + nK + 1 + nk;
    for (i = 0; i < (size_t) ctx->k; i++)
        total += 1 + adf_hex_len(ctx->q[i]);
    buf = flint_malloc(total + 1);
    pos = 0;
    memcpy(buf + pos, "adf1 Q modctx", 13);
    pos += 13;
    buf[pos++] = ' ';
    memcpy(buf + pos, Khex, nK);
    pos += nK;
    buf[pos++] = ' ';
    adf_hex_write(tmp, (ulong) ctx->k);
    memcpy(buf + pos, tmp, nk);
    pos += nk;
    for (i = 0; i < (size_t) ctx->k; i++)
    {
        size_t n;
        buf[pos++] = ' ';
        n = adf_hex_write(tmp, ctx->q[i]);
        memcpy(buf + pos, tmp, n);
        pos += n;
    }
    /* The length computed above and the bytes written must agree; a disagreement would be a
       heap error below (the NUL at buf[pos]). */
    if (pos != total)
        flint_abort();
    buf[pos] = '\0';
    flint_free(Khex);
    *len = pos;
    return buf;
}

/* ------------------------------------------------------------------ context descriptors */

/* docs/conventions.md 10.2, closure C2: K = 1, k = 0, q = NULL. */
void
adf_ctx_desc_init(adf_ctx_desc_t * d)
{
    fmpz_init_set_ui(d->K, 1);
    d->k = 0;
    d->q = NULL;
}

/* docs/conventions.md 10.2, closure C2: releases K and the block array. */
void
adf_ctx_desc_clear(adf_ctx_desc_t * d)
{
    fmpz_clear(d->K);
    flint_free(d->q);
    d->q = NULL;
    d->k = 0;
}

/* docs/conventions.md 10.2: 1 if ctx has the modulus d->K and the blocks d->q[0..d->k-1] in
   this order. */
int
adf_modctx_matches_desc(const adf_modctx_struct * ctx, const adf_ctx_desc_t * d)
{
    slong i;

    if (ctx == NULL || d == NULL)
        return 0;
    if (!fmpz_equal(ctx->K, d->K))
        return 0;
    if (ctx->k != d->k)
        return 0;
    for (i = 0; i < ctx->k; i++)
        if (ctx->q[i] != d->q[i])
            return 0;
    return 1;
}

/* ------------------------------------------------------------------ reduction / recombination */

/* The residues of a modulo the blocks, in block order (docs/proofs/policies.md Definition 16,
   Lemma 17; docs/conventions.md 5.14). The caller passes a context with k >= 1 and an array
   of k ulongs. The scratch of fmpz_comb is local, so two threads may call this on one context
   at the same time (docs/conventions.md 4.5). */
ADF_HIDDEN void
adf_modctx_reduce(const adf_modctx_struct * ctx, const fmpz_t a, ulong * res)
{
    fmpz * outs;
    slong i, k = ctx->k;

    outs = _fmpz_vec_init(k);
    fmpz_multi_mod_precomp(outs, ctx->mod_P, a, 0);
    for (i = 0; i < k; i++)
        res[i] = fmpz_get_ui(&outs[i]);
    _fmpz_vec_clear(outs, k);
}

/* The unique A in [0, K) with A = res[i] mod q_i (docs/proofs/policies.md Lemma 17.1). The
   caller passes a context with k >= 1 and residues already reduced. */
ADF_HIDDEN void
adf_modctx_recombine(fmpz_t out, const adf_modctx_struct * ctx, const ulong * res)
{
    fmpz * ins;
    slong i, k = ctx->k;

    ins = _fmpz_vec_init(k);
    for (i = 0; i < k; i++)
        fmpz_set_ui(&ins[i], res[i]);
    fmpz_multi_CRT_precomp(out, ctx->crt_P, ins, 0);
    _fmpz_vec_clear(ins, k);
}

/* ------------------------------------------------------------------ dump loader: body "modctx" */

/* A cursor is the byte string plus a position. The grammar of the dump form is conventions
   10.1; a token `h` is `"0" | ["-"] , hnz , { hdig }`, lower-case hexadecimal, no leading
   zeros. No function here relies on a NUL terminator, and no raw input is passed to a FLINT
   string function: a validated token is copied into a buffer of its own (8.1, closure D9). */

/* Parse the `h` token at *pos (which is not a space) and stop at the next space or at len.
   The token must satisfy the grammar. On success writes its span and advances *pos. The span
   is written before the validation so that a caller that ignores the return value never reads
   an uninitialised span; the caller always checks the return value. */
static int
adf_dump_h(const char * s, size_t len, size_t * pos, const char ** start, size_t * n)
{
    size_t i = *pos, b = i, j;

    while (i < len && s[i] != ' ')
        i++;
    if (i == b)
        return 0;
    *start = s + b;
    *n = i - b;
    *pos = i;
    if (i - b == 1 && s[b] == '0')
        return 1;                      /* the token "0" */
    j = b;
    if (s[j] == '-')
        j++;
    if (j >= i || !adf_is_hexnonzero(s[j]))
        return 0;
    for (j++; j < i; j++)
        if (!adf_is_hexdigit(s[j]))
            return 0;
    return 1;
}

/* Parse a validated `h` token into a size_t count. The grammar allows a sign; a negative count
   and one that does not fit are grammar failures (the reference T.count does the same). */
static int
adf_dump_count(const char * s, size_t n, size_t * out)
{
    size_t i, v = 0;

    if (n == 0)
        return 0;
    if (s[0] == '-')
        return 0;
    for (i = 0; i < n; i++)
    {
        size_t d;
        char c = s[i];
        if (c >= '0' && c <= '9')
            d = (size_t) (c - '0');
        else
            d = (size_t) (c - 'a' + 10);
        if (v > ((size_t) -1 - d) / 16)
            return 0;
        v = v * 16 + d;
    }
    *out = v;
    return 1;
}

/* Parse a validated `h` token into a word. A leading '-' or a value that does not fit a word
   is reported through *bad (semantic DOMAIN at stage 6, not a grammar failure). */
static int
adf_dump_word(const char * s, size_t n, ulong * out, int * bad)
{
    size_t i;
    ulong v = 0;

    if (n == 0)
        return 0;
    if (s[0] == '-')
    {
        *bad = 1;
        return 1;
    }
    for (i = 0; i < n; i++)
    {
        unsigned d;
        char c = s[i];
        if (c >= '0' && c <= '9')
            d = (unsigned) (c - '0');
        else
            d = (unsigned) (c - 'a' + 10);
        if (v > ((ulong) -1 - d) / 16)
        {
            *bad = 1;
            return 1;
        }
        v = v * 16 + d;
    }
    *out = v;
    return 1;
}

/* Copy a validated token and hand it to fmpz_set_str, so that no raw input reaches FLINT. */
static void
adf_dump_fmpz(fmpz_t v, const char * s, size_t n)
{
    char * buf = flint_malloc(n + 1);
    memcpy(buf, s, n);
    buf[n] = '\0';
    fmpz_set_str(v, buf, 16);
    flint_free(buf);
}

/* docs/conventions.md 10.2, closure C3: the context recorded at the occurrence-th context
   occurrence of the dump text s. The body implemented here is "modctx" (10.1): the header
   `adf1 Q ` is validated, then `modctx K k q_1 ... q_k`. The whole text is validated in the
   order of 8.5 before the occurrence index is checked or a context is allocated. A `modctx`
   body has exactly one context occurrence. Nested occurrences of other bodies are not handled
   here; the dump loaders of lane m1-text own those (HEADER-FINDING, see the lane report). */
int
adf_modctx_new_from_dump(adf_modctx_struct ** out, const char * s, size_t len, size_t occurrence,
                         const adf_text_limits_t * lim)
{
    adf_text_limits_t dflt;
    const char * Ktext, * ktext;
    size_t Klen, klen;
    size_t pos, i, kcount, qblocks;
    ulong k, * blocks;
    int bad = 0;
    fmpz_t K;

    if (out == NULL)
        return ADF_DOMAIN;
    if (lim == NULL)
    {
        adf_text_limits_default(&dflt);
        lim = &dflt;
    }
    /* stage 1: length, before any byte is read */
    if (len > lim->max_len)
        return ADF_LIMIT;
    /* stage 2: the alphabet of 8.2. A dump uses exactly one space between tokens; TAB, LF
       and CR are rejected with the same status, as they are forbidden here. */
    if (s == NULL)
        return ADF_PARSE;
    for (i = 0; i < len; i++)
        if ((unsigned char) s[i] < 0x20 || (unsigned char) s[i] > 0x7e)
            return ADF_PARSE;
    /* stage 3: the header and the grammar */
    if (len < 3 || s[0] != 'a' || s[1] != 'd' || s[2] != 'f')
        return ADF_PARSE;
    pos = 3;
    {
        size_t v0 = pos;
        while (pos < len && s[pos] >= '0' && s[pos] <= '9')
            pos++;
        if (pos == v0)
            return ADF_PARSE;
        if (pos - v0 > 1 && s[v0] == '0')
            return ADF_PARSE;
        if (!(pos - v0 == 1 && s[v0] == '1'))
            return ADF_UNSUPPORTED;
    }
    if (pos >= len || s[pos] != ' ')
        return ADF_PARSE;
    pos++;
    {
        size_t f0 = pos;
        if (pos >= len || !adf_is_upper(s[pos]))
            return ADF_PARSE;
        while (pos < len && s[pos] != ' ')
            pos++;
        if (!(pos - f0 == 1 && s[f0] == 'Q'))
            return ADF_UNSUPPORTED;
    }
    if (pos >= len || s[pos] != ' ')
        return ADF_PARSE;
    pos++;
    if (!(pos + 6 <= len && memcmp(s + pos, "modctx", 6) == 0 &&
          (pos + 6 == len || s[pos + 6] == ' ')))
        return ADF_PARSE;
    pos += 6;
    /* stage 3, continued: the ctx tokens. The grammar is validated in full first. */
    if (pos >= len || s[pos] != ' ')
        return ADF_PARSE;
    pos++;
    if (!adf_dump_h(s, len, &pos, &Ktext, &Klen))
        return ADF_PARSE;
    if (pos >= len || s[pos] != ' ')
        return ADF_PARSE;
    pos++;
    if (!adf_dump_h(s, len, &pos, &ktext, &klen))
        return ADF_PARSE;
    if (!adf_dump_count(ktext, klen, &kcount))
        return ADF_PARSE;
    qblocks = pos;
    for (i = 0; i < kcount; i++)
    {
        const char * btext;
        size_t bn;
        if (pos >= len || s[pos] != ' ')
            return ADF_PARSE;
        pos++;
        if (!adf_dump_h(s, len, &pos, &btext, &bn))
            return ADF_PARSE;
    }
    if (pos != len)
        return ADF_PARSE;
    /* stage 4: the block count of the context occurrence, before any semantic check */
    if (lim->max_items < 0 || kcount > (size_t) lim->max_items)
        return ADF_LIMIT;
    /* stage 6: semantic validation. Extract the values now that the grammar holds and the
       count is within the limit. */
    fmpz_init(K);
    adf_dump_fmpz(K, Ktext, Klen);
    blocks = kcount > 0 ? flint_malloc(kcount * sizeof(ulong)) : NULL;
    {
        size_t q = qblocks;
        for (i = 0; i < kcount; i++)
        {
            const char * btext;
            size_t bn;
            ulong v = 0;
            q++;
            if (!adf_dump_h(s, len, &q, &btext, &bn))
            {                              /* cannot happen after the validating pass */
                fmpz_clear(K);
                flint_free(blocks);
                return ADF_PARSE;
            }
            adf_dump_word(btext, bn, &v, &bad);
            blocks[i] = v;
        }
    }
    k = (ulong) kcount;
    /* the predicate of 5.14 */
    if (fmpz_sgn(K) < 1)
        bad = 1;
    if (!bad && k > 0)
    {
        size_t j;
        for (i = 0; i < (size_t) k; i++)
            if (blocks[i] < 2)
            {
                bad = 1;
                break;
            }
        if (!bad)
            for (i = 0; i < (size_t) k && !bad; i++)
                for (j = 0; j < i; j++)
                    if (n_gcd(blocks[i], blocks[j]) != 1)
                    {
                        bad = 1;
                        break;
                    }
        if (!bad)
        {
            fmpz_t prod;
            fmpz_init_set_ui(prod, 1);
            for (i = 0; i < (size_t) k; i++)
                fmpz_mul_ui(prod, prod, blocks[i]);
            if (!fmpz_equal(prod, K))
                bad = 1;
            fmpz_clear(prod);
        }
    }
    if (bad)
    {
        fmpz_clear(K);
        flint_free(blocks);
        return ADF_DOMAIN;
    }
    if (occurrence != 0)
    {
        fmpz_clear(K);
        flint_free(blocks);
        return ADF_DOMAIN;
    }
    *out = adf_modctx_alloc(K, blocks, (slong) k);
    fmpz_clear(K);
    flint_free(blocks);
    return ADF_OK;
}
