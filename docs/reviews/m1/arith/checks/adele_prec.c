/* Enclosure of the real coordinate of adf_adele_add_rat, mul_rat, div_rat (and set_rat), at small
   and ordinary precisions, with and without aliasing of the output with the input.

   For each case the program prints one line
       op prec alias | x_lo x_hi | q | r_lo r_hi finite | fin_in | fin_out
   with the exact end points of the input real ball and of the output real ball as rationals (read
   with arb_get_interval_fmpz_2exp, arb.rst:461, a FLINT function that is not under review), the
   rational q, and the finite parts as canonical triples. adele_prec_check.py checks with
   fractions.Fraction that the output interval contains { x op q : x in [x_lo, x_hi] } and that the
   finite part is the exact set image. The program also prints, for set_rat, the claim of adele.h:102
   ("When q is a dyadic number whose odd mantissa has at most prec bits the real ball is exact").

   Usage: adele_prec <ncases> <seed> */

#include <stdio.h>
#include <stdlib.h>
#include <adelefeld.h>
#include <flint/arb.h>

static void
print_interval(const arb_t x)
{
    fmpz_t a, b, e;
    fmpz_init(a); fmpz_init(b); fmpz_init(e);
    if (!arb_is_finite(x))
    {
        printf("nan nan 0");
    }
    else
    {
        arb_get_interval_fmpz_2exp(a, b, e, x);
        fmpz_print(a); printf("*2^"); fmpz_print(e); printf(" ");
        fmpz_print(b); printf("*2^"); fmpz_print(e); printf(" 1");
    }
    fmpz_clear(a); fmpz_clear(b); fmpz_clear(e);
}

static void
print_fball(const adf_fball_t f)
{
    fmpz_t A, H, d;
    fmpz_init(A); fmpz_init(H); fmpz_init(d);
    adf_fball_get_fmpz3(A, H, d, f);
    fmpz_print(A); printf(" "); fmpz_print(H); printf(" "); fmpz_print(d);
    fmpz_clear(A); fmpz_clear(H); fmpz_clear(d);
}

static void
rand_fmpz(fmpz_t r, flint_rand_t st, int bits, int allow_neg)
{
    fmpz_randtest(r, st, bits);
    if (!allow_neg)
        fmpz_abs(r, r);
}

int
main(int argc, char ** argv)
{
    slong ncases = argc > 1 ? atol(argv[1]) : 1000;
    ulong seed = argc > 2 ? strtoul(argv[2], NULL, 10) : 1;
    slong precs[] = { 2, 3, 4, 7, 10, 53, 128 };
    flint_rand_t st;
    slong i;

    flint_randinit(st);
    flint_randseed(st, seed, seed * 7 + 3);

    for (i = 0; i < ncases; i++)
    {
        adf_adele_t x, z;
        adf_rat_t q;
        fmpz_t n, dd, A, H, D;
        slong prec = precs[n_randint(st, 7)];
        int op = n_randint(st, 3);
        int alias = n_randint(st, 2);
        int stt = ADF_OK;

        adf_adele_init(x);
        adf_adele_init(z);
        adf_rat_init(q);
        fmpz_init(n); fmpz_init(dd); fmpz_init(A); fmpz_init(H); fmpz_init(D);

        /* the real ball: a random finite arb (arb_randtest: random mid and radius) */
        arb_randtest(x->inf, st, 1 + n_randint(st, 200), 1 + n_randint(st, 10));
        /* the finite ball: random raw triple */
        rand_fmpz(A, st, 1 + n_randint(st, 40), 1);
        rand_fmpz(H, st, n_randint(st, 20), 0);
        rand_fmpz(D, st, 1 + n_randint(st, 20), 1);
        if (fmpz_is_zero(D)) fmpz_one(D);
        adf_fball_set_fmpz3(&x->fin, A, H, D);
        /* q: random rational, sometimes dyadic, sometimes 0 */
        rand_fmpz(n, st, 1 + n_randint(st, 150), 1);
        if (n_randint(st, 3) == 0)
        {
            fmpz_one(dd);
            fmpz_mul_2exp(dd, dd, n_randint(st, 100));
        }
        else
        {
            rand_fmpz(dd, st, 1 + n_randint(st, 150), 0);
            if (fmpz_is_zero(dd)) fmpz_one(dd);
        }
        adf_rat_set_fmpz2(q, n, dd);

        if (!alias)
        {
            if (op == 0) adf_adele_add_rat(z, x, q, prec);
            else if (op == 1) adf_adele_mul_rat(z, x, q, prec);
            else stt = adf_adele_div_rat(z, x, q, prec);
        }
        else
        {
            adf_adele_set(z, x);
            if (op == 0) adf_adele_add_rat(z, z, q, prec);
            else if (op == 1) adf_adele_mul_rat(z, z, q, prec);
            else stt = adf_adele_div_rat(z, z, q, prec);
        }
        printf("%s %ld %d %d | ", op == 0 ? "add" : op == 1 ? "mul" : "div", (long) prec, alias,
               stt);
        print_interval(x->inf);
        printf(" | ");
        fmpq_print(q->q);
        printf(" | ");
        print_interval(z->inf);
        printf(" | ");
        print_fball(&x->fin);
        printf(" | ");
        print_fball(&z->fin);
        printf(" | %d\n", adf_adele_is_canonical(z));

        /* set_rat */
        adf_adele_set_rat(z, q, prec);
        printf("set %ld 0 0 | 0*2^0 0*2^0 1 | ", (long) prec);
        fmpq_print(q->q);
        printf(" | ");
        print_interval(z->inf);
        printf(" | 0 0 1 | ");
        print_fball(&z->fin);
        printf(" | %d\n", adf_adele_is_canonical(z));

        adf_adele_clear(x);
        adf_adele_clear(z);
        adf_rat_clear(q);
        fmpz_clear(n); fmpz_clear(dd); fmpz_clear(A); fmpz_clear(H); fmpz_clear(D);
    }
    flint_randclear(st);
    flint_cleanup();
    return 0;
}
