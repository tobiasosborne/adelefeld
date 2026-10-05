/* tests/test_localfactor.c: the local zeta factor of the trivial character, adf_local_zeta_factor_at
   (include/adelefeld/localfactor.h; design docs/design/local-zeta.md Z1-Z9; docs/api-1f9.md Y16, Y17;
   lane f-slice14).

   Oracle: tests/ref/vectors/f-slice14/zeta.jsonl, a subset of the fixtures of proto/zeta_checks.py chosen by
   lanes/f-slice14/select_fixtures.py (its docstring states the rule). Every row is run. The whole fixture file
   (11 MB, not committed) is run instead when the environment variable ADF_ZETA_FIXTURES names it.
   A row's input is the exact dyadic rectangle s_mid +/- s_rad (the effective radii the oracle returns); its
   status must be the one the oracle simulated; on OK every certified sample (a decimal point with 230 digits and
   an outward Euclidean error bound point_error) must lie inside the returned rectangle, and the width target
   2 |rad| <= 64 width_lower + 2^(-max(2,prec)+8) max(1, image_bound) of design section 4 is counted
   separately from soundness.

   Every call goes through call(), which checks the state of the outputs that the header promises: on a status
   other than OK the representation of y is unchanged and *where = v; on OK where is unchanged and y is finite
   with midpoints of at most max(2, prec) bits. */
#include <adelefeld.h>
#include <adelefeld/localfactor.h>
#include <flint/arb_hypgeom.h>
#include <stdlib.h>
#include <string.h>
#ifdef ADF_CHECK_INVARIANTS
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif
#include "support/jsonl.h"
#include "test_runner.h"

/* ---- helpers ---- */

static adf_place_t prime_place(ulong p)
{
    adf_place_t v = adf_place_inf();
    ADF_CHECK_MSG(adf_place_prime(&v, p) == ADF_OK, "prime %lu", (unsigned long) p);
    return v;
}

static adf_place_t sentinel_place(void) { return prime_place(97); }

/* A nontrivial sentinel: inexact in both components, so that a write of any kind changes it. */
static void set_sentinel(acb_t y)
{
    arb_set_si(acb_realref(y), 7);
    arb_div_ui(acb_realref(y), acb_realref(y), 3, 40);
    arb_set_si(acb_imagref(y), -5);
    mag_set_ui_2exp_si(arb_radref(acb_imagref(y)), 3, -12);
}

/* Bits of a midpoint; 0 for zero. */
static slong mid_bits(const arb_t x) { return arf_bits(arb_midref(x)); }

static int status_code(const char *name)
{
    if (strcmp(name, "OK") == 0) return ADF_OK;
    if (strcmp(name, "DOMAIN") == 0) return ADF_DOMAIN;
    if (strcmp(name, "NOT_DETERMINED") == 0) return ADF_NOT_DETERMINED;
    if (strcmp(name, "LIMIT") == 0) return ADF_LIMIT;
    return -1;
}

/* mode 0: y separate, where given; 1: y = s (aliased), where given; 2: y separate, where = NULL. */
static int call_mode(acb_t y, const acb_t s, adf_place_t v, slong prec, int mode)
{
    acb_t before, t;
    adf_place_t w = sentinel_place(), mark = w;
    int st;
    acb_init(before);
    acb_init(t);
    if (mode == 1)
    {
        acb_set(t, s);
        acb_set(before, t);
        st = adf_local_zeta_factor_at(t, &w, t, v, prec);
        if (st != ADF_OK) ADF_CHECK_MSG(acb_equal(t, before), "aliased y changed on status %d", st);
        acb_set(y, t);
    }
    else
    {
        acb_set(before, y);
        st = adf_local_zeta_factor_at(y, mode == 2 ? NULL : &w, s, v, prec);
        if (st != ADF_OK) ADF_CHECK_MSG(acb_equal(y, before), "y changed on status %d", st);
    }
    if (mode != 2)
    {
        if (st == ADF_OK) ADF_CHECK_MSG(adf_place_equal(w, mark), "where written on OK");
        else ADF_CHECK_MSG(adf_place_equal(w, v), "where is not v on status %d", st);
    }
    if (st == ADF_OK)
    {
        slong p = prec < 2 ? 2 : prec;
        ADF_CHECK(acb_is_finite(mode == 1 ? t : y));
        if (acb_is_finite(y))
            ADF_CHECK_MSG(mid_bits(acb_realref(y)) <= p && mid_bits(acb_imagref(y)) <= p,
                          "midpoint above prec");
    }
    acb_clear(before);
    acb_clear(t);
    return st;
}

/* The main entry of the tests: y receives the result on OK. Every mode is run and must agree (status and
   the representation of the result), so aliasing and where = NULL are checked on every call. */
static int call(acb_t y, const acb_t s, adf_place_t v, slong prec)
{
    acb_t y0, y1, y2;
    int st0, st1, st2;
    acb_init(y0);
    acb_init(y1);
    acb_init(y2);
    set_sentinel(y0);
    set_sentinel(y2);
    st0 = call_mode(y0, s, v, prec, 0);
    st1 = call_mode(y1, s, v, prec, 1);
    st2 = call_mode(y2, s, v, prec, 2);
    ADF_CHECK_MSG(st0 == st1 && st0 == st2, "modes disagree: %d %d %d", st0, st1, st2);
    if (st0 == ADF_OK)
    {
        ADF_CHECK(acb_equal(y0, y1) && acb_equal(y0, y2));
        acb_set(y, y0);
    }
    acb_clear(y0);
    acb_clear(y1);
    acb_clear(y2);
    return st0;
}

/* s = x + i y exactly, x and y small integers or exact dyadics given as fmpq strings. */
static void set_point_si(acb_t s, slong x, slong y)
{
    acb_set_si_si(s, x, y);
}

/* Exact dyadic from "num" or "num/2^k"; returns 0 if the text is not a dyadic rational. */
static int arf_set_dyadic_str(arf_t x, const char *text)
{
    fmpz_t num, den, e;
    const char *slash = strchr(text, '/');
    int ok = 1;
    fmpz_init(num);
    fmpz_init(den);
    fmpz_init(e);
    if (slash == NULL)
    {
        ok = fmpz_set_str(num, text, 10) == 0;
        fmpz_one(den);
    }
    else
    {
        size_t n = (size_t) (slash - text);
        char *head = malloc(n + 1);
        memcpy(head, text, n);
        head[n] = '\0';
        ok = fmpz_set_str(num, head, 10) == 0 && fmpz_set_str(den, slash + 1, 10) == 0;
        free(head);
    }
    if (ok && fmpz_sgn(den) > 0)
    {
        ulong k = fmpz_val2(den);
        fmpz_tdiv_q_2exp(den, den, k);
        ok = fmpz_is_one(den);
        fmpz_set_si(e, -(slong) k);
        arf_set_fmpz_2exp(x, num, e);
    }
    else ok = 0;
    fmpz_clear(num);
    fmpz_clear(den);
    fmpz_clear(e);
    return ok;
}

/* ---- the vectors ---- */

static const char *str_field(const jsonl_value *r, const char *key)
{
    jsonl_error_t e;
    const jsonl_value *v;
    const char *s = NULL;
    if (!jsonl_field(r, key, &v, &e)) return NULL;
    if (jsonl_is(v, JSONL_STR)) return jsonl_string(v, NULL, &e);
    if (!jsonl_int_text_or_string(v, &s, &e)) return NULL;
    return s;
}

