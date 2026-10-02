/* tests/test_text_idele.c: the value form of adf_ucoset, adf_idele and adf_idclass (adf_ucoset_set_str,
   adf_ucoset_get_str, adf_idele_set_str, adf_idele_get_str, adf_idclass_set_str, adf_idclass_get_str) and
   the classification of their texts (lane t-slice1, milestone 2).

   Contract: include/adelefeld/text.h (the block "adf_ucoset, adf_idele, adf_idclass"); docs/conventions.md
   5.6 (residue range 1..N, modulus as written, normal form), 5.7 (predicates), 8.1 to 8.5 (interface,
   alphabet, limits, order of checks), 9.2 (ucoset_v, idele_v, idclass_v), 9.3 (constraints; canonicalisation
   on input), 9.4 (templates), 9.5 (reading: containment and tightness; constrained printing), 9.6 (round
   trips), 9.7 (type of a text), 11.3 (how a C test uses the golden files, prec = 128).
   Oracles: tests/golden/{ucoset,idele,idclass}.tsv, every row; the vector files in tests/ref/vectors/t-slice1/ written by
   lanes/t-slice1/gen_vectors.py with the reference proto/text_grammar.py: golden_parts.jsonl (the exact parts of
   every golden row), text_idele.jsonl (2200 random and mutated texts of the three kinds, with the reference
   status or text, the exact parts, the stored unit, the gap), print_constrained.jsonl (1500 dyadic balls and
   the constrained printing of 9.5). The exact decimal reader of this file is its own, independent of
   src/text.c. Strings are freed with flint_free, which adf_str_free calls (common.h). */

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/fmpq.h>
#include <flint/arb.h>

#include <adelefeld.h>

#include "support/golden.h"
#include "support/jsonl.h"
#include "test_runner.h"

#define GOLDEN_PREC 128

/* a string literal and its length */
#define LIT(s) (s), strlen(s)

/* ================================================================== helpers */

static int
status_of_name(const char * name)
{
    int k;

    for (k = 0; k < ADF_STATUS_COUNT; k++)
        if (strcmp(adf_status_str(k), name) == 0)
            return k;
    return -1;
}

static unsigned char *
hex_decode(const char * hex, size_t hlen, size_t * n)
{
    unsigned char * b = (unsigned char *) malloc(hlen / 2 + 1);
    size_t i;

    for (i = 0; i < hlen / 2; i++)
    {
        unsigned v = 0;
        int j;
        for (j = 0; j < 2; j++)
        {
            char c = hex[2 * i + j];
            v = 16 * v + (unsigned) (c <= '9' ? c - '0' : c - 'a' + 10);
        }
        b[i] = (unsigned char) v;
    }
    *n = hlen / 2;
    b[*n] = 0;
    return b;
}

static void
pow10_fmpq(fmpq_t q, slong e)
{
    fmpz_t t;

    fmpz_init(t);
    fmpz_set_ui(t, 10);
    fmpz_pow_ui(t, t, (ulong) (e < 0 ? -e : e));
    fmpq_set_fmpz(q, t);
    if (e < 0)
        fmpq_inv(q, q);
    fmpz_clear(t);
}

/* q = D 10^E, D a decimal integer string. */
static void
dec_value(fmpq_t q, const char * D, slong E)
{
    fmpz_t d;
    fmpq_t p;

    fmpz_init(d);
    fmpq_init(p);
    ADF_CHECK(fmpz_set_str(d, D, 10) == 0);
    pow10_fmpq(p, E);
    fmpq_set_fmpz(q, d);
    fmpq_mul(q, q, p);
    fmpz_clear(d);
    fmpq_clear(p);
}

/* The exact decimal [-]digits[.digits][(e|E)[+-]digits] at s[*i..n): the test's own reader. */
static int
read_decimal(fmpq_t q, const char * s, size_t n, size_t * i)
{
    size_t j = *i;
    int neg = 0, eneg = 0;
    fmpz_t D;
    slong E = 0, X = 0;
    fmpq_t p;

    if (j < n && s[j] == '-')
    {
        neg = 1;
        j++;
    }
    if (j >= n || s[j] < '0' || s[j] > '9')
        return 0;
    fmpz_init(D);
    while (j < n && s[j] >= '0' && s[j] <= '9')
    {
        fmpz_mul_ui(D, D, 10);
        fmpz_add_ui(D, D, (ulong) (s[j] - '0'));
        j++;
    }
    if (j + 1 < n && s[j] == '.' && s[j + 1] >= '0' && s[j + 1] <= '9')
    {
        j++;
        while (j < n && s[j] >= '0' && s[j] <= '9')
        {
            fmpz_mul_ui(D, D, 10);
            fmpz_add_ui(D, D, (ulong) (s[j] - '0'));
            E--;
            j++;
        }
    }
    if (j < n && (s[j] == 'e' || s[j] == 'E'))
    {
        size_t k = j + 1;
        if (k < n && (s[k] == '+' || s[k] == '-'))
        {
            eneg = s[k] == '-';
            k++;
        }
        if (k < n && s[k] >= '0' && s[k] <= '9')
        {
            while (k < n && s[k] >= '0' && s[k] <= '9')
            {
                X = 10 * X + (s[k] - '0');
                k++;
            }
            E += eneg ? -X : X;
            j = k;
        }
    }
    fmpq_init(p);
    pow10_fmpq(p, E);
    fmpq_set_fmpz(q, D);
    fmpq_mul(q, q, p);
    if (neg)
        fmpq_neg(q, q);
    fmpq_clear(p);
    fmpz_clear(D);
    *i = j;
    return 1;
}

/* A printed real ball "M" or "M +/- R" (conventions 9.5), read exactly. */
static int
read_printed_real(fmpq_t mid, fmpq_t rad, const char * s, size_t n)
{
    size_t i = 0;

    if (!read_decimal(mid, s, n, &i))
        return 0;
    fmpq_zero(rad);
    if (i == n)
        return 1;
    if (i + 5 > n || memcmp(s + i, " +/- ", 5) != 0)
        return 0;
    i += 5;
    if (!read_decimal(rad, s, n, &i))
        return 0;
    return i == n && fmpq_sgn(rad) >= 0;
}

/* n copies of c at dst (a loop: gcc 13 with fortification reports a false -Warray-bounds for memset on a
   pointer into a large malloc block under -fsanitize) */
static void
repeat_char(char * dst, char c, size_t n)
{
    size_t i;

    for (i = 0; i < n; i++)
        dst[i] = c;
}

static int
well_formed(const char * t, size_t len)
{
    size_t i;

    if (t == NULL || t[len] != '\0' || len == 0 || t[0] == ' ' || t[len - 1] == ' ')
        return 0;
    for (i = 0; i < len; i++)
        if ((unsigned char) t[i] < 0x20 || (unsigned char) t[i] > 0x7e)
            return 0;
    return 1;
}

static int
dyadic_bits_at_most(const fmpq_t v, ulong bits)
{
    fmpz_t m;
    int ok;

    if (fmpq_is_zero(v))
        return 1;
    if (fmpz_val2(fmpq_denref(v)) + 1 != fmpz_bits(fmpq_denref(v)))
        return 0;
    fmpz_init(m);
    fmpz_abs(m, fmpq_numref(v));
    fmpz_fdiv_q_2exp(m, m, fmpz_val2(m));
    ok = fmpz_bits(m) <= bits;
    fmpz_clear(m);
    return ok;
}

/* The exactness condition of conventions 9.5 ("Reading") at prec. */
static int
exact_condition(const fmpq_t mid, const fmpq_t rad, slong prec)
{
    slong p = prec < 2 ? 2 : prec;

    return dyadic_bits_at_most(mid, (ulong) p) && dyadic_bits_at_most(rad, 30);
}

static int
arb_is_exactly(const arb_t x, const fmpq_t mid, const fmpq_t rad)
{
    fmpq_t a, b;
    int ok;

    fmpq_init(a);
    fmpq_init(b);
    arf_get_fmpq(a, arb_midref(x));
    mag_get_fmpq(b, arb_radref(x));
    ok = fmpq_equal(a, mid) && fmpq_equal(b, rad);
    fmpq_clear(a);
    fmpq_clear(b);
    return ok;
}

static int
arb_contains_interval(const arb_t x, const fmpq_t mid, const fmpq_t rad)
{
    fmpq_t e;
    int ok;

    fmpq_init(e);
    fmpq_sub(e, mid, rad);
    ok = arb_contains_fmpq(x, e);
    fmpq_add(e, mid, rad);
    ok = ok && arb_contains_fmpq(x, e);
    fmpq_clear(e);
    return ok;
}

/* [pm - pr, pm + pr] contains [mid - rad, mid + rad]. */
static int
interval_contains(const fmpq_t pm, const fmpq_t pr, const fmpq_t mid, const fmpq_t rad)
{
    fmpq_t a, b;
    int ok;

    fmpq_init(a);
    fmpq_init(b);
    fmpq_sub(a, pm, pr);
    fmpq_sub(b, mid, rad);
    ok = fmpq_cmp(a, b) <= 0;
    fmpq_add(a, pm, pr);
    fmpq_add(b, mid, rad);
    ok = ok && fmpq_cmp(a, b) >= 0;
    fmpq_clear(a);
    fmpq_clear(b);
    return ok;
}

/* the interval [mid - rad, mid + rad] satisfies cond: 0 nothing, 1 excludes 0, 2 is positive */
static int
interval_sign_ok(const fmpq_t mid, const fmpq_t rad, int cond)
{
    fmpq_t lo, hi;
    int ok;

    fmpq_init(lo);
    fmpq_init(hi);
    fmpq_sub(lo, mid, rad);
    fmpq_add(hi, mid, rad);
    if (cond == 2)
        ok = fmpq_sgn(lo) > 0;
    else
        ok = fmpq_sgn(lo) > 0 || fmpq_sgn(hi) < 0;
    fmpq_clear(lo);
    fmpq_clear(hi);
    return ok;
}

