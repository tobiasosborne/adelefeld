#include <stdio.h>
#include <adelefeld.h>

int main(void)
{
    adf_lball_t unit;
    adf_lball_init(unit);
    unit->p = 5; unit->exact = 1; unit->v = 1; unit->N = 0; fmpq_one(unit->u);
    int canonical = adf_lball_is_canonical(unit);
    int assertion = unit->v == 0 || (unit->exact && !fmpq_is_zero(unit->u));
    printf("wrong_unit=5 canonical=%d valuation=%ld assertion_at_test_lball_976=%d\n",
           canonical, unit->v, assertion);
    adf_lball_clear(unit); flint_cleanup();
    return canonical && assertion ? 1 : 0;
}
