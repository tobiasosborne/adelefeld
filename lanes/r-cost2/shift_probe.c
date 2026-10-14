/* Measurement only: the exact degree-50 cluster mean and Taylor shift. */
#define main count_bench_main
#include "../../bench/bench_roots_real.c"
#undef main

int
main(void)
{
    fmpz_poly_t g, h;
    fmpq_t center;
    fmpz_t power;
    slong i, run, d = 50;
    double start, shift_start, shift_seconds, transform_seconds;
    fmpz_poly_init(g); fmpz_poly_init(h);
    fmpq_init(center); fmpz_init(power);
    family(g, "cluster", 190, d);
    fmpz_neg(fmpq_numref(center), g->coeffs + d - 1);
    fmpz_mul_ui(fmpq_denref(center), g->coeffs + d, (ulong) d);
    fmpq_canonicalise(center);
    printf("mean bits %lu\n", fmpz_bits(fmpq_numref(center)) + fmpz_bits(fmpq_denref(center)));
    for (run = 0; run < 5; run++)
    {
        start = now();
        fmpz_poly_set(h, g); fmpz_one(power);
        for (i = d - 1; i >= 0; i--)
        {
            fmpz_mul(power, power, fmpq_denref(center));
            fmpz_mul(h->coeffs + i, h->coeffs + i, power);
        }
        shift_start = now();
        _fmpz_poly_taylor_shift(h->coeffs, fmpq_numref(center), d + 1);
        shift_seconds = now() - shift_start;
        fmpz_poly_primitive_part(h, h);
        transform_seconds = now() - start;
        printf("run %ld shift %.9f s, rational transform %.9f s, height %ld\n", (long) run,
               shift_seconds, transform_seconds, (long) FLINT_ABS(fmpz_poly_max_bits(h)));
    }
    fmpz_clear(power); fmpq_clear(center); fmpz_poly_clear(g); fmpz_poly_clear(h);
    flint_cleanup();
    return 0;
}
