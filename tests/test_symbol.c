/* Symbol tests: all fixture rows, independent Euler/square-count oracle in proto/symbol_checks.py.
   An exact sign/status, report-place or sentinel mismatch fails a case. Identity tests supplement,
   rather than replace, those values. No output/input alias exists: scalar output has another type. */
#include <adelefeld.h>
#include <flint/ulong_extras.h>
#include <stdlib.h>
#include <string.h>
#include "support/jsonl.h"
#include "test_runner.h"

static adf_place_t place(ulong p)
{
    adf_place_t v;
    ADF_CHECK(adf_place_prime(&v, p) == ADF_OK);
    return v;
}
static const char *field(const jsonl_value *r, const char *key)
{
    jsonl_error_t e;
    const jsonl_value *v;
    const char *s = NULL;
    ADF_CHECK(jsonl_field(r, key, &v, &e));
    ADF_CHECK(jsonl_int_text_or_string(v, &s, &e));
    return s;
}
static const char *text_field(const jsonl_value *r, const char *key)
{
    jsonl_error_t e;
    const jsonl_value *v;
    size_t len;
    ADF_CHECK(jsonl_field(r,key,&v,&e));
    const char *s=jsonl_string(v,&len,&e);
    ADF_CHECK(s!=NULL);
    return s;
}
static int call(int kind, int inp, int *z, adf_place_t *w, const fmpz_t a,
                const fmpz_t b, const fmpz_t N, const fmpz_t d)
{
    adf_place_t p = kind == 0 ? place(fmpz_get_ui(b)) : adf_place_inf();
    adf_fball_t f;
    adf_ucoset_t u;
    int st;
    if (inp == 0)
    {
        if (kind == 0) return adf_fmpz_legendre(z,w,a,p);
        if (kind == 1) return adf_fmpz_jacobi(z,w,a,b);
        return adf_fmpz_kronecker(z,w,a,b);
    }
    if (inp == 1)
    {
        adf_fball_init(f);
        ADF_CHECK(adf_fball_set_fmpz3(f,a,N,d) == ADF_OK);
        if (kind == 0) st = adf_fball_legendre(z,w,f,p);
        else if (kind == 1) st = adf_fball_jacobi(z,w,f,b);
        else st = adf_fball_kronecker(z,w,f,b);
        adf_fball_clear(f);
    }
    else
    {
        adf_ucoset_init(u);
        ADF_CHECK(adf_ucoset_set_fmpz2(u,a,N) == ADF_OK);
        if (kind == 0) st = adf_ucoset_legendre(z,w,u,p);
        else if (kind == 1) st = adf_ucoset_jacobi(z,w,u,b);
        else st = adf_ucoset_kronecker(z,w,u,b);
        adf_ucoset_clear(u);
    }
    return st;
}
ADF_TEST(residue_vectors)
{
    jsonl_file *f = NULL;
    jsonl_error_t e;
    fmpz_t a,b,N,d;
    adf_place_t sentinel = place(97), w;
    ADF_CHECK_MSG(jsonl_open("tests/ref/vectors/f-slice12/residue.jsonl",&f,&e),
                  "%s",jsonl_error_message(&e));
    if (!f) return;
    fmpz_init(a); fmpz_init(b); fmpz_init(N); fmpz_init(d);
    for (size_t i=0; i<jsonl_count(f); i++)
    {
        const jsonl_value *r = jsonl_record(f,i);
        int kind=atoi(field(r,"kind")), inp=atoi(field(r,"input"));
        int want=atoi(field(r,"value")), status=atoi(field(r,"status")), z=99, st;
        ADF_CHECK(fmpz_set_str(a,field(r,"a"),10) == 0);
        ADF_CHECK(fmpz_set_str(b,field(r,"b"),10) == 0);
        ADF_CHECK(fmpz_set_str(N,field(r,"N"),10) == 0);
        ADF_CHECK(fmpz_set_str(d,field(r,"d"),10) == 0);
        w=sentinel;
        st=call(kind,inp,&z,&w,a,b,N,d);
        ADF_CHECK_MSG(st == status,"row %zu: got %d want %d",i+1,st,status);
        ADF_CHECK_MSG(z == want,"row %zu: value %d want %d",i+1,z,want);
        ADF_CHECK(adf_place_equal(w,kind==0 && st!=ADF_OK ? place(fmpz_get_ui(b)) : sentinel));
    }
    printf("residue fixture rows: %zu\n",jsonl_count(f));
    jsonl_close(f);
    fmpz_clear(a); fmpz_clear(b); fmpz_clear(N); fmpz_clear(d);
}
static int kron(slong a, slong b)
{
    fmpz_t x,y;
    int z=99;
    fmpz_init(x); fmpz_init(y); fmpz_set_si(x,a); fmpz_set_si(y,b);
    ADF_CHECK(adf_fmpz_kronecker(&z,NULL,x,y) == ADF_OK);
    fmpz_clear(x); fmpz_clear(y);
    return z;
}
ADF_TEST(residue_identities)
{
    ulong ps[]={3,5,7,11,13,17,19,23,65537,UWORD(18446744073709551557)};
    fmpz_t a,b,c,t,n;
    int x,y,z;
    fmpz_init(a); fmpz_init(b); fmpz_init(c); fmpz_init(t); fmpz_init(n);
    /* Reciprocity and both supplementary laws, with full-word primes (no signed cast). */
    for (size_t i=0;i<10;i++)
    {
        fmpz_set_ui(a,ps[i]);
        ADF_CHECK(adf_fmpz_legendre(&x,NULL,a,place(ps[i]))==ADF_OK && x==0);
        fmpz_set_si(a,-1);
        ADF_CHECK(adf_fmpz_legendre(&x,NULL,a,place(ps[i]))==ADF_OK);
        ADF_CHECK(x == (ps[i]%4==1 ? 1 : -1));
        fmpz_set_ui(a,2);
        ADF_CHECK(adf_fmpz_legendre(&x,NULL,a,place(ps[i]))==ADF_OK);
        ADF_CHECK(x == (ps[i]%8==1 || ps[i]%8==7 ? 1 : -1));
        for(size_t j=0;j<10;j++) if(i!=j)
        {
            fmpz_set_ui(a,ps[i]); fmpz_set_ui(b,ps[j]);
            ADF_CHECK(adf_fmpz_legendre(&x,NULL,a,place(ps[j]))==ADF_OK);
            ADF_CHECK(adf_fmpz_legendre(&y,NULL,b,place(ps[i]))==ADF_OK);
            ADF_CHECK(x*y == (ps[i]%4==3 && ps[j]%4==3 ? -1 : 1));
        }
    }
    for(slong aa=-15;aa<=15;aa++) for(slong bb=-15;bb<=15;bb++)
        for(slong m=1;m<=31;m++)
        {
            ADF_CHECK(kron(aa*bb,m)==kron(aa,m)*kron(bb,m));
            /* Definition 1 at lower entry 0 prevents this law for a=-1; see Y4. */
            if (bb != 0) ADF_CHECK(kron(aa,bb*m)==kron(aa,bb)*kron(aa,m));
            slong odd=m;
            while(odd%2==0) odd/=2;
            slong K=m%2 ? m : 8*odd;
            ADF_CHECK(kron(aa+K,m)==kron(aa,m));
            if(m%2)
            {
                fmpz_set_si(a,aa); fmpz_set_si(b,bb); fmpz_mul(c,a,b); fmpz_set_si(n,m);
                ADF_CHECK(adf_fmpz_jacobi(&x,NULL,a,n)==ADF_OK);
                ADF_CHECK(adf_fmpz_jacobi(&y,NULL,b,n)==ADF_OK);
                ADF_CHECK(adf_fmpz_jacobi(&z,NULL,c,n)==ADF_OK && z==x*y);
                fmpz_mul(t,n,n);
                ADF_CHECK(adf_fmpz_jacobi(&z,NULL,a,t)==ADF_OK && z==x*x);
                fmpz_add(t,a,n);
                ADF_CHECK(adf_fmpz_jacobi(&z,NULL,t,n)==ADF_OK && z==x);
            }
        }
    /* Same object as both fmpz inputs is permitted. */
    fmpz_set_si(a,3);
    ADF_CHECK(adf_fmpz_jacobi(&z,NULL,a,a)==ADF_OK && z==0);
    ADF_CHECK(adf_fmpz_kronecker(&z,NULL,a,a)==ADF_OK && z==0);
    fmpz_clear(a); fmpz_clear(b); fmpz_clear(c); fmpz_clear(t); fmpz_clear(n);
}
ADF_TEST(residue_statuses_and_witnesses)
{
    fmpz_t a,b,N,d;
    adf_fball_t f;
    adf_ucoset_t u;
    adf_place_t w, sentinel=place(97), p;
    int z;
    fmpz_init(a); fmpz_init(b); fmpz_init(N); fmpz_init(d);
    adf_fball_init(f); adf_ucoset_init(u);
    fmpz_one(a); fmpz_one(d);
    for(int i=0;i<2;i++)
    {
        p=i ? adf_place_inf() : place(2);
        z=99; w=sentinel;
        ADF_CHECK(adf_fmpz_legendre(&z,&w,a,p)==ADF_DOMAIN);
        ADF_CHECK(z==99 && adf_place_equal(w,p));
        w=sentinel;
        ADF_CHECK(adf_fball_legendre(&z,&w,f,p)==ADF_DOMAIN);
        ADF_CHECK(z==99 && adf_place_equal(w,p));
        w=sentinel;
        ADF_CHECK(adf_ucoset_legendre(&z,&w,u,p)==ADF_DOMAIN);
        ADF_CHECK(z==99 && adf_place_equal(w,p));
    }
    for(slong m=-2;m<=2;m++) if(m!=1)
    {
        fmpz_set_si(b,m); z=99; w=sentinel;
        ADF_CHECK(adf_fmpz_jacobi(&z,&w,a,b)==ADF_DOMAIN);
        ADF_CHECK(adf_fball_jacobi(&z,&w,f,b)==ADF_DOMAIN);
        ADF_CHECK(adf_ucoset_jacobi(&z,&w,u,b)==ADF_DOMAIN);
        ADF_CHECK(z==99 && adf_place_equal(w,sentinel));
    }
    /* Actual ambiguity: 1 and 5 in 1+4 Zhat at lower entry 2; 1 and 3 in 1+2 Zhat. */
    ADF_CHECK(kron(1,2)==1 && kron(5,2)==-1);
    ADF_CHECK(kron(1,2)==1 && kron(3,2)==-1);
    /* A coarse Legendre input 1+Zhat contains 1 and 2, of opposite signs at 3. */
    ADF_CHECK(kron(1,3)==1 && kron(2,3)==-1);
    /* The rejected constant case is intentional; it refutes necessity in the brief. */
    ADF_CHECK(kron(1,9)==1 && kron(4,9)==1 && kron(7,9)==1);
    ADF_CHECK(kron(-1,0)==1 && kron(-1,3)==-1);
    ADF_CHECK(kron(-1,0*3)!=kron(-1,0)*kron(-1,3));
    adf_fball_clear(f); adf_ucoset_clear(u);
    fmpz_clear(a); fmpz_clear(b); fmpz_clear(N); fmpz_clear(d);
}

