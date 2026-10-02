/* rfunc.c: the real functions at the archimedean place, thin wrappers around arb (milestone 1F.2, first slice;
   include/adelefeld/rfunc.h).

   Contract: docs/SPEC.md 9.3.1, 9.3.3 (real roots), 9.3.6; docs/conventions.md 3.1 (DOMAIN only when every point of
   the input is outside the domain; NOT_DETERMINED when the ball meets the domain and its complement), 4.3, 4.4 (a
   non-finite ball produced from finite inputs is never stored: NOT_DETERMINED, CV-08); docs/proofs/functions.md
   Proposition 14 (line 446: real roots; the root is increasing for odd degree and on [0, infinity) for even degree),
   Proposition 22 (line 725); the statements S5 to S7 of docs/api-1f.md.

   arb functions used, from refs/src/flint-3.0.1/arb.rst: arb_exp (line 1082), arb_log (1050), arb_sin and arb_cos
   (1101, 1103), arb_sqrt (945), arb_sqrt_arf (947), arb_root_ui (979), arb_abs (735), arb_union (405),
   arb_get_interval_arf (492: a ball containing [a, b] is not required, the interval [a, b] contains the ball, rounded
   outwards at prec bits), arb_is_positive, arb_is_nonnegative, arb_is_negative, arb_is_nonpositive (639 to 649: "all
   points p satisfy p > 0, ..."), arb_contains_zero (682), arb_is_zero (593), arb_is_finite (606), arb_set_round (155).
   The odd root is computed with the sign removed and, for a ball that contains 0, by the monotonicity of the root:
   the image of [a, b] is inside [root(a), root(b)] because t -> t^(1/n) is increasing on R for odd n and on [0, inf)
   for even n (Proposition 14, step 1: x^n is strictly increasing on the non-negative half line). arb_root_ui is only
   called on a strictly positive ball, where it is defined (arb.rst:979: "if input interval is [m-r, m+r] with
   r <= m").

   Every function computes into a temporary and copies to the output only on ADF_OK, so y may be x and a status leaves
   y untouched. */

#include <adelefeld.h>
#include <flint/arf.h>

