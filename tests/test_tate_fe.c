/* tests/test_tate_fe.c: slice 5e of docs/api-5.md, the caller-computed functional equation (design section 5,
   :250-260; statements in docs/api-5s.md "Slice 5e"). There is no library function under test that returns an
   equation: the side evaluation fe_sides below is the caller's computation of design section 5, the same steps as
   the driver command `tate_fe` (tools/adf/adf.c, adf_drv_tate_fe):

       A = adf_tate_completed(chi, s, bits),
       B = adf_tate_completed(conj(chi), 1 - s, bits + 4),
       W = adf_char_root_number(chi) at wp = max(prec, bits + m + 16), |B| < 2^m,
       R = W B at wp, its coordinate diameters checked <= 2^-bits.

   Analysis P13 (docs/proofs/analysis.md:564-574, step 4 :613-614) states Lambda(s, chi) = W_chi Lambda(1 - s,
   conj(chi)) with W_chi = tau(chi)/(i^e sqrt(C)), the positive finite Gauss sum (:569, :578). So A and R are two
   enclosures of the same number computed by separate calls; the test requires both finite, narrow, overlapping each
   other AND each containing two independent references: python-flint's L completed by hand and mpmath's Hurwitz L
   (tests/ref/vectors/t5-slice5/fe.jsonl, written by lanes/t5-slice5/gen_vectors.py; FLINT's normalisation
   refs/src/flint-3.0.1/acb_dirichlet.rst:98-100, :524-534, :567-569; at a trivial zero by the quotient of the two
   derivatives, not by the functional equation). "overlap alone can hold for two equal wrong values"
   (api-5.md:256-257). W is compared with tests/golden/gauss.tsv. No FLINT L function is called in this binary.
   The independence test of design section 7 (:350-352): one side's evaluator replaced by a disjoint sentinel
   changes that side alone. The six faults of design 7 (5.4) are planted by lanes/t5-slice5/faults.py in a scratch
   copy of fe_sides and must make this test fail. */
#include <adelefeld.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "support/jsonl.h"
#include "support/golden.h"

#define VEC "tests/ref/vectors/t5-slice5/fe.jsonl"
#define RP 512                                            /* precision of the reference balls */

static unsigned long checks;
static const char *only;                                  /* run one section (argv[1]) */

#define CHECK(c) do { checks++; if (!(c)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #c); exit(1); } } while (0)

static double seconds(void) { return (double) clock() / (double) CLOCKS_PER_SEC; }
static int table;                                         /* argv[1] = "table": the values, one line per case */
static int section(const char *name)
{
    return only == NULL || !strcmp(only, name) || (table && !strcmp(name, "values"));
}

/* ------------------------------------------------------------------ the side evaluation (design section 5) */
#define FE_SENT_A 1                                       /* replace A's evaluator by the sentinel 10^6 */
#define FE_SENT_B 2                                       /* replace B's evaluator by the sentinel -10^6 i */

/* The caller's computation of design section 5 (api-5.md:251-255). On OK: A = Lambda(s, chi) with each coordinate
   diameter <= 2^-bits (tate.h, adf_tate_completed), B = Lambda(1 - s, conj(chi)) with diameters <= 2^-(bits + 4),
   W the root number, R = W B with diameters checked <= 2^-bits. On failure *which names the first failing step
   ('A', 'B', 'W', or 'R' for the width check of R) and the status is returned; the steps run in this order and the
   first failure stops. The two sides share no evaluator: A is one call; R is built from a second call on another
   character and point, and from the root number. 1 - s is formed at prec (exact for the dyadic points used). */
