/* tests/test_resid_full.c: adf_resid_reconstruct in the whole range (slice 2 of milestone S, lane
   s3-slice2; docs/api-s.md section 2; docs/proofs/solvers.md Definition 1.1, Propositions 1.5, 1.6, 1.7,
   Algorithm R at lines 256 to 324).

   The oracles, in order of independence:
   1. enumerate (this file): Definition 1.1 by enumeration with machine integers; for m <= 12 it is
      cross-checked against the literal double loop over (n, d).
   2. brute_full: every m <= 36, c from -m to 2 m, A from 0 to 2 m, B in {1, 2, 3, 5, m - 1, m, m + 1, 2 m},
      limit so large that no search is cut: status and q against the enumeration; the count of every status.
   3. limits_grid: the limits -1, 0, 1, 2, 5 on the same grid for m <= 24; the status is recounted here from the
      enumeration and Proposition 1.5 (the round of a point by Cramer's rule, an own Euclidean algorithm in
      machine integers), not from the loop of the library.
   4. tests/ref/vectors/s3-slice2/recon.jsonl, written by lanes/s3-slice2/gen_vectors.py from recon_partial of
      proto/solvers_checks.py (which the script imports): small grids and operands of 64, 300, 2000 bits.
   5. fixed cases (check_s3_edge, notes 2 and 5 of docs/api-s.md section 2), large operands with the
      time of each call, q and cert on every status, cert = NULL, aliasing. */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include <flint/fmpq.h>
#include <flint/fmpz.h>

#include <adelefeld/resid.h>

#include "support/jsonl.h"
#include "test_runner.h"

/* ---- helpers ---- */

static slong
gcd_sl(slong a, slong b)
{
    if (a < 0)
        a = -a;
    if (b < 0)
        b = -b;
    while (b != 0)
    {
        slong t = a % b;
        a = b;
        b = t;
    }
    return a;
}

#define MAXSOL 4096

typedef struct
{
    slong n[MAXSOL], d[MAXSOL];
    int cnt;
} sollist;

static sollist LIST, LIST2;

/* Definition 1.1 by enumeration: for each d prime to m, the integers n in [-A, A] of the class c d modulo m. */
static void
enumerate(sollist * L, slong m, slong c, slong A, slong B)
{
    slong d;

    L->cnt = 0;
    for (d = 1; d <= B; d++)
    {
        slong r, n;

        if (gcd_sl(d, m) != 1)
            continue;
        r = (((c * d) % m) + m) % m;
        n = r - m * ((r + A) / m);                  /* the least n >= -A with n = r modulo m */
        for (; n <= A; n += m)
            if (gcd_sl(n, d) == 1)
            {
                ADF_CHECK(L->cnt < MAXSOL);
                if (L->cnt < MAXSOL)
                {
                    L->n[L->cnt] = n;
                    L->d[L->cnt] = d;
                    L->cnt++;
                }
            }
    }
}

/* Definition 1.1, literally: the double loop over (n, d). */
static void
enumerate_literal(sollist * L, slong m, slong c, slong A, slong B)
{
    slong n, d;

    L->cnt = 0;
    for (d = 1; d <= B; d++)
        for (n = -A; n <= A; n++)
            if (gcd_sl(n, d) == 1 && gcd_sl(d, m) == 1 && (n - c * d) % m == 0 && L->cnt < MAXSOL)
            {
                L->n[L->cnt] = n;
                L->d[L->cnt] = d;
                L->cnt++;
            }
}

static int
status_of_count(int cnt)
{
    return cnt == 0 ? ADF_NO_SOLUTION : (cnt == 1 ? ADF_OK : ADF_NOT_UNIQUE);
}

typedef struct
{
    fmpz_t c, m, A, B;
    adf_resid_t x;
    adf_rat_t q;
    adf_recon_cert_t cert;
} fixture;

static void
fx_init(fixture * f)
{
    fmpz_init(f->c);
    fmpz_init(f->m);
    fmpz_init(f->A);
    fmpz_init(f->B);
    adf_resid_init(f->x);
    adf_rat_init(f->q);
    adf_recon_cert_init(f->cert);
}

static void
fx_clear(fixture * f)
{
    fmpz_clear(f->c);
    fmpz_clear(f->m);
    fmpz_clear(f->A);
    fmpz_clear(f->B);
    adf_resid_clear(f->x);
    adf_rat_clear(f->q);
    adf_recon_cert_clear(f->cert);
}

static void
fx_set_si(fixture * f, slong c, slong m, slong A, slong B)
{
    fmpz_set_si(f->c, c);
    fmpz_set_si(f->m, m);
    fmpz_set_si(f->A, A);
    fmpz_set_si(f->B, B);
    ADF_CHECK(adf_resid_set_fmpz2(f->x, f->c, f->m) == ADF_OK);
}

/* q = -777/13 and a certificate that no call would produce: the sentinels of "untouched". */
static void
sentinel(fixture * f)
{
    fmpz_set_si(fmpq_numref(f->q->q), -777);
    fmpz_set_si(fmpq_denref(f->q->q), 13);
    fmpz_set_si(f->cert->Rp, 101);
    fmpz_set_si(f->cert->Tp, -102);
    fmpz_set_si(f->cert->R, 103);
    fmpz_set_si(f->cert->T, 104);
    f->cert->kind = 1;
}

