#include <adelefeld.h>
#include "test_runner.h"
ADF_TEST(each_public_residue_function)
{
    fmpz_t a,b;
    adf_fball_t f;
    adf_ucoset_t u;
    adf_place_t p;
    int z=99;
    fmpz_init(a); fmpz_init(b); fmpz_one(a); fmpz_set_ui(b,3);
    adf_fball_init(f); adf_fball_set_si(f,1); adf_ucoset_init(u);
    ADF_CHECK(adf_place_prime(&p,3)==ADF_OK);
    ADF_CHECK(adf_fmpz_legendre(&z,NULL,a,p)==ADF_OK && z==1);
    ADF_CHECK(adf_fmpz_jacobi(&z,NULL,a,b)==ADF_OK && z==1);
    ADF_CHECK(adf_fmpz_kronecker(&z,NULL,a,b)==ADF_OK && z==1);
    ADF_CHECK(adf_fball_legendre(&z,NULL,f,p)==ADF_OK && z==1);
    ADF_CHECK(adf_fball_jacobi(&z,NULL,f,b)==ADF_OK && z==1);
    ADF_CHECK(adf_fball_kronecker(&z,NULL,f,b)==ADF_OK && z==1);
    ADF_CHECK(adf_ucoset_legendre(&z,NULL,u,p)==ADF_OK && z==1);
    ADF_CHECK(adf_ucoset_jacobi(&z,NULL,u,b)==ADF_OK && z==1);
    ADF_CHECK(adf_ucoset_kronecker(&z,NULL,u,b)==ADF_OK && z==1);
    adf_fball_clear(f); adf_ucoset_clear(u); fmpz_clear(a); fmpz_clear(b);
}
