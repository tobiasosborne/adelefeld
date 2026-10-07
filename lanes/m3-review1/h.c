#define _POSIX_C_SOURCE 200809L
/* Harness of lane m3-review1 (Claude Opus). Lane-private. One command per line on stdin.
   Class:  form n piece*n ; piece: mm me rm re cn cd Nn Nd bk
     real ball mm 2^me +/- rm 2^re (rm < 2^30, exact); finite cn/cd + (Nn/Nd) Zhat;
     bk 0 global, 1 local ctx1 (blocks 8,9,5), 2 local ctx2 (blocks 7,11,16).
     form 0: LIFT of piece 0; form 1: adf_qclass_set_pieces(n, limit n).
   Commands:
     Q kind W X Y          set query (0 equal, 1 contains, 2 overlaps); prints "R st truth untouched"
     N lim prec X          neg; prints "R st len canon untouched alias" then S lines
     A lim prec X Y        add; also z=x=y aliasing on X; prints as N plus "W aliasstatus aliasidentical"
     P prec X              class psi default/strict/phase; for a LIFT also adele and fball calls
     T prec v X            psi_at / strict_at on piece 0 at place v (0 = inf)
     L prec p cn cd N ex   lball default/strict/phase
     U prec maxitems digits text   qclass_set_str, then get_str
     D X                   dump, load, identical, re-dump, inspect
     M nb text             load_str_binds with nb copies of ctx1 (nb<0: load_str ctx1), inspect
*/
#include <adelefeld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
/* Interposition (hunt 4): count FLINT string loaders called while g_on is set. */
static int g_on = 0; static long g_calls = 0;
int fmpz_set_str(fmpz_t f, const char *str, int b)
{
    static int (*real)(fmpz_t, const char *, int) = NULL;
    if (!real) *(void **) &real = dlsym(RTLD_NEXT, "fmpz_set_str");
    if (g_on) g_calls++;
    return real(f, str, b);
}
int arb_load_str(arb_t x, const char *str) { (void) x; (void) str; if (g_on) g_calls += 1000; return 1; }
int arf_load_str(arf_t x, const char *str) { (void) x; (void) str; if (g_on) g_calls += 1000; return 1; }
int mag_load_str(mag_t x, const char *str) { (void) x; (void) str; if (g_on) g_calls += 1000; return 1; }

