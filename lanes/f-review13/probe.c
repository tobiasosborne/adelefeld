/* f-review13 probe: prints raw library results for lines on stdin.
   at  p num den c M N            -> adf_idele_Log_at at the prime p, real coordinate 2
   ref s num den c M N k p1..pk   -> adf_idele_Log_refine, real coordinate s (an integer), prec 64
   Output per line: status, then fields. where is printed as inf, a prime, or "untouched". */
#include <stdio.h>
#include <string.h>
#include <adelefeld.h>

static void mk(adf_idele_t x, slong s, const char *num, const char *den, const char *c, const char *M)
{
    arb_set_si(x->inf, s);
    fmpz_set_str(fmpq_numref(x->r), num, 10);
    fmpz_set_str(fmpq_denref(x->r), den, 10);
    fmpq_canonicalise(x->r);
    fmpz_set_str(x->u.c, c, 10);
    if (strchr(M, '^'))
    {
        /* M given as B^E */
        unsigned long base = 0, e = 0;
        sscanf(M, "%lu^%lu", &base, &e);
        fmpz_set_ui(x->u.N, base); fmpz_pow_ui(x->u.N, x->u.N, e);
    }
    else
        fmpz_set_str(x->u.N, M, 10);
    if (!adf_idele_is_canonical(x)) { printf("NONCANONICAL\n"); }
}

static void pw(adf_place_t w, adf_place_t sentinel)
{
    if (adf_place_equal(w, sentinel)) printf(" where=untouched");
    else if (adf_place_is_archimedean(w)) printf(" where=inf");
    else printf(" where=%lu", adf_place_prime_get(w));
}

int main(void)
{
    char line[1 << 16];
    adf_idele_t x;
    adf_sball_t s;
    adf_adele_t y;
    adf_place_t sentinel;
    adf_place_prime(&sentinel, 1000003);
    adf_idele_init(x); adf_sball_init(s); adf_adele_init(y);
    while (fgets(line, sizeof line, stdin))
    {
        char op[8], num[4096], den[4096], c[4096], M[4096];
        long N, sg, k;
        int off = 0, st;
        adf_place_t w = sentinel, v;
        if (sscanf(line, "%7s%n", op, &off) != 1) continue;
        if (strcmp(op, "at") == 0)
        {
            unsigned long pu = 0;
            sscanf(line + off, "%lu %4095s %4095s %4095s %4095s %ld", &pu, num, den, c, M, &N);
            mk(x, 2, num, den, c, M);
            if (adf_place_prime(&v, pu) != ADF_OK) { printf("BADPRIME\n"); continue; }
            st = adf_idele_Log_at(s, &w, x, v, N);
            printf("st=%d", st);
            pw(w, sentinel);
            if (st == ADF_OK)
            {
                adf_lball_struct *l = &s->loc[0];
                printf(" exact=%d v=%ld N=%ld u=", l->exact, (long) l->v, (long) l->N);
                fmpq_print(l->u);
            }
            printf("\n");
        }
        else if (strcmp(op, "atinf") == 0)
        {
            /* atinf s: Log_at and log_abs_at at infinity on (s ; 4 * [1 mod 9]) */
            sscanf(line + off, "%ld", &sg);
            mk(x, sg, "4", "1", "1", "9");
            st = adf_idele_Log_at(s, &w, x, adf_place_inf(), 64);
            printf("Log_at st=%d", st); pw(w, sentinel);
            w = sentinel;
            st = adf_idele_log_abs_at(s, &w, x, adf_place_inf(), 64);
            printf(" log_abs_at st=%d", st); pw(w, sentinel);
            w = sentinel;
            st = adf_idele_Log(y, &w, x, 64);
            printf(" Log st=%d", st); pw(w, sentinel);
            w = sentinel;
            st = adf_idele_log_abs(y, &w, x, 64);
            printf(" log_abs st=%d", st); pw(w, sentinel);
            printf("\n");
        }
        else if (strcmp(op, "ref") == 0 || strcmp(op, "refq") == 0)
        {
            int o2 = 0;
            long prec = 64;
            adf_place_t ps[64];
            adf_adele_t before;
            if (op[3] == 'q')
                sscanf(line + off, "%ld %4095s %4095s %4095s %4095s %ld %ld %ld%n", &sg, num, den, c, M, &N,
                       &prec, &k, &o2);
            else
                sscanf(line + off, "%ld %4095s %4095s %4095s %4095s %ld %ld%n", &sg, num, den, c, M, &N, &k, &o2);
            off += o2;
            /* sentinel output value (5 ; 3 mod 11), compared after a failure */
            arb_set_si(y->inf, 5); fmpz_set_ui(y->fin.A, 3); fmpz_set_ui(y->fin.H, 11); fmpz_one(y->fin.d);
            adf_adele_init(before); adf_adele_set(before, y);
            for (long i = 0; i < k && k <= 64; i++)
            {
                int o3 = 0;
                unsigned long pu = 0;
                sscanf(line + off, "%lu%n", &pu, &o3); off += o3;
                if (pu == 0) ps[i] = adf_place_inf();
                else if (adf_place_prime(&ps[i], pu) != ADF_OK) { printf("BADPRIME "); ps[i] = adf_place_inf(); }
            }
            mk(x, sg, num, den, c, M);
            st = adf_idele_Log_refine(y, &w, x, (k > 0 && k <= 64) ? ps : NULL, k, N, prec);
            printf("st=%d", st);
            pw(w, sentinel);
            if (st == ADF_OK && fmpz_bits(y->fin.H) > 4096)
            {
                printf(" Hbits=%lu Abits=%lu A_mod_4=%lu d=", (ulong) fmpz_bits(y->fin.H),
                       (ulong) fmpz_bits(y->fin.A), fmpz_fdiv_ui(y->fin.A, 4));
                fmpz_print(y->fin.d);
            }
            else if (st == ADF_OK)
            {
                printf(" A="); fmpz_print(y->fin.A);
                printf(" H="); fmpz_print(y->fin.H);
                printf(" d="); fmpz_print(y->fin.d);
            }
            else
                printf(" y=%s", adf_adele_identical(y, before) ? "kept" : "CHANGED");
            adf_adele_clear(before);
            printf("\n");
        }
        fflush(stdout);
    }
    adf_idele_clear(x); adf_sball_clear(s); adf_adele_clear(y);
    flint_cleanup_master();
    return 0;
}
