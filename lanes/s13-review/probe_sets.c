#include <adelefeld.h>
#include <stdio.h>
#include <stdlib.h>

static long gcdl(long a, long b)
{
    long t;
    if (a < 0) a = -a;
    if (b < 0) b = -b;
    while (b) { t = a % b; a = b; b = t; }
    return a;
}

int main(void)
{
    adf_resid_t x, marker;
    adf_rat_t q;
    adf_fball_t ball, local;
    adf_modctx_struct *ctx = NULL;
    fmpz_t a, h, d, n, den, aa, hh, dd;
    long sets = 0, rat_members = 0, forgets = 0, local_cases = 0, huge = 0, life = 0, negraw = 0;
    adf_resid_init(x); adf_resid_init(marker); adf_rat_init(q);
    adf_fball_init(ball); adf_fball_init(local);
    fmpz_init(a); fmpz_init(h); fmpz_init(d); fmpz_init(n); fmpz_init(den);
    fmpz_init(aa); fmpz_init(hh); fmpz_init(dd);
    for (long mm = 1; mm <= 30; mm++)
    for (long nn = -12; nn <= 12; nn++)
    for (long rawden = -12; rawden <= 12; rawden++)
    {
        if (!rawden) continue;
        fmpz_set_si(h, mm); fmpz_set_si(n, nn); fmpz_set_si(den, rawden);
        if (adf_rat_set_fmpz2(q, n, den) != ADF_OK) abort();
        fmpz_set_si(a, 11); adf_resid_set_fmpz2(marker, a, h);
        adf_resid_set(x, marker);
        int st = adf_resid_set_rat(x, q, h);
        long qn = fmpz_get_si(fmpq_numref(q->q));
        long qd = fmpz_get_si(fmpq_denref(q->q));
        int exp = gcdl(qd, mm) == 1;
        if ((st == ADF_OK) != exp || (st != ADF_OK && !adf_resid_identical(x, marker)))
            abort();
        if (exp && (qn - fmpz_get_si(x->c)*qd) % mm != 0) abort();
        if (exp && !adf_resid_is_canonical(x)) abort();
        sets++;
        for (long cc = 0; cc < mm; cc++)
        {
            fmpz_set_si(a, cc); adf_resid_set_fmpz2(x, a, h);
            int want = exp && ((qn - cc*qd) % mm == 0);
            if (adf_resid_contains_rat(x, q) != want) abort();
            rat_members++;
        }
    }
    for (long rawh = 0; rawh <= 22; rawh++)
    for (long rawa = -30; rawa <= 30; rawa++)
    for (long rawd = -8; rawd <= 8; rawd++)
    {
        if (!rawd) continue;
        fmpz_set_si(a, rawa); fmpz_set_si(h, rawh); fmpz_set_si(d, rawd);
        if (adf_fball_set_fmpz3(ball, a, h, d) != ADF_OK) abort();
        adf_fball_get_fmpz3(aa, hh, dd, ball);
        fmpz_set_si(a, 7); fmpz_set_si(h, 13);
        adf_resid_set_fmpz2(marker, a, h); adf_resid_set(x, marker);
        int st = adf_resid_set_fball_forget(x, ball);
        long H = fmpz_get_si(hh), D = fmpz_get_si(dd);
        int want = H > 0 && gcdl(H, D) == 1;
        if ((st == ADF_OK) != want || (st != ADF_OK && !adf_resid_identical(x, marker))) abort();
        if (want)
        {
            for (long k = -5; k <= 5; k++)
            {
                fmpz_mul_si(n, hh, k); fmpz_add(n, n, aa);
                adf_rat_set_fmpz2(q, n, dd);
                if (!adf_resid_contains_rat(x, q)) abort();
            }
            if (fmpz_get_si(x->m) != H ||
                (fmpz_get_si(aa) - fmpz_get_si(x->c)*D) % H != 0) abort();
        }
        forgets++;
    }
    for (long bits = 256; bits <= 2048; bits *= 2)
    for (long shift = 1; shift <= 12; shift++)
    {
        fmpz_one(h); fmpz_mul_2exp(h, h, bits); fmpz_add_ui(h, h, 15);
        fmpz_one(n); fmpz_mul_2exp(n, n, bits + shift); fmpz_add_ui(n, n, 7);
        fmpz_one(den); fmpz_mul_2exp(den, den, bits/2 + shift);
        fmpz_add_ui(den, den, 1); if (shift % 2) fmpz_neg(den, den);
        adf_rat_set_fmpz2(q, n, den);
        fmpz_set_si(a, 3); adf_resid_set_fmpz2(marker, a, h);
        adf_resid_set(x, marker);
        fmpz_gcd(d, fmpq_denref(q->q), h);
        int want = fmpz_is_one(d);
        int st = adf_resid_set_rat(x, q, h);
        if ((st == ADF_OK) != want || (st != ADF_OK && !adf_resid_identical(x, marker))) abort();
        if (want)
        {
            fmpz_mul(d, x->c, fmpq_denref(q->q));
            fmpz_sub(d, d, fmpq_numref(q->q));
            if (!fmpz_divisible(d, h) || !adf_resid_contains_rat(x, q)) abort();
        }
        huge++;
    }
    {
        adf_rat_t qc;
        adf_rat_init(qc);
        for (long mm = 1; mm <= 10; mm++)
        for (long rawden = -7; rawden <= -1; rawden++)
        {
            fmpz_set_si(h, mm);
            fmpz_one(fmpq_numref(q->q));
            fmpz_set_si(fmpq_denref(q->q), rawden);
            if (adf_rat_set_fmpq(qc, q->q) != ADF_OK) abort();
            int s1 = adf_resid_set_rat(x, q, h);
            int s2 = adf_resid_set_rat(marker, qc, h);
            if (s1 != s2 || (s1 == ADF_OK && !adf_resid_identical(x, marker))) abort();
            if (s1 == ADF_OK && adf_resid_contains_rat(x, q) != adf_resid_contains_rat(x, qc)) abort();
            negraw++;
        }
        adf_rat_clear(qc);
    }
    {
        const ulong blocks[] = {6, 35};
        if (adf_modctx_new_blocks(&ctx, blocks, 2) != ADF_OK) abort();
        fmpz_set_si(h, 210);
        for (long rawa = -300; rawa <= 300; rawa += 7)
        for (long rawd = 1; rawd <= 12; rawd++)
        {
            fmpz_set_si(a, rawa); fmpz_set_si(d, rawd);
            adf_fball_set_fmpz3(ball, a, h, d);
            if (adf_fball_set_local(local, ball, ctx) != ADF_OK) continue;
            adf_resid_set_fball_forget(x, ball);
            adf_resid_set_fball_forget(marker, local);
            if (!adf_resid_identical(x, marker)) abort();
            local_cases++;
        }
    }
    {
        adf_recon_cert_t cert, other;
        adf_recon_cert_init(cert); adf_recon_cert_init(other);
        for (long i = 0; i < 1000; i++)
        {
            fmpz_set_si(a, 3*i - 700); fmpz_set_si(h, 1 + i % 97);
            adf_resid_set_fmpz2(x, a, h);
            adf_resid_set(marker, x); adf_resid_set(x, x);
            if (!adf_resid_identical(x, marker)) abort();
            adf_resid_swap(x, marker);
            if (!adf_resid_identical(x, marker)) abort();
            cert->kind = 1;
            fmpz_set_si(cert->Rp, i); fmpz_set_si(cert->Tp, -i);
            fmpz_set_si(cert->R, 2*i); fmpz_set_si(cert->T, 3*i);
            adf_recon_cert_set(other, cert); adf_recon_cert_set(cert, cert);
            if (!adf_recon_cert_identical(other, cert)) abort();
            adf_recon_cert_swap(cert, other);
            if (!adf_recon_cert_identical(other, cert)) abort();
            fmpz_add_ui(other->T, other->T, 1);
            if (adf_recon_cert_identical(other, cert)) abort();
            life++;
        }
        adf_recon_cert_clear(cert); adf_recon_cert_clear(other);
    }
    printf("set_rat=%ld contains=%ld forget=%ld local=%ld huge=%ld lifecycle=%ld raw-negative=%ld\n",
           sets, rat_members, forgets, local_cases, huge, life, negraw);
    adf_fball_clear(local); adf_fball_clear(ball); adf_modctx_free(ctx);
    adf_rat_clear(q); adf_resid_clear(x); adf_resid_clear(marker);
    fmpz_clear(a); fmpz_clear(h); fmpz_clear(d); fmpz_clear(n); fmpz_clear(den);
    fmpz_clear(aa); fmpz_clear(hh); fmpz_clear(dd);
    return 0;
}
