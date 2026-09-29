/* Raw-field predicate oracle, including invalid rational representations. */
#include <stdio.h>
#include <adelefeld.h>

static long gcdsmall(long a, long b)
{
    if (a < 0) a = -a;
    if (b < 0) b = -b;
    while (b) { long t = a % b; a = b; b = t; }
    return a;
}

int main(void)
{
    adf_lball_t x;
    unsigned long cases = 0, failures = 0;
    ulong ps[] = {0, 1, 2, 3, 4, 5, 7, 9};
    adf_lball_init(x);
    for (unsigned i = 0; i < sizeof(ps)/sizeof(ps[0]); i++)
    for (int e = -1; e <= 2; e++)
    for (long a = -8; a <= 8; a++)
    for (long b = -3; b <= 8; b++)
    for (long v = -2; v <= 2; v++)
    for (long N = -2; N <= 3; N++)
    {
        ulong p = ps[i];
        int want = 0;
        x->p = p; x->exact = e; x->v = v; x->N = N;
        fmpz_set_si(fmpq_numref(x->u), a); fmpz_set_si(fmpq_denref(x->u), b);
        if ((p == 2 || p == 3 || p == 5 || p == 7) && (e == 0 || e == 1) &&
            b > 0 && gcdsmall(a, b) == 1)
        {
            if (e)
                want = N == 0 && (a == 0 ? v == 0 : a % (long)p != 0 && b % (long)p != 0);
            else if (b == 1)
            {
                if (!a) want = v == 0;
                else if (a > 0 && v < N && a % (long)p != 0)
                {
                    long power = 1;
                    for (long k = 0; k < N-v; k++) power *= (long)p;
                    want = a < power;
                }
            }
        }
        int got = adf_lball_is_canonical(x);
        cases++;
        if (got != want)
        {
            failures++;
            if (failures < 10)
                printf("p=%lu e=%d a=%ld b=%ld v=%ld N=%ld got=%d want=%d\n", p, e, a, b, v, N, got, want);
        }
    }
    printf("raw_predicate_cases=%lu failures=%lu\n", cases, failures);
    adf_lball_clear(x); flint_cleanup();
    return failures ? 1 : 0;
}
