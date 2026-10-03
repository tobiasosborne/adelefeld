#include <stdio.h>
#include <time.h>
#include <adelefeld.h>

int main(void)
{
    const ulong p = UWORD(18446744073709551557), n = 824329;
    adf_lball_t x;
    adf_lball_ptr rs = flint_malloc(n*sizeof(adf_lball_struct));
    ulong *ids = flint_malloc(n*sizeof(ulong));
    slong len = -1;
    adf_lball_init(x);
    x->p = p; x->v = 0; x->N = 0; x->exact = 1; fmpq_one(x->u);
    for (ulong i = 0; i < n; i++) adf_lball_init(rs+i);
    const slong Ns[3] = { ADF_LBALL_EXP_MAX+1, 0, 40 };
    for (int i = 0; i < 3; i++)
    {
        clock_t start = clock();
        int st = adf_lball_roots(rs, ids, &len, (slong)n, x, n, Ns[i]);
        double cpu = (double)(clock()-start)/CLOCKS_PER_SEC;
        printf("n=%lu N=%ld status=%d len=%ld CPU=%.6f\n", n, Ns[i], st, len, cpu);
        if (st != (i == 0 ? ADF_LIMIT : ADF_OK)) return 1;
    }
    for (ulong i = 0; i < n; i++) adf_lball_clear(rs+i);
    flint_free(rs); flint_free(ids); adf_lball_clear(x);
    flint_cleanup();
    return 0;
}
