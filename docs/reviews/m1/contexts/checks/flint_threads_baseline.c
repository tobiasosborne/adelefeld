/* flint_threads_baseline.c: two threads that only create and clear big fmpz values, no
   adelefeld code. If helgrind reports the same "BSS segment of libflint" races here, the reports
   of reduce_run under helgrind belong to FLINT's fmpz memory manager, not to the context.
   Build: cc -std=c11 -O1 -g flint_threads_baseline.c -lflint -lgmp -lpthread */
#include <pthread.h>
#include <stdio.h>
#include <flint/fmpz.h>

static void * w(void * p)
{
    fmpz_t a, b;
    int i;
    (void) p;
    fmpz_init(a); fmpz_init(b);
    for (i = 0; i < 200; i++)
    {
        fmpz_set_ui(a, 1);
        fmpz_mul_2exp(a, a, 200 + i);
        fmpz_mul(b, a, a);
        fmpz_zero(a);
        fmpz_zero(b);
    }
    fmpz_clear(a); fmpz_clear(b);
    flint_cleanup();
    return NULL;
}

int main(void)
{
    pthread_t t[2];
    pthread_create(&t[0], NULL, w, NULL);
    pthread_create(&t[1], NULL, w, NULL);
    pthread_join(t[0], NULL);
    pthread_join(t[1], NULL);
    printf("done\n");
    return 0;
}
