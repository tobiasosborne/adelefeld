/* Random global finite balls through every public function of the global backend of fball.c and
   the functions of rat.c; the results are printed, and fball_fuzz_check.py decides them with its
   own oracle (fractions.Fraction, enumeration of lattice points). Aliasing is checked here: every
   binary operation is also run as (x, x, y), (y, x, y) and (x, x, x) and must give a result
   identical (adf_fball_identical) to the unaliased call (conventions 4.1).

   Line format (one per case, fields separated by " ; "):
     x A H d ; y A H d ; q num/den ;
     add ... ; sub ... ; neg ... ; mul ... ; sq ... ; mulq ... ; divq st A H d ;
     eq ; ov ; cxy ; cyx ; cmp ; crq ; ctr c ; rad N ; den d ; vol v ; prec2 st e ; prec3 st e ;
     canon flags ; alias flags
   The inputs are built from raw random triples with adf_fball_set_fmpz3 (the raw triple is also
   printed, so the oracle checks the canonicalisation). Usage: fball_fuzz <ncases> <seed> <bits> */

#include <stdio.h>
#include <stdlib.h>
#include <adelefeld.h>

static void
pr3(const adf_fball_t x)
{
    fmpz_t A, H, d;
    fmpz_init(A); fmpz_init(H); fmpz_init(d);
    adf_fball_get_fmpz3(A, H, d, x);
    fmpz_print(A); printf(" "); fmpz_print(H); printf(" "); fmpz_print(d);
    fmpz_clear(A); fmpz_clear(H); fmpz_clear(d);
}

static void
rnd(fmpz_t r, flint_rand_t st, int bits)
{
    if (bits <= 0) { fmpz_zero(r); return; }
    if (n_randint(st, 2))
        fmpz_randtest(r, st, bits);     /* up to bits bits, biased to special values */
    else
        fmpz_set_si(r, (slong) n_randint(st, 25) - 12);   /* uniform in [-12, 12] */
}

typedef void (*binop)(adf_fball_t, const adf_fball_t, const adf_fball_t);

static int alias_fail = 0;

static void
check_alias(binop f, const adf_fball_t x, const adf_fball_t y, const adf_fball_t ref,
            const char * name)
{
    adf_fball_t a, b, r2;
    adf_fball_init(a); adf_fball_init(b); adf_fball_init(r2);
    adf_fball_set(a, x);
    f(a, a, y);
    if (!adf_fball_identical(a, ref)) { alias_fail++; printf("#ALIAS %s (x,x,y)\n", name); }
    adf_fball_set(b, y);
    f(b, x, b);
    if (!adf_fball_identical(b, ref)) { alias_fail++; printf("#ALIAS %s (y,x,y)\n", name); }
    adf_fball_set(a, x);
    f(r2, x, x);
    f(a, a, a);
    if (!adf_fball_identical(a, r2)) { alias_fail++; printf("#ALIAS %s (x,x,x)\n", name); }
    adf_fball_clear(a); adf_fball_clear(b); adf_fball_clear(r2);
}

