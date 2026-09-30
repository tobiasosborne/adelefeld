#include <adelefeld.h>
#include <stdio.h>
int main(void)
{
    char op, q[4096]; ulong p; int exact, mode; slong v, N, arg;
    adf_lball_t locals[2], expected;
    adf_sball_t x, y, saved;
    arb_t a;
    adf_lball_init(locals[0]); adf_lball_init(locals[1]); adf_lball_init(expected);
    adf_sball_init(x); adf_sball_init(y); adf_sball_init(saved); arb_init(a); arb_set_si(a, 7);
    while (scanf(" %c %lu %d %ld %ld %4095s %ld %d", &op, &p, &exact, &v, &N, q, &arg, &mode) == 8)
    {
        adf_lball_ptr l = locals[0];
        l->p=p; l->exact=exact; l->v=v; l->N=N;
        if (fmpq_set_str(l->u, q, 10)) return 2;
        fmpq_canonicalise(l->u);
        locals[1]->p = p==2 ? 3 : 2; fmpq_one(locals[1]->u);
        locals[1]->v=0; locals[1]->N=0; locals[1]->exact=1;
        if (adf_sball_set_arb_lballs(x, NULL, a, locals[0], 2) != ADF_OK) return 3;
        if (mode & 2) x->arch=ADF_ARCH_COMPLEX;
        adf_sball_set(saved, x);
        adf_sball_clear(y); adf_sball_init(y);
        adf_place_t place, where, sentinel;
        adf_place_prime(&place, (mode & 4) ? 13 : p); adf_place_prime(&sentinel, 17); where=sentinel;
        adf_sball_ptr out = (mode & 1) ? x : y;
        int local = op=='E' ? adf_lball_exp(expected,l,arg) : op=='L' ? adf_lball_log(expected,l,arg)
            : op=='G' ? adf_lball_Log(expected,l,arg) : ADF_UNSUPPORTED;
        int st = op=='E' ? adf_sball_exp_at(out,&where,x,place,arg)
            : op=='L' ? adf_sball_log_at(out,&where,x,place,arg)
            : op=='G' ? adf_sball_Log_at(out,&where,x,place,arg)
            : adf_sball_sin_at(out,&where,x,place,arg);
        int unchanged = st==ADF_OK ? 1 : (mode & 1) ? adf_sball_identical(x,saved) : y->len==0 && y->arch==0;
        int transport = st==ADF_OK ? out->arch==0 && out->len==1 && adf_lball_identical(out->loc,expected)
            : st==((mode & 4) ? ADF_DOMAIN : local);
        printf("%d %lu %d %d %d", st, adf_place_is_archimedean(where) ? 0 : adf_place_prime_get(where),
               unchanged, transport, adf_sball_is_canonical(out));
        if (st==ADF_OK)
        {
            l=out->loc;
            printf(" %lu %d %ld %ld ", l->p,l->exact,l->v,l->N); fmpq_print(l->u);
        }
        putchar('\n'); fflush(stdout);
    }
    adf_lball_clear(locals[0]); adf_lball_clear(locals[1]); adf_lball_clear(expected);
    adf_sball_clear(x); adf_sball_clear(y); adf_sball_clear(saved); arb_clear(a); flint_cleanup(); return 0;
}
