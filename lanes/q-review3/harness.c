/* Harness of lane q-review3: read a lift, call adf_qclass_reduce, print every stored
   piece exactly. Lane-private; nothing of the repository is changed.

   Input, one case per line (tokens separated by blanks):
     L <piece_limit> <prec> <n>
     then n times:
     P <mid_mant> <mid_exp> <rad_mant> <rad_exp> <c_num> <c_den> <N_num> <N_den> <backend>
       <k> <b_1> ... <b_k> <r_1> ... <r_k>
   The real ball is mid_mant * 2^mid_exp  +/-  rad_mant * 2^rad_exp (rad_mant >= 0).
   The finite ball is centre c_num/c_den and radius N_num/N_den.
   backend 0: global.  backend 1: convert to the context with blocks b (k of them).

   Output, for one case:
     I <mid_num> <mid_den> <rad_num> <rad_den> <A> <H> <d>      (n lines, the input as stored)
     <status> <form> <len> <canonical> <untouched>
     S <mid_num> <mid_den> <rad_num> <rad_den> <A> <H> <d>      (len lines, if status OK)

   Magnitudes and arf values are printed through mag_get_fmpq and arf_get_fmpq, never
   through the raw fields: in FLINT 3.0.1 the field exponent of mag and arf is the binary
   point position, not the exponent of the mantissa (probes probe2, probe4, probe6).

   All integers are decimal and may be of any length.
*/
#include <adelefeld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static void setz(fmpz_t z, char *s) { fmpz_set_str(z, s, 10); }

static void print_q(const fmpq_t q)
{
    fmpz_fprint(stdout, fmpq_numref(q)); putchar(' ');
    fmpz_fprint(stdout, fmpq_denref(q)); putchar(' ');
}

static void print_z(const fmpz_t z) { fmpz_fprint(stdout, z); putchar(' '); }

