/* adelefeld/fball_local.c: conversion of adf_fball into and out of the local backend, and the two
   inspection functions (include/adelefeld/modctx.h, last block; work package 1.8).

   Ground truth: docs/proofs/policies.md section 4. A local value (d; r_1..r_k) of a context with
   blocks q_i and modulus K is the set (A + K Zhat)/d, A any integer with A = r_i mod q_i
   (Definition 16, line 305; Lemma 17, line 312). Stored form: predicate L of fball.h and
   docs/conventions.md 5.3 (A = 0, H = K, d >= 1, 0 <= res[i] < q_i; raw data, no gcd condition,
   CV-55).

   Rules for the C code of the local backend (policies.md, after Proposition 25, line 526): the
   residues are those of the integer numerator; d is stored once per value and applied only after
   recombination; no modular inverse is computed in this file at all. The denominators are only
   multiplied and divided exactly as integers (P25.4, line 508).

   The residues are computed by adf_modctx_reduce and recombined by adf_modctx_recombine of
   src/modctx_internal.h (each allocates a vector of k fmpz per call; a cost noted in the report of
   lane m1-local and not repaired here).

   Aliasing (fball.h "Common rules"; modctx.h): the output may be the input. Every function reads
   the input completely into temporaries before it writes the output, and on a status other than
   ADF_OK writes nothing (docs/conventions.md 4.3). */

#include "adelefeld/modctx.h"
#include "modctx_internal.h"

#include <flint/fmpz.h>
#include <flint/fmpq.h>
#include <flint/flint.h>

/* The canonical global triple of the set of x (fball.h adf_fball_get_fmpz3): a copy for a global
   x; for a local x the CRT lift A0 in [0, K) (Lemma 17.3, line 319), g = gcd(A0, K, d), and
   (A0/g, K/g, d/g) (Proposition 24.1, line 462; Lemma 18, line 331, is the identity
   g = gcd(product of gcd(r_i, q_i), d); here g is taken from the recombined A0, as the header's
   cost line says: "a CRT recombination and a gcd"). A0/g lies in [0, K/g) because g | A0, so no
   reduction is needed (Summary 26, proof of the canonical column, line 564). */
static void
fl_triple(fmpz_t A, fmpz_t H, fmpz_t d, const adf_fball_t x)
{
    fmpz_t g;

    if (x->backend != ADF_LOCAL)
    {
        fmpz_set(A, x->A);
        fmpz_set(H, x->H);
        fmpz_set(d, x->d);
        return;
    }
    adf_modctx_recombine(A, x->mctx, x->res);
    adf_modctx_get_modulus(H, x->mctx);
    fmpz_set(d, x->d);
    fmpz_init(g);
    fmpz_gcd3(g, A, H, d);
    fmpz_divexact(A, A, g);
    fmpz_divexact(H, H, g);
    fmpz_divexact(d, d, g);
    fmpz_clear(g);
}

/* y = the local value (d; residues of N) of ctx. N is any integer (adf_modctx_reduce reduces into
   [0, q_i)). The residue array is computed before y is touched, and the old array of y is freed
   only then, so d and N may be fields of y. */
static void
fl_store_local(adf_fball_t y, const adf_modctx_struct * ctx, const fmpz_t d, const fmpz_t N)
{
    slong k = adf_modctx_nblocks(ctx);
    ulong * res = (ulong *) flint_malloc(k * sizeof(ulong));

    adf_modctx_reduce(ctx, N, res);
    flint_free(y->res);
    y->res = res;
    fmpz_set(y->d, d);
    fmpz_zero(y->A);
    adf_modctx_get_modulus(y->H, ctx);
    y->backend = ADF_LOCAL;
    y->mctx = ctx;
}

/* Proposition 19 (policies.md:347): the ball c + R Zhat, R > 0, is a local value of a context of
   modulus K exactly when K/R and c K/R are integers; then d = K/R and r_i = (c K/R) mod q_i. With
   the canonical triple (A, H, d) of x: c = A/d, R = H/d, K/R = K d/H and c K/R = A K/H.
   Statuses (modctx.h; conventions 3.2 row "Exact conversion to the local backend"): UNSUPPORTED
   for a context without blocks (conventions 5.14), checked first: it is also the larger status
   when both apply (conventions 3.3, maximum); DOMAIN for an exact x (Definition 16) or when the
   two quotients are not integers. */
