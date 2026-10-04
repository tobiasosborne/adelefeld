/* f-review14 probe: numeric entry points for the referee checks and for fault survivors.
   One command per line from stdin, one output line per command.

   sym  kind inp a b N d          kind 0 Legendre(at b) 1 Jacobi 2 Kronecker;
                                   inp 0 fmpz 1 fball(A,H,d) 2 ucoset(c,N)
   hib  kind a b p v n            kind 0 rat 1 lball exact 2 lball ball(rel prec v,n)
                                   3 idele of the two rationals 4 real ball
   hibd pa pb a b                 lball exact at two different places
   hid  pa pb ra ca Na rb cb Nb   ideles (r, c U(N)); r = 0 means 1
   bino tight k A H d
   pp   policy c N e M d         policy 0 strict 1 coarse 2 fine
   cy   inv c N n
   vol  A H d
   Output: <tag> <status> [value fields] [where: untouched / inf / p / S(sentinel)]
*/
#include <adelefeld.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const char *st(int s)
{
    switch (s) {
    case ADF_OK: return "OK";
    case ADF_NOT_DETERMINED: return "ND";
    case ADF_LIMIT: return "LIMIT";
    case ADF_DOMAIN: return "DOMAIN";
    case ADF_UNSUPPORTED: return "UNSUP";
    case ADF_PARSE: return "PARSE";
    default: return "OTHER";
    }
}
static char *gs(const fmpz_t f)
{
    static char b[4][512];
    static int i;
    char *s = b[i = (i + 1) & 3];
    fmpz_get_str(s, 10, f);
    return s;
}
static const char *where(adf_place_t w, adf_place_t mark)
{
    static char s[32];
    if (adf_place_equal(w, mark)) return "S";
    if (adf_place_is_archimedean(w)) return "inf";
    snprintf(s, sizeof s, "%llu", (unsigned long long) adf_place_prime_get(w));
    return s;
}
static void ball(adf_fball_t x, const fmpz_t A, const fmpz_t H, const fmpz_t d)
{
    if (adf_fball_set_fmpz3(x, A, H, d) != ADF_OK) { fprintf(stderr, "ball\n"); exit(2); }
}
static void pplace(adf_place_t *p, long q)
{
    if (q == 0) *p = adf_place_inf();
    else adf_place_prime(p, (ulong) q);
}

