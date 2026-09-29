/* tests/test_resid.c: adf_resid and adf_resid_reconstruct in the range 2 A B < m (slice 1 of
   milestone S, lane s3-slice1; docs/api-s.md section 2; docs/proofs/solvers.md Definition 1.1,
   Definition 1.3, Lemma 1.4, Proposition 1.6).

   The oracles, in order of independence:
   1. brute_force_by_definition: the enumeration of Definition 1.1 over (n, d), written in this
      file with machine integers, for every m <= 36, c from -m to 2 m, A from 0 to 2 m and B in
      {1, 2, 3, 5, m - 1, m, m + 1, 2 m}. For 2 A B < m the status and q must agree with it; for
      2 A B >= m the status must be ADF_UNSUPPORTED.
   2. tests/ref/vectors/s3-slice1/recon.jsonl, written by lanes/s3-slice1/gen_vectors.py from
      recon_partial of proto/solvers_checks.py (2643 lines at the time of writing; every line is run).
   3. check_cert: the four conditions (C1) to (C4) of Definition 1.3, tested on every pair that
      a call returns (brute force, vectors, boundary cases).
   4. fmpq_reconstruct_fmpz_2 of FLINT, in the test labelled "wrapper test", inside 2 A B < m,
      m > 2, A >= 1 only (S-D2: FLINT is used in tests only). */

#include <stdio.h>
#include <string.h>

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

/* (C1) to (C4) of Definition 1.3 for the pair in cert, for (m, c, A). Returns 1 if all hold. */
static int
check_cert(const adf_recon_cert_t cert, const fmpz_t c, const fmpz_t m, const fmpz_t A)
{
    fmpz_t t, u;
    int ok = 1;

    if (cert->kind != 1)
        return 0;
    fmpz_init(t);
    fmpz_init(u);
    /* C1: R = c T and R' = c T' modulo m */
    fmpz_mul(t, c, cert->T);
    fmpz_sub(t, cert->R, t);
    ok = ok && fmpz_divisible(t, m);
    fmpz_mul(t, c, cert->Tp);
    fmpz_sub(t, cert->Rp, t);
    ok = ok && fmpz_divisible(t, m);
    /* C2: |T R' - T' R| = m */
    fmpz_mul(t, cert->T, cert->Rp);
    fmpz_mul(u, cert->Tp, cert->R);
    fmpz_sub(t, t, u);
    ok = ok && fmpz_cmpabs(t, m) == 0;
    /* C3: 0 <= R <= A < R' */
    ok = ok && fmpz_sgn(cert->R) >= 0 && fmpz_cmp(cert->R, A) <= 0 && fmpz_cmp(A, cert->Rp) < 0;
    /* C4: T not 0 and T T' <= 0 */
    ok = ok && !fmpz_is_zero(cert->T);
    fmpz_mul(t, cert->T, cert->Tp);
    ok = ok && fmpz_sgn(t) <= 0;
    fmpz_clear(t);
    fmpz_clear(u);
    return ok;
}

