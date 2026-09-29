/* Compile the reviewed translation unit here to call its private refinement directly. */
#include "../../src/roots_real.c"
#include <stdio.h>

static void emit(const fmpz_t c, slong k, int point)
{
    fmpz_print(c); printf(" k=%ld point=%d\n", k, point);
}

int main(void)
{
    fmpz_poly_t g, dg;
    fmpz_t c;
    arf_t floor;
    slong k;
    int point, st;
    fmpz_poly_init(g); fmpz_poly_init(dg); fmpz_init(c); arf_init(floor);
    fmpz_poly_set_coeff_si(g, 0, -100); fmpz_poly_set_coeff_si(g, 1, 3);
    fmpz_poly_derivative(dg, g);
    fmpz_zero(c); k = 6; arf_set_si(floor, 33);
    st = refine_cell(c, &k, &point, g, dg, floor, 1, 2);
    printf("need=2 floor=33 status=%d c=", st); emit(c, k, point);
    fmpz_zero(c); k = 6;
    st = refine_cell(c, &k, &point, g, dg, floor, 0, 3);
    printf("need=3 no_floor status=%d c=", st); emit(c, k, point);
    fmpz_poly_zero(g); fmpz_poly_set_coeff_si(g, 0, -1);
    fmpz_one(c); fmpz_mul_2exp(c, c, ADF_ROOTS_BITS_MAX - 1);
    fmpz_poly_set_coeff_fmpz(g, 1, c); fmpz_poly_derivative(dg, g);
    fmpz_zero(c); k = KMIN;
    st = refine_cell(c, &k, &point, g, dg, floor, 0, 2);
    printf("dyadic root=2^(1-M) need=2 status=%d c=", st); emit(c, k, point);
    fmpz_poly_clear(g); fmpz_poly_clear(dg); fmpz_clear(c); arf_clear(floor);
    flint_cleanup();
    return 0;
}
