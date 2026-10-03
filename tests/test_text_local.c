/* tests/test_text_local.c: the value form of adf_lball and adf_sball: adf_lball_set_str, adf_lball_get_str,
   adf_sball_set_str, adf_sball_get_str (lane t-slice2).

   Contract: include/adelefeld/text.h (the block "adf_lball, adf_sball"); docs/conventions.md 5.8 (a local ball:
   struct, predicate, the canonical centre), 5.9 (a partial ball: struct, predicate, the order of the places), 7
   (places, canonical order), 8.1 to 8.5 (interface, alphabet, tokens, limits, order of checks), 9.2 (lball_v,
   lcoord, sball_v, sentry), 9.3 (semantic constraints and canonicalisation on input), 9.4 (templates), 9.5
   (reading and printing of real balls), 9.6 (round trips), 9.7 (the type of a text), 11.3 (how a C test uses the
   golden files, prec = 128); docs/api-1f.md L1 (the canonical centre, adf_lball_set_rat_ball).

   Oracles: tests/golden/lball.tsv and tests/golden/sball.tsv, every row; the vector file
   tests/ref/vectors/t-slice2/vectors_local.jsonl written by lanes/t-slice2/gen_vectors.py from the reference
   proto/text_grammar.py (2000 generated and mutated texts, with the verdict of the reference); the exact decimal
   reader of this file is its own, independent of src/text.c; the round trips are checked against values built
   here with the constructors of lball.h and sball.h. Strings are freed with adf_str_free (common.h).

   What the golden comparison can and cannot require (docs/conventions.md 9.6, gate finding G4): the reader of a
   printed decimal is a ball at prec, so the C printer is required to print the canonical text of the reference
   character for character only where the real ball of the text is read exactly, that is where the midpoint is
   dyadic with an odd mantissa of at most prec bits and the radius is dyadic with an odd mantissa below 2^30
   (conventions 9.5, "Reading"). Everywhere else the test requires containment both ways (the printed interval
   contains the stored arb, and the stored arb contains the exact interval of the text) and that the printed text
   is a text the reader accepts. The local entries have no real part and are compared exactly, always. */

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <string.h>
#include <stdlib.h>

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

/* a string literal and its length */
#define LIT(s) (s), strlen(s)

/* the file the generator of this lane writes; its path is in the header of this file */
#define VECTORS "tests/ref/vectors/t-slice2/vectors_local.jsonl"

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

/* The exactness condition of conventions 9.5 ("Reading") at prec. */
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

/* ---------------------------------------------------------------- the two types, split into places */

/* One entry of a partial ball as it stands in a printed text: the byte range [b, e) of the entry. A text
   "{" E (";" E)* "}" (conventions 9.2, sball_v): the separators are the ';' at the top level, that is not
   inside a real ball (a real ball has no ';' and a complex entry has none either). */
typedef struct
{
    size_t b, e;
} entry_range;

static size_t
split_sball(const char * t, size_t len, entry_range * out, size_t max)
{
    size_t n = 0, i;

    if (len < 2 || t[0] != '{' || t[len - 1] != '}')
        return 0;
    i = 1;
    while (i < len - 1)
    {
        size_t b, j;

        while (i < len - 1 && (t[i] == ' ' || t[i] == '\t' || t[i] == '\n' || t[i] == '\r'))
            i++;
        if (i >= len - 1)
            break;
        b = i;
        for (j = i; j < len - 1; j++)
            if (t[j] == ';')
                break;
        while (j > b && (t[j - 1] == ' ' || t[j - 1] == '\t' || t[j - 1] == '\n' || t[j - 1] == '\r'))
            j--;
        if (n < max)
        {
            out[n].b = b;
            out[n].e = j;
        }
        n++;
        if (j >= len - 1)
            break;                      /* no ';' found: that was the last entry */
        i = j + 1;
    }
    return n;
}

static size_t
trimmed(const char * t, size_t b, size_t e, size_t * ob, size_t * oe)
{
    while (b < e && (t[b] == ' ' || t[b] == '\t'))
        b++;
    while (e > b && (t[e - 1] == ' ' || t[e - 1] == '\t'))
        e--;
    *ob = b;
    *oe = e;
    return e - b;
}

/* ================================================================== sentinels and the parse helpers */

/* A canonical local ball that no parse of this lane changes: the exact 3 at 7 ("[p=7: 3]"). */
static void
set_sentinel_lball(adf_lball_t x)
{
    adf_rat_t q;
    adf_place_t v;

    adf_rat_init(q);
    fmpq_set_si(q->q, 3, 1);
    ADF_CHECK(adf_place_prime(&v, 7) == ADF_OK);
    ADF_CHECK(adf_lball_set_rat(x, v, q) == ADF_OK);
    adf_rat_clear(q);
}

/* 1 if the two are the same value with the same fields (conventions 5.8, representation identity). */
static int
lball_same(const adf_lball_t x, const adf_lball_t y)
{
    return adf_lball_identical(x, y) && adf_lball_is_canonical(x);
}

/* 1 if the sentinel is unchanged. */
static int
lball_is_sentinel(const adf_lball_t x)
{
    adf_lball_t t;
    int ok;

    adf_lball_init(t);
    set_sentinel_lball(t);
    ok = lball_same(x, t);
    adf_lball_clear(t);
    return ok;
}

/* A canonical partial ball with an archimedean component and two primes. */
static void
set_sentinel_sball(adf_sball_t y)
{
    adf_lball_struct loc[2];
    arb_t r;
    adf_place_t v;
    adf_rat_t q;
    int i;

    arb_init(r);
    arb_set_si(r, 1);
    arb_add_error_2exp_si(r, -8);          /* 1 +/- 2^-8, a ball that is not the point */
    adf_rat_init(q);
    for (i = 0; i < 2; i++)
        adf_lball_init(&loc[i]);
    fmpq_set_si(q->q, 3, 1);
    ADF_CHECK(adf_place_prime(&v, 2) == ADF_OK);
    ADF_CHECK(adf_lball_set_rat(&loc[0], v, q) == ADF_OK);
    fmpq_set_si(q->q, 5, 1);
    ADF_CHECK(adf_place_prime(&v, 5) == ADF_OK);
    ADF_CHECK(adf_lball_set_rat_ball(&loc[1], v, q, 3) == ADF_OK);
    ADF_CHECK(adf_sball_set_arb_lballs(y, NULL, r, loc, 2) == ADF_OK);
    adf_rat_clear(q);
    arb_clear(r);
    for (i = 0; i < 2; i++)
        adf_lball_clear(&loc[i]);
}

/* The text of a partial ball, for the comparison of a sentinel. */
static char *
sball_text(size_t * len, const adf_sball_t y)
{
    return adf_sball_get_str(len, y, ADF_DIGITS_DEFAULT);
}

static int
sball_is_sentinel(const adf_sball_t y)
{
    adf_sball_t t;
    size_t l1, l2;
    char * a, * b;
    int ok;

    adf_sball_init(t);
    set_sentinel_sball(t);
    a = sball_text(&l1, t);
    b = sball_text(&l2, y);
    ok = a != NULL && b != NULL && l1 == l2 && memcmp(a, b, l1) == 0;
    flint_free(a);
    flint_free(b);
    adf_sball_clear(t);
    return ok;
}

/* Parse (s, n) as a local ball and check the status and, on a status, the untouched output. */
static int
parse_lball(adf_lball_t x, const char * s, size_t n, const adf_text_limits_t * lim, int want, const char * where)
{
    int st;

    set_sentinel_lball(x);
    st = adf_lball_set_str(x, s, n, lim);
    ADF_CHECK_MSG(st == want, "%s: status %d (%s), expected %d (%s)", where, st, adf_status_str(st), want,
                  adf_status_str(want));
    if (st != ADF_OK)
        ADF_CHECK_MSG(lball_is_sentinel(x), "%s: the output changed on status %s", where, adf_status_str(st));
    else
        ADF_CHECK_MSG(adf_lball_is_canonical(x), "%s: the result is not canonical", where);
    return st;
}

static int
parse_sball(adf_sball_t x, const char * s, size_t n, slong prec, const adf_text_limits_t * lim, int want,
            const char * where)
{
    int st;

    set_sentinel_sball(x);
    st = adf_sball_set_str(x, s, n, prec, lim);
    ADF_CHECK_MSG(st == want, "%s: status %d (%s), expected %d (%s)", where, st, adf_status_str(st), want,
                  adf_status_str(want));
    if (st != ADF_OK)
        ADF_CHECK_MSG(sball_is_sentinel(x), "%s: the output changed on status %s", where, adf_status_str(st));
    else
        ADF_CHECK_MSG(adf_sball_is_canonical(x), "%s: the result is not canonical", where);
    return st;
}

/* ================================================================== adf_lball: the golden file */