ADF_TEST(residue_local_backend)
{
    ulong blocks[]={8,9};
    adf_modctx_struct *ctx=NULL;
    adf_fball_t f,l;
    fmpz_t A,H,d,b;
    int z;
    adf_fball_init(f); adf_fball_init(l);
    fmpz_init(A); fmpz_init(H); fmpz_init(d); fmpz_init(b);
    ADF_CHECK(adf_modctx_new_blocks(&ctx,blocks,2)==ADF_OK);
    fmpz_set_ui(A,5); fmpz_set_ui(H,72); fmpz_one(d);
    ADF_CHECK(adf_fball_set_fmpz3(f,A,H,d)==ADF_OK);
    ADF_CHECK(adf_fball_set_local(l,f,ctx)==ADF_OK);
    fmpz_set_ui(b,2);
    ADF_CHECK(adf_fball_kronecker(&z,NULL,l,b)==ADF_OK && z==-1);
    fmpz_set_ui(b,9);
    ADF_CHECK(adf_fball_jacobi(&z,NULL,l,b)==ADF_OK && z==1);
    ADF_CHECK(adf_fball_legendre(&z,NULL,l,place(3))==ADF_OK && z==-1);
    adf_fball_clear(f); adf_fball_clear(l); adf_modctx_free(ctx);
    fmpz_clear(A); fmpz_clear(H); fmpz_clear(d); fmpz_clear(b);
}

