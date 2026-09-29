#include <adelefeld.h>
#include <stdio.h>
#include <stdlib.h>

int baseline_resid_reconstruct(adf_rat_t, adf_recon_cert_t, const adf_resid_t,
                               const fmpz_t, const fmpz_t, slong);

static long gcdl(long a, long b)
{
    long t;
    if (a < 0) a = -a;
    while (b) { t = a % b; a = b; b = t; }
    return a;
}

static int cert_equal(const adf_recon_cert_t a, const adf_recon_cert_t b)
{
    return a->kind == b->kind && fmpz_equal(a->Rp, b->Rp) && fmpz_equal(a->Tp, b->Tp)
        && fmpz_equal(a->R, b->R) && fmpz_equal(a->T, b->T);
}

static void die(const char *what, long m, long c, long a, long b, long lim)
{
    fprintf(stderr, "%s: m=%ld c=%ld A=%ld B=%ld limit=%ld\n", what, m, c, a, b, lim);
    exit(1);
}

int main(void)
{
    adf_resid_t x;
    adf_rat_t q, qb;
    adf_recon_cert_t cert, cb;
    fmpz_t m, c, A, B;
    long cases = 0, first_cases = 0, verified = 0, rejected_bad_cert = 0;
    long statuses[11] = {0};
    const long limits[] = {-2, 0, 1, 2, 5, 20};
    adf_resid_init(x);
    adf_rat_init(q); adf_rat_init(qb);
    adf_recon_cert_init(cert); adf_recon_cert_init(cb);
    fmpz_init(m); fmpz_init(c); fmpz_init(A); fmpz_init(B);

    for (long mm = 1; mm <= 24; mm++)
    for (long cc = -2; cc <= mm + 2; cc++)
    for (long aa = -1; aa <= mm + 1; aa++)
    for (long bb = -1; bb <= 12; bb++)
    for (size_t li = 0; li < sizeof(limits)/sizeof(limits[0]); li++)
    {
        long lim = limits[li], bestx = 999, besty = 999, bestn = 0, bestd = 1;
        long cutx = 999, cuty = 999, cutn = 0, cutd = 1;
        int total = 0, seen = 0, expected_count, expected_status;
        fmpz_set_si(m, mm); fmpz_set_si(c, cc);
        fmpz_set_si(A, aa); fmpz_set_si(B, bb);
        if (adf_resid_set_fmpz2(x, c, m) != ADF_OK) abort();
        adf_rat_set_si(q, -77); adf_rat_set_si(qb, -77);
        int old = baseline_resid_reconstruct(qb, cb, x, A, B, lim);
        int strict = adf_resid_reconstruct(q, cert, x, A, B, lim);
        if (old != strict || !adf_rat_identical(q, qb) || !cert_equal(cert, cb))
            die("baseline difference", mm, cc, aa, bb, lim);
        cases++; statuses[strict]++;
        if (!adf_resid_verify_result(x, A, B, lim, strict, q, cert))
            die("verifier rejected library result", mm, cc, aa, bb, lim);
        verified++;
        if (cert->kind == 1 && strict != ADF_NOT_UNIQUE)
        {
            cert->kind = 0;
            if (adf_resid_verify_result(x, A, B, lim, strict, q, cert))
                die("verifier accepted absent certificate", mm, cc, aa, bb, lim);
            cert->kind = 1;
            rejected_bad_cert++;
        }

        adf_rat_set_si(q, -77);
        int count = -99;
        int st = adf_resid_reconstruct_first(q, &count, cert, x, A, B, lim);
        long c0 = ((cc % mm) + mm) % mm;
        if (aa >= 0 && bb >= 1)
        for (long d = 1; d <= bb; d++)
        for (long n = -aa; n <= aa; n++)
        {
            if (gcdl(n, d) != 1 || ((n - c0*d) % mm) != 0) continue;
            total++;
            if (aa >= mm) continue;
            long R = fmpz_get_si(cert->R), Rp = fmpz_get_si(cert->Rp);
            long T = fmpz_get_si(cert->T), Tp = fmpz_get_si(cert->Tp);
            long sg = T < 0 ? -1 : 1, at = labs(T), ap = labs(Tp);
            long xx = (sg*n*ap + Rp*d) / mm;
            long yy = (R*d - sg*n*at) / mm;
            if (xx < bestx || (xx == bestx && yy < besty))
            { bestx = xx; besty = yy; bestn = n; bestd = d; }
            if (xx <= (lim > 0 ? lim : 0))
            {
                seen++;
                if (xx < cutx || (xx == cutx && yy < cuty))
                { cutx = xx; cuty = yy; cutn = n; cutd = d; }
            }
        }
        if (aa < 0 || bb < 1) { expected_status = ADF_NO_SOLUTION; expected_count = 0; }
        else if (aa >= mm) { expected_status = ADF_OK; expected_count = 2; bestn = c0; bestd = 1; }
        else if (strict == ADF_NOT_DETERMINED)
        {
            expected_status = seen ? ADF_OK : ADF_NOT_DETERMINED;
            expected_count = 0;
            if (seen) { bestn = cutn; bestd = cutd; }
        }
        else if (total == 0) { expected_status = ADF_NO_SOLUTION; expected_count = 0; }
        else if (total == 1) { expected_status = ADF_OK; expected_count = 1; }
        else { expected_status = ADF_OK; expected_count = 2; }
        if (st != expected_status || count != expected_count)
            die("first status/count", mm, cc, aa, bb, lim);
        if (st == ADF_OK)
        {
            if (fmpz_get_si(fmpq_numref(q->q)) != bestn ||
                fmpz_get_si(fmpq_denref(q->q)) != bestd)
                die("first q", mm, cc, aa, bb, lim);
        }
        else if (fmpz_get_si(fmpq_numref(q->q)) != -77)
            die("first touched q", mm, cc, aa, bb, lim);
        first_cases++;
    }

    printf("grid baseline=%ld first=%ld verified=%ld bad-cert-rejected=%ld "
           "OK=%ld NOT_UNIQUE=%ld NO_SOLUTION=%ld NOT_DETERMINED=%ld\n",
           cases, first_cases, verified, rejected_bad_cert, statuses[ADF_OK], statuses[ADF_NOT_UNIQUE],
           statuses[ADF_NO_SOLUTION], statuses[ADF_NOT_DETERMINED]);

    long big = 0;
    for (long bits = 128; bits <= 2048; bits *= 2)
    for (long ci = 0; ci < 4; ci++)
    for (long ai = 0; ai < 4; ai++)
    for (long bi = 0; bi < 3; bi++)
    for (long li = 0; li < 3; li++)
    {
        fmpz_one(m); fmpz_mul_2exp(m, m, bits); fmpz_add_ui(m, m, 13);
        if (ci == 0) fmpz_zero(c);
        if (ci == 1) fmpz_set_si(c, -17);
        if (ci == 2) { fmpz_fdiv_q_ui(c, m, 3); }
        if (ci == 3) { fmpz_add_ui(c, m, 7); }
        if (ai == 0) fmpz_zero(A);
        if (ai == 1) fmpz_set_ui(A, 1000000);
        if (ai == 2) { fmpz_sub_ui(A, m, 1); }
        if (ai == 3) fmpz_set(A, m);
        if (bi == 0) fmpz_one(B);
        if (bi == 1) fmpz_set_ui(B, 1000000);
        if (bi == 2) { fmpz_one(B); fmpz_mul_2exp(B, B, bits + 30); }
        adf_resid_set_fmpz2(x, c, m);
        adf_rat_set_si(q, -77); adf_rat_set_si(qb, -77);
        int old = baseline_resid_reconstruct(qb, cb, x, A, B, li);
        int now = adf_resid_reconstruct(q, cert, x, A, B, li);
        if (old != now || !adf_rat_identical(q, qb) || !cert_equal(cert, cb))
            die("big baseline difference", bits, ci, ai, bi, li);
        big++;
    }
    printf("large baseline=%ld (2^128+13 through 2^2048+13 moduli)\n", big);

    const long claims[][6] = {
        {10, 1, 2, 5, 0, ADF_OK},
        {2, 1, 1, 1, 0, ADF_NOT_UNIQUE},
        {4, 2, 1, 2, 0, ADF_NO_SOLUTION}
    };
    for (size_t i = 0; i < 3; i++)
    {
        fmpz_set_si(m, claims[i][0]); fmpz_set_si(c, claims[i][1]);
        fmpz_set_si(A, claims[i][2]); fmpz_set_si(B, claims[i][3]);
        adf_resid_set_fmpz2(x, c, m);
        adf_rat_set_si(q, 1);
        int actual = adf_resid_reconstruct(qb, cert, x, A, B, claims[i][4]);
        cert->kind = 1;
        if (i < 2)
        {
            fmpz_set_si(cert->Rp, claims[i][0]); fmpz_zero(cert->Tp);
            fmpz_one(cert->R); fmpz_one(cert->T);
        }
        else
        {
            fmpz_set_si(cert->Rp, 2); fmpz_one(cert->Tp);
            fmpz_zero(cert->R); fmpz_set_si(cert->T, -2);
        }
        if (!adf_recon_cert_check(cert, x, A)) abort();
        int accepted = adf_resid_verify_result(x, A, B, claims[i][4], claims[i][5], q, cert);
        printf("claim %zu actual=%s claimed=%s accepted=%d\n", i,
               adf_status_str(actual), adf_status_str(claims[i][5]), accepted);
    }

    fmpz_clear(m); fmpz_clear(c); fmpz_clear(A); fmpz_clear(B);
    adf_resid_clear(x); adf_rat_clear(q); adf_rat_clear(qb);
    adf_recon_cert_clear(cert); adf_recon_cert_clear(cb);
    return 0;
}
