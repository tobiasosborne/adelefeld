/* reduce_run.c: the hidden kernels adf_modctx_reduce and adf_modctx_recombine (modctx_internal.h:
   "res[i] = a mod q_i, 0 <= i < k, for any integer a (the residue is in [0, q_i))"; recombine:
   "the unique integer in [0, K) that is res[i] mod q_i"). Prints each context and each call;
   reduce_oracle.py checks with Python integers. Contexts: one block at the word edges, two
   blocks, 64 and 200 blocks, mixed sizes. Inputs a: 0, +-1, +-(K-1), +-K, +-(K+1), K^3 + 5,
   -(K^3) - 7, and random of up to 3 times the bits of K, both signs.
   With argument "threads": four threads reduce and recombine on one shared context and compare
   against the single-thread answer (for -fsanitize=thread).
   Build: cc -std=c11 -O1 -g -Iinclude reduce_run.c build/libadelefeld.a -lflint -lgmp -lm -lpthread */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <adelefeld.h>
#include <flint/ulong_extras.h>

void adf_modctx_reduce(const adf_modctx_struct * ctx, const fmpz_t a, ulong * res);
void adf_modctx_recombine(fmpz_t out, const adf_modctx_struct * ctx, const ulong * res);

static void pz(const fmpz_t z) { char * s = fmpz_get_str(NULL, 10, z); printf("%s", s); flint_free(s); }

static long iters = 3000;
typedef struct { const adf_modctx_struct * ctx; int id; long bad; } targ;

static void * worker(void * p)
{
    targ * t = p;
    flint_rand_t R;
    slong k = adf_modctx_nblocks(t->ctx), i;
    ulong * r1 = flint_malloc(k * sizeof(ulong)), * r2 = flint_malloc(k * sizeof(ulong));
    fmpz_t a, K, b;
    long it;
    flint_randinit(R);
    for (i = 0; i < t->id * 7; i++) n_randint(R, 10);
    fmpz_init(a); fmpz_init(K); fmpz_init(b);
    adf_modctx_get_modulus(K, t->ctx);
    for (it = 0; it < iters; it++)
    {
        fmpz_randm(a, R, K);
        adf_modctx_reduce(t->ctx, a, r1);
        for (i = 0; i < k; i++)
            if (r1[i] != fmpz_fdiv_ui(a, adf_modctx_block(t->ctx, i))) t->bad++;
        adf_modctx_recombine(b, t->ctx, r1);
        if (!fmpz_equal(a, b)) t->bad++;
        (void) r2;
    }
    fmpz_clear(a); fmpz_clear(K); fmpz_clear(b);
    flint_free(r1); flint_free(r2);
    flint_randclear(R);
    flint_cleanup();
    return NULL;
}

int main(int argc, char ** argv)
{
    flint_rand_t R;
    ulong one[][1] = {{2}, {3}, {UWORD_MAX}, {UWORD_MAX - 58}, {UWORD(1) << 63}};
    ulong two[2] = {UWORD_MAX, UWORD_MAX - 1};
    ulong mixed[5] = {UWORD(1) << 63, 3, UWORD_MAX - 58, 25, 7};
    ulong many[200];
    adf_modctx_struct * ctxs[12];
    int nc = 0, c;
    slong i;

    {
        ulong p = 2;
        for (i = 0; i < 200; i++) { many[i] = p; p = n_nextprime(p, 1); }
    }
    for (i = 0; i < 5; i++) adf_modctx_new_blocks(&ctxs[nc++], one[i], 1);
    adf_modctx_new_blocks(&ctxs[nc++], two, 2);
    adf_modctx_new_blocks(&ctxs[nc++], mixed, 5);
    adf_modctx_new_blocks(&ctxs[nc++], many, 64);
    adf_modctx_new_blocks(&ctxs[nc++], many, 200);
    adf_modctx_new_factorial(&ctxs[nc++], 65);
    adf_modctx_new_primorial_pow(&ctxs[nc++], 1000, 5);

    if (argc > 2) iters = atol(argv[2]);
    if (argc > 1 && strcmp(argv[1], "threads") == 0)
    {
        pthread_t th[4];
        targ ta[4];
        long bad = 0;
        for (c = 0; c < nc; c++)
        {
            for (i = 0; i < 4; i++) { ta[i].ctx = ctxs[c]; ta[i].id = (int) i; ta[i].bad = 0; }
            for (i = 0; i < 4; i++) pthread_create(&th[i], NULL, worker, &ta[i]);
            for (i = 0; i < 4; i++) { pthread_join(th[i], NULL); bad += ta[i].bad; }
        }
        printf("threads: contexts %d, 4 threads x %ld round trips each, wrong %ld\n", nc, iters, bad);
        for (c = 0; c < nc; c++) adf_modctx_free(ctxs[c]);
        flint_cleanup();
        return bad != 0;
    }

    flint_randinit(R);
    for (c = 0; c < nc; c++)
    {
        slong k = adf_modctx_nblocks(ctxs[c]);
        ulong * res = flint_malloc(k * sizeof(ulong));
        fmpz_t K, a, out;
        int t;
        fmpz_init(K); fmpz_init(a); fmpz_init(out);
        adf_modctx_get_modulus(K, ctxs[c]);
        printf("CTX ");
        for (i = 0; i < k; i++) printf("%s%lu", i ? "," : "", adf_modctx_block(ctxs[c], i));
        printf("\n");
        for (t = 0; t < 400; t++)
        {
            switch (t)
            {
            case 0: fmpz_zero(a); break;
            case 1: fmpz_one(a); break;
            case 2: fmpz_set_si(a, -1); break;
            case 3: fmpz_sub_ui(a, K, 1); break;
            case 4: fmpz_sub_ui(a, K, 1); fmpz_neg(a, a); break;
            case 5: fmpz_set(a, K); break;
            case 6: fmpz_neg(a, K); break;
            case 7: fmpz_add_ui(a, K, 1); break;
            case 8: fmpz_add_ui(a, K, 1); fmpz_neg(a, a); break;
            case 9: fmpz_pow_ui(a, K, 3); fmpz_add_ui(a, a, 5); break;
            case 10: fmpz_pow_ui(a, K, 3); fmpz_add_ui(a, a, 7); fmpz_neg(a, a); break;
            default:
                fmpz_randbits(a, R, 1 + n_randint(R, 3 * fmpz_bits(K) + 2));
                break;
            }
            adf_modctx_reduce(ctxs[c], a, res);
            printf("RED ");
            pz(a);
            printf(" ");
            for (i = 0; i < k; i++) printf("%s%lu", i ? "," : "", res[i]);
            printf("\n");
            /* recombine random residues */
            for (i = 0; i < k; i++)
            {
                ulong q = adf_modctx_block(ctxs[c], i);
                ulong m = n_randint(R, 4);
                res[i] = m == 0 ? 0 : m == 1 ? q - 1 : n_randint(R, q);
            }
            adf_modctx_recombine(out, ctxs[c], res);
            printf("REC ");
            for (i = 0; i < k; i++) printf("%s%lu", i ? "," : "", res[i]);
            printf(" ");
            pz(out);
            printf("\n");
        }
        flint_free(res);
        fmpz_clear(K); fmpz_clear(a); fmpz_clear(out);
    }
    for (c = 0; c < nc; c++) adf_modctx_free(ctxs[c]);
    flint_randclear(R);
    flint_cleanup();
    return 0;
}
