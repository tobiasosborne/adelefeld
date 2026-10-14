/* tests/test_tate_continue.c: slice 5d of docs/api-5.md, the all-plane continuation adf_tate_continue and the
   completed function adf_tate_completed (include/adelefeld/tate.h; statements in docs/api-5b.md "Slice 5d").
   Contract: docs/api-5.md section 1 (statuses, order of checks, D2), section 4 (:167-249: P12/P13's split on the
   whole plane, the pole geometry, T4 near a pole, the two comment blocks), section 6 (T4), section 7 (acceptance,
   the link-wrap, the independence test); docs/SPEC.md 8 (:470-502: FLINT's values completed by hand; the width
   shrinks with precision; points at and near the poles; (s - 1) Lambda -> 1 and s Lambda -> -1).
   Oracle: tests/ref/vectors/t5-slice4/continue.jsonl, written by lanes/t5-slice4/gen_vectors.py from
   proto/tate_checks.py: I_chi(s) and Lambda_chi(s) at 60 digits (mpmath) and from python-flint (a second source;
   at the trivial zeros by the functional equation), for C = 1 and the 17 characters of tests/golden/gauss.tsv; the
   input balls with corner references; the oracle's pole_status; the residue products; the oracle's cutoffs; the
   two Mellin sides separately (mpmath's incomplete Gamma).
   What OK must give: every reference inside the result and each coordinate diameter <= 2^-bits. Failure: z
   untouched (bytes and value).

   The forbidden path (api-5.md:242, :353-354): the continuation must not reach FLINT's L, zeta, xi, Hurwitz,
   theta or AFE functions. This binary defines those symbols itself, so that every call from the library (linked
   statically into this executable) binds to a definition here that aborts at once (`forbidden` checks that the
   interposition is active). Built with -DADF_TATE_LINK_WRAP and -Wl,--wrap=<name> for each name below, the same
   aborting bodies are the __wrap_ definitions instead (the form of api-5.md:353; the Makefile has no per-test
   linker flags, so `make check` uses the interposition). No FLINT L value is computed in this binary: FLINT's
   values come from the vector file. The finite character pairing and adf_char_root_number remain permitted. */
#include <adelefeld.h>
#include <flint/acb_dirichlet.h>
#include <flint/acb_hypgeom.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include "support/jsonl.h"

#define VEC "tests/ref/vectors/t5-slice4/continue.jsonl"
#define RP 512                                            /* precision of the reference balls */

static unsigned long checks;
static const char *only;                                  /* run one section (argv[1]) */

#define CHECK(c) do { checks++; if (!(c)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #c); exit(1); } } while (0)

/* ------------------------------------------------------------------ the forbidden path */
static void forbidden_call(const char *name)
{
    fprintf(stderr, "test_tate_continue: forbidden path reached: %s\n", name);
    abort();
}
#ifdef ADF_TATE_LINK_WRAP
#define FORBID(name) __wrap_##name
#else
#define FORBID(name) name
#endif
void FORBID(acb_dirichlet_zeta)(acb_t r, const acb_t s, slong p)
{ (void) r; (void) s; (void) p; forbidden_call("acb_dirichlet_zeta"); }
void FORBID(acb_dirichlet_zeta_rs)(acb_t r, const acb_t s, slong K, slong p)
{ (void) r; (void) s; (void) K; (void) p; forbidden_call("acb_dirichlet_zeta_rs"); }
void FORBID(acb_dirichlet_zeta_jet)(acb_t r, const acb_t s, int d, slong n, slong p)
{ (void) r; (void) s; (void) d; (void) n; (void) p; forbidden_call("acb_dirichlet_zeta_jet"); }
void FORBID(acb_dirichlet_zeta_jet_rs)(acb_ptr r, const acb_t s, slong n, slong p)
{ (void) r; (void) s; (void) n; (void) p; forbidden_call("acb_dirichlet_zeta_jet_rs"); }
void FORBID(acb_dirichlet_hurwitz)(acb_t r, const acb_t s, const acb_t a, slong p)
{ (void) r; (void) s; (void) a; (void) p; forbidden_call("acb_dirichlet_hurwitz"); }
void FORBID(acb_dirichlet_hurwitz_precomp_eval)(acb_t r, const acb_dirichlet_hurwitz_precomp_t pre, ulong a,
                                                 ulong q, slong p)
{ (void) r; (void) pre; (void) a; (void) q; (void) p; forbidden_call("acb_dirichlet_hurwitz_precomp_eval"); }
void FORBID(acb_dirichlet_eta)(acb_t r, const acb_t s, slong p)
{ (void) r; (void) s; (void) p; forbidden_call("acb_dirichlet_eta"); }
void FORBID(acb_dirichlet_xi)(acb_t r, const acb_t s, slong p)
{ (void) r; (void) s; (void) p; forbidden_call("acb_dirichlet_xi"); }
void FORBID(acb_dirichlet_ui_theta_arb)(acb_t r, const dirichlet_group_t G, ulong a, const arb_t t, slong p)
{ (void) r; (void) G; (void) a; (void) t; (void) p; forbidden_call("acb_dirichlet_ui_theta_arb"); }
void FORBID(acb_dirichlet_chi_theta_arb)(acb_t r, const dirichlet_group_t G, const dirichlet_char_t c,
                                         const arb_t t, slong p)
{ (void) r; (void) G; (void) c; (void) t; (void) p; forbidden_call("acb_dirichlet_chi_theta_arb"); }
void FORBID(acb_dirichlet_root_number_theta)(acb_t r, const dirichlet_group_t G, const dirichlet_char_t c, slong p)
{ (void) r; (void) G; (void) c; (void) p; forbidden_call("acb_dirichlet_root_number_theta"); }
void FORBID(acb_dirichlet_l_euler_product)(acb_t r, const acb_t s, const dirichlet_group_t G,
                                           const dirichlet_char_t c, slong p)
{ (void) r; (void) s; (void) G; (void) c; (void) p; forbidden_call("acb_dirichlet_l_euler_product"); }
void FORBID(acb_dirichlet_l_hurwitz)(acb_t r, const acb_t s, const acb_dirichlet_hurwitz_precomp_t pre,
                                     const dirichlet_group_t G, const dirichlet_char_t c, slong p)
{ (void) r; (void) s; (void) pre; (void) G; (void) c; (void) p; forbidden_call("acb_dirichlet_l_hurwitz"); }
void FORBID(acb_dirichlet_l)(acb_t r, const acb_t s, const dirichlet_group_t G, const dirichlet_char_t c, slong p)
{ (void) r; (void) s; (void) G; (void) c; (void) p; forbidden_call("acb_dirichlet_l"); }
void FORBID(acb_dirichlet_l_vec_hurwitz)(acb_ptr r, const acb_t s, const acb_dirichlet_hurwitz_precomp_t pre,
                                         const dirichlet_group_t G, slong p)
{ (void) r; (void) s; (void) pre; (void) G; (void) p; forbidden_call("acb_dirichlet_l_vec_hurwitz"); }
void FORBID(acb_dirichlet_l_jet)(acb_ptr r, const acb_t s, const dirichlet_group_t G, const dirichlet_char_t c,
                                 int d, slong n, slong p)
{ (void) r; (void) s; (void) G; (void) c; (void) d; (void) n; (void) p; forbidden_call("acb_dirichlet_l_jet"); }
void FORBID(acb_dirichlet_l_fmpq)(acb_t r, const fmpq_t s, const dirichlet_group_t G, const dirichlet_char_t c,
                                  slong p)
{ (void) r; (void) s; (void) G; (void) c; (void) p; forbidden_call("acb_dirichlet_l_fmpq"); }
void FORBID(acb_dirichlet_l_fmpq_afe)(acb_t r, const fmpq_t s, const dirichlet_group_t G, const dirichlet_char_t c,
                                      slong p)
{ (void) r; (void) s; (void) G; (void) c; (void) p; forbidden_call("acb_dirichlet_l_fmpq_afe"); }
void FORBID(acb_zeta)(acb_t r, const acb_t s, slong p)
{ (void) r; (void) s; (void) p; forbidden_call("acb_zeta"); }
void FORBID(acb_hurwitz_zeta)(acb_t r, const acb_t s, const acb_t a, slong p)
{ (void) r; (void) s; (void) a; (void) p; forbidden_call("acb_hurwitz_zeta"); }
void FORBID(arb_zeta)(arb_t r, const arb_t s, slong p)
{ (void) r; (void) s; (void) p; forbidden_call("arb_zeta"); }
void FORBID(arb_zeta_ui)(arb_t r, ulong n, slong p)
{ (void) r; (void) n; (void) p; forbidden_call("arb_zeta_ui"); }
void FORBID(arb_hurwitz_zeta)(arb_t r, const arb_t s, const arb_t a, slong p)
{ (void) r; (void) s; (void) a; (void) p; forbidden_call("arb_hurwitz_zeta"); }

