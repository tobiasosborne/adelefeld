/* Real-place dispatch, aliasing, finiteness and precision checks.
   The arb enclosure contract is refs/src/flint-3.0.1/arb.rst:6-12;
   hyperbolic calls are documented at 1209-1219. This tests the wrapper, not arb itself. */
#include <adelefeld.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

typedef int (*place_fn)(adf_sball_t, adf_place_t *, const adf_sball_t, adf_place_t, slong);
typedef void (*arb_fn)(arb_t, const arb_t, slong);
static place_fn fs[] = {adf_sball_sin_at, adf_sball_cos_at, adf_sball_sinh_at, adf_sball_cosh_at};
static arb_fn afs[] = {arb_sin, arb_cos, arb_sinh, arb_cosh};
static unsigned long rows, checks, finite, lost, limits, unsupported, absent;
#define REQUIRE(e) do { checks++; if (!(e)) { \
    fprintf(stderr, "row=%lu line=%d failed: %s\n", rows, __LINE__, #e); exit(2); } } while (0)

int main(void)
{
    adf_sball_t x, y, alias, saved;
    adf_lball_t local;
    arb_t a, ref;
    adf_place_t inf = adf_place_inf(), prime, where, awhere;
    slong precs[] = {LONG_MIN, 0, 2, 17, 80, 257, ADF_REAL_PREC_MAX+1};
    adf_sball_init(x); adf_sball_init(y); adf_sball_init(alias); adf_sball_init(saved);
    adf_lball_init(local); arb_init(a); arb_init(ref);
    local->p = 5; fmpq_set_si(local->u, 1, 1);
    REQUIRE(adf_place_prime(&prime, 5) == ADF_OK);
    arb_set_si(a, 29);
    REQUIRE(adf_sball_set_arb_lballs(saved, NULL, a, local, 1) == ADF_OK);
    for (int kind = 0; kind < 3; kind++)
    for (int j = 0; j < 15; j++)
    for (size_t ip = 0; ip < sizeof precs / sizeof *precs; ip++)
    for (int f = 0; f < 4; f++)
    {
        int want, st, ast;
        slong prec = precs[ip];
        rows++;
        arb_set_si(a, j-6);
        if (j % 3 == 0) arb_add_error_2exp_si(a, 2);
        if (j % 3 == 1) arb_add_error_2exp_si(a, -20);
        if (j >= 12)
        {
            arb_one(a); arb_mul_2exp_si(a, a, j == 12 ? 60 : 1000);
            if (j == 14) arb_neg(a, a);
        }
        REQUIRE(adf_sball_set_arb_lballs(x, NULL, kind == 2 ? NULL : a, local, 1) == ADF_OK);
        if (kind == 1) { x->arch = ADF_ARCH_COMPLEX; arb_one(acb_imagref(x->inf)); }
        if (prec > ADF_REAL_PREC_MAX) { want = ADF_LIMIT; limits++; }
        else if (kind == 1) { want = ADF_UNSUPPORTED; unsupported++; }
        else if (kind == 2) { want = ADF_DOMAIN; absent++; }
        else
        {
            afs[f](ref, a, prec < 2 ? 2 : prec);
            want = arb_is_finite(ref) ? ADF_OK : ADF_NOT_DETERMINED;
            if (want == ADF_OK) finite++; else lost++;
        }
        adf_sball_set(y, saved); adf_sball_set(alias, x); where = awhere = prime;
        st = fs[f](y, &where, x, inf, prec); ast = fs[f](alias, &awhere, alias, inf, prec);
        REQUIRE(st == want && ast == want);
        if (want == ADF_OK)
        {
            REQUIRE(y->arch == ADF_ARCH_REAL && y->len == 0 && arb_is_zero(acb_imagref(y->inf)));
            REQUIRE(arb_equal(acb_realref(y->inf), ref) && adf_sball_identical(y, alias));
            REQUIRE(adf_place_equal(where, prime) && adf_place_equal(awhere, prime));
        }
        else
        {
            REQUIRE(adf_sball_identical(y, saved) && adf_sball_identical(alias, x));
            REQUIRE(adf_place_equal(where, inf) && adf_place_equal(awhere, inf));
        }
    }
    /* The admitted maximum itself: exact zero avoids making this a large computation. */
    arb_zero(a);
    REQUIRE(adf_sball_set_arb_lballs(x, NULL, a, NULL, 0) == ADF_OK);
    for (int f = 0; f < 4; f++)
    {
        rows++;
        REQUIRE(fs[f](y, NULL, x, inf, ADF_REAL_PREC_MAX) == ADF_OK);
        REQUIRE(f % 2 ? arb_is_one(acb_realref(y->inf)) : arb_is_zero(acb_realref(y->inf)));
    }
    printf("rows=%lu checks=%lu finite=%lu lost=%lu limits=%lu unsupported=%lu absent=%lu failures=0\n",
           rows, checks, finite, lost, limits, unsupported, absent);
    adf_sball_clear(x); adf_sball_clear(y); adf_sball_clear(alias); adf_sball_clear(saved);
    adf_lball_clear(local); arb_clear(a); arb_clear(ref); flint_cleanup();
    return 0;
}