#ifdef ADF_CHECK_INVARIANTS
#include <signal.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
ADF_TEST(residue_debug_entry_checks)
{
    for(int inp=1;inp<=2;inp++) for(int kind=0;kind<3;kind++)
    {
        pid_t pid=fork();
        ADF_CHECK(pid>=0);
        if(pid==0)
        {
            struct rlimit lim={0,0};
            fmpz_t b;
            adf_fball_t f;
            adf_ucoset_t u;
            adf_place_t p;
            int z;
            setrlimit(RLIMIT_CORE,&lim);
            if(freopen("/dev/null","w",stderr)==NULL) _exit(2);
            /* Invalid lower parameters must not bypass the public entry check. */
            fmpz_init(b); fmpz_set_ui(b,kind==1 ? 2 : 3);
            adf_place_prime(&p,2);
            if(inp==1)
            {
                adf_fball_init(f); fmpz_zero(f->d);
                if(kind==0) adf_fball_legendre(&z,NULL,f,p);
                else if(kind==1) adf_fball_jacobi(&z,NULL,f,b);
                else adf_fball_kronecker(&z,NULL,f,b);
                adf_fball_clear(f);
            }
            else
            {
                adf_ucoset_init(u); fmpz_set_si(u->N,-1);
                if(kind==0) adf_ucoset_legendre(&z,NULL,u,p);
                else if(kind==1) adf_ucoset_jacobi(&z,NULL,u,b);
                else adf_ucoset_kronecker(&z,NULL,u,b);
                adf_ucoset_clear(u);
            }
            fmpz_clear(b);
            _exit(0);
        }
        if(pid>0)
        {
            int st;
            ADF_CHECK(waitpid(pid,&st,0)==pid);
            ADF_CHECK(WIFSIGNALED(st) && WTERMSIG(st)==SIGABRT);
        }
    }
}
#endif