ADF_TEST(every_row_of_the_golden_file_lball)
{
    golden_error_t err;
    golden_file * f = NULL;
    size_t i, rows = 0, ok = 0, bad = 0;

    ADF_CHECK_MSG(golden_open("tests/golden/lball.tsv", GOLDEN_DEFAULT_MAX_INPUT, &f, &err) == 1, "%s",
                  golden_error_message(&err));
    if (f == NULL)
        return;
    for (i = 0; i < golden_count(f); i++)
    {
        const golden_record * r = golden_record_at(f, i);
        adf_lball_t x;
        char where[64];

        snprintf(where, sizeof(where), "lball.tsv line %lu", r->line);
        adf_lball_init(x);
        if (r->is_status)
        {
            parse_lball(x, r->input, r->input_len, NULL, status_of_name(r->status), where);
            bad++;
        }
        else if (parse_lball(x, r->input, r->input_len, NULL, ADF_OK, where) == ADF_OK)
        {
            size_t len;
            char * t = adf_lball_get_str(&len, x);
            adf_text_kind kind;
            adf_lball_t y;

            /* a local ball has no real part: the C text is the canonical text (conventions 9.6) */
            ADF_CHECK_MSG(well_formed(t, len) && len == r->expected_len && memcmp(t, r->expected, len) == 0,
                          "%s: printed \"%s\", expected \"%s\"", where, t == NULL ? "(null)" : t, r->expected);
            ADF_CHECK_MSG(adf_text_classify(&kind, r->input, r->input_len, NULL) == ADF_OK
                              && kind == ADF_TEXT_LBALL,
                          "%s: not classified as a local ball", where);
            /* the printed text is a text of its own and gives the same value back */
            adf_lball_init(y);
            ADF_CHECK_MSG(adf_lball_set_str(y, t, len, NULL) == ADF_OK, "%s: the printed text is not read", where);
            ADF_CHECK_MSG(adf_lball_equal_set(x, y) && adf_lball_identical(x, y), "%s: round trip", where);
            {
                size_t l2;
                char * u = adf_lball_get_str(&l2, y);
                ADF_CHECK_MSG(l2 == len && memcmp(u, t, len) == 0, "%s: the text is not a fixed point", where);
                flint_free(u);
            }
            adf_lball_clear(y);
            flint_free(t);
            ok++;
        }
        adf_lball_clear(x);
        rows++;
    }
    ADF_CHECK_MSG(rows == 52, "tests/golden/lball.tsv has %zu vectors, 52", rows);
    ADF_CHECK_MSG(ok == 29 && bad == 23, "%zu read, %zu refused", ok, bad);
    golden_close(f);
}

/* The stored fields of a local ball (conventions 5.8), against hand-computed values. */
ADF_TEST(lball_stored_fields)
{
    adf_lball_t x;
    fmpq_t u;

    adf_lball_init(x);
    fmpq_init(u);
    /* exact: p^v u with u a unit at p. [p=5: 25/3]: v = 2, u = 1/3 */
    parse_lball(x, LIT("[p=5: 25/3]"), NULL, ADF_OK, "exact 25/3");
    ADF_CHECK(x->p == 5 && x->v == 2 && x->N == 0 && x->exact == 1);
    fmpq_set_si(u, 1, 3);
    ADF_CHECK(fmpq_equal(x->u, u));
    /* exact 1/3 at 5: v = 0 */
    parse_lball(x, LIT("[p=5: 1/3]"), NULL, ADF_OK, "exact 1/3");
    ADF_CHECK(x->v == 0 && x->exact == 1 && fmpq_equal(x->u, u));
    /* a ball: the canonical centre, an integer u in (0, p^(N-v)), and the ball around 0 as u = 0 */
    parse_lball(x, LIT("[p=5: 1/3 + O(5^4)]"), NULL, ADF_OK, "ball 1/3 + O(5^4)");
    ADF_CHECK_MSG(x->p == 5 && x->v == 0 && x->N == 4 && x->exact == 0, "the centre of 1/3 + O(5^4)");
    fmpq_set_si(u, 417, 1);
    ADF_CHECK_MSG(fmpq_equal(x->u, u), "3 * 417 = 1251 = 2 * 625 + 1, so the centre is 417");
    parse_lball(x, LIT("[p=5: 28 + O(5^2)]"), NULL, ADF_OK, "ball 28 + O(5^2)");
    fmpq_set_si(u, 3, 1);
    ADF_CHECK_MSG(x->v == 0 && x->N == 2 && x->exact == 0 && fmpq_equal(x->u, u), "28 = 3 + 5 * 5");
    parse_lball(x, LIT("[p=5: 1/125 + O(5^-2)]"), NULL, ADF_OK, "ball 1/125 + O(5^-2)");
    ADF_CHECK_MSG(x->v == -3 && x->N == -2 && x->exact == 0, "the centre 1/125 = 5^-3 has v = -3");
    fmpq_set_si(u, 1, 1);
    ADF_CHECK(fmpq_equal(x->u, u));
    parse_lball(x, LIT("[p=5: 3 + O(5^0)]"), NULL, ADF_OK, "ball 3 + O(5^0)");
    fmpq_zero(u);
    ADF_CHECK_MSG(x->v == 0 && x->N == 0 && x->exact == 0 && fmpq_is_zero(x->u), "the ball around 0");
    ADF_CHECK(adf_lball_contains_zero(x));
    /* exact 0 and the ball O(p^N) are different values (conventions 5.8) */
    parse_lball(x, LIT("[p=5: 0]"), NULL, ADF_OK, "exact 0");
    ADF_CHECK_MSG(x->exact == 1 && x->N == 0 && fmpq_is_zero(x->u) && x->v == 0, "exact 0");
    /* "O(p)" is N = 1 and is printed with the exponent (conventions 9.3, 9.4, A9) */
    parse_lball(x, LIT("[p=2: 5 + O(2)]"), NULL, ADF_OK, "O(2)");
    ADF_CHECK(x->N == 1);
    /* the exact value is p^v u: [p=7: 1/8] has v = 0, [p=2: 1/8] has v = -3 */
    parse_lball(x, LIT("[p=2: 1/8]"), NULL, ADF_OK, "1/8 at 2");
    ADF_CHECK(x->v == -3 && x->exact == 1);
    fmpq_set_si(u, 1, 1);
    ADF_CHECK(fmpq_equal(x->u, u));
    parse_lball(x, LIT("[p=7: 1/8]"), NULL, ADF_OK, "1/8 at 7");
    ADF_CHECK(x->v == 0 && x->exact == 1);
    fmpq_set_si(u, 1, 8);
    ADF_CHECK(fmpq_equal(x->u, u));
    /* the centre of an exact value is read back from the constructor (adf_lball_get_center) */
    {
        adf_rat_t c;

        adf_rat_init(c);
        ADF_CHECK(adf_lball_get_center(c, x) == ADF_OK && fmpq_equal(c->q, u));
        adf_rat_clear(c);
    }
    fmpq_clear(u);
    adf_lball_clear(x);
}

/* ================================================================== adf_sball: the golden file */

/* The byte ranges of the two reals of a printed complex entry "(R) + (R)*i" (conventions 9.4, z(x)):
   a0, a1 the first, b0, b1 the second. 1 if the shape is right. */
static int
scan_complex(const char * t, size_t n, size_t * a0, size_t * a1, size_t * b0, size_t * b1)
{
    size_t j;

    *a0 = *a1 = *b0 = *b1 = 0;
    if (n < 6 || t[0] != '(' || t[n - 2] != '*' || t[n - 1] != 'i')
        return 0;
    for (j = 1; j < n; j++)
        if (t[j] == ')')
        {
            *a0 = 1;
            *a1 = j;
            break;
        }
    for (j = *a1 + 1; j + 1 < n; j++)
        if (t[j] == '(')
        {
            *b0 = j + 1;
            break;
        }
    for (j = *b0; j + 1 < n; j++)
        if (t[j] == ')')
        {
            *b1 = j;
            break;
        }
    return *a1 > *a0 && *b1 > *b0 && *b1 + 3 == n;
}

/* Check the printed text of a partial ball against the canonical text E of the reference. The local entries
   are compared character by character; the archimedean entry as in conventions 9.5 (the exact ball of the text
   is contained in the stored arb, the printed interval contains the exact ball, and the two texts are equal
   when the arb is that exact ball). */
