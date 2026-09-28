#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/arb.h>
#include <flint/acb.h>
#include <adelefeld.h>

static int cases, failed;
#define CHECK(x) do { cases++; if (!(x)) { failed++; fprintf(stderr, "failed line %d\n", __LINE__); } } while (0)

static void
print_edge(const char * e_text, int expected)
{
    adf_adele_t x;
    adf_cadele_t z;
    fmpz_t one, e;
    size_t n = 99;
    char *s;
    adf_adele_init(x);
    adf_cadele_init(z);
    fmpz_init_set_ui(one, 1);
    fmpz_init(e);
    CHECK(fmpz_set_str(e, e_text, 10) == 0);
    arf_set_fmpz_2exp(arb_midref(x->inf), one, e);
    CHECK(adf_adele_is_canonical(x));
    s = adf_adele_get_str(&n, x, 1);
    CHECK((s != NULL) == expected);
    CHECK(expected ? n == strlen(s) : n == 0);
    adf_str_free(s);
    mag_set_fmpz_2exp_fmpz(arb_radref(acb_imagref(z->inf)), one, e);
    n = 99;
    s = adf_cadele_get_str(&n, z, 1);
    CHECK((s != NULL) == expected);
    CHECK(expected ? n == strlen(s) : n == 0);
    adf_str_free(s);
    fmpz_clear(one);
    fmpz_clear(e);
    adf_adele_clear(x);
    adf_cadele_clear(z);
}

static void
roundtrip_limit(void)
{
    const char *input = "(9.99e21 ; 0)";
    adf_adele_t x, y;
    adf_text_limits_t lim;
    size_t n = 0;
    char *s;
    adf_adele_init(x);
    adf_adele_init(y);
    adf_text_limits_default(&lim);
    lim.max_exp10 = 21;
    CHECK(adf_adele_set_str(x, input, strlen(input), 128, &lim) == ADF_OK);
    s = adf_adele_get_str(&n, x, 1);
    CHECK(s != NULL && n > 0);
    if (s != NULL)
    {
        CHECK(adf_adele_set_str(y, s, n, 128, &lim) == ADF_LIMIT);
        lim.max_exp10 = 22;
        CHECK(adf_adele_set_str(y, s, n, 128, &lim) == ADF_OK);
        CHECK(arb_contains(y->inf, x->inf));
        adf_str_free(s);
    }
    adf_adele_clear(x);
    adf_adele_clear(y);
}

static void
cap_edges(void)
{
    adf_modctx_struct *ctx = NULL;
    adf_fball_t g, l, y, expected;
    adf_rat_t cap;
    ulong q = 6;
    CHECK(adf_modctx_new_blocks(&ctx, &q, 1) == ADF_OK);
    adf_fball_init(g);
    adf_fball_init(l);
    adf_fball_init(y);
    adf_fball_init(expected);
    adf_rat_init(cap);
    CHECK(adf_fball_set_str(g, "1 mod 6", 7, NULL) == ADF_OK);
    CHECK(adf_fball_set_local(l, g, ctx) == ADF_OK);
    CHECK(adf_fball_set_str(expected, "1 mod 2", 7, NULL) == ADF_OK);
    adf_rat_set_si(cap, 4);
    CHECK(adf_fball_cap(y, l, cap) == ADF_OK);
    CHECK(adf_fball_equal_set(y, expected));
    CHECK(adf_fball_cap(l, l, cap) == ADF_OK);
    CHECK(adf_fball_equal_set(l, expected));
    CHECK(adf_fball_set_local(l, g, ctx) == ADF_OK);
    CHECK(adf_fball_add_cap(l, l, g, cap) == ADF_OK);
    CHECK(adf_fball_set_str(expected, "0 mod 2", 7, NULL) == ADF_OK);
    CHECK(adf_fball_equal_set(l, expected));
    CHECK(adf_fball_set_local(l, g, ctx) == ADF_OK);
    CHECK(adf_fball_sub_cap(l, l, g, cap) == ADF_OK);
    CHECK(adf_fball_equal_set(l, expected));
    CHECK(adf_fball_set_local(l, g, ctx) == ADF_OK);
    CHECK(adf_fball_mul_cap(l, l, g, cap) == ADF_OK);
    CHECK(adf_fball_set_str(expected, "1 mod 2", 7, NULL) == ADF_OK);
    CHECK(adf_fball_equal_set(l, expected));
    adf_rat_set_si(cap, -1);
    CHECK(adf_fball_mul_rat_cap(l, l, cap, cap) == ADF_DOMAIN);
    CHECK(adf_fball_identical(l, expected));
    CHECK(adf_fball_is_canonical(l));
    adf_rat_set_si(cap, 4);
    CHECK(adf_fball_mul_rat_cap(l, l, cap, cap) == ADF_OK);
    CHECK(adf_fball_is_canonical(l));
    adf_rat_clear(cap);
    adf_fball_clear(expected);
    adf_fball_clear(y);
    adf_fball_clear(l);
    adf_fball_clear(g);
    adf_modctx_free(ctx);
}

static void
predicate_edges(void)
{
    adf_modctx_struct *ctx = NULL;
    adf_fball_t g, l;
    adf_scaled_t s;
    ulong q = 6;
    CHECK(adf_modctx_new_blocks(&ctx, &q, 1) == ADF_OK);
    adf_fball_init(g);
    adf_fball_init(l);
    adf_scaled_init(s, ctx);
    CHECK(adf_fball_set_str(g, "1 mod 6", 7, NULL) == ADF_OK);
    CHECK(adf_fball_set_local(l, g, ctx) == ADF_OK);
    fmpz_zero(l->d);
    CHECK(adf_fball_is_canonical(l) == 0);
    fmpz_one(l->d);
    s->exact = 2;
    CHECK(adf_scaled_is_canonical(s) == 0);
    s->exact = 1;
    adf_scaled_clear(s);
    adf_fball_clear(l);
    adf_fball_clear(g);
    adf_modctx_free(ctx);
}

int
main(void)
{
    print_edge("99999", 1);
    print_edge("100000", 0);
    print_edge("-100001", 1);
    print_edge("-100002", 0);
    print_edge("18446744073709551616", 0);
    roundtrip_limit();
    cap_edges();
    predicate_edges();
    printf("cases=%d failed=%d\n", cases, failed);
    flint_cleanup();
    return failed != 0;
}
