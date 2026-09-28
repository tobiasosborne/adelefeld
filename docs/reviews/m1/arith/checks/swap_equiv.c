/* Checks the excuse of tools/mutate/equivalent.txt:52-65 (src/adele.c, swap_args): "arb_add /
   arb_mul / acb_add / acb_mul with the two inputs exchanged ... so the same value is written".
   For random finite balls it compares f(x, y) and f(y, x) bitwise (arb_equal / acb_equal: the same
   midpoint and the same radius) and counts the pairs where they differ. It also runs the finite
   parts, adf_fball_add and adf_fball_mul with the operands exchanged, through adf_fball_identical.
   Usage: swap_equiv <ncases> <seed> */

#include <stdio.h>
#include <stdlib.h>
#include <adelefeld.h>
#include <flint/arb.h>
#include <flint/acb.h>

int
main(int argc, char ** argv)
{
    slong n = argc > 1 ? atol(argv[1]) : 100000;
    ulong seed = argc > 2 ? strtoul(argv[2], NULL, 10) : 1;
    slong i, diff_add = 0, diff_mul = 0, diff_cadd = 0, diff_cmul = 0, diff_fadd = 0, diff_fmul = 0;
    slong shown = 0;
    flint_rand_t st;
    arb_t x, y, z1, z2;
    acb_t cx, cy, c1, c2;
    adf_fball_t fx, fy, f1, f2;
    fmpz_t A, H, d;

    flint_randinit(st);
    flint_randseed(st, seed, seed + 1);
    arb_init(x); arb_init(y); arb_init(z1); arb_init(z2);
    acb_init(cx); acb_init(cy); acb_init(c1); acb_init(c2);
    adf_fball_init(fx); adf_fball_init(fy); adf_fball_init(f1); adf_fball_init(f2);
    fmpz_init(A); fmpz_init(H); fmpz_init(d);

    for (i = 0; i < n; i++)
    {
        slong prec = 2 + n_randint(st, 200);
        arb_randtest(x, st, 1 + n_randint(st, 300), 1 + n_randint(st, 8));
        arb_randtest(y, st, 1 + n_randint(st, 300), 1 + n_randint(st, 8));
        if (n_randint(st, 4) == 0) mag_zero(arb_radref(x));
        arb_add(z1, x, y, prec);
        arb_add(z2, y, x, prec);
        if (!arb_equal(z1, z2)) diff_add++;
        arb_mul(z1, x, y, prec);
        arb_mul(z2, y, x, prec);
        if (!arb_equal(z1, z2))
        {
            diff_mul++;
            if (shown < 3)
            {
                shown++;
                printf("arb_mul differs at prec %ld:\n  x = ", (long) prec); arb_printd(x, 20);
                printf("\n  y = "); arb_printd(y, 20);
                printf("\n  x*y rad = "); mag_printd(arb_radref(z1), 20);
                printf("\n  y*x rad = "); mag_printd(arb_radref(z2), 20);
                printf("\n  midpoints equal: %d\n", arf_equal(arb_midref(z1), arb_midref(z2)));
            }
        }
        acb_randtest(cx, st, 1 + n_randint(st, 300), 1 + n_randint(st, 8));
        acb_randtest(cy, st, 1 + n_randint(st, 300), 1 + n_randint(st, 8));
        if (n_randint(st, 4) == 0) arb_zero(acb_imagref(cx));
        acb_add(c1, cx, cy, prec);
        acb_add(c2, cy, cx, prec);
        if (!acb_equal(c1, c2)) diff_cadd++;
        acb_mul(c1, cx, cy, prec);
        acb_mul(c2, cy, cx, prec);
        if (!acb_equal(c1, c2)) diff_cmul++;

        fmpz_randtest(A, st, 60); fmpz_randtest_unsigned(H, st, 40); fmpz_randtest_not_zero(d, st, 40);
        adf_fball_set_fmpz3(fx, A, H, d);
        fmpz_randtest(A, st, 60); fmpz_randtest_unsigned(H, st, 40); fmpz_randtest_not_zero(d, st, 40);
        adf_fball_set_fmpz3(fy, A, H, d);
        adf_fball_add(f1, fx, fy); adf_fball_add(f2, fy, fx);
        if (!adf_fball_identical(f1, f2)) diff_fadd++;
        adf_fball_mul(f1, fx, fy); adf_fball_mul(f2, fy, fx);
        if (!adf_fball_identical(f1, f2)) diff_fmul++;
    }
    printf("cases %ld: arb_add differs %ld, arb_mul differs %ld, acb_add differs %ld, "
           "acb_mul differs %ld, adf_fball_add differs %ld, adf_fball_mul differs %ld\n",
           (long) n, (long) diff_add, (long) diff_mul, (long) diff_cadd, (long) diff_cmul,
           (long) diff_fadd, (long) diff_fmul);
    arb_clear(x); arb_clear(y); arb_clear(z1); arb_clear(z2);
    acb_clear(cx); acb_clear(cy); acb_clear(c1); acb_clear(c2);
    adf_fball_clear(fx); adf_fball_clear(fy); adf_fball_clear(f1); adf_fball_clear(f2);
    fmpz_clear(A); fmpz_clear(H); fmpz_clear(d);
    flint_randclear(st);
    flint_cleanup();
    return 0;
}
