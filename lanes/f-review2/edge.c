#include <adelefeld.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void raw(adf_lball_t x, int e, slong v, slong N)
{
    x->p = 2; x->exact = e; x->v = v; x->N = N; fmpq_one(x->u);
}

int main(int argc, char **argv)
{
    if (argc < 2) return 2;
    if (!strcmp(argv[1], "limits"))
    {
        adf_lball_t x, y, z;
        fmpz_t power;
        slong k = ADF_LBALL_BITS_MAX / 2 + 1;
        adf_lball_init(x); adf_lball_init(y); adf_lball_init(z); fmpz_init(power);
        fmpz_one(power); fmpz_mul_2exp(power, power, (ulong)k);
        raw(x, 0, 0, k+1); raw(y, 1, k, 0);
        fmpz_add_ui(fmpq_numref(x->u), power, 1);
        adf_rat_t q;
        adf_rat_init(q); fmpq_set(q->q,x->u);
        adf_place_t p2; adf_place_prime(&p2,2);
        printf("sub_ball_constructor=%s\n", adf_status_str(adf_lball_set_rat_ball(x,p2,q,k+1)));
        printf("sub inputs_canonical=%d,%d k=%ld output_status=%s\n",
               adf_lball_is_canonical(x), adf_lball_is_canonical(y), k,
               adf_status_str(adf_lball_sub(z, x, y)));
        raw(x, 0, 0, k); raw(y, 1, 0, 0);
        fmpz_add_ui(fmpq_numref(y->u), power, 1);
        printf("div inputs_canonical=%d,%d k=%ld output_status=%s\n",
               adf_lball_is_canonical(x), adf_lball_is_canonical(y), k,
               adf_status_str(adf_lball_div(z, x, y)));
        adf_lball_clear(x); adf_lball_clear(y); adf_lball_clear(z); fmpz_clear(power);
        adf_rat_clear(q);
    }
    else if (!strcmp(argv[1], "inv"))
    {
        adf_sball_t x, y;
        adf_sball_init(x); adf_sball_init(y); x->arch = 9;
        if (argc != 3) return 2;
        if (!strcmp(argv[2], "arch")) printf("returned=%d\n", adf_sball_arch(x));
        else if (!strcmp(argv[2], "num")) printf("returned=%ld\n", adf_sball_num_places(x));
        else if (!strcmp(argv[2], "ident")) printf("returned=%d\n", adf_sball_identical(x, x));
        else if (!strcmp(argv[2], "setself")) adf_sball_set(x, x);
        else if (!strcmp(argv[2], "swap")) adf_sball_swap(x, y);
        else if (!strcmp(argv[2], "swapself")) adf_sball_swap(x, x);
        else if (!strcmp(argv[2], "control")) adf_sball_neg(y, NULL, x);
        else return 2;
        adf_sball_clear(x); adf_sball_clear(y);
    }
    else if (!strcmp(argv[1], "prec"))
    {
        arb_t x, y;
        int s;
        slong p = LONG_MAX;
        arb_init(x); arb_init(y); arb_one(x);
        if (argc != 3) return 2;
        printf("start %s prec=%ld\n", argv[2], p); fflush(stdout);
        if (!strcmp(argv[2], "exp")) s = adf_real_exp(y, x, p);
        else if (!strcmp(argv[2], "sin")) s = adf_real_sin(y, x, p);
        else if (!strcmp(argv[2], "root")) s = adf_real_root(y, x, 3, p);
        else if (!strcmp(argv[2], "log")) s = adf_real_log(y, x, p);
        else if (!strcmp(argv[2], "sqrt")) s = adf_real_sqrt(y, x, p);
        else return 2;
        printf("status=%s finite=%d\n", adf_status_str(s), arb_is_finite(y));
        arb_printn(y, 30, 0); putchar('\n');
        arb_clear(x); arb_clear(y);
    }
    else if (!strcmp(argv[1], "nonfinite"))
    {
        adf_sball_t x, y, z;
        fmpz_t e;
        adf_sball_init(x); adf_sball_init(y); adf_sball_init(z); fmpz_init(e);
        x->arch = y->arch = ADF_ARCH_REAL;
        fmpz_one(e); fmpz_mul_2exp(e, e, 100);
        arb_one(acb_realref(x->inf)); arb_mul_2exp_fmpz(acb_realref(x->inf), acb_realref(x->inf), e);
        adf_sball_set(y, x);
        int st = adf_sball_mul(z, NULL, x, y, 53);
        printf("input=%d status=%s output=%d finite=%d\n", adf_sball_is_canonical(x),
               adf_status_str(st), adf_sball_is_canonical(z), acb_is_finite(z->inf));
        adf_sball_clear(x); adf_sball_clear(y); adf_sball_clear(z); fmpz_clear(e);
    }
    else if (!strcmp(argv[1], "precedence"))
    {
        adf_sball_t x,y,z;
        adf_sball_init(x);adf_sball_init(y);adf_sball_init(z);
        x->arch=ADF_ARCH_COMPLEX;x->len=1;
        x->loc=flint_malloc(sizeof(adf_lball_struct));adf_lball_init(x->loc);
        raw(x->loc,1,ADF_LBALL_EXP_MAX,0);adf_sball_set(y,x);
        adf_place_t where;adf_place_prime(&where,11);
        adf_lball_t local_result;adf_lball_init(local_result);
        int local=adf_lball_mul(local_result,x->loc,y->loc);
        int st=adf_sball_mul(z,&where,x,y,53);
        printf("inputs_canonical=%d,%d local_status=%s partial_status=%s where_inf=%d output_empty=%d\n",
               adf_sball_is_canonical(x),adf_sball_is_canonical(y),adf_status_str(local),
               adf_status_str(st),adf_place_is_archimedean(where),z->arch==0&&z->len==0);
        adf_lball_clear(local_result);
        adf_sball_clear(x);adf_sball_clear(y);adf_sball_clear(z);
    }
    else if (!strcmp(argv[1], "s5"))
    {
        arb_t x,y;
        arb_init(x);arb_init(y);arb_one(x);arb_mul_2exp_si(x,x,1000);
        printf("input_finite=%d status=%s\n",arb_is_finite(x),adf_status_str(adf_real_exp(y,x,53)));
        arb_clear(x);arb_clear(y);
    }
    else return 2;
    flint_cleanup(); return 0;
}
