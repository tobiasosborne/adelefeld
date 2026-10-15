/* Slice 5b, docs/api-5.md:48-118 (section 2), :327-336 (5.1 acceptance); docs/api-5s.md "Slice 5b".
   adf_local_gamma_at and adf_local_epsilon_at: P9 (analysis.md:331-400) and P10 (:401-451);
   conventions 6.4 (local constants, the negative kernel G_minus, the real place).
   Every row of tests/ref/vectors/t5-slice3/gamma.jsonl and every certified sample enclosure is read
   (lanes/t5-slice3/generate_vectors.py). Width target, the same as slice 5a's:
   2*max(radius) <= 64*sampled diameter + 2^(-prec+8)*max(1, image), at prec = 128. */
#include <adelefeld.h>
#include <adelefeld/localfactor.h>
#include <string.h>
#include <stdlib.h>
#include <flint/ulong_extras.h>
#ifdef ADF_CHECK_INVARIANTS
#include <signal.h>
#include <sys/resource.h>
#ifdef __linux__
#include <sys/prctl.h>
#endif
#include <sys/wait.h>
#include <unistd.h>
#endif
#include "support/jsonl.h"
#include "test_runner.h"

typedef int (*local_fn)(acb_t, adf_place_t *, const acb_t, adf_place_t, const acb_t, const adf_char_t, slong);

/* Inactive arf bytes are defined before init, so that byte snapshots are meaningful to memory checkers. */
static void initialized_acb(acb_t x)
{
    memset(x, 0, sizeof(acb_struct));
    acb_init(x);
}

/* The test owns no live objects at exit; release FLINT's caches too (tools/memcheck). */
static void __attribute__((destructor)) cleanup_flint_test_cache(void)
{
    flint_cleanup();
}

static adf_place_t place(ulong p)
{
    adf_place_t v = adf_place_inf();
    if (p) ADF_CHECK(adf_place_prime(&v, p) == ADF_OK);
    return v;
}

static const jsonl_value *field(const jsonl_value *r, const char *key)
{
    jsonl_error_t err;
    const jsonl_value *v = NULL;
    ADF_CHECK(jsonl_field(r, key, &v, &err));
    return v;
}

static const char *string(const jsonl_value *v)
{
    jsonl_error_t err;
    return jsonl_string(v, NULL, &err);
}

static ulong number(const jsonl_value *v)
{
    jsonl_error_t err;
    return strtoul(jsonl_int_text(v, &err), NULL, 10);
}

static const jsonl_value *at(const jsonl_value *v, size_t i)
{
    jsonl_error_t err;
    return jsonl_at(v, i, &err);
}

static void rational(arb_t v, const char *s)
{
    fmpq_t q;
    fmpq_init(q);
    ADF_CHECK(fmpq_set_str(q, s, 10) == 0);
    fmpq_canonicalise(q);
    arb_set_fmpq(v, q, 4096);
    fmpq_clear(q);
}

static void input(acb_t z, const jsonl_value *mid, const jsonl_value *rad)
{
    arb_t r;
    arb_init(r);
    for (int j = 0; j < 2; j++)
    {
        arb_ptr c = j ? acb_imagref(z) : acb_realref(z);
        rational(c, string(at(mid, j)));
        rational(r, string(at(rad, j)));
        arb_add_error(c, r);
    }
    arb_clear(r);
}

/* Every call is repeated with z separate, z = s, z = alpha and where = NULL. A failure must leave the output's
   bytes untouched and write v to where; OK must leave where untouched and give identical results. */
static int call(local_fn f, acb_t y, const acb_t s, adf_place_t v, const acb_t a, const adf_char_t chi, slong prec)
{
    acb_t t, ss, aa, first;
    int first_st = -1;
    initialized_acb(t); initialized_acb(ss); initialized_acb(aa); initialized_acb(first);
    for (int mode = 0; mode < 4; mode++)
    {
        unsigned char before[sizeof(acb_struct)];
        adf_place_t w = place(97), mark = w;
        acb_ptr out;
        int st;
        acb_set(ss, s); acb_set(aa, a);
        acb_set_si_si(t, 7, -5);
        mag_set_ui_2exp_si(arb_radref(acb_realref(t)), 3, -12);
        mag_set_ui_2exp_si(arb_radref(acb_imagref(t)), 5, -11);
        out = mode == 1 ? ss : mode == 2 ? aa : t;
        memcpy(before, out, sizeof(before));
        st = f(out, mode == 3 ? NULL : &w, ss, v, aa, chi, prec);
        if (mode == 0) { first_st = st; acb_set(first, out); }
        else ADF_CHECK(st == first_st);
        if (st == ADF_OK)
        {
            ADF_CHECK(acb_is_finite(out));
            ADF_CHECK(acb_equal(first, out));
            if (mode != 3) ADF_CHECK(adf_place_equal(w, mark));
        }
        else
        {
            ADF_CHECK(memcmp(before, out, sizeof(before)) == 0);
            if (mode != 3) ADF_CHECK(adf_place_equal(w, v));
        }
    }
    if (first_st == ADF_OK) acb_set(y, first);
    acb_clear(t); acb_clear(ss); acb_clear(aa); acb_clear(first);
    return first_st;
}

