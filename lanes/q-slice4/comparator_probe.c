/* Component check: comparing an exact key with itself returns zero. No public symbol is added. */
#ifndef QCLASS_SOURCE
#define QCLASS_SOURCE "../../src/qclass.c"
#endif
#include QCLASS_SOURCE
#include <stdio.h>
int main(void)
{
    adf_adele_t a; qp_key key; int c;
    adf_adele_init(a); key.piece = a; key.ordinal = 0; fmpz_init(key.A); fmpz_init(key.H);
    c = qp_sort_cmp(&key, &key);
    fmpz_clear(key.A); fmpz_clear(key.H); adf_adele_clear(a); flint_cleanup();
    if (c != 0) { fprintf(stderr, "self comparison: %d, expected 0\n", c); return 1; }
    puts("comparator self comparison: 0"); return 0;
}