static void
check_sball_text(const adf_sball_t x, const char * t, size_t tn, const char * E, size_t en, slong prec,
                 const char * where)
{
    entry_range pt[64], et[64];
    size_t np, ne, i;

    ADF_CHECK_MSG(well_formed(t, tn), "%s: the printed text is not well formed", where);
    np = split_sball(t, tn, pt, 64);
    ne = split_sball(E, en, et, 64);
    ADF_CHECK_MSG(np == ne && np <= 64, "%s: %zu printed entries, %zu expected", where, np, ne);
    if (np != ne || np > 64)
        return;
    /* the archimedean place comes first when it is there (conventions 7) */
    ADF_CHECK_MSG((x->arch != ADF_ARCH_NONE) == (ne > 0 && en - et[0].b >= 3 && memcmp(E + et[0].b, "inf", 3) == 0),
                  "%s: the tag of the archimedean place", where);
    for (i = 0; i < np; i++)
    {
        size_t pb, pe, eb, ee;
        int is_inf = (i == 0 && x->arch != ADF_ARCH_NONE);

        trimmed(t, pt[i].b, pt[i].e, &pb, &pe);
        trimmed(E, et[i].b, et[i].e, &eb, &ee);
        if (!is_inf)
        {
            ADF_CHECK_MSG(pe - pb == ee - eb && memcmp(t + pb, E + eb, pe - pb) == 0,
                          "%s: entry %zu is \"%.*s\", expected \"%.*s\"", where, i, (int) (pe - pb), t + pb,
                          (int) (ee - eb), E + eb);
            continue;
        }
        /* the archimedean entry: "inf: " and a real, or a complex "(R) + (R)*i" */
        {
            const char * rb = t + pb;
            size_t rn = pe - pb;
            const char * qb = E + eb;
            size_t qn = ee - eb;
            fmpq_t mid, rad, pm, pr;
            const size_t off = 5;                  /* "inf: " */

            ADF_CHECK_MSG(rn > off && memcmp(rb, "inf: ", off) == 0 && qn > off && memcmp(qb, "inf: ", off) == 0,
                          "%s: entry %zu is not an inf entry", where, i);
            if (rn <= off || qn <= off)
                continue;
            rb += off;
            rn -= off;
            qb += off;
            qn -= off;
            fmpq_init(mid);
            fmpq_init(rad);
            fmpq_init(pm);
            fmpq_init(pr);
            if (x->arch == ADF_ARCH_COMPLEX)
            {
                /* "(R) + (R)*i": the two reals are compared as real balls; the text is the reference's
                   shape "(R) + (R)*i" */
                size_t a0 = 0, a1 = 0, b0 = 0, b1 = 0, qa0 = 0, qa1 = 0, qb0 = 0, qb1 = 0;
                int ok = 0;

                if (scan_complex(rb, rn, &a0, &a1, &b0, &b1) && scan_complex(qb, qn, &qa0, &qa1, &qb0, &qb1))
                    ok = 1;
                ADF_CHECK_MSG(ok, "%s: entry %zu is not of the form (R) + (R)*i", where, i);
                if (ok)
                {
                    fmpq_t im, ir;

                    fmpq_init(im);
                    fmpq_init(ir);
                    /* the real part of the expected text, against the real part of the stored value */
                    ADF_CHECK_MSG(read_printed_real(mid, rad, qb + qa0, qa1 - qa0), "%s: the real part", where);
                    ADF_CHECK_MSG(arb_contains_interval(acb_realref(x->inf), mid, rad),
                                  "%s: the real part does not contain the exact ball", where);
                    /* the imaginary part of the printed text, against the imaginary part of the stored
                       value; the golden texts have an exact imaginary part, and then the two texts must be
                       equal */
                    ADF_CHECK_MSG(read_printed_real(im, ir, rb + b0, b1 - b0), "%s: the imaginary part", where);
                    ADF_CHECK_MSG(arb_contains_interval(acb_imagref(x->inf), im, ir),
                                  "%s: the imaginary part does not contain the exact ball", where);
                    if (exact_condition(im, ir, prec))
                    {
                        ADF_CHECK_MSG(arb_is_exactly(acb_imagref(x->inf), im, ir),
                                      "%s: the imaginary part is not the exact ball of the text", where);
                        ADF_CHECK_MSG(b1 - b0 == qb1 - qb0 && memcmp(rb + b0, qb + qb0, b1 - b0) == 0,
                                      "%s: printed \"%.*s\", expected \"%.*s\"", where, (int) (b1 - b0), rb + b0,
                                      (int) (qb1 - qb0), qb + qb0);
                    }
                    ADF_CHECK_MSG(rn == qn && memcmp(rb, qb, rn) == 0, "%s: printed \"%.*s\", expected \"%.*s\"",
                                  where, (int) rn, rb, (int) qn, qb);
                    fmpq_clear(im);
                    fmpq_clear(ir);
                }
            }
            else
            {
                ADF_CHECK_MSG(read_printed_real(pm, pr, rb, rn), "%s: the printed inf entry is not a real ball",
                              where);
                ADF_CHECK_MSG(read_printed_real(mid, rad, qb, qn), "%s: the expected inf entry is not a real "
                                                                    "ball",
                              where);
                ADF_CHECK_MSG(arb_contains_interval(acb_realref(x->inf), mid, rad),
                              "%s: the stored ball does not contain the exact interval of the text", where);
                ADF_CHECK_MSG(interval_contains(pm, pr, mid, rad),
                              "%s: the printed interval does not contain the exact interval of the text", where);
                if (exact_condition(mid, rad, prec))
                {
                    ADF_CHECK_MSG(arb_is_exactly(acb_realref(x->inf), mid, rad),
                                  "%s: the arb is not exactly the ball of the text", where);
                    ADF_CHECK_MSG(rn == qn && memcmp(rb, qb, rn) == 0, "%s: printed \"%.*s\", expected \"%.*s\"",
                                  where, (int) rn, rb, (int) qn, qb);
                }
            }
            fmpq_clear(mid);
            fmpq_clear(rad);
            fmpq_clear(pm);
            fmpq_clear(pr);
        }
    }
}

ADF_TEST(every_row_of_the_golden_file_sball)
{
    golden_error_t err;
    golden_file * f = NULL;
    size_t i, rows = 0, ok = 0, bad = 0;

    ADF_CHECK_MSG(golden_open("tests/golden/sball.tsv", GOLDEN_DEFAULT_MAX_INPUT, &f, &err) == 1, "%s",
                  golden_error_message(&err));
    if (f == NULL)
        return;
    for (i = 0; i < golden_count(f); i++)
    {
        const golden_record * r = golden_record_at(f, i);
        adf_sball_t x;
        char where[64];

        snprintf(where, sizeof(where), "sball.tsv line %lu", r->line);
        adf_sball_init(x);
        if (r->is_status)
        {
            parse_sball(x, r->input, r->input_len, GOLDEN_PREC, NULL, status_of_name(r->status), where);
            bad++;
        }
        else if (parse_sball(x, r->input, r->input_len, GOLDEN_PREC, NULL, ADF_OK, where) == ADF_OK)
        {
            size_t len;
            char * t = adf_sball_get_str(&len, x, ADF_DIGITS_DEFAULT);
            adf_text_kind kind;
            adf_sball_t y;

            check_sball_text(x, t, len, r->expected, r->expected_len, GOLDEN_PREC, where);
            ADF_CHECK_MSG(adf_text_classify(&kind, r->input, r->input_len, NULL) == ADF_OK
                              && kind == ADF_TEXT_SBALL,
                          "%s: not classified as a partial ball", where);
            /* the printed text is a text of its own (conventions 9.6) */
            adf_sball_init(y);
            ADF_CHECK_MSG(adf_sball_set_str(y, t, len, GOLDEN_PREC, NULL) == ADF_OK, "%s: the printed text is "
                                                                                   "not read",
                          where);
            ADF_CHECK_MSG(adf_sball_equal_set(x, y) || arb_contains(acb_realref(y->inf), acb_realref(x->inf)),
                          "%s: the round trip loses the value", where);
            ADF_CHECK_MSG(y->arch == x->arch && y->len == x->len, "%s: the round trip changes the places", where);
            for (slong k = 0; k < x->len; k++)
                ADF_CHECK_MSG(adf_lball_identical(&x->loc[k], &y->loc[k]), "%s: the round trip changes the local "
                                                                            "component %ld",
                              where, (long) k);
            adf_sball_clear(y);
            flint_free(t);
            ok++;
        }
        adf_sball_clear(x);
        rows++;
    }
    ADF_CHECK_MSG(rows == 25, "tests/golden/sball.tsv has %zu vectors, 25", rows);
    ADF_CHECK_MSG(ok == 11 && bad == 14, "%zu read, %zu refused", ok, bad);
    golden_close(f);
}

/* A complex archimedean entry is stored with the tag ADF_ARCH_COMPLEX (conventions 5.9, the archimedean place
   carries the tag). */
