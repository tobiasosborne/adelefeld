/* Independent wrapper, conversion, precision and exponent edge checks. */
#include <stdio.h>
#include <stdlib.h>
#include <adelefeld.h>

static unsigned long count = 0;
#define CHECK(x) do { count++; if (!(x)) { fprintf(stderr, "FAIL line %d\n", __LINE__); abort(); } } while (0)
static void bounds(fmpq_t lo, fmpq_t hi, const arb_t t)
{
    arf_t rad; fmpq_t m, r;
    arf_init(rad); fmpq_init(m); fmpq_init(r);
    arf_get_fmpq(m, arb_midref(t)); arf_set_mag(rad, arb_radref(t)); arf_get_fmpq(r, rad);
    fmpq_sub(lo, m, r); fmpq_add(hi, m, r);
    arf_clear(rad); fmpq_clear(m); fmpq_clear(r);
}
int main(void)
{
    adf_idele_t x, y, old;
    adf_idclass_t a, b, z, ali, saved;
    adf_adele_t ax, az, ao;
    adf_ucoset_t u;
    adf_rat_t q;
    arb_t t, sentinel;
    fmpq_t al, ah, bl, bh, v;
    fmpz_t c, n, exp, kf;
    slong ps[] = {WORD_MIN, -1, 0, 1, 2, 3, 8, 30, 64};
    slong ks[] = {WORD_MIN, WORD_MAX, -12, -3, -2, -1, 0, 1, 2, 3, 12, 30, 60, 64, 210};
    int st, sa;
    unsigned long class_ops = 0, conversions = 0, limits = 0, huge = 0;
    adf_idele_init(x); adf_idele_init(y); adf_idele_init(old);
    adf_idclass_init(a); adf_idclass_init(b); adf_idclass_init(z);
    adf_idclass_init(ali); adf_idclass_init(saved);
    adf_adele_init(ax); adf_adele_init(az); adf_adele_init(ao); adf_ucoset_init(u); adf_rat_init(q);
    arb_init(t); arb_init(sentinel); arb_set_si(sentinel, 97);
    fmpq_init(al); fmpq_init(ah); fmpq_init(bl); fmpq_init(bh); fmpq_init(v);
    fmpz_init(c); fmpz_init(n); fmpz_init(exp); fmpz_init(kf);
    for (int i = 0; i < 2500; i++)
    {
        slong p = ps[i % 9], k = ks[i % 15];
        arb_set_si(a->t, 1 + i % 17); mag_set_ui_2exp_si(arb_radref(a->t), 1, -(1 + i % 30));
        arb_set_si(b->t, 1 + i % 31); mag_set_ui_2exp_si(arb_radref(b->t), 1, -(1 + i % 20));
        fmpz_set_si(c, 5); fmpz_set_si(n, 6); CHECK(adf_ucoset_set_fmpz2(&a->u, c, n) == ADF_OK);
        fmpz_set_si(c, 3); fmpz_set_si(n, 8); CHECK(adf_ucoset_set_fmpz2(&b->u, c, n) == ADF_OK);
        bounds(al, ah, a->t); bounds(bl, bh, b->t);
        adf_idclass_set(saved, z);
        st = adf_idclass_mul(z, a, b, p);
        if (st == ADF_OK)
        {
            fmpq_mul(v, al, bl); CHECK(arb_contains_fmpq(z->t, v));
            fmpq_mul(v, ah, bh); CHECK(arb_contains_fmpq(z->t, v));
            CHECK(adf_idclass_is_canonical(z)); CHECK(adf_ucoset_is_normal(&z->u));
        }
        else CHECK(st == ADF_NOT_DETERMINED && adf_idclass_identical(saved, z));
        adf_idclass_set(ali, a); sa = adf_idclass_mul(ali, ali, b, p);
        CHECK(sa == st && (st == ADF_OK ? adf_idclass_identical(ali, z) : adf_idclass_identical(ali, a)));
        adf_idclass_set(ali, b); sa = adf_idclass_mul(ali, a, ali, p);
        CHECK(sa == st && (st == ADF_OK ? adf_idclass_identical(ali, z) : adf_idclass_identical(ali, b)));
        st = adf_idclass_mul(z, a, a, p); adf_idclass_set(ali, a); sa = adf_idclass_mul(ali, ali, ali, p);
        CHECK(sa == st && (st == ADF_OK ? adf_idclass_identical(ali, z) : adf_idclass_identical(ali, a)));
        st = adf_idclass_inv(z, a, p); adf_idclass_set(ali, a); sa = adf_idclass_inv(ali, ali, p);
        CHECK(sa == st && (st == ADF_OK ? adf_idclass_identical(ali, z) : adf_idclass_identical(ali, a)));
        if (st == ADF_OK)
        {
            fmpq_inv(v, ah); CHECK(arb_contains_fmpq(z->t, v));
            fmpq_inv(v, al); CHECK(arb_contains_fmpq(z->t, v));
        }
        st = adf_idclass_pow(z, a, k, p); adf_idclass_set(ali, a); sa = adf_idclass_pow(ali, ali, k, p);
        CHECK(sa == st && (st == ADF_OK ? adf_idclass_identical(ali, z) : adf_idclass_identical(ali, a)));
        st = adf_idclass_pow_tight(z, a, k, p); adf_idclass_set(ali, a);
        sa = adf_idclass_pow_tight(ali, ali, k, p);
        CHECK(sa == st && (st == ADF_OK ? adf_idclass_identical(ali, z) : adf_idclass_identical(ali, a)));
        class_ops += 6;
        /* The valid idele divisor always excludes zero; the additive numerator may cross it. */
        arb_set(x->inf, a->t); if (i % 2) arb_neg(x->inf, x->inf);
        fmpq_set_si(x->r, 3, 7); adf_ucoset_set(&x->u, &a->u);
        adf_idele_abs_inf(t, x); CHECK(arb_equal(t, a->t));
    }
    /* All combinations of additive input certificates. */
    for (int real = 0; real < 5; real++) for (int fin = 0; fin < 4; fin++)
    {
        int expected;
        arb_set_si(ax->inf, real < 3 ? real - 1 : 1);
        if (real == 3) mag_one(arb_radref(ax->inf));
        if (real == 4) mag_set_ui(arb_radref(ax->inf), 2);
        if (fin < 3) adf_fball_set_si(&ax->fin, fin - 1);
        else
        {
            fmpz_set_si(c, 2); fmpz_set_si(n, 3); fmpz_one(exp);
            CHECK(adf_fball_set_fmpz3(&ax->fin, c, n, exp) == ADF_OK);
        }
        expected = (real == 1 || fin == 1) ? ADF_NOT_UNIT :
            (real >= 3 || fin == 3) ? ADF_UNIT_NOT_CERTIFIED : ADF_OK;
        adf_idele_set(old, x); st = adf_idele_set_adele(x, ax);
        CHECK(st == expected);
        if (st != ADF_OK) CHECK(adf_idele_identical(x, old));
        else
        {
            adf_adele_set_idele(az, x); CHECK(adf_adele_identical(az, ax));
        }
        conversions++;
    }
    /* Cap precedence, unchanged output, and the inclusive cap on a small exact result. */
    arb_one(x->inf); fmpq_one(x->r); adf_ucoset_one(&x->u); fmpq_zero(q->q);
    adf_adele_set_si(ax, 1); adf_adele_set(ao, az); adf_idclass_set(saved, z); adf_idele_set(old, y);
    slong precs[] = {ADF_IDELE_PREC_MAX + 1, WORD_MAX};
    for (int i = 0; i < 2; i++)
    {
        slong p = precs[i];
        CHECK(adf_idele_mul_rat(y, x, q, p) == ADF_LIMIT && adf_idele_identical(y, old));
        CHECK(adf_idele_pow(y, x, 0, p) == ADF_LIMIT && adf_idele_identical(y, old));
        CHECK(adf_idele_pow_tight(y, x, WORD_MIN, p) == ADF_LIMIT && adf_idele_identical(y, old));
        CHECK(adf_idclass_pow(z, a, 0, p) == ADF_LIMIT && adf_idclass_identical(z, saved));
        CHECK(adf_idclass_pow_tight(z, a, WORD_MIN, p) == ADF_LIMIT && adf_idclass_identical(z, saved));
        CHECK(adf_idclass_mul(z, a, b, p) == ADF_LIMIT && adf_idclass_identical(z, saved));
        CHECK(adf_idclass_inv(z, a, p) == ADF_LIMIT && adf_idclass_identical(z, saved));
        CHECK(adf_idclass_set_idele(z, x, p) == ADF_LIMIT && adf_idclass_identical(z, saved));
        arb_set(t, sentinel); CHECK(adf_idele_norm(t, x, p) == ADF_LIMIT && arb_equal(t, sentinel));
        CHECK(adf_adele_div_idele(az, ax, x, p) == ADF_LIMIT && adf_adele_identical(az, ao));
        limits += 10;
    }
    fmpq_one(q->q); arb_one(a->t); arb_one(b->t);
    CHECK(adf_idele_mul_rat(y, x, q, ADF_IDELE_PREC_MAX) == ADF_OK);
    CHECK(adf_idele_norm(t, x, ADF_IDELE_PREC_MAX) == ADF_OK);
    CHECK(adf_idclass_set_idele(z, x, ADF_IDELE_PREC_MAX) == ADF_OK);
    CHECK(adf_idele_pow(y, x, WORD_MIN, ADF_IDELE_PREC_MAX) == ADF_OK);
    CHECK(adf_idele_pow_tight(y, x, WORD_MAX, ADF_IDELE_PREC_MAX) == ADF_OK);
    CHECK(adf_idclass_pow(z, a, WORD_MIN, ADF_IDELE_PREC_MAX) == ADF_OK);
    CHECK(adf_idclass_pow_tight(z, a, WORD_MAX, ADF_IDELE_PREC_MAX) == ADF_OK);
    CHECK(adf_idclass_mul(z, a, b, ADF_IDELE_PREC_MAX) == ADF_OK);
    CHECK(adf_idclass_inv(z, a, ADF_IDELE_PREC_MAX) == ADF_OK);
    CHECK(adf_adele_div_idele(az, ax, x, ADF_IDELE_PREC_MAX) == ADF_OK);
    /* Arbitrarily large dyadic exponents, exact powers with both word extremes. */
    fmpz_one(c);
    for (int i = 0; i < 4; i++) for (int j = 0; j < 2; j++)
    {
        fmpz_set_si(exp, i < 2 ? WORD_MAX : WORD_MIN);
        fmpz_mul_2exp(exp, exp, i % 2 ? 100 : 0);
        arf_set_fmpz_2exp(arb_midref(x->inf), c, exp); mag_zero(arb_radref(x->inf));
        slong k = j ? WORD_MIN : WORD_MAX;
        st = adf_idele_pow(y, x, k, 2); CHECK(st == ADF_OK && arb_is_exact(y->inf));
        fmpz_set_si(kf, k); fmpz_mul(n, exp, kf); arf_set_fmpz_2exp(arb_midref(t), c, n);
        mag_zero(arb_radref(t)); CHECK(arb_equal(y->inf, t)); huge++;
    }
    printf("edges: assertions=%lu class_operations=%lu conversions=%lu overcap_calls=%lu "
           "atcap_calls=10 huge_exponent_powers=%lu failures=0\n", count, class_ops, conversions, limits, huge);
    adf_idele_clear(x); adf_idele_clear(y); adf_idele_clear(old);
    adf_idclass_clear(a); adf_idclass_clear(b); adf_idclass_clear(z); adf_idclass_clear(ali);
    adf_idclass_clear(saved); adf_adele_clear(ax); adf_adele_clear(az); adf_adele_clear(ao);
    adf_ucoset_clear(u); adf_rat_clear(q); arb_clear(t); arb_clear(sentinel);
    fmpq_clear(al); fmpq_clear(ah); fmpq_clear(bl); fmpq_clear(bh); fmpq_clear(v);
    fmpz_clear(c); fmpz_clear(n); fmpz_clear(exp); fmpz_clear(kf); flint_cleanup(); return 0;
}
