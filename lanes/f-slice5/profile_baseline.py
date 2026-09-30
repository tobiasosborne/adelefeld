from pathlib import Path
s=Path('lanes/f-slice5/baseline.c').read_text()
s='#define _POSIX_C_SOURCE 200809L\n#include <time.h>\n'+s
mark='static void\nlog_sum'
s=s.replace(mark,'''static double tick(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC_RAW,&t);
    return t.tv_sec+1e-9*t.tv_nsec; }
static void
log_sum''')
s=s.replace('    slong k;\n    fmpz_init(zk);','    slong k;\n    double a, mul=0, div=0, inverse=0, product=0, sum=0;\n    fmpz_init(zk);')
s=s.replace('        fmpz_mul(zk, zk, zr);','        a=tick(); fmpz_mul(zk, zk, zr);')
s=s.replace('        fmpz_mod(zk, zk, P);','        fmpz_mod(zk, zk, P); mul+=tick()-a; a=tick();')
s=s.replace('        fmpz_set_ui(kk, kp);','        div+=tick()-a; a=tick(); fmpz_set_ui(kk, kp);')
s=s.replace('        fmpz_mul(term, term, inv);','        inverse+=tick()-a; a=tick(); fmpz_mul(term, term, inv);')
s=s.replace('        if (k % 2 == 1)','        product+=tick()-a; a=tick(); if (k % 2 == 1)')
s=s.replace('        fmpz_mod(S, S, P);','''        fmpz_mod(S, S, P); sum+=tick()-a;
        if (k%1000==0 || k==T) {
            fprintf(stderr,"terms=%ld power_mod=%.6f division=%.6f inverse=%.6f "
                "term_product=%.6f sum_mod=%.6f\\n", k,mul,div,inverse,product,sum);
            fflush(stderr);
        }''')
Path('lanes/f-slice5/profile-baseline.c').write_text(s)