int main(void)
{
    char line[1024];
    while (fgets(line, sizeof line, stdin)) {
        char cmd[32];
        if (sscanf(line, "%31s", cmd) != 1 || cmd[0] == '#') continue;
        if (!strcmp(cmd, "sym")) {
            int kind, inp, z = 99, stt;
            char a[300], b[300], n[300], d[300];
            adf_place_t p, w, w0;
            fmpz_t A, B, H, D;
            adf_fball_t f; adf_ucoset_t u;
            if (sscanf(line, "%*s %d %d %299s %299s %299s %299s", &kind, &inp, a, b, n, d) != 6) continue;
            fmpz_init(A); fmpz_init(B); fmpz_init(H); fmpz_init(D);
            fmpz_set_str(A, a, 10); fmpz_set_str(B, b, 10); fmpz_set_str(H, n, 10); fmpz_set_str(D, d, 10);
            adf_fball_init(f); adf_ucoset_init(u);
            pplace(&p, 0); pplace(&w, 97); w0 = w;
            if (kind == 0 && fmpz_sgn(B) > 0 && fmpz_fits_si(B)) pplace(&p, fmpz_get_si(B));
            if (inp == 0)
                stt = kind == 0 ? adf_fmpz_legendre(&z, &w, A, p)
                    : kind == 1 ? adf_fmpz_jacobi(&z, &w, A, B) : adf_fmpz_kronecker(&z, &w, A, B);
            else if (inp == 1) {
                ball(f, A, H, D);
                stt = kind == 0 ? adf_fball_legendre(&z, &w, f, p)
                    : kind == 1 ? adf_fball_jacobi(&z, &w, f, B) : adf_fball_kronecker(&z, &w, f, B);
            } else {
                if (adf_ucoset_set_fmpz2(u, A, H) != ADF_OK) { printf("sym INVALIDCOSET\n"); continue; }
                stt = kind == 0 ? adf_ucoset_legendre(&z, &w, u, p)
                    : kind == 1 ? adf_ucoset_jacobi(&z, &w, u, B) : adf_ucoset_kronecker(&z, &w, u, B);
            }
            printf("sym %s z=%d where=%s\n", st(stt), z, where(w, w0));
            fmpz_clear(A); fmpz_clear(B); fmpz_clear(H); fmpz_clear(D);
            adf_fball_clear(f); adf_ucoset_clear(u);
        } else if (!strcmp(cmd, "hib") || !strcmp(cmd, "hibd")) {
            int kind = 0, pv = 0, pb = 0, stt, z = 99;
            long v = 0, n = 0;
            char a[300], b[300];
            adf_place_t p, q, w, w0;
            adf_rat_t ra, rb; adf_lball_t x, y; adf_idele_t i, j;
            int two = !strcmp(cmd, "hibd");
            if (two) {
                if (sscanf(line, "%*s %d %d %299s %299s", &pv, &pb, a, b) != 4) continue;
            } else if (sscanf(line, "%*s %d %299s %299s %d %ld %ld", &kind, a, b, &pv, &v, &n) != 6)
                continue;
            adf_rat_init(ra); adf_rat_init(rb); adf_lball_init(x); adf_lball_init(y);
            adf_idele_init(i); adf_idele_init(j);
            pplace(&p, pv); pplace(&q, pb); pplace(&w, 97); w0 = w;
            adf_rat_set_str(ra, a, strlen(a), NULL); adf_rat_set_str(rb, b, strlen(b), NULL);
            if (two) {
                adf_lball_set_rat(x, p, ra); adf_lball_set_rat(y, q, rb);
                stt = adf_lball_hilbert(&z, &w, x, y);
            } else if (kind == 0) stt = adf_rat_hilbert_at(&z, &w, ra, rb, p);
            else if (kind == 1) {
                adf_lball_set_rat(x, p, ra); adf_lball_set_rat(y, p, rb);
                stt = adf_lball_hilbert(&z, &w, x, y);
            } else if (kind == 2) {
                adf_lball_set_rat_ball(x, p, ra, v); adf_lball_set_rat_ball(y, p, rb, n);
                stt = adf_lball_hilbert(&z, &w, x, y);
            } else if (kind == 3) {
                adf_idele_set_rat(i, ra, 64); adf_idele_set_rat(j, rb, 64);
                stt = adf_idele_hilbert_at(&z, &w, i, j, p);
            } else {
                arb_t A2, B2; fmpq_t qa, qb;
                fmpq_init(qa); fmpq_init(qb); arb_init(A2); arb_init(B2);
                adf_rat_get_fmpq(qa, ra); adf_rat_get_fmpq(qb, rb);
                arb_set_fmpq(A2, qa, 64); arb_set_fmpq(B2, qb, 64);
                stt = adf_real_hilbert(&z, &w, A2, B2);
                arb_clear(A2); arb_clear(B2); fmpq_clear(qa); fmpq_clear(qb);
            }
            printf("%s %s z=%d where=%s\n", cmd, st(stt), z, where(w, w0));
            adf_rat_clear(ra); adf_rat_clear(rb); adf_lball_clear(x); adf_lball_clear(y);
            adf_idele_clear(i); adf_idele_clear(j);
        } else if (!strcmp(cmd, "hid")) {
            int stt, z = 99, pa, pb;
            char ra[120], ca[300], na[300], rb[120], cb[300], nb[300];
            adf_place_t p, q, w, w0;
            adf_idele_t i, j;
            fmpz_t t;
            if (sscanf(line, "%*s %d %d %119s %299s %299s %119s %299s %299s",
                       &pa, &pb, ra, ca, na, rb, cb, nb) != 8) continue;
            adf_idele_init(i); adf_idele_init(j); fmpz_init(t);
            pplace(&p, pa); pplace(&q, pb); pplace(&w, 97); w0 = w;
            fmpq_set_ui(i->r, atol(ra) ? (ulong) atol(ra) : 1, 1);
            fmpq_set_ui(j->r, atol(rb) ? (ulong) atol(rb) : 1, 1);
            {
                fmpz_t c1, N1, c2, N2;
                fmpz_init(c1); fmpz_init(N1); fmpz_init(c2); fmpz_init(N2);
                fmpz_set_str(c1, ca, 10); fmpz_set_str(N1, na, 10);
                fmpz_set_str(c2, cb, 10); fmpz_set_str(N2, nb, 10);
                if (adf_ucoset_set_fmpz2(&i->u, c1, N1) != ADF_OK || adf_ucoset_set_fmpz2(&j->u, c2, N2) != ADF_OK)
                    printf("hid INVALIDCOSET\n");
                else {
                    arb_set_si(i->inf, 1); arb_set_si(j->inf, 1);
                    stt = adf_idele_hilbert_at(&z, &w, i, j, p);
                    printf("hid %s z=%d where=%s\n", st(stt), z, where(w, w0));
                }
                fmpz_clear(c1); fmpz_clear(N1); fmpz_clear(c2); fmpz_clear(N2);
            }
            fmpz_clear(t); adf_idele_clear(i); adf_idele_clear(j);
        } else if (!strcmp(cmd, "bino")) {
            int tight, stt; unsigned long k; char a[300], h[300], d[300];
            fmpz_t A, H, D; adf_fball_t x, y;
            if (sscanf(line, "%*s %d %lu %299s %299s %299s", &tight, &k, a, h, d) != 5) continue;
            fmpz_init(A); fmpz_init(H); fmpz_init(D);
            fmpz_set_str(A, a, 10); fmpz_set_str(H, h, 10); fmpz_set_str(D, d, 10);
            adf_fball_init(x); adf_fball_init(y); ball(x, A, H, D);
            stt = tight ? adf_fball_binom_tight(y, NULL, x, k) : adf_fball_binom(y, NULL, x, k);
            printf("bino %s A=%s H=%s d=%s\n", st(stt), gs(y->A), gs(y->H), gs(y->d));
            fmpz_clear(A); fmpz_clear(H); fmpz_clear(D); adf_fball_clear(x); adf_fball_clear(y);
        } else if (!strcmp(cmd, "pp")) {
            int policy, stt; char c[300], n[300], e[300], m[300], d[300];
            fmpz_t C, N, E, M, D; adf_ucoset_t a, y; adf_fball_t x;
            if (sscanf(line, "%*s %d %299s %299s %299s %299s %299s", &policy, c, n, e, m, d) != 6) continue;
            fmpz_init(C); fmpz_init(N); fmpz_init(E); fmpz_init(M); fmpz_init(D);
            fmpz_set_str(C, c, 10); fmpz_set_str(N, n, 10); fmpz_set_str(E, e, 10);
            fmpz_set_str(M, m, 10); fmpz_set_str(D, d, 10);
            adf_ucoset_init(a); adf_ucoset_init(y); adf_fball_init(x);
            if (adf_ucoset_set_fmpz2(a, C, N) != ADF_OK) { printf("pp INVALIDCOSET\n"); continue; }
            ball(x, E, M, D);
            stt = policy == 0 ? adf_ucoset_profpow(y, NULL, a, x)
                : policy == 1 ? adf_ucoset_profpow_coarse(y, NULL, a, x) : adf_ucoset_profpow_fine(y, NULL, a, x);
            printf("pp %s c=%s N=%s\n", st(stt), gs(y->c), gs(y->N));
            fmpz_clear(C); fmpz_clear(N); fmpz_clear(E); fmpz_clear(M); fmpz_clear(D);
            adf_ucoset_clear(a); adf_ucoset_clear(y); adf_fball_clear(x);
        } else if (!strcmp(cmd, "cy")) {
            int inv, stt; char c[300], n[300], nn[300];
            fmpz_t C, N, ORD, J; adf_idclass_t x;
            if (sscanf(line, "%*s %d %299s %299s %299s", &inv, c, n, nn) != 4) continue;
            fmpz_init(C); fmpz_init(N); fmpz_init(ORD); fmpz_init(J);
            fmpz_set_str(C, c, 10); fmpz_set_str(N, n, 10); fmpz_set_str(ORD, nn, 10);
            adf_idclass_init(x); arb_set_si(x->t, 3);
            if (adf_ucoset_set_fmpz2(&x->u, C, N) != ADF_OK) { printf("cy INVALIDCOSET\n"); continue; }
            stt = inv ? adf_idclass_cyclo_exp_uinv(J, NULL, x, ORD) : adf_idclass_cyclo_exp_u(J, NULL, x, ORD);
            printf("cy %s j=%s\n", st(stt), gs(J));
            fmpz_clear(C); fmpz_clear(N); fmpz_clear(ORD); fmpz_clear(J); adf_idclass_clear(x);
        } else if (!strcmp(cmd, "vol")) {
            char a[300], h[300], d[300];
            fmpz_t A, H, D; adf_fball_t x; adf_rat_t v;
            if (sscanf(line, "%*s %299s %299s %299s", a, h, d) != 3) continue;
            fmpz_init(A); fmpz_init(H); fmpz_init(D);
            fmpz_set_str(A, a, 10); fmpz_set_str(H, h, 10); fmpz_set_str(D, d, 10);
            adf_fball_init(x); adf_rat_init(v); ball(x, A, H, D);
            adf_fball_haar_volume(v, x);
            printf("vol %s/%s\n", gs(fmpq_numref(v->q)), gs(fmpq_denref(v->q)));
            fmpz_clear(A); fmpz_clear(H); fmpz_clear(D); adf_fball_clear(x); adf_rat_clear(v);
        }
    }
    return 0;
}