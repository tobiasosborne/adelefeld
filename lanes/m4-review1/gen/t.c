/* m4-review1 text/dump harness with interposed FLINT readers.
   Lines: "ftext <prec> <digits> <text>" / "rtext ..." : set_str, then get_str; prints status and the text.
          "fdump <ffun text>" / "rdump <rfun text>"     : set_str at 128, dump; prints the dump.
          "fload <bytes>" / "rload <bytes>"              : load into a sentinel; prints status, U/W, the redump,
                                                           and the counts of interposed calls during the load. */
#define _GNU_SOURCE
#include <adelefeld.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static long n_arb_load, n_arb_set_str, n_fmpz_set_str, n_bad_fmpz_str;
int arb_load_str(arb_t x, const char *s) { (void) x; (void) s; n_arb_load++; abort(); }
int arb_set_str(arb_t x, const char *s, slong p) { (void) x; (void) s; (void) p; n_arb_set_str++; return 1; }
int fmpz_set_str(fmpz_t z, const char *s, int b)
{
    static int (*real)(fmpz_t, const char *, int);
    if (!real) real = (int (*)(fmpz_t, const char *, int)) dlsym(RTLD_NEXT, "fmpz_set_str");
    n_fmpz_set_str++;
    for (const char *p = s; *p; p++)
        if (!((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') || *p == '-')) { n_bad_fmpz_str++; break; }
    return real(z, s, b);
}

static void sent_f(adf_ffun_t f) { acb_t v; acb_init(v); acb_set_si(v, 7); adf_ffun_set_acb_vec(f, 1, 1, v, 1); acb_clear(v); }
static void sent_r(adf_rfun_t r)
{
    adf_rterm_struct t; acb_poly_init(t.P); acb_init(t.A); acb_init(t.B); acb_init(t.C);
    acb_poly_set_si(t.P, 7); acb_set_si(t.A, 3);
    adf_rfun_set_terms(r, &t, 1);
    acb_poly_clear(t.P); acb_clear(t.A); acb_clear(t.B); acb_clear(t.C);
}

int main(void)
{
    static char line[1 << 24];
    adf_text_limits_t lim;
    adf_text_limits_default(&lim);
    while (fgets(line, sizeof line, stdin)) {
        size_t n = strlen(line);
        if (n && line[n - 1] == '\n') line[--n] = 0;
        char *sp = strchr(line, ' ');
        if (!sp) continue;
        *sp = 0;
        char *arg = sp + 1;
        size_t len = 0;
        if (!strcmp(line, "ftext") || !strcmp(line, "rtext")) {
            long prec, digits; int off;
            sscanf(arg, "%ld %ld %n", &prec, &digits, &off);
            char *s = arg + off;
            int st;
            char *out = NULL;
            if (line[0] == 'f') { adf_ffun_t f; adf_ffun_init(f); st = adf_ffun_set_str(f, s, strlen(s), prec, &lim); if (!st) out = adf_ffun_get_str(&len, f, digits); adf_ffun_clear(f); }
            else { adf_rfun_t r; adf_rfun_init(r); st = adf_rfun_set_str(r, s, strlen(s), prec, &lim); if (!st) out = adf_rfun_get_str(&len, r, digits); adf_rfun_clear(r); }
            printf("%d\t%s\n", st, out ? out : "-");
            if (out) adf_str_free(out);
        } else if (!strcmp(line, "fdump") || !strcmp(line, "rdump")) {
            int st; char *out = NULL;
            if (line[0] == 'f') { adf_ffun_t f; adf_ffun_init(f); st = adf_ffun_set_str(f, arg, strlen(arg), 128, &lim); if (!st) out = adf_ffun_dump_str(&len, f); adf_ffun_clear(f); }
            else { adf_rfun_t r; adf_rfun_init(r); st = adf_rfun_set_str(r, arg, strlen(arg), 128, &lim); if (!st) out = adf_rfun_dump_str(&len, r); adf_rfun_clear(r); }
            printf("%d\t%s\n", st, out ? out : "-");
            if (out) adf_str_free(out);
        } else if (!strcmp(line, "fload") || !strcmp(line, "rload")) {
            long a0 = n_arb_load + n_arb_set_str, f0 = n_fmpz_set_str, b0 = n_bad_fmpz_str;
            int st, untouched = 0, st2; size_t nctx = 99; char *out = NULL;
            if (line[0] == 'f') {
                adf_ffun_t f, s0; adf_ffun_init(f); adf_ffun_init(s0); sent_f(f); sent_f(s0);
                st = adf_ffun_load_str(f, arg, strlen(arg), NULL, &lim);
                untouched = adf_ffun_identical(f, s0);
                if (!st) out = adf_ffun_dump_str(&len, f);
                st2 = adf_ffun_dump_inspect(&nctx, NULL, arg, strlen(arg), &lim);
                adf_ffun_clear(f); adf_ffun_clear(s0);
            } else {
                adf_rfun_t f, s0; adf_rfun_init(f); adf_rfun_init(s0); sent_r(f); sent_r(s0);
                st = adf_rfun_load_str(f, arg, strlen(arg), NULL, &lim);
                untouched = adf_rfun_identical(f, s0);
                if (!st) out = adf_rfun_dump_str(&len, f);
                st2 = adf_rfun_dump_inspect(&nctx, NULL, arg, strlen(arg), &lim);
                adf_rfun_clear(f); adf_rfun_clear(s0);
            }
            printf("%d\t%s\t%d\t%zu\t%ld\t%ld\t%ld\t%s\n", st, untouched ? "U" : "W", st2, nctx,
                   n_arb_load + n_arb_set_str - a0, n_fmpz_set_str - f0, n_bad_fmpz_str - b0, out ? out : "-");
            if (out) adf_str_free(out);
        } else printf("BAD\n");
        fflush(stdout);
    }
    flint_cleanup_master();
    return 0;
}
