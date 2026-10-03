#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <adelefeld.h>
#include <adelefeld/text.h>
static adf_idele_t X;
static void setx(const char *s) {
    int st = adf_idele_set_str(X, s, strlen(s), 200, NULL);
    if (st) { printf("PARSE %d for %s\n", st, s); exit(1); }
}
static adf_place_t P(unsigned long p) { adf_place_t v; int r = adf_place_prime(&v, p); if (r) { printf("badprime %lu -> %d\n", p, r); exit(1);} return v; }
static const char *pn(adf_place_t v) { static char b[8][64]; static int i; i = (i+1)&7;
    if (adf_place_is_archimedean(v)) strcpy(b[i], "inf"); else sprintf(b[i], "%lu", adf_place_prime_get(v)); return b[i]; }
#define SENT_PL 9999991UL
static void chk_adele(const char *name, int st, adf_place_t w, adf_adele_t y, adf_adele_t y0) {
    printf("%-34s st=%d where=%s  y %s\n", name, st, pn(w), adf_adele_identical(y, y0) ? "UNCHANGED" : "CHANGED");
}
static void chk_sball(const char *name, int st, adf_place_t w, adf_sball_t y, adf_sball_t y0) {
    printf("%-34s st=%d where=%s  y %s\n", name, st, pn(w), adf_sball_identical(y, y0) ? "UNCHANGED" : "CHANGED");
}
int main(void) {
    adf_idele_init(X);
    adf_adele_t y, y0; adf_adele_init(y); adf_adele_init(y0);
    adf_adele_set_str(y, "(7 ; 5 mod 6)", 13, 100, NULL);
    adf_adele_set(y0, y);
    adf_sball_t s, s0; adf_sball_init(s); adf_sball_init(s0);
    {arb_t a; arb_init(a); arb_set_ui(a,7); adf_sball_set_arb_lballs(s,NULL,a,NULL,0); arb_clear(a);}
    printf("sball init str status check\n");
    adf_sball_set(s0, s);
    adf_place_t w;
    int st;
    /* negative I */
    setx("(-2 ; 1 * [1 mod 4])");
    w = P(SENT_PL % 1 + 7); st = adf_idele_Log(y, &w, X, 53); chk_adele("Log neg I", st, w, y, y0);
    w = P(7); st = adf_idele_log_abs(y, &w, X, 53); chk_adele("log_abs neg I (OK expected)", st, w, y, y0);
    adf_adele_set(y, y0);
    w = P(7); st = adf_idele_Log_at(s, &w, X, adf_place_inf(), 53); chk_sball("Log_at inf neg I", st, w, s, s0);
    w = P(7); st = adf_idele_log_abs_at(s, &w, X, adf_place_inf(), 53); chk_sball("log_abs_at inf neg I OK", st, w, s, s0);
    adf_sball_set(s, s0);
    w = P(7); st = adf_idele_Log_at(s, &w, X, P(3), 53); chk_sball("Log_at 3 neg I (OK)", st, w, s, s0);
    adf_sball_set(s, s0);
    /* precision cap */
    setx("(2 ; 3 * [1 mod 4])");
    w = P(7); st = adf_idele_Log(y, &w, X, ADF_REAL_PREC_MAX); chk_adele("Log prec=MAX (OK, w untouched=7)", st, w, y, y0);
    adf_adele_set(y, y0);
    w = P(7); st = adf_idele_Log(y, &w, X, ADF_REAL_PREC_MAX + 1); chk_adele("Log prec=MAX+1 LIMIT", st, w, y, y0);
    w = P(7); st = adf_idele_log_abs(y, &w, X, ADF_REAL_PREC_MAX + 1); chk_adele("log_abs prec=MAX+1", st, w, y, y0);
    w = P(7); st = adf_idele_Log_at(s, &w, X, adf_place_inf(), ADF_REAL_PREC_MAX + 1); chk_sball("Log_at inf MAX+1", st, w, s, s0);
    w = P(7); st = adf_idele_log_abs_at(s, &w, X, adf_place_inf(), ADF_REAL_PREC_MAX + 1); chk_sball("log_abs_at inf MAX+1", st, w, s, s0);
    w = P(7); st = adf_idele_log_abs_at(s, &w, X, P(3), ADF_REAL_PREC_MAX + 1); chk_sball("log_abs_at 3 MAX+1 (UNSUPP)", st, w, s, s0);
    w = P(7); st = adf_idele_log_abs_at(s, &w, X, P(3), 5); chk_sball("log_abs_at 3 (UNSUPP,w=3)", st, w, s, s0);
    w = P(7); st = adf_idele_Log_at(s, &w, X, P(3), 3000000); chk_sball("Log_at 3 prec 3e6, M=4 (OK)", st, w, s, s0);
    adf_sball_set(s, s0);
    /* prec 0, 1, -5 */
    w = P(7); st = adf_idele_Log(y, &w, X, 0); chk_adele("Log prec 0", st, w, y, y0); adf_adele_set(y, y0);
    w = P(7); st = adf_idele_Log(y, &w, X, -5); chk_adele("Log prec -5", st, w, y, y0); adf_adele_set(y, y0);
    /* exact unit, huge N */
    setx("(2 ; 3 * [-1])");
    w = P(7); st = adf_idele_Log_at(s, &w, X, P(2), 100000000L); chk_sball("exact [-1] r=3 2 N=1e8", st, w, s, s0);
    w = P(7); st = adf_idele_Log_at(s, &w, X, P(5), 1152921504606846976L); chk_sball("exact r=3 5 N=EXP_MAX", st, w, s, s0);
    w = P(7); st = adf_idele_Log_at(s, &w, X, P(5), 1152921504606846977L); chk_sball("exact r=3 5 N=EXP_MAX+1", st, w, s, s0);
    w = P(7); st = adf_idele_Log_at(s, &w, X, P(5), -1152921504606846977L); chk_sball("exact r=3 5 N=-EXP_MAX-1", st, w, s, s0);
    setx("(2 ; 5 * [-1])");
    w = P(7); st = adf_idele_Log_at(s, &w, X, P(5), 100000000000L); chk_sball("exact r=5 p=5 N=1e11 (exact zero)", st, w, s, s0);
    adf_sball_set(s, s0);
    w = P(7); st = adf_idele_Log_at(s, &w, X, P(5), 1152921504606846977L); chk_sball("exact r=5 p=5 N=EXP_MAX+1 (zero)", st, w, s, s0);
    adf_sball_set(s, s0);
    /* refine shape and precedence */
    adf_place_t pl[8];
    setx("(2 ; 3 * [1 mod 4])");
    pl[0] = P(3); pl[1] = P(5); pl[2] = P(3);
    w = P(7); st = adf_idele_Log_refine(y, &w, X, pl, 3, 5, 53); chk_adele("refine repeat 3", st, w, y, y0);
    pl[0] = P(5); pl[1] = adf_place_inf();
    w = P(7); st = adf_idele_Log_refine(y, &w, X, pl, 2, 5, 53); chk_adele("refine inf in list", st, w, y, y0);
    w = P(7); st = adf_idele_Log_refine(y, &w, X, pl, -1, 5, 53); chk_adele("refine n=-1 (DOMAIN, w untouched)", st, w, y, y0);
    w = P(7); st = adf_idele_Log_refine(y, &w, X, pl, -1, 5, ADF_REAL_PREC_MAX + 1); chk_adele("refine n=-1 prec MAX+1 (LIMIT,inf)", st, w, y, y0);
    w = P(7); st = adf_idele_Log_refine(y, &w, X, NULL, 65537, 5, 53); chk_adele("refine n=65537 (LIMIT, w untouched)", st, w, y, y0);
    w = P(7); st = adf_idele_Log_refine(y, &w, X, NULL, 0, 5, 53); chk_adele("refine n=0 NULL (OK)", st, w, y, y0); adf_adele_set(y, y0);
    pl[0] = P(3); pl[1] = P(3);
    setx("(-2 ; 3 * [1 mod 4])");
    w = P(7); st = adf_idele_Log_refine(y, &w, X, pl, 2, 5, 53); chk_adele("refine neg I + repeat 3", st, w, y, y0);
    pl[1] = P(5);
    w = P(7); st = adf_idele_Log_refine(y, &w, X, pl, 2, 5, 53); chk_adele("refine neg I (DOMAIN, inf)", st, w, y, y0);
    w = P(7); st = adf_idele_log_abs_refine(y, &w, X, pl, 2, 5, 53); chk_adele("log_abs_refine neg I (OK)", st, w, y, y0); adf_adele_set(y, y0);
    /* local limit + negative I: LIMIT should win (10>7) with where=5 */
    pl[0] = P(3); pl[1] = P(5);
    setx("(-2 ; 3 * [-1])");
    w = P(7); st = adf_idele_Log_refine(y, &w, X, pl, 2, 1152921504606846977L, 53); chk_adele("refine neg I + local LIMIT", st, w, y, y0);
    w = P(7); st = adf_idele_Log_refine(y, &w, X, pl, 2, 100000000L, 53); chk_adele("refine neg I + N=1e8 3,5 (agg LIMIT)", st, w, y, y0);
    pl[0] = P(5); pl[1] = P(3);
    w = P(7); st = adf_idele_Log_refine(y, &w, X, pl, 2, 1152921504606846977L, 53); chk_adele("refine unsorted 5,3 local LIMIT", st, w, y, y0);
    return 0;
}
