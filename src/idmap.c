/* src/idmap.c: ideles and adeles: the two hulls (idele -> adele), adele -> idele, and the division of an adele by
   an idele (milestone 2, slice 3, lane i-slice3).

   Contract: include/adelefeld/idmap.h. Sources, read before the code was written:
   - docs/SPEC.md 4.5, 5 ("Idele to adele", "Division of an adele by an idele"); docs/conventions.md 5.5, 5.7
     ("Idele to adele", adf_adele_div_idele, CV-50), 3.2, 3.3, 4.1, 4.3, 4.4 (CV-08: a non-finite ball is never
     stored); decisions M0-D7, M1-D4;
   - docs/proofs/ideles.md Proposition 3 (P3.4, line 80), Proposition 16 (line 404; P16.1 line 409, P16.2 line 410,
     P16.3 line 411, P16.4 line 412), Proposition 17 (line 443), Proposition 18 (line 462), Proposition 19 (line
     472; P19.6, the agreement with the product rule, line 506);
   - docs/api-2.md 3.3, Statements M (the hulls as computed), N (adele to idele), O (the division as computed);
   - adelefeld/fball.h: adf_fball_set_fmpz3 (canonical triple), adf_fball_mul (the tight product rule, SPEC 4.3,
     precision.md Proposition 2), adf_fball_div_rat (P18), adf_fball_set_global;
   - FLINT 3.0.1: arb_div, refs/src/flint-3.0.1/arb.rst:856-876; arb_is_zero, arb_is_nonzero, arb_is_finite,
     arb.rst:593-609.
   Reference: proto/ideles_checks.py, ref_hull, ref_hull_simple, ref_idele_set_adele, ref_div_fin (part 4). */

#include <adelefeld.h>

#include "invariants.h"

