/* Disposable behaviour probe; only documented APIs, finite validated inputs.
   Ground truth for dump fields: refs/src/flint-3.0.1/arb.rst:288-293.
   No malformed input is passed to arb_load_str. */
#include <stdio.h>
#include <flint/arb.h>
#include <flint/acb_poly.h>

static void dump(const char *name, const arb_t x)
{
    char *s = arb_dump_str(x);
    printf("%s=%s\n", name, s);
    flint_free(s);
}

int main(void)
{
    arb_t x, y, z;
    acb_poly_t p;
    arb_init(x); arb_init(y); arb_init(z);
    acb_poly_init(p);
    printf("flint=%s\n", flint_version);
    arb_one(x);
    mag_set_ui(arb_radref(x), (1UL << 30) - 1);
    mag_mul_2exp_si(arb_radref(x), arb_radref(x), -30);
    arb_set(y, x);
    arb_mul(z, x, y, 128);
    printf("input_nonzero=%d product_nonzero=%d\n", arb_is_nonzero(x), arb_is_nonzero(z));
    dump("input", x); dump("product", z);
    arb_set_str(x, "1 +/- 0.13", 128);
    dump("read_decimal_1", x);
    arb_set_str(y, "1 +/- 0.14", 128);
    dump("read_decimal_2", y);
    acb_poly_set_coeff_si(p, 0, 0);
    printf("zero_polynomial_length=%ld\n", acb_poly_length(p));
    acb_poly_set_coeff_si(p, 0, 1);
    acb_poly_set_coeff_si(p, 1, 0);
    printf("trailing_zero_polynomial_length=%ld\n", acb_poly_length(p));
    acb_poly_clear(p);
    arb_clear(x); arb_clear(y); arb_clear(z);
    flint_cleanup();
    return 0;
}