/* Not part of the interface (hidden in a shared object, as src/roots.c:49-53): one attempt of the balanced split
   at max(prec, 2) without the gate, mode 0 (I, the budget and factor C^-z) or 1 (Lambda); its cutoffs N, R, panel
   count K, the panel degrees of the two sides (deg[0..K), deg[K..2K)), the work charged; lam the assembled value
   (may be NULL), fwd and dual the two Mellin sides of P13 step 3 as assembled (the dual including W_chi), each
   with the total omitted and quadrature error added to both radii (may be NULL); sentinel bit 1 (2) replaces the
   computed forward (dual) side by the disjoint sentinel 10^6 (-10^6 i) before the assembly. */
int adf_tate_trace(ulong *N, ulong *R, ulong *K, ulong *deg, ulong *work, acb_t lam, acb_t fwd, acb_t dual,
                   const adf_char_t chi, const acb_t s, slong bits, slong prec, int mode, int sentinel);

static double seconds(void) { return (double) clock() / (double) CLOCKS_PER_SEC; }
static int section(const char *name) { return only == NULL || !strcmp(only, name); }

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
/* The spectral input: the point [re, im] (exact rationals, rounded at 4 RP bits, exact for the dyadic points of
   the vectors) plus the exact dyadic radii rad (NULL: exact). */
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
static void char_of(adf_char_t chi, const jsonl_value *rec)
{
    CHECK(adf_char_set_conrey(chi, (ulong) number(field(rec, "q")), (ulong) number(field(rec, "n"))) == ADF_OK);
}

/* ------------------------------------------------------------------ predicates and sentinels */
static int narrow(const acb_t z, slong bits)
{
    return acb_is_finite(z) && mag_cmp_2exp_si(arb_radref(acb_realref(z)), -bits - 1) <= 0
        && mag_cmp_2exp_si(arb_radref(acb_imagref(z)), -bits - 1) <= 0;
}
static void maxrad(mag_t m, const acb_t z) { mag_max(m, arb_radref(acb_realref(z)), arb_radref(acb_imagref(z))); }

typedef int (*tate_fn)(acb_t, const adf_char_t, const acb_t, slong, slong);
static const char *fname(tate_fn f) { return f == adf_tate_completed ? "completed" : "continue"; }

typedef struct { acb_t z, copy; unsigned char bytes[sizeof(acb_struct)]; } sentinel;
static void sent_init(sentinel *s)
{
    memset(s, 0, sizeof *s);                              /* defined padding and limbs for the byte comparison */
    acb_init(s->z); acb_init(s->copy);
    acb_set_si_si(s->z, 12345, -678); mag_set_ui_2exp_si(arb_radref(acb_realref(s->z)), 3, -7);
    acb_set(s->copy, s->z); memcpy(s->bytes, s->z, sizeof(acb_struct));
}
static void sent_check(const sentinel *s)
{
    CHECK(memcmp(s->bytes, s->z, sizeof(acb_struct)) == 0); CHECK(acb_equal(s->z, s->copy));
}
static void sent_clear(sentinel *s) { acb_clear(s->z); acb_clear(s->copy); }
/* The status of f; on failure the output is checked untouched. */
static int sent_call(tate_fn f, const adf_char_t chi, const acb_t s, slong bits, slong prec)
{
    sentinel t; int st;
    sent_init(&t); st = f(t.z, chi, s, bits, prec);
    if (st != ADF_OK) sent_check(&t);
    sent_clear(&t); return st;
}
/* C^w for w = (s + e)/2 (sgn = 1) or C^-w (sgn = -1) at RP bits. */
static void cfac(acb_t y, const adf_char_t chi, const acb_t s, int sgn)
{
    acb_t w; acb_init(w);
    acb_add_si(w, s, adf_char_get_parity(chi), RP); acb_mul_2exp_si(w, w, -1);
    if (sgn < 0) acb_neg(w, w);
    acb_set_ui(y, adf_char_get_conductor(chi)); acb_pow(y, y, w, RP);
    acb_clear(w);
}

