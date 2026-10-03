from pathlib import Path
import subprocess
P=Path('lanes/f-review8')
s=Path('src/lroot.c').read_text()
s=s.replace('/* (Aq gq^(-k))', 'review_in_digit=1;\n            /* (Aq gq^(-k))')
s=s.replace('dq=d/n_pow(q,b);','review_in_digit=0;\n        dq=d/n_pow(q,b);')
(P/'build/r8-counted-source.c').write_text(s)
s=(P/'repro_r8.c').read_text()
s=s.replace('static unsigned long mul_calls, pow_calls;',
 '''static unsigned long mul_calls, pow_calls, digit_mul, digit_pow, binary_digit_products;
static int review_in_digit;''')
s=s.replace('{ mul_calls++; return n_mulmod2_preinv(a,b,n,inv); }',
 '''{ mul_calls++; digit_mul+=review_in_digit; return n_mulmod2_preinv(a,b,n,inv); }''')
s=s.replace('{ pow_calls++; return n_powmod2_ui_preinv(a,e,n,inv); }',
 '''{ pow_calls++; digit_pow+=review_in_digit;
  if(review_in_digit) { ulong t=e; while(t) { if(t&1) binary_digit_products++; t>>=1;
      if(t) binary_digit_products++; } }
  return n_powmod2_ui_preinv(a,e,n,inv); }''')
s=s.replace('#include "../../src/lroot.c"','#include "build/r8-counted-source.c"')
s=s.replace('mul_calls=pow_calls=0;',
            'mul_calls=pow_calls=digit_mul=digit_pow=binary_digit_products=0; review_in_digit=0;')
s=s.replace('    return bad ? 1 : 0;',
 '''    printf("digit_explicit_products=%lu digit_power_calls=%lu "
           "own_binary_power_reference_products=%lu loop_bound=6\\n",
           digit_mul,digit_pow,binary_digit_products);
    return bad ? 1 : 0;''')
(P/'repro_r8_counted.c').write_text(s)
subprocess.run(['timeout','60','cc','-std=gnu11','-O2','-g','-Iinclude','-Isrc',str(P/'repro_r8_counted.c'),
                str(P/'build/libadelefeld.a'),'-lflint','-lgmp','-lm','-o',str(P/'build/repro_r8_counted')],
               check=True)
subprocess.run(['timeout','60',str(P/'build/repro_r8_counted')],check=True)
