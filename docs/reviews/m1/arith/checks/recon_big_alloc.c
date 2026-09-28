/* R2 reproducer: adf_adele_reconstruct on a finite real ball whose exact end points need a huge
   integer. The answer is decidable from a few words (the finite ball is the exact 1), but the
   function materialises the end points in full and aborts.

   argv[1] = 1: inf = 2^(2^36), exact (one-word exponent). recon.c:95 shifts by 2^36 bits (8 GiB).
   argv[1] = 2: inf = 1 +/- 2^(-2^36). arb_get_interval_fmpz_2exp itself builds integers of 2^36 bits
                (refs/src/flint-3.0.1/arb.rst:468-474 warns of exactly this).
   argv[1] = 3: inf = 2^(2^62), exact. recon.c:95 shifts by 2^62 bits.
   In every case the true answer is ADF_NO_SOLUTION (case 1, 3: the point is not 1; case 2: the
   interval [1 - 2^-(2^36), 1 + 2^-(2^36)] contains 1, so the true answer is ADF_OK with q = 1).
   Run under `ulimit -v 2000000` so that the allocation fails fast instead of swapping. */

#include <stdio.h>
#include <stdlib.h>
#include <adelefeld.h>
#include <flint/arb.h>

int
main(int argc, char ** argv)
{
    adf_adele_t x;
    adf_rat_t q;
    fmpz_t e;
    int which = argc > 1 ? atoi(argv[1]) : 1;
    int st;

    adf_adele_init(x);
    adf_rat_init(q);
    fmpz_init(e);
    adf_fball_one(&x->fin);

    if (which == 1 || which == 3)
    {
        fmpz_one(e);
        fmpz_mul_2exp(e, e, which == 1 ? 36 : 62);
        arb_one(x->inf);
        arb_mul_2exp_fmpz(x->inf, x->inf, e);
    }
    else
    {
        fmpz_one(e);
        fmpz_mul_2exp(e, e, 36);
        fmpz_neg(e, e);
        arb_one(x->inf);
        mag_one(arb_radref(x->inf));
        mag_mul_2exp_fmpz(arb_radref(x->inf), arb_radref(x->inf), e);
    }
    printf("case %d: arb_is_finite=%d adf_adele_is_canonical=%d; calling adf_adele_reconstruct\n",
           which, arb_is_finite(x->inf), adf_adele_is_canonical(x));
    fflush(stdout);
    st = adf_adele_reconstruct(q, x);
    printf("status = %s\n", adf_status_str(st));
    adf_adele_clear(x);
    adf_rat_clear(q);
    fmpz_clear(e);
    return 0;
}
