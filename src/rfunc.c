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
    F_SIN,
    F_COS,
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

static int
at_place(adf_sball_t y, adf_place_t * where, const adf_sball_t x, adf_place_t v, rfn f, ulong n, slong prec)
{
    arb_t t;
    int st;
    ADF_INV_SBALL(x);
    if (!adf_sball_has_place(x, v))
    {
        if (where != NULL)
            *where = v;
        return ADF_DOMAIN;
    }
    if (!adf_place_is_archimedean(v) || x->arch == ADF_ARCH_COMPLEX)
    {
        if (where != NULL)
            *where = v;
        return ADF_UNSUPPORTED;
    }
    if (f == F_ROOT && n == 0)
        return ADF_DOMAIN;
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