static int
q_is_sentinel(const fixture * f)
{
    return fmpz_equal_si(fmpq_numref(f->q->q), -777) && fmpz_equal_si(fmpq_denref(f->q->q), 13);
}

static int
cert_is_none(const adf_recon_cert_t cert)
{
    return cert->kind == 0 && fmpz_is_zero(cert->Rp) && fmpz_is_zero(cert->Tp) && fmpz_is_zero(cert->R)
           && fmpz_is_zero(cert->T);
}

static int
q_equals_si(const adf_rat_t q, slong n, slong d)
{
    return fmpz_equal_si(fmpq_numref(q->q), n) && fmpz_equal_si(fmpq_denref(q->q), d) && adf_rat_is_canonical(q);
}

/* the certificate of the call, as the header promises: none if A < 0, B < 1 or A >= m, else a pair (C1) to (C4) */
static int
cert_is_right(const fixture * f)
{
    if (fmpz_sgn(f->A) < 0 || fmpz_sgn(f->B) < 1 || fmpz_cmp(f->A, f->m) >= 0)
        return cert_is_none(f->cert);
    return adf_recon_cert_check(f->cert, f->x, f->A);
}

/* q is a solution of Definition 1.1 for the fixture (tested with fmpz, whatever the size) */
static int
q_is_solution(const fixture * f)
{
    fmpz_t t, u;
    const fmpz * n = fmpq_numref(f->q->q), * d = fmpq_denref(f->q->q);
    int ok;

    fmpz_init(t);
    fmpz_init(u);
    ok = fmpz_sgn(d) > 0 && fmpz_cmp(d, f->B) <= 0 && fmpz_cmpabs(n, f->A) <= 0;
    fmpz_gcd(t, n, d);
    ok = ok && fmpz_is_one(t);
    fmpz_gcd(t, d, f->m);
    ok = ok && fmpz_is_one(t);
    fmpz_mul(t, f->c, d);
    fmpz_sub(t, n, t);
    ok = ok && fmpz_divisible(t, f->m);
    fmpz_clear(t);
    fmpz_clear(u);
    return ok;
}

/* ---- 1. the oracle against the literal definition ---- */

ADF_TEST(enumeration_agrees_with_the_literal_definition)
{
    slong m, c, A, B;
    unsigned long n = 0;

    for (m = 1; m <= 12; m++)
        for (c = -m; c <= 2 * m; c++)
            for (A = 0; A <= 2 * m; A++)
                for (B = 1; B <= 2 * m; B++)
                {
                    int i;

                    enumerate(&LIST, m, c, A, B);
                    enumerate_literal(&LIST2, m, c, A, B);
                    n++;
                    ADF_CHECK_MSG(LIST.cnt == LIST2.cnt, "m=%ld c=%ld A=%ld B=%ld", m, c, A, B);
                    for (i = 0; i < LIST.cnt && i < LIST2.cnt; i++)
                        ADF_CHECK(LIST.n[i] == LIST2.n[i] && LIST.d[i] == LIST2.d[i]);
                }
    printf("enumeration against the literal definition: %lu problems\n", n);
}

/* ---- 2. every m <= 36, no search cut ---- */

ADF_TEST(brute_force_every_m_up_to_36_whole_range)
{
    fixture f;
    slong m, c, A, k;
    unsigned long cases = 0, cnt[3] = { 0, 0, 0 }, cnt_r2[3] = { 0, 0, 0 };   /* NO_SOLUTION, OK, NOT_UNIQUE */

    fx_init(&f);
    for (m = 1; m <= 36; m++)
        for (c = -m; c <= 2 * m; c++)
            for (A = 0; A <= 2 * m; A++)
            {
                slong Bs[] = { 1, 2, 3, 5, m - 1, m, m + 1, 2 * m };

                for (k = 0; k < 8; k++)
                {
                    slong B = Bs[k];
                    int st, want;

                    if (B < 1)
                        continue;
                    fx_set_si(&f, c, m, A, B);
                    sentinel(&f);
                    st = adf_resid_reconstruct(f.q, f.cert, f.x, f.A, f.B, 1000000);
                    cases++;
                    enumerate(&LIST, m, c, A, B);
                    want = status_of_count(LIST.cnt);
                    ADF_CHECK_MSG(st == want, "m=%ld c=%ld A=%ld B=%ld: status %d, %d solutions", m, c, A, B, st,
                                  LIST.cnt);
                    ADF_CHECK_MSG(cert_is_right(&f), "m=%ld c=%ld A=%ld B=%ld: certificate", m, c, A, B);
                    if (st == ADF_OK)
                        ADF_CHECK_MSG(q_equals_si(f.q, LIST.n[0], LIST.d[0]), "m=%ld c=%ld A=%ld B=%ld: q", m, c, A, B);
                    else
                        ADF_CHECK(q_is_sentinel(&f));
                    cnt[want == ADF_NO_SOLUTION ? 0 : (want == ADF_OK ? 1 : 2)]++;
                    if (2 * A * B >= m)
                        cnt_r2[want == ADF_NO_SOLUTION ? 0 : (want == ADF_OK ? 1 : 2)]++;
                }
            }
    printf("brute force whole range: %lu cases; NO_SOLUTION %lu, OK %lu, NOT_UNIQUE %lu; in the range 2AB >= m: "
           "NO_SOLUTION %lu, OK %lu, NOT_UNIQUE %lu\n", cases, cnt[0], cnt[1], cnt[2], cnt_r2[0], cnt_r2[1],
           cnt_r2[2]);
    ADF_CHECK(cnt[0] > 0 && cnt[1] > 0 && cnt[2] > 0);
    ADF_CHECK(cnt_r2[0] > 0 && cnt_r2[1] > 0 && cnt_r2[2] > 0);
    fx_clear(&f);
}

