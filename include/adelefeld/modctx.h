/* adelefeld/modctx.h: modulus contexts (constructors, free, read access), context descriptors of
   dump occurrences, and the conversions of adf_fball into and out of the local backend.

   Contract: docs/conventions.md 0.4, 4.6 (lifetime and construction; closure edits E1, E2), 5.14
   (contents and constructors), 3.2 (rows "Raw context constructors", "adf_modctx_new_from_dump",
   "Exact conversion to the local backend"; closure findings C3, C5), 5.3 (local backend), 10.2
   (descriptors and inspection; closure finding C2), 12.4, 12.6, 12.10; docs/SPEC.md 10 item 4;
   docs/PLAN.md section 4 ("Contexts"); docs/proofs/policies.md section 4. Contexts are implemented
   in work package 1.8 (the scaled policy of 1.7 already uses them; docs/PLAN.md section 6).

   Rules for every constructor adf_modctx_new_* (conventions 4.6, 5.14; closure E2):
   - out is the first argument, of type adf_modctx_struct **. On ADF_OK the constructor allocates
     and fully initialises an immutable context and writes its pointer to *out. It never frees,
     mutates or reuses the context previously pointed to by *out; on success it overwrites only the
     pointer slot, and the caller must retain any previous owned pointer separately.
   - On any other status *out is untouched and no allocation is retained.
   - The caller owns the context. It frees it with adf_modctx_free only after every value that
     borrows it has been cleared or moved to another context or backend; freeing a borrowed context
     is undefined behaviour (with -DADF_CHECK_INVARIANTS it aborts, conventions 4.6).
   - Arrays passed in are copied (conventions 4.2). No operation of the library creates a context
     implicitly (conventions 4.6).
   - A context may be read by any number of threads (conventions 4.5).
   - Size (decision M1-D5): a context has at most ADF_MODCTX_MAX_BLOCKS blocks. A constructor
     whose arguments ask for more returns ADF_UNSUPPORTED, and it decides this from the arguments
     before it builds a table or allocates in proportion to them: adf_modctx_new_blocks and
     adf_modctx_new_prime_powers from k; adf_modctx_new_factorial and
     adf_modctx_new_primorial_pow from n (the number of primes up to n is above the bound as soon
     as n >= ADF_MODCTX_MAX_PRIME, the prime of index ADF_MODCTX_MAX_BLOCKS + 1). For
     adf_modctx_new_primorial_pow with e >= 2 the status for "some p^e >= 2^64" is likewise
     decided from n and e alone, by comparing n with the largest prime whose e-th power is below
     2^64, before any prime is enumerated. The order of the checks is: ADF_DOMAIN first, then
     ADF_UNSUPPORTED (conventions 3.3).

   Contents, not part of the interface (conventions 5.14): K >= 1; k >= 0 word blocks q_1..q_k,
   pairwise coprime, 2 <= q_i < 2^64, in the order supplied, with product K when k >= 1. */

#ifndef ADELEFELD_MODCTX_H
#define ADELEFELD_MODCTX_H

#include "adelefeld/common.h"
#include "adelefeld/status.h"
#include "adelefeld/fball.h"
#include "adelefeld/text.h"

/* The largest number of blocks of a context, and the prime of index ADF_MODCTX_MAX_BLOCKS + 1
   (decision M1-D5). 65536 blocks of one word are a modulus of about 4 million bits. The 65537th
   prime is 821647 (the 65536th is 821641). */
#define ADF_MODCTX_MAX_BLOCKS 65536
#define ADF_MODCTX_MAX_PRIME 821647