static const jsonl_value *sub_field(const jsonl_value *r, const char *key)
{
    jsonl_error_t e;
    const jsonl_value *v = NULL;
    ADF_CHECK_MSG(jsonl_field(r, key, &v, &e), "missing %s", key);
    return v;
}

static const char *at_str(const jsonl_value *a, size_t i)
{
    jsonl_error_t e;
    const jsonl_value *v = jsonl_at(a, i, &e);
    return v ? jsonl_string(v, NULL, &e) : NULL;
}

/* {man, exp} -> an upper bound as a mag (exact when the mantissa is short). */
static void bound_mag(mag_t m, const jsonl_value *b)
{
    fmpz_t man, ex;
    arf_t t;
    fmpz_init(man);
    fmpz_init(ex);
    arf_init(t);
    ADF_CHECK(fmpz_set_str(man, str_field(b, "man"), 10) == 0);
    ADF_CHECK(fmpz_set_str(ex, str_field(b, "exp"), 10) == 0);
    arf_set_fmpz_2exp(t, man, ex);
    arf_get_mag(m, t);
    fmpz_clear(man);
    fmpz_clear(ex);
    arf_clear(t);
}

/* A mag equal to the dyadic r >= 0 when r has a mantissa of at most 30 bits: mag_set_ui_2exp_si stores such a
   value exactly (probe lanes/f-slice14/probes/mag_exact.c: arf_get_mag and mag_set_fmpz_2exp_fmpz round up by
   one unit even then on FLINT 3.0.1). The caller checks the round trip. */
static void mag_set_arf_exact(mag_t m, const arf_t r)
{
    fmpz_t man, ex;
    fmpz_init(man);
    fmpz_init(ex);
    arf_get_fmpz_2exp(man, ex, r);
    if (fmpz_abs_fits_ui(man) && fmpz_fits_si(ex)) mag_set_ui_2exp_si(m, fmpz_get_ui(man), fmpz_get_si(ex));
    else arf_get_mag(m, r);
    fmpz_clear(man);
    fmpz_clear(ex);
}

/* The rectangle of a row: exact dyadic midpoints, exact dyadic radii (checked to be representable). */
static void row_input(acb_t s, const jsonl_value *mid, const jsonl_value *rad)
{
    arf_t r, back;
    arf_init(r);
    arf_init(back);
    for (int i = 0; i < 2; i++)
    {
        arb_ptr c = i ? acb_imagref(s) : acb_realref(s);
        ADF_CHECK(arf_set_dyadic_str(arb_midref(c), at_str(mid, i)));
        ADF_CHECK(arf_set_dyadic_str(r, at_str(rad, i)));
        mag_set_arf_exact(arb_radref(c), r);
        arf_set_mag(back, arb_radref(c));
        ADF_CHECK_MSG(arf_equal(back, r), "radius %s not exact as a mag", at_str(rad, i));
    }
    arf_clear(r);
    arf_clear(back);
}

/* Is the certified point (value, point_error) of a sample at s inside y? A rigorous enclosure of the point
   (the decimal read at 1100 bits, enlarged by the Euclidean error in both components) must lie inside y.
   A sample at a real s (imaginary part exactly 0) has a real value at a regular point (Z1, Z2: both factors
   are real on the real axis), so its imaginary check is that y's imaginary part contains 0. */
static int box_inside(const acb_t y, const acb_t box, int real_s)
{
    int ok = arb_contains(acb_realref(y), acb_realref(box));
    if (real_s) return ok && arb_contains_zero(acb_imagref(y));
    return ok && arb_contains(acb_imagref(y), acb_imagref(box));
}

static void reference_prime(acb_t ref, const acb_t pt, ulong p, slong bits);
static void reference_real(acb_t ref, const acb_t pt, slong bits);

static long raised_certificates = 0;

/* The oracle's point error is absolute (about 1e-229 max(1, |value|), design Z8 item 7), so a component of tiny
   modulus (the imaginary part -5.96e-302 of L_2(1000 + i), the real part of L_2(-2^1000)) is not resolved by it
   while the returned ball, of relative precision, is. Then the box overlaps y without lying inside it, and the
   design (section 4, :421-423) says to raise the independent certificate precision: the point is evaluated
   again here at 4000 bits by another formula (acb_pow at a prime, reference_prime; Gamma at the real place,
   reference_real), and that enclosure must lie inside y. A disjoint box is a failure at once. Counted in
   raised_certificates. */
static int sample_inside(const acb_t y, const jsonl_value *value, const jsonl_value *err, const acb_t pt,
                         const adf_place_t v, int real_s)
{
    acb_t box;
    mag_t e;
    int ok;
    acb_init(box);
    mag_init(e);
    ADF_CHECK(arb_set_str(acb_realref(box), at_str(value, 0), 1100) == 0);
    ADF_CHECK(arb_set_str(acb_imagref(box), at_str(value, 1), 1100) == 0);
    bound_mag(e, err);
    acb_add_error_mag(box, e);
    ok = box_inside(y, box, real_s);
    if (!ok && acb_overlaps(y, box))
    {
        if (adf_place_is_archimedean(v)) reference_real(box, pt, 4000);
        else reference_prime(box, pt, adf_place_prime_get(v), 4000);
        ok = box_inside(y, box, real_s);
        raised_certificates++;
    }
    acb_clear(box);
    mag_clear(e);
    return ok;
}

typedef struct
{
    long rows, ok_rows, samples, width_checked, width_fail, status_diff;
} vec_counts;