static int gamma_call(acb_t y, const acb_t s, adf_place_t v, const acb_t a, const adf_char_t chi, slong prec)
{
    return call(adf_local_gamma_at, y, s, v, a, chi, prec);
}

static int epsilon_call(acb_t y, const acb_t s, adf_place_t v, const acb_t a, const adf_char_t chi, slong prec)
{
    return call(adf_local_epsilon_at, y, s, v, a, chi, prec);
}

/* A ball of radius 2^-k on both coordinates around the integer n + i m, shifted by d 2^-10 on the real axis. */
static void point(acb_t s, slong n, slong m, slong d, slong k)
{
    acb_set_si_si(s, n, m);
    if (d) { arb_t h; arb_init(h); arb_set_si(h, d); arb_mul_2exp_si(h, h, -10);
             arb_add(acb_realref(s), acb_realref(s), h, 4096); arb_clear(h); }
    if (k >= 0)
    {
        mag_set_ui_2exp_si(arb_radref(acb_realref(s)), 1, -k);
        mag_set_ui_2exp_si(arb_radref(acb_imagref(s)), 1, -k);
    }
}

ADF_TEST(oracle_vectors)
{
    jsonl_file *file = NULL;
    jsonl_error_t err;
    acb_t s, a, y, ref;
    arb_t target, x;
    adf_char_t chi;
    size_t samples = 0, rows = 0;
    initialized_acb(s); initialized_acb(a); initialized_acb(y); initialized_acb(ref);
    arb_init(target); arb_init(x); adf_char_init(chi);
    ADF_CHECK(jsonl_open("tests/ref/vectors/t5-slice3/gamma.jsonl", &file, &err));
    if (!file) goto done;
    rows = jsonl_count(file);
    for (size_t i = 0; i < rows; i++)
    {
        const jsonl_value *r = jsonl_record(file, i);
        ulong q = number(field(r, "q")), n = number(field(r, "n"));
        int expected = (int) number(field(r, "status")), st;
        int is_gamma = strcmp(string(field(r, "fn")), "gamma") == 0;
        adf_place_t v = place(number(field(r, "p")));
        input(s, field(r, "s"), field(r, "sr"));
        input(a, field(r, "alpha"), field(r, "ar"));
        if (q <= ADF_CHAR_MOD_MAX)
        {
            ADF_CHECK(adf_char_set_conrey(chi, q, n) == ADF_OK);
            ADF_CHECK(adf_char_get_conductor(chi) == q);
            ADF_CHECK((ulong) adf_char_get_parity(chi) == number(field(r, "e")) || number(field(r, "p")) != 0);
        }
        else { chi->q = q; chi->n = n; }
        st = is_gamma ? gamma_call(y, s, v, a, chi, 128) : epsilon_call(y, s, v, a, chi, 128);
        /* The oversize descriptor is refused before INV; restore it before the lifecycle checks. */
        if (q > ADF_CHAR_MOD_MAX) { chi->q = 1; chi->n = 1; chi->parity = 0; }
        ADF_CHECK_MSG(st == expected, "vector %zu: %d expected %d", i, st, expected);
        if (st == ADF_OK && expected == ADF_OK)
        {
            const jsonl_value *pts = field(r, "samples");
            size_t count = jsonl_size(pts);
            for (size_t j = 0; j < count; j++)
            {
                const jsonl_value *pt = at(pts, j);
                ADF_CHECK(arb_set_str(acb_realref(ref), string(at(pt, 0)), 512) == 0);
                ADF_CHECK(arb_set_str(acb_imagref(ref), string(at(pt, 1)), 512) == 0);
                ADF_CHECK_MSG(acb_contains(y, ref), "vector %zu sample %zu", i, j);
                samples++;
            }
            rational(target, string(field(r, "width_lower")));
            arb_mul_ui(target, target, 64, 4096);
            rational(x, string(field(r, "image")));
            arb_mul_2exp_si(x, x, -120);
            arb_add(target, target, x, 4096);
            for (int j = 0; j < 2; j++)
            {
                arf_set_mag(arb_midref(x), arb_radref(j ? acb_imagref(y) : acb_realref(y)));
                mag_zero(arb_radref(x));
                arb_mul_2exp_si(x, x, 1);
                ADF_CHECK_MSG(arb_le(x, target), "vector %zu width %d", i, j);
            }
        }
    }
    printf("vectors: %zu; certified samples: %zu\n", rows, samples);
    ADF_CHECK(rows == 721); ADF_CHECK(samples == 1950);
    jsonl_close(file);
done:
    acb_clear(s); acb_clear(a); acb_clear(y); acb_clear(ref);
    arb_clear(target); arb_clear(x); adf_char_clear(chi);
}

/* P9 (analysis.md:344), a = 0: gamma_p = (1 - alpha p^-s)/(1 - alpha^-1 p^(s-1)). By hand:
   p = 2, alpha = 1, s = 2:  (1 - 1/4)/(1 - 2)            = -3/4;
   p = 3, alpha = -1, s = -1: (1 + 3)/(1 + 1/9)            = 18/5;
   p = 5, alpha = i, s = 0:   (1 - i)/(1 - 1/(5i)) = (1 - i)/(1 + i/5) = (1 - i)(1 - i/5)(25/26) = (10 - 15i)/13;
   p = 2, alpha = 1, s = 0:   (1 - 1)/(1 - 1/2)            = 0 (the integral's pole is a zero of gamma). */
