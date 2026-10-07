/* Lane m3-review1: resources and phase_get_acb edges (q-review4 hunts 4, 5), timed. */
#define _POSIX_C_SOURCE 200809L
#include <adelefeld.h>
#include <stdio.h>
#include <time.h>
static double now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec * 1e-9; }
static void sent(acb_t z) { acb_set_si(z, 77); }
static int is_sent(const acb_t z) { acb_t s; int r; acb_init(s); sent(s); r = acb_equal(s, z); acb_clear(s); return r; }
int main(void)
{
    acb_t z; fmpq_t th; adf_adele_t x; adf_rat_t c, N; double t0; int st; slong precs[] = {1, 2, 53, ADF_REAL_PREC_MAX, ADF_REAL_PREC_MAX + 1};
    int i; adf_qclass_t q;
    acb_init(z); fmpq_init(th); adf_adele_init(x); adf_rat_init(c); adf_rat_init(N); adf_qclass_init(q);
    for (i = 0; i < 5; i++) {
        slong ths[][2] = {{0, 1}, {1, 4}, {1, 2}, {3, 4}, {1, 3}, {7, 1000000}};
        int k;
        for (k = 0; k < 6; k++) {
            fmpq_set_si(th, ths[k][0], ths[k][1]); sent(z); t0 = now();
            st = adf_phase_get_acb(z, th, precs[i]);
            printf("phase %ld/%ld prec %ld: st %d %s %.3fs", ths[k][0], ths[k][1], (long) precs[i], st,
                   st ? (is_sent(z) ? "untouched" : "WRITTEN") : "", now() - t0);
            if (!st) { printf(" "); acb_printd(z, 20); }
            printf("\n");
        }
    }
    /* finite denominator 2^20000 bits: N = 1/3^12700 (about 20130 bits); centre 1/(3^12700 + 2) */
    {
        fmpz_t d; fmpz_init(d); fmpz_ui_pow_ui(d, 3, 12700);
        fmpz_one(fmpq_numref(N->q)); fmpz_set(fmpq_denref(N->q), d);
        fmpz_add_ui(d, d, 2); fmpz_one(fmpq_numref(c->q)); fmpz_set(fmpq_denref(c->q), d);
        adf_fball_set_center_radius(&x->fin, c, N);
        arb_set_d(x->inf, 0.25);
        t0 = now(); sent(z); st = adf_adele_psi_tate(z, x, 128);
        printf("den 2^20000-bit default: st %d %.3fs\n", st, now() - t0);
        t0 = now(); sent(z); st = adf_adele_psi_tate_strict(z, x, 128);
        printf("den 2^20000-bit strict: st %d %s %.3fs\n", st, is_sent(z) ? "untouched" : "WRITTEN", now() - t0);
        fmpz_clear(d);
    }
    /* real exponent at ADF_QCLASS_EXP_MAX and one beyond */
    {
        slong e[] = {ADF_QCLASS_EXP_MAX - 1, ADF_QCLASS_EXP_MAX, ADF_QCLASS_EXP_MAX + 1, -ADF_QCLASS_EXP_MAX, -ADF_QCLASS_EXP_MAX - 1};
        int k;
        adf_fball_zero(&x->fin);
        for (k = 0; k < 5; k++) {
            arf_set_ui_2exp_si(arb_midref(x->inf), 3, e[k]); mag_zero(arb_radref(x->inf));
            t0 = now(); sent(z); st = adf_adele_psi_tate(z, x, 53);
            printf("mid 3*2^%ld: psi st %d %s %.3fs", (long) e[k], st, st ? (is_sent(z) ? "untouched" : "WRITTEN") : "", now() - t0);
            t0 = now(); fmpq_set_si(th, 5, 7); st = adf_adele_psi_tate_phase(th, x);
            printf(" | phase st %d %.3fs", st, now() - t0);
            adf_qclass_set_adele(q, x);
            { int truth = 7; t0 = now(); st = adf_qclass_equal_set(&truth, q, q, 1000);
              printf(" | equal_set st %d truth %d %.3fs", st, truth, now() - t0); }
            { adf_qclass_t r; adf_qclass_init(r); t0 = now(); st = adf_qclass_neg(r, q, 1000, 53);
              printf(" | neg st %d %.3fs\n", st, now() - t0); adf_qclass_clear(r); }
        }
    }
    /* prec at the cap on a non-cardinal adele */
    {
        fmpq_set_si(c->q, 1, 3); fmpq_zero(N->q); adf_fball_set_center_radius(&x->fin, c, N);
        arb_zero(x->inf);
        t0 = now(); sent(z); st = adf_adele_psi_tate(z, x, ADF_REAL_PREC_MAX);
        printf("(0;1/3) prec cap: st %d %s %.3fs\n", st, st ? (is_sent(z) ? "untouched" : "WRITTEN") : "", now() - t0);
        t0 = now(); sent(z); st = adf_adele_psi_tate(z, x, ADF_REAL_PREC_MAX / 2);
        printf("(0;1/3) prec cap/2: st %d %.3fs\n", st, now() - t0);
    }
    acb_clear(z); fmpq_clear(th); adf_adele_clear(x); adf_rat_clear(c); adf_rat_clear(N); adf_qclass_clear(q);
    flint_cleanup_master();
    return 0;
}