/* Run one fixture row; place_filter: 0 primes only, 1 real only, 2 both. */
static void run_row(const jsonl_value *r, size_t i, int place_filter, vec_counts *c)
{
    const char *place = str_field(r, "place"), *status = str_field(r, "status");
    const jsonl_value *mid, *rad;
    jsonl_error_t e;
    adf_place_t v;
    acb_t s, y;
    slong prec;
    int want, got, real;
    if (place == NULL || status == NULL) return;   /* rows without an input: tested by hand below */
    real = strcmp(place, "real") == 0;
    if ((place_filter == 0 && real) || (place_filter == 1 && !real)) return;
    if (!jsonl_field(r, "s_mid", &mid, &e)) return;
    rad = sub_field(r, "s_rad");
    v = real ? adf_place_inf() : prime_place(strtoull(place, NULL, 10));
    prec = atol(str_field(r, "prec"));
    want = status_code(status);
    acb_init(s);
    acb_init(y);
    row_input(s, mid, rad);
    got = call(y, s, v, prec);
    c->rows++;
    ADF_CHECK_MSG(got == want || (want == ADF_OK && got == ADF_NOT_DETERMINED),
                  "row %zu (%s, %s, line %lu): status %d, oracle %s", i, place, str_field(r, "kind"),
                  jsonl_line_of(r), got, status);
    if (got != want)
    {
        c->status_diff++;
        printf("status differs from the simulation: line %lu kind %s place %s prec %ld: %d, oracle %s\n",
               jsonl_line_of(r), str_field(r, "kind"), place, (long) prec, got, status);
    }
    /* A wrong OK on a pole/status row has already failed above. Such a row has no value certificate;
       reading its absent value/samples after that assertion caused the review's two SIGSEGV runs. */
    if (got == ADF_OK && want == ADF_OK && acb_is_finite(y))
    {
        const jsonl_value *samples = sub_field(r, "samples");
        mag_t diam, target, t, ib;
        c->ok_rows++;
        acb_t pt;
        acb_init(pt);
        acb_get_mid(pt, s);
        ADF_CHECK_MSG(sample_inside(y, sub_field(r, "value"), sub_field(r, "point_error"), pt, v,
                                    arb_is_zero(acb_imagref(pt))),
                      "row line %lu: midpoint value outside", jsonl_line_of(r));
        for (size_t k = 0; k < jsonl_size(samples); k++)
        {
            const jsonl_value *smp = jsonl_at(samples, k, &e);
            const jsonl_value *sp = sub_field(smp, "s");
            ADF_CHECK(arf_set_dyadic_str(arb_midref(acb_realref(pt)), at_str(sp, 0)));
            ADF_CHECK(arf_set_dyadic_str(arb_midref(acb_imagref(pt)), at_str(sp, 1)));
            ADF_CHECK_MSG(sample_inside(y, sub_field(smp, "value"), sub_field(smp, "point_error"), pt, v,
                                        arb_is_zero(acb_imagref(pt))),
                          "row line %lu: sample %zu outside", jsonl_line_of(r), k);
            c->samples++;
        }
        acb_clear(pt);
        /* width target, design section 4 */
        mag_init(diam);
        mag_init(target);
        mag_init(t);
        mag_init(ib);
        mag_hypot(diam, arb_radref(acb_realref(y)), arb_radref(acb_imagref(y)));
        mag_mul_2exp_si(diam, diam, 1);
        bound_mag(target, sub_field(r, "width_lower"));
        mag_mul_ui(target, target, 64);
        bound_mag(ib, sub_field(r, "image_bound"));
        if (mag_cmp_2exp_si(ib, 0) < 0) mag_one(ib);
        mag_mul_2exp_si(t, ib, -(prec < 2 ? 2 : prec) + 8);
        mag_add(target, target, t);
        /* points (width_lower = 0) are tested by containment only, as in the oracle (design Z7 item 7) */
        if (mag_is_zero(arb_radref(acb_realref(s))) && mag_is_zero(arb_radref(acb_imagref(s)))) mag_inf(target);
        else c->width_checked++;
        if (mag_cmp(diam, target) > 0)
        {
            c->width_fail++;
            printf("width target missed: line %lu kind %s place %s\n", jsonl_line_of(r), str_field(r, "kind"),
                   place);
        }
        mag_clear(diam);
        mag_clear(target);
        mag_clear(t);
        mag_clear(ib);
    }
    acb_clear(s);
    acb_clear(y);
}

static void run_vectors(const char *path, int place_filter)
{
    jsonl_file *f = NULL;
    jsonl_error_t err;
    vec_counts c = {0, 0, 0, 0, 0, 0};
    raised_certificates = 0;
    ADF_CHECK_MSG(jsonl_open(path, &f, &err), "%s", jsonl_error_message(&err));
    if (!f) return;
    for (size_t i = 0; i < jsonl_count(f); i++) run_row(jsonl_record(f, i), i, place_filter, &c);
    printf("%s (places %d): rows %ld, OK %ld, samples contained %ld, width checked %ld, width missed %ld, "
           "status differs %ld, raised certificates %ld\n", path, place_filter, c.rows, c.ok_rows, c.samples,
           c.width_checked, c.width_fail, c.status_diff, raised_certificates);
    ADF_CHECK_MSG(c.width_fail == 0, "width target missed on %ld rows", c.width_fail);
    jsonl_close(f);
}

#define VECTORS "tests/ref/vectors/f-slice14/zeta.jsonl"

/* ======================= slice A: the prime place ======================= */

/* (1 - p^(-s))^(-1) at s = 1, 2, -1, p = 2, 3: 2, 4/3, -1; 3/2, 9/8, -1/2 (notes.txt:1733). */
ADF_TEST(prime_exact_values)
{
    static const slong ss[3] = {1, 2, -1};
    static const slong num[2][3] = {{2, 4, -1}, {3, 9, -1}};
    static const slong den[2][3] = {{1, 3, 1}, {2, 8, 2}};
    static const ulong ps[2] = {2, 3};
    static const slong precs[5] = {2, 16, 64, 256, 1000};
    acb_t s, y;
    fmpq_t q;
    acb_init(s);
    acb_init(y);
    fmpq_init(q);
    for (int a = 0; a < 2; a++)
        for (int b = 0; b < 3; b++)
            for (int k = 0; k < 5; k++)
            {
                slong prec = precs[k];
                set_point_si(s, ss[b], 0);
                ADF_CHECK(call(y, s, prime_place(ps[a]), prec) == ADF_OK);
                fmpq_set_si(q, num[a][b], (ulong) den[a][b]);
                ADF_CHECK_MSG(arb_contains_fmpq(acb_realref(y), q), "p=%lu s=%ld prec=%ld",
                              (unsigned long) ps[a], (long) ss[b], (long) prec);
                ADF_CHECK(arb_contains_zero(acb_imagref(y)));
                /* radius consistent with prec: at most 2^(8-prec) relative to |value| <= 4 */
                ADF_CHECK_MSG(mag_cmp_2exp_si(arb_radref(acb_realref(y)), 10 - prec) <= 0,
                              "p=%lu s=%ld prec=%ld: radius too large", (unsigned long) ps[a], (long) ss[b],
                              (long) prec);
                ADF_CHECK(mag_cmp_2exp_si(arb_radref(acb_imagref(y)), 10 - prec) <= 0);
            }
    acb_clear(s);
    acb_clear(y);
    fmpq_clear(q);
}

/* The poles 2 pi i k / log p (Z1): a ball around each, of radius 10^-j, and the vertical segment through it,
   are NOT_DETERMINED; a ball at distance d = 16 r with radius r is OK and contains the reference values at its
   centre and its corners (computed by acb_pow at 1100 bits, a formula the code does not use). */
static void pole_centre(arb_t yk, ulong p, slong k)
{
    arb_t a;
    arb_init(a);
    arb_const_pi(yk, 1100);
    arb_mul_si(yk, yk, 2 * k, 1100);
    arb_log_ui(a, p, 1100);
    arb_div(yk, yk, a, 1100);
    arb_clear(a);
}

static void reference_prime(acb_t ref, const acb_t pt, ulong p, slong bits)
{
    acb_t base;
    acb_init(base);
    acb_set_ui(base, p);
    acb_neg(ref, pt);
    acb_pow(ref, base, ref, bits);      /* p^(-s) = exp(-s log p), acb.rst:639-643 */
    acb_sub_ui(ref, ref, 1, bits);
    acb_neg(ref, ref);
    acb_inv(ref, ref, bits);
    acb_clear(base);
}

/* pi^(-s/2) Gamma(s/2) at a point, by acb_pow and acb_gamma at bits (a FLINT Gamma value: used only to raise
   the precision of a sample whose oracle box overlaps the result, never as the primary reference). */
static void reference_real(acb_t ref, const acb_t pt, slong bits)
{
    acb_t z, pi;
    acb_init(z);
    acb_init(pi);
    acb_mul_2exp_si(z, pt, -1);
    acb_gamma(ref, z, bits);
    acb_const_pi(pi, bits);
    acb_neg(z, z);
    acb_pow(z, pi, z, bits);
    acb_mul(ref, ref, z, bits);
    acb_clear(z);
    acb_clear(pi);
}

