/* Allocator API: refs/src/flint-3.0.1/memory.rst:16-21. */
#include <adelefeld.h>
#include <stdio.h>
static void *(*om)(size_t), *(*oc)(size_t, size_t), *(*orr)(void *, size_t);
static void (*of)(void *);
static size_t calls, bytes;
static int tracking;
static void *am(size_t n) { if (tracking) { calls++; bytes += n; } return om(n); }
static void *ac(size_t n, size_t s) { if (tracking) { calls++; bytes += n*s; } return oc(n,s); }
static void *ar(void *p, size_t n) { if (tracking) { calls++; bytes += n; } return orr(p,n); }
static void af(void *p) { of(p); }
int main(void)
{
    adf_lball_t l;
    adf_sball_t x, y;
    arb_t a;
    adf_place_t where;
    adf_lball_init(l); adf_sball_init(x); adf_sball_init(y); arb_init(a); arb_one(a);
    l->p=5; l->exact=1;
    fmpz_one(fmpq_numref(l->u)); fmpz_mul_2exp(fmpq_numref(l->u), fmpq_numref(l->u), 4096);
    fmpz_add_ui(fmpq_numref(l->u), fmpq_numref(l->u), 3);
    fmpz_one(fmpq_denref(l->u)); fmpz_mul_2exp(fmpq_denref(l->u), fmpq_denref(l->u), 2048);
    fmpz_add_ui(fmpq_denref(l->u), fmpq_denref(l->u), 7); fmpq_canonicalise(l->u);
    if (adf_sball_set_arb_lballs(x, NULL, a, l, 1) != ADF_OK) return 2;
    printf("canonical=%d\n", adf_sball_is_canonical(x));
    __flint_get_memory_functions(&om, &oc, &orr, &of);
    __flint_set_memory_functions(am, ac, ar, af);
    const char *names[] = {"exp_at", "log_at", "Log_at", "sin_at", "cos_at", "sqrt_at", "root_at",
                           "add", "sub", "mul"};
    for (int i=0; i<10; i++)
    {
        flint_cleanup(); calls=bytes=0; where=adf_place_inf(); tracking=1;
        int st;
        slong p=ADF_REAL_PREC_MAX+1;
        if (i==0) st=adf_sball_exp_at(y, &where, x, adf_place_inf(), p);
        else if (i==1) st=adf_sball_log_at(y, &where, x, adf_place_inf(), p);
        else if (i==2) st=adf_sball_Log_at(y, &where, x, adf_place_inf(), p);
        else if (i==3) st=adf_sball_sin_at(y, &where, x, adf_place_inf(), p);
        else if (i==4) st=adf_sball_cos_at(y, &where, x, adf_place_inf(), p);
        else if (i==5) st=adf_sball_sqrt_at(y, &where, x, adf_place_inf(), p);
        else if (i==6) st=adf_sball_root_at(y, &where, x, adf_place_inf(), 3, p);
        else if (i==7) st=adf_sball_add(y, &where, x, x, p);
        else if (i==8) st=adf_sball_sub(y, &where, x, x, p);
        else st=adf_sball_mul(y, &where, x, x, p);
        tracking=0;
        printf("function=%s status=%s calls=%zu bytes=%zu where_inf=%d output_empty=%d\n", names[i],
               adf_status_str(st), calls, bytes, adf_place_is_archimedean(where), y->arch==0 && y->len==0);
    }
    adf_lball_clear(l); adf_sball_clear(x); adf_sball_clear(y); arb_clear(a); flint_cleanup();
    __flint_set_memory_functions(om, oc, orr, of); return 0;
}