/* ---- 3. the limits, recounted from the enumeration and Proposition 1.5 ---- */

/* The certificate pair of Lemma 1.4 by an own Euclidean algorithm in machine integers (m, c mod m, 0 <= A < m). */
static void
own_pair(slong m, slong c, slong A, slong * Rp, slong * Tp, slong * R, slong * T)
{
    slong r0 = m, t0 = 0, r1 = ((c % m) + m) % m, t1 = 1;

    while (r1 > A)
    {
        slong qq = r0 / r1, r2 = r0 - qq * r1, t2 = t0 - qq * t1;

        r0 = r1;
        r1 = r2;
        t0 = t1;
        t1 = t2;
    }
    *Rp = r0;
    *Tp = t0;
    *R = r1;
    *T = t1;
}

/* the round x = |mu| of a point (n, d) = mu (R, T) + nu (R', T'), by Cramer's rule (Proposition 1.5, step 2) */
static slong
round_of(slong n, slong d, slong Rp, slong Tp, slong R, slong T)
{
    slong num = n * Tp - d * Rp, den = R * Tp - Rp * T;

    ADF_CHECK(den != 0 && num % den == 0 && num != 0);
    num /= den;
    return num < 0 ? -num : num;
}

/* what Algorithm R must return, from Proposition 1.7 (3) and the enumeration; *low is the number of reduced points
   of the rounds x <= ell (only for a cut search) */
static int
expected(slong m, slong c, slong A, slong B, slong limit, const sollist * L, int * cut, int * low_out)
{
    slong Rp, Tp, R, T, X, ell = limit > 0 ? limit : 0;
    int i, low = 0;

    *cut = 0;
    *low_out = 0;
    if (A < 0 || B < 1)
        return ADF_NO_SOLUTION;
    if (A >= m)
        return ADF_NOT_UNIQUE;
    own_pair(m, c, A, &Rp, &Tp, &R, &T);
    if (labs(T) > B || 2 * A * B < m)
        return status_of_count(L->cnt);
    X = B / labs(T);
    if (X <= ell)
        return status_of_count(L->cnt);
    *cut = 1;
    for (i = 0; i < L->cnt; i++)
        if (round_of(L->n[i], L->d[i], Rp, Tp, R, T) <= ell)
            low++;
    *low_out = low;
    return low >= 2 ? ADF_NOT_UNIQUE : ADF_NOT_DETERMINED;
}

