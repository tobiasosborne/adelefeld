/* Random inputs through every function of rat.c; rat_fuzz_check.py decides with Fraction.
   Line: n1 d1 n2 d2 | x | y | add | sub | mul | neg | div st v | inv st v | sgn | eq | zero |
         canon flags | alias ok
   x = adf_rat_set_fmpq of the raw (possibly non-canonical, negative-denominator) fraction n1/d1
   (d1 = 0 gives DOMAIN and x keeps the marker 5/7); y = adf_rat_set_fmpz2(n2, d2) likewise.
   Division and inversion by 0 must return NOT_UNIT with the output untouched (marker -99/7).
   Usage: rat_fuzz <ncases> <seed> <bits> */

#include <stdio.h>
#include <stdlib.h>
#include <adelefeld.h>

static void
setm(adf_rat_t q, slong n, slong d)
{
    fmpz_t a, b;
    fmpz_init(a); fmpz_init(b);
    fmpz_set_si(a, n); fmpz_set_si(b, d);
    adf_rat_set_fmpz2(q, a, b);
    fmpz_clear(a); fmpz_clear(b);
}

static void
rnd(fmpz_t r, flint_rand_t st, int bits)
{
    if (n_randint(st, 2))
        fmpz_randtest(r, st, bits);
    else
        fmpz_set_si(r, (slong) n_randint(st, 13) - 6);
}

typedef void (*bin)(adf_rat_t, const adf_rat_t, const adf_rat_t);

static int
alias_ok(bin f, const adf_rat_t x, const adf_rat_t y)
{
    adf_rat_t r, a, b, r2;
    int ok;
    adf_rat_init(r); adf_rat_init(a); adf_rat_init(b); adf_rat_init(r2);
    f(r, x, y);
    adf_rat_set(a, x); f(a, a, y);
    adf_rat_set(b, y); f(b, x, b);
    ok = adf_rat_identical(a, r) && adf_rat_identical(b, r);
    f(r2, x, x); adf_rat_set(a, x); f(a, a, a);
    ok = ok && adf_rat_identical(a, r2);
    adf_rat_clear(r); adf_rat_clear(a); adf_rat_clear(b); adf_rat_clear(r2);
    return ok;
}

int
main(int argc, char ** argv)
{
    slong n = argc > 1 ? atol(argv[1]) : 1000;
    ulong seed = argc > 2 ? strtoul(argv[2], NULL, 10) : 1;
    int bits = argc > 3 ? atoi(argv[3]) : 8;
    flint_rand_t st;
    slong i;

    flint_randinit(st);
    flint_randseed(st, seed, seed + 99);
    for (i = 0; i < n; i++)
    {
        fmpz_t n1, d1, n2, d2;
        fmpq_t raw;
        adf_rat_t x, y, z;
        int s1, s2, s, canon = 1, aok = 1;

        fmpz_init(n1); fmpz_init(d1); fmpz_init(n2); fmpz_init(d2); fmpq_init(raw);
        adf_rat_init(x); adf_rat_init(y); adf_rat_init(z);
        rnd(n1, st, bits); rnd(d1, st, bits); rnd(n2, st, bits); rnd(d2, st, bits);
        if (n_randint(st, 3) == 0) { fmpz_mul_si(n1, n1, 6); fmpz_mul_si(d1, d1, -4); }
        fmpz_set(fmpq_numref(raw), n1);
        fmpz_set(fmpq_denref(raw), d1);
        setm(x, 5, 7);
        setm(y, 5, 7);
        s1 = adf_rat_set_fmpq(x, raw);
        s2 = adf_rat_set_fmpz2(y, n2, d2);
        fmpz_print(n1); printf(" "); fmpz_print(d1); printf(" "); fmpz_print(n2); printf(" ");
        fmpz_print(d2);
        printf(" | %d ", s1); fmpq_print(x->q); printf(" | %d ", s2); fmpq_print(y->q);
        canon &= adf_rat_is_canonical(x) && adf_rat_is_canonical(y);
        adf_rat_add(z, x, y); canon &= adf_rat_is_canonical(z);
        printf(" | "); fmpq_print(z->q); aok &= alias_ok(adf_rat_add, x, y);
        adf_rat_sub(z, x, y); canon &= adf_rat_is_canonical(z);
        printf(" | "); fmpq_print(z->q); aok &= alias_ok(adf_rat_sub, x, y);
        adf_rat_mul(z, x, y); canon &= adf_rat_is_canonical(z);
        printf(" | "); fmpq_print(z->q); aok &= alias_ok(adf_rat_mul, x, y);
        adf_rat_neg(z, x); canon &= adf_rat_is_canonical(z);
        printf(" | "); fmpq_print(z->q);
        setm(z, -99, 7);
        s = adf_rat_div(z, x, y); canon &= adf_rat_is_canonical(z);
        printf(" | %d ", s); fmpq_print(z->q);
        {
            adf_rat_t a;
            adf_rat_init(a);
            adf_rat_set(a, x);
            adf_rat_div(a, a, y);
            if (s == ADF_OK) aok &= adf_rat_identical(a, z);
            else aok &= adf_rat_identical(a, x);
            adf_rat_set(a, y);
            adf_rat_div(a, a, a);
            if (s == ADF_OK) aok &= adf_rat_is_canonical(a) && fmpq_is_one(a->q);
            adf_rat_clear(a);
        }
        setm(z, -99, 7);
        s = adf_rat_inv(z, y); canon &= adf_rat_is_canonical(z);
        printf(" | %d ", s); fmpq_print(z->q);
        {
            adf_rat_t a;
            adf_rat_init(a);
            adf_rat_set(a, y);
            adf_rat_inv(a, a);
            if (s == ADF_OK) aok &= adf_rat_identical(a, z);
            else aok &= adf_rat_identical(a, y);
            adf_rat_clear(a);
        }
        printf(" | %d | %d | %d | %d | %d\n", adf_rat_sgn(x), adf_rat_equal(x, y),
               adf_rat_is_zero(y), canon, aok);
        fmpz_clear(n1); fmpz_clear(d1); fmpz_clear(n2); fmpz_clear(d2); fmpq_clear(raw);
        adf_rat_clear(x); adf_rat_clear(y); adf_rat_clear(z);
    }
    flint_randclear(st);
    flint_cleanup();
    return 0;
}
