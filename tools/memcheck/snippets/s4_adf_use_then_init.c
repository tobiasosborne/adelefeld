/* s4_adf_use_then_init.c: the shape of S1 with an adelefeld type and the public interface: an
   adf_fball_t is added to before its init.  Missed for the same reason as S1. */
#include <stdio.h>
#include <adelefeld.h>

static int
adf_use_then_init(void)
{
    adf_fball_t x, y;
    int r;

    adf_fball_init(y);
    adf_fball_set_si(y, 3);
    adf_fball_add(x, x, y);        /* x read before its init */
    adf_fball_init(x);
    r = adf_fball_is_exact(x);
    adf_fball_clear(x);
    adf_fball_clear(y);
    return r;
}

int
main(void)
{
    printf("%d\n", adf_use_then_init());
    return 0;
}