/* ---- the parts of a record of golden_parts.jsonl and text_idele.jsonl ---- */

static int
record_part(const jsonl_value * rec, size_t k, fmpq_t mid, fmpq_t rad)
{
    jsonl_error_t err;
    const jsonl_value * parts, * p, * md, * me, * rd, * re;
    size_t l;

    if (jsonl_field(rec, "parts", &parts, &err) != 1 || k >= jsonl_size(parts))
        return 0;
    p = jsonl_at(parts, k, &err);
    if (jsonl_field(p, "md", &md, &err) != 1 || jsonl_field(p, "me", &me, &err) != 1
        || jsonl_field(p, "rd", &rd, &err) != 1 || jsonl_field(p, "re", &re, &err) != 1)
        return 0;
    dec_value(mid, jsonl_string(md, &l, &err), atol(jsonl_int_text(me, &err)));
    dec_value(rad, jsonl_string(rd, &l, &err), atol(jsonl_int_text(re, &err)));
    return 1;
}

static const char *
rec_str(const jsonl_value * rec, const char * key, size_t * len)
{
    jsonl_error_t err;
    const jsonl_value * v;

    if (jsonl_field(rec, key, &v, &err) != 1)
        return NULL;
    return jsonl_string(v, len, &err);
}

static int
rec_int(const jsonl_value * rec, const char * key, long * out)
{
    jsonl_error_t err;
    const jsonl_value * v;
    const char * t;

    if (jsonl_field(rec, key, &v, &err) != 1)
        return 0;
    t = jsonl_int_text(v, &err);
    if (t == NULL)
        return 0;
    *out = atol(t);
    return 1;
}

static const jsonl_value *
find_golden(const jsonl_file * f, const char * file, unsigned long line)
{
    jsonl_error_t err;
    size_t i, l;

    for (i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value * rec = jsonl_record(f, i), * fv, * lv;
        if (jsonl_field(rec, "file", &fv, &err) != 1 || jsonl_field(rec, "line", &lv, &err) != 1)
            continue;
        if (strcmp(jsonl_string(fv, &l, &err), file) == 0 && strtoul(jsonl_int_text(lv, &err), NULL, 10) == line)
            return rec;
    }
    return NULL;
}

static jsonl_file *
open_vectors(const char * path)
{
    jsonl_error_t err;
    jsonl_file * f = NULL;

    ADF_CHECK_MSG(jsonl_open(path, &f, &err) == 1, "%s", jsonl_error_message(&err));
    return f;
}

/* ---- sentinels and parse wrappers: on a status other than OK the output is untouched ---- */

static void
set_sentinel_unit(adf_ucoset_t u)
{
    fmpz_t c, N;

    fmpz_init_set_si(c, 5);
    fmpz_init_set_si(N, 18);
    ADF_CHECK(adf_ucoset_set_fmpz2(u, c, N) == ADF_OK);
    fmpz_clear(c);
    fmpz_clear(N);
}

static void
set_sentinel_arb(arb_t r)
{
    arf_set_si(arb_midref(r), 3);
    arf_mul_2exp_si(arb_midref(r), arb_midref(r), -3);
    mag_set_ui_2exp_si(arb_radref(r), 1, -10);
}

static void
set_sentinel_idele(adf_idele_t x)
{
    set_sentinel_arb(x->inf);
    fmpq_set_si(x->r, 7, 5);
    set_sentinel_unit(&x->u);
}

static void
set_sentinel_idclass(adf_idclass_t x)
{
    set_sentinel_arb(x->t);
    set_sentinel_unit(&x->u);
}

static int
parse_ucoset(adf_ucoset_t x, const char * s, size_t n, const adf_text_limits_t * lim, int want, const char * where)
{
    adf_ucoset_t keep;
    int st;

    adf_ucoset_init(keep);
    set_sentinel_unit(x);
    adf_ucoset_set(keep, x);
    st = adf_ucoset_set_str(x, s, n, lim);
    ADF_CHECK_MSG(st == want, "%s: status %d, expected %d", where, st, want);
    if (st != ADF_OK)
        ADF_CHECK_MSG(adf_ucoset_identical(x, keep), "%s: the output changed on status %d", where, st);
    else
        ADF_CHECK_MSG(adf_ucoset_is_canonical(x), "%s: the result is not canonical", where);
    adf_ucoset_clear(keep);
    return st;
}

/* the status is any of the two in want1, want2 (want2 < 0: only want1) */
static int
parse_idele2(adf_idele_t x, const char * s, size_t n, slong prec, const adf_text_limits_t * lim, int want1,
             int want2, const char * where)
{
    adf_idele_t keep;
    int st;

    adf_idele_init(keep);
    set_sentinel_idele(x);
    adf_idele_set(keep, x);
    st = adf_idele_set_str(x, s, n, prec, lim);
    ADF_CHECK_MSG(st == want1 || st == want2, "%s: status %d, expected %d (or %d)", where, st, want1, want2);
    if (st != ADF_OK)
        ADF_CHECK_MSG(adf_idele_identical(x, keep), "%s: the output changed on status %d", where, st);
    else
        ADF_CHECK_MSG(adf_idele_is_canonical(x), "%s: the result is not canonical", where);
    adf_idele_clear(keep);
    return st;
}

static int
parse_idele(adf_idele_t x, const char * s, size_t n, slong prec, const adf_text_limits_t * lim, int want,
            const char * where)
{
    return parse_idele2(x, s, n, prec, lim, want, -1, where);
}

static int
parse_idclass2(adf_idclass_t x, const char * s, size_t n, slong prec, const adf_text_limits_t * lim, int want1,
               int want2, const char * where)
{
    adf_idclass_t keep;
    int st;

    adf_idclass_init(keep);
    set_sentinel_idclass(x);
    adf_idclass_set(keep, x);
    st = adf_idclass_set_str(x, s, n, prec, lim);
    ADF_CHECK_MSG(st == want1 || st == want2, "%s: status %d, expected %d (or %d)", where, st, want1, want2);
    if (st != ADF_OK)
        ADF_CHECK_MSG(adf_idclass_identical(x, keep), "%s: the output changed on status %d", where, st);
    else
        ADF_CHECK_MSG(adf_idclass_is_canonical(x), "%s: the result is not canonical", where);
    adf_idclass_clear(keep);
    return st;
}

static int
parse_idclass(adf_idclass_t x, const char * s, size_t n, slong prec, const adf_text_limits_t * lim, int want,
              const char * where)
{
    return parse_idclass2(x, s, n, prec, lim, want, -1, where);
}

/* the stored unit is (c, N) as decimal strings */
static void
check_unit(const adf_ucoset_t u, const char * c, const char * N, const char * where)
{
    fmpz_t cc, NN, gc, gN;

    fmpz_init(cc);
    fmpz_init(NN);
    fmpz_init(gc);
    fmpz_init(gN);
    ADF_CHECK(fmpz_set_str(cc, c, 10) == 0 && fmpz_set_str(NN, N, 10) == 0);
    adf_ucoset_get_fmpz2(gc, gN, u);
    ADF_CHECK_MSG(fmpz_equal(gc, cc) && fmpz_equal(gN, NN), "%s: the stored unit is not (%s, %s)", where, c, N);
    fmpz_clear(cc);
    fmpz_clear(NN);
    fmpz_clear(gc);
    fmpz_clear(gN);
}

/* split "(R ; F)" or "<R ; F>": the real part is text[1 .. first " ; "), the rest text[k + 3 .. n - 1) */
static int
split_form(const char * t, size_t n, size_t * re, size_t * fb)
{
    size_t k;

    if (n < 6)
        return 0;
    for (k = 1; k + 3 <= n; k++)
        if (memcmp(t + k, " ; ", 3) == 0)
            break;
    if (k + 3 > n)
        return 0;
    *re = k;
    *fb = k + 3;
    return 1;
}

/* Check the printed text P (length n) of a value whose exact real part is (mid, rad) and whose expected
   canonical text is E (length en): well formed, the tail equals the expected tail, the printed real interval
   contains the exact one and satisfies cond, and for an exactly read value the text is the expected one. */
static void
check_printed(const char * P, size_t n, const fmpq_t mid, const fmpq_t rad, int cond, int exact, const char * E,
              size_t en, const char * where)
{
    size_t pre, pfb, ere, efb;
    fmpq_t pm, pr;

    fmpq_init(pm);
    fmpq_init(pr);
    ADF_CHECK_MSG(well_formed(P, n), "%s: the printed text is not well formed", where);
    if (split_form(P, n, &pre, &pfb) && split_form(E, en, &ere, &efb))
    {
        ADF_CHECK_MSG(n - pfb == en - efb && memcmp(P + pfb, E + efb, n - pfb) == 0,
                      "%s: tail \"%s\", expected \"%s\"", where, P, E);
        ADF_CHECK_MSG(P[0] == E[0], "%s: the opening bracket differs", where);
        ADF_CHECK_MSG(read_printed_real(pm, pr, P + 1, pre - 1), "%s: the printed part of \"%s\" is not a real ball",
                      where, P);
        ADF_CHECK_MSG(interval_contains(pm, pr, mid, rad), "%s: the printed \"%s\" does not contain the exact ball",
                      where, P);
        ADF_CHECK_MSG(interval_sign_ok(pm, pr, cond), "%s: the printed \"%s\" violates the sign condition", where,
                      P);
        if (exact)
            ADF_CHECK_MSG(n == en && memcmp(P, E, n) == 0, "%s: printed \"%s\", expected \"%s\"", where, P, E);
    }
    else
        ADF_CHECK_MSG(0, "%s: \"%s\" or \"%s\" is not of the form (R ; F)", where, P, E);
    fmpq_clear(pm);
    fmpq_clear(pr);
}