/* Primitive congruence oracle rows exercise rational, exact local and exact-idele forms independently. */
ADF_TEST(hilbert_vectors)
{
    jsonl_file *f=NULL;
    jsonl_error_t e;
    adf_rat_t a,b;
    adf_lball_t x,y;
    adf_idele_t i,j;
    adf_place_t w,sentinel=place(97),p;
    int z;
    ADF_CHECK_MSG(jsonl_open("tests/ref/vectors/f-slice12/hilbert.jsonl",&f,&e),
                  "%s",jsonl_error_message(&e));
    if(!f) return;
    adf_rat_init(a); adf_rat_init(b); adf_lball_init(x); adf_lball_init(y);
    adf_idele_init(i); adf_idele_init(j);
    for(size_t k=0;k<jsonl_count(f);k++)
    {
        const jsonl_value *r=jsonl_record(f,k);
        const char *as=text_field(r,"a"), *bs=text_field(r,"b");
        ulong pv=strtoul(field(r,"p"),NULL,10);
        int want=atoi(field(r,"value"));
        /* Rational strings contain '/', so they use the string accessor rather than integer reader. */
        ADF_CHECK(adf_rat_set_str(a,as,strlen(as),NULL)==ADF_OK);
        ADF_CHECK(adf_rat_set_str(b,bs,strlen(bs),NULL)==ADF_OK);
        p=pv ? place(pv) : adf_place_inf();
        z=99; w=sentinel;
        ADF_CHECK(adf_rat_hilbert_at(&z,&w,a,b,p)==ADF_OK && z==want);
        ADF_CHECK(adf_place_equal(w,sentinel));
        if(pv)
        {
            ADF_CHECK(adf_lball_set_rat(x,p,a)==ADF_OK);
            ADF_CHECK(adf_lball_set_rat(y,p,b)==ADF_OK);
            ADF_CHECK(adf_lball_hilbert(&z,&w,x,y)==ADF_OK && z==want);
            ADF_CHECK(adf_place_equal(w,sentinel));
            /* Two relative digits at odd primes, three at 2: the same square classes. */
            ADF_CHECK(adf_lball_set_rat_ball(x,p,a,x->v+(pv==2 ? 3 : 1))==ADF_OK);
            ADF_CHECK(adf_lball_set_rat_ball(y,p,b,y->v+(pv==2 ? 3 : 1))==ADF_OK);
            ADF_CHECK(adf_lball_hilbert(&z,&w,x,y)==ADF_OK && z==want);
        }
        ADF_CHECK(adf_idele_set_rat(i,a,64)==ADF_OK);
        ADF_CHECK(adf_idele_set_rat(j,b,64)==ADF_OK);
        ADF_CHECK(adf_idele_hilbert_at(&z,&w,i,j,p)==ADF_OK && z==want);
        ADF_CHECK(adf_place_equal(w,sentinel));
    }
    printf("hilbert fixture rows: %zu\n",jsonl_count(f));
    jsonl_close(f);
    adf_rat_clear(a); adf_rat_clear(b); adf_lball_clear(x); adf_lball_clear(y);
    adf_idele_clear(i); adf_idele_clear(j);
}
static int hrat(const adf_rat_t a,const adf_rat_t b,adf_place_t p)
{
    int z=99;
    ADF_CHECK(adf_rat_hilbert_at(&z,NULL,a,b,p)==ADF_OK);
    ADF_CHECK(z==1 || z==-1);
    return z;
}
ADF_TEST(hilbert_identities)
{
    ulong ps[]={0,2,3,5,7,65537,UWORD(18446744073709551557)};
    adf_rat_t a,b,c,ab,ac,neg,one_minus;
    adf_rat_init(a); adf_rat_init(b); adf_rat_init(c); adf_rat_init(ab);
    adf_rat_init(ac); adf_rat_init(neg); adf_rat_init(one_minus);
    for(size_t k=0;k<7;k++)
    {
        adf_place_t p=ps[k] ? place(ps[k]) : adf_place_inf();
        for(slong aa=-7;aa<=7;aa++) if(aa)
        {
            adf_rat_set_si(a,aa); adf_rat_neg(neg,a);
            adf_rat_one(one_minus); adf_rat_sub(one_minus,one_minus,a);
            ADF_CHECK(hrat(a,neg,p)==1);
            if(aa!=1) ADF_CHECK(hrat(a,one_minus,p)==1);
            for(slong bb=-7;bb<=7;bb++) if(bb)
            {
                adf_rat_set_si(b,bb);
                ADF_CHECK(hrat(a,b,p)==hrat(b,a,p));
                for(slong cc=-3;cc<=3;cc++) if(cc)
                {
                    adf_rat_set_si(c,cc);
                    adf_rat_mul(ab,a,b); adf_rat_mul(ac,a,c);
                    ADF_CHECK(hrat(ab,c,p)==hrat(a,c,p)*hrat(b,c,p));
                    ADF_CHECK(hrat(b,ac,p)==hrat(b,a,p)*hrat(b,c,p));
                }
            }
        }
    }
    adf_rat_clear(a); adf_rat_clear(b); adf_rat_clear(c); adf_rat_clear(ab);
    adf_rat_clear(ac); adf_rat_clear(neg); adf_rat_clear(one_minus);
}
static ulong random_word(ulong *s)
{
    *s=*s*UWORD(6364136223846793005)+1;
    return *s;
}
ADF_TEST(hilbert_product_2000)
{
    adf_rat_t a,b;
    fmpz_t n,d;
    ulong seed=1;
    unsigned long local_factors=0;
    adf_rat_init(a); adf_rat_init(b); fmpz_init(n); fmpz_init(d);
    for(int k=0;k<2000;k++)
    {
        slong an=(slong)(random_word(&seed)%500+1)*(k%2 ? -1 : 1);
        ulong ad=random_word(&seed)%500+1;
        slong bn=(slong)(random_word(&seed)%500+1)*(k%3 ? -1 : 1);
        ulong bd=random_word(&seed)%500+1;
        fmpz_set_si(n,an); fmpz_set_ui(d,ad); ADF_CHECK(adf_rat_set_fmpz2(a,n,d)==ADF_OK);
        fmpz_set_si(n,bn); fmpz_set_ui(d,bd); ADF_CHECK(adf_rat_set_fmpz2(b,n,d)==ADF_OK);
        int prod=hrat(a,b,adf_place_inf());
        ulong support[]={2,(ulong)labs(an),ad,(ulong)labs(bn),bd};
        int used[501]={0};
        for(int i=0;i<5;i++)
        {
            ulong m=support[i];
            for(ulong q=2;q<=m/q;q++) if(m%q==0)
            {
                used[q]=1;
                do m/=q; while(m%q==0);
            }
            if(m>1) used[m]=1;
        }
        for(ulong q=2;q<=500;q++) if(used[q])
        { prod*=hrat(a,b,place(q)); local_factors++; }
        ADF_CHECK(prod==1);
    }
    printf("product formula: 2000 rational pairs, %lu finite factors\n",local_factors);
    adf_rat_clear(a); adf_rat_clear(b); fmpz_clear(n); fmpz_clear(d);
}
ADF_TEST(hilbert_statuses_and_precision)
{
    adf_rat_t a,b;
    adf_lball_t x,y;
    adf_idele_t i,j;
    arb_t ar,br;
    fmpz_t c,N;
    adf_place_t p2=place(2),p3=place(3),sentinel=place(97),w;
    int z,st;
    adf_rat_init(a); adf_rat_init(b); adf_lball_init(x); adf_lball_init(y);
    adf_idele_init(i); adf_idele_init(j); arb_init(ar); arb_init(br); fmpz_init(c); fmpz_init(N);
    adf_rat_one(a); adf_rat_set_si(b,2);
    ADF_CHECK(adf_lball_set_rat_ball(x,p2,a,2)==ADF_OK);
    ADF_CHECK(adf_lball_set_rat(y,p2,b)==ADF_OK);
    z=99; w=sentinel;
    ADF_CHECK(adf_lball_hilbert(&z,&w,x,y)==ADF_NOT_DETERMINED);
    ADF_CHECK(z==99 && adf_place_equal(w,p2));
    /* Witnesses: 1 and 5 in 1+4 Z_2 against 2 give +1 and -1. */
    ADF_CHECK(hrat(a,b,p2)==1); adf_rat_set_si(a,5); ADF_CHECK(hrat(a,b,p2)==-1);
    adf_rat_one(a); ADF_CHECK(adf_lball_set_rat_ball(x,p2,a,3)==ADF_OK);
    w=sentinel; ADF_CHECK(adf_lball_hilbert(&z,&w,x,y)==ADF_OK && z==1);
    ADF_CHECK(adf_place_equal(w,sentinel));
    ADF_CHECK(adf_lball_set_rat_ball(x,p2,a,1)==ADF_OK);
    ADF_CHECK(adf_lball_set_rat(y,p2,a)==ADF_OK);
    ADF_CHECK(adf_lball_hilbert(&z,NULL,x,y)==ADF_OK && z==1);
    /* Both real signs and every zero/mixed case; scalar outputs must be preserved. */
    arb_set_si(ar,-3); arb_set_si(br,-5); w=sentinel;
    ADF_CHECK(adf_real_hilbert(&z,&w,ar,br)==ADF_OK && z==-1);
    ADF_CHECK(adf_place_equal(w,sentinel));
    arb_zero(ar); z=99; w=sentinel;
    ADF_CHECK(adf_real_hilbert(&z,&w,ar,br)==ADF_DOMAIN);
    ADF_CHECK(z==99 && adf_place_is_archimedean(w));
    arb_add_error_2exp_si(ar,0); w=sentinel;
    ADF_CHECK(adf_real_hilbert(&z,&w,ar,br)==ADF_NOT_DETERMINED);
    ADF_CHECK(z==99 && adf_place_is_archimedean(w));
    arb_zero(br); ADF_CHECK(adf_real_hilbert(&z,&w,ar,br)==ADF_DOMAIN && z==99);
    arb_indeterminate(ar); ADF_CHECK(adf_real_hilbert(&z,&w,ar,br)==ADF_DOMAIN && z==99);
    adf_rat_zero(a); w=sentinel;
    ADF_CHECK(adf_rat_hilbert_at(&z,&w,a,b,p3)==ADF_DOMAIN && z==99);
    ADF_CHECK(adf_place_equal(w,p3));
    ADF_CHECK(adf_lball_set_rat(x,p2,a)==ADF_OK);
    ADF_CHECK(adf_lball_set_rat_ball(y,p2,a,0)==ADF_OK);
    ADF_CHECK(adf_lball_hilbert(&z,&w,x,y)==ADF_DOMAIN && z==99);
    ADF_CHECK(adf_place_equal(w,p2));
    adf_rat_one(a); ADF_CHECK(adf_lball_set_rat(x,p2,a)==ADF_OK);
    ADF_CHECK(adf_lball_hilbert(&z,&w,x,y)==ADF_NOT_DETERMINED && z==99);
    adf_rat_zero(a); ADF_CHECK(adf_lball_set_rat_ball(x,p3,a,1)==ADF_OK);
    /* 3 and 9 in 0+3 Z_3 give -1 and +1 against 2 at 3. */
    adf_rat_set_si(a,9); ADF_CHECK(hrat(a,b,p3)==1);
    adf_rat_set_si(a,3); ADF_CHECK(hrat(a,b,p3)==-1);
    ADF_CHECK(adf_lball_set_rat_ball(x,p3,a,1)==ADF_OK); /* 0+3 Z_3 includes zero */
    ADF_CHECK(adf_lball_set_rat(y,p3,b)==ADF_OK);
    z=99; ADF_CHECK(adf_lball_hilbert(&z,&w,x,y)==ADF_NOT_DETERMINED && z==99);
    ADF_CHECK(adf_place_equal(w,p3));
    ADF_CHECK(adf_lball_set_rat(y,p2,b)==ADF_OK);
    ADF_CHECK(adf_lball_hilbert(&z,&w,x,y)==ADF_DOMAIN && z==99);
    ADF_CHECK(adf_place_equal(w,p2));
    /* Unknown units and scale cofactors. */
    fmpz_one(c); fmpz_one(N);
    ADF_CHECK(adf_ucoset_set_fmpz2(&i->u,c,N)==ADF_OK);
    ADF_CHECK(adf_ucoset_set_fmpz2(&j->u,c,N)==ADF_OK);
    z=99; w=sentinel;
    st=adf_idele_hilbert_at(&z,&w,i,j,p2);
    ADF_CHECK(st==ADF_NOT_DETERMINED && z==99 && adf_place_equal(w,p2));
    w=sentinel;
    ADF_CHECK(adf_idele_hilbert_at(&z,&w,i,j,p3)==ADF_OK && z==1);
    ADF_CHECK(adf_place_equal(w,sentinel));
    fmpq_set_si(i->r,3,1); fmpq_set_si(j->r,3,1);
    adf_ucoset_one(&i->u); adf_ucoset_one(&j->u);
    ADF_CHECK(adf_idele_hilbert_at(&z,NULL,i,j,p2)==ADF_OK && z==-1);
    fmpq_set_si(i->r,1,1); fmpq_set_si(j->r,3,1); fmpz_one(N);
    ADF_CHECK(adf_ucoset_set_fmpz2(&i->u,c,N)==ADF_OK);
    z=99; w=sentinel;
    ADF_CHECK(adf_idele_hilbert_at(&z,&w,i,j,p3)==ADF_NOT_DETERMINED && z==99);
    ADF_CHECK(adf_place_equal(w,p3));
    /* Fixed non-square unit, odd other valuation => -1; opposite order tests the other unit. */
    fmpz_set_ui(c,2); fmpz_set_ui(N,3);
    ADF_CHECK(adf_ucoset_set_fmpz2(&i->u,c,N)==ADF_OK);
    ADF_CHECK(adf_idele_hilbert_at(&z,NULL,i,j,p3)==ADF_OK && z==-1);
    ADF_CHECK(adf_idele_hilbert_at(&z,NULL,j,i,p3)==ADF_OK && z==-1);
    adf_rat_clear(a); adf_rat_clear(b); adf_lball_clear(x); adf_lball_clear(y);
    adf_idele_clear(i); adf_idele_clear(j); arb_clear(ar); arb_clear(br); fmpz_clear(c); fmpz_clear(N);
}

