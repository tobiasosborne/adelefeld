/* i-review1: line-driven harness for adf_ucoset. stdin lines: op c N [c2 N2]  (decimal, big allowed)
   ops: mul inv norm contains eq ov set. Output: "c N" or an int; ALIASMISMATCH if the aliased calls differ. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <adelefeld.h>

static void raw(adf_ucoset_t x, const char *c, const char *N)
{
    /* build the stored pair directly, bypassing set_fmpz2 (the test feeds canonical pairs; set is tested
       through the op "set") */
    fmpz_set_str(x->c, c, 10);
    fmpz_set_str(x->N, N, 10);
}

static void pu(const adf_ucoset_t x) { fmpz_print(x->c); printf(" "); fmpz_print(x->N); }

int main(void)
{
    char op[16], c[8192], N[8192], c2[8192], N2[8192];
    while (scanf("%15s %s %s", op, c, N) == 3)
    {
        adf_ucoset_t x, y, z, a, b;
        adf_ucoset_init(x); adf_ucoset_init(y); adf_ucoset_init(z); adf_ucoset_init(a); adf_ucoset_init(b);
        if (!strcmp(op, "set"))
        {
            fmpz_t fc, fN; int st;
            fmpz_init(fc); fmpz_init(fN);
            fmpz_set_str(fc, c, 10); fmpz_set_str(fN, N, 10);
            st =adf_ucoset_set_fmpz2(z, fc, fN);
            printf("%d ", st); pu(z);
            /* aliased: c and N members of x */
            fmpz_set(x->c, fc); fmpz_set(x->N, fN);
            {
                adf_ucoset_t w; fmpz_t sc, sN; int st2;
                adf_ucoset_init(w);
                fmpz_set(w->c, fc); fmpz_set(w->N, fN);
                fmpz_init_set(sc, fc); fmpz_init_set(sN, fN);
                st2 = adf_ucoset_set_fmpz2(w, w->c, w->N);
                if (st2 != st || (st == 0 && !adf_ucoset_identical(w, z)))
                    printf(" ALIASMISMATCH(%d)", st2);
                if (st != 0)
                {   /* untouched check: init z to (5,12) and see */
                    adf_ucoset_t u; fmpz_t f5, f12;
                    adf_ucoset_init(u); fmpz_init_set_ui(f5, 5); fmpz_init_set_ui(f12, 12);
                    adf_ucoset_set_fmpz2(u, f5, f12);
                    adf_ucoset_set_fmpz2(u, fc, fN);
                    if (fmpz_cmp_ui(u->c, 5) || fmpz_cmp_ui(u->N, 12)) printf(" NOTUNTOUCHED");
                    adf_ucoset_clear(u); fmpz_clear(f5); fmpz_clear(f12);
                }
                fmpz_clear(sc); fmpz_clear(sN); adf_ucoset_clear(w);
            }
            printf("\n");
            fmpz_clear(fc); fmpz_clear(fN);
        }
        else if (!strcmp(op, "inv") || !strcmp(op, "norm"))
        {
            int inv = !strcmp(op, "inv");
            raw(x, c, N);
            if (inv) adf_ucoset_inv(z, x); else adf_ucoset_normalise(z, x);
            adf_ucoset_set(a, x);
            if (inv) adf_ucoset_inv(a, a); else adf_ucoset_normalise(a, a);
            pu(z);
            if (!adf_ucoset_identical(a, z)) printf(" ALIASMISMATCH");
            if (!adf_ucoset_is_normal(z)) printf(" NOTNORMAL");
            printf("\n");
        }
        else
        {
            if (scanf("%s %s", c2, N2) != 2) return 2;
            raw(x, c, N); raw(y, c2, N2);
            if (!strcmp(op, "mul"))
            {
                adf_ucoset_mul(z, x, y);
                adf_ucoset_set(a, x); adf_ucoset_mul(a, a, y);
                adf_ucoset_set(b, y); adf_ucoset_mul(b, x, b);
                pu(z);
                if (!adf_ucoset_identical(a, z) || !adf_ucoset_identical(b, z)) printf(" ALIASMISMATCH");
                if (!strcmp(c, c2) && !strcmp(N, N2))
                { adf_ucoset_mul(x, x, x); if (!adf_ucoset_identical(x, z)) printf(" ALIASMISMATCH2"); }
                if (!adf_ucoset_is_normal(z)) printf(" NOTNORMAL");
                printf("\n");
            }
            else if (!strcmp(op, "contains")) printf("%d\n", adf_ucoset_contains(x, y));
            else if (!strcmp(op, "eq")) printf("%d\n", adf_ucoset_equal_set(x, y));
            else if (!strcmp(op, "ov")) printf("%d\n", adf_ucoset_overlaps(x, y));
        }
        adf_ucoset_clear(x); adf_ucoset_clear(y); adf_ucoset_clear(z); adf_ucoset_clear(a); adf_ucoset_clear(b);
    }
    return 0;
}