static int fe_sides(acb_t A, acb_t R, acb_t B, acb_t W, char *which, const adf_char_t chi, const acb_t s,
                    slong bits, slong prec, int sentinel)
{
    adf_char_t cc; acb_t t; mag_t mb; arf_t u; slong m, wp; int st;
    adf_char_init(cc); acb_init(t); mag_init(mb); arf_init(u);
    *which = 'A';
    if (sentinel & FE_SENT_A) { acb_set_si(A, 1000000); st = ADF_OK; }
    else st = adf_tate_completed(A, chi, s, bits, prec);
    if (st != ADF_OK) goto done;
    *which = 'B';
    adf_char_conj(cc, chi);
    acb_neg(t, s); acb_add_ui(t, t, 1, prec);
    if (sentinel & FE_SENT_B) { acb_set_si_si(B, 0, -1000000); st = ADF_OK; }
    else st = adf_tate_completed(B, cc, t, bits + 4, prec);
    if (st != ADF_OK) goto done;
    *which = 'W';
    acb_get_mag(mb, B); arf_set_mag(u, mb);                /* |B| <= mb < 2^m, acb.rst:290-292, arf.rst:393-398 */
    m = FLINT_MAX(0, arf_abs_bound_lt_2exp_si(u));
    wp = FLINT_MAX(prec, bits + m + 16);
    st = adf_char_root_number(W, chi, wp);
    if (st != ADF_OK) goto done;
    *which = 'R';
    acb_mul(R, W, B, wp);
    if (!acb_is_finite(R) || mag_cmp_2exp_si(arb_radref(acb_realref(R)), -bits - 1) > 0
        || mag_cmp_2exp_si(arb_radref(acb_imagref(R)), -bits - 1) > 0) st = ADF_NOT_DETERMINED;
done:
    adf_char_clear(cc); acb_clear(t); mag_clear(mb); arf_clear(u);
    return st;
}

/* ------------------------------------------------------------------ JSON helpers */
static const jsonl_value *field(const jsonl_value *v, const char *name)
{
    const jsonl_value *out; jsonl_error_t e;
    CHECK(jsonl_field(v, name, &out, &e)); return out;
}
static const jsonl_value *at(const jsonl_value *v, size_t i)
{
    jsonl_error_t e; const jsonl_value *out = jsonl_at(v, i, &e);
    CHECK(out != NULL); return out;
}
static const char *str(const jsonl_value *v)
{
    jsonl_error_t e; size_t n; const char *s = jsonl_string(v, &n, &e);
    CHECK(s != NULL); return s;
}
static long number(const jsonl_value *v)
{
    jsonl_error_t e; const char *s = jsonl_int_text(v, &e);
    CHECK(s != NULL); return strtol(s, NULL, 10);
}
static void q_of(fmpq_t q, const char *s) { CHECK(fmpq_set_str(q, s, 10) == 0); fmpq_canonicalise(q); }
static void ball_of(acb_t z, const jsonl_value *v)
{
    CHECK(arb_set_str(acb_realref(z), str(at(v, 0)), RP) == 0);
    CHECK(arb_set_str(acb_imagref(z), str(at(v, 1)), RP) == 0);
}
/* The point [re, im] (exact rationals, rounded at 4 RP bits: exact for the dyadic points of the vectors) plus the
   exact dyadic radii rad (NULL: exact). */
static void s_of(acb_t s, const jsonl_value *pt, const jsonl_value *rad)
{
    fmpq_t q; arb_t r; int k;
    fmpq_init(q); arb_init(r);
    for (k = 0; k < 2; k++) {
        arb_ptr c = k ? acb_imagref(s) : acb_realref(s);
        q_of(q, str(at(pt, (size_t) k))); arb_set_fmpq(c, q, 4 * RP);
        if (rad != NULL) {
            q_of(q, str(at(rad, (size_t) k))); arb_set_fmpq(r, q, 4 * RP);
            CHECK(arb_is_exact(r));
            arb_add_error(c, r);
        }
    }
    fmpq_clear(q); arb_clear(r);
}
/* The raw pair of the record, lowered by the constructor; the lowered (C, label, e) must be the record's. */
static void char_of(adf_char_t chi, const jsonl_value *rec)
{
    CHECK(adf_char_set_conrey(chi, (ulong) number(field(rec, "q")), (ulong) number(field(rec, "n"))) == ADF_OK);
    CHECK(adf_char_get_conductor(chi) == (ulong) number(field(rec, "C")));
    CHECK(adf_char_get_parity(chi) == (int) number(field(rec, "e")));
    if (number(field(rec, "C")) > 1) CHECK(adf_char_get_label(chi) == (ulong) number(field(rec, "label")));
}

/* ------------------------------------------------------------------ predicates and sentinels */
static int narrow(const acb_t z, slong bits)
{
    return acb_is_finite(z) && mag_cmp_2exp_si(arb_radref(acb_realref(z)), -bits - 1) <= 0
        && mag_cmp_2exp_si(arb_radref(acb_imagref(z)), -bits - 1) <= 0;
}
static void maxrad(mag_t m, const acb_t z) { mag_max(m, arb_radref(acb_realref(z)), arb_radref(acb_imagref(z))); }

