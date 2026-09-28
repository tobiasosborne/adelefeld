#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "adelefeld/dump.h"

static unsigned checks = 0, failures = 0, trips = 0;
#define CHECK(x) do { checks++; if (!(x)) { failures++; \
    fprintf(stderr, "line=%d failed=%s\n", __LINE__, #x); } } while (0)

/* Create the input through the public arithmetic API. Destroy its dump before reading the result. */
#define ROUNDTRIP(TYPE, INIT, CTX) do { \
    adf_##TYPE##_t y; size_t n, m; char *a, *b, *saved; \
    INIT; a = adf_##TYPE##_dump_str(&n, x); saved = malloc(n+1); memcpy(saved, a, n+1); \
    CHECK(adf_##TYPE##_load_str(y, a, n, CTX, NULL) == ADF_OK); \
    memset(a, '!', n); adf_str_free(a); \
    CHECK(adf_##TYPE##_is_canonical(y)); CHECK(adf_##TYPE##_identical(x, y)); \
    b = adf_##TYPE##_dump_str(&m, y); CHECK(n == m && !memcmp(saved, b, n+1)); \
    free(saved); adf_str_free(b); adf_##TYPE##_clear(y); trips++; \
} while (0)

static void finite_trip(const adf_fball_t x)
{
    ROUNDTRIP(fball, adf_fball_init(y), adf_fball_context(x));
}
static void scaled_trip(const adf_scaled_t x)
{
    ROUNDTRIP(scaled, adf_scaled_init(y, x->mctx), x->mctx);
}
static void adele_trip(const adf_adele_t x)
{
    ROUNDTRIP(adele, adf_adele_init(y), adf_fball_context(&x->fin));
}
static void cadele_trip(const adf_cadele_t x)
{
    ROUNDTRIP(cadele, adf_cadele_init(y), adf_fball_context(&x->fin));
}

int main(void)
{
    adf_modctx_struct *c, *same, *reversed, *wrong;
    ulong qs[] = {2, 3}, rev[] = {3, 2}, bad[] = {2, 5};
    adf_fball_t g, f, h;
    adf_scaled_t s, t;
    adf_adele_t a;
    adf_cadele_t z;
    adf_rat_t scalar;
    fmpz_t A, H, d;
    CHECK(adf_modctx_new_blocks(&c, qs, 2) == ADF_OK);
    CHECK(adf_modctx_new_blocks(&same, qs, 2) == ADF_OK);
    CHECK(adf_modctx_new_blocks(&reversed, rev, 2) == ADF_OK);
    CHECK(adf_modctx_new_blocks(&wrong, bad, 2) == ADF_OK);
    memset(g, 0, sizeof(g)); memset(f, 0, sizeof(f)); memset(h, 0, sizeof(h));
    adf_fball_init(g); adf_fball_init(f); adf_fball_init(h);
    adf_scaled_init(s, c); adf_scaled_init(t, c);
    adf_adele_init(a); adf_cadele_init(z); adf_rat_init(scalar);
    fmpz_init(A); fmpz_init_set_ui(H, 6); fmpz_init(d);
    for (int i = 0; i < 500; i++) {
        fmpz_set_si(A, i%13-6); fmpz_set_ui(d, (ulong)(i%17+1));
        CHECK(adf_fball_set_fmpz3(g, A, H, d) == ADF_OK); finite_trip(g);
        CHECK(adf_fball_set_local(f, g, c) == ADF_OK); finite_trip(f);
        adf_fball_add(h, f, f); finite_trip(h);
        adf_fball_mul(h, f, f); finite_trip(h);
        adf_fball_neg(h, f); finite_trip(h);
        int lost;
        CHECK(adf_scaled_set_fball(s, &lost, f, c) == ADF_OK); scaled_trip(s);
        CHECK(adf_scaled_add(t, s, s) == ADF_OK); scaled_trip(t);
        CHECK(adf_scaled_mul(t, s, s) == ADF_OK); scaled_trip(t);
        CHECK(adf_scaled_mul_tight(t, s, s) == ADF_OK); scaled_trip(t);
        adf_rat_set_si(scalar, i-250);
        adf_scaled_set_rat(t, scalar, c); scaled_trip(t);
        adf_fball_set(&a->fin, i%2 ? f : g);
        arb_set_ui(a->inf, (ulong)i+1); arb_sqrt(a->inf, a->inf, 32+i%100);
        adele_trip(a);
        adf_fball_set(&z->fin, i%2 ? f : g);
        acb_set_ui(z->inf, (ulong)i+1); acb_log(z->inf, z->inf, 32+i%100);
        cadele_trip(z);
    }
    size_t len, dl;
    char *text = adf_fball_dump_str(&len, f);
    const adf_modctx_struct *binds[] = {c, c};
    adf_fball_set(h, f);
    char *before = adf_fball_dump_str(&dl, h);
    unsigned char snapshot[sizeof(adf_fball_struct)]; memcpy(snapshot, h, sizeof(snapshot));
    size_t counts[] = {0, 2, SIZE_MAX};
    for (size_t i = 0; i < 3; i++) {
        CHECK(adf_fball_load_str_binds(h, text, len, binds, counts[i], NULL) == ADF_DOMAIN);
        CHECK(!memcmp(snapshot, h, sizeof(snapshot)));
    }
    const adf_modctx_struct *contexts[] = {NULL, reversed, wrong};
    for (size_t i = 0; i < 3; i++) {
        binds[0] = contexts[i];
        CHECK(adf_fball_load_str_binds(h, text, len, binds, 1, NULL) == ADF_DOMAIN);
        CHECK(!memcmp(snapshot, h, sizeof(snapshot)));
    }
    CHECK(adf_fball_load_str_binds(h, text, len, NULL, 1, NULL) == ADF_DOMAIN);
    CHECK(!memcmp(snapshot, h, sizeof(snapshot)));
    CHECK(adf_fball_load_str(h, text, len, same, NULL) == ADF_OK);
    CHECK(adf_fball_equal_set(h, f) && !adf_fball_identical(h, f));
    adf_ctx_desc_t descs[2]; adf_ctx_desc_init(&descs[0]); adf_ctx_desc_init(&descs[1]);
    fmpz_set_ui(descs[0].K, 11); fmpz_set_ui(descs[1].K, 13);
    for (size_t capacity = 0; capacity <= 2; capacity++) {
        size_t n = capacity;
        int st = adf_fball_dump_inspect(&n, descs, text, len, NULL);
        CHECK(st == (capacity ? ADF_OK : ADF_LIMIT));
        CHECK(capacity ? n == 1 && adf_modctx_matches_desc(c, &descs[0]) :
                        n == 0 && fmpz_equal_ui(descs[0].K, 11));
        CHECK(fmpz_equal_ui(descs[1].K, 13));
    }
    adf_ctx_desc_clear(&descs[0]); adf_ctx_desc_clear(&descs[1]);
    adf_str_free(text); adf_str_free(before);
    fmpz_clear(A); fmpz_clear(H); fmpz_clear(d);
    adf_rat_clear(scalar); adf_fball_clear(g); adf_fball_clear(f); adf_fball_clear(h);
    adf_scaled_clear(s); adf_scaled_clear(t); adf_adele_clear(a); adf_cadele_clear(z);
    adf_modctx_free(c); adf_modctx_free(same); adf_modctx_free(reversed); adf_modctx_free(wrong);
    flint_cleanup_master();
    printf("roundtrips=%u checks=%u failures=%u\n", trips, checks, failures);
    return failures != 0;
}
