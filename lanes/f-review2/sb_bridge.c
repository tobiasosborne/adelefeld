#include <adelefeld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void readsb(adf_sball_t x)
{
    int arch;
    slong m, e, re, len;
    ulong rm;
    if (scanf("%d %ld %ld %lu %ld %ld", &arch, &m, &e, &rm, &re, &len) != 6) exit(2);
    adf_sball_clear(x); adf_sball_init(x); x->arch = arch; x->len = len;
    if (arch) { arb_set_si(acb_realref(x->inf), m); arb_mul_2exp_si(acb_realref(x->inf),
        acb_realref(x->inf), e); mag_set_ui_2exp_si(arb_radref(acb_realref(x->inf)), rm, re); }
    if (len) x->loc = flint_malloc((size_t)len * sizeof(adf_lball_struct));
    for (slong i = 0; i < len; i++)
    {
        char q[20000];
        adf_lball_struct *b = x->loc+i;
        adf_lball_init(b);
        if (scanf("%lu %d %19999s %ld %ld", &b->p, &b->exact, q, &b->v, &b->N) != 5) exit(2);
        if (fmpq_set_str(b->u, q, 10)) exit(2);
    }
}

static void printsb(const adf_sball_t x)
{
    fmpq_t m, r;
    fmpq_init(m); fmpq_init(r);
    arf_get_fmpq(m, arb_midref(acb_realref(x->inf)));
    mag_get_fmpq(r, arb_radref(acb_realref(x->inf)));
    printf(" %d ", x->arch); fmpq_print(m); putchar(' '); fmpq_print(r); printf(" %ld", x->len);
    for (slong i = 0; i < x->len; i++)
    {
        const adf_lball_struct *b = x->loc+i;
        printf(" %lu %d ", b->p, b->exact); fmpq_print(b->u); printf(" %ld %ld", b->v, b->N);
    }
    fmpq_clear(m); fmpq_clear(r);
}

int main(void)
{
    char op[20];
    int alias, st, bit;
    adf_sball_t x, y, z;
    adf_place_t where, marker;
    adf_sball_init(x); adf_sball_init(y); adf_sball_init(z); adf_place_prime(&marker, 11);
    while (scanf("%19s %d", op, &alias) == 2)
    {
        adf_sball_clear(z); adf_sball_init(z); z->arch = 1; arb_set_si(acb_realref(z->inf), 13);
        where = marker; st = ADF_OK; bit = -1;
        adf_sball_ptr out = alias == 1 ? x : alias == 2 ? y : z;
        if (!strcmp(op, "project"))
        {
            char a[2000], h[2000], d[2000];
            slong m, e, re, np;
            ulong rm, prime;
            int local;
            adf_adele_t adele;
            adf_modctx_struct *ctx = NULL;
            adf_fball_t fin;
            fmpz_t A, H, D;
            ulong blocks[3] = {4,9,5};
            adf_adele_init(adele); adf_fball_init(fin); fmpz_init(A); fmpz_init(H); fmpz_init(D);
            if (scanf("%1999s %1999s %1999s %ld %ld %lu %ld %d %ld",
                      a,h,d,&m,&e,&rm,&re,&local,&np) != 9) exit(2);
            fmpz_set_str(A,a,10); fmpz_set_str(H,h,10); fmpz_set_str(D,d,10);
            if (adf_fball_set_fmpz3(fin,A,H,D) != ADF_OK) exit(3);
            if (local)
            {
                if (adf_modctx_new_blocks(&ctx,blocks,3) != ADF_OK) exit(3);
                if (adf_fball_set_local(&adele->fin,fin,ctx) != ADF_OK) exit(3);
            }
            else adf_fball_set(&adele->fin,fin);
            arb_set_si(adele->inf,m); arb_mul_2exp_si(adele->inf,adele->inf,e);
            mag_set_ui_2exp_si(arb_radref(adele->inf),rm,re);
            adf_place_t *ps = flint_malloc((size_t)(np ? np : 1)*sizeof(adf_place_t));
            for (slong i = 0; i < np; i++)
            {
                if (scanf("%lu", &prime) != 1) exit(2);
                if (!prime) ps[i] = adf_place_inf();
                else if (adf_place_prime(ps+i,prime) != ADF_OK) exit(3);
            }
            st = adf_sball_project(z,&where,adele,ps,np);
            flint_free(ps); adf_adele_clear(adele); adf_fball_clear(fin); adf_modctx_free(ctx);
            fmpz_clear(A); fmpz_clear(H); fmpz_clear(D); out = z;
        }
        else
        {
            readsb(x); readsb(y);
            if (!strcmp(op,"add")) st = adf_sball_add(out,&where,x,y,53);
            else if (!strcmp(op,"sub")) st = adf_sball_sub(out,&where,x,y,53);
            else if (!strcmp(op,"mul")) st = adf_sball_mul(out,&where,x,y,53);
            else if (!strcmp(op,"neg")) st = adf_sball_neg(out,&where,x);
            else if (!strcmp(op,"selfsub")) st = adf_sball_sub(out,&where,x,x,53);
            else if (!strcmp(op,"eq")) bit = adf_sball_equal_set(x,y);
            else if (!strcmp(op,"inside")) bit = adf_sball_contains(x,y);
            else if (!strcmp(op,"over")) bit = adf_sball_overlaps(x,y);
            else if (!strcmp(op,"canon")) bit = adf_sball_is_canonical(x);
            else exit(4);
        }
        printf("%s %lu %d %d",adf_status_str(st),adf_place_prime_get(where),bit,adf_sball_is_canonical(out));
        printsb(out); putchar('\n'); fflush(stdout);
    }
    adf_sball_clear(x); adf_sball_clear(y); adf_sball_clear(z); flint_cleanup(); return 0;
}
