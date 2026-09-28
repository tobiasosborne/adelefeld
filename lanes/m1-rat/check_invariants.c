/* lanes/m1-rat/check_invariants.c: the run with -DADF_CHECK_INVARIANTS (conventions 4.4).

   A non-canonical adf_rat is a precondition violation, which is undefined behaviour; with the
   flag the library must notice it and call flint_abort with a message. This program builds one
   non-canonical value (8/2) and hands it to every public function of rat.h that takes an
   adf_rat input, one call per process run, and the shell script around it checks that each run
   dies with the message. The argument names the function to call, so that one binary serves
   every case. */

#include <adelefeld.h>
#include <string.h>

int
main(int argc, char ** argv)
{
    adf_rat_t x, y;
    const char * who;

    if (argc != 2)
    {
        flint_printf("usage: check_invariants <function>\n");
        return 2;
    }
    who = argv[1];

    adf_rat_init(x);
    adf_rat_init(y);
    adf_rat_set_si(y, 3);
    fmpz_set_si(fmpq_numref(x->q), 8);       /* 8/2: not in lowest terms */
    fmpz_set_si(fmpq_denref(x->q), 2);
    if (fmpq_is_canonical(x->q))
    {
        flint_printf("the test value is canonical; the test is wrong\n");
        return 3;
    }

    if (strcmp(who, "is_canonical") == 0)
    {
        /* The predicate is the one function that must accept such a value and say 0. */
        if (adf_rat_is_canonical(x) != 0)
        {
            flint_printf("adf_rat_is_canonical did not report 0 for 8/2\n");
            return 4;
        }
        flint_printf("adf_rat_is_canonical reported 0 for 8/2\n");
    }
    else if (strcmp(who, "set") == 0)
        adf_rat_set(y, x);
    else if (strcmp(who, "swap") == 0)
        adf_rat_swap(x, y);
    else if (strcmp(who, "identical") == 0)
        (void) adf_rat_identical(x, y);
    else if (strcmp(who, "get_fmpq") == 0)
    {
        fmpq_t q;

        fmpq_init(q);
        adf_rat_get_fmpq(q, x);
        fmpq_clear(q);
    }
    else if (strcmp(who, "is_zero") == 0)
        (void) adf_rat_is_zero(x);
    else if (strcmp(who, "equal") == 0)
        (void) adf_rat_equal(x, y);
    else if (strcmp(who, "sgn") == 0)
        (void) adf_rat_sgn(x);
    else if (strcmp(who, "add") == 0)
        adf_rat_add(y, x, y);
    else if (strcmp(who, "sub") == 0)
        adf_rat_sub(y, x, y);
    else if (strcmp(who, "mul") == 0)
        adf_rat_mul(y, x, y);
    else if (strcmp(who, "neg") == 0)
        adf_rat_neg(y, x);
    else if (strcmp(who, "div") == 0)
        (void) adf_rat_div(y, x, y);
    else if (strcmp(who, "inv") == 0)
        (void) adf_rat_inv(y, x);
    else
    {
        flint_printf("unknown function %s\n", who);
        return 2;
    }

    flint_printf("no abort: %s accepted a non-canonical input\n", who);
    adf_rat_clear(x);
    adf_rat_clear(y);
    return 1;
}
