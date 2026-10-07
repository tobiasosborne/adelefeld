/* Slice 5a, docs/api-5.md:48-108; P9/P10 standard vectors, NOT gamma.
   Every JSONL row and every certified point enclosure is read. Width target:
   2*max(radius) <= 64*sampled diameter + 2^(-prec+8)*max(1,image).
   Points use independent scalar intervals and Gamma integration with tails.
   The output promises enclosure, so 4/3 need not be an exact acb. */
#include <adelefeld.h>
#include <adelefeld/localfactor.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
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

/* Define inactive arf bytes before init so byte-level snapshots are meaningful
   to memory checkers as well as C. This does not change a represented value. */
static void initialized_acb(acb_t x)
{
    memset(x, 0, sizeof(acb_struct));
    acb_init(x);
}

/* Interpose the first log call only while checking the NEW finite guard cap.
   The target Linux ABI has dlsym in libc. Forward all evaluation to FLINT;
   no reference value is computed by this instrumentation. */
static int watch_log_precision;
static slong first_log_precision;
void arb_log_ui(arb_t y, ulong n, slong prec)
{
    typedef void (*log_function)(arb_t, ulong, slong);
    static log_function original;
    if (!original)
    {
        void *symbol = dlsym(RTLD_NEXT, "arb_log_ui");
        _Static_assert(sizeof(original) == sizeof(symbol), "target function-pointer ABI");
        if (!symbol) abort();
        memcpy(&original, &symbol, sizeof(original));
    }
    if (watch_log_precision && first_log_precision < 0) first_log_precision = prec;
    original(y, n, prec);
}

/* The test owns no live objects at process exit. Release FLINT's process
   caches too, so the memory check measures unreleased objects, not caches. */
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

/* Byte snapshots, not only equal value sets. All permitted raw aliases and NULL
   reporting repeat every case, with identical OK results. */