typedef struct { acb_t z, copy; unsigned char bytes[sizeof(acb_struct)]; } sentinel;
static void sent_init(sentinel *s)
{
    memset(s, 0, sizeof *s);                              /* defined padding for the byte comparison */
    acb_init(s->z); acb_init(s->copy);
    acb_set_si_si(s->z, 12345, -678); mag_set_ui_2exp_si(arb_radref(acb_realref(s->z)), 3, -7);
    acb_set(s->copy, s->z); memcpy(s->bytes, s->z, sizeof(acb_struct));
}
static int sent_same(const sentinel *s)
{
    return memcmp(s->bytes, s->z, sizeof(acb_struct)) == 0 && acb_equal(s->z, s->copy);
}
static void sent_clear(sentinel *s) { acb_clear(s->z); acb_clear(s->copy); }

/* The golden root number of the raw pair (tests/golden/gauss.tsv), or 0 if the pair is not a golden row. */
static int golden_W(acb_t W, ulong q, ulong n)
{
    golden_file *f; golden_error_t e; adf_char_t x; adf_cadele_t parsed; int found = 0; char text[600];
    adf_char_init(x); adf_cadele_init(parsed);
    CHECK(golden_open("tests/golden/gauss.tsv", GOLDEN_DEFAULT_MAX_INPUT, &f, &e));
    for (size_t i = 0; i < golden_count(f) && !found; i++) {
        const golden_record *r = golden_record_at(f, i);
        char head[64];
        snprintf(head, sizeof head, "char(q=%lu, n=%lu,", q, n);
        if (strncmp(r->input, head, strlen(head))) continue;
        const char *w = strstr(r->expected, " W=");
        CHECK(w != NULL);
        snprintf(text, sizeof(text), "(%s ; 0)", w + 3);
        CHECK(adf_cadele_set_str(parsed, text, strlen(text), 256, NULL) == ADF_OK);
        acb_set(W, parsed->inf); found = 1;
    }
    golden_close(f); adf_char_clear(x); adf_cadele_clear(parsed);
    return found;
}

/* ------------------------------------------------------------------ the 17 goldens: W, the lowering of (8,7) */
static void goldens(void)
{
    golden_file *f; golden_error_t e; adf_char_t x; acb_t W, Wg; size_t i;
    adf_char_init(x); acb_init(W); acb_init(Wg);
    CHECK(golden_open("tests/golden/gauss.tsv", GOLDEN_DEFAULT_MAX_INPUT, &f, &e));
    CHECK(golden_count(f) == 17);
    for (i = 0; i < golden_count(f); i++) {
        const golden_record *r = golden_record_at(f, i);
        ulong q, n;
        CHECK(sscanf(r->input, "char(q=%lu, n=%lu,", &q, &n) == 2);
        CHECK(adf_char_set_str(x, r->input, r->input_len, 128, NULL) == ADF_OK);
        CHECK(golden_W(Wg, q, n));
        CHECK(adf_char_root_number(W, x, 120) == ADF_OK && acb_overlaps(W, Wg));
        if (q == 8 && n == 7)                             /* the raw (8,7) lowers to (4,3): conductor 4 */
            CHECK(adf_char_get_conductor(x) == 4 && adf_char_get_label(x) == 3);
    }
    golden_close(f); adf_char_clear(x); acb_clear(W); acb_clear(Wg);
    printf("goldens: 17 root numbers overlap the golden W; (8,7) lowers to (4,3)\n");
}

/* ------------------------------------------------------------------ the values against the vectors */
static unsigned long fe_ok, fe_want, refs_in, zeros_ok, zeros_want;
static double worst_a = 0, worst_r = 0;                   /* the largest diameter / 2^-bits seen for A and R */

static double ratio(const acb_t z, slong bits)
{
    mag_t m; double d;
    mag_init(m); maxrad(m, z); mag_mul_2exp_si(m, m, bits + 1); d = mag_get_d(m); mag_clear(m);
    return d;
}

/* One acceptance case: both sides OK, finite, A narrow at bits, R narrow at bits + 2 (the guard targets make
   R's diameter about sqrt(2) 2^-(bits+4), docs/api-5s.md S16), overlapping each other, each containing both
   references of Lambda(s, chi); B containing both references of Lambda(1 - s, conj(chi)); W overlapping the
   golden W. */
