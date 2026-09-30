#include <adelefeld.h>
#include <stdio.h>

static void put(const adf_lball_t x)
{
    printf(" %lu %d %ld %ld ", x->p, x->exact, x->v, x->N);
    fmpq_print(x->u);
}

int main(void)
{
    char op, q[4096];
    ulong p, r;
    slong v, N, arg, m;
    int exact, alias, st;
    adf_lball_t x, y, w, repeated;
    adf_rat_t frac;
    fmpz_t out;
    adf_lball_init(x); adf_lball_init(y); adf_lball_init(w); adf_lball_init(repeated);
    adf_rat_init(frac); fmpz_init(out);
    while (scanf(" %c %lu %d %ld %ld %4095s %ld %d", &op, &p, &exact, &v, &N, q, &arg, &alias) == 8)
    {
        x->p = p; x->exact = exact; x->v = v; x->N = N;
        if (fmpq_set_str(x->u, q, 10)) return 3;
        fmpq_canonicalise(x->u);
        if (!adf_lball_is_canonical(x)) return 4;
        adf_lball_init(y); adf_lball_init(w);
        m = 777; r = 777;
        adf_place_t place;
        adf_place_prime(&place, p);
        if (op == 'T')
        {
            st = adf_lball_teichmuller(y, place, fmpz_get_ui(fmpq_numref(x->u)), arg);
            printf("%d", st); put(y);
        }
        else if (op == 'S')
        {
            st = adf_lball_decompose_teich(&m, alias == 1 ? x : w, &r, alias == 2 ? x : y, x, arg);
            printf("%d %ld %lu", st, m, r);
            put(alias == 1 ? x : w); put(alias == 2 ? x : y);
        }
        else if (op == 'P')
        {
            st = adf_lball_pow_si(alias ? x : y, x, arg);
            printf("%d", st); put(alias ? x : y);
        }
        else if (op == 'F')
        {
            fmpq_set_si(frac->q, 17, 1);
            st = adf_lball_frac(frac, x);
            printf("%d ", st); fmpq_print(frac->q);
        }
        else if (op == 'U')
        {
            fmpz_set_si(out, 17);
            st = adf_lball_unit_mod(out, x, arg);
            printf("%d ", st); fmpz_print(out);
        }
        else if (op == 'R')
        {
            adf_lball_t factor;
            adf_lball_init(factor);
            st = arg < 0 ? adf_lball_inv(factor, x) : (adf_lball_set(factor, x), ADF_OK);
            adf_lball_set_rat(repeated, place, frac);
            fmpq_one(repeated->u); repeated->v = 0; repeated->exact = 1;
            for (slong i = 0; i < (arg < 0 ? -arg : arg) && st == ADF_OK; i++)
                st = adf_lball_mul(repeated, repeated, factor);
            printf("%d", st); put(repeated);
            adf_lball_clear(factor);
        }
        else return 5;
        putchar('\n'); fflush(stdout);
        adf_lball_clear(y); adf_lball_clear(w);
    }
    adf_lball_clear(x); adf_lball_clear(repeated); adf_rat_clear(frac); fmpz_clear(out);
    flint_cleanup(); return 0;
}
