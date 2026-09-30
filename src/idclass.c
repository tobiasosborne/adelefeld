/* src/idclass.c: adf_idclass, the idele class group of Q to finite precision, and the class map (milestone 2,
   slice 2, lane i-slice2).

   Contract: include/adelefeld/idclass.h. Sources, read before the code was written:
   - docs/SPEC.md 5 ("Operations": t = |x_inf| / r, u' = sign(x_inf) u, "the sign on the unit is essential";
     "Sign preservation": the rule applies to the idele-to-class conversion); docs/conventions.md 5.7 (struct,
     class predicate, class init, the result sign check G5), 3.2, 4.1, 4.3, 4.4; M1-D4; N-D6;
   - docs/proofs/ideles.md Proposition 15 (line 366; P15.1 line 370, the map is an isomorphism of groups onto
     R_{>0} x Zhat^x with kernel Q^x; P15.3 line 375; "On data", line 393);
   - docs/api-2.md 1.3, Statements A, C, E; 2.3, Statements F and G;
   - FLINT 3.0.1: arb_is_finite, arb_is_positive, refs/src/flint-3.0.1/arb.rst:606, :639-649; the kernel of
     src/idele.c through src/idele_internal.h.
   Reference: proto/ideles_checks.py, ref_idele_class, ref_idclass_mul, ref_idclass_inv (part 3). */

#include <adelefeld.h>

#include "invariants.h"
#include "idele_internal.h"