/* ------------------------------------------------------------------ the forbidden path is wrapped */
static void forbidden(void)
{
    int k;
    for (k = 0; k < 4; k++) {
        pid_t p = fork(); int status; CHECK(p >= 0);
        if (!p) {
            acb_t r, s; dirichlet_group_t G; dirichlet_char_t c;
            acb_init(r); acb_init(s); acb_set_ui(s, 2);
            dirichlet_group_init(G, 5); dirichlet_char_init(c, G); dirichlet_char_log(c, G, 2);
            if (k == 0) acb_dirichlet_zeta(r, s, 64);
            if (k == 1) acb_dirichlet_l(r, s, G, c, 64);
            if (k == 2) acb_dirichlet_xi(r, s, 64);
            if (k == 3) acb_dirichlet_hurwitz(r, s, s, 64);
            /* reached only if the wrap is missing; the clears are for tools/memcheck */
            dirichlet_char_clear(c); dirichlet_group_clear(G); acb_clear(r); acb_clear(s);
            _exit(0);
        }
        CHECK(waitpid(p, &status, 0) == p && WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
    }
    printf("forbidden: 4 wrapped FLINT L/zeta/xi/Hurwitz calls abort\n");
}

/* ------------------------------------------------------------------ the values against the vectors */
static unsigned long values_ok, points_in, zeros_ok, values_want, zeros_want;
static void value_call(tate_fn f, const adf_char_t chi, const acb_t s, slong bits, const jsonl_value *rec,
                       const char *mp, const char *fl)
{
    acb_t z, ref; int st;
    acb_init(z); acb_init(ref);
    st = f(z, chi, s, bits, bits + 32);
    if (st != ADF_OK)
        fprintf(stderr, "value %s: q=%ld n=%ld s=%s+%si bits=%ld: status %d\n", fname(f), number(field(rec, "q")),
                number(field(rec, "n")), str(at(field(rec, "s"), 0)), str(at(field(rec, "s"), 1)), (long) bits, st);
    CHECK(st == ADF_OK);
    CHECK(narrow(z, bits));
    ball_of(ref, field(rec, mp)); CHECK(acb_contains(z, ref)); points_in++;
    ball_of(ref, field(rec, fl)); CHECK(acb_contains(z, ref)); points_in++;
    values_ok++;
    acb_clear(z); acb_clear(ref);
}
static void value_record(const jsonl_value *rec)
{
    adf_char_t chi; acb_t s, zi, zl, y; long q = number(field(rec, "q")); int k;
    static const slong more[2] = { 20, 80 };
    adf_char_init(chi); acb_init(s); acb_init(zi); acb_init(zl); acb_init(y);
    char_of(chi, rec); s_of(s, field(rec, "s"), NULL);
    CHECK(acb_is_exact(s));
    value_call(adf_tate_continue, chi, s, 53, rec, "I", "If");
    value_call(adf_tate_completed, chi, s, 53, rec, "L", "Lf");
    values_want += 2;
    zeros_want += (unsigned long) number(field(rec, "zero"));
    if (q == 1) {
        /* a coarse target: N = 1 (one nonzero coefficient), where the T3 remainder alone carries the error */
        value_call(adf_tate_continue, chi, s, 4, rec, "I", "If");
        value_call(adf_tate_completed, chi, s, 4, rec, "L", "Lf");
        values_want += 2;
    }
    if (q == 1 || q == 4 || q == 5 || q == 16)
        for (k = 0; k < 2; k++) {
            values_want += 2;
            value_call(adf_tate_continue, chi, s, more[k], rec, "I", "If");
            value_call(adf_tate_completed, chi, s, more[k], rec, "L", "Lf");
        }
    if (number(field(rec, "zero"))) {
        /* a trivial zero of L: Lambda finite and nonzero, I = C^-z Lambda finite (the product Gamma(z) L(s) would
           be infinity times 0) */
        CHECK(adf_tate_completed(zl, chi, s, 53, 85) == ADF_OK && narrow(zl, 53) && !acb_contains_zero(zl));
        CHECK(adf_tate_continue(zi, chi, s, 53, 85) == ADF_OK && narrow(zi, 53));
        cfac(y, chi, s, -1); acb_mul(y, y, zl, RP);
        CHECK(acb_overlaps(zi, y));
        zeros_ok++;
    }
    adf_char_clear(chi); acb_clear(s); acb_clear(zi); acb_clear(zl); acb_clear(y);
}

/* The input balls: at bits 80 the radius 2^-60 holds the width (two corner references differ by more than 2^-79
   in a coordinate: no OK is possible, N-D23): NOT_DETERMINED with z untouched; at bits 40 OK containing every
   corner. The zeta witness on [-1/4, -1/8] (end values more than 1 apart): NOT_DETERMINED at bits 0 and 20. The
   pole-free rectangle near 1: OK, finite, containing both end values (mpmath and FLINT). */
static unsigned long balls_done;
static void ball_record(const jsonl_value *rec)
{
    adf_char_t chi; acb_t s, z, ref, d; const jsonl_value *pts = field(rec, "points"); size_t k, j, np;
    fmpq_t rr; int f;
    adf_char_init(chi); acb_init(s); acb_init(z); acb_init(ref); acb_init(d); fmpq_init(rr);
    char_of(chi, rec); s_of(s, field(rec, "s"), field(rec, "rad"));
    np = jsonl_size(pts);
    q_of(rr, str(at(field(rec, "rad"), 0)));
    if (fmpz_cmp_ui(fmpq_denref(rr), 16) == 0) {                     /* the witness, radius 1/16 */
        CHECK(sent_call(adf_tate_completed, chi, s, 0, 64) == ADF_NOT_DETERMINED);
        CHECK(sent_call(adf_tate_completed, chi, s, 20, 64) == ADF_NOT_DETERMINED);
        CHECK(sent_call(adf_tate_continue, chi, s, 0, 64) == ADF_NOT_DETERMINED);
        ball_of(ref, field(at(pts, 0), "L")); ball_of(d, field(at(pts, 1), "L")); acb_sub(d, d, ref, RP);
        CHECK(arf_cmpabs_2exp_si(arb_midref(acb_realref(d)), 0) > 0);
    } else if (fmpz_cmp_ui(fmpq_numref(rr), 1) == 0 && fmpz_bits(fmpq_denref(rr)) > 100) {   /* 2^-200 near 1 */
        CHECK(adf_tate_completed(z, chi, s, 70, 128) == ADF_OK && narrow(z, 70));
        for (k = 0; k < np; k++) {
            ball_of(ref, field(at(pts, k), "L")); CHECK(acb_contains(z, ref));
            ball_of(ref, field(at(pts, k), "Lf")); CHECK(acb_contains(z, ref));
        }
        CHECK(adf_tate_continue(z, chi, s, 70, 128) == ADF_OK && narrow(z, 70));
        for (k = 0; k < np; k++) { ball_of(ref, field(at(pts, k), "I")); CHECK(acb_contains(z, ref)); }
    } else {                                                          /* radius 2^-60 */
        for (f = 0; f < 2; f++) {
            tate_fn fn = f ? adf_tate_completed : adf_tate_continue;
            const char *key = f ? "L" : "I";
            int far = 0;
            for (k = 0; k < np; k++)
                for (j = k + 1; j < np; j++) {
                    ball_of(ref, field(at(pts, k), key)); ball_of(d, field(at(pts, j), key));
                    acb_sub(d, d, ref, RP);
                    CHECK(mag_cmp_2exp_si(arb_radref(acb_realref(d)), -90) < 0);
                    far |= arf_cmpabs_2exp_si(arb_midref(acb_realref(d)), -79) > 0
                        || arf_cmpabs_2exp_si(arb_midref(acb_imagref(d)), -79) > 0;
                }
            CHECK(far);                                               /* no OK at bits 80 is possible */
            CHECK(sent_call(fn, chi, s, 80, 112) == ADF_NOT_DETERMINED);
            CHECK(fn(z, chi, s, 40, 72) == ADF_OK && narrow(z, 40));
            for (k = 0; k < np; k++) { ball_of(ref, field(at(pts, k), key)); CHECK(acb_contains(z, ref)); }
        }
    }
    balls_done++;
    adf_char_clear(chi); acb_clear(s); acb_clear(z); acb_clear(ref); acb_clear(d); fmpq_clear(rr);
}

/* The pole geometry against the oracle's pole_status (api-5.md:230-235): DOMAIN and NOT_DETERMINED exactly as the
   oracle says, for both calls, z untouched; an OK geometry gives OK (finite, narrow at bits 4) when the radius is
   at most 2^-10, and is never DOMAIN (a wide pole-free rectangle may still fail the width). */
static unsigned long poles_done;
static void pole_record(const jsonl_value *rec)
{
    adf_char_t chi; acb_t s, z; const char *want = str(field(rec, "status")); int f, st;
    mag_t m;
    adf_char_init(chi); acb_init(s); acb_init(z); mag_init(m);
    char_of(chi, rec); s_of(s, field(rec, "s"), field(rec, "rad"));
    maxrad(m, s);
    for (f = 0; f < 2; f++) {
        tate_fn fn = f ? adf_tate_completed : adf_tate_continue;
        st = sent_call(fn, chi, s, 4, 64);
        if (!strcmp(want, "DOMAIN")) CHECK(st == ADF_DOMAIN);
        else if (!strcmp(want, "NOT_DETERMINED")) CHECK(st == ADF_NOT_DETERMINED);
        else {
            CHECK(!strcmp(want, "OK"));
            if (mag_cmp_2exp_si(m, -10) <= 0) {
                CHECK(st == ADF_OK);
                CHECK(fn(z, chi, s, 4, 64) == ADF_OK && narrow(z, 4));
            } else
                CHECK(st == ADF_OK || st == ADF_NOT_DETERMINED);
        }
    }
    poles_done++;
    adf_char_clear(chi); acb_clear(s); acb_clear(z); mag_clear(m);
}

/* (s - 1) Lambda(s) -> 1 at s = 1 + 2^-k and s Lambda(s) -> -1 at s = 2^-k (SPEC 8; P13: residues +1, -1): the
   product contains the 60-digit reference and its distance to the residue is below 2^(1-k). */
static unsigned long residues_done;
static void residue_record(const jsonl_value *rec)
{
    adf_char_t chi; acb_t s, z, h, ref; long k = number(field(rec, "k"));
    int one = !strcmp(str(field(rec, "which")), "one");
    adf_char_init(chi); acb_init(s); acb_init(z); acb_init(h); acb_init(ref);
    s_of(s, field(rec, "s"), NULL);
    CHECK(adf_tate_completed(z, chi, s, 80, 160) == ADF_OK && narrow(z, 80));
    acb_one(h); acb_mul_2exp_si(h, h, -k);                           /* the displacement 2^-k */
    acb_mul(z, z, h, RP);
    ball_of(ref, field(rec, "value"));
    CHECK(acb_contains(z, ref));
    acb_add_si(z, z, one ? -1 : 1, RP);                               /* minus the residue */
    CHECK(arb_is_finite(acb_realref(z)) && arf_cmpabs_2exp_si(arb_midref(acb_realref(z)), 1 - k) < 0);
    CHECK(mag_cmp_2exp_si(arb_radref(acb_realref(z)), 1 - k) < 0);
    residues_done++;
    adf_char_clear(chi); acb_clear(s); acb_clear(z); acb_clear(h); acb_clear(ref);
}

/* The certificate against the oracle's continuation (Lambda): N, R and every panel degree of both sides. */
static unsigned long cutoffs_done, bounds_checked;
static void cutoff_record(const jsonl_value *rec)
{
    adf_char_t chi; acb_t s, lam; ulong N, R, K, deg[128], work; size_t k, j;
    const jsonl_value *dg = field(rec, "degrees"); slong bits = number(field(rec, "bits"));
    adf_char_init(chi); acb_init(s); acb_init(lam);
    char_of(chi, rec); s_of(s, field(rec, "s"), NULL);
    CHECK(adf_tate_trace(&N, &R, &K, deg, &work, lam, NULL, NULL, chi, s, bits, bits + 64, 1, 0) == ADF_OK);
    CHECK(N == (ulong) number(field(rec, "N")) && R == (ulong) number(field(rec, "R")));
    /* the certified bound E_n + E_t + both remainders is on the radii (api-5.md:218): for a real s the imaginary
       part of Lambda is 0 up to rounding and the bound, so its radius is at least the oracle's bound */
    if (arb_is_zero(acb_imagref(s))) {
        arb_t b; arf_t u; arb_init(b); arf_init(u);
        CHECK(arb_set_str(b, str(field(rec, "bound")), 64) == 0);
        arb_mul_ui(b, b, 999999, 64); arb_div_ui(b, b, 1000000, 64);          /* (1 - 10^-6) b: decimal rounding */
        arb_get_lbound_arf(u, b, 64);
        CHECK(arf_sgn(u) > 0);
        { arf_t r; arf_init(r); arf_set_mag(r, arb_radref(acb_imagref(lam)));
          CHECK(arf_cmp(r, u) >= 0); arf_clear(r); }
        bounds_checked++;
        arb_clear(b); arf_clear(u);
    }
    CHECK(jsonl_size(at(dg, 0)) == K && jsonl_size(at(dg, 1)) == K);
    for (k = 0; k < 2; k++)
        for (j = 0; j < K; j++)
            CHECK(deg[k * K + j] == (ulong) number(at(at(dg, k), j)));
    CHECK(work > 0 && work <= (ulong) ADF_TATE_WORK_MAX);
    cutoffs_done++;
    adf_char_clear(chi); acb_clear(s); acb_clear(lam);
}

/* The independence of the two Mellin sides (api-5.md:350-352): each side against its own 60-digit reference
   (mpmath's incomplete Gamma, independent of the other side); replacing one side by a disjoint sentinel changes
   that side alone (the other side bit for bit equal) and moves the assembled value. */
static unsigned long sides_done;
static void side_record(const jsonl_value *rec)
{
    adf_char_t chi; acb_t s, l0, f0, d0, l1, f1, d1, ref, t; ulong N, R, K, deg[128], work; int k;
    adf_char_init(chi); acb_init(s); acb_init(l0); acb_init(f0); acb_init(d0); acb_init(l1); acb_init(f1);
    acb_init(d1); acb_init(ref); acb_init(t);
    char_of(chi, rec); s_of(s, field(rec, "s"), NULL);
    CHECK(adf_tate_trace(&N, &R, &K, deg, &work, l0, f0, d0, chi, s, 60, 128, 1, 0) == ADF_OK);
    ball_of(ref, field(rec, "fwd")); CHECK(acb_contains(f0, ref));
    ball_of(ref, field(rec, "dual")); CHECK(acb_contains(d0, ref));
    CHECK(narrow(f0, 56) && narrow(d0, 56));
    for (k = 1; k <= 2; k++) {
        CHECK(adf_tate_trace(&N, &R, &K, deg, &work, l1, f1, d1, chi, s, 60, 128, 1, k) == ADF_OK);
        if (k == 1) {
            CHECK(acb_equal(d1, d0));
            CHECK(arf_cmp_si(arb_midref(acb_realref(f1)), 1000000) == 0);
            CHECK(arf_is_zero(arb_midref(acb_imagref(f1))));
            CHECK(!acb_overlaps(f1, f0));
            acb_sub(t, l0, f0, RP); acb_add(t, t, f1, RP);
        } else {
            CHECK(acb_equal(f1, f0));
            CHECK(arf_cmp_si(arb_midref(acb_imagref(d1)), -1000000) == 0);
            CHECK(arf_is_zero(arb_midref(acb_realref(d1))));
            CHECK(!acb_overlaps(d1, d0));
            acb_sub(t, l0, d0, RP); acb_add(t, t, d1, RP);
        }
        CHECK(!acb_overlaps(l1, l0) && acb_overlaps(l1, t));
    }
    sides_done++;
    adf_char_clear(chi); acb_clear(s); acb_clear(l0); acb_clear(f0); acb_clear(d0); acb_clear(l1); acb_clear(f1);
    acb_clear(d1); acb_clear(ref); acb_clear(t);
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
        else if (!strcmp(kind, "pole")) { if (section("poles")) pole_record(rec); }
        else if (!strcmp(kind, "residue")) { if (section("residues")) residue_record(rec); }
        else if (!strcmp(kind, "cutoff")) { if (section("cutoffs")) cutoff_record(rec); }
        else if (!strcmp(kind, "side")) { if (section("sides")) side_record(rec); }
        else CHECK(0);
    }
    jsonl_close(file);
    CHECK(nval == 208);
    /* 208 records: 2 calls each at bits 53, 4 more for C = 1, 4, 5, 16 (81 records), 2 more at bits 4 for C = 1 (24
       records): 788 OK calls; 40 trivial zeros */
    if (section("values")) CHECK(values_ok == values_want && values_want == 788 && zeros_ok == zeros_want
                                 && zeros_want == 40);
    if (section("balls")) CHECK(balls_done == 8);
    if (section("poles")) CHECK(poles_done == 45);
    if (section("residues")) CHECK(residues_done == 6);
    if (section("cutoffs")) CHECK(cutoffs_done == 27 && bounds_checked == 18);
    if (section("sides")) CHECK(sides_done == 6);
    printf("vectors: %lu OK calls (%lu reference points inside, %lu trivial zeros), %lu balls, %lu pole statuses,"
           " %lu residues, %lu cutoff records, %lu side records in %.2f s\n", values_ok, points_in, zeros_ok,
           balls_done, poles_done, residues_done, cutoffs_done, sides_done, seconds() - t0);
}