ADF_TEST(sball_the_complex_tag)
{
    adf_sball_t x;
    size_t len;
    char * t;

    adf_sball_init(x);
    parse_sball(x, LIT("{inf: (1) + (2)*i; p=5: 3 + O(5^4)}"), GOLDEN_PREC, NULL, ADF_OK, "a complex entry");
    ADF_CHECK_MSG(x->arch == ADF_ARCH_COMPLEX, "the tag is %d, not ADF_ARCH_COMPLEX", x->arch);
    ADF_CHECK(arf_equal_si(arb_midref(acb_realref(x->inf)), 1) && arf_equal_si(arb_midref(acb_imagref(x->inf)), 2));
    ADF_CHECK(mag_is_zero(arb_radref(acb_realref(x->inf))) && mag_is_zero(arb_radref(acb_imagref(x->inf))));
    ADF_CHECK(x->len == 1 && x->loc[0].p == 5);
    t = adf_sball_get_str(&len, x, ADF_DIGITS_DEFAULT);
    ADF_CHECK_MSG(strcmp(t, "{inf: (1) + (2)*i; p=5: 3 + O(5^4)}") == 0, "printed \"%s\"", t);
    flint_free(t);
    /* a complex entry with several primes: the entries are in the canonical order of places */
    parse_sball(x, LIT("{p=7: 1; inf: (1) + (2)*i; p=2: 3 + O(2^3)}"), GOLDEN_PREC, NULL, ADF_OK,
                "a complex entry and two primes, out of order");
    ADF_CHECK(x->arch == ADF_ARCH_COMPLEX && x->len == 2);
    ADF_CHECK_MSG(x->loc[0].p == 2 && x->loc[1].p == 7, "the primes are in the canonical order");
    ADF_CHECK(adf_sball_is_canonical(x));
    t = adf_sball_get_str(&len, x, ADF_DIGITS_DEFAULT);
    ADF_CHECK_MSG(strcmp(t, "{inf: (1) + (2)*i; p=2: 3 + O(2^3); p=7: 1}") == 0, "printed \"%s\"", t);
    flint_free(t);
    /* a real entry: the imaginary part is the exact zero and the text has no complex */
    parse_sball(x, LIT("{inf: (1) + (0)*i; p=5: 1}"), GOLDEN_PREC, NULL, ADF_OK, "a zero complex entry");
    ADF_CHECK_MSG(x->arch == ADF_ARCH_COMPLEX, "the tag is %d", x->arch);
    parse_sball(x, LIT("{inf: 0.5; p=5: 1}"), GOLDEN_PREC, NULL, ADF_OK, "a real entry");
    ADF_CHECK_MSG(x->arch == ADF_ARCH_REAL, "the tag is %d", x->arch);
    ADF_CHECK(mag_is_zero(arb_radref(acb_imagref(x->inf))) && arf_is_zero(arb_midref(acb_imagref(x->inf))));
    t = adf_sball_get_str(&len, x, ADF_DIGITS_DEFAULT);
    ADF_CHECK_MSG(strcmp(t, "{inf: 0.5; p=5: 1}") == 0, "printed \"%s\"", t);
    flint_free(t);
    /* the empty set of places: a fresh value (adf_sball_init releases nothing, so x is cleared first) */
    adf_sball_clear(x);
    adf_sball_init(x);
    t = adf_sball_get_str(&len, x, ADF_DIGITS_DEFAULT);
    ADF_CHECK_MSG(strcmp(t, "{}") == 0, "printed \"%s\"", t);
    flint_free(t);
    adf_sball_clear(x);
}

/* The order of the places: the archimedean place first, then the primes increasing (conventions 7, 5.9). */
ADF_TEST(sball_the_canonical_order)
{
    adf_sball_t x;
    size_t len;
    char * t;

    adf_sball_init(x);
    parse_sball(x, LIT("{p=5: 3 + O(5^4); inf: 1.5; p=2: 1 + O(2^3); p=65537: -7}"), GOLDEN_PREC, NULL, ADF_OK,
                  "four places out of order");
    ADF_CHECK_MSG(x->arch == ADF_ARCH_REAL && x->len == 3, "%ld places", (long) x->len);
    ADF_CHECK(x->loc[0].p == 2 && x->loc[1].p == 5 && x->loc[2].p == 65537);
    ADF_CHECK(adf_sball_is_canonical(x));
    t = adf_sball_get_str(&len, x, ADF_DIGITS_DEFAULT);
    ADF_CHECK_MSG(strcmp(t, "{inf: 1.5; p=2: 1 + O(2^3); p=5: 3 + O(5^4); p=65537: -7}") == 0, "printed \"%s\"", t);
    flint_free(t);
    /* the places of the value are those of the text, in the canonical order (conventions 7) */
    {
        adf_place_t v;

        ADF_CHECK(adf_sball_get_place(&v, x, 0) == ADF_OK && adf_place_is_archimedean(v));
        ADF_CHECK(adf_sball_get_place(&v, x, 1) == ADF_OK && adf_place_prime_get(v) == 2);
        ADF_CHECK(adf_sball_get_place(&v, x, 3) == ADF_OK && adf_place_prime_get(v) == 65537);
        ADF_CHECK(adf_sball_get_place(&v, x, 4) == ADF_DOMAIN);
        ADF_CHECK(adf_sball_num_places(x) == 4);
    }
    adf_sball_clear(x);
}

/* ================================================================== the reference on generated texts */

/* The exact ball of part k of the record (mid, rad, read by the reference from the text) against the
   stored ball x and against the printed real ball P of n bytes: x contains [mid - rad, mid + rad] (the
   reader of conventions 9.5) and the printed interval contains it (9.5, "Properties"). */
static void
check_exact_part(const arb_struct * x, fmpq_t mid, fmpq_t rad, fmpq_t pm, fmpq_t pr, const char * P, size_t n,
                 const jsonl_value * pv, size_t k, const char * where)
{
    const jsonl_value * pair = jsonl_at(pv, k, NULL), * ms, * rs;
    const char * cs;
    size_t cl;

    if (pair == NULL || jsonl_size(pair) != 2)
    {
        ADF_CHECK_MSG(0, "%s: no pair %zu", where, k);
        return;
    }
    ms = jsonl_at(pair, 0, NULL);
    rs = jsonl_at(pair, 1, NULL);
    cs = jsonl_string(ms, &cl, NULL);
    if (cs == NULL)
    {
        ADF_CHECK_MSG(0, "%s: the midpoint of part %zu", where, k);
        return;
    }
    if (fmpq_set_str(mid, cs, 10) != 0)
    {
        ADF_CHECK_MSG(0, "%s: the midpoint of part %zu", where, k);
        return;
    }
    cs = jsonl_string(rs, &cl, NULL);
    if (cs == NULL || fmpq_set_str(rad, cs, 10) != 0)
    {
        ADF_CHECK_MSG(0, "%s: the radius of part %zu", where, k);
        return;
    }
    ADF_CHECK_MSG(arb_contains_interval(x, mid, rad), "%s: the stored ball does not contain the exact interval "
                                                      "of the text",
                  where);
    ADF_CHECK_MSG(read_printed_real(pm, pr, P, n), "%s: the printed real ball", where);
    ADF_CHECK_MSG(interval_contains(pm, pr, mid, rad),
                  "%s: the printed interval does not contain the exact interval of the text", where);
}

