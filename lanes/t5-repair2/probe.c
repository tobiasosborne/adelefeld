#include <flint/acb.h>
#include <stdio.h>
int main(void) {
 acb_t z,g,q,f,p,r;
 acb_init(z);
 acb_init(g);
 acb_init(q);
 acb_init(f);
 acb_init(p);
 acb_init(r);
 acb_set_si(z,-3);
 acb_mul_2exp_si(z,z,-3);
 arb_add_error_2exp_si(acb_realref(z),-13);
 arb_add_error_2exp_si(acb_imagref(z),-13);
 acb_const_pi(p,192);
 acb_log(p,p,192);
 acb_mul(p,p,z,192);
 acb_neg(p,p);
 acb_exp(p,p,192);
 for(int n=0;n<6;n++) {
  acb_one(q);for(int j=0;j<n;j++){acb_add_ui(f,z,j,192);
 acb_mul(q,q,f,192);}
  acb_add_ui(g,z,n,192);
 acb_gamma(g,g,192);
 acb_div(r,g,q,192);
 acb_mul(r,r,p,192);
  printf("n=%d q=",n);
 acb_printd(q,16);
  printf(" gamma=");
 acb_printd(g,16);
  printf(" value=");
 acb_printd(r,16);
  puts("");
 }
 acb_clear(z);
 acb_clear(g);
 acb_clear(q);
 acb_clear(f);
 acb_clear(p);
 acb_clear(r);flint_cleanup();return 0;
}
