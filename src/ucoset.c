/* src/ucoset.c: adf_ucoset, unit cosets with the exact units (milestone 2, slice 1, lane i-slice1).

   Contract: include/adelefeld/ucoset.h. Sources, read before the code was written:
   - docs/conventions.md 5.6 (struct, predicate, residue range 1..N, modulus as supplied, normal form,
     containment with exact units), 2.3, 3.2, 4.1, 4.3, 4.4;
   - docs/SPEC.md 5 ("Unit precision", "Exact units": U(0) = {1}, the product rule with gcd(0, N') = N');
   - docs/proofs/ideles.md Definition 4 (line 87), Lemma 7 (line 132), Proposition 9 (line 152),
     Proposition 10 (line 195), Proposition 11 (line 213);
   - docs/api-2.md 1.3, Statements A (exact units), B (normal form), C (product and inverse as computed);
   - FLINT 3.0.1: fmpz_invmod, refs/src/flint-3.0.1/fmpz.rst:1154-1160 (the inverse modulo h = +-1 is 0);
     fmpz_fdiv_r, fmpz.rst:824; fmpz_gcd, fmpz.rst:1040-1044.
   Reference: proto/ideles_checks.py, ref_uc_* (part 2). */

#include <adelefeld.h>

#include "invariants.h"

