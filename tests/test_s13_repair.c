/* tests/test_s13_repair.c: the repair of the review docs/reviews/s13/review.md.
   1. adf_resid_verify_result verifies the claim "adf_resid_reconstruct returned this status for this limit"
      (findings 1 and 2): the three hand certificates of the reviewer, a grid against the function itself,
      and the cost bound min(ell, X) on the input of finding 2.
   2. adf_linsolve_fball and adf_linsol_verify_fball decide DOMAIN, LIMIT and UNSUPPORTED before any
      allocation (finding 3), counted with the allocation hooks of FLINT. */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/fmpz_mat.h>

#include <adelefeld.h>

#include "test_runner.h"

/* ---- 1. the verifier ---- */

/* A claim on a hand certificate (Rp, Tp, R, T); q = qn/qd for OK. Returns what the verifier says. */
static int
verify_hand(slong m, slong c, slong A, slong B, slong limit, int status, slong qn, slong qd, slong Rp,
            slong Tp, slong R, slong T)
{
    adf_resid_t x;
    adf_rat_t q;
    adf_recon_cert_t cert;
    fmpz_t fm, fc, fA, fB;
    int got;

    adf_resid_init(x);
    adf_rat_init(q);
    adf_recon_cert_init(cert);
    fmpz_init_set_si(fm, m);
    fmpz_init_set_si(fc, c);
    fmpz_init_set_si(fA, A);
    fmpz_init_set_si(fB, B);
    ADF_CHECK(adf_resid_set_fmpz2(x, fc, fm) == ADF_OK);
    fmpz_set_si(fmpq_numref(q->q), qn);
    fmpz_set_si(fmpq_denref(q->q), qd);
    fmpz_set_si(cert->Rp, Rp);
    fmpz_set_si(cert->Tp, Tp);
    fmpz_set_si(cert->R, R);
    fmpz_set_si(cert->T, T);
    cert->kind = 1;
    got = adf_resid_verify_result(x, fA, fB, limit, status, q, cert);
    fmpz_clear(fm);
    fmpz_clear(fc);
    fmpz_clear(fA);
    fmpz_clear(fB);
    adf_recon_cert_clear(cert);
    adf_rat_clear(q);
    adf_resid_clear(x);
    return got;
}

static int
reconstruct_status(slong m, slong c, slong A, slong B, slong limit)
{
    adf_resid_t x;
    adf_rat_t q;
    adf_recon_cert_t cert;
    fmpz_t fm, fc, fA, fB;
    int st;

    adf_resid_init(x);
    adf_rat_init(q);
    adf_recon_cert_init(cert);
    fmpz_init_set_si(fm, m);
    fmpz_init_set_si(fc, c);
    fmpz_init_set_si(fA, A);
    fmpz_init_set_si(fB, B);
    adf_resid_set_fmpz2(x, fc, fm);
    st = adf_resid_reconstruct(q, cert, x, fA, fB, limit);
    fmpz_clear(fm);
    fmpz_clear(fc);
    fmpz_clear(fA);
    fmpz_clear(fB);
    adf_recon_cert_clear(cert);
    adf_rat_clear(q);
    adf_resid_clear(x);
    return st;
}

