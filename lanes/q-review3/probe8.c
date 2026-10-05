#include <adelefeld.h>
#include <stdio.h>
static void show(const char *t, arf_t x){
    fmpq_t q; fmpq_init(q); arf_get_fmpq(q, x);
    printf("%-24s = ", t); fmpq_print(q);
    printf("   rawexp=%ld\n", (long)(*(long*)ARF_EXPREF(x)));
    fmpq_clear(q);
}
static void div_q(arf_t u, long n, long d, slong prec, arf_rnd_t rnd){
    fmpq_t q; arf_t A, B;
    fmpq_init(q); arf_init(A); arf_init(B);
    fmpz_set_si(fmpq_numref(q), n); fmpz_set_si(fmpq_denref(q), d); fmpq_canonicalise(q);
    arf_set_fmpq(A, q, 2000000, ARF_RND_NEAR);
    arf_set_ui(B, 1);
    arf_div(u, A, B, prec, rnd);
    fmpq_clear(q); arf_clear(A); arf_clear(B);
}
int main(void){
    arf_t u; arf_init(u);
    div_q(u,1,4,30,ARF_RND_CEIL); show("ceil(1/4,30)", u);
    div_q(u,1,4,30,ARF_RND_NEAR); show("near(1/4,30)", u);
    div_q(u,1,4,32,ARF_RND_CEIL); show("ceil(1/4,32)", u);
    div_q(u,1,4,29,ARF_RND_CEIL); show("ceil(1/4,29)", u);
    div_q(u,1,4,30,ARF_RND_UP); show("up(1/4,30)", u);
    div_q(u,1,4,30,ARF_RND_FLOOR); show("floor(1/4,30)", u);
    div_q(u,1,8,30,ARF_RND_CEIL); show("ceil(1/8,30)", u);
    div_q(u,3,4,30,ARF_RND_CEIL); show("ceil(3/4,30)", u);
    div_q(u,4,3,30,ARF_RND_CEIL); show("ceil(4/3,30)", u);
    div_q(u,3,1,30,ARF_RND_CEIL); show("ceil(3,30)", u);
    div_q(u,1,1,30,ARF_RND_CEIL); show("ceil(1,30)", u);
    div_q(u,2,1,30,ARF_RND_CEIL); show("ceil(2,30)", u);
    return 0;
}