static adf_modctx_struct *ctx1, *ctx2;
static char line[1 << 23];
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
static void pq(const fmpq_t q) { fmpz_print(fmpq_numref(q)); putchar(' '); fmpz_print(fmpq_denref(q)); putchar(' '); }
static void print_adele(const char *tag, const adf_adele_struct *a)
{
    fmpq_t q; fmpz_t A, H, d;
    fmpq_init(q); fmpz_init(A); fmpz_init(H); fmpz_init(d);
    printf("%s ", tag);
    arf_get_fmpq(q, arb_midref(a->inf)); pq(q);
    mag_get_fmpq(q, arb_radref(a->inf)); pq(q);
    adf_fball_get_fmpz3(A, H, d, &a->fin);
    fmpz_print(A); putchar(' '); fmpz_print(H); putchar(' '); fmpz_print(d);
    printf(" %d\n", a->fin.backend);
    fmpq_clear(q); fmpz_clear(A); fmpz_clear(H); fmpz_clear(d);
}
static void print_arb(const arb_t x)
{
    fmpq_t q; fmpq_init(q);
    if (!arb_is_finite(x)) { printf("inf 1 inf 1 "); fmpq_clear(q); return; }
    arf_get_fmpq(q, arb_midref(x)); pq(q);
    mag_get_fmpq(q, arb_radref(x)); pq(q);
    fmpq_clear(q);
}
static void print_acb(const char *tag, int st, const acb_t z, int untouched)
{
    printf("%s %d %d ", tag, st, untouched);
    if (st == ADF_OK) { print_arb(acb_realref(z)); print_arb(acb_imagref(z)); }
    putchar('\n');
}
/* returns 0 on success, else a code */
static int read_piece(adf_adele_struct *x, char **p)
{
    fmpz_t mm, me, rm, re, cn, cd, nn, nd; adf_rat_t c, N; int bk, bad = 0; char *t[9]; int i;
    for (i = 0; i < 9; i++) { t[i] = tok(p); if (!t[i]) return 9; }
    fmpz_init(mm); fmpz_init(me); fmpz_init(rm); fmpz_init(re);
    fmpz_init(cn); fmpz_init(cd); fmpz_init(nn); fmpz_init(nd);
    fmpz_set_str(mm, t[0], 10); fmpz_set_str(me, t[1], 10); fmpz_set_str(rm, t[2], 10);
    fmpz_set_str(re, t[3], 10); fmpz_set_str(cn, t[4], 10); fmpz_set_str(cd, t[5], 10);
    fmpz_set_str(nn, t[6], 10); fmpz_set_str(nd, t[7], 10); bk = atoi(t[8]);
    arf_set_fmpz_2exp(arb_midref(x->inf), mm, me);
    if (fmpz_sgn(rm) < 0 || fmpz_bits(rm) > 30) bad = 5;
    else if (fmpz_is_zero(rm)) mag_zero(arb_radref(x->inf));
    else {
        arf_t r; fmpq_t a, b; arf_init(r); fmpq_init(a); fmpq_init(b);
        arf_set_fmpz_2exp(r, rm, re);
        mag_set_ui_2exp_si(arb_radref(x->inf), fmpz_get_ui(rm), fmpz_get_si(re));
        arf_get_fmpq(a, r); mag_get_fmpq(b, arb_radref(x->inf));
        if (!fmpq_equal(a, b)) bad = 6;
        arf_clear(r); fmpq_clear(a); fmpq_clear(b);
    }
    adf_rat_init(c); adf_rat_init(N);
    adf_rat_set_fmpz2(c, cn, cd); adf_rat_set_fmpz2(N, nn, nd);
    if (!bad && adf_fball_set_center_radius(&x->fin, c, N) != ADF_OK) bad = 1;
    if (!bad && bk && adf_fball_set_local(&x->fin, &x->fin, bk == 1 ? ctx1 : ctx2) != ADF_OK) bad = 2;
    if (!bad && !adf_adele_is_canonical(x)) bad = 3;
    adf_rat_clear(c); adf_rat_clear(N);
    fmpz_clear(mm); fmpz_clear(me); fmpz_clear(rm); fmpz_clear(re);
    fmpz_clear(cn); fmpz_clear(cd); fmpz_clear(nn); fmpz_clear(nd);
    return bad;
}
static int read_class(adf_qclass_t x, char **p)
{
    char *t = tok(p); int form, bad = 0, st; slong n, i; adf_adele_struct *arr;
    if (!t) return 9;
    form = atoi(t); n = strtol(tok(p), NULL, 10);
    arr = flint_malloc(n * sizeof(*arr));
    for (i = 0; i < n; i++) adf_adele_init(arr + i);
    for (i = 0; i < n && !bad; i++) bad = read_piece(arr + i, p);
    if (!bad) {
        if (form == 0) adf_qclass_set_adele(x, arr);
        else if ((st = adf_qclass_set_pieces(x, arr, n, n)) != ADF_OK) bad = 100 + st;
    }
    if (!bad && !adf_qclass_is_canonical(x)) bad = 4;
    for (i = 0; i < n; i++) adf_adele_clear(arr + i);
    flint_free(arr);
    return bad;
}
static void sentinel(adf_qclass_t s)
{
    adf_adele_t a; const char *g = "(-13.75 +/- 0.03125 ; 5/7 mod 9)";
    adf_adele_init(a); adf_adele_set_str(a, g, strlen(g), 128, NULL);
    adf_qclass_set_adele(s, a); adf_adele_clear(a);
}
static void acb_sentinel(acb_t z)
{
    arb_set_d(acb_realref(z), 123.25); arb_set_d(acb_imagref(z), -7.5);
    mag_set_ui_2exp_si(arb_radref(acb_realref(z)), 3, -5);
}
static int acb_same(const acb_t a, const acb_t b) { return acb_equal(a, b); }
static void print_class(const adf_qclass_t y)
{
    slong i; for (i = 0; i < y->len; i++) print_adele("S", y->piece + i);
}

