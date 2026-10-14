#define adf_tate_vector review_vector
#define adf_tate_integral review_integral
#define adf_tate_continue review_continue
#define adf_tate_completed review_completed
#define adf_tate_cutoffs review_cutoffs
#define adf_tate_trace review_trace
#define adf_tate_taylor_piece review_piece
#include "../../src/tate.c"
#include <stdio.h>
static void prb(const arb_t x){char *p=arb_dump_str(x);puts(p);flint_free(p);}
int main(void){double r,b;unsigned long R;arf_t rr,bb;arb_t J;arf_init(rr);arf_init(bb);arb_init(J);
while(scanf("%lf %lf %lu",&r,&b,&R)==3){arf_set_d(rr,r);arf_set_d(bb,b);tt_jbound(J,rr,bb,R);prb(J);}arf_clear(rr);arf_clear(bb);arb_clear(J);flint_cleanup();return 0;}