static int
arb_printable(const arb_t x)
{
    fmpz_t lim;
    int ok = 1;

    fmpz_init_set_ui(lim, ADF_PRINT_EXP_MAX);
    if (!arf_is_zero(arb_midref(x)) && fmpz_cmpabs(ARF_EXPREF(arb_midref(x)), lim) > 0)
        ok = 0;
    if (!mag_is_zero(arb_radref(x)) && fmpz_cmpabs(MAG_EXPREF(arb_radref(x)), lim) > 0)
        ok = 0;
    fmpz_clear(lim);
    return ok;
}

/* The stored value of a parsed idele or class against the exact parts of the reference. */
static void
check_real_value(const arb_t x, const fmpq_t mid, const fmpq_t rad, slong prec, int cond, const char * where)
{
    ADF_CHECK_MSG(arb_is_finite(x), "%s: not finite", where);
    ADF_CHECK_MSG(arb_contains_interval(x, mid, rad), "%s: the arb does not contain the exact ball", where);
    ADF_CHECK_MSG(cond == 2 ? arb_is_positive(x) : arb_is_nonzero(x), "%s: the arb violates the sign condition",
                  where);
    if (exact_condition(mid, rad, prec))
        ADF_CHECK_MSG(arb_is_exactly(x, mid, rad), "%s: the arb is not exactly the dyadic ball", where);
}

/* A valid record read as an idele (kind 1) or class (kind 2) at prec, with the gap rule of
   lanes/t-slice1/gen_vectors.py: gap <= p - 1 requires OK; otherwise OK or NOT_DETERMINED. When the record is
   the reference's (golden) one, E is its canonical text. */
static void
check_valid_record(const jsonl_value * rec, int kind, const char * s, size_t n, const char * E, size_t en,
                   slong prec, const char * where)
{
    slong p = prec < 2 ? 2 : prec;
    long gap = 0;
    fmpq_t mid, rad, r;
    size_t l;
    const char * cs, * Ns, * rs;
    int st, cond = kind == 1 ? 1 : 2, exact;

    fmpq_init(mid);
    fmpq_init(rad);
    fmpq_init(r);
    ADF_CHECK_MSG(record_part(rec, 0, mid, rad), "%s: no exact parts", where);
    ADF_CHECK_MSG(rec_int(rec, "gap", &gap), "%s: no gap", where);
    cs = rec_str(rec, "c", &l);
    Ns = rec_str(rec, "N", &l);
    exact = exact_condition(mid, rad, prec);
    if (kind == 1)
    {
        adf_idele_t x;
        fmpq_t got;

        adf_idele_init(x);
        rs = rec_str(rec, "r", &l);
        ADF_CHECK(rs != NULL && fmpq_set_str(r, rs, 10) == 0);
        st = parse_idele2(x, s, n, prec, NULL, ADF_OK, gap <= p - 1 ? -1 : ADF_NOT_DETERMINED, where);
        if (st == ADF_OK)
        {
            char * t;
            size_t len;

            fmpq_init(got);
            adf_idele_content(got, x);
            ADF_CHECK_MSG(fmpq_equal(got, r), "%s: the content differs", where);
            fmpq_clear(got);
            if (cs != NULL && Ns != NULL)
                check_unit(&x->u, cs, Ns, where);
            check_real_value(x->inf, mid, rad, prec, cond, where);
            t = adf_idele_get_str(&len, x, ADF_DIGITS_DEFAULT);
            if (E != NULL)
                check_printed(t, len, mid, rad, cond, exact, E, en, where);
            flint_free(t);
        }
        adf_idele_clear(x);
    }
    else
    {
        adf_idclass_t x;

        adf_idclass_init(x);
        st = parse_idclass2(x, s, n, prec, NULL, ADF_OK, gap <= p - 1 ? -1 : ADF_NOT_DETERMINED, where);
        if (st == ADF_OK)
        {
            char * t;
            size_t len;

            if (cs != NULL && Ns != NULL)
                check_unit(&x->u, cs, Ns, where);
            check_real_value(x->t, mid, rad, prec, cond, where);
            t = adf_idclass_get_str(&len, x, ADF_DIGITS_DEFAULT);
            if (E != NULL)
                check_printed(t, len, mid, rad, cond, exact, E, en, where);
            flint_free(t);
        }
        adf_idclass_clear(x);
    }
    fmpq_clear(mid);
    fmpq_clear(rad);
    fmpq_clear(r);
}

/* ================================================================== ucoset: golden and hand cases */

ADF_TEST(every_row_of_the_golden_file_ucoset)
{
    golden_error_t err;
    golden_file * f = NULL;
    jsonl_file * parts = open_vectors("tests/ref/vectors/t-slice1/golden_parts.jsonl");
    size_t i, rows = 0;

    ADF_CHECK_MSG(golden_open("tests/golden/ucoset.tsv", GOLDEN_DEFAULT_MAX_INPUT, &f, &err) == 1, "%s",
                  golden_error_message(&err));
    if (f == NULL || parts == NULL)
        return;
    for (i = 0; i < golden_count(f); i++)
    {
        const golden_record * r = golden_record_at(f, i);
        adf_ucoset_t x;
        char where[64];

        snprintf(where, sizeof(where), "ucoset.tsv line %lu", r->line);
        adf_ucoset_init(x);
        if (r->is_status)
            parse_ucoset(x, r->input, r->input_len, NULL, status_of_name(r->status), where);
        else if (parse_ucoset(x, r->input, r->input_len, NULL, ADF_OK, where) == ADF_OK)
        {
            const jsonl_value * rec = find_golden(parts, "ucoset", r->line);
            size_t len, l;
            char * t;
            const char * cs, * Ns;
            adf_text_kind kind;

            ADF_CHECK_MSG(rec != NULL, "%s: no exact parts", where);
            if (rec != NULL)
            {
                cs = rec_str(rec, "c", &l);
                Ns = rec_str(rec, "N", &l);
                if (cs != NULL && Ns != NULL)
                    check_unit(x, cs, Ns, where);
            }
            t = adf_ucoset_get_str(&len, x);
            ADF_CHECK_MSG(well_formed(t, len) && len == r->expected_len && memcmp(t, r->expected, len) == 0,
                          "%s: printed \"%s\", expected \"%s\"", where, t, r->expected);
            ADF_CHECK_MSG(adf_text_classify(&kind, r->input, r->input_len, NULL) == ADF_OK
                              && kind == ADF_TEXT_UCOSET,
                          "%s: not classified as a unit coset", where);
            /* the printed text is a fixed point: the same set, the normal form */
            {
                adf_ucoset_t y;

                adf_ucoset_init(y);
                ADF_CHECK(adf_ucoset_set_str(y, t, len, NULL) == ADF_OK);
                ADF_CHECK_MSG(adf_ucoset_equal_set(x, y) && adf_ucoset_is_normal(y), "%s: round trip", where);
                adf_ucoset_clear(y);
            }
            flint_free(t);
        }
        adf_ucoset_clear(x);
        rows++;
    }
    ADF_CHECK_MSG(rows == 38, "ucoset.tsv has %zu rows", rows);
    golden_close(f);
    jsonl_close(parts);
}

ADF_TEST(ucoset_stored_form_and_printing)
{
    adf_ucoset_t x;
    size_t len;
    char * t;

    adf_ucoset_init(x);
    /* the modulus stays as written (CV-17), the residue is reduced into 1..N */
    ADF_CHECK(adf_ucoset_set_str(x, LIT("[5 mod 6]"), NULL) == ADF_OK);
    check_unit(x, "5", "6", "[5 mod 6]");
    t = adf_ucoset_get_str(&len, x);
    ADF_CHECK(len == 9 && strcmp(t, "[2 mod 3]") == 0);
    flint_free(t);
    ADF_CHECK(adf_ucoset_set_str(x, LIT("[-7 mod 5]"), NULL) == ADF_OK);
    check_unit(x, "3", "5", "[-7 mod 5]");
    ADF_CHECK(adf_ucoset_set_str(x, LIT("[0 mod 1]"), NULL) == ADF_OK);
    check_unit(x, "1", "1", "[0 mod 1]");
    ADF_CHECK(adf_ucoset_set_str(x, LIT("[1 mod 0]"), NULL) == ADF_OK);
    check_unit(x, "1", "0", "[1 mod 0]");
    ADF_CHECK(adf_ucoset_is_exact(x));
    ADF_CHECK(adf_ucoset_set_str(x, LIT("[-1 mod 0]"), NULL) == ADF_OK);
    check_unit(x, "-1", "0", "[-1 mod 0]");
    t = adf_ucoset_get_str(&len, x);
    ADF_CHECK(len == 4 && strcmp(t, "[-1]") == 0);
    flint_free(t);
    /* init prints [1]; a constructed non-normal coset prints in normal form */
    adf_ucoset_init(x);
    t = adf_ucoset_get_str(&len, x);
    ADF_CHECK(len == 3 && strcmp(t, "[1]") == 0);
    flint_free(t);
    {
        fmpz_t c, N;

        fmpz_init_set_si(c, 11);
        fmpz_init_set_si(N, 30);
        ADF_CHECK(adf_ucoset_set_fmpz2(x, c, N) == ADF_OK);
        t = adf_ucoset_get_str(&len, x);
        ADF_CHECK_MSG(len == 11 && strcmp(t, "[11 mod 15]") == 0, "got %s", t);
        flint_free(t);
        fmpz_clear(c);
        fmpz_clear(N);
    }
    /* s may be NULL only when len = 0: then PARSE */
    parse_ucoset(x, NULL, 0, NULL, ADF_PARSE, "NULL, 0");
    parse_ucoset(x, LIT(""), NULL, ADF_PARSE, "empty");
    adf_ucoset_clear(x);
}

/* ================================================================== idele and class: golden */