/* y contains the reference at the centre and at the four corners of s. */
static int contains_corners(const acb_t y, const acb_t s, ulong p)
{
    acb_t pt, ref;
    int ok = 1;
    acb_init(pt);
    acb_init(ref);
    for (int i = -1; i <= 1; i++)
        for (int j = -1; j <= 1; j++)
        {
            if (i * j == 0 && (i != 0 || j != 0)) continue;
            acb_get_mid(pt, s);
            arb_set_arf(acb_realref(pt), arb_midref(acb_realref(s)));
            if (i != 0)
            {
                arf_t r;
                arf_init(r);
                arf_set_mag(r, arb_radref(acb_realref(s)));
                arf_mul_si(r, r, i, ARF_PREC_EXACT, ARF_RND_DOWN);
                arf_add(arb_midref(acb_realref(pt)), arb_midref(acb_realref(pt)), r, ARF_PREC_EXACT,
                        ARF_RND_DOWN);
                arf_set_mag(r, arb_radref(acb_imagref(s)));
                arf_mul_si(r, r, j, ARF_PREC_EXACT, ARF_RND_DOWN);
                arf_add(arb_midref(acb_imagref(pt)), arb_midref(acb_imagref(pt)), r, ARF_PREC_EXACT,
                        ARF_RND_DOWN);
                arf_clear(r);
            }
            reference_prime(ref, pt, p, 1100);
            if (!acb_contains(y, ref)) ok = 0;
        }
    acb_clear(pt);
    acb_clear(ref);
    return ok;
}

static const ulong lattice_primes[6] = {2, 3, 5, 7, 65537, UWORD(18446744073709551557)};

ADF_TEST(prime_pole_lattice)
{
    static const int js[5] = {1, 4, 12, 30, 60};
    acb_t s, y;
    arb_t yk, r;
    long nd = 0, ok = 0;
    acb_init(s);
    acb_init(y);
    arb_init(yk);
    arb_init(r);
    for (int a = 0; a < 6; a++)
        for (slong k = -3; k <= 3; k++)
            for (int jj = 0; jj < 5; jj++)
            {
                ulong p = lattice_primes[a];
                adf_place_t v = prime_place(p);
                pole_centre(yk, p, k);
                arb_set_ui(r, 10);
                arb_pow_ui(r, r, (ulong) js[jj], 300);
                arb_inv(r, r, 300);                         /* 10^-j, its upper bound as the radius */
                acb_zero(s);
                arb_set_arf(acb_imagref(s), arb_midref(yk));
                arb_get_mag(arb_radref(acb_realref(s)), r);
                arb_get_mag(arb_radref(acb_imagref(s)), r);
                /* certified membership of the pole: the 1100-bit pole ball lies inside the input */
                ADF_CHECK(arb_contains(acb_imagref(s), yk));
                for (int pi = 0; pi < 3; pi++)
                {
                    slong prec = pi == 0 ? 16 : pi == 1 ? 64 : 256;
                    ADF_CHECK_MSG(call(y, s, v, prec) == ADF_NOT_DETERMINED, "around p=%lu k=%ld j=%d",
                                  (unsigned long) p, (long) k, js[jj]);
                    nd++;
                }
                mag_zero(arb_radref(acb_realref(s)));          /* the vertical segment */
                ADF_CHECK_MSG(call(y, s, v, 256) == ADF_NOT_DETERMINED, "segment p=%lu k=%ld j=%d",
                              (unsigned long) p, (long) k, js[jj]);
                nd++;
                /* distance d = 16 r to the right and to the left, radius r */
                for (int sign = -1; sign <= 1; sign += 2)
                {
                    arb_get_mag(arb_radref(acb_realref(s)), r);
                    arf_set_mag(arb_midref(acb_realref(s)), arb_radref(acb_realref(s)));
                    arf_mul_si(arb_midref(acb_realref(s)), arb_midref(acb_realref(s)), 16 * sign,
                               ARF_PREC_EXACT, ARF_RND_DOWN);
                    ADF_CHECK_MSG(call(y, s, v, 256) == ADF_OK, "near p=%lu k=%ld j=%d sign=%d",
                                  (unsigned long) p, (long) k, js[jj], sign);
                    ADF_CHECK_MSG(contains_corners(y, s, p), "near p=%lu k=%ld j=%d sign=%d: reference outside",
                                  (unsigned long) p, (long) k, js[jj], sign);
                    ok++;
                }
            }
    printf("pole lattice: %ld NOT_DETERMINED calls, %ld OK calls with reference containment\n", nd, ok);
    acb_clear(s);
    acb_clear(y);
    arb_clear(yk);
    arb_clear(r);
}

/* Statuses and the state of the outputs at a prime: exact 0 (DOMAIN), non-finite inputs (DOMAIN), prec above
   the cap (LIMIT, first: also for a non-finite input and for the exact pole), prec 1 and 2, the cap itself. */
ADF_TEST(prime_statuses_and_outputs)
{
    acb_t s, y, y2;
    adf_place_t v = prime_place(2), w;
    acb_init(s);
    acb_init(y);
    acb_init(y2);
    acb_zero(s);
    ADF_CHECK(call(y, s, v, 128) == ADF_DOMAIN);
    ADF_CHECK(call(y, s, prime_place(UWORD(18446744073709551557)), 16) == ADF_DOMAIN);
    ADF_CHECK(call(y, s, v, ADF_REAL_PREC_MAX + 1) == ADF_LIMIT);
    ADF_CHECK(call(y, s, v, WORD_MAX) == ADF_LIMIT);
    /* non-finite inputs */
    acb_one(s);
    arf_nan(arb_midref(acb_realref(s)));
    ADF_CHECK(call(y, s, v, 64) == ADF_DOMAIN);
    ADF_CHECK(call(y, s, adf_place_inf(), 64) == ADF_DOMAIN);
    ADF_CHECK(call(y, s, v, ADF_REAL_PREC_MAX + 1) == ADF_LIMIT);
    acb_one(s);
    arf_pos_inf(arb_midref(acb_imagref(s)));
    ADF_CHECK(call(y, s, v, 64) == ADF_DOMAIN);
    acb_one(s);
    mag_inf(arb_radref(acb_realref(s)));
    ADF_CHECK(call(y, s, v, 64) == ADF_DOMAIN);
    /* precision below 2 is 2 */
    acb_set_si(s, 1);
    ADF_CHECK(call(y, s, v, 1) == ADF_OK);
    ADF_CHECK(call(y2, s, v, 2) == ADF_OK);
    ADF_CHECK(acb_equal(y, y2));
    ADF_CHECK(call(y2, s, v, -5) == ADF_OK);
    ADF_CHECK(acb_equal(y, y2));
    ADF_CHECK(arb_contains_si(acb_realref(y), 2));
    /* the cap itself is permitted */
    ADF_CHECK(adf_local_zeta_factor_at(y, NULL, s, v, ADF_REAL_PREC_MAX) == ADF_OK);
    ADF_CHECK(arb_contains_si(acb_realref(y), 2) && arb_rel_accuracy_bits(acb_realref(y)) > 2000000);
    /* where untouched on OK, written on a status, also by a direct call */
    w = sentinel_place();
    ADF_CHECK(adf_local_zeta_factor_at(y, &w, s, v, 64) == ADF_OK && adf_place_equal(w, sentinel_place()));
    acb_zero(s);
    ADF_CHECK(adf_local_zeta_factor_at(y, &w, s, v, 64) == ADF_DOMAIN && adf_place_equal(w, v));
    acb_clear(s);
    acb_clear(y);
    acb_clear(y2);
}

