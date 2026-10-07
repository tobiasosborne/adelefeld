/* m4-review1 harness: a line interpreter over the milestone 4 calls; exact dyadic output.
   Real input token group: n d m e  = ball around n/d (exact if dyadic) plus radius m*2^e.
   Output of an arb: mid_man mid_exp rad_man rad_exp (exact). */
#include <adelefeld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *tok(void) { return strtok(NULL, " \t\n"); }
static slong rsi(void) { char *t = tok(); if (!t) { fprintf(stderr, "eof\n"); exit(3); } return strtoll(t, 0, 10); }
static void rfz(fmpz_t z) { char *t = tok(); if (!t) exit(3); fmpz_set_str(z, t, 10); }

static void rarb(arb_t x)
{
    fmpq_t q; fmpz_t m; slong e;
    fmpq_init(q); fmpz_init(m);
    rfz(fmpq_numref(q)); rfz(fmpq_denref(q)); fmpq_canonicalise(q);
    rfz(m); e = rsi();
    arb_set_fmpq(x, q, 400);
    if (!fmpz_is_zero(m)) {
        arf_t r; arf_init(r); arf_set_fmpz_2exp(r, m, (fmpz_t){e}); /* small e */
        arb_add_error_arf(x, r); arf_clear(r);
    }
    fmpq_clear(q); fmpz_clear(m);
}
static void racb(acb_t x) { rarb(acb_realref(x)); rarb(acb_imagref(x)); }

static void parf(const arf_t a)
{
    fmpz_t m, e; fmpz_init(m); fmpz_init(e);
    if (arf_is_zero(a)) printf(" 0 0");
    else if (!arf_is_finite(a)) printf(" nan 0");
    else { arf_get_fmpz_2exp(m, e, a); printf(" "); fmpz_print(m); printf(" "); fmpz_print(e); }
    fmpz_clear(m); fmpz_clear(e);
}
static void parb(const arb_t x)
{
    arf_t r; arf_init(r);
    parf(arb_midref(x)); arf_set_mag(r, arb_radref(x)); parf(r); arf_clear(r);
}
static void pacb(const acb_t x) { parb(acb_realref(x)); parb(acb_imagref(x)); }

static void rffun(adf_ffun_t f)
{
    ulong D = rsi(), M = rsi(), L = D * M;
    acb_ptr v = _acb_vec_init(L);
    for (ulong j = 0; j < L; j++) racb(v + j);
    int st = adf_ffun_set_acb_vec(f, D, M, v, L);
    if (st) { printf("SETFAIL %d\n", st); exit(4); }
    _acb_vec_clear(v, L);
}
static void pffun(const adf_ffun_t f)
{
    printf(" %lu %lu", f->D, f->M);
    for (ulong j = 0; j < f->D * f->M; j++) pacb(f->f + j);
}
static void rrfun(adf_rfun_t r)
{
    slong n = rsi();
    adf_rterm_struct *t = flint_malloc((n ? n : 1) * sizeof(adf_rterm_struct));
    for (slong i = 0; i < n; i++) {
        slong l = rsi();
        acb_poly_init(t[i].P); acb_init(t[i].A); acb_init(t[i].B); acb_init(t[i].C);
        acb_poly_fit_length(t[i].P, l);
        for (slong k = 0; k < l; k++) racb(t[i].P->coeffs + k);
        _acb_poly_set_length(t[i].P, l);
        racb(t[i].A); racb(t[i].B); racb(t[i].C);
    }
    int st = adf_rfun_set_terms(r, t, n);
    if (st) { printf("SETFAIL %d\n", st); exit(4); }
    for (slong i = 0; i < n; i++) { acb_poly_clear(t[i].P); acb_clear(t[i].A); acb_clear(t[i].B); acb_clear(t[i].C); }
    flint_free(t);
}
static void prfun(const adf_rfun_t r)
{
    printf(" %ld", r->len);
    for (slong i = 0; i < r->len; i++) {
        printf(" %ld", r->term[i].P->length);
        for (slong k = 0; k < r->term[i].P->length; k++) pacb(r->term[i].P->coeffs + k);
        pacb(r->term[i].A); pacb(r->term[i].B); pacb(r->term[i].C);
    }
}
static void rrat(adf_rat_t q)
{
    fmpz_t a, b; fmpz_init(a); fmpz_init(b); rfz(a); rfz(b);
    if (adf_rat_set_fmpz2(q, a, b)) exit(5);
    fmpz_clear(a); fmpz_clear(b);
}
/* idele: real ball, content n/d, unit c mod N */
static void ridele(adf_idele_t a)
{
    arb_t x; fmpq_t r; fmpz_t c, N; adf_ucoset_t u;
    arb_init(x); fmpq_init(r); fmpz_init(c); fmpz_init(N); adf_ucoset_init(u);
    rarb(x); rfz(fmpq_numref(r)); rfz(fmpq_denref(r)); fmpq_canonicalise(r); rfz(c); rfz(N);
    if (adf_ucoset_set_fmpz2(u, c, N)) { printf("UCOSETFAIL\n"); exit(6); }
    if (adf_idele_set_parts(a, x, r, u)) { printf("IDELEFAIL\n"); exit(6); }
    arb_clear(x); fmpq_clear(r); fmpz_clear(c); fmpz_clear(N); adf_ucoset_clear(u);
}
static adf_modctx_struct *ctx958;
static int localfail;
static void rfball(adf_fball_t x, int local)
{
    fmpz_t A, H, d; fmpz_init(A); fmpz_init(H); fmpz_init(d);
    rfz(A); rfz(H); rfz(d);
    if (adf_fball_set_fmpz3(x, A, H, d)) { printf("FBALLFAIL\n"); exit(7); }
    if (local) {
        adf_fball_t y; adf_fball_init(y);
        int st = adf_fball_set_local(y, x, ctx958);
        if (st) { printf("LOCALFAIL %d\n", st); localfail = 1; adf_fball_clear(y); fmpz_clear(A); fmpz_clear(H); fmpz_clear(d); return; }
        adf_fball_swap(x, y); adf_fball_clear(y);
    }
    fmpz_clear(A); fmpz_clear(H); fmpz_clear(d);
}

