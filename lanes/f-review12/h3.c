#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <adelefeld.h>
static long checks = 0, fails = 0;
#define CHECK(cond, ...) do { checks++; if (!(cond)) { fails++; printf("FAIL line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

static void setf(adf_fball_t x, long A, long H, long d)
{
    fmpz_t a, h, dd; fmpz_init_set_si(a, A); fmpz_init_set_si(h, H); fmpz_init_set_si(dd, d);
    int st = adf_fball_set_fmpz3(x, a, h, dd); if (st) { printf("setf %d\n", st); exit(2); }
    fmpz_clear(a); fmpz_clear(h); fmpz_clear(dd);
}
int main(void)
{
    adf_place_t w, w0; adf_place_prime(&w, 5); w0 = w;
    adf_fball_t x, y, y0, z; adf_fball_init(x); adf_fball_init(y); adf_fball_init(y0); adf_fball_init(z);
    setf(y0, 7, 9, 1);
    /* ---- binom statuses, untouched ---- */
    long ins[][3] = {{1, 0, 2}, {1, 2, 4}, {3, 6, 9}, {2, 4, 6}, {5, 0, 3}, {1, 3, 2}, {0, 0, 5}};
    ulong ks[] = {0, 1, 4, 256, 257, 4096, 4097, 100000};
    for (int tight = 0; tight < 2; tight++)
    for (unsigned i = 0; i < sizeof ins / sizeof ins[0]; i++)
    for (unsigned q = 0; q < 8; q++)
    {
        setf(x, ins[i][0], ins[i][1], ins[i][2]);
        adf_fball_set(y, y0);
        ulong k = ks[q];
        int st = tight ? adf_fball_binom_tight(y, &w, x, k) : adf_fball_binom(y, &w, x, k);
        ulong lim = tight ? 256 : 4096;
        /* expected */
        fmpz_t A, H, d; fmpz_init(A); fmpz_init(H); fmpz_init(d); adf_fball_get_fmpz3(A, H, d, x);
        int exp;
        if (k > lim) exp = ADF_LIMIT;
        else if (fmpz_is_one(d)) exp = ADF_OK;
        else { fmpz_t g; fmpz_init(g); fmpz_gcd(g, H, d); exp = fmpz_divisible(A, g) ? ADF_NOT_DETERMINED : ADF_DOMAIN; fmpz_clear(g); }
        CHECK(st == exp, "binom tight=%d in=(%ld,%ld,%ld) k=%lu st=%d exp=%d", tight, ins[i][0], ins[i][1], ins[i][2], k, st, exp);
        if (st != ADF_OK) CHECK(adf_fball_identical(y, y0), "binom value touched on status %d", st);
        else CHECK(adf_fball_is_canonical(y), "binom out not canonical");
        CHECK(memcmp(&w, &w0, sizeof w) == 0, "where touched");
        fmpz_clear(A); fmpz_clear(H); fmpz_clear(d);
    }
    /* where NULL */
    setf(x, 3, 8, 1);
    CHECK(adf_fball_binom(y, NULL, x, 4) == ADF_OK, "null where");
    /* alias y == x, random */
    for (int i = 0; i < 2000; i++)
    {
        long A = (long) (rand() % 2001) - 1000, H = rand() % 50, k = rand() % 13;
        for (int tight = 0; tight < 2; tight++)
        {
            setf(x, A, H, 1); adf_fball_set(z, x);
            int s1 = tight ? adf_fball_binom_tight(y, NULL, x, k) : adf_fball_binom(y, NULL, x, k);
            int s2 = tight ? adf_fball_binom_tight(z, NULL, z, k) : adf_fball_binom(z, NULL, z, k);
            CHECK(s1 == s2 && adf_fball_identical(y, z), "alias differs A=%ld H=%ld k=%ld", A, H, k);
        }
    }
    /* local ball */
    {
        adf_modctx_struct *ctx; ulong q[] = {8, 9, 5};
        CHECK(adf_modctx_new_blocks(&ctx, q, 3) == ADF_OK, "ctx");
        setf(x, 17, 360, 1); /* 17 + 360 Zhat */
        adf_fball_t L; adf_fball_init(L);
        CHECK(adf_fball_set_local(L, x, ctx) == ADF_OK, "set_local");
        CHECK(adf_fball_is_local(L), "is local");
        adf_fball_t R1, R2; adf_fball_init(R1); adf_fball_init(R2);
        for (ulong k = 0; k < 9; k++) {
            int s1 = adf_fball_binom_tight(R1, NULL, L, k), s2 = adf_fball_binom_tight(R2, NULL, x, k);
            CHECK(s1 == s2 && adf_fball_equal_set(R1, R2), "local binom k=%lu", k);
            adf_fball_t Lc; adf_fball_init(Lc);
            adf_fball_clear(Lc);
        }
        adf_fball_clear(R1); adf_fball_clear(R2); adf_fball_clear(L); adf_modctx_free(ctx);
    }
    /* big: 2000-bit a, N at k max */
    {
        fmpz_t a, N, one; fmpz_init(a); fmpz_init(N); fmpz_init(one); fmpz_one(one);
        fmpz_set_ui(a, 3); fmpz_pow_ui(a, a, 1260); fmpz_sub_ui(a, a, 12345);
        fmpz_set_ui(N, 7); fmpz_pow_ui(N, N, 700); fmpz_neg(N, N); fmpz_abs(N, N);
        CHECK(adf_fball_set_fmpz3(x, a, N, one) == ADF_OK, "big set");
        int s = adf_fball_binom(y, NULL, x, ADF_BINOM_K_MAX);
        CHECK(s == ADF_OK, "big binom st %d", s);
        s = adf_fball_binom_tight(y, NULL, x, ADF_BINOM_TIGHT_K_MAX);
        CHECK(s == ADF_OK, "big tight st %d", s);
        fmpz_clear(a); fmpz_clear(N); fmpz_clear(one);
    }
    /* ---- profpow: statuses, alias, untouched ---- */
    {
        adf_ucoset_t u, r, r0; adf_ucoset_init(u); adf_ucoset_init(r); adf_ucoset_init(r0);
        fmpz_t c, N; fmpz_init(c); fmpz_init(N);
        fmpz_set_ui(c, 5); fmpz_set_ui(N, 7); adf_ucoset_set_fmpz2(r0, c, N); /* sentinel [5 mod 7] */
        fmpz_set_ui(c, 3); fmpz_set_ui(N, 20); adf_ucoset_set_fmpz2(u, c, N);
        long exps[][3] = {{1, 0, 2}, {1, 2, 4}, {3, 6, 9}, {5, 0, 3}, {2, 4, 6}, {0, 0, 1}, {4, 3, 1}, {-3, 12, 1}, {9, 0, 1}};
        for (int mode = 0; mode < 3; mode++)
        for (unsigned i = 0; i < sizeof exps / sizeof exps[0]; i++)
        {
            setf(x, exps[i][0], exps[i][1], exps[i][2]);
            adf_ucoset_set(r, r0);
            int st = mode == 0 ? adf_ucoset_profpow(r, &w, u, x) : mode == 1 ? adf_ucoset_profpow_coarse(r, &w, u, x) : adf_ucoset_profpow_fine(r, &w, u, x);
            fmpz_t A, H, d, g; fmpz_init(A); fmpz_init(H); fmpz_init(d); fmpz_init(g); adf_fball_get_fmpz3(A, H, d, x);
            int domain_nd = 0;
            if (!fmpz_is_one(d)) { fmpz_gcd(g, H, d); domain_nd = fmpz_divisible(A, g) ? 1 : 2; }
            if (domain_nd == 1) CHECK(st == ADF_NOT_DETERMINED, "pp nd mode %d i %u st %d", mode, i, st);
            if (domain_nd == 2) CHECK(st == ADF_DOMAIN, "pp dom mode %d i %u st %d", mode, i, st);
            if (st != ADF_OK) CHECK(adf_ucoset_identical(r, r0), "pp touched mode %d i %u", mode, i);
            CHECK(memcmp(&w, &w0, sizeof w) == 0, "where touched pp");
            /* alias */
            adf_ucoset_t a2; adf_ucoset_init(a2); adf_ucoset_set(a2, u);
            int st2 = mode == 0 ? adf_ucoset_profpow(a2, NULL, a2, x) : mode == 1 ? adf_ucoset_profpow_coarse(a2, NULL, a2, x) : adf_ucoset_profpow_fine(a2, NULL, a2, x);
            CHECK(st2 == st, "alias status");
            if (st == ADF_OK) CHECK(adf_ucoset_identical(a2, r), "alias value");
            else CHECK(adf_ucoset_identical(a2, u), "alias untouched on failure");
            adf_ucoset_clear(a2);
            fmpz_clear(A); fmpz_clear(H); fmpz_clear(d); fmpz_clear(g);
        }
        /* LIMIT after domain: nonintegral exponent beyond limit -> domain first */
        setf(x, 1000, 0, 7); /* 1000/7 non-integral exact */
        CHECK(adf_ucoset_profpow_fine(r, NULL, u, x) == ADF_DOMAIN, "limit-vs-domain");
        /* exact [1] with huge exponent in fine: constant, no LIMIT; [-1] huge */
        fmpz_t bigE; fmpz_init(bigE); fmpz_set_ui(bigE, 3); fmpz_pow_ui(bigE, bigE, 2000);
        fmpz_one(c);
        adf_fball_t big; adf_fball_init(big);
        { fmpz_t zero, one; fmpz_init(zero); fmpz_init(one); fmpz_one(one);
          adf_fball_set_fmpz3(big, bigE, zero, one);
          adf_ucoset_t m1; adf_ucoset_init(m1); adf_ucoset_minus_one(m1);
          CHECK(adf_ucoset_profpow_fine(r, NULL, m1, big) == ADF_OK && adf_ucoset_is_exact(r), "big exp on -1 fine");
          CHECK(fmpz_equal_si(r->c, -1) , "(-1)^(3^2000) = -1, got c");
          fmpz_neg(bigE, bigE); adf_fball_set_fmpz3(big, bigE, zero, one);
          CHECK(adf_ucoset_profpow(r, NULL, m1, big) == ADF_OK && fmpz_equal_si(r->c, -1), "(-1)^(-3^2000)");
          adf_ucoset_clear(m1); fmpz_clear(zero); fmpz_clear(one); }
        adf_fball_clear(big);
        adf_ucoset_clear(u); adf_ucoset_clear(r); adf_ucoset_clear(r0); fmpz_clear(c); fmpz_clear(N); fmpz_clear(bigE);
    }
    /* ---- cyclo alias j == n, where, n<=0 ---- */
    {
        adf_ucoset_t u; adf_idclass_t k; arb_t t; fmpz_t j, n, c, N;
        adf_ucoset_init(u); adf_idclass_init(k); arb_init(t); arb_one(t);
        fmpz_init(j); fmpz_init(n); fmpz_init(c); fmpz_init(N);
        fmpz_set_ui(c, 31); fmpz_set_ui(N, 35); adf_ucoset_set_fmpz2(u, c, N); adf_idclass_set_parts(k, t, u);
        for (int inv = 0; inv < 2; inv++)
        for (long nn = -2; nn <= 40; nn++)
        {
            fmpz_set_si(n, nn); fmpz_set_si(j, 12345);
            int s1 = inv ? adf_idclass_cyclo_exp_uinv(j, &w, k, n) : adf_idclass_cyclo_exp_u(j, &w, k, n);
            fmpz_t jj; fmpz_init_set(jj, n);
            int s2 = inv ? adf_idclass_cyclo_exp_uinv(jj, NULL, k, jj) : adf_idclass_cyclo_exp_u(jj, NULL, k, jj);
            CHECK(s1 == s2, "cyclo alias status n=%ld", nn);
            if (s1 == ADF_OK) CHECK(fmpz_equal(j, jj), "cyclo alias value n=%ld", nn);
            else { CHECK(fmpz_equal_si(j, 12345), "j touched n=%ld", nn); CHECK(fmpz_equal_si(jj, nn), "alias j=n touched n=%ld", nn); }
            CHECK(memcmp(&w, &w0, sizeof w) == 0, "where touched cyclo");
            fmpz_clear(jj);
        }
        fmpz_clear(j); fmpz_clear(n); fmpz_clear(c); fmpz_clear(N); adf_ucoset_clear(u); adf_idclass_clear(k); arb_clear(t);
    }
    /* ---- identities: random ---- */
    for (int it = 0; it < 1500; it++)
    {
        /* binom conservative contains tight; Pascal on exact ints */
        long A = (long) (rand() % 20001) - 10000; long H = rand() % 200; ulong k = 1 + rand() % 20;
        adf_fball_t c1, t1; adf_fball_init(c1); adf_fball_init(t1);
        setf(x, A, H, 1);
        int s1 = adf_fball_binom(c1, NULL, x, k), s2 = adf_fball_binom_tight(t1, NULL, x, k);
        CHECK(s1 == 0 && s2 == 0, "ident st");
        CHECK(adf_fball_contains(t1, c1), "tight inside cons A=%ld H=%ld k=%lu", A, H, k);
        /* Pascal exact */
        adf_fball_t p1, p2, p3; adf_fball_init(p1); adf_fball_init(p2); adf_fball_init(p3);
        setf(x, A, 0, 1); adf_fball_binom(p1, NULL, x, k); adf_fball_binom(p2, NULL, x, k - 1);
        setf(z, A + 1, 0, 1); adf_fball_binom(p3, NULL, z, k);
        fmpz_t a1, h1, d1, a2, h2, d2, a3, h3, d3; fmpz_init(a1); fmpz_init(h1); fmpz_init(d1); fmpz_init(a2); fmpz_init(h2); fmpz_init(d2); fmpz_init(a3); fmpz_init(h3); fmpz_init(d3);
        adf_fball_get_fmpz3(a1, h1, d1, p1); adf_fball_get_fmpz3(a2, h2, d2, p2); adf_fball_get_fmpz3(a3, h3, d3, p3);
        fmpz_add(a1, a1, a2);
        CHECK(fmpz_equal(a1, a3), "pascal A=%ld k=%lu", A, k);
        fmpz_clear(a1); fmpz_clear(h1); fmpz_clear(d1); fmpz_clear(a2); fmpz_clear(h2); fmpz_clear(d2); fmpz_clear(a3); fmpz_clear(h3); fmpz_clear(d3);
        adf_fball_clear(p1); adf_fball_clear(p2); adf_fball_clear(p3); adf_fball_clear(c1); adf_fball_clear(t1);
    }
    printf("h3: checks %ld fails %ld\n", checks, fails);
    adf_fball_clear(x); adf_fball_clear(y); adf_fball_clear(y0); adf_fball_clear(z);
    return fails != 0;
}
