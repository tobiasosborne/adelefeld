/* f-review6: limits, small results, aliasing, capacity, transactions. */
#include <stdio.h>
#include <time.h>
#include <adelefeld.h>
static void setb(adf_lball_t x, ulong p, slong u, slong v, slong N, int exact)
{ x->p=p; fmpq_set_si(x->u,u,1); x->v=v; x->N=N; x->exact=exact; }
static void show(const char *what, int st, const adf_lball_t y)
{
    printf("%-58s %-14s", what, adf_status_str(st));
    if (st==ADF_OK) { printf(" exact=%d v=%ld N=%ld u=", y->exact,(long)y->v,(long)y->N);
        if (fmpz_bits(fmpq_numref(y->u))<64) fmpq_print(y->u); else printf("<%lu bits>", (ulong)fmpz_bits(fmpq_numref(y->u))); }
    printf("\n");
}
int main(void)
{
    adf_lball_t x,y,l; adf_place_t v2; int st; clock_t t0;
    adf_lball_init(x); adf_lball_init(y); adf_lball_init(l);
    adf_place_prime(&v2,2);
    const slong B=ADF_LBALL_EXP_MAX;
    /* (a) 1 + 2^R Z_2, R = 2^27: result 1 + 2^(R-1) Z_2 is small (u = 1). Compare Log and pow. */
    slong R=(slong)1<<27;
    setb(x,2,1,0,R,0); printf("canonical %d\n", adf_lball_is_canonical(x));
    t0=clock(); st=adf_lball_root_seed(y,x,2,1,0); show("root_seed(1+2^(2^27)Z_2, n=2, seed 1)",st,y);
    printf("  %.2fs\n",(double)(clock()-t0)/CLOCKS_PER_SEC);
    st=adf_lball_Log(l,x,R); show("Log(1+2^(2^27)Z_2, N=2^27)",st,l);
    setb(y,2,1,0,R-1,0); st=adf_lball_pow_si(l,y,2); show("pow_si(1+2^(2^27-1)Z_2, 2)",st,l);
    setb(x,2,1,0,B,0);
    st=adf_lball_root_seed(y,x,2,1,0); show("root_seed(1+2^(2^60)Z_2, n=2)",st,y);
    setb(x,5,1,0,B,0);
    st=adf_lball_root_seed(y,x,3,1,0); show("root_seed(1+5^(2^60)Z_5, n=3)",st,y);
    st=adf_lball_root_seed(y,x,1,1,0); show("root_seed(1+5^(2^60)Z_5, n=1)",st,y);
    /* exact powers of p near the bound */
    setb(x,2,1,B,0,1); st=adf_lball_root_seed(y,x,2,1,5); show("root_seed(exact 2^(2^60), n=2, N=5)",st,y);
    st=adf_lball_root_seed(y,x,2,3,5); show("root_seed(exact 2^(2^60), n=2, seed 3)",st,y);
    setb(x,2,1,-B,0,1); st=adf_lball_root_seed(y,x,(ulong)B,1,5); show("root_seed(exact 2^-(2^60), n=2^60)",st,y);
    setb(x,7,2,B,0,1); st=adf_lball_root_seed(y,x,2,3,B); show("exact 7^(2^60)*2, n=2, seed 3, N=2^60",st,y);
    st=adf_lball_root_seed(y,x,2,3,B/2+5); show("exact 7^(2^60)*2, n=2, seed 3, N=2^59+5",st,y);
    st=adf_lball_root_seed(y,x,2,3,-B); show("exact 7^(2^60)*2, n=2, seed 3, N=-2^60",st,y);
    st=adf_lball_root_seed(y,x,2,3,B+1); show("exact 7^(2^60)*2, n=2, seed 3, N=2^60+1",st,y);
    st=adf_lball_root_seed(y,x,2,3,WORD_MIN); show("exact 7^(2^60)*2, n=2, seed 3, N=WORD_MIN",st,y);
    st=adf_lball_root_seed(y,x,2,3,WORD_MAX); show("exact 7^(2^60)*2, n=2, seed 3, N=WORD_MAX",st,y);
    /* balls near the bound */
    setb(x,7,2,-B,B,0); st=adf_lball_root_seed(y,x,2,3,0); show("ball 7^-(2^60)*2 + 7^(2^60), n=2",st,y);
    setb(x,7,2,-B,-B+3,0); st=adf_lball_root_seed(y,x,2,3,0); show("ball 7^-(2^60)*2 + 7^(-2^60+3), n=2",st,y);
    setb(x,7,2,B-3,B,0); st=adf_lball_root_seed(y,x,2,3,0); show("ball 7^(2^60-4)... v=B-3 odd -> n=2",st,y);
    setb(x,7,2,B-4,B,0); st=adf_lball_root_seed(y,x,2,3,0); show("ball 7^(2^60-4)*2 + 7^(2^60), n=2",st,y);
    setb(x,7,2,B-4,B+1,0); st=adf_lball_root_seed(y,x,2,3,0); show("ball N=2^60+1 (input over bound)",st,y);
    /* exact unit 1 with huge N, irrational branch: LIMIT; rational branch: exact */
    setb(x,5,1,0,0,1); st=adf_lball_root_seed(y,x,4,2,B); show("exact 1 at 5, n=4, seed 2 (i), N=2^60",st,y);
    st=adf_lball_root_seed(y,x,4,4,B); show("exact 1 at 5, n=4, seed 4 (-1), N=2^60",st,y);
    st=adf_lball_root_seed(y,x,4,1,B); show("exact 1 at 5, n=4, seed 1, N=2^60",st,y);
    /* exact 1 with N small: irrational i at 5, N<=0 */
    st=adf_lball_root_seed(y,x,4,2,0); show("exact 1 at 5, n=4, seed 2, N=0",st,y);
    st=adf_lball_root_seed(y,x,4,2,-4); show("exact 1 at 5, n=4, seed 2, N=-4",st,y);
    /* roots: late LIMIT after an exact first branch, y untouched */
    {
        adf_lball_struct ys[4]; ulong ids[4]={9,9,9,9}; slong len=-1;
        for (int i=0;i<4;i++) { adf_lball_init(ys+i); setb(ys+i,3,7,0,0,1); }
        st=adf_lball_roots(ys,ids,&len,4,x,4,B);
        printf("roots(exact 1 at 5, n=4, N=2^60): %s len=%ld ids=%lu,%lu y0=%ld\n",adf_status_str(st),(long)len,ids[0],ids[1],
               fmpz_get_si(fmpq_numref(ys[0].u)));
        st=adf_lball_roots(ys,ids,&len,4,x,4,10);
        printf("roots(exact 1 at 5, n=4, N=10): %s len=%ld ids=%lu,%lu,%lu,%lu\n",adf_status_str(st),(long)len,ids[0],ids[1],ids[2],ids[3]);
        for (int i=0;i<4;i++) show("  ",0,ys+i);
        /* capacity tests */
        len=-1; st=adf_lball_roots(ys,ids,&len,3,x,4,10); printf("cap 3: %s len=%ld\n",adf_status_str(st),(long)len);
        st=adf_lball_roots(ys,ids,&len,-1,x,4,10); printf("cap -1: %s len=%ld\n",adf_status_str(st),(long)len);
        st=adf_lball_roots(ys,ids,&len,0,x,4,10); printf("cap 0: %s len=%ld\n",adf_status_str(st),(long)len);
        setb(x,5,2,0,0,1);
        st=adf_lball_roots(ys,ids,&len,-1,x,4,10); printf("cap -1, DOMAIN input 2 at 5 n=4: %s\n",adf_status_str(st));
        /* aliasing x = ys[k] */
        for (int k=0;k<4;k++) {
            for (int i=0;i<4;i++) setb(ys+i,3,7,0,0,1);
            setb(ys+k,5,1,0,0,1);  /* x = exact 1 at 5, stored in slot k */
            st=adf_lball_roots(ys,ids,&len,4,ys+k,4,6);
            printf("alias x=y[%d]: %s len=%ld:",k,adf_status_str(st),(long)len);
            for (int i=0;i<len;i++) { printf(" [%lu] ",ids[i]); fmpq_print(ys[i].u); printf(" v%ld N%ld e%d;",(long)ys[i].v,(long)ys[i].N,ys[i].exact); }
            printf("\n");
        }
        for (int i=0;i<4;i++) adf_lball_clear(ys+i);
    }
    /* aliasing y = x in seed */
    setb(x,7,2,0,5,0); st=adf_lball_root_seed(x,x,2,3,0); show("alias y=x, 2+7^5Z_7, n=2, seed 3",st,x);
    adf_lball_clear(x); adf_lball_clear(y); adf_lball_clear(l);
    flint_cleanup();
    return 0;
}