static int call(acb_t y, const acb_t s, adf_place_t v, const acb_t a,
                const adf_char_t chi, slong prec)
{
    acb_t t, ss, aa, first;
    int first_st = -1;
    /* FLINT leaves inactive arf storage unspecified. Zero it before lifecycle
       initialization so raw-byte sentinels do not read undefined padding. */
    memset(t, 0, sizeof(t)); memset(ss, 0, sizeof(ss));
    memset(aa, 0, sizeof(aa)); memset(first, 0, sizeof(first));
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
        st = adf_local_tate_at(out, mode == 3 ? NULL : &w, ss, v, aa, chi, prec);
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

ADF_TEST(oracle_vectors)
{
    jsonl_file *file = NULL;
    jsonl_error_t err;
    acb_t s, a, y, ref;
    arb_t target, x;
    adf_char_t chi;
    size_t samples = 0, rows;
    initialized_acb(s); initialized_acb(a); initialized_acb(y); initialized_acb(ref);
    arb_init(target); arb_init(x); adf_char_init(chi);
    ADF_CHECK(jsonl_open("tests/ref/vectors/t-slice1/local.jsonl", &file, &err));
    if (!file) goto done;
    rows = jsonl_count(file);
    for (size_t i = 0; i < rows; i++)
    {
        const jsonl_value *r = jsonl_record(file, i);
        ulong q = number(field(r, "q")), n = number(field(r, "n"));
        int expected = (int) number(field(r, "status")), st;
        adf_place_t v = place(number(field(r, "p")));
        input(s, field(r, "s"), field(r, "sr"));
        input(a, field(r, "alpha"), field(r, "ar"));
        if (q <= ADF_CHAR_MOD_MAX)
        {
            ADF_CHECK(adf_char_set_conrey(chi, q, n) == ADF_OK);
            ADF_CHECK(adf_char_get_conductor(chi) == q);
        }
        else { chi->q = q; chi->n = n; }
        st = call(y, s, v, a, chi, 128);
        /* Restore before lifecycle INV; oversize descriptor is checked before INV. */
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
    ADF_CHECK(rows == 394); ADF_CHECK(samples == 1861);
    jsonl_close(file);
done:
    acb_clear(s); acb_clear(a); acb_clear(y); acb_clear(ref);
    arb_clear(target); arb_clear(x); adf_char_clear(chi);
}

ADF_TEST(delegation_examples_and_precision)
{
    acb_t s, a, y, ref, shifted;
    adf_char_t chi;
    initialized_acb(s); initialized_acb(a); initialized_acb(y); initialized_acb(ref); initialized_acb(shifted);
    adf_char_init(chi); acb_one(a);
    for (int parity = 0; parity < 2; parity++)
    {
        ADF_CHECK(adf_char_set_conrey(chi, parity ? 4 : 1, parity ? 3 : 1) == ADF_OK);
        for (int k = -4; k <= 4; k++)
        {
            adf_place_t v = place(0);
            acb_set_si_si(s, k, 1);
            mag_set_ui_2exp_si(arb_radref(acb_realref(s)), 1, -12);
            acb_add_ui(shifted, s, (ulong) parity, 4096);
            ADF_CHECK(call(y, s, v, a, chi, 128) ==
                      adf_local_zeta_factor_at(ref, NULL, shifted, v, 128));
            ADF_CHECK(acb_equal(y, ref));
        }
    }
    ADF_CHECK(adf_char_set_conrey(chi, 1, 1) == ADF_OK);
    acb_set_si(s, 2);
    ADF_CHECK(call(y, s, place(2), a, chi, 128) == ADF_OK);
    acb_zero(ref); arb_set_ui(acb_realref(ref), 4); arb_div_ui(acb_realref(ref), acb_realref(ref), 3, 512);
    ADF_CHECK(acb_contains(y, ref));
    ADF_CHECK(adf_local_zeta_factor_at(ref, NULL, s, place(2), 128) == ADF_OK);
    ADF_CHECK(acb_equal(y, ref));
    ADF_CHECK(call(y, s, place(0), a, chi, 128) == ADF_OK);
    acb_const_pi(ref, 512); acb_inv(ref, ref, 512); ADF_CHECK(acb_contains(y, ref));
    for (int p = 2; p <= 53; p += 51)
    {
        ADF_CHECK(call(y, s, place(2), a, chi, p) == ADF_OK);
        ADF_CHECK(adf_local_zeta_factor_at(ref, NULL, s, place(2), p) == ADF_OK);
        ADF_CHECK(acb_equal(y, ref));
    }
    ADF_CHECK(call(y, s, place(2), a, chi, 1) == ADF_OK);
    ADF_CHECK(call(ref, s, place(2), a, chi, -3) == ADF_OK); ADF_CHECK(acb_equal(y, ref));
    ADF_CHECK(adf_char_set_conrey(chi, 4, 3) == ADF_OK); acb_onei(a);
    ADF_CHECK(call(y, s, place(2), a, chi, 128) == ADF_OK); ADF_CHECK(acb_is_one(y));
    ADF_CHECK(call(y, s, place(2), a, chi, ADF_REAL_PREC_MAX) == ADF_OK);
    ADF_CHECK(acb_is_one(y));
    acb_zero(a); acb_zero(s);
    ADF_CHECK(call(y, s, place(2), a, chi, ADF_REAL_PREC_MAX+1) == ADF_LIMIT);
    acb_clear(s); acb_clear(a); acb_clear(y); acb_clear(ref); acb_clear(shifted);
    adf_char_clear(chi);
}

ADF_TEST(raw_domains_extremes_and_double_alias)
{
    acb_t s, a, y, ref;
    adf_char_t chi;
    initialized_acb(s); initialized_acb(a); initialized_acb(y); initialized_acb(ref); adf_char_init(chi);
    acb_set_si(s, 2); acb_one(a);
    arf_nan(arb_midref(acb_realref(s)));
    ADF_CHECK(call(y, s, place(2), a, chi, 128) == ADF_DOMAIN);
    acb_set_si(s, 2); arf_pos_inf(arb_midref(acb_imagref(a)));
    ADF_CHECK(call(y, s, place(2), a, chi, 128) == ADF_DOMAIN);
    ADF_CHECK(call(y, s, place(0), a, chi, 128) == ADF_OK); /* alpha ignored */
    acb_one(a); acb_set_si(s, 2); acb_set_si(a, 4);
    mag_set_ui_2exp_si(arb_radref(acb_realref(a)), 1, -100);
    ADF_CHECK(call(y, s, place(2), a, chi, 128) == ADF_NOT_DETERMINED);
    acb_set_si(a, 4); arb_add_ui(acb_realref(a), acb_realref(a), 1, 128);
    ADF_CHECK(call(y, s, place(2), a, chi, 128) == ADF_OK);
    ADF_CHECK(arb_contains_si(acb_realref(y), -4));
    acb_set_si(a, 4); acb_set_si(s, 2);
    arb_add_error_2exp_si(acb_realref(s), -1000);
    ADF_CHECK(call(y, s, place(2), a, chi, 128) == ADF_NOT_DETERMINED);
    /* Near an exact pole with a huge regular value and enough precision. */
    acb_one(a); acb_one(s); acb_mul_2exp_si(s, s, -500);
    ADF_CHECK(call(y, s, place(2), a, chi, 1024) == ADF_OK);
    ADF_CHECK(acb_is_finite(y));
    /* General alpha path: s=2, alpha=4+2^-498 gives exactly -2^500. */
    acb_set_si(s, 2); acb_one(ref); acb_mul_2exp_si(ref, ref, -498);
    acb_add_ui(a, ref, 4, 1024);
    ADF_CHECK(call(y, s, place(2), a, chi, 1024) == ADF_OK);
    acb_one(ref); acb_mul_2exp_si(ref, ref, 500); acb_neg(ref, ref);
    ADF_CHECK(acb_contains(y, ref));
    ADF_CHECK(arb_rel_accuracy_bits(acb_realref(y)) >= 480);
    /* Large raw operands, thousands of bits, do not cause integer power loops. */
    acb_one(a); acb_one(s); acb_mul_2exp_si(s, s, 1000);
    ADF_CHECK(call(y, s, place(2), a, chi, 128) == ADF_OK);
    acb_neg(s, s); ADF_CHECK(call(y, s, place(2), a, chi, 128) == ADF_OK);
    /* Outside the optional exact-power recognition bound is a regular point. */
    acb_set_si(a, 2); acb_set_si(s, 1048576);
    ADF_CHECK(call(y, s, place(2), a, chi, 128) == ADF_OK);
    ADF_CHECK(acb_is_finite(y));
    acb_neg(s, s);
    ADF_CHECK(call(y, s, place(2), a, chi, 128) == ADF_OK);
    acb_set_si_si(a, 2, 1); acb_set(s, a);
    ADF_CHECK(call(ref, s, place(2), a, chi, 128) == ADF_OK);
    ADF_CHECK(adf_local_tate_at(s, NULL, s, place(2), s, chi, 128) == ADF_OK);
    ADF_CHECK(acb_equal(s, ref));
    acb_clear(s); acb_clear(a); acb_clear(y); acb_clear(ref); adf_char_clear(chi);
}

ADF_TEST(odd_exact_large_poles)
{
    adf_char_t chi;
    acb_t s, a, y;
    adf_char_init(chi); initialized_acb(s); initialized_acb(a); initialized_acb(y);
    ADF_CHECK(adf_char_set_conrey(chi, 4, 3) == ADF_OK);
    acb_one(a); acb_one(s); acb_mul_2exp_si(s, s, 1000); acb_neg(s, s);
    acb_add_ui(s, s, 3, 4096);
    ADF_CHECK(call(y, s, place(0), a, chi, 53) == ADF_DOMAIN);
    acb_clear(s); acb_clear(a); acb_clear(y); adf_char_clear(chi);
}

ADF_TEST(closed_pole_boundaries_and_caps)
{
    acb_t s, a, y, ref;
    adf_char_t chi;
    adf_place_t w;
    unsigned char before[sizeof(acb_struct)];
    initialized_acb(s); initialized_acb(a); initialized_acb(y); initialized_acb(ref); adf_char_init(chi);
    acb_set_si(s, 1); arb_add_ui(acb_realref(s), acb_realref(s), 1, 128);
    acb_set_si(s, 1025); acb_mul_2exp_si(s, s, -10); acb_set_si(a, 3);
    mag_set_ui_2exp_si(arb_radref(acb_realref(s)), 1, -10);
    ADF_CHECK(call(y, s, place(3), a, chi, 128) == ADF_NOT_DETERMINED);
    mag_set_ui_2exp_si(arb_radref(acb_realref(s)), 1, -11);
    ADF_CHECK(call(y, s, place(3), a, chi, 128) == ADF_OK);
    ADF_CHECK(adf_char_set_conrey(chi, 4, 3) == ADF_OK);
    acb_zero(s);
    ADF_CHECK(call(y, s, place(0), a, chi, 128) == ADF_OK);
    ADF_CHECK(arb_contains_si(acb_realref(y), 1) && arb_is_zero(acb_imagref(y)));
    acb_set_si(s, -1023); acb_mul_2exp_si(s, s, -10);
    mag_set_ui_2exp_si(arb_radref(acb_realref(s)), 1, -10);
    ADF_CHECK(call(y, s, place(0), a, chi, 128) == ADF_NOT_DETERMINED);
    mag_set_ui_2exp_si(arb_radref(acb_realref(s)), 1, -11);
    ADF_CHECK(call(y, s, place(0), a, chi, 128) == ADF_OK);
    ADF_CHECK(adf_char_set_conrey(chi, 1, 1) == ADF_OK);
    acb_set_si(s, 1); acb_set_si(a, -1);
    first_log_precision = -1; watch_log_precision = 1;
    ADF_CHECK(adf_local_tate_at(y, NULL, s, place(2), a, chi, ADF_REAL_PREC_MAX) == ADF_OK);
    watch_log_precision = 0;
    ADF_CHECK(first_log_precision == ADF_REAL_PREC_MAX);
    acb_set_ui(ref, 2); acb_div_ui(ref, ref, 3, ADF_REAL_PREC_MAX+64);
    ADF_CHECK(acb_contains(y, ref));
    /* A bounded exact power at the recognizer's upper boundary is a pole. */
    acb_set_si(s, 524288); acb_one(a); acb_mul_2exp_si(a, a, 524288);
    ADF_CHECK(call(y, s, place(2), a, chi, 128) == ADF_DOMAIN);
    /* Cap refusal precedes even forged precondition inputs. */
    acb_clear(y); initialized_acb(y); /* discard inactive bytes from a prior successful swap */
    w = place(97); chi->parity = 9; acb_set_si_si(y, 17, 3);
    memcpy(before, y, sizeof(before));
    ADF_CHECK(adf_local_tate_at(y, &w, s, (adf_place_t){4}, a, chi,
                               ADF_REAL_PREC_MAX+1) == ADF_LIMIT);
    ADF_CHECK(w.opaque == 4); ADF_CHECK(memcmp(before, y, sizeof(before)) == 0);
    chi->parity = 0;
    acb_zero(s); w = place(97); memcpy(before, s, sizeof(before));
    ADF_CHECK(adf_local_tate_at(s, &w, s, place(2), s, chi, 128) == ADF_DOMAIN);
    ADF_CHECK(adf_place_equal(w, place(2))); ADF_CHECK(memcmp(before, s, sizeof(before)) == 0);
    acb_clear(s); acb_clear(a); acb_clear(y); acb_clear(ref); adf_char_clear(chi);
}

ADF_TEST(nonreal_inverse_vector)
{
    /* P9 step 1: conj(chi)*chi averages to 1; chi*chi averages to 0 for (5,2).
       The API's closed form must return exact 1 for this same non-real character. */
    adf_char_t chi, inverse;
    acb_t s, a, y, value, inv, term, correct, wrong;
    fmpz_t u;
    adf_char_init(chi); adf_char_init(inverse); fmpz_init(u);
    initialized_acb(s); initialized_acb(a); initialized_acb(y); initialized_acb(value); initialized_acb(inv);
    initialized_acb(term); initialized_acb(correct); initialized_acb(wrong);
    ADF_CHECK(adf_char_set_conrey(chi, 5, 2) == ADF_OK); adf_char_conj(inverse, chi);
    for (ulong j = 1; j < 5; j++)
    {
        fmpz_set_ui(u, j); ADF_CHECK(adf_char_chi(value, chi, u, 128) == ADF_OK);
        ADF_CHECK(adf_char_chi(inv, inverse, u, 128) == ADF_OK);
        acb_mul(term, value, inv, 128); acb_add(correct, correct, term, 128);
        acb_mul(term, value, value, 128); acb_add(wrong, wrong, term, 128);
    }
    acb_div_ui(correct, correct, 4, 128); acb_div_ui(wrong, wrong, 4, 128);
    ADF_CHECK(acb_is_one(correct)); ADF_CHECK(acb_is_zero(wrong));
    acb_set_si_si(s, -2, 1); acb_onei(a);
    ADF_CHECK(call(y, s, place(5), a, chi, 128) == ADF_OK); ADF_CHECK(acb_is_one(y));
    ADF_CHECK(!acb_overlaps(y, wrong));
    acb_set_si_si(chi->s, 17, -9); /* finite-character API ignores its exponent */
    ADF_CHECK(call(y, s, place(5), a, chi, 128) == ADF_OK); ADF_CHECK(acb_is_one(y));
    acb_clear(s); acb_clear(a); acb_clear(y); acb_clear(value); acb_clear(inv);
    acb_clear(term); acb_clear(correct); acb_clear(wrong); fmpz_clear(u);
    adf_char_clear(chi); adf_char_clear(inverse);
}

#ifdef ADF_CHECK_INVARIANTS
ADF_TEST(invariant_entry_aborts)
{
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
            setrlimit(RLIMIT_CORE, &nocore);
#ifdef __linux__
            prctl(PR_SET_DUMPABLE, 0);
#endif
            adf_place_t v = place(2);
            initialized_acb(s); initialized_acb(a); initialized_acb(y); adf_char_init(chi);
            acb_one(s); acb_one(a);
            if (mode == 0) v.opaque = 4;
            if (mode == 1) chi->parity = 1;
            adf_local_tate_at(mode == 2 ? chi->s : y, NULL, mode == 3 ? chi->s : s,
                              v, mode == 4 ? chi->s : a, chi, 128);
            /* Children also clear on a missing abort (tools/memcheck). */
            chi->parity = 0;
            acb_clear(s); acb_clear(a); acb_clear(y); adf_char_clear(chi);
            flint_cleanup(); _exit(42);
        }
        if (pid > 0)
        {
            ADF_CHECK(waitpid(pid, &st, 0) == pid);
            ADF_CHECK(WIFSIGNALED(st) && WTERMSIG(st) == SIGABRT);
        }
    }
}
#endif
