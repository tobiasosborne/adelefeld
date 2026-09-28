/* ctor_cases.c: the raw context constructors on ordinary and hostile arguments. Prints one
   line per call: the call, the status, and on ADF_OK the modulus and the blocks; ctor_oracle.py
   checks each line against its own computation (prime powers, n!, primorials, statuses of
   modctx.h and conventions 5.14 / 3.2). The C side checks that *out is untouched on failure
   (a sentinel pointer) and the build under -fsanitize=address reports leaks.
   Build: cc -std=c11 -O0 -g -fsanitize=address,undefined -Iinclude ctor_cases.c src/*.c -lflint -lgmp -lm */
#include <stdio.h>
#include <string.h>
#include <adelefeld.h>
#include <flint/ulong_extras.h>

static adf_modctx_struct * const SENT = (adf_modctx_struct *) 0x1234;
static long untouched_bad = 0;

static void report(const char * call, int st, adf_modctx_struct * ctx)
{
    printf("%s st=%d", call, st);
    if (st == ADF_OK)
    {
        fmpz_t K;
        slong i, k = adf_modctx_nblocks(ctx);
        char * s;
        fmpz_init(K);
        adf_modctx_get_modulus(K, ctx);
        s = fmpz_get_str(NULL, 10, K);
        printf(" K=%s q=", s);
        flint_free(s);
        for (i = 0; i < k; i++)
            printf("%s%lu", i ? "," : "", adf_modctx_block(ctx, i));
        fmpz_clear(K);
        adf_modctx_free(ctx);
    }
    else if (ctx != SENT)
    {
        untouched_bad++;
        printf(" OUT-TOUCHED");
    }
    printf("\n");
}