ADF_TEST(verify_result_hand_certificates_of_the_reviewer_limit_zero)
{
    /* (10, 1, 2, 5): the only reduced solution is 1/1; certificate (10, 0, 1, 1); X = 5 > 0 */
    ADF_CHECK(reconstruct_status(10, 1, 2, 5, 0) == ADF_NOT_DETERMINED);
    ADF_CHECK_MSG(verify_hand(10, 1, 2, 5, 0, ADF_OK, 1, 1, 10, 0, 1, 1) == 0, "OK accepted at limit 0");
    ADF_CHECK(verify_hand(10, 1, 2, 5, 0, ADF_NOT_DETERMINED, 1, 1, 10, 0, 1, 1) == 1);
    /* with a limit that reaches X = 5 the claim OK is what the function returns */
    ADF_CHECK(reconstruct_status(10, 1, 2, 5, 5) == ADF_OK);
    ADF_CHECK(verify_hand(10, 1, 2, 5, 5, ADF_OK, 1, 1, 10, 0, 1, 1) == 1);
    ADF_CHECK(reconstruct_status(10, 1, 2, 5, 4) == ADF_NOT_DETERMINED);
    ADF_CHECK_MSG(verify_hand(10, 1, 2, 5, 4, ADF_OK, 1, 1, 10, 0, 1, 1) == 0, "OK accepted with X > ell");
    ADF_CHECK(verify_hand(10, 1, 2, 5, 4, ADF_NOT_DETERMINED, 1, 1, 10, 0, 1, 1) == 1);

    /* (2, 1, 1, 1): both -1/1 and 1/1 solve it; certificate (2, 0, 1, 1) */
    ADF_CHECK(reconstruct_status(2, 1, 1, 1, 0) == ADF_NOT_DETERMINED);
    ADF_CHECK_MSG(verify_hand(2, 1, 1, 1, 0, ADF_NOT_UNIQUE, 0, 1, 2, 0, 1, 1) == 0,
                  "NOT_UNIQUE accepted at limit 0");
    ADF_CHECK(verify_hand(2, 1, 1, 1, 0, ADF_NOT_DETERMINED, 0, 1, 2, 0, 1, 1) == 1);
    ADF_CHECK(reconstruct_status(2, 1, 1, 1, 1) == ADF_NOT_UNIQUE);
    ADF_CHECK(verify_hand(2, 1, 1, 1, 1, ADF_NOT_UNIQUE, 0, 1, 2, 0, 1, 1) == 1);

    /* (4, 2, 1, 2): the only congruent pair in the box is 0/2, not reduced; certificate (2, 1, 0, -2) */
    ADF_CHECK(reconstruct_status(4, 2, 1, 2, 0) == ADF_NOT_DETERMINED);
    ADF_CHECK_MSG(verify_hand(4, 2, 1, 2, 0, ADF_NO_SOLUTION, 0, 1, 2, 1, 0, -2) == 0,
                  "NO_SOLUTION accepted at limit 0");
    ADF_CHECK(verify_hand(4, 2, 1, 2, 0, ADF_NOT_DETERMINED, 0, 1, 2, 1, 0, -2) == 1);
    ADF_CHECK(reconstruct_status(4, 2, 1, 2, 1) == ADF_NO_SOLUTION);
    ADF_CHECK(verify_hand(4, 2, 1, 2, 1, ADF_NO_SOLUTION, 0, 1, 2, 1, 0, -2) == 1);
}

/* Every claim (status, q) on the grid is accepted exactly when it is what the function returned. */
ADF_TEST(verify_result_is_the_status_of_the_function_on_the_grid)
{
    static const slong limits[] = { -3, 0, 1, 2, 3, 7, 1000 };
    static const int stats[] = { ADF_OK, ADF_NO_SOLUTION, ADF_NOT_UNIQUE, ADF_NOT_DETERMINED };
    slong m, c, A, B, li, si, calls = 0, acc = 0, ref = 0;
    slong by_status[4] = { 0, 0, 0, 0 };
    adf_resid_t x;
    adf_rat_t q, qw;
    adf_recon_cert_t cert;
    fmpz_t fm, fc, fA, fB;

    adf_resid_init(x);
    adf_rat_init(q);
    adf_rat_init(qw);
    adf_recon_cert_init(cert);
    fmpz_init(fm);
    fmpz_init(fc);
    fmpz_init(fA);
    fmpz_init(fB);
    for (m = 1; m <= 14; m++)
        for (c = 0; c < m + 2; c++)
            for (A = -1; A <= m + 1; A++)
                for (B = -1; B <= 9; B++)
                    for (li = 0; li < 7; li++)
                    {
                        int st;

                        fmpz_set_si(fm, m);
                        fmpz_set_si(fc, c);
                        fmpz_set_si(fA, A);
                        fmpz_set_si(fB, B);
                        ADF_CHECK(adf_resid_set_fmpz2(x, fc, fm) == ADF_OK);
                        adf_rat_set_si(q, -777);
                        st = adf_resid_reconstruct(q, cert, x, fA, fB, limits[li]);
                        for (si = 0; si < 4; si++)
                        {
                            int want = (stats[si] == st);
                            int got = adf_resid_verify_result(x, fA, fB, limits[li], stats[si], q, cert);

                            calls++;
                            ADF_CHECK_MSG(got == want,
                                          "m=%ld c=%ld A=%ld B=%ld limit=%ld: function %d, claim %d, verifier %d",
                                          m, c, A, B, limits[li], st, stats[si], got);
                            if (got)
                                acc++;
                            else
                                ref++;
                        }
                        by_status[st == ADF_OK ? 0 : st == ADF_NO_SOLUTION ? 1 : st == ADF_NOT_UNIQUE ? 2 : 3]++;
                        if (st == ADF_OK)
                        {
                            /* a wrong q on the claim OK is refused */
                            adf_rat_set_si(qw, 12345);
                            ADF_CHECK(adf_resid_verify_result(x, fA, fB, limits[li], ADF_OK, qw, cert) == 0);
                            fmpz_neg(fmpq_numref(q->q), fmpq_numref(q->q));
                            if (!fmpz_is_zero(fmpq_numref(q->q)))
                                ADF_CHECK(adf_resid_verify_result(x, fA, fB, limits[li], ADF_OK, q, cert) == 0);
                        }
                    }
    printf("   grid: %ld claims, %ld accepted, %ld refused; results OK %ld, NO_SOLUTION %ld, NOT_UNIQUE %ld, "
           "NOT_DETERMINED %ld\n", calls, acc, ref, by_status[0], by_status[1], by_status[2], by_status[3]);
    ADF_CHECK(by_status[0] > 500 && by_status[1] > 500 && by_status[2] > 500 && by_status[3] > 500);
    ADF_CHECK(acc == calls / 4);
    fmpz_clear(fm);
    fmpz_clear(fc);
    fmpz_clear(fA);
    fmpz_clear(fB);
    adf_recon_cert_clear(cert);
    adf_rat_clear(qw);
    adf_rat_clear(q);
    adf_resid_clear(x);
}

