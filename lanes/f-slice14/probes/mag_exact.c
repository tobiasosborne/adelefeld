/* Probe: which FLINT 3.0.1 call stores a 30-bit dyadic radius exactly as a mag. */
#include <flint/arb.h>
int main(void)
{
    fmpz_t m, e;
    mag_t a, b;
    arf_t x, y;
    fmpz_init(m); fmpz_init(e); mag_init(a); mag_init(b); arf_init(x); arf_init(y);
    fmpz_set_ui(m, 536870913);
    fmpz_set_si(e, -209);
    arf_set_fmpz_2exp(x, m, e);
    arf_get_mag(a, x);
    arf_set_mag(y, a);
    printf("arf_get_mag exact %d\n", arf_equal(x, y));
    mag_set_fmpz_2exp_fmpz(b, m, e);
    arf_set_mag(y, b);
    printf("mag_set_fmpz_2exp_fmpz exact %d\n", arf_equal(x, y));
    mag_set_ui_2exp_si(b, 536870913, -209);
    arf_set_mag(y, b);
    printf("mag_set_ui_2exp_si exact %d\n", arf_equal(x, y));
    fmpz_clear(m); fmpz_clear(e); mag_clear(a); mag_clear(b); arf_clear(x); arf_clear(y);
    return 0;
}