static void fe_case(const adf_char_t chi, const acb_t s, slong bits, const jsonl_value *rec, const acb_t Wg)
{
    acb_t A, R, B, W, ref; char which; int st; double ra, rr;
    acb_init(A); acb_init(R); acb_init(B); acb_init(W); acb_init(ref);
    st = fe_sides(A, R, B, W, &which, chi, s, bits, bits + 32, 0);
    if (st != ADF_OK)
        fprintf(stderr, "fe: q=%ld n=%ld s=%s+%si bits=%ld: %c status %d\n", number(field(rec, "q")),
                number(field(rec, "n")), str(at(field(rec, "s"), 0)), str(at(field(rec, "s"), 1)), (long) bits,
                which, st);
    CHECK(st == ADF_OK);
    CHECK(narrow(A, bits));
    CHECK(narrow(R, bits + 2));
    CHECK(narrow(B, bits + 4));
    CHECK(acb_overlaps(A, R));
    ball_of(ref, field(rec, "Lf")); CHECK(acb_contains(A, ref)); CHECK(acb_contains(R, ref));
    ball_of(ref, field(rec, "L")); CHECK(acb_contains(A, ref)); CHECK(acb_contains(R, ref));
    ball_of(ref, field(rec, "Bf")); CHECK(acb_contains(B, ref));
    ball_of(ref, field(rec, "B")); CHECK(acb_contains(B, ref));
    CHECK(acb_overlaps(W, Wg));
    refs_in += 6;
    ra = ratio(A, bits); rr = ratio(R, bits);
    if (ra > worst_a) worst_a = ra;
    if (rr > worst_r) worst_r = rr;
    if (number(field(rec, "zero")) == 1) CHECK(!acb_contains_zero(A) && !acb_contains_zero(R));
    if (table && bits == 53)
        printf("| (%ld, %ld) | %s + %s i | %.2e | %.2e | yes | yes |\n", number(field(rec, "q")),
               number(field(rec, "n")), str(at(field(rec, "s"), 0)), str(at(field(rec, "s"), 1)),
               2 * ra * ldexp(1.0, -(int) bits - 1), 2 * rr * ldexp(1.0, -(int) bits - 1));
    fe_ok++;
    acb_clear(A); acb_clear(R); acb_clear(B); acb_clear(W); acb_clear(ref);
}
static void value_record(const jsonl_value *rec)
{
    adf_char_t chi; acb_t s, Wg; long q = number(field(rec, "q")), C = number(field(rec, "C")); int k;
    static const slong more[2] = { 20, 80 };
    adf_char_init(chi); acb_init(s); acb_init(Wg);
    char_of(chi, rec); s_of(s, field(rec, "s"), NULL);
    CHECK(acb_is_exact(s));
    CHECK(golden_W(Wg, (ulong) q, (ulong) number(field(rec, "n"))));
    fe_case(chi, s, 53, rec, Wg); fe_want++;
    if (C == 1 || C == 4 || C == 5 || C == 15 || C == 16)          /* the oracle's five pairs among them */
        for (k = 0; k < 2; k++) { fe_case(chi, s, more[k], rec, Wg); fe_want++; }
    if (number(field(rec, "zero"))) { zeros_want++; zeros_ok++; }
    adf_char_clear(chi); acb_clear(s); acb_clear(Wg);
}

/* The balls of radius 2^-60 at 3/8 + 3i/4: at bits 40 both sides OK and narrow, overlapping, each containing the
   references at the four corners (every point of S: R encloses W Lambda(1 - x, conj chi) = Lambda(x, chi) for x
   in S, since the computed 1 - S contains every 1 - x). At bits 80 the radius holds the width of A (N-D23):
   NOT_DETERMINED from A, the outputs untouched. */
