#include <adelefeld.h>
#include <stdio.h>
#include <stdlib.h>
int __real_adf_roots_real(adf_rootlist_t, const fmpz_poly_t, slong);
int __real_adf_roots_real_isolate(arb_ptr, slong *, const fmpz_poly_t, slong);
static void save(int st, slong prec, slong n, arb_srcptr b)
{
    FILE *f = fopen(getenv("BALL_CAPTURE"), "a");
    fprintf(f, "%d %ld %ld\n", st, prec, st == ADF_OK ? n : 0);
    for (slong i = 0; st == ADF_OK && i < n; i++) {
        char *s = arb_dump_str(b + i); fprintf(f, "%s\n", s); flint_free(s);
    }
    fclose(f);
}
int __wrap_adf_roots_real(adf_rootlist_t L, const fmpz_poly_t f, slong p)
{
    int st = __real_adf_roots_real(L, f, p); save(st, p, L->n, L->ball); return st;
}
int __wrap_adf_roots_real_isolate(arb_ptr b, slong *m, const fmpz_poly_t g, slong p)
{
    int st = __real_adf_roots_real_isolate(b, m, g, p); save(st, p, *m, b); return st;
}

int __real_adf_roots_real_isolate_counted(arb_ptr, slong *, const fmpz_poly_t, slong, slong);
int __wrap_adf_roots_real_isolate_counted(arb_ptr b, slong *m, const fmpz_poly_t g, slong p, slong n)
{
    int st = __real_adf_roots_real_isolate_counted(b, m, g, p, n); save(st, p, *m, b); return st;
}