ADF_TEST(every_row_of_the_golden_file_idele)
{
    golden_error_t err;
    golden_file * f = NULL;
    jsonl_file * parts = open_vectors("tests/ref/vectors/t-slice1/golden_parts.jsonl");
    size_t i, rows = 0;

    ADF_CHECK_MSG(golden_open("tests/golden/idele.tsv", GOLDEN_DEFAULT_MAX_INPUT, &f, &err) == 1, "%s",
                  golden_error_message(&err));
    if (f == NULL || parts == NULL)
        return;
    for (i = 0; i < golden_count(f); i++)
    {
        const golden_record * r = golden_record_at(f, i);
        adf_idele_t x;
        char where[64];

        snprintf(where, sizeof(where), "idele.tsv line %lu", r->line);
        adf_idele_init(x);
        if (r->is_status)
            parse_idele(x, r->input, r->input_len, GOLDEN_PREC, NULL, status_of_name(r->status), where);
        else
        {
            const jsonl_value * rec = find_golden(parts, "idele", r->line);
            adf_text_kind kind;

            ADF_CHECK_MSG(rec != NULL, "%s: no exact parts", where);
            if (rec != NULL)
                check_valid_record(rec, 1, r->input, r->input_len, r->expected, r->expected_len, GOLDEN_PREC,
                                   where);
            ADF_CHECK_MSG(adf_text_classify(&kind, r->input, r->input_len, NULL) == ADF_OK && kind == ADF_TEXT_IDELE,
                          "%s: not classified as an idele", where);
        }
        adf_idele_clear(x);
        rows++;
    }
    ADF_CHECK_MSG(rows == 24, "idele.tsv has %zu rows", rows);
    golden_close(f);
    jsonl_close(parts);
}

ADF_TEST(every_row_of_the_golden_file_idclass)
{
    golden_error_t err;
    golden_file * f = NULL;
    jsonl_file * parts = open_vectors("tests/ref/vectors/t-slice1/golden_parts.jsonl");
    size_t i, rows = 0;

    ADF_CHECK_MSG(golden_open("tests/golden/idclass.tsv", GOLDEN_DEFAULT_MAX_INPUT, &f, &err) == 1, "%s",
                  golden_error_message(&err));
    if (f == NULL || parts == NULL)
        return;
    for (i = 0; i < golden_count(f); i++)
    {
        const golden_record * r = golden_record_at(f, i);
        adf_idclass_t x;
        char where[64];

        snprintf(where, sizeof(where), "idclass.tsv line %lu", r->line);
        adf_idclass_init(x);
        if (r->is_status)
            parse_idclass(x, r->input, r->input_len, GOLDEN_PREC, NULL, status_of_name(r->status), where);
        else
        {
            const jsonl_value * rec = find_golden(parts, "idclass", r->line);
            adf_text_kind kind;

            ADF_CHECK_MSG(rec != NULL, "%s: no exact parts", where);
            if (rec != NULL)
                check_valid_record(rec, 2, r->input, r->input_len, r->expected, r->expected_len, GOLDEN_PREC,
                                   where);
            ADF_CHECK_MSG(adf_text_classify(&kind, r->input, r->input_len, NULL) == ADF_OK
                              && kind == ADF_TEXT_IDCLASS,
                          "%s: not classified as a class", where);
        }
        adf_idclass_clear(x);
        rows++;
    }
    ADF_CHECK_MSG(rows == 15, "idclass.tsv has %zu rows", rows);
    golden_close(f);
    jsonl_close(parts);
}

/* ================================================================== the reference on 2200 generated texts */

/* every record of text_idele.jsonl at the precisions 128, 2, 7 and 30: the status of the reference (the gap rule
   for the valid ones), the value, the printed text; and classify against the reference. */
ADF_TEST(the_reference_on_generated_texts)
{
    jsonl_file * f = open_vectors("tests/ref/vectors/t-slice1/text_idele.jsonl");
    static const slong precs[] = { 128, 2, 7, 30 };
    size_t i, count = 0, valid = 0, nd = 0;

    if (f == NULL)
        return;
    for (i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value * rec = jsonl_record(f, i);
        size_t hl, el, kl, cl;
        const char * hex = rec_str(rec, "hex", &hl), * exp = rec_str(rec, "expected", &el);
        const char * kn = rec_str(rec, "kind", &kl), * cls = rec_str(rec, "cls", &cl);
        unsigned char * data;
        size_t n, k;
        char where[64];
        adf_text_kind ck;
        int cst, kind, want;

        if (hex == NULL || exp == NULL || kn == NULL || cls == NULL)
        {
            ADF_CHECK_MSG(0, "record %zu incomplete", i);
            continue;
        }
        data = hex_decode(hex, hl, &n);
        snprintf(where, sizeof(where), "text_idele.jsonl record %zu (%s)", i, kn);
        kind = strcmp(kn, "ucoset") == 0 ? 0 : (strcmp(kn, "idele") == 0 ? 1 : 2);
        /* the classification agrees with the reference (9.7) */
        cst = adf_text_classify(&ck, (const char *) data, n, NULL);
        if (strcmp(cls, "!PARSE") == 0)
            ADF_CHECK_MSG(cst == ADF_PARSE, "%s: classify status %d, reference !PARSE", where, cst);
        else
        {
            static const char * const names[] = { "rat", "fball", "adele", "cadele", "ucoset", "idele", "idclass",
                                                  "lball", "sball", "qclass", "ffun", "rfun", "char" };
            ADF_CHECK_MSG(cst == ADF_OK && strcmp(names[ck], cls) == 0, "%s: classify %d/%d, reference %s", where,
                          cst, cst == ADF_OK ? (int) ck : -1, cls);
        }
        want = exp[0] == '!' ? status_of_name(exp + 1) : ADF_OK;
        if (kind == 0)
        {
            adf_ucoset_t x;

            adf_ucoset_init(x);
            if (want != ADF_OK)
                parse_ucoset(x, (const char *) data, n, NULL, want, where);
            else if (parse_ucoset(x, (const char *) data, n, NULL, ADF_OK, where) == ADF_OK)
            {
                size_t len, l;
                char * t = adf_ucoset_get_str(&len, x);
                const char * cs = rec_str(rec, "c", &l), * Ns = rec_str(rec, "N", &l);

                ADF_CHECK_MSG(len == el && memcmp(t, exp, len) == 0, "%s: printed \"%s\", expected \"%s\"", where,
                              t, exp);
                if (cs != NULL && Ns != NULL)
                    check_unit(x, cs, Ns, where);
                flint_free(t);
                valid++;
            }
            adf_ucoset_clear(x);
        }
        else if (want != ADF_OK)
        {
            for (k = 0; k < sizeof(precs) / sizeof(precs[0]); k++)
            {
                if (kind == 1)
                {
                    adf_idele_t x;
                    adf_idele_init(x);
                    parse_idele(x, (const char *) data, n, precs[k], NULL, want, where);
                    adf_idele_clear(x);
                }
                else
                {
                    adf_idclass_t x;
                    adf_idclass_init(x);
                    parse_idclass(x, (const char *) data, n, precs[k], NULL, want, where);
                    adf_idclass_clear(x);
                }
            }
        }
        else
        {
            for (k = 0; k < sizeof(precs) / sizeof(precs[0]); k++)
            {
                check_valid_record(rec, kind, (const char *) data, n, exp, el, precs[k], where);
                if (k > 0)
                {
                    /* count the NOT_DETERMINED answers at the small precisions (reported by a check below) */
                    if (kind == 1)
                    {
                        adf_idele_t x;
                        adf_idele_init(x);
                        nd += adf_idele_set_str(x, (const char *) data, n, precs[k], NULL) == ADF_NOT_DETERMINED;
                        adf_idele_clear(x);
                    }
                    else
                    {
                        adf_idclass_t x;
                        adf_idclass_init(x);
                        nd += adf_idclass_set_str(x, (const char *) data, n, precs[k], NULL) == ADF_NOT_DETERMINED;
                        adf_idclass_clear(x);
                    }
                }
            }
            valid++;
        }
        free(data);
        count++;
    }
    /* what would make this test blind: too few valid texts, or no NOT_DETERMINED at all at the small precisions */
    printf("   %zu records, %zu valid, %zu NOT_DETERMINED answers at the precisions 2, 7 and 30\n", count, valid, nd);
    ADF_CHECK_MSG(count == 2200, "%zu records", count);
    ADF_CHECK_MSG(valid >= 800, "only %zu valid records", valid);
    ADF_CHECK_MSG(nd >= 5, "only %zu NOT_DETERMINED answers at the small precisions: the kernel is not reached", nd);
    jsonl_close(f);
}

/* ================================================================== constrained printing */