static unsigned long balls_done;
static void ball_record(const jsonl_value *rec)
{
    adf_char_t chi; acb_t s, A, R, B, W, ref; const jsonl_value *cs = field(rec, "corners"); size_t k;
    char which; sentinel a, r, b, w;
    adf_char_init(chi); acb_init(s); acb_init(A); acb_init(R); acb_init(B); acb_init(W); acb_init(ref);
    char_of(chi, rec); s_of(s, field(rec, "s"), field(rec, "rad"));
    CHECK(jsonl_size(cs) == 4);
    CHECK(fe_sides(A, R, B, W, &which, chi, s, 40, 72, 0) == ADF_OK);
    CHECK(narrow(A, 40) && narrow(R, 40) && acb_overlaps(A, R));
    for (k = 0; k < 4; k++) {
        ball_of(ref, field(at(cs, k), "L")); CHECK(acb_contains(A, ref) && acb_contains(R, ref));
        ball_of(ref, field(at(cs, k), "Lf")); CHECK(acb_contains(A, ref) && acb_contains(R, ref));
    }
    sent_init(&a); sent_init(&r); sent_init(&b); sent_init(&w);
    CHECK(fe_sides(a.z, r.z, b.z, w.z, &which, chi, s, 80, 112, 0) == ADF_NOT_DETERMINED && which == 'A');
    CHECK(sent_same(&a) && sent_same(&r) && sent_same(&b) && sent_same(&w));
    sent_clear(&a); sent_clear(&r); sent_clear(&b); sent_clear(&w);
    balls_done++;
    adf_char_clear(chi); acb_clear(s); acb_clear(A); acb_clear(R); acb_clear(B); acb_clear(W); acb_clear(ref);
}

static void vectors(void)
{
    jsonl_file *file; jsonl_error_t err; size_t i, n, nval = 0;
    double t0 = seconds();
    CHECK(jsonl_open(VEC, &file, &err));
    n = jsonl_count(file);
    for (i = 0; i < n; i++) {
        const jsonl_value *rec = jsonl_record(file, i); const char *kind = str(field(rec, "kind"));
        if (!strcmp(kind, "value")) { if (section("values")) value_record(rec); nval++; }
        else if (!strcmp(kind, "ball")) { if (section("balls")) ball_record(rec); }
        else CHECK(0);
    }
    jsonl_close(file);
    /* 136 records (17 characters, 8 points); bits 53, and 20, 80 for C = 1, 4, 5, 15, 16 (the raw (8,7) is C = 4,
       the raw (10,3) is C = 5): 9 rows of 8 records, 72 records with 2 more cases: 280 cases; 46 records at a
       trivial zero of a side */
    CHECK(nval == 136);
    if (section("values")) {
        CHECK(fe_ok == fe_want && fe_want == 280 && zeros_ok == zeros_want && zeros_want == 46);
        printf("values: %lu functional-equation cases OK, %lu references inside, 46 at a trivial zero; largest "
               "diameter/2^-bits: A %.3g, R %.3g (%.1f s)\n", fe_ok, refs_in, worst_a, worst_r, seconds() - t0);
    }
    if (section("balls")) { CHECK(balls_done == 17); printf("balls: 17 radius 2^-60, every corner inside\n"); }
}

