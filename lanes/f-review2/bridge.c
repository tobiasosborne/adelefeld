/* Review adapter. No arithmetic oracle is implemented here. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <adelefeld.h>

static void readball(adf_lball_t x)
{
    char q[20000];
    if (scanf("%lu %d %19999s %ld %ld", &x->p, &x->exact, q, &x->v, &x->N) != 5)
        exit(2);
    if (fmpq_set_str(x->u, q, 10)) exit(3);
    fmpq_canonicalise(x->u);
}

int main(void)
{
    char op[30];
    int alias, st, b;
    slong m;
    adf_lball_t x, y, z;
    adf_rat_t r;
    adf_place_t p;
    adf_lball_ptr out;
    adf_lball_srcptr rhs;
    adf_lball_init(x); adf_lball_init(y); adf_lball_init(z); adf_rat_init(r);
    while (scanf("%29s %d", op, &alias) == 2)
    {
        readball(x); readball(y);
        z->p = 7; z->exact = 1; z->v = 0; z->N = 0; fmpq_set_si(z->u, 11, 1);
        fmpq_set_si(r->q, 13, 1); m = 987; b = 986;
        out = alias == 1 || alias == 4 ? x : alias == 2 ? y : z;
        rhs = alias == 3 || alias == 4 ? x : y;
        st = ADF_OK;
        if (!strcmp(op, "add")) st = adf_lball_add(out, x, y);
        else if (!strcmp(op, "sub")) st = adf_lball_sub(out, x, rhs);
        else if (!strcmp(op, "mul")) st = adf_lball_mul(out, x, y);
        else if (!strcmp(op, "div")) st = adf_lball_div(out, x, rhs);
        else if (!strcmp(op, "neg")) st = adf_lball_neg(out, x);
        else if (!strcmp(op, "inv")) st = adf_lball_inv(out, x);
        else if (!strcmp(op, "dec")) st = adf_lball_decompose(&m, out, x);
        else if (!strcmp(op, "val")) st = adf_lball_valuation(&m, &b, x);
        else if (!strcmp(op, "abs")) st = adf_lball_abs(r, x);
        else if (!strcmp(op, "center")) st = adf_lball_get_center(r, x);
        else if (!strcmp(op, "prec")) st = adf_lball_get_prec(&m, x);
        else if (!strcmp(op, "cz")) b = adf_lball_contains_zero(x);
        else if (!strcmp(op, "exact")) b = adf_lball_is_exact(x);
        else if (!strcmp(op, "eq")) b = adf_lball_equal_set(x, y);
        else if (!strcmp(op, "ident")) b = adf_lball_identical(x, y);
        else if (!strcmp(op, "over")) b = adf_lball_overlaps(x, y);
        else if (!strcmp(op, "inside")) b = adf_lball_contains(x, y);
        else if (!strcmp(op, "canon")) b = adf_lball_is_canonical(x);
        else if (!strcmp(op, "set")) adf_lball_set(out, x);
        else if (!strcmp(op, "project") || !strcmp(op, "projectlocal"))
        {
            adf_fball_t f, local;
            adf_modctx_struct *ctx = NULL;
            ulong blocks[3] = {y->p, (ulong)y->v, (ulong)y->N};
            slong count = y->N ? 3 : y->v ? 2 : 1;
            adf_fball_init(f); adf_fball_init(local);
            if (adf_place_prime(&p, x->p) != ADF_OK) exit(4);
            st = adf_fball_set_fmpz3(f, fmpq_numref(x->u), fmpq_numref(y->u), fmpq_denref(x->u));
            if (st == ADF_OK && !strcmp(op, "projectlocal"))
            {
                st = adf_modctx_new_blocks(&ctx, blocks, count);
                if (st == ADF_OK) st = adf_fball_set_local(local, f, ctx);
                if (st == ADF_OK) st = adf_lball_set_fball(out, p, local);
            }
            else if (st == ADF_OK) st = adf_lball_set_fball(out, p, f);
            adf_fball_clear(f); adf_fball_clear(local); adf_modctx_free(ctx);
        }
        else if (!strcmp(op, "construct"))
        {
            if (adf_place_prime(&p, x->p) != ADF_OK) exit(4);
            fmpq_set(r->q, x->u);
            st = x->exact ? adf_lball_set_rat(out, p, r)
                          : adf_lball_set_rat_ball(out, p, r, x->N);
        }
        else exit(5);
        printf("%s %lu %d ", adf_status_str(st), out->p, out->exact);
        fmpq_print(out->u);
        printf(" %ld %ld %ld %d ", out->v, out->N, m, b);
        fmpq_print(r->q);
        putchar('\n'); fflush(stdout);
    }
    adf_lball_clear(x); adf_lball_clear(y); adf_lball_clear(z); adf_rat_clear(r);
    flint_cleanup();
    return 0;
}
