/* lanes/i-slice2/limit_repro.c: the red run of the precision limit (orchestrator's addition to the brief).
   adf_idele_set_rat of 1/3 at prec WORD_MAX; before the limit FLINT tries to allocate 2^63 bits.
   Build: cc -Iinclude lanes/i-slice2/limit_repro.c build/libadelefeld.a -lflint -lgmp -lm -o build/limit_repro */
#include <stdio.h>
#include <adelefeld.h>

int
main(void)
{
    adf_idele_t x;
    adf_rat_t q;
    int st;
    adf_idele_init(x);
    adf_rat_init(q);
    fmpq_set_si(q->q, 1, 3);
    st = adf_idele_set_rat(x, q, WORD_MAX);
    printf("adf_idele_set_rat(1/3, WORD_MAX) = %d (ADF_LIMIT = %d)\n", st, ADF_LIMIT);
    adf_rat_clear(q);
    adf_idele_clear(x);
    return st == ADF_LIMIT ? 0 : 1;
}