/* Finding 2: m = 2, c = 0, A = 1, B = 5e9, limit 0: the claim OK is false (the function returns
   NOT_DETERMINED) and is refused without the 5e9 rounds; the cost is min(ell, X) rounds. */
ADF_TEST(verify_result_cost_is_bounded_by_the_limit)
{
    adf_resid_t x;
    adf_rat_t q;
    adf_recon_cert_t cert;
    fmpz_t fm, fc, fA, fB;
    clock_t t0;
    double secs;
    int st;

    adf_resid_init(x);
    adf_rat_init(q);
    adf_recon_cert_init(cert);
    fmpz_init_set_si(fm, 2);
    fmpz_init_set_si(fc, 0);
    fmpz_init_set_si(fA, 1);
    fmpz_init(fB);
    ADF_CHECK(fmpz_set_str(fB, "5000000000", 10) == 0);
    ADF_CHECK(adf_resid_set_fmpz2(x, fc, fm) == ADF_OK);
    adf_rat_set_si(q, 0);
    st = adf_resid_reconstruct(q, cert, x, fA, fB, 0);
    ADF_CHECK(st == ADF_NOT_DETERMINED);
    ADF_CHECK(fmpz_equal_si(cert->Rp, 2) && fmpz_is_zero(cert->Tp) && fmpz_is_zero(cert->R)
              && fmpz_is_one(cert->T));
    t0 = clock();
    ADF_CHECK(adf_resid_verify_result(x, fA, fB, 0, ADF_OK, q, cert) == 0);
    ADF_CHECK(adf_resid_verify_result(x, fA, fB, 0, ADF_NO_SOLUTION, q, cert) == 0);
    ADF_CHECK(adf_resid_verify_result(x, fA, fB, 0, ADF_NOT_UNIQUE, q, cert) == 0);
    ADF_CHECK(adf_resid_verify_result(x, fA, fB, 0, ADF_NOT_DETERMINED, q, cert) == 1);
    /* a limit of 100000 rounds, X = 5e9: NOT_DETERMINED, in min(ell, X) = 100000 rounds */
    ADF_CHECK(adf_resid_reconstruct(q, cert, x, fA, fB, 100000) == ADF_NOT_DETERMINED);
    ADF_CHECK(adf_resid_verify_result(x, fA, fB, 100000, ADF_OK, q, cert) == 0);
    ADF_CHECK(adf_resid_verify_result(x, fA, fB, 100000, ADF_NOT_DETERMINED, q, cert) == 1);
    secs = (double) (clock() - t0) / CLOCKS_PER_SEC;
    printf("   verifier with B = 5e9: six calls, %.3f s of CPU\n", secs);
    ADF_CHECK_MSG(secs < 5.0, "the verifier took %.1f s: it does not stop at min(ell, X) rounds", secs);
    fmpz_clear(fm);
    fmpz_clear(fc);
    fmpz_clear(fA);
    fmpz_clear(fB);
    adf_recon_cert_clear(cert);
    adf_rat_clear(q);
    adf_resid_clear(x);
}

/* ---- 2. allocation before DOMAIN, LIMIT, UNSUPPORTED ---- */

static long n_malloc, n_calloc, n_realloc;
static void * count_malloc(size_t n) { n_malloc++; return malloc(n); }
static void * count_calloc(size_t n, size_t k) { n_calloc++; return calloc(n, k); }
static void * count_realloc(void * p, size_t n) { n_realloc++; return realloc(p, n); }
static void count_free(void * p) { free(p); }

#define ALLOC_ZERO(what) \
    do { \
        ADF_CHECK_MSG(n_malloc == 0 && n_calloc == 0 && n_realloc == 0, \
                      what ": %ld malloc, %ld calloc, %ld realloc", n_malloc, n_calloc, n_realloc); \
        n_malloc = n_calloc = n_realloc = 0; \
    } while (0)

