/* Reproducible old/new Log_refine comparison. Two executables read the same PRNG stream.
   Print status, where and the canonical value (or preservation marker) for each request. */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <adelefeld.h>

static uint64_t state = UINT64_C(0x8f1300261004);
static ulong next(void)
{
    state ^= state << 13; state ^= state >> 7; state ^= state << 17;
    return (ulong)state;
}

int main(int argc, char **argv)
{
    const ulong small[]={2,3,5,7,11,13,17,19,23,31,65537,UWORD_MAX-58};
    const slong large[]={1048577,22369620,33554430,33554431,33554432,33554433,
                         ADF_LBALL_EXP_MAX,ADF_LBALL_EXP_MAX+1,-ADF_LBALL_EXP_MAX-1,WORD_MIN,WORD_MAX};
    slong count=argc>1?strtol(argv[1],NULL,10):20000;
    adf_idele_t x;
    adf_adele_t y, saved;
    adf_place_t sentinel,ps[6];
    ulong counts[16]={0}, large_requests=0, empty=0, negative=0;
    adf_idele_init(x); adf_adele_init(y); adf_adele_init(saved);
    adf_place_prime(&sentinel,1000003);
    arb_set_si(saved->inf,5); fmpz_set_ui(saved->fin.A,3); fmpz_set_ui(saved->fin.H,11);
    for (slong i=0;i<count;i++)
    {
        slong n=(slong)(next()%6), N=(slong)(next()%17)-3, prec=(slong)(next()%128)-2;
        int absval=(int)(next()%2), mode=(int)(next()%12);
        adf_place_t w=sentinel;
        arb_set_si(x->inf,next()%2?2:-2);
        fmpq_set_si(x->r,(slong)(next()%40)+1,next()%40+1);
        fmpz_one(x->u.c); fmpz_one(x->u.N);
        if (mode<=2)
        {
            /* Large N with a small image: includes nonzero restricted centres. */
            fmpz_set_ui(x->u.N,mode==0?8:mode==1?9:125);
            fmpz_set_ui(x->u.c,mode==0?3:2);
            if (next()%2) N=large[next()%(sizeof(large)/sizeof(large[0]))];
        }
        else if (mode==3)
        {
            /* Exact zero: large successful moduli are bounded to keep this run on a laptop. */
            fmpq_one(x->r); fmpz_zero(x->u.N); fmpz_set_si(x->u.c,next()%2?1:-1);
            N=next()%2?(slong)(next()%1024):large[next()%(sizeof(large)/sizeof(large[0]))];
            if (n>0 && N>=1048577 && N<=ADF_LBALL_EXP_MAX) n=6;
        }
        else if (mode==4)
        {
            fmpz_zero(x->u.N); fmpz_set_si(x->u.c,next()%2?1:-1);
            /* Negative exponent refusals do not do any series work. */
            N=next()%2?-ADF_LBALL_EXP_MAX-1:WORD_MIN;
        }
        else if (mode==5)
        {
            /* Known refusal after small capped earlier components. */
            fmpq_set_si(x->r,3,1); fmpz_zero(x->u.N);
            fmpz_set_si(x->u.c,next()%2?1:-1); n=2;
            N=large[next()%(sizeof(large)/sizeof(large[0]))];
        }
        else if (mode==6 && i<120)
        {
            /* Near a working bound: earlier failure before a later known refusal. */
            ulong p=small[next()%3];
            fmpq_set_si(x->r,(slong)(next()%23)+2,next()%23+1); fmpz_zero(x->u.N);
            fmpz_set_si(x->u.c,next()%2?1:-1);
            N=ADF_LBALL_BITS_MAX/(p==5?3:2)-(slong)(next()%3); n=2;
            adf_place_prime(&ps[0],p); adf_place_prime(&ps[1],UWORD_MAX-58);
        }
        else if (mode==7) prec=ADF_REAL_PREC_MAX+1;
        else if (mode==8) n=-1;
        else if (mode==9) n=ADF_IDLOG_PLACES_MAX+1;
        for (slong j=0;j<n && n<=6;j++)
        {
            ulong p=small[next()%(sizeof(small)/sizeof(small[0]))];
            if (mode==3 && n==6) p=j==5?UWORD_MAX-58:small[j];
            if (mode==5) p=j==0?3:UWORD_MAX-58;
            if (mode==6 && i<120) continue;
            adf_place_prime(&ps[j],p);
        }
        if (mode==10 && n>0) ps[next()%(ulong)n]=adf_place_inf();
        if (mode==11 && n>1) ps[n-1]=ps[0];
        large_requests += N>=1048577 || N<=-1048577;
        empty += n==0; negative += n<0;
        adf_adele_set(y,saved);
        int st=absval?adf_idele_log_abs_refine(y,&w,x,n>0&&n<=6?ps:NULL,n,N,prec):
                      adf_idele_Log_refine(y,&w,x,n>0&&n<=6?ps:NULL,n,N,prec);
        if (st>=0 && st<16) counts[st]++;
        printf("%ld %d %lu ",i,st,adf_place_is_archimedean(w)?0:adf_place_prime_get(w));
        if (st==ADF_OK)
        {
            /* The only real inputs are +-2, so the real result is equal across all OK requests. */
            arb_dump_file(stdout,y->inf); putchar(' ');
            fmpz_print(y->fin.A); putchar(' '); fmpz_print(y->fin.H); putchar(' '); fmpz_print(y->fin.d);
        }
        else printf("kept=%d",adf_adele_identical(y,saved));
        putchar('\n');
    }
    fprintf(stderr,"requests=%ld large_N=%lu empty=%lu negative_n=%lu",count,large_requests,empty,negative);
    for (int j=0;j<16;j++) if(counts[j]) fprintf(stderr," status%d=%lu",j,counts[j]);
    putc('\n',stderr);
    adf_idele_clear(x); adf_adele_clear(y); adf_adele_clear(saved); flint_cleanup_master();
    return 0;
}