ADF_TEST(hilbert_finite_classes)
{
    jsonl_file *f=NULL;
    jsonl_error_t e;
    adf_rat_t a,b;
    adf_lball_t x,y;
    adf_idele_t i,j;
    fmpz_t c,N;
    adf_place_t sentinel=place(97),w,p;
    int z,st;
    unsigned long local_rows=0;
    ADF_CHECK_MSG(jsonl_open("tests/ref/vectors/f-slice12/hilbert_finite.jsonl",&f,&e),
                  "%s",jsonl_error_message(&e));
    if(!f) return;
    adf_rat_init(a); adf_rat_init(b); adf_lball_init(x); adf_lball_init(y);
    adf_idele_init(i); adf_idele_init(j); fmpz_init(c); fmpz_init(N);
    for(size_t k=0;k<jsonl_count(f);k++)
    {
        const jsonl_value *r=jsonl_record(f,k);
        ulong prime=strtoul(field(r,"p"),NULL,10);
        int av=atoi(field(r,"av")), bv=atoi(field(r,"bv"));
        int au=atoi(field(r,"au")), bu=atoi(field(r,"bu"));
        int ak=atoi(field(r,"ak")), bk=atoi(field(r,"bk"));
        int status=atoi(field(r,"status")),want=atoi(field(r,"value"));
        p=place(prime);
        /* Scales fix valuations; the cosets are multiplicative, not additive hulls. */
        fmpq_set_si(i->r,av ? (slong)prime : 1,1);
        fmpq_set_si(j->r,bv ? (slong)prime : 1,1);
        fmpz_set_ui(c,au); fmpz_set_ui(N,prime); fmpz_pow_ui(N,N,ak);
        ADF_CHECK(adf_ucoset_set_fmpz2(&i->u,c,N)==ADF_OK);
        fmpz_set_ui(c,bu); fmpz_set_ui(N,prime); fmpz_pow_ui(N,N,bk);
        ADF_CHECK(adf_ucoset_set_fmpz2(&j->u,c,N)==ADF_OK);
        z=99; w=sentinel;
        st=adf_idele_hilbert_at(&z,&w,i,j,p);
        ADF_CHECK_MSG(st==status && z==want,"finite row %zu: status %d value %d",k+1,st,z);
        ADF_CHECK(adf_place_equal(w,st==ADF_OK ? sentinel : p));
        if(ak && bk)
        {
            adf_rat_set_si(a,(av ? (slong)prime : 1)*au);
            adf_rat_set_si(b,(bv ? (slong)prime : 1)*bu);
            ADF_CHECK(adf_lball_set_rat_ball(x,p,a,av+ak)==ADF_OK);
            ADF_CHECK(adf_lball_set_rat_ball(y,p,b,bv+bk)==ADF_OK);
            z=99; w=sentinel;
            st=adf_lball_hilbert(&z,&w,x,y);
            ADF_CHECK(st==status && z==want);
            ADF_CHECK(adf_place_equal(w,st==ADF_OK ? sentinel : p));
            local_rows++;
        }
    }
    printf("finite Hilbert rows: %zu idele, %lu local\n",jsonl_count(f),local_rows);
    jsonl_close(f);
    adf_rat_clear(a); adf_rat_clear(b); adf_lball_clear(x); adf_lball_clear(y);
    adf_idele_clear(i); adf_idele_clear(j); fmpz_clear(c); fmpz_clear(N);
}

