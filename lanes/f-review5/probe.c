/* Review-only stream probe. Input: function p numerator denominator v M exact requested_N.
   Functions 0..3 are sin, cos, sinh, cosh; 4..6 compare exp, log, Log to 27f7e5f.
   Each input also exercises distinct/aliased local and named-place outputs. */
#include <adelefeld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int old_exp(adf_lball_t, const adf_lball_t, slong);
int old_log(adf_lball_t, const adf_lball_t, slong);
int old_Log(adf_lball_t, const adf_lball_t, slong);
typedef int (*local_fn)(adf_lball_t, const adf_lball_t, slong);
typedef int (*place_fn)(adf_sball_t, adf_place_t *, const adf_sball_t, adf_place_t, slong);
static local_fn fs[] = {adf_lball_sin, adf_lball_cos, adf_lball_sinh, adf_lball_cosh,
                       adf_lball_exp, adf_lball_log, adf_lball_Log};
static local_fn old[] = {old_exp, old_log, old_Log};
static place_fn ats[] = {adf_sball_sin_at, adf_sball_cos_at, adf_sball_sinh_at, adf_sball_cosh_at,
                        adf_sball_exp_at, adf_sball_log_at, adf_sball_Log_at};
static unsigned long rows, checks;
#define REQUIRE(e) do { checks++; if (!(e)) { \
    fprintf(stderr, "row=%lu line=%d failed: %s\n", rows, __LINE__, #e); exit(2); } } while (0)

int main(void)
{
    adf_lball_t x, y, alias, sentinel, historic;
    adf_lball_struct loc[2];
    adf_sball_t sx, sy, sa, saved;
    arb_t real;
    int f, exact;
    ulong p;
    slong v, M, N;
    char un[8192], ud[8192];
    adf_lball_init(x); adf_lball_init(y); adf_lball_init(alias);
    adf_lball_init(sentinel); adf_lball_init(historic);
    adf_lball_init(loc); adf_lball_init(loc+1);
    adf_sball_init(sx); adf_sball_init(sy); adf_sball_init(sa); adf_sball_init(saved);
    arb_init(real); arb_set_si(real, 7); arb_add_error_2exp_si(real, -4);
    sentinel->p = 19; fmpq_set_si(sentinel->u, -23, 29);
    loc[1].p = 17; fmpq_set_si(loc[1].u, 1, 1);
    REQUIRE(adf_sball_set_arb_lballs(saved, NULL, real, sentinel, 1) == ADF_OK);
    while (scanf("%d %lu %8191s %8191s %ld %ld %d %ld", &f, &p, un, ud, &v, &M, &exact, &N) == 8)
    {
        adf_place_t place, where, awhere, missing;
        adf_lball_struct before_y, before_alias;
        int st, ast, pst, past;
        rows++;
        REQUIRE(f >= 0 && f < 7);
        x->p = p; x->v = v; x->N = M; x->exact = exact;
        REQUIRE(fmpz_set_str(fmpq_numref(x->u), un, 10) == 0);
        REQUIRE(fmpz_set_str(fmpq_denref(x->u), ud, 10) == 0);
        REQUIRE(adf_lball_is_canonical(x));
        adf_lball_set(y, sentinel); adf_lball_set(alias, x);
        memcpy(&before_y, y, sizeof before_y); memcpy(&before_alias, alias, sizeof before_alias);
        st = fs[f](y, x, N); ast = fs[f](alias, alias, N);
        REQUIRE(ast == st);
        if (st == ADF_OK)
        {
            REQUIRE(adf_lball_identical(y, alias)); REQUIRE(adf_lball_is_canonical(y));
        }
        else
        {
            REQUIRE(adf_lball_identical(y, sentinel) && adf_lball_identical(alias, x));
            REQUIRE(memcmp(&before_y, y, sizeof before_y) == 0);
            REQUIRE(memcmp(&before_alias, alias, sizeof before_alias) == 0);
        }
        if (f >= 4)
        {
            adf_lball_set(historic, sentinel);
            REQUIRE(old[f-4](historic, x, N) == st);
            REQUIRE(adf_lball_identical(historic, y));
            adf_lball_set(historic, x);
            REQUIRE(old[f-4](historic, historic, N) == st);
            REQUIRE(adf_lball_identical(historic, alias));
        }
        REQUIRE(adf_place_prime(&place, p) == ADF_OK);
        adf_lball_set(loc, x);
        REQUIRE(adf_sball_set_arb_lballs(sx, NULL, real, loc, 2) == ADF_OK);
        /* Alternate REAL and COMPLEX tags. A complex real component must not poison a prime. */
        if (rows % 2) { sx->arch = ADF_ARCH_COMPLEX; arb_one(acb_imagref(sx->inf)); }
        REQUIRE(adf_sball_is_canonical(sx));
        adf_sball_set(sy, saved); adf_sball_set(sa, sx);
        where = awhere = adf_place_inf();
        pst = ats[f](sy, &where, sx, place, N); past = ats[f](sa, &awhere, sa, place, N);
        REQUIRE(pst == st && past == st);
        if (st == ADF_OK)
        {
            REQUIRE(sy->len == 1 && sy->arch == ADF_ARCH_NONE && acb_is_zero(sy->inf));
            REQUIRE(adf_lball_identical(sy->loc, y) && adf_sball_identical(sy, sa));
            REQUIRE(adf_place_is_archimedean(where) && adf_place_is_archimedean(awhere));
        }
        else
        {
            REQUIRE(adf_sball_identical(sy, saved) && adf_sball_identical(sa, sx));
            REQUIRE(adf_place_equal(where, place) && adf_place_equal(awhere, place));
        }
        REQUIRE(adf_place_prime(&missing, 23) == ADF_OK);
        adf_sball_set(sy, saved); where = adf_place_inf();
        REQUIRE(ats[f](sy, &where, sx, missing, N) == ADF_DOMAIN);
        REQUIRE(adf_sball_identical(sy, saved) && adf_place_equal(where, missing));
        printf("%d %lu ", st, y->p); fmpq_fprint(stdout, y->u);
        printf(" %ld %ld %d\n", y->v, y->N, y->exact);
    }
    REQUIRE(feof(stdin));
    fprintf(stderr, "rows=%lu checks=%lu failures=0\n", rows, checks);
    adf_lball_clear(x); adf_lball_clear(y); adf_lball_clear(alias);
    adf_lball_clear(sentinel); adf_lball_clear(historic);
    adf_lball_clear(loc); adf_lball_clear(loc+1);
    adf_sball_clear(sx); adf_sball_clear(sy); adf_sball_clear(sa); adf_sball_clear(saved);
    arb_clear(real); flint_cleanup();
    return 0;
}
