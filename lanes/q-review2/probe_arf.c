/* Probe: is arf_cmp_ui exact at tiny distance from 1, and what does arf_sum(prec=2,
   ARF_RND_DOWN) return when terms of hugely different magnitude cancel? */
#include <adelefeld.h>
#include <stdio.h>

static void set_pow2(arf_t r, slong man, mp_limb_signed_t e)
{
    fmpz_t m, x;
    fmpz_init(m); fmpz_init(x);
    fmpz_set_si(m, man);
    fmpz_set_si(x, (slong) e);
    arf_set_fmpz_2exp(r, m, x);
    fmpz_clear(m); fmpz_clear(x);
}

static void cmp_probe(mp_limb_signed_t e)
{
    arb_t b;
    arf_t m;
    arf_init(m);
    arb_init(b);
    set_pow2(m, 1, -e);
    arf_set(arb_midref(b), m);
    printf("mid = 2^-%-6ld  cmp_ui(.,1) = %2d  sgn = %d\n", (long) e,
           arf_cmp_ui(arb_midref(b), 1), arf_sgn(arb_midref(b)));
    arf_add(arb_midref(b), arb_midref(b), m, 2, ARF_RND_NEAR);   /* 2^(1-e) */
    printf("mid = 2^%-6ld cmp_ui(.,1) = %2d  sgn = %d\n", (long) (1 - e),
           arf_cmp_ui(arb_midref(b), 1), arf_sgn(arb_midref(b)));
    arf_clear(m); arb_clear(b);
}

static void sum_probe(const char *name, arf_srcptr t0, arf_srcptr t1, arf_srcptr t2, arf_srcptr t3)
{
    arf_struct terms[4];
    arf_t sum;
    int i;
    arf_init(terms + 0); arf_init(terms + 1); arf_init(terms + 2); arf_init(terms + 3);
    arf_init(sum);
    arf_set(terms + 0, t0); arf_set(terms + 1, t1); arf_set(terms + 2, t2); arf_set(terms + 3, t3);
    arf_sum(sum, terms, 4, 2, ARF_RND_DOWN);
    printf("%-30s sign = %2d", name, arf_sgn(sum));
    if (!arf_is_zero(sum)) { char *s = arf_get_str(sum, 0); printf("  val = %s", s); flint_free(s); }
    printf("\n");
    for (i = 0; i < 4; i++) arf_clear(terms + i);
    arf_clear(sum);
}

int main(void)
{
    arf_t a, b, c, d;
    mp_limb_signed_t big = 4611686018427387904LL / 4;
    arf_init(a); arf_init(b); arf_init(c); arf_init(d);
    cmp_probe(2); cmp_probe(10); cmp_probe(30); cmp_probe(60); cmp_probe(61);
    cmp_probe(100); cmp_probe(1000);

    set_pow2(a, 1, 0); set_pow2(b, -1, 0); set_pow2(c, 1, -1000); set_pow2(d, 0, 0);
    sum_probe("1 - 1 + 2^-1000 + 0  (=+)", a, b, c, d);
    sum_probe("1 - 1 + 0 - 2^-1000  (=-)", a, b, d, c);
    set_pow2(c, 1, -1000); set_pow2(d, 1, -1001);
    sum_probe("1 - 1 + 2^-1000 - 2^-1001", a, b, c, d);
    set_pow2(a, 0, 0); set_pow2(b, 0, 0); set_pow2(c, 1, -1000); set_pow2(d, -1, -1000);
    sum_probe("0 0 2^-1000 -2^-1000 (=0)", a, b, c, d);
    set_pow2(a, 0, 0); set_pow2(b, 1, -1000); set_pow2(c, -1, -1000); set_pow2(d, 1, -1001);
    sum_probe("0 -2^-1000 +2^-1000 +2^-1001", a, b, c, d);
    set_pow2(a, 1, -big); set_pow2(b, -1, -big); set_pow2(c, 1, -big - 1); set_pow2(d, 0, 0);
    sum_probe("t - t + t/2 + 0 (e = -2^60)", a, b, c, d);
    set_pow2(a, 1, big); set_pow2(b, -1, big); set_pow2(c, 1, big - 1); set_pow2(d, 0, 0);
    sum_probe("T - T + T/2 + 0 (e = +2^60)", a, b, c, d);
    set_pow2(a, 0, 0); set_pow2(b, 0, 0); set_pow2(c, 1, big); set_pow2(d, -1, big - 1);
    sum_probe("0 0 2^big - 2^(big-1)", a, b, c, d);
    set_pow2(a, 1, 0); set_pow2(b, 0, 0); set_pow2(c, 0, 0); set_pow2(d, 0, 0);
    sum_probe("1 0 0 0", a, b, c, d);
    set_pow2(a, 0, 0); set_pow2(b, 0, 0); set_pow2(c, 0, 0); set_pow2(d, 0, 0);
    sum_probe("0 0 0 0", a, b, c, d);
    arf_clear(a); arf_clear(b); arf_clear(c); arf_clear(d);
    return 0;
}