ADF_TEST(limits_grid_m_up_to_24)
{
    fixture f;
    slong m, c, A, k, li;
    slong limits[] = { -1, 0, 1, 2, 5 };
    unsigned long calls = 0, nd = 0, cut_unique = 0, cut_ok_forbidden = 0, zero_cx = 0, nullcalls = 0, st_count[9];
    int i;

    for (i = 0; i < 9; i++)
        st_count[i] = 0;
    fx_init(&f);
    for (m = 1; m <= 24; m++)
        for (c = -m; c <= 2 * m; c++)
            for (A = 0; A <= 2 * m; A++)
            {
                slong Bs[] = { 1, 2, 3, 5, m - 1, m, m + 1, 2 * m };

                for (k = 0; k < 8; k++)
                {
                    slong B = Bs[k];

                    if (B < 1)
                        continue;
                    enumerate(&LIST, m, c, A, B);
                    fx_set_si(&f, c, m, A, B);
                    for (li = 0; li < 5; li++)
                    {
                        int cut, low, want, st;
                        slong Rp, Tp, R, T;

                        want = expected(m, c, A, B, limits[li], &LIST, &cut, &low);
                        sentinel(&f);
                        st = adf_resid_reconstruct(f.q, f.cert, f.x, f.A, f.B, limits[li]);
                        calls++;
                        ADF_CHECK_MSG(st == want, "m=%ld c=%ld A=%ld B=%ld limit=%ld: status %d, want %d", m, c, A, B,
                                      limits[li], st, want);
                        if (st != want)
                            continue;
                        st_count[st]++;
                        ADF_CHECK(cert_is_right(&f));
                        if (A >= 0 && A < m)
                        {
                            own_pair(m, c, A, &Rp, &Tp, &R, &T);
                            ADF_CHECK(fmpz_equal_si(f.cert->Rp, Rp) && fmpz_equal_si(f.cert->Tp, Tp)
                                      && fmpz_equal_si(f.cert->R, R) && fmpz_equal_si(f.cert->T, T));
                        }
                        if (st == ADF_OK)
                        {
                            ADF_CHECK(!cut && LIST.cnt == 1 && q_equals_si(f.q, LIST.n[0], LIST.d[0]));
                            cut_ok_forbidden += cut;
                        }
                        else
                            ADF_CHECK(q_is_sentinel(&f));
                        if (st == ADF_NOT_DETERMINED)
                        {
                            nd++;
                            ADF_CHECK(cut && low < 2);
                        }
                        if (st == ADF_NOT_UNIQUE && cut)
                            cut_unique++;
                        if (limits[li] == 0 && (st == ADF_NOT_DETERMINED) != (2 * A * B >= m))
                            zero_cx++;
                        if (calls % 5 == 0)     /* cert = NULL: the same status and q */
                        {
                            adf_rat_t q2;

                            adf_rat_init(q2);
                            fmpz_set_si(fmpq_numref(q2->q), -5);
                            fmpz_set_si(fmpq_denref(q2->q), 7);
                            ADF_CHECK(adf_resid_reconstruct(q2, NULL, f.x, f.A, f.B, limits[li]) == st);
                            if (st == ADF_OK)
                                ADF_CHECK(fmpz_equal(fmpq_numref(q2->q), fmpq_numref(f.q->q))
                                          && fmpz_equal(fmpq_denref(q2->q), fmpq_denref(f.q->q)));
                            else
                                ADF_CHECK(fmpz_equal_si(fmpq_numref(q2->q), -5) && fmpz_equal_si(fmpq_denref(q2->q), 7));
                            adf_rat_clear(q2);
                            nullcalls++;
                        }
                    }
                }
            }
    printf("limits grid: %lu calls (NO_SOLUTION %lu, OK %lu, NOT_UNIQUE %lu, NOT_DETERMINED %lu), NOT_UNIQUE found in a "
           "cut search %lu, OK from a cut search %lu, 'limit 0 is 2AB >= m' fails %lu times, %lu cert = NULL calls\n",
           calls, st_count[ADF_NO_SOLUTION], st_count[ADF_OK], st_count[ADF_NOT_UNIQUE], st_count[ADF_NOT_DETERMINED],
           cut_unique, cut_ok_forbidden, zero_cx, nullcalls);
    ADF_CHECK(nd > 0 && cut_unique > 0 && zero_cx > 0 && cut_ok_forbidden == 0);
    ADF_CHECK(st_count[ADF_NO_SOLUTION] > 0 && st_count[ADF_OK] > 0 && st_count[ADF_NOT_UNIQUE] > 0);
    fx_clear(&f);
}

/* ---- 4. the vectors ---- */

static int
read_fmpz(fmpz_t v, const jsonl_value * j)
{
    jsonl_error_t err;
    const char * text = jsonl_int_text(j, &err);

    if (text == NULL)
    {
        printf("%s\n", jsonl_error_message(&err));
        return 0;
    }
    return fmpz_set_str(v, text, 10) == 0;
}

static int
field_fmpz(fmpz_t v, const jsonl_value * rec, const char * key)
{
    jsonl_error_t err;
    const jsonl_value * j;

    if (!jsonl_field(rec, key, &j, &err))
    {
        printf("%s\n", jsonl_error_message(&err));
        return 0;
    }
    return read_fmpz(v, j);
}

static int
status_from_name(const char * s)
{
    if (strcmp(s, "OK") == 0)
        return ADF_OK;
    if (strcmp(s, "NO_SOLUTION") == 0)
        return ADF_NO_SOLUTION;
    if (strcmp(s, "NOT_UNIQUE") == 0)
        return ADF_NOT_UNIQUE;
    if (strcmp(s, "NOT_DETERMINED") == 0)
        return ADF_NOT_DETERMINED;
    return -1;
}