/* The handle constructor refuses 0, 1 and composites (place.h) and leaves its output untouched; so no invalid
   handle reaches the function except a forged one (a precondition violation, checked under INV below). */
ADF_TEST(prime_handles)
{
    static const ulong bad[7] = {0, 1, 4, 9, 65535, UWORD(18446744073709551615), UWORD(18446744073709551559)};
    adf_place_t v = sentinel_place();
    for (int i = 0; i < 7; i++)
    {
        ADF_CHECK(adf_place_prime(&v, bad[i]) == ADF_DOMAIN);
        ADF_CHECK(adf_place_equal(v, sentinel_place()));
    }
    ADF_CHECK(adf_place_prime(&v, UWORD(18446744073709551557)) == ADF_OK);
    ADF_CHECK(adf_place_prime_get(v) == UWORD(18446744073709551557));
}

/* Extreme arguments at p = 2: Re(s) = 2^1000 gives a value in [1, 1 + 2^-1000]; Re(s) = -2^1000 gives
   -1/(2^(2^1000) - 1), in [-2^(-2^1000) (1 + 2^-100), -2^(-2^1000)]; s = 1 + i 2^1000 is OK and contains the
   value at 2400 bits; s = i 2^1000 is not recognised as a pole and is NOT_DETERMINED (design Z3, Z5 item 8). */
ADF_TEST(prime_extreme_arguments)
{
    acb_t s, y, ref, base;
    arb_t lo, hi;
    fmpz_t e;
    adf_place_t v = prime_place(2);
    acb_init(s);
    acb_init(y);
    acb_init(ref);
    acb_init(base);
    arb_init(lo);
    arb_init(hi);
    fmpz_init(e);
    acb_zero(s);
    arf_set_si_2exp_si(arb_midref(acb_realref(s)), 1, 1000);
    ADF_CHECK(call(y, s, v, 128) == ADF_OK);
    arb_one(lo);
    arb_one(hi);
    arb_mul_2exp_si(hi, hi, -1000);
    arb_add_ui(hi, hi, 1, 2000);
    ADF_CHECK(arb_contains(acb_realref(y), lo) && arb_contains(acb_realref(y), hi));
    ADF_CHECK(arb_contains_zero(acb_imagref(y)));
    ADF_CHECK(mag_cmp_2exp_si(arb_radref(acb_realref(y)), -100) < 0);
    /* Re(s) = -2^1000 */
    arf_neg(arb_midref(acb_realref(s)), arb_midref(acb_realref(s)));
    ADF_CHECK(call(y, s, v, 128) == ADF_OK);
    fmpz_one(e);
    fmpz_mul_2exp(e, e, 1000);
    fmpz_neg(e, e);
    arb_one(lo);
    arb_mul_2exp_fmpz(lo, lo, e);
    arb_neg(lo, lo);
    arb_one(hi);
    arb_mul_2exp_si(hi, hi, -100);
    arb_add_ui(hi, hi, 1, 300);
    arb_mul(hi, hi, lo, 300);
    ADF_CHECK(arb_contains(acb_realref(y), lo) && arb_contains(acb_realref(y), hi));
    ADF_CHECK(arb_contains_zero(acb_imagref(y)));
    /* s = 1 + i 2^1000 */
    acb_zero(s);
    arb_one(acb_realref(s));
    arf_set_si_2exp_si(arb_midref(acb_imagref(s)), 1, 1000);
    ADF_CHECK(call(y, s, v, 128) == ADF_OK);
    acb_set_ui(base, 2);
    acb_neg(ref, s);
    acb_pow(ref, base, ref, 2400);
    acb_sub_ui(ref, ref, 1, 2400);
    acb_neg(ref, ref);
    acb_inv(ref, ref, 2400);
    ADF_CHECK(acb_rel_accuracy_bits(ref) > 1000);
    ADF_CHECK(acb_contains(y, ref));
    /* s = i 2^1000 */
    arb_zero(acb_realref(s));
    ADF_CHECK(call(y, s, v, 128) == ADF_NOT_DETERMINED);
    acb_clear(s);
    acb_clear(y);
    acb_clear(ref);
    acb_clear(base);
    arb_clear(lo);
    arb_clear(hi);
    fmpz_clear(e);
}

ADF_TEST(prime_vectors)
{
    const char *full = getenv("ADF_ZETA_FIXTURES");
    run_vectors(VECTORS, 0);
    if (full != NULL && full[0] != '\0') run_vectors(full, 0);
}

/* ======================= slice B: the real place ======================= */

/* Closed forms at integers (notes.txt:1014-1016 with Gamma(1/2) = sqrt(pi) and Gamma(z+1) = z Gamma(z),
   :58-60): s = 2m: (m-1)!/pi^m; s = 2m+1 >= 1: (2m)!/(4^m m! pi^m); s = 1-2m <= -1: (-4 pi)^m m!/(2m)!.
   s = 2: 1/pi, s = 1: 1, s = 4: 1/pi^2, s = -1: -2 pi. No Gamma value is used. */
static void real_closed_form(arb_t r, slong s, slong bits)
{
    arb_t pi, t;
    arb_init(pi);
    arb_init(t);
    arb_const_pi(pi, bits);
    if (s > 0 && s % 2 == 0)
    {
        ulong m = (ulong) s / 2;
        arb_fac_ui(r, m - 1, bits);
        arb_pow_ui(t, pi, m, bits);
        arb_div(r, r, t, bits);
    }
    else if (s > 0)
    {
        ulong m = (ulong) (s - 1) / 2;
        arb_fac_ui(r, 2 * m, bits);
        arb_fac_ui(t, m, bits);
        arb_div(r, r, t, bits);
        arb_mul_2exp_si(r, r, -2 * (slong) m);
        arb_pow_ui(t, pi, m, bits);
        arb_div(r, r, t, bits);
    }
    else
    {
        ulong m = (ulong) (1 - s) / 2;
        arb_mul_si(t, pi, -4, bits);
        arb_pow_ui(r, t, m, bits);
        arb_fac_ui(t, m, bits);
        arb_mul(r, r, t, bits);
        arb_fac_ui(t, 2 * m, bits);
        arb_div(r, r, t, bits);
    }
    arb_clear(pi);
    arb_clear(t);
}

ADF_TEST(real_exact_values)
{
    static const slong precs[4] = {2, 16, 64, 256};
    acb_t s, y;
    arb_t r;
    adf_place_t inf = adf_place_inf();
    acb_init(s);
    acb_init(y);
    arb_init(r);
    for (slong k = -41; k <= 41; k++)
    {
        if (k <= 0 && k % 2 == 0) continue;
        real_closed_form(r, k, 1100);
        for (int i = 0; i < 4; i++)
        {
            set_point_si(s, k, 0);
            ADF_CHECK_MSG(call(y, s, inf, precs[i]) == ADF_OK, "s=%ld prec=%ld", (long) k, (long) precs[i]);
            ADF_CHECK_MSG(arb_contains(acb_realref(y), r), "s=%ld prec=%ld: value outside", (long) k,
                          (long) precs[i]);
            ADF_CHECK(arb_contains_zero(acb_imagref(y)));
            if (precs[i] >= 64)
                ADF_CHECK_MSG(arb_rel_accuracy_bits(acb_realref(y)) >= precs[i] - 12,
                              "s=%ld prec=%ld: accuracy %ld", (long) k, (long) precs[i],
                              (long) arb_rel_accuracy_bits(acb_realref(y)));
        }
    }
    set_point_si(s, 1, 0);
    ADF_CHECK(call(y, s, inf, 128) == ADF_OK && arb_contains_si(acb_realref(y), 1));
    acb_clear(s);
    acb_clear(y);
    arb_clear(r);
}