ADF_TEST(unramified_exact_quotients)
{
    const slong cases[4][6] = {{2, 1, 0, 2, -3, 4}, {3, -1, 0, -1, 18, 5}, {5, 0, 1, 0, 10, 13},
                               {2, 1, 0, 0, 0, 1}};
    acb_t s, a, y, ref;
    adf_char_t chi;
    initialized_acb(s); initialized_acb(a); initialized_acb(y); initialized_acb(ref); adf_char_init(chi);
    for (int j = 0; j < 4; j++)
    {
        acb_set_si_si(a, cases[j][1], cases[j][2]);
        acb_set_si(s, cases[j][3]);
        arb_set_si(acb_realref(ref), cases[j][4]);
        arb_div_si(acb_realref(ref), acb_realref(ref), cases[j][5], 512);
        arb_set_si(acb_imagref(ref), j == 2 ? -15 : 0);
        arb_div_si(acb_imagref(ref), acb_imagref(ref), cases[j][5], 512);
        ADF_CHECK(gamma_call(y, s, place((ulong) cases[j][0]), a, chi, 128) == ADF_OK);
        ADF_CHECK_MSG(acb_contains(y, ref), "case %d", j);
        ADF_CHECK(mag_cmp_2exp_si(arb_radref(acb_realref(y)), -120) <= 0);
        ADF_CHECK(mag_cmp_2exp_si(arb_radref(acb_imagref(y)), -120) <= 0);
    }
    acb_clear(s); acb_clear(a); acb_clear(y); acb_clear(ref); adf_char_clear(chi);
}

/* G_minus(eta0^-1) = sum_(u mod q, p not | u) eta0(u)^-1 E(-u/q) (P9:345-346). Reference: conj(eta0) through
   adf_char_conj and adf_char_chi, E(-u/q) through acb_exp_pi_i. Wrong variants: the positive kernel E(+u/q),
   and eta0 for eta0^-1. For a non-real eta0 all three differ. Lemma 8 (:308, m = -1) gives a fourth check:
   sum conj(eta0)(u) E(-u/q) = conj(conj(eta0)(-1)) tau(conj(eta0)) = (-1)^e tau(conj(eta0)). */
ADF_TEST(ramified_negative_sum)
{
    const ulong chars[6][3] = {{5, 2, 5}, {13, 2, 13}, {16, 5, 2}, {9, 2, 3}, {25, 2, 5}, {4, 3, 2}};
    acb_t s, a, y, g, pos, same, term, e, scale, tau, expected;
    adf_char_t chi, inv;
    fmpz_t u;
    initialized_acb(s); initialized_acb(a); initialized_acb(y); initialized_acb(g); initialized_acb(pos);
    initialized_acb(same); initialized_acb(term); initialized_acb(e); initialized_acb(scale); initialized_acb(tau);
    initialized_acb(expected); adf_char_init(chi); adf_char_init(inv); fmpz_init(u);
    for (int j = 0; j < 6; j++)
    {
        ulong q = chars[j][0], p = chars[j][2], k = 0;
        for (ulong r = q; r > 1; r /= p) k++;
        ADF_CHECK(adf_char_set_conrey(chi, q, chars[j][1]) == ADF_OK);
        adf_char_conj(inv, chi);
        acb_zero(g); acb_zero(pos); acb_zero(same);
        for (ulong m = 1; m < q; m++)
        {
            if (m % p == 0) continue;
            fmpz_set_ui(u, m);
            acb_set_si(e, -2 * (slong) m); acb_div_ui(e, e, q, 256); acb_exp_pi_i(e, e, 256);
            ADF_CHECK(adf_char_chi(term, inv, u, 256) == ADF_OK);
            acb_mul(term, term, e, 256); acb_add(g, g, term, 256);
            ADF_CHECK(adf_char_chi(term, chi, u, 256) == ADF_OK);
            acb_mul(term, term, e, 256); acb_add(same, same, term, 256);
            acb_conj(e, e);
            ADF_CHECK(adf_char_chi(term, inv, u, 256) == ADF_OK);
            acb_mul(term, term, e, 256); acb_add(pos, pos, term, 256);
        }
        ADF_CHECK(adf_char_gauss_sum(tau, inv, 256) == ADF_OK);
        if (chi->parity) acb_neg(tau, tau);
        ADF_CHECK(acb_overlaps(g, tau));
        /* alpha^a p^(-a s) with alpha = 1/2 + i, s = 1/2 + 3i/4. */
        acb_set_si_si(a, 1, 2); acb_mul_2exp_si(a, a, -1);
        acb_set_si_si(s, 2, 3); acb_mul_2exp_si(s, s, -2);
        acb_pow_ui(scale, a, k, 256);
        acb_set_ui(e, p); acb_log(e, e, 256); acb_mul(e, e, s, 256); acb_mul_ui(e, e, k, 256); acb_neg(e, e);
        acb_exp(e, e, 256); acb_mul(scale, scale, e, 256);
        ADF_CHECK(gamma_call(y, s, place(p), a, chi, 128) == ADF_OK);
        acb_mul(expected, scale, g, 256);
        ADF_CHECK_MSG(acb_overlaps(y, expected), "char %d", j);
        if (q != 4)   /* (4,3) is real: the variants coincide up to the parity sign, tested below */
        {
            acb_mul(expected, scale, same, 256);
            ADF_CHECK_MSG(!acb_overlaps(y, expected), "char %d eta0 for eta0^-1", j);
        }
        acb_mul(expected, scale, pos, 256);
        ADF_CHECK_MSG(chi->parity == 0 || !acb_overlaps(y, expected), "char %d positive kernel", j);
        ADF_CHECK(epsilon_call(g, s, place(p), a, chi, 128) == ADF_OK);
        ADF_CHECK(acb_equal(g, y));
    }
    acb_clear(s); acb_clear(a); acb_clear(y); acb_clear(g); acb_clear(pos); acb_clear(same); acb_clear(term);
    acb_clear(e); acb_clear(scale); acb_clear(tau); acb_clear(expected);
    adf_char_clear(chi); adf_char_clear(inv); fmpz_clear(u);
}