#ifdef ADF_CHECK_INVARIANTS
#define IC_INV(x)                                                                                      \
    do { if (!adf_idclass_is_canonical(x)) adf_inv_fail(__func__, #x, "adf_idclass"); } while (0)
#define IC_INV_IDELE(x)                                                                                \
    do { if (!adf_idele_is_canonical(x)) adf_inv_fail(__func__, #x, "adf_idele"); } while (0)
#define IC_INV_UC(x)                                                                                   \
    do { if (!adf_ucoset_is_canonical(x)) adf_inv_fail(__func__, #x, "adf_ucoset"); } while (0)
#else
#define IC_INV(x) ((void) 0)
#define IC_INV_IDELE(x) ((void) 0)
#define IC_INV_UC(x) ((void) 0)
#endif

/* ---- life cycle ---- */

/* conventions 5.7: class init <1 ; [1 mod 0]> */
void
adf_idclass_init(adf_idclass_t x)
{
    arb_init(x->t);
    arb_one(x->t);
    adf_ucoset_init(&x->u);
}

void
adf_idclass_clear(adf_idclass_t x)
{
    arb_clear(x->t);
    adf_ucoset_clear(&x->u);
}

void
adf_idclass_set(adf_idclass_t y, const adf_idclass_t x)
{
    IC_INV(x);
    arb_set(y->t, x->t);
    adf_ucoset_set(&y->u, &x->u);
}

void
adf_idclass_swap(adf_idclass_t x, adf_idclass_t y)
{
    arb_swap(x->t, y->t);
    adf_ucoset_swap(&x->u, &y->u);
}

/* conventions 5.7: arb_is_finite(t) and arb_is_positive(t); u satisfies 5.6. */
int
adf_idclass_is_canonical(const adf_idclass_t x)
{
    return arb_is_finite(x->t) && arb_is_positive(x->t) && adf_ucoset_is_canonical(&x->u);
}

int
adf_idclass_identical(const adf_idclass_t x, const adf_idclass_t y)
{
    IC_INV(x);
    IC_INV(y);
    return arb_equal(x->t, y->t) && adf_ucoset_identical(&x->u, &y->u);
}

/* ---- constructors and accessors ---- */

int
adf_idclass_set_parts(adf_idclass_t x, const arb_t t, const adf_ucoset_t u)
{
    IC_INV_UC(u);
    if (!arb_is_finite(t) || !arb_is_positive(t))
        return ADF_DOMAIN;
    arb_set(x->t, t);
    adf_ucoset_set(&x->u, u);          /* the modulus as supplied (CV-17) */
    return ADF_OK;
}

/* Statement G.4: the class of (X, r, u) is (abs(X)/r, sign(X) u). T is the norm (Statement F with s = d/n,
   kernel B, sign +1), the unit adf_ucoset_mul(u, [sign X]) in normal form (A.1, B). */
int
adf_idclass_set_idele(adf_idclass_t c, const adf_idele_t x, slong prec)
{
    arb_t t;
    int st;

    if (prec > ADF_IDELE_PREC_MAX)
        return ADF_LIMIT;                  /* first: from prec alone, before the entry checks (N-D8, F1) */
    IC_INV_IDELE(x);
    arb_init(t);
    st = adf_idele_norm(t, x, prec);   /* T: the norm, Statement I.4 (also the limit and B1) */
    if (st == ADF_OK)
    {
        adf_ucoset_t e, u;
        adf_ucoset_init(e);
        adf_ucoset_init(u);
        if (arf_sgn(arb_midref(x->inf)) < 0)          /* the sign of every point of X (E1) */
            adf_ucoset_minus_one(e);
        adf_ucoset_mul(u, &x->u, e);
        arb_swap(c->t, t);
        adf_ucoset_swap(&c->u, u);
        adf_ucoset_clear(e);
        adf_ucoset_clear(u);
    }
    arb_clear(t);
    return st;
}

void
adf_idclass_get_t(arb_t t, const adf_idclass_t x)
{
    IC_INV(x);
    arb_set(t, x->t);
}

void
adf_idclass_get_unit(adf_ucoset_t u, const adf_idclass_t x)
{
    IC_INV(x);
    adf_ucoset_set(u, &x->u);
}

/* The norm of a class is t (ideles.md P14.2 with r = 1 and x_inf = t > 0; P14.3: trivial on Q^x). */
void
adf_idclass_norm(arb_t t, const adf_idclass_t x)
{
    IC_INV(x);
    arb_set(t, x->t);
}

/* ---- arithmetic ---- */

/* Statement G.1 with E2 (sign +1): lo = RD_p(l_x l_y), hi = RU_p(h_x h_y). */
int
adf_idclass_mul(adf_idclass_t z, const adf_idclass_t x, const adf_idclass_t y, slong prec)
{
    slong p = prec < 2 ? 2 : prec;
    arf_t lx, hx, ly, hy, lo, hi;
    arb_t t;
    int st;

    if (prec > ADF_IDELE_PREC_MAX)
        return ADF_LIMIT;                  /* first: from prec alone, before the entry checks (N-D8, F1) */
    IC_INV(x);
    IC_INV(y);
    arf_init(lx);
    arf_init(hx);
    arf_init(ly);
    arf_init(hy);
    arf_init(lo);
    arf_init(hi);
    arb_init(t);
    adf_idele_ends_of_abs(lx, hx, x->t, p);
    adf_idele_ends_of_abs(ly, hy, y->t, p);
    arf_mul(lo, lx, ly, p, ARF_RND_FLOOR);
    arf_mul(hi, hx, hy, p, ARF_RND_CEIL);
    st = adf_idele_ball_from_ends(t, lo, hi, 1, p);
    if (st == ADF_OK)
    {
        adf_ucoset_t u;
        adf_ucoset_init(u);
        adf_ucoset_mul(u, &x->u, &y->u);
        arb_swap(z->t, t);
        adf_ucoset_swap(&z->u, u);
        adf_ucoset_clear(u);
    }
    arf_clear(lx);
    arf_clear(hx);
    arf_clear(ly);
    arf_clear(hy);
    arf_clear(lo);
    arf_clear(hi);
    arb_clear(t);
    return st;
}

/* Statement G.2 with E3 (sign +1): lo = RD_p(1/h_x), hi = RU_p(1/l_x). */
int
adf_idclass_inv(adf_idclass_t y, const adf_idclass_t x, slong prec)
{
    slong p = prec < 2 ? 2 : prec;
    arf_t lx, hx, lo, hi;
    arb_t t;
    int st;

    if (prec > ADF_IDELE_PREC_MAX)
        return ADF_LIMIT;                  /* first: from prec alone, before the entry checks (N-D8, F1) */
    IC_INV(x);
    arf_init(lx);
    arf_init(hx);
    arf_init(lo);
    arf_init(hi);
    arb_init(t);
    adf_idele_ends_of_abs(lx, hx, x->t, p);
    arf_ui_div(lo, 1, hx, p, ARF_RND_FLOOR);
    arf_ui_div(hi, 1, lx, p, ARF_RND_CEIL);
    st = adf_idele_ball_from_ends(t, lo, hi, 1, p);
    if (st == ADF_OK)
    {
        adf_ucoset_t u;
        adf_ucoset_init(u);
        adf_ucoset_inv(u, &x->u);
        arb_swap(y->t, t);
        adf_ucoset_swap(&y->u, u);
        adf_ucoset_clear(u);
    }
    arf_clear(lx);
    arf_clear(hx);
    arf_clear(lo);
    arf_clear(hi);
    arb_clear(t);
    return st;
}
