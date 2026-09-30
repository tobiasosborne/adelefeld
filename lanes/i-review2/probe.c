/* Independent review transport. Oracles live in review.py, not in the implementation reference. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <adelefeld.h>

static void uc(adf_ucoset_t x, slong c, slong N)
{
    fmpz_t a, b;
    fmpz_init_set_si(a, c); fmpz_init_set_si(b, N);
    if (adf_ucoset_set_fmpz2(x, a, b) != ADF_OK) abort();
    fmpz_clear(a); fmpz_clear(b);
}
static void ball(arb_t x, slong m, slong em, ulong rad, slong er)
{
    arf_set_si(arb_midref(x), m);
    arf_mul_2exp_si(arb_midref(x), arb_midref(x), em);
    mag_set_ui_2exp_si(arb_radref(x), rad, er);
}
static void print_uc(const adf_ucoset_t x)
{
    fmpz_print(x->c); putchar(' '); fmpz_print(x->N); putchar(' ');
}
static void print_ball(const arb_t x)
{
    arf_t t; fmpq_t q;
    arf_init(t); fmpq_init(q);
    arf_get_fmpq(q, arb_midref(x)); fmpq_print(q); putchar(' ');
    arf_set_mag(t, arb_radref(x)); arf_get_fmpq(q, t); fmpq_print(q); putchar(' ');
    arf_clear(t); fmpq_clear(q);
}
static void print_fin(const adf_fball_t x)
{
    fmpz_t a, h, d;
    fmpz_init(a); fmpz_init(h); fmpz_init(d);
    adf_fball_get_fmpz3(a, h, d, x);
    fmpz_print(a); putchar(' '); fmpz_print(h); putchar(' '); fmpz_print(d); putchar(' ');
    fmpz_clear(a); fmpz_clear(h); fmpz_clear(d);
}
int main(void)
{
    char op[24];
    adf_ucoset_t u, v, w;
    adf_idele_t x, z, alias, saved;
    adf_idclass_t a, b, ca;
    adf_adele_t ax, az, aa, small, simple;
    adf_rat_t rat, av;
    arb_t t, sentinel;
    fmpz_t A, H, D;
    adf_ucoset_init(u); adf_ucoset_init(v); adf_ucoset_init(w);
    adf_idele_init(x); adf_idele_init(z); adf_idele_init(alias); adf_idele_init(saved);
    adf_idclass_init(a); adf_idclass_init(b); adf_idclass_init(ca);
    adf_adele_init(ax); adf_adele_init(az); adf_adele_init(aa);
    adf_adele_init(small); adf_adele_init(simple);
    adf_rat_init(rat); adf_rat_init(av); arb_init(t); arb_init(sentinel);
    fmpz_init(A); fmpz_init(H); fmpz_init(D);
    while (scanf("%23s", op) == 1)
    {
        slong c, N, k, m, em, er, rn, rd, p, qn, qd, ai, hi, di;
        ulong rad;
        int st = 0, sa = 0, ok = 1;
        if (!strcmp(op, "uc"))
        {
            if (scanf("%ld%ld%ld", &c, &N, &k) != 3) abort();
            uc(u, c, N);
            adf_ucoset_pow(v, u, k); adf_ucoset_pow_tight(w, u, k);
            adf_ucoset_set(&alias->u, u); adf_ucoset_pow(&alias->u, &alias->u, k);
            ok &= adf_ucoset_identical(v, &alias->u);
            adf_ucoset_set(&alias->u, u); adf_ucoset_pow_tight(&alias->u, &alias->u, k);
            ok &= adf_ucoset_identical(w, &alias->u);
            print_uc(v); print_uc(w);
            printf("%d %d\n", ok, adf_ucoset_is_normal(v) && adf_ucoset_is_normal(w));
        }
        else if (!strcmp(op, "map"))
        {
            if (scanf("%ld%ld%ld%ld%ld%ld%ld", &ai, &hi, &di, &c, &N, &rn, &rd) != 7) abort();
            uc(&x->u, c, N); fmpq_set_si(x->r, rn, rd); ball(x->inf, -3, 0, 1, -2);
            fmpz_set_si(A, ai); fmpz_set_si(H, hi); fmpz_set_si(D, di);
            if (adf_fball_set_fmpz3(&ax->fin, A, H, D) != ADF_OK) abort();
            ball(ax->inf, 0, 0, 5, -1);
            adf_adele_set_idele(small, x); adf_adele_set_idele_simple(simple, x);
            st = adf_adele_div_idele(az, ax, x, 64);
            adf_adele_set(aa, ax); sa = adf_adele_div_idele(aa, aa, x, 64);
            ok &= sa == st && adf_adele_identical(aa, az);
            print_fin(&small->fin); print_fin(&simple->fin); print_fin(&az->fin);
            printf("%d %d ", st, ok); print_ball(az->inf); putchar('\n');
        }
        else if (!strcmp(op, "real"))
        {
            char fn[24];
            if (scanf("%23s%ld%ld%lu%ld%ld%ld%ld%ld%ld%ld%ld%ld", fn, &m, &em, &rad, &er,
                      &rn, &rd, &c, &N, &k, &p, &qn, &qd) != 13) abort();
            ball(x->inf, m, em, rad, er); fmpq_set_si(x->r, rn, rd); uc(&x->u, c, N);
            if (!adf_idele_is_canonical(x)) abort();
            adf_idele_set(alias, x); adf_idele_set(saved, x);
            arb_one(z->inf); fmpq_one(z->r); adf_ucoset_one(&z->u);
            arb_set_si(sentinel, 97); arb_set(t, sentinel);
            fmpq_set_si(rat->q, qn, qd);
            if (!strcmp(fn, "pow") || !strcmp(fn, "tight"))
            {
                st = !strcmp(fn, "pow") ? adf_idele_pow(z, x, k, p) : adf_idele_pow_tight(z, x, k, p);
                sa = !strcmp(fn, "pow") ? adf_idele_pow(alias, alias, k, p) :
                    adf_idele_pow_tight(alias, alias, k, p);
                ok &= sa == st && (st == ADF_OK ? adf_idele_identical(alias, z) :
                                  adf_idele_identical(alias, saved));
                if (st != ADF_OK) ok &= arb_is_one(z->inf) && fmpq_is_one(z->r) &&
                    fmpz_is_zero(z->u.N) && fmpz_is_one(z->u.c);
            }
            else if (!strcmp(fn, "mulrat"))
            {
                st = adf_idele_mul_rat(z, x, rat, p); sa = adf_idele_mul_rat(alias, alias, rat, p);
                ok &= sa == st && (st == ADF_OK ? adf_idele_identical(alias, z) :
                                  adf_idele_identical(alias, saved));
            }
            else if (!strcmp(fn, "norm") || !strcmp(fn, "class"))
            {
                st = adf_idele_norm(t, x, p);
                sa = adf_idclass_set_idele(a, x, p);
                ok &= sa == st;
                if (st == ADF_OK) ok &= arb_equal(t, a->t);
                else ok &= arb_equal(t, sentinel);
                arb_set(z->inf, t); fmpq_one(z->r); adf_ucoset_set(&z->u, &a->u);
            }
            else abort();
            printf("%d ", st); print_ball(z->inf); fmpq_print(z->r); putchar(' '); print_uc(&z->u);
            printf("%d\n", ok);
        }
        else if (!strcmp(op, "place"))
        {
            ulong prime; slong val = 123456;
            adf_place_t place;
            if (scanf("%ld%ld%lu", &rn, &rd, &prime) != 3) abort();
            fmpq_set_si(x->r, rn, rd);
            place = adf_place_inf();
            if (prime && adf_place_prime(&place, prime) != ADF_OK) abort();
            fmpq_set_si(av->q, 17, 19);
            st = adf_idele_valuation_at(&val, x, place); sa = adf_idele_abs_at(av, x, place);
            printf("%d %d %ld ", st, sa, val); fmpq_print(av->q); putchar('\n');
        }
        else abort();
        fflush(stdout);
    }
    adf_ucoset_clear(u); adf_ucoset_clear(v); adf_ucoset_clear(w);
    adf_idele_clear(x); adf_idele_clear(z); adf_idele_clear(alias); adf_idele_clear(saved);
    adf_idclass_clear(a); adf_idclass_clear(b); adf_idclass_clear(ca);
    adf_adele_clear(ax); adf_adele_clear(az); adf_adele_clear(aa);
    adf_adele_clear(small); adf_adele_clear(simple);
    adf_rat_clear(rat); adf_rat_clear(av); arb_clear(t); arb_clear(sentinel);
    fmpz_clear(A); fmpz_clear(H); fmpz_clear(D);
    flint_cleanup(); return 0;
}