/* epsilon (P9:350-351, P10:410): exactly 1 unramified, exactly i^e at infinity, also at gamma's poles. */
ADF_TEST(epsilon_exact)
{
    acb_t s, a, y, i;
    adf_char_t chi;
    initialized_acb(s); initialized_acb(a); initialized_acb(y); initialized_acb(i); adf_char_init(chi);
    acb_onei(i);
    acb_set_si_si(s, 1, 3); acb_set_si_si(a, 2, -1);
    for (ulong p = 2; p < 14; p += 1 + (p > 2))
    {
        if (!n_is_prime(p)) continue;
        ADF_CHECK(epsilon_call(y, s, place(p), a, chi, 128) == ADF_OK);
        ADF_CHECK(acb_is_one(y));
    }
    /* at gamma's poles: p = 3, alpha = 1, s = 1; p = 2, alpha = 1/2, s = 0; a ball meeting s = 1 */
    acb_one(a); acb_one(s);
    ADF_CHECK(epsilon_call(y, s, place(3), a, chi, 128) == ADF_OK); ADF_CHECK(acb_is_one(y));
    point(s, 1, 0, 0, 10);
    ADF_CHECK(epsilon_call(y, s, place(3), a, chi, 128) == ADF_OK); ADF_CHECK(acb_is_one(y));
    acb_one(a); acb_mul_2exp_si(a, a, -1); acb_zero(s);
    ADF_CHECK(epsilon_call(y, s, place(2), a, chi, 128) == ADF_OK); ADF_CHECK(acb_is_one(y));
    for (int e = 0; e < 2; e++)
    {
        ADF_CHECK(adf_char_set_conrey(chi, e ? 4 : 1, e ? 3 : 1) == ADF_OK);
        for (slong k = 0; k < 3; k++)
        {
            acb_set_si(s, 1 + e + 2 * k);
            ADF_CHECK(epsilon_call(y, s, place(0), a, chi, 128) == ADF_OK);
            ADF_CHECK(e ? acb_equal(y, i) : acb_is_one(y));
            point(s, 1 + e + 2 * k, 0, 0, 8);
            ADF_CHECK(epsilon_call(y, s, place(0), a, chi, 128) == ADF_OK);
            ADF_CHECK(e ? acb_equal(y, i) : acb_is_one(y));
        }
        acb_set_si_si(s, -7, 2);
        ADF_CHECK(epsilon_call(y, s, place(0), a, chi, 2) == ADF_OK);
        ADF_CHECK(e ? acb_equal(y, i) : acb_is_one(y));
        ADF_CHECK(epsilon_call(y, s, place(0), a, chi, ADF_REAL_PREC_MAX) == ADF_OK);
        ADF_CHECK(e ? acb_equal(y, i) : acb_is_one(y));
    }
    acb_clear(s); acb_clear(a); acb_clear(y); acb_clear(i); adf_char_clear(chi);
}

/* P10 (analysis.md:406; api-5.md:78): gamma_inf = i^e pi^(s-1/2) Gamma((1-s+e)/2) rgamma((s+e)/2). By hand at
   s = 1/2: e = 0 gives pi^0 Gamma(1/4) rgamma(1/4) = 1; e = 1 gives i pi^0 Gamma(3/4) rgamma(3/4) = i.
   Zeros at s = -e-2k (rgamma at (s+e)/2 = -k): exactly 0, including huge exact zeros. */
