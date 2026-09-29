#include <adelefeld.h>
#include <stdio.h>

static void * (*old_malloc)(size_t);
static void * (*old_calloc)(size_t, size_t);
static void * (*old_realloc)(void *, size_t);
static void (*old_free)(void *);
static unsigned long calls;

static void * counted_malloc(size_t n) { calls++; return old_malloc(n); }
static void * counted_calloc(size_t a, size_t b) { calls++; return old_calloc(a, b); }
static void * counted_realloc(void * p, size_t n) { calls++; return old_realloc(p, n); }

int main(void)
{
    fmpz_poly_t f;
    fmpz_t coeff, seed;
    adf_rootlist_t L;
    adf_place_t p;
    const ulong m = ADF_ROOTS_BITS_MAX / 4;
    int status;

    fmpz_poly_init(f); fmpz_init(coeff); fmpz_init(seed); adf_rootlist_init(L);
    if (adf_place_prime(&p, 2) != ADF_OK) return 2;
    fmpz_one(coeff);
    fmpz_mul_2exp(coeff, coeff, m);
    fmpz_neg(coeff, coeff);
    fmpz_poly_set_coeff_fmpz(f, 1, coeff);
    fmpz_poly_set_coeff_si(f, 2, 1); /* X(X - 2^m), root at seed 0, s = m */
    __flint_get_memory_functions(&old_malloc, &old_calloc, &old_realloc, &old_free);
    calls = 0;
    __flint_set_memory_functions(counted_malloc, counted_calloc, counted_realloc, old_free);
    status = adf_root_padic_from_seed(L, f, p, seed, 1);
    __flint_set_memory_functions(old_malloc, old_calloc, old_realloc, old_free);
    printf("m=%lu status=%d FLINT_allocations_before_return=%lu L_unchanged=%d\n",
           m, status, calls, fmpz_poly_is_one(L->g) && L->n == 0);
    adf_rootlist_clear(L); fmpz_poly_clear(f); fmpz_clear(coeff); fmpz_clear(seed);
    return status == ADF_LIMIT && calls > 0 ? 0 : 1;
}
