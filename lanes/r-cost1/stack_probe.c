#include "roots_stack_probe.c"
#include <stdio.h>

int main(void)
{
    static const slong numerators[] = { 32, 36, 80, 84, 104, 108, 116, 118, 122, 123, 125, 126 };
    fmpz_poly_t f, linear;
    arb_ptr b = _arb_vec_init(12);
    slong i, m;
    int st;
    fmpz_poly_init(f); fmpz_poly_init(linear); fmpz_poly_one(f);
    for (i = 0; i < 12; i++)
    {
        fmpz_poly_set_coeff_si(linear, 1, 2);
        fmpz_poly_set_coeff_si(linear, 0, -numerators[i]);
        fmpz_poly_mul(f, f, linear);
    }
    fmpz_poly_primitive_part(f, f);
    st = adf_roots_real_isolate_counted(b, &m, f, 2, 12);
    printf("stack fixture status %d roots %ld\n", st, (long) m);
    _arb_vec_clear(b, 12); fmpz_poly_clear(f); fmpz_poly_clear(linear); flint_cleanup();
    return st != ADF_OK || m != 12;
}