ADF_TEST(real_values_and_zeros)
{
    acb_t s, a, y, ref;
    adf_char_t chi;
    initialized_acb(s); initialized_acb(a); initialized_acb(y); initialized_acb(ref); adf_char_init(chi);
    for (int e = 0; e < 2; e++)
    {
        ADF_CHECK(adf_char_set_conrey(chi, e ? 4 : 1, e ? 3 : 1) == ADF_OK);
        acb_one(s); acb_mul_2exp_si(s, s, -1);
        arf_nan(arb_midref(acb_realref(a)));   /* alpha is ignored at infinity, even nonfinite */
        ADF_CHECK(gamma_call(y, s, place(0), a, chi, 128) == ADF_OK);
        if (e) acb_onei(ref); else acb_one(ref);
        ADF_CHECK(acb_contains(y, ref));
        ADF_CHECK(mag_cmp_2exp_si(arb_radref(acb_realref(y)), -120) <= 0);
        ADF_CHECK(mag_cmp_2exp_si(arb_radref(acb_imagref(y)), -120) <= 0);
        for (slong k = 0; k < 4; k++)
        {
            acb_set_si(s, -e - 2 * k);
            ADF_CHECK(gamma_call(y, s, place(0), a, chi, 128) == ADF_OK);
            ADF_CHECK_MSG(acb_is_zero(y), "zero e=%d k=%ld", e, (long) k);
            point(s, -e - 2 * k, 0, 0, 12);   /* a ball around a zero: finite, contains 0 */
            ADF_CHECK(gamma_call(y, s, place(0), a, chi, 128) == ADF_OK);
            ADF_CHECK(acb_contains_zero(y));
            point(s, -e - 2 * k, 0, 1, 12);
            ADF_CHECK(gamma_call(y, s, place(0), a, chi, 128) == ADF_OK);
            ADF_CHECK(!acb_contains_zero(y));
        }
        /* huge exact zeros: -2^1000 (e = 0), -2^1000 - 3 (e = 1) */
        acb_one(s); acb_mul_2exp_si(s, s, 1000); acb_neg(s, s);
        if (e) acb_sub_ui(s, s, 3, 4096);
        ADF_CHECK(gamma_call(y, s, place(0), a, chi, 53) == ADF_OK);
        ADF_CHECK(acb_is_zero(y));
        acb_set_si_si(s, 3, 1); acb_mul_2exp_si(s, s, -2);
        ADF_CHECK(gamma_call(y, s, place(0), a, chi, 2) == ADF_OK);
        ADF_CHECK(gamma_call(ref, s, place(0), a, chi, 53) == ADF_OK);
        ADF_CHECK(acb_overlaps(y, ref));
    }
    acb_clear(s); acb_clear(a); acb_clear(y); acb_clear(ref); adf_char_clear(chi);
}

/* Gamma's OWN poles (api-5.md:84-88): at infinity 1+e+2k, k >= 0 (the poles of Gamma((1-s+e)/2),
   notes.txt:62-64), not the integral's -e-2k; at p, alpha^-1 p^(s-1) = 1. Exact: DOMAIN; a ball meeting one,
   closed boundary included: NOT_DETERMINED; just beside: OK. The integral's poles are regular for gamma. */
