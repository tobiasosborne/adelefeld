/* f-repair3: print the residues pinned in tests/test_lfunc_trig.c (pinned_residues_odd_and_even_top_degree),
   computed by the library before the paired loop, with p, x, N, the count C and the top degree L. */
#include <stdio.h>
#include <adelefeld.h>
typedef int (*fn_t)(adf_lball_t, const adf_lball_t, slong);
int main(void)
{
    fn_t fs[] = {adf_lball_sin, adf_lball_cos, adf_lball_sinh, adf_lball_cosh};
    const char *names[] = {"sin", "cos", "sinh", "cosh"};
    struct { ulong p; slong num, den; int pw; slong N; } cs[] = {
        {3, 3, 2, 1, 20}, {3, 9, 2, 1, 21}, {5, 35, 3, 1, 12}, {2, 12, 5, 1, 30},
        {UWORD(18446744073709551557), 3, 1, 1, 4}};
    for (int i = 0; i < 5; i++)
    for (int f = 0; f < 4; f++)
    {
        adf_lball_t x, y; adf_rat_t r; adf_place_t pl; fmpz_t P, res, a, b;
        ulong p = cs[i].p; slong C, L, K = cs[i].N;
        adf_lball_init(x); adf_lball_init(y); adf_rat_init(r);
        fmpz_init(P); fmpz_init(res); fmpz_init(a); fmpz_init(b);
        fmpq_set_si(r->q, cs[i].num, cs[i].den);
        if (p > 13) { fmpz_set_ui(a, p); fmpq_mul_fmpz(r->q, r->q, a); }
        adf_place_prime(&pl, p); adf_lball_set_rat(x, pl, r);
        int st = fs[f](y, x, K);
        fmpz_set_ui(a, p - 1); fmpz_mul_si(a, a, K); fmpz_sub_ui(a, a, 1);
        fmpz_set_ui(b, p - 1); fmpz_mul_si(b, b, x->v); fmpz_sub_ui(b, b, 1);
        fmpz_cdiv_q(a, a, b); C = fmpz_cmp_si(a, 1) < 0 ? 1 : fmpz_get_si(a);
        L = C - 1;
        if ((L % 2 == 1) != (f % 2 == 0)) L--;
        fmpz_set_ui(P, p); fmpz_pow_ui(P, P, (ulong) y->v); fmpz_mul(res, P, fmpq_numref(y->u));
        printf("%s p=%lu x=%s w=%ld N=%ld C=%ld L=%ld status=%d yN=%ld exact=%d residue=", names[f], p,
               fmpq_get_str(NULL, 10, r->q), x->v, K, C, L, st, y->N, y->exact);
        fmpz_print(res); printf("\n");
        adf_lball_clear(x); adf_lball_clear(y); adf_rat_clear(r);
        fmpz_clear(P); fmpz_clear(res); fmpz_clear(a); fmpz_clear(b);
    }
    return 0;
}