ADF_TEST(linsolve_fball_refuses_before_any_allocation)
{
    slong rows = ADF_LINSOLVE_DIM_MAX + 1, i;
    adf_fball_struct * many = malloc((size_t) rows * sizeof(*many));
    adf_fball_struct * small = malloc(3 * sizeof(*small));
    fmpz_mat_t A, A5, A1;
    adf_linsol_t sol, copy;
    fmpz_t a, h, d;
    void * (*fm)(size_t);
    void * (*fc)(size_t, size_t);
    void * (*fr)(void *, size_t);
    void (*ff)(void *);
    int st;

    ADF_CHECK(many != NULL && small != NULL);
    if (many == NULL || small == NULL)
    {
        free(many);
        free(small);
        return;
    }
    fmpz_init(a);
    fmpz_init(h);
    fmpz_init(d);
    fmpz_set_si(h, 1);
    fmpz_set_si(d, 1);
    fmpz_mat_init(A, rows, 0);
    fmpz_mat_init(A5, 3, 2);
    fmpz_mat_init(A1, 3, 1);
    fmpz_set_si(fmpz_mat_entry(A1, 0, 0), 1);
    adf_linsol_init(sol);
    adf_linsol_init(copy);
    for (i = 0; i < rows; i++)
    {
        adf_fball_init(many + i);
        adf_fball_set_fmpz3(many + i, a, h, d);           /* 0 + Zhat: finite */
    }
    for (i = 0; i < 3; i++)
    {
        adf_fball_init(small + i);
        adf_fball_set_fmpz3(small + i, a, h, d);
    }
    __flint_get_memory_functions(&fm, &fc, &fr, &ff);
    __flint_set_memory_functions(count_malloc, count_calloc, count_realloc, count_free);
    n_malloc = n_calloc = n_realloc = 0;

    /* LIMIT */
    st = adf_linsolve_fball(sol, A, many, rows);
    ADF_CHECK(st == ADF_LIMIT);
    ADF_CHECK(adf_linsol_identical(sol, copy));
    ALLOC_ZERO("LIMIT");
    ADF_CHECK(adf_linsol_verify_fball(sol, A, many, rows) == 0);
    ALLOC_ZERO("verify LIMIT");

    /* an exact ball and r + c above the limit: LIMIT is decided first */
    fmpz_zero(h);
    adf_fball_set_fmpz3(many, a, h, d);
    st = adf_linsolve_fball(sol, A, many, rows);
    ADF_CHECK(st == ADF_LIMIT);
    ADF_CHECK(adf_linsol_identical(sol, copy));
    ALLOC_ZERO("LIMIT with an exact ball");

    /* UNSUPPORTED: the last of three balls is exact, the size is within the limit */
    adf_fball_set_fmpz3(small + 2, a, h, d);
    st = adf_linsolve_fball(sol, A5, small, 3);
    ADF_CHECK(st == ADF_UNSUPPORTED);
    ADF_CHECK(adf_linsol_identical(sol, copy));
    ALLOC_ZERO("UNSUPPORTED");
    ADF_CHECK(adf_linsol_verify_fball(sol, A5, small, 3) == 0);
    ALLOC_ZERO("verify UNSUPPORTED");

    /* DOMAIN: r is not the number of rows */
    st = adf_linsolve_fball(sol, A5, small, 2);
    ADF_CHECK(st == ADF_DOMAIN);
    ADF_CHECK(adf_linsol_identical(sol, copy));
    ALLOC_ZERO("DOMAIN");
    ADF_CHECK(adf_linsol_verify_fball(sol, A5, small, 2) == 0);
    ALLOC_ZERO("verify DOMAIN");
    __flint_set_memory_functions(fm, fc, fr, ff);

    /* the accepted case still works after the reordering: finite balls, one variable */
    fmpz_one(h);
    adf_fball_set_fmpz3(small + 2, a, h, d);
    st = adf_linsolve_fball(sol, A1, small, 3);
    ADF_CHECK(st == ADF_OK);
    ADF_CHECK(adf_linsol_verify_fball(sol, A1, small, 3) == 1);

    for (i = 0; i < rows; i++)
        adf_fball_clear(many + i);
    for (i = 0; i < 3; i++)
        adf_fball_clear(small + i);
    free(many);
    free(small);
    fmpz_mat_clear(A);
    fmpz_mat_clear(A5);
    fmpz_mat_clear(A1);
    adf_linsol_clear(sol);
    adf_linsol_clear(copy);
    fmpz_clear(a);
    fmpz_clear(h);
    fmpz_clear(d);
}