/* ------------------------------------------------------------------ hand values and the half-plane overlap */
static void overlap(void)
{
    static const ulong pq[3][2] = { { 1, 1 }, { 5, 2 }, { 16, 3 } };
    static const slong sv[3][2] = { { 2, 0 }, { 3, 0 }, { 2, 3 } };
    adf_char_t chi; acb_t s, a, b, y; int c, k;
    adf_char_init(chi); acb_init(s); acb_init(a); acb_init(b); acb_init(y);
    for (c = 0; c < 3; c++) {
        CHECK(adf_char_set_conrey(chi, pq[c][0], pq[c][1]) == ADF_OK);
        for (k = 0; k < 3; k++) {
            acb_set_si_si(s, sv[k][0], sv[k][1]);
            /* adf_tate_continue equals adf_tate_integral on Re(s) > 1: both narrow at the same bits, overlapping */
            CHECK(adf_tate_integral(a, chi, s, 53, 96) == ADF_OK && narrow(a, 53));
            CHECK(adf_tate_continue(b, chi, s, 53, 96) == ADF_OK && narrow(b, 53));
            CHECK(acb_overlaps(a, b));
            /* adf_tate_completed is C^z times it, narrow at its own target (not C^z times a narrow I) */
            CHECK(adf_tate_completed(b, chi, s, 53, 96) == ADF_OK && narrow(b, 53));
            cfac(y, chi, s, 1); acb_mul(y, y, a, RP);
            CHECK(acb_overlaps(b, y));
        }
    }
    /* chi_4 at 1 (e = 1, z = 1): L(1, chi_4) = pi/4, Lambda = (4/pi) Gamma(1) pi/4 = 1, I = 4^-1 Lambda = 1/4; at
       0 by the functional equation (P13) with W = 1 and conj(chi_4) = chi_4: Lambda(0) = Lambda(1) = 1 */
    CHECK(adf_char_set_conrey(chi, 4, 3) == ADF_OK);
    acb_one(s);
    CHECK(adf_tate_completed(b, chi, s, 60, 96) == ADF_OK && narrow(b, 60));
    acb_one(y); CHECK(acb_contains(b, y));
    CHECK(adf_tate_continue(b, chi, s, 60, 96) == ADF_OK && narrow(b, 60));
    acb_mul_2exp_si(y, y, -2); CHECK(acb_contains(b, y));
    acb_zero(s);
    CHECK(adf_tate_completed(b, chi, s, 60, 96) == ADF_OK && narrow(b, 60));
    acb_one(y); CHECK(acb_contains(b, y));
    /* zeta(2): Lambda(2) = I(2) = pi/6; Lambda(-1) = Lambda(2) by the functional equation */
    CHECK(adf_char_set_conrey(chi, 1, 1) == ADF_OK);
    acb_set_si(s, -1);
    CHECK(adf_tate_completed(b, chi, s, 80, 128) == ADF_OK && narrow(b, 80));
    arb_const_pi(acb_realref(y), RP); arb_div_ui(acb_realref(y), acb_realref(y), 6, RP); arb_zero(acb_imagref(y));
    CHECK(acb_contains(b, y));
    /* C > 1: the local Gamma poles s = -e - 2k and s = 0, 1 are no global poles: OK, finite */
    {
        static const ulong cq[3][2] = { { 5, 2 }, { 5, 4 }, { 4, 3 } };
        for (c = 0; c < 3; c++) {
            int e;
            CHECK(adf_char_set_conrey(chi, cq[c][0], cq[c][1]) == ADF_OK); e = adf_char_get_parity(chi);
            for (k = 0; k < 4; k++) {
                acb_set_si(s, k < 2 ? k : -e - 2 * (k - 2));
                CHECK(adf_tate_continue(b, chi, s, 40, 64) == ADF_OK && narrow(b, 40));
                CHECK(adf_tate_completed(b, chi, s, 40, 64) == ADF_OK && narrow(b, 40));
            }
        }
    }
    adf_char_clear(chi); acb_clear(s); acb_clear(a); acb_clear(b); acb_clear(y);
}