ADF_TEST(gamma_poles)
{
    acb_t s, a, y;
    adf_char_t chi;
    initialized_acb(s); initialized_acb(a); initialized_acb(y); adf_char_init(chi);
    acb_one(a);
    for (int e = 0; e < 2; e++)
    {
        ADF_CHECK(adf_char_set_conrey(chi, e ? 4 : 1, e ? 3 : 1) == ADF_OK);
        for (slong k = 0; k < 3; k++)
        {
            slong pole = 1 + e + 2 * k;
            acb_set_si(s, pole);
            ADF_CHECK(gamma_call(y, s, place(0), a, chi, 128) == ADF_DOMAIN);
            point(s, pole, 0, 0, 12);
            ADF_CHECK(gamma_call(y, s, place(0), a, chi, 128) == ADF_NOT_DETERMINED);
            point(s, pole, 0, 1, 10);   /* closed boundary touches the pole */
            ADF_CHECK(gamma_call(y, s, place(0), a, chi, 128) == ADF_NOT_DETERMINED);
            point(s, pole, 0, 1, 11);
            ADF_CHECK(gamma_call(y, s, place(0), a, chi, 128) == ADF_OK);
            point(s, pole, 0, -1, 11);
            ADF_CHECK(gamma_call(y, s, place(0), a, chi, 128) == ADF_OK);
            acb_set_si(s, pole + 1);   /* the other parity's pole is regular here */
            ADF_CHECK(gamma_call(y, s, place(0), a, chi, 128) == ADF_OK);
        }
        /* the integral's poles -e-2k are zeros of gamma, and the trivial list 0, -2, ... is not gamma's */
        acb_set_si(s, -1 - e);
        ADF_CHECK(gamma_call(y, s, place(0), a, chi, 128) == ADF_OK);
        ADF_CHECK(acb_is_finite(y) && !acb_contains_zero(y));
        /* huge exact poles: 2^1000 + 1 (e = 0), 2^1000 (e = 1) */
        acb_one(s); acb_mul_2exp_si(s, s, 1000);
        if (!e) acb_add_ui(s, s, 1, 4096);
        ADF_CHECK(gamma_call(y, s, place(0), a, chi, 53) == ADF_DOMAIN);
    }
    ADF_CHECK(adf_char_set_conrey(chi, 1, 1) == ADF_OK);
    /* p = 3, alpha = 1: pole at s = 1 */
    acb_one(s); acb_one(a);
    ADF_CHECK(gamma_call(y, s, place(3), a, chi, 128) == ADF_DOMAIN);
    point(s, 1, 0, 1, 10);
    ADF_CHECK(gamma_call(y, s, place(3), a, chi, 128) == ADF_NOT_DETERMINED);
    point(s, 1, 0, 1, 11);
    ADF_CHECK(gamma_call(y, s, place(3), a, chi, 128) == ADF_OK);
    /* p = 2: alpha = 1/2 at s = 0 and alpha = 4 at s = 3 are exact poles; alpha = 1/4 at s = 0 is not */
    acb_one(a); acb_mul_2exp_si(a, a, -1); acb_zero(s);
    ADF_CHECK(gamma_call(y, s, place(2), a, chi, 128) == ADF_DOMAIN);
    acb_mul_2exp_si(a, a, -1);
    ADF_CHECK(gamma_call(y, s, place(2), a, chi, 128) == ADF_OK);
    acb_set_si(a, 4); acb_set_si(s, 3);
    ADF_CHECK(gamma_call(y, s, place(2), a, chi, 128) == ADF_DOMAIN);
    /* outside the exact-power recognizer's bound (5a's 2^20 bits) a huge exact integer is a regular point: no power
       of that size is formed (s = 2^62, alpha = 2: alpha p^-s and p^(s-1)/alpha are far from 1) */
    acb_set_si(a, 2); acb_one(s); acb_mul_2exp_si(s, s, 62);
    ADF_CHECK(gamma_call(y, s, place(2), a, chi, 128) == ADF_OK);
    acb_set_si(s, 524289);   /* s - 1 = 524288 = the bound at p = 2, alpha = 2^524288: an exact recognized pole */
    acb_one(a); acb_mul_2exp_si(a, a, 524288);
    ADF_CHECK(gamma_call(y, s, place(2), a, chi, 128) == ADF_DOMAIN);
    /* the integral's poles (alpha, s) = (1, 0) and (2, 1) at p = 2: gamma is 0 there, a value */
    acb_one(a); acb_zero(s);
    ADF_CHECK(gamma_call(y, s, place(2), a, chi, 128) == ADF_OK); ADF_CHECK(acb_contains_zero(y));
    acb_set_si(a, 2); acb_one(s);
    ADF_CHECK(gamma_call(y, s, place(2), a, chi, 128) == ADF_OK); ADF_CHECK(acb_contains_zero(y));
    /* a nonzero lattice pole of gamma, s = 1 + 2 pi i/log 3 for alpha = 1, in a small ball: NOT_DETERMINED */
    acb_one(a); arb_const_pi(acb_imagref(s), 256); arb_mul_2exp_si(acb_imagref(s), acb_imagref(s), 1);
    {
        arb_t l; arb_init(l); arb_log_ui(l, 3, 256); arb_div(acb_imagref(s), acb_imagref(s), l, 256); arb_clear(l);
    }
    arb_one(acb_realref(s)); mag_set_ui_2exp_si(arb_radref(acb_realref(s)), 1, -100);
    ADF_CHECK(gamma_call(y, s, place(3), a, chi, 128) == ADF_NOT_DETERMINED);
    ADF_CHECK(epsilon_call(y, s, place(3), a, chi, 128) == ADF_OK);
    /* ramified gamma has no s poles: s = 1, alpha = 1 at conductor 5 */
    ADF_CHECK(adf_char_set_conrey(chi, 5, 2) == ADF_OK);
    acb_one(s);
    ADF_CHECK(gamma_call(y, s, place(5), a, chi, 128) == ADF_OK);
    acb_clear(s); acb_clear(a); acb_clear(y); adf_char_clear(chi);
}

/* The duality identity, derived from P9 and P10 by hand:
   gamma_v(alpha, eta0, s) gamma_v(alpha^-1, eta0^-1, 1 - s) = eta0(-1), where both sides are finite.
   a = 0: (1 - alpha p^-s)/(1 - alpha^-1 p^(s-1)) times (1 - alpha^-1 p^(s-1))/(1 - alpha p^-s) = 1 = eta0(-1).
   a > 0: put H(chi) = sum_(u mod q) chi(u) E(-u/q); u -> -u gives H(chi) = chi(-1) tau(chi). P9:345-347 gives
     gamma(alpha, eta0, s) = alpha^a p^(-as) H(conj eta0) and gamma(alpha^-1, eta0^-1, 1-s) = alpha^-a p^(-a(1-s))
     H(eta0). The product is p^-a eta0(-1)^2 tau(conj eta0) tau(eta0) = p^-a (-1)^e p^a (Lemma 8:309) = eta0(-1).
   infinity: i^e pi^(s-1/2) Gamma((1-s+e)/2)/Gamma((s+e)/2) times i^e pi^(1/2-s) Gamma((s+e)/2)/Gamma((1-s+e)/2)
     = i^(2e) = (-1)^e = eta0(-1). eta0^-1 has the parity of eta0. */
