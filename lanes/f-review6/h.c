/* f-review6 harness: reads raw lball fields and a request per line, prints the library's answer.
   line: MODE p unum uden v N exact n seed Nreq cap
   MODE: C (count), S (root_seed), Q (sqrt_seed), A (roots), T (_at seed via sball), U (_at roots) */
#include <stdio.h>
#include <string.h>
#include <adelefeld.h>
static void sent(adf_lball_t y){ y->p=3; fmpq_set_si(y->u,7,1); y->v=0; y->N=0; y->exact=1; }
static int is_sent(const adf_lball_t y){ adf_lball_t s; int r; adf_lball_init(s); sent(s); r=adf_lball_identical(s,y); adf_lball_clear(s); return r; }
static void pr(const adf_lball_t y)
{
    printf(" %d %ld %ld ", y->exact, (long)y->v, (long)y->N);
    fmpq_print(y->u);
}
int main(void)
{
    char mode[8], unum[200000], uden[200000];
    unsigned long p, n, seed; long v, N, Nreq, cap; int exact;
    while (scanf("%7s %lu %199999s %199999s %ld %ld %d %lu %lu %ld %ld", mode, &p, unum, uden, &v, &N, &exact,
                 &n, &seed, &Nreq, &cap) == 11)
    {
        adf_lball_t x; adf_lball_init(x);
        x->p = p; fmpz_set_str(fmpq_numref(x->u), unum, 10); fmpz_set_str(fmpq_denref(x->u), uden, 10);
        x->v = v; x->N = N; x->exact = exact;
        if (!adf_lball_is_canonical(x)) { printf("NONCANON\n"); adf_lball_clear(x); fflush(stdout); continue; }
        if (mode[0] == 'C') {
            ulong c = 777; int st = adf_lball_root_count(&c, x, n);
            printf("%s %lu\n", adf_status_str(st), (unsigned long)c);
        } else if (mode[0] == 'S' || mode[0] == 'Q') {
            adf_lball_t y; adf_lball_init(y); sent(y);
            int st = mode[0]=='S' ? adf_lball_root_seed(y, x, n, seed, Nreq) : adf_lball_sqrt_seed(y, x, seed, Nreq);
            printf("%s", adf_status_str(st)); if (st) printf(is_sent(y)?" UNTOUCHED":" TOUCHED"); else pr(y); printf("\n");
            adf_lball_clear(y);
        } else if (mode[0] == 'A') {
            slong c = cap < 0 ? 0 : cap, len = -7, i; adf_lball_ptr y = flint_malloc((c+1)*sizeof(adf_lball_struct));
            ulong *ids = flint_malloc((c+1)*sizeof(ulong));
            for (i = 0; i < c+1; i++) { adf_lball_init(y+i); sent(y+i); ids[i] = 999; }
            int st = adf_lball_roots(y, ids, &len, cap, x, n, Nreq);
            printf("%s %ld", adf_status_str(st), (long)len);
            if (st == 0) for (i = 0; i < len; i++) { printf(" | %lu", (unsigned long)ids[i]); pr(y+i); }
            { int bad=0; for (i = (st==0?len:0); i < c+1; i++) if (!is_sent(y+i) || ids[i]!=999) bad=1;
              if (st!=0 && len!=-7) bad=1; if (bad) printf(" TOUCHED"); }
            printf("\n");
            for (i = 0; i < c+1; i++) adf_lball_clear(y+i);
            flint_free(y); flint_free(ids);
        }
        fflush(stdout);
        adf_lball_clear(x);
    }
    return 0;
}
