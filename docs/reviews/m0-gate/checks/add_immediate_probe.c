/* Own measurement of three instruction chains. No theoretical latency is assumed.
   Compare the precise add $1 used by bench/asm/chain_mul_run.s:2748 with add $0.
   The summary page refs/src/uops-intel/ADD_R64_I8.html:190-201 does not name the immediate. */
#define _GNU_SOURCE
#include <inttypes.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

static volatile uint64_t sink;
static double now(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC_RAW, &t);
    return (double)t.tv_sec + 1e-9 * t.tv_nsec;
}

#define RUN(NAME, INSTRUCTION) \
static __attribute__((noinline)) double NAME(void) \
{ \
    uint64_t x = 1, y = 1; \
    double begin = now(); \
    for (unsigned long i = 0; i < 400000; ++i) \
        __asm__ volatile(".rept 128\n\t" INSTRUCTION "\n\t.endr" : "+a"(x) : "c"(y) : "cc"); \
    double elapsed = now() - begin; \
    sink ^= x; \
    return elapsed * 1e9 / (400000.0 * 128); \
}

RUN(imm0, "add $0, %%rax")
RUN(imm1, "add $1, %%rax")
RUN(reg1, "add %%rcx, %%rax")

int main(void)
{
    cpu_set_t cpus;
    CPU_ZERO(&cpus);
    CPU_SET(2, &cpus);
    int status = sched_setaffinity(0, sizeof(cpus), &cpus);
    printf("affinity_status=%d cpu=%d\n", status, sched_getcpu());
    for (int i = 0; i < 5; ++i) {
        double a = imm0(), b = imm1(), c = reg1();
        printf("trial=%d imm0_ns=%.6f imm1_ns=%.6f reg1_ns=%.6f imm1_over_imm0=%.3f\n",
               i, a, b, c, b / a);
    }
    printf("checksum=%" PRIu64 "\n", sink);
    return 0;
}
