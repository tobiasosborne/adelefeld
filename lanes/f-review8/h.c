/* f-review7 harness: raw lball fields per line, prints the library's answer.
   P p un ud v N ex  e n seed Nreq        powrat (also y = x aliased, compared)
   W p un ud v N ex  sn sd sv sN sex Nreq  powunit (also y = u, y = s; u = s when the two inputs are equal)
   I p un ud v N ex  e                    pow_si
   S p un ud v N ex  n seed Nreq          root_seed
   A p un ud v N ex  n Nreq cap           roots
   Output: STATUS [exact v N num/den] or STATUS UNTOUCHED|TOUCHED; ALIASDIFF if an aliased call differs. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <adelefeld.h>
#include <time.h>
static double now(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+1e-9*t.tv_nsec; }
static char b1[400000], b2[400000];
static void sent(adf_lball_t y){ y->p=3; fmpq_set_si(y->u,7,1); y->v=0; y->N=0; y->exact=1; }
static int is_sent(const adf_lball_t y){ adf_lball_t s; int r; adf_lball_init(s); sent(s); r=adf_lball_identical(s,y); adf_lball_clear(s); return r; }
static void pr(const adf_lball_t y)
{
    printf(" %d %ld %ld ", y->exact, (long)y->v, (long)y->N);
    fmpq_print(y->u);
}
static int rd(adf_lball_t x, unsigned long p)
{
    long v, N; int ex;
    if (scanf("%399999s %399999s %ld %ld %d", b1, b2, &v, &N, &ex) != 5) return 0;
    x->p = p; fmpz_set_str(fmpq_numref(x->u), b1, 10); fmpz_set_str(fmpq_denref(x->u), b2, 10);
    x->v = v; x->N = N; x->exact = ex;
    return 1;
}
static void out(int st, const adf_lball_t y)
{
    printf("%s", adf_status_str(st));
    if (st) printf(is_sent(y) ? " UNTOUCHED" : " TOUCHED"); else pr(y);
}
int main(void)
{
    char mode[8]; unsigned long p; long lineno = 0; int tm = getenv("HTIME") != NULL; double t0 = 0;
    while (scanf("%7s %lu", mode, &p) == 2)
    {
        lineno++; if (tm) t0 = now();
        adf_lball_t x, s, y, z; adf_lball_init(x); adf_lball_init(s); adf_lball_init(y); adf_lball_init(z);
        if (!rd(x, p)) break;
        if (!adf_lball_is_canonical(x)) { printf("NONCANON\n"); fflush(stdout); goto next; }
        if (mode[0] == 'P') {
            long e, Nreq; unsigned long n, seed;
            if (scanf("%ld %lu %lu %ld", &e, &n, &seed, &Nreq) != 4) break;
            sent(y); int st = adf_lball_powrat(y, x, e, n, seed, Nreq); out(st, y);
            adf_lball_set(z, x); int st2 = adf_lball_powrat(z, z, e, n, seed, Nreq);
            if (st2 != st || (st == 0 && !adf_lball_identical(z, y)) || (st && !adf_lball_identical(z, x))) printf(" ALIASDIFF");
        } else if (mode[0] == 'W') {
            long Nreq; adf_lball_t w; unsigned long ps = p;
            if (!rd(s, ps)) break;
            if (scanf("%ld", &Nreq) != 1) break;
            if (!adf_lball_is_canonical(s)) { printf("NONCANON\n"); fflush(stdout); goto next; }
            sent(y); int st = adf_lball_powunit(y, x, s, Nreq); out(st, y);
            adf_lball_init(w);
            adf_lball_set(z, x); int st2 = adf_lball_powunit(z, z, s, Nreq);
            if (st2 != st || (st == 0 && !adf_lball_identical(z, y)) || (st && !adf_lball_identical(z, x))) printf(" ALIASDIFF_U");
            adf_lball_set(w, s); st2 = adf_lball_powunit(w, x, w, Nreq);
            if (st2 != st || (st == 0 && !adf_lball_identical(w, y)) || (st && !adf_lball_identical(w, s))) printf(" ALIASDIFF_S");
            if (adf_lball_identical(x, s)) {
                adf_lball_set(w, x); st2 = adf_lball_powunit(w, w, w, Nreq);
                if (st2 != st || (st == 0 && !adf_lball_identical(w, y)) || (st && !adf_lball_identical(w, x))) printf(" ALIASDIFF_US");
            }
            adf_lball_clear(w);
        } else if (mode[0] == 'I') {
            long e; if (scanf("%ld", &e) != 1) break;
            sent(y); int st = adf_lball_pow_si(y, x, e); out(st, y);
        } else if (mode[0] == 'S') {
            unsigned long n, seed; long Nreq;
            if (scanf("%lu %lu %ld", &n, &seed, &Nreq) != 3) break;
            sent(y); int st = adf_lball_root_seed(y, x, n, seed, Nreq); out(st, y);
        } else if (mode[0] == 'A') {
            unsigned long n; long Nreq, cap;
            if (scanf("%lu %ld %ld", &n, &Nreq, &cap) != 3) break;
            slong c = cap < 0 ? 0 : cap, len = -7, i; adf_lball_ptr ys = flint_malloc((c+1)*sizeof(adf_lball_struct));
            ulong *ids = flint_malloc((c+1)*sizeof(ulong));
            for (i = 0; i < c+1; i++) { adf_lball_init(ys+i); sent(ys+i); ids[i] = 999; }
            int st = adf_lball_roots(ys, ids, &len, cap, x, n, Nreq);
            printf("%s %ld", adf_status_str(st), (long)len);
            if (st == 0) for (i = 0; i < len; i++) { printf(" | %lu", (unsigned long)ids[i]); pr(ys+i); }
            { int bad=0; for (i = (st==0?len:0); i < c+1; i++) if (!is_sent(ys+i) || ids[i]!=999) bad=1;
              if (st!=0 && len!=-7) bad=1; if (bad) printf(" TOUCHED"); }
            for (i = 0; i < c+1; i++) adf_lball_clear(ys+i);
            flint_free(ys); flint_free(ids);
        }
        printf("\n"); fflush(stdout);
        if (tm) fprintf(stderr, "line %ld %c %.3f s\n", lineno, mode[0], now() - t0);
    next:
        adf_lball_clear(x); adf_lball_clear(s); adf_lball_clear(y); adf_lball_clear(z);
    }
    return 0;
}
