#define main jbound_probe_main
#include "certprobe.c"
#undef main
int main(void){double zr,zi;unsigned long C;acb_t z,val;acb_ptr coef=_acb_vec_init(2);arb_t a0,err;arf_t target;acb_init(z);acb_init(val);arb_init(a0);arb_init(err);arf_init(target);acb_one(coef+1);arf_one(target);arf_mul_2exp_si(target,target,-64);
while(scanf("%lu %lf %lf",&C,&zr,&zi)==3){ulong work=0,deg[64]={0};arb_set_d(acb_realref(z),zr);arb_set_d(acb_imagref(z),zi);arb_const_pi(a0,256);arb_div_ui(a0,a0,C,256);int st=tt_side(val,err,z,coef,1,0,a0,1UL<<20,target,256,&work,deg);printf("%d %lu | ",st,work);char *a=arb_dump_str(acb_realref(val)),*b=arb_dump_str(acb_imagref(val)),*c=arb_dump_str(err);printf("%s;%s | %s | ",a,b,c);for(int k=0;k<20;k++)printf("%lu,",deg[k]);puts("");flint_free(a);flint_free(b);flint_free(c);}
_acb_vec_clear(coef,2);acb_clear(z);acb_clear(val);arb_clear(a0);arb_clear(err);arf_clear(target);flint_cleanup();return 0;}
