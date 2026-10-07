/* Evidence adapter, not library code. /usr/include/flint/dirichlet.h:68,110-136,162;
   refs/src/flint-3.0.1/acb_dirichlet.rst:347-380. Run only under timeout.
   Emits lossless ball dumps so Python can compare exact rational rectangles. */
#include <stdio.h>
#include <stddef.h>
#include <flint/dirichlet.h>
#include <flint/acb_dirichlet.h>
#include <flint/ulong_extras.h>

typedef struct { ulong q; ulong n; int parity; acb_t s; } proposed_char;

int main(void)
{
    ulong q, n, a;
    flint_set_num_threads(1);
    printf("FLINT %s %s\n", FLINT_VERSION, flint_version);
    printf("ABI %zu %zu %zu %zu %zu %zu\n", sizeof(proposed_char), _Alignof(proposed_char),
           offsetof(proposed_char,q), offsetof(proposed_char,n), offsetof(proposed_char,parity),
           offsetof(proposed_char,s));
    for (q = 1; q <= 80; q++)
    {
        dirichlet_group_t G;
        if (!dirichlet_group_init(G, q)) return 2;
        for (n = 1; n <= q; n++)
        {
            dirichlet_group_t H;
            dirichlet_char_t x, y;
            acb_t tau;
            char *re, *im;
            ulong C;
            if (n_gcd(n,q) != 1) continue;
            dirichlet_char_init(x,G);
            dirichlet_char_log(x,G,n);
            C = dirichlet_conductor_char(G,x);
            if (!dirichlet_group_init(H,C)) return 3;
            dirichlet_char_init(y,H);
            dirichlet_char_lower(y,H,x,G);
            printf("%lu %lu %lu %lu %d %lu %lu\t", q,n,C,dirichlet_char_exp(H,y),
                   dirichlet_parity_char(H,y),dirichlet_order_char(H,y),H->expo);
            for (a = 0; a < C; a++)
            {
                ulong k = dirichlet_chi(H,y,a);
                if (a) putchar(',');
                if (k == DIRICHLET_CHI_NULL) putchar('-'); else printf("%lu",k);
            }
            acb_init(tau);
            acb_dirichlet_gauss_sum(tau,H,y,256);
            re = arb_dump_str(acb_realref(tau));
            im = arb_dump_str(acb_imagref(tau));
            printf("\t%s\t%s\n",re,im);
            flint_free(re); flint_free(im); acb_clear(tau);
            dirichlet_char_clear(y); dirichlet_group_clear(H);
            dirichlet_char_clear(x);
        }
        dirichlet_group_clear(G);
    }
    flint_cleanup();
    return 0;
}
