#include <adelefeld.h>
#include <stdio.h>
int main(void)
{
    arb_t a;
    fmpz_t man, ex;
    char buf[4000000];
    arb_init(a);
    fmpz_init(man); fmpz_init(ex);
    fmpz_setbit(man, 999999);
    fmpz_add_ui(man, man, 1);
    fmpz_set_si(ex, -1000000);
    arf_set_fmpz_2exp(arb_midref(a), man, ex);
    printf("finite %d  lagom %d\n", arb_is_finite(a), ARB_IS_LAGOM(a));
    arf_get_fmpz_2exp(man, ex, arb_midref(a));
    fmpz_get_str(buf, 10, man);
    printf("mid man has %d digits, first %s\n", (int) strlen(buf), buf);
    printf("exp %s\n", fmpz_get_str(NULL, 10, ex));

    return 0;
}
