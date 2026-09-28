/* Random inputs of adf_fball_reconstruct and adf_adele_reconstruct; recon_fuzz_check.py decides
   each line by enumerating the candidates a + N k with its own arithmetic.
   Lines:
     F A H d | lo | hi | status | q | alias_ok      (finite-ball call; the output is also run
                                                    aliased with lo and with hi, and must agree)
     R A H d | mid | rad | status | q              (adele call; the real ball as the exact rationals
                                                    mid and rad, read with arf_get_fmpq and
                                                    arf_set_mag, not with arb_get_interval_fmpz_2exp)
   The marker -99/7 is put into the output before each call; on a status other than OK the output
   must still be -99/7 (conventions 4.3).
   Usage: recon_fuzz <ncases> <seed> <bits> */

#include <stdio.h>
#include <stdlib.h>
#include <adelefeld.h>
#include <flint/arb.h>

static void
rnd(fmpz_t r, flint_rand_t st, int bits)
{
    if (n_randint(st, 2))
        fmpz_randtest(r, st, bits);
    else
        fmpz_set_si(r, (slong) n_randint(st, 25) - 12);
}

static void
marker(adf_rat_t q)
{
    fmpz_t n, d;
    fmpz_init(n); fmpz_init(d);
    fmpz_set_si(n, -99); fmpz_set_si(d, 7);
    adf_rat_set_fmpz2(q, n, d);
    fmpz_clear(n); fmpz_clear(d);
}

static void
rnd_rat(adf_rat_t q, flint_rand_t st, int bits)
{
    fmpz_t n, d;
    fmpz_init(n); fmpz_init(d);
    rnd(n, st, bits); rnd(d, st, bits / 2 + 1);
    if (fmpz_is_zero(d)) fmpz_one(d);
    adf_rat_set_fmpz2(q, n, d);
    fmpz_clear(n); fmpz_clear(d);
}

int
main(int argc, char ** argv)
{
    slong ncases = argc > 1 ? atol(argv[1]) : 1000;
    ulong seed = argc > 2 ? strtoul(argv[2], NULL, 10) : 1;
    int bits = argc > 3 ? atoi(argv[3]) : 6;
    flint_rand_t st;
    slong i;

    flint_randinit(st);
    flint_randseed(st, seed, seed + 12345);
    for (i = 0; i < ncases; i++)
    {
        adf_fball_t x;
        adf_adele_t X;
        adf_rat_t lo, hi, q, lo2, hi2;
        fmpz_t A, H, d;
        fmpq_t m, r;
        arf_t rr;
        int s, s1, s2, aok;

        adf_fball_init(x); adf_adele_init(X);
        adf_rat_init(lo); adf_rat_init(hi); adf_rat_init(q); adf_rat_init(lo2); adf_rat_init(hi2);
        fmpz_init(A); fmpz_init(H); fmpz_init(d);
        fmpq_init(m); fmpq_init(r); arf_init(rr);

        rnd(A, st, bits); rnd(H, st, bits); rnd(d, st, bits);
        fmpz_abs(H, H);
        if (n_randint(st, 4) == 0) fmpz_zero(H);
        if (fmpz_is_zero(d)) fmpz_one(d);
        adf_fball_set_fmpz3(x, A, H, d);
        adf_fball_get_fmpz3(A, H, d, x);

        rnd_rat(lo, st, bits);
        if (n_randint(st, 3) == 0)
        {
            /* lo on a lattice point: lo = a + N k */
            adf_rat_t t;
            fmpz_t k;
            adf_rat_init(t); fmpz_init(k);
            fmpz_set_si(k, (slong) n_randint(st, 9) - 4);
            adf_fball_get_radius(t, x);
            fmpq_mul_fmpz(lo->q, t->q, k);
            adf_fball_get_center(t, x);
            adf_rat_add(lo, lo, t);
            adf_rat_clear(t); fmpz_clear(k);
        }
        rnd_rat(hi, st, bits);
        if (n_randint(st, 3) == 0)
            adf_rat_set(hi, lo);
        else if (n_randint(st, 2) == 0)
        {
            adf_rat_t t;
            adf_rat_init(t);
            adf_fball_get_radius(t, x);
            adf_rat_add(hi, lo, t);           /* width exactly N */
            if (n_randint(st, 2)) { adf_rat_add(hi, hi, t); }
            adf_rat_clear(t);
        }
        marker(q);
        s = adf_fball_reconstruct(q, x, lo, hi);
        /* aliased: output = lo, output = hi */
        adf_rat_set(lo2, lo); adf_rat_set(hi2, hi);
        s1 = adf_fball_reconstruct(lo2, x, lo2, hi);
        s2 = adf_fball_reconstruct(hi2, x, lo, hi2);
        aok = (s1 == s) && (s2 == s)
              && (s == ADF_OK ? adf_rat_equal(lo2, q) && adf_rat_equal(hi2, q)
                              : adf_rat_equal(lo2, lo) && adf_rat_equal(hi2, hi));
        printf("F "); fmpz_print(A); printf(" "); fmpz_print(H); printf(" "); fmpz_print(d);
        printf(" | "); fmpq_print(lo->q); printf(" | "); fmpq_print(hi->q);
        printf(" | %d | ", s); fmpq_print(q->q); printf(" | %d\n", aok);

        /* adele call: a random finite arb */
        arb_randtest(X->inf, st, 1 + n_randint(st, 3 * bits), 1 + n_randint(st, 6));
        if (n_randint(st, 3) == 0)
        {
            /* centre the real ball on a candidate */
            adf_rat_t t;
            adf_rat_init(t);
            adf_fball_get_center(t, x);
            arb_set_fmpq(X->inf, t->q, 64);
            if (n_randint(st, 2)) mag_zero(arb_radref(X->inf));
            adf_rat_clear(t);
        }
        adf_fball_set(&X->fin, x);
        marker(q);
        s = adf_adele_reconstruct(q, X);
        arf_get_fmpq(m, arb_midref(X->inf));
        arf_set_mag(rr, arb_radref(X->inf));
        arf_get_fmpq(r, rr);
        printf("R "); fmpz_print(A); printf(" "); fmpz_print(H); printf(" "); fmpz_print(d);
        printf(" | "); fmpq_print(m); printf(" | "); fmpq_print(r);
        printf(" | %d | ", s); fmpq_print(q->q); printf("\n");

        adf_fball_clear(x); adf_adele_clear(X);
        adf_rat_clear(lo); adf_rat_clear(hi); adf_rat_clear(q); adf_rat_clear(lo2); adf_rat_clear(hi2);
        fmpz_clear(A); fmpz_clear(H); fmpz_clear(d);
        fmpq_clear(m); fmpq_clear(r); arf_clear(rr);
    }
    flint_randclear(st);
    flint_cleanup();
    return 0;
}
