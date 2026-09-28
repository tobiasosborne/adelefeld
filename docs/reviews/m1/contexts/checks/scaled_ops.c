/* scaled_ops.c: random and extreme inputs for every operation of src/scaled.c. The program
   only runs the library and prints inputs and outputs; scaled_oracle.py checks them with
   exact arithmetic (fractions.Fraction), independently of the code under review.
   Also checked here, in C, against the non-aliased call on copies: every permitted aliasing
   ((x,x,y), (y,x,y), (x,x,x), unary y = x) gives the identical value and the same lost flag,
   and the shared-pointer rule: two contexts of equal modulus at different pointers give
   ADF_DOMAIN with the output bitwise untouched, also when the output aliases an input.

   usage: scaled_ops SEED ROUNDS  > out.txt ; python3 scaled_oracle.py out.txt
   Build: cc -std=c11 -O1 -g -Iinclude scaled_ops.c build/libadelefeld.a -lflint -lgmp -lm */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <adelefeld.h>
#include <flint/ulong_extras.h>

static flint_rand_t R;
static long alias_bad = 0, domain_bad = 0, local_bad = 0, local_cases = 0;

/* ------------------------------------------------------------ printing */
static void pz(const char * key, const fmpz_t z)
{
    char * s = fmpz_get_str(NULL, 10, z);
    printf(" %s=%s", key, s);
    flint_free(s);
}
static void pq(const char * key, const fmpq_t q)
{
    char * s = fmpq_get_str(NULL, 10, q);
    printf(" %s=%s", key, s);
    flint_free(s);
}
static void psc(const char * key, const adf_scaled_t x)
{
    char * s = fmpq_get_str(NULL, 10, x->s);
    char * u = fmpz_get_str(NULL, 10, x->u);
    printf(" %s=%d,%s,%s", key, x->exact, s, u);
    flint_free(s);
    flint_free(u);
}
static void pball(const char * key, const adf_fball_t b)
{
    fmpz_t A, H, d;
    char *sa, *sh, *sd;
    fmpz_init(A); fmpz_init(H); fmpz_init(d);
    adf_fball_get_fmpz3(A, H, d, b);
    sa = fmpz_get_str(NULL, 10, A); sh = fmpz_get_str(NULL, 10, H); sd = fmpz_get_str(NULL, 10, d);
    printf(" %s=%s,%s,%s", key, sa, sh, sd);
    flint_free(sa); flint_free(sh); flint_free(sd);
    fmpz_clear(A); fmpz_clear(H); fmpz_clear(d);
}

/* ------------------------------------------------------------ random data */
static const ulong small_primes[] = {2, 3, 5, 7, 11, 13};

/* a random positive integer of mixed shape; big = 1 allows hundreds of bits */
static void rand_pos(fmpz_t z, int big)
{
    ulong c = n_randint(R, 6);
    if (c == 0)
        fmpz_one(z);
    else if (c == 1)
        fmpz_set_ui(z, 1 + n_randint(R, 12));
    else if (c == 2)
    {   /* a product of small prime powers */
        int i;
        fmpz_one(z);
        for (i = 0; i < 3; i++)
        {
            ulong p = small_primes[n_randint(R, 6)];
            fmpz_mul_ui(z, z, n_pow(p, (ulong) n_randint(R, 4)));
        }
    }
    else if (c == 3)
        fmpz_set_ui(z, 1 + n_randint(R, 1000));
    else
    {
        fmpz_randbits(z, R, big ? 1 + n_randint(R, 300) : 1 + n_randint(R, 40));
        fmpz_abs(z, z);
        if (fmpz_is_zero(z))
            fmpz_one(z);
    }
}
static void rand_rat(fmpq_t q, int big, int allow_zero, int allow_neg)
{
    fmpz_t n, d;
    fmpz_init(n); fmpz_init(d);
    rand_pos(n, big);
    rand_pos(d, big);
    if (allow_zero && n_randint(R, 8) == 0)
        fmpz_zero(n);
    if (allow_neg && n_randint(R, 2))
        fmpz_neg(n, n);
    fmpq_set_fmpz_frac(q, n, d);
    fmpz_clear(n); fmpz_clear(d);
}
/* a random admitted scaled value of ctx: exact with prob 1/5, else s > 0 and 0 <= u < K */
static void rand_scaled(adf_scaled_t x, const adf_modctx_struct * ctx, int big)
{
    fmpz_t K;
    fmpz_init(K);
    adf_modctx_get_modulus(K, ctx);
    x->mctx = ctx;
    if (n_randint(R, 5) == 0)
    {
        x->exact = 1;
        rand_rat(x->s, big, 1, 1);
        fmpz_zero(x->u);
    }
    else
    {
        x->exact = 0;
        rand_rat(x->s, big, 0, 0);
        switch (n_randint(R, 4))
        {
        case 0: fmpz_zero(x->u); break;
        case 1: fmpz_sub_ui(x->u, K, 1); break;
        default: fmpz_randm(x->u, R, K); break;
        }
    }
    fmpz_clear(K);
}

