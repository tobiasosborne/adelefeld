/* gcc -O2 flint_probe.c -o flint_probe -lflint -lgmp; ./flint_probe; rm flint_probe */
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>
#include <gmp.h>
#include <flint/flint.h>
#include <flint/padic.h>
#include <flint/fmpq.h>
#include <flint/arb.h>
#include <flint/acb.h>
#include <flint/nmod.h>

static void show(const char *name, int ok, const padic_t y, const padic_ctx_t ctx)
{
    printf(" %s=%d", name, ok);
    if (ok) { printf(":"); padic_print(y, ctx); }
}

int main(void)
{
    printf("FLINT %s; sizes: mpz struct=%zu, mpz_t=%zu, fmpz=%zu, fmpq=%zu, arb=%zu, acb=%zu, nmod=%zu\n",
           FLINT_VERSION, sizeof(__mpz_struct), sizeof(mpz_t), sizeof(fmpz),
           sizeof(fmpq), sizeof(arb_struct), sizeof(acb_struct), sizeof(nmod_t));
    struct perf_event_attr attr = {0};
    attr.type = PERF_TYPE_HARDWARE; attr.size = sizeof(attr);
    attr.config = PERF_COUNT_HW_CPU_CYCLES; attr.disabled = 1;
    attr.exclude_kernel = 1; attr.exclude_hv = 1;
    int fd = (int) syscall(__NR_perf_event_open, &attr, 0, -1, -1, 0);
    if (fd < 0) printf("perf_event_open cycles: errno=%d %s (this invocation only)\n",errno,strerror(errno));
    else { puts("perf_event_open cycles: opened successfully (not a calibration)"); close(fd); }
    size_t sizes[] = {1,24,25,48,512};
    for (size_t i=0;i<5;i++) { void *q=malloc(sizes[i]);
        printf("malloc requested=%zu usable=%zu (not total chunk footprint)\n",sizes[i],malloc_usable_size(q)); free(q); }
    const long inputs[] = {0,1,2,3,4,5,9,-1};
    const unsigned long primes[] = {2,3,5};
    for (size_t j=0;j<3;j++) {
        fmpz_t p; fmpz_init_set_ui(p,primes[j]);
        padic_ctx_t ctx; padic_ctx_init(ctx,p,0,32,PADIC_TERSE);
        padic_t x,y; padic_init2(x,12); padic_init2(y,12);
        for(size_t i=0;i<8;i++) {
            padic_set_si(x,inputs[i],ctx);
            printf("p=%lu x=%ld",primes[j],inputs[i]);
            int ok=padic_exp(y,x,ctx); show("exp",ok,y,ctx);
            ok=padic_log(y,x,ctx); show("log",ok,y,ctx);
            ok=padic_sqrt(y,x,ctx); show("sqrt",ok,y,ctx);
            if (inputs[i] != 0 && inputs[i] % (long)primes[j] != 0) {
                padic_teichmuller(y,x,ctx); show("teich",1,y,ctx);
            }
            puts("");
        }
        padic_clear(x);padic_clear(y);padic_ctx_clear(ctx);fmpz_clear(p);
    }
    fmpz_t p;fmpz_init_set_ui(p,3);
    padic_ctx_t ctx;padic_ctx_init(ctx,p,0,32,PADIC_TERSE);
    padic_t x,y,z,w,d;padic_init2(x,2);padic_init2(y,8);padic_init2(z,12);padic_init2(w,8);padic_init2(d,8);
    padic_set_ui(x,3,ctx);padic_exp(y,x,ctx);
    padic_set_ui(z,12,ctx);padic_exp(w,z,ctx);padic_sub(d,y,w,ctx);
    printf("exp low input precision: Nin=2 Nout=%ld; v3(exp(3)-exp(12))=%ld\n",padic_prec(y),padic_val(d));
    padic_set_ui(x,1,ctx);padic_log(y,x,ctx);
    padic_set_ui(z,10,ctx);padic_log(w,z,ctx);padic_sub(d,y,w,ctx);
    printf("log low input precision: Nin=2 Nout=%ld; v3(log(1)-log(10))=%ld\n",padic_prec(y),padic_val(d));
    padic_clear(x);padic_clear(y);padic_clear(z);padic_clear(w);padic_clear(d);padic_ctx_clear(ctx);fmpz_clear(p);
    flint_cleanup();
    return 0;
}
