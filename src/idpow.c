/* src/idpow.c: integer powers of unit cosets, ideles and idele classes (milestone 2, slice 3, lane i-slice3).

   Contract: include/adelefeld/idpow.h. Sources, read before the code was written:
   - docs/SPEC.md 5 ("Integer powers of a unit coset"; "Sign preservation"); docs/conventions.md 5.6 ("Power",
     CV-49), 5.7, 3.2, 3.3, 4.1, 4.3, 4.4; decisions M0-D1, M0-D6, M1-D4, N-D6;
   - docs/proofs/ideles.md Lemma 12 (line 242), Proposition 13 (line 261; the table of b_p at lines 267-274, P13.2
     at line 279, P13.3 at 282, P13.4 at 284, P13.6 at 288), Proposition 3 (line 67), Proposition 15 (line 366);
   - docs/api-2.md 3.3, Statements J (the two powers of a unit coset), K (the real kernel of a power), L (ideles and
     classes); 1.3, Statements B (normal form), E (kernel B); 2.3, Statement F;
   - FLINT 3.0.1: n_factor, refs/src/flint-3.0.1/ulong_extras.rst:1203-1216, and the fields num, p, exp of
     n_factor_t, ulong_extras.rst:1126-1130; n_is_prime (certified below 2^64), ulong_extras.rst:833-840;
     fmpz_powm_ui, fmpz.rst:923-929; fmpz_invmod, fmpz.rst:1154-1160; fmpz_CRT, fmpz.rst:1292-1302;
     fmpq_pow_si, fmpq.rst:480-485; arf_mul and arf_ui_div with directed rounding, arf.rst:590, and the correct
     rounding of arf.rst:24-39.
   Reference: proto/ideles_checks.py, ref_uc_pow, ref_tight_parts, ref_uc_pow_tight, ref_real_pow, ref_idele_pow,
   ref_idclass_pow (part 4). */

#include <flint/ulong_extras.h>

#include <adelefeld.h>

#include "invariants.h"
#include "idele_internal.h"

