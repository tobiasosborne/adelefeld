/* s0_control.c: the shape the checker does catch (no init at all), to show that the checker
   runs on these files: expected one use-before-init. */
#include <stdio.h>
#include <flint/fmpz.h>

static long
no_init_at_all(void)
{
    fmpz_t a;
    long r;

    fmpz_set_ui(a, 5);
    r = (long) fmpz_get_si(a);
    fmpz_clear(a);
    return r;
}

int
main(void)
{
    printf("%ld\n", no_init_at_all());
    return 0;
}