ADF_TEST(constrained_printing_against_the_reference)
{
    jsonl_file * f = open_vectors("tests/ref/vectors/t-slice1/print_constrained.jsonl");
    size_t i, count = 0, refused = 0;

    if (f == NULL)
        return;
    for (i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value * rec = jsonl_record(f, i);
        size_t l;
        const char * cond = rec_str(rec, "cond", &l), * mm = rec_str(rec, "mm", &l), * rm = rec_str(rec, "rm", &l);
        const char * text = rec_str(rec, "text", &l);
        long me, re, digits;
        fmpz_t m, r;
        arb_t x;
        char * t;
        size_t len, tl = l;
        char where[64];
        int idele = cond != NULL && strcmp(cond, "nonzero") == 0;

        if (cond == NULL || mm == NULL || rm == NULL || text == NULL || !rec_int(rec, "me", &me)
            || !rec_int(rec, "re", &re) || !rec_int(rec, "digits", &digits))
        {
            ADF_CHECK_MSG(0, "record %zu incomplete", i);
            continue;
        }
        snprintf(where, sizeof(where), "print_constrained.jsonl record %zu", i);
        fmpz_init(m);
        fmpz_init(r);
        arb_init(x);
        ADF_CHECK(fmpz_set_str(m, mm, 10) == 0 && fmpz_set_str(r, rm, 10) == 0);
        {
            fmpz_t e;

            fmpz_init(e);
            fmpz_set_si(e, me);
            arf_set_fmpz_2exp(arb_midref(x), m, e);
            mag_set_ui_2exp_si(arb_radref(x), fmpz_get_ui(r), re);
            fmpz_clear(e);
        }
        /* the radius is exact below 2^30 (mag_set_ui_2exp_si); the arb is the exact dyadic ball */
        ADF_CHECK_MSG(arb_is_finite(x) && (idele ? arb_is_nonzero(x) : arb_is_positive(x)), "%s: bad vector", where);
        if (!arb_printable(x))
            refused++;
        {
            fmpq_t one;
            adf_ucoset_t u;
            int st;

            fmpq_init(one);
            fmpq_one(one);
            adf_ucoset_init(u);
            if (idele)
            {
                adf_idele_t y;

                adf_idele_init(y);
                st = adf_idele_set_parts(y, x, one, u);
                ADF_CHECK_MSG(st == ADF_OK, "%s: set_parts %d", where, st);
                t = adf_idele_get_str(&len, y, (slong) digits);
                adf_idele_clear(y);
            }
            else
            {
                adf_idclass_t y;

                adf_idclass_init(y);
                st = adf_idclass_set_parts(y, x, u);
                ADF_CHECK_MSG(st == ADF_OK, "%s: set_parts %d", where, st);
                t = adf_idclass_get_str(&len, y, (slong) digits);
                adf_idclass_clear(y);
            }
            adf_ucoset_clear(u);
            fmpq_clear(one);
        }
        if (t == NULL)
            ADF_CHECK_MSG(!arb_printable(x) && len == 0, "%s: NULL for a printable ball", where);
        else
        {
            size_t pre, pfb;

            ADF_CHECK_MSG(well_formed(t, len), "%s: not well formed", where);
            ADF_CHECK_MSG(split_form(t, len, &pre, &pfb), "%s: \"%s\" has no \" ; \"", where, t);
            if (split_form(t, len, &pre, &pfb))
                ADF_CHECK_MSG(pre - 1 == tl && memcmp(t + 1, text, tl) == 0, "%s: real part \"%.*s\", reference \"%s\"",
                              where, (int) (pre - 1), t + 1, text);
        }
        flint_free(t);
        fmpz_clear(m);
        fmpz_clear(r);
        arb_clear(x);
        count++;
    }
    ADF_CHECK_MSG(count == 1500, "%zu records", count);
    ADF_CHECK_MSG(refused == 0, "%zu vectors beyond the printer bound", refused);
    jsonl_close(f);
}

/* The example of conventions 9.5: 1 +/- (1 - 2^-20) needs k = 7 (the reference prints it at 0.9999991); the
   interval read back exactly from a printed text of integers is a fixed point of printing. */
ADF_TEST(constrained_printing_examples)
{
    adf_idele_t x;
    adf_idclass_t c;
    size_t len;
    char * t;
    arb_t b;

    adf_idele_init(x);
    adf_idclass_init(c);
    arb_init(b);
    arf_set_si(arb_midref(b), 1);
    /* rad = 1 - 2^-20 = (2^20 - 1) 2^-20 */
    mag_set_ui_2exp_si(arb_radref(b), (1 << 20) - 1, -20);
    {
        fmpq_t one;
        adf_ucoset_t u;

        fmpq_init(one);
        fmpq_one(one);
        adf_ucoset_init(u);
        ADF_CHECK(adf_idele_set_parts(x, b, one, u) == ADF_OK);
        ADF_CHECK(adf_idclass_set_parts(c, b, u) == ADF_OK);
        adf_ucoset_clear(u);
        fmpq_clear(one);
    }
    t = adf_idele_get_str(&len, x, 20);
    ADF_CHECK_MSG(t != NULL && strcmp(t, "(1 +/- 0.9999991 ; 1 * [1])") == 0, "idele: %s", t);
    flint_free(t);
    t = adf_idclass_get_str(&len, c, 20);
    ADF_CHECK_MSG(t != NULL && strcmp(t, "<1 +/- 0.9999991 ; [1]>") == 0, "class: %s", t);
    flint_free(t);
    /* mid = 20396493/16 with a radius near 1.258e6: level 4 prints 1275000 +/- 1259000 whose least level is 3;
       the repetition of 9.5 ends at a fixed point (the text read back and printed again is the same) */
    {
        fmpz_t m;

        fmpz_init(m);
        arf_set_si(arb_midref(b), 20396493);
        arf_mul_2exp_si(arb_midref(b), arb_midref(b), -4);
        mag_set_ui_2exp_si(arb_radref(b), 1258031, 0);
        fmpz_clear(m);
    }
    {
        fmpq_t one;
        adf_ucoset_t u;
        char * t2;
        size_t len2;
        adf_idele_t y;
        int st;

        fmpq_init(one);
        fmpq_one(one);
        adf_ucoset_init(u);
        adf_idele_init(y);
        ADF_CHECK(adf_idele_set_parts(x, b, one, u) == ADF_OK);
        t = adf_idele_get_str(&len, x, 2);
        ADF_CHECK(t != NULL);
        /* read it back at a precision that holds integers exactly, print again */
        st = adf_idele_set_str(y, t, len, 128, NULL);
        ADF_CHECK_MSG(st == ADF_OK, "reading back \"%s\": %d", t, st);
        t2 = adf_idele_get_str(&len2, y, 2);
        ADF_CHECK_MSG(t2 != NULL && len == len2 && memcmp(t, t2, len) == 0, "not a fixed point: \"%s\" then \"%s\"", t,
                      t2);
        flint_free(t2);
        flint_free(t);
        adf_idele_clear(y);
        adf_ucoset_clear(u);
        fmpq_clear(one);
    }
    arb_clear(b);
    adf_idele_clear(x);
    adf_idclass_clear(c);
}

ADF_TEST(printing_of_hand_made_values)
{
    adf_idele_t x;
    adf_idclass_t c;
    adf_rat_t q;
    size_t len;
    char * t;
    fmpz_t cc, NN;

    adf_idele_init(x);
    adf_idclass_init(c);
    adf_rat_init(q);
    fmpz_init(cc);
    fmpz_init(NN);
    t = adf_idele_get_str(&len, x, 20);
    ADF_CHECK_MSG(t != NULL && len == 13 && strcmp(t, "(1 ; 1 * [1])") == 0, "init idele: %s", t);
    flint_free(t);
    t = adf_idclass_get_str(&len, c, 20);
    ADF_CHECK_MSG(t != NULL && strcmp(t, "<1 ; [1]>") == 0, "init class: %s", t);
    flint_free(t);
    /* the idele of the exact rationals 3/2 and -3/2 */
    fmpq_set_si(q->q, 3, 2);
    ADF_CHECK(adf_idele_set_rat(x, q, 64) == ADF_OK);
    t = adf_idele_get_str(&len, x, 20);
    ADF_CHECK_MSG(t != NULL && strcmp(t, "(1.5 ; 3/2 * [1])") == 0, "3/2: %s", t);
    flint_free(t);
    fmpq_set_si(q->q, -3, 2);
    ADF_CHECK(adf_idele_set_rat(x, q, 64) == ADF_OK);
    t = adf_idele_get_str(&len, x, 20);
    ADF_CHECK_MSG(t != NULL && strcmp(t, "(-1.5 ; 3/2 * [-1])") == 0, "-3/2: %s", t);
    flint_free(t);
    /* the unit is printed in normal form, the stored modulus is not */
    {
        adf_ucoset_t u;
        arb_t b;
        fmpq_t r;

        adf_ucoset_init(u);
        arb_init(b);
        fmpq_init(r);
        arb_set_si(b, 5);
        fmpq_set_si(r, 6, 4);
        fmpz_set_si(cc, 5);
        fmpz_set_si(NN, 6);
        ADF_CHECK(adf_ucoset_set_fmpz2(u, cc, NN) == ADF_OK);
        ADF_CHECK(adf_idele_set_parts(x, b, r, u) == ADF_OK);
        t = adf_idele_get_str(&len, x, 20);
        ADF_CHECK_MSG(t != NULL && strcmp(t, "(5 ; 3/2 * [2 mod 3])") == 0, "non-normal unit: %s", t);
        flint_free(t);
        ADF_CHECK(adf_idclass_set_parts(c, b, u) == ADF_OK);
        t = adf_idclass_get_str(&len, c, 20);
        ADF_CHECK_MSG(t != NULL && strcmp(t, "<5 ; [2 mod 3]>") == 0, "non-normal unit (class): %s", t);
        flint_free(t);
        /* a value beyond the printer bound of M1-D6 is refused with len = 0 */
        arb_set_ui(b, 1);
        arb_mul_2exp_si(b, b, ADF_PRINT_EXP_MAX + 1);
        ADF_CHECK(adf_idele_set_parts(x, b, r, u) == ADF_OK);
        len = 77;
        t = adf_idele_get_str(&len, x, 20);
        ADF_CHECK(t == NULL && len == 0);
        len = 77;
        ADF_CHECK(adf_idclass_set_parts(c, b, u) == ADF_OK);
        t = adf_idclass_get_str(&len, c, 20);
        ADF_CHECK(t == NULL && len == 0);
        /* the bound itself is printable (2^ADF_PRINT_EXP_MAX is a mantissa 1 with exponent MAX + 1: over; 2^(MAX-1)
           has ARF_EXP = MAX: not over) */
        arb_set_ui(b, 1);
        arb_mul_2exp_si(b, b, ADF_PRINT_EXP_MAX - 1);
        ADF_CHECK(adf_idele_set_parts(x, b, r, u) == ADF_OK);
        t = adf_idele_get_str(&len, x, 20);
        ADF_CHECK_MSG(t != NULL && strstr(t, "e30102 +/- ") != NULL, "got %s", t);
        flint_free(t);
        fmpq_clear(r);
        arb_clear(b);
        adf_ucoset_clear(u);
    }
    fmpz_clear(cc);
    fmpz_clear(NN);
    adf_rat_clear(q);
    adf_idele_clear(x);
    adf_idclass_clear(c);
}

/* ================================================================== the working precision, NOT_DETERMINED */

