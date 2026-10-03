#include <adelefeld.h>
#include <flint/ulong_extras.h>
#include <stdio.h>
#include <time.h>

static double now(void) {
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}
int main(void) {
    adf_adele_t x, y; adf_place_t w; fmpz_t A;
    adf_adele_init(x); adf_adele_init(y); fmpz_init(A); fmpz_set_ui(A, 4);
    /* Independent sieve: choose as many initial odd primes as fit below 100000 bits. */
    unsigned char composite[200001] = {0};
    for (unsigned long d = 2; d*d <= 200000; d++)
        if (!composite[d]) for (unsigned long k = d*d; k <= 200000; k += d) composite[k] = 1;
    unsigned long count = 0, expected = 0;
    for (unsigned long p = 3; p <= 200000; p += 2) {
        if (composite[p]) continue;
        fmpz_mul_ui(A, A, p);
        if (fmpz_bits(A) > 100000) { fmpz_divexact_ui(A, A, p); expected = p; break; }
        count++;
    }
    fmpz_mul_2exp(A, A, 100000 - fmpz_bits(A));
    fmpz_set(x->fin.A, A); arb_zero(x->inf);
    double start = now(); int st = adf_adele_exp(y, &w, x, 64); double elapsed = now() - start;
    printf("numerator_bits=%lu odd_primes_dividing=%lu expected_prime=%lu status=%s reported_prime=%lu seconds=%.9f\n",
           fmpz_bits(A), count, expected, adf_status_str(st), adf_place_prime_get(w), elapsed);
    int fail = st != ADF_DOMAIN || adf_place_prime_get(w) != expected;
    fmpz_clear(A); adf_adele_clear(x); adf_adele_clear(y); flint_cleanup(); return fail;
}
