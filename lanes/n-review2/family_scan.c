/* Exact replay with a proved skip, not a proposed replacement for the library printer.
   If M > R, then M-R is a positive multiple of U=10^(X(rad)-k+1), since both M and R are.
   Since M-R <= mid-rad = delta, necessarily U <= delta. Thus k >= X(rad)-X(delta)+1.
   All skipped levels still contribute their S to the recorded work. */
#include "review_text.c"
#include <stdio.h>
#include <stdlib.h>

static unsigned long replay(slong b, slong *first_k, unsigned long *levels, unsigned long *passes,
                            unsigned long *maxsize)
{
    fmpq_t mid,rad,M,R,delta;
    char *prev=NULL;
    size_t prevlen=0;
    unsigned long work=0;
    *levels=*passes=*maxsize=0;
    fmpq_init(mid); fmpq_init(rad); fmpq_init(M); fmpq_init(R); fmpq_init(delta);
    fmpz_one(fmpq_numref(mid)); fmpz_mul_2exp(fmpq_numref(mid),fmpq_numref(mid),b);
    fmpz_add_ui(fmpq_numref(mid),fmpq_numref(mid),1); fmpz_set_ui(fmpq_denref(mid),2);
    fmpz_one(fmpq_numref(rad)); fmpz_mul_2exp(fmpq_numref(rad),fmpq_numref(rad),b-1);
    for (;;) {
        tx_buf buf;
        unsigned long size=tx_cond_size(mid,rad);
        if(size>*maxsize) *maxsize=size;
        fmpq_sub(delta,mid,rad);
        slong k=FLINT_MAX(2,tx_floor_log10(rad)-tx_floor_log10(delta)+1);
        for(;;k++) {
            tx_buf_init(&buf); tx_real_level(&buf,M,R,mid,rad,1,k);
            if(fmpq_cmp(M,R)>0) break;
            flint_free(buf.p);
        }
        if(*passes==0) *first_k=k;
        (*passes)++; *levels+=k-1; work+=(k-1)*size;
        if(prev && prevlen==buf.len && memcmp(prev,buf.p,prevlen)==0) {
            flint_free(buf.p); break;
        }
        flint_free(prev); prev=buf.p; prevlen=buf.len;
        fmpq_set(mid,M); fmpq_set(rad,R);
        if(*passes>20) abort();
    }
    flint_free(prev);
    fmpq_clear(mid); fmpq_clear(rad); fmpq_clear(M); fmpq_clear(R); fmpq_clear(delta);
    return work;
}

int main(int argc,char **argv)
{
    slong lo=argc>1 ? atol(argv[1]):6001, hi=argc>2 ? atol(argv[2]):8000;
    slong first=0;
    unsigned refused=0, nonmonotone=0;
    int previous=0;
    for(slong b=lo;b<=hi;b++) {
        slong k;
        unsigned long levels,passes,maxsize;
        unsigned long work=replay(b,&k,&levels,&passes,&maxsize);
        int over=work>TX_COND_WORK_MAX;
        if(over) {refused++; if(!first) first=b;}
        if(previous && !over) nonmonotone++;
        if((b>=7440 && b<=7490) || b==lo || b==hi)
            printf("b=%ld k=%ld levels=%lu passes=%lu Smax=%lu work=%lu refused=%d\n",
                   b,k,levels,passes,maxsize,work,over);
        previous=over;
    }
    printf("inputs=%ld first_refused=%ld refused=%u reversals=%u\n",hi-lo+1,first,refused,nonmonotone);
    flint_cleanup(); return 0;
}