/* ------------------------------------------------------------------ poles, statuses, hand values */
/* Each side's status at a point, the side call made directly (fe_sides stops at the first failure). */
static void side_status(int *sa, int *sb, const adf_char_t chi, const acb_t s, slong bits)
{
    adf_char_t cc; acb_t t; sentinel a, b;
    adf_char_init(cc); acb_init(t); sent_init(&a); sent_init(&b);
    adf_char_conj(cc, chi); acb_neg(t, s); acb_add_ui(t, t, 1, 128);
    *sa = adf_tate_completed(a.z, chi, s, bits, 128);
    *sb = adf_tate_completed(b.z, cc, t, bits + 4, 128);
    if (*sa != ADF_OK) CHECK(sent_same(&a));
    if (*sb != ADF_OK) CHECK(sent_same(&b));
    adf_char_clear(cc); acb_clear(t); sent_clear(&a); sent_clear(&b);
}
static void dyadic_ball(acb_t s, slong re_num, slong re_exp, slong rad_exp)
{
    acb_zero(s); arb_set_si(acb_realref(s), re_num); arb_mul_2exp_si(acb_realref(s), acb_realref(s), re_exp);
    if (rad_exp != WORD_MIN) mag_set_ui_2exp_si(arb_radref(acb_realref(s)), 1, rad_exp);
}
static void poles(void)
{
    adf_char_t one, chi4; acb_t s, A, R, B, W; int sa, sb; char which; sentinel a, r, b, w;
    adf_char_init(one); adf_char_init(chi4); acb_init(s); acb_init(A); acb_init(R); acb_init(B); acb_init(W);
    CHECK(adf_char_set_conrey(chi4, 4, 3) == ADF_OK);
    /* C = 1 at exact 0 and 1: A is DOMAIN, and B (at 1 - s = 1 and 0) is DOMAIN too: both are statuses */
    acb_zero(s); side_status(&sa, &sb, one, s, 53); CHECK(sa == ADF_DOMAIN && sb == ADF_DOMAIN);
    acb_one(s); side_status(&sa, &sb, one, s, 53); CHECK(sa == ADF_DOMAIN && sb == ADF_DOMAIN);
    sent_init(&a); sent_init(&r); sent_init(&b); sent_init(&w);
    CHECK(fe_sides(a.z, r.z, b.z, w.z, &which, one, s, 53, 85, 0) == ADF_DOMAIN && which == 'A');
    CHECK(sent_same(&a) && sent_same(&r) && sent_same(&b) && sent_same(&w));
    /* a ball meeting a pole: 1 +/- 2^-10 (B at 0 -/+ 2^-10), the contact [1 - 2^-9, 1] (B at [0, 2^-9]), and
       1/2 +/- 3/4 (both poles): NOT_DETERMINED on both sides, also at bits 2^21 (the geometry before the work) */
    dyadic_ball(s, 1, 0, -10); side_status(&sa, &sb, one, s, 53);
    CHECK(sa == ADF_NOT_DETERMINED && sb == ADF_NOT_DETERMINED);
    CHECK(fe_sides(a.z, r.z, b.z, w.z, &which, one, s, 53, 85, 0) == ADF_NOT_DETERMINED && which == 'A');
    dyadic_ball(s, 1023, -10, -10); side_status(&sa, &sb, one, s, 53);
    CHECK(sa == ADF_NOT_DETERMINED && sb == ADF_NOT_DETERMINED);
    dyadic_ball(s, 1, -1, WORD_MIN); arb_add_error_2exp_si(acb_realref(s), -1);
    arb_add_error_2exp_si(acb_realref(s), -2); side_status(&sa, &sb, one, s, 2097148);
    CHECK(sa == ADF_NOT_DETERMINED && sb == ADF_NOT_DETERMINED);
    CHECK(sent_same(&a) && sent_same(&r) && sent_same(&b) && sent_same(&w));
    sent_clear(&a); sent_clear(&r); sent_clear(&b); sent_clear(&w);
    /* C > 1 has no pole: chi_4 at s = 1 and s = 0, Lambda(1, chi_4) = (4/pi) Gamma(1) L(1, chi_4) = 1 with
       L(1, chi_4) = pi/4 (api-5.md:338), and Lambda(0, chi_4) = W Lambda(1, chi_4) = 1 (W = 1, the golden row) */
    acb_one(s);
    CHECK(fe_sides(A, R, B, W, &which, chi4, s, 53, 85, 0) == ADF_OK && acb_overlaps(A, R));
    CHECK(arb_contains_si(acb_realref(A), 1) && arb_contains_si(acb_realref(R), 1));
    acb_zero(s);
    CHECK(fe_sides(A, R, B, W, &which, chi4, s, 53, 85, 0) == ADF_OK && acb_overlaps(A, R));
    CHECK(arb_contains_si(acb_realref(A), 1) && arb_contains_si(acb_realref(R), 1));
    adf_char_clear(one); adf_char_clear(chi4); acb_clear(s); acb_clear(A); acb_clear(R); acb_clear(B);
    acb_clear(W);
    printf("poles: C = 1 at 0, 1 DOMAIN on both sides; balls meeting a pole NOT_DETERMINED on both; C = 4 at 0, 1 "
           "OK\n");
}
static void hand(void)
{
    adf_char_t one; acb_t s, A, R, B, W, pi6; char which;
    adf_char_init(one); acb_init(s); acb_init(A); acb_init(R); acb_init(B); acb_init(W); acb_init(pi6);
    /* zeta: Lambda(-1) = Lambda(2) = pi^-1 Gamma(1) zeta(2) = pi/6 (api-5.md:338): both sides at s = -1 and 2 */
    arb_const_pi(acb_realref(pi6), RP); acb_div_ui(pi6, pi6, 6, RP);
    acb_set_si(s, -1);
    CHECK(fe_sides(A, R, B, W, &which, one, s, 80, 112, 0) == ADF_OK);
    CHECK(acb_contains(A, pi6) && acb_contains(R, pi6));
    acb_set_si(s, 2);
    CHECK(fe_sides(A, R, B, W, &which, one, s, 80, 112, 0) == ADF_OK);
    CHECK(acb_contains(A, pi6) && acb_contains(R, pi6));
    /* zeta at 1/2: W = 1 exactly and conj(chi) = chi, so R = B: the same function at the same point, computed at
       bits + 4, a second call (not a copy of A) */
    acb_one(s); acb_mul_2exp_si(s, s, -1);
    CHECK(fe_sides(A, R, B, W, &which, one, s, 80, 160, 0) == ADF_OK && acb_overlaps(A, R));
    CHECK(acb_is_one(W) && acb_contains(R, B) && !acb_equal(A, R));
    adf_char_clear(one); acb_clear(s); acb_clear(A); acb_clear(R); acb_clear(B); acb_clear(W); acb_clear(pi6);
    printf("hand: Lambda_zeta(-1) = Lambda_zeta(2) = pi/6 on both sides; at 1/2 R = 1 * B, B not A\n");
}
static void statuses(void)
{
    adf_char_t one, big; acb_t s; char which; sentinel a, r, b, w;
    adf_char_init(one); adf_char_init(big); acb_init(s);
    sent_init(&a); sent_init(&r); sent_init(&b); sent_init(&w);
    acb_set_si(s, -1);
    /* bits outside [0, 2^21]: A is DOMAIN, B is not reached (bits + 4 is formed only after A is OK) */
    CHECK(fe_sides(a.z, r.z, b.z, w.z, &which, one, s, -1, 64, 0) == ADF_DOMAIN && which == 'A');
    CHECK(fe_sides(a.z, r.z, b.z, w.z, &which, one, s, WORD_MAX, 64, 0) == ADF_DOMAIN && which == 'A');
    CHECK(fe_sides(a.z, r.z, b.z, w.z, &which, one, s, 2097153, 64, 0) == ADF_DOMAIN && which == 'A');
    /* precision above the cap and C above 1024: LIMIT from A */
    CHECK(fe_sides(a.z, r.z, b.z, w.z, &which, one, s, 20, ADF_REAL_PREC_MAX + 1, 0) == ADF_LIMIT && which == 'A');
    CHECK(adf_char_set_conrey(big, 1031, 2) == ADF_OK);
    CHECK(fe_sides(a.z, r.z, b.z, w.z, &which, big, s, 20, 64, 0) == ADF_LIMIT && which == 'A');
    CHECK(sent_same(&a) && sent_same(&r) && sent_same(&b) && sent_same(&w));
    sent_clear(&a); sent_clear(&r); sent_clear(&b); sent_clear(&w);
    adf_char_clear(one); adf_char_clear(big); acb_clear(s);
    printf("statuses: bits -1, 2^21 + 1, WORD_MAX DOMAIN at A; prec cap and C = 1031 LIMIT at A\n");
}

