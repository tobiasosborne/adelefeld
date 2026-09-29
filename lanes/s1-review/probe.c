#include <stdio.h>
#include <stdlib.h>
#include <flint/fmpz.h>
#include <flint/fmpz_mat.h>
#include <adelefeld/linsolve.h>

static void print_mat(const fmpz_mat_t m)
{
    slong i, j;
    printf(" %ld %ld", m->r, m->c);
    for (i = 0; i < m->r; i++)
        for (j = 0; j < m->c; j++) {
            char *s = fmpz_get_str(NULL, 10, fmpz_mat_entry(m, i, j));
            printf(" %s", s);
            flint_free(s);
        }
}

int main(void)
{
    char word[16384];
    slong r, c, i, j;
    while (scanf("%ld %ld %16383s", &r, &c, word) == 3) {
        fmpz_mat_t A, b;
        fmpz_t N;
        adf_linsol_t sol;
        int st;
        fmpz_init(N);
        fmpz_set_str(N, word, 10);
        fmpz_mat_init(A, r, c);
        fmpz_mat_init(b, r, 1);
        for (i = 0; i < r; i++)
            for (j = 0; j < c; j++) {
                if (scanf("%16383s", word) != 1) return 2;
                fmpz_set_str(fmpz_mat_entry(A, i, j), word, 10);
            }
        for (i = 0; i < r; i++) {
            if (scanf("%16383s", word) != 1) return 2;
            fmpz_set_str(fmpz_mat_entry(b, i, 0), word, 10);
        }
        adf_linsol_init(sol);
        st = adf_linsolve_mod(sol, A, b, N);
        printf("%d %d %d", st, adf_linsol_is_canonical(sol),
               adf_linsol_verify(sol, A, b, N));
        print_mat(sol->G);
        print_mat(sol->E);
        print_mat(sol->V);
        print_mat(sol->x0);
        print_mat(sol->y);
        putchar('\n');
        fflush(stdout);
        adf_linsol_clear(sol);
        fmpz_mat_clear(A);
        fmpz_mat_clear(b);
        fmpz_clear(N);
    }
    return 0;
}