/* sentinels */
static void ff_sent(adf_ffun_t f) { acb_t v; acb_init(v); acb_set_si(v, 7); arb_set_si(acb_imagref(v), -7); adf_ffun_set_acb_vec(f, 1, 1, v, 1); acb_clear(v); }
static void rf_sent(adf_rfun_t r)
{
    adf_rterm_struct t; acb_poly_init(t.P); acb_init(t.A); acb_init(t.B); acb_init(t.C);
    acb_poly_set_si(t.P, 7); acb_set_si(t.A, 3); acb_set_si(t.B, 5); acb_set_si(t.C, -1);
    adf_rfun_set_terms(r, &t, 1);
    acb_poly_clear(t.P); acb_clear(t.A); acb_clear(t.B); acb_clear(t.C);
}
static int acb_is_sent(const acb_t z) { acb_t s; acb_init(s); acb_set_si(s, 7); arb_set_si(acb_imagref(s), -7); int r = acb_equal(s, z); acb_clear(s); return r; }

static void outff(int st, const adf_ffun_t y)
{
    adf_ffun_t s; adf_ffun_init(s); ff_sent(s);
    printf("%d", st);
    if (st == 0) pffun(y); else printf(" %s", adf_ffun_identical(s, y) ? "U" : "W");
    printf("\n"); adf_ffun_clear(s);
}
static void outrf(int st, const adf_rfun_t y)
{
    adf_rfun_t s; adf_rfun_init(s); rf_sent(s);
    printf("%d", st);
    if (st == 0) prfun(y); else printf(" %s", adf_rfun_identical(s, y) ? "U" : "W");
    printf("\n"); adf_rfun_clear(s);
}
static void outacb(int st, const acb_t z)
{
    printf("%d", st);
    if (st == 0) pacb(z); else printf(" %s", acb_is_sent(z) ? "U" : "W");
    printf("\n");
}

