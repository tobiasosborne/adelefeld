#include <adelefeld/roots.h>
#include <flint/nmod_poly.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

slong adf_roots_modp(ulong *, const nmod_poly_t, int);
int adf_roots_padic_core(adf_rootlist_t, const fmpz_poly_t, adf_place_t, slong, slong, int, slong);

static int same_sentinel(const adf_rootlist_struct *a, const adf_rootlist_struct *b)
{
    return a->place.opaque == b->place.opaque && a->scope == b->scope && a->reduced == b->reduced &&
           a->complete == b->complete && a->g->coeffs == b->g->coeffs && a->g->alloc == b->g->alloc &&
           a->g->length == b->g->length && a->n == b->n && a->a == b->a && a->K == b->K && a->s == b->s &&
           a->nu == b->nu && a->ua == b->ua && a->ue == b->ue && a->ball == b->ball && a->count == b->count &&
           fmpz_equal_ui(a->a, 9) && a->K[0] == 2 && a->s[0] == 0 &&
           fmpz_equal_si(a->g->coeffs, -9) && fmpz_equal_ui(a->g->coeffs + 1, 1);
}

static void print_list(const adf_rootlist_t L, const fmpz_poly_t f, slong depth)
{
    slong i;
    printf(" %d %ld %ld %d %d %d %d", L->complete, (long) L->n, (long) L->nu,
           adf_rootlist_is_canonical(L), adf_rootlist_verify_entries(L, f),
           adf_rootlist_verify_complete(L, f, depth), L->reduced);
    for (i = 0; i < L->n; i++)
    {
        putchar(' ');
        fmpz_print(L->a + i);
        printf(" %ld %ld", (long) L->K[i], (long) L->s[i]);
    }
    for (i = 0; i < L->nu; i++)
    {
        putchar(' ');
        fmpz_print(L->ua + i);
        printf(" %ld", (long) L->ue[i]);
    }
}

int main(void)
{
    char mode, token[20000];
    ulong p;
    slong prec, depth, len, i, nr;
    fmpz_poly_t f;
    fmpz_t z;
    nmod_poly_t h;
    adf_rootlist_t L, M;
    adf_rootlist_struct before, before_partial;
    adf_place_t place;
    ulong *r;
    int st, sp, untouched;
    fmpz_poly_init(f);
    fmpz_init(z);
    adf_rootlist_init(L);
    adf_rootlist_init(M);
    while (scanf(" %c %lu %ld %ld %ld", &mode, &p, &prec, &depth, &len) == 5)
    {
        fmpz_poly_zero(f);
        for (i = 0; i < len; i++)
        {
            if (scanf("%19999s", token) != 1 || fmpz_set_str(z, token, 10) != 0)
                return 2;
            fmpz_poly_set_coeff_fmpz(f, i, z);
        }
        if (adf_place_prime(&place, p) != ADF_OK)
            return 3;
        if (mode == 'M')
        {
            nmod_poly_init(h, p);
            fmpz_poly_get_nmod_poly(h, f);
            r = malloc((len + 1) * sizeof(ulong));
            nr = adf_roots_modp(r, h, (int) prec);
            printf("M %ld", (long) nr);
            for (i = 0; i < nr; i++)
                printf(" %lu", r[i]);
            free(r);
            nmod_poly_clear(h);
        }
        else
        {
            /* Keep a populated sentinel to expose writes on any non-OK status. */
            fmpz_poly_t sentinel;
            fmpz_poly_init(sentinel);
            fmpz_poly_set_coeff_si(sentinel, 0, -9);
            fmpz_poly_set_coeff_si(sentinel, 1, 1);
            if (adf_roots_padic(L, sentinel, place, 2, 0) != ADF_OK)
                return 4;
            if (adf_roots_padic(M, sentinel, place, 2, 0) != ADF_OK)
                return 4;
            fmpz_poly_clear(sentinel);
            before = *L;
            before_partial = *M;
            st = mode == 'C' ? adf_roots_padic_core(L, f, place, prec, depth, 1, 256)
                             : adf_roots_padic(L, f, place, prec, depth);
            untouched = st == ADF_OK || same_sentinel(L, &before);
            sp = mode == 'C' ? adf_roots_padic_core(M, f, place, prec, depth, 0, 256)
                             : adf_roots_padic_partial(M, f, place, prec, depth);
            untouched = untouched && (sp == ADF_OK || same_sentinel(M, &before_partial));
            printf("P %d %d %d", st, sp, untouched);
            if (sp == ADF_OK)
                print_list(M, f, depth);
            if (st == ADF_OK)
                print_list(L, f, depth);
        }
        putchar('\n');
        fflush(stdout);
    }
    adf_rootlist_clear(L);
    adf_rootlist_clear(M);
    fmpz_poly_clear(f);
    fmpz_clear(z);
    flint_cleanup();
    return 0;
}