ADF_TEST(duality_identity)
{
    const ulong chars[7][3] = {{1, 1, 2}, {1, 1, 3}, {1, 1, 5}, {5, 2, 5}, {13, 2, 13}, {16, 5, 2}, {9, 2, 3}};
    const slong alphas[5][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {2, 0, 0}, {1, 2, 1}};
    acb_t s, s1, a, ai, y, y1, sign;
    adf_char_t chi, inv;
    int checked = 0;
    initialized_acb(s); initialized_acb(s1); initialized_acb(a); initialized_acb(ai); initialized_acb(y);
    initialized_acb(y1); initialized_acb(sign); adf_char_init(chi); adf_char_init(inv);
    for (int c = 0; c < 9; c++)
    {
        ulong p = c < 7 ? chars[c][2] : 0;
        ulong q = c < 7 ? chars[c][0] : (c == 7 ? 1 : 4), n = c < 7 ? chars[c][1] : (c == 7 ? 1 : 3);
        ADF_CHECK(adf_char_set_conrey(chi, q, n) == ADF_OK);
        adf_char_conj(inv, chi);
        acb_set_si(sign, chi->parity ? -1 : 1);
        for (int j = 0; j < 5; j++)
            for (int k = 0; k < 3; k++)
            {
                acb_set_si_si(a, alphas[j][0], alphas[j][1]); acb_mul_2exp_si(a, a, -alphas[j][2]);
                acb_inv(ai, a, 256);
                if (k == 0) { acb_set_si_si(s, 2, 3); acb_mul_2exp_si(s, s, -2); }
                else if (k == 1) { acb_set_si_si(s, -2, 1); acb_mul_2exp_si(s, s, -1); }
                else acb_set_si(s, 2);
                acb_sub_ui(s1, s, 1, 256); acb_neg(s1, s1);
                int st = gamma_call(y, s, place(p), a, chi, 128);
                int st1 = gamma_call(y1, s1, place(p), ai, inv, 128);
                if (st != ADF_OK || st1 != ADF_OK)
                {
                    /* only exact poles may interrupt: p = 2, alpha = 2, s = 2; e = 1 at infinity, s = 2 */
                    ADF_CHECK_MSG(st == ADF_DOMAIN || st1 == ADF_DOMAIN, "c=%d j=%d k=%d", c, j, k);
                    continue;
                }
                acb_mul(y, y, y1, 256);
                ADF_CHECK_MSG(acb_contains(y, sign), "c=%d j=%d k=%d", c, j, k);
                checked++;
            }
    }
    ADF_CHECK(checked >= 120);
    acb_clear(s); acb_clear(s1); acb_clear(a); acb_clear(ai); acb_clear(y); acb_clear(y1); acb_clear(sign);
    adf_char_clear(chi); adf_char_clear(inv);
}

/* Validation, precision, aliasing (api-5.md:26-37, 89-90). */
ADF_TEST(statuses_precision_and_aliases)
{
    local_fn fns[2] = {adf_local_gamma_at, adf_local_epsilon_at};
    acb_t s, a, y, ref;
    adf_char_t chi;
    adf_place_t w;
    unsigned char before[sizeof(acb_struct)];
    initialized_acb(s); initialized_acb(a); initialized_acb(y); initialized_acb(ref); adf_char_init(chi);
    for (int f = 0; f < 2; f++)
    {
        for (int ram = 0; ram < 2; ram++)
        {
            ADF_CHECK(adf_char_set_conrey(chi, ram ? 4 : 1, ram ? 3 : 1) == ADF_OK);
            acb_set_si(s, 2); acb_zero(a);
            ADF_CHECK(call(fns[f], y, s, place(2), a, chi, 128) == ADF_DOMAIN);
            mag_set_ui_2exp_si(arb_radref(acb_imagref(a)), 1, -20);
            ADF_CHECK(call(fns[f], y, s, place(2), a, chi, 128) == ADF_NOT_DETERMINED);
            acb_one(a); arf_pos_inf(arb_midref(acb_imagref(a)));
            ADF_CHECK(call(fns[f], y, s, place(2), a, chi, 128) == ADF_DOMAIN);
            acb_one(a); arf_nan(arb_midref(acb_realref(s)));
            ADF_CHECK(call(fns[f], y, s, place(2), a, chi, 128) == ADF_DOMAIN);
            ADF_CHECK(call(fns[f], y, s, place(0), a, chi, 128) == ADF_DOMAIN);
            /* conductor 4 is not a power of 3 */
            acb_set_si(s, 2);
            ADF_CHECK(call(fns[f], y, s, place(3), a, chi, 128) == (ram ? ADF_DOMAIN : ADF_OK));
            acb_set_si_si(chi->s, 17, -9);   /* the finite-character calls ignore the exponent */
            ADF_CHECK(call(fns[f], y, s, place(2), a, chi, 128) == ADF_OK);
            ADF_CHECK(call(fns[f], ref, s, place(2), a, chi, 53) == ADF_OK);
            ADF_CHECK(acb_overlaps(y, ref));
            ADF_CHECK(call(fns[f], ref, s, place(2), a, chi, 2) == ADF_OK);
            ADF_CHECK(acb_overlaps(y, ref));
            ADF_CHECK(call(fns[f], ref, s, place(2), a, chi, 1) == ADF_OK);
            ADF_CHECK(call(fns[f], y, s, place(2), a, chi, -5) == ADF_OK);
            ADF_CHECK(acb_equal(y, ref));
            acb_zero(chi->s);
            /* z = s = alpha */
            acb_set_si_si(a, 1, 1); acb_set(s, a);
            ADF_CHECK(call(fns[f], ref, s, place(2), a, chi, 128) == ADF_OK);
            ADF_CHECK(fns[f](s, NULL, s, place(2), s, chi, 128) == ADF_OK);
            ADF_CHECK(acb_equal(s, ref));
            /* lane t5-repair1: s now holds the struct of the function's own temporary, whose inactive mantissa
               limbs FLINT leaves unwritten; arf_set copies them whole, so the byte comparison of call() on the
               next failing call read them (valgrind: 18 errors from 6 contexts). A fresh s keeps every byte
               defined. */
            acb_clear(s); initialized_acb(s);
        }
        /* the precision cap: OK at the cap (cheap cardinal-phase case), LIMIT one above, decided first */
        ADF_CHECK(adf_char_set_conrey(chi, 4, 3) == ADF_OK);
        acb_onei(a); acb_zero(s);
        ADF_CHECK(call(fns[f], y, s, place(2), a, chi, ADF_REAL_PREC_MAX) == ADF_OK);
        acb_set_si_si(ref, 0, 2);   /* i^2 2^0 G_minus(conj chi_4) = -(E(-1/4) - E(-3/4)) = -(-2i) = 2i */
        ADF_CHECK(acb_contains(y, ref));
        acb_clear(y); initialized_acb(y);
        w = place(97); chi->parity = 9; acb_set_si_si(y, 17, 3);
        memcpy(before, y, sizeof(before));
        ADF_CHECK(fns[f](y, &w, s, (adf_place_t){4}, a, chi, ADF_REAL_PREC_MAX + 1) == ADF_LIMIT);
        ADF_CHECK(w.opaque == 4); ADF_CHECK(memcmp(before, y, sizeof(before)) == 0);
        chi->parity = 1;
        ADF_CHECK(fns[f](y, NULL, s, place(2), a, chi, ADF_REAL_PREC_MAX + 1) == ADF_LIMIT);
        ADF_CHECK(memcmp(before, y, sizeof(before)) == 0);
        /* the conductor cap is the next check, before INV and setup */
        chi->q = 65537 * 2; w = place(97);
        ADF_CHECK(fns[f](y, &w, s, place(2), a, chi, 128) == ADF_LIMIT);
        ADF_CHECK(adf_place_equal(w, place(2))); ADF_CHECK(memcmp(before, y, sizeof(before)) == 0);
        chi->q = 4;
    }
    acb_clear(s); acb_clear(a); acb_clear(y); acb_clear(ref); adf_char_clear(chi);
}

