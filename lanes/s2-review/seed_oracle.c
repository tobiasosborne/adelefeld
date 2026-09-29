#include <adelefeld.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct { long c0, c1, c2; int degree, multiplicity; } factor;
typedef struct { int p; long scale; const factor * factors; int nf; } problem;

static const factor f0[] = {{-1,1,0,1,2},{1,1,0,1,1}};
static const factor f1[] = {{0,1,0,1,1},{-8,1,0,1,1}};
static const factor f2[] = {{-1,1,0,1,2},{1,2,0,1,1}};
static const factor f3[] = {{1,0,1,2,1},{-2,1,0,1,2}};
static const factor f4[] = {{1,0,1,2,2}};
static const factor f5[] = {{-3,1,0,1,2},{6,1,0,1,1}};
static const factor f6[] = {{0,1,0,1,1}};
static const problem cases[] = {
    {2, 1, f0, 2}, {2, 1, f1, 2}, {2, 6, f2, 2}, {2, 1, f3, 2},
    {5, 1, f4, 1}, {3, -9, f5, 2}, {3, 27, f6, 1}
};

static void make_polys(fmpz_poly_t f, fmpz_poly_t g, const problem * q)
{
    fmpz_poly_t h, t;
    fmpz_poly_init(h); fmpz_poly_init(t);
    fmpz_poly_one(f); fmpz_poly_one(g);
    for (int i = 0; i < q->nf; i++) {
        const factor * z = q->factors + i;
        fmpz_poly_zero(h);
        fmpz_poly_set_coeff_si(h, 0, z->c0);
        fmpz_poly_set_coeff_si(h, 1, z->c1);
        if (z->degree == 2) fmpz_poly_set_coeff_si(h, 2, z->c2);
        fmpz_poly_mul(t, g, h); fmpz_poly_swap(g, t);
        for (int j = 0; j < z->multiplicity; j++) {
            fmpz_poly_mul(t, f, h); fmpz_poly_swap(f, t);
        }
    }
    fmpz_poly_scalar_mul_si(t, f, q->scale); fmpz_poly_swap(f, t);
    fmpz_poly_primitive_part(t, g); fmpz_poly_swap(g, t);
    fmpz_poly_clear(h); fmpz_poly_clear(t);
}

static long valuation(const fmpz_t x, int p)
{
    fmpz_t t;
    long v = 0;
    if (fmpz_is_zero(x)) return 1000000;
    fmpz_init_set(t, x);
    while (fmpz_divisible_si(t, (slong) p)) {
        fmpz_divexact_ui(t, t, (ulong) p);
        v++;
    }
    fmpz_clear(t);
    return v;
}

static long power(int p, long e)
{
    long v = 1;
    while (e-- > 0) v *= p;
    return v;
}

static long mod(long a, long m)
{
    long r = a % m;
    return r < 0 ? r + m : r;
}

static long eval_mod(const fmpz_poly_t g, long x, long m)
{
    long y = 0;
    for (long i = fmpz_poly_degree(g); i >= 0; i--)
        y = mod(y * x + fmpz_get_si(g->coeffs + i), m);
    return y;
}

int main(void)
{
    fmpz_poly_t f, g, d;
    fmpz_t seed, gs, ds;
    adf_rootlist_t L;
    long calls = 0, ok = 0, nd = 0, enumerated = 0, skipped = 0, bad = 0;
    long s_positive = 0, direct_path = 0;
    fmpz_poly_init(f); fmpz_poly_init(g); fmpz_poly_init(d);
    fmpz_init(seed); fmpz_init(gs); fmpz_init(ds); adf_rootlist_init(L);
    for (size_t c = 0; c < sizeof(cases)/sizeof(cases[0]); c++) {
        const problem * q = cases + c;
        adf_place_t place;
        int reduced_expected = 0;
        for (int j = 0; j < q->nf; j++)
            if (q->factors[j].multiplicity > 1) reduced_expected = 1;
        if (adf_place_prime(&place, (ulong) q->p) != ADF_OK) abort();
        make_polys(f, g, q);
        fmpz_poly_derivative(d, g);
        for (long a = -24; a <= 24; a++) for (long prec = 1; prec <= 5; prec++) {
            long s, vg, K, m, step, count;
            int strong, status;
            fmpz_set_si(seed, a);
            fmpz_poly_evaluate_fmpz(gs, g, seed);
            fmpz_poly_evaluate_fmpz(ds, d, seed);
            s = valuation(ds, q->p); vg = valuation(gs, q->p);
            strong = s != 1000000 && vg > 2*s;
            status = adf_root_padic_from_seed(L, f, place, seed, prec);
            calls++;
            if (status != (strong ? ADF_OK : ADF_NOT_DETERMINED)) {
                printf("status mismatch case=%zu p=%d a=%ld prec=%ld status=%d strong=%d\n",
                       c, q->p, a, prec, status, strong);
                bad++; continue;
            }
            if (!strong) { nd++; continue; }
            ok++;
            if (s > 0) s_positive++;
            K = prec > s ? prec : s+1;
            if (vg - s >= K) direct_path++;
            if (!fmpz_poly_equal(L->g, g) || L->reduced != reduced_expected ||
                L->K[0] != K || L->s[0] != s ||
                L->n != 1 || L->nu != 0 || L->scope != ADF_ROOTLIST_SEED || L->complete != 0 ||
                !adf_rootlist_is_canonical(L) || !adf_rootlist_verify_entries(L, f)) {
                printf("metadata mismatch case=%zu p=%d a=%ld prec=%ld\n", c, q->p, a, prec);
                bad++; continue;
            }
            if (mod(fmpz_get_si(L->a), power(q->p, s+1)) != mod(a, power(q->p, s+1))) {
                printf("wrong seed class case=%zu p=%d a=%ld prec=%ld\n", c, q->p, a, prec);
                bad++; continue;
            }
            if (K+s+2 > 14 || power(q->p, K+s+2) > 100000) { skipped++; continue; }
            m = power(q->p, K+s+2); step = power(q->p, s+1); count = 0;
            for (long x = mod(a, step); x < m; x += step) {
                if (eval_mod(g, x, m) == 0) {
                    count++;
                    if (mod(x, power(q->p, K)) != fmpz_get_si(L->a)) {
                        printf("wrong root class case=%zu p=%d a=%ld prec=%ld x=%ld\n",
                               c, q->p, a, prec, x);
                        bad++; break;
                    }
                }
            }
            enumerated++;
            if (count != power(q->p, s)) {
                printf("wrong root count case=%zu p=%d a=%ld prec=%ld count=%ld want=%ld\n",
                       c, q->p, a, prec, count, power(q->p, s));
                bad++;
            }
        }
    }
    printf("cases=%zu calls=%ld OK=%ld NOT_DETERMINED=%ld s_positive=%ld direct_path=%ld "
           "enumerated=%ld skipped=%ld failures=%ld\n",
           sizeof(cases)/sizeof(cases[0]), calls, ok, nd, s_positive, direct_path,
           enumerated, skipped, bad);
    adf_rootlist_clear(L); fmpz_poly_clear(f); fmpz_poly_clear(g); fmpz_poly_clear(d);
    fmpz_clear(seed); fmpz_clear(gs); fmpz_clear(ds);
    return bad ? 1 : 0;
}