/* ------------------------------------------------------------------ statuses, near the poles, caps */
static void statuses(void)
{
    adf_char_t chi, big; acb_t s, z; double t0; int f;
    adf_char_init(chi); adf_char_init(big); acb_init(s); acb_init(z);
    for (f = 0; f < 2; f++) {
        tate_fn fn = f ? adf_tate_completed : adf_tate_continue;
        CHECK(adf_char_set_conrey(chi, 1, 1) == ADF_OK);
        /* exact poles DOMAIN, also when the radius is the exact zero of a computed-looking ball */
        acb_zero(s); CHECK(sent_call(fn, chi, s, 20, 64) == ADF_DOMAIN);
        acb_one(s); CHECK(sent_call(fn, chi, s, 20, 64) == ADF_DOMAIN);
        /* a straddling ball and closed end-point contact: NOT_DETERMINED */
        acb_one(s); mag_set_ui_2exp_si(arb_radref(acb_realref(s)), 1, -10);
        CHECK(sent_call(fn, chi, s, 20, 64) == ADF_NOT_DETERMINED);
        arf_set_si_2exp_si(arb_midref(acb_realref(s)), 1023, -10);               /* [1 - 2^-9, 1] */
        CHECK(sent_call(fn, chi, s, 20, 64) == ADF_NOT_DETERMINED);
        arf_set_si_2exp_si(arb_midref(acb_realref(s)), -1, -10);                 /* [-2^-9, 0] */
        CHECK(sent_call(fn, chi, s, 20, 64) == ADF_NOT_DETERMINED);
        acb_one(s); arf_set_si_2exp_si(arb_midref(acb_imagref(s)), 1, -10);      /* 1 + [0, 2^-9] i */
        mag_set_ui_2exp_si(arb_radref(acb_imagref(s)), 1, -10);
        CHECK(sent_call(fn, chi, s, 20, 64) == ADF_NOT_DETERMINED);
        /* the pole geometry comes before the work (tate.h): at bits 2^21 a ball meeting a pole is NOT_DETERMINED at
           once, while an attempt would end in LIMIT (the work cap below); also for end-point contact */
        CHECK(sent_call(fn, chi, s, 2097152, 64) == ADF_NOT_DETERMINED);
        acb_one(s); arf_set_si_2exp_si(arb_midref(acb_realref(s)), 1023, -10);
        mag_set_ui_2exp_si(arb_radref(acb_realref(s)), 1, -10);
        CHECK(sent_call(fn, chi, s, 2097152, 64) == ADF_NOT_DETERMINED);
        acb_zero(s); arf_set_si_2exp_si(arb_midref(acb_realref(s)), 1, -10);
        mag_set_ui_2exp_si(arb_radref(acb_realref(s)), 1, -10);
        CHECK(sent_call(fn, chi, s, 2097152, 64) == ADF_NOT_DETERMINED);
        /* 1 + 10^-3 i (a dyadic 2^-10 here): OK, near the pole, the value about -2^10 i */
        acb_one(s); arb_set_si(acb_imagref(s), 1); arb_mul_2exp_si(acb_imagref(s), acb_imagref(s), -10);
        CHECK(fn(z, chi, s, 40, 64) == ADF_OK && narrow(z, 40));
        CHECK(arf_cmp_si(arb_midref(acb_imagref(z)), -1000) < 0);
        CHECK(arf_cmp_si(arb_midref(acb_imagref(z)), -1050) > 0);
        /* a complex rectangle near 1 that its covering disk excludes (T4): 1 + 2^-30 (1 + i) +/- 2^-110 (the image
           radius is about |s - 1|^-2 sqrt(2) 2^-110 = 2^-50.5, below the target) */
        acb_set_si_si(s, 1, 1); acb_mul_2exp_si(s, s, -30); acb_add_ui(s, s, 1, 128);
        mag_set_ui_2exp_si(arb_radref(acb_realref(s)), 1, -110);
        mag_set_ui_2exp_si(arb_radref(acb_imagref(s)), 1, -110);
        CHECK(fn(z, chi, s, 40, 64) == ADF_OK && narrow(z, 40));
        /* a rectangle that excludes 1 while its covering disk does not: 1 + [2^-20, 2^-10 + ...] x [-r, r] with the
           width far above 1: never DOMAIN, NOT_DETERMINED by the width */
        arf_set_si_2exp_si(arb_midref(acb_realref(s)), 1025, -10); arb_zero(acb_imagref(s));
        mag_set_ui_2exp_si(arb_radref(acb_realref(s)), 1023, -20);
        mag_set_ui_2exp_si(arb_radref(acb_imagref(s)), 1, -11);
        acb_sub_ui(z, s, 1, 128); CHECK(!acb_contains_zero(z));
        CHECK(sent_call(fn, chi, s, 0, 64) == ADF_NOT_DETERMINED);
        /* nonfinite s DOMAIN */
        acb_indeterminate(s); CHECK(sent_call(fn, chi, s, 20, 64) == ADF_DOMAIN);
        acb_set_ui(s, 2); arb_pos_inf(acb_imagref(s)); CHECK(sent_call(fn, chi, s, 20, 64) == ADF_DOMAIN);
        /* bits outside [0, 2^21]: DOMAIN after the precision cap and the size cap */
        acb_set_si(s, -1);
        CHECK(sent_call(fn, chi, s, -1, 64) == ADF_DOMAIN);
        CHECK(sent_call(fn, chi, s, 2097153, 64) == ADF_DOMAIN);
        CHECK(sent_call(fn, chi, s, WORD_MIN, 64) == ADF_DOMAIN);
        CHECK(sent_call(fn, chi, s, -1, ADF_REAL_PREC_MAX + 1) == ADF_LIMIT);
        acb_zero(s); CHECK(sent_call(fn, chi, s, 20, ADF_REAL_PREC_MAX + 1) == ADF_LIMIT);
        acb_zero(s); CHECK(sent_call(fn, chi, s, -1, 64) == ADF_DOMAIN);
        acb_set_si(s, -1);
        CHECK(adf_char_set_conrey(big, 1031, 2) == ADF_OK);
        CHECK(sent_call(fn, big, s, -1, 64) == ADF_LIMIT);
        CHECK(sent_call(fn, big, s, 20, 64) == ADF_LIMIT);
        /* the D2 work cap: bits 2^21 and C = 1021 are LIMIT within 10 s */
        t0 = seconds();
        CHECK(sent_call(fn, chi, s, 2097152, 64) == ADF_LIMIT);
        CHECK(adf_char_set_conrey(big, 1021, 2) == ADF_OK);
        CHECK(sent_call(fn, big, s, 20, 64) == ADF_LIMIT);
        printf("work cap (%s): bits 2^21 and C = 1021 LIMIT in %.2f s\n", fname(fn), seconds() - t0);
        CHECK(seconds() - t0 < 10);
        /* C = 1024 = 2^10 (n = 3): passes the size check, but C + C^2 > 2^20: the charge before the transform
           refuses at once (as in test_tate) */
        CHECK(adf_char_set_conrey(big, 1024, 3) == ADF_OK && adf_char_get_conductor(big) == 1024);
        t0 = seconds();
        CHECK(sent_call(fn, big, s, 20, 64) == ADF_LIMIT);
        CHECK(seconds() - t0 < 0.5);
        /* bits 0 admitted; prec below 2 works at 2 */
        CHECK(fn(z, chi, s, 0, 64) == ADF_OK && narrow(z, 0));
        CHECK(fn(z, chi, s, 30, -5) == ADF_OK && narrow(z, 30));
    }
    adf_char_clear(chi); adf_char_clear(big); acb_clear(s); acb_clear(z);
}

