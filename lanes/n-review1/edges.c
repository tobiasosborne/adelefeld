#include <adelefeld.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char **argv)
{
    if (argc < 2) return 2;
    if (!strcmp(argv[1], "overflow"))
    {
        adf_lball_t x, w, u;
        slong m = 17; ulong index = 17;
        adf_lball_init(x); adf_lball_init(w); adf_lball_init(u);
        x->p = 2; x->exact = 0; x->v = -1; x->N = LONG_MAX; fmpq_one(x->u);
        printf("canonical=%d v=%ld N=%ld\n", adf_lball_is_canonical(x), x->v, x->N); fflush(stdout);
        int st = adf_lball_decompose_teich(&m, w, &index, u, x, 2);
        printf("status=%s m=%ld index=%lu\n", adf_status_str(st), m, index);
        adf_lball_clear(x); adf_lball_clear(w); adf_lball_clear(u);
    }
    else if (!strcmp(argv[1], "projection"))
    {
        fmpz_t A, H, d;
        adf_fball_t f;
        adf_lball_t x;
        adf_place_t p;
        ulong k = ADF_LBALL_BITS_MAX/2 + 1;
        fmpz_init(A); fmpz_init(H); fmpz_init(d);
        adf_fball_init(f); adf_lball_init(x); adf_place_prime(&p, 3);
        fmpz_set_ui(H, 6); fmpz_pow_ui(H, H, k);
        fmpz_one(A); fmpz_mul_2exp(A, A, 2*k); fmpz_one(d);
        adf_fball_set_fmpz3(f, A, H, d);
        printf("k=%lu bitsA=%lu bitsH=%lu canonical=%d\n", k, fmpz_bits(A), fmpz_bits(H),
               adf_fball_is_canonical(f)); fflush(stdout);
        clock_t begin = clock();
        int st = adf_lball_set_fball(x, p, f);
        printf("status=%s seconds=%.6f\n", adf_status_str(st), (double)(clock()-begin)/CLOCKS_PER_SEC);
        fmpz_clear(A); fmpz_clear(H); fmpz_clear(d); adf_fball_clear(f); adf_lball_clear(x);
    }
    else if (!strcmp(argv[1], "printer"))
    {
        if (argc != 5) return 2;
        slong b = atol(argv[2]);
        slong e = atol(argv[3]);
        int negative = atoi(argv[4]);
        adf_idele_t x; adf_idclass_t c;
        arf_t small; fmpz_t z;
        adf_idele_init(x); adf_idclass_init(c); arf_init(small); fmpz_init(z);
        fmpz_one(z); fmpz_mul_2exp(z, z, (ulong)b); fmpz_add_ui(z, z, 1);
        arf_set_fmpz(arb_midref(x->inf), z);
        arf_mul_2exp_si(arb_midref(x->inf), arb_midref(x->inf), e-b);
        mag_one(arb_radref(x->inf)); mag_mul_2exp_si(arb_radref(x->inf), arb_radref(x->inf), e);
        if (negative) arb_neg(x->inf, x->inf);
        printf("b=%ld scale=%ld negative=%d midpoint_exp=%ld radius_exp=%ld canonical=%d\n",
               b, e, negative, ARF_EXP(arb_midref(x->inf)), MAG_EXP(arb_radref(x->inf)),
               adf_idele_is_canonical(x)); fflush(stdout);
        size_t len = 17; clock_t begin = clock();
        char *s = adf_idele_get_str(&len, x, 1);
        printf("seconds=%.6f null=%d len=%zu\n", (double)(clock()-begin)/CLOCKS_PER_SEC, s == NULL, len);
        if (s) { printf("prefix=%.80s\n", s); adf_str_free(s); }
        adf_idele_clear(x); adf_idclass_clear(c); arf_clear(small); fmpz_clear(z);
    }
    else return 2;
    flint_cleanup(); return 0;
}
