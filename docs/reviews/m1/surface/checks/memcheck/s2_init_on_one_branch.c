/* s2_init_on_one_branch.c: the init stands on one branch only; on the other branch the value
   is read uninitialised.  The checker is not path-sensitive: any init anywhere in the scope
   counts. */
#include <stdio.h>
#include <flint/fmpq.h>

static int
init_on_one_branch(int flag)
{
    fmpq_t q;
    int s;

    if (flag)
        fmpq_init(q);
    fmpq_set_si(q, 7, 3);          /* flag = 0: q was never initialised */
    s = fmpq_sgn(q);
    fmpq_clear(q);
    return s;
}

int
main(int argc, char ** argv)
{
    (void) argv;
    printf("%d\n", init_on_one_branch(argc > 1));   /* no argument: the bad branch */
    return 0;
}