/* ------------------------------------------------------------------ widths and aliasing */
static void widths(void)
{
    static const slong bt[3] = { 20, 53, 80 };
    static const ulong pq[3][2] = { { 1, 1 }, { 5, 2 }, { 16, 3 } };
    adf_char_t chi; acb_t s, z, w; mag_t prev, m; int k, c, f, p;
    adf_char_init(chi); acb_init(s); acb_init(z); acb_init(w); mag_init(prev); mag_init(m);
    for (f = 0; f < 2; f++) {
        tate_fn fn = f ? adf_tate_completed : adf_tate_continue;
        for (c = 0; c < 3; c++) {
            CHECK(adf_char_set_conrey(chi, pq[c][0], pq[c][1]) == ADF_OK);
            for (p = 0; p < 2; p++) {
                acb_set_si(s, p ? -3 : 1); acb_mul_2exp_si(s, s, -1);              /* s = 1/2, -3/2 */
                for (k = 0; k < 3; k++) {
                    CHECK(fn(z, chi, s, bt[k], bt[k] + 64) == ADF_OK && narrow(z, bt[k]));
                    maxrad(m, z);
                    if (k) CHECK(mag_cmp(m, prev) < 0);
                    mag_set(prev, m);
                    CHECK(fn(w, chi, s, bt[k], 2) == ADF_OK && narrow(w, bt[k]) && acb_overlaps(z, w));
                }
            }
            /* z = s: the same value as into a distinct output; a failing aliased call leaves s */
            acb_set_si_si(s, -1, 2);
            CHECK(fn(z, chi, s, 53, 100) == ADF_OK);
            acb_set(w, s);
            CHECK(fn(w, chi, w, 53, 100) == ADF_OK && acb_equal(w, z));
            acb_set(w, s); mag_set_ui_2exp_si(arb_radref(acb_realref(w)), 1, -2);
            acb_set(z, w);
            CHECK(fn(w, chi, w, 53, 100) == ADF_NOT_DETERMINED && acb_equal(w, z));           /* width */
            if (c == 0) { acb_one(w); CHECK(fn(w, chi, w, 53, 100) == ADF_DOMAIN && acb_is_one(w)); }
        }
    }
    adf_char_clear(chi); acb_clear(s); acb_clear(z); acb_clear(w); mag_clear(prev); mag_clear(m);
}