int main(void)
{
    char line[1 << 20];
    adf_modctx_struct *ctx = NULL;
    ulong blocks[16];
    while (fgets(line, sizeof(line), stdin)) {
        char *p = line, *t;
        slong limit, prec, n, i;
        int st;
        adf_qclass_t x, y, sent;
        adf_adele_t s;
        fmpq_t fq;
        t = tok(&p);
        if (!t) continue;
        if (strcmp(t, "L") != 0) { fprintf(stderr, "bad token %s\n", t); return 2; }
        limit = strtol(tok(&p), NULL, 10);
        prec = strtol(tok(&p), NULL, 10);
        n = strtol(tok(&p), NULL, 10);
        fmpq_init(fq);
        adf_qclass_init(x); adf_qclass_init(y); adf_qclass_init(sent);
        adf_adele_init(s);
        {
            const char *g = "(-13.75 +/- 0.03125 ; 5/7 mod 9)";
            adf_adele_set_str(s, g, strlen(g), 128, NULL);
        }
        adf_qclass_set_adele(sent, s);
        if (n < 1) { fprintf(stderr, "n<1\n"); return 2; }
        if (n > 1) {
            adf_qclass_clear(x);
            x->form = ADF_QCLASS_PIECES;
            x->len = n;
            x->piece = flint_malloc((size_t) n * sizeof(*x->piece));
            for (i = 0; i < n; i++) adf_adele_init(x->piece + i);
        }
        for (i = 0; i < n; i++) {
            fmpz_t mm, me, rm, re, cn, cd, nn, nd;
            arf_t mid, rad;
            adf_rat_t c, N;
            int backend, k;
            t = tok(&p);
            if (!t || strcmp(t, "P") != 0) { fprintf(stderr, "want P\n"); return 2; }
            fmpz_init(mm); fmpz_init(me); fmpz_init(rm); fmpz_init(re);
            fmpz_init(cn); fmpz_init(cd); fmpz_init(nn); fmpz_init(nd);
            setz(mm, tok(&p)); setz(me, tok(&p)); setz(rm, tok(&p)); setz(re, tok(&p));
            setz(cn, tok(&p)); setz(cd, tok(&p)); setz(nn, tok(&p)); setz(nd, tok(&p));
            arf_init(mid); arf_init(rad);
            arf_set_fmpz_2exp(mid, mm, me);
            arf_set_fmpz_2exp(rad, rm, re);
            arf_set(arb_midref(x->piece[i].inf), mid);
            arf_get_mag(arb_radref(x->piece[i].inf), rad);
            adf_rat_init(c); adf_rat_init(N);
            adf_rat_set_fmpz2(c, cn, cd);
            adf_rat_set_fmpz2(N, nn, nd);
            if (adf_fball_set_center_radius(&x->piece[i].fin, c, N) != ADF_OK) {
                fprintf(stderr, "bad finite ball\n"); return 2;
            }
            backend = (int) strtol(tok(&p), NULL, 10);
            k = (int) strtol(tok(&p), NULL, 10);
            if (backend == 1) {
                slong j;
                adf_modctx_struct *c2 = NULL;
                if (k > 16) { fprintf(stderr, "k too big\n"); return 2; }
                for (j = 0; j < k; j++) blocks[j] = strtoul(tok(&p), NULL, 10);
                for (j = 0; j < k; j++) (void) strtoul(tok(&p), NULL, 10);
                if (adf_modctx_new_blocks(&c2, blocks, k) != ADF_OK) { fprintf(stderr, "ctx\n"); return 2; }
                if (adf_fball_set_local(&x->piece[i].fin, &x->piece[i].fin, c2) != ADF_OK) {
                    fprintf(stderr, "not local: refused\n"); return 3;
                }
                if (ctx) adf_modctx_free(ctx);
                ctx = c2;
            }
            if (!adf_adele_is_canonical(x->piece + i)) { fprintf(stderr, "piece not canonical\n"); return 2; }
            adf_rat_clear(c); adf_rat_clear(N);
            arf_clear(mid); arf_clear(rad);
            fmpz_clear(mm); fmpz_clear(me); fmpz_clear(rm); fmpz_clear(re);
            fmpz_clear(cn); fmpz_clear(cd); fmpz_clear(nn); fmpz_clear(nd);
        }
        st = adf_qclass_reduce(y, x, limit, prec);
        for (i = 0; i < n; i++) {
            fmpz_t A, H, d;
            fmpz_init(A); fmpz_init(H); fmpz_init(d);
            adf_fball_get_fmpz3(A, H, d, &x->piece[i].fin);
            printf("I ");
            arf_get_fmpq(fq, arb_midref(x->piece[i].inf)); print_q(fq);
            mag_get_fmpq(fq, arb_radref(x->piece[i].inf)); print_q(fq);
            print_z(A); print_z(H); print_z(d); printf("\n");
            fmpz_clear(A); fmpz_clear(H); fmpz_clear(d);
        }
        printf("%d %d %ld %d %d\n", st, y->form, (long) y->len,
               adf_qclass_is_canonical(y), adf_qclass_identical(y, sent) ? 1 : 0);
        if (st == ADF_OK) {
            for (i = 0; i < y->len; i++) {
                fmpz_t A, H, d;
                fmpz_init(A); fmpz_init(H); fmpz_init(d);
                adf_fball_get_fmpz3(A, H, d, &y->piece[i].fin);
                printf("S ");
                arf_get_fmpq(fq, arb_midref(y->piece[i].inf)); print_q(fq);
                mag_get_fmpq(fq, arb_radref(y->piece[i].inf)); print_q(fq);
                print_z(A); print_z(H); print_z(d); printf("\n");
                fmpz_clear(A); fmpz_clear(H); fmpz_clear(d);
            }
        }
        fflush(stdout);
        adf_qclass_clear(x); adf_qclass_clear(y); adf_qclass_clear(sent); adf_adele_clear(s);
        fmpq_clear(fq);
    }
    if (ctx) adf_modctx_free(ctx);
    return 0;
}