int main(void)
{
    ulong b1[3] = {8, 9, 5}, b2[3] = {7, 11, 16};
    adf_text_limits_t lim;
    if (adf_modctx_new_blocks(&ctx1, b1, 3) != ADF_OK) return 2;
    if (adf_modctx_new_blocks(&ctx2, b2, 3) != ADF_OK) return 2;
    while (fgets(line, sizeof(line), stdin)) {
        char *p = line, *c = tok(&p);
        adf_qclass_t x, y, z, s;
        int bad = 0, st;
        if (!c) continue;
        adf_qclass_init(x); adf_qclass_init(y); adf_qclass_init(z); adf_qclass_init(s);
        sentinel(s);
        if (c[0] == 'Q') {
            int kind = atoi(tok(&p)); slong W = strtol(tok(&p), NULL, 10); int truth = 7;
            bad = read_class(x, &p); if (!bad) bad = read_class(y, &p);
            if (bad) printf("X %d\n", bad);
            else {
                st = kind == 0 ? adf_qclass_equal_set(&truth, x, y, W) :
                     kind == 1 ? adf_qclass_contains(&truth, x, y, W) : adf_qclass_overlaps(&truth, x, y, W);
                printf("R %d %d %d\n", st, truth, st == ADF_OK ? 1 : truth == 7);
            }
        } else if (c[0] == 'N' || c[0] == 'A') {
            slong lim_ = strtol(tok(&p), NULL, 10), prec = strtol(tok(&p), NULL, 10);
            bad = read_class(x, &p); if (!bad && c[0] == 'A') bad = read_class(y, &p);
            if (bad) printf("X %d\n", bad);
            else {
                int al = 1, st2;
                adf_qclass_set(z, s);
                st = c[0] == 'N' ? adf_qclass_neg(z, x, lim_, prec) : adf_qclass_add(z, x, y, lim_, prec);
                {   /* alias: y=x for neg; z=x for add */
                    adf_qclass_t w; adf_qclass_init(w); adf_qclass_set(w, x);
                    st2 = c[0] == 'N' ? adf_qclass_neg(w, w, lim_, prec) : adf_qclass_add(w, w, y, lim_, prec);
                    al = st2 == st && (st == ADF_OK ? adf_qclass_identical(w, z) : adf_qclass_identical(w, x));
                    adf_qclass_clear(w);
                }
                printf("R %d %ld %d %d %d\n", st, (long) z->len, adf_qclass_is_canonical(z),
                       st == ADF_OK ? 1 : adf_qclass_identical(z, s), al);
                if (st == ADF_OK) print_class(z);
                if (c[0] == 'A') {  /* z = x = y */
                    adf_qclass_t w, v; adf_qclass_init(w); adf_qclass_init(v);
                    adf_qclass_set(w, x);
                    st2 = adf_qclass_add(v, x, x, lim_, prec);
                    st = adf_qclass_add(w, w, w, lim_, prec);
                    printf("W %d %d %d\n", st, st2, st == st2 && (st != ADF_OK || adf_qclass_identical(w, v)));
                    if (st == ADF_OK) print_class(w);
                    adf_qclass_clear(w); adf_qclass_clear(v);
                }
            }
        } else if (c[0] == 'P') {
            slong prec = strtol(tok(&p), NULL, 10); acb_t a, sn; fmpq_t th, ths;
            acb_init(a); acb_init(sn); fmpq_init(th); fmpq_init(ths);
            acb_sentinel(sn); fmpq_set_si(ths, 5, 7);
            bad = read_class(x, &p);
            if (bad) printf("X %d\n", bad);
            else {
                acb_set(a, sn); st = adf_qclass_psi_tate(a, x, prec); print_acb("C", st, a, st == ADF_OK || acb_same(a, sn));
                acb_set(a, sn); st = adf_qclass_psi_tate_strict(a, x, prec); print_acb("K", st, a, st == ADF_OK || acb_same(a, sn));
                fmpq_set(th, ths); st = adf_qclass_psi_tate_phase(th, x);
                printf("H %d %d ", st, st == ADF_OK || fmpq_equal(th, ths)); if (st == ADF_OK) pq(th); putchar('\n');
                if (x->form == ADF_QCLASS_LIFT) {
                    acb_set(a, sn); st = adf_adele_psi_tate(a, x->piece, prec); print_acb("D", st, a, st == ADF_OK || acb_same(a, sn));
                    acb_set(a, sn); st = adf_adele_psi_tate_strict(a, x->piece, prec); print_acb("E", st, a, st == ADF_OK || acb_same(a, sn));
                    fmpq_set(th, ths); st = adf_adele_psi_tate_phase(th, x->piece);
                    printf("G %d %d ", st, st == ADF_OK || fmpq_equal(th, ths)); if (st == ADF_OK) pq(th); putchar('\n');
                    fmpq_set(th, ths); st = adf_fball_psi_tate_phase(th, &x->piece->fin);
                    printf("F %d %d ", st, st == ADF_OK || fmpq_equal(th, ths)); if (st == ADF_OK) pq(th); putchar('\n');
                }
            }
            acb_clear(a); acb_clear(sn); fmpq_clear(th); fmpq_clear(ths);
        } else if (c[0] == 'T') {
            slong prec = strtol(tok(&p), NULL, 10); ulong pr = strtoul(tok(&p), NULL, 10);
            adf_place_t v, w0, w; acb_t a, sn; acb_init(a); acb_init(sn); acb_sentinel(sn);
            if (pr == 0) v = adf_place_inf(); else adf_place_prime(&v, pr);
            adf_place_prime(&w0, 3);
            bad = read_class(x, &p);
            if (bad) printf("X %d\n", bad);
            else {
                int k;
                for (k = 0; k < 2; k++) {
                    w = w0; acb_set(a, sn);
                    st = k ? adf_adele_psi_tate_strict_at(a, &w, x->piece, v, prec)
                           : adf_adele_psi_tate_at(a, &w, x->piece, v, prec);
                    printf("%c %d %d %d ", k ? 'J' : 'I', st,
                           st == ADF_OK ? acb_same(a, a) : acb_same(a, sn),
                           st == ADF_OK ? adf_place_equal(w, w0) : adf_place_equal(w, v));
                    if (st == ADF_OK) { print_arb(acb_realref(a)); print_arb(acb_imagref(a)); }
                    putchar('\n');
                }
            }
            acb_clear(a); acb_clear(sn);
        } else if (c[0] == 'L') {
            slong prec = strtol(tok(&p), NULL, 10); ulong pr = strtoul(tok(&p), NULL, 10);
            fmpz_t cn, cd; adf_rat_t q; slong N; int ex; adf_place_t v; adf_lball_t lb;
            acb_t a, sn; fmpq_t th, ths;
            fmpz_init(cn); fmpz_init(cd); adf_rat_init(q); adf_lball_init(lb);
            acb_init(a); acb_init(sn); acb_sentinel(sn); fmpq_init(th); fmpq_init(ths); fmpq_set_si(ths, 5, 7);
            fmpz_set_str(cn, tok(&p), 10); fmpz_set_str(cd, tok(&p), 10);
            N = strtol(tok(&p), NULL, 10); ex = atoi(tok(&p));
            adf_rat_set_fmpz2(q, cn, cd);
            if (adf_place_prime(&v, pr) != ADF_OK) printf("X place\n");
            else if ((st = ex ? adf_lball_set_rat(lb, v, q) : adf_lball_set_rat_ball(lb, v, q, N)) != ADF_OK)
                printf("X lball %d\n", st);
            else {
                acb_set(a, sn); st = adf_lball_psi_tate(a, lb, prec); print_acb("C", st, a, st == ADF_OK || acb_same(a, sn));
                acb_set(a, sn); st = adf_lball_psi_tate_strict(a, lb, prec); print_acb("K", st, a, st == ADF_OK || acb_same(a, sn));
                fmpq_set(th, ths); st = adf_lball_psi_tate_phase(th, lb);
                printf("H %d %d ", st, st == ADF_OK || fmpq_equal(th, ths)); if (st == ADF_OK) pq(th); putchar('\n');
                printf("V %ld %ld %d\n", (long) lb->v, (long) lb->N, lb->exact);
            }
            fmpz_clear(cn); fmpz_clear(cd); adf_rat_clear(q); adf_lball_clear(lb);
            acb_clear(a); acb_clear(sn); fmpq_clear(th); fmpq_clear(ths);
        } else if (c[0] == 'U') {
            slong prec = strtol(tok(&p), NULL, 10), mi = strtol(tok(&p), NULL, 10), dg = strtol(tok(&p), NULL, 10);
            size_t len; char *txt;
            while (*p == ' ') p++;
            len = strlen(p); if (len && p[len-1] == '\n') p[--len] = 0;
            adf_text_limits_default(&lim); lim.max_items = mi;
            adf_qclass_set(x, s);
            st = adf_qclass_set_str(x, p, len, prec, &lim);
            printf("R %d %d %d %ld\n", st, st == ADF_OK ? adf_qclass_is_canonical(x) : adf_qclass_identical(x, s),
                   x->form, (long) x->len);
            if (st == ADF_OK) {
                print_class(x);
                txt = adf_qclass_get_str(&len, x, dg);
                printf("T %s\n", txt ? txt : "NULL");
                if (txt) adf_str_free(txt);
            }
        } else if (c[0] == 'D') {
            bad = read_class(x, &p);
            if (bad) printf("X %d\n", bad);
            else {
                size_t len, len2, nctx = 99; char *d1, *d2; const adf_modctx_struct *binds[4096]; size_t nb = 0; slong i;
                d1 = adf_qclass_dump_str(&len, x);
                for (i = 0; i < x->len; i++) if (x->piece[i].fin.backend == ADF_LOCAL && nb < 4096)
                    binds[nb++] = x->piece[i].fin.mctx;
                adf_qclass_set(y, s);
                st = adf_qclass_load_str_binds(y, d1, len, binds, nb, NULL);
                printf("R %d %d", st, st == ADF_OK && adf_qclass_identical(x, y));
                if (st == ADF_OK) {
                    d2 = adf_qclass_dump_str(&len2, y);
                    printf(" %d", len == len2 && !memcmp(d1, d2, len)); adf_str_free(d2);
                } else printf(" -1");
                st = adf_qclass_dump_inspect(&nctx, NULL, d1, len, NULL);
                printf(" %d %ld %ld\n", st, (long) nctx, (long) nb);
                printf("T %s\n", d1);
                adf_str_free(d1);
            }
        } else if (c[0] == 'M') {
            long nb = strtol(tok(&p), NULL, 10); size_t len, nctx = 99; const adf_modctx_struct *binds[4096]; long i;
            while (*p == ' ') p++;
            len = strlen(p); if (len && p[len-1] == '\n') p[--len] = 0;
            for (i = 0; i < 4096; i++) binds[i] = ctx1;
            adf_qclass_set(x, s);
            g_on = 1; g_calls = 0;
            st = nb < 0 ? adf_qclass_load_str(x, p, len, ctx1, NULL)
                        : adf_qclass_load_str_binds(x, p, len, binds, (size_t) nb, NULL);
            g_on = 0;
            printf("R %d %d %ld", st, st == ADF_OK ? adf_qclass_is_canonical(x) : adf_qclass_identical(x, s), g_calls);
            st = adf_qclass_dump_inspect(&nctx, NULL, p, len, NULL);
            printf(" %d %ld\n", st, st == ADF_OK ? (long) nctx : -1L);
            if (adf_qclass_identical(x, s) == 0 && x->len) {
                size_t l2; char *d = adf_qclass_dump_str(&l2, x);
                printf("T %d %s\n", l2 == len && !memcmp(d, p, len), d); adf_str_free(d);
            }
        } else printf("X cmd\n");
        fflush(stdout);
        adf_qclass_clear(x); adf_qclass_clear(y); adf_qclass_clear(z); adf_qclass_clear(s);
    }
    adf_modctx_free(ctx1); adf_modctx_free(ctx2);
    flint_cleanup_master();
    return 0;
}
