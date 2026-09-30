/* Reproducers for claims in the public headers and api-2.md. */
#include <stdio.h>
#include <adelefeld.h>

static void show(const char *name, const arb_t t)
{
    printf("%s: ", name); arb_printd(t, 25);
    printf(" exact=%d\n", arb_is_exact(t));
}
int main(void)
{
    adf_idele_t x, y;
    adf_idclass_t cl;
    adf_adele_t simple, small;
    adf_rat_t q, radius;
    arb_t t;
    fmpz_t c, n;
    int st;
    adf_idele_init(x); adf_idele_init(y); adf_idclass_init(cl);
    adf_adele_init(simple); adf_adele_init(small); adf_rat_init(q); adf_rat_init(radius);
    arb_init(t); fmpz_init(c); fmpz_init(n);

    /* Exact input rational, higher constructor precision than operation precision. */
    fmpq_set_si(q->q, 5, 1);
    st = adf_idele_set_rat(x, q, 64);
    printf("F1 rational construction: %s input_exact=%d\n", adf_status_str(st), arb_is_exact(x->inf));
    st = adf_idclass_set_idele(cl, x, 2);
    printf("F1 class status=%s ", adf_status_str(st)); show("t", cl->t);
    st = adf_idele_norm(t, x, 2);
    printf("F1 norm status=%s ", adf_status_str(st)); show("t", t);

    /* A negative exponent whose positive power fits in the working mantissa. */
    arb_set_si(x->inf, 3); fmpq_one(x->r); adf_ucoset_one(&x->u);
    st = adf_idele_pow(y, x, -1, 2);
    printf("F2 negative power: status=%s ", adf_status_str(st)); show("real", y->inf);
    printf("F2 |m|^|k|=3, bits=2, prec=2; true power=1/3\n");

    /* A proof item that does not restrict k to positive values. */
    arb_set_si(x->inf, 2); fmpq_set_si(x->r, 2, 1);
    st = adf_idele_pow(y, x, -1, 64);
    printf("F3 k=-1 status=%s ", adf_status_str(st)); show("power", y->inf);
    show("product of |k|=1 copies", x->inf);
    printf("F3 power content="); fmpq_print(y->r);
    printf(" product content="); fmpq_print(x->r); putchar('\n');

    /* Numerical radius is the positive rational generator, not a real interval width. */
    arb_one(x->inf); fmpq_one(x->r); fmpz_set_si(c, 2); fmpz_set_si(n, 3);
    st = adf_ucoset_set_fmpz2(&x->u, c, n);
    if (st != ADF_OK) return 2;
    adf_adele_set_idele_simple(simple, x); adf_adele_set_idele(small, x);
    adf_fball_get_radius(radius, &simple->fin);
    printf("F4 simple radius="); fmpq_print(radius->q);
    adf_fball_get_radius(radius, &small->fin);
    printf(" smallest radius="); fmpq_print(radius->q); putchar('\n');

    adf_idele_clear(x); adf_idele_clear(y); adf_idclass_clear(cl);
    adf_adele_clear(simple); adf_adele_clear(small); adf_rat_clear(q); adf_rat_clear(radius);
    arb_clear(t); fmpz_clear(c); fmpz_clear(n); flint_cleanup();
    return 0;
}