int main(void)
{
    static char line[1 << 24];
    ulong q[3] = {8, 9, 5};
    if (adf_modctx_new_blocks(&ctx958, q, 3)) return 9;
    while (fgets(line, sizeof line, stdin)) {
        char *c = strtok(line, " \t\n");
        if (!c) continue;
        adf_ffun_t f, g, y; adf_rfun_t r, s, ry; acb_t z; arb_t x;
        adf_ffun_init(f); adf_ffun_init(g); adf_ffun_init(y); ff_sent(y);
        adf_rfun_init(r); adf_rfun_init(s); adf_rfun_init(ry); rf_sent(ry);
        acb_init(z); acb_set_si(z, 7); arb_set_si(acb_imagref(z), -7); arb_init(x);
        if (!strcmp(c, "fourier")) { slong p = rsi(); rffun(f); outff(adf_ffun_fourier(y, f, p), y); }
        else if (!strcmp(c, "fourier_alias")) { slong p = rsi(); rffun(f); outff(adf_ffun_fourier(f, f, p), f); }
        else if (!strcmp(c, "add")) { slong p = rsi(); rffun(f); rffun(g); outff(adf_ffun_add(y, f, g, p), y); }
        else if (!strcmp(c, "mul")) { slong p = rsi(); rffun(f); rffun(g); outff(adf_ffun_mul(y, f, g, p), y); }
        else if (!strcmp(c, "refine")) { rffun(f); ulong D2 = rsi(), M2 = rsi(); outff(adf_ffun_refine(y, f, D2, M2), y); }
        else if (!strcmp(c, "translate")) { adf_rat_t a; adf_rat_init(a); rffun(f); rrat(a); outff(adf_ffun_translate_rat(y, f, a), y); adf_rat_clear(a); }
        else if (!strcmp(c, "reflect")) { rffun(f); outff(adf_ffun_reflect(y, f), y); }
        else if (!strcmp(c, "conj")) { rffun(f); outff(adf_ffun_conj(y, f), y); }
        else if (!strcmp(c, "dilrat")) { adf_rat_t a; adf_rat_init(a); rffun(f); rrat(a); outff(adf_ffun_dilate_rat(y, f, a), y); adf_rat_clear(a); }
        else if (!strcmp(c, "dilid")) { adf_idele_t a; adf_idele_init(a); rffun(f); ridele(a); outff(adf_ffun_dilate_idele(y, f, a), y); adf_idele_clear(a); }
        else if (!strcmp(c, "ffint")) { slong p = rsi(); rffun(f); outacb(adf_ffun_integral(z, f, p), z); }
        else if (!strcmp(c, "ffnorm")) { slong p = rsi(); rffun(f); arb_set_si(x, 7); int st = adf_ffun_norm2(x, f, p); printf("%d", st); parb(x); printf("\n"); }
        else if (!strcmp(c, "ffeval") || !strcmp(c, "ffevalloc")) {
            slong p = rsi(); adf_fball_t b; adf_fball_init(b); rffun(f);
            localfail = 0; rfball(b, c[6] == 'l');
            if (!localfail) outacb(adf_ffun_eval(z, f, b, p), z);
            adf_fball_clear(b);
        }
        else if (!strcmp(c, "teval")) {
            slong p = rsi(); adf_adele_t a; adf_adele_init(a); rrfun(r); rffun(f); rarb(a->inf); rfball(&a->fin, 0);
            outacb(adf_tensor_eval(z, r, f, a, p), z); adf_adele_clear(a);
        }
        else if (!strcmp(c, "tsball")) {
            /* tsball prec R F arch(0 none,1 real,2 complex) x(real) im(real) n {p cnum cden N exact} */
            slong p = rsi(); rrfun(r); rffun(f); int arch = rsi(); arb_t re, im; arb_init(re); arb_init(im); rarb(re); rarb(im);
            slong n = rsi(); adf_lball_struct *loc = flint_malloc((n ? n : 1) * sizeof(adf_lball_struct));
            for (slong i = 0; i < n; i++) {
                ulong pr = rsi(); adf_rat_t cc; adf_rat_init(cc); rrat(cc); slong N = rsi(); int ex = rsi();
                adf_place_t pl; adf_place_prime(&pl, pr); adf_lball_init(loc + i);
                int st = ex ? adf_lball_set_rat(loc + i, pl, cc) : adf_lball_set_rat_ball(loc + i, pl, cc, N);
                if (st) { printf("LBALLFAIL %d\n", st); exit(8); }
                adf_rat_clear(cc);
            }
            adf_sball_t sb; adf_sball_init(sb); adf_place_t wh;
            int st = adf_sball_set_arb_lballs(sb, &wh, arch == 1 ? re : NULL, loc, n);
            if (st) { printf("SBALLFAIL %d\n", st); exit(8); }
            if (arch == 2) { sb->arch = ADF_ARCH_COMPLEX; arb_set(acb_realref(sb->inf), re); arb_set(acb_imagref(sb->inf), im); }
            outacb(adf_tensor_eval_sball(z, r, f, sb, p), z);
            adf_sball_clear(sb); for (slong i = 0; i < n; i++) adf_lball_clear(loc + i); flint_free(loc); arb_clear(re); arb_clear(im);
        }
        else if (!strcmp(c, "tint")) { slong p = rsi(); rrfun(r); rffun(f); outacb(adf_tensor_integral(z, r, f, p), z); }
        else if (!strcmp(c, "tnorm")) { slong p = rsi(); rrfun(r); rffun(f); arb_set_si(x, 7); int st = adf_tensor_norm2(x, r, f, p); printf("%d", st); parb(x); printf("\n"); }
        else if (!strcmp(c, "radd")) { slong p = rsi(); rrfun(r); rrfun(s); outrf(adf_rfun_add(ry, r, s, p), ry); }
        else if (!strcmp(c, "rmul")) { slong p = rsi(); rrfun(r); rrfun(s); outrf(adf_rfun_mul(ry, r, s, p), ry); }
        else if (!strcmp(c, "rtrans")) { slong p = rsi(); adf_rat_t a; adf_rat_init(a); rrfun(r); rrat(a); outrf(adf_rfun_translate_rat(ry, r, a, p), ry); adf_rat_clear(a); }
        else if (!strcmp(c, "rdil")) { slong p = rsi(); adf_rat_t a; adf_rat_init(a); rrfun(r); rrat(a); outrf(adf_rfun_dilate_rat(ry, r, a, p), ry); adf_rat_clear(a); }
        else if (!strcmp(c, "rdilid")) { slong p = rsi(); adf_idele_t a; adf_idele_init(a); rrfun(r); ridele(a); outrf(adf_rfun_dilate_idele(ry, r, a, p), ry); adf_idele_clear(a); }
        else if (!strcmp(c, "rrefl")) { rrfun(r); outrf(adf_rfun_reflect(ry, r), ry); }
        else if (!strcmp(c, "rconj")) { rrfun(r); outrf(adf_rfun_conj(ry, r), ry); }
        else if (!strcmp(c, "rderiv")) { slong p = rsi(); rrfun(r); outrf(adf_rfun_derivative(ry, r, p), ry); }
        else if (!strcmp(c, "rfour")) { slong p = rsi(); rrfun(r); outrf(adf_rfun_fourier(ry, r, p), ry); }
        else if (!strcmp(c, "reval")) { slong p = rsi(); rrfun(r); rarb(x); outacb(adf_rfun_eval(z, r, x, p), z); }
        else if (!strcmp(c, "revalm")) {
            slong p = rsi(); rrfun(r); slong n = rsi();
            for (slong i = 0; i < n; i++) { rarb(x); acb_set_si(z, 7); arb_set_si(acb_imagref(z), -7); int st = adf_rfun_eval(z, r, x, p); printf("%d", st); if (st == 0) pacb(z); else printf(" %s", acb_is_sent(z) ? "U" : "W"); printf(i + 1 < n ? " ; " : "\n"); }
        }
        else if (!strcmp(c, "rint")) { slong p = rsi(); rrfun(r); outacb(adf_rfun_integral(z, r, p), z); }
        else if (!strcmp(c, "rnorm")) { slong p = rsi(); rrfun(r); arb_set_si(x, 7); int st = adf_rfun_norm2(x, r, p); printf("%d", st); parb(x); printf("\n"); }
        else if (!strcmp(c, "poisson")) {
            slong bits = rsi(), p = rsi(); rrfun(r); rffun(f); acb_t l2; acb_init(l2); acb_set(l2, z);
            ulong NL = 777, NR = 777;
            int st = adf_tensor_poisson(z, l2, &NL, &NR, r, f, bits, p);
            printf("%d", st);
            if (st == 0) { printf(" %lu %lu", NL, NR); pacb(z); pacb(l2); }
            else printf(" %s", (acb_is_sent(z) && acb_is_sent(l2) && NL == 777 && NR == 777) ? "U" : "W");
            printf("\n"); acb_clear(l2);
        }
        else { printf("BADCMD %s\n", c); }
        fflush(stdout);
        adf_ffun_clear(f); adf_ffun_clear(g); adf_ffun_clear(y); adf_rfun_clear(r); adf_rfun_clear(s); adf_rfun_clear(ry);
        acb_clear(z); arb_clear(x);
    }
    adf_modctx_free(ctx958);
    flint_cleanup_master();
    return 0;
}
