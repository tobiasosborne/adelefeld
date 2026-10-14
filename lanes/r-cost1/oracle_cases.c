/* Run benchmark cases through the test's independent rational Sturm oracle. */
#define ADF_TEST_NO_MAIN
#include "../../tests/test_roots_real_isolate.c"
#define main benchmark_main
#define now benchmark_now
#define sign_at benchmark_sign_at
#define family benchmark_family
#include "../../bench/bench_roots_real.c"
#undef sign_at
#undef family
#undef now
#undef main

int main(int argc, char ** argv)
{
    fmpz_poly_t f;
    adf_rootlist_t L;
    slong prec;
    int st, truth;
    if (argc != 5)
        return 2;
    prec = atol(argv[4]);
    fmpz_poly_init(f); adf_rootlist_init(L);
    if (!benchmark_family(f, argv[1], strtoul(argv[2], NULL, 10), atol(argv[3])))
        return 2;
    st = adf_roots_real(L, f, prec);
    truth = st == ADF_OK && balls_true(L->g, L->ball, L->n, prec, argv[1]);
    printf("%s prec %ld status %d roots %ld oracle %d checks %lu failures %lu\n",
           argv[1], (long) prec, st, (long) L->n, truth, adf_test_checks, adf_test_failures);
    adf_rootlist_clear(L); fmpz_poly_clear(f); flint_cleanup();
    return !truth || adf_test_failures != 0;
}