ADF_TEST(hilbert_word_primes_huge_and_extreme)
{
    ulong ps[]={2,3,5,7,65537,UWORD(18446744073709551557)};
    fmpz_t n,d,t,pz;
    adf_rat_t a,b;
    adf_lball_t x,y;
    int z;
    fmpz_init(n); fmpz_init(d); fmpz_init(t); fmpz_init(pz);
    adf_rat_init(a); adf_rat_init(b); adf_lball_init(x); adf_lball_init(y);
    for(size_t k=0;k<6;k++)
    {
        adf_place_t p=place(ps[k]);
        fmpz_set_ui(pz,ps[k]);
        /* A 4097-bit square rational, divided by a square denominator: +1 at every place. */
        fmpz_one(n); fmpz_mul_2exp(n,n,2048); fmpz_add_ui(n,n,17); fmpz_mul(n,n,n);
        fmpz_one(d); fmpz_mul_2exp(d,d,1024); fmpz_add_ui(d,d,3); fmpz_mul(d,d,d);
        ADF_CHECK(adf_rat_set_fmpz2(a,n,d)==ADF_OK);
        adf_rat_set_si(b,-3);
        ADF_CHECK(hrat(a,b,p)==1);
        ADF_CHECK(adf_lball_set_rat(x,p,a)==ADF_OK);
        ADF_CHECK(adf_lball_set_rat(y,p,b)==ADF_OK);
        ADF_CHECK(adf_lball_hilbert(&z,NULL,x,y)==ADF_OK && z==1);
        if(ps[k]!=2)
        {
            /* Reduction mod p forces u to be a square for (p,u); Euler supplies that sign. */
            adf_rat_set_fmpz(a,pz);
            for(ulong u=1;u<=17;u++)
            {
                int want;
                if(u%ps[k]==0) continue;
                adf_rat_set_si(b,(slong)u);
                fmpz_set_ui(n,u); fmpz_sub_ui(t,pz,1); fmpz_fdiv_q_2exp(t,t,1);
                fmpz_powm(n,n,t,pz);
                want=fmpz_is_one(n) ? 1 : -1;
                ADF_CHECK(hrat(a,b,p)==want);
            }
        }
        /* Compact local inputs never require a power of p, even at the slong endpoints. */
        x->p=ps[k]; x->exact=1; x->N=0; x->v=WORD_MIN; fmpq_one(x->u);
        y->p=ps[k]; y->exact=1; y->N=0; y->v=WORD_MAX; fmpq_one(y->u);
        ADF_CHECK(adf_lball_is_canonical(x) && adf_lball_is_canonical(y));
        ADF_CHECK(adf_lball_hilbert(&z,NULL,x,y)==ADF_OK && z==1);
        x->exact=0; x->v=WORD_MAX-1; x->N=WORD_MAX;
        if(ps[k]==2)
        {
            ADF_CHECK(adf_lball_hilbert(&z,NULL,x,y)==ADF_NOT_DETERMINED);
            x->v=WORD_MAX-3; x->N=WORD_MAX;
            ADF_CHECK(adf_lball_hilbert(&z,NULL,x,y)==ADF_OK && z==1);
            fmpq_set_si(x->u,5,1);
            ADF_CHECK(adf_lball_hilbert(&z,NULL,x,y)==ADF_OK && z==-1);
            fmpq_one(x->u);
            x->v=WORD_MIN; x->N=WORD_MAX;
            ADF_CHECK(adf_lball_hilbert(&z,NULL,x,y)==ADF_OK && z==1);
        }
    }
    fmpz_clear(n); fmpz_clear(d); fmpz_clear(t); fmpz_clear(pz);
    adf_rat_clear(a); adf_rat_clear(b); adf_lball_clear(x); adf_lball_clear(y);
}