ADF_TEST(vectors_s3_slice2_every_line)
{
    jsonl_error_t err;
    jsonl_file * file;
    size_t i, k;
    fixture f;
    fmpz_t n, d, w[4], lim, wmax;
    unsigned long cnt[9], bigx = 0, wordmax = 0, maxbits = 0;
    double worst = 0;
    int s;

    for (s = 0; s < 9; s++)
        cnt[s] = 0;
    ADF_CHECK_MSG(jsonl_open("tests/ref/vectors/s3-slice2/recon.jsonl", &file, &err) == 1, "%s",
                  jsonl_error_message(&err));
    if (file == NULL)
        return;
    fx_init(&f);
    fmpz_init(n);
    fmpz_init(d);
    fmpz_init(lim);
    fmpz_init(wmax);
    fmpz_set_si(wmax, WORD_MAX);
    for (k = 0; k < 4; k++)
        fmpz_init(w[k]);
    for (i = 0; i < jsonl_count(file); i++)
    {
        const jsonl_value * rec = jsonl_record(file, i), * v, * cj;
        const char * st_name;
        size_t len;
        int st, want_st;
        slong limit;
        clock_t t0;
        double dt;

        ADF_CHECK(field_fmpz(f.c, rec, "c") && field_fmpz(f.m, rec, "m") && field_fmpz(f.A, rec, "A")
                  && field_fmpz(f.B, rec, "B") && field_fmpz(lim, rec, "limit"));
        ADF_CHECK(fmpz_fits_si(lim));
        limit = fmpz_get_si(lim);
        ADF_CHECK(jsonl_field(rec, "status", &v, &err));
        st_name = jsonl_string(v, &len, &err);
        want_st = status_from_name(st_name);
        ADF_CHECK(want_st >= 0);
        ADF_CHECK(adf_resid_set_fmpz2(f.x, f.c, f.m) == ADF_OK);
        sentinel(&f);
        t0 = clock();
        st = adf_resid_reconstruct(f.q, f.cert, f.x, f.A, f.B, limit);
        dt = (double) (clock() - t0) / CLOCKS_PER_SEC;
        if (dt > worst)
            worst = dt;
        ADF_CHECK_MSG(dt < 1.0, "line %lu: %.2f s", (unsigned long) (i + 1), dt);
        ADF_CHECK_MSG(st == want_st, "line %lu: status %d, want %d (%s)", (unsigned long) (i + 1), st, want_st, st_name);
        if (st != want_st)
            continue;
        cnt[st]++;
        if (limit == WORD_MAX)
            wordmax++;
        if ((unsigned long) fmpz_bits(f.m) > maxbits)
            maxbits = fmpz_bits(f.m);
        ADF_CHECK(jsonl_field(rec, "cert", &cj, &err));
        ADF_CHECK(cert_is_right(&f));
        if (jsonl_is_null(cj, &err) == 1)
            ADF_CHECK(cert_is_none(f.cert));
        else
        {
            for (k = 0; k < 4; k++)
                ADF_CHECK(read_fmpz(w[k], jsonl_at(cj, k, &err)));
            ADF_CHECK_MSG(fmpz_equal(f.cert->Rp, w[0]) && fmpz_equal(f.cert->Tp, w[1]) && fmpz_equal(f.cert->R, w[2])
                              && fmpz_equal(f.cert->T, w[3]),
                          "line %lu: certificate differs from recon_partial", (unsigned long) (i + 1));
        }
        if (st == ADF_OK)
        {
            ADF_CHECK(field_fmpz(n, rec, "n") && field_fmpz(d, rec, "d"));
            ADF_CHECK_MSG(fmpz_equal(fmpq_numref(f.q->q), n) && fmpz_equal(fmpq_denref(f.q->q), d)
                              && adf_rat_is_canonical(f.q) && q_is_solution(&f),
                          "line %lu: q differs", (unsigned long) (i + 1));
        }
        else
            ADF_CHECK(q_is_sentinel(&f));
        ADF_CHECK(jsonl_field(rec, "bigX", &v, &err));
        {
            fmpz_t bx;

            fmpz_init(bx);
            ADF_CHECK(read_fmpz(bx, v));
            if (fmpz_is_one(bx))
            {
                /* X = floor(B / |T|) is at least 2^64: recomputed here from the certificate that the call returned */
                fmpz_t X;

                fmpz_init(X);
                fmpz_abs(X, f.cert->T);
                fmpz_fdiv_q(X, f.B, X);
                ADF_CHECK(fmpz_bits(X) >= 65);
                fmpz_clear(X);
                bigx++;
            }
            fmpz_clear(bx);
        }
    }
    printf("vectors: %lu lines (NO_SOLUTION %lu, OK %lu, NOT_UNIQUE %lu, NOT_DETERMINED %lu), X >= 2^64 in %lu, limit "
           "WORD_MAX in %lu, largest m %lu bits, slowest call %.3f s\n", (unsigned long) jsonl_count(file),
           cnt[ADF_NO_SOLUTION], cnt[ADF_OK], cnt[ADF_NOT_UNIQUE], cnt[ADF_NOT_DETERMINED], bigx, wordmax, maxbits,
           worst);
    ADF_CHECK(jsonl_count(file) > 10000 && cnt[ADF_NO_SOLUTION] > 0 && cnt[ADF_OK] > 0 && cnt[ADF_NOT_UNIQUE] > 0
              && cnt[ADF_NOT_DETERMINED] > 0 && bigx > 0 && wordmax > 0 && maxbits >= 1900);
    for (k = 0; k < 4; k++)
        fmpz_clear(w[k]);
    fmpz_clear(n);
    fmpz_clear(d);
    fmpz_clear(lim);
    fmpz_clear(wmax);
    fx_clear(&f);
    jsonl_close(file);
}

/* ---- 5. the fixed cases of check_s3_edge, and the notes 2 and 5 of docs/api-s.md section 2 ---- */

typedef struct
{
    slong m, c, A, B;
    int status;
    int nsol;
} edge_case;