#ifdef __cplusplus
extern "C" {
#endif

/* ---- construction and release ---- */

/* adf_modctx_new_blocks(out, q, k): the context with the k blocks q[0..k-1], in the order supplied,
   K = their product; k = 0 gives K = 1 without blocks. Status: ADF_OK; ADF_DOMAIN if k < 0, or
   unless every q[i] >= 2 and the q[i] are pairwise coprime (conventions 5.14). q is read only
   (it may be NULL when k = 0). Cost: k^2 word gcds and the tables of
   the context. */
int adf_modctx_new_blocks(adf_modctx_struct ** out, const ulong * q, slong k);

/* adf_modctx_new_prime_powers(out, p, e, k): the context with the blocks p[i]^e[i], i = 0..k-1, in
   the order supplied; k = 0 gives K = 1 without blocks. Status: ADF_OK; ADF_DOMAIN if k < 0, if
   some p[i] is not prime (n_is_prime),
   if a prime repeats, or if some e[i] = 0; ADF_UNSUPPORTED if some p[i]^e[i] >= 2^64 (conventions
   5.14; 3.2 row "Raw context constructors"). */
int adf_modctx_new_prime_powers(adf_modctx_struct ** out, const ulong * p, const ulong * e, slong k);

/* adf_modctx_new_fmpz(out, K): the context of modulus K with one block K if 2 <= K < 2^64, and no
   block if K = 1 or K >= 2^64; a context without blocks serves only the scaled policy
   (conventions 5.14, last paragraph). Status: ADF_OK; ADF_DOMAIN if K < 1. */
int adf_modctx_new_fmpz(adf_modctx_struct ** out, const fmpz_t K);

/* adf_modctx_new_factorial(out, n): the context of modulus K = n!, with the prime powers of n! as
   blocks in increasing order of the prime (conventions 5.14, CV-21); n = 0 and n = 1 give K = 1
   without blocks. Status: ADF_OK; ADF_UNSUPPORTED if one of these prime powers is >= 2^64
   (conventions 3.2 row "Raw context constructors"; the choice between UNSUPPORTED and a context
   without blocks is recorded in docs/api-m1.md, section "Choices"). */
int adf_modctx_new_factorial(adf_modctx_struct ** out, ulong n);

/* adf_modctx_new_primorial_pow(out, n, e): the context of modulus K = (product of the primes
   p <= n)^e, with the blocks p^e in increasing order of p (conventions 5.14, "powers of a
   primorial"; PLAN section 4, "Modulus families"). n < 2 or e = 0 gives K = 1 without blocks.
   Status: ADF_OK; ADF_UNSUPPORTED if some p^e >= 2^64. */
int adf_modctx_new_primorial_pow(adf_modctx_struct ** out, ulong n, ulong e);

/* adf_modctx_new_from_dump(out, s, len, occurrence, lim): the context recorded at the
   occurrence-th context occurrence (0-based, dump traversal order) of the dump text s (conventions
   10.2; closure findings C2, C3). A "modctx" dump has exactly one occurrence. The whole dump is
   validated in the order of conventions 8.5 before the occurrence index is checked or a context
   allocated. Status: ADF_OK; ADF_PARSE, ADF_LIMIT, ADF_UNSUPPORTED, ADF_DOMAIN as a loader of the
   dump (conventions 3.2 row "adf_modctx_new_from_dump"); ADF_DOMAIN also if occurrence is not
   below the number of occurrences. The block order of the occurrence is kept. */
int adf_modctx_new_from_dump(adf_modctx_struct ** out, const char * s, size_t len, size_t occurrence,
                             const adf_text_limits_t * lim);

/* adf_modctx_free(ctx): releases a context made by a constructor. ctx = NULL does nothing
   (conventions 4.6). */
void adf_modctx_free(adf_modctx_struct * ctx);

/* ---- read access (a context is immutable; these never fail) ---- */

/* adf_modctx_get_modulus(K, ctx): K = the modulus of ctx. K is an fmpz initialised by the caller. */
void adf_modctx_get_modulus(fmpz_t K, const adf_modctx_struct * ctx);

/* adf_modctx_nblocks(ctx): the number k >= 0 of word blocks. */
slong adf_modctx_nblocks(const adf_modctx_struct * ctx);

/* adf_modctx_block(ctx, i): the block q_i, 0 <= i < k (precondition; undefined otherwise). */
ulong adf_modctx_block(const adf_modctx_struct * ctx, slong i);

/* adf_modctx_dump_str(len, ctx): the dump "adf1 Q modctx K k q_1 ... q_k" (conventions 10.1, body
   "modctx"; lower-case hexadecimal). Returned as adf_x_dump_str returns (conventions 8.1): allocated
   with flint_malloc, byte length in *len, NUL at s[len], freed with adf_str_free. */
char * adf_modctx_dump_str(size_t * len, const adf_modctx_struct * ctx);

/* ---- descriptors of context occurrences (conventions 10.2; closure finding C2) ---- */

/* One context occurrence of a dump: modulus K, block count k, blocks q[0..k-1] owned by the
   descriptor. Layout, 64-bit: K (fmpz) at 0, k (slong) at 8, q (ulong *) at 16; size 24, alignment 8
   (conventions 10.2, 12.4). The name ends in _t although the type is a plain struct, as conventions
   10.2 writes it. */
typedef struct
{
    fmpz_t K;
    slong k;
    ulong * q;
} adf_ctx_desc_t;

/* adf_ctx_desc_init(d): K = 1, k = 0, q = NULL. Never fails (conventions 10.2, closure C2). */
void adf_ctx_desc_init(adf_ctx_desc_t * d);

/* adf_ctx_desc_clear(d): releases K and the block array (conventions 10.2, closure C2). */
void adf_ctx_desc_clear(adf_ctx_desc_t * d);

/* adf_modctx_matches_desc(ctx, d): 1 if ctx has the modulus d->K and the blocks d->q[0..k-1] in
   this order, else 0. This is the match that a dump binding requires (conventions 10.2: "Every
   binding must match that occurrence's modulus and ordered blocks"). */
int adf_modctx_matches_desc(const adf_modctx_struct * ctx, const adf_ctx_desc_t * d);

/* ---- adf_fball: conversion into and out of the local backend (conventions 5.3; WP 1.8) ---- */

/* adf_fball_set_local(y, x, ctx): y = the set of x as a local value of ctx, exactly
   (conventions 5.3, "Conversion into a context"; policies Proposition 19, line 347): the ball
   c + R Zhat, R > 0, lives in a context of modulus K exactly when K/R and c K/R are integers; then
   d = K/R and res[i] = (c K/R) mod q_i. y borrows ctx.
   Status: ADF_OK; ADF_DOMAIN if the set is not a local value of ctx (an exact value included:
   exact values are never local, policies Definition 16); ADF_UNSUPPORTED if ctx has no block
   (conventions 5.14, 3.2 row "Exact conversion to the local backend"). y untouched on failure.
   Aliasing: y may be x. Cost: one division and k word reductions. */
int adf_fball_set_local(adf_fball_t y, const adf_fball_t x, const adf_modctx_struct * ctx);

/* adf_fball_set_local_enclose(y, lost, x, ctx): y = the best enclosure of x among the local values
   of ctx (policies Proposition 20, line 361): with K/R = n/m in lowest terms and e the denominator
   of a rational point c of x, d0 = lcm(n, e), and y = (c d0 + K Zhat)/d0. If lost is not NULL,
   *lost = 1 when the set changes (d0 != K/R), else 0 (conventions 5.3). y borrows ctx.
   Status: ADF_OK; ADF_DOMAIN if x is exact (R = 0: no local value contains a point only, policies
   Definition 16); ADF_UNSUPPORTED if ctx has no block. On failure y and *lost are untouched.
   Aliasing: y may be x. (Conventions 3.2 has no row for this function; the statuses above are the
   choice recorded in docs/api-m1.md, section "Choices".) */
int adf_fball_set_local_enclose(adf_fball_t y, int * lost, const adf_fball_t x,
                                const adf_modctx_struct * ctx);

/* adf_fball_set_global(y, x): y = x in the global backend: recombination by CRT and canonical
   cancellation (conventions 5.3; policies Lemma 17.3 and Proposition 24.1). A global x is copied.
   The set is unchanged. Aliasing: y may be x. */
void adf_fball_set_global(adf_fball_t y, const adf_fball_t x);

/* adf_fball_is_local(x): 1 if backend = ADF_LOCAL, else 0. adf_fball_context(x): the borrowed
   context of a local value, NULL for a global one. */
int adf_fball_is_local(const adf_fball_t x);
const adf_modctx_struct * adf_fball_context(const adf_fball_t x);

#ifdef __cplusplus
}
#endif

#endif /* ADELEFELD_MODCTX_H */
