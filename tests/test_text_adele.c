/* tests/test_text_adele.c: the value form of adf_adele and adf_cadele (adf_adele_set_str,
   adf_adele_get_str, adf_cadele_set_str, adf_cadele_get_str), and the real-ball reader and printer
   of docs/conventions.md 9.5 that they use.

   Contract: include/adelefeld/text.h; docs/conventions.md 8 (8.1, 8.2, 8.4 max_exp10, 8.5), 9.2
   (adele_v, cadele_v, real, complex), 9.3, 9.4 (templates), 9.5 (reading: containment, and exactness
   for a dyadic midpoint of at most prec bits and a dyadic radius with odd mantissa below 2^30;
   printing), 9.6 (round trips enclose), 11.3 (how a C test uses the golden files, prec = 128).
   Oracles: tests/golden/{adele,cadele,realball_read,realball_print}.tsv (every row) and
   tests/ref/vectors/m1-text/{real_parts,print_real,text_adele,text_cadele}.jsonl (written by
   lanes/m1-text/gen_text_vectors.py with proto/text_grammar.py).

   adele.h is implemented by another lane at the same time: its functions are not called here.
   Values are initialised and cleared field by field (arb_init, adf_fball_init; brief of lane
   m1-text). Strings are freed with flint_free, which adf_str_free calls (common.h). */

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/fmpq.h>
#include <flint/arb.h>
#include <flint/acb.h>

#include <adelefeld.h>

#include "support/golden.h"
#include "support/jsonl.h"
#include "test_runner.h"

#define GOLDEN_PREC 128

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
    return b;
}

static void
adele_init(adf_adele_t x)
{
    arb_init(x->inf);
    adf_fball_init(&x->fin);
}

static void
adele_clear(adf_adele_t x)
{
    arb_clear(x->inf);
    adf_fball_clear(&x->fin);
}

static void
cadele_init(adf_cadele_t x)
{
    acb_init(x->inf);
    adf_fball_init(&x->fin);
}

static void
cadele_clear(adf_cadele_t x)
{
    acb_clear(x->inf);
    adf_fball_clear(&x->fin);
}

