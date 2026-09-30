/* Resource-edge probes, with no construction of an over-limit rational power. */
#include <stdio.h>
#include <time.h>
#include <adelefeld.h>
int main(void)
{
    slong ks[] = {WORD_MIN, WORD_MAX, 897612484786617600, 963761198400, 720720};
    adf_ucoset_t u, v;
    adf_idele_t x, y;
    adf_ucoset_init(u); adf_ucoset_init(v); adf_idele_init(x); adf_idele_init(y);
    fmpz_one(u->c); fmpz_one(u->N);
    for (int i = 0; i < 5; i++)
    {
        clock_t begin = clock();
        adf_ucoset_pow_tight(v, u, ks[i]);
        printf("k=%ld modulus_bits=%lu CPU_seconds=%.6f normal=%d\n", ks[i], fmpz_bits(v->N),
               (double) (clock() - begin) / CLOCKS_PER_SEC, adf_ucoset_is_normal(v));
    }
    fmpq_set_si(x->r, 2, 1);
    slong k = ADF_IDELE_POW_BITS_MAX / 3 + 1;
    int st = adf_idele_pow(y, x, k, 2);
    printf("content_r=2 k=%ld conservative_bound=%ld actual_num_bits=%ld actual_den_bits=1 status=%s\n",
           k, 3 * k, k + 1, adf_status_str(st));
    adf_ucoset_clear(u); adf_ucoset_clear(v); adf_idele_clear(x); adf_idele_clear(y); flint_cleanup();
    return 0;
}
