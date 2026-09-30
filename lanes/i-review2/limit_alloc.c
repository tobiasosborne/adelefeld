/* FLINT allocator instrumentation: refs/src/flint-3.0.1/memory.rst:16-21. */
#include <stdio.h>
#include <adelefeld.h>

static void *(*original_malloc)(size_t);
static void *(*original_realloc)(void *, size_t);
static void *(*original_calloc)(size_t, size_t);
static void (*original_free)(void *);
static size_t allocations, bytes;
static int tracking;
static void *am(size_t n) { if (tracking) { allocations++; bytes += n; } return original_malloc(n); }
static void *ar(void *p, size_t n) { if (tracking) { allocations++; bytes += n; } return original_realloc(p, n); }
static void *ac(size_t n, size_t s) { if (tracking) { allocations++; bytes += n * s; }
    return original_calloc(n, s); }
static void af(void *p) { original_free(p); }

int main(void)
{
    adf_idele_t x, y;
    adf_idclass_t cl;
    arb_t t;
    int st;
    adf_idele_init(x); adf_idele_init(y); adf_idclass_init(cl); arb_init(t);
    fmpz_one(fmpq_numref(x->r)); fmpz_mul_2exp(fmpq_numref(x->r), fmpq_numref(x->r), 4096);
    fmpz_add_ui(fmpq_numref(x->r), fmpq_numref(x->r), 3);
    fmpz_one(fmpq_denref(x->r)); fmpz_mul_2exp(fmpq_denref(x->r), fmpq_denref(x->r), 2048);
    fmpz_add_ui(fmpq_denref(x->r), fmpq_denref(x->r), 7); fmpq_canonicalise(x->r);
    fmpz_one(x->u.c); fmpz_one(x->u.N); fmpz_mul_2exp(x->u.N, x->u.N, 2048);
    fmpz_add_ui(x->u.N, x->u.N, 1);
    if (!adf_idele_is_canonical(x)) return 2;
    __flint_get_memory_functions(&original_malloc, &original_calloc, &original_realloc, &original_free);
    __flint_set_memory_functions(am, ac, ar, af);
    for (int i = 0; i < 4; i++)
    {
        flint_cleanup(); allocations = bytes = 0; tracking = 1;
        if (i == 0) st = adf_idele_norm(t, x, ADF_IDELE_PREC_MAX + 1);
        else if (i == 1) st = adf_idclass_set_idele(cl, x, ADF_IDELE_PREC_MAX + 1);
        else if (i == 2) st = adf_idele_pow(y, x, 0, ADF_IDELE_PREC_MAX + 1);
        else st = adf_idele_pow_tight(y, x, 0, ADF_IDELE_PREC_MAX + 1);
        tracking = 0;
        printf("function=%d status=%s allocator_calls=%zu requested_bytes=%zu\n",
               i, adf_status_str(st), allocations, bytes);
    }
    adf_idele_clear(x); adf_idele_clear(y); adf_idclass_clear(cl); arb_clear(t); flint_cleanup();
    __flint_set_memory_functions(original_malloc, original_calloc, original_realloc, original_free);
    return 0;
}