/* y contains the 1100-bit value at the centre and the corners of s (reference_real, a cross-check). */
static int real_contains_corners(const acb_t y, const acb_t s)
{
    acb_t pt, ref;
    arf_t r;
    int ok = 1;
    acb_init(pt);
    acb_init(ref);
    arf_init(r);
    for (int i = -1; i <= 1; i++)
        for (int j = -1; j <= 1; j++)
        {
            if ((i == 0) != (j == 0)) continue;
            acb_get_mid(pt, s);
            arf_set_mag(r, arb_radref(acb_realref(s)));
            arf_mul_si(r, r, i, ARF_PREC_EXACT, ARF_RND_DOWN);
            arf_add(arb_midref(acb_realref(pt)), arb_midref(acb_realref(pt)), r, ARF_PREC_EXACT, ARF_RND_DOWN);
            arf_set_mag(r, arb_radref(acb_imagref(s)));
            arf_mul_si(r, r, j, ARF_PREC_EXACT, ARF_RND_DOWN);
            arf_add(arb_midref(acb_imagref(pt)), arb_midref(acb_imagref(pt)), r, ARF_PREC_EXACT, ARF_RND_DOWN);
            reference_real(ref, pt, 1100);
            if (!acb_contains(y, ref)) ok = 0;
        }
    acb_clear(pt);
    acb_clear(ref);
    arf_clear(r);
    return ok;
}

/* The poles 0, -2, ..., -40 (Z2): exact ones are DOMAIN; balls of radius 10^-j around them, horizontal and
   vertical segments through them, and closed boxes with a pole on their boundary are NOT_DETERMINED; balls at
   distance 16 r (to the right, and in the imaginary direction) with radius r are OK. Exact -2^1000 is DOMAIN. */
ADF_TEST(real_poles)
{
    static const int js[5] = {1, 4, 12, 30, 60};
    acb_t s, y;
    arb_t r;
    adf_place_t inf = adf_place_inf();
    long nd = 0, ok = 0;
    acb_init(s);
    acb_init(y);
    arb_init(r);
    for (slong n = 0; n <= 20; n++)
    {
        set_point_si(s, -2 * n, 0);
        ADF_CHECK_MSG(call(y, s, inf, 128) == ADF_DOMAIN, "exact pole %ld", (long) (-2 * n));
        ADF_CHECK(call(y, s, inf, 2) == ADF_DOMAIN);
        for (int jj = 0; jj < 5; jj++)
        {
            arb_set_ui(r, 10);
            arb_pow_ui(r, r, (ulong) js[jj], 300);
            arb_inv(r, r, 300);
            set_point_si(s, -2 * n, 0);
            arb_get_mag(arb_radref(acb_realref(s)), r);
            arb_get_mag(arb_radref(acb_imagref(s)), r);
            ADF_CHECK_MSG(call(y, s, inf, 256) == ADF_NOT_DETERMINED, "around %ld j=%d", (long) (-2 * n),
                          js[jj]);
            ADF_CHECK(call(y, s, inf, 16) == ADF_NOT_DETERMINED);
            mag_zero(arb_radref(acb_imagref(s)));
            ADF_CHECK_MSG(call(y, s, inf, 256) == ADF_NOT_DETERMINED, "segment %ld j=%d", (long) (-2 * n),
                          js[jj]);
            arb_get_mag(arb_radref(acb_imagref(s)), r);
            mag_zero(arb_radref(acb_realref(s)));
            ADF_CHECK_MSG(call(y, s, inf, 256) == ADF_NOT_DETERMINED, "vertical %ld j=%d", (long) (-2 * n),
                          js[jj]);
            nd += 4;
            /* to the right: x = -2n + 16 r, radius r */
            arb_get_mag(arb_radref(acb_realref(s)), r);
            arf_set_mag(arb_midref(acb_realref(s)), arb_radref(acb_realref(s)));
            arf_mul_ui(arb_midref(acb_realref(s)), arb_midref(acb_realref(s)), 16, ARF_PREC_EXACT,
                       ARF_RND_DOWN);
            arf_sub_ui(arb_midref(acb_realref(s)), arb_midref(acb_realref(s)), (ulong) (2 * n), ARF_PREC_EXACT,
                       ARF_RND_DOWN);
            ADF_CHECK_MSG(call(y, s, inf, 256) == ADF_OK, "near %ld j=%d", (long) (-2 * n), js[jj]);
            ADF_CHECK_MSG(real_contains_corners(y, s), "near %ld j=%d: reference outside", (long) (-2 * n),
                          js[jj]);
            /* in the imaginary direction: -2n + 16 r i, radius r */
            arf_set_si(arb_midref(acb_realref(s)), -2 * n);
            arf_set_mag(arb_midref(acb_imagref(s)), arb_radref(acb_imagref(s)));
            arf_mul_ui(arb_midref(acb_imagref(s)), arb_midref(acb_imagref(s)), 16, ARF_PREC_EXACT,
                       ARF_RND_DOWN);
            ADF_CHECK_MSG(call(y, s, inf, 256) == ADF_OK, "near_imaginary %ld j=%d", (long) (-2 * n), js[jj]);
            ADF_CHECK_MSG(real_contains_corners(y, s), "near_imaginary %ld j=%d: reference outside",
                          (long) (-2 * n), js[jj]);
            ok += 2;
        }
    }
    /* closed boundaries: [-2, 0], [0, 2], [-4, -2] + i [0, 2] (the pole -2 on a corner) */
    acb_set_si(s, -1);
    mag_one(arb_radref(acb_realref(s)));
    ADF_CHECK(call(y, s, inf, 128) == ADF_NOT_DETERMINED);
    acb_set_si(s, 1);
    mag_one(arb_radref(acb_realref(s)));
    ADF_CHECK(call(y, s, inf, 128) == ADF_NOT_DETERMINED);
    acb_set_si_si(s, -3, 1);
    mag_one(arb_radref(acb_realref(s)));
    mag_one(arb_radref(acb_imagref(s)));
    ADF_CHECK(call(y, s, inf, 128) == ADF_NOT_DETERMINED);
    /* a regular point above a pole is OK */
    acb_set_si_si(s, -2, 1);
    ADF_CHECK(call(y, s, inf, 128) == ADF_OK);
    ADF_CHECK(real_contains_corners(y, s));
    /* exact -2^1000: an exact pole */
    acb_zero(s);
    arf_set_si_2exp_si(arb_midref(acb_realref(s)), -1, 1000);
    ADF_CHECK(call(y, s, inf, 128) == ADF_DOMAIN);
    /* -2^1000 + 1 is odd: a regular point whose value FLINT may not certify; never OK with a non-finite ball */
    arf_add_ui(arb_midref(acb_realref(s)), arb_midref(acb_realref(s)), 1, ARF_PREC_EXACT, ARF_RND_DOWN);
    {
        int st = call(y, s, inf, 128);
        ADF_CHECK_MSG(st == ADF_OK || st == ADF_NOT_DETERMINED || st == ADF_LIMIT, "odd -2^1000+1: %d", st);
    }
    printf("real poles: %ld NOT_DETERMINED calls, %ld OK calls with cross-check containment\n", nd, ok);
    acb_clear(s);
    acb_clear(y);
    arb_clear(r);
}

