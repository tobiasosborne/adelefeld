#include <stdio.h>
#include <string.h>
#include <adelefeld.h>

int main(int argc, char **argv)
{
    adf_lball_t x, y;
    adf_rat_t q;
    adf_place_t p;
    slong N = 91;
    int result = 0;
    if (argc != 2) return 2;
    adf_lball_init(x); adf_lball_init(y); adf_rat_init(q);
    x->p = 4; /* A composite prime: initialised but non-canonical. */
    if (!strcmp(argv[1], "place")) result = adf_place_is_archimedean(adf_lball_place(x));
    else if (!strcmp(argv[1], "exact")) result = adf_lball_is_exact(x);
    else if (!strcmp(argv[1], "zero")) result = adf_lball_contains_zero(x);
    else if (!strcmp(argv[1], "prec")) { x->exact = 0; result = adf_lball_get_prec(&N, x); }
    else if (!strcmp(argv[1], "set")) { adf_lball_set(y, x); result = adf_lball_is_canonical(y); }
    else if (!strcmp(argv[1], "swap")) { adf_lball_swap(x, y); result = adf_lball_is_canonical(y); }
    else if (!strcmp(argv[1], "identical")) result = adf_lball_identical(x, y);
    else if (!strcmp(argv[1], "rat"))
    {
        if (adf_place_prime(&p, 5) != ADF_OK) return 3;
        fmpz_set_si(fmpq_numref(q->q), 2); fmpz_set_si(fmpq_denref(q->q), 4);
        result = adf_lball_set_rat(y, p, q);
    }
    else if (!strcmp(argv[1], "ratball"))
    {
        if (adf_place_prime(&p, 5) != ADF_OK) return 3;
        fmpz_set_si(fmpq_numref(q->q), 2); fmpz_set_si(fmpq_denref(q->q), 4);
        result = adf_lball_set_rat_ball(y, p, q, 3);
    }
    else if (!strcmp(argv[1], "add_control")) result = adf_lball_add(y, x, x);
    else return 4;
    printf("%s: returned=%d N=%ld output_canonical=%d\n", argv[1], result, N,
           adf_lball_is_canonical(y));
    adf_lball_clear(x); adf_lball_clear(y); adf_rat_clear(q); flint_cleanup();
    return 0;
}
