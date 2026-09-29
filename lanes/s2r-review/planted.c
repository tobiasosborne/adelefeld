#include <stdio.h>
#include <stdlib.h>
#include <flint/fmpz_poly.h>
#include <flint/fmpq.h>
#include <flint/arb.h>
#include <adelefeld.h>

static void add_root(fmpz_poly_t f, const fmpz_t num, const fmpz_t den, int mult)
{
    fmpz_poly_t t;
    fmpz_t neg;
    int j;
    fmpz_poly_init(t);
    fmpz_init(neg);
    fmpz_neg(neg, num);
    fmpz_poly_set_coeff_fmpz(t, 0, neg);
    fmpz_poly_set_coeff_fmpz(t, 1, den);
    for (j = 0; j < mult; j++) fmpz_poly_mul(f, f, t);
    fmpz_clear(neg);
    fmpz_poly_clear(t);
}

static int contains(const arb_t b, const fmpq_t root)
{
    fmpz_t lo, hi, e, x;
    fmpq_t q;
    int hit;
    fmpz_init(lo); fmpz_init(hi); fmpz_init(e); fmpz_init(x); fmpq_init(q);
    arb_get_interval_fmpz_2exp(lo, hi, e, b);
    if (fmpz_sgn(e) >= 0) {
        fmpz_mul_2exp(lo, lo, fmpz_get_ui(e));
        fmpz_mul_2exp(hi, hi, fmpz_get_ui(e));
        fmpz_one(x);
    } else {
        fmpz_neg(e, e);
        fmpz_one(x);
        fmpz_mul_2exp(x, x, fmpz_get_ui(e));
    }
    fmpq_set_fmpz_frac(q, lo, x);
    hit = fmpq_cmp(q, root) <= 0;
    fmpq_set_fmpz_frac(q, hi, x);
    hit = hit && fmpq_cmp(root, q) <= 0;
    fmpq_clear(q); fmpz_clear(lo); fmpz_clear(hi); fmpz_clear(e); fmpz_clear(x);
    return hit;
}

int main(int argc, char **argv)
{
    fmpz_poly_t f, q;
    adf_rootlist_t L;
    fmpz_t num, den, c;
    fmpq_t r[4];
    int k, i, n, status, hits, errors = 0, prec, exponent;
    if (argc < 3 || argc > 5) return 2;
    k = atoi(argv[1]); prec = atoi(argv[2]);
    exponent = argc > 3 ? atoi(argv[3]) : 3000;
    fmpz_poly_init(f); fmpz_poly_init(q); adf_rootlist_init(L);
    fmpz_init(num); fmpz_init(den); fmpz_init(c);
    for (i = 0; i < 4; i++) fmpq_init(r[i]);
    fmpz_poly_one(f);
    if (k == 0) n = 0;
    else if (k == 1) {
        n = 2;
        fmpz_zero(num); fmpz_one(den); add_root(f, num, den, 2); fmpq_zero(r[0]);
        fmpz_one(num); fmpz_mul_2exp(den, den, 200); add_root(f, num, den, 1);
        fmpq_set_fmpz_frac(r[1], num, den);
    } else if (k == 2) {
        n = 1;
        fmpz_one(num); fmpz_mul_2exp(num, num, 3000); fmpz_one(den);
        add_root(f, num, den, 1); fmpq_set_fmpz_frac(r[0], num, den);
    } else if (k == 3) {
        n = 1;
        fmpz_one(num); fmpz_one(den); fmpz_mul_2exp(den, den, 3000);
        add_root(f, num, den, 1); fmpq_set_fmpz_frac(r[0], num, den);
    } else if (k == 4) {
        n = 2;
        fmpz_one(den); fmpz_one(num); fmpz_mul_2exp(num, num, exponent);
        add_root(f, num, den, 1); fmpq_set_fmpz_frac(r[0], num, den);
        fmpz_add_ui(num, num, 1); add_root(f, num, den, 1);
        fmpq_set_fmpz_frac(r[1], num, den);
    } else if (k == 5) {
        n = 3; fmpz_one(den);
        for (i = 0; i < n; i++) {
            fmpz_set_si(num, i - 1); add_root(f, num, den, 1);
            fmpq_set_fmpz_frac(r[i], num, den);
        }
    } else if (k == 6) {
        n = 2;
        fmpz_poly_set_coeff_si(f, 0, -1);
        fmpz_poly_set_coeff_si(f, 60, 1);
        fmpq_set_si(r[0], -1, 1); fmpq_set_si(r[1], 1, 1);
    } else if (k == 7) {
        n = 0;
        fmpz_poly_set_coeff_si(f, 0, 1);
        fmpz_poly_set_coeff_si(f, 2, 1);
    } else if (k == 8) {
        n = 2; fmpz_one(den);
        fmpz_set_si(num, -2); add_root(f, num, den, 1); fmpq_set_si(r[0], -2, 1);
        fmpz_set_si(num, 1); add_root(f, num, den, 1); fmpq_set_si(r[1], 1, 1);
        fmpz_poly_set_coeff_si(q, 0, 1);
        fmpz_poly_set_coeff_si(q, 2, 1);
        fmpz_poly_mul(f, f, q);
    } else return 2;
    fmpz_set_ui(c, 1234567); fmpz_poly_scalar_mul_fmpz(f, f, c);
    status = adf_roots_real(L, f, prec);
    printf("case=%d prec=%d degree=%ld status=%d n=%ld count=%ld\n",
           k, prec, (long)fmpz_poly_degree(f), status, (long)L->n, (long)L->count);
    if (k == 4 && status == ADF_OK) {
        fmpz_t lo, hi, exp;
        fmpz_init(lo); fmpz_init(hi); fmpz_init(exp);
        for (i = 0; i < L->n; i++) {
            arb_get_interval_fmpz_2exp(lo, hi, exp, L->ball + i);
            printf("ball_%d_midpoint_bits=%ld accuracy=%ld\n", i,
                   (long)arb_bits(L->ball + i), (long)arb_rel_accuracy_bits(L->ball + i));
            printf("ball_%d_endpoint_bits=%ld,%ld exponent=", i,
                   (long)fmpz_bits(lo), (long)fmpz_bits(hi));
            fmpz_print(exp); putchar('\n');
        }
        fmpz_clear(lo); fmpz_clear(hi); fmpz_clear(exp);
    }
    fflush(stdout);
    if (status == ADF_OK) {
        if (L->n != n) errors++;
        for (i = 0; i < n; i++) {
            slong j;
            hits = 0;
            for (j = 0; j < L->n; j++) hits += contains(L->ball + j, r[i]);
            if (hits != 1) errors++;
        }
        if (argc < 5 && (!adf_rootlist_verify_entries(L, f) ||
                         !adf_rootlist_verify_complete(L, f, 0))) errors++;
        for (i = 0; i < L->n; i++)
            if (!arb_is_exact(L->ball + i) && arb_rel_accuracy_bits(L->ball + i) < FLINT_MAX(prec, 2)) errors++;
    }
    printf("errors=%d\n", errors);
    for (i = 0; i < 4; i++) fmpq_clear(r[i]);
    fmpz_clear(num); fmpz_clear(den); fmpz_clear(c);
    fmpz_poly_clear(f); fmpz_poly_clear(q); adf_rootlist_clear(L);
    return errors != 0;
}