/* ------------------------------------------------------------------ the guard target of B alone */
/* B has the target bits + 4: a ball whose image width lies between 2^-(bits+4) and 2^-bits passes at A and fails
   at B (the width floor of N-D23). zeta on 3/8 +/- 2^-k, bits 20, k = 10..40: at least one k gives A OK and the
   failure named B (A was not reported); whenever B fails, the direct call of A is OK and B's direct call fails. */
static void guard(void)
{
    adf_char_t one; acb_t s, A, R, B, W; char which; int k, st, hits = 0;
    adf_char_init(one); acb_init(s); acb_init(A); acb_init(R); acb_init(B); acb_init(W);
    for (k = 10; k <= 40; k++) {
        acb_set_si(s, 3); acb_mul_2exp_si(s, s, -3); mag_set_ui_2exp_si(arb_radref(acb_realref(s)), 1, -k);
        st = fe_sides(A, R, B, W, &which, one, s, 20, 64, 0);
        if (st != ADF_OK && which == 'B') {
            int sa, sb;
            CHECK(st == ADF_NOT_DETERMINED);
            side_status(&sa, &sb, one, s, 20);
            CHECK(sa == ADF_OK && sb == ADF_NOT_DETERMINED);
            hits++;
        } else CHECK(st == ADF_OK || which == 'A');
    }
    CHECK(hits >= 1);
    adf_char_clear(one); acb_clear(s); acb_clear(A); acb_clear(R); acb_clear(B); acb_clear(W);
    printf("guard: %d radii with A OK and the failure named B (bits 20, B at 24)\n", hits);
}

/* ------------------------------------------------------------------ independence (design 7, 5.4) */
/* A disjoint sentinel in place of one side's evaluator changes that side alone: with A's evaluator replaced, R (and
   B, W) are bit for bit those of the real run; with B's evaluator replaced, A is bit for bit the real one and R
   moves to W times the sentinel. Equal fake sides are not a certificate (api-5.md:350-352). */
