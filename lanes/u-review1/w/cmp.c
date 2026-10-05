/* a loaded lball / sball against the same set built by the public constructors; identical? */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "adelefeld/dump.h"

static int build_lb(adf_lball_t x, char **t)  /* t: p form a b c */
{
    fmpz_t a, b, c, pw, P; fmpq_t q; adf_rat_t r; adf_place_t pl; int st; ulong p = strtoull(t[0], 0, 16);
    fmpz_init(a); fmpz_init(b); fmpz_init(c); fmpz_init(pw); fmpz_init(P); fmpq_init(q); adf_rat_init(r);
    fmpz_set_str(a, t[2], 16); fmpz_set_str(b, t[3], 16); fmpz_set_str(c, t[4], 16);
    adf_place_prime(&pl, p); fmpz_set_ui(P, p);
    if (t[1][0] == 'x') {                 /* q = p^v num/den, v = c */
        slong v = fmpz_get_si(c);
        if (labs(v) > 20000) { st = 99; goto out; }
        fmpz_pow_ui(pw, P, (ulong) labs(v));
        fmpz_set(fmpq_numref(q), a); fmpz_set(fmpq_denref(q), b);
        if (v >= 0) fmpq_mul_fmpz(q, q, pw); else fmpq_div_fmpz(q, q, pw);
        adf_rat_set_fmpq(r, q);
        st = adf_lball_set_rat(x, pl, r);
    } else {                              /* ball u v N: centre p^v u */
        slong v = fmpz_get_si(b), N = fmpz_get_si(c);
        if (labs(v) > 20000 || labs(N) > 20000) { st = 99; goto out; }
        fmpz_pow_ui(pw, P, (ulong) labs(v));
        fmpz_set(fmpq_numref(q), a); fmpz_one(fmpq_denref(q));
        if (v >= 0) fmpq_mul_fmpz(q, q, pw); else fmpq_div_fmpz(q, q, pw);
        adf_rat_set_fmpq(r, q);
        st = adf_lball_set_rat_ball(x, pl, r, N);
    }
out:
    fmpz_clear(a); fmpz_clear(b); fmpz_clear(c); fmpz_clear(pw); fmpz_clear(P); fmpq_clear(q); adf_rat_clear(r);
    return st;
}

int main(void)
{
    char *line = NULL; size_t cap = 0; ssize_t n; long cnt = 0, skip = 0, diff = 0, bstat = 0;
    adf_text_limits_t lim = {1u << 30, 100000, LONG_MAX, 1 << 30};
    while ((n = getline(&line, &cap, stdin)) > 0) {
        char *tok[400]; int nt = 0; char *cp, *s0;
        if (line[n - 1] == '\n') line[--n] = 0;
        s0 = strdup(line + 6);
        memmove(line, line + 6, strlen(line + 6) + 1);
        for (cp = strtok(line, " "); cp && nt < 400; cp = strtok(NULL, " ")) tok[nt++] = cp;
        if (!strcmp(tok[2], "lball")) {
            adf_lball_t x, y; int st1, st2;
            adf_lball_init(x); adf_lball_init(y);
            st1 = adf_lball_load_str(x, s0, strlen(s0), NULL, &lim);
            if (st1) { skip++; goto nxt1; }
            st2 = build_lb(y, tok + 3);
            if (st2 == 99) skip++;
            else if (st2) { bstat++; printf("CONSTRUCTOR STATUS %d for %s\n", st2, s0); }
            else { cnt++; if (!adf_lball_identical(x, y)) { diff++; printf("NOT IDENTICAL %s\n", s0); } }
nxt1:       adf_lball_clear(x); adf_lball_clear(y);
        } else {
            adf_sball_t x, y; int st1, st2, i, k = 4, ok = 1; arb_t r; arb_t *rp = NULL; slong cntp; adf_lball_struct *loc; adf_place_t wh;
            adf_sball_init(x); adf_sball_init(y); arb_init(r);
            st1 = adf_sball_load_str(x, s0, strlen(s0), NULL, &lim);
            if (st1) { skip++; goto nxt2; }
            if (tok[3][0] == 'c') { skip++; goto nxt2; }
            if (tok[3][0] == 'r') {
                fmpz_t m, e, rm, re; mag_t mg; fmpz_init(m); fmpz_init(e); fmpz_init(rm); fmpz_init(re);
                fmpz_set_str(m, tok[4], 16); fmpz_set_str(e, tok[5], 16); fmpz_set_str(rm, tok[6], 16); fmpz_set_str(re, tok[7], 16);
                arf_set_fmpz_2exp(arb_midref(r), m, e); mag_init(mg); mag_set_ui_2exp_si(arb_radref(r), fmpz_get_ui(rm), fmpz_fits_si(re) ? fmpz_get_si(re) : 0);
                rp = &r; k = 8; mag_clear(mg); fmpz_clear(m); fmpz_clear(e); fmpz_clear(rm); fmpz_clear(re);
            }
            k = (tok[3][0] == 'n') ? 4 : 8;
            cntp = strtol(tok[k], 0, 16);
            loc = flint_malloc(sizeof(adf_lball_struct) * (cntp ? cntp : 1));
            for (i = 0; i < cntp; i++) {
                adf_lball_init(&loc[i]);
                st2 = build_lb(&loc[i], tok + k + 1 + 5 * i);
                if (st2) { ok = 0; skip++; break; }
            }
            if (ok) {
                st2 = adf_sball_set_arb_lballs(y, &wh, rp ? r : NULL, loc, cntp);
                if (st2) { bstat++; printf("CONSTRUCTOR STATUS %d for %s\n", st2, s0); }
                else { cnt++; if (!adf_sball_identical(x, y)) { diff++; size_t l1; char *d1 = adf_sball_dump_str(&l1, x); char *d2 = adf_sball_dump_str(&l1, y); printf("NOT IDENTICAL %s\n   loaded  %s\n   built   %s\n", s0, d1, d2); } }
            }
            for (i = 0; i < cntp; i++) if (i < 0) ;
            flint_free(loc);
nxt2:       adf_sball_clear(x); adf_sball_clear(y); arb_clear(r);
        }
        free(s0);
    }
    printf("compared %ld, skipped %ld, constructor refused %ld, not identical %ld\n", cnt, skip, bstat, diff);
    return 0;
}
