#include <stdio.h>
#include <stdlib.h>
#include <flint/fmpz.h>
#include <flint/fmpz_mat.h>
#include <adelefeld/linsolve.h>

static int read_mat(fmpz_mat_t m)
{
    char word[16384];
    slong r, c, i, j;
    if (scanf("%ld %ld", &r, &c) != 2) return 0;
    fmpz_mat_clear(m);
    fmpz_mat_init(m, r, c);
    for (i = 0; i < r; i++)
        for (j = 0; j < c; j++) {
            if (scanf("%16383s", word) != 1) return 0;
            fmpz_set_str(fmpz_mat_entry(m, i, j), word, 10);
        }
    return 1;
}

int main(void)
{
    slong r, c, i, j;
    int kind;
    char word[16384];
    while (scanf("%ld %ld %16383s %d", &r, &c, word, &kind) == 4) {
        fmpz_mat_t A, b;
        fmpz_t N;
        adf_linsol_t sol;
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
        sol->kind = kind;
        fmpz_set(sol->N, N);
        sol->r = r;
        sol->c = c;
        if (!read_mat(sol->G) || !read_mat(sol->E) || !read_mat(sol->V) ||
            !read_mat(sol->x0) || !read_mat(sol->y)) return 2;
        printf("%d %d\n", adf_linsol_is_canonical(sol), adf_linsol_verify(sol, A, b, N));
        fflush(stdout);
        adf_linsol_clear(sol);
        fmpz_mat_clear(A);
        fmpz_mat_clear(b);
        fmpz_clear(N);
    }
    return 0;
}
