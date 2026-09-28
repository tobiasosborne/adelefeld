#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "adelefeld.h"

int main(int argc, char **argv)
{
    if (argc < 2) return 2;
    if (!strcmp(argv[1], "print")) {
        adf_adele_t x;
        adf_fball_t f;
        arb_t a;
        fmpz_t m, e;
        size_t len;
        char *out;
        int st;
        if (argc != 3) return 2;
        adf_adele_init(x); adf_fball_init(f); arb_init(a);
        fmpz_init_set_ui(m, 1); fmpz_init(e); fmpz_set_str(e, argv[2], 10);
        arf_set_fmpz_2exp(arb_midref(a), m, e);
        st = adf_adele_set_arb_fball(x, a, f);
        printf("constructor=%d finite=%d canonical=%d\n", st, arb_is_finite(a), adf_adele_is_canonical(x));
        fflush(stdout);
        out = adf_adele_get_str(&len, x, 20);
        printf("length=%zu text=%s\n", len, out);
        adf_str_free(out); adf_adele_clear(x); adf_fball_clear(f); arb_clear(a);
        fmpz_clear(m); fmpz_clear(e);
    } else if (!strcmp(argv[1], "predicate")) {
        adf_adele_t a;
        adf_cadele_t c;
        adf_scaled_t s;
        adf_modctx_struct *ctx = NULL;
        ulong q = 2, residue = 0;
        int which = argc == 3 ? atoi(argv[2]) : 0;
        adf_adele_init(a); adf_cadele_init(c);
        a->fin.backend = ADF_LOCAL;
        a->fin.mctx = (const adf_modctx_struct *)(uintptr_t)1;
        a->fin.res = &residue;
        fmpz_set_ui(a->fin.H, 2);
        printf("predicate=%d\n", which); fflush(stdout);
        if (which == 0) printf("result=%d\n", adf_fball_is_canonical(&a->fin));
        if (which == 1) printf("result=%d\n", adf_adele_is_canonical(a));
        if (which == 2) {
            c->fin = a->fin;
            printf("result=%d\n", adf_cadele_is_canonical(c));
        }
        if (which == 3) {
            adf_modctx_new_blocks(&ctx, &q, 1); adf_scaled_init(s, ctx);
            s->mctx = (const adf_modctx_struct *)(uintptr_t)1;
            s->exact = 0; fmpq_one(s->s);
            printf("result=%d\n", adf_scaled_is_canonical(s));
        }
    } else if (!strcmp(argv[1], "roundtrip")) {
        adf_adele_t x, y;
        char *out;
        size_t len;
        const char *text = "(9.99e100000 ; 0)";
        int st;
        adf_adele_init(x); adf_adele_init(y);
        st = adf_adele_set_str(x, text, strlen(text), 128, NULL);
        out = adf_adele_get_str(&len, x, 1);
        printf("input_status=%d text=%s reread_status=%d\n", st, out,
               adf_adele_set_str(y, out, len, 128, NULL));
        adf_str_free(out); adf_adele_clear(x); adf_adele_clear(y);
    } else if (!strcmp(argv[1], "nested")) {
        adf_modctx_struct *ctx = NULL;
        const char *text = "adf1 Q fball l 1 6 2 2 3 0 0";
        int st = adf_modctx_new_from_dump(&ctx, text, strlen(text), 0, NULL);
        printf("nested_status=%d output_null=%d\n", st, ctx == NULL);
        adf_modctx_free(ctx);
    } else if (!strcmp(argv[1], "local")) {
        adf_modctx_struct *ctx = NULL;
        adf_fball_t g, l, y;
        adf_scaled_t sc;
        adf_rat_t cap;
        ulong q = 6;
        int statuses[6], lost = 99;
        char *s;
        size_t len;
        adf_modctx_new_blocks(&ctx, &q, 1);
        adf_fball_init(g); adf_fball_init(l); adf_fball_init(y); adf_rat_init(cap);
        adf_scaled_init(sc, ctx); adf_rat_set_si(cap, 1);
        adf_fball_set_str(g, "1 mod 6", 7, NULL);
        adf_fball_set_local(l, g, ctx);
        s = adf_fball_get_str(&len, l);
        printf("local_canonical=%d printed=%s\n", adf_fball_is_canonical(l), s);
        adf_str_free(s);
        statuses[0] = adf_fball_cap(y, l, cap);
        statuses[1] = adf_fball_add_cap(y, l, g, cap);
        statuses[2] = adf_fball_sub_cap(y, l, g, cap);
        statuses[3] = adf_fball_mul_cap(y, l, g, cap);
        statuses[4] = adf_fball_mul_rat_cap(y, l, cap, cap);
        statuses[5] = adf_scaled_set_fball(sc, &lost, l, ctx);
        printf("cap_statuses=%d,%d,%d,%d,%d scaled_status=%d lost=%d\n",
               statuses[0], statuses[1], statuses[2], statuses[3], statuses[4], statuses[5], lost);
        adf_scaled_clear(sc); adf_rat_clear(cap); adf_fball_clear(g); adf_fball_clear(l);
        adf_fball_clear(y); adf_modctx_free(ctx);
    } else if (!strcmp(argv[1], "lifetime")) {
        adf_modctx_struct *ctx = NULL;
        adf_scaled_t x;
        ulong q = 2;
        adf_modctx_new_blocks(&ctx, &q, 1); adf_scaled_init(x, ctx);
        adf_modctx_free(ctx);
        puts("freed_borrowed_context_without_abort=1");
        /* clear does not dereference the borrowed context. */
        adf_scaled_clear(x);
    } else return 2;
    flint_cleanup();
    return 0;
}