int main(void)
{
    char call[256];
    adf_modctx_struct * c;
    int st;
    ulong top = UWORD_MAX - 58;  /* 2^64 - 59, the largest prime below 2^64 */

    /* new_blocks */
    {
        struct { const char * name; ulong q[4]; slong k; } B[] = {
            {"blocks(-1)", {2}, -1}, {"blocks()", {0}, 0}, {"blocks(0)", {0}, 1},
            {"blocks(1)", {1}, 1}, {"blocks(2)", {2}, 1}, {"blocks(max)", {UWORD_MAX}, 1},
            {"blocks(max,max-1)", {UWORD_MAX, UWORD_MAX - 1}, 2},
            {"blocks(max,max-2)", {UWORD_MAX, UWORD_MAX - 2}, 2},
            {"blocks(2,2)", {2, 2}, 2}, {"blocks(6,35,143)", {6, 35, 143}, 3},
            {"blocks(4,9,25,49)", {4, 9, 25, 49}, 4}, {"blocks(6,10)", {6, 10}, 2},
            {"blocks(3,1)", {3, 1}, 2}, {"blocks(3,0)", {3, 0}, 2}};
        size_t i;
        for (i = 0; i < sizeof(B) / sizeof(B[0]); i++)
        {
            c = SENT;
            st = adf_modctx_new_blocks(&c, B[i].k == 0 ? NULL : B[i].q, B[i].k);
            report(B[i].name, st, c);
        }
        c = SENT;
        st = adf_modctx_new_blocks(&c, NULL, 3);
        report("blocks(NULL,3)", st, c);
    }
    /* new_prime_powers */
    {
        struct { const char * name; ulong p[3]; ulong e[3]; slong k; } P[] = {
            {"pp(-1)", {2}, {1}, -1}, {"pp()", {0}, {0}, 0},
            {"pp(0^1)", {0}, {1}, 1}, {"pp(1^1)", {1}, {1}, 1}, {"pp(4^1)", {4}, {1}, 1},
            {"pp(2^0)", {2}, {0}, 1}, {"pp(2^63)", {2}, {63}, 1}, {"pp(2^64)", {2}, {64}, 1},
            {"pp(2^maxe)", {2}, {UWORD_MAX}, 1}, {"pp(3^40)", {3}, {40}, 1},
            {"pp(3^41)", {3}, {41}, 1}, {"pp(top^1)", {0}, {1}, 1}, {"pp(top^2)", {0}, {2}, 1},
            {"pp(2^1,2^2)", {2, 2}, {1, 2}, 2}, {"pp(2^64,4^1)", {2, 4}, {64, 1}, 2},
            {"pp(4^1,2^64)", {4, 2}, {1, 64}, 2}, {"pp(2^64,3^0)", {2, 3}, {64, 0}, 2},
            {"pp(5^2,3^1,2^3)", {5, 3, 2}, {2, 1, 3}, 3},
            {"pp(maxodd^1)", {UWORD_MAX}, {1}, 1}};
        size_t i;
        for (i = 0; i < sizeof(P) / sizeof(P[0]); i++)
        {
            if (strncmp(P[i].name, "pp(top", 6) == 0)
                P[i].p[0] = top;
            c = SENT;
            st = adf_modctx_new_prime_powers(&c, P[i].p, P[i].e, P[i].k);
            if (strncmp(P[i].name, "pp(top", 6) == 0)
                snprintf(call, sizeof call, "pp(%lu^%lu)", top, P[i].e[0]);
            else
                snprintf(call, sizeof call, "%s", P[i].name);
            report(call, st, c);
        }
    }
    /* new_fmpz */
    {
        const char * Ks[] = {"-5", "0", "1", "2", "18446744073709551615", "18446744073709551616",
                             "36893488147419103232"};
        size_t i;
        for (i = 0; i < sizeof(Ks) / sizeof(Ks[0]); i++)
        {
            fmpz_t K;
            fmpz_init(K);
            fmpz_set_str(K, Ks[i], 10);
            c = SENT;
            st = adf_modctx_new_fmpz(&c, K);
            snprintf(call, sizeof call, "fmpz(%s)", Ks[i]);
            report(call, st, c);
            fmpz_clear(K);
        }
    }
    /* new_factorial: every n in 0..80 and the extremes */
    {
        ulong n;
        ulong ext[] = {1000, UWORD_MAX / 2, UWORD_MAX};
        size_t i;
        for (n = 0; n <= 80; n++)
        {
            c = SENT;
            st = adf_modctx_new_factorial(&c, n);
            snprintf(call, sizeof call, "fact(%lu)", n);
            report(call, st, c);
        }
        for (i = 0; i < 3; i++)
        {
            c = SENT;
            st = adf_modctx_new_factorial(&c, ext[i]);
            snprintf(call, sizeof call, "fact(%lu)", ext[i]);
            report(call, st, c);
        }
    }
    /* new_primorial_pow: n in 0..120, e in 0..65, and some e = 1 values */
    {
        ulong n, e;
        for (n = 0; n <= 120; n++)
            for (e = 0; e <= 65; e++)
            {
                c = SENT;
                st = adf_modctx_new_primorial_pow(&c, n, e);
                snprintf(call, sizeof call, "prim(%lu,%lu)", n, e);
                report(call, st, c);
            }
        {
            ulong ns[] = {1000, 100000};
            size_t i;
            for (i = 0; i < 2; i++)
            {
                c = SENT;
                st = adf_modctx_new_primorial_pow(&c, ns[i], 1);
                snprintf(call, sizeof call, "prim(%lu,1)", ns[i]);
                report(call, st, c);
            }
        }
        c = SENT;
        st = adf_modctx_new_primorial_pow(&c, UWORD_MAX, UWORD_MAX);
        report("prim(max,max)", st, c);
        c = SENT;
        st = adf_modctx_new_primorial_pow(&c, UWORD_MAX, 64);
        report("prim(max,64)", st, c);
    }
    /* out = NULL */
    printf("nullout blocks=%d fmpz=%d fact=%d prim=%d\n", adf_modctx_new_blocks(NULL, NULL, 0),
           0, adf_modctx_new_factorial(NULL, 3), adf_modctx_new_primorial_pow(NULL, 3, 1));
    printf("SUMMARY untouched_bad=%ld\n", untouched_bad);
    flint_cleanup();
    return 0;
}