/* Definition 1.1 by enumeration. Writes the number of solutions and the last one found. */
static int
brute_force_by_definition(slong m, slong c, slong A, slong B, slong * n_out, slong * d_out)
{
    slong n, d, count = 0;

    for (d = 1; d <= B; d++)
    {
        if (gcd_sl(d, m) != 1)
            continue;
        for (n = -A; n <= A; n++)
        {
            slong diff = n - c * d;
            if (gcd_sl(n, d) == 1 && diff % m == 0)
            {
                count++;
                *n_out = n;
                *d_out = d;
            }
        }
    }
    return (int) count;
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

/* Set the inputs from machine integers (c may be outside [0, m)). */
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
cert_is_sentinel(const fixture * f)
{
    return f->cert->kind == 1 && fmpz_equal_si(f->cert->Rp, 101) && fmpz_equal_si(f->cert->Tp, -102)
           && fmpz_equal_si(f->cert->R, 103) && fmpz_equal_si(f->cert->T, 104);
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

/* ---- life cycle and set_fmpz2 ---- */

ADF_TEST(life_cycle_and_init_values)
{
    adf_resid_t x;
    adf_recon_cert_t cert;
    fmpz_t c, m;

    adf_resid_init(x);
    ADF_CHECK(fmpz_is_zero(x->c) && fmpz_is_one(x->m));
    ADF_CHECK(adf_resid_is_canonical(x));
    adf_recon_cert_init(cert);
    ADF_CHECK(cert_is_none(cert));
    fmpz_init(c);
    fmpz_init(m);
    adf_resid_get_fmpz2(c, m, x);
    ADF_CHECK(fmpz_is_zero(c) && fmpz_is_one(m));
    fmpz_clear(c);
    fmpz_clear(m);
    adf_resid_clear(x);
    adf_recon_cert_clear(cert);
    ADF_CHECK(adf_sizeof_resid() == 16 && adf_alignof_resid() == 8);
    ADF_CHECK(adf_sizeof_recon_cert() == 40 && adf_alignof_recon_cert() == 8);
}

ADF_TEST(set_fmpz2_reduces_and_refuses_m_below_one)
{
    adf_resid_t x;
    fmpz_t c, m, cc, mm;
    slong cs[] = { -13, -7, -1, 0, 1, 5, 6, 13, 60 };
    size_t i;

    adf_resid_init(x);
    fmpz_init(c);
    fmpz_init(m);
    fmpz_init(cc);
    fmpz_init(mm);
    fmpz_set_si(m, 6);
    for (i = 0; i < sizeof(cs) / sizeof(cs[0]); i++)
    {
        slong want = ((cs[i] % 6) + 6) % 6;

        fmpz_set_si(c, cs[i]);
        ADF_CHECK(adf_resid_set_fmpz2(x, c, m) == ADF_OK);
        adf_resid_get_fmpz2(cc, mm, x);
        ADF_CHECK_MSG(fmpz_equal_si(cc, want) && fmpz_equal_si(mm, 6), "c = %ld gives %ld", cs[i], (long) fmpz_get_si(cc));
        ADF_CHECK(adf_resid_is_canonical(x));
    }
    /* m < 1: DOMAIN, x untouched (it holds (1, 6) from the last loop step 60 mod 6 = 0: set it first) */
    fmpz_set_si(c, 4);
    ADF_CHECK(adf_resid_set_fmpz2(x, c, m) == ADF_OK);
    for (i = 0; i < 3; i++)
    {
        slong bad[] = { 0, -1, -6 };

        fmpz_set_si(mm, bad[i]);
        ADF_CHECK(adf_resid_set_fmpz2(x, c, mm) == ADF_DOMAIN);
        ADF_CHECK(fmpz_equal_si(x->c, 4) && fmpz_equal_si(x->m, 6));
    }
    /* m = 1: everything reduces to 0 */
    fmpz_one(mm);
    ADF_CHECK(adf_resid_set_fmpz2(x, c, mm) == ADF_OK);
    ADF_CHECK(fmpz_is_zero(x->c) && fmpz_is_one(x->m));
    /* aliasing: the inputs are the members of x, in both orders */
    fmpz_set_si(x->c, 3);
    fmpz_set_si(x->m, 7);
    ADF_CHECK(adf_resid_set_fmpz2(x, x->m, x->c) == ADF_OK);   /* c = 7, m = 3: (1, 3) */
    ADF_CHECK(fmpz_equal_si(x->c, 1) && fmpz_equal_si(x->m, 3));
    ADF_CHECK(adf_resid_set_fmpz2(x, x->c, x->m) == ADF_OK);   /* c = 1, m = 3 */
    ADF_CHECK(fmpz_equal_si(x->c, 1) && fmpz_equal_si(x->m, 3));
    /* huge values */
    fmpz_set_ui(m, 1);
    fmpz_mul_2exp(m, m, 3000);
    fmpz_add_ui(m, m, 7);
    fmpz_neg(c, m);
    fmpz_sub_ui(c, c, 1);
    ADF_CHECK(adf_resid_set_fmpz2(x, c, m) == ADF_OK);
    ADF_CHECK(adf_resid_is_canonical(x) && fmpz_equal(x->m, m));
    fmpz_sub_ui(cc, m, 1);
    ADF_CHECK(fmpz_equal(x->c, cc));
    fmpz_clear(c);
    fmpz_clear(m);
    fmpz_clear(cc);
    fmpz_clear(mm);
    adf_resid_clear(x);
}

ADF_TEST(get_fmpz2_aliasing_and_is_canonical)
{
    adf_resid_t x;
    fmpz_t a, b;

    adf_resid_init(x);
    fmpz_init(a);
    fmpz_init(b);
    fmpz_set_si(x->c, 3);
    fmpz_set_si(x->m, 7);
    adf_resid_get_fmpz2(x->m, x->c, x);          /* the outputs are the members, swapped */
    ADF_CHECK(fmpz_equal_si(x->m, 3) && fmpz_equal_si(x->c, 7));
    fmpz_set_si(x->c, 3);
    fmpz_set_si(x->m, 7);
    adf_resid_get_fmpz2(a, b, x);
    ADF_CHECK(fmpz_equal_si(a, 3) && fmpz_equal_si(b, 7));
    ADF_CHECK(adf_resid_is_canonical(x));
    fmpz_set_si(x->c, 7);                        /* c = m */
    ADF_CHECK(!adf_resid_is_canonical(x));
    fmpz_set_si(x->c, -1);
    ADF_CHECK(!adf_resid_is_canonical(x));
    fmpz_set_si(x->c, 0);
    fmpz_set_si(x->m, 0);
    ADF_CHECK(!adf_resid_is_canonical(x));
    fmpz_set_si(x->m, -3);
    ADF_CHECK(!adf_resid_is_canonical(x));
    fmpz_clear(a);
    fmpz_clear(b);
    adf_resid_clear(x);
}

/* ---- 1. brute force by Definition 1.1 ---- */

ADF_TEST(brute_force_every_m_up_to_36)
{
    fixture f;
    slong m, c, A, k;
    unsigned long cases = 0, n_ok = 0, n_b = 0, n_c = 0, n_unsup = 0, n_boxes = 0;

    fx_init(&f);
    for (m = 1; m <= 36; m++)
        for (c = -m; c <= 2 * m; c++)
            for (A = 0; A <= 2 * m; A++)
            {
                slong Bs[] = { 1, 2, 3, 5, m - 1, m, m + 1, 2 * m };

                for (k = 0; k < 8; k++)
                {
                    slong B = Bs[k], n = 0, d = 0;
                    int st, sols;

                    if (B < 1)
                        continue;
                    fx_set_si(&f, c, m, A, B);
                    sentinel(&f);
                    st = adf_resid_reconstruct(f.q, f.cert, f.x, f.A, f.B, 0);
                    cases++;
                    if (2 * A * B >= m)
                    {
                        n_unsup++;
                        ADF_CHECK_MSG(st == ADF_UNSUPPORTED, "m=%ld c=%ld A=%ld B=%ld: status %d", m, c, A, B, st);
                        ADF_CHECK(q_is_sentinel(&f) && cert_is_sentinel(&f));
                        continue;
                    }
                    n_boxes++;
                    sols = brute_force_by_definition(m, c, A, B, &n, &d);
                    ADF_CHECK_MSG(sols <= 1, "m=%ld c=%ld A=%ld B=%ld: %d solutions in the range 2AB < m", m, c, A, B, sols);
                    ADF_CHECK_MSG(check_cert(f.cert, f.c, f.m, f.A), "m=%ld c=%ld A=%ld B=%ld: certificate", m, c, A, B);
                    if (sols == 1)
                    {
                        n_ok++;
                        ADF_CHECK_MSG(st == ADF_OK && q_equals_si(f.q, n, d), "m=%ld c=%ld A=%ld B=%ld: want %ld/%ld, status %d",
                                      m, c, A, B, n, d, st);
                    }
                    else
                    {
                        ADF_CHECK_MSG(st == ADF_NO_SOLUTION, "m=%ld c=%ld A=%ld B=%ld: want NO_SOLUTION, status %d", m, c, A, B, st);
                        ADF_CHECK(q_is_sentinel(&f));
                        if (fmpz_cmpabs(f.cert->T, f.B) > 0)
                            n_b++;
                        else
                        {
                            fmpz_t g;

                            fmpz_init(g);
                            fmpz_gcd(g, f.cert->R, f.cert->T);
                            ADF_CHECK(!fmpz_is_one(g));
                            fmpz_clear(g);
                            n_c++;
                        }
                    }
                }
            }
    printf("brute force: %lu cases, %lu inside 2AB < m (OK %lu, NO_SOLUTION by (b) %lu, by (c) %lu), "
           "%lu UNSUPPORTED\n", cases, n_boxes, n_ok, n_b, n_c, n_unsup);
    ADF_CHECK(n_ok > 0 && n_b > 0 && n_c > 0);
    ADF_CHECK(n_ok + n_b + n_c == n_boxes);
    ADF_CHECK(n_unsup > 0);
    fx_clear(&f);
}

/* ---- 2. the vectors ---- */

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

ADF_TEST(vectors_s3_slice1_every_line)
{
    jsonl_error_t err;
    jsonl_file * file;
    size_t i, k;
    fixture f;
    fmpz_t n, d, w[4];
    unsigned long n_ok = 0, n_b = 0, n_c = 0, n_empty = 0;

    ADF_CHECK_MSG(jsonl_open("tests/ref/vectors/s3-slice1/recon.jsonl", &file, &err) == 1, "%s", jsonl_error_message(&err));
    if (file == NULL)
        return;
    fx_init(&f);
    fmpz_init(n);
    fmpz_init(d);
    for (k = 0; k < 4; k++)
        fmpz_init(w[k]);
    for (i = 0; i < jsonl_count(file); i++)
    {
        const jsonl_value * rec = jsonl_record(file, i), * v, * cj;
        const char * st_name, * why;
        size_t len;
        int st, want_st;

        ADF_CHECK(field_fmpz(f.c, rec, "c") && field_fmpz(f.m, rec, "m") && field_fmpz(f.A, rec, "A")
                  && field_fmpz(f.B, rec, "B"));
        ADF_CHECK(jsonl_field(rec, "status", &v, &err));
        st_name = jsonl_string(v, &len, &err);
        ADF_CHECK(jsonl_field(rec, "why", &v, &err));
        why = jsonl_string(v, &len, &err);
        want_st = strcmp(st_name, "OK") == 0 ? ADF_OK : ADF_NO_SOLUTION;
        ADF_CHECK(strcmp(st_name, "OK") == 0 || strcmp(st_name, "NO_SOLUTION") == 0);
        ADF_CHECK(adf_resid_set_fmpz2(f.x, f.c, f.m) == ADF_OK);
        sentinel(&f);
        st = adf_resid_reconstruct(f.q, f.cert, f.x, f.A, f.B, 0);
        ADF_CHECK_MSG(st == want_st, "line %lu: status %d, want %d", (unsigned long) (i + 1), st, want_st);
        if (st != want_st)
            continue;
        ADF_CHECK(jsonl_field(rec, "cert", &cj, &err));
        if (strcmp(why, "empty") == 0)
        {
            n_empty++;
            ADF_CHECK(jsonl_is_null(cj, &err) == 1);
            ADF_CHECK(cert_is_none(f.cert) && q_is_sentinel(&f));
            continue;
        }
        for (k = 0; k < 4; k++)
            ADF_CHECK(read_fmpz(w[k], jsonl_at(cj, k, &err)));
        ADF_CHECK_MSG(fmpz_equal(f.cert->Rp, w[0]) && fmpz_equal(f.cert->Tp, w[1]) && fmpz_equal(f.cert->R, w[2])
                          && fmpz_equal(f.cert->T, w[3]),
                      "line %lu: certificate differs from recon_partial", (unsigned long) (i + 1));
        ADF_CHECK(check_cert(f.cert, f.c, f.m, f.A));
        if (st == ADF_OK)
        {
            n_ok++;
            ADF_CHECK(field_fmpz(n, rec, "n") && field_fmpz(d, rec, "d"));
            ADF_CHECK_MSG(fmpz_equal(fmpq_numref(f.q->q), n) && fmpz_equal(fmpq_denref(f.q->q), d)
                              && adf_rat_is_canonical(f.q),
                          "line %lu: q differs", (unsigned long) (i + 1));
        }
        else
        {
            ADF_CHECK(q_is_sentinel(&f));
            if (strcmp(why, "b") == 0)
            {
                n_b++;
                ADF_CHECK(fmpz_cmpabs(f.cert->T, f.B) > 0);
            }
            else
            {
                n_c++;
                ADF_CHECK(strcmp(why, "c") == 0 && fmpz_cmpabs(f.cert->T, f.B) <= 0);
            }
        }
    }
    printf("vectors: %lu lines, OK %lu, NO_SOLUTION (b) %lu, (c) %lu, empty box %lu\n",
           (unsigned long) jsonl_count(file), n_ok, n_b, n_c, n_empty);
    ADF_CHECK(jsonl_count(file) > 2000 && n_ok > 0 && n_b > 0 && n_c > 0 && n_empty > 0);
    for (k = 0; k < 4; k++)
        fmpz_clear(w[k]);
    fmpz_clear(n);
    fmpz_clear(d);
    fx_clear(&f);
    jsonl_close(file);
}

/* ---- 3. the boundary of the range, at large m ---- */

/* m = 2 A B + delta with A = 10^15 + 1, B = 10^15 + 3 (delta = 1: inside, 0 and -1: outside). */
ADF_TEST(boundary_2AB_against_m)
{
    fixture f;
    fmpz_t twoAB, nn, dd, inv;
    slong delta;

    fx_init(&f);
    fmpz_init(twoAB);
    fmpz_init(nn);
    fmpz_init(dd);
    fmpz_init(inv);
    fmpz_set_str(f.A, "1000000000000001", 10);
    fmpz_set_str(f.B, "1000000000000003", 10);
    fmpz_mul(twoAB, f.A, f.B);
    fmpz_mul_2exp(twoAB, twoAB, 1);
    /* the planted fraction -A/B is reduced (A and B are odd and 2 apart, so gcd 1) */
    fmpz_neg(nn, f.A);
    fmpz_set(dd, f.B);
    for (delta = -1; delta <= 1; delta++)
    {
        int st;

        fmpz_add_si(f.m, twoAB, delta);           /* m = 2AB - 1, 2AB, 2AB + 1 */
        if (fmpz_invmod(inv, dd, f.m))
        {
            fmpz_mul(f.c, nn, inv);
            fmpz_mod(f.c, f.c, f.m);
        }
        else
            fmpz_set_si(f.c, 12345);
        ADF_CHECK(adf_resid_set_fmpz2(f.x, f.c, f.m) == ADF_OK);
        sentinel(&f);
        st = adf_resid_reconstruct(f.q, f.cert, f.x, f.A, f.B, 0);
        if (delta <= 0)
        {
            ADF_CHECK_MSG(st == ADF_UNSUPPORTED, "delta %ld: status %d", delta, st);
            ADF_CHECK(q_is_sentinel(&f) && cert_is_sentinel(&f));
        }
        else
        {
            ADF_CHECK_MSG(st == ADF_OK, "delta %ld: status %d", delta, st);
            ADF_CHECK(fmpz_equal(fmpq_numref(f.q->q), nn) && fmpz_equal(fmpq_denref(f.q->q), dd));
            ADF_CHECK(check_cert(f.cert, f.c, f.m, f.A));
        }
    }
    /* m = 2AB + 1 with c = 10^25 + 7: NO_SOLUTION by (b) (value from recon_partial of proto/solvers_checks.py) */
    fmpz_add_si(f.m, twoAB, 1);
    fmpz_set_str(f.c, "10000000000000000000000007", 10);
    ADF_CHECK(adf_resid_set_fmpz2(f.x, f.c, f.m) == ADF_OK);
    sentinel(&f);
    ADF_CHECK(adf_resid_reconstruct(f.q, f.cert, f.x, f.A, f.B, 0) == ADF_NO_SOLUTION);
    ADF_CHECK(q_is_sentinel(&f) && check_cert(f.cert, f.c, f.m, f.A));
    /* the word-size trap: 2 A B does not fit in 64 bits, and m is far below 2^63 * 2 */
    fmpz_set_str(f.A, "4611686018427387904", 10);      /* 2^62 */
    fmpz_set_si(f.B, 4);
    fmpz_set_str(f.m, "18446744073709551616", 10);     /* 2^64 = 2 A B / 2 ; 2AB = 2^65 >= m */
    fmpz_set_si(f.c, 1);
    ADF_CHECK(adf_resid_set_fmpz2(f.x, f.c, f.m) == ADF_OK);
    sentinel(&f);
    ADF_CHECK(adf_resid_reconstruct(f.q, f.cert, f.x, f.A, f.B, 0) == ADF_UNSUPPORTED);
    fmpz_set_str(f.m, "36893488147419103233", 10);     /* 2^65 + 1 > 2AB = 2^65: in range */
    ADF_CHECK(adf_resid_set_fmpz2(f.x, f.c, f.m) == ADF_OK);
    ADF_CHECK(adf_resid_reconstruct(f.q, f.cert, f.x, f.A, f.B, 0) == ADF_OK);
    ADF_CHECK(q_equals_si(f.q, 1, 1));
    fmpz_clear(twoAB);
    fmpz_clear(nn);
    fmpz_clear(dd);
    fmpz_clear(inv);
    fx_clear(&f);
}

ADF_TEST(boundary_small_moduli_and_zero_A)
{
    fixture f;
    fmpz_t big;
    int st;

    fx_init(&f);
    fmpz_init(big);
    /* m = 1: only A = 0 lies in the range (2 A B < 1); the answer is 0/1 (every c is 0 modulo 1) */
    fx_set_si(&f, 7, 1, 0, 5);
    sentinel(&f);
    ADF_CHECK(adf_resid_reconstruct(f.q, f.cert, f.x, f.A, f.B, 0) == ADF_OK && q_equals_si(f.q, 0, 1));
    ADF_CHECK(check_cert(f.cert, f.c, f.m, f.A));
    fx_set_si(&f, 0, 1, 1, 1);
    sentinel(&f);
    ADF_CHECK(adf_resid_reconstruct(f.q, f.cert, f.x, f.A, f.B, 0) == ADF_UNSUPPORTED);
    /* m = 2 */
    fx_set_si(&f, 0, 2, 0, 9);
    ADF_CHECK(adf_resid_reconstruct(f.q, f.cert, f.x, f.A, f.B, 0) == ADF_OK && q_equals_si(f.q, 0, 1));
    fx_set_si(&f, 1, 2, 0, 9);                                 /* the pair (2, 1, 0, -2): gcd(0, -2) = 2, case (c) */
    sentinel(&f);
    ADF_CHECK(adf_resid_reconstruct(f.q, f.cert, f.x, f.A, f.B, 0) == ADF_NO_SOLUTION && q_is_sentinel(&f));
    ADF_CHECK(check_cert(f.cert, f.c, f.m, f.A) && fmpz_equal_si(f.cert->T, -2) && fmpz_is_zero(f.cert->R));
    fx_set_si(&f, 1, 2, 0, 1);                                 /* case (b): |T| = 2 > 1 */
    ADF_CHECK(adf_resid_reconstruct(f.q, f.cert, f.x, f.A, f.B, 0) == ADF_NO_SOLUTION);
    fx_set_si(&f, 1, 2, 1, 1);                                 /* 2 A B = 2 >= 2 */
    sentinel(&f);
    ADF_CHECK(adf_resid_reconstruct(f.q, f.cert, f.x, f.A, f.B, 0) == ADF_UNSUPPORTED);
    ADF_CHECK(q_is_sentinel(&f) && cert_is_sentinel(&f));
    /* A = 0 at a large modulus: c = 0 gives 0/1; c = 3 has no solution, for B = 1 and for B huge */
    fmpz_set_si(f.m, 7);
    fmpz_set_str(f.B, "1", 10);
    fmpz_pow_ui(big, f.B, 1);
    fmpz_set_ui(big, 10);
    fmpz_pow_ui(big, big, 100);
    fx_set_si(&f, 0, 7, 0, 3);
    ADF_CHECK(adf_resid_reconstruct(f.q, f.cert, f.x, f.A, big, 0) == ADF_OK && q_equals_si(f.q, 0, 1));
    fx_set_si(&f, 3, 7, 0, 3);
    sentinel(&f);
    st = adf_resid_reconstruct(f.q, f.cert, f.x, f.A, big, 0);
    ADF_CHECK(st == ADF_NO_SOLUTION && q_is_sentinel(&f));
    ADF_CHECK(check_cert(f.cert, f.c, f.m, f.A));
    st = adf_resid_reconstruct(f.q, f.cert, f.x, f.A, f.B, 0);
    ADF_CHECK(st == ADF_NO_SOLUTION);
    /* huge B and A = 0 at a huge modulus, c = 0 */
    fmpz_set_ui(f.m, 3);
    fmpz_pow_ui(f.m, f.m, 500);
    fmpz_zero(f.c);
    fmpz_zero(f.A);
    ADF_CHECK(adf_resid_set_fmpz2(f.x, f.c, f.m) == ADF_OK);
    ADF_CHECK(adf_resid_reconstruct(f.q, f.cert, f.x, f.A, big, 0) == ADF_OK && q_equals_si(f.q, 0, 1));
    fmpz_clear(big);
    fx_clear(&f);
}

ADF_TEST(empty_box_is_no_solution_with_no_certificate)
{
    fixture f;
    slong cases[][2] = { { -1, 1 }, { -5, 5 }, { 0, 0 }, { 3, 0 }, { 3, -4 }, { -5, -5 }, { -1, 0 } };
    size_t i;

    fx_init(&f);
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
    {
        int st;

        fx_set_si(&f, 3, 7, cases[i][0], cases[i][1]);
        if (cases[i][0] >= 0 && cases[i][1] >= 1)
            continue;
        sentinel(&f);
        st = adf_resid_reconstruct(f.q, f.cert, f.x, f.A, f.B, 0);
        ADF_CHECK_MSG(st == ADF_NO_SOLUTION, "A=%ld B=%ld: status %d", cases[i][0], cases[i][1], st);
        ADF_CHECK(q_is_sentinel(&f) && cert_is_none(f.cert));
        /* cert = NULL is allowed and q is still untouched */
        sentinel(&f);
        ADF_CHECK(adf_resid_reconstruct(f.q, NULL, f.x, f.A, f.B, 0) == ADF_NO_SOLUTION && q_is_sentinel(&f));
    }
    /* the one case of the list with A >= 0 and B = 0: (3, 0) has B = 0, so the box is empty too (checked above) */
    fx_clear(&f);
}

ADF_TEST(null_cert_gives_the_same_answers_and_limit_is_ignored)
{
    fixture f;
    slong m, c;
    slong limits[] = { -5, 0, 1, 2, 100 };
    size_t k;
    unsigned long n = 0;

    fx_init(&f);
    for (m = 1; m <= 40; m++)
        for (c = -1; c <= m; c++)
        {
            slong A = 0, B = 1;
            int st1, st2;
            adf_rat_t q2;

            for (A = 0; A <= 4; A++)
                for (B = 1; B <= 6; B++)
                    for (k = 0; k < 5; k++)
                    {
                        fx_set_si(&f, c, m, A, B);
                        adf_rat_init(q2);
                        st1 = adf_resid_reconstruct(f.q, f.cert, f.x, f.A, f.B, 0);
                        st2 = adf_resid_reconstruct(q2, NULL, f.x, f.A, f.B, limits[k]);
                        n++;
                        ADF_CHECK(st1 == st2);
                        ADF_CHECK(st1 != ADF_OK || adf_rat_identical(f.q, q2));
                        adf_rat_clear(q2);
                    }
        }
    ADF_CHECK(n > 0);
    fx_clear(&f);
}

ADF_TEST(aliasing_of_the_inputs)
{
    fixture f;
    fmpz_t v;
    int st;

    fx_init(&f);
    fmpz_init(v);
    /* A and B the same object: m = 101, c = 34 (1/3), A = B = 3 gives 1/3 */
    fx_set_si(&f, 34, 101, 3, 3);
    st = adf_resid_reconstruct(f.q, f.cert, f.x, f.A, f.A, 0);
    ADF_CHECK(st == ADF_OK && q_equals_si(f.q, 1, 3));
    /* A and B the members of x: c = 3, m = 100: A = c = 3, B = c = 3, 2AB = 18 < 100 */
    fx_set_si(&f, 3, 100, 0, 0);
    st = adf_resid_reconstruct(f.q, f.cert, f.x, f.x->c, f.x->c, 0);
    ADF_CHECK_MSG(st == ADF_OK || st == ADF_NO_SOLUTION, "status %d", st);
    fmpz_set_si(f.A, 3);
    ADF_CHECK(st == adf_resid_reconstruct(f.q, f.cert, f.x, f.A, f.A, 0));
    fmpz_clear(v);
    fx_clear(&f);
}

/* ---- 4. wrapper test against FLINT's fmpq_reconstruct_fmpz_2 (inside 2 A B < m, m > 2, A >= 1) ---- */

ADF_TEST(wrapper_test_against_fmpq_reconstruct_fmpz_2)
{
    fixture f;
    fmpq_t r;
    slong m, c, A, B;
    unsigned long n = 0, agree_ok = 0, agree_none = 0;
    flint_rand_t rs;
    int round;

    fx_init(&f);
    fmpq_init(r);
    for (m = 3; m <= 60; m++)
        for (c = 0; c < m; c++)
            for (A = 1; 2 * A < m; A++)
                for (B = 1; 2 * A * B < m; B++)
                {
                    int flint_ok, st;

                    fx_set_si(&f, c, m, A, B);
                    flint_ok = fmpq_reconstruct_fmpz_2(r, f.c, f.m, f.A, f.B);
                    st = adf_resid_reconstruct(f.q, NULL, f.x, f.A, f.B, 0);
                    n++;
                    ADF_CHECK_MSG((st == ADF_OK) == (flint_ok != 0), "m=%ld c=%ld A=%ld B=%ld: adf %d, flint %d", m, c, A, B, st,
                                  flint_ok);
                    if (st == ADF_OK && flint_ok)
                    {
                        agree_ok++;
                        ADF_CHECK(fmpq_equal(r, f.q->q));
                    }
                    else
                        agree_none++;
                }
    flint_randinit(rs);
    for (round = 0; round < 3000; round++)
    {
        slong bits = 8 + (slong) n_randint(rs, 300);
        fmpz_t nn, dd, inv;
        int flint_ok, st;

        fmpz_init(nn);
        fmpz_init(dd);
        fmpz_init(inv);
        fmpz_randbits(f.A, rs, bits);
        fmpz_abs(f.A, f.A);
        fmpz_add_ui(f.A, f.A, 1);
        fmpz_randbits(f.B, rs, bits);
        fmpz_abs(f.B, f.B);
        fmpz_add_ui(f.B, f.B, 1);
        fmpz_mul(f.m, f.A, f.B);
        fmpz_mul_2exp(f.m, f.m, 1);
        fmpz_add_ui(f.m, f.m, 1 + n_randint(rs, 3));
        if (round % 2 == 0)
        {
            fmpz_randm(dd, rs, f.B);
            fmpz_add_ui(dd, dd, 1);
            fmpz_randm(nn, rs, f.A);
            fmpz_sub(nn, nn, f.A);
            fmpz_randm(f.c, rs, f.m);
            if (fmpz_invmod(inv, dd, f.m))
            {
                fmpz_mul(f.c, nn, inv);
                fmpz_mod(f.c, f.c, f.m);
            }
        }
        else
            fmpz_randm(f.c, rs, f.m);
        ADF_CHECK(adf_resid_set_fmpz2(f.x, f.c, f.m) == ADF_OK);
        flint_ok = fmpq_reconstruct_fmpz_2(r, f.c, f.m, f.A, f.B);
        st = adf_resid_reconstruct(f.q, NULL, f.x, f.A, f.B, 0);
        n++;
        ADF_CHECK_MSG((st == ADF_OK) == (flint_ok != 0), "random round %d: adf %d, flint %d", round, st, flint_ok);
        if (st == ADF_OK && flint_ok)
        {
            agree_ok++;
            ADF_CHECK(fmpq_equal(r, f.q->q));
        }
        else
            agree_none++;
        fmpz_clear(nn);
        fmpz_clear(dd);
        fmpz_clear(inv);
    }
    flint_randclear(rs);
    printf("wrapper test (FLINT fmpq_reconstruct_fmpz_2): %lu cases, both OK and equal %lu, both none %lu\n", n,
           agree_ok, agree_none);
    ADF_CHECK(agree_ok > 100 && agree_none > 100);
    fmpq_clear(r);
    fx_clear(&f);
}