/* q = 10^e, e of any sign */
static void
pow10_fmpq(fmpq_t q, slong e)
{
    fmpz_t t;

    fmpz_init(t);
    fmpz_set_ui(t, 10);
    fmpz_pow_ui(t, t, (ulong) (e < 0 ? -e : e));
    if (e >= 0)
        fmpq_set_fmpz(q, t);
    else
    {
        fmpq_set_fmpz(q, t);
        fmpq_inv(q, q);
    }
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

/* An exact decimal [-]digits[.digits][(e|E)[+-]digits] at s[*i..n); 1 on success. The test's own
   reader, independent of src/text.c. */
static int
read_decimal(fmpq_t q, const char * s, size_t n, size_t * i)
{
    size_t j = *i;
    int neg = 0, eneg = 0;
    fmpz_t D;
    slong E = 0, X = 0;

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
    {
        fmpq_t p;
        fmpq_init(p);
        pow10_fmpq(p, E);
        fmpq_set_fmpz(q, D);
        fmpq_mul(q, q, p);
        if (neg)
            fmpq_neg(q, q);
        fmpq_clear(p);
    }
    fmpz_clear(D);
    *i = j;
    return 1;
}

/* A printed real ball "M" or "M +/- R" (conventions 9.5), read exactly. 1 on success. */
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

/* The text of a printed value: ASCII 0x20-0x7E, no whitespace at either end, NUL at len. */
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

/* Split "(R ; F)" into R and F. */
static int
split_adele(const char * t, size_t n, size_t * rb, size_t * re, size_t * fb, size_t * fe)
{
    size_t k;

    if (n < 6 || t[0] != '(' || t[n - 1] != ')')
        return 0;
    for (k = 1; k + 3 <= n; k++)
        if (memcmp(t + k, " ; ", 3) == 0)
            break;
    if (k + 3 > n)
        return 0;
    *rb = 1;
    *re = k;
    *fb = k + 3;
    *fe = n - 1;
    return 1;
}

/* Split "((X) + (Y)*i ; F)" into X, Y and F. */
static int
split_cadele(const char * t, size_t n, size_t sp[6])
{
    size_t k, m;

    if (n < 16 || memcmp(t, "((", 2) != 0 || t[n - 1] != ')')
        return 0;
    for (k = 2; k + 5 <= n; k++)
        if (memcmp(t + k, ") + (", 5) == 0)
            break;
    if (k + 5 > n)
        return 0;
    for (m = k + 5; m + 6 <= n; m++)
        if (memcmp(t + m, ")*i ; ", 6) == 0)
            break;
    if (m + 6 > n)
        return 0;
    sp[0] = 2;
    sp[1] = k;
    sp[2] = k + 5;
    sp[3] = m;
    sp[4] = m + 6;
    sp[5] = n - 1;
    return 1;
}

/* The exactness condition of conventions 9.5 ("Reading") at prec: the midpoint dyadic with an odd
   mantissa of at most prec bits, the radius dyadic with an odd mantissa below 2^30. */
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

static int
exact_condition(const fmpq_t mid, const fmpq_t rad, slong prec)
{
    return dyadic_bits_at_most(mid, (ulong) prec) && dyadic_bits_at_most(rad, 30);
}

/* The arb is exactly [mid +/- rad]. */
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

/* The arb contains the exact interval [mid - rad, mid + rad]. */
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

/* conventions 11.3 item 3 for one real part: the arb x read from a text whose exact ball is
   (mid, rad), and the printed part P of length n; E the expected part (NULL: not compared). */
static void
check_real_part(const arb_t x, const fmpq_t mid, const fmpq_t rad, slong prec, const char * P, size_t n,
                const char * E, size_t en, const char * where)
{
    fmpq_t pm, pr;

    fmpq_init(pm);
    fmpq_init(pr);
    ADF_CHECK_MSG(arb_is_finite(x), "%s: not finite", where);
    ADF_CHECK_MSG(arb_contains_interval(x, mid, rad), "%s: the arb does not contain the exact ball", where);
    ADF_CHECK_MSG(read_printed_real(pm, pr, P, n), "%s: the printed part \"%.*s\" is not a real ball", where,
                  (int) n, P);
    ADF_CHECK_MSG(interval_contains(pm, pr, mid, rad), "%s: the printed \"%.*s\" does not contain the exact ball",
                  where, (int) n, P);
    if (exact_condition(mid, rad, prec))
    {
        ADF_CHECK_MSG(arb_is_exactly(x, mid, rad), "%s: the arb is not exactly the dyadic ball", where);
        if (E != NULL)
            ADF_CHECK_MSG(n == en && memcmp(P, E, n) == 0, "%s: printed \"%.*s\", expected \"%.*s\"", where,
                          (int) n, P, (int) en, E);
    }
    fmpq_clear(pm);
    fmpq_clear(pr);
}

/* The sentinels, which no failing parse may change. */
static void
set_sentinel_arb(arb_t r)
{
    arf_set_si(arb_midref(r), 3);
    arf_mul_2exp_si(arb_midref(r), arb_midref(r), -3);
    mag_set_ui_2exp_si(arb_radref(r), 1, -10);
}

static void
set_sentinel_fin(adf_fball_t f)
{
    fmpz_set_si(f->A, 5);
    fmpz_set_si(f->H, 18);
    fmpz_set_si(f->d, 1);
}

/* The sentinel comparison is field by field, not memcmp: a whole-struct memcmp also reads the
   mantissa limbs and padding that the ordinary init did not define (finding R10 of reviewer
   text). arb_equal and fmpz_equal read only defined fields. */
static int
adele_is_sentinel(const adf_adele_t x, const adf_adele_struct * bytes)
{
    return arb_equal(x->inf, bytes->inf)
           && fmpz_equal(x->fin.A, bytes->fin.A) && fmpz_equal(x->fin.H, bytes->fin.H)
           && fmpz_equal(x->fin.d, bytes->fin.d) && x->fin.backend == bytes->fin.backend
           && x->fin.mctx == bytes->fin.mctx && x->fin.res == bytes->fin.res;
}

static int
cadele_is_sentinel(const adf_cadele_t x, const adf_cadele_struct * bytes)
{
    return arb_equal(acb_realref(x->inf), acb_realref(bytes->inf))
           && arb_equal(acb_imagref(x->inf), acb_imagref(bytes->inf))
           && fmpz_equal(x->fin.A, bytes->fin.A) && fmpz_equal(x->fin.H, bytes->fin.H)
           && fmpz_equal(x->fin.d, bytes->fin.d) && x->fin.backend == bytes->fin.backend
           && x->fin.mctx == bytes->fin.mctx && x->fin.res == bytes->fin.res;
}

/* Parse (s, n) as an adele at prec and check the status and, on a status, the untouched output. On
   ADF_OK the value is left in *x for the caller. Returns the status. */
static int
parse_adele(adf_adele_t x, const char * s, size_t n, slong prec, const adf_text_limits_t * lim,
            int want, const char * where)
{
    adf_adele_struct bytes;
    int st;

    set_sentinel_arb(x->inf);
    set_sentinel_fin(&x->fin);
    memcpy(&bytes, x, sizeof(bytes));
    st = adf_adele_set_str(x, s, n, prec, lim);
    ADF_CHECK_MSG(st == want, "%s: status %d, expected %d", where, st, want);
    if (st != ADF_OK)
        ADF_CHECK_MSG(adele_is_sentinel(x, &bytes), "%s: the output changed on status %d", where, st);
    else
        ADF_CHECK_MSG(arb_is_finite(x->inf) && adf_fball_is_canonical(&x->fin) && x->fin.backend == ADF_GLOBAL,
                      "%s: the result is not canonical", where);
    return st;
}

static int
parse_cadele(adf_cadele_t x, const char * s, size_t n, slong prec, const adf_text_limits_t * lim,
             int want, const char * where)
{
    adf_cadele_struct bytes;
    int st;

    set_sentinel_arb(acb_realref(x->inf));
    set_sentinel_arb(acb_imagref(x->inf));
    set_sentinel_fin(&x->fin);
    memcpy(&bytes, x, sizeof(bytes));
    st = adf_cadele_set_str(x, s, n, prec, lim);
    ADF_CHECK_MSG(st == want, "%s: status %d, expected %d", where, st, want);
    if (st != ADF_OK)
        ADF_CHECK_MSG(cadele_is_sentinel(x, &bytes), "%s: the output changed on status %d", where, st);
    else
        ADF_CHECK_MSG(acb_is_finite(x->inf) && adf_fball_is_canonical(&x->fin) && x->fin.backend == ADF_GLOBAL,
                      "%s: the result is not canonical", where);
    return st;
}

/* The parts of a record of real_parts.jsonl or of text_*.jsonl: part k as (mid, rad). */
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

/* The record of real_parts.jsonl for (file, line), or NULL. */
static const jsonl_value *
find_parts(const jsonl_file * f, const char * file, unsigned long line)
{
    jsonl_error_t err;
    size_t i, l;

    for (i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value * rec = jsonl_record(f, i), * fv, * lv;
        if (jsonl_field(rec, "file", &fv, &err) != 1 || jsonl_field(rec, "line", &lv, &err) != 1)
            continue;
        if (strcmp(jsonl_string(fv, &l, &err), file) == 0
            && strtoul(jsonl_int_text(lv, &err), NULL, 10) == line)
            return rec;
    }
    return NULL;
}

/* Decision M1-D6: 1 if a non-zero binary exponent is above ADF_PRINT_EXP_MAX in absolute value;
   the printer of a value with a real or complex part then returns NULL with *len = 0. */
static int
bin_exp_over(const fmpz_t e)
{
    fmpz_t lim;
    int over;

    fmpz_init_set_ui(lim, ADF_PRINT_EXP_MAX);
    over = fmpz_cmpabs(e, lim) > 0;
    fmpz_clear(lim);
    return over;
}

static int
arb_printable(const arb_t x)
{
    if (!arf_is_zero(arb_midref(x)) && bin_exp_over(ARF_EXPREF(arb_midref(x))))
        return 0;
    if (!mag_is_zero(arb_radref(x)) && bin_exp_over(MAG_EXPREF(arb_radref(x))))
        return 0;
    return 1;
}

/* Check a parsed adele against the expected canonical text E and the exact ball of the input. */
static void
check_adele_value(const adf_adele_t x, const jsonl_value * rec, const char * E, size_t en, slong prec,
                  const char * where)
{
    size_t len, rb, re, fb, fe, erb, ere, efb, efe;
    char * t = adf_adele_get_str(&len, x, ADF_DIGITS_DEFAULT);
    fmpq_t mid, rad;

    fmpq_init(mid);
    fmpq_init(rad);
    if (!arb_printable(x->inf))
    {
        /* M1-D6: the exact-rational reference prints this value, the C printer refuses it before
           any conversion, so the golden text cannot be produced. The stored arb is still checked
           against the exact ball of the vector file. */
        ADF_CHECK_MSG(t == NULL && len == 0, "%s: M1-D6 did not refuse the over-bound value", where);
        if (record_part(rec, 0, mid, rad) && exact_condition(mid, rad, prec))
            ADF_CHECK_MSG(arb_is_exactly(x->inf, mid, rad), "%s: the arb is not exactly the ball", where);
        flint_free(t);
        fmpq_clear(mid);
        fmpq_clear(rad);
        return;
    }
    ADF_CHECK_MSG(well_formed(t, len), "%s: the printed text is not well formed", where);
    if (split_adele(t, len, &rb, &re, &fb, &fe) && split_adele(E, en, &erb, &ere, &efb, &efe))
    {
        ADF_CHECK_MSG(fe - fb == efe - efb && memcmp(t + fb, E + efb, fe - fb) == 0,
                      "%s: finite part \"%s\", expected \"%s\"", where, t, E);
        ADF_CHECK_MSG(record_part(rec, 0, mid, rad), "%s: no exact parts", where);
        check_real_part(x->inf, mid, rad, prec, t + rb, re - rb, E + erb, ere - erb, where);
    }
    else
        ADF_CHECK_MSG(0, "%s: \"%s\" or \"%s\" is not of the form (R ; F)", where, t, E);
    flint_free(t);
    fmpq_clear(mid);
    fmpq_clear(rad);
}

static void
check_cadele_value(const adf_cadele_t x, const jsonl_value * rec, const char * E, size_t en, slong prec,
                   const char * where)
{
    size_t len, sp[6], ep[6];
    char * t = adf_cadele_get_str(&len, x, ADF_DIGITS_DEFAULT);
    fmpq_t mid, rad;

    fmpq_init(mid);
    fmpq_init(rad);
    if (!arb_printable(acb_realref(x->inf)) || !arb_printable(acb_imagref(x->inf)))
    {
        /* M1-D6, as in check_adele_value. */
        ADF_CHECK_MSG(t == NULL && len == 0, "%s: M1-D6 did not refuse the over-bound value", where);
        flint_free(t);
        fmpq_clear(mid);
        fmpq_clear(rad);
        return;
    }
    ADF_CHECK_MSG(well_formed(t, len), "%s: the printed text is not well formed", where);
    if (split_cadele(t, len, sp) && split_cadele(E, en, ep))
    {
        ADF_CHECK_MSG(sp[5] - sp[4] == ep[5] - ep[4] && memcmp(t + sp[4], E + ep[4], sp[5] - sp[4]) == 0,
                      "%s: finite part \"%s\", expected \"%s\"", where, t, E);
        ADF_CHECK_MSG(record_part(rec, 0, mid, rad), "%s: no exact real part", where);
        check_real_part(acb_realref(x->inf), mid, rad, prec, t + sp[0], sp[1] - sp[0], E + ep[0], ep[1] - ep[0],
                        where);
        ADF_CHECK_MSG(record_part(rec, 1, mid, rad), "%s: no exact imaginary part", where);
        check_real_part(acb_imagref(x->inf), mid, rad, prec, t + sp[2], sp[3] - sp[2], E + ep[2], ep[3] - ep[2],
                        where);
    }
    else
        ADF_CHECK_MSG(0, "%s: \"%s\" or \"%s\" is not of the form ((X) + (Y)*i ; F)", where, t, E);
    flint_free(t);
    fmpq_clear(mid);
    fmpq_clear(rad);
}

/* Set an arb exactly to mm 2^me +/- rm 2^re (rm < 2^30), checked. */
static void
set_arb_exact(arb_t x, const fmpz_t mm, slong me, const fmpz_t rm, slong re)
{
    fmpz_t e;
    arf_t t;

    fmpz_init(e);
    arf_init(t);
    fmpz_set_si(e, me);
    arf_set_fmpz_2exp(arb_midref(x), mm, e);
    fmpz_set_si(e, re);
    arf_set_fmpz_2exp(t, rm, e);
    /* arf_get_mag adds a unit to a 30-bit mantissa; mag_set_ui_2exp_si is exact below 2^30
       (probed; the check below makes sure) */
    mag_set_ui_2exp_si(arb_radref(x), fmpz_get_ui(rm), re);
    {
        fmpq_t a, b;
        fmpq_init(a);
        fmpq_init(b);
        mag_get_fmpq(a, arb_radref(x));
        arf_get_fmpq(b, t);
        ADF_CHECK_MSG(fmpq_equal(a, b), "the test could not set the radius exactly");
        fmpq_clear(a);
        fmpq_clear(b);
    }
    arf_clear(t);
    fmpz_clear(e);
}

/* Print the adele (x ; 0) with digits and return the real part as a new string. */
static char *
print_real_part(const arb_t r, slong digits)
{
    adf_adele_t x;
    size_t len, rb, re, fb, fe;
    char * t, * out;

    adele_init(x);
    arb_set(x->inf, r);
    t = adf_adele_get_str(&len, x, digits);
    out = (char *) malloc(len + 32);
    if (split_adele(t, len, &rb, &re, &fb, &fe) && fe - fb == 1 && t[fb] == '0')
    {
        memcpy(out, t + rb, re - rb);
        out[re - rb] = '\0';
    }
    else
        strcpy(out, "(not an adele text)");
    flint_free(t);
    adele_clear(x);
    return out;
}

/* ================================================================== golden files */

static jsonl_file *
open_vectors(const char * path)
{
    jsonl_error_t err;
    jsonl_file * f = NULL;

    ADF_CHECK_MSG(jsonl_open(path, &f, &err) == 1, "%s", jsonl_error_message(&err));
    return f;
}

ADF_TEST(every_row_of_the_golden_file_adele)
{
    golden_error_t err;
    golden_file * f = NULL;
    jsonl_file * parts = open_vectors("tests/ref/vectors/m1-text/real_parts.jsonl");
    size_t i, rows = 0;

    ADF_CHECK_MSG(golden_open("tests/golden/adele.tsv", GOLDEN_DEFAULT_MAX_INPUT, &f, &err) == 1, "%s",
                  golden_error_message(&err));
    if (f == NULL || parts == NULL)
        return;
    for (i = 0; i < golden_count(f); i++)
    {
        const golden_record * r = golden_record_at(f, i);
        adf_adele_t x;
        char where[64];

        snprintf(where, sizeof(where), "adele.tsv line %lu", r->line);
        adele_init(x);
        if (r->is_status)
            parse_adele(x, r->input, r->input_len, GOLDEN_PREC, NULL, status_of_name(r->status), where);
        else if (parse_adele(x, r->input, r->input_len, GOLDEN_PREC, NULL, ADF_OK, where) == ADF_OK)
        {
            const jsonl_value * rec = find_parts(parts, "adele", r->line);
            ADF_CHECK_MSG(rec != NULL, "%s: no exact parts in real_parts.jsonl", where);
            if (rec != NULL)
                check_adele_value(x, rec, r->expected, r->expected_len, GOLDEN_PREC, where);
        }
        adele_clear(x);
        rows++;
    }
    /* tests/golden/README.md: 69 vectors in adele.tsv */
    ADF_CHECK_MSG(rows == 69, "adele.tsv has %zu rows", rows);
    golden_close(f);
    jsonl_close(parts);
}

ADF_TEST(every_row_of_the_golden_file_cadele)
{
    golden_error_t err;
    golden_file * f = NULL;
    jsonl_file * parts = open_vectors("tests/ref/vectors/m1-text/real_parts.jsonl");
    size_t i, rows = 0;

    ADF_CHECK_MSG(golden_open("tests/golden/cadele.tsv", GOLDEN_DEFAULT_MAX_INPUT, &f, &err) == 1, "%s",
                  golden_error_message(&err));
    if (f == NULL || parts == NULL)
        return;
    for (i = 0; i < golden_count(f); i++)
    {
        const golden_record * r = golden_record_at(f, i);
        adf_cadele_t x;
        char where[64];

        snprintf(where, sizeof(where), "cadele.tsv line %lu", r->line);
        cadele_init(x);
        if (r->is_status)
            parse_cadele(x, r->input, r->input_len, GOLDEN_PREC, NULL, status_of_name(r->status), where);
        else if (parse_cadele(x, r->input, r->input_len, GOLDEN_PREC, NULL, ADF_OK, where) == ADF_OK)
        {
            const jsonl_value * rec = find_parts(parts, "cadele", r->line);
            ADF_CHECK_MSG(rec != NULL, "%s: no exact parts in real_parts.jsonl", where);
            if (rec != NULL)
                check_cadele_value(x, rec, r->expected, r->expected_len, GOLDEN_PREC, where);
        }
        cadele_clear(x);
        rows++;
    }
    /* tests/golden/README.md: 16 vectors in cadele.tsv */
    ADF_CHECK_MSG(rows == 16, "cadele.tsv has %zu rows", rows);
    golden_close(f);
    jsonl_close(parts);
}

/* realball_read.tsv: a real ball of the value form -> "lo hi". There is no public real-ball
   parser; each input X is read as the adele "(" X " ; 0)" at prec 128. */
ADF_TEST(every_row_of_the_golden_file_realball_read)
{
    golden_error_t err;
    golden_file * f = NULL;
    jsonl_file * parts = open_vectors("tests/ref/vectors/m1-text/real_parts.jsonl");
    size_t i, rows = 0;

    ADF_CHECK_MSG(golden_open("tests/golden/realball_read.tsv", GOLDEN_DEFAULT_MAX_INPUT, &f, &err) == 1, "%s",
                  golden_error_message(&err));
    if (f == NULL || parts == NULL)
        return;
    for (i = 0; i < golden_count(f); i++)
    {
        const golden_record * r = golden_record_at(f, i);
        adf_adele_t x;
        char where[64];
        char * text = (char *) malloc(r->input_len + 8);
        size_t n = r->input_len + 6;

        memcpy(text, "(", 1);
        memcpy(text + 1, r->input, r->input_len);
        memcpy(text + 1 + r->input_len, " ; 0)", 5);
        snprintf(where, sizeof(where), "realball_read.tsv line %lu", r->line);
        adele_init(x);
        if (r->is_status)
            parse_adele(x, text, n, GOLDEN_PREC, NULL, status_of_name(r->status), where);
        else if (parse_adele(x, text, n, GOLDEN_PREC, NULL, ADF_OK, where) == ADF_OK)
        {
            fmpq_t lo, hi, mid, rad;
            const char * sp = strchr(r->expected, ' ');
            const jsonl_value * rec = find_parts(parts, "realball_read", r->line);

            fmpq_init(lo);
            fmpq_init(hi);
            fmpq_init(mid);
            fmpq_init(rad);
            ADF_CHECK(sp != NULL && rec != NULL);
            if (sp != NULL && rec != NULL)
            {
                char * los = (char *) malloc((size_t) (sp - r->expected) + 1);
                memcpy(los, r->expected, (size_t) (sp - r->expected));
                los[sp - r->expected] = '\0';
                ADF_CHECK(fmpq_set_str(lo, los, 10) == 0 && fmpq_set_str(hi, sp + 1, 10) == 0);
                free(los);
                ADF_CHECK_MSG(arb_contains_fmpq(x->inf, lo) && arb_contains_fmpq(x->inf, hi),
                              "%s: the arb does not contain [%s]", where, r->expected);
                ADF_CHECK(record_part(rec, 0, mid, rad));
                /* the exact parts of the vector file and the golden interval agree */
                fmpq_sub(lo, lo, mid);
                fmpq_neg(lo, lo);
                fmpq_sub(hi, hi, mid);
                ADF_CHECK_MSG(fmpq_equal(lo, rad) && fmpq_equal(hi, rad), "%s: vector files disagree", where);
                if (exact_condition(mid, rad, GOLDEN_PREC))
                    ADF_CHECK_MSG(arb_is_exactly(x->inf, mid, rad), "%s: not exact", where);
            }
            fmpq_clear(lo);
            fmpq_clear(hi);
            fmpq_clear(mid);
            fmpq_clear(rad);
        }
        adele_clear(x);
        free(text);
        rows++;
    }
    /* tests/golden/README.md: 30 vectors in realball_read.tsv */
    ADF_CHECK_MSG(rows == 30, "realball_read.tsv has %zu rows", rows);
    golden_close(f);
    jsonl_close(parts);
}

/* realball_print.tsv: "mid rad digits" (dyadic) -> the text of conventions 9.5 (11.3 item 4). */
ADF_TEST(every_row_of_the_golden_file_realball_print)
{
    golden_error_t err;
    golden_file * f = NULL;
    size_t i, rows = 0;

    ADF_CHECK_MSG(golden_open("tests/golden/realball_print.tsv", GOLDEN_DEFAULT_MAX_INPUT, &f, &err) == 1,
                  "%s", golden_error_message(&err));
    if (f == NULL)
        return;
    for (i = 0; i < golden_count(f); i++)
    {
        const golden_record * r = golden_record_at(f, i);
        char * a = strdup(r->input), * b, * c, * got;
        fmpq_t mid, rad;
        arb_t x;
        fmpz_t mm, rm;
        slong me, re;

        fmpq_init(mid);
        fmpq_init(rad);
        arb_init(x);
        fmpz_init(mm);
        fmpz_init(rm);
        b = strchr(a, ' ');
        c = b != NULL ? strchr(b + 1, ' ') : NULL;
        ADF_CHECK(b != NULL && c != NULL);
        if (b != NULL && c != NULL)
        {
            *b = '\0';
            *c = '\0';
            ADF_CHECK(fmpq_set_str(mid, a, 10) == 0 && fmpq_set_str(rad, b + 1, 10) == 0);
            me = -(slong) fmpz_val2(fmpq_denref(mid));
            re = -(slong) fmpz_val2(fmpq_denref(rad));
            fmpz_set(mm, fmpq_numref(mid));
            fmpz_set(rm, fmpq_numref(rad));
            set_arb_exact(x, mm, me, rm, re);
            got = print_real_part(x, atol(c + 1));
            ADF_CHECK_MSG(strcmp(got, r->expected) == 0, "realball_print.tsv line %lu: printed \"%s\", "
                          "expected \"%s\"", r->line, got, r->expected);
            free(got);
        }
        free(a);
        fmpq_clear(mid);
        fmpq_clear(rad);
        arb_clear(x);
        fmpz_clear(mm);
        fmpz_clear(rm);
        rows++;
    }
    /* tests/golden/README.md: 26 vectors in realball_print.tsv */
    ADF_CHECK_MSG(rows == 26, "realball_print.tsv has %zu rows", rows);
    golden_close(f);
}

/* ================================================================== reference vectors */

ADF_TEST(every_random_print_of_the_reference)
{
    jsonl_file * f = open_vectors("tests/ref/vectors/m1-text/print_real.jsonl");
    jsonl_error_t err;
    size_t i;

    if (f == NULL)
        return;
    ADF_CHECK(jsonl_count(f) >= 1500);
    for (i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value * rec = jsonl_record(f, i), * v;
        fmpz_t mm, rm;
        slong me, re, digits;
        const char * want;
        size_t l;
        arb_t x;
        char * got;

        fmpz_init(mm);
        fmpz_init(rm);
        arb_init(x);
        ADF_CHECK(jsonl_field(rec, "mm", &v, &err) == 1 && fmpz_set_str(mm, jsonl_string(v, &l, &err), 10) == 0);
        ADF_CHECK(jsonl_field(rec, "rm", &v, &err) == 1 && fmpz_set_str(rm, jsonl_string(v, &l, &err), 10) == 0);
        ADF_CHECK(jsonl_field(rec, "me", &v, &err) == 1);
        me = atol(jsonl_int_text(v, &err));
        ADF_CHECK(jsonl_field(rec, "re", &v, &err) == 1);
        re = atol(jsonl_int_text(v, &err));
        ADF_CHECK(jsonl_field(rec, "digits", &v, &err) == 1);
        digits = atol(jsonl_int_text(v, &err));
        ADF_CHECK(jsonl_field(rec, "text", &v, &err) == 1);
        want = jsonl_string(v, &l, &err);
        set_arb_exact(x, mm, me, rm, re);
        got = print_real_part(x, digits);
        ADF_CHECK_MSG(strcmp(got, want) == 0, "print_real.jsonl record %zu: printed \"%s\", expected \"%s\"", i + 1,
                      got, want);
        free(got);
        arb_clear(x);
        fmpz_clear(mm);
        fmpz_clear(rm);
    }
    jsonl_close(f);
}

static void
run_text_vectors(const char * path, int complex_type)
{
    jsonl_file * f = open_vectors(path);
    jsonl_error_t err;
    size_t i;

    if (f == NULL)
        return;
    ADF_CHECK(jsonl_count(f) == 1200);
    for (i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value * rec = jsonl_record(f, i), * h, * e;
        const char * hex, * exp;
        size_t hlen, elen, n;
        unsigned char * b;
        char where[96];
        int want;

        ADF_CHECK(jsonl_field(rec, "hex", &h, &err) == 1);
        ADF_CHECK(jsonl_field(rec, "expected", &e, &err) == 1);
        hex = jsonl_string(h, &hlen, &err);
        exp = jsonl_string(e, &elen, &err);
        b = hex_decode(hex, hlen, &n);
        snprintf(where, sizeof(where), "%s record %zu", path, i + 1);
        want = exp[0] == '!' ? status_of_name(exp + 1) : ADF_OK;
        if (!complex_type)
        {
            adf_adele_t x;
            adele_init(x);
            if (parse_adele(x, (const char *) b, n, GOLDEN_PREC, NULL, want, where) == ADF_OK && want == ADF_OK)
                check_adele_value(x, rec, exp, elen, GOLDEN_PREC, where);
            adele_clear(x);
        }
        else
        {
            adf_cadele_t x;
            cadele_init(x);
            if (parse_cadele(x, (const char *) b, n, GOLDEN_PREC, NULL, want, where) == ADF_OK && want == ADF_OK)
                check_cadele_value(x, rec, exp, elen, GOLDEN_PREC, where);
            cadele_clear(x);
        }
        free(b);
    }
    jsonl_close(f);
}

ADF_TEST(every_random_adele_text_of_the_reference)
{
    run_text_vectors("tests/ref/vectors/m1-text/text_adele.jsonl", 0);
}

ADF_TEST(every_random_cadele_text_of_the_reference)
{
    run_text_vectors("tests/ref/vectors/m1-text/text_cadele.jsonl", 1);
}

/* ================================================================== reading: exactness, enclosure */

/* The decimal text of m 2^k (exact), into buf. */
static void
dyadic_text(char * buf, size_t cap, const fmpz_t m, slong k)
{
    fmpz_t D;
    char * s;
    size_t n;

    fmpz_init(D);
    if (k >= 0)
    {
        fmpz_mul_2exp(D, m, (ulong) k);
        s = fmpz_get_str(NULL, 10, D);
        snprintf(buf, cap, "%s", s);
    }
    else
    {
        /* m 2^k = m 5^(-k) 10^k */
        fmpz_set_ui(D, 5);
        fmpz_pow_ui(D, D, (ulong) -k);
        fmpz_mul(D, D, m);
        s = fmpz_get_str(NULL, 10, D);
        n = strlen(s);
        snprintf(buf, cap, "%se%ld", s, (long) k);
        (void) n;
    }
    flint_free(s);
    fmpz_clear(D);
}

ADF_TEST(a_dyadic_ball_is_read_exactly_at_every_prec)
{
    const slong precs[] = {2, 3, 10, 30, 53, 64, 128, 300};
    flint_rand_t st;
    size_t pi;
    int k;

    flint_randinit(st);
    for (pi = 0; pi < sizeof(precs) / sizeof(precs[0]); pi++)
    {
        slong p = precs[pi];
        for (k = 0; k < 60; k++)
        {
            fmpz_t m, r;
            slong e = (slong) n_randint(st, 200) - 100, re = e - (slong) n_randint(st, 60);
            char * text = (char *) malloc(4000);
            char ms[1800], rs[1800];
            fmpq_t mid, rad, two;
            adf_adele_t x;
            ulong bits = (k % 2 == 0) ? (ulong) p : 1 + n_randint(st, (ulong) p);
            size_t n;

            fmpz_init(m);
            fmpz_init(r);
            fmpq_init(mid);
            fmpq_init(rad);
            fmpq_init(two);
            /* an odd midpoint mantissa of exactly `bits` bits, a radius mantissa below 2^30 */
            fmpz_one(m);
            fmpz_mul_2exp(m, m, bits - 1);
            {
                fmpz_t t;
                fmpz_init(t);
                fmpz_randbits(t, st, bits - 1);
                fmpz_abs(t, t);
                fmpz_add(m, m, t);
                fmpz_clear(t);
            }
            if (fmpz_is_even(m))
                fmpz_add_ui(m, m, 1);
            if (fmpz_bits(m) > bits)
                fmpz_sub_ui(m, m, 2);
            if (k % 3 == 0)
                fmpz_neg(m, m);
            fmpz_set_ui(r, (k % 5 == 0) ? 0 : (n_randint(st, (1UL << 30) - 1) | 1));
            dyadic_text(ms, sizeof(ms), m, e);
            dyadic_text(rs, sizeof(rs), r, re);
            n = (size_t) snprintf(text, 4000, "(%s +/- %s ; 0)", ms, rs);
            fmpq_set_fmpz(mid, m);
            fmpq_set_fmpz(rad, r);
            fmpz_one(fmpq_numref(two));
            fmpz_mul_2exp(fmpq_numref(two), fmpq_numref(two), (ulong) (e < 0 ? -e : e));
            if (e < 0)
                fmpq_inv(two, two);
            fmpq_mul(mid, mid, two);
            fmpz_one(fmpq_numref(two));
            fmpz_one(fmpq_denref(two));
            fmpz_mul_2exp(fmpq_numref(two), fmpq_numref(two), (ulong) (re < 0 ? -re : re));
            if (re < 0)
                fmpq_inv(two, two);
            fmpq_mul(rad, rad, two);
            adele_init(x);
            if (parse_adele(x, text, n, p, NULL, ADF_OK, text) == ADF_OK)
            {
                ADF_CHECK_MSG(arb_is_exactly(x->inf, mid, rad), "prec %ld: %s is not read exactly", (long) p, text);
                ADF_CHECK_MSG(exact_condition(mid, rad, p), "the test's own ball is not exact");
            }
            /* one bit more than prec: contained, and the midpoint has at most prec bits */
            if (fmpz_bits(m) == (ulong) p)
            {
                fmpz_mul_2exp(m, m, 1);
                fmpz_add_ui(m, m, 1);
                dyadic_text(ms, sizeof(ms), m, e - 1);
                n = (size_t) snprintf(text, 4000, "(%s ; 0)", ms);
                fmpq_set_fmpz(mid, m);
                fmpz_one(fmpq_numref(two));
                fmpz_one(fmpq_denref(two));
                fmpz_mul_2exp(fmpq_numref(two), fmpq_numref(two), (ulong) (e - 1 < 0 ? 1 - e : e - 1));
                if (e - 1 < 0)
                    fmpq_inv(two, two);
                fmpq_mul(mid, mid, two);
                fmpq_zero(rad);
                if (parse_adele(x, text, n, p, NULL, ADF_OK, text) == ADF_OK)
                {
                    ADF_CHECK_MSG(arb_contains_interval(x->inf, mid, rad) && !arb_is_exact(x->inf),
                                  "prec %ld: %s", (long) p, text);
                    ADF_CHECK_MSG(arf_bits(arb_midref(x->inf)) <= p, "prec %ld: %s: midpoint of %ld bits", (long) p,
                                  text, (long) arf_bits(arb_midref(x->inf)));
                }
            }
            adele_clear(x);
            free(text);
            fmpz_clear(m);
            fmpz_clear(r);
            fmpq_clear(mid);
            fmpq_clear(rad);
            fmpq_clear(two);
        }
    }
    flint_randclear(st);
}

ADF_TEST(the_radius_mantissa_bound_of_2_to_the_30)
{
    adf_adele_t x;
    fmpq_t mid, rad;
    char text[64];
    size_t n;

    adele_init(x);
    fmpq_init(mid);
    fmpq_init(rad);
    /* 2^30 - 1 = 1073741823: exact */
    n = (size_t) snprintf(text, sizeof(text), "(1 +/- 1073741823 ; 0)");
    fmpq_set_si(mid, 1, 1);
    fmpq_set_si(rad, 1073741823, 1);
    if (parse_adele(x, text, n, 64, NULL, ADF_OK, text) == ADF_OK)
        ADF_CHECK(arb_is_exactly(x->inf, mid, rad));
    /* 2^30 + 1: not representable in a mag; contained */
    n = (size_t) snprintf(text, sizeof(text), "(1 +/- 1073741825 ; 0)");
    fmpq_set_si(rad, 1073741825, 1);
    if (parse_adele(x, text, n, 64, NULL, ADF_OK, text) == ADF_OK)
        ADF_CHECK(arb_contains_interval(x->inf, mid, rad) && !arb_is_exactly(x->inf, mid, rad));
    /* 0.5 +/- 0.25 at the smallest prec */
    n = (size_t) snprintf(text, sizeof(text), "(-0.5 +/- 0.25 ; 0)");
    fmpq_set_si(mid, -1, 2);
    fmpq_set_si(rad, 1, 4);
    if (parse_adele(x, text, n, 2, NULL, ADF_OK, text) == ADF_OK)
        ADF_CHECK(arb_is_exactly(x->inf, mid, rad));
    fmpq_clear(mid);
    fmpq_clear(rad);
    adele_clear(x);
}

ADF_TEST(random_decimals_are_enclosed_at_small_prec)
{
    const slong precs[] = {1, 2, 3, 5, 8, 20, 64, 200};
    flint_rand_t st;
    int k;

    flint_randinit(st);
    for (k = 0; k < 3000; k++)
    {
        slong p = precs[k % 8];
        char text[512], mtxt[200], rtxt[200];
        size_t n, i;
        fmpq_t mid, rad;
        adf_adele_t x;
        int j, len;

        fmpq_init(mid);
        fmpq_init(rad);
        /* [-]digits[.digits][e[+-]x] */
        len = 0;
        if (n_randint(st, 2))
            mtxt[len++] = '-';
        for (j = 0; j < 1 + (int) n_randint(st, 30); j++)
            mtxt[len++] = (char) ('0' + n_randint(st, 10));
        if (n_randint(st, 2))
        {
            mtxt[len++] = '.';
            for (j = 0; j < 1 + (int) n_randint(st, 30); j++)
                mtxt[len++] = (char) ('0' + n_randint(st, 10));
        }
        if (n_randint(st, 2))
            len += snprintf(mtxt + len, 40, "e%s%d", n_randint(st, 2) ? "-" : "", (int) n_randint(st, 400));
        mtxt[len] = '\0';
        len = 0;
        for (j = 0; j < 1 + (int) n_randint(st, 12); j++)
            rtxt[len++] = (char) ('0' + n_randint(st, 10));
        if (n_randint(st, 2))
            len += snprintf(rtxt + len, 40, "e-%d", (int) n_randint(st, 420));
        rtxt[len] = '\0';
        n = (size_t) snprintf(text, sizeof(text), "(%s +/- %s ; 0)", mtxt, rtxt);
        i = 0;
        ADF_CHECK(read_decimal(mid, mtxt, strlen(mtxt), &i));
        i = 0;
        ADF_CHECK(read_decimal(rad, rtxt, strlen(rtxt), &i));
        adele_init(x);
        if (parse_adele(x, text, n, p, NULL, ADF_OK, text) == ADF_OK)
        {
            ADF_CHECK_MSG(arb_contains_interval(x->inf, mid, rad), "prec %ld: %s not enclosed", (long) p, text);
            ADF_CHECK_MSG(arf_bits(arb_midref(x->inf)) <= (p < 2 ? 2 : p), "prec %ld: %s: midpoint of %ld bits",
                          (long) p, text, (long) arf_bits(arb_midref(x->inf)));
            if (exact_condition(mid, rad, p < 2 ? 2 : p))
                ADF_CHECK_MSG(arb_is_exactly(x->inf, mid, rad), "prec %ld: %s not exact", (long) p, text);
        }
        adele_clear(x);
        fmpq_clear(mid);
        fmpq_clear(rad);
    }
    flint_randclear(st);
}

ADF_TEST(the_extreme_exponents_of_the_default_limits)
{
    adf_adele_t x;
    fmpq_t mid, rad;
    char * t;
    size_t len;

    adele_init(x);
    fmpq_init(mid);
    fmpq_init(rad);
    pow10_fmpq(mid, 100000);
    if (parse_adele(x, "(1e100000 ; 0)", 14, 128, NULL, ADF_OK, "1e100000") == ADF_OK)
    {
        ADF_CHECK(arb_contains_interval(x->inf, mid, rad));
        /* the binary exponent is about 332193, above ADF_PRINT_EXP_MAX = 100000: M1-D6 refuses */
        t = adf_adele_get_str(&len, x, ADF_DIGITS_DEFAULT);
        ADF_CHECK_MSG(t == NULL && len == 0, "M1-D6 did not refuse 1e100000");
        flint_free(t);
    }
    pow10_fmpq(mid, -100000);
    if (parse_adele(x, "(-1e-100000 +/- 1e-100000 ; 0)", 30, 128, NULL, ADF_OK, "-1e-100000") == ADF_OK)
    {
        fmpq_set(rad, mid);
        fmpq_neg(mid, mid);
        ADF_CHECK(arb_contains_interval(x->inf, mid, rad));
        t = adf_adele_get_str(&len, x, ADF_DIGITS_DEFAULT);
        ADF_CHECK_MSG(t == NULL && len == 0, "M1-D6 did not refuse 1e-100000");
        flint_free(t);
    }
    fmpq_clear(mid);
    fmpq_clear(rad);
    adele_clear(x);
}

/* ================================================================== printing */

ADF_TEST(the_printers_write_the_templates)
{
    adf_adele_t x;
    adf_cadele_t z;
    size_t len;
    char * t;

    adele_init(x);
    t = adf_adele_get_str(&len, x, ADF_DIGITS_DEFAULT);
    ADF_CHECK(strcmp(t, "(0 ; 0)") == 0 && len == 7);
    flint_free(t);
    arb_set_si(x->inf, -5);
    arb_mul_2exp_si(x->inf, x->inf, -1);
    mag_set_ui_2exp_si(arb_radref(x->inf), 1, -2);
    fmpz_set_si(x->fin.A, 5);
    fmpz_set_si(x->fin.H, 18);
    fmpz_set_si(x->fin.d, 3);
    t = adf_adele_get_str(&len, x, ADF_DIGITS_DEFAULT);
    ADF_CHECK_MSG(strcmp(t, "(-2.5 +/- 0.25 ; 5/3 mod 6)") == 0, "%s", t);
    flint_free(t);
    adele_clear(x);

    cadele_init(z);
    t = adf_cadele_get_str(&len, z, ADF_DIGITS_DEFAULT);
    ADF_CHECK_MSG(strcmp(t, "((0) + (0)*i ; 0)") == 0 && len == 17, "%s", t);
    flint_free(t);
    arb_set_si(acb_realref(z->inf), 3);
    arb_mul_2exp_si(acb_realref(z->inf), acb_realref(z->inf), -1);
    arb_set_si(acb_imagref(z->inf), -2);
    mag_set_ui_2exp_si(arb_radref(acb_imagref(z->inf)), 1, -4);
    fmpz_set_si(z->fin.A, -7);
    fmpz_set_si(z->fin.d, 3);
    t = adf_cadele_get_str(&len, z, ADF_DIGITS_DEFAULT);
    ADF_CHECK_MSG(strcmp(t, "((1.5) + (-2 +/- 0.063)*i ; -7/3)") == 0, "%s", t);
    flint_free(t);
    cadele_clear(z);
}

ADF_TEST(the_digits_argument)
{
    arb_t r;
    char * t;

    arb_init(r);
    /* 1/3 at 128 bits: its radius limits the digits whatever n is */
    arb_set_ui(r, 1);
    arb_div_ui(r, r, 3, 128);
    t = print_real_part(r, 1);
    ADF_CHECK_MSG(strcmp(t, "0.3 +/- 0.034") == 0, "%s", t);
    free(t);
    t = print_real_part(r, ADF_DIGITS_MAX);
    ADF_CHECK_MSG(strncmp(t, "0.3333333333333333333333333333333333333", 39) == 0, "%s", t);
    free(t);
    /* an exact dyadic point with 54 significant digits prints fully at n >= 54, rounded below */
    arb_set_si(r, 1);
    arb_mul_2exp_si(r, r, -54);
    t = print_real_part(r, 54);
    ADF_CHECK_MSG(strcmp(t, "5.5511151231257827021181583404541015625e-17") == 0, "%s", t);
    free(t);
    t = print_real_part(r, ADF_DIGITS_MAX);
    ADF_CHECK_MSG(strcmp(t, "5.5511151231257827021181583404541015625e-17") == 0, "%s", t);
    free(t);
    t = print_real_part(r, 3);
    ADF_CHECK_MSG(strcmp(t, "5.55e-17 +/- 1.2e-20") == 0, "%s", t);
    free(t);
    arb_clear(r);
}

/* ================================================================== round trips (9.6) */

ADF_TEST(print_then_parse_encloses_the_value)
{
    flint_rand_t st;
    int k;

    flint_randinit(st);
    for (k = 0; k < 1500; k++)
    {
        adf_adele_t x, y;
        adf_cadele_t z, w;
        size_t len;
        char * t;
        slong digits = (k % 4 == 0) ? 1 + (slong) n_randint(st, 40) : ADF_DIGITS_DEFAULT;

        adele_init(x);
        adele_init(y);
        arb_randtest(x->inf, st, 1 + n_randint(st, 300), 1 + n_randint(st, 12));
        {
            fmpz_t A, H, d;
            fmpz_init(A);
            fmpz_init(H);
            fmpz_init(d);
            fmpz_randtest(A, st, 100);
            fmpz_randtest_unsigned(H, st, 100);
            fmpz_randtest_not_zero(d, st, 50);
            ADF_CHECK(adf_fball_set_fmpz3(&x->fin, A, H, d) == ADF_OK);
            fmpz_clear(A);
            fmpz_clear(H);
            fmpz_clear(d);
        }
        t = adf_adele_get_str(&len, x, digits);
        ADF_CHECK(well_formed(t, len));
        if (parse_adele(y, t, len, 128, NULL, ADF_OK, t) == ADF_OK)
        {
            ADF_CHECK_MSG(arb_contains(y->inf, x->inf), "\"%s\" does not enclose the value", t);
            ADF_CHECK_MSG(adf_fball_identical(&y->fin, &x->fin), "\"%s\": the finite part changed", t);
        }
        flint_free(t);

        cadele_init(z);
        cadele_init(w);
        acb_randtest(z->inf, st, 1 + n_randint(st, 200), 1 + n_randint(st, 12));
        adf_fball_set(&z->fin, &x->fin);
        t = adf_cadele_get_str(&len, z, digits);
        ADF_CHECK(well_formed(t, len));
        if (parse_cadele(w, t, len, 128, NULL, ADF_OK, t) == ADF_OK)
        {
            ADF_CHECK_MSG(acb_contains(w->inf, z->inf), "\"%s\" does not enclose the value", t);
            ADF_CHECK_MSG(adf_fball_identical(&w->fin, &z->fin), "\"%s\": the finite part changed", t);
        }
        flint_free(t);
        adele_clear(x);
        adele_clear(y);
        cadele_clear(z);
        cadele_clear(w);
    }
    flint_randclear(st);
}

ADF_TEST(parse_print_parse_encloses_the_first_value)
{
    const char * texts[] = {"(1 +/- 0.13 ; 0)", "(0.1 ; 1/3 mod 1)", "(3.14159265358979323846 +/- 1e-25 ; 0)",
                            "(-1e-30 +/- 1e-31 ; 2 mod 6)", "(12345678901234567890123 ; 0)"};
    size_t k;

    for (k = 0; k < sizeof(texts) / sizeof(texts[0]); k++)
    {
        adf_adele_t x, y;
        size_t len;
        char * t;
        int pass;

        adele_init(x);
        adele_init(y);
        ADF_CHECK(adf_adele_set_str(x, texts[k], strlen(texts[k]), 128, NULL) == ADF_OK);
        arb_set(y->inf, x->inf);
        adf_fball_set(&y->fin, &x->fin);
        for (pass = 0; pass < 4; pass++)
        {
            t = adf_adele_get_str(&len, y, ADF_DIGITS_DEFAULT);
            ADF_CHECK(adf_adele_set_str(y, t, len, 128, NULL) == ADF_OK);
            ADF_CHECK_MSG(arb_contains(y->inf, x->inf), "pass %d of %s: \"%s\"", pass, texts[k], t);
            ADF_CHECK(adf_fball_identical(&y->fin, &x->fin));
            flint_free(t);
        }
        adele_clear(x);
        adele_clear(y);
    }
}

/* ================================================================== limits, hostile input, order */

ADF_TEST(max_exp10_at_the_limit_and_one_above)
{
    adf_text_limits_t lim;
    adf_adele_t x;
    adf_cadele_t z;

    adf_text_limits_default(&lim);
    lim.max_exp10 = 7;
    adele_init(x);
    cadele_init(z);
    parse_adele(x, "(1e7 ; 0)", 9, 64, &lim, ADF_OK, "1e7");
    parse_adele(x, "(1e8 ; 0)", 9, 64, &lim, ADF_LIMIT, "1e8");
    parse_adele(x, "(1e-7 ; 0)", 10, 64, &lim, ADF_OK, "1e-7");
    parse_adele(x, "(1e-8 ; 0)", 10, 64, &lim, ADF_LIMIT, "1e-8");
    parse_adele(x, "(1E+0000007 ; 0)", 16, 64, &lim, ADF_OK, "1E+0000007");
    parse_adele(x, "(1 +/- 1e7 ; 0)", 15, 64, &lim, ADF_OK, "radius 1e7");
    parse_adele(x, "(1 +/- 1e8 ; 0)", 15, 64, &lim, ADF_LIMIT, "radius 1e8");
    parse_adele(x, "(1 +/- 1e-0000008 ; 0)", 22, 64, &lim, ADF_LIMIT, "radius 1e-8");
    parse_adele(x, "(12345678.5 ; 0)", 16, 64, &lim, ADF_OK, "no exponent");
    parse_cadele(z, "((1e7) + (1e-7)*i ; 0)", 22, 64, &lim, ADF_OK, "cadele at the limit");
    parse_cadele(z, "((1e8) + (0)*i ; 0)", 19, 64, &lim, ADF_LIMIT, "cadele real mid");
    parse_cadele(z, "((0 +/- 1e8) + (0)*i ; 0)", 25, 64, &lim, ADF_LIMIT, "cadele real rad");
    parse_cadele(z, "((0) + (1e8)*i ; 0)", 19, 64, &lim, ADF_LIMIT, "cadele imag mid");
    parse_cadele(z, "((0) + (0 +/- 1e-8)*i ; 0)", 26, 64, &lim, ADF_LIMIT, "cadele imag rad");
    lim.max_exp10 = 0;
    parse_adele(x, "(1e0 ; 0)", 9, 64, &lim, ADF_OK, "1e0 at max 0");
    parse_adele(x, "(1e1 ; 0)", 9, 64, &lim, ADF_LIMIT, "1e1 at max 0");
    lim.max_exp10 = -1;
    parse_adele(x, "(1e0 ; 0)", 9, 64, &lim, ADF_LIMIT, "1e0 at max -1");
    parse_adele(x, "(1 ; 0)", 7, 64, &lim, ADF_OK, "no exponent at max -1");
    /* M1-D7 (finding R4): the exponent is compared with max_exp10 as a number of any length, with
       no hidden 18-digit bound; a zero coefficient is the exact zero and forms no power of ten. */
    lim.max_exp10 = WORD_MAX;
    parse_adele(x, "(0e1000000000000000000 ; 0)", 27, 64, &lim, ADF_OK, "19-digit exponent, zero");
    parse_adele(x, "(0e10000000000000000000 ; 0)", 28, 64, &lim, ADF_LIMIT, "20-digit exponent");
    parse_adele(x, "(1e000000000000000000000 ; 0)", 29, 64, &lim, ADF_OK, "zero exponent with many zeros");
    adele_clear(x);
    cadele_clear(z);
}

ADF_TEST(max_len_at_the_limit_and_one_above)
{
    adf_text_limits_t lim;
    adf_adele_t x;
    adf_cadele_t z;

    adf_text_limits_default(&lim);
    lim.max_len = 7;
    adele_init(x);
    cadele_init(z);
    parse_adele(x, "(1 ; 0)", 7, 64, &lim, ADF_OK, "7 bytes");
    parse_adele(x, "(1 ; 0) ", 8, 64, &lim, ADF_LIMIT, "8 bytes");
    lim.max_len = 17;
    parse_cadele(z, "((1) + (0)*i ; 0)", 17, 64, &lim, ADF_OK, "17 bytes");
    parse_cadele(z, " ((1) + (0)*i ; 0)", 18, 64, &lim, ADF_LIMIT, "18 bytes");
    adele_clear(x);
    cadele_clear(z);
}

ADF_TEST(empty_null_nul_bytes_and_ends_of_memory)
{
    const char * t = "(1.5 +/- 0.25 ; 1 mod 6)";
    const char * c = "((1) + (2)*i ; 0)";
    size_t n = strlen(t), cn = strlen(c), pos;
    adf_adele_t x;
    adf_cadele_t z;
    long page = sysconf(_SC_PAGESIZE);
    char * m;
    int b;

    adele_init(x);
    cadele_init(z);
    parse_adele(x, NULL, 0, 64, NULL, ADF_PARSE, "NULL");
    parse_cadele(z, NULL, 0, 64, NULL, ADF_PARSE, "NULL");
    parse_adele(x, t, 0, 64, NULL, ADF_PARSE, "len 0");
    for (pos = 0; pos <= n; pos++)
    {
        char buf[64];
        memcpy(buf, t, pos);
        buf[pos] = '\0';
        memcpy(buf + pos + 1, t + pos, n - pos);
        parse_adele(x, buf, n + 1, 64, NULL, ADF_PARSE, "NUL inside");
    }
    for (pos = 0; pos <= cn; pos++)
    {
        char buf[64];
        memcpy(buf, c, pos);
        buf[pos] = '\0';
        memcpy(buf + pos + 1, c + pos, cn - pos);
        parse_cadele(z, buf, cn + 1, 64, NULL, ADF_PARSE, "NUL inside");
    }
    for (b = 0; b < 256; b++)
    {
        char buf[64];
        if ((b >= 0x20 && b <= 0x7e) || b == 9 || b == 10 || b == 13)
            continue;
        memcpy(buf, t, n);
        buf[n - 1] = (char) b;       /* in place of the closing parenthesis */
        parse_adele(x, buf, n, 64, NULL, ADF_PARSE, "forbidden byte");
        memcpy(buf, c, cn);
        buf[3] = (char) b;
        parse_cadele(z, buf, cn, 64, NULL, ADF_PARSE, "forbidden byte");
    }
    m = (char *) mmap(NULL, 2 * (size_t) page, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    ADF_CHECK(m != MAP_FAILED);
    if (m != MAP_FAILED)
    {
        const char * texts[] = {"(1.5 +/- 0.25 ; 1 mod 6)", "(1.5 +/- 0.25 ; 1 mod 6", "(1.5e", "(1.", "(1 +/",
                                "(1 +/- 0.1 ; 1/", "((1) + (2)*i ; 0)", "((1) + (2)*", "((1) + (2)*i"};
        const int want[] = {ADF_OK, ADF_PARSE, ADF_PARSE, ADF_PARSE, ADF_PARSE, ADF_PARSE, ADF_OK, ADF_PARSE,
                            ADF_PARSE};
        size_t k;

        ADF_CHECK(mprotect(m + page, (size_t) page, PROT_NONE) == 0);
        for (k = 0; k < sizeof(texts) / sizeof(texts[0]); k++)
        {
            size_t l = strlen(texts[k]);
            char * h = (char *) malloc(l);
            memcpy(h, texts[k], l);
            memcpy(m + page - l, texts[k], l);
            if (k < 6)
            {
                parse_adele(x, h, l, 64, NULL, want[k], texts[k]);
                parse_adele(x, m + page - l, l, 64, NULL, want[k], texts[k]);
            }
            else
            {
                parse_cadele(z, h, l, 64, NULL, want[k], texts[k]);
                parse_cadele(z, m + page - l, l, 64, NULL, want[k], texts[k]);
            }
            free(h);
        }
        munmap(m, 2 * (size_t) page);
    }
    adele_clear(x);
    cadele_clear(z);
}

ADF_TEST(the_earlier_stage_decides_the_status)
{
    adf_text_limits_t lim;
    adf_adele_t x;
    adf_cadele_t z;

    adele_init(x);
    cadele_init(z);
    adf_text_limits_default(&lim);
    lim.max_len = 16;
    /* stage 1 against 2, 3, 4 and 6 (17 bytes each) */
    parse_adele(x, "(1 ; 0)\x80        ", 16 + 1, 64, &lim, ADF_LIMIT, "1 vs 2");
    parse_adele(x, "(1 ; 0          ", 16 + 1, 64, &lim, ADF_LIMIT, "1 vs 3");
    parse_adele(x, "(1e100001 ; 0)  ", 16 + 1, 64, &lim, ADF_LIMIT, "1 vs 4");
    parse_adele(x, "(1 ; 1/0)       ", 16 + 1, 64, &lim, ADF_LIMIT, "1 vs 6");
    /* stage 2 against 3, 4 and 6: the forbidden byte is the last byte */
    parse_adele(x, "(1 ; 0\x80", 7, 64, NULL, ADF_PARSE, "2 vs 3");
    parse_adele(x, "(1e100001 ; 0)\xff", 15, 64, NULL, ADF_PARSE, "2 vs 4");
    parse_adele(x, "(1 ; 1/0)\x01", 10, 64, NULL, ADF_PARSE, "2 vs 6");
    parse_cadele(z, "((1e100001) + (0)*i ; 0)\x80", 25, 64, NULL, ADF_PARSE, "2 vs 4, cadele");
    /* stage 3 against 4 and 6: the grammar fault is after the other fault */
    parse_adele(x, "(1e100001 ; 0", 13, 64, NULL, ADF_PARSE, "3 vs 4");
    parse_adele(x, "(1 ; 1/0) + Q", 13, 64, NULL, ADF_PARSE, "3 vs 6");
    parse_cadele(z, "((1) + (1e100001)*i ; 1/0", 25, 64, NULL, ADF_PARSE, "3 vs 4 and 6, cadele");
    /* stage 4 against 6 */
    parse_adele(x, "(1e100001 ; 1/0)", 16, 64, NULL, ADF_LIMIT, "4 vs 6");
    parse_adele(x, "(1 +/- 1e-100001 ; 1 mod 1/0)", 29, 64, NULL, ADF_LIMIT, "4 vs 6, radius");
    parse_cadele(z, "((1) + (1e100001)*i ; 1/0)", 26, 64, NULL, ADF_LIMIT, "4 vs 6, cadele");
    /* stage 6 alone */
    parse_adele(x, "(1 ; 1 mod 1/0)", 15, 64, NULL, ADF_DOMAIN, "6");
    parse_cadele(z, "((1) + (1)*i ; 1/00)", 20, 64, NULL, ADF_DOMAIN, "6, cadele");
    adele_clear(x);
    cadele_clear(z);
}

ADF_TEST(the_grammar_of_real_and_complex)
{
    adf_adele_t x;
    adf_cadele_t z;

    adele_init(x);
    cadele_init(z);
    parse_adele(x, "(1+/-1;0)", 9, 64, NULL, ADF_OK, "no spaces");
    parse_adele(x, "(1 + /- 1 ; 0)", 14, 64, NULL, ADF_PARSE, "split +/-");
    parse_adele(x, "(1 +/- -1 ; 0)", 14, 64, NULL, ADF_PARSE, "negative radius");
    parse_adele(x, "(1 +/- 1/2 ; 0)", 15, 64, NULL, ADF_PARSE, "fraction radius");
    parse_adele(x, "(1/2 ; 0)", 9, 64, NULL, ADF_PARSE, "fraction midpoint");
    parse_adele(x, "(1.5E3 ; 0)", 11, 64, NULL, ADF_OK, "E");
    parse_adele(x, "(1.5e+3 ; 0)", 12, 64, NULL, ADF_OK, "e+");
    parse_adele(x, "(1.5e-3 ; 0)", 12, 64, NULL, ADF_OK, "e-");
    parse_adele(x, "(1.5e ; 0)", 10, 64, NULL, ADF_PARSE, "e without digits");
    parse_adele(x, "(1.5e+ ; 0)", 11, 64, NULL, ADF_PARSE, "e+ without digits");
    parse_adele(x, "(1. ; 0)", 8, 64, NULL, ADF_PARSE, "point without digits");
    parse_adele(x, "(1.e5 ; 0)", 10, 64, NULL, ADF_PARSE, "point then e");
    parse_adele(x, "(.5 ; 0)", 8, 64, NULL, ADF_PARSE, "no integer digits");
    parse_adele(x, "(-.5 ; 0)", 9, 64, NULL, ADF_PARSE, "minus point");
    parse_adele(x, "(1 ; 0 mod 1e2)", 15, 64, NULL, ADF_PARSE, "decimal radius of fin");
    parse_adele(x, "(1 ; 0.5)", 9, 64, NULL, ADF_PARSE, "decimal fin");
    parse_adele(x, "(1 ; 0) x", 9, 64, NULL, ADF_PARSE, "trailing");
    parse_adele(x, "(1 ; 0))", 8, 64, NULL, ADF_PARSE, "extra parenthesis");
    parse_adele(x, "1 ; 0", 5, 64, NULL, ADF_PARSE, "no parentheses");
    parse_adele(x, "(1 ; 0 mod)", 11, 64, NULL, ADF_PARSE, "mod without radius");
    parse_adele(x, "((1) + (0)*i ; 0)", 17, 64, NULL, ADF_PARSE, "a cadele is not an adele");
    parse_cadele(z, "(1 ; 0)", 7, 64, NULL, ADF_PARSE, "an adele is not a cadele");
    parse_cadele(z, "((1) + (0)*i;0)", 15, 64, NULL, ADF_OK, "no spaces");
    parse_cadele(z, "((1)+(0)*ii ; 0)", 16, 64, NULL, ADF_PARSE, "ii");
    parse_cadele(z, "((1)+(0)*i2 ; 0)", 16, 64, NULL, ADF_PARSE, "i2");
    parse_cadele(z, "((1)+(0)* i ; 0)", 16, 64, NULL, ADF_OK, "space before i");
    parse_cadele(z, "((1)+/-(0)*i ; 0)", 17, 64, NULL, ADF_PARSE, "+/- for +");
    parse_cadele(z, "((1 +/- 1)+(0 +/- 2)*i ; 0)", 27, 64, NULL, ADF_OK, "radii");
    parse_cadele(z, "((1)+(0) ; 0)", 13, 64, NULL, ADF_PARSE, "no *i");
    parse_cadele(z, "((1)+(0)* ; 0)", 14, 64, NULL, ADF_PARSE, "no i");
    parse_cadele(z, "((1)(0)*i ; 0)", 14, 64, NULL, ADF_PARSE, "no +");
    parse_cadele(z, "((1)+0*i ; 0)", 13, 64, NULL, ADF_PARSE, "no parentheses");
    parse_cadele(z, "((1)+(0)*i ; 0 mod 3", 20, 64, NULL, ADF_PARSE, "open");
    adele_clear(x);
    cadele_clear(z);
}