#ifdef ADF_CHECK_INVARIANTS
#define PW_INV_UC(x)                                                                                   \
    do { if (!adf_ucoset_is_canonical(x)) adf_inv_fail(__func__, #x, "adf_ucoset"); } while (0)
#define PW_INV_IDELE(x)                                                                                \
    do { if (!adf_idele_is_canonical(x)) adf_inv_fail(__func__, #x, "adf_idele"); } while (0)
#define PW_INV_CLASS(x)                                                                                \
    do { if (!adf_idclass_is_canonical(x)) adf_inv_fail(__func__, #x, "adf_idclass"); } while (0)
#else
#define PW_INV_UC(x) ((void) 0)
#define PW_INV_IDELE(x) ((void) 0)
#define PW_INV_CLASS(x) ((void) 0)
#endif

/* |k| as a ulong; -(ulong) k is defined for every k, WORD_MIN included (2^63). */
static ulong
abs_k(slong k)
{
    return k < 0 ? -(ulong) k : (ulong) k;
}

/* ---- unit cosets (api-2.md Statement J) ---- */

/* The cases k = 0 and x exact of J.1 and J.2: 1 if y was written. */
static int
pow_trivial(adf_ucoset_t y, const adf_ucoset_t x, slong k)
{
    if (k == 0)
    {
        adf_ucoset_one(y);              /* P_0 = {1}: the exact unit 1 (P13.6; M0-D1) */
        return 1;
    }
    if (fmpz_is_zero(x->N))
    {
        if (k % 2 == 0)
            adf_ucoset_one(y);          /* e^k = 1 for even k */
        else
            adf_ucoset_set(y, x);       /* e^k = e for odd k */
        return 1;
    }
    return 0;
}

/* r = c^k modulo m (m >= 2, gcd(c, m) = 1), the power of the inverse for k < 0, in [0, m). */
static void
powm_si(fmpz_t r, const fmpz_t c, slong k, const fmpz_t m)
{
    fmpz_t b;
    fmpz_init(b);
    if (k < 0)
    {
        if (!fmpz_invmod(b, c, m))
            flint_abort();              /* gcd(c, m) = 1 in every call: cannot happen */
    }
    else
        fmpz_mod(b, c, m);
    fmpz_powm_ui(r, b, abs_k(k), m);
    fmpz_clear(b);
}

/* Statement J.1: c'^k U(N') for the normal form (c', N'), residue in 1..N'. */
void
adf_ucoset_pow(adf_ucoset_t y, const adf_ucoset_t x, slong k)
{
    adf_ucoset_t u;

    PW_INV_UC(x);
    if (pow_trivial(y, x, k))
        return;
    adf_ucoset_init(u);
    adf_ucoset_normalise(u, x);
    if (fmpz_is_one(u->N))
        fmpz_one(u->c);                 /* U(1): the residue 1 of 1..1 */
    else
        powm_si(u->c, u->c, k, u->N);   /* a unit modulo N' >= 2 is not 0 modulo N' */
    adf_ucoset_swap(y, u);
    adf_ucoset_clear(u);
}

/* Statement J.2: M_k = A B with A = N' prod_{p | N'} p^(v_p(k)) and B = [2^(2 + v_2(k)) if N' odd, k even] times
   prod p^(1 + v_p(k)) over the odd primes p not dividing N' with (p - 1) | k (the table of P13, ideles.md:267-274,
   read prime by prime); the residue is c'^k modulo A and 1 modulo B. The divisors d of |k| are enumerated from the
   factorisation of |k| by a counter over the exponents, without an array; p = d + 1 <= 2^63 + 1 < 2^64. */
void
adf_ucoset_pow_tight(adf_ucoset_t y, const adf_ucoset_t x, slong k)
{
    adf_ucoset_t u;
    n_factor_t fac;
    int cnt[FLINT_MAX_FACTORS_IN_LIMB];
    ulong n = abs_k(k);
    fmpz_t A, B, t, rA, one;
    int i;

    PW_INV_UC(x);
    if (pow_trivial(y, x, k))
        return;
    adf_ucoset_init(u);
    fmpz_init(A);
    fmpz_init(B);
    fmpz_init(t);
    fmpz_init(rA);
    fmpz_init_set_ui(one, 1);
    adf_ucoset_normalise(u, x);
    n_factor_init(&fac);
    n_factor(&fac, n, 1);

    /* A = N' times p^(v_p(k)) for the primes p of k that divide N' (rows "a_p >= 1" and "a_2 >= 2") */
    fmpz_set(A, u->N);
    for (i = 0; i < fac.num; i++)
        if (fmpz_fdiv_ui(u->N, fac.p[i]) == 0)
        {
            fmpz_set_ui(t, fac.p[i]);
            fmpz_pow_ui(t, t, (ulong) fac.exp[i]);
            fmpz_mul(A, A, t);
        }
    /* B, the primes that do not divide N' */
    fmpz_one(B);
    if (fmpz_is_odd(u->N) && n % 2 == 0)
        for (i = 0; i < fac.num; i++)
            if (fac.p[i] == 2)
                fmpz_mul_2exp(B, B, 2 + (ulong) fac.exp[i]);               /* row "2, a_2 = 0, k even" */
    for (i = 0; i < fac.num; i++)
        cnt[i] = 0;
    for (;;)
    {
        /* the divisor d = prod p_i^cnt[i] */
        ulong d = 1, p;
        int j;
        for (j = 0; j < fac.num; j++)
        {
            int e;
            for (e = 0; e < cnt[j]; e++)
                d *= fac.p[j];
        }
        p = d + 1;
        /* row "p odd, a_p = 0, p - 1 divides k": p > 2 (d even), p prime, p not dividing N' */
        if (d % 2 == 0 && fmpz_fdiv_ui(u->N, p) != 0 && n_is_prime(p))
        {
            ulong e = 1;
            for (j = 0; j < fac.num; j++)
                if (fac.p[j] == p)
                    e += (ulong) fac.exp[j];                               /* 1 + v_p(k) */
            fmpz_set_ui(t, p);
            fmpz_pow_ui(t, t, e);
            fmpz_mul(B, B, t);
        }
        /* the next exponent vector */
        for (j = 0; j < fac.num && cnt[j] == fac.exp[j]; j++)
            cnt[j] = 0;
        if (j == fac.num)
            break;
        cnt[j]++;
    }
    /* the residue: c'^k mod A, 1 mod B (P13.2 with chat = c' mod A, chat = 1 mod B) */
    if (fmpz_is_one(A))
        fmpz_zero(rA);
    else
        powm_si(rA, u->c, k, A);        /* gcd(c', A) = 1: the primes of A are those of N' */
    if (fmpz_is_one(B))
        fmpz_set(u->c, rA);
    else if (fmpz_is_one(A))
        fmpz_one(u->c);
    else
        fmpz_CRT(u->c, rA, A, one, B, 0);
    fmpz_mul(u->N, A, B);
    if (fmpz_is_zero(u->c))
        fmpz_set(u->c, u->N);           /* the residue 1..M_k; 0 occurs only for M_k = 1 */
    adf_ucoset_swap(y, u);
    adf_ucoset_clear(u);
    fmpz_clear(A);
    fmpz_clear(B);
    fmpz_clear(t);
    fmpz_clear(rA);
    fmpz_clear(one);
}

/* ---- the real kernel of a power (api-2.md Statement K) ---- */

/* K.1, K.2: for dyadic 0 < l <= h of at most p bits and k != 0, lo <= t^|k| <= hi for every t in [l, h] by binary
   powering from the lowest bit of |k|, each product rounded down (lo) or up (hi) at p bits; for k < 0 then
   lo = RD_p(1/hi), hi = RU_p(1/lo). lo > 0 and both have at most p bits. */
static void
ends_pow(arf_t lo, arf_t hi, const arf_t l, const arf_t h, slong k, slong p)
{
    arf_t bl, bh, t;
    ulong n = abs_k(k);

    arf_init(bl);
    arf_init(bh);
    arf_init(t);
    arf_set(bl, l);
    arf_set(bh, h);
    arf_one(lo);
    arf_one(hi);
    while (n != 0)
    {
        if (n & 1)
        {
            arf_mul(lo, lo, bl, p, ARF_RND_FLOOR);
            arf_mul(hi, hi, bh, p, ARF_RND_CEIL);
        }
        n >>= 1;
        if (n != 0)
        {
            arf_mul(bl, bl, bl, p, ARF_RND_FLOOR);
            arf_mul(bh, bh, bh, p, ARF_RND_CEIL);
        }
    }
    if (k < 0)
    {
        arf_ui_div(t, 1, hi, p, ARF_RND_FLOOR);
        arf_ui_div(hi, 1, lo, p, ARF_RND_CEIL);
        arf_swap(lo, t);
    }
    arf_clear(bl);
    arf_clear(bh);
    arf_clear(t);
}

/* z = kernel B on the ends of |X|^k with the sign sign(X)^k (K.3); k != 0. */
static int
ball_pow(arb_t z, const arb_t x, slong k, slong p)
{
    arf_t l, h, lo, hi;
    int s, st;

    arf_init(l);
    arf_init(h);
    arf_init(lo);
    arf_init(hi);
    s = adf_idele_ends_of_abs(l, h, x, p);          /* E1: the sign of every point of x */
    ends_pow(lo, hi, l, h, k, p);
    st = adf_idele_ball_from_ends(z, lo, hi, k % 2 != 0 ? s : 1, p);
    arf_clear(l);
    arf_clear(h);
    arf_clear(lo);
    arf_clear(hi);
    return st;
}

/* ---- ideles and classes (api-2.md Statement L) ---- */

/* L.4: r != 1 and |k| (bits(n) + bits(d)) > ADF_IDELE_POW_BITS_MAX, without overflow: for integers,
   n b > M exactly when n > floor(M / b). */
static int
pow_bits_exceeded(const fmpq_t r, slong k)
{
    ulong b;
    if (fmpq_is_one(r))
        return 0;
    b = (ulong) fmpz_bits(fmpq_numref(r)) + (ulong) fmpz_bits(fmpq_denref(r));
    if (b == 0)
        return 0;                       /* 0/0 is no canonical content: the debug build aborts on entry */
    return abs_k(k) > (ulong) ADF_IDELE_POW_BITS_MAX / b;
}

static int
idele_pow(adf_idele_t z, const adf_idele_t x, slong k, slong prec, int tight)
{
    slong p = prec < 2 ? 2 : prec;
    arb_t t;
    int st;

    if (k == 0)
    {
        arb_one(z->inf);                /* L.2: the exact idele 1 */
        fmpq_one(z->r);
        adf_ucoset_one(&z->u);
        return ADF_OK;
    }
    arb_init(t);
    st = ball_pow(t, x->inf, k, p);
    if (st == ADF_OK)
    {
        fmpq_t r;
        adf_ucoset_t u;
        fmpq_init(r);
        adf_ucoset_init(u);
        if (fmpq_is_one(x->r))
            fmpq_one(r);                /* no call with k = WORD_MIN: the limit excludes it for r != 1 */
        else
            fmpq_pow_si(r, x->r, k);
        if (tight)
            adf_ucoset_pow_tight(u, &x->u, k);
        else
            adf_ucoset_pow(u, &x->u, k);
        arb_swap(z->inf, t);
        fmpq_swap(z->r, r);
        adf_ucoset_swap(&z->u, u);
        fmpq_clear(r);
        adf_ucoset_clear(u);
    }
    arb_clear(t);
    return st;
}

int
adf_idele_pow(adf_idele_t z, const adf_idele_t x, slong k, slong prec)
{
    if (prec > ADF_IDELE_PREC_MAX || pow_bits_exceeded(x->r, k))
        return ADF_LIMIT;               /* first: before the entry check of the debug build (N-D8, F1) */
    PW_INV_IDELE(x);                    /* in the public function, so that the message names it */
    return idele_pow(z, x, k, prec, 0);
}

int
adf_idele_pow_tight(adf_idele_t z, const adf_idele_t x, slong k, slong prec)
{
    if (prec > ADF_IDELE_PREC_MAX || pow_bits_exceeded(x->r, k))
        return ADF_LIMIT;               /* first: before the entry check of the debug build (N-D8, F1) */
    PW_INV_IDELE(x);                    /* in the public function, so that the message names it */
    return idele_pow(z, x, k, prec, 1);
}

static int
idclass_pow(adf_idclass_t z, const adf_idclass_t x, slong k, slong prec, int tight)
{
    slong p = prec < 2 ? 2 : prec;
    arb_t t;
    int st;

    if (k == 0)
    {
        arb_one(z->t);                  /* <1 ; [1]> */
        adf_ucoset_one(&z->u);
        return ADF_OK;
    }
    arb_init(t);
    st = ball_pow(t, x->t, k, p);       /* t > 0, so the sign is +1 */
    if (st == ADF_OK)
    {
        adf_ucoset_t u;
        adf_ucoset_init(u);
        if (tight)
            adf_ucoset_pow_tight(u, &x->u, k);
        else
            adf_ucoset_pow(u, &x->u, k);
        arb_swap(z->t, t);
        adf_ucoset_swap(&z->u, u);
        adf_ucoset_clear(u);
    }
    arb_clear(t);
    return st;
}

int
adf_idclass_pow(adf_idclass_t z, const adf_idclass_t x, slong k, slong prec)
{
    if (prec > ADF_IDELE_PREC_MAX)
        return ADF_LIMIT;               /* first: before the entry check of the debug build (N-D8, F1) */
    PW_INV_CLASS(x);
    return idclass_pow(z, x, k, prec, 0);
}

int
adf_idclass_pow_tight(adf_idclass_t z, const adf_idclass_t x, slong k, slong prec)
{
    if (prec > ADF_IDELE_PREC_MAX)
        return ADF_LIMIT;               /* first: before the entry check of the debug build (N-D8, F1) */
    PW_INV_CLASS(x);
    return idclass_pow(z, x, k, prec, 1);
}