ADF_TEST(the_reference_on_generated_texts)
{
    jsonl_error_t jerr;
    jsonl_file * f = NULL;
    size_t i, count = 0, valid = 0, exact = 0, inexact = 0;

    if (!jsonl_open(VECTORS, &f, &jerr))
    {
        ADF_CHECK_MSG(0, "%s (run python3 lanes/t-slice2/gen_vectors.py)", jsonl_error_message(&jerr));
        return;
    }
    for (i = 0; i < jsonl_count(f); i++)
    {
        const jsonl_value * rec = jsonl_record(f, i);
        const jsonl_value * ty, * tx, * rf, * er;
        const char * type, * ref;
        size_t tl, rl;
        char where[64];
        int is_lball, want, exact_real;

        ADF_CHECK(jsonl_field(rec, "type", &ty, &jerr) == 1 && jsonl_field(rec, "text", &tx, &jerr) == 1
                  && jsonl_field(rec, "ref", &rf, &jerr) == 1
                  && jsonl_field(rec, "exact_real", &er, &jerr) == 1);
        if (ty == NULL || tx == NULL || rf == NULL || er == NULL)
            break;
        type = jsonl_string(ty, &tl, &jerr);
        ref = jsonl_string(rf, &rl, &jerr);
        is_lball = type != NULL && strcmp(type, "lball") == 0;
        exact_real = 0;
        ADF_CHECK(jsonl_bool(er, &exact_real, &jerr) == 1);
        snprintf(where, sizeof(where), "vectors line %zu", i + 1);
        if (ref[0] == '!')
            want = status_of_name(ref + 1);
        else
        {
            want = ADF_OK;
            valid++;
            if (exact_real)
                exact++;
            else
                inexact++;
        }
        {
            size_t tl2;
            const char * text = jsonl_string(tx, &tl2, &jerr);

            if (text == NULL)
                break;
            if (is_lball)
            {
                adf_lball_t x;
                size_t len;
                char * t;

                adf_lball_init(x);
                if (parse_lball(x, text, tl2, NULL, want, where) == ADF_OK)
                {
                    t = adf_lball_get_str(&len, x);
                    ADF_CHECK_MSG(well_formed(t, len) && len == rl && memcmp(t, ref, len) == 0,
                                  "%s: printed \"%s\", the reference says \"%s\"", where,
                                  t == NULL ? "(null)" : t, ref);
                    flint_free(t);
                }
                adf_lball_clear(x);
            }
            else
            {
                adf_sball_t x;
                size_t len;
                char * t;

                adf_sball_init(x);
                if (parse_sball(x, text, tl2, GOLDEN_PREC, NULL, want, where) == ADF_OK)
                {
                    t = adf_sball_get_str(&len, x, ADF_DIGITS_DEFAULT);
                    if (exact_real)
                        ADF_CHECK_MSG(well_formed(t, len) && len == rl && memcmp(t, ref, len) == 0,
                                      "%s: printed \"%s\", the reference says \"%s\"", where,
                                      t == NULL ? "(null)" : t, ref);
                    else
                    {
                        /* The C reader rounds the real part at prec, so the printed text differs from the
                           text of the reference (conventions 9.6, gate finding G4). What must hold: the
                           stored ball contains the exact interval of the text (the parts of the record),
                           the printed interval contains it as well (9.5, "Properties"), every local entry
                           is printed as the reference prints it, and the printed text is a fixed point. */
                        entry_range pt[64], et[64];
                        size_t np = split_sball(t, len, pt, 64), ne = split_sball(ref, rl, et, 64);
                        const jsonl_value * pv;
                        fmpq_t mid, rad, pm, pr;

                        ADF_CHECK_MSG(np == ne && np <= 64, "%s: %zu entries printed, %zu expected", where, np,
                                      ne);
                        fmpq_init(mid);
                        fmpq_init(rad);
                        fmpq_init(pm);
                        fmpq_init(pr);
                        pv = NULL;
                        ADF_CHECK_MSG(jsonl_field(rec, "parts", &pv, &jerr) == 1, "%s: no parts", where);
                        if (pv != NULL && x->arch != ADF_ARCH_NONE && np == ne && np <= 64)
                        {
                            size_t pb, pe, npair = jsonl_size(pv);

                            trimmed(t, pt[0].b, pt[0].e, &pb, &pe);
                            ADF_CHECK_MSG(pe - pb > 5 && memcmp(t + pb, "inf: ", 5) == 0, "%s: no inf entry",
                                          where);
                            ADF_CHECK_MSG(npair == (size_t) (x->arch == ADF_ARCH_COMPLEX ? 2 : 1),
                                          "%s: %zu parts for the tag %d", where, npair, x->arch);
                            if (x->arch == ADF_ARCH_REAL && npair == 1 && pe - pb > 5)
                                check_exact_part(acb_realref(x->inf), mid, rad, pm, pr, t + pb + 5,
                                                 pe - pb - 5, pv, 0, where);
                            else if (x->arch == ADF_ARCH_COMPLEX && npair == 2 && pe - pb > 5)
                            {
                                size_t a1 = 0, b0 = 0, cl;

                                for (cl = 0; cl < pe - pb; cl++)
                                    if (t[pb + cl] == ')')
                                    {
                                        a1 = cl;
                                        break;
                                    }
                                for (cl = a1 + 1; cl < pe - pb; cl++)
                                    if (t[pb + cl] == '(')
                                    {
                                        b0 = cl + 1;
                                        break;
                                    }
                                ADF_CHECK_MSG(a1 > 6 && b0 > a1 + 1 && pe - pb > b0 + 3, "%s: the shape of the "
                                                                                          "complex entry",
                                              where);
                                if (a1 > 6 && b0 > a1 + 1 && pe - pb > b0 + 3)
                                {
                                    check_exact_part(acb_realref(x->inf), mid, rad, pm, pr, t + pb + 6, a1 - 6,
                                                     pv, 0, where);
                                    check_exact_part(acb_imagref(x->inf), mid, rad, pm, pr, t + pb + b0,
                                                     pe - pb - b0 - 3, pv, 1, where);
                                }
                            }
                        }
                        /* every local entry is printed as the reference prints it, whatever the real part */
                        for (slong k = (x->arch != ADF_ARCH_NONE ? 1 : 0); k < (slong) np; k++)
                        {
                            size_t pb, pe, eb, ee;

                            trimmed(t, pt[k].b, pt[k].e, &pb, &pe);
                            trimmed(ref, et[k].b, et[k].e, &eb, &ee);
                            ADF_CHECK_MSG(pe - pb == ee - eb && memcmp(t + pb, ref + eb, pe - pb) == 0,
                                          "%s: entry %ld printed \"%.*s\", the reference \"%.*s\"", where,
                                          (long) k, (int) (pe - pb), t + pb, (int) (ee - eb), ref + eb);
                        }
                        fmpq_clear(mid);
                        fmpq_clear(rad);
                        fmpq_clear(pm);
                        fmpq_clear(pr);
                        ADF_CHECK_MSG(well_formed(t, len), "%s: the printed text is not well formed", where);
                        /* the printed text is a text the reader accepts, with the same local components */
                        {
                            adf_sball_t y;
                            size_t len2;
                            char * u;

                            adf_sball_init(y);
                            ADF_CHECK_MSG(adf_sball_set_str(y, t, len, GOLDEN_PREC, NULL) == ADF_OK,
                                          "%s: the printed text is not read back", where);
                            ADF_CHECK_MSG(y->arch == x->arch && y->len == x->len, "%s: the places", where);
                            for (slong k = 0; k < x->len && k < y->len; k++)
                                ADF_CHECK_MSG(adf_lball_identical(&x->loc[k], &y->loc[k]),
                                              "%s: the local component %ld", where, (long) k);
                            u = adf_sball_get_str(&len2, y, ADF_DIGITS_DEFAULT);
                            /* A fixed point of printing only where the real part is read exactly: with a
                               rounded real ball every pass widens it a little (conventions 9.6, gate
                               finding G4: "Repeated C value-text round trips may widen the value and
                               change their text on every pass"). */
                            if (exact_real || x->arch == ADF_ARCH_NONE)
                                ADF_CHECK_MSG(u != NULL && len2 == len && memcmp(u, t, len) == 0,
                                              "%s: the printed text is not a fixed point of printing", where);
                            ADF_CHECK_MSG(arb_contains(acb_realref(y->inf), acb_realref(x->inf)),
                                          "%s: the ball read back does not contain the stored one", where);
                            flint_free(u);
                            adf_sball_clear(y);
                        }
                    }
                    flint_free(t);
                }
                adf_sball_clear(x);
            }
        }
        count++;
    }
    ADF_CHECK_MSG(count == 2000, "%zu vectors read", count);
    ADF_CHECK_MSG(valid == 805, "%zu of them valid texts", valid);
    ADF_CHECK_MSG(exact + inexact == valid, "%zu exact, %zu inexact real parts", exact, inexact);
    jsonl_close(f);
}

/* ================================================================== round trips */

/* A small generator, so that the values of this test do not depend on the order of the other tests. */
static ulong rng_state = 12345;

static ulong
next_rand(void)
{
    rng_state = rng_state * 6364136223846793005UL + 1442695040888963407UL;
    return rng_state >> 17;
}

static slong
rand_range(slong lo, slong hi)
{
    return lo + (slong) (next_rand() % (ulong) (hi - lo + 1));
}

