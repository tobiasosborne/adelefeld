#define _POSIX_C_SOURCE 200809L
#include <adelefeld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

extern char *old_adf_idele_get_str(size_t *, const adf_idele_t, slong);
extern char *old_adf_idclass_get_str(size_t *, const adf_idclass_t, slong);
extern char *review_adf_idele_get_str(size_t *, const adf_idele_t, slong);
extern char *review_level(size_t *, fmpq_t, fmpq_t, const fmpq_t, const fmpq_t, slong, slong);
extern unsigned long review_levels, review_passes, review_work, review_attempt;
extern unsigned long review_first_size, review_max_size;

static double now(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec * 1e-9;
}

static void family(arb_t x, slong b)
{
    fmpz_t z, e;
    fmpz_init(z); fmpz_init_set_si(e, -1);
    fmpz_one(z); fmpz_mul_2exp(z, z, b); fmpz_add_ui(z, z, 1);
    arf_set_fmpz_2exp(arb_midref(x), z, e);
    mag_one(arb_radref(x)); mag_mul_2exp_si(arb_radref(x), arb_radref(x), b - 1);
    fmpz_clear(z); fmpz_clear(e);
}

static int same(char *a, size_t na, char *b, size_t nb)
{
    return (a == NULL && b == NULL && na == 0 && nb == 0) ||
        (a && b && na == nb && memcmp(a, b, na + 1) == 0);
}

static int probe(arb_t x, slong digits, int history, int verbose)
{
    adf_idele_t id;
    adf_idclass_t cl;
    char *a, *b, *c, *d = NULL;
    size_t na = 99, nb = 99, nc = 99, nd = 99;
    int fail = 0;
    double start;
    adf_idele_init(id); adf_idclass_init(cl);
    arb_set(id->inf, x); arb_abs(cl->t, x);
    if (!adf_idele_is_canonical(id) || !adf_idclass_is_canonical(cl)) abort();
    start = now();
    a = adf_idele_get_str(&na, id, digits);
    double elapsed = now() - start;
    b = review_adf_idele_get_str(&nb, id, digits);
    c = adf_idclass_get_str(&nc, cl, digits);
    if (!same(a, na, b, nb)) fail++;
    if ((a == NULL) != (c == NULL) || (!a && (na || nc))) fail++;
    if (history) {
        size_t ne = 99;
        d = old_adf_idele_get_str(&nd, id, digits);
        if (!same(a, na, d, nd)) fail++;
        char *e = old_adf_idclass_get_str(&ne, cl, digits);
        if (!same(c, nc, e, ne)) fail++;
        adf_str_free(e);
    }
    if (verbose) {
        printf("digits=%ld mid_exp=%ld rad_exp=%ld null=%d len=%zu seconds=%.6f "
               "levels=%lu passes=%lu work=%lu attempt=%lu S0=%lu Smax=%lu "
               "class_null=%d failures=%d\n", digits,
               fmpz_get_si(ARF_EXPREF(arb_midref(x))),
               mag_is_zero(arb_radref(x)) ? 0 : fmpz_get_si(MAG_EXPREF(arb_radref(x))),
               a == NULL, na, elapsed, review_levels, review_passes, review_work,
               review_attempt, review_first_size, review_max_size, c == NULL, fail);
        if (a && na < 150) printf("text=%s\n", a);
        fflush(stdout);
    }
    adf_str_free(a); adf_str_free(b); adf_str_free(c); adf_str_free(d);
    adf_idele_clear(id); adf_idclass_clear(cl);
    return fail;
}

static unsigned long rng = 7829367;
static unsigned long next(void) { rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17; return rng; }

int main(int argc, char **argv)
{
    arb_t x;
    int fail = 0;
    arb_init(x);
    if (argc < 2) return 2;
    if (!strcmp(argv[1], "level")) {
        fmpq_t m, r, M, R;
        fmpq_init(m); fmpq_init(r); fmpq_init(M); fmpq_init(R);
        fmpq_set_si(m, 793, 512); fmpq_set_si(r, 741, 512);
        for (int k = 2; k <= 5; k++) {
            size_t len; char *s = review_level(&len, M, R, m, r, 1, k);
            printf("k=%d text=%s satisfies=%d\n", k, s, fmpq_cmp(M, R) > 0);
            if (k == 2 && fmpq_cmp(M, R) <= 0) fail++;
            if (k == 3 && fmpq_cmp(M, R) != 0) fail++;
            adf_str_free(s);
        }
        fmpq_clear(m); fmpq_clear(r); fmpq_clear(M); fmpq_clear(R);
    } else if (!strcmp(argv[1], "counterexample")) {
        arf_set_ui(arb_midref(x), 793);
        arf_mul_2exp_si(arb_midref(x), arb_midref(x), -9);
        mag_set_ui(arb_radref(x), 741);
        mag_mul_2exp_si(arb_radref(x), arb_radref(x), -9);
        fail += probe(x, 1, 1, 1);
    } else if (!strcmp(argv[1], "family")) {
        slong b = atol(argv[2]);
        family(x, b);
        fail += probe(x, argc > 3 ? atol(argv[3]) : 1, 0, 1);
    } else if (!strcmp(argv[1], "point")) {
        arb_one(x); arb_mul_2exp_si(x, x, atol(argv[2]));
        fail += probe(x, atol(argv[3]), 1, 1);
    } else if (!strcmp(argv[1], "mantissa")) {
        slong bits = atol(argv[2]);
        fmpz_t z, e;
        fmpz_init(z); fmpz_init_set_si(e, -bits);
        fmpz_one(z); fmpz_mul_2exp(z, z, bits); fmpz_add_ui(z, z, 1);
        arf_set_fmpz_2exp(arb_midref(x), z, e);
        fmpz_clear(z); fmpz_clear(e);
        fail += probe(x, atol(argv[3]), 0, 1);
    } else if (!strcmp(argv[1], "diff")) {
        for (int i = 0; i < 320; i++) {
            slong e = (slong)(next() % 2001) - 1000;
            if (i % 3 == 0) family(x, 2 + next() % 250);
            else {
                arf_set_ui(arb_midref(x), 1024 + next() % 1024);
                mag_set_ui(arb_radref(x), 1 + next() % 1023);
            }
            arb_mul_2exp_si(x, x, e);
            if (i & 1) arb_neg(x, x);
            fail += probe(x, 1 + next() % 35, 1, 0);
        }
        printf("balls=320 printer_calls=1600 failures=%d\n", fail);
    } else return 2;
    arb_clear(x); flint_cleanup();
    return fail ? 1 : 0;
}
