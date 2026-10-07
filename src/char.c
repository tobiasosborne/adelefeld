/* Slice a, docs/api-3c.md 1-4, P3/P4. Only s is owned; groups are call-local.
   FLINT declarations: /usr/include/flint/dirichlet.h:51,68-70,90-91,110-136,139,162.
   The universal pairing identification remains source pending, as api-3c P3.5 states. */
#include "adelefeld/char.h"
#include "invariants.h"
#include <flint/dirichlet.h>
#include <flint/ulong_extras.h>
#include <string.h>

#ifdef ADF_CHECK_INVARIANTS
static ADF_INV_NOINLINE void char_inv(const adf_char_struct *x, const char *fn, const char *arg)
{
    if (!adf_char_is_canonical(x)) adf_inv_fail(fn, arg, "adf_char");
}
#define ADF_INV_CHAR(x) char_inv(x, __func__, #x)
#define CHAR_INDEPENDENT(z, x) \
    do { if ((z) == (x)->s) adf_inv_fail(__func__, #z, "independent acb output/input"); } while (0)
#else
#define ADF_INV_CHAR(x) ((void) 0)
#define CHAR_INDEPENDENT(z, x) ((void) 0)
#endif

/* api-3c 1; conventions 5.13: principal initialization, exact lifecycle. */
void adf_char_init(adf_char_t x)
{
    x->q = 1; x->n = 1; x->parity = 0; acb_init(x->s);
}
void adf_char_clear(adf_char_t x) { ADF_INV_CHAR(x); acb_clear(x->s); }
void adf_char_set(adf_char_t y, const adf_char_t x)
{
    ADF_INV_CHAR(x); ADF_INV_CHAR(y);
    y->q = x->q; y->n = x->n; y->parity = x->parity; acb_set(y->s, x->s);
}
void adf_char_swap(adf_char_t x, adf_char_t y)
{
    ulong t; int e;
    ADF_INV_CHAR(x); ADF_INV_CHAR(y);
    t = x->q; x->q = y->q; y->q = t;
    t = x->n; x->n = y->n; y->n = t;
    e = x->parity; x->parity = y->parity; y->parity = e; acb_swap(x->s, y->s);
}
int adf_char_is_canonical(const adf_char_t x)
{
    dirichlet_group_t G; dirichlet_char_t c; int ok;
    if (x->q == 0 || x->n == 0 || x->n > x->q || n_gcd(x->n, x->q) != 1 ||
        (x->parity != 0 && x->parity != 1) || !acb_is_finite(x->s)) return 0;
    if (x->q == 1) return x->n == 1 && x->parity == 0;
    /* HEADER-FINDING: api-3c 1 forbids abort and resource-false, but supplies no error result.
       Until group-init failure semantics are sourced, fail-stop rather than report predicate false.
       D1 is NOT tested here. */
    if (!dirichlet_group_init(G, x->q)) flint_abort();
    dirichlet_char_init(c, G); dirichlet_char_log(c, G, x->n);
    ok = dirichlet_conductor_char(G, c) == x->q && dirichlet_parity_char(G, c) == x->parity;
    dirichlet_char_clear(c); dirichlet_group_clear(G); return ok;
}
int adf_char_identical(const adf_char_t x, const adf_char_t y)
{
    ADF_INV_CHAR(x); ADF_INV_CHAR(y);
    return x->q == y->q && x->n == y->n && x->parity == y->parity && acb_equal(x->s, y->s);
}

/* P3.1-3: conductor and unique inducing label, via FLINT lower. Header:110-136.
   Cheap DOMAIN checks precede D1; no output field changes before all setup completes. */
int adf_char_set_conrey_acb(adf_char_t x, ulong q, ulong n, const acb_t s)
{
    dirichlet_group_t G, H; dirichlet_char_t c, d;
    ulong C, label; int e;
    if (q == 0 || !acb_is_finite(s) || n_gcd(n, q) != 1) return ADF_DOMAIN;
    if (q > ADF_CHAR_MOD_MAX) return ADF_LIMIT;
    ADF_INV_CHAR(x); CHAR_INDEPENDENT(s, x);
    if (q == 1) { C = 1; label = 1; e = 0; }
    else {
        if (!dirichlet_group_init(G, q)) return ADF_UNSUPPORTED;
        dirichlet_char_init(c, G); dirichlet_char_log(c, G, n % q);
        C = dirichlet_conductor_char(G, c);
        if (C == 1) { label = 1; e = 0; }
        else {
            if (!dirichlet_group_init(H, C)) {
                dirichlet_char_clear(c); dirichlet_group_clear(G); return ADF_UNSUPPORTED;
            }
            dirichlet_char_init(d, H); dirichlet_char_lower(d, H, c, G);
            label = dirichlet_char_exp(H, d); e = dirichlet_parity_char(H, d);
            dirichlet_char_clear(d); dirichlet_group_clear(H);
        }
        dirichlet_char_clear(c); dirichlet_group_clear(G);
    }
    /* Exact acb copy has no recoverable status. Commit after the final check. */
    acb_set(x->s, s); x->q = C; x->n = label; x->parity = e; return ADF_OK;
}
int adf_char_set_conrey(adf_char_t x, ulong q, ulong n)
{
    acb_t s; int st;
    /* D1 before even acb initialization, as well as before group setup. */
    if (q == 0 || n_gcd(n, q) != 1) return ADF_DOMAIN;
    if (q > ADF_CHAR_MOD_MAX) return ADF_LIMIT;
    acb_init(s); st = adf_char_set_conrey_acb(x, q, n, s); acb_clear(s); return st;
}
int adf_char_set_s(adf_char_t x, const acb_t s)
{
    if (!acb_is_finite(s)) return ADF_DOMAIN;
    ADF_INV_CHAR(x); CHAR_INDEPENDENT(s, x); acb_set(x->s, s); return ADF_OK;
}
ulong adf_char_get_conductor(const adf_char_t x) { ADF_INV_CHAR(x); return x->q; }
ulong adf_char_get_label(const adf_char_t x) { ADF_INV_CHAR(x); return x->n; }
int adf_char_get_parity(const adf_char_t x) { ADF_INV_CHAR(x); return x->parity; }
void adf_char_get_s(acb_t s, const adf_char_t x)
{
    ADF_INV_CHAR(x); CHAR_INDEPENDENT(s, x); acb_set(s, x->s);
}
/* api-3c 2: order is a property of the character, not the group exponent. */
int adf_char_get_order(ulong *order, const adf_char_t x)
{
    dirichlet_group_t G; dirichlet_char_t c; ulong value;
    if (x->q > ADF_CHAR_MOD_MAX) return ADF_LIMIT;
    ADF_INV_CHAR(x);
    if (x->q == 1) { *order = 1; return ADF_OK; }
    if (!dirichlet_group_init(G, x->q)) return ADF_UNSUPPORTED;
    dirichlet_char_init(c, G); dirichlet_char_log(c, G, x->n);
    value = dirichlet_order_char(G, c);
    dirichlet_char_clear(c); dirichlet_group_clear(G); *order = value; return ADF_OK;
}

/* api-3c 3/F1; header:51 "exponent = largest order in G", :139 NULL, :162 chi.
   k/G->expo is corroborated by the independent oracle and the 3.0.1 adapter.
   [source pending: FLINT 3.0.1 dirichlet_chi implementation under refs/]. */
static void char_phase(int *zero, fmpq_t theta, const dirichlet_group_t G,
                       const dirichlet_char_t c, ulong a)
{
    ulong k = dirichlet_chi(G, c, a);
    *zero = k == DIRICHLET_CHI_NULL;
    if (*zero) fmpq_zero(theta); else fmpq_set_ui(theta, k, G->expo);
}
int adf_char_chi_phase(int *is_zero, fmpq_t theta, const adf_char_t chi, const fmpz_t a)
{
    dirichlet_group_t G; dirichlet_char_t c; fmpq_t t; int zero;
    if (chi->q > ADF_CHAR_MOD_MAX) return ADF_LIMIT;
    ADF_INV_CHAR(chi);
    if (chi->q == 1) { fmpq_zero(theta); *is_zero = 0; return ADF_OK; }
    if (!dirichlet_group_init(G, chi->q)) return ADF_UNSUPPORTED;
    dirichlet_char_init(c, G); dirichlet_char_log(c, G, chi->n); fmpq_init(t);
    char_phase(&zero, t, G, c, fmpz_fdiv_ui(a, chi->q));
    dirichlet_char_clear(c); dirichlet_group_clear(G);
    fmpq_swap(theta, t); *is_zero = zero; fmpq_clear(t); return ADF_OK;
}
int adf_char_chi(acb_t z, const adf_char_t chi, const fmpz_t a, slong prec)
{
    fmpq_t t; acb_t out; int zero, st;
    if (prec > ADF_REAL_PREC_MAX || chi->q > ADF_CHAR_MOD_MAX) return ADF_LIMIT;
    ADF_INV_CHAR(chi); CHAR_INDEPENDENT(z, chi);
    fmpq_init(t); acb_init(out); st = adf_char_chi_phase(&zero, t, chi, a);
    if (st == ADF_OK) {
        if (zero) acb_zero(out); else st = adf_phase_get_acb(out, t, prec);
        if (st == ADF_OK && !acb_is_finite(out)) st = ADF_NOT_DETERMINED;
        if (st == ADF_OK) acb_swap(z, out);
    }
    fmpq_clear(t); acb_clear(out); return st;
}

/* P4.1-3: exact endpoint accumulation and Q1 final rounding, as src/psi.c:106-132.
   Correct rounding: refs/src/flint-3.0.1/arf.rst:24-35. General mag conversions may
   add extra ulps (mag.rst:6-17); directly construct RU30 plus its successor. */
static void char_round(arb_t z, const arf_t lo, const arf_t hi, slong w)
{
    arf_t mid, r, t; fmpz_t man, exp; ulong v;
    arf_init(mid); arf_init(r); arf_init(t); fmpz_init(man); fmpz_init(exp);
    arf_add(mid, lo, hi, ARF_PREC_EXACT, ARF_RND_NEAR); arf_mul_2exp_si(mid, mid, -1);
    arf_set_round(arb_midref(z), mid, w, ARF_RND_NEAR);
    arf_sub(r, arb_midref(z), lo, ARF_PREC_EXACT, ARF_RND_NEAR);
    arf_sub(t, hi, arb_midref(z), ARF_PREC_EXACT, ARF_RND_NEAR);
    if (arf_cmp(t, r) > 0) arf_set(r, t);
    if (arf_is_zero(r)) mag_zero(arb_radref(z));
    else {
        arf_set_round(r, r, 30, ARF_RND_CEIL); arf_get_fmpz_2exp(man, exp, r);
        fmpz_mul_2exp(man, man, 30-fmpz_bits(man)); v = fmpz_get_ui(man)+1;
        fmpz_set(MAG_EXPREF(arb_radref(z)), ARF_EXPREF(r));
        if (v == (UWORD(1) << 30)) {
            v >>= 1; fmpz_add_ui(MAG_EXPREF(arb_radref(z)), MAG_EXPREF(arb_radref(z)), 1);
        }
        MAG_MAN(arb_radref(z)) = v;
    }
    arf_clear(mid); arf_clear(r); arf_clear(t); fmpz_clear(man); fmpz_clear(exp);
}
static slong char_work(ulong C, slong prec)
{
    return FLINT_MIN(ADF_REAL_PREC_MAX, FLINT_MAX(prec, 2)+FLINT_CLOG2(C)+8);
}
static int char_radii(const acb_t z, ulong C, slong w, ulong factor)
{
    arf_t r, bound; int ok;
    if (!acb_is_finite(z)) return 0;
    arf_init(r); arf_init(bound); arf_set_ui(bound, factor*C); arf_mul_2exp_si(bound, bound, -w);
    arf_set_mag(r, arb_radref(acb_realref(z))); ok = arf_cmp(r, bound) <= 0;
    arf_set_mag(r, arb_radref(acb_imagref(z))); ok = ok && arf_cmp(r, bound) <= 0;
    arf_clear(r); arf_clear(bound); return ok;
}
/* CV-60, analysis L8:295-327; positive sign in local source acb_dirichlet.rst:358-364.
   P4 adds C exact certified intervals, sharing a single FLINT group throughout the loop. */
int adf_char_gauss_sum(acb_t tau, const adf_char_t chi, slong prec)
{
    dirichlet_group_t G; dirichlet_char_t c; acb_t term, out;
    fmpq_t theta, additive; arf_struct sums[4]; arf_t lo, hi;
    int st = ADF_OK, zero; slong w; ulong a;
    if (prec > ADF_REAL_PREC_MAX || chi->q > ADF_CHAR_MOD_MAX) return ADF_LIMIT;
    ADF_INV_CHAR(chi); CHAR_INDEPENDENT(tau, chi);
    if (chi->q == 1) { acb_one(tau); return ADF_OK; }
    if (!dirichlet_group_init(G, chi->q)) return ADF_UNSUPPORTED;
    dirichlet_char_init(c, G); dirichlet_char_log(c, G, chi->n);
    w = char_work(chi->q, prec); acb_init(term); acb_init(out); fmpq_init(theta); fmpq_init(additive);
    arf_init(lo); arf_init(hi); for (int j = 0; j < 4; j++) arf_init(sums+j);
    for (a = 0; a < chi->q; a++) {
        char_phase(&zero, theta, G, c, a); if (zero) continue;
        fmpq_set_ui(additive, a, chi->q); fmpq_add(theta, theta, additive);
        /* Each angle is in [0,1); their sum is in [0,2). Reduce exactly. */
        if (fmpq_cmp_si(theta, 1) >= 0) fmpq_sub_ui(theta, theta, 1);
        st = adf_phase_get_acb(term, theta, w); if (st != ADF_OK) break;
        for (int j = 0; j < 2; j++) {
            const arb_struct *part = j ? acb_imagref(term) : acb_realref(term);
            arb_get_interval_arf(lo, hi, part, ARF_PREC_EXACT);
            arf_add(sums+2*j, sums+2*j, lo, ARF_PREC_EXACT, ARF_RND_NEAR);
            arf_add(sums+2*j+1, sums+2*j+1, hi, ARF_PREC_EXACT, ARF_RND_NEAR);
        }
    }
    if (st == ADF_OK) {
        char_round(acb_realref(out), sums, sums+1, w);
        char_round(acb_imagref(out), sums+2, sums+3, w);
        if (!char_radii(out, chi->q, w, 8)) st = ADF_NOT_DETERMINED;
        else acb_swap(tau, out);
    }
    for (int j = 0; j < 4; j++) arf_clear(sums+j);
    arf_clear(lo); arf_clear(hi); fmpq_clear(theta); fmpq_clear(additive); acb_clear(term); acb_clear(out);
    dirichlet_char_clear(c); dirichlet_group_clear(G); return st;
}
/* P4.4, analysis P13:564-580. Sqrt: refs/src/flint-3.0.1/arb.rst:951-953;
   division/rotation: acb.rst:449-451,519-523. No pending signed quadratic shortcut (P13:580-584). */
int adf_char_root_number(acb_t W, const adf_char_t chi, slong prec)
{
    acb_t out; arb_t root; int st; slong w;
    if (prec > ADF_REAL_PREC_MAX || chi->q > ADF_CHAR_MOD_MAX) return ADF_LIMIT;
    ADF_INV_CHAR(chi); CHAR_INDEPENDENT(W, chi);
    if (chi->q == 1) { acb_one(W); return ADF_OK; }
    acb_init(out); arb_init(root); w = char_work(chi->q, prec);
    st = adf_char_gauss_sum(out, chi, prec);
    if (st == ADF_OK) {
        arb_sqrt_ui(root, chi->q, w); acb_div_arb(out, out, root, w);
        if (chi->parity) acb_div_onei(out, out);
        if (!char_radii(out, chi->q, w, 32)) st = ADF_NOT_DETERMINED;
        else acb_swap(W, out);
    }
    acb_clear(out); arb_clear(root); return st;
}

/* Slice b, api-3c P1/F2: residues of global units in c U(N) are exactly the units a mod C
   with a=c mod gcd(C,N). Scanning those residues avoids any incompatible choice of lift. */
static int char_unit(acb_t z, const adf_char_t chi, const adf_ucoset_t u, slong prec, int strict)
{
    dirichlet_group_t G; dirichlet_char_t c; ulong C = chi->q, g, residue, a;
    fmpq_t theta, distance, complement; fmpq best[4]; arf_struct upper[4];
    arf_t lo, hi; acb_t point, out; int zero, first = 1, st = ADF_OK;
    slong p = FLINT_MAX(prec, 2), work;
    const ulong quarters[] = {0, 2, 1, 3};
    if (prec > ADF_REAL_PREC_MAX || C > ADF_CHAR_MOD_MAX) return ADF_LIMIT;
    ADF_INV_CHAR(chi); CHAR_INDEPENDENT(z, chi);
#ifdef ADF_CHECK_INVARIANTS
    if (!adf_ucoset_is_canonical(u)) adf_inv_fail(__func__, "u", "adf_ucoset");
#endif
    if (fmpz_is_zero(u->N) || fmpz_fdiv_ui(u->N, C) == 0)
        return adf_char_chi(z, chi, u->c, prec);
    if (strict) return ADF_NOT_DETERMINED;
    g = n_gcd(fmpz_fdiv_ui(u->N, C), C); residue = fmpz_fdiv_ui(u->c, g);
    if (!dirichlet_group_init(G, C)) return ADF_UNSUPPORTED;
    dirichlet_char_init(c, G); dirichlet_char_log(c, G, chi->n);
    fmpq_init(theta); fmpq_init(distance); fmpq_init(complement);
    for (int j = 0; j < 4; j++) { fmpq_init(best+j); arf_init(upper+j); }
    arf_init(lo); arf_init(hi); acb_init(point); acb_init(out);
    /* P2: minimize circle distance separately to all four cardinal phases. Each extremum
       is attained in this finite set. No chi(c) zero branch applies to a unit coset. */
    for (a = 1; a < C; a++) if (a % g == residue && n_gcd(a, C) == 1) {
        char_phase(&zero, theta, G, c, a);
        for (int j = 0; j < 4; j++) {
            fmpq_set_ui(distance, quarters[j], 4); fmpq_sub(distance, distance, theta);
            fmpz_mod(fmpq_numref(distance), fmpq_numref(distance), fmpq_denref(distance));
            fmpq_one(complement); fmpq_sub(complement, complement, distance);
            if (fmpq_cmp(complement, distance) < 0) fmpq_set(distance, complement);
            if (first || fmpq_cmp(distance, best+j) < 0) fmpq_set(best+j, distance);
        }
        first = 0;
    }
    if (first) st = ADF_NOT_DETERMINED;
    /* Q4 certificate, api-3c 3: refine phase's cosine until width <=2^-p, then clip to
       [-1,1]. Reuse slice a's Q1 radius kernel rather than introduce another rounder. */
    for (int j = 0; st == ADF_OK && j < 4; j++) {
        work = FLINT_MIN(p+32, ADF_REAL_PREC_MAX);
        for (;;) {
            st = adf_phase_get_acb(point, best+j, work);
            if (st != ADF_OK) break;
            if (acb_is_finite(point) && mag_cmp_2exp_si(arb_radref(acb_realref(point)), -p-1) <= 0) {
                arb_get_interval_arf(lo, upper+j, acb_realref(point), ARF_PREC_EXACT);
                if (arf_cmp_si(upper+j, 1) > 0) arf_one(upper+j);
                break;
            }
            if (work == ADF_REAL_PREC_MAX) { st = ADF_NOT_DETERMINED; break; }
            work = FLINT_MIN(2*work, ADF_REAL_PREC_MAX);
        }
    }
    if (st == ADF_OK) {
        arf_neg(lo, upper+1); arf_set(hi, upper); char_round(acb_realref(out), lo, hi, p);
        arf_neg(lo, upper+3); arf_set(hi, upper+2); char_round(acb_imagref(out), lo, hi, p);
        if (!acb_is_finite(out)) st = ADF_NOT_DETERMINED;
        else acb_swap(z, out);
    }
    fmpq_clear(theta); fmpq_clear(distance); fmpq_clear(complement);
    for (int j = 0; j < 4; j++) { fmpq_clear(best+j); arf_clear(upper+j); }
    arf_clear(lo); arf_clear(hi); acb_clear(point); acb_clear(out);
    dirichlet_char_clear(c); dirichlet_group_clear(G); return st;
}
int adf_char_eval_ucoset(acb_t z, const adf_char_t chi, const adf_ucoset_t u, slong prec)
{ return char_unit(z, chi, u, prec, 0); }
int adf_char_eval_ucoset_strict(acb_t z, const adf_char_t chi, const adf_ucoset_t u, slong prec)
{ return char_unit(z, chi, u, prec, 1); }

/* api-3c P3.4: inversion negates cyclic exponents. Header:84 states
   "s.t. prod generators[k]^log[k] = number"; :36 stores component phi, :123 rebuilds n.
   [source pending: FLINT pairing identification]. The brief requires this fallback.
   HEADER-FINDING: the fallback costs D(q), not only extended gcd; the void ABI has no
   recoverable setup-failure result. No D1 cutoff is imposed on this exact operation. */
void adf_char_conj(adf_char_t y, const adf_char_t x)
{
    dirichlet_group_t G; dirichlet_char_t c; ulong label = 1;
    ADF_INV_CHAR(x); ADF_INV_CHAR(y);
    if (x->q != 1) {
        if (!dirichlet_group_init(G, x->q)) flint_abort();
        dirichlet_char_init(c, G); dirichlet_char_log(c, G, x->n);
        for (slong j = 0; j < G->num; j++) {
            ulong order = G->P[j].phi.n;
            c->log[j] = c->log[j] == 0 ? 0 : order-c->log[j];
        }
        /* Public char_exp (:116-120) only returns cached n; the underscore function
           (:123) updates it after log edits. Calling the inline would retain the old label. */
        label = _dirichlet_char_exp(c, G);
        dirichlet_char_clear(c); dirichlet_group_clear(G);
    }
    y->q = x->q; y->n = label; y->parity = x->parity; acb_conj(y->s, x->s);
}

/* Slice c: api-3c 3, P1/P2, conventions 5.13. A canonical class has t>0.
   refs/src/flint-3.0.1/arb.rst:1034-1040 and acb.rst:637-643 define the power
   as exp(s log(t)), with integer/half-integer shortcuts. On this positive-real
   input log(t) is real; no conjugation or complex branch choice is introduced. */
#ifdef ADF_CHECK_INVARIANTS
static void char_value_independent(acb_t z, const adf_char_t chi, const arb_t t)
{
    const arb_struct *a = acb_realref(z), *b = acb_imagref(z);
    if (a == t || b == t || a == acb_realref(chi->s) || b == acb_realref(chi->s) ||
        a == acb_imagref(chi->s) || b == acb_imagref(chi->s))
        adf_inv_fail(__func__, "z", "independent acb output/input");
}
#endif
static int char_class(acb_t z, const adf_char_t chi, const adf_idclass_t x, slong prec, int strict)
{
    acb_t unit, power, base, out; int st; slong p = FLINT_MAX(prec, 2);
    if (prec > ADF_REAL_PREC_MAX || chi->q > ADF_CHAR_MOD_MAX) return ADF_LIMIT;
    ADF_INV_CHAR(chi); CHAR_INDEPENDENT(z, chi);
#ifdef ADF_CHECK_INVARIANTS
    if (!adf_idclass_is_canonical(x)) adf_inv_fail(__func__, "x", "adf_idclass");
    char_value_independent(z, chi, x->t);
#endif
    acb_init(unit); acb_init(power); acb_init(base); acb_init(out);
    st = char_unit(unit, chi, &x->u, prec, strict);
    if (st == ADF_OK) {
        if (arb_is_zero(acb_imagref(chi->s))) {
            arb_pow(acb_realref(power), x->t, acb_realref(chi->s), p);
            /* Initialized power has exact zero imaginary part. */
        } else {
            acb_set_arb(base, x->t); acb_pow(power, base, chi->s, p);
        }
        if (!acb_is_finite(power)) st = ADF_NOT_DETERMINED;
        else {
            acb_mul(out, power, unit, p);
            if (!acb_is_finite(out)) st = ADF_NOT_DETERMINED;
            else acb_swap(z, out);
        }
    }
    acb_clear(unit); acb_clear(power); acb_clear(base); acb_clear(out); return st;
}
int adf_char_eval_idclass(acb_t z, const adf_char_t chi, const adf_idclass_t x, slong prec)
{ return char_class(z, chi, x, prec, 0); }
int adf_char_eval_idclass_strict(acb_t z, const adf_char_t chi, const adf_idclass_t x, slong prec)
{ return char_class(z, chi, x, prec, 1); }

/* ideles P15:368-394, conventions 5.7/5.13: conversion supplies BOTH the norm
   |x_inf|/r and sign(x_inf)u. Use it once, then evaluate the canonical class.
   api-3c 3 requires preflight bounds before conversion and transactional outputs. */
static int char_idele(acb_t z, const adf_char_t chi, const adf_idele_t x, slong prec, int strict)
{
    adf_idclass_t c; int st;
    if (prec > ADF_REAL_PREC_MAX || chi->q > ADF_CHAR_MOD_MAX) return ADF_LIMIT;
    ADF_INV_CHAR(chi); CHAR_INDEPENDENT(z, chi);
#ifdef ADF_CHECK_INVARIANTS
    if (!adf_idele_is_canonical(x)) adf_inv_fail(__func__, "x", "adf_idele");
    char_value_independent(z, chi, x->inf);
#endif
    adf_idclass_init(c); st = adf_idclass_set_idele(c, x, prec);
    if (st == ADF_OK) st = char_class(z, chi, c, prec, strict);
    adf_idclass_clear(c); return st;
}
int adf_char_eval_idele(acb_t z, const adf_char_t chi, const adf_idele_t x, slong prec)
{ return char_idele(z, chi, x, prec, 0); }
int adf_char_eval_idele_strict(acb_t z, const adf_char_t chi, const adf_idele_t x, slong prec)
{ return char_idele(z, chi, x, prec, 1); }