#ifdef ADF_CHECK_INVARIANTS
ADF_TEST(hilbert_debug_entry_checks)
{
    const char *names[]={"adf_lball_hilbert","adf_rat_hilbert_at","adf_idele_hilbert_at"};
    for(int type=0;type<3;type++) for(int arg=0;arg<2;arg++)
    {
        int fds[2];
        ADF_CHECK(pipe(fds)==0);
        pid_t pid=fork();
        ADF_CHECK(pid>=0);
        if(pid==0)
        {
            struct rlimit lim={0,0};
            adf_rat_t a,b;
            adf_lball_t x,y;
            adf_idele_t i,j;
            adf_place_t p;
            int z;
            close(fds[0]); dup2(fds[1],STDERR_FILENO); close(fds[1]);
            setrlimit(RLIMIT_CORE,&lim);
            adf_rat_init(a); adf_rat_init(b); adf_lball_init(x); adf_lball_init(y);
            adf_idele_init(i); adf_idele_init(j); adf_place_prime(&p,2);
            if(type==0)
            {
                (arg ? y : x)->exact=2;
                adf_lball_hilbert(&z,NULL,x,y);
            }
            else if(type==1)
            {
                fmpz_zero(fmpq_denref((arg ? b : a)->q));
                adf_rat_hilbert_at(&z,NULL,a,b,p);
            }
            else
            {
                fmpq_zero((arg ? j : i)->r);
                adf_idele_hilbert_at(&z,NULL,i,j,p);
            }
            adf_rat_clear(a); adf_rat_clear(b); adf_lball_clear(x); adf_lball_clear(y);
            adf_idele_clear(i); adf_idele_clear(j);
            _exit(0);
        }
        close(fds[1]);
        if(pid>0)
        {
            int st;
            char msg[512];
            ssize_t n=read(fds[0],msg,sizeof(msg)-1);
            if(n>=0) msg[n]=0; else msg[0]=0;
            ADF_CHECK(waitpid(pid,&st,0)==pid);
            ADF_CHECK(WIFSIGNALED(st) && WTERMSIG(st)==SIGABRT);
            ADF_CHECK(strstr(msg,names[type])!=NULL);
            ADF_CHECK(strstr(msg,arg ? "argument b" : "argument a")!=NULL);
        }
        close(fds[0]);
    }
}
#endif