/* ------------------------------------------------------------------ L14's second branch (lane t5-repair1) */
/* The cutoffs, panel degrees, work and the bound added to both radii (api-5.md:218) against an evaluation of one
   attempt written for lane t5-repair1 (lanes/t5-repair1/gen_vectors.py, mpmath at 60 digits, from L6
   analysis.md:215-221, L14 :630-639, P15 :652-661, api-5.md:196-220 and api-5b.md decision 5), for zeta (C = 1,
   e = 0, mode Lambda) at s = 41/2, 9, -39/2, -8, 2 and bits 4, 20, 53, 80. With the sentinel bit 1 the forward
   side is the exact 10^6 before the error is added, so its radii are the bound itself (rounded up to a mag).
   Why the second branch of L14 (analysis.md:630-639) is the one taken: b = lower(pi/C) < 3.15 for C = 1, and
   the first branch needs b >= 2 rplus/R. At s = 41/2, r1 = Re(z) - 1 = 41/4 - 1 = 37/4 > 0, so 2 rplus = 37/2;
   every step of the N search has R = 1 and b R < 3.15 < 37/2, and the R search steps R = 1, 2, 4 have
   b R < 12.6 < 37/2: J_bound(r1, b, R) is (R0 - R) t*^r exp(-b t*) + 2 R0^r exp(-b R0)/b there. At s = -39/2 the
   dual exponent z' = (1 - s)/2 = 41/4 gives r2 = 37/4, the same. At s = 9 (r1 = 7/2) and s = -8 (r2 = 7/2),
   2 rplus = 7 > b R for R = 1, 2. The generator records the branch of every step (nbranches, rbranches) and
   asserts every comparison with a relative margin of 10^-9. Lane t5-slice4's survivor 247 (exp(-(R0 - R)) for
   exp(-b R0)) and lane t5-slice2's survivor 551 (r2 = -1) each change N, R, a degree, the work or the bound by
   more than 10^-6 relative in the records whose "kills" names them (the generator evaluates both mutants). */