ADF_TEST(edge_cases_of_check_s3_edge)
{
    fixture f;
    edge_case cases[] = {
        { 1, 0, 0, 1, ADF_OK, 1 },        { 1, 7, 0, 5, ADF_OK, 1 },      { 1, 0, 1, 1, ADF_NOT_UNIQUE, 2 },
        { 2, 1, 1, 1, ADF_NOT_UNIQUE, 2 }, { 2, 1, 0, 9, ADF_NO_SOLUTION, 0 }, { 2, 0, 0, 9, ADF_OK, 1 },
        { 2, 1, 1, 0, ADF_NO_SOLUTION, 0 }, { 5, 1, -1, 1, ADF_NO_SOLUTION, 0 },
        { 6, 5, 1, 5, ADF_NOT_UNIQUE, 2 }, { 6, 5, 1, 4, ADF_OK, 1 },      { 6, -1, 1, 4, ADF_OK, 1 },
        { 6, 11, 1, 4, ADF_OK, 1 },       { 7, 0, 0, 3, ADF_OK, 1 },      { 7, 3, 0, 3, ADF_NO_SOLUTION, 0 },
        { 12, 6, 5, 50, ADF_NO_SOLUTION, 0 }, { 101, 34, 1, 3, ADF_OK, 1 },
        { 2, 1, 2, 1, ADF_NOT_UNIQUE, 2 }, { 12, 6, 1, 5, ADF_NO_SOLUTION, 0 }
    };
    size_t i;
    slong limits[] = { 0, 1, 1000000 };

    fx_init(&f);
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
    {
        edge_case * e = &cases[i];
        int st;

        enumerate(&LIST, e->m, e->c, e->A < 0 ? 0 : e->A, e->B < 1 ? 0 : e->B);
        if (e->A < 0 || e->B < 1)
            LIST.cnt = 0;
        ADF_CHECK_MSG(status_of_count(LIST.cnt) == e->status && (e->nsol >= 2 ? LIST.cnt >= 2 : LIST.cnt == e->nsol),
                      "case %lu: enumeration says %d solutions", (unsigned long) i, LIST.cnt);
        fx_set_si(&f, e->c, e->m, e->A, e->B);
        sentinel(&f);
        st = adf_resid_reconstruct(f.q, f.cert, f.x, f.A, f.B, 1000000);
        ADF_CHECK_MSG(st == e->status, "case %lu (m=%ld c=%ld A=%ld B=%ld): status %d", (unsigned long) i, e->m, e->c,
                      e->A, e->B, st);
        ADF_CHECK(cert_is_right(&f));
        if (st == ADF_OK)
            ADF_CHECK(q_equals_si(f.q, LIST.n[0], LIST.d[0]));
        else
            ADF_CHECK(q_is_sentinel(&f));
        /* the other limits: never a wrong status. Where limit 0 or 1 differs from the complete answer it is
           NOT_DETERMINED, and only for A < m <= 2AB with |T| <= B */
        {
            size_t k;

            for (k = 0; k < 3; k++)
            {
                sentinel(&f);
                st = adf_resid_reconstruct(f.q, f.cert, f.x, f.A, f.B, limits[k]);
                ADF_CHECK(st == e->status || (st == ADF_NOT_DETERMINED && 2 * e->A * e->B >= e->m && e->A < e->m));
                ADF_CHECK(cert_is_right(&f));
            }
        }
    }
    /* note 5: m = 2, c = 1, A = 2, B = 1, limit 0: A >= m gives NOT_UNIQUE, not NOT_DETERMINED, and no certificate */
    fx_set_si(&f, 1, 2, 2, 1);
    sentinel(&f);
    ADF_CHECK(adf_resid_reconstruct(f.q, f.cert, f.x, f.A, f.B, 0) == ADF_NOT_UNIQUE);
    ADF_CHECK(q_is_sentinel(&f) && cert_is_none(f.cert));
    ADF_CHECK(adf_resid_reconstruct(f.q, f.cert, f.x, f.A, f.B, -7) == ADF_NOT_UNIQUE);
    /* m = 2, c = 1, A = B = 1: two solutions, both with denominator 1 (so a second call with B lowered would not
       separate them); limit 0 does not decide, limit 1 does */
    fx_set_si(&f, 1, 2, 1, 1);
    enumerate(&LIST, 2, 1, 1, 1);
    ADF_CHECK(LIST.cnt == 2 && LIST.d[0] == 1 && LIST.d[1] == 1 && LIST.n[0] == -1 && LIST.n[1] == 1);
    sentinel(&f);
    ADF_CHECK(adf_resid_reconstruct(f.q, f.cert, f.x, f.A, f.B, 0) == ADF_NOT_DETERMINED);
    ADF_CHECK(q_is_sentinel(&f) && cert_is_right(&f));
    ADF_CHECK(adf_resid_reconstruct(f.q, f.cert, f.x, f.A, f.B, 1) == ADF_NOT_UNIQUE);
    fmpz_zero(f.B);
    ADF_CHECK(adf_resid_reconstruct(f.q, f.cert, f.x, f.A, f.B, 1) == ADF_NO_SOLUTION);
    ADF_CHECK(cert_is_none(f.cert));
    /* the example of note 1 of api-s.md: 1/5 = 5 modulo 6; A = 1, B = 5 has -1/1 and 1/5; B = 4 has -1/1 only */
    fx_set_si(&f, 5, 6, 1, 5);
    enumerate(&LIST, 6, 5, 1, 5);
    ADF_CHECK(LIST.cnt == 2 && LIST.n[0] == -1 && LIST.d[0] == 1 && LIST.n[1] == 1 && LIST.d[1] == 5);
    ADF_CHECK(adf_resid_reconstruct(f.q, f.cert, f.x, f.A, f.B, 1000) == ADF_NOT_UNIQUE);
    fx_set_si(&f, 5, 6, 1, 4);
    ADF_CHECK(adf_resid_reconstruct(f.q, f.cert, f.x, f.A, f.B, 1000) == ADF_OK && q_equals_si(f.q, -1, 1));
    fx_clear(&f);
}

