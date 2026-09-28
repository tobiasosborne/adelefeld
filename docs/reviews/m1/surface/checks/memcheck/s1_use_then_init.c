/* s1_use_then_init.c: a use before the init, in straight-line code.  The checker records the
   first use and the first init separately and reports use-before-init only when the variable
   has no init at all (tools/memcheck/check_uninit.py:496), so a use that comes first is
   missed. */
#include <stdio.h>
#include <flint/fmpz.h>

static int
first_use_then_init(int k)
{
    fmpz_t a;
    int r;

    fmpz_add_ui(a, a, (ulong) k);  /* read of the uninitialised a */
    fmpz_init(a);
    r = (int) fmpz_get_si(a);
    fmpz_clear(a);
    return r;
}

int
main(void)
{
    printf("%d\n", first_use_then_init(3));
    return 0;
}
