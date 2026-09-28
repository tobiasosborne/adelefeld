/* Probe the proposed decisions M1-D10 and M1-D11 in release and invariant builds.
   Each mode runs in a separate process. The bad values are intentional contract violations. */
#include <adelefeld.h>
#include <string.h>

int
main(int argc, char ** argv)
{
    if (argc != 2)
        return 90;
    if (strcmp(argv[1], "noncanonical") == 0)
    {
        adf_adele_t x;
        adf_rat_t out;
        int status;

        adf_adele_init(x);
        adf_rat_init(out);
        arb_pos_inf(x->inf);
        status = adf_adele_reconstruct(out, x);
        adf_rat_clear(out);
        adf_adele_clear(x);
        return status == ADF_DOMAIN ? 0 : 91;
    }
    if (strcmp(argv[1], "handwrite") == 0)
    {
        adf_modctx_struct * a = NULL, * b = NULL;
        adf_fball_t x;
        fmpz_t A, H, d;
        ulong qa = 6, qb = 5;

        if (adf_modctx_new_blocks(&a, &qa, 1) != ADF_OK
            || adf_modctx_new_blocks(&b, &qb, 1) != ADF_OK)
            return 92;
        adf_fball_init(x);
        fmpz_init_set_si(A, 1);
        fmpz_init_set_si(H, 6);
        fmpz_init_set_si(d, 1);
        if (adf_fball_set_fmpz3(x, A, H, d) != ADF_OK
            || adf_fball_set_local(x, x, a) != ADF_OK)
            return 93;
        x->mctx = b;  /* outside M1-D10: A remains counted */
        adf_modctx_free(a);
        return 94;  /* with the invariant flag, free must abort first */
    }
    return 95;
}
