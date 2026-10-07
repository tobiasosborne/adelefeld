/* Slice c, api-3c 3/P1/P2, conventions 5.13 and ideles P15. */
#define _POSIX_C_SOURCE 200809L
#include <adelefeld.h>
#include <flint/dirichlet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <sys/wait.h>
#include <unistd.h>
#include "support/jsonl.h"

static long checks;
#define CHECK(c) do { checks++; if (!(c)) { \
    fprintf(stderr, "test_char_class:%d: %s\n", __LINE__, #c); abort(); } } while (0)
static jsonl_error_t err;
static const jsonl_value *field(const jsonl_value *v, const char *key)
{ const jsonl_value *p; CHECK(jsonl_field(v, key, &p, &err)); return p; }
static const jsonl_value *at(const jsonl_value *v, size_t i)
{ const jsonl_value *p = jsonl_at(v, i, &err); CHECK(p != NULL); return p; }
static const char *str(const jsonl_value *v)
{ size_t n; const char *s = jsonl_string(v, &n, &err); CHECK(s != NULL); return s; }
static ulong ui(const jsonl_value *v)
{ const char *s = jsonl_int_text(v, &err); CHECK(s != NULL); return strtoul(s, NULL, 10); }
static jsonl_file *vectors(const char *name, size_t count)
{
    char path[120]; jsonl_file *f;
    snprintf(path, sizeof(path), "tests/ref/vectors/c-slice3/%s.jsonl", name);
    CHECK(jsonl_open(path, &f, &err)); CHECK(jsonl_count(f) == count); return f;
}
static void rational(fmpq_t q, const jsonl_value *v)
{ CHECK(fmpq_set_str(q, str(v), 10) == 0); fmpq_canonicalise(q); }
static void endpoint(fmpq_t q, const jsonl_value *v)
{
    rational(q, v); fmpz_ui_pow_ui(fmpq_denref(q), 10, 60); fmpq_canonicalise(q);
}
static void input_ball(arb_t x, const jsonl_value *mid, const jsonl_value *rad)
{
    fmpq_t q; arb_t r; fmpq_init(q); arb_init(r);
    rational(q, mid); arb_set_fmpq(x, q, 512);
    rational(q, rad); arb_set_fmpq(r, q, 512); arb_add_error(x, r);
    fmpq_clear(q); arb_clear(r);
}
static void input_s(acb_t s, const jsonl_value *p)
{
    input_ball(acb_realref(s), at(p, 2), at(p, 3));
    input_ball(acb_imagref(s), at(p, 4), at(p, 5));
}
static void unit(adf_ucoset_t u, slong c, ulong N)
{
    fmpz_t a, m; fmpz_init(a); fmpz_init(m); fmpz_set_si(a, c); fmpz_set_ui(m, N);
    CHECK(adf_ucoset_set_fmpz2(u, a, m) == ADF_OK); fmpz_clear(a); fmpz_clear(m);
}
static void sentinel(acb_t z)
{ acb_set_si(z, 17); arb_set_si(acb_imagref(z), -19); arb_add_error_2exp_si(acb_realref(z), -9); }
static void unchanged(const acb_struct *bytes, const acb_t z)
{ CHECK(memcmp(bytes, z, sizeof(*bytes)) == 0); }

/* Bounds are against the design's rectangle product, not the generally smaller rotated
   finite-value hull. For each endpoint allow 256*M*2^-p + 2^-24*(W+M*2^-p), where
   M=max(1, magnitudes of oracle box endpoints), W is that coordinate's box width.
   Equivalently, excess over the exact point-value hull is its box overhang plus this
   allowance. The family extrema use a finite-candidate proof; the box encloses the continuum.
   This is a stated test bound, not a new public numerical certificate. */
static void hull_check(const acb_t z, const jsonl_value *image, slong p)
{
    fmpq_t q[4]; arb_t bound, width, scale, t, eps, excess; arf_t lo, hi;
    const jsonl_value *box = field(image, "box");
    const jsonl_value *hull = field(image, "hull");
    for (int i = 0; i < 4; i++) { fmpq_init(q[i]); endpoint(q[i], at(box, i)); }
    arb_init(bound); arb_init(width); arb_init(scale); arb_init(t); arb_init(eps); arb_init(excess);
    arf_init(lo); arf_init(hi); arb_one(scale);
    for (int i = 0; i < 4; i++) {
        arb_set_fmpq(t, q[i], 512); arb_abs(t, t); arb_max(scale, scale, t, 512);
    }
    arb_one(eps); arb_mul_2exp_si(eps, eps, -p); arb_mul(eps, eps, scale, 512);
    for (int k = 0; k < 2; k++) {
        arb_get_interval_arf(lo, hi, k ? acb_imagref(z) : acb_realref(z), ARF_PREC_EXACT);
        arb_set_fmpq(width, q[2*k+1], 512); arb_set_fmpq(t, q[2*k], 512);
        arb_sub(width, width, t, 512); arb_add(width, width, eps, 512);
        arb_mul_2exp_si(width, width, -24); arb_mul_2exp_si(bound, eps, 8);
        arb_add(bound, bound, width, 512);
        arb_sub_arf(excess, t, lo, 512); CHECK(arb_le(excess, bound));
        arb_set_arf(excess, hi); arb_set_fmpq(t, q[2*k+1], 512);
        arb_sub(excess, excess, t, 512); CHECK(arb_le(excess, bound));
        /* Each attained hull extremum has a certified 60-decimal outer endpoint. */
        endpoint(q[0], at(hull, 2*k)); endpoint(q[1], at(hull, 2*k+1));
        CHECK(arb_contains_fmpq(k ? acb_imagref(z) : acb_realref(z), q[0]));
        CHECK(arb_contains_fmpq(k ? acb_imagref(z) : acb_realref(z), q[1]));
    }
    for (int i = 0; i < 4; i++) fmpq_clear(q[i]);
    arb_clear(bound); arb_clear(width); arb_clear(scale); arb_clear(t); arb_clear(eps); arb_clear(excess);
    arf_clear(lo); arf_clear(hi);
}
static void values_check(const acb_t z, const jsonl_value *image, jsonl_file *values)
{
    const jsonl_value *ids = field(image, "values"); fmpq_t q; fmpq_init(q);
    for (size_t i = 0; i < jsonl_size(ids); i++) {
        const jsonl_value *v = jsonl_record(values, ui(at(ids, i)));
        for (int j = 0; j < 4; j++) {
            endpoint(q, at(v, j));
            if (!arb_contains_fmpq(j < 2 ? acb_realref(z) : acb_imagref(z), q))
                fprintf(stderr, "value id=%lu, coordinate=%d\n", ui(at(ids, i)), j);
            CHECK(arb_contains_fmpq(j < 2 ? acb_realref(z) : acb_imagref(z), q));
        }
    }
    fmpq_clear(q);
}
static void class_vectors(int strict_only)
{
    jsonl_file *cases = vectors("classes", 2172), *params = vectors("params", 25);
    jsonl_file *images = vectors("images", 143), *values = vectors("values", 640);
    adf_char_t chi, conjugate; adf_idclass_t x; adf_ucoset_t u; arb_t t; acb_t s, z, other;
    adf_char_init(chi); adf_char_init(conjugate); adf_idclass_init(x); adf_ucoset_init(u);
    arb_init(t); acb_init(s); acb_init(z); acb_init(other);
    for (size_t i = 0; i < jsonl_count(cases); i++) {
        const jsonl_value *v = jsonl_record(cases, i), *p = jsonl_record(params, ui(at(v, 4)));
        const jsonl_value *im = jsonl_record(images, ui(at(v, 5)));
        input_s(s, p); CHECK(adf_char_set_conrey_acb(chi, ui(at(v, 0)), ui(at(v, 1)), s) == ADF_OK);
        unit(u, (slong) strtol(jsonl_int_text(at(v, 2), &err), NULL, 10), ui(at(v, 3)));
        input_ball(t, at(p, 0), at(p, 1)); CHECK(adf_idclass_set_parts(x, t, u) == ADF_OK);
        for (int k = 0; k < 3; k++) {
            slong prec = k == 0 ? 2 : k == 1 ? 53 : 128; acb_struct bytes;
            if (strict_only) {
                sentinel(z); bytes = *z;
                CHECK(adf_char_eval_idclass_strict(z, chi, x, prec) ==
                      (ui(at(v, 6)) ? ADF_OK : ADF_NOT_DETERMINED));
                if (!ui(at(v, 6))) unchanged(&bytes, z);
                else { values_check(z, im, values); hull_check(z, im, prec); }
            } else {
                CHECK(adf_char_eval_idclass(z, chi, x, prec) == ADF_OK);
                values_check(z, im, values); hull_check(z, im, prec);
            }
        }
        if (!strict_only) {
            adf_char_conj(conjugate, chi); CHECK(adf_char_eval_idclass(other, conjugate, x, 128) == ADF_OK);
            acb_conj(other, other); CHECK(acb_overlaps(z, other));
            /* Algebraic conjugation of the entire set, including the uncertain s rectangle. */
            values_check(other, im, values);
        }
    }
    adf_char_clear(chi); adf_char_clear(conjugate); adf_idclass_clear(x); adf_ucoset_clear(u);
    arb_clear(t); acb_clear(s); acb_clear(z); acb_clear(other);
    jsonl_close(cases); jsonl_close(params); jsonl_close(images); jsonl_close(values);
}
static void idele_vectors(void)
{
    jsonl_file *cases = vectors("ideles", 128), *params = vectors("params", 25);
    jsonl_file *images = vectors("images", 143), *values = vectors("values", 640);
    adf_char_t chi; adf_idclass_t c; adf_idele_t x; adf_ucoset_t u;
    arb_t inf; fmpq_t r; acb_t s, z, other, negated;
    adf_char_init(chi); adf_idclass_init(c); adf_idele_init(x); adf_ucoset_init(u);
    arb_init(inf); fmpq_init(r); acb_init(s); acb_init(z); acb_init(other); acb_init(negated);
    for (size_t i = 0; i < jsonl_count(cases); i++) {
        const jsonl_value *v = jsonl_record(cases, i), *p = jsonl_record(params, ui(at(v, 7)));
        const jsonl_value *im = jsonl_record(images, ui(at(v, 8)));
        input_s(s, p); CHECK(adf_char_set_conrey_acb(chi, ui(at(v, 0)), ui(at(v, 1)), s) == ADF_OK);
        input_ball(inf, at(v, 2), at(v, 3)); rational(r, at(v, 4));
        unit(u, (slong) ui(at(v, 5)), ui(at(v, 6))); CHECK(adf_idele_set_parts(x, inf, r, u) == ADF_OK);
        for (int k = 0; k < 3; k++) {
            slong prec = k == 0 ? 2 : k == 1 ? 53 : 128; acb_struct bytes; int st;
            CHECK(adf_idclass_set_idele(c, x, prec) == ADF_OK);
            CHECK(adf_char_eval_idclass(other, chi, c, prec) == ADF_OK);
            CHECK(adf_char_eval_idele(z, chi, x, prec) == ADF_OK && acb_equal(z, other));
            values_check(z, im, values); hull_check(z, im, prec);
            /* Negating only the real coordinate changes chi by chi(-1), not a diagonal -1. */
            arb_neg(x->inf, x->inf); CHECK(adf_char_eval_idele(negated, chi, x, prec) == ADF_OK);
            arb_neg(x->inf, x->inf); if (chi->parity) acb_neg(negated, negated);
            CHECK(acb_equal(z, negated));
            sentinel(z); bytes = *z; sentinel(other);
            st = adf_char_eval_idclass_strict(other, chi, c, prec);
            CHECK(adf_char_eval_idele_strict(z, chi, x, prec) == st);
            if (st != ADF_OK) unchanged(&bytes, z); else CHECK(acb_equal(z, other));
        }
    }
    adf_char_clear(chi); adf_idclass_clear(c); adf_idele_clear(x); adf_ucoset_clear(u);
    arb_clear(inf); fmpq_clear(r); acb_clear(s); acb_clear(z); acb_clear(other); acb_clear(negated);
    jsonl_close(cases); jsonl_close(params); jsonl_close(images); jsonl_close(values);
}
static void edges(void)
{
    adf_char_t chi; adf_idclass_t c, saved; adf_idele_t x; acb_t z, s; arb_t t;
    adf_char_init(chi); adf_idclass_init(c); adf_idclass_init(saved); adf_idele_init(x);
    acb_init(z); acb_init(s); arb_init(t);
    /* api-3c 7 by hand: t=2, u'=-1, chi_3(2)(-1)=-1, so t^1 chi=-2.
       We promise containment and a bound. The real integer-power path also gives exact -2. */
    acb_one(s); CHECK(adf_char_set_conrey_acb(chi, 3, 2, s) == ADF_OK); arb_set_si(x->inf, -2);
    CHECK(adf_char_eval_idele(z, chi, x, 128) == ADF_OK); CHECK(arb_contains_si(acb_realref(z), -2));
    CHECK(acb_is_exact(z) && arb_is_zero(acb_imagref(z)));
    CHECK(mag_cmp_2exp_si(arb_radref(acb_realref(z)), -120) <= 0);
    unit(&x->u, -1, 0); arb_set_si(x->inf, -1);
    CHECK(adf_char_eval_idele(z, chi, x, 128) == ADF_OK && acb_is_one(z));
    jsonl_file *bad = vectors("invalid", 5); adf_idclass_set(saved, c);
    for (size_t i = 0; i < jsonl_count(bad); i++) {
        const jsonl_value *v = jsonl_record(bad, i); adf_idclass_struct bytes = *c;
        input_ball(t, at(v, 0), at(v, 1));
        CHECK(adf_idclass_set_parts(c, t, &saved->u) == ADF_DOMAIN);
        CHECK(!memcmp(&bytes, c, sizeof(bytes)) && adf_idclass_identical(c, saved));
    }
    jsonl_close(bad);
    /* The existing driver fixture fixes this printed Q4 successor radius, independently
       of the weaker containment/upper-width contract. Removing the successor prints 1. */
    adf_cadele_t printed; adf_fball_t finite_zero; size_t len; char *text;
    adf_cadele_init(printed); adf_fball_init(finite_zero);
    CHECK(adf_char_set_conrey(chi, 3, 2) == ADF_OK); unit(&c->u, 1, 1);
    CHECK(adf_char_eval_idclass(z, chi, c, 128) == ADF_OK);
    CHECK(adf_cadele_set_acb_fball(printed, z, finite_zero) == ADF_OK);
    text = adf_cadele_get_str(&len, printed, 2);
    CHECK(text != NULL && !strcmp(text, "((0 +/- 1.1) + (0)*i ; 0)"));
    adf_str_free(text);
    /* Class multiplication can add another radius ulp, masking a missing finite-hull
       successor. Check slice b's existing printed result before that multiplication. */
    CHECK(adf_char_eval_ucoset(z, chi, &c->u, 128) == ADF_OK);
    CHECK(adf_cadele_set_acb_fball(printed, z, finite_zero) == ADF_OK);
    text = adf_cadele_get_str(&len, printed, 2);
    CHECK(text != NULL && !strcmp(text, "((0 +/- 1.1) + (0)*i ; 0)"));
    adf_str_free(text); adf_cadele_clear(printed); adf_fball_clear(finite_zero); unit(&c->u, 1, 0);
    /* All four caps precede INVALID storage under INV, and preserve bytes. */
    for (int mode = 0; mode < 4; mode++) {
        acb_struct bytes; sentinel(z); bytes = *z; chi->q = 0;
        int st = mode == 0 ? adf_char_eval_idclass(z, chi, c, ADF_REAL_PREC_MAX+1) :
                 mode == 1 ? adf_char_eval_idclass_strict(z, chi, c, ADF_REAL_PREC_MAX+1) :
                 mode == 2 ? adf_char_eval_idele(z, chi, x, ADF_REAL_PREC_MAX+1) :
                             adf_char_eval_idele_strict(z, chi, x, ADF_REAL_PREC_MAX+1);
        CHECK(st == ADF_LIMIT); unchanged(&bytes, z); chi->q = 3;
    }
    for (int mode = 0; mode < 4; mode++) {
        acb_struct bytes; sentinel(z); bytes = *z; chi->q = ADF_CHAR_MOD_MAX+1;
        int st = mode == 0 ? adf_char_eval_idclass(z, chi, c, 53) :
                 mode == 1 ? adf_char_eval_idclass_strict(z, chi, c, 53) :
                 mode == 2 ? adf_char_eval_idele(z, chi, x, 53) : adf_char_eval_idele_strict(z, chi, x, 53);
        CHECK(st == ADF_LIMIT); unchanged(&bytes, z); chi->q = 3;
    }
    CHECK(adf_char_set_conrey(chi, ADF_CHAR_MOD_MAX, 5) == ADF_OK);
    for (int k = 0; k < 3; k++) {
        slong prec = k == 0 ? -100 : k == 1 ? 53 : ADF_REAL_PREC_MAX;
        CHECK(adf_char_eval_idclass(z, chi, c, prec) == ADF_OK && acb_is_one(z));
        CHECK(adf_char_eval_idclass_strict(z, chi, c, prec) == ADF_OK && acb_is_one(z));
        CHECK(adf_char_eval_idele(z, chi, x, prec) == ADF_OK && acb_is_one(z));
        CHECK(adf_char_eval_idele_strict(z, chi, x, prec) == ADF_OK && acb_is_one(z));
    }
    /* A valid idele whose norm cannot retain its positive sign at low precision. */
    arb_set_si(x->inf, -1); mag_set_ui_2exp_si(arb_radref(x->inf), (UWORD(1) << 30)-1, -30);
    fmpq_set_si(x->r, 5, 3);
    for (int strict = 0; strict < 2; strict++) {
        acb_struct bytes; sentinel(z); bytes = *z;
        int st = strict ? adf_char_eval_idele_strict(z, chi, x, 16) : adf_char_eval_idele(z, chi, x, 16);
        CHECK(st == ADF_NOT_DETERMINED); unchanged(&bytes, z);
    }
    adf_char_clear(chi); adf_idclass_clear(c); adf_idclass_clear(saved); adf_idele_clear(x);
    acb_clear(z); acb_clear(s); arb_clear(t);
}

#ifdef ADF_CHAR_CLASS_WRAP
static int phase_failure, bad_real_power, bad_complex_power, bad_product, conversion_failure, setup_failure;
static long powers, conversions, phases, setups;
int __real_dirichlet_group_init(dirichlet_group_t G, ulong q);
int __wrap_dirichlet_group_init(dirichlet_group_t G, ulong q)
{ setups++; return setup_failure ? 0 : __real_dirichlet_group_init(G, q); }
int __real_adf_phase_get_acb(acb_t z, const fmpq_t t, slong prec);
int __wrap_adf_phase_get_acb(acb_t z, const fmpq_t t, slong prec)
{ phases++; return phase_failure ? phase_failure : __real_adf_phase_get_acb(z, t, prec); }
void __real_arb_pow(arb_t z, const arb_t x, const arb_t y, slong prec);
void __wrap_arb_pow(arb_t z, const arb_t x, const arb_t y, slong prec)
{ powers++; if (bad_real_power) arb_indeterminate(z); else __real_arb_pow(z, x, y, prec); }
void __real_acb_pow(acb_t z, const acb_t x, const acb_t y, slong prec);
void __wrap_acb_pow(acb_t z, const acb_t x, const acb_t y, slong prec)
{ powers++; if (bad_complex_power) acb_indeterminate(z); else __real_acb_pow(z, x, y, prec); }
void __real_acb_mul(acb_t z, const acb_t x, const acb_t y, slong prec);
void __wrap_acb_mul(acb_t z, const acb_t x, const acb_t y, slong prec)
{ if (bad_product) acb_indeterminate(z); else __real_acb_mul(z, x, y, prec); }
int __real_adf_idclass_set_idele(adf_idclass_t c, const adf_idele_t x, slong prec);
int __wrap_adf_idclass_set_idele(adf_idclass_t c, const adf_idele_t x, slong prec)
{ conversions++; return conversion_failure ? conversion_failure : __real_adf_idclass_set_idele(c, x, prec); }
static void injected(void)
{
    adf_char_t chi; adf_idclass_t c; adf_idele_t x; acb_t z, s;
    adf_char_init(chi); adf_idclass_init(c); adf_idele_init(x); acb_init(z); acb_init(s);
    CHECK(adf_char_set_conrey(chi, 5, 2) == ADF_OK);
    for (int mode = 0; mode < 4; mode++) {
        for (int failure = 0; failure < 6; failure++) {
            acb_struct bytes; sentinel(z); bytes = *z; acb_one(s);
            if (failure == 3) arb_one(acb_imagref(s));
            CHECK(adf_char_set_s(chi, s) == ADF_OK);
            phase_failure = failure == 0 ? ADF_LIMIT : failure == 1 ? ADF_NOT_DETERMINED : 0;
            bad_real_power = failure == 2; bad_complex_power = failure == 3;
            bad_product = failure == 4;
            /* INV's predicate also sets up groups, so setup injection uses a principal input there. */
            if (failure == 5) {
#ifdef ADF_CHECK_INVARIANTS
                continue;
#else
                setup_failure = 1;
#endif
            }
            int st = mode == 0 ? adf_char_eval_idclass(z, chi, c, 53) :
                     mode == 1 ? adf_char_eval_idclass_strict(z, chi, c, 53) :
                     mode == 2 ? adf_char_eval_idele(z, chi, x, 53) : adf_char_eval_idele_strict(z, chi, x, 53);
            CHECK(st == (failure == 0 ? ADF_LIMIT : failure == 5 ? ADF_UNSUPPORTED : ADF_NOT_DETERMINED));
            unchanged(&bytes, z);
            phase_failure = bad_real_power = bad_complex_power = bad_product = setup_failure = 0;
        }
    }
    for (int failure = 0; failure < 2; failure++) {
        acb_struct bytes; sentinel(z); bytes = *z;
        long old_powers = powers, old_phases = phases;
        conversion_failure = failure ? ADF_LIMIT : ADF_NOT_DETERMINED;
        CHECK(adf_char_eval_idele(z, chi, x, 53) == conversion_failure); unchanged(&bytes, z);
        CHECK(adf_char_eval_idele_strict(z, chi, x, 53) == conversion_failure); unchanged(&bytes, z);
        CHECK(powers == old_powers && phases == old_phases); conversion_failure = 0;
    }
    CHECK(adf_char_set_conrey(chi, 1, 0) == ADF_OK);
    setup_failure = 1; long old_setups = setups;
    CHECK(adf_char_set_conrey_acb(chi, 1, 0, s) == ADF_OK);
    CHECK(setups == old_setups); setup_failure = 0;
    CHECK(adf_char_set_conrey(chi, 5, 2) == ADF_OK);
    unit(&c->u, 1, 1); unit(&x->u, 1, 1);
    bad_real_power = 1; long old_powers = powers, old_phases = phases;
    CHECK(adf_char_eval_idclass_strict(z, chi, c, 53) == ADF_NOT_DETERMINED);
    CHECK(adf_char_eval_idele_strict(z, chi, x, 53) == ADF_NOT_DETERMINED);
    CHECK(powers == old_powers && phases == old_phases); bad_real_power = 0;
    /* Bounds must precede even conversion and setup when the finite image is ambiguous. */
    for (int bound = 0; bound < 2; bound++) {
        long old_setups = setups, old_conversions = conversions;
        if (bound) chi->q = ADF_CHAR_MOD_MAX+1;
        slong prec = bound ? 53 : ADF_REAL_PREC_MAX+1;
        CHECK(adf_char_eval_idclass_strict(z, chi, c, prec) == ADF_LIMIT);
        CHECK(adf_char_eval_idele_strict(z, chi, x, prec) == ADF_LIMIT);
        CHECK(setups == old_setups && conversions == old_conversions); chi->q = 5;
    }
    adf_char_clear(chi); adf_idclass_clear(c); adf_idele_clear(x); acb_clear(z); acb_clear(s);
}
#endif

/* FLINT allocation balance, refs/src/flint-3.0.1/memory.rst:9-44.
   This observes FLINT allocations, not all GMP/runtime allocations. */
static void *(*old_alloc)(size_t);
static void *(*old_calloc)(size_t, size_t);
static void *(*old_realloc)(void *, size_t);
static void (*old_free)(void *);
static long live_blocks, allocations;
static void *count_alloc(size_t n)
{ void *p = old_alloc(n); if (p) { live_blocks++; allocations++; } return p; }
static void *count_calloc(size_t n, size_t size)
{ void *p = old_calloc(n, size); if (p) { live_blocks++; allocations++; } return p; }
static void *count_realloc(void *p, size_t n)
{
    void *q = old_realloc(p, n);
    if (!p && q) { live_blocks++; allocations++; }
    if (p && !n && !q) live_blocks--;
    return q;
}
static void count_free(void *p) { if (p) live_blocks--; old_free(p); }
static void memory_cycle(void)
{
    adf_char_t chi; adf_idclass_t c; adf_idele_t x; acb_t z, s;
    adf_char_init(chi); adf_idclass_init(c); adf_idele_init(x); acb_init(z); acb_init(s);
    acb_one(s); arb_set_si(acb_imagref(s), 3);
    CHECK(adf_char_set_conrey_acb(chi, 5, 2, s) == ADF_OK); arb_set_si(x->inf, -2);
    /* A heap-sized real endpoint accumulator makes a missing arf_clear observable,
       even when LeakSanitizer is unavailable. s is ignored by the finite Gauss sum. */
    CHECK(adf_char_gauss_sum(z, chi, 1024) == ADF_OK);
    for (int ambiguous = 0; ambiguous < 2; ambiguous++) {
        unit(&x->u, 1, ambiguous ? 1 : 0); CHECK(adf_idclass_set_idele(c, x, 53) == ADF_OK);
        CHECK(adf_char_eval_idclass(z, chi, c, 53) == ADF_OK);
        CHECK(adf_char_eval_idele(z, chi, x, 53) == ADF_OK);
        CHECK(adf_char_eval_idclass_strict(z, chi, c, 53) == (ambiguous ? ADF_NOT_DETERMINED : ADF_OK));
        CHECK(adf_char_eval_idele_strict(z, chi, x, 53) == (ambiguous ? ADF_NOT_DETERMINED : ADF_OK));
    }
    adf_char_clear(chi); adf_idclass_clear(c); adf_idele_clear(x); acb_clear(z); acb_clear(s);
}
static void memory(void)
{
    for (int j = 0; j < 10; j++) memory_cycle();
    __flint_get_memory_functions(&old_alloc, &old_calloc, &old_realloc, &old_free);
    __flint_set_memory_functions(count_alloc, count_calloc, count_realloc, count_free);
    for (int j = 0; j < 100; j++) { memory_cycle(); CHECK(live_blocks == 0); }
    __flint_set_memory_functions(old_alloc, old_calloc, old_realloc, old_free);
    printf("memory: %ld FLINT blocks, %ld live after 100 cycles\n", allocations, live_blocks);
}

#ifdef ADF_CHECK_INVARIANTS
static void invariants(void)
{
    for (int mode = 0; mode < 32; mode++) {
        fflush(stdout); pid_t child = fork(); CHECK(child >= 0);
        if (!child) {
            adf_char_t chi; adf_idclass_t c; adf_idele_t x; acb_t z;
            adf_char_init(chi); adf_idclass_init(c); adf_idele_init(x); acb_init(z);
            int which = mode % 4;
            if (mode < 4) chi->parity = 1;
            else if (mode < 8) { arb_zero(c->t); arb_zero(x->inf); }
            else if (mode < 12) {
                if (which < 2) arb_set_si(c->t, -1); else fmpq_zero(x->r);
            }
            acb_ptr output = z;
            if (mode >= 12) {
                uintptr_t member = mode < 24 ? (uintptr_t) chi->s :
                                   which < 2 ? (uintptr_t) c->t : (uintptr_t) x->inf;
                if (mode >= 16 && mode < 20) member += sizeof(arb_struct);
                if ((mode >= 20 && mode < 24) || mode >= 28) member -= sizeof(arb_struct);
                output = (acb_ptr) member;
            }
            if (which == 0) (void) adf_char_eval_idclass(output, chi, c, 53);
            if (which == 1) (void) adf_char_eval_idclass_strict(output, chi, c, 53);
            if (which == 2) (void) adf_char_eval_idele(output, chi, x, 53);
            if (which == 3) (void) adf_char_eval_idele_strict(output, chi, x, 53);
            /* Reached only if the entry check is missing. The clears are for tools/memcheck. */
            adf_char_clear(chi); adf_idclass_clear(c); adf_idele_clear(x); acb_clear(z);
            _exit(0);
        }
        int status; CHECK(waitpid(child, &status, 0) == child);
        CHECK(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
    }
}
#endif
int main(int argc, char **argv)
{
    if (argc == 1 || !strcmp(argv[1], "class")) class_vectors(0);
    if (argc == 1 || !strcmp(argv[1], "strict")) class_vectors(1);
    if (argc == 1 || !strcmp(argv[1], "idele")) { idele_vectors(); edges(); }
    if (argc == 1) memory();
#ifdef ADF_CHAR_CLASS_WRAP
    if (argc == 1) injected();
#endif
#ifdef ADF_CHECK_INVARIANTS
    if (argc == 1) invariants();
#endif
    flint_cleanup(); printf("test_char_class: %ld checks\n", checks); return 0;
}