ADF_TEST(prec_rules_and_not_determined)
{
    adf_idele_t x;
    adf_idclass_t c;
    const char * near_zero = "(1 +/- 0.99999999999 ; 1 * [1])";
    const char * near_zero_neg = "(-1 +/- 0.99999999999 ; 1 * [1])";
    const char * near_zero_c = "<1 +/- 0.99999999999 ; [1]>";
    fmpq_t lo, hi, mid, rad;
    slong p;

    adf_idele_init(x);
    adf_idclass_init(c);
    fmpq_init(lo);
    fmpq_init(hi);
    fmpq_init(mid);
    fmpq_init(rad);
    /* the exact interval is [1e-11, 2 - 1e-11]; the ball of 9.5 has radius 1 (30 bits), so contains 0, and the
       kernel needs e(hi) - e(lo) = 37 or 38 <= p */
    ADF_CHECK(parse_idele(x, near_zero, strlen(near_zero), 30, NULL, ADF_NOT_DETERMINED, "near zero at 30") ==
              ADF_NOT_DETERMINED);
    ADF_CHECK(parse_idele(x, near_zero, strlen(near_zero), 2, NULL, ADF_NOT_DETERMINED, "near zero at 2") ==
              ADF_NOT_DETERMINED);
    ADF_CHECK(parse_idele(x, near_zero, strlen(near_zero), -9, NULL, ADF_NOT_DETERMINED, "near zero at -9") ==
              ADF_NOT_DETERMINED);
    ADF_CHECK(parse_idclass(c, near_zero_c, strlen(near_zero_c), 30, NULL, ADF_NOT_DETERMINED, "class at 30") ==
              ADF_NOT_DETERMINED);
    /* a prec of 64 and of 128 certifies it: the ball contains the exact interval and excludes 0 */
    for (p = 64; p <= 128; p += 64)
    {
        char where[40];

        snprintf(where, sizeof(where), "near zero at %ld", (long) p);
        ADF_CHECK(parse_idele(x, near_zero, strlen(near_zero), p, NULL, ADF_OK, where) == ADF_OK);
        dec_value(mid, "1", 0);
        dec_value(rad, "99999999999", -11);
        ADF_CHECK_MSG(arb_contains_interval(x->inf, mid, rad) && arb_is_positive(x->inf), "%s", where);
        ADF_CHECK(parse_idclass(c, near_zero_c, strlen(near_zero_c), p, NULL, ADF_OK, where) == ADF_OK);
        ADF_CHECK_MSG(arb_contains_interval(c->t, mid, rad) && arb_is_positive(c->t), "%s", where);
        ADF_CHECK(parse_idele(x, near_zero_neg, strlen(near_zero_neg), p, NULL, ADF_OK, where) == ADF_OK);
        dec_value(mid, "-1", 0);
        ADF_CHECK_MSG(arb_contains_interval(x->inf, mid, rad) && arb_is_negative(x->inf), "%s", where);
    }
    /* a prec below 2 is taken as 2 (M1-D4): the same result at -5, 0, 1 and 2 */
    {
        adf_idele_t y, z;
        slong k;

        adf_idele_init(y);
        adf_idele_init(z);
        ADF_CHECK(adf_idele_set_str(y, LIT("(0.1 ; 1 * [1])"), 2, NULL) == ADF_OK);
        for (k = -5; k <= 1; k++)
        {
            ADF_CHECK(adf_idele_set_str(z, LIT("(0.1 ; 1 * [1])"), k, NULL) == ADF_OK);
            ADF_CHECK_MSG(adf_idele_identical(y, z), "prec %ld differs from 2", (long) k);
        }
        adf_idele_clear(y);
        adf_idele_clear(z);
    }
    /* a prec above ADF_IDELE_PREC_MAX is ADF_LIMIT, before every other status (even for a text that is no text
       of the kind), and the output is untouched */
    parse_idele(x, LIT("(1 ; 1 * [1])"), ADF_IDELE_PREC_MAX + 1, NULL, ADF_LIMIT, "prec max + 1");
    parse_idele(x, LIT("garbage"), ADF_IDELE_PREC_MAX + 1, NULL, ADF_LIMIT, "prec max + 1, garbage");
    parse_idele(x, LIT("(0 ; 1 * [1])"), WORD_MAX, NULL, ADF_LIMIT, "prec WORD_MAX");
    parse_idclass(c, LIT("<1 ; [1]>"), ADF_IDELE_PREC_MAX + 1, NULL, ADF_LIMIT, "class prec max + 1");
    parse_idclass(c, LIT("junk"), WORD_MAX, NULL, ADF_LIMIT, "class prec WORD_MAX");
    /* the bound itself is admitted */
    ADF_CHECK(adf_idele_set_str(x, LIT("(1 ; 1 * [1])"), ADF_IDELE_PREC_MAX, NULL) == ADF_OK);
    fmpq_clear(lo);
    fmpq_clear(hi);
    fmpq_clear(mid);
    fmpq_clear(rad);
    adf_idele_clear(x);
    adf_idclass_clear(c);
}

/* ================================================================== limits and hostile input */

ADF_TEST(limits_of_conventions_8_4)
{
    adf_text_limits_t lim;
    adf_idele_t x;
    adf_idclass_t c;
    adf_ucoset_t u;
    char * big;
    size_t n;

    adf_idele_init(x);
    adf_idclass_init(c);
    adf_ucoset_init(u);
    adf_text_limits_default(&lim);
    /* an exponent above max_exp10 (the default 100000) */
    parse_idele(x, LIT("(1e100001 ; 1 * [1])"), 64, NULL, ADF_LIMIT, "1e100001");
    parse_idele(x, LIT("(1e100000 ; 1 * [1])"), 64, NULL, ADF_OK, "1e100000");
    parse_idele(x, LIT("(1e-100001 ; 1 * [1])"), 64, NULL, ADF_LIMIT, "1e-100001");
    parse_idclass(c, LIT("<1 +/- 1e100001 ; [1]>"), 64, NULL, ADF_LIMIT, "class radius 1e100001");
    parse_idclass(c, LIT("<1e100001 ; [1]>"), 64, NULL, ADF_LIMIT, "class 1e100001");
    /* a 39-digit exponent is LIMIT, and a zero coefficient does not excuse it (M1-D7) */
    parse_idele(x, LIT("(0e123456789012345678901234567890123456789 ; 1 * [1])"), 64, NULL, ADF_LIMIT, "0e39 digits");
    parse_idele(x, LIT("(1e00000000000000000000000000000000000005 ; 1 * [1])"), 64, NULL, ADF_OK, "1e0...05");
    lim.max_exp10 = 7;
    parse_idele(x, LIT("(1e8 ; 1 * [1])"), 64, &lim, ADF_LIMIT, "1e8 at max_exp10 7");
    parse_idele(x, LIT("(1e-7 ; 1 * [1])"), 64, &lim, ADF_OK, "1e-7 at max_exp10 7");
    parse_idele(x, LIT("(1e-8 ; 1 * [1])"), 64, &lim, ADF_LIMIT, "1e-8 at max_exp10 7");
    /* grammar before limits (8.5): an exponent over the limit in a text that is not an idele is PARSE */
    parse_idele(x, LIT("(1e100001 ; 1 [1])"), 64, NULL, ADF_PARSE, "grammar first");
    /* limits before the semantic stage: LIMIT wins over DOMAIN */
    parse_idele(x, LIT("(0 +/- 1e100001 ; 0 * [2 mod 4])"), 64, NULL, ADF_LIMIT, "limit before domain");
    /* max_len: 1 byte over is LIMIT before any byte is read (even a forbidden one); exactly max_len is read */
    adf_text_limits_default(&lim);
    lim.max_len = 13;
    parse_idele(x, LIT("(1 ; 1 * [1])"), 64, &lim, ADF_OK, "len = max_len");
    parse_idele(x, LIT("(1 ; 1 * [1]) "), 64, &lim, ADF_LIMIT, "len = max_len + 1");
    parse_idele(x, LIT("(1 ; 1 * [1])\x01"), 64, &lim, ADF_LIMIT, "len = max_len + 1, forbidden byte");
    lim.max_len = 9;
    parse_ucoset(u, LIT("[5 mod 6]"), &lim, ADF_OK, "ucoset len = max_len");
    parse_ucoset(u, LIT("[5 mod 6] "), &lim, ADF_LIMIT, "ucoset len = max_len + 1");
    parse_idclass(c, LIT("<1 ; [1]>"), 64, &lim, ADF_OK, "class len = max_len");
    parse_idclass(c, LIT("<1 ; [1]> "), 64, &lim, ADF_LIMIT, "class len = max_len + 1");
    /* the default: 1048576 bytes are read, 1048577 are LIMIT */
    n = ADF_TEXT_MAX_LEN_DEFAULT;
    big = (char *) malloc(n + 2);
    memset(big, ' ', n + 1);
    memcpy(big, "[5 mod 6]", 9);
    parse_ucoset(u, big, n, NULL, ADF_OK, "1048576 bytes");
    parse_ucoset(u, big, n + 1, NULL, ADF_LIMIT, "1048577 bytes");
    memcpy(big, "(1 ; 1 * [1])", 13);
    parse_idele(x, big, n, 64, NULL, ADF_OK, "idele 1048576 bytes");
    parse_idele(x, big, n + 1, 64, NULL, ADF_LIMIT, "idele 1048577 bytes");
    free(big);
    adf_idele_clear(x);
    adf_idclass_clear(c);
    adf_ucoset_clear(u);
}