/* Entry check of the debug build (conventions 4.4, CV-09; src/invariants.h). */
#ifdef ADF_CHECK_INVARIANTS
#define UC_INV(x)                                                                                      \
    do { if (!adf_ucoset_is_canonical(x)) adf_inv_fail(__func__, #x, "adf_ucoset"); } while (0)
#else
#define UC_INV(x) ((void) 0)
#endif

/* ---- helpers ---- */

/* c = the residue of c in 1..N, N >= 1 (conventions 5.6, CV-16: the residue 0 occurs only for N = 1,
   and is written N = 1). */
static void
uc_reduce(fmpz_t c, const fmpz_t N)
{
    fmpz_fdiv_r(c, c, N);
    if (fmpz_is_zero(c))
        fmpz_set(c, N);
}

/* (c, N) = its normal form, for a canonical pair (conventions 5.6; api-2.md Statement B): N' = N/2 if
   N = 2 mod 4, else N; c reduced into 1..N'. An exact unit (N = 0) is unchanged. */
static void
uc_normal_inplace(fmpz_t c, fmpz_t N)
{
    if (fmpz_is_zero(N))
        return;
    if (fmpz_fdiv_ui(N, 4) == 2)
        fmpz_fdiv_q_2exp(N, N, 1);
    uc_reduce(c, N);
}

/* g = gcd(N, N') with gcd(0, N') = N' (SPEC 5, "Exact units"), and gcd(0, 0) = 0. */
static void
uc_gcd(fmpz_t g, const fmpz_t N, const fmpz_t N2)
{
    if (fmpz_is_zero(N))
        fmpz_set(g, N2);
    else if (fmpz_is_zero(N2))
        fmpz_set(g, N);
    else
        fmpz_gcd(g, N, N2);
}

/* ---- life cycle ---- */

void
adf_ucoset_init(adf_ucoset_t x)
{
    fmpz_init_set_ui(x->c, 1);
    fmpz_init(x->N);
}

void
adf_ucoset_clear(adf_ucoset_t x)
{
    fmpz_clear(x->c);
    fmpz_clear(x->N);
}

void
adf_ucoset_set(adf_ucoset_t y, const adf_ucoset_t x)
{
    UC_INV(x);
    fmpz_set(y->c, x->c);
    fmpz_set(y->N, x->N);
}

void
adf_ucoset_swap(adf_ucoset_t x, adf_ucoset_t y)
{
    fmpz_swap(x->c, y->c);
    fmpz_swap(x->N, y->N);
}

/* conventions 5.6: (N >= 1 and 1 <= c <= N and gcd(c, N) = 1) or (N = 0 and c in {1, -1}). */
int
adf_ucoset_is_canonical(const adf_ucoset_t x)
{
    int ok;
    fmpz_t g;

    if (fmpz_is_zero(x->N))
        return fmpz_is_one(x->c) || fmpz_equal_si(x->c, -1);
    if (fmpz_sgn(x->N) < 0 || fmpz_sgn(x->c) <= 0 || fmpz_cmp(x->c, x->N) > 0)
        return 0;
    fmpz_init(g);
    fmpz_gcd(g, x->c, x->N);
    ok = fmpz_is_one(g);
    fmpz_clear(g);
    return ok;
}

int
adf_ucoset_is_normal(const adf_ucoset_t x)
{
    if (!adf_ucoset_is_canonical(x))
        return 0;
    return fmpz_is_zero(x->N) || fmpz_fdiv_ui(x->N, 4) != 2;
}

int
adf_ucoset_identical(const adf_ucoset_t x, const adf_ucoset_t y)
{
    UC_INV(x);
    UC_INV(y);
    return fmpz_equal(x->c, y->c) && fmpz_equal(x->N, y->N);
}

/* ---- constructors and accessors ---- */

int
adf_ucoset_set_fmpz2(adf_ucoset_t x, const fmpz_t c, const fmpz_t N)
{
    fmpz_t tc, tN;
    int ok;

    if (fmpz_sgn(N) < 0)
        return ADF_DOMAIN;
    if (fmpz_is_zero(N))
    {
        /* the exact unit (M0-D1): c = +-1 only */
        if (!fmpz_is_one(c) && !fmpz_equal_si(c, -1))
            return ADF_DOMAIN;
        fmpz_set(x->c, c);
        fmpz_zero(x->N);
        return ADF_OK;
    }
    /* c and N may be members of x: compute into temporaries, then move (conventions 4.1, 4.3). */
    fmpz_init(tc);
    fmpz_init(tN);
    fmpz_gcd(tc, c, N);
    ok = fmpz_is_one(tc);
    if (ok)
    {
        fmpz_set(tc, c);
        fmpz_set(tN, N);
        uc_reduce(tc, tN);
        fmpz_swap(x->c, tc);
        fmpz_swap(x->N, tN);
    }
    fmpz_clear(tc);
    fmpz_clear(tN);
    return ok ? ADF_OK : ADF_DOMAIN;
}

void
adf_ucoset_one(adf_ucoset_t x)
{
    fmpz_one(x->c);
    fmpz_zero(x->N);
}

void
adf_ucoset_minus_one(adf_ucoset_t x)
{
    fmpz_set_si(x->c, -1);
    fmpz_zero(x->N);
}

void
adf_ucoset_get_fmpz2(fmpz_t c, fmpz_t N, const adf_ucoset_t x)
{
    fmpz_t tc, tN;

    UC_INV(x);
    /* c or N may be a member of x: both are read before either is written. */
    fmpz_init_set(tc, x->c);
    fmpz_init_set(tN, x->N);
    fmpz_swap(c, tc);
    fmpz_swap(N, tN);
    fmpz_clear(tc);
    fmpz_clear(tN);
}

int
adf_ucoset_is_exact(const adf_ucoset_t x)
{
    UC_INV(x);
    return fmpz_is_zero(x->N);
}

void
adf_ucoset_normalise(adf_ucoset_t y, const adf_ucoset_t x)
{
    UC_INV(x);
    adf_ucoset_set(y, x);
    uc_normal_inplace(y->c, y->N);
}

/* ---- set predicates ---- */

/* api-2.md A.5 and ideles.md P9.2 (line 157): the same set exactly when the normal forms are equal. */
int
adf_ucoset_equal_set(const adf_ucoset_t x, const adf_ucoset_t y)
{
    adf_ucoset_t a, b;
    int eq;

    UC_INV(x);
    UC_INV(y);
    adf_ucoset_init(a);
    adf_ucoset_init(b);
    adf_ucoset_normalise(a, x);
    adf_ucoset_normalise(b, y);
    eq = fmpz_equal(a->c, b->c) && fmpz_equal(a->N, b->N);
    adf_ucoset_clear(a);
    adf_ucoset_clear(b);
    return eq;
}

/* x inside y. ideles.md P9.1 (line 156) for N, N' >= 1 with the normal moduli; api-2.md A.3 for an
   exact unit on either side. */
int
adf_ucoset_contains(const adf_ucoset_t x, const adf_ucoset_t y)
{
    adf_ucoset_t a, b;
    fmpz_t t;
    int in;

    UC_INV(x);
    UC_INV(y);
    adf_ucoset_init(a);
    adf_ucoset_init(b);
    fmpz_init(t);
    adf_ucoset_normalise(a, x);
    adf_ucoset_normalise(b, y);
    if (fmpz_is_zero(b->N))
        in = fmpz_is_zero(a->N) && fmpz_equal(a->c, b->c);
    else
    {
        fmpz_sub(t, a->c, b->c);
        in = fmpz_divisible(t, b->N) && (fmpz_is_zero(a->N) || fmpz_divisible(a->N, b->N));
    }
    fmpz_clear(t);
    adf_ucoset_clear(a);
    adf_ucoset_clear(b);
    return in;
}

/* ideles.md P9.3 (line 159): c = c' modulo gcd(N, N'); api-2.md A.4 with gcd(0, N') = N' and, for two
   exact units, equality. */
int
adf_ucoset_overlaps(const adf_ucoset_t x, const adf_ucoset_t y)
{
    fmpz_t g, t;
    int meet;

    UC_INV(x);
    UC_INV(y);
    fmpz_init(g);
    fmpz_init(t);
    uc_gcd(g, x->N, y->N);
    if (fmpz_is_zero(g))
        meet = fmpz_equal(x->c, y->c);
    else
    {
        fmpz_sub(t, x->c, y->c);
        meet = fmpz_divisible(t, g);
    }
    fmpz_clear(g);
    fmpz_clear(t);
    return meet;
}

/* ---- arithmetic ---- */

/* SPEC 5: c c' mod gcd(N, N'); ideles.md P10.1 (line 199), P11.1 (line 217); api-2.md A.1, C.1. The
   result is stored in normal form (D2-1). */
void
adf_ucoset_mul(adf_ucoset_t z, const adf_ucoset_t x, const adf_ucoset_t y)
{
    fmpz_t tc, tN;

    UC_INV(x);
    UC_INV(y);
    fmpz_init(tc);
    fmpz_init(tN);
    uc_gcd(tN, x->N, y->N);
    fmpz_mul(tc, x->c, y->c);
    if (!fmpz_is_zero(tN))
        uc_normal_inplace(tc, tN);      /* reduces c c' into 1..g and removes the factor 2 */
    /* for g = 0 both are exact and c c' = +-1 (A.1) */
    fmpz_swap(z->c, tc);
    fmpz_swap(z->N, tN);
    fmpz_clear(tc);
    fmpz_clear(tN);
}

/* ideles.md P10.2 (line 200); api-2.md C.2 (the inverse modulo N; for N = 1, fmpz_invmod gives 0,
   fmpz.rst:1159-1160, which the reduction into 1..1 writes as 1) and A.2 (an exact unit is its own
   inverse). The result is stored in normal form (D2-1). */
void
adf_ucoset_inv(adf_ucoset_t y, const adf_ucoset_t x)
{
    fmpz_t tc, tN;

    UC_INV(x);
    if (fmpz_is_zero(x->N))
    {
        adf_ucoset_set(y, x);
        return;
    }
    fmpz_init(tc);
    fmpz_init(tN);
    fmpz_set(tN, x->N);
    if (!fmpz_invmod(tc, x->c, tN))
        flint_abort();                  /* gcd(c, N) = 1 for a canonical x: cannot happen */
    uc_normal_inplace(tc, tN);
    fmpz_swap(y->c, tc);
    fmpz_swap(y->N, tN);
    fmpz_clear(tc);
    fmpz_clear(tN);
}
