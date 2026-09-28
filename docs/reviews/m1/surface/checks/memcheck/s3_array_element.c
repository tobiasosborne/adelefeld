/* s3_array_element.c: an array of fmpz_t; one element is initialised, another is read.  The
   checker keys everything on the base identifier `v` (argument_base, check_uninit.py:387-397),
   so the init of v[0] counts for v[1]. */
#include <stdio.h>
#include <flint/fmpz.h>

static long
array_element(void)
{
    fmpz_t v[2];
    long r;

    fmpz_init(v[0]);
    fmpz_set_ui(v[0], 5);
    fmpz_add(v[0], v[0], v[1]);    /* v[1] was never initialised */
    r = (long) fmpz_get_si(v[0]);
    fmpz_clear(v[0]);
    return r;
}

int
main(void)
{
    printf("%ld\n", array_element());
    return 0;
}
