#include <adelefeld.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    char name[20], mantissa[2000];
    slong e, re, prec;
    ulong rm, n;
    int st;
    arb_t x, y, saved, alias;
    fmpz_t m, exponent;
    fmpq_t mid, rad;
    adf_sball_t sx, sy, sy_saved, sx_alias;
    adf_place_t where, marker;
    arb_init(x); arb_init(y); arb_init(saved); arb_init(alias);
    fmpz_init(m); fmpz_init(exponent); fmpq_init(mid); fmpq_init(rad);
    adf_sball_init(sx); adf_sball_init(sy); adf_sball_init(sy_saved); adf_sball_init(sx_alias);
    adf_place_prime(&marker, 11);
    while (scanf("%19s %1999s %ld %lu %ld %lu %ld", name, mantissa, &e, &rm, &re, &n, &prec) == 7)
    {
        fmpz_set_str(m, mantissa, 10); fmpz_set_si(exponent, e);
        arb_set_fmpz_2exp(x, m, exponent); mag_set_ui_2exp_si(arb_radref(x), rm, re);
        arb_set_si(saved, 13); arb_set(y, saved); arb_set(alias, x);
        int (*fn)(arb_t, const arb_t, slong) = NULL;
        int (*at)(adf_sball_t, adf_place_t *, const adf_sball_t, adf_place_t, slong) = NULL;
        if (!strcmp(name, "exp")) { fn = adf_real_exp; at = adf_sball_exp_at; }
        if (!strcmp(name, "log")) { fn = adf_real_log; at = adf_sball_log_at; }
        if (!strcmp(name, "logabs")) { fn = adf_real_log_abs; at = adf_sball_log_abs_at; }
        if (!strcmp(name, "sin")) { fn = adf_real_sin; at = adf_sball_sin_at; }
        if (!strcmp(name, "cos")) { fn = adf_real_cos; at = adf_sball_cos_at; }
        if (!strcmp(name, "sqrt")) { fn = adf_real_sqrt; at = adf_sball_sqrt_at; }
        st = fn ? fn(y, x, prec) : adf_real_root(y, x, n, prec);
        int sa = fn ? fn(alias, alias, prec) : adf_real_root(alias, alias, n, prec);
        int alias_ok = sa == st && arb_equal(alias, st == ADF_OK ? y : x);
        adf_sball_set_arb_lballs(sx, NULL, x, NULL, 0); where = marker;
        adf_sball_set(sy_saved,sy);
        int ss = at ? at(sy, &where, sx, adf_place_inf(), prec)
                    : adf_sball_root_at(sy, &where, sx, adf_place_inf(), n, prec);
        int at_ok = ss == st && (st == ADF_OK ? arb_equal(acb_realref(sy->inf), y)
                                             : adf_sball_identical(sy,sy_saved));
        int where_ok = adf_place_equal(where, st == ADF_OK || (!fn && n == 0) ? marker : adf_place_inf());
        adf_sball_set(sx_alias,sx);where=marker;
        ss=at ? at(sx_alias,&where,sx_alias,adf_place_inf(),prec)
              : adf_sball_root_at(sx_alias,&where,sx_alias,adf_place_inf(),n,prec);
        at_ok &= ss==st && adf_sball_identical(sx_alias,st==ADF_OK ? sy : sx);
        where_ok &= adf_place_equal(where, st == ADF_OK || (!fn && n == 0) ? marker : adf_place_inf());
        printf("%s %d %d %d %d %d ", adf_status_str(st), arb_is_finite(y), alias_ok, at_ok, where_ok,
               st == ADF_OK || arb_equal(y, saved));
        arf_get_fmpq(mid, arb_midref(y)); mag_get_fmpq(rad, arb_radref(y));
        fmpq_print(mid); putchar(' '); fmpq_print(rad); putchar('\n'); fflush(stdout);
    }
    arb_clear(x); arb_clear(y); arb_clear(saved); arb_clear(alias);
    fmpz_clear(m); fmpz_clear(exponent); fmpq_clear(mid); fmpq_clear(rad);
    adf_sball_clear(sx); adf_sball_clear(sy); adf_sball_clear(sy_saved); adf_sball_clear(sx_alias);
    flint_cleanup(); return 0;
}