/* The design's direct-Gamma failure (local-zeta.md:594-597): S = [-2.1, -1.9] + i [1.5, 1.7] is pole-free; on
   FLINT 3.0.1 acb_gamma(S/2) is also non-finite at 48, 160 and 288 bits (lanes/f-slice14/probes/gamma_probe.c),
   so the value comes from the recurrence: OK, and it contains the values at the centre and the corners. The
   recurrence bound: S = [-124 +/- 1] + i [0.5 +/- 0.375] needs n = 64 factors (min Re(S/2) = -62.5) and is not
   LIMIT; [-126 +/- 1] + i [0.5 +/- 0.375] needs n = 65 and is LIMIT, as is the fixture box [-131, -129] +
   i [0.15, 0.35] (n = 67). The direct Gamma of such boxes is non-finite on FLINT 3.0.1 (same probe). */
ADF_TEST(real_recurrence)
{
    acb_t s, y, y2;
    arb_t t;
    adf_place_t inf = adf_place_inf();
    int st;
    acb_init(s);
    acb_init(y);
    acb_init(y2);
    arb_init(t);
    acb_set_si(s, -2);
    ADF_CHECK(arb_set_str(acb_imagref(s), "1.6", 64) == 0);
    ADF_CHECK(arb_set_str(t, "0 +/- 0.1", 64) == 0);
    arb_add(acb_realref(s), acb_realref(s), t, 64);
    arb_add(acb_imagref(s), acb_imagref(s), t, 64);
    ADF_CHECK(arb_contains_si(acb_realref(s), -2) && !arb_contains_zero(acb_imagref(s)));
    st = call(y, s, inf, 256);
    ADF_CHECK_MSG(st == ADF_OK, "design counterexample: status %d", st);
    if (st == ADF_OK) ADF_CHECK(real_contains_corners(y, s));
    st = call(y, s, inf, 16);
    ADF_CHECK_MSG(st == ADF_OK, "design counterexample at prec 16: status %d", st);
    if (st == ADF_OK) ADF_CHECK(real_contains_corners(y, s));
    /* n = 64: not LIMIT */
    acb_zero(s);
    arf_set_si(arb_midref(acb_realref(s)), -124);
    mag_one(arb_radref(acb_realref(s)));
    arf_set_si_2exp_si(arb_midref(acb_imagref(s)), 1, -1);
    mag_set_ui_2exp_si(arb_radref(acb_imagref(s)), 3, -3);
    st = call(y, s, inf, 128);
    ADF_CHECK_MSG(st == ADF_OK || st == ADF_NOT_DETERMINED, "n = 64: status %d", st);
    if (st == ADF_OK) ADF_CHECK(real_contains_corners(y, s));
    /* n = 65: LIMIT */
    arf_set_si(arb_midref(acb_realref(s)), -126);
    ADF_CHECK(call(y, s, inf, 128) == ADF_LIMIT);
    /* the fixture box */
    arf_set_si(arb_midref(acb_realref(s)), -130);
    arf_set_si_2exp_si(arb_midref(acb_imagref(s)), 1, -2);
    ADF_CHECK(arb_set_str(t, "0 +/- 0.1", 64) == 0);
    mag_set(arb_radref(acb_imagref(s)), arb_radref(t));
    ADF_CHECK(call(y, s, inf, 256) == ADF_LIMIT);
    /* LIMIT from prec first, also at an exact pole */
    acb_zero(s);
    ADF_CHECK(call(y, s, inf, ADF_REAL_PREC_MAX + 1) == ADF_LIMIT);
    ADF_CHECK(call(y, s, inf, 128) == ADF_DOMAIN);
    /* prec below 2 is 2 */
    acb_set_si(s, 3);
    ADF_CHECK(call(y, s, inf, 1) == ADF_OK && call(y2, s, inf, 2) == ADF_OK && acb_equal(y, y2));
    /* 2^1000 + i: Gamma overflows to a non-finite ball on FLINT 3.0.1; never OK with a non-finite ball (call()
       checks finiteness on OK) */
    acb_zero(s);
    arf_set_si_2exp_si(arb_midref(acb_realref(s)), 1, 1000);
    arb_one(acb_imagref(s));
    st = call(y, s, inf, 128);
    ADF_CHECK(st == ADF_OK || st == ADF_NOT_DETERMINED);
    acb_clear(s);
    acb_clear(y);
    acb_clear(y2);
    arb_clear(t);
}

ADF_TEST(real_vectors)
{
    const char *full = getenv("ADF_ZETA_FIXTURES");
    run_vectors(VECTORS, 1);
    if (full != NULL && full[0] != '\0') run_vectors(full, 1);
}

/* Review F2: zero is on the CLOSED boundary, in a segment or at a corner.
   These are exact dyadic contacts, without an approximate pole lattice construction.
   Z4 step 3 needs an upper Eplus even when its ideal value equals the midpoint denominator. */
ADF_TEST(prime_closed_zero_boundary)
{
    static const slong precs[] = {2, 16, 64, 128, 256};
    acb_t s, y;
    long calls = 0;
    acb_init(s);
    acb_init(y);
    for (int a = 0; a < 6; a++)
        for (slong e = -10; e <= 3; e++)
            for (int sx = -1; sx <= 1; sx += 2)
                for (int sy = -1; sy <= 1; sy++)
                    for (size_t k = 0; k < sizeof(precs)/sizeof(precs[0]); k++)
                    {
                        acb_zero(s);
                        arf_set_si_2exp_si(arb_midref(acb_realref(s)), sx, e);
                        mag_set_ui_2exp_si(arb_radref(acb_realref(s)), 1, e);
                        if (sy != 0)
                        {
                            arf_set_si_2exp_si(arb_midref(acb_imagref(s)), sy, e);
                            mag_set_ui_2exp_si(arb_radref(acb_imagref(s)), 1, e);
                        }
                        int st = call(y, s, prime_place(lattice_primes[a]), precs[k]);
                        ADF_CHECK_MSG(st == ADF_NOT_DETERMINED,
                                      "closed zero p=%lu e=%ld sx=%d sy=%d prec=%ld: status %d",
                                      (unsigned long) lattice_primes[a], (long) e, sx, sy,
                                      (long) precs[k], st);
                        calls++;
                    }
    printf("prime closed zero boundary: %ld NOT_DETERMINED calls\n", calls);
    acb_clear(s);
    acb_clear(y);
}

/* Review F3: the endpoint pole test precedes Gamma evaluation and the recurrence size limit.
   Widths are 2^-20, 1/2 and 1, so every endpoint is stored exactly. */