static void independence(void)
{
    static const ulong pairs[4][2] = { { 1, 1 }, { 4, 3 }, { 5, 2 }, { 13, 2 } };
    adf_char_t chi; acb_t s, A0, R0, B0, W0, A, R, B, W, y; char which; int k;
    adf_char_init(chi); acb_init(s); acb_init(A0); acb_init(R0); acb_init(B0); acb_init(W0); acb_init(A);
    acb_init(R); acb_init(B); acb_init(W); acb_init(y);
    acb_set_si_si(s, 3, 6); acb_mul_2exp_si(s, s, -3);                 /* 3/8 + 3i/4 */
    for (k = 0; k < 4; k++) {
        CHECK(adf_char_set_conrey(chi, pairs[k][0], pairs[k][1]) == ADF_OK);
        CHECK(fe_sides(A0, R0, B0, W0, &which, chi, s, 53, 85, 0) == ADF_OK);
        CHECK(fe_sides(A, R, B, W, &which, chi, s, 53, 85, FE_SENT_A) == ADF_OK);
        CHECK(acb_equal(R, R0) && acb_equal(B, B0) && acb_equal(W, W0));
        CHECK(!acb_overlaps(A, A0) && acb_equal_si(A, 1000000));
        CHECK(fe_sides(A, R, B, W, &which, chi, s, 53, 85, FE_SENT_B) == ADF_OK);
        CHECK(acb_equal(A, A0) && !acb_overlaps(R, R0));
        acb_set_si_si(y, 0, -1000000); acb_mul(y, y, W, RP); CHECK(acb_overlaps(R, y));
    }
    adf_char_clear(chi); acb_clear(s); acb_clear(A0); acb_clear(R0); acb_clear(B0); acb_clear(W0); acb_clear(A);
    acb_clear(R); acb_clear(B); acb_clear(W); acb_clear(y);
    printf("independence: 4 characters, each sentinel moves its own side only\n");
}

/* ------------------------------------------------------------------ the width shrinks with bits */
static void widths(void)
{
    static const ulong pairs[3][2] = { { 1, 1 }, { 5, 2 }, { 16, 3 } };
    static const slong bits[3] = { 20, 53, 80 };
    adf_char_t chi; acb_t s, A, R, B, W; mag_t ma, mr, pa, pr; char which; int k, j;
    adf_char_init(chi); acb_init(s); acb_init(A); acb_init(R); acb_init(B); acb_init(W);
    mag_init(ma); mag_init(mr); mag_init(pa); mag_init(pr);
    acb_set_si_si(s, 1, 4);                                            /* 1/4 + i */
    arb_mul_2exp_si(acb_realref(s), acb_realref(s), -2);
    for (k = 0; k < 3; k++) {
        CHECK(adf_char_set_conrey(chi, pairs[k][0], pairs[k][1]) == ADF_OK);
        for (j = 0; j < 3; j++) {
            CHECK(fe_sides(A, R, B, W, &which, chi, s, bits[j], 2, 0) == ADF_OK);   /* from prec 2 */
            CHECK(narrow(A, bits[j]) && narrow(R, bits[j]) && acb_overlaps(A, R));
            maxrad(ma, A); maxrad(mr, R);
            if (j) CHECK(mag_cmp(ma, pa) < 0 && mag_cmp(mr, pr) < 0);
            mag_set(pa, ma); mag_set(pr, mr);
        }
    }
    adf_char_clear(chi); acb_clear(s); acb_clear(A); acb_clear(R); acb_clear(B); acb_clear(W);
    mag_clear(ma); mag_clear(mr); mag_clear(pa); mag_clear(pr);
    printf("widths: bits 20, 53, 80 from prec 2, both sides narrow and shrinking (3 characters)\n");
}

int main(int argc, char **argv)
{
    only = argc > 1 ? argv[1] : NULL;
    table = only != NULL && !strcmp(only, "table");
    if (section("goldens")) goldens();
    if (section("statuses")) statuses();
    if (section("poles")) poles();
    if (section("hand")) hand();
    if (section("guard")) guard();
    if (section("independence")) independence();
    if (section("widths")) widths();
    vectors();
    printf("tate_fe: %lu checks\n", checks);
    flint_cleanup();
    return 0;
}