/* ------------------------------------------------------------ helpers */
static int bitwise_same(const adf_scaled_struct * a, const adf_scaled_struct * b)
{
    /* the fields byte by byte (the 4 padding bytes after exact are not compared) */
    return memcmp(a->s, b->s, sizeof(fmpq)) == 0 && memcmp(a->u, b->u, sizeof(fmpz)) == 0 &&
           a->mctx == b->mctx && a->exact == b->exact;
}

typedef int (*binop)(adf_scaled_t, const adf_scaled_t, const adf_scaled_t);

static void run_binop(const char * name, binop f, const adf_scaled_t x, const adf_scaled_t y,
                      const adf_modctx_struct * other)
{
    adf_scaled_t z, a, b, zz;
    int st;
    adf_scaled_init(z, x->mctx);
    adf_scaled_init(a, x->mctx);
    adf_scaled_init(b, x->mctx);
    adf_scaled_init(zz, x->mctx);
    st = f(z, x, y);
    printf("%s", name);
    { fmpz_t K; fmpz_init(K); adf_modctx_get_modulus(K, x->mctx); pz("K", K); fmpz_clear(K); }
    psc("X", x); psc("Y", y); psc("Z", z);
    printf(" ST=%d\n", st);
    /* (x, x, y) */
    adf_scaled_set(a, x);
    f(a, a, y);
    if (!adf_scaled_identical(a, z)) { alias_bad++; printf("ALIASBAD %s (x,x,y)\n", name); }
    /* (y, x, y) */
    adf_scaled_set(b, y);
    f(b, x, b);
    if (!adf_scaled_identical(b, z)) { alias_bad++; printf("ALIASBAD %s (y,x,y)\n", name); }
    /* (x, x, x) against the fresh f(zz, x, x) */
    f(zz, x, x);
    adf_scaled_set(a, x);
    f(a, a, a);
    if (!adf_scaled_identical(a, zz)) { alias_bad++; printf("ALIASBAD %s (x,x,x)\n", name); }
    printf("%s", name);
    { fmpz_t K; fmpz_init(K); adf_modctx_get_modulus(K, x->mctx); pz("K", K); fmpz_clear(K); }
    psc("X", x); psc("Y", x); psc("Z", zz);
    printf(" ST=0\n");
    /* shared-pointer rule: y moved to another context of the same modulus */
    if (other != NULL)
    {
        adf_scaled_struct save;
        adf_scaled_set(b, y);
        b->mctx = other;
        adf_scaled_set(a, x);
        save = *a;
        if (f(a, a, b) != ADF_DOMAIN || !bitwise_same(a, &save) || a->mctx != x->mctx)
        { domain_bad++; printf("DOMAINBAD %s (x,x,y')\n", name); }
        save = *b;
        if (f(b, x, b) != ADF_DOMAIN || !bitwise_same(b, &save))
        { domain_bad++; printf("DOMAINBAD %s (y',x,y')\n", name); }
        adf_scaled_set(zz, z);
        save = *zz;
        if (f(zz, x, b) != ADF_DOMAIN || !bitwise_same(zz, &save))
        { domain_bad++; printf("DOMAINBAD %s (z,x,y')\n", name); }
    }
    adf_scaled_clear(z); adf_scaled_clear(a); adf_scaled_clear(b); adf_scaled_clear(zz);
}