/* ---- 6. large operands ---- */

/* A value of about `bits` bits from the random state, at least 2. */
static void
rnd_bits(fmpz_t v, flint_rand_t rs, slong bits)
{
    fmpz_randbits(v, rs, bits);
    fmpz_abs(v, v);
    fmpz_add_ui(v, v, 2);
}

/* one call, timed; the answer's properties that do not need an oracle; returns the status */
static int
timed_call(fixture * f, slong limit, double * dt_out)
{
    clock_t t0;
    int st;

    sentinel(f);
    t0 = clock();
    st = adf_resid_reconstruct(f->q, f->cert, f->x, f->A, f->B, limit);
    *dt_out = (double) (clock() - t0) / CLOCKS_PER_SEC;
    ADF_CHECK(cert_is_right(f));
    if (st == ADF_OK)
        ADF_CHECK(q_is_solution(f));
    else
        ADF_CHECK(q_is_sentinel(f));
    return st;
}

ADF_TEST(large_operands_return_at_once)
{
    fixture f;
    flint_rand_t rs;
    slong bits_list[] = { 64, 300, 2000 };
    size_t bi;
    int rep, variant;
    unsigned long calls = 0, st_count[9], wordmax_calls = 0, huge_x = 0;
    double worst = 0, dt;
    fmpz_t B0, X, one_shot;
    int i;

    for (i = 0; i < 9; i++)
        st_count[i] = 0;
    fx_init(&f);
    fmpz_init(B0);
    fmpz_init(X);
    fmpz_init(one_shot);
    flint_randinit(rs);
    for (bi = 0; bi < 3; bi++)
        for (rep = 0; rep < 6; rep++)
            for (variant = 0; variant < 8; variant++)
            {
                slong bits = bits_list[bi], lim[3] = { 0, 3, WORD_MAX };
                int k, st3 = -1;

                /* m of `bits` bits, A of half its size; B near m / (2 A) (variants 0 to 3: 2AB = m - 2 .. m + 1) or near
                   m / A (variants 4 to 7: A B near m), c random or 1 */
                rnd_bits(f.m, rs, bits);
                fmpz_setbit(f.m, bits - 1);
                rnd_bits(f.A, rs, bits / 2);
                if (variant < 4)
                {
                    fmpz_mul_2exp(B0, f.A, 1);
                    fmpz_cdiv_q(B0, f.m, B0);               /* ceil(m / (2A)): 2AB >= m */
                    fmpz_add_si(f.B, B0, variant - 1);      /* 2AB below, at and above m (or thereabouts) */
                }
                else
                {
                    fmpz_cdiv_q(B0, f.m, f.A);
                    fmpz_add_si(f.B, B0, variant - 5);
                }
                if (fmpz_sgn(f.B) < 1)
                    fmpz_one(f.B);
                if (variant % 2 == 0)
                    fmpz_randm(f.c, rs, f.m);
                else
                    fmpz_one(f.c);
                ADF_CHECK(adf_resid_set_fmpz2(f.x, f.c, f.m) == ADF_OK);
                for (k = 0; k < 3; k++)
                {
                    int st = timed_call(&f, lim[k], &dt);

                    calls++;
                    st_count[st]++;
                    if (dt > worst)
                        worst = dt;
                    ADF_CHECK_MSG(dt < 1.0, "bits %ld variant %d limit %ld: %.2f s", bits, variant, lim[k], dt);
                    if (k == 1)
                        st3 = st;
                    if (k == 2)
                    {
                        /* a status decided at limit 3 is the same at WORD_MAX; a cut search at limit 3 is not run
                           with WORD_MAX (it would run for as long as B), so this call was made only when decided */
                        wordmax_calls++;
                        ADF_CHECK(st == st3);
                    }
                    if (k == 1 && st == ADF_NOT_DETERMINED)
                        break;              /* the limit WORD_MAX is not run on an undecided problem */
                }
            }
    /* X above 2^64: c = 1 (T = 1), A = m / 2^69 so that A B >= m / 2 with B = 2^70; the search is cut at once and
       the count of rounds is never converted to a word */
    for (bi = 0; bi < 3; bi++)
    {
        slong bits = bits_list[bi];
        slong lim[3] = { 0, 3, 1000000 };
        int k;

        rnd_bits(f.m, rs, bits + 80);
        fmpz_setbit(f.m, bits + 79);
        fmpz_one(f.B);
        fmpz_mul_2exp(f.B, f.B, 70);
        fmpz_fdiv_q_2exp(f.A, f.m, 69);                /* 2 A B >= m */
        fmpz_add_ui(f.A, f.A, 1);
        fmpz_one(f.c);
        ADF_CHECK(adf_resid_set_fmpz2(f.x, f.c, f.m) == ADF_OK);
        for (k = 0; k < 3; k++)
        {
            int st = timed_call(&f, lim[k], &dt);

            /* c = 1 <= A: the pair is (m, 0, 1, 1), T = 1, X = B = 2^70 > every limit; the one solution 1/1 and the
               points x - y m; with these limits at most one reduced point (1/1) is in the rounds x <= limit */
            ADF_CHECK_MSG(st == ADF_NOT_DETERMINED, "bits %ld limit %ld: status %d", bits, lim[k], st);
            ADF_CHECK(fmpz_is_one(f.cert->T) && fmpz_is_one(f.cert->R));
            ADF_CHECK_MSG(dt < 1.0, "%.2f s", dt);
            calls++;
            st_count[st]++;
            huge_x++;
        }
    }
    /* X above 2^64 and NOT_UNIQUE at once, with the limit WORD_MAX: A = m / 3, B = 2^70 m */
    for (bi = 0; bi < 3; bi++)
    {
        slong bits = bits_list[bi];
        int rep2;

        for (rep2 = 0; rep2 < 5; rep2++)
        {
            int st;

            rnd_bits(f.m, rs, bits);
            fmpz_setbit(f.m, bits - 1);
            fmpz_fdiv_q_ui(f.A, f.m, 3);
            fmpz_mul_2exp(f.B, f.m, 70);
            fmpz_randm(f.c, rs, f.m);
            ADF_CHECK(adf_resid_set_fmpz2(f.x, f.c, f.m) == ADF_OK);
            st = timed_call(&f, WORD_MAX, &dt);
            calls++;
            st_count[st]++;
            wordmax_calls++;
            ADF_CHECK_MSG(dt < 1.0, "%.2f s", dt);
            ADF_CHECK_MSG(st == ADF_NOT_UNIQUE, "bits %ld: status %d", bits, st);
            fmpz_abs(X, f.cert->T);
            fmpz_fdiv_q(X, f.B, X);
            ADF_CHECK(fmpz_bits(X) >= 65);
            huge_x++;
            /* and with limit 0 and 3 the same problem is cut: NOT_DETERMINED or (found in <= 3 rounds) NOT_UNIQUE,
               never OK */
            st = timed_call(&f, 0, &dt);
            ADF_CHECK(st == ADF_NOT_DETERMINED);
            st = timed_call(&f, 3, &dt);
            ADF_CHECK(st == ADF_NOT_DETERMINED || st == ADF_NOT_UNIQUE);
        }
    }
    printf("large operands: %lu calls (NO_SOLUTION %lu, OK %lu, NOT_UNIQUE %lu, NOT_DETERMINED %lu), %lu with X above "
           "2^64, %lu with a decided problem at limit WORD_MAX, slowest call %.3f s\n", calls,
           st_count[ADF_NO_SOLUTION], st_count[ADF_OK], st_count[ADF_NOT_UNIQUE], st_count[ADF_NOT_DETERMINED], huge_x,
           wordmax_calls, worst);
    ADF_CHECK(st_count[ADF_NOT_DETERMINED] > 0 && st_count[ADF_NOT_UNIQUE] > 0 && huge_x >= 24);
    flint_randclear(rs);
    fmpz_clear(B0);
    fmpz_clear(X);
    fmpz_clear(one_shot);
    fx_clear(&f);
}

