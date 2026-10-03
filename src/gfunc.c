/* gfunc.c: functions at all places (milestone 1F, WP 1F.8; lane f-slice10; include/adelefeld/gfunc.h).

   Contract: docs/SPEC.md 9.3.1, 9.3.3 lines 636-646, 15.4 N-D8; docs/conventions.md 3.1, 3.3 lines 234-247 (the
   maximum; the first place in canonical order, the real place first); docs/proofs/functions.md Proposition 14
   (line 446), Proposition 16 (line 538), Proposition 22 (line 725); docs/api-1f8.md G1 to G4.

   FLINT used (refs/src/flint-3.0.1/): fmpz_root (fmpz.rst:983-988: "Set r to the integer part of the n-th root of
   f. Requires that n > 0 and that if n is even then f be non-negative ... returns 1 if the root was exact"),
   fmpz_bits, fmpq_sgn (fmpq.rst:149). The real coordinate is adf_real_root (include/adelefeld/rfunc.h:79), which
   decides its own domain (Proposition 14) and never stores a non-finite ball.

   Every function computes into temporaries and writes y only on ADF_OK, so y may be x and a status leaves y
   untouched.

   The debug build -DADF_CHECK_INVARIANTS checks each argument that is read on entry and calls flint_abort with
   one line on stderr (docs/conventions.md 4.4, DECISION CV-09, line 288; src/invariants.h:53-57). The macros of
   src/invariants.h exist for adf_rat and adf_adele; adf_idele has none, so ADF_INV_IDELE is defined here in the
   pattern of ADF_INV_LBALL of src/lball.c:33-46. ADF_LIMIT from prec alone is decided before the entry checks
   (N-D8; as src/idele.c:258, "from prec alone, before the entry checks"). Without the flag every macro is empty
   and the object code is the one of before. */

#include <adelefeld.h>
#include <flint/ulong_extras.h>
#include "invariants.h"

