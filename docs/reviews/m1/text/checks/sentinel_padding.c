/* Run the exact reviewed helper on one rejected input, without the rest of the suite. */
#define main reviewed_suite_main
#include "../../../../../tests/test_text_adele.c"
#undef main

int main(void)
{
    adf_adele_t x;
    adele_init(x);
    int st = parse_adele(x, "@", 1, 64, NULL, ADF_PARSE, "one rejected byte");
    printf("parse_status=%d\n", st);
    adele_clear(x);
    flint_cleanup();
    return 0;
}
