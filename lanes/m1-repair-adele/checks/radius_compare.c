/* radius_compare.c: the radius of the new adf_adele_mul_rat and adf_adele_div_rat against the
   radius of the old implementation, on 1000 random inputs at prec = 53.

   The old real part (src/adele.c before the M1-D4 repair) was
       mul_rat: arb_set_fmpq(t, q, prec); arb_mul(z, x, t, prec)
       div_rat: arb_set_fmpq(t, q, prec); arb_div(z, x, t, prec)
   The new one is arb_mul_fmpz/arb_div_fmpz by the exact numerator and denominator. This program
   reproduces the old calls with FLINT directly and compares the radii of the two output balls
   with mag_cmp. Build and run from the repository root against build/libadelefeld.a. */

#include <stdio.h>

#include <flint/arb.h>
#include <flint/fmpz.h>
#include <flint/fmpq.h>
#include <flint/flint.h>

#include <adelefeld/adele.h>

#define CASES 1000

static void
count(int * smaller, int * equal, int * larger, int * old_nonfinite, int cmp, int oldfinite)
{
    if (!oldfinite)
        (*old_nonfinite)++;
    else if (cmp < 0)
        (*smaller)++;
    else if (cmp == 0)
        (*equal)++;
    else
        (*larger)++;
}

int
main(void)
{
    flint_rand_t state;
    slong i;
    int ms = 0, me = 0, ml = 0, mnf = 0;
    int ds = 0, de = 0, dl = 0, dnf = 0;
    adf_adele_t x, zm, zd;
    adf_fball_t f;
    arb_t r, t, old;
    adf_rat_t qr;
    fmpz_t n, d;
    fmpq_t q;

    flint_randinit(state);
    adf_adele_init(x);
    adf_adele_init(zm);
    adf_adele_init(zd);
    adf_fball_init(f);
    arb_init(r);
    arb_init(t);
    arb_init(old);
    adf_rat_init(qr);
    fmpz_init(n);
    fmpz_init(d);
    fmpq_init(q);
    adf_fball_set_si(f, 1);

    for (i = 0; i < CASES; i++)
    {
        arb_randtest(r, state, 53, 12);
        fmpz_randtest_not_zero(n, state, 80);
        fmpz_randtest_not_zero(d, state, 80);
        fmpz_abs(d, d);
        /* q = n/d, canonical. */
        adf_rat_set_fmpz2(qr, n, d);
        adf_rat_get_fmpq(q, qr);
        if (fmpq_is_zero(q))
            continue;
        if (adf_adele_set_arb_fball(x, r, f) != ADF_OK)
            continue;

        /* new mul_rat. */
        adf_adele_mul_rat(zm, x, qr, 53);
        /* old mul_rat. */
        arb_set_fmpq(t, q, 53);
        arb_mul(old, x->inf, t, 53);
        count(&ms, &me, &ml, &mnf,
              arb_is_finite(old) ? mag_cmp(arb_radref(zm->inf), arb_radref(old)) : 0,
              arb_is_finite(old));

        /* new div_rat. */
        if (adf_adele_div_rat(zd, x, qr, 53) == ADF_OK)
        {
            arb_set_fmpq(t, q, 53);
            arb_div(old, x->inf, t, 53);
            count(&ds, &de, &dl, &dnf,
                  arb_is_finite(old) ? mag_cmp(arb_radref(zd->inf), arb_radref(old)) : 0,
                  arb_is_finite(old));
        }
    }

    printf("mul_rat: %d smaller, %d equal, %d larger, %d old non-finite\n", ms, me, ml, mnf);
    printf("div_rat: %d smaller, %d equal, %d larger, %d old non-finite\n", ds, de, dl, dnf);

    flint_randclear(state);
    adf_adele_clear(x);
    adf_adele_clear(zm);
    adf_adele_clear(zd);
    adf_fball_clear(f);
    arb_clear(r);
    arb_clear(t);
    arb_clear(old);
    adf_rat_clear(qr);
    fmpz_clear(n);
    fmpz_clear(d);
    fmpq_clear(q);
    return 0;
}