#define JVEC "tests/ref/vectors/t5-repair1/jbound.jsonl"
static void jbound(void)
{
    jsonl_file *file; jsonl_error_t err; size_t i, n, j, k, m247 = 0, m551 = 0, second = 0;
    adf_char_t chi; acb_t s, lam, fwd; fmpq_t q; arb_t b, lo, hi; arf_t r;
    adf_char_init(chi); acb_init(s); acb_init(lam); acb_init(fwd); fmpq_init(q);
    arb_init(b); arb_init(lo); arb_init(hi); arf_init(r);
    CHECK(jsonl_open(JVEC, &file, &err));
    n = jsonl_count(file);
    CHECK(n == 20);
    for (i = 0; i < n; i++) {
        const jsonl_value *rec = jsonl_record(file, i), *dg = field(rec, "degrees"), *kl = field(rec, "kills");
        ulong N, R, K, deg[128], work; slong bits = number(field(rec, "bits")); int c;
        CHECK(!strcmp(str(field(rec, "kind")), "jbound"));
        char_of(chi, rec);
        q_of(q, str(field(rec, "s"))); acb_zero(s); arb_set_fmpq(acb_realref(s), q, 4 * RP);
        CHECK(acb_is_exact(s));
        CHECK(adf_tate_trace(&N, &R, &K, deg, &work, lam, fwd, NULL, chi, s, bits, bits + 64, 1, 1) == ADF_OK);
        CHECK(N == (ulong) number(field(rec, "N")) && R == (ulong) number(field(rec, "R")));
        CHECK(K == (ulong) number(field(rec, "K")));
        CHECK(jsonl_size(at(dg, 0)) == K && jsonl_size(at(dg, 1)) == K);
        for (k = 0; k < 2; k++)
            for (j = 0; j < K; j++)
                CHECK(deg[k * K + j] == (ulong) number(at(at(dg, k), j)));
        CHECK(work == (ulong) number(field(rec, "work")));
        /* the radii of the sentinel side are the bound E_n + E_t + both remainders, within 10^-6 relative */
        CHECK(arf_cmp_si(arb_midref(acb_realref(fwd)), 1000000) == 0 && arf_is_zero(arb_midref(acb_imagref(fwd))));
        CHECK(arb_set_str(b, str(field(rec, "bound")), 128) == 0);
        arb_mul_ui(lo, b, 999999, 128); arb_div_ui(lo, lo, 1000000, 128);
        arb_mul_ui(hi, b, 1000001, 128); arb_div_ui(hi, hi, 1000000, 128);
        for (c = 0; c < 2; c++) {
            arf_set_mag(r, arb_radref(c ? acb_imagref(fwd) : acb_realref(fwd)));
            CHECK(arf_cmp(r, arb_midref(lo)) > 0 && arf_cmp(r, arb_midref(hi)) < 0);
        }
        for (j = 0; j < jsonl_size(kl); j++) {
            m247 += !strcmp(str(at(kl, j)), "m247");
            m551 += !strcmp(str(at(kl, j)), "m551");
        }
        {   /* the second branch is recorded at the last N step for s = 41/2, 9, -39/2, -8 */
            const jsonl_value *nb = field(rec, "nbranches"), *last = at(nb, jsonl_size(nb) - 1);
            second += number(at(last, 0)) == 2 || number(at(last, 1)) == 2;
        }
    }
    jsonl_close(file);
    CHECK(m247 == 10 && m551 == 13 && second == 16);
    printf("jbound: %lu records (cutoffs, degrees, work and the radius bound; %lu tell m247, %lu tell m551)\n",
           (unsigned long) n, (unsigned long) m247, (unsigned long) m551);
    adf_char_clear(chi); acb_clear(s); acb_clear(lam); acb_clear(fwd); fmpq_clear(q);
    arb_clear(b); arb_clear(lo); arb_clear(hi); arf_clear(r);
}

/* ------------------------------------------------------------------ INV: preconditions */
static void debug(void)
{
#ifdef ADF_CHECK_INVARIANTS
    int k;
    for (k = 0; k < 4; k++) {
        pid_t p = fork(); int status; CHECK(p >= 0);
        if (!p) {
            adf_char_t chi; acb_t z, s;
            adf_char_init(chi); acb_init(z); acb_init(s); acb_set_si(s, -1);
            if (k == 0) { chi->n = 0; (void) adf_tate_continue(z, chi, s, 20, 64); }
            if (k == 1) { chi->n = 0; (void) adf_tate_completed(z, chi, s, 20, 64); }
            if (k == 2) { acb_set_ui(chi->s, 2); (void) adf_tate_continue(chi->s, chi, s, 20, 64); }
            if (k == 3) { CHECK(adf_char_set_conrey(chi, 5, 2) == ADF_OK); chi->parity = 0;
                          (void) adf_tate_completed(z, chi, s, 20, 64); }
            /* reached only if the entry check is missing; the clears are for tools/memcheck */
            chi->n = 1; chi->parity = 0; chi->q = 1;
            adf_char_clear(chi); acb_clear(z); acb_clear(s);
            _exit(0);
        }
        CHECK(waitpid(p, &status, 0) == p && WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);
    }
    {   /* the precision cap is decided before the entry predicates */
        adf_char_t chi; acb_t z, s;
        adf_char_init(chi); acb_init(z); acb_init(s); acb_set_si(s, -1);
        chi->n = 0;
        CHECK(adf_tate_continue(z, chi, s, 20, ADF_REAL_PREC_MAX + 1) == ADF_LIMIT);
        CHECK(adf_tate_completed(z, chi, s, 20, ADF_REAL_PREC_MAX + 1) == ADF_LIMIT);
        chi->n = 1;
        adf_char_clear(chi); acb_clear(z); acb_clear(s);
    }
    printf("INV: 4 precondition aborts\n");
#endif
}

int main(int argc, char **argv)
{
    only = argc > 1 ? argv[1] : NULL;
    if (section("forbidden")) forbidden();
    if (section("statuses")) statuses();
    if (section("overlap")) overlap();
    if (section("widths")) widths();
    vectors();
    if (section("jbound")) jbound();
    if (section("debug")) debug();
    printf("tate_continue: %lu checks\n", checks);
    flint_cleanup();
    return 0;
}
