#include <adelefeld.h>
#include "test_runner.h"
ADF_TEST(each_public_hilbert_function)
{
    adf_rat_t a;
    adf_lball_t b;
    adf_idele_t i;
    arb_t r;
    adf_place_t p;
    int z=99;
    adf_rat_init(a); adf_lball_init(b); adf_idele_init(i); arb_init(r);
    adf_rat_one(a); arb_one(r); ADF_CHECK(adf_place_prime(&p,2)==ADF_OK);
    ADF_CHECK(adf_lball_set_rat(b,p,a)==ADF_OK);
    ADF_CHECK(adf_lball_hilbert(&z,NULL,b,b)==ADF_OK && z==1);
    ADF_CHECK(adf_real_hilbert(&z,NULL,r,r)==ADF_OK && z==1);
    ADF_CHECK(adf_rat_hilbert_at(&z,NULL,a,a,p)==ADF_OK && z==1);
    ADF_CHECK(adf_idele_hilbert_at(&z,NULL,i,i,p)==ADF_OK && z==1);
    adf_rat_clear(a); adf_lball_clear(b); adf_idele_clear(i); arb_clear(r);
}
