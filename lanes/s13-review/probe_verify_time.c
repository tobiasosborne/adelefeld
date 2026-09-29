#include <adelefeld.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    adf_resid_t x;
    adf_recon_cert_t cert;
    adf_rat_t q;
    fmpz_t A, B;
    if (argc != 2) return 2;
    adf_resid_init(x); adf_recon_cert_init(cert); adf_rat_init(q);
    fmpz_init_set_ui(A, 1); fmpz_init(B);
    if (fmpz_set_str(B, argv[1], 10)) return 2;
    fmpz_set_si(x->c, 0); fmpz_set_si(x->m, 2);
    fmpz_set_si(cert->Rp, 2); fmpz_zero(cert->Tp);
    fmpz_zero(cert->R); fmpz_one(cert->T); cert->kind = 1;
    int accepted = adf_resid_verify_result(x, A, B, 0, ADF_OK, q, cert);
    printf("B=%s accepted=%d\n", argv[1], accepted);
    fmpz_clear(A); fmpz_clear(B); adf_rat_clear(q);
    adf_recon_cert_clear(cert); adf_resid_clear(x);
    return accepted ? 0 : 1;
}
