/* Trace the sole trusted count call without changing production source or headers. */
#define main count_bench_main
#include "../../bench/bench_roots_real.c"
#undef main

static fmpz_poly_t expected;
static slong calls;
slong __real_fmpz_poly_num_real_roots(const fmpz_poly_t g);

slong
__wrap_fmpz_poly_num_real_roots(const fmpz_poly_t g)
{
    calls++;
    printf("count input height %ld, degree %ld, original normalized input %d\n",
           (long) FLINT_ABS(fmpz_poly_max_bits(g)), (long) fmpz_poly_degree(g),
           fmpz_poly_equal(expected, g));
    return __real_fmpz_poly_num_real_roots(g);
}

int
main(int argc, char ** argv)
{
    fmpz_poly_t g;
    adf_rootlist_t L;
    int st, ok;
    if (argc != 4)
        return 2;
    fmpz_poly_init(g); fmpz_poly_init(expected); adf_rootlist_init(L);
    if (!family(g, argv[1], strtoul(argv[2], NULL, 10), strtol(argv[3], NULL, 10)))
        return 2;
    fmpz_poly_primitive_part(expected, g);
    printf("original normalized height %ld\n", (long) FLINT_ABS(fmpz_poly_max_bits(expected)));
    st = adf_roots_real(L, g, 2);
    printf("status %d, calls %ld, roots %ld, complete %d\n", st, (long) calls, (long) L->n, L->complete);
    ok = st == ADF_OK && calls == 1 && L->complete == 1 && L->n == L->count;
    ok = ok && L->n == (!strcmp(argv[1], "random") ? 12 : fmpz_poly_degree(g));
    adf_rootlist_clear(L); fmpz_poly_clear(g); fmpz_poly_clear(expected); flint_cleanup();
    return !ok;
}