/* ---- 7. aliasing of the inputs in the search range ---- */

ADF_TEST(aliasing_in_the_search_range)
{
    fixture f, g;
    slong m, c;
    unsigned long n = 0;

    fx_init(&f);
    fx_init(&g);
    for (m = 2; m <= 30; m++)
        for (c = 0; c < m; c++)
        {
            int st1, st2;
            slong lim;

            /* A = c (a member of x), B = m (a member of x): 2 A B = 2 c m >= m for c >= 1 */
            for (lim = 0; lim <= 3; lim++)
            {
                fx_set_si(&f, c, m, c, m);
                fx_set_si(&g, c, m, c, m);
                sentinel(&f);
                sentinel(&g);
                st1 = adf_resid_reconstruct(f.q, f.cert, f.x, f.x->c, f.x->m, lim);
                st2 = adf_resid_reconstruct(g.q, g.cert, g.x, g.A, g.B, lim);
                n++;
                ADF_CHECK_MSG(st1 == st2, "m=%ld c=%ld limit %ld: %d against %d", m, c, lim, st1, st2);
                ADF_CHECK(st1 != ADF_OK || fmpz_equal(fmpq_numref(f.q->q), fmpq_numref(g.q->q)));
                ADF_CHECK(f.cert->kind == g.cert->kind && fmpz_equal(f.cert->T, g.cert->T)
                          && fmpz_equal(f.cert->R, g.cert->R));
                /* A and B the same object */
                sentinel(&f);
                st1 = adf_resid_reconstruct(f.q, f.cert, f.x, f.x->m, f.x->m, lim);
                fmpz_set_si(g.A, m);
                fmpz_set_si(g.B, m);
                st2 = adf_resid_reconstruct(g.q, g.cert, g.x, g.A, g.B, lim);
                ADF_CHECK(st1 == st2);
            }
        }
    ADF_CHECK(n > 100);
    fx_clear(&f);
    fx_clear(&g);
}