ADF_TEST(round_trips_of_generated_values)
{
    static const ulong primes[6] = { 2, 3, 5, 7, 65537, 18446744073709551557UL };
    slong i, nlocal = 0, nexact = 0;

    /* 2000 local balls: exact rationals and balls, exponents negative, zero and positive, six primes */
    for (i = 0; i < 2000; i++)
    {
        adf_lball_t x, y;
        adf_rat_t q;
        adf_place_t v;
        char * t;
        size_t nelen = 0;
        int ball = (int) (next_rand() % 2);

        adf_lball_init(x);
        adf_lball_init(y);
        adf_rat_init(q);
        ADF_CHECK(adf_place_prime(&v, primes[next_rand() % 6]) == ADF_OK);
        fmpq_set_si(q->q, (slong) (next_rand() % 2001) - 1000, (slong) (1 + next_rand() % 12));
        if (ball)
        {
            slong N = rand_range(-4, 6);

            ADF_CHECK(adf_lball_set_rat_ball(x, v, q, N) == ADF_OK);
        }
        else
        {
            ADF_CHECK(adf_lball_set_rat(x, v, q) == ADF_OK);
            nexact++;
        }
        /* the text of the value is what the reader must give back (conventions 9.6) */
        t = adf_lball_get_str(&nelen, x);
        ADF_CHECK_MSG(t != NULL, "the printer refused the value");
        if (t == NULL)
            break;
        nlocal++;
        ADF_CHECK_MSG(adf_lball_set_str(y, t, nelen, NULL) == ADF_OK, "\"%s\" is not a local ball text", t);
        ADF_CHECK_MSG(adf_lball_identical(x, y), "\"%s\": the round trip changed the value", t);
        ADF_CHECK_MSG(adf_lball_equal_set(x, y), "\"%s\": the round trip changed the set", t);
        {
            size_t l2;
            char * u = adf_lball_get_str(&l2, y);
            ADF_CHECK_MSG(l2 == nelen && memcmp(u, t, l2) == 0, "\"%s\": the text is not a fixed point", t);
            flint_free(u);
        }
        flint_free(t);
        adf_rat_clear(q);
        adf_lball_clear(y);
        adf_lball_clear(x);
    }
    ADF_CHECK_MSG(nlocal == 2000 && nexact > 700 && nexact < 1300, "%ld values, %ld of them exact", (long)
                  nlocal, (long) nexact);
    /* 2000 partial balls: no place, one real place, several primes, the empty set of places */
    for (i = 0; i < 2000; i++)
    {
        adf_sball_t x, y;
        adf_lball_struct loc[6];
        arb_t r;
        adf_rat_t q;
        char * t;
        size_t len;
        slong n, k, first;
        int arch, exact_real;

        adf_sball_init(x);
        adf_sball_init(y);
        arb_init(r);
        adf_rat_init(q);
        arch = (int) (next_rand() % 3);
        n = rand_range(0, 5);
        for (k = 0; k < 6; k++)
            adf_lball_init(&loc[k]);
        /* the local components, at n distinct primes drawn from the six (conventions 5.9: no repetition) */
        {
            int avail[6];
            int na = 0, j, t2;

            for (j = 0; j < 6; j++)
                avail[na++] = j;
            for (j = 0; j < 6; j++)
            {
                t2 = (int) (next_rand() % (ulong) (na - j));
                {
                    int tmp = avail[j];

                    avail[j] = avail[j + t2];
                    avail[j + t2] = tmp;
                }
            }
            for (k = 0; k < n; k++)
            {
                adf_place_t v;

                ADF_CHECK(adf_place_prime(&v, primes[avail[k]]) == ADF_OK);
                fmpq_set_si(q->q, (slong) (next_rand() % 601) - 300, (slong) (1 + next_rand() % 6));
                ADF_CHECK(adf_lball_set_rat_ball(&loc[k], v, q, rand_range(-3, 5)) == ADF_OK);
            }
        }
        /* the archimedean component: a dyadic point of few digits, so that it is read back exactly */
        if (arch == 0)
        {
            arb_zero(r);
            first = 0;
            exact_real = 1;
        }
        else
        {
            slong e = rand_range(-8, 8);

            arb_set_si(r, (slong) (next_rand() % 401) - 200);
            arb_mul_2exp_si(r, r, -e);
            exact_real = 1;
            first = n;
        }
        ADF_CHECK(adf_sball_set_arb_lballs(x, NULL, arch == 0 ? NULL : r, loc, n) == ADF_OK);
        ADF_CHECK(adf_sball_is_canonical(x));
        t = adf_sball_get_str(&len, x, ADF_DIGITS_DEFAULT);
        ADF_CHECK_MSG(t != NULL && well_formed(t, len), "no text for the value");
        ADF_CHECK_MSG(adf_sball_set_str(y, t, len, GOLDEN_PREC, NULL) == ADF_OK, "\"%s\" is not a partial ball "
                                                                                "text",
                      t);
        ADF_CHECK_MSG(y->arch == x->arch && y->len == x->len, "\"%s\": the places", t);
        for (k = 0; k < x->len; k++)
            ADF_CHECK_MSG(adf_lball_identical(&x->loc[k], &y->loc[k]), "\"%s\": the component %ld", t, (long) k);
        if (x->arch != ADF_ARCH_NONE)
        {
            ADF_CHECK_MSG(arb_equal(acb_realref(y->inf), acb_realref(x->inf)),
                          "\"%s\": the archimedean component is not read back exactly", t);
            ADF_CHECK_MSG(arb_contains(acb_realref(y->inf), acb_realref(x->inf)), "\"%s\": not contained", t);
        }
        ADF_CHECK_MSG(adf_sball_equal_set(x, y), "\"%s\": the round trip changed the set", t);
        {
            size_t len2;
            char * u = adf_sball_get_str(&len2, y, ADF_DIGITS_DEFAULT);

            ADF_CHECK_MSG(u != NULL && len2 == len && memcmp(u, t, len) == 0, "\"%s\": the text is not a fixed "
                                                                              "point",
                          t);
            flint_free(u);
        }
        (void) first;
        (void) exact_real;
        flint_free(t);
        for (k = 0; k < 6; k++)
            adf_lball_clear(&loc[k]);
        adf_rat_clear(q);
        arb_clear(r);
        adf_sball_clear(y);
        adf_sball_clear(x);
    }
}

/* ================================================================== hostile input */

