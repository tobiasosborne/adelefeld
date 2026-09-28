/* s5_goto_skips_init.c: an early exit jumps over the init to the common clear; the clear then
   frees an uninitialised arb.  The checker reads the text in order and sees the init first. */
#include <stdio.h>
#include <flint/arb.h>

static int
goto_skips_init(int bad)
{
    arb_t x;
    int r = 0;

    if (bad)
        goto done;
    arb_init(x);
    arb_set_si(x, 2);
    r = arb_is_exact(x);
done:
    arb_clear(x);                  /* bad = 1: x was never initialised */
    return r;
}

int
main(int argc, char ** argv)
{
    (void) argv;
    printf("%d\n", goto_skips_init(argc == 1));      /* no argument: the bad path */
    return 0;
}
