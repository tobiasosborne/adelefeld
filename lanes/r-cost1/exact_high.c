/* Independent exact signs: power for X^50 - 2, balanced factor product for Wilkinson + 1.
   fmpz_pow_ui: refs/src/flint-3.0.1/fmpz.rst:913-917.
   The identities are algebraic: clear the positive dyadic denominator from each expression.
   Count two: evenness and strict monotonicity of x^50 on x > 0, with signs at 1 and 2.
   Count fifty: exact opposite signs at i +/- 1/4, fifty disjoint intervals, degree fifty.
   The existence inference uses the same pending IVT source as solvers P3.8. */
#define _POSIX_C_SOURCE 200809L
#include <adelefeld.h>
#include <flint/fmpz_vec.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int adf_roots_real_isolate_counted(arb_ptr b, slong * m, const fmpz_poly_t g, slong prec, slong count);

static int sign_value(const fmpz_t m, slong e, int wilkinson)
{
    fmpz_t den, power, value;
    fmpz * factors;
    slong i, n, j;
    int sign;
    if (e >= 0 || e == WORD_MIN || (ulong) (-e) > ((ulong) WORD_MAX - 1) / 50)
        flint_abort();
    fmpz_init_set_ui(den, 1); fmpz_init_set_ui(power, 1); fmpz_init(value);
    fmpz_mul_2exp(den, den, (ulong) (-e));
    fmpz_mul_2exp(power, power, 50 * (ulong) (-e) + (wilkinson ? 0 : 1));
    if (!wilkinson)
    {
        fmpz_pow_ui(value, m, 50);
        fmpz_sub(value, value, power);
    }
    else
    {
        factors = _fmpz_vec_init(50);
        for (i = 0; i < 50; i++)
        {
            fmpz_mul_ui(value, den, (ulong) i + 1);
            fmpz_sub(factors + i, m, value);
        }
        for (n = 50; n > 1; n = (n + 1) / 2)
        {
            for (j = 0; j < n / 2; j++)
                fmpz_mul(factors + j, factors + 2 * j, factors + 2 * j + 1);
            if (n % 2)
                fmpz_set(factors + n / 2, factors + n - 1);
        }
        fmpz_add(value, factors, power);
        _fmpz_vec_clear(factors, 50);
    }
    sign = fmpz_sgn(value);
    fmpz_clear(den); fmpz_clear(power); fmpz_clear(value);
    return sign;
}

int main(int argc, char ** argv)
{
    int wilkinson = argc > 1 && !strcmp(argv[1], "wilkinson"), st, failures = 0;
    slong prec = argc > 2 ? atol(argv[2]) : 2097152, n = wilkinson ? 50 : 2, m, i, exponent;
    arb_ptr balls = _arb_vec_init(50);
    fmpz_poly_t f, linear;
    fmpz_t a, b, e, width, midpoint, bound;
    fmpz_poly_init(f); fmpz_poly_init(linear);
    fmpz_init(a); fmpz_init(b); fmpz_init(e);
    fmpz_init(width); fmpz_init(midpoint); fmpz_init(bound);
    if (wilkinson)
    {
        fmpz_poly_one(f);
        for (i = 1; i <= 50; i++)
        {
            fmpz_poly_set_coeff_si(linear, 1, 1); fmpz_poly_set_coeff_si(linear, 0, -i);
            fmpz_poly_mul(f, f, linear);
            fmpz_set_si(a, 4 * i - 1); fmpz_set_si(b, 4 * i + 1);
            failures += sign_value(a, -2, 1) * sign_value(b, -2, 1) >= 0;
        }
        fmpz_add_ui(f->coeffs, f->coeffs, 1);
    }
    else
    {
        fmpz_poly_set_coeff_si(f, 50, 1); fmpz_poly_set_coeff_si(f, 0, -2);
    }
    st = adf_roots_real_isolate_counted(balls, &m, f, prec, n);
    failures += st != ADF_OK || m != n;
    for (i = 0; st == ADF_OK && i < m; i++)
    {
        arb_get_interval_fmpz_2exp(a, b, e, balls + i); exponent = fmpz_get_si(e);
        failures += fmpz_cmp(a, b) >= 0 || sign_value(a, exponent, wilkinson) *
                    sign_value(b, exponent, wilkinson) >= 0;
        fmpz_sub(width, b, a); fmpz_mul_2exp(width, width, (ulong) prec);
        fmpz_add(midpoint, a, b); fmpz_abs(midpoint, midpoint);
        failures += fmpz_cmp(width, midpoint) > 0;
        if (wilkinson)
        {
            fmpz_one(bound); fmpz_mul_2exp(bound, bound, (ulong) (-exponent));
            fmpz_mul_ui(midpoint, bound, (ulong) (4 * (i + 1) - 1));
            fmpz_mul_2exp(width, a, 2); failures += fmpz_cmp(width, midpoint) < 0;
            fmpz_mul_ui(midpoint, bound, (ulong) (4 * (i + 1) + 1));
            fmpz_mul_2exp(width, b, 2); failures += fmpz_cmp(width, midpoint) > 0;
        }
        else
            failures += i == 0 ? fmpz_sgn(b) >= 0 : fmpz_sgn(a) <= 0;
    }
    printf("%s prec %ld status %d roots %ld independent-sign failures %d\n",
           wilkinson ? "wilkinson+1" : "X^50-2", (long) prec, st, (long) m, failures);
    fmpz_clear(a); fmpz_clear(b); fmpz_clear(e); fmpz_clear(width);
    fmpz_clear(midpoint); fmpz_clear(bound);
    _arb_vec_clear(balls, 50); fmpz_poly_clear(f); fmpz_poly_clear(linear); flint_cleanup();
    return failures != 0;
}