ADF_TEST(hostile_input)
{
    adf_lball_t x;
    adf_sball_t y;
    char buf[4096];
    size_t i;

    adf_lball_init(x);
    adf_sball_init(y);
    /* a prime of 100 digits: UNSUPPORTED (stage 5, before any number is formed) */
    {
        char p[128];
        size_t k = 0;

        p[k++] = '9';
        for (i = 1; i < 100; i++)
            p[k++] = (char) ('0' + (int) (i % 10));
        p[k] = '\0';
        snprintf(buf, sizeof(buf), "[p=%s: 1]", p);
        parse_lball(x, buf, strlen(buf), NULL, ADF_UNSUPPORTED, "a prime of 100 digits");
        snprintf(buf, sizeof(buf), "{p=%s: 1}", p);
        parse_sball(y, buf, strlen(buf), GOLDEN_PREC, NULL, ADF_UNSUPPORTED, "a prime of 100 digits");
    }
    /* 2^64 exactly and 2^64 + 1: UNSUPPORTED; 2^64 - 1 is accepted as a place and refused as not prime */
    parse_lball(x, LIT("[p=18446744073709551616: 1]"), NULL, ADF_UNSUPPORTED, "2^64");
    parse_lball(x, LIT("[p=18446744073709551617: 1]"), NULL, ADF_UNSUPPORTED, "2^64 + 1");
    parse_lball(x, LIT("[p=18446744073709551615: 1]"), NULL, ADF_DOMAIN, "2^64 - 1 is not prime");
    parse_lball(x, LIT("[p=18446744073709551557: 1 + O(18446744073709551557^1)]"), NULL, ADF_OK,
                "2^64 - 59 is prime");
    /* an exponent of 100 digits: LIMIT at stage 4 (the digit string is compared, no number is formed) */
    {
        char e[128];
        size_t k = 0;

        e[k++] = '9';
        for (i = 1; i < 100; i++)
            e[k++] = (char) ('0' + (int) (i % 10));
        e[k] = '\0';
        snprintf(buf, sizeof(buf), "[p=5: 3 + O(5^%s)]", e);
        parse_lball(x, buf, strlen(buf), NULL, ADF_LIMIT, "an exponent of 100 digits");
        snprintf(buf, sizeof(buf), "[p=5: 3 + O(5^-%s)]", e);
        parse_lball(x, buf, strlen(buf), NULL, ADF_LIMIT, "a negative exponent of 100 digits");
        snprintf(buf, sizeof(buf), "{p=5: 3 + O(5^%s)}", e);
        parse_sball(y, buf, strlen(buf), GOLDEN_PREC, NULL, ADF_LIMIT, "an exponent of 100 digits");
        /* the same with a limit that admits it: then the exponent is beyond ADF_LBALL_EXP_MAX, or the
           centre needs a power that lball.h refuses */
        {
            adf_text_limits_t lim;
            char t[64];

            adf_text_limits_default(&lim);
            lim.max_prec = 4611686018427387904;      /* 2^62, above ADF_LBALL_EXP_MAX = 2^60 */
            snprintf(t, sizeof(t), "[p=5: 3 + O(5^%ld)]", (long) ADF_LBALL_EXP_MAX + 1);
            parse_lball(x, t, strlen(t), &lim, ADF_LIMIT, "N above ADF_LBALL_EXP_MAX");
            snprintf(t, sizeof(t), "[p=5: 3 + O(5^-%ld)]", (long) ADF_LBALL_EXP_MAX + 1);
            parse_lball(x, t, strlen(t), &lim, ADF_LIMIT, "N below -ADF_LBALL_EXP_MAX");
            snprintf(t, sizeof(t), "[p=5: 1/3 + O(5^%ld)]", (long) ADF_LBALL_EXP_MAX);
            parse_lball(x, t, strlen(t), &lim, ADF_LIMIT,
                        "N = ADF_LBALL_EXP_MAX: the centre of 1/3 needs too large a power");
            snprintf(t, sizeof(t), "[p=5: 3 + O(5^%ld)]", (long) ADF_LBALL_EXP_MAX);
            parse_lball(x, t, strlen(t), &lim, ADF_OK,
                        "N = ADF_LBALL_EXP_MAX with a small integer centre: no power is needed");
        }
    }
    /* "O(5^" cut at every position */
    {
        const char * full = "[p=5: 3 + O(5^4)]";

        for (i = 0; i <= strlen(full); i++)
        {
            char cut[64];

            memcpy(cut, full, i);
            cut[i] = '\0';
            parse_lball(x, cut, i, NULL, i == strlen(full) ? ADF_OK : ADF_PARSE, "a cut text");
        }
    }
    /* missing brackets, and the wrong ones */
    parse_lball(x, LIT("[p=5: 3"), NULL, ADF_PARSE, "no closing bracket");
    parse_lball(x, LIT("p=5: 3]"), NULL, ADF_PARSE, "no opening bracket");
    parse_lball(x, LIT("[p=5: 3]]"), NULL, ADF_PARSE, "one bracket too many");
    parse_lball(x, LIT("(p=5: 3)"), NULL, ADF_PARSE, "the wrong bracket");
    parse_sball(y, LIT("{p=5: 1"), GOLDEN_PREC, NULL, ADF_PARSE, "no closing brace");
    parse_sball(y, LIT("p=5: 1}"), GOLDEN_PREC, NULL, ADF_PARSE, "no opening brace");
    parse_sball(y, LIT("{p=5: 1; p=7: 2]"), GOLDEN_PREC, NULL, ADF_PARSE, "the wrong bracket");
    parse_sball(y, LIT("[p=5: 1]"), GOLDEN_PREC, NULL, ADF_PARSE, "a local ball is not a partial ball");
    parse_sball(y, LIT("{p=5: 1}}"), GOLDEN_PREC, NULL, ADF_PARSE, "one brace too many");
    /* a NUL byte at every position of three valid texts */
    {
        static const char * texts[3] = { "[p=5: 3 + O(5^4)]", "{p=2: 1 + O(2^3); inf: 1.5 +/- 0.25}",
                                        "{inf: (1) + (2)*i}" };
        int k;

        for (k = 0; k < 3; k++)
        {
            size_t n = strlen(texts[k]);

            for (i = 0; i < n; i++)
            {
                memcpy(buf, texts[k], i);
                buf[i] = '\0';
                buf[i + 1] = texts[k][i];
                buf[n + 1] = '\0';
                if (texts[k][0] == '[')
                    parse_lball(x, buf, n + 1, NULL, ADF_PARSE, "a NUL byte inside");
                else
                    parse_sball(y, buf, n + 1, GOLDEN_PREC, NULL, ADF_PARSE, "a NUL byte inside");
            }
        }
    }
    /* a length of 0 and a NULL pointer */
    parse_lball(x, NULL, 0, NULL, ADF_PARSE, "NULL and 0");
    parse_lball(x, "", 0, NULL, ADF_PARSE, "the empty text");
    parse_sball(y, NULL, 0, GOLDEN_PREC, NULL, ADF_PARSE, "NULL and 0");
    parse_sball(y, "", 0, GOLDEN_PREC, NULL, ADF_PARSE, "the empty text");
    parse_sball(y, "", 0, GOLDEN_PREC, NULL, ADF_PARSE, "the empty text");
    /* whitespace only */
    parse_lball(x, "   ", 3, NULL, ADF_PARSE, "spaces only");
    parse_sball(y, "{  }", 4, GOLDEN_PREC, NULL, ADF_OK, "braces and spaces");
    parse_sball(y, "{\t\n\r }", 6, GOLDEN_PREC, NULL, ADF_OK, "braces and white space");
    /* every prefix of three valid texts: refused, or a valid text of its own, never a crash */
    {
        static const char * texts[3] = { "[p=5: 3 + O(5^4)]", "{p=2: 1 + O(2^3); inf: 1.5 +/- 0.25}",
                                        "{inf: (1) + (2)*i; p=65537: -3 + O(65537^2)}" };

        for (i = 0; i < 3; i++)
        {
            size_t n = strlen(texts[i]);
            size_t k;

            for (k = 0; k <= n; k++)
            {
                int st;

                if (texts[i][0] == '[')
                    st = adf_lball_set_str(x, texts[i], k, NULL);
                else
                    st = adf_sball_set_str(y, texts[i], k, GOLDEN_PREC, NULL);
                ADF_CHECK_MSG(st == ADF_OK || st == ADF_PARSE || st == ADF_DOMAIN || st == ADF_UNSUPPORTED
                                  || st == ADF_LIMIT,
                              "the prefix of length %zu of \"%s\" gave the status %d", k, texts[i], st);
                if (st == ADF_OK)
                {
                    size_t len;
                    char * t = texts[i][0] == '[' ? adf_lball_get_str(&len, x)
                                                  : adf_sball_get_str(&len, y, ADF_DIGITS_DEFAULT);

                    ADF_CHECK_MSG(well_formed(t, len), "the prefix of length %zu prints nothing", k);
                    flint_free(t);
                }
            }
        }
    }
    /* 10000 entries in one partial ball, against the item limit */
    {
        size_t off, n = 10000;
        char * big = (char *) malloc(16 * n + 8);
        adf_text_limits_t lim;

        ADF_CHECK_MSG(big != NULL, "no memory for the text of 10000 entries");
        if (big == NULL)
            goto done_hostile;
        off = (size_t) snprintf(big, 16 * n + 8, "{");
        for (i = 0; i < n; i++)
            off += (size_t) snprintf(big + off, 16 * n + 8 - off, "%sp=5: %ld", i ? ";" : "", (long) i);
        off += (size_t) snprintf(big + off, 16 * n + 8 - off, "}");
        ADF_CHECK_MSG(off <= 16 * n + 8, "the text of 10000 entries is %zu bytes", off);
        /* the same prime 10000 times: the count is below the default max_items, so the repetition of the
           prime is DOMAIN (stage 6) */
        parse_sball(y, big, off, GOLDEN_PREC, NULL, ADF_DOMAIN, "10000 entries at the default limit");
        /* the same text with max_items = 9999: LIMIT at stage 4, before the semantic checks */
        adf_text_limits_default(&lim);
        lim.max_items = 9999;
        parse_sball(y, big, off, GOLDEN_PREC, &lim, ADF_LIMIT, "10000 entries, max_items = 9999");
        /* max_len is checked before any byte is read (stage 1) */
        adf_text_limits_default(&lim);
        lim.max_len = 1000;
        parse_sball(y, big, off, GOLDEN_PREC, &lim, ADF_LIMIT, "10000 entries, max_len = 1000");
        free(big);
    }
    {
        adf_text_limits_t lim;

        adf_text_limits_default(&lim);
        lim.max_items = 3;
        parse_sball(y, LIT("{p=2: 1; p=3: 1; p=5: 1; p=7: 1}"), GOLDEN_PREC, &lim, ADF_LIMIT,
                    "4 entries, max_items = 3");
        parse_sball(y, LIT("{p=2: 1; p=3: 1; p=5: 1}"), GOLDEN_PREC, &lim, ADF_OK, "3 entries, max_items = 3");
        lim.max_items = 1;
        parse_sball(y, LIT("{inf: 1; p=2: 1}"), GOLDEN_PREC, &lim, ADF_LIMIT, "2 entries, max_items = 1");
        lim.max_items = 0;
        parse_sball(y, LIT("{p=2: 1}"), GOLDEN_PREC, &lim, ADF_LIMIT, "1 entry, max_items = 0");
        parse_sball(y, LIT("{}"), GOLDEN_PREC, &lim, ADF_OK, "no entry, max_items = 0");
    }
    /* max_len, max_prec and max_exp10 (conventions 8.4) */
    {
        adf_text_limits_t lim;

        adf_text_limits_default(&lim);
        lim.max_len = 3;
        parse_lball(x, LIT("[p=5: 1]"), &lim, ADF_LIMIT, "max_len = 3");
        parse_sball(y, LIT("{p=5: 1}"), GOLDEN_PREC, &lim, ADF_LIMIT, "max_len = 3");
        parse_lball(x, LIT("[p=4: 1]"), &lim, ADF_LIMIT, "max_len = 3 beats DOMAIN");
        adf_text_limits_default(&lim);
        lim.max_prec = 2;
        parse_lball(x, LIT("[p=5: 3 + O(5^3)]"), &lim, ADF_LIMIT, "max_prec = 2");
        parse_lball(x, LIT("[p=5: 3 + O(5^-3)]"), &lim, ADF_LIMIT, "max_prec = 2, negative");
        parse_lball(x, LIT("[p=5: 3 + O(5^2)]"), &lim, ADF_OK, "max_prec = 2");
        parse_lball(x, LIT("[p=5: 3 + O(5)]"), &lim, ADF_OK, "O(p) is N = 1");
        parse_lball(x, LIT("[p=5: 3 + O(5^0002)]"), &lim, ADF_OK, "leading zeros");
        parse_sball(y, LIT("{p=5: 3 + O(5^3)}"), GOLDEN_PREC, &lim, ADF_LIMIT, "max_prec = 2, a partial ball");
        adf_text_limits_default(&lim);
        lim.max_exp10 = 3;
        parse_sball(y, LIT("{inf: 1e4}"), GOLDEN_PREC, &lim, ADF_LIMIT, "max_exp10 = 3");
        parse_sball(y, LIT("{inf: 1e3}"), GOLDEN_PREC, &lim, ADF_OK, "max_exp10 = 3");
        parse_sball(y, LIT("{inf: (1e4) + (0)*i}"), GOLDEN_PREC, &lim, ADF_LIMIT, "max_exp10 = 3, complex");
        parse_sball(y, LIT("{inf: (1e-3) + (0)*i}"), GOLDEN_PREC, &lim, ADF_OK, "max_exp10 = 3, negative");
        parse_sball(y, LIT("{inf: (1e-4) + (0)*i}"), GOLDEN_PREC, &lim, ADF_LIMIT, "max_exp10 = 3, negative");
    }
done_hostile:
    adf_sball_clear(y);
    adf_lball_clear(x);
}

