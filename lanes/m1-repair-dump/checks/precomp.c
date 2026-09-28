/* lanes/m1-repair-dump/checks/precomp.c: where the time of adf_modctx_alloc goes for a context of
   the first k primes: fmpz_multi_mod_precompute and fmpz_multi_CRT_precompute of src/modctx.c,
   timed apart. Usage: precomp <k> */
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/fmpz_vec.h>
#include <flint/ulong_extras.h>

static double
now(void)
{
    return (double) clock() / CLOCKS_PER_SEC;
}

int
main(int argc, char ** argv)
{
    slong i, k = atol(argv[1]);
    ulong p = 1;
    fmpz * mods;
    fmpz_multi_mod_t mod_P;
    fmpz_multi_CRT_t crt_P;
    double t0, t1, t2, t3;

    mods = _fmpz_vec_init(k);
    for (i = 0; i < k; i++)
    {
        p = n_nextprime(p, 1);
        fmpz_set_ui(&mods[i], p);
    }
    t0 = now();
    fmpz_multi_mod_init(mod_P);
    fmpz_multi_mod_precompute(mod_P, mods, k);
    t1 = now();
    fmpz_multi_CRT_init(crt_P);
    fmpz_multi_CRT_precompute(crt_P, mods, k);
    t2 = now();
    {
        fmpz * outs = _fmpz_vec_init(k);
        fmpz_t a;
        fmpz_init(a);
        t3 = now();
        fmpz_multi_mod_precomp(outs, mod_P, a, 0);
        printf("one_mod_precomp_seconds=%.6f\n", now() - t3);
        fmpz_clear(a);
        _fmpz_vec_clear(outs, k);
    }
    printf("k=%ld mod_precompute=%.6f crt_precompute=%.6f one_reduction=%.6f\n", (long) k, t1 - t0, t2 - t1,
           now() - t2);
    fmpz_multi_mod_clear(mod_P);
    fmpz_multi_CRT_clear(crt_P);
    _fmpz_vec_clear(mods, k);
    flint_cleanup_master();
    return 0;
}
