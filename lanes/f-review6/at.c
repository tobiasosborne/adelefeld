/* f-review6: _at forms: component, where, real place, missing place, aliasing, transactions. */
#include <stdio.h>
#include <adelefeld.h>
static void setb(adf_lball_t x, ulong p, slong u, slong v, slong N, int exact)
{ x->p=p; fmpq_set_si(x->u,u,1); x->v=v; x->N=N; x->exact=exact; }
static void dump(const char *t, int st, adf_place_t w, const adf_sball_t y)
{
    printf("%-44s %-15s where=%s%lu arch=%d len=%ld", t, adf_status_str(st),
           adf_place_is_archimedean(w)?"real":"", adf_place_is_archimedean(w)?0:adf_place_prime_get(w), adf_sball_arch(y), (long)y->len);
    for (slong i=0;i<y->len;i++) { printf(" | p%lu e%d v%ld N%ld u=", y->loc[i].p, y->loc[i].exact, (long)y->loc[i].v, (long)y->loc[i].N); fmpq_print(y->loc[i].u); }
    printf("\n");
}
int main(void)
{
    adf_lball_struct c[3]; adf_sball_t x, y, x0, xc; arb_t r; adf_place_t p3,p5,p7,p11,inf,w; int st;
    for (int i=0;i<3;i++) adf_lball_init(c+i);
    setb(c+0,3,2,0,4,0);   /* 2 + 3^4 Z_3 : not a square */
    setb(c+1,5,4,0,0,1);   /* exact 4 at 5 */
    setb(c+2,7,2,0,3,0);   /* 2 + 7^3 Z_7 */
    adf_sball_init(x); adf_sball_init(y); adf_sball_init(x0); adf_sball_init(xc); arb_init(r); arb_set_si(r,4);
    adf_place_prime(&p3,3); adf_place_prime(&p5,5); adf_place_prime(&p7,7); adf_place_prime(&p11,11); inf=adf_place_inf();
    st=adf_sball_set_arb_lballs(x,NULL,r,c,3); printf("build %s\n",adf_status_str(st));
    adf_sball_set_arb_lballs(x0,NULL,NULL,c,3);
    w=p11; st=adf_sball_root_seed_at(y,&w,x,p5,2,2,10); dump("seed_at 5 n=2 seed 2",st,w,y);
    w=p11; st=adf_sball_root_seed_at(y,&w,x,p7,2,3,10); dump("seed_at 7 n=2 seed 3",st,w,y);
    w=p11; st=adf_sball_root_seed_at(y,&w,x,p7,2,4,10); dump("seed_at 7 n=2 seed 4 (y keeps prev)",st,w,y);
    w=p11; st=adf_sball_root_seed_at(y,&w,x,p3,2,1,10); dump("seed_at 3 n=2 (DOMAIN)",st,w,y);
    w=p3; st=adf_sball_root_seed_at(y,&w,x,p11,2,1,10); dump("seed_at 11 not a place",st,w,y);
    w=p3; st=adf_sball_root_seed_at(y,&w,x,inf,2,1,10); dump("seed_at real (has real)",st,w,y);
    w=p3; st=adf_sball_root_seed_at(y,&w,x0,inf,2,1,10); dump("seed_at real (no real)",st,w,y);
    w=p3; st=adf_sball_sqrt_seed_at(y,&w,x,inf,1,10); dump("sqrt_seed_at real (has real)",st,w,y);
    w=p3; st=adf_sball_root_seed_at(y,&w,x,p7,0,1,10); dump("seed_at 7 n=0",st,w,y);
    w=p3; st=adf_sball_root_seed_at(y,NULL,x,p7,2,3,10); dump("seed_at 7 where NULL",st,w,y);
    adf_sball_set(xc,x);
    w=p3; st=adf_sball_root_seed_at(xc,&w,xc,p7,2,3,10); dump("alias y=x seed_at 7",st,w,xc);
    adf_sball_set(xc,x);
    w=p3; st=adf_sball_root_seed_at(xc,&w,xc,p7,2,4,10); dump("alias y=x bad seed (x unchanged?)",st,w,xc);
    printf("  identical to x: %d\n", adf_sball_identical(xc,x));
    {
        adf_lball_struct ys[4]; ulong ids[4]={9,9,9,9}; slong len=-3;
        for (int i=0;i<4;i++) { adf_lball_init(ys+i); setb(ys+i,3,7,0,0,1); }
        w=p11; st=adf_sball_roots_at(ys,ids,&len,4,&w,x,p5,4,6); printf("roots_at 5 n=4: %s len=%ld ids %lu %lu %lu %lu where=%lu\n",adf_status_str(st),(long)len,ids[0],ids[1],ids[2],ids[3],adf_place_prime_get(w));
        for (int i=0;i<len;i++){ printf("   p%lu e%d v%ld N%ld ",ys[i].p,ys[i].exact,(long)ys[i].v,(long)ys[i].N); fmpq_print(ys[i].u); printf("\n"); }
        w=p11; len=-3; st=adf_sball_roots_at(ys,ids,&len,4,&w,x,inf,2,6); printf("roots_at real: %s len=%ld where real=%d\n",adf_status_str(st),(long)len,adf_place_is_archimedean(w));
        w=p11; st=adf_sball_roots_at(ys,ids,&len,4,&w,x0,inf,2,6); printf("roots_at real no real: %s where real=%d\n",adf_status_str(st),adf_place_is_archimedean(w));
        w=p11; st=adf_sball_roots_at(ys,ids,&len,1,&w,x,p5,4,6); printf("roots_at cap 1: %s len=%ld where=%lu\n",adf_status_str(st),(long)len,adf_place_prime_get(w));
        w=p11; st=adf_sball_roots_at(ys,ids,&len,4,&w,x,p3,2,6); printf("roots_at 3 DOMAIN: %s where=%lu\n",adf_status_str(st),adf_place_prime_get(w));
        for (int i=0;i<4;i++) adf_lball_clear(ys+i);
    }
    for (int i=0;i<3;i++) adf_lball_clear(c+i);
    adf_sball_clear(x); adf_sball_clear(y); adf_sball_clear(x0); adf_sball_clear(xc); arb_clear(r);
    flint_cleanup();
    return 0;
}