/* ================================================================== the order of the stages */

ADF_TEST(the_order_of_the_stages)
{
    adf_lball_t x;
    adf_sball_t y;
    adf_text_limits_t lim;

    adf_lball_init(x);
    adf_sball_init(y);
    /* 1 before 2: a too long text with a forbidden byte is LIMIT */
    adf_text_limits_default(&lim);
    lim.max_len = 4;
    {
        char buf[16];

        memcpy(buf, "[p=5:\xff" "1]", 9);
        parse_lball(x, buf, 9, &lim, ADF_LIMIT, "stage 1 before stage 2");
    }
    /* 2 before 3: a forbidden byte in an otherwise valid text */
    {
        char buf[32];

        memcpy(buf, "[p=5: 1]\xc3", 9);
        parse_lball(x, buf, 9, NULL, ADF_PARSE, "stage 2 before stage 3");
    }
    /* 3 before 4: a syntax fault and a limit fault in the same text */
    adf_text_limits_default(&lim);
    lim.max_prec = 1;
    parse_lball(x, LIT("[p=4: 1 + O(5^9)"), &lim, ADF_PARSE, "stage 3 before stage 4");
    /* 4 before 5: an exponent above max_prec and a prime above 2^64 */
    parse_lball(x, LIT("[p=18446744073709551616: 1 + O(5^100001)]"), NULL, ADF_LIMIT, "stage 4 before stage 5");
    parse_sball(y, LIT("{p=18446744073709551616: 1 + O(5^100001)}"), GOLDEN_PREC, NULL, ADF_LIMIT,
                "stage 4 before stage 5");
    parse_sball(y, LIT("{inf: 1e100001; p=18446744073709551616: 1}"), GOLDEN_PREC, NULL, ADF_LIMIT,
                "a decimal exponent before an unsupported prime");
    /* 5 before 6: a prime above 2^64 that is not prime either */
    parse_lball(x, LIT("[p=18446744073709551616: 1/0]"), NULL, ADF_UNSUPPORTED, "stage 5 before stage 6");
    parse_lball(x, LIT("[p=4: 1/0]"), NULL, ADF_DOMAIN, "a composite prime and a zero denominator");
    parse_lball(x, LIT("[p=5: 3 + O(7^4)]"), NULL, ADF_DOMAIN, "the base is not the prime");
    parse_lball(x, LIT("[p=5: 1/0 + O(5^2)]"), NULL, ADF_DOMAIN, "a zero denominator with a ball");
    /* the item limit is applied to every entry before any semantic check (gate finding G8) */
    adf_text_limits_default(&lim);
    lim.max_items = 1;
    parse_sball(y, LIT("{p=4: 1; p=6: 1}"), GOLDEN_PREC, &lim, ADF_LIMIT, "the count before primality");
    lim.max_items = 10;
    parse_sball(y, LIT("{inf: 1; inf: 2}"), GOLDEN_PREC, &lim, ADF_DOMAIN, "two inf entries");
    parse_sball(y, LIT("{p=5: 1; p=5: 2}"), GOLDEN_PREC, &lim, ADF_DOMAIN, "the same prime twice");
    parse_sball(y, LIT("{p=5: 1; p=6: 1}"), GOLDEN_PREC, &lim, ADF_DOMAIN, "6 is not prime");
    parse_sball(y, LIT("{inf: 1; p=5: 1 + O(7^2)}"), GOLDEN_PREC, &lim, ADF_DOMAIN, "the base is not the prime");
    /* prec is decided first of all (the rule of sball.h; conventions 8.5 has no stage for it) */
    parse_sball(y, LIT("{p=5: 1}"), ADF_REAL_PREC_MAX + 1, NULL, ADF_LIMIT, "prec above the bound");
    parse_sball(y, LIT("{"), ADF_REAL_PREC_MAX + 1, NULL, ADF_LIMIT, "prec above the bound, a bad text");
    parse_sball(y, LIT("{p=5: 1}"), ADF_REAL_PREC_MAX, NULL, ADF_OK, "prec at the bound");
    /* a prec below 2 is taken as 2 (decision M1-D4) */
    {
        adf_sball_t z;
        adf_sball_t w;

        adf_sball_init(z);
        adf_sball_init(w);
        ADF_CHECK(adf_sball_set_str(z, LIT("{inf: 0.333 +/- 0.333}"), -5, NULL) == ADF_OK);
        ADF_CHECK(adf_sball_set_str(w, LIT("{inf: 0.333 +/- 0.333}"), 2, NULL) == ADF_OK);
        ADF_CHECK_MSG(acb_equal(z->inf, w->inf), "a prec below 2 is taken as 2");
        adf_sball_clear(z);
        adf_sball_clear(w);
    }
    adf_sball_clear(y);
    adf_lball_clear(x);
}

/* ================================================================== the printers refuse what M1-D6 refuses */

ADF_TEST(printer_bounds_and_digits)
{
    adf_lball_t x;
    adf_sball_t y;
    size_t len;
    char * t;

    adf_lball_init(x);
    adf_sball_init(y);
    /* an exact value whose centre needs a power p^v beyond the bit bound of lball.h: the printer refuses it */
    {
        adf_place_t v;
        adf_rat_t q;
        fmpz_t num;

        adf_rat_init(q);
        fmpz_init(num);
        ADF_CHECK(adf_place_prime(&v, 2) == ADF_OK);
        fmpz_one(num);
        fmpz_mul_2exp(num, num, (ulong) ADF_LBALL_BITS_MAX + 1);
        fmpq_set_fmpz(q->q, num);
        ADF_CHECK(adf_lball_set_rat(x, v, q) == ADF_OK);
        ADF_CHECK(x->v == (slong) ADF_LBALL_BITS_MAX + 1 && x->exact == 1);
        t = adf_lball_get_str(&len, x);
        ADF_CHECK_MSG(t == NULL && len == 0, "a centre beyond the bit bound is refused");
        flint_free(t);
        /* the ball around it prints: its centre is 0, so no power is needed */
        ADF_CHECK(adf_lball_set_rat_ball(x, v, q, (slong) ADF_LBALL_BITS_MAX + 1) == ADF_OK);
        t = adf_lball_get_str(&len, x);
        ADF_CHECK_MSG(t != NULL && well_formed(t, len), "the ball around it prints");
        flint_free(t);
        fmpz_clear(num);
        adf_rat_clear(q);
    }
    /* digits: the real part of a partial ball is printed with the given number of digits */
    {
        adf_sball_t z;
        size_t l1, l2;
        char * a, * b;

        adf_sball_init(z);
        ADF_CHECK(adf_sball_set_str(z, LIT("{inf: 1.2345678901234567890123 +/- 1e-30}"), 64, NULL) == ADF_OK);
        a = adf_sball_get_str(&l1, z, 20);
        b = adf_sball_get_str(&l2, z, 4);
        ADF_CHECK_MSG(a != NULL && b != NULL && l1 != l2 && strncmp(a, b, 5) == 0, "%s / %s", a, b);
        flint_free(a);
        flint_free(b);
        adf_sball_clear(z);
    }
    /* a local component whose centre needs a power p^v beyond the bit bound of lball.h: the printer of the
       partial ball refuses it as well */
    {
        adf_sball_t z;
        adf_place_t v;
        adf_rat_t q;
        fmpz_t num;

        adf_sball_init(z);
        adf_rat_init(q);
        fmpz_init(num);
        ADF_CHECK(adf_place_prime(&v, 2) == ADF_OK);
        fmpz_one(num);
        fmpz_mul_2exp(num, num, (ulong) ADF_LBALL_BITS_MAX + 1);
        fmpq_set_fmpz(q->q, num);
        ADF_CHECK(adf_lball_set_rat(x, v, q) == ADF_OK);
        ADF_CHECK(adf_sball_set_arb_lballs(z, NULL, NULL, x, 1) == ADF_OK);
        ADF_CHECK(adf_sball_is_canonical(z));
        t = adf_sball_get_str(&len, z, ADF_DIGITS_DEFAULT);
        ADF_CHECK_MSG(t == NULL && len == 0, "a component whose centre does not fit is refused");
        flint_free(t);
        fmpz_clear(num);
        adf_rat_clear(q);
        adf_sball_clear(z);
    }
    /* a partial ball with a component beyond ADF_PRINT_EXP_MAX: NULL, *len = 0 */
    {
        adf_sball_t z;
        arb_t r;

        adf_sball_init(z);
        arb_init(r);
        arb_set_si(r, 1);
        arb_mul_2exp_si(r, r, ADF_PRINT_EXP_MAX + 10);
        ADF_CHECK(adf_sball_set_arb_lballs(z, NULL, r, NULL, 0) == ADF_OK);
        t = adf_sball_get_str(&len, z, ADF_DIGITS_DEFAULT);
        ADF_CHECK_MSG(t == NULL && len == 0, "M1-D6 does not refuse");
        flint_free(t);
        arb_clear(r);
        adf_sball_clear(z);
    }
    adf_sball_clear(y);
    adf_lball_clear(x);
}