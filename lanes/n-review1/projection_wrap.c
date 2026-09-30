/* Valuation API: refs/src/flint-3.0.1/fmpz.rst:1142-1150. */
#include <flint/fmpz.h>
#include <stdio.h>
slong __real_fmpz_remove(fmpz_t, const fmpz_t, const fmpz_t);
slong __wrap_fmpz_remove(fmpz_t out, const fmpz_t a, const fmpz_t p)
{
    ulong bits = fmpz_bits(a);
    slong v = __real_fmpz_remove(out, a, p);
    if (bits > 80000000)
        fprintf(stderr, "full_remove input_bits=%lu p=%lu returned_valuation=%ld\n", bits, fmpz_get_ui(p), v);
    return v;
}
