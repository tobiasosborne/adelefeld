/* Exact dyadic protocol. Scratch instrumentation does not change the arithmetic. */
#include <adelefeld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static mag_t review_B;
static int review_has_B, review_fallback;
#ifdef REVIEW_TRACE
#ifndef REVIEW_SOURCE
#define REVIEW_SOURCE "build/localfactor_trace.c"
#endif
#include REVIEW_SOURCE
#endif

static void read_arf(arf_t x, const char *m, const char *e)
{
    fmpz_t a, b;
    fmpz_init(a); fmpz_init(b);
    if (!strcmp(m, "nan")) arf_nan(x);
    else if (!strcmp(m, "inf")) arf_pos_inf(x);
    else if (!strcmp(m, "ninf")) arf_neg_inf(x);
    else {
        if (fmpz_set_str(a, m, 10) || fmpz_set_str(b, e, 10)) abort();
        arf_set_fmpz_2exp(x, a, b);
    }
    fmpz_clear(a); fmpz_clear(b);
}

static void read_rad(mag_t r, const char *m, const char *e)
{
    fmpz_t a, b;
    fmpz_init(a); fmpz_init(b);
    if (!strcmp(m, "inf")) mag_inf(r);
    else {
        if (fmpz_set_str(a, m, 10) || fmpz_set_str(b, e, 10)) abort();
        if (fmpz_sgn(a) < 0) abort();
        if (fmpz_bits(a) <= 30 && fmpz_fits_si(b))
            mag_set_ui_2exp_si(r, fmpz_get_ui(a), fmpz_get_si(b));
        else mag_set_fmpz_2exp_fmpz(r, a, b);
    }
    fmpz_clear(a); fmpz_clear(b);
}

static void emit_arf(const arf_t x)
{
    fmpz_t a, b;
    if (!arf_is_finite(x)) { printf("null"); return; }
    fmpz_init(a); fmpz_init(b);
    arf_get_fmpz_2exp(a, b, x);
    char *sa = fmpz_get_str(NULL, 10, a), *sb = fmpz_get_str(NULL, 10, b);
    printf("[\"%s\",\"%s\"]", sa, sb);
    flint_free(sa); flint_free(sb);
    fmpz_clear(a); fmpz_clear(b);
}

static void emit_mag(const mag_t r)
{
    arf_t t;
    arf_init(t); arf_set_mag(t, r); emit_arf(t); arf_clear(t);
}

static void emit_acb(const acb_t s)
{
    printf("["); emit_arf(arb_midref(acb_realref(s))); printf(",");
    emit_arf(arb_midref(acb_imagref(s))); printf(",");
    emit_mag(arb_radref(acb_realref(s))); printf(",");
    emit_mag(arb_radref(acb_imagref(s))); printf("]");
}

static int same_repr(const acb_t a, const acb_t b)
{
    int ok = 1;
    for (int j = 0; j < 2; j++) {
        char *x = arb_dump_str(j ? acb_imagref(a) : acb_realref(a));
        char *y = arb_dump_str(j ? acb_imagref(b) : acb_realref(b));
        ok &= !strcmp(x, y); flint_free(x); flint_free(y);
    }
    return ok;
}

int main(int argc, char **argv)
{
    int modes = argc > 1 && !strcmp(argv[1], "modes");
    acb_t s, y, saved, alias, normal;
    acb_init(s); acb_init(y); acb_init(saved); acb_init(alias); acb_init(normal);
    mag_init(review_B);
    ulong p; slong prec;
    static char tok[8][16384];
    unsigned long count = 0, errors = 0;
    while (scanf("%lu %ld", &p, &prec) == 2) {
        for (int j = 0; j < 8; j++) if (scanf("%16383s", tok[j]) != 1) abort();
        acb_zero(s);
        read_arf(arb_midref(acb_realref(s)), tok[0], tok[1]);
        read_arf(arb_midref(acb_imagref(s)), tok[2], tok[3]);
        read_rad(arb_radref(acb_realref(s)), tok[4], tok[5]);
        read_rad(arb_radref(acb_imagref(s)), tok[6], tok[7]);
        acb_set_si_si(y, 7, -5);
        mag_set_ui_2exp_si(arb_radref(acb_realref(y)), 3, -12);
        mag_set_ui_2exp_si(arb_radref(acb_imagref(y)), 7, -15);
        acb_set(saved, y);
        adf_place_t v = adf_place_inf(), where, mark;
        if (p && adf_place_prime(&v, p) != ADF_OK) abort();
        if (adf_place_prime(&mark, 97) != ADF_OK) abort();
        where = mark; review_has_B = review_fallback = 0;
        int st = adf_local_zeta_factor_at(y, &where, s, v, prec);
        int repr = st == ADF_OK ? acb_is_finite(y) : same_repr(y, saved);
        int wh = adf_place_equal(where, st == ADF_OK ? mark : v);
        int has_B = review_has_B, fb = review_fallback;
        mag_t bound; mag_init(bound); mag_set(bound, review_B);
        if (modes) {
            acb_set(alias, s); where = mark;
            int sta = adf_local_zeta_factor_at(alias, &where, alias, v, prec);
            repr &= sta == st && (st == ADF_OK ? acb_equal(alias, y) : same_repr(alias, s));
            wh &= adf_place_equal(where, st == ADF_OK ? mark : v);
            acb_set(normal, saved);
            int stn = adf_local_zeta_factor_at(normal, NULL, s, v, prec);
            repr &= stn == st && same_repr(normal, st == ADF_OK ? y : saved);
        }
        printf("{\"status\":%d,\"repr\":%d,\"where\":%d,\"fallback\":%d,\"s\":", st, repr, wh, fb);
        emit_acb(s); printf(",\"y\":"); emit_acb(y); printf(",\"B\":");
        if (has_B) emit_mag(bound); else printf("null");
        printf("}\n"); fflush(stdout);
        mag_clear(bound); count++; if (!repr || !wh) errors++;
    }
    fprintf(stderr, "bridge calls=%lu output_errors=%lu\n", count, errors);
    mag_clear(review_B);
    acb_clear(s); acb_clear(y); acb_clear(saved); acb_clear(alias); acb_clear(normal);
    flint_cleanup(); return errors ? 1 : 0;
}
