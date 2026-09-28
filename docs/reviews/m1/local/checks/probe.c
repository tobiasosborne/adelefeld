/* Review driver. All normal inputs are constructed through the public interface.
   The oracle lives in oracle.py and uses only Python integer/Fraction arithmetic. */
#include <adelefeld.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static adf_modctx_struct *ctx[10];
static void contexts(void)
{
    ulong a[] = {4}, b[] = {4, 9, 25}, c[] = {25, 9, 4};
    ulong d[] = {UWORD(18446744073709551615)};
    ulong e[] = {UWORD(18446744073709551614), UWORD(18446744073709551615)};
    ulong p[128], n = 2, two = 2;
    int k = 0;
    while (k < 128) {
        int prime = 1;
        for (ulong j = 2; j*j <= n; j++) if (n % j == 0) prime = 0;
        if (prime) p[k++] = n;
        n++;
    }
    assert(adf_modctx_new_blocks(&ctx[1], a, 1) == 0);
    assert(adf_modctx_new_blocks(&ctx[2], b, 3) == 0);
    assert(adf_modctx_new_blocks(&ctx[3], c, 3) == 0);
    assert(adf_modctx_new_blocks(&ctx[4], b, 3) == 0);
    assert(adf_modctx_new_blocks(&ctx[5], d, 1) == 0);
    assert(adf_modctx_new_blocks(&ctx[6], e, 2) == 0);
    assert(adf_modctx_new_blocks(&ctx[7], p, 128) == 0);
    fmpz_t one;
    fmpz_init_set_ui(one, 1);
    assert(adf_modctx_new_fmpz(&ctx[8], one) == 0);
    fmpz_clear(one);
    assert(adf_modctx_new_blocks(&ctx[9], &two, 1) == 0);
}
static void readz(fmpz_t x)
{
    char s[8192];
    assert(scanf("%8191s", s) == 1);
    assert(fmpz_set_str(x, s, 10) == 0);
}
static void readball(adf_fball_t x, int c)
{
    fmpz_t A, H, d;
    fmpz_init(A); fmpz_init(H); fmpz_init(d);
    readz(A); readz(H); readz(d);
    assert(adf_fball_set_fmpz3(x, A, H, d) == 0);
    if (c) assert(adf_fball_set_local(x, x, ctx[c]) == 0);
    fmpz_clear(A); fmpz_clear(H); fmpz_clear(d);
}
static void emit(const adf_fball_t z, int st, int lost)
{
    fmpz_t A, H, d;
    fmpz_init(A); fmpz_init(H); fmpz_init(d);
    adf_fball_get_fmpz3(A, H, d, z);
    int ci = 0;
    for (int i = 1; i < 10; i++) if (adf_fball_context(z) == ctx[i]) ci = i;
    printf("%d %d %d %d ", st, lost, adf_fball_is_canonical(z), ci);
    fmpz_print(A); putchar(' '); fmpz_print(H); putchar(' '); fmpz_print(d);
    putchar(' '); fmpz_print(z->d);
    if (ci) for (slong j = 0; j < adf_modctx_nblocks(ctx[ci]); j++) printf(" %lu", z->res[j]);
    putchar('\n');
    fmpz_clear(A); fmpz_clear(H); fmpz_clear(d);
}
int main(int argc, char **argv)
{
    contexts();
    if (argc == 2 && strcmp(argv[1], "bad-context") == 0) {
        /* is_canonical alone promises to tolerate arbitrary fields (fball.h:92-94).
           Other functions are never called on this object. */
        adf_fball_t x;
        adf_fball_init(x);
        x->backend = ADF_LOCAL;
        fmpz_set_ui(x->H, 4);
        x->mctx = (adf_modctx_struct *) (void *) flint_malloc(1);
        x->res = flint_malloc(sizeof(ulong));
        x->res[0] = 0;
        printf("predicate=%d\n", adf_fball_is_canonical(x));
        flint_free((void *) x->mctx);
        adf_fball_clear(x);
    } else {
        char op;
        int cx, cy, alias, target;
        adf_fball_t x, y, z;
        adf_rat_t q, C;
        adf_fball_init(x); adf_fball_init(y); adf_fball_init(z);
        adf_rat_init(q); adf_rat_init(C);
        while (scanf(" %c %d %d %d %d", &op, &cx, &cy, &alias, &target) == 5) {
            readball(x, cx); readball(y, cy);
            readz(fmpq_numref(q->q)); readz(fmpq_denref(q->q)); fmpq_canonicalise(q->q);
            readz(fmpq_numref(C->q)); readz(fmpq_denref(C->q)); fmpq_canonicalise(C->q);
            /* Reuse an output with a different allocation size/context on every request. */
            adf_fball_set_si(z, 0);
            fmpz_t A, H, d;
            fmpz_init_set_ui(A, 1); fmpz_init(H); fmpz_init_set_ui(d, 1);
            adf_modctx_get_modulus(H, ctx[7]);
            assert(adf_fball_set_fmpz3(z, A, H, d) == 0);
            assert(adf_fball_set_local(z, z, ctx[7]) == 0);
            fmpz_clear(A); fmpz_clear(H); fmpz_clear(d);
            adf_fball_ptr out = alias == 0 ? z : alias == 2 ? y : x;
            adf_fball_srcptr rhs = alias == 3 ? x : y;
            int st = 0, lost = -1;
            if (op == 'p') {
                printf("%d %d %d %d %d %d\n", adf_fball_equal_set(x, rhs),
                       adf_fball_overlaps(x, rhs), adf_fball_contains(x, rhs),
                       adf_fball_compare(x, rhs), adf_fball_contains_rat(x, q),
                       adf_fball_identical(x, rhs));
                continue;
            }
            switch (op) {
                case '+': adf_fball_add(out, x, rhs); break;
                case '-': adf_fball_sub(out, x, rhs); break;
                case '*': adf_fball_mul(out, x, rhs); break;
                case 'n': adf_fball_neg(out, x); break;
                case 'm': adf_fball_mul_rat(out, x, q); break;
                case 'd': st = adf_fball_div_rat(out, x, q); break;
                case 'l': st = adf_fball_set_local(out, x, ctx[target]); break;
                case 'e': st = adf_fball_set_local_enclose(out, &lost, x, ctx[target]); break;
                case 'g': adf_fball_set_global(out, x); break;
                case 's': adf_fball_set(out, x); break;
                case 'w': adf_fball_swap(out, x); break;
                case 'c': st = adf_fball_cap(out, x, C); break;
                case 'a': st = adf_fball_add_cap(out, x, rhs, C); break;
                case 'b': st = adf_fball_sub_cap(out, x, rhs, C); break;
                case 't': st = adf_fball_mul_cap(out, x, rhs, C); break;
                case 'r': st = adf_fball_mul_rat_cap(out, x, q, C); break;
                default: abort();
            }
            emit(out, st, lost);
        }
        adf_fball_clear(x); adf_fball_clear(y); adf_fball_clear(z);
        adf_rat_clear(q); adf_rat_clear(C);
    }
    for (int i = 1; i < 10; i++) adf_modctx_free(ctx[i]);
    flint_cleanup();
    return 0;
}
