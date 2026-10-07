/* Standalone public headers in C11 and C++17; inclusive precision-cap observation. */
#include <adelefeld/char.h>
#include <adelefeld/dump.h>
#include <stdio.h>
#include <string.h>
int main(void)
{
    adf_char_t x, y; adf_ucoset_t u; acb_t z; fmpz_t c, N; size_t len, count = 55;
    adf_char_init(x); adf_char_init(y); adf_ucoset_init(u); acb_init(z); fmpz_init(c); fmpz_init(N);
    int st = adf_char_set_conrey(x, 7, 3);
    fmpz_set_ui(c, 3); fmpz_set_ui(N, 7); st |= adf_ucoset_set_fmpz2(u, c, N);
    int cap = adf_char_eval_ucoset(z, x, u, ADF_REAL_PREC_MAX);
    if (cap != ADF_OK && cap != ADF_NOT_DETERMINED) return 1;
    adf_char_conj(y, x); adf_char_conj(y, y);
    if (!adf_char_identical(x, y)) return 2;
    char *s = adf_char_dump_str(&len, x);
    st |= adf_char_dump_inspect(&count, NULL, s, len, NULL);
    st |= adf_char_load_str(y, s, len, NULL, NULL);
    if (count != 0 || !adf_char_identical(x, y)) return 3;
    printf("header probe: sizeof=%zu align=%zu cap-status=%d status=%d\n",
           adf_sizeof_char(), adf_alignof_char(), cap, st);
    adf_str_free(s); fmpz_clear(c); fmpz_clear(N); acb_clear(z);
    adf_ucoset_clear(u); adf_char_clear(x); adf_char_clear(y); flint_cleanup(); return st;
}