/* a text of the kind with len bytes: n copies of a digit as the modulus etc. */
ADF_TEST(hostile_big_numbers_nesting_nul_and_cuts)
{
    adf_ucoset_t u, u2;
    adf_idele_t x;
    adf_idclass_t c;
    size_t i, k, n;
    char * s, * t;
    size_t len;
    static const char * const valid[] = { "[5 mod 36]", "(2.5 +/- 1e-9 ; 3/2 * [5 mod 36])",
                                          "<1.25 +/- 1e-30 ; [5 mod 36]>", "(-0.5 +/- 0.25 ; 1/7 * [1 mod 1])",
                                          "  ( 1 ;\t1 *\r[ -1 mod 0 ] )  ", "< 1 ; [ 1 ] >" };

    adf_ucoset_init(u);
    adf_ucoset_init(u2);
    adf_idele_init(x);
    adf_idclass_init(c);

    /* a modulus of 100000 digits: a 1 and 99999 zeros; the coset [1 mod N] is valid and prints back */
    n = 100000;
    s = (char *) malloc(n + 32);
    strcpy(s, "[1 mod 1");
    repeat_char(s + 8, '0', n - 1);
    strcpy(s + 8 + n - 1, "]");
    parse_ucoset(u, s, 8 + n, NULL, ADF_OK, "N of 100000 digits");
    t = adf_ucoset_get_str(&len, u);
    ADF_CHECK_MSG(len == 8 + n && memcmp(t, s, len) == 0, "the modulus of 100000 digits prints back");
    flint_free(t);
    /* a residue of 100000 digits against a small modulus, and both huge and not coprime */
    strcpy(s, "[");
    repeat_char(s + 1, '7', n);
    strcpy(s + 1 + n, " mod 5]");
    parse_ucoset(u, s, 1 + n + 7, NULL, ADF_OK, "c of 100000 digits mod 5");
    t = adf_ucoset_get_str(&len, u);
    ADF_CHECK_MSG(strcmp(t, "[2 mod 5]") == 0, "got %.40s", t);
    flint_free(t);
    strcpy(s, "[");
    repeat_char(s + 1, '6', n);
    strcpy(s + 1 + n, " mod 6]");
    parse_ucoset(u, s, 1 + n + 7, NULL, ADF_DOMAIN, "c of 100000 digits, gcd 6");
    /* an idele with a content of 100000 digits in numerator and denominator */
    strcpy(s, "(1 ; ");
    repeat_char(s + 5, '9', 50000);
    s[5 + 50000] = '/';
    repeat_char(s + 5 + 50001, '7', 49999);
    strcpy(s + 5 + 50001 + 49999, " * [1])");
    parse_idele(x, s, 5 + 50001 + 49999 + 7, 64, NULL, ADF_OK, "content of 100000 digits");
    free(s);

    /* nesting, missing brackets, wrong brackets */
    parse_idele(x, LIT("((((1 ; 1 * [1])))"), 64, NULL, ADF_PARSE, "nesting");
    parse_idele(x, LIT("(1 ; 1 * [1]"), 64, NULL, ADF_PARSE, "missing )");
    parse_idele(x, LIT("1 ; 1 * [1])"), 64, NULL, ADF_PARSE, "missing (");
    parse_idele(x, LIT("(1 ; 1 * 1])"), 64, NULL, ADF_PARSE, "missing [");
    parse_idele(x, LIT("(1 ; 1 * [1)"), 64, NULL, ADF_PARSE, "missing ]");
    parse_idele(x, LIT("[1 ; 1 * (1])"), 64, NULL, ADF_PARSE, "swapped brackets");
    parse_idclass(c, LIT("<1 ; [1]"), 64, NULL, ADF_PARSE, "missing >");
    parse_idclass(c, LIT("1 ; [1]>"), 64, NULL, ADF_PARSE, "missing <");
    parse_idclass(c, LIT("<<1 ; [1]>>"), 64, NULL, ADF_PARSE, "nesting <<");
    parse_idclass(c, LIT("<(1) ; [1]>"), 64, NULL, ADF_PARSE, "parenthesised real");
    parse_ucoset(u, LIT("[[1]]"), NULL, ADF_PARSE, "nesting [[");
    parse_ucoset(u, LIT("[1 mod [2]]"), NULL, ADF_PARSE, "nesting mod [");
    parse_ucoset(u, LIT("[1 mod 2"), NULL, ADF_PARSE, "missing ]");
    parse_ucoset(u, LIT("1 mod 2]"), NULL, ADF_PARSE, "missing [");
    parse_ucoset(u, LIT("[1 MOD 2]"), NULL, ADF_PARSE, "keywords are case-sensitive");
    parse_ucoset(u, LIT("[1 mod 2] x"), NULL, ADF_PARSE, "trailing junk");
    parse_ucoset(u, LIT("[+1]"), NULL, ADF_PARSE, "no plus sign");
    parse_ucoset(u, LIT("[1.5]"), NULL, ADF_PARSE, "no decimal point");
    parse_ucoset(u, LIT("[1e2]"), NULL, ADF_PARSE, "no exponent");
    parse_ucoset(u, LIT("[\xe2\x88\x92" "1]"), NULL, ADF_PARSE, "unicode minus");
    parse_idele(x, LIT("(1 ; 1 * [1]) (1 ; 1 * [1])"), 64, NULL, ADF_PARSE, "two ideles");
    parse_idele(x, LIT("(1 \xc2\xb1 2 ; 1 * [1])"), 64, NULL, ADF_PARSE, "plus-minus sign");
    parse_idele(x, LIT("(1 +/-- 2 ; 1 * [1])"), 64, NULL, ADF_PARSE, "+/-- ");

    /* NUL bytes: at every position of a valid text; the typed parsers refuse (PARSE) */
    for (k = 0; k < sizeof(valid) / sizeof(valid[0]); k++)
    {
        size_t vl = strlen(valid[k]);
        char buf[128];

        for (i = 0; i <= vl; i++)
        {
            char where[64];

            memcpy(buf, valid[k], i);
            buf[i] = '\0';
            memcpy(buf + i + 1, valid[k] + i, vl - i);
            snprintf(where, sizeof(where), "valid[%zu] with a NUL at %zu", k, i);
            if (k == 0)
                parse_ucoset(u, buf, vl + 1, NULL, ADF_PARSE, where);
            else if (k == 2 || k == 5)
                parse_idclass(c, buf, vl + 1, 64, NULL, ADF_PARSE, where);
            else
                parse_idele(x, buf, vl + 1, 64, NULL, ADF_PARSE, where);
        }
    }
    /* texts cut at every position of a valid text: each prefix is refused, or is a valid text of its own (it
       classifies as the kind), never a crash; and a text with one byte deleted is refused or classified */
    for (k = 0; k < sizeof(valid) / sizeof(valid[0]); k++)
    {
        size_t vl = strlen(valid[k]);
        char buf[128];
        int isu = k == 0, isc = (k == 2 || k == 5);

        for (i = 0; i <= vl; i++)
        {
            char where[64];
            int st;
            adf_text_kind kind;
            int cst;

            /* the prefix of length i */
            snprintf(where, sizeof(where), "valid[%zu] cut at %zu", k, i);
            if (isu)
                st = adf_ucoset_set_str(u, valid[k], i, NULL);
            else if (isc)
                st = adf_idclass_set_str(c, valid[k], i, 64, NULL);
            else
                st = adf_idele_set_str(x, valid[k], i, 64, NULL);
            cst = adf_text_classify(&kind, valid[k], i, NULL);
            /* classify and the typed parser agree (9.7): PARSE exactly when the kind is not the one of the parser */
            if (cst == ADF_OK && kind == (adf_text_kind) (isu ? ADF_TEXT_UCOSET : (isc ? ADF_TEXT_IDCLASS : ADF_TEXT_IDELE)))
                ADF_CHECK_MSG(st != ADF_PARSE, "%s: classified, but PARSE", where);
            else
                ADF_CHECK_MSG(st == ADF_PARSE, "%s: not classified, but status %d", where, st);
            if (i == vl)
                ADF_CHECK_MSG(st == ADF_OK, "%s: the whole text: status %d", where, st);
            /* the suffix from i */
            if (isu)
                st = adf_ucoset_set_str(u, valid[k] + i, vl - i, NULL);
            else if (isc)
                st = adf_idclass_set_str(c, valid[k] + i, vl - i, 64, NULL);
            else
                st = adf_idele_set_str(x, valid[k] + i, vl - i, 64, NULL);
            ADF_CHECK_MSG(st == ADF_OK || st == ADF_PARSE || st == ADF_DOMAIN || st == ADF_LIMIT
                              || st == ADF_NOT_DETERMINED,
                          "%s (suffix): status %d", where, st);
            /* one byte deleted */
            if (i < vl)
            {
                memcpy(buf, valid[k], i);
                memcpy(buf + i, valid[k] + i + 1, vl - i - 1);
                if (isu)
                    st = adf_ucoset_set_str(u, buf, vl - 1, NULL);
                else if (isc)
                    st = adf_idclass_set_str(c, buf, vl - 1, 64, NULL);
                else
                    st = adf_idele_set_str(x, buf, vl - 1, 64, NULL);
                ADF_CHECK_MSG(st == ADF_OK || st == ADF_PARSE || st == ADF_DOMAIN || st == ADF_LIMIT
                                  || st == ADF_NOT_DETERMINED,
                              "%s (deleted): status %d", where, st);
            }
        }
    }
    adf_ucoset_clear(u);
    adf_ucoset_clear(u2);
    adf_idele_clear(x);
    adf_idclass_clear(c);
}