int
adf_fball_set_local(adf_fball_t y, const adf_fball_t x, const adf_modctx_struct * ctx)
{
    fmpz_t A, H, d, K, u, v;
    int status = ADF_OK;

    if (adf_modctx_nblocks(ctx) < 1)
        return ADF_UNSUPPORTED;

    fmpz_init(A);
    fmpz_init(H);
    fmpz_init(d);
    fmpz_init(K);
    fmpz_init(u);
    fmpz_init(v);
    fl_triple(A, H, d, x);
    if (fmpz_is_zero(H))
    {
        status = ADF_DOMAIN;
        goto done;
    }
    adf_modctx_get_modulus(K, ctx);
    fmpz_mul(u, K, d);                    /* K/R = K d/H */
    fmpz_mul(v, K, A);                    /* c K/R = A K/H */
    if (!fmpz_divisible(u, H) || !fmpz_divisible(v, H))
    {
        status = ADF_DOMAIN;
        goto done;
    }
    fmpz_divexact(u, u, H);
    fmpz_divexact(v, v, H);
    fl_store_local(y, ctx, u, v);

done:
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
    fmpz_clear(K);
    fmpz_clear(u);
    fmpz_clear(v);
    return status;
}

/* Proposition 20 (policies.md:361): X = c + R Zhat, R > 0, c a rational point of X. With
   K/R = n/m in lowest terms and e the denominator of c, d0 = lcm(n, e) and A0 = c d0; the local
   value (A0 + K Zhat)/d0 contains X, lies inside every local value of the context that contains
   X, and equals X exactly when d0 = K/R. Here c = A/d from the canonical triple, so
   e = d/gcd(A, d), and A0 = A d0/d is an integer because e | d0.
   *lost = 1 exactly when d0 != K/R (as rationals: m != 1 or d0 != n).
   Statuses (modctx.h; docs/api-m1.md "Choices" item 6): UNSUPPORTED for a context without blocks,
   first (conventions 3.3); DOMAIN for an exact x (Definition 16). */
int
adf_fball_set_local_enclose(adf_fball_t y, int * lost, const adf_fball_t x,
                            const adf_modctx_struct * ctx)
{
    fmpz_t A, H, d, K, e, d0, A0;
    fmpq_t KR;
    int is_lost;

    if (adf_modctx_nblocks(ctx) < 1)
        return ADF_UNSUPPORTED;

    fmpz_init(A);
    fmpz_init(H);
    fmpz_init(d);
    fl_triple(A, H, d, x);
    if (fmpz_is_zero(H))
    {
        fmpz_clear(A);
        fmpz_clear(H);
        fmpz_clear(d);
        return ADF_DOMAIN;
    }

    fmpz_init(K);
    fmpz_init(e);
    fmpz_init(d0);
    fmpz_init(A0);
    fmpq_init(KR);
    adf_modctx_get_modulus(K, ctx);
    fmpz_mul(e, K, d);
    fmpq_set_fmpz_frac(KR, e, H);                            /* K/R = n/m, lowest terms */
    fmpz_gcd(e, A, d);
    fmpz_divexact(e, d, e);                                  /* e = den(A/d) */
    fmpz_lcm(d0, fmpq_numref(KR), e);                        /* d0 = lcm(n, e) */
    fmpz_mul(A0, A, d0);
    fmpz_divexact(A0, A0, d);                                /* A0 = c d0 */
    is_lost = !fmpz_is_one(fmpq_denref(KR)) || !fmpz_equal(d0, fmpq_numref(KR));
    fl_store_local(y, ctx, d0, A0);
    if (lost != NULL)
        *lost = is_lost;

    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
    fmpz_clear(K);
    fmpz_clear(e);
    fmpz_clear(d0);
    fmpz_clear(A0);
    fmpq_clear(KR);
    return ADF_OK;
}

/* Conversion to the global backend: recombination by CRT and canonical cancellation (Lemma 17.3,
   Proposition 24.1; conventions 5.3). A global x is copied. The set is unchanged. */
void
adf_fball_set_global(adf_fball_t y, const adf_fball_t x)
{
    fmpz_t A, H, d;

    fmpz_init(A);
    fmpz_init(H);
    fmpz_init(d);
    fl_triple(A, H, d, x);
    flint_free(y->res);
    y->res = NULL;
    fmpz_swap(y->A, A);
    fmpz_swap(y->H, H);
    fmpz_swap(y->d, d);
    y->backend = ADF_GLOBAL;
    y->mctx = NULL;
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
}

int
adf_fball_is_local(const adf_fball_t x)
{
    return x->backend == ADF_LOCAL;
}

const adf_modctx_struct *
adf_fball_context(const adf_fball_t x)
{
    return x->backend == ADF_LOCAL ? x->mctx : NULL;
}
