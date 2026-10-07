#include <adelefeld.h>
#include <stdio.h>
#include <string.h>
int main(void)
{
    const char *s = "union((0.5 +/- 1e100000000000 ; 0)) + Q";
    adf_qclass_t x; adf_text_limits_t lim; int st;
    adf_qclass_init(x); adf_text_limits_default(&lim); lim.max_exp10 = WORD_MAX;
    st = adf_qclass_set_str(x, s, strlen(s), 128, &lim);
    printf("huge decimal exponent: %s\n", adf_status_str(st));
    adf_qclass_clear(x); flint_cleanup(); return st == ADF_LIMIT ? 0 : 1;
}