#ifdef ADF_CHECK_INVARIANTS
#include <stdio.h>
#include <flint/flint.h>
#define ADF_INV_IDELE(x)                                                                                    \
    do                                                                                                      \
    {                                                                                                       \
        if (!adf_idele_is_canonical(x))                                                                     \
        {                                                                                                   \
            fprintf(stderr, "adelefeld: ADF_CHECK_INVARIANTS: %s: argument %s is not a canonical adf_idele\n", \
                    __func__, #x);                                                                          \
            fflush(stderr);                                                                                 \
            flint_abort();                                                                                  \
        }                                                                                                   \
    }                                                                                                       \
    while (0)
#else
#define ADF_INV_IDELE(x) ((void) 0)
#endif

/* The entry check of x, after the ADF_LIMIT of a prec above the cap: series() decides that status before it
   reads anything (N-D8; as src/idele.c:258), so the check stands only for a prec at or below the cap. Without
   -DADF_CHECK_INVARIANTS both halves are empty and no code is generated. */
#define ADF_INV_ADELE_LIM(x, prec)                                                                         \
    do                                                                                                     \
    {                                                                                                      \
        if ((prec) <= ADF_REAL_PREC_MAX)                                                                   \
            ADF_INV_ADELE(x);                                                                              \
    }                                                                                                      \
    while (0)

/* r = the exact n-th root of the integer a >= 0 and 1, or 0 if a is not an n-th power; n >= 2.
   docs/api-1f5.md R3 step 4 (lines 124-126): an integer q >= 2 with an integer root >= 2 has q >= 2^n, so bits(q)
   >= n + 1; hence n >= bits(q) rules out exactness, and 0 and 1 are their own roots for every degree. The same test
   as the static integer_root of src/lroot.c:99-106 (not callable from here). Only n < bits(a) <= WORD_MAX reaches
   fmpz_root, whose degree is an slong (fmpz.rst:983). */
static int
int_root(fmpz_t r, const fmpz_t a, ulong n)
{
    if (fmpz_cmp_ui(a, 1) <= 0)
    {
        fmpz_set(r, a);
        return 1;
    }
    if (n >= (ulong) fmpz_bits(a))
        return 0;
    return fmpz_root(r, a, (slong) n);
}

/* r = the rational n-th root of q on the branch sign (sign = +1 or -1; n >= 2; for odd n the root has the sign of q
   and `sign` is not read). Returns ADF_OK or ADF_DOMAIN; *real = 1 when the cause is the sign at the real place (n
   even, q < 0; Proposition 14, functions.md:446-449). Proposition 16 step 4 (functions.md:565-568): "testing
   numerator and denominator separately for exact integer n-th powers gives the same criterion and needs no prime
   factorisation"; docs/api-1f8.md G1, G2. q is canonical, so |num| and den are coprime, and so are their roots: the
   result is canonical. */
static int
rat_root_core(fmpq_t r, int * real, const fmpq_t q, ulong n, int sign)
{
    fmpz_t a, rn, rd;
    int s = fmpq_sgn(q), st = ADF_OK;

    *real = 0;
    if (s == 0)
    {
        fmpq_zero(r);
        return ADF_OK;
    }
    if (n % 2 == 0 && s < 0)
    {
        *real = 1;
        return ADF_DOMAIN;
    }
    fmpz_init(a);
    fmpz_init(rn);
    fmpz_init(rd);
    fmpz_abs(a, fmpq_numref(q));
    if (!int_root(rn, a, n) || !int_root(rd, fmpq_denref(q), n))
        st = ADF_DOMAIN;
    else
    {
        if ((n % 2 == 1 && s < 0) || (n % 2 == 0 && sign < 0))
            fmpz_neg(rn, rn);
        fmpz_swap(fmpq_numref(r), rn);
        fmpz_swap(fmpq_denref(r), rd);
    }
    fmpz_clear(a);
    fmpz_clear(rn);
    fmpz_clear(rd);
    return st;
}

/* 1 if sign is a valid selector for a degree n >= 2 on a value that is not the exact 0 (gfunc.h, "THE BRANCH"). */
static int
selects(ulong n, int sign)
{
    return sign == 1 || (sign == -1 && n % 2 == 0);
}

/* The real coordinate of a root on the branch sign: J = adf_real_root(I, n, prec) (rfunc.h:79), negated for even n
   and sign = -1 (Proposition 14: the two real roots of a positive number are opposite). */
static int
real_branch(arb_t J, const arb_t I, ulong n, int sign, slong prec)
{
    int st = adf_real_root(J, I, n, prec);
    if (st == ADF_OK && n % 2 == 0 && sign < 0)
        arb_neg(J, J);
    return st;
}

/* The combination of conventions 3.3 over the real place (first in the canonical order) and the finite part, whose
   failures name no prime: the maximum; `where` = the real place if the real status is the maximum. */
static int
combine(adf_place_t * where, int st_real, int st_fin)
{
    int st = st_real > st_fin ? st_real : st_fin;
    if (st != ADF_OK && st == st_real && where != NULL)
        *where = adf_place_inf();
    return st;
}

/* G1, G2 (docs/api-1f8.md); Proposition 16, functions.md:538; gfunc.h. */
int
adf_rat_root(adf_rat_t y, adf_place_t * where, const adf_rat_t a, ulong n, int sign)
{
    fmpq_t r;
    int st, real;

    ADF_INV_RAT(a);
    if (n == 0)
        return ADF_DOMAIN;
    if (n == 1)
    {
        adf_rat_set(y, a);
        return ADF_OK;
    }
    if (sign != 1 && sign != -1)
        return ADF_DOMAIN;
    if (adf_rat_is_zero(a))
    {
        adf_rat_zero(y);       /* Proposition 16: "For a=0 the sole adelic root is zero" (functions.md:543) */
        return ADF_OK;
    }
    if (!selects(n, sign))
        return ADF_DOMAIN;
    fmpq_init(r);
    st = rat_root_core(r, &real, a->q, n, sign);
    if (st == ADF_OK)
        fmpq_swap(y->q, r);
    else if (real && where != NULL)
        *where = adf_place_inf();
    fmpq_clear(r);
    return st;
}

/* G3 (docs/api-1f8.md): the coordinates independently (adele.h:12), on one branch; Proposition 22,
   functions.md:739: every coordinate's domain condition; a finite part that is not exact is NOT_DETERMINED. */
int
adf_adele_root(adf_adele_t y, adf_place_t * where, const adf_adele_t x, ulong n, int sign, slong prec)
{
    adf_adele_t t;
    adf_rat_t q;
    fmpq_t r;
    int st_real, st_fin = ADF_NOT_DETERMINED, real, st;
    int exact = adf_fball_is_exact(&x->fin);

    if (prec > ADF_REAL_PREC_MAX)   /* N-D8: from prec alone, first */
    {
        if (where != NULL)
            *where = adf_place_inf();
        return ADF_LIMIT;
    }
    ADF_INV_ADELE(x);
    if (n == 0)
        return ADF_DOMAIN;
    if (n == 1)
    {
        adf_adele_set(y, x);
        return ADF_OK;
    }
    if (sign != 1 && sign != -1)
        return ADF_DOMAIN;
    if (!selects(n, sign))
    {
        /* odd n, sign -1: only the exact zero adele (0 ; 0) has a root on it, (0 ; 0); the branch +1 gives it */
        if (!(arb_is_zero(x->inf) && exact && fmpz_is_zero(x->fin.A)))
            return ADF_DOMAIN;
        sign = 1;
    }
    adf_adele_init(t);
    adf_rat_init(q);
    fmpq_init(r);
    st_real = real_branch(t->inf, x->inf, n, sign, prec);
    if (exact)
    {
        adf_fball_get_center(q, &x->fin);
        st_fin = rat_root_core(r, &real, q->q, n, sign);   /* the finite part names no prime: real is not used */
    }
    st = combine(where, st_real, st_fin);
    if (st == ADF_OK)
    {
        fmpq_swap(q->q, r);
        adf_fball_set_rat(&t->fin, q);
        adf_adele_swap(y, t);
    }
    adf_adele_clear(t);
    adf_rat_clear(q);
    fmpq_clear(r);
    return st;
}

/* G4 (docs/api-1f8.md): an inexact unit is NOT_DETERMINED (Proposition 16 steps 1-2, functions.md:549-557); an
   exact unit [c] makes the finite coordinate the rational c r (idele.h:135), which follows the rational
   contract. */
int
adf_idele_root(adf_idele_t y, adf_place_t * where, const adf_idele_t x, ulong n, int sign, slong prec)
{
    adf_idele_t t;
    fmpq_t q, r;
    int st_real, st_fin = ADF_NOT_DETERMINED, real, st;

    if (prec > ADF_IDELE_PREC_MAX)   /* N-D8 and idele.h: from prec alone, first */
    {
        if (where != NULL)
            *where = adf_place_inf();
        return ADF_LIMIT;
    }
    ADF_INV_IDELE(x);
    if (n == 0)
        return ADF_DOMAIN;
    if (n == 1)
    {
        adf_idele_set(y, x);
        return ADF_OK;
    }
    if (sign != 1 && sign != -1)
        return ADF_DOMAIN;
    if (!selects(n, sign))           /* odd n, sign -1: an idele is never 0 */
        return ADF_DOMAIN;
    adf_idele_init(t);
    fmpq_init(q);
    fmpq_init(r);
    st_real = real_branch(t->inf, x->inf, n, sign, prec);
    if (st_real == ADF_OK && !arb_is_nonzero(t->inf))
        st_real = ADF_NOT_DETERMINED;   /* the idele predicate: the real ball excludes 0 (idele.h) */
    if (adf_ucoset_is_exact(&x->u))
    {
        fmpq_set(q, x->r);
        if (fmpz_sgn(x->u.c) < 0)
            fmpq_neg(q, q);
        st_fin = rat_root_core(r, &real, q, n, sign);
    }
    st = combine(where, st_real, st_fin);
    if (st == ADF_OK)
    {
        fmpq_abs(t->r, r);
        if (fmpq_sgn(r) < 0)
            adf_ucoset_minus_one(&t->u);
        else
            adf_ucoset_one(&t->u);
        adf_idele_swap(y, t);
    }
    adf_idele_clear(t);
    fmpq_clear(q);
    fmpq_clear(r);
    return st;
}

/* ---- the five factorial series at all places (SPEC 9.3.2 lines 588-596; G5, G6 of docs/api-1f8.md) ---- */

typedef int (*series_at_fn)(adf_sball_t, adf_place_t *, const adf_sball_t, adf_place_t, slong);

/* The first prime p, in increasing order, with v_p(q) < c_p for the exact rational q != 0 (c_2 = 2, c_p = 1 at
   odd p): the domain of the five series at p is p^c Z_p (Proposition 6, functions.md:140). With q = A/B in lowest
   terms: v_2(q) >= 2 exactly when 4 divides A (if 2 divides B, A is odd); at odd p, v_p(q) >= 1 exactly when p
   divides A. So the answer is 2 unless 4 | A, and otherwise the first odd prime that does not divide A. G6 (c): if
   the first k odd primes divide A != 0, their product, at least 3^k, divides |A|, so at most floor(log_3 |A|) + 1
   odd primes are tried. G6 (d): the candidate fits a ulong while the numerator has at most 365651249660515264 bits
   (docs/api-1f8.md, proof step 4); above that the loop has no guard and n_nextprime's assumption
   (refs/src/flint-3.0.1/ulong_extras.rst:688-692) would fail. n_nextprime ("Returns the next prime after n");
   fmpz_fdiv_ui (fmpz.rst:845-850). */
static ulong
first_failing_prime(const fmpz_t A)
{
    ulong p;
    if (fmpz_fdiv_ui(A, 4) != 0)
        return 2;
    for (p = 3; fmpz_fdiv_ui(A, p) == 0; p = n_nextprime(p, 1))
        ;
    return p;
}

/* G5, G6: y = (f at the real place of the real ball ; the exact constant f(0)) when the finite part is exactly 0.
   The real coordinate by the function at the real place of rfunc.h (adf_sball_exp_at ...: arb_exp, arb_sin,
   arb_sinh, arb_cos, arb_cosh), the finite part by Proposition 12 step 1 (functions.md:394). The statuses combine
   by conventions 3.3: the real place first, then the finite part, whose DOMAIN names its first failing prime.

   The caller has already entered with the entry check done for a prec at or below ADF_REAL_PREC_MAX; the status
   of a prec above the cap is decided here, before anything is read (N-D8). */
static int
series(adf_adele_t y, adf_place_t * where, const adf_adele_t x, slong prec, series_at_fn at, slong constant)
{
    adf_sball_t s, t;
    adf_adele_t r;
    adf_place_t inf = adf_place_inf(), wp;
    int st_real, st_fin = ADF_NOT_DETERMINED, st;
    ulong p = 0;

    if (prec > ADF_REAL_PREC_MAX)   /* N-D8: from prec alone, first */
    {
        if (where != NULL)
            *where = inf;
        return ADF_LIMIT;
    }
    adf_sball_init(s);
    adf_sball_init(t);
    adf_adele_init(r);
    st_real = adf_sball_set_arb_lballs(s, NULL, x->inf, NULL, 0);   /* the real ball of a canonical adele: OK */
    if (st_real == ADF_OK)
        st_real = at(t, NULL, s, inf, prec);
    if (st_real == ADF_OK)
        st_real = adf_sball_get_arb(r->inf, t, inf);
    if (adf_fball_is_exact(&x->fin))
    {
        if (fmpz_is_zero(x->fin.A))
            st_fin = ADF_OK;
        else
        {
            st_fin = ADF_DOMAIN;
            p = first_failing_prime(x->fin.A);
        }
    }
    st = st_real > st_fin ? st_real : st_fin;
    if (st != ADF_OK && where != NULL)
    {
        if (st == st_real)
            *where = inf;
        else if (st == ADF_DOMAIN && adf_place_prime(&wp, p) == ADF_OK)
            *where = wp;
    }
    if (st == ADF_OK)
    {
        adf_fball_set_si(&r->fin, constant);
        adf_adele_swap(y, r);
    }
    adf_sball_clear(s);
    adf_sball_clear(t);
    adf_adele_clear(r);
    return st;
}

int
adf_adele_exp(adf_adele_t y, adf_place_t * where, const adf_adele_t x, slong prec)
{
    ADF_INV_ADELE_LIM(x, prec);
    return series(y, where, x, prec, adf_sball_exp_at, 1);
}

int
adf_adele_sin(adf_adele_t y, adf_place_t * where, const adf_adele_t x, slong prec)
{
    ADF_INV_ADELE_LIM(x, prec);
    return series(y, where, x, prec, adf_sball_sin_at, 0);
}

int
adf_adele_sinh(adf_adele_t y, adf_place_t * where, const adf_adele_t x, slong prec)
{
    ADF_INV_ADELE_LIM(x, prec);
    return series(y, where, x, prec, adf_sball_sinh_at, 0);
}

int
adf_adele_cos(adf_adele_t y, adf_place_t * where, const adf_adele_t x, slong prec)
{
    ADF_INV_ADELE_LIM(x, prec);
    return series(y, where, x, prec, adf_sball_cos_at, 1);
}

int
adf_adele_cosh(adf_adele_t y, adf_place_t * where, const adf_adele_t x, slong prec)
{
    ADF_INV_ADELE_LIM(x, prec);
    return series(y, where, x, prec, adf_sball_cosh_at, 1);
}