#ifdef ADF_CHECK_INVARIANTS
ADF_TEST(invariant_entry_aborts)
{
    for (int f = 0; f < 2; f++)
        for (int mode = 0; mode < 5; mode++)
        {
            pid_t pid = fork();
            int st;
            ADF_CHECK(pid >= 0);
            if (pid == 0)
            {
                struct rlimit nocore = {0, 0};
                acb_t s, a, y;
                adf_char_t chi;
                local_fn fn = f ? adf_local_epsilon_at : adf_local_gamma_at;
                setrlimit(RLIMIT_CORE, &nocore);
#ifdef __linux__
                prctl(PR_SET_DUMPABLE, 0);
#endif
                adf_place_t v = place(2);
                initialized_acb(s); initialized_acb(a); initialized_acb(y); adf_char_init(chi);
                acb_one(s); acb_one(a);
                if (mode == 0) v.opaque = 4;
                if (mode == 1) chi->parity = 1;
                fn(mode == 2 ? chi->s : y, NULL, mode == 3 ? chi->s : s, v, mode == 4 ? chi->s : a, chi, 128);
                /* The children clear their objects when the expected abort does not come (tools/memcheck). */
                chi->parity = 0;
                acb_clear(s); acb_clear(a); acb_clear(y); adf_char_clear(chi);
                flint_cleanup(); _exit(42);
            }
            if (pid > 0)
            {
                ADF_CHECK(waitpid(pid, &st, 0) == pid);
                ADF_CHECK_MSG(WIFSIGNALED(st) && WTERMSIG(st) == SIGABRT, "f=%d mode=%d", f, mode);
            }
        }
}
#endif

/* The conductor cap is inclusive (localfactor.h:99-103). Conrey (2^16,5)
   is primitive and even. At infinity, P10 gives gamma(1/2) = epsilon = 1.
   The real branch avoids a 65536-term finite Gauss sum at this boundary. */
ADF_TEST(inclusive_character_cap)
{
    acb_t s, a, y, expected;
    adf_char_t chi;
    initialized_acb(s); initialized_acb(a); initialized_acb(y); initialized_acb(expected);
    adf_char_init(chi);
    ADF_CHECK(adf_char_set_conrey(chi, ADF_CHAR_MOD_MAX, 5) == ADF_OK);
    ADF_CHECK(chi->q == ADF_CHAR_MOD_MAX && adf_char_get_conductor(chi) == ADF_CHAR_MOD_MAX);
    ADF_CHECK(chi->parity == 0);
    acb_one(s); acb_mul_2exp_si(s, s, -1); acb_one(a); acb_one(expected);
    ADF_CHECK(gamma_call(y, s, place(0), a, chi, 160) == ADF_OK);
    ADF_CHECK(acb_contains(y, expected));
    ADF_CHECK(epsilon_call(y, s, place(0), a, chi, 160) == ADF_OK);
    ADF_CHECK(acb_contains(y, expected));
    acb_clear(s); acb_clear(a); acb_clear(y); acb_clear(expected); adf_char_clear(chi);
}