/* ------------------------------------------------------------ main */
int main(int argc, char ** argv)
{
    ulong seed = argc > 1 ? strtoul(argv[1], NULL, 10) : 1;
    long rounds = argc > 2 ? strtol(argv[2], NULL, 10) : 200;
    /* moduli: the extreme and the ordinary; each built twice (two pointers, equal modulus) */
    const char * Ks[] = {"1", "2", "3", "4", "6", "12", "30", "36", "60", "64", "210", "360", "1000",
                         "7", "97", "65536", "18446744073709551615", "18446744073709551616",
                         "18446744073709551617", "340282366920938463463374607431768211456",
                         "1606938044258990275541962092341162602522202993782792835301376",
                         "0x-big"};
    int nK = (int) (sizeof(Ks) / sizeof(Ks[0]));
    adf_modctx_struct * ctx[32], * ctx2[32];
    long r;
    int i;

    flint_randinit(R);
    {   /* flint_randinit is deterministic; advance by the seed */
        ulong j;
        for (j = 0; j < seed; j++) n_randint(R, 1000);
    }
    for (i = 0; i < nK; i++)
    {
        fmpz_t K;
        fmpz_init(K);
        if (strcmp(Ks[i], "0x-big") == 0)
        {   /* a 4096-bit modulus: 2^4000 * 3^60, no blocks */
            fmpz_ui_pow_ui(K, 2, 4000);
            { fmpz_t t; fmpz_init(t); fmpz_ui_pow_ui(t, 3, 60); fmpz_mul(K, K, t); fmpz_clear(t); }
        }
        else
            fmpz_set_str(K, Ks[i], 10);
        if (adf_modctx_new_fmpz(&ctx[i], K) != ADF_OK || adf_modctx_new_fmpz(&ctx2[i], K) != ADF_OK)
        { printf("ctx failed\n"); return 2; }
        fmpz_clear(K);
    }
    /* a context with blocks of several words, for the local path */
    {
        ulong q[4] = {4, 9, 25, 7};
        if (adf_modctx_new_blocks(&ctx[nK], q, 4) != ADF_OK) return 2;
        ctx2[nK] = NULL;
        {
            ulong q2[1] = {12};
            if (adf_modctx_new_blocks(&ctx2[nK], q2, 1) != ADF_OK) return 2;
        }
    }

    for (r = 0; r < rounds; r++)
    {
        int ki = (int) n_randint(R, (ulong) nK), kj = (int) n_randint(R, (ulong) nK);
        int big = ki >= 16 || n_randint(R, 4) == 0;
        adf_modctx_struct * c = ctx[ki];
        adf_scaled_t x, y, w;
        adf_rat_t q;
        adf_fball_t b, g;
        int lost, lost2;

        adf_scaled_init(x, c); adf_scaled_init(y, c); adf_scaled_init(w, c);
        adf_rat_init(q);
        adf_fball_init(b); adf_fball_init(g);
        rand_scaled(x, c, big);
        rand_scaled(y, c, big);
        if (n_randint(R, 6) == 0) adf_scaled_set(y, x);
        if (!adf_scaled_is_canonical(x) || !adf_scaled_is_canonical(y)) { printf("gen not canonical\n"); return 2; }

        run_binop("add", adf_scaled_add, x, y, ctx2[ki]);
        run_binop("sub", adf_scaled_sub, x, y, ctx2[ki]);
        run_binop("mul", adf_scaled_mul, x, y, ctx2[ki]);
        run_binop("mul_tight", adf_scaled_mul_tight, x, y, ctx2[ki]);

        /* neg */
        adf_scaled_neg(w, x);
        printf("neg"); { fmpz_t K; fmpz_init(K); adf_modctx_get_modulus(K, c); pz("K", K); fmpz_clear(K); }
        psc("X", x); psc("Z", w); printf("\n");
        adf_scaled_set(y, x); adf_scaled_neg(y, y);
        if (!adf_scaled_identical(y, w)) { alias_bad++; printf("ALIASBAD neg\n"); }

        /* mul_rat, add_rat */
        rand_rat(q->q, big, 1, 1);
        adf_scaled_mul_rat(w, x, q);
        printf("mul_rat"); { fmpz_t K; fmpz_init(K); adf_modctx_get_modulus(K, c); pz("K", K); fmpz_clear(K); }
        psc("X", x); pq("Q", q->q); psc("Z", w); printf("\n");
        adf_scaled_set(y, x); adf_scaled_mul_rat(y, y, q);
        if (!adf_scaled_identical(y, w)) { alias_bad++; printf("ALIASBAD mul_rat\n"); }

        lost = -1;
        adf_scaled_add_rat(w, &lost, x, q);
        printf("add_rat"); { fmpz_t K; fmpz_init(K); adf_modctx_get_modulus(K, c); pz("K", K); fmpz_clear(K); }
        psc("X", x); pq("Q", q->q); psc("Z", w); printf(" L=%d\n", lost);
        lost2 = -1;
        adf_scaled_set(y, x); adf_scaled_add_rat(y, &lost2, y, q);
        if (!adf_scaled_identical(y, w) || lost2 != lost) { alias_bad++; printf("ALIASBAD add_rat\n"); }

        /* get_fball */
        adf_scaled_get_fball(b, x);
        printf("get_fball"); { fmpz_t K; fmpz_init(K); adf_modctx_get_modulus(K, c); pz("K", K); fmpz_clear(K); }
        psc("X", x); pball("B", b); printf("\n");

        /* set_context to another modulus kj, and to lcm-like targets */
        {
            adf_modctx_struct * t = ctx[kj];
            fmpz_t K, K2;
            fmpz_init(K); fmpz_init(K2);
            adf_modctx_get_modulus(K, c);
            adf_modctx_get_modulus(K2, t);
            lost = -1;
            adf_scaled_set_context(w, &lost, x, t);
            printf("set_context"); pz("K", K); pz("K2", K2); psc("X", x); psc("Z", w); printf(" L=%d\n", lost);
            /* aliased: y = x */
            lost2 = -1;
            adf_scaled_set(y, x);
            adf_scaled_set_context(y, &lost2, y, t);
            printf("set_context_alias"); pz("K", K); pz("K2", K2); psc("X", x); psc("Z", y); printf(" L=%d\n", lost2);
            fmpz_clear(K); fmpz_clear(K2);
        }

        /* set_fball from a random tight ball c + R Zhat (R = 0 with prob 1/5) */
        {
            adf_rat_t cc, rr;
            fmpz_t K;
            adf_rat_init(cc); adf_rat_init(rr);
            fmpz_init(K);
            rand_rat(cc->q, big, 1, 1);
            if (n_randint(R, 5) == 0) fmpq_zero(rr->q); else rand_rat(rr->q, big, 0, 0);
            if (n_randint(R, 4) == 0)
            {   /* a radius that is a multiple of K: often lossless */
                adf_modctx_get_modulus(K, c);
                fmpq_mul_fmpz(rr->q, rr->q, K);
            }
            adf_fball_set_center_radius(g, cc, rr);
            lost = -1;
            adf_scaled_set_fball(w, &lost, g, c);
            adf_modctx_get_modulus(K, c);
            printf("set_fball"); pz("K", K); pball("B", g); psc("Z", w); printf(" L=%d\n", lost);
            adf_rat_clear(cc); adf_rat_clear(rr);
            fmpz_clear(K);
        }

        adf_scaled_clear(x); adf_scaled_clear(y); adf_scaled_clear(w);
        adf_rat_clear(q);
        adf_fball_clear(b); adf_fball_clear(g);
    }

    /* the local path of adf_scaled_set_fball: a local value and its global copy give the
       identical scaled value and the same lost flag, in every target context */
    {
        adf_modctx_struct * lc = ctx[nK];
        long t;
        for (t = 0; t < 2000; t++)
        {
            adf_fball_t gb, lb;
            adf_rat_t cc, rr;
            fmpz_t K;
            int j, st;
            adf_fball_init(gb); adf_fball_init(lb);
            adf_rat_init(cc); adf_rat_init(rr);
            fmpz_init(K);
            adf_modctx_get_modulus(K, lc);
            {   /* (A0 + K Zhat)/d */
                fmpz_t A0, d;
                fmpz_init(A0); fmpz_init(d);
                fmpz_randm(A0, R, K);
                fmpz_set_ui(d, 1 + n_randint(R, 50));
                fmpq_set_fmpz_frac(cc->q, A0, d);
                fmpq_set_fmpz_frac(rr->q, K, d);
                fmpz_clear(A0); fmpz_clear(d);
            }
            adf_fball_set_center_radius(gb, cc, rr);
            st = adf_fball_set_local(lb, gb, lc);
            if (st != ADF_OK) { printf("set_local status %d\n", st); local_bad++; }
            else
                for (j = 0; j <= nK; j++)
                {
                    adf_modctx_struct * tc = j < nK ? ctx[j] : ctx2[nK];
                    adf_scaled_t s1, s2;
                    int l1 = -1, l2 = -2;
                    adf_scaled_init(s1, tc); adf_scaled_init(s2, tc);
                    adf_scaled_set_fball(s1, &l1, gb, tc);
                    adf_scaled_set_fball(s2, &l2, lb, tc);
                    local_cases++;
                    if (!adf_scaled_identical(s1, s2) || l1 != l2) { local_bad++; printf("LOCALBAD\n"); }
                    if (t < 200)
                    {
                        printf("set_fball"); fmpz_t K2; fmpz_init(K2); adf_modctx_get_modulus(K2, tc);
                        pz("K", K2); pball("B", lb); psc("Z", s2); printf(" L=%d\n", l2);
                        fmpz_clear(K2);
                    }
                    adf_scaled_clear(s1); adf_scaled_clear(s2);
                }
            adf_fball_clear(gb); adf_fball_clear(lb);
            adf_rat_clear(cc); adf_rat_clear(rr);
            fmpz_clear(K);
        }
    }

    printf("SUMMARY alias_bad=%ld domain_bad=%ld local_cases=%ld local_bad=%ld\n",
           alias_bad, domain_bad, local_cases, local_bad);
    for (i = 0; i <= nK; i++) { adf_modctx_free(ctx[i]); adf_modctx_free(ctx2[i]); }
    flint_randclear(R);
    flint_cleanup();
    return 0;
}
