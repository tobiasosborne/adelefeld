/* lanes/q-review2/cycle.c: 1000 init / set / clear cycles on hand-built PIECES values with a
   borrowed local context; the context is freed at the end. Run against the sanitised archive
   with ASAN_OPTIONS=detect_leaks=1, and under valgrind. */
#include <adelefeld.h>
#include <stdio.h>

static unsigned long long st = 987654321;
static unsigned long long rnd(void) { st = st * 6364136223846793005ULL + 1442695040888963407ULL; return st >> 17; }

int main(void)
{
    adf_modctx_struct *ctx = NULL;
    ulong q[2] = {2, 3};
    adf_qclass_struct xs[1], ys[1];
    adf_qclass_struct * x = xs;
    adf_qclass_struct * y = ys;
    slong i, n;
    int bad = 0;

    if (adf_modctx_new_blocks(&ctx, q, 2) != ADF_OK) { printf("ctx\n"); return 2; }
    adf_qclass_init(x);
    adf_qclass_init(y);
    for (i = 0; i < 1000; i++) {
        n = 1 + (slong) (rnd() % 12);
        adf_qclass_clear(x);
        x->form = ADF_QCLASS_PIECES;
        x->len = n;
        x->piece = flint_calloc((size_t) n, sizeof(*x->piece));
        for (slong j = 0; j < n; j++) {
            adf_adele_init(&x->piece[j]);
            arf_set_si(arb_midref(x->piece[j].inf), 2 * j + 1);
            arf_mul_2exp_si(arb_midref(x->piece[j].inf), arb_midref(x->piece[j].inf), -5);
            mag_set_ui_2exp_si(arb_radref(x->piece[j].inf), 1, -(j + 2));
            if (j % 3 == 0) {
                x->piece[j].fin.backend = ADF_LOCAL;
                x->piece[j].fin.mctx = ctx;
                x->piece[j].fin.res = flint_calloc(2, sizeof(ulong));
                x->piece[j].fin.res[0] = rnd() % 2;
                x->piece[j].fin.res[1] = rnd() % 3;
                fmpz_set_ui(x->piece[j].fin.H, 6);
                fmpz_one(x->piece[j].fin.d);
            }
            else {
                {
                    slong H = (slong) (rnd() % 5);
                    fmpz_set_si(x->piece[j].fin.H, H);
                    fmpz_set_si(x->piece[j].fin.A, H ? (slong) (rnd() % (ulong) H)
                                                      : (slong) (rnd() % 5));
                    fmpz_one(x->piece[j].fin.d);
                }
            }
        }
        if (!adf_qclass_is_canonical(x)) bad++;
        adf_qclass_set(y, x);
        if (!adf_qclass_identical(y, x)) bad++;
        adf_qclass_swap(y, x);
        if (!adf_qclass_identical(y, x)) bad++;
    }
    adf_qclass_clear(x);
    adf_qclass_clear(y);
    adf_modctx_free(ctx);
    printf("cycles 1000, problems %d\n", bad);
    return bad ? 1 : 0;
}