#ifdef ADF_CHECK_INVARIANTS
#include <stdio.h>
#include <flint/flint.h>
#define ADF_INV_SBALL(x)                                                                                    \
    do                                                                                                      \
    {                                                                                                       \
        if (!adf_sball_is_canonical(x))                                                                     \
        {                                                                                                   \
            fprintf(stderr, "adelefeld: ADF_CHECK_INVARIANTS: %s: argument %s is not a canonical adf_sball\n", \
                    __func__, #x);                                                                          \
            fflush(stderr);                                                                                 \
            flint_abort();                                                                                  \
        }                                                                                                   \
    }                                                                                                       \
    while (0)
#else
#define ADF_INV_SBALL(x) ((void) 0)
#endif

typedef enum
{
    F_EXP,
    F_LOG,
    F_LOG_ABS,
    F_LOG_IW,   /* the Iwasawa logarithm Log: at the real place log (domain t > 0), at a prime adf_lball_Log */
    F_SIN,
    F_COS,
    F_SINH,
    F_COSH,
    F_ROOT   /* degree n >= 1; the square root is F_ROOT with n = 2 */
} rfn;

/* t = the n-th root of the arf a >= 0 (n >= 1), an enclosure. */
static void
rootpos_arf(arb_t t, const arf_t a, ulong n, slong prec)
{
    if (arf_is_zero(a))
    {
        arb_zero(t);
        return;
    }
    if (n == 1)
    {
        arb_set_arf(t, a);
        arb_set_round(t, t, prec);
    }
    else if (n == 2)
        arb_sqrt_arf(t, a, prec);
    else
    {
        arb_set_arf(t, a);
        arb_root_ui(t, t, n, prec);
    }
}

/* t = the n-th root of x, the domain of x having been checked: x is a ball for odd n, a non-negative ball for even n. */
static void
root_of_checked(arb_t t, const arb_t x, ulong n, slong prec)
{
    int even = n % 2 == 0;
    if (n == 1)
    {
        arb_set_round(t, x, prec);
        return;
    }
    if (arb_is_positive(x))
    {
        if (n == 2)
            arb_sqrt(t, x, prec);
        else
            arb_root_ui(t, x, n, prec);
        return;
    }
    if (!even && arb_is_negative(x))
    {
        /* the odd root is odd: root(x) = -root(-x); arb_root_ui(x) is NaN for x < 0 */
        arb_t u;
        arb_init(u);
        arb_neg(u, x);
        arb_root_ui(u, u, n, prec);
        arb_neg(t, u);
        arb_clear(u);
        return;
    }
    if (arb_is_zero(x))
    {
        arb_zero(t);   /* arb_root_ui(0, n) is NaN (probe of lane d-functions); the root of 0 is 0 */
        return;
    }
    {
        /* x contains 0 and is not the exact 0: odd n, or even n with lower end exactly 0. The image is inside
           [root(a), root(b)] for the outer end points a <= 0 <= b of x. */
        arf_t a, b;
        arb_t hi, lo;
        arf_init(a);
        arf_init(b);
        arb_init(hi);
        arb_init(lo);
        arb_get_interval_arf(a, b, x, prec);
        rootpos_arf(hi, b, n, prec);
        if (even)
            arb_zero(lo);
        else
        {
            arf_neg(a, a);
            rootpos_arf(lo, a, n, prec);
            arb_neg(lo, lo);
        }
        arb_union(t, lo, hi, prec);
        arf_clear(a);
        arf_clear(b);
        arb_clear(hi);
        arb_clear(lo);
    }
}

static int
real_apply(arb_t y, const arb_t x, rfn f, ulong n, slong prec)
{
    arb_t t;
    int st = ADF_OK;
    if (prec > ADF_REAL_PREC_MAX)   /* from prec alone, before every other status and any allocation (finding R5) */
        return ADF_LIMIT;
    if (prec < 2)
        prec = 2;
    if (!arb_is_finite(x))
        return ADF_DOMAIN;
    if (f == F_ROOT && n == 0)
        return ADF_DOMAIN;
    if (f == F_LOG && arb_is_nonpositive(x))
        return ADF_DOMAIN;
    if (f == F_LOG && !arb_is_positive(x))
        return ADF_NOT_DETERMINED;
    if (f == F_LOG_ABS && arb_is_zero(x))
        return ADF_DOMAIN;
    if (f == F_LOG_ABS && arb_contains_zero(x))
        return ADF_NOT_DETERMINED;
    if (f == F_ROOT && n % 2 == 0 && arb_is_negative(x))
        return ADF_DOMAIN;
    if (f == F_ROOT && n % 2 == 0 && !arb_is_nonnegative(x))
        return ADF_NOT_DETERMINED;

    arb_init(t);
    switch (f)
    {
        case F_EXP:
            arb_exp(t, x, prec);
            break;
        case F_LOG:
            arb_log(t, x, prec);
            break;
        case F_LOG_IW:   /* never passed: at_place maps the Iwasawa logarithm at the real place to F_LOG */
            break;
        case F_LOG_ABS:
            arb_abs(t, x);
            arb_log(t, t, prec);
            break;
        case F_SIN:
            arb_sin(t, x, prec);
            break;
        case F_COS:
            arb_cos(t, x, prec);
            break;
        case F_SINH:
            /* refs/src/flint-3.0.1/arb.rst:1209-1219; F14, docs/api-1f4.md. */
            arb_sinh(t, x, prec);
            break;
        case F_COSH:
            /* refs/src/flint-3.0.1/arb.rst:1211-1219; F14, docs/api-1f4.md. */
            arb_cosh(t, x, prec);
            break;
        case F_ROOT:
            root_of_checked(t, x, n, prec);
            break;
    }
    if (!arb_is_finite(t))   /* conventions 4.4, CV-08: never stored */
        st = ADF_NOT_DETERMINED;
    else
        arb_swap(y, t);
    arb_clear(t);
    return st;
}

int
adf_real_exp(arb_t y, const arb_t x, slong prec)
{
    return real_apply(y, x, F_EXP, 0, prec);
}

int
adf_real_log(arb_t y, const arb_t x, slong prec)
{
    return real_apply(y, x, F_LOG, 0, prec);
}

int
adf_real_log_abs(arb_t y, const arb_t x, slong prec)
{
    return real_apply(y, x, F_LOG_ABS, 0, prec);
}

int
adf_real_sin(arb_t y, const arb_t x, slong prec)
{
    return real_apply(y, x, F_SIN, 0, prec);
}

int
adf_real_cos(arb_t y, const arb_t x, slong prec)
{
    return real_apply(y, x, F_COS, 0, prec);
}

int
adf_real_sqrt(arb_t y, const arb_t x, slong prec)
{
    return real_apply(y, x, F_ROOT, 2, prec);
}

int
adf_real_root(arb_t y, const arb_t x, ulong n, slong prec)
{
    return real_apply(y, x, F_ROOT, n, prec);
}

/* ---- at a place of a partial ball (SPEC 9.3.1: f_at with one place; Proposition 22) ---- */

/* At a PRIME (lane f-slice6): exp, log and Log go through adf_lball_exp, adf_lball_log and adf_lball_Log
   (include/adelefeld/lfunc.h) on the component of x at v, with N = prec, the absolute precision (docs/api-1f.md,
   section "Slice 1F.4-b"). The result is the partial ball over v: arch NONE, inf = 0, len 1, loc[0] = the result.
   Statuses of lfunc.h are passed on with where = v. The component is copied first, so y may be x. Every other
   function except the four parity series is UNSUPPORTED with where = v (a later slice).
   F14 (docs/api-1f4.md) extends this component rule to sin, cos, sinh, cosh. */
static int
at_prime(adf_sball_t y, adf_place_t * where, const adf_sball_t x, adf_place_t v, rfn f, slong N)
{
    adf_lball_t c, r;
    adf_sball_t t;
    int st;

    if (f != F_EXP && f != F_LOG && f != F_LOG_IW &&
        f != F_SIN && f != F_COS && f != F_SINH && f != F_COSH)
    {
        if (where != NULL)
            *where = v;
        return ADF_UNSUPPORTED;
    }
    adf_lball_init(c);
    adf_lball_init(r);
    st = adf_sball_get_lball(c, x, v);   /* v is a prime of x: OK */
    if (st == ADF_OK)
    {
        if (f == F_EXP)
            st = adf_lball_exp(r, c, N);
        else if (f == F_LOG)
            st = adf_lball_log(r, c, N);
        else if (f == F_LOG_IW)
            st = adf_lball_Log(r, c, N);
        else if (f == F_SIN)
            st = adf_lball_sin(r, c, N);
        else if (f == F_COS)
            st = adf_lball_cos(r, c, N);
        else if (f == F_SINH)
            st = adf_lball_sinh(r, c, N);
        else
            st = adf_lball_cosh(r, c, N);
    }
    if (st == ADF_OK)
    {
        adf_sball_init(t);
        st = adf_sball_set_arb_lballs(t, NULL, NULL, r, 1);   /* r is canonical: OK */
        if (st == ADF_OK)
            adf_sball_swap(y, t);
        adf_sball_clear(t);
    }
    if (st != ADF_OK && where != NULL)
        *where = v;
    adf_lball_clear(c);
    adf_lball_clear(r);
    return st;
}

static int
at_place(adf_sball_t y, adf_place_t * where, const adf_sball_t x, adf_place_t v, rfn f, ulong n, slong prec)
{
    arb_t t;
    int st;
    /* prec above the limit: LIMIT from prec alone, before every other status, before the entry check of x and any
       allocation (finding R5), where = the archimedean place. Only where prec is a number of bits: at a prime it is
       the absolute precision N of lfunc.h (f-slice6). */
    if (adf_place_is_archimedean(v) && prec > ADF_REAL_PREC_MAX)
    {
        if (where != NULL)
            *where = adf_place_inf();
        return ADF_LIMIT;
    }
    ADF_INV_SBALL(x);
    if (!adf_sball_has_place(x, v))
    {
        if (where != NULL)
            *where = v;
        return ADF_DOMAIN;
    }
    if (!adf_place_is_archimedean(v))
        return at_prime(y, where, x, v, f, prec);
    if (x->arch == ADF_ARCH_COMPLEX)
    {
        if (where != NULL)
            *where = v;
        return ADF_UNSUPPORTED;
    }
    if (f == F_ROOT && n == 0)
        return ADF_DOMAIN;
    if (f == F_LOG_IW)
        f = F_LOG;   /* the real place: Log = log on t > 0 (SPEC 9.3.2: "the real coordinate needs a positive input") */
    arb_init(t);
    st = real_apply(t, acb_realref(x->inf), f, n, prec);
    if (st != ADF_OK)
    {
        if (where != NULL)
            *where = v;
        arb_clear(t);
        return st;
    }
    adf_sball_clear(y);
    adf_sball_init(y);
    y->arch = ADF_ARCH_REAL;
    arb_swap(acb_realref(y->inf), t);
    arb_clear(t);
    return ADF_OK;
}

int
adf_sball_exp_at(adf_sball_t y, adf_place_t * where, const adf_sball_t x, adf_place_t v, slong prec)
{
    return at_place(y, where, x, v, F_EXP, 0, prec);
}

int
adf_sball_log_at(adf_sball_t y, adf_place_t * where, const adf_sball_t x, adf_place_t v, slong prec)
{
    return at_place(y, where, x, v, F_LOG, 0, prec);
}

int
adf_sball_Log_at(adf_sball_t y, adf_place_t * where, const adf_sball_t x, adf_place_t v, slong prec)
{
    return at_place(y, where, x, v, F_LOG_IW, 0, prec);
}

int
adf_sball_log_abs_at(adf_sball_t y, adf_place_t * where, const adf_sball_t x, adf_place_t v, slong prec)
{
    return at_place(y, where, x, v, F_LOG_ABS, 0, prec);
}

int
adf_sball_sin_at(adf_sball_t y, adf_place_t * where, const adf_sball_t x, adf_place_t v, slong prec)
{
    return at_place(y, where, x, v, F_SIN, 0, prec);
}

int
adf_sball_cos_at(adf_sball_t y, adf_place_t * where, const adf_sball_t x, adf_place_t v, slong prec)
{
    return at_place(y, where, x, v, F_COS, 0, prec);
}

/* F14; docs/proofs/functions.md:24,26,140,299,725: component series and image enclosure.
   At real: refs/src/flint-3.0.1/arb.rst:1209-1219; real_apply enforces CV-08 on lost finiteness. */
int
adf_sball_sinh_at(adf_sball_t y, adf_place_t * where, const adf_sball_t x, adf_place_t v, slong prec)
{
    return at_place(y, where, x, v, F_SINH, 0, prec);
}

/* F14; same component, aliasing and status proof as sinh_at. */
int
adf_sball_cosh_at(adf_sball_t y, adf_place_t * where, const adf_sball_t x, adf_place_t v, slong prec)
{
    return at_place(y, where, x, v, F_COSH, 0, prec);
}

int
adf_sball_sqrt_at(adf_sball_t y, adf_place_t * where, const adf_sball_t x, adf_place_t v, slong prec)
{
    return at_place(y, where, x, v, F_ROOT, 2, prec);
}

int
adf_sball_root_at(adf_sball_t y, adf_place_t * where, const adf_sball_t x, adf_place_t v, ulong n, slong prec)
{
    return at_place(y, where, x, v, F_ROOT, n, prec);
}

/* R6, docs/api-1f5.md: projection then Proposition 13/15 on that entire component.
   Copy the input before the transaction, including when an output aliases a component. */
int adf_sball_root_seed_at(adf_sball_t y, adf_place_t *where, const adf_sball_t x,
                          adf_place_t v, ulong n, ulong seed, slong N)
{
    adf_lball_t c,r;
    adf_sball_t t;
    int st;
    ADF_INV_SBALL(x);
    adf_lball_init(c); adf_lball_init(r); adf_sball_init(t);
    if (!adf_sball_has_place(x,v)) st=ADF_DOMAIN;
    else if (adf_place_is_archimedean(v)) st=ADF_UNSUPPORTED;
    else
    {
        st=adf_sball_get_lball(c,x,v);
        if (st==ADF_OK) st=adf_lball_root_seed(r,c,n,seed,N);
        if (st==ADF_OK) st=adf_sball_set_arb_lballs(t,NULL,NULL,r,1);
        if (st==ADF_OK) adf_sball_set(y,t);
    }
    if (st!=ADF_OK && where) *where=v;
    adf_lball_clear(c); adf_lball_clear(r); adf_sball_clear(t);
    return st;
}

/* Proposition 13 step 3, functions.md:435; same transaction as root_seed_at. */
int adf_sball_sqrt_seed_at(adf_sball_t y, adf_place_t *where, const adf_sball_t x,
                          adf_place_t v, ulong seed, slong N)
{ return adf_sball_root_seed_at(y,where,x,v,2,seed,N); }

/* R6; functions.md:410,:463,:725. The local list function owns the output transaction. */
int adf_sball_roots_at(adf_lball_ptr y, ulong *ids, slong *len, slong capacity, adf_place_t *where,
                      const adf_sball_t x, adf_place_t v, ulong n, slong N)
{
    adf_lball_t c;
    int st;
    ADF_INV_SBALL(x);
    adf_lball_init(c);
    if (!adf_sball_has_place(x,v)) st=ADF_DOMAIN;
    else if (adf_place_is_archimedean(v)) st=ADF_UNSUPPORTED;
    else
    {
        st=adf_sball_get_lball(c,x,v);
        if (st==ADF_OK) st=adf_lball_roots(y,ids,len,capacity,c,n,N);
    }
    if (st!=ADF_OK && where) *where=v;
    adf_lball_clear(c);
    return st;
}

/* lane f-slice9 (1F.6): rational power at a prime, rfunc.h; docs/api-1f6.md P8 (the pattern of R6 step 5 of
   api-1f5.md): place membership first, then the real place (UNSUPPORTED), then lpow.h on the projected component;
   the partial ball over v alone (Proposition 22, docs/proofs/functions.md:725). The component is copied before the
   transaction, so y may be x; a failure leaves y untouched and sets where = v. */
int adf_sball_powrat_at(adf_sball_t y, adf_place_t *where, const adf_sball_t x, adf_place_t v,
                        slong e, ulong n, ulong seed, slong N)
{
    adf_lball_t c,r;
    adf_sball_t t;
    int st;
    ADF_INV_SBALL(x);
    adf_lball_init(c); adf_lball_init(r); adf_sball_init(t);
    if (!adf_sball_has_place(x,v)) st=ADF_DOMAIN;
    else if (adf_place_is_archimedean(v)) st=ADF_UNSUPPORTED;
    else
    {
        st=adf_sball_get_lball(c,x,v);
        if (st==ADF_OK) st=adf_lball_powrat(r,c,e,n,seed,N);
        if (st==ADF_OK) st=adf_sball_set_arb_lballs(t,NULL,NULL,r,1);
        if (st==ADF_OK) adf_sball_set(y,t);
    }
    if (st!=ADF_OK && where) *where=v;
    adf_lball_clear(c); adf_lball_clear(r); adf_sball_clear(t);
    return st;
}

/* lane f-slice9 (1F.6): power of a principal unit at a prime, rfunc.h; api-1f6.md P8. v must be a place of x and of
   s; both components are copied before the transaction, so y may be x, s or both, and x may be s. */
int adf_sball_powunit_at(adf_sball_t y, adf_place_t *where, const adf_sball_t x, const adf_sball_t s,
                         adf_place_t v, slong N)
{
    adf_lball_t c,e,r;
    adf_sball_t t;
    int st;
    ADF_INV_SBALL(x);
    ADF_INV_SBALL(s);
    adf_lball_init(c); adf_lball_init(e); adf_lball_init(r); adf_sball_init(t);
    if (!adf_sball_has_place(x,v) || !adf_sball_has_place(s,v)) st=ADF_DOMAIN;
    else if (adf_place_is_archimedean(v)) st=ADF_UNSUPPORTED;
    else
    {
        st=adf_sball_get_lball(c,x,v);
        if (st==ADF_OK) st=adf_sball_get_lball(e,s,v);
        if (st==ADF_OK) st=adf_lball_powunit(r,c,e,N);
        if (st==ADF_OK) st=adf_sball_set_arb_lballs(t,NULL,NULL,r,1);
        if (st==ADF_OK) adf_sball_set(y,t);
    }
    if (st!=ADF_OK && where) *where=v;
    adf_lball_clear(c); adf_lball_clear(e); adf_lball_clear(r); adf_sball_clear(t);
    return st;
}
