/* i-review1: line-driven harness for the real kernel of adf_idele_mul / inv / set_rat. Protocol on stdin:
   mul ALIAS P  M1 E1 RM1 RE1  M2 E2 RM2 RE2    ball i = M*2^E +- RM*2^RE (RM < 2^30)
   inv ALIAS P  M E RM RE
   rat 0 P NUM DEN
   Output: status, then mid mant, mid exp, rad mant, rad exp (odd mantissa form). */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <adelefeld.h>

static void mk(adf_idele_t x, const char *ms, long e, unsigned long rm, long re)
{
    arb_t b; fmpq_t q; adf_ucoset_t u; fmpz_t m, ee; int st;
    arb_init(b); fmpq_init(q); adf_ucoset_init(u); fmpz_init(m); fmpz_init_set_si(ee, e);
    fmpz_set_str(m, ms, 10);
    arf_set_fmpz_2exp(arb_midref(b), m, ee);
    mag_set_ui_2exp_si(arb_radref(b), rm, re);
    fmpq_one(q);
    st = adf_idele_set_parts(x, b, q, u);
    if (st != ADF_OK) { printf("SETPARTS_FAIL %d\n", st); exit(1); }
    arb_clear(b); fmpq_clear(q); adf_ucoset_clear(u); fmpz_clear(m); fmpz_clear(ee);
}

static void pr(const adf_idele_t z)
{
    fmpz_t m, e; arf_t a;
    fmpz_init(m); fmpz_init(e); arf_init(a);
    arf_get_fmpz_2exp(m, e, arb_midref(z->inf));
    printf(" "); fmpz_print(m); printf(" "); fmpz_print(e);
    arf_set_mag(a, arb_radref(z->inf));
    if (arf_is_zero(a)) printf(" 0 0");
    else { arf_get_fmpz_2exp(m, e, a); printf(" "); fmpz_print(m); printf(" "); fmpz_print(e); }
    printf("\n");
    fmpz_clear(m); fmpz_clear(e); arf_clear(a);
}

int main(void)
{
    char op[16], ms[4096], ms2[4096]; long p, e, re, e2, re2; unsigned long rm, rm2; int alias;
    while (scanf("%15s %d %ld", op, &alias, &p) == 3)
    {
        adf_idele_t x, y, z, w; int st;
        adf_idele_init(x); adf_idele_init(y); adf_idele_init(z); adf_idele_init(w);
        if (!strcmp(op, "mul")) {
            if (scanf("%s %ld %lu %ld %s %ld %lu %ld", ms, &e, &rm, &re, ms2, &e2, &rm2, &re2) != 8) return 2;
            mk(x, ms, e, rm, re); mk(y, ms2, e2, rm2, re2);
            if (alias == 0) { st = adf_idele_mul(z, x, y, p); printf("%d", st);
                if (st == 0) pr(z); else printf(" sentinel_ok=%d\n", arb_is_one(z->inf)); }
            else if (alias == 1) { adf_idele_set(w, x); st = adf_idele_mul(x, x, y, p); printf("%d", st);
                if (st == 0) pr(x); else printf(" untouched=%d\n", adf_idele_identical(x, w)); }
            else if (alias == 2) { adf_idele_set(w, y); st = adf_idele_mul(y, x, y, p); printf("%d", st);
                if (st == 0) pr(y); else printf(" untouched=%d\n", adf_idele_identical(y, w)); }
            else { adf_idele_set(w, x); st = adf_idele_mul(x, x, x, p); printf("%d", st);
                if (st == 0) pr(x); else printf(" untouched=%d\n", adf_idele_identical(x, w)); }
        } else if (!strcmp(op, "inv")) {
            if (scanf("%s %ld %lu %ld", ms, &e, &rm, &re) != 4) return 2;
            mk(x, ms, e, rm, re);
            if (alias == 0) { st = adf_idele_inv(z, x, p); printf("%d", st);
                if (st == 0) pr(z); else printf(" sentinel_ok=%d\n", arb_is_one(z->inf)); }
            else { adf_idele_set(w, x); st = adf_idele_inv(x, x, p); printf("%d", st);
                if (st == 0) pr(x); else printf(" untouched=%d\n", adf_idele_identical(x, w)); }
        } else if (!strcmp(op, "rat")) {
            adf_rat_t q; fmpz_t n, d; adf_rat_init(q); fmpz_init(n); fmpz_init(d);
            if (scanf("%s %s", ms, ms2) != 2) return 2;
            fmpz_set_str(n, ms, 10); fmpz_set_str(d, ms2, 10);
            adf_rat_set_fmpz2(q, n, d);
            st = adf_idele_set_rat(z, q, p); printf("%d", st);
            if (st == 0) pr(z); else printf("\n");
            adf_rat_clear(q); fmpz_clear(n); fmpz_clear(d);
        }
        adf_idele_clear(x); adf_idele_clear(y); adf_idele_clear(z); adf_idele_clear(w);
    }
    return 0;
}
