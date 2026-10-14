/* Exact comparison with the saved pre-change isolation/refinement code. */
#define main benchmark_main
#include "../../bench/bench_roots_real.c"
#undef main

int adf_roots_real_isolate_counted(arb_ptr b, slong * m, const fmpz_poly_t g, slong prec, slong count);
int adf_roots_real_isolate_counted_before(arb_ptr b, slong * m, const fmpz_poly_t g,
                                        slong prec, slong count);

int main(void)
{
    static const struct { const char * name; ulong e; slong d, count; } cases[] = {
        { "xn2", 0, 50, 2 }, { "wilkinson", 0, 50, 50 }, { "cluster", 190, 50, 50 },
        { "scaled", 150, 50, 50 }, { "tiny", 150, 50, 50 }, { "mignotte", 100, 20, 4 },
        { "thirds", 3000, 2, 2 }, { "deg", 190, 50, 50 }, { "small", 0, 50, 50 }
    };
    static const slong precisions[] = { 2, 53, 1000, 4096 };
    fmpz_poly_t g;
    arb_ptr before = _arb_vec_init(50), after = _arb_vec_init(50);
    slong i, j, k, n_before, n_after, runs = 0, balls = 0, differences = 0;
    int st_before, st_after;
    fmpz_poly_init(g);
    for (i = 0; i < (slong) (sizeof(cases) / sizeof(cases[0])); i++)
    {
        family(g, cases[i].name, cases[i].e, cases[i].d);
        for (j = 0; j < 4; j++)
        {
            st_before = adf_roots_real_isolate_counted_before(before, &n_before, g,
                                                              precisions[j], cases[i].count);
            st_after = adf_roots_real_isolate_counted(after, &n_after, g,
                                                       precisions[j], cases[i].count);
            runs++;
            if (st_before != st_after || st_before != ADF_OK || n_before != n_after)
                differences++;
            else
                for (k = 0; k < n_after; k++)
                {
                    balls++;
                    differences += !arb_equal(before + k, after + k);
                }
        }
    }
    printf("%ld runs, %ld balls, %ld differences\n", (long) runs, (long) balls, (long) differences);
    fmpz_poly_clear(g); _arb_vec_clear(before, 50); _arb_vec_clear(after, 50); flint_cleanup();
    return differences != 0;
}
