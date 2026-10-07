#define _POSIX_C_SOURCE 200809L
/* Harness of lane q-review3 (Claude Opus). Reads cases, calls adf_qclass_reduce, prints the input
   as stored and every stored output piece exactly. Lane-private.

   Input, one case per line:
     C <form> <limit> <prec> <digits> <alias> <n> then n times
       <mid_mant> <mid_exp> <rad_mant> <rad_exp> <cnum> <cden> <Nnum> <Nden> <bk>
   form: 0 LIFT (n = 1), 1 PIECES (struct written directly; must be canonical).
   Real ball: mid_mant 2^mid_exp +/- rad_mant 2^rad_exp. Finite: cnum/cden + (Nnum/Nden) Zhat.
   bk = 1: converted to the local context with blocks 8, 9, 5 (K = 360).
   alias = 1: also call reduce(x, x) on a copy and compare with the non-aliased result.
   digits > 0: print the value text of the result with adf_qclass_get_str.

   Output per case:
     I <mid_num> <mid_den> <rad_num> <rad_den> <A> <H> <d>     (input as stored, n lines)
     R <status> <form> <len> <canonical> <untouched_on_status> <alias_ok> <usec>
     S <mid_num> <mid_den> <rad_num> <rad_den> <A> <H> <d>     (len lines, status OK)
     T <text>                                                  (digits > 0, status OK)
   A malformed or refused input prints "X <reason>". */
#include <adelefeld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static char *tok(char **p)
{
    char *s = *p, *q;
    while (*s == ' ' || *s == '\t' || *s == '\n') s++;
    if (!*s) { *p = s; return NULL; }
    q = s;
    while (*q && *q != ' ' && *q != '\t' && *q != '\n') q++;
    if (*q) { *q = 0; q++; }
    *p = q;
    return s;
}

static void pq(const fmpq_t q)
{
    fmpz_print(fmpq_numref(q)); putchar(' ');
    fmpz_print(fmpq_denref(q)); putchar(' ');
}

static void print_adele(const char *tag, const adf_adele_struct *a)
{
    fmpq_t q; fmpz_t A, H, d;
    fmpq_init(q); fmpz_init(A); fmpz_init(H); fmpz_init(d);
    printf("%s ", tag);
    arf_get_fmpq(q, arb_midref(a->inf)); pq(q);
    mag_get_fmpq(q, arb_radref(a->inf)); pq(q);
    adf_fball_get_fmpz3(A, H, d, &a->fin);
    fmpz_print(A); putchar(' '); fmpz_print(H); putchar(' '); fmpz_print(d); putchar('\n');
    fmpq_clear(q); fmpz_clear(A); fmpz_clear(H); fmpz_clear(d);
}

static double now_us(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1e6 + t.tv_nsec / 1e3;
}

static char line[1 << 22];