/* the stages of 8.5 in order, with texts of two faults: the earlier stage gives the status */
ADF_TEST(order_of_the_stages)
{
    adf_idele_t x;
    adf_idclass_t c;
    adf_ucoset_t u;

    adf_idele_init(x);
    adf_idclass_init(c);
    adf_ucoset_init(u);
    /* PARSE (grammar) before DOMAIN */
    parse_idele(x, LIT("(0 ; 0 * [2 mod 4]"), 64, NULL, ADF_PARSE, "parse before domain");
    /* a forbidden byte (stage 2) before the grammar: PARSE either way; before LIMIT of an exponent: PARSE */
    parse_idele(x, LIT("(1e100001 \x01 ; 1 * [1])"), 64, NULL, ADF_PARSE, "forbidden byte before limit");
    /* DOMAIN of the unit and of the real part and of the content: all DOMAIN */
    parse_idele(x, LIT("(1 ; 1 * [2 mod 4])"), 64, NULL, ADF_DOMAIN, "unit");
    parse_idele(x, LIT("(0 ; 1 * [1])"), 64, NULL, ADF_DOMAIN, "real 0");
    parse_idele(x, LIT("(1 ; 0 * [1])"), 64, NULL, ADF_DOMAIN, "content 0");
    parse_idele(x, LIT("(1 ; 1/0 * [1])"), 64, NULL, ADF_DOMAIN, "content 1/0");
    parse_idele(x, LIT("(-1 ; 1 * [1])"), 64, NULL, ADF_OK, "negative real is an idele");
    parse_idclass(c, LIT("<-1 ; [1]>"), 64, NULL, ADF_DOMAIN, "class of a negative real");
    parse_idclass(c, LIT("<0 +/- 1 ; [1]>"), 64, NULL, ADF_DOMAIN, "class of a ball with 0");
    /* DOMAIN before NOT_DETERMINED: the unit is checked whatever the precision */
    parse_idele(x, LIT("(1 +/- 0.99999999999 ; 1 * [2 mod 4])"), 30, NULL, ADF_DOMAIN, "domain before not determined");
    /* the typed parsers do not coerce: another kind is PARSE */
    parse_idele(x, LIT("7/3"), 64, NULL, ADF_PARSE, "a rational is no idele");
    parse_idele(x, LIT("<1 ; [1]>"), 64, NULL, ADF_PARSE, "a class is no idele");
    parse_idclass(c, LIT("(1 ; 1 * [1])"), 64, NULL, ADF_PARSE, "an idele is no class");
    parse_idclass(c, LIT("[1]"), 64, NULL, ADF_PARSE, "a unit is no class");
    parse_ucoset(u, LIT("(1 ; 1 * [1])"), NULL, ADF_PARSE, "an idele is no unit coset");
    adf_idele_clear(x);
    adf_idclass_clear(c);
    adf_ucoset_clear(u);
}

/* the round trip of 9.6 on the values of the generated texts: print then read encloses; the unit of a re-read
   value is the normal form as a set */
ADF_TEST(round_trips_enclose)
{
    jsonl_file * f = open_vectors("tests/ref/vectors/t-slice1/text_idele.jsonl");
    size_t i, count = 0;

    if (f == NULL)
        return;
    for (i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value * rec = jsonl_record(f, i);
        size_t hl, el, kl;
        const char * hex = rec_str(rec, "hex", &hl), * exp = rec_str(rec, "expected", &el), * kn = rec_str(rec, "kind", &kl);
        unsigned char * data;
        size_t n, len;
        char where[64], * t;

        if (hex == NULL || exp == NULL || kn == NULL || exp[0] == '!')
            continue;
        data = hex_decode(hex, hl, &n);
        snprintf(where, sizeof(where), "record %zu (%s)", i, kn);
        if (strcmp(kn, "idele") == 0)
        {
            adf_idele_t x, y;
            int st;

            adf_idele_init(x);
            adf_idele_init(y);
            if (adf_idele_set_str(x, (const char *) data, n, 128, NULL) == ADF_OK)
            {
                t = adf_idele_get_str(&len, x, 20);
                if (t != NULL)
                {
                    st = adf_idele_set_str(y, t, len, 128, NULL);
                    ADF_CHECK_MSG(st == ADF_OK || st == ADF_NOT_DETERMINED, "%s: reread status %d of \"%s\"", where, st,
                                  t);
                    if (st == ADF_OK)
                    {
                        fmpq_t r1, r2;

                        ADF_CHECK_MSG(arb_contains(y->inf, x->inf), "%s: the re-read ball does not contain the first",
                                      where);
                        fmpq_init(r1);
                        fmpq_init(r2);
                        adf_idele_content(r1, x);
                        adf_idele_content(r2, y);
                        ADF_CHECK_MSG(fmpq_equal(r1, r2), "%s: content", where);
                        ADF_CHECK_MSG(adf_ucoset_equal_set(&x->u, &y->u), "%s: unit", where);
                        fmpq_clear(r1);
                        fmpq_clear(r2);
                        count++;
                    }
                    flint_free(t);
                }
            }
            adf_idele_clear(x);
            adf_idele_clear(y);
        }
        else if (strcmp(kn, "idclass") == 0)
        {
            adf_idclass_t x, y;
            int st;

            adf_idclass_init(x);
            adf_idclass_init(y);
            if (adf_idclass_set_str(x, (const char *) data, n, 128, NULL) == ADF_OK)
            {
                t = adf_idclass_get_str(&len, x, 20);
                if (t != NULL)
                {
                    st = adf_idclass_set_str(y, t, len, 128, NULL);
                    ADF_CHECK_MSG(st == ADF_OK || st == ADF_NOT_DETERMINED, "%s: reread status %d of \"%s\"", where, st,
                                  t);
                    if (st == ADF_OK)
                    {
                        ADF_CHECK_MSG(arb_contains(y->t, x->t), "%s: the re-read ball does not contain the first",
                                      where);
                        ADF_CHECK_MSG(adf_ucoset_equal_set(&x->u, &y->u), "%s: unit", where);
                        count++;
                    }
                    flint_free(t);
                }
            }
            adf_idclass_clear(x);
            adf_idclass_clear(y);
        }
        else
        {
            adf_ucoset_t x, y;

            adf_ucoset_init(x);
            adf_ucoset_init(y);
            if (adf_ucoset_set_str(x, (const char *) data, n, NULL) == ADF_OK)
            {
                t = adf_ucoset_get_str(&len, x);
                ADF_CHECK_MSG(adf_ucoset_set_str(y, t, len, NULL) == ADF_OK && adf_ucoset_equal_set(x, y)
                                  && adf_ucoset_is_normal(y),
                              "%s: unit round trip", where);
                flint_free(t);
                count++;
            }
            adf_ucoset_clear(x);
            adf_ucoset_clear(y);
        }
        free(data);
    }
    ADF_CHECK_MSG(count >= 700, "only %zu round trips", count);
    jsonl_close(f);
}

/* Review n-review1, D2: the constrained printer ends in bounded time on every ball that decision M1-D6 admits.
   The ball 2^(b-1) + 1/2 +/- 2^(b-1), b = 100000, has both exponents 100000 (admitted) and needs about 30000 levels of
   30000-digit numbers (it did not end in 170 s).  The printer now counts the work (levels times the size of the
   numbers in bits) and returns NULL with *len = 0 when the work passes ADF_PRINT_COND_WORK_MAX (docs/api-2.md 4.2,
   Statement Q; decision N-D11): the driver prints "error: LIMIT".  What would make a case fail: a text where NULL
   is due, NULL where the text is small, or a time above 30 s. The bound is a guard against the non-termination
   that N-D11 repaired (170 s and more), not a measurement: the refusal takes 1.2 s on mains power and 2.75 s on
   battery in the powersave governor (2026-10-02), so a bound of 2 s failed on battery. */
static void
ball_two_pow(arb_t x, slong b, slong e, int negative)
{
    fmpz_t z;

    fmpz_init(z);
    fmpz_one(z);
    fmpz_mul_2exp(z, z, (ulong) b);
    fmpz_add_ui(z, z, 1);
    arf_set_fmpz(arb_midref(x), z);
    arf_mul_2exp_si(arb_midref(x), arb_midref(x), e - b);
    mag_one(arb_radref(x));
    mag_mul_2exp_si(arb_radref(x), arb_radref(x), e);
    if (negative)
        arb_neg(x, x);
    fmpz_clear(z);
}

ADF_TEST(constrained_printer_ends_in_bounded_time)
{
    adf_idele_t x;
    adf_idclass_t c;
    size_t len;
    char * s;
    clock_t t0;
    double sec;

    adf_idele_init(x);
    adf_idclass_init(c);
    /* the worst admitted input, idele and class */
    ball_two_pow(x->inf, 100000, 99999, 0);
    ADF_CHECK(adf_idele_is_canonical(x));
    len = 17;
    t0 = clock();
    s = adf_idele_get_str(&len, x, 1);
    sec = (double) (clock() - t0) / CLOCKS_PER_SEC;
    ADF_CHECK_MSG(s == NULL && len == 0, "idele: NULL, len 0 expected (len %zu)", len);
    ADF_CHECK_MSG(sec < 30.0, "idele: %.2f s (bound 30 s)", sec);
    if (s != NULL)
        adf_str_free(s);
    arb_set(c->t, x->inf);
    ADF_CHECK(adf_idclass_is_canonical(c));
    len = 17;
    t0 = clock();
    s = adf_idclass_get_str(&len, c, 1);
    sec = (double) (clock() - t0) / CLOCKS_PER_SEC;
    ADF_CHECK_MSG(s == NULL && len == 0, "class: NULL, len 0 expected (len %zu)", len);
    ADF_CHECK_MSG(sec < 30.0, "class: %.2f s (bound 30 s)", sec);
    if (s != NULL)
        adf_str_free(s);
    /* the negative ball, likewise */
    ball_two_pow(x->inf, 100000, 99999, 1);
    len = 17;
    t0 = clock();
    s = adf_idele_get_str(&len, x, 1);
    sec = (double) (clock() - t0) / CLOCKS_PER_SEC;
    ADF_CHECK_MSG(s == NULL && len == 0 && sec < 30.0, "negative idele: %.2f s, len %zu", sec, len);
    if (s != NULL)
        adf_str_free(s);
    /* a ball of 4000 bits needs about 1200 levels: inside the bound, printed as before (it takes 0.1 s) */
    ball_two_pow(x->inf, 4000, 4000, 0);
    len = 0;
    s = adf_idele_get_str(&len, x, 1);
    ADF_CHECK_MSG(s != NULL && len > 1000 && len < 4000 && strncmp(s, "(1.318204093430943", 18) == 0,
                  "4000 bits: %s, len %zu", s == NULL ? "NULL" : "text", len);
    if (s != NULL)
        adf_str_free(s);
    adf_idele_clear(x);
    adf_idclass_clear(c);
}