int
main(int argc, char ** argv)
{
    slong ncases = argc > 1 ? atol(argv[1]) : 1000;
    ulong seed = argc > 2 ? strtoul(argv[2], NULL, 10) : 1;
    int bits = argc > 3 ? atoi(argv[3]) : 6;
    flint_rand_t st;
    slong i;
    adf_place_t p2, p3;

    adf_place_prime(&p2, 2);
    adf_place_prime(&p3, 3);
    flint_randinit(st);
    flint_randseed(st, seed, seed ^ 0x9e3779b97f4a7c15ULL);

    for (i = 0; i < ncases; i++)
    {
        adf_fball_t x, y, z, t;
        adf_rat_t q, r;
        fmpz_t A, H, d, B, K, e, qn, qd;
        slong ex;
        int s, canon = 1;

        adf_fball_init(x); adf_fball_init(y); adf_fball_init(z); adf_fball_init(t);
        adf_rat_init(q); adf_rat_init(r);
        fmpz_init(A); fmpz_init(H); fmpz_init(d); fmpz_init(B); fmpz_init(K); fmpz_init(e);
        fmpz_init(qn); fmpz_init(qd);

        rnd(A, st, bits); rnd(H, st, bits); rnd(d, st, bits);
        fmpz_abs(H, H);
        if (n_randint(st, 4) == 0) fmpz_zero(H);
        if (fmpz_is_zero(d)) fmpz_set_si(d, n_randint(st, 2) ? 1 : -1);
        s = adf_fball_set_fmpz3(x, A, H, d);
        printf("x "); fmpz_print(A); printf(" "); fmpz_print(H); printf(" "); fmpz_print(d);
        printf(" %d ", s); pr3(x);

        rnd(B, st, bits); rnd(K, st, bits); rnd(e, st, bits);
        fmpz_abs(K, K);
        if (n_randint(st, 4) == 0) fmpz_zero(K);
        if (fmpz_is_zero(e)) fmpz_set_si(e, n_randint(st, 2) ? 1 : -1);
        /* sometimes y shares structure with x */
        if (n_randint(st, 5) == 0) { fmpz_set(K, H); fmpz_set(e, d); }
        s = adf_fball_set_fmpz3(y, B, K, e);
        printf(" ; y "); fmpz_print(B); printf(" "); fmpz_print(K); printf(" "); fmpz_print(e);
        printf(" %d ", s); pr3(y);

        rnd(qn, st, bits); rnd(qd, st, bits);
        if (n_randint(st, 6) == 0) fmpz_zero(qn);
        s = adf_rat_set_fmpz2(q, qn, qd);
        if (s != ADF_OK) adf_rat_set_si(q, n_randint(st, 2) ? 0 : -1);
        printf(" ; q "); fmpq_print(q->q);

        adf_fball_add(z, x, y); canon &= adf_fball_is_canonical(z);
        printf(" ; add "); pr3(z); check_alias(adf_fball_add, x, y, z, "add");
        adf_fball_sub(z, x, y); canon &= adf_fball_is_canonical(z);
        printf(" ; sub "); pr3(z); check_alias(adf_fball_sub, x, y, z, "sub");
        adf_fball_neg(z, x); canon &= adf_fball_is_canonical(z);
        printf(" ; neg "); pr3(z);
        adf_fball_set(t, x); adf_fball_neg(t, t);
        if (!adf_fball_identical(t, z)) { alias_fail++; printf("#ALIAS neg\n"); }
        adf_fball_mul(z, x, y); canon &= adf_fball_is_canonical(z);
        printf(" ; mul "); pr3(z); check_alias(adf_fball_mul, x, y, z, "mul");
        adf_fball_mul(z, x, x); canon &= adf_fball_is_canonical(z);
        printf(" ; sq "); pr3(z);
        adf_fball_mul_rat(z, x, q); canon &= adf_fball_is_canonical(z);
        printf(" ; mulq "); pr3(z);
        adf_fball_set(t, x); adf_fball_mul_rat(t, t, q);
        if (!adf_fball_identical(t, z)) { alias_fail++; printf("#ALIAS mul_rat\n"); }
        adf_fball_set_si(z, -12345);
        s = adf_fball_div_rat(z, x, q); canon &= adf_fball_is_canonical(z);
        printf(" ; divq %d ", s); pr3(z);
        adf_fball_set(t, x); adf_fball_div_rat(t, t, q);
        if (s == ADF_OK && !adf_fball_identical(t, z)) { alias_fail++; printf("#ALIAS div_rat\n"); }

        printf(" ; eq %d ; ov %d ; cxy %d ; cyx %d ; cmp %d ; crq %d",
               adf_fball_equal_set(x, y), adf_fball_overlaps(x, y), adf_fball_contains(x, y),
               adf_fball_contains(y, x), adf_fball_compare(x, y), adf_fball_contains_rat(x, q));
        adf_fball_get_center(r, x); printf(" ; ctr "); fmpq_print(r->q);
        adf_fball_get_radius(r, x); printf(" ; rad "); fmpq_print(r->q);
        adf_fball_get_den(A, x); printf(" ; den "); fmpz_print(A);
        adf_fball_haar_volume(r, x); printf(" ; vol "); fmpq_print(r->q);
        ex = 777;
        s = adf_fball_prec_at(&ex, x, p2); printf(" ; prec2 %d %ld", s, (long) ex);
        ex = 777;
        s = adf_fball_prec_at(&ex, x, p3); printf(" ; prec3 %d %ld", s, (long) ex);
        printf(" ; canon %d ; exact %d\n", canon, adf_fball_is_exact(x));

        adf_fball_clear(x); adf_fball_clear(y); adf_fball_clear(z); adf_fball_clear(t);
        adf_rat_clear(q); adf_rat_clear(r);
        fmpz_clear(A); fmpz_clear(H); fmpz_clear(d); fmpz_clear(B); fmpz_clear(K); fmpz_clear(e);
        fmpz_clear(qn); fmpz_clear(qd);
    }
    printf("#alias_failures %d\n", alias_fail);
    flint_randclear(st);
    flint_cleanup();
    return 0;
}