#ifdef ADF_CHECK_INVARIANTS
#define MP_INV_IDELE(x)                                                                                \
    do { if (!adf_idele_is_canonical(x)) adf_inv_fail(__func__, #x, "adf_idele"); } while (0)
#else
#define MP_INV_IDELE(x) ((void) 0)
#endif

/* Statement M: f = r c' + r L Zhat, global canonical, for the unit u = (c, N) and the content r > 0.
   smallest = 1: from the pair as given, L = lcm(N, 2) and c' odd, c' = c mod N (P16.2); smallest = 0: from the
   normal form (c', N') of u, L = N' (P16.1 of the normal form). N = 0: the exact rational r c (SPEC 5). The ball
   is (n c' + n L Zhat)/d for r = n/d: the set r c' + r L Zhat, made canonical by adf_fball_set_fmpz3. */
static void
hull_fin(adf_fball_t f, const fmpq_t r, const adf_ucoset_t u, int smallest)
{
    adf_ucoset_t v;
    fmpz_t A, H;

    adf_ucoset_init(v);
    fmpz_init(A);
    fmpz_init(H);
    if (smallest)
        adf_ucoset_set(v, u);
    else
        adf_ucoset_normalise(v, u);
    fmpz_set(A, v->c);
    if (!fmpz_is_zero(v->N))
    {
        fmpz_set(H, v->N);
        if (fmpz_is_odd(v->N) && smallest)
        {
            fmpz_mul_2exp(H, H, 1);                     /* lcm(N, 2) = 2 N for odd N */
            if (fmpz_is_even(A))
                fmpz_add(A, A, v->N);                   /* c + N is odd */
        }
        fmpz_mul(H, H, fmpq_numref(r));
    }
    fmpz_mul(A, A, fmpq_numref(r));
    if (adf_fball_set_fmpz3(f, A, H, fmpq_denref(r)) != ADF_OK)
        flint_abort();                                  /* d = den(r) > 0 and H >= 0: cannot happen */
    adf_ucoset_clear(v);
    fmpz_clear(A);
    fmpz_clear(H);
}

static void
adele_set_hull(adf_adele_t y, const adf_idele_t x, int smallest)
{
    adf_fball_t f;
    MP_INV_IDELE(x);
    adf_fball_init(f);
    hull_fin(f, x->r, &x->u, smallest);
    arb_set(y->inf, x->inf);                            /* the real coordinate, a copy */
    adf_fball_swap(&y->fin, f);                         /* a local old value goes to f and is cleared */
    adf_fball_clear(f);
}

void
adf_adele_set_idele(adf_adele_t y, const adf_idele_t x)
{
    adele_set_hull(y, x, 1);
}

void
adf_adele_set_idele_simple(adf_adele_t y, const adf_idele_t x)
{
    adele_set_hull(y, x, 0);
}

/* Statement N: the statuses by the maximum (conventions 3.3); OK only for an exact finite part a != 0 and a real
   ball that excludes 0; then y = (X, |a|, [sign a]) (P3.4). */
int
adf_idele_set_adele(adf_idele_t y, const adf_adele_t x)
{
    int st = ADF_OK;
    adf_rat_t a;

    ADF_INV_ADELE(x);
    if (arb_is_zero(x->inf))
        st = FLINT_MAX(st, ADF_NOT_UNIT);               /* every point has x_inf = 0 */
    else if (!arb_is_nonzero(x->inf))
        st = FLINT_MAX(st, ADF_UNIT_NOT_CERTIFIED);     /* 0 and non-zero reals */
    adf_rat_init(a);
    if (!adf_fball_is_exact(&x->fin))
        st = FLINT_MAX(st, ADF_UNIT_NOT_CERTIFIED);     /* SPEC 4.5, ideles.md P17 */
    else
    {
        adf_fball_get_center(a, &x->fin);
        if (fmpq_is_zero(a->q))
            st = FLINT_MAX(st, ADF_NOT_UNIT);           /* every point has finite part 0 */
    }
    if (st == ADF_OK)
    {
        arb_set(y->inf, x->inf);
        fmpq_abs(y->r, a->q);
        if (fmpq_sgn(a->q) > 0)
            adf_ucoset_one(&y->u);
        else
            adf_ucoset_minus_one(&y->u);
    }
    adf_rat_clear(a);
    return st;
}

/* Statement O: the finite part is the product rule of the finite part of x with the smallest hull e + L Zhat of
   the inverse coset (hull_fin with r = 1 of adf_ucoset_inv(u); for an exact unit the exact e), then divided by the
   exact r (P19.6, P18); the real part arb_div at max(prec, 2) bits, NOT_DETERMINED if it is not finite
   (conventions 4.4, CV-08). Everything is computed into temporaries before z is written (z may be x). */
int
adf_adele_div_idele(adf_adele_t z, const adf_adele_t x, const adf_idele_t y, slong prec)
{
    slong p = prec < 2 ? 2 : prec;
    arb_t t;
    adf_fball_t h, q;
    adf_ucoset_t w;
    adf_rat_t r;
    fmpq_t one;
    int st = ADF_OK;

    if (prec > ADF_IDELE_PREC_MAX)
        return ADF_LIMIT;                       /* first: from prec alone, before the entry checks (N-D8, F1) */
    ADF_INV_ADELE(x);
    MP_INV_IDELE(y);
    arb_init(t);
    arb_div(t, x->inf, y->inf, p);
    if (!arb_is_finite(t))
    {
        arb_clear(t);
        return ADF_NOT_DETERMINED;
    }
    adf_fball_init(h);
    adf_fball_init(q);
    adf_ucoset_init(w);
    adf_rat_init(r);
    fmpq_init(one);
    fmpq_one(one);
    adf_ucoset_inv(w, &y->u);                           /* c* U(N), normal form (P10.2) */
    hull_fin(h, one, w, 1);                             /* e + lcm(N, 2) Zhat, e odd (P16) */
    adf_fball_mul(q, &x->fin, h);                       /* a e + gcd(|a| L, M) Zhat (P19.6) */
    fmpq_set(r->q, y->r);
    if (adf_fball_div_rat(q, q, r) != ADF_OK)           /* exact division by r > 0 (P18) */
        flint_abort();
    adf_fball_set_global(q, q);
    arb_swap(z->inf, t);
    adf_fball_swap(&z->fin, q);
    adf_fball_clear(h);
    adf_fball_clear(q);
    adf_ucoset_clear(w);
    adf_rat_clear(r);
    fmpq_clear(one);
    arb_clear(t);
    return st;
}