ADF_TEST(real_closed_pole_before_limit)
{
    static const slong ns[] = {1, 2, 33, 64, 65, 100, 1000};
    static const slong widths[] = {-20, -1, 0};
    static const slong precs[] = {2, 16, 64, 128, 256};
    acb_t s, y;
    arf_t r;
    long calls = 0;
    acb_init(s);
    acb_init(y);
    arf_init(r);
    for (size_t a = 0; a < sizeof(ns)/sizeof(ns[0]); a++)
        for (size_t b = 0; b < sizeof(widths)/sizeof(widths[0]); b++)
            for (int sign = -1; sign <= 1; sign += 2)
                for (size_t k = 0; k < sizeof(precs)/sizeof(precs[0]); k++)
                {
                    acb_set_si(s, -2*ns[a]);
                    arf_set_si_2exp_si(r, sign, widths[b]-1);
                    arf_add(arb_midref(acb_realref(s)), arb_midref(acb_realref(s)), r,
                            ARF_PREC_EXACT, ARF_RND_DOWN);
                    mag_set_ui_2exp_si(arb_radref(acb_realref(s)), 1, widths[b]-1);
                    int st = call(y, s, adf_place_inf(), precs[k]);
                    ADF_CHECK_MSG(st == ADF_NOT_DETERMINED,
                                  "closed pole n=%ld width=2^%ld side=%d prec=%ld: status %d",
                                  (long) ns[a], (long) widths[b], sign, (long) precs[k], st);
                    calls++;
                }
    printf("real closed poles before limit: %ld NOT_DETERMINED calls\n", calls);
    arf_clear(r);
    acb_clear(s);
    acb_clear(y);
}

/* Compact integer arrays in the independent reference file. */
static slong repair_integer(const jsonl_value *a, size_t i)
{
    jsonl_error_t err;
    const char *text = NULL;
    const jsonl_value *v = jsonl_at(a, i, &err);
    int ok = v && jsonl_int_text_or_string(v, &text, &err);
    ADF_CHECK(ok);
    return ok ? strtol(text, NULL, 10) : 0;
}

/* A certified cell [k*2^e,(k+1)*2^e], stored as [k,e], or exact zero stored as null.
   The exporter puts its entire Z8 integral reference inside the cell. */
static void repair_cell(arb_t out, const jsonl_value *v)
{
    if (jsonl_is(v, JSONL_NULL)) arb_zero(out);
    else
    {
        slong k = repair_integer(v, 0), e = repair_integer(v, 1);
        arf_set_si_2exp_si(arb_midref(out), 2*k+1, e-1);
        mag_set_ui_2exp_si(arb_radref(out), 1, e-1);
    }
}

/* Review F1: full product of four poles, 12 distances, 14 radius ratios and two shapes.
   References use proto/zeta_checks.py Z8, not Gamma or the function under test.
   Box 0 is [-17/256,-15/256]; box 1 is -2^-4 +/- 2^-14.
   Each input record is followed by its three or five reference records, with lines at most 116 chars. */
ADF_TEST(real_certified_near_poles)
{
    static const slong precs[] = {16, 32, 64, 128, 256};
    const char *path = "tests/ref/vectors/f-repair9/near-poles.jsonl";
    jsonl_file *file = NULL;
    jsonl_error_t err;
    acb_t s, y, ref;
    long calls = 0, samples = 0, boxes = 0;
    ADF_CHECK_MSG(jsonl_open(path, &file, &err), "%s", jsonl_error_message(&err));
    if (!file) return;
    ADF_CHECK(jsonl_count(file) == 6728);
    acb_init(s);
    acb_init(y);
    acb_init(ref);
    for (size_t i = 0; i < jsonl_count(file); i++)
    {
        const jsonl_value *row = jsonl_record(file, i), *input = sub_field(row, "s");
        size_t n = (size_t) atol(str_field(row, "n"));
        acb_zero(s);
        arf_set_si_2exp_si(arb_midref(acb_realref(s)), repair_integer(input, 0), repair_integer(input, 1));
        arf_set_si_2exp_si(arb_midref(acb_imagref(s)), repair_integer(input, 2), repair_integer(input, 3));
        mag_set_ui_2exp_si(arb_radref(acb_realref(s)), 1, repair_integer(input, 4));
        const jsonl_value *ir = jsonl_at(input, 5, &err);
        int real = jsonl_is(ir, JSONL_NULL);
        if (!real) mag_set_ui_2exp_si(arb_radref(acb_imagref(s)), 1, repair_integer(input, 5));
        ADF_CHECK(n == (real ? 3 : 5));
        ADF_CHECK(i+n < jsonl_count(file));
        if (i+n >= jsonl_count(file)) break;
        for (size_t k = 0; k < sizeof(precs)/sizeof(precs[0]); k++)
        {
            int st = call(y, s, adf_place_inf(), precs[k]);
            ADF_CHECK_MSG(st == ADF_OK, "near-pole box %ld prec=%ld: status %d", boxes, (long) precs[k], st);
            calls++;
            if (st != ADF_OK || !acb_is_finite(y)) continue;
            for (size_t j = 0; j < n; j++)
            {
                const jsonl_value *point = sub_field(jsonl_record(file, i+j+1), "v");
                repair_cell(acb_realref(ref), jsonl_at(point, 0, &err));
                repair_cell(acb_imagref(ref), jsonl_at(point, 1, &err));
                ADF_CHECK_MSG(acb_contains(y, ref), "near-pole box %ld sample %zu prec=%ld: outside",
                              boxes, j, (long) precs[k]);
                samples++;
            }
        }
        boxes++;
        i += n;
    }
    ADF_CHECK(boxes == 1346);
    ADF_CHECK(calls == 6730);
    ADF_CHECK(samples == 26910);
    printf("certified near poles: %ld calls, %ld endpoint/corner/midpoint samples\n", calls, samples);
    acb_clear(s);
    acb_clear(y);
    acb_clear(ref);
    jsonl_close(file);
}

#ifdef ADF_CHECK_INVARIANTS
/* The debug entry check (conventions 4.4): a forged handle (the word 4, 1, or 2^64 - 1, which adf_place_prime
   refuses) aborts with a line that names the function; a valid one does not. The prec LIMIT comes first
   (localfactor.h), so a forged handle with prec above the cap returns LIMIT. A child exiting with 42 means a
   missing check. */
ADF_TEST(debug_entry_check)
{
    static const ulong forged[3] = {4, 1, UWORD(18446744073709551615)};
    for (int which = 0; which < 4; which++)
    {
        int fd[2];
        int opened = pipe(fd);
        ADF_CHECK(opened == 0);
        if (opened != 0) return;
        pid_t child = fork();
        ADF_CHECK(child >= 0);
        if (child == 0)
        {
            acb_t s, y;
            adf_place_t v = adf_place_inf();
            close(fd[0]);
            dup2(fd[1], 2);
            acb_init(s);
            acb_init(y);
            acb_one(s);
            if (which < 3)
            {
                v.opaque = forged[which];
                (void) adf_local_zeta_factor_at(y, NULL, s, v, 64);
            }
            else
            {
                v.opaque = 4;
                if (adf_local_zeta_factor_at(y, NULL, s, v, ADF_REAL_PREC_MAX + 1) == ADF_LIMIT) _exit(43);
            }
            acb_clear(s);
            acb_clear(y);
            _exit(42);
        }
        if (child > 0)
        {
            int status = 0;
            char message[2048];
            close(fd[1]);
            ssize_t len = read(fd[0], message, sizeof(message) - 1);
            close(fd[0]);
            message[len > 0 ? (size_t) len : 0] = '\0';
            ADF_CHECK(waitpid(child, &status, 0) == child);
            if (which < 3)
            {
                ADF_CHECK_MSG(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT, "forged %d not caught", which);
                ADF_CHECK(strstr(message, "adf_local_zeta_factor_at") != NULL);
            }
            else ADF_CHECK(WIFEXITED(status) && WEXITSTATUS(status) == 43);
        }
        else { close(fd[0]); close(fd[1]); }
    }
}
#endif