ADF_TEST(hilbert_input_aliases)
{
    adf_rat_t a;
    adf_lball_t x;
    adf_idele_t i;
    arb_t ar;
    adf_place_t p=place(2),w=place(97);
    int z=99;
    adf_rat_init(a); adf_lball_init(x); adf_idele_init(i); arb_init(ar);
    adf_rat_set_si(a,3);
    ADF_CHECK(adf_rat_hilbert_at(&z,NULL,a,a,p)==ADF_OK && z==-1);
    ADF_CHECK(adf_lball_set_rat(x,p,a)==ADF_OK);
    ADF_CHECK(adf_lball_hilbert(&z,NULL,x,x)==ADF_OK && z==-1);
    ADF_CHECK(adf_idele_set_rat(i,a,64)==ADF_OK);
    ADF_CHECK(adf_idele_hilbert_at(&z,NULL,i,i,p)==ADF_OK && z==-1);
    arb_set_si(ar,-3);
    ADF_CHECK(adf_real_hilbert(&z,NULL,ar,ar)==ADF_OK && z==-1);
    adf_rat_zero(a); z=99;
    ADF_CHECK(adf_rat_hilbert_at(&z,&w,a,a,adf_place_inf())==ADF_DOMAIN && z==99);
    ADF_CHECK(adf_place_is_archimedean(w));
    adf_rat_clear(a); adf_lball_clear(x); adf_idele_clear(i); arb_clear(ar);
}
