from pathlib import Path
import subprocess
P=Path('lanes/f-review8')
s=Path('src/lpow.c').read_text()
s='''#include <time.h>
double review_first,review_second,review_third;
static double review_now(void)
{ struct timespec t; clock_gettime(CLOCK_MONOTONIC_RAW,&t); return t.tv_sec+1e-9*t.tv_nsec; }
'''+s
pairs=[('st = adf_lball_root_seed(r, x, n1, seed, Nr);','review_first'),
       ('st = adf_lball_pow_si(P, r, e1);','review_second'),
       ('st = adf_lball_Log(l, uc, K - beta > c ? K - beta : c);','review_first'),
       ('st = adf_lball_mul(t, l, sc);','review_second'),
       ('st = adf_lball_exp(z, t, K);','review_third')]
for old,clock in pairs:
    assert s.count(old)==1
    s=s.replace(old,'{ double review_start=review_now(); '+old+' '+clock+'+=review_now()-review_start; }')
(P/'build/cost-lpow.c').write_text(s)
s=(P/'cost_phase.c').read_text()
s=s.replace('static double now(void)','extern double review_first,review_second,review_third;\nstatic double now(void)')
s=s.replace('double elapsed=now()-start;',
'''double elapsed=now()-start;
    printf("INSTR %s %lu %ld %s %.9f %.9f %.9f %.9f %.9f\\n",fn,p,N,
           ball ? "ball" : "exact",elapsed,review_first+review_second+review_third,
           review_first,review_second,review_third); fflush(stdout);''')
(P/'cost_instrument.c').write_text(s)
subprocess.run(['timeout','60','cc','-std=gnu11','-O2','-g','-Iinclude','-Isrc',
                str(P/'cost_instrument.c'),str(P/'build/cost-lpow.c'),str(P/'build/libadelefeld.a'),
                '-lflint','-lgmp','-lm','-o',str(P/'build/cost_instrument')],check=True)
