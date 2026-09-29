#include <stdio.h>
#include <flint/fmpz.h>
#include <flint/fmpz_mat.h>
#include <adelefeld/linsolve.h>

int main(void)
{
    adf_linsol_t sol;
    fmpz_mat_t A, b;
    fmpz_t N;
    int checks = 0;
    adf_linsol_init(sol);
    fmpz_mat_init(A, 2, 1);
    fmpz_mat_init(b, 2, 1);
    fmpz_init_set_ui(N, 4);
    fmpz_set_si(fmpz_mat_entry(A, 0, 0), 2);
    fmpz_set_si(fmpz_mat_entry(A, 1, 0), 3);
    fmpz_set_si(fmpz_mat_entry(b, 0, 0), 2);
    fmpz_set_si(fmpz_mat_entry(b, 1, 0), 3);

    /* A and b are the same fmpz_mat_t. */
    if (adf_linsolve_mod(sol, A, A, N) != 0 || !adf_linsol_verify(sol, A, A, N) ||
        !fmpz_equal_ui(fmpz_mat_entry(sol->x0, 0, 0), 1)) return 1;
    checks++;
    /* Reuse sol, with its N field also serving as the N input. */
    if (adf_linsolve_mod(sol, A, b, sol->N) != 0 || !adf_linsol_verify(sol, A, b, sol->N)) return 2;
    checks++;
    fmpz_zero(fmpz_mat_entry(b, 0, 0));
    if (adf_linsolve_mod(sol, A, b, sol->N) != 5 || !adf_linsol_verify(sol, A, b, sol->N)) return 3;
    checks++;
    /* A DOMAIN result preserves all output fields, even with an aliased N. */
    fmpz_zero(sol->N);
    if (adf_linsolve_mod(sol, A, b, sol->N) != 7 || !fmpz_is_zero(sol->N) || sol->kind != 1) return 4;
    checks++;
    fmpz_set_ui(sol->N, 4);
    fmpz_mat_clear(A);
    fmpz_mat_clear(b);
    fmpz_mat_init(A, 4097, 0);
    fmpz_mat_init(b, 4097, 1);
    if (adf_linsolve_mod(sol, A, b, sol->N) != 10 || !fmpz_equal_ui(sol->N, 4) || sol->kind != 1)
        return 5;
    checks++;
    fmpz_mat_clear(A);
    fmpz_mat_clear(b);
    fmpz_clear(N);
    adf_linsol_clear(sol);
    printf("alias/status checks=%d failures=0\n", checks);
    return 0;
}
