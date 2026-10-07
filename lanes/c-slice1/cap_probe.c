#include <adelefeld.h>
#include <stdio.h>
#include <time.h>
int main(void)
{
    adf_char_t x; acb_t z; clock_t t = clock(); int st;
    flint_set_num_threads(1); adf_char_init(x); acb_init(z);
    st = adf_char_set_conrey(x, 65536, 5);
    printf("constructor %d conductor %lu\n", st, x->q); fflush(stdout);
    st = adf_char_gauss_sum(z, x, 2);
    printf("gauss %d seconds %.6f\n", st, (double)(clock()-t)/CLOCKS_PER_SEC); fflush(stdout);
    acb_clear(z); adf_char_clear(x); flint_cleanup(); return st;
}
