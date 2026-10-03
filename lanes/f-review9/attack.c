#include <adelefeld.h>
#include <flint/arf.h>
#include <flint/mag.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned long checks, failures, calls;
static unsigned long rowno;
#define CHECK(x) do { checks++; if (!(x)) { failures++; \
    if (failures < 30) printf("row=%lu line=%d check=%s\n", rowno, __LINE__, #x); } } while (0)
static adf_place_t sentinel(void) {
    adf_place_t p; CHECK(adf_place_prime(&p, 97) == ADF_OK); return p;
}
static int samewhere(adf_place_t w, int inf) {
    return inf ? adf_place_is_archimedean(w) : adf_place_equal(w, sentinel());
}
static void setq(fmpq_t q, const char *a, const char *b) {
    CHECK(fmpz_set_str(fmpq_numref(q), a, 10) == 0);
    CHECK(fmpz_set_str(fmpq_denref(q), b, 10) == 0);
    CHECK(fmpq_is_canonical(q));
}
static int fields(char *line, char **v, int n) {
    int k = 0; char *s = strtok(line, " \t\r\n");
    while (s && k < n) { v[k++] = s; s = strtok(NULL, " \t\r\n"); }
    return k;
}
static void rat_cases(const char *path) {
    FILE *f = fopen(path, "r"); char *line = NULL, *v[8]; size_t cap = 0;
    adf_rat_t q, r, y, t; adf_adele_t a, b, bt; adf_idele_t i, j, jt;
    adf_rat_init(q); adf_rat_init(r); adf_rat_init(y); adf_rat_init(t);
    adf_adele_init(a); adf_adele_init(b); adf_adele_init(bt);
    adf_idele_init(i); adf_idele_init(j); adf_idele_init(jt);
    CHECK(f != NULL); if (!f) return;
    unsigned long rows = 0;
    while (getline(&line, &cap, f) > 0) {
        rowno = ++rows; CHECK(fields(line, v, 8) == 8);
        setq(q->q, v[0], v[1]); setq(r->q, v[6], v[7]);
        ulong n = strtoul(v[2], NULL, 10); int sign = atoi(v[3]);
        int st = atoi(v[4]), inf = atoi(v[5]);
        for (int mode = 0; mode < 3; mode++) {
            adf_place_t w = sentinel();
            if (mode == 1) adf_rat_set(y, q); else adf_rat_set_si(y, 123);
            adf_rat_set(t, y);
            int got = adf_rat_root(y, mode == 2 ? NULL : &w, mode == 1 ? y : q, n, sign);
            calls++; CHECK(got == st);
            CHECK(mode == 2 || samewhere(w, inf));
            CHECK(st == ADF_OK ? adf_rat_identical(y, r) : adf_rat_identical(y, t));
        }
        arb_one(a->inf); adf_fball_set_rat(&a->fin, q);
        int afin = 0;
        /* With the independent real coordinate 1, only a finite failure occurs. */
        for (int mode = 0; mode < 3; mode++) {
            adf_place_t w = sentinel();
            if (mode == 1) adf_adele_set(b, a); else { arb_zero(b->inf); adf_fball_zero(&b->fin); }
            adf_adele_set(bt, b);
            int got = adf_adele_root(b, mode == 2 ? NULL : &w, mode == 1 ? b : a, n, sign, 64);
            /* Odd negative selector on a finite zero is invalid since real coordinate is 1. */
            int ast = (!n || (n > 1 && sign != 1 && sign != -1) ||
                (n > 1 && n % 2 && sign == -1)) ? ADF_DOMAIN : st;
            calls++; CHECK(got == ast); CHECK(mode == 2 || samewhere(w, afin));
            if (ast == ADF_OK) {
                CHECK(adf_adele_is_canonical(b));
                adf_fball_get_center(y, &b->fin); CHECK(adf_rat_identical(y, r));
                CHECK(arb_is_exact(b->inf));
                CHECK(arf_equal_si(arb_midref(b->inf), n > 1 && n % 2 == 0 ? sign : 1));
            } else CHECK(adf_adele_identical(b, bt));
        }
        if (!adf_rat_is_zero(q)) {
            arb_one(i->inf); fmpq_abs(i->r, q->q);
            if (fmpq_sgn(q->q) < 0) adf_ucoset_minus_one(&i->u); else adf_ucoset_one(&i->u);
            for (int mode = 0; mode < 3; mode++) {
                adf_place_t w = sentinel();
                if (mode == 1) adf_idele_set(j, i);
                else { arb_one(j->inf); fmpq_one(j->r); adf_ucoset_one(&j->u); }
                adf_idele_set(jt, j);
                int got = adf_idele_root(j, mode == 2 ? NULL : &w, mode == 1 ? j : i, n, sign, 64);
                calls++; CHECK(got == st); CHECK(mode == 2 || samewhere(w, 0));
                if (st == ADF_OK) {
                    CHECK(adf_idele_is_canonical(j)); CHECK(arb_is_nonzero(j->inf));
                    fmpq_abs(y->q, r->q); CHECK(fmpq_equal(j->r, y->q));
                    CHECK(adf_ucoset_is_exact(&j->u)); CHECK(fmpz_equal_si(j->u.c, fmpq_sgn(r->q)));
                } else CHECK(adf_idele_identical(j, jt));
            }
        }
    }
    printf("rational_rows=%lu\n", rows);
    free(line); fclose(f);
    adf_rat_clear(q); adf_rat_clear(r); adf_rat_clear(y); adf_rat_clear(t);
    adf_adele_clear(a); adf_adele_clear(b); adf_adele_clear(bt);
    adf_idele_clear(i); adf_idele_clear(j); adf_idele_clear(jt);
}
typedef int (*series_fn)(adf_adele_t, adf_place_t *, const adf_adele_t, slong);
static series_fn funs[] = {adf_adele_exp, adf_adele_sin, adf_adele_sinh, adf_adele_cos, adf_adele_cosh};
static void real_cases(const char *path) {
    FILE *f = fopen(path, "r"); char *line = NULL, *v[13]; size_t cap = 0;
    adf_adele_t x, y, before; fmpz_t m, e; arf_t lo, hi;
    adf_adele_init(x); adf_adele_init(y); adf_adele_init(before);
    fmpz_init(m); fmpz_init(e); arf_init(lo); arf_init(hi);
    CHECK(f != NULL); if (!f) return;
    unsigned long rows = 0;
    while (getline(&line, &cap, f) > 0) {
        rowno = ++rows; CHECK(fields(line, v, 13) == 13);
        int op = atoi(v[0]); ulong n = strtoul(v[5], NULL, 10); int sign = atoi(v[6]);
        slong p = strtol(v[7], NULL, 10); int st = atoi(v[8]);
        fmpz_set_str(m, v[1], 10); fmpz_set_str(e, v[2], 10); arb_set_fmpz_2exp(x->inf, m, e);
        mag_set_ui_2exp_si(arb_radref(x->inf), strtoul(v[3], NULL, 10), strtol(v[4], NULL, 10));
        adf_fball_zero(&x->fin); CHECK(adf_adele_is_canonical(x));
        fmpz_set_str(m, v[9], 10); fmpz_set_str(e, v[10], 10); arf_set_fmpz_2exp(lo, m, e);
        fmpz_set_str(m, v[11], 10); fmpz_set_str(e, v[12], 10); arf_set_fmpz_2exp(hi, m, e);
        for (int mode = 0; mode < 3; mode++) {
            adf_place_t w = sentinel();
            if (mode == 1) adf_adele_set(y, x);
            else { arb_one(y->inf); adf_fball_one(&y->fin); }
            adf_adele_set(before, y);
            int got = op ? funs[op - 1](y, mode == 2 ? NULL : &w, mode == 1 ? y : x, p) :
                adf_adele_root(y, mode == 2 ? NULL : &w, mode == 1 ? y : x, n, sign, p);
            calls++; CHECK(got == st);
            int inf = st != ADF_OK && !(op == 0 && n % 2 && sign == -1);
            CHECK(mode == 2 || samewhere(w, inf));
            if (got == ADF_OK && st == ADF_OK) {
                CHECK(adf_adele_is_canonical(y));
                CHECK(arb_contains_arf(y->inf, lo)); CHECK(arb_contains_arf(y->inf, hi));
                CHECK(adf_fball_is_exact(&y->fin));
                CHECK(fmpz_equal_si(y->fin.A, op == 1 || op == 4 || op == 5 ? 1 : 0));
                CHECK(fmpz_is_one(y->fin.d));
            } else CHECK(adf_adele_identical(y, before));
        }
    }
    printf("real_rows=%lu\n", rows);
    free(line); fclose(f); adf_adele_clear(x); adf_adele_clear(y); adf_adele_clear(before);
    fmpz_clear(m); fmpz_clear(e); arf_clear(lo); arf_clear(hi);
}
int main(void) {
    rat_cases("lanes/f-review9/rat.tsv"); real_cases("lanes/f-review9/real.tsv");
    printf("calls=%lu checks=%lu failures=%lu\n", calls, checks, failures);
    flint_cleanup(); return failures ? 1 : 0;
}