int main(void)
{
    adf_modctx_struct *ctx = NULL;
    ulong blocks[3] = {8, 9, 5};
    if (adf_modctx_new_blocks(&ctx, blocks, 3) != ADF_OK) return 2;
    while (fgets(line, sizeof(line), stdin)) {
        char *p = line, *t;
        slong limit, prec, digits, n, i;
        int form, alias, st, bad = 0;
        adf_qclass_t x, y, sent;
        adf_adele_t s;
        double t0, t1;
        t = tok(&p);
        if (!t) continue;
        if (strcmp(t, "C")) { printf("X token\n"); continue; }
        form = atoi(tok(&p));
        limit = strtol(tok(&p), NULL, 10);
        prec = strtol(tok(&p), NULL, 10);
        digits = strtol(tok(&p), NULL, 10);
        alias = atoi(tok(&p));
        n = strtol(tok(&p), NULL, 10);
        adf_qclass_init(x); adf_qclass_init(y); adf_qclass_init(sent);
        adf_adele_init(s);
        {
            const char *g = "(-13.75 +/- 0.03125 ; 5/7 mod 9)";
            adf_adele_set_str(s, g, strlen(g), 128, NULL);
            adf_qclass_set_adele(sent, s);
            adf_qclass_set(y, sent);
        }
        if (form == 1) {
            adf_qclass_clear(x);
            x->form = ADF_QCLASS_PIECES; x->len = n;
            x->piece = flint_malloc((size_t) n * sizeof(*x->piece));
            for (i = 0; i < n; i++) adf_adele_init(x->piece + i);
        }
        for (i = 0; i < n; i++) {
            fmpz_t mm, me, rm, re, cn, cd, nn, nd;
            arf_t r;
            adf_rat_t c, N;
            int bk;
            fmpz_init(mm); fmpz_init(me); fmpz_init(rm); fmpz_init(re);
            fmpz_init(cn); fmpz_init(cd); fmpz_init(nn); fmpz_init(nd);
            fmpz_set_str(mm, tok(&p), 10); fmpz_set_str(me, tok(&p), 10);
            fmpz_set_str(rm, tok(&p), 10); fmpz_set_str(re, tok(&p), 10);
            fmpz_set_str(cn, tok(&p), 10); fmpz_set_str(cd, tok(&p), 10);
            fmpz_set_str(nn, tok(&p), 10); fmpz_set_str(nd, tok(&p), 10);
            bk = atoi(tok(&p));
            arf_init(r);
            arf_set_fmpz_2exp(arb_midref(x->piece[i].inf), mm, me);
            arf_set_fmpz_2exp(r, rm, re);
            /* exact radius: mantissa below 2^30 (checked), mag_set_ui_2exp_si is exact then */
            if (fmpz_sgn(rm) < 0 || fmpz_bits(rm) > 30 || !fmpz_fits_si(re)) bad = 5;
            else mag_set_ui_2exp_si(arb_radref(x->piece[i].inf), fmpz_get_ui(rm), fmpz_get_si(re));
            {
                fmpq_t q1, q2; fmpq_init(q1); fmpq_init(q2);
                arf_get_fmpq(q1, r); mag_get_fmpq(q2, arb_radref(x->piece[i].inf));
                if (!bad && !fmpq_equal(q1, q2)) bad = 6;
                fmpq_clear(q1); fmpq_clear(q2);
            }
            adf_rat_init(c); adf_rat_init(N);
            adf_rat_set_fmpz2(c, cn, cd);
            adf_rat_set_fmpz2(N, nn, nd);
            if (adf_fball_set_center_radius(&x->piece[i].fin, c, N) != ADF_OK) bad = 1;
            if (!bad && bk == 1 &&
                adf_fball_set_local(&x->piece[i].fin, &x->piece[i].fin, ctx) != ADF_OK) bad = 2;
            if (!bad && !adf_adele_is_canonical(x->piece + i)) bad = 3;
            adf_rat_clear(c); adf_rat_clear(N); arf_clear(r);
            fmpz_clear(mm); fmpz_clear(me); fmpz_clear(rm); fmpz_clear(re);
            fmpz_clear(cn); fmpz_clear(cd); fmpz_clear(nn); fmpz_clear(nd);
        }
        if (!bad && !adf_qclass_is_canonical(x)) bad = 4;
        if (bad) {
            printf("X bad %d\n", bad);
        } else {
            int untouched = 1, alias_ok = 1;
            for (i = 0; i < n; i++) print_adele("I", x->piece + i);
            t0 = now_us();
            st = adf_qclass_reduce(y, x, limit, prec);
            t1 = now_us();
            if (st != ADF_OK) untouched = adf_qclass_identical(y, sent);
            if (alias) {
                adf_qclass_t z;
                int st2;
                adf_qclass_init(z);
                adf_qclass_set(z, x);
                st2 = adf_qclass_reduce(z, z, limit, prec);
                if (st2 != st) alias_ok = 0;
                else if (st == ADF_OK) alias_ok = adf_qclass_identical(z, y);
                else alias_ok = adf_qclass_identical(z, x);
                adf_qclass_clear(z);
            }
            printf("R %d %d %ld %d %d %d %.0f\n", st, y->form, (long) y->len,
                   adf_qclass_is_canonical(y), untouched, alias_ok, t1 - t0);
            if (st == ADF_OK) {
                for (i = 0; i < y->len; i++) print_adele("S", y->piece + i);
                if (digits > 0) {
                    size_t len;
                    char *txt = adf_qclass_get_str(&len, y, digits);
                    if (txt) { printf("T %s\n", txt); adf_str_free(txt); }
                    else printf("T NULL\n");
                }
            }
        }
        fflush(stdout);
        adf_qclass_clear(x); adf_qclass_clear(y); adf_qclass_clear(sent); adf_adele_clear(s);
    }
    adf_modctx_free(ctx);
    flint_cleanup_master();
    return 0;
}
