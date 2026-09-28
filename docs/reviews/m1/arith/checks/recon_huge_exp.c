/* R1 reproducer: adf_adele_reconstruct with a finite real ball whose exponent does not fit a
   machine word. recon.c:95 and :106 truncate the fmpz exponent with fmpz_get_ui / fmpz_get_si.

   Case 1: inf = 3 * 2^(2^64 + 1), exact (radius 0); fin = the exact 6.
           True interval: the single point 3 * 2^(2^64+1), which is not 6 (it is > 2^(2^64)).
           True answer: ADF_NO_SOLUTION.
   Case 2: inf = 1 * 2^(-(2^64 + 3)), exact; fin = the exact 1/8.
           True interval: the single point 2^-(2^64+3), which is not 1/8.
           True answer: ADF_NO_SOLUTION.
   Case 3: inf = 1 * 2^(2^64), exact; fin = the exact 1. True answer: ADF_NO_SOLUTION.

   Build: cc -Iinclude recon_huge_exp.c build/libadelefeld.a -lflint -lgmp -lm */

#include <stdio.h>
#include <adelefeld.h>
#include <flint/arb.h>

static void
run(const char * label, const char * exp_str, slong fin_num, slong fin_den)
{
    adf_adele_t x;
    adf_rat_t q, f;
    fmpz_t e, one;
    int st;
    char * s;

    adf_adele_init(x);
    adf_rat_init(q);
    adf_rat_init(f);
    fmpz_init(e);
    fmpz_init(one);

    fmpz_set_str(e, exp_str, 10);
    /* inf = mantissa * 2^e: built from an exact integer with arb_mul_2exp_fmpz. */
    arb_set_si(x->inf, label[0] == 'A' ? 3 : 1);
    arb_mul_2exp_fmpz(x->inf, x->inf, e);
    fmpz_set_si(one, fin_den);
    {
        fmpz_t n;
        fmpz_init(n);
        fmpz_set_si(n, fin_num);
        adf_rat_set_fmpz2(f, n, one);
        fmpz_clear(n);
    }
    adf_fball_set_rat(&x->fin, f);
    adf_rat_set_si(q, -99);

    printf("%s: arb_is_finite=%d arb_is_exact=%d adf_adele_is_canonical=%d\n", label,
           arb_is_finite(x->inf), arb_is_exact(x->inf), adf_adele_is_canonical(x));
    printf("  exponent of the ball = %s\n", exp_str);
    {
        fmpz_t a, b, ex;
        fmpz_init(a); fmpz_init(b); fmpz_init(ex);
        arb_get_interval_fmpz_2exp(a, b, ex, x->inf);
        printf("  arb_get_interval_fmpz_2exp: a = "); fmpz_print(a);
        printf(", b = "); fmpz_print(b); printf(", exp = "); fmpz_print(ex);
        printf("; fmpz_get_ui(exp) = %lu, fmpz_get_si(exp) = %ld\n",
               (unsigned long) fmpz_get_ui(ex), (long) fmpz_get_si(ex));
        fmpz_clear(a); fmpz_clear(b); fmpz_clear(ex);
    }
    st = adf_adele_reconstruct(q, x);
    s = fmpq_get_str(NULL, 10, q->q);
    printf("  status = %s (%d), q = %s\n", adf_status_str(st), st, s);
    flint_free(s);
    printf("  expected: NO_SOLUTION (the real ball is the single point m*2^e, and m*2^e != %ld/%ld)\n",
           (long) fin_num, (long) fin_den);

    adf_adele_clear(x);
    adf_rat_clear(q);
    adf_rat_clear(f);
    fmpz_clear(e);
    fmpz_clear(one);
}

int
main(void)
{
    /* 2^64 + 1 = 18446744073709551617 */
    run("A (3 * 2^(2^64+1), fin = 6)", "18446744073709551617", 6, 1);
    /* -(2^64 + 3) = -18446744073709551619 */
    run("B (2^-(2^64+3), fin = 1/8)", "-18446744073709551619", 1, 8);
    /* 2^64 = 18446744073709551616 */
    run("B' (2^(2^64), fin = 1)", "18446744073709551616", 1, 1);
    /* Exponents that fit a word: 2^62 + 1 and -(2^62 + 3). fmpz_mul_2exp wraps the shift
       (see probe_mul_2exp.out), so these fail as well. */
    run("A2 (3 * 2^(2^62+1), fin = 6)", "4611686018427387905", 6, 1);
    run("C2 (2^(2^62+1), fin = 2)", "4611686018427387905", 2, 1);
    run("C3 (2^(2^63+5), fin = 32)", "9223372036854775813", 32, 1);
    run("B2 (2^-(2^62+3), fin = 1/8)", "-4611686018427387907", 1, 8);
    /* exponent -2^63 = WORD_MIN: recon.c:106 computes -fmpz_get_si(exp) = -WORD_MIN, a signed
       overflow (undefined behaviour; the .san build reports it). */
    run("D (2^-(2^63), fin = 1)", "-9223372036854775808", 1, 1);
    return 0;
}
