#include <adelefeld.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    adf_idele_t x, y;
    adf_idele_init(x); adf_idele_init(y);
    const char *text = "(1 ; 1 * [5 mod 6])";
    int parsed = adf_idele_set_str(x, text, strlen(text), 64, NULL);
    if (parsed != ADF_OK) { printf("parse=%d\n", parsed); return 2; }
    int st = adf_idele_root(y, NULL, x, 1, 0, 2);
    printf("status=%s input_unit=", adf_status_str(st));
    fmpz_print(x->u.c); printf(" mod "); fmpz_print(x->u.N);
    printf(" output_unit="); fmpz_print(y->u.c); printf(" mod "); fmpz_print(y->u.N);
    printf(" identical=%d canonical=%d\n", adf_idele_identical(x, y), adf_idele_is_canonical(y));
    int identical = adf_idele_identical(x, y);
    adf_idele_clear(x); adf_idele_clear(y); flint_cleanup();
    return st == ADF_OK && identical ? 0 : 1;
}
