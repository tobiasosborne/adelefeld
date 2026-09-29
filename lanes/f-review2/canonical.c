#include <adelefeld.h>
#include <stdio.h>

static int gcd(int a,int b) { if(a<0)a=-a; while(b){int r=a%b;a=b;b=r;} return a; }

int main(void)
{
    adf_sball_t x;
    adf_sball_init(x);
    adf_lball_struct *storage=flint_malloc(2*sizeof(adf_lball_struct));
    for(int i=0;i<2;i++)adf_lball_init(storage+i);
    unsigned long cases=0, failures=0;
    for(int arch=-1;arch<=3;arch++) for(int real=0;real<3;real++) for(int imag=0;imag<3;imag++)
    for(int len=-1;len<=2;len++) for(int null=0;null<2;null++) for(int ord=0;ord<3;ord++)
    {
        x->arch=arch; x->len=len; x->loc=null?NULL:storage;
        acb_zero(x->inf);
        if(real==1)arb_one(acb_realref(x->inf));
        if(real==2)arb_indeterminate(acb_realref(x->inf));
        if(imag==1)arb_one(acb_imagref(x->inf));
        if(imag==2)arb_indeterminate(acb_imagref(x->inf));
        storage[0].p=ord==2?3:2;storage[1].p=ord==0?3:2;
        int want=arch>=0&&arch<=2&&real!=2&&imag!=2&&
                 (arch!=1||imag==0)&&(arch!=0||(real==0&&imag==0))&&len>=0&&
                 (!len||!null)&&(len<2||ord==0);
        int got=adf_sball_is_canonical(x); cases++; failures+=got!=want;
    }
    acb_zero(x->inf);x->arch=0;x->len=1;x->loc=storage;
    for(int prime=1;prime<=7;prime++) for(int ex=-1;ex<=2;ex++)
    for(int num=-8;num<=8;num++) for(int den=-1;den<=4;den++)
    for(int v=-2;v<=2;v++) for(int N=-2;N<=2;N++)
    {
        adf_lball_struct *b=storage;
        b->p=(ulong)prime;b->exact=ex;b->v=v;b->N=N;
        fmpz_set_si(fmpq_numref(b->u),num);fmpz_set_si(fmpq_denref(b->u),den);
        int want=(prime==2||prime==3||prime==5||prime==7)&&den>0&&gcd(num,den)==1&&
                 (ex==0||ex==1);
        if(ex==1)want &= N==0&&((num==0&&v==0)||(num!=0&&num%prime&&den%prime));
        if(ex==0)
        {
            long power=1;
            for(int k=0;k<N-v;k++)power*=prime;
            want &= den==1&&((num==0&&v==0)||(v<N&&num>0&&num%prime&&num<power));
        }
        int got=adf_sball_is_canonical(x);cases++;failures+=got!=want;
    }
    printf("cases=%lu mismatches=%lu\n",cases,failures);
    /* Restore valid ownership and rational denominators before releasing the raw test storage. */
    for(int i=0;i<2;i++)fmpz_one(fmpq_denref(storage[i].u));
    x->len=2;x->loc=storage;adf_sball_clear(x);flint_cleanup();return failures?1:0;
}
