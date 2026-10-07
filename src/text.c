/* adelefeld/text.c: the value form (docs/conventions.md 8 and 9) for adf_rat, adf_fball,
   adf_adele and adf_cadele, and the classification of a text (9.7). Work package 1.4, lane
   m1-text.

   Sources, read before the code was written, and cited at each function:
   - docs/conventions.md 8.1 (interface, lines 1021-1039), 8.2 (alphabet, 1041-1049), 8.3
     (tokens, 1051-1057), 8.4 (limits, 1059-1073), 8.5 (order of checks, 1075-1090), 9.1 and 9.2
     (grammar, 1097-1147), 9.3 (semantic constraints, 1149-1178), 9.4 (templates, 1180-1206),
     9.5 (real balls, 1208-1267), 9.6 (round trips, 1269-1281), 9.7 (type of a text,
     1283-1297), 4.2 and 4.3 (ownership, outputs after a status, 243-279).
   - The reference implementation proto/text_grammar.py: Parser (lines 272-417), _syntax
     (419-544), _prep (547-556), _check_limits (577-599), _rat (618-628), _dec (631-638), _fin
     (651-656), print_real_detail (187-216), fmt_decimal (146-167), classify (907-922).

   Lane t-slice1 (milestone 2) appends the reader and the printer of the unit coset, the idele and the
   idele class at the end of this file (section "adf_ucoset, adf_idele, adf_idclass"): the machinery of
   this file (cursor, literals, exact decimals, the enclosing ball, the printer of 9.5) is used by them, so
   it stays in this translation unit, and src/text_idele.c reaches it through the two hidden functions
   adf_tx_read_unit_form and adf_tx_write_unit_form (hidden visibility: tests/test_exports.sh).

   This file reads untrusted input. The rules of the lane (lanes/m1-text/brief.md):
   - The parser works over (s, len); it never needs a NUL terminator and never calls strlen,
     strtol, sscanf or atoi on the input, and never gives raw input to a FLINT string function.
     A number literal is copied into a buffer of its own after the grammar and the limits have
     accepted it, and only then given to fmpz_set_str.
   - Real balls are built here from the exact decimal values with exact integer arithmetic and
     outward rounding; arb_set_str and arb_load_str are not called.
   - The stages of conventions 8.5 run in order over the whole text, so a text with two faults
     gets the status of the earlier stage.
   - The output is built in temporaries and swapped in only on ADF_OK (conventions 4.3).
   - The grammar is not recursive (conventions 8.4, line 1072), and neither is this parser. Every
     allocation is bounded by the length of the input or by the limits of 8.4. */

#include <math.h>
#include <string.h>

#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/fmpq.h>
#include <flint/arb.h>
#include <flint/acb.h>

#include "adelefeld/text.h"
#include "invariants.h"
#include "idele_internal.h"

/* ------------------------------------------------------------------------------------------------
   Stages 1 and 2 (conventions 8.5 items 1 and 2, lines 1079-1080; proto/text_grammar.py _prep,
   lines 547-556). */

static const adf_text_limits_t *
tx_limits(const adf_text_limits_t * lim, adf_text_limits_t * store)
{
    if (lim != NULL)
        return lim;
    adf_text_limits_default(store);
    return store;
}

/* conventions 8.2 (line 1043): allowed bytes are 0x20 to 0x7E, TAB, LF, CR. */
static int
tx_byte_allowed(unsigned char c)
{
    return (c >= 0x20 && c <= 0x7E) || c == 0x09 || c == 0x0A || c == 0x0D;
}

/* Stage 1 before any byte is read, then stage 2 over every byte. */
static int
tx_prep(const char * s, size_t len, const adf_text_limits_t * lim)
{
    size_t i;

    if (len > lim->max_len)
        return ADF_LIMIT;
    for (i = 0; i < len; i++)
        if (!tx_byte_allowed((unsigned char) s[i]))
            return ADF_PARSE;
    return ADF_OK;
}

/* ------------------------------------------------------------------------------------------------
   Stage 3: the cursor and the tokens (conventions 8.2 whitespace, 8.3 tokens; proto Parser, lines
   272-317). The reference matches tokens by regular expressions at the cursor after skipping
   whitespace; the functions below do the same by hand. */

typedef struct
{
    const char * s;
    size_t len;
    size_t i;
} tx_cur;

static int
tx_is_ws(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static int
tx_is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static int
tx_is_letter(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static void
tx_ws(tx_cur * c)
{
    while (c->i < c->len && tx_is_ws(c->s[c->i]))
        c->i++;
}

/* The three bytes "+/-" at position j. */
static int
tx_pm_at(const tx_cur * c, size_t j)
{
    return j + 3 <= c->len && c->s[j] == '+' && c->s[j + 1] == '/' && c->s[j + 2] == '-';
}

/* proto Parser.peek (lines 290-294): a single-character token; "+" is not matched where "+/-"
   stands, because the longest match is taken (conventions 8.3, line 1055). */
static int
tx_peek(tx_cur * c, char ch)
{
    tx_ws(c);
    if (c->i >= c->len || c->s[c->i] != ch)
        return 0;
    if (ch == '+' && tx_pm_at(c, c->i))
        return 0;
    return 1;
}

static int
tx_expect(tx_cur * c, char ch)
{
    if (!tx_peek(c, ch))
        return 0;
    c->i++;
    return 1;
}

/* The token "+/-". */
static int
tx_peek_pm(tx_cur * c)
{
    tx_ws(c);
    return tx_pm_at(c, c->i);
}


/* proto Parser.peek_kw (lines 301-304): the maximal run of ASCII letters at the cursor must be
   the keyword (conventions 8.3, line 1053). */
static int
tx_peek_kw(tx_cur * c, const char * w, size_t wlen)
{
    size_t j;

    tx_ws(c);
    j = c->i;
    while (j < c->len && tx_is_letter(c->s[j]))
        j++;
    return j - c->i == wlen && memcmp(c->s + c->i, w, wlen) == 0;
}

static int
tx_kw(tx_cur * c, const char * w, size_t wlen)
{
    if (!tx_peek_kw(c, w, wlen))
        return 0;
    c->i += wlen;
    return 1;
}

#define TX_KW(c, lit) tx_kw((c), (lit), sizeof(lit) - 1)
#define TX_PEEK_KW(c, lit) tx_peek_kw((c), (lit), sizeof(lit) - 1)

/* At the end: only whitespace is left (proto Parser.at_end, lines 286-288). */
static int
tx_at_end(tx_cur * c)
{
    tx_ws(c);
    return c->i == c->len;
}

/* The end of the run of digits that starts at j. */
static size_t
tx_digits_end(const tx_cur * c, size_t j)
{
    while (j < c->len && tx_is_digit(c->s[j]))
        j++;
    return j;
}

/* ------------------------------------------------------------------------------------------------
   Number literals (conventions 9.1, lines 1099-1107). A literal is recorded as byte offsets into
   the input; nothing is converted before stages 3 and 4 have passed. */

typedef struct
{
    int neg;              /* a leading '-' */
    size_t ib, ie;        /* integer digits [ib, ie) */
    size_t fb, fe;        /* fraction digits after '.', fb == fe if none */
    int has_exp;          /* an exponent e/E [+-] digits */
    int exp_neg;
    size_t eb, ee;        /* exponent digits */
    int has_den;          /* a denominator "/" digits */
    size_t db, de;        /* denominator digits */
} tx_num;

/* rat = int ["/" uint], urat = uint ["/" uint], int = ["-"] uint, uint = digit {digit}
   (conventions 9.1; proto RE_UINT, RE_INT, RE_URAT, RE_RAT, lines 261-264). A "/" not followed
   by a digit is not part of the literal, as in the regular expressions. */
static int
tx_scan_rat(tx_cur * c, int allow_neg, int allow_den, tx_num * out)
{
    size_t j;
    tx_num n;

    memset(&n, 0, sizeof(n));
    tx_ws(c);
    j = c->i;
    if (allow_neg && j < c->len && c->s[j] == '-')
    {
        n.neg = 1;
        j++;
    }
    if (j >= c->len || !tx_is_digit(c->s[j]))
        return 0;
    n.ib = j;
    j = tx_digits_end(c, j);
    n.ie = j;
    if (allow_den && j + 1 < c->len && c->s[j] == '/' && tx_is_digit(c->s[j + 1]))
    {
        n.has_den = 1;
        n.db = j + 1;
        j = tx_digits_end(c, j + 1);
        n.de = j;
    }
    c->i = j;
    if (out != NULL)
        *out = n;
    return 1;
}

/* dec = ["-"] udec, udec = uint ["." uint] [exp10], exp10 = ("e" | "E") ["+" | "-"] uint
   (conventions 9.1; proto RE_UDEC, RE_DEC, lines 265-266). An optional part that does not
   complete ("1.", "1e", "1e+") is not taken, as in the regular expressions. */
static int
tx_scan_dec(tx_cur * c, int allow_neg, tx_num * out)
{
    size_t j, k;
    tx_num n;

    memset(&n, 0, sizeof(n));
    tx_ws(c);
    j = c->i;
    if (allow_neg && j < c->len && c->s[j] == '-')
    {
        n.neg = 1;
        j++;
    }
    if (j >= c->len || !tx_is_digit(c->s[j]))
        return 0;
    n.ib = j;
    j = tx_digits_end(c, j);
    n.ie = j;
    n.fb = n.fe = j;
    if (j + 1 < c->len && c->s[j] == '.' && tx_is_digit(c->s[j + 1]))
    {
        n.fb = j + 1;
        j = tx_digits_end(c, j + 1);
        n.fe = j;
    }
    if (j < c->len && (c->s[j] == 'e' || c->s[j] == 'E'))
    {
        int eneg = 0;

        k = j + 1;
        if (k < c->len && (c->s[k] == '+' || c->s[k] == '-'))
        {
            eneg = c->s[k] == '-';
            k++;
        }
        if (k < c->len && tx_is_digit(c->s[k]))
        {
            n.has_exp = 1;
            n.exp_neg = eneg;
            n.eb = k;
            j = tx_digits_end(c, k);
            n.ee = j;
        }
    }
    c->i = j;
    if (out != NULL)
        *out = n;
    return 1;
}


/* ------------------------------------------------------------------------------------------------
   Conversion of accepted literals (after stage 4). */

/* All digits of [b, e) are '0'. */
static int
tx_all_zero(const char * s, size_t b, size_t e)
{
    size_t i;

    for (i = b; i < e; i++)
        if (s[i] != '0')
            return 0;
    return 1;
}

/* z = the non-negative integer of the digits [b, e) of s, e > b. Leading zeros are dropped, the
   rest is copied into a buffer of its own (bounded by the length of the input) and given to
   fmpz_set_str (fmpz.h:348). */
static void
tx_fmpz_digits(fmpz_t z, const char * s, size_t b, size_t e)
{
    char * buf;
    size_t n;

    while (b + 1 < e && s[b] == '0')
        b++;
    n = e - b;
    buf = (char *) flint_malloc(n + 1);
    memcpy(buf, s + b, n);
    buf[n] = '\0';
    fmpz_set_str(z, buf, 10);
    flint_free(buf);
}

/* q = the rational of an accepted rat or urat literal whose denominator is not zero (proto _rat,
   lines 618-628; conventions 9.3, line 1175: leading zeros removed, fractions reduced, -0 is 0). */
static void
tx_fmpq_rat(fmpq_t q, const char * s, const tx_num * n)
{
    tx_fmpz_digits(fmpq_numref(q), s, n->ib, n->ie);
    if (n->has_den)
        tx_fmpz_digits(fmpq_denref(q), s, n->db, n->de);
    else
        fmpz_one(fmpq_denref(q));
    if (n->neg)
        fmpz_neg(fmpq_numref(q), fmpq_numref(q));
    fmpq_canonicalise(q);
}

/* Stage 6 for a literal: a denominator of zero (conventions 9.3, line 1153). */
static int
tx_den_zero(const char * s, const tx_num * n)
{
    return n->has_den && tx_all_zero(s, n->db, n->de);
}

/* ------------------------------------------------------------------------------------------------
   The output string (conventions 8.1, lines 1037-1039; 12.8): flint_malloc, a NUL at s[len]. */

typedef struct
{
    char * p;
    size_t len;
    size_t cap;
} tx_buf;

static void
tx_buf_init(tx_buf * b)
{
    b->cap = 64;
    b->len = 0;
    b->p = (char *) flint_malloc(b->cap);
}

static void
tx_put(tx_buf * b, const char * s, size_t n)
{
    if (b->len + n + 1 > b->cap)
    {
        size_t cap = b->cap;
        while (b->len + n + 1 > cap)
            cap *= 2;
        b->p = (char *) flint_realloc(b->p, cap);
        b->cap = cap;
    }
    memcpy(b->p + b->len, s, n);
    b->len += n;
}

static void
tx_puts(tx_buf * b, const char * s)
{
    tx_put(b, s, strlen(s));
}

static char *
tx_finish(tx_buf * b, size_t * len)
{
    b->p[b->len] = '\0';
    *len = b->len;
    return b->p;
}

/* The decimal digits of an integer, with its sign (fmpz_get_str, fmpz.h:349). */
static void
tx_put_fmpz(tx_buf * b, const fmpz_t z)
{
    char * t = fmpz_get_str(NULL, 10, z);
    tx_puts(b, t);
    flint_free(t);
}

/* q(x) of conventions 9.4 (line 1182): n if the denominator is 1, else n/d; x canonical. */
static void
tx_put_q(tx_buf * b, const fmpq_t x)
{
    tx_put_fmpz(b, fmpq_numref(x));
    if (!fmpz_is_one(fmpq_denref(x)))
    {
        tx_put(b, "/", 1);
        tx_put_fmpz(b, fmpq_denref(x));
    }
}

/* ------------------------------------------------------------------------------------------------
   adf_rat (start symbol rat_v = rat, conventions 9.2 line 1120; template q(x), 9.4). */

int
adf_rat_set_str(adf_rat_t x, const char * s, size_t len, const adf_text_limits_t * lim)
{
    adf_text_limits_t store;
    tx_cur c;
    tx_num n;
    int st;

    lim = tx_limits(lim, &store);
    st = tx_prep(s, len, lim);
    if (st != ADF_OK)
        return st;
    c.s = s;
    c.len = len;
    c.i = 0;
    if (!tx_scan_rat(&c, 1, 1, &n) || !tx_at_end(&c))
        return ADF_PARSE;
    /* stage 4: a rational has no literal limit (conventions 8.4, line 1072); stage 6: */
    if (tx_den_zero(s, &n))
        return ADF_DOMAIN;
    {
        fmpq_t q;
        fmpq_init(q);
        tx_fmpq_rat(q, s, &n);
        fmpq_swap(x->q, q);
        fmpq_clear(q);
    }
    return ADF_OK;
}

char *
adf_rat_get_str(size_t * len, const adf_rat_t x)
{
    ADF_INV_RAT(x);
    tx_buf b;

    tx_buf_init(&b);
    tx_put_q(&b, x->q);
    return tx_finish(&b, len);
}

/* ------------------------------------------------------------------------------------------------
   The finite part fin = rat ["mod" urat] (conventions 9.2, line 1115; proto Parser.fin, lines
   341-347) and adf_fball (fball_v, line 1121; template (* ; F), 9.4 line 1189). */

typedef struct
{
    tx_num a;             /* the centre, a rat literal */
    int has_mod;
    tx_num N;             /* the radius, a urat literal */
} tx_fin;

static int
tx_fin_syntax(tx_cur * c, tx_fin * f)
{
    tx_fin t;

    memset(&t, 0, sizeof(t));
    if (!tx_scan_rat(c, 1, 1, &t.a))
        return 0;
    if (TX_KW(c, "mod"))
    {
        if (!tx_scan_rat(c, 0, 1, &t.N))
            return 0;
        t.has_mod = 1;
    }
    if (f != NULL)
        *f = t;
    return 1;
}

/* fball_v = "(" "*" ";" fin ")" | rat "mod" urat (proto _syntax, lines 424-435). */
static int
tx_fball_syntax(tx_cur * c, tx_fin * f)
{
    if (tx_peek(c, '('))
    {
        c->i++;
        return tx_expect(c, '*') && tx_expect(c, ';') && tx_fin_syntax(c, f) && tx_expect(c, ')');
    }
    if (!tx_scan_rat(c, 1, 1, f != NULL ? &f->a : NULL) || !TX_KW(c, "mod")
        || !tx_scan_rat(c, 0, 1, f != NULL ? &f->N : NULL))
        return 0;
    if (f != NULL)
        f->has_mod = 1;
    return 1;
}

/* Stage 6 for fin: a zero denominator of the centre or of the radius (conventions 9.3, line
   1153). */
static int
tx_fin_domain(const char * s, const tx_fin * f)
{
    return tx_den_zero(s, &f->a) || (f->has_mod && tx_den_zero(s, &f->N));
}

/* out = the global canonical ball a + N Zhat of an accepted fin (proto _fin, lines 651-656: N = 0
   when "mod" is absent; the centre is reduced into [0, N), "mod 0" is dropped, conventions 9.3
   line 1174). adf_fball_set_center_radius (fball.h:128) gives the canonical triple of
   conventions 5.2; it returns ADF_DOMAIN only for N < 0, which a urat cannot be. */
static void
tx_fin_build(adf_fball_t out, const char * s, const tx_fin * f)
{
    adf_rat_t a, N;

    adf_rat_init(a);
    adf_rat_init(N);
    tx_fmpq_rat(a->q, s, &f->a);
    if (f->has_mod)
        tx_fmpq_rat(N->q, s, &f->N);
    (void) adf_fball_set_center_radius(out, a, N);
    adf_rat_clear(a);
    adf_rat_clear(N);
}

/* F of conventions 9.4 (line 1188): q(a) if N = 0, else q(a) mod q(N), a in [0, N), from the
   canonical triple (A, H, d) (adf_fball_get_fmpz3, fball.h:151; a = A/d, N = H/d). */
static void
tx_put_fin(tx_buf * b, const adf_fball_t x)
{
    fmpz_t A, H, d;
    fmpq_t q;

    fmpz_init(A);
    fmpz_init(H);
    fmpz_init_set_ui(d, 1);
    fmpq_init(q);
    adf_fball_get_fmpz3(A, H, d, x);
    fmpq_set_fmpz_frac(q, A, d);
    tx_put_q(b, q);
    if (!fmpz_is_zero(H))
    {
        tx_puts(b, " mod ");
        fmpq_set_fmpz_frac(q, H, d);
        tx_put_q(b, q);
    }
    fmpq_clear(q);
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
}

int
adf_fball_set_str(adf_fball_t x, const char * s, size_t len, const adf_text_limits_t * lim)
{
    adf_text_limits_t store;
    tx_cur c;
    tx_fin f;
    adf_fball_t t;
    int st;

    lim = tx_limits(lim, &store);
    st = tx_prep(s, len, lim);
    if (st != ADF_OK)
        return st;
    c.s = s;
    c.len = len;
    c.i = 0;
    if (!tx_fball_syntax(&c, &f) || !tx_at_end(&c))
        return ADF_PARSE;
    /* stage 4: no literal limit applies to rationals (conventions 8.4, line 1072) */
    if (tx_fin_domain(s, &f))
        return ADF_DOMAIN;
    adf_fball_init(t);
    tx_fin_build(t, s, &f);
    adf_fball_swap(x, t);
    adf_fball_clear(t);
    return ADF_OK;
}

char *
adf_fball_get_str(size_t * len, const adf_fball_t x)
{
    ADF_INV_FBALL(x);
    tx_buf b;

    tx_buf_init(&b);
    tx_puts(&b, "(* ; ");
    tx_put_fin(&b, x);
    tx_put(&b, ")", 1);
    return tx_finish(&b, len);
}

/* ------------------------------------------------------------------------------------------------
   Real balls: syntax (real = dec ["+/-" udec], conventions 9.2 line 1113; complex, line 1114;
   proto Parser.real and Parser.complex, lines 321-339), the limit on exponents (stage 4), the
   exact value of a decimal, the enclosing arb (9.5 "Reading", lines 1210-1214) and the printer
   (9.5 "Printing", lines 1216-1233). */

typedef struct
{
    tx_num m;             /* the midpoint, a dec literal */
    int has_r;
    tx_num r;             /* the radius, a udec literal */
} tx_real;

static int
tx_real_syntax(tx_cur * c, tx_real * out)
{
    tx_real t;

    memset(&t, 0, sizeof(t));
    if (!tx_scan_dec(c, 1, &t.m))
        return 0;
    if (tx_peek_pm(c))
    {
        c->i += 3;
        if (!tx_scan_dec(c, 0, &t.r))
            return 0;
        t.has_r = 1;
    }
    if (out != NULL)
        *out = t;
    return 1;
}

/* complex = "(" real ")" "+" "(" real ")" "*" "i" */
static int
tx_complex_syntax(tx_cur * c, tx_real * re, tx_real * im)
{
    return tx_expect(c, '(') && tx_real_syntax(c, re) && tx_expect(c, ')') && tx_expect(c, '+')
           && tx_expect(c, '(') && tx_real_syntax(c, im) && tx_expect(c, ')') && tx_expect(c, '*')
           && TX_KW(c, "i");
}

/* Stage 4 for a decimal: abs(exponent) <= max_exp10 (conventions 8.4 line 1066, 9.3 line 1165),
   checked on the digit string before any number is formed (8.5 item 4, line 1083; decision M1-D7,
   finding R4). The exponent has no hidden bound of 18 digits: the digit string is compared with
   max_exp10 as a number, so it may be as long as the input (the value of the literal is not
   formed here). max_exp10 is signed; an exponent is a non-negative magnitude, so a negative
   max_exp10 admits no literal with an exponent. Zero digits are skipped, so 1e0007 compares as 7.
   The comparison is on the absolute value: 1e-8 is over a limit of 7. */
static int
tx_dec_over(const char * s, const tx_num * n, const adf_text_limits_t * lim)
{
    size_t b = n->eb, nd, i, ld = 0;
    char lbuf[32];
    slong m = lim->max_exp10;

    if (!n->has_exp)
        return 0;
    if (m < 0)
        return 1;
    while (b < n->ee && s[b] == '0')
        b++;
    if (b == n->ee)
        return 0;               /* the exponent is 0 */
    while (m > 0)
    {
        lbuf[ld++] = (char) ('0' + (m % 10));
        m /= 10;
    }
    nd = n->ee - b;
    if (nd != ld)
        return nd > ld;
    for (i = 0; i < nd; i++)
    {
        char lc = lbuf[ld - 1 - i];
        char sc = s[b + i];
        if (sc != lc)
            return sc > lc;
    }
    return 0;
}

static int
tx_real_over(const char * s, const tx_real * r, const adf_text_limits_t * lim)
{
    return tx_dec_over(s, &r->m, lim) || (r->has_r && tx_dec_over(s, &r->r, lim));
}

/* The exponent of an accepted decimal. Stage 4 admitted it, so its magnitude is at most
   max_exp10 (a signed 64-bit bound); the unsigned accumulator cannot overflow. */
static slong
tx_dec_exp(const char * s, const tx_num * n)
{
    size_t b;
    ulong v = 0;

    if (!n->has_exp)
        return 0;
    for (b = n->eb; b < n->ee; b++)
        v = 10 * v + (ulong) (s[b] - '0');
    return n->exp_neg ? -(slong) v : (slong) v;
}

/* 10^e as an integer, e >= 0. */
static void
tx_pow10_fmpz(fmpz_t z, ulong e)
{
    fmpz_set_ui(z, 10);
    fmpz_pow_ui(z, z, e);
}

/* num/den = the exact value of an accepted decimal (proto _dec, lines 631-638): the digits of the
   integer and fraction parts form D, and the value is D 10^(exponent - number of fraction
   digits). den > 0; the fraction is not reduced. The digits are copied into a buffer of their own
   (bounded by the length of the input) before fmpz_set_str (fmpz.h:348) sees them. */
static void
tx_dec_value(fmpz_t num, fmpz_t den, const char * s, const tx_num * n)
{
    size_t ni = n->ie - n->ib, nf = n->fe - n->fb;
    char * buf = (char *) flint_malloc(ni + nf + 1);
    slong e;

    memcpy(buf, s + n->ib, ni);
    memcpy(buf + ni, s + n->fb, nf);
    buf[ni + nf] = '\0';
    fmpz_set_str(num, buf, 10);
    flint_free(buf);
    /* A zero coefficient is the exact zero whatever the exponent (decision M1-D7, finding R4):
       the power of ten is not formed, so a huge exponent within max_exp10 costs nothing. */
    if (fmpz_is_zero(num))
    {
        fmpz_one(den);
        return;
    }
    e = tx_dec_exp(s, n) - (slong) nf;
    if (e >= 0)
    {
        fmpz_t p;
        fmpz_init(p);
        tx_pow10_fmpz(p, (ulong) e);
        fmpz_mul(num, num, p);
        fmpz_clear(p);
        fmpz_one(den);
    }
    else
        tx_pow10_fmpz(den, (ulong) -e);
    if (n->neg)
        fmpz_neg(num, num);
}

/* m = an upper bound of the rational R >= 0, exactly R when R is a dyadic number whose odd
   mantissa is below 2^30 (MAG_BITS, mag.h:117).
   With u/v = R, e = bits(u) - bits(v) - 29: 2^(bits(u)-1) <= u < 2^bits(u) and likewise for v give
   2^28 < R 2^-e < 2^30, so w = ceil(R 2^-e) <= 2^30 and w 2^e >= R. If R = o 2^k with o odd,
   bits(o) <= 30, then e <= k (by the same bounds), so R 2^-e = o 2^(k-e) is an integer below 2^30
   and w 2^e = R. mag_set_ui_2exp_si (mag.h:615) sets m to an upper bound of w 2^e, exact for
   w < 2^30 [source pending: FLINT 3.0.1 mag.rst, mag_set_ui_2exp_si; the header marks it "TODO:
   test functions below". Probed on this machine: exact for 1, 3, 2^29, 2^29 + 1, 2^29 + 7,
   2^30 - 1, while mag_set_fmpz_2exp_fmpz (mag.h:479) and arf_get_mag add one unit to a
   mantissa of 30 bits. The exactness is checked by tests/test_text_adele.c]. */
static void
tx_mag_upper(mag_t m, const fmpq_t R)
{
    fmpz_t num, den, w;
    slong ex;

    if (fmpq_is_zero(R))
    {
        mag_zero(m);
        return;
    }
    fmpz_init(num);
    fmpz_init(den);
    fmpz_init(w);
    ex = (slong) fmpz_bits(fmpq_numref(R)) - (slong) fmpz_bits(fmpq_denref(R)) - (MAG_BITS - 1);
    if (ex >= 0)
    {
        fmpz_set(num, fmpq_numref(R));
        fmpz_mul_2exp(den, fmpq_denref(R), (ulong) ex);
    }
    else
    {
        fmpz_mul_2exp(num, fmpq_numref(R), (ulong) -ex);
        fmpz_set(den, fmpq_denref(R));
    }
    fmpz_cdiv_q(w, num, den);
    mag_set_ui_2exp_si(m, fmpz_get_ui(w), ex);
    fmpz_clear(num);
    fmpz_clear(den);
    fmpz_clear(w);
}

/* z = an arb that contains [m - r, m + r], m = mn/md, r = rn/rd >= 0 (conventions 9.5, "Reading",
   lines 1210-1214). With p = max(prec, 2) the midpoint is t 2^e, t = floor(|m| 2^-e). The first
   e = bits(|mn|) - bits(md) - p + 1 gives 2^(p-2) < |m| 2^-e < 2^p (from 2^(bits-1) <= n <
   2^bits for numerator and denominator, reduced or not); if t < 2^(p-1), e is lowered by one, and
   then 2^(p-1) <= |m| 2^-e < 2^p. So e = floor(log2 |m|) - p + 1 and t has exactly p bits. The
   error |m| - t 2^e = rem 2^e / den2 is added to the radius exactly, before the one upward
   rounding of tx_mag_upper. If m = o 2^k with o odd of at most p bits, then floor(log2 |m|) =
   bits(o) - 1 + k and e = bits(o) + k - p <= k, so the division is exact and the midpoint is m;
   with r dyadic of odd mantissa below 2^30 the arb is exactly [m +/- r] (the tightness
   requirement of 9.5). arf_set_fmpz_2exp (arf.h:694) is exact. */
static void
tx_arb_set_ball(arb_t z, const fmpz_t mn, const fmpz_t md, const fmpz_t rn, const fmpz_t rd, slong prec)
{
    slong p = prec < 2 ? 2 : prec, e;
    fmpz_t a, num2, den2, t, rem, ee;
    fmpq_t R, err;

    fmpz_init(a);
    fmpz_init(num2);
    fmpz_init(den2);
    fmpz_init(t);
    fmpz_init(rem);
    fmpz_init(ee);
    fmpq_init(R);
    fmpq_init(err);
    fmpq_set_fmpz_frac(R, rn, rd);
    if (fmpz_is_zero(mn))
        arf_zero(arb_midref(z));
    else
    {
        fmpz_abs(a, mn);
        e = (slong) fmpz_bits(a) - (slong) fmpz_bits(md) - p + 1;
        for (;;)
        {
            if (e >= 0)
            {
                fmpz_set(num2, a);
                fmpz_mul_2exp(den2, md, (ulong) e);
            }
            else
            {
                fmpz_mul_2exp(num2, a, (ulong) -e);
                fmpz_set(den2, md);
            }
            fmpz_fdiv_qr(t, rem, num2, den2);
            /* t >= 2^(p-1): e = floor(log2 |m|) - p + 1; else one bit more (at most once) */
            if (fmpz_bits(t) >= (ulong) p)
                break;
            e--;
        }
        fmpz_set_si(ee, e);
        arf_set_fmpz_2exp(arb_midref(z), t, ee);
        if (fmpz_sgn(mn) < 0)
            arf_neg(arb_midref(z), arb_midref(z));
        /* err = rem 2^e / den2 */
        if (e >= 0)
            fmpz_mul_2exp(rem, rem, (ulong) e);
        else
            fmpz_mul_2exp(den2, den2, (ulong) -e);
        fmpq_set_fmpz_frac(err, rem, den2);
        fmpq_add(R, R, err);
    }
    tx_mag_upper(arb_radref(z), R);
    fmpz_clear(a);
    fmpz_clear(num2);
    fmpz_clear(den2);
    fmpz_clear(t);
    fmpz_clear(rem);
    fmpz_clear(ee);
    fmpq_clear(R);
    fmpq_clear(err);
}

/* z = the enclosing arb of an accepted real ball at prec. */
static void
tx_arb_from_real(arb_t z, const char * s, const tx_real * r, slong prec)
{
    fmpz_t mn, md, rn, rd;

    fmpz_init(mn);
    fmpz_init(md);
    fmpz_init(rn);
    fmpz_init_set_ui(rd, 1);
    tx_dec_value(mn, md, s, &r->m);
    if (r->has_r)
        tx_dec_value(rn, rd, s, &r->r);
    tx_arb_set_ball(z, mn, md, rn, rd, prec);
    fmpz_clear(mn);
    fmpz_clear(md);
    fmpz_clear(rn);
    fmpz_clear(rd);
}

/* ---- the printer of conventions 9.5 ("Printing", lines 1216-1233; proto print_real_detail,
   lines 187-216, with k = 2, the unconstrained algorithm). All arithmetic is exact. ---- */

/* q = 10^e, e of either sign. */
static void
tx_pow10_fmpq(fmpq_t q, slong e)
{
    fmpz_one(fmpq_denref(q));
    tx_pow10_fmpz(fmpq_numref(q), (ulong) (e < 0 ? -e : e));
    if (e < 0)
        fmpz_swap(fmpq_numref(q), fmpq_denref(q));
}

/* The sign of |y| - 10^x, y != 0. */
static int
tx_cmp_pow10(const fmpq_t y, slong x)
{
    fmpz_t u, v, p;
    int c;

    fmpz_init(u);
    fmpz_init(v);
    fmpz_init(p);
    fmpz_abs(u, fmpq_numref(y));
    fmpz_set(v, fmpq_denref(y));
    tx_pow10_fmpz(p, (ulong) (x < 0 ? -x : x));
    if (x >= 0)
        fmpz_mul(v, v, p);
    else
        fmpz_mul(u, u, p);
    c = fmpz_cmp(u, v);
    fmpz_clear(u);
    fmpz_clear(v);
    fmpz_clear(p);
    return c;
}

/* X(y) = floor(log10 |y|), y != 0, exactly (proto _floor_log10, lines 79-88). With d = bits(num)
   - bits(den), |y| > 2^(d-1), so log10 |y| > (d - 1) log10 2 > d c - 2 for the constant c below
   (it exceeds log10 2 = 0.301029995663981195... by less than 10^-17, and |d| < 2^50); the start
   x = floor(d c) - 2 is therefore at most X(y), and the loop only increases it. */
static slong
tx_floor_log10(const fmpq_t y)
{
    double d = (double) fmpz_bits(fmpq_numref(y)) - (double) fmpz_bits(fmpq_denref(y));
    slong x = (slong) floor(d * 0.30102999566398120) - 2;

    while (tx_cmp_pow10(y, x + 1) >= 0)
        x++;
    return x;
}

/* M = y rounded to the nearest multiple of 10^q, ties to the even multiple (conventions 9.5,
   round(y, q); proto _round_q, lines 170-173). */
static void
tx_round_q(fmpq_t M, const fmpq_t y, slong q)
{
    fmpq_t u;
    fmpz_t f, r;
    int c;

    fmpq_init(u);
    fmpz_init(f);
    fmpz_init(r);
    tx_pow10_fmpq(u, q);
    fmpq_div(u, y, u);
    fmpz_fdiv_qr(f, r, fmpq_numref(u), fmpq_denref(u));
    fmpz_mul_2exp(r, r, 1);
    c = fmpz_cmp(r, fmpq_denref(u));
    if (c > 0 || (c == 0 && fmpz_is_odd(f)))
        fmpz_add_ui(f, f, 1);
    tx_pow10_fmpq(u, q);
    fmpq_mul_fmpz(M, u, f);
    fmpq_clear(u);
    fmpz_clear(f);
    fmpz_clear(r);
}

/* R = ceil2(E), E > 0: E rounded up to two significant digits (conventions 9.5 line 1218; proto
   _ceil_k, lines 176-180, k = 2). Returns the unit exponent X(E) - 1; R is a multiple of 10^unit. */
static slong
tx_ceil2(fmpq_t R, const fmpq_t E)
{
    slong unit = tx_floor_log10(E) - 1;
    fmpq_t u;
    fmpz_t f;

    fmpq_init(u);
    fmpz_init(f);
    tx_pow10_fmpq(u, unit);
    fmpq_div(R, E, u);
    fmpz_cdiv_q(f, fmpq_numref(R), fmpq_denref(R));
    fmpq_mul_fmpz(R, u, f);
    fmpq_clear(u);
    fmpz_clear(f);
    return unit;
}

/* 1 if y is a multiple of 10^q (proto _is_multiple, lines 183-184). */
static int
tx_is_multiple(const fmpq_t y, slong q)
{
    fmpq_t u;
    int ok;

    fmpq_init(u);
    tx_pow10_fmpq(u, q);
    fmpq_div(u, y, u);
    ok = fmpz_is_one(fmpq_denref(u));
    fmpq_clear(u);
    return ok;
}

/* The digits of |y| = D 10^E for y != 0 a multiple of 10^q: a string of D (flint_malloc) with no
   trailing zero, its length in *k, E in *E. */
static char *
tx_decimal_digits(const fmpq_t y, slong q, size_t * k, slong * E)
{
    fmpq_t u;
    fmpz_t D;
    char * str;
    size_t n;

    fmpq_init(u);
    fmpz_init(D);
    tx_pow10_fmpq(u, q);
    fmpq_div(u, y, u);
    fmpz_abs(D, fmpq_numref(u));          /* the denominator of u is 1 */
    str = fmpz_get_str(NULL, 10, D);
    n = strlen(str);
    *E = q;
    while (n > 1 && str[n - 1] == '0')
    {
        n--;
        (*E)++;
    }
    str[n] = '\0';
    *k = n;
    fmpq_clear(u);
    fmpz_clear(D);
    return str;
}

/* fmt(y) of conventions 9.5 (lines 1229-1233; proto fmt_decimal, lines 146-167) for y a multiple
   of 10^q: "0"; positional notation if -4 <= X <= 20; else d.ddd e X. */
static void
tx_put_fmt(tx_buf * b, const fmpq_t y, slong q)
{
    char * str;
    size_t k, i;
    slong E, X;

    if (fmpq_is_zero(y))
    {
        tx_put(b, "0", 1);
        return;
    }
    str = tx_decimal_digits(y, q, &k, &E);
    X = E + (slong) k - 1;
    if (fmpq_sgn(y) < 0)
        tx_put(b, "-", 1);
    if (X >= -4 && X <= 20)
    {
        if (E >= 0)
        {
            tx_put(b, str, k);
            for (i = 0; i < (size_t) E; i++)
                tx_put(b, "0", 1);
        }
        else if ((slong) k > -E)
        {
            tx_put(b, str, (size_t) ((slong) k + E));
            tx_put(b, ".", 1);
            tx_put(b, str + ((slong) k + E), (size_t) -E);
        }
        else
        {
            tx_put(b, "0.", 2);
            for (i = 0; i < (size_t) (-E - (slong) k); i++)
                tx_put(b, "0", 1);
            tx_put(b, str, k);
        }
    }
    else
    {
        char ex[32];
        tx_put(b, str, 1);
        if (k > 1)
        {
            tx_put(b, ".", 1);
            tx_put(b, str + 1, k - 1);
        }
        snprintf(ex, sizeof(ex), "e%ld", (long) X);
        tx_puts(b, ex);
    }
    flint_free(str);
}

/* 1 if the non-zero binary exponent e is above ADF_PRINT_EXP_MAX in absolute value (decision
   M1-D6). 2^17 = 131072 > ADF_PRINT_EXP_MAX = 100000, so 18 bits or more are over without
   reading the exponent into a word; below that fmpz_get_si is exact. */
static int
tx_bin_exp_over(const fmpz_t e)
{
    slong v;

    if (fmpz_bits(e) > 17)
        return 1;
    v = fmpz_get_si(e);
    if (v < 0)
        v = -v;
    return v > ADF_PRINT_EXP_MAX;
}

/* 1 if the printer of decision M1-D6 admits the arb x: x is zero or every non-zero midpoint and
   radius has a binary exponent at most ADF_PRINT_EXP_MAX in absolute value. The test is made
   before any conversion, so the cost of a printer is bounded (include/adelefeld/text.h:32-38). */
static int
tx_arb_printable(const arb_t x)
{
    if (!arf_is_zero(arb_midref(x)) && tx_bin_exp_over(ARF_EXPREF(arb_midref(x))))
        return 0;
    if (!mag_is_zero(arb_radref(x)) && tx_bin_exp_over(MAG_EXPREF(arb_radref(x))))
        return 0;
    return 1;
}

/* y = the exact value of an arf, canonical; returns an exponent q <= 0 such that y is a multiple
   of 10^q (x = man 2^ex = man 5^(-ex) 10^ex for ex < 0). arf_get_fmpz_2exp (arf.h:678) gives
   man 2^ex exactly. The callers of the value form test the exponent of M1-D6 before this
   function, so it fits a slong; an exponent beyond a word is not reached (an earlier
   flint_abort here was finding R2 of reviewer text). */
static slong
tx_arf_get_fmpq(fmpq_t y, const arf_t x)
{
    fmpz_t man, ex;
    slong e;

    if (arf_is_zero(x))
    {
        fmpq_zero(y);
        return 0;
    }
    fmpz_init(man);
    fmpz_init(ex);
    arf_get_fmpz_2exp(man, ex, x);
    e = fmpz_get_si(ex);
    fmpz_one(fmpq_denref(y));
    if (e >= 0)
        fmpz_mul_2exp(fmpq_numref(y), man, (ulong) e);
    else
    {
        fmpz_set(fmpq_numref(y), man);
        fmpz_mul_2exp(fmpq_denref(y), fmpq_denref(y), (ulong) -e);
    }
    fmpq_canonicalise(y);
    fmpz_clear(man);
    fmpz_clear(ex);
    return e < 0 ? e : 0;
}

/* r(x) of conventions 9.4 (line 1183): the text of 9.5 for an arb with n = digits. */
static void
tx_put_real(tx_buf * b, const arb_t x, slong n)
{
    fmpq_t mid, rad, M, R, t;
    slong qm, q, q2, unit;
    arf_t r;

    fmpq_init(mid);
    fmpq_init(rad);
    fmpq_init(M);
    fmpq_init(R);
    fmpq_init(t);
    arf_init(r);
    qm = tx_arf_get_fmpq(mid, arb_midref(x));
    arf_set_mag(r, arb_radref(x));        /* exact (arf.h:1035-1051) */
    tx_arf_get_fmpq(rad, r);
    /* step 1: rad = 0 and an exact decimal of at most n significant digits (0 included) */
    if (fmpq_is_zero(rad))
    {
        size_t k = 0;
        slong E;

        if (!fmpq_is_zero(mid))
            flint_free(tx_decimal_digits(mid, qm, &k, &E));
        if ((slong) k <= n)
        {
            tx_put_fmt(b, mid, qm);
            goto done;
        }
    }
    /* step 2 */
    if (fmpq_is_zero(mid))
    {
        tx_puts(b, "0 +/- ");
        unit = tx_ceil2(R, rad);
        tx_put_fmt(b, R, unit);
        goto done;
    }
    /* step 3 */
    q = tx_floor_log10(mid) - n + 1;
    if (!fmpq_is_zero(rad))
        q = FLINT_MAX(q, tx_floor_log10(rad) - 1);
    /* steps 4 and 5. R > 0 here: with rad = 0, step 1 did not stop, so mid is not a multiple of
       10^q for this q nor for any larger one, and M != mid. */
    for (;;)
    {
        tx_round_q(M, mid, q);
        fmpq_sub(t, M, mid);
        fmpq_abs(t, t);
        fmpq_add(t, t, rad);
        unit = tx_ceil2(R, t);
        if (fmpq_is_zero(M))
            q2 = tx_floor_log10(R) - 1;
        else
            q2 = FLINT_MAX(tx_floor_log10(M) - n + 1, tx_floor_log10(R) - 1);
        if (tx_is_multiple(M, q2))
            break;
        q = q2;
    }
    /* step 6 */
    tx_put_fmt(b, M, q);
    tx_puts(b, " +/- ");
    tx_put_fmt(b, R, unit);
done:
    fmpq_clear(mid);
    fmpq_clear(rad);
    fmpq_clear(M);
    fmpq_clear(R);
    fmpq_clear(t);
    arf_clear(r);
}

/* ------------------------------------------------------------------------------------------------
   adf_adele (adele_v = "(" real ";" fin ")", conventions 9.2 line 1118; template (r(x_inf) ; F),
   9.4 line 1191) and adf_cadele (cadele_v = "(" complex ";" fin ")", line 1122; template
   (z(x_inf) ; F), z = (r(re)) + (r(im))*i, 9.4 lines 1183, 1192). adf_adele and adf_cadele have
   no sign condition: stages 5 and 7 of 8.5 do not occur. adele.h is implemented by another lane;
   the fields are reached directly (brief of lane m1-text). */

int
adf_adele_set_str(adf_adele_t x, const char * s, size_t len, slong prec, const adf_text_limits_t * lim)
{
    adf_text_limits_t store;
    tx_cur c;
    tx_real r;
    tx_fin f;
    arb_t t;
    adf_fball_t tf;
    int st;

    lim = tx_limits(lim, &store);
    st = tx_prep(s, len, lim);
    if (st != ADF_OK)
        return st;
    c.s = s;
    c.len = len;
    c.i = 0;
    if (!(tx_expect(&c, '(') && tx_real_syntax(&c, &r) && tx_expect(&c, ';') && tx_fin_syntax(&c, &f)
          && tx_expect(&c, ')') && tx_at_end(&c)))
        return ADF_PARSE;
    if (tx_real_over(s, &r, lim))
        return ADF_LIMIT;
    if (tx_fin_domain(s, &f))
        return ADF_DOMAIN;
    arb_init(t);
    adf_fball_init(tf);
    tx_arb_from_real(t, s, &r, prec);
    tx_fin_build(tf, s, &f);
    arb_swap(x->inf, t);
    adf_fball_swap(&x->fin, tf);
    arb_clear(t);
    adf_fball_clear(tf);
    return ADF_OK;
}

char *
adf_adele_get_str(size_t * len, const adf_adele_t x, slong digits)
{
    ADF_INV_ADELE(x);
    tx_buf b;

    if (!tx_arb_printable(x->inf))
    {
        *len = 0;
        return NULL;
    }
    tx_buf_init(&b);
    tx_put(&b, "(", 1);
    tx_put_real(&b, x->inf, digits);
    tx_puts(&b, " ; ");
    tx_put_fin(&b, &x->fin);
    tx_put(&b, ")", 1);
    return tx_finish(&b, len);
}

int
adf_cadele_set_str(adf_cadele_t x, const char * s, size_t len, slong prec, const adf_text_limits_t * lim)
{
    adf_text_limits_t store;
    tx_cur c;
    tx_real re, im;
    tx_fin f;
    acb_t t;
    adf_fball_t tf;
    int st;

    lim = tx_limits(lim, &store);
    st = tx_prep(s, len, lim);
    if (st != ADF_OK)
        return st;
    c.s = s;
    c.len = len;
    c.i = 0;
    if (!(tx_expect(&c, '(') && tx_complex_syntax(&c, &re, &im) && tx_expect(&c, ';')
          && tx_fin_syntax(&c, &f) && tx_expect(&c, ')') && tx_at_end(&c)))
        return ADF_PARSE;
    if (tx_real_over(s, &re, lim) || tx_real_over(s, &im, lim))
        return ADF_LIMIT;
    if (tx_fin_domain(s, &f))
        return ADF_DOMAIN;
    acb_init(t);
    adf_fball_init(tf);
    tx_arb_from_real(acb_realref(t), s, &re, prec);
    tx_arb_from_real(acb_imagref(t), s, &im, prec);
    tx_fin_build(tf, s, &f);
    acb_swap(x->inf, t);
    adf_fball_swap(&x->fin, tf);
    acb_clear(t);
    adf_fball_clear(tf);
    return ADF_OK;
}

char *
adf_cadele_get_str(size_t * len, const adf_cadele_t x, slong digits)
{
    ADF_INV_CADELE(x);
    tx_buf b;

    if (!tx_arb_printable(acb_realref(x->inf)) || !tx_arb_printable(acb_imagref(x->inf)))
    {
        *len = 0;
        return NULL;
    }
    tx_buf_init(&b);
    tx_puts(&b, "((");
    tx_put_real(&b, acb_realref(x->inf), digits);
    tx_puts(&b, ") + (");
    tx_put_real(&b, acb_imagref(x->inf), digits);
    tx_puts(&b, ")*i ; ");
    tx_put_fin(&b, &x->fin);
    tx_put(&b, ")", 1);
    return tx_finish(&b, len);
}

/* ------------------------------------------------------------------------------------------------
   The type of a text (conventions 9.7, lines 1283-1297; proto classify, lines 907-922): stages 1
   to 3 of 8.5 only. The recognisers below follow the productions of 9.2 (lines 1113-1134) and proto
   _syntax (lines 419-544) and Parser (lines 321-416), token for token. None of them recurses; the
   loops over list items do not count them (counts are stage 4, which classification does not
   run). */

/* ucoset = "[" int ["mod" uint] "]" (proto Parser.ucoset, lines 349-357) */
static int
tx_ucoset_syntax(tx_cur * c)
{
    if (!tx_expect(c, '[') || !tx_scan_rat(c, 1, 0, NULL))
        return 0;
    if (TX_KW(c, "mod") && !tx_scan_rat(c, 0, 0, NULL))
        return 0;
    return tx_expect(c, ']');
}

/* lcoord = rat ["+" "O" "(" uint ["^" sint] ")"] (proto Parser.lcoord, lines 359-372) */
static int
tx_lcoord_syntax(tx_cur * c)
{
    if (!tx_scan_rat(c, 1, 1, NULL))
        return 0;
    if (!tx_peek(c, '+'))
        return 1;
    c->i++;
    if (!TX_KW(c, "O") || !tx_expect(c, '(') || !tx_scan_rat(c, 0, 0, NULL))
        return 0;
    if (tx_expect(c, '^') && !tx_scan_rat(c, 1, 0, NULL))
        return 0;
    return tx_expect(c, ')');
}

/* adele_v = "(" real ";" fin ")" */
static int
tx_adele_syntax(tx_cur * c)
{
    return tx_expect(c, '(') && tx_real_syntax(c, NULL) && tx_expect(c, ';') && tx_fin_syntax(c, NULL)
           && tx_expect(c, ')');
}

/* "p" "=" uint ":" lcoord, the prime entry of lball_v and sentry */
static int
tx_prime_entry_syntax(tx_cur * c)
{
    return TX_KW(c, "p") && tx_expect(c, '=') && tx_scan_rat(c, 0, 0, NULL) && tx_expect(c, ':')
           && tx_lcoord_syntax(c);
}

/* sentry = "inf" ":" (real | complex) | "p" "=" uint ":" lcoord (proto Parser.sentry, lines
   382-394: a complex is chosen when "(" follows the colon) */
static int
tx_sentry_syntax(tx_cur * c)
{
    if (TX_KW(c, "inf"))
    {
        if (!tx_expect(c, ':'))
            return 0;
        if (tx_peek(c, '('))
            return tx_complex_syntax(c, NULL, NULL);
        return tx_real_syntax(c, NULL);
    }
    return tx_prime_entry_syntax(c);
}

/* rterm (proto Parser.rterm, lines 396-416) */
static int
tx_rterm_syntax(tx_cur * c)
{
    if (!TX_KW(c, "term") || !tx_expect(c, '(') || !TX_KW(c, "P") || !tx_expect(c, '=') || !tx_expect(c, '['))
        return 0;
    if (!tx_peek(c, ']'))
    {
        if (!tx_complex_syntax(c, NULL, NULL))
            return 0;
        while (tx_expect(c, ','))
            if (!tx_complex_syntax(c, NULL, NULL))
                return 0;
    }
    return tx_expect(c, ']') && tx_expect(c, ',') && TX_KW(c, "A") && tx_expect(c, '=')
           && tx_complex_syntax(c, NULL, NULL) && tx_expect(c, ',') && TX_KW(c, "B") && tx_expect(c, '=')
           && tx_complex_syntax(c, NULL, NULL) && tx_expect(c, ',') && TX_KW(c, "C") && tx_expect(c, '=')
           && tx_complex_syntax(c, NULL, NULL) && tx_expect(c, ')');
}

/* The start symbol of kind k derives the whole text (proto _syntax, lines 419-544). */
static int
tx_start_syntax(tx_cur * c, int k)
{
    int ok = 0;

    switch (k)
    {
        case ADF_TEXT_RAT:          /* rat_v = rat */
            ok = tx_scan_rat(c, 1, 1, NULL);
            break;
        case ADF_TEXT_FBALL:
            ok = tx_fball_syntax(c, NULL);
            break;
        case ADF_TEXT_ADELE:
            ok = tx_adele_syntax(c);
            break;
        case ADF_TEXT_CADELE:       /* "(" complex ";" fin ")" */
            ok = tx_expect(c, '(') && tx_complex_syntax(c, NULL, NULL) && tx_expect(c, ';')
                 && tx_fin_syntax(c, NULL) && tx_expect(c, ')');
            break;
        case ADF_TEXT_UCOSET:
            ok = tx_ucoset_syntax(c);
            break;
        case ADF_TEXT_IDELE:        /* "(" real ";" urat "*" ucoset ")" */
            ok = tx_expect(c, '(') && tx_real_syntax(c, NULL) && tx_expect(c, ';') && tx_scan_rat(c, 0, 1, NULL)
                 && tx_expect(c, '*') && tx_ucoset_syntax(c) && tx_expect(c, ')');
            break;
        case ADF_TEXT_IDCLASS:      /* "<" real ";" ucoset ">" */
            ok = tx_expect(c, '<') && tx_real_syntax(c, NULL) && tx_expect(c, ';') && tx_ucoset_syntax(c)
                 && tx_expect(c, '>');
            break;
        case ADF_TEXT_LBALL:        /* "[" "p" "=" uint ":" lcoord "]" */
            ok = tx_expect(c, '[') && tx_prime_entry_syntax(c) && tx_expect(c, ']');
            break;
        case ADF_TEXT_SBALL:        /* "{" [sentry {";" sentry}] "}" */
            if (!tx_expect(c, '{'))
                break;
            if (!tx_peek(c, '}'))
            {
                if (!tx_sentry_syntax(c))
                    break;
                while (tx_expect(c, ';'))
                    if (!tx_sentry_syntax(c))
                        return 0;
            }
            ok = tx_expect(c, '}');
            break;
        case ADF_TEXT_QCLASS:       /* adele_v "+" "Q" | "union" "(" adele_v {"," adele_v} ")" "+" "Q" */
            if (TX_KW(c, "union"))
            {
                if (!tx_expect(c, '(') || !tx_adele_syntax(c))
                    break;
                while (tx_expect(c, ','))
                    if (!tx_adele_syntax(c))
                        return 0;
                if (!tx_expect(c, ')'))
                    break;
            }
            else if (!tx_adele_syntax(c))
                break;
            ok = tx_expect(c, '+') && TX_KW(c, "Q");
            break;
        case ADF_TEXT_FFUN:         /* "ffun" "(" "D" "=" uint "," "M" "=" uint ";" complex {"," complex} ")" */
            if (!(TX_KW(c, "ffun") && tx_expect(c, '(') && TX_KW(c, "D") && tx_expect(c, '=')
                  && tx_scan_rat(c, 0, 0, NULL) && tx_expect(c, ',') && TX_KW(c, "M") && tx_expect(c, '=')
                  && tx_scan_rat(c, 0, 0, NULL) && tx_expect(c, ';') && tx_complex_syntax(c, NULL, NULL)))
                break;
            while (tx_expect(c, ','))
                if (!tx_complex_syntax(c, NULL, NULL))
                    return 0;
            ok = tx_expect(c, ')');
            break;
        case ADF_TEXT_RFUN:         /* "rfun" "(" [rterm {"," rterm}] ")" */
            if (!TX_KW(c, "rfun") || !tx_expect(c, '('))
                break;
            if (!tx_peek(c, ')'))
            {
                if (!tx_rterm_syntax(c))
                    break;
                while (tx_expect(c, ','))
                    if (!tx_rterm_syntax(c))
                        return 0;
            }
            ok = tx_expect(c, ')');
            break;
        default:    /* ADF_TEXT_CHAR: "char" "(" "q" "=" uint "," "n" "=" uint "," "s" "=" complex ")" */
            ok = TX_KW(c, "char") && tx_expect(c, '(') && TX_KW(c, "q") && tx_expect(c, '=')
                 && tx_scan_rat(c, 0, 0, NULL) && tx_expect(c, ',') && TX_KW(c, "n") && tx_expect(c, '=')
                 && tx_scan_rat(c, 0, 0, NULL) && tx_expect(c, ',') && TX_KW(c, "s") && tx_expect(c, '=')
                 && tx_complex_syntax(c, NULL, NULL) && tx_expect(c, ')');
            break;
    }
    return ok && tx_at_end(c);
}

int
adf_text_classify(adf_text_kind * kind, const char * s, size_t len, const adf_text_limits_t * lim)
{
    adf_text_limits_t store;
    tx_cur c;
    int k, st;

    lim = tx_limits(lim, &store);
    st = tx_prep(s, len, lim);
    if (st != ADF_OK)
        return st;
    /* the thirteen languages are pairwise disjoint (conventions 9.2, line 1136): the first match
       is the only one */
    for (k = ADF_TEXT_RAT; k <= ADF_TEXT_CHAR; k++)
    {
        c.s = s;
        c.len = len;
        c.i = 0;
        if (tx_start_syntax(&c, k))
        {
            *kind = (adf_text_kind) k;
            return ADF_OK;
        }
    }
    return ADF_PARSE;
}

/* ------------------------------------------------------------------------------------------------
   adf_ucoset, adf_idele, adf_idclass (lane t-slice1, milestone 2). Sources, read before the code:
   docs/conventions.md 5.6 (predicate; residue range 1..N; modulus as supplied, CV-17; normal form), 5.7
   (predicates of the idele and the class), 9.2 (ucoset_v, idele_v, idclass_v), 9.3 (constraints and
   canonicalisation on input), 9.4 (templates), 9.5 (reading; "Constrained printing"), 8.5 (order of
   checks); proto/text_grammar.py: Parser.ucoset (351-359), _syntax idele and idclass (449-464), _ucoset
   (668-680), ucoset_normal (683-692), _fmt_ucoset (736-740), _check_real (772-775), _build_and_print idele
   and idclass (822-836), _ceil_k (178-181), print_real_detail (189-218), _satisfies (221-228), print_real
   (231-250); docs/api-2.md 1.3 Statement E (kernel B) and src/idele_internal.h
   (adf_idele_ball_from_ends); refs/src/flint-3.0.1/arf.rst:676 (arf_fmpz_div_fmpz: correct rounding to prec
   bits), fmpz.rst:1142 (fmpz_remove). The two functions at the end of the section are the interface to
   src/text_idele.c and are hidden. */

#ifndef ADF_TX_HIDDEN
#if defined(__GNUC__) || defined(__clang__)
#define ADF_TX_HIDDEN __attribute__((visibility("hidden")))
#else
#define ADF_TX_HIDDEN
#endif
#endif

#define ADF_TX_FORM_UCOSET 0
#define ADF_TX_FORM_IDELE 1
#define ADF_TX_FORM_IDCLASS 2

/* ucoset = "[" int ["mod" uint] "]" with the literals kept (proto Parser.ucoset, 351-359). */
typedef struct
{
    tx_num c;
    int has_mod;
    tx_num N;
} tx_uc;

static int
tx_ucoset_scan(tx_cur * c, tx_uc * u)
{
    tx_uc t;

    memset(&t, 0, sizeof(t));
    if (!tx_expect(c, '[') || !tx_scan_rat(c, 1, 0, &t.c))
        return 0;
    if (TX_KW(c, "mod"))
    {
        if (!tx_scan_rat(c, 0, 0, &t.N))
            return 0;
        t.has_mod = 1;
    }
    if (!tx_expect(c, ']'))
        return 0;
    *u = t;
    return 1;
}

/* Stage 6 for a unit coset (conventions 9.3; proto _ucoset, 668-680). c and N are set to the unit as stored:
   N as written (0 when "mod" is absent or 0), c a residue in 1..N, or +-1 for N = 0 (5.6, CV-15 to CV-17).
   ADF_DOMAIN, c and N unspecified, if N = 0 and c is not +-1, or N >= 1 and gcd(c, N) != 1. */
static int
tx_unit_build(fmpz_t c, fmpz_t N, const char * s, const tx_uc * u)
{
    fmpz_t g;
    int ok;

    tx_fmpz_digits(c, s, u->c.ib, u->c.ie);
    if (u->c.neg)
        fmpz_neg(c, c);
    if (u->has_mod)
        tx_fmpz_digits(N, s, u->N.ib, u->N.ie);
    else
        fmpz_zero(N);
    if (fmpz_is_zero(N))
        return fmpz_is_pm1(c) ? ADF_OK : ADF_DOMAIN;
    fmpz_init(g);
    fmpz_gcd(g, c, N);
    ok = fmpz_is_one(g);
    fmpz_clear(g);
    if (!ok)
        return ADF_DOMAIN;
    fmpz_fdiv_r(c, c, N);
    if (fmpz_is_zero(c))
        fmpz_set(c, N);
    return ADF_OK;
}

/* The exact rational value of an accepted decimal (tx_dec_value), reduced. */
static void
tx_dec_fmpq(fmpq_t q, const char * s, const tx_num * n)
{
    fmpz_t num, den;

    fmpz_init(num);
    fmpz_init(den);
    tx_dec_value(num, den, s, n);
    fmpq_set_fmpz_frac(q, num, den);
    fmpz_clear(num);
    fmpz_clear(den);
}

/* lo = m - r and hi = m + r, the exact interval of an accepted real (conventions 9.5, "Reading"). */
static void
tx_real_interval(fmpq_t lo, fmpq_t hi, const char * s, const tx_real * r)
{
    fmpq_t m, d;

    fmpq_init(m);
    fmpq_init(d);
    tx_dec_fmpq(m, s, &r->m);
    if (r->has_r)
        tx_dec_fmpq(d, s, &r->r);
    fmpq_sub(lo, m, d);
    fmpq_add(hi, m, d);
    fmpq_clear(m);
    fmpq_clear(d);
}

/* z = the ball of the reader for a real whose exact interval [lo, hi] satisfies the condition (conventions
   9.3: excludes 0 for an idele, lies in (0, infinity) for a class), or ADF_NOT_DETERMINED (stage 7), z
   untouched. First the enclosing ball of 9.5 (exact for a dyadic input that fits: the tightness of 9.5);
   if it does not satisfy the condition, kernel B of api-2.md Statement E on the end points of the absolute
   interval rounded outwards at p = max(prec, 2) bits: lo' = RD_p(a) and hi' = RU_p(b) are dyadic with at
   most p bits, 0 < lo' <= hi', so adf_idele_ball_from_ends applies; it returns ADF_NOT_DETERMINED exactly
   when e(hi') - e(lo') > p (B1), and otherwise a ball that contains sign * [lo', hi'], hence [lo, hi], and
   excludes 0. */
static int
tx_real_ball_signed(arb_t z, const char * s, const tx_real * r, const fmpq_t lo, const fmpq_t hi, int positive,
                    slong prec)
{
    arb_t t;
    fmpq_t a, b;
    arf_t l, h;
    slong p = prec < 2 ? 2 : prec;
    int sign, st;

    arb_init(t);
    tx_arb_from_real(t, s, r, prec);
    if (positive ? arb_is_positive(t) : arb_is_nonzero(t))
    {
        arb_swap(z, t);
        arb_clear(t);
        return ADF_OK;
    }
    fmpq_init(a);
    fmpq_init(b);
    arf_init(l);
    arf_init(h);
    if (fmpq_sgn(lo) > 0)
    {
        sign = 1;
        fmpq_set(a, lo);
        fmpq_set(b, hi);
    }
    else
    {
        sign = -1;              /* hi < 0: the absolute interval is [-hi, -lo] */
        fmpq_neg(a, hi);
        fmpq_neg(b, lo);
    }
    (void) arf_fmpz_div_fmpz(l, fmpq_numref(a), fmpq_denref(a), p, ARF_RND_FLOOR);
    (void) arf_fmpz_div_fmpz(h, fmpq_numref(b), fmpq_denref(b), p, ARF_RND_CEIL);
    st = adf_idele_ball_from_ends(t, l, h, sign, p);
    if (st == ADF_OK)
        arb_swap(z, t);
    fmpq_clear(a);
    fmpq_clear(b);
    arf_clear(l);
    arf_clear(h);
    arb_clear(t);
    return st;
}

/* The stages 1 to 7 of conventions 8.5 for the three forms. On ADF_OK: c, N the unit as stored (tx_unit_build);
   for the idele and the class also inf, for the idele also r (the reduced content, > 0). Nothing is written on
   another status. A prec above ADF_IDELE_PREC_MAX is ADF_LIMIT before everything (the rule of adelefeld/idele.h;
   the ucoset form has no prec). */
ADF_TX_HIDDEN int adf_tx_read_unit_form(int form, arb_t inf, fmpq_t r, fmpz_t c, fmpz_t N, const char * s,
                                        size_t len, slong prec, const adf_text_limits_t * lim);

int
adf_tx_read_unit_form(int form, arb_t inf, fmpq_t r, fmpz_t c, fmpz_t N, const char * s, size_t len, slong prec,
                      const adf_text_limits_t * lim)
{
    adf_text_limits_t store;
    tx_cur cur;
    tx_real real;
    tx_num content;
    tx_uc u;
    fmpz_t cc, NN;
    fmpq_t rr, lo, hi;
    arb_t t;
    int st, syntax;

    if (form != ADF_TX_FORM_UCOSET && prec > ADF_IDELE_PREC_MAX)
        return ADF_LIMIT;
    lim = tx_limits(lim, &store);
    st = tx_prep(s, len, lim);
    if (st != ADF_OK)
        return st;
    cur.s = s;
    cur.len = len;
    cur.i = 0;
    memset(&real, 0, sizeof(real));
    memset(&content, 0, sizeof(content));
    memset(&u, 0, sizeof(u));
    if (form == ADF_TX_FORM_UCOSET)
        syntax = tx_ucoset_scan(&cur, &u);
    else if (form == ADF_TX_FORM_IDELE)      /* idele_v = "(" real ";" urat "*" ucoset ")" */
        syntax = tx_expect(&cur, '(') && tx_real_syntax(&cur, &real) && tx_expect(&cur, ';')
                 && tx_scan_rat(&cur, 0, 1, &content) && tx_expect(&cur, '*') && tx_ucoset_scan(&cur, &u)
                 && tx_expect(&cur, ')');
    else                                     /* idclass_v = "<" real ";" ucoset ">" */
        syntax = tx_expect(&cur, '<') && tx_real_syntax(&cur, &real) && tx_expect(&cur, ';')
                 && tx_ucoset_scan(&cur, &u) && tx_expect(&cur, '>');
    if (!syntax || !tx_at_end(&cur))
        return ADF_PARSE;
    /* stage 4: decimal exponents (the literals of a unit coset and of the content have no limit, 8.4) */
    if (form != ADF_TX_FORM_UCOSET && tx_real_over(s, &real, lim))
        return ADF_LIMIT;
    /* stage 6: a zero denominator of the content, then the unit, the content and the real part (all DOMAIN) */
    if (form == ADF_TX_FORM_IDELE && tx_den_zero(s, &content))
        return ADF_DOMAIN;
    fmpz_init(cc);
    fmpz_init(NN);
    fmpq_init(rr);
    fmpq_init(lo);
    fmpq_init(hi);
    arb_init(t);
    st = tx_unit_build(cc, NN, s, &u);
    if (st == ADF_OK && form == ADF_TX_FORM_IDELE)
    {
        tx_fmpq_rat(rr, s, &content);
        if (fmpq_sgn(rr) <= 0)
            st = ADF_DOMAIN;
    }
    if (st == ADF_OK && form != ADF_TX_FORM_UCOSET)
    {
        tx_real_interval(lo, hi, s, &real);
        if (form == ADF_TX_FORM_IDELE ? !(fmpq_sgn(lo) > 0 || fmpq_sgn(hi) < 0) : !(fmpq_sgn(lo) > 0))
            st = ADF_DOMAIN;
    }
    /* stage 7: the ball at the working precision */
    if (st == ADF_OK && form != ADF_TX_FORM_UCOSET)
        st = tx_real_ball_signed(t, s, &real, lo, hi, form == ADF_TX_FORM_IDCLASS, prec);
    if (st == ADF_OK)
    {
        fmpz_swap(c, cc);
        fmpz_swap(N, NN);
        if (form != ADF_TX_FORM_UCOSET)
            arb_swap(inf, t);
        if (form == ADF_TX_FORM_IDELE)
            fmpq_swap(r, rr);
    }
    fmpz_clear(cc);
    fmpz_clear(NN);
    fmpq_clear(rr);
    fmpq_clear(lo);
    fmpq_clear(hi);
    arb_clear(t);
    return st;
}

/* ---- the constrained printer of conventions 9.5 ("Constrained printing"; proto print_real_detail with k,
   189-218, _satisfies 221-228, print_real 231-250). All arithmetic is exact. ---- */

/* R = ceil_k(E), E > 0: E rounded up to k significant digits, k >= 2 (proto _ceil_k, 178-181); returns the unit
   exponent X(E) - k + 1, of which R is a multiple. tx_ceil2 is the case k = 2. */
static slong
tx_ceil_k(fmpq_t R, const fmpq_t E, slong k)
{
    slong unit = tx_floor_log10(E) - k + 1;
    fmpq_t u;
    fmpz_t f;

    fmpq_init(u);
    fmpz_init(f);
    tx_pow10_fmpq(u, unit);
    fmpq_div(R, E, u);
    fmpz_cdiv_q(f, fmpq_numref(R), fmpq_denref(R));
    fmpq_mul_fmpz(R, u, f);
    fmpq_clear(u);
    fmpz_clear(f);
    return unit;
}

/* An exponent q <= 0 such that the decimal fraction y (a rational whose denominator has no prime factor but 2 and
   5) is a multiple of 10^q: q = -max(v_2(d), v_5(d)) for y = n/d in lowest terms (fmpz_remove, fmpz.rst:1142). */
static slong
tx_dec_multiple_exp(const fmpq_t y)
{
    fmpz_t d, five;
    slong a, b;

    fmpz_init(d);
    fmpz_init_set_ui(five, 5);
    a = (slong) fmpz_val2(fmpq_denref(y));
    b = (slong) fmpz_remove(d, fmpq_denref(y), five);
    fmpz_clear(d);
    fmpz_clear(five);
    return -FLINT_MAX(a, b);
}

/* One level k of the algorithm of 9.5 (proto print_real_detail(mid, rad, n, k), 189-218) for exact rationals
   mid and rad >= 0, mid a decimal fraction (a dyadic midpoint, or a printed value) and rad likewise: the text
   is appended to b, and M, R are the printed midpoint and radius (R = 0 when the text has no radius). The
   text of level 2 is the text of the unconstrained printer tx_put_real. */
static void
tx_real_level(tx_buf * b, fmpq_t M, fmpq_t R, const fmpq_t mid, const fmpq_t rad, slong n, slong k)
{
    slong nk = n + k - 2, q, q2, unit;
    fmpq_t t;

    fmpq_init(t);
    /* step 1: rad = 0 and mid a decimal of at most n + k - 2 significant digits (0 included) */
    if (fmpq_is_zero(rad))
    {
        size_t kd = 0;
        slong E, qm = 0;

        if (!fmpq_is_zero(mid))
        {
            qm = tx_dec_multiple_exp(mid);
            flint_free(tx_decimal_digits(mid, qm, &kd, &E));
        }
        if ((slong) kd <= nk)
        {
            tx_put_fmt(b, mid, qm);
            fmpq_set(M, mid);
            fmpq_zero(R);
            goto done;
        }
    }
    /* step 2: mid = 0 */
    if (fmpq_is_zero(mid))
    {
        tx_puts(b, "0 +/- ");
        unit = tx_ceil_k(R, rad, k);
        tx_put_fmt(b, R, unit);
        fmpq_zero(M);
        goto done;
    }
    /* step 3 */
    q = tx_floor_log10(mid) - nk + 1;
    if (!fmpq_is_zero(rad))
        q = FLINT_MAX(q, tx_floor_log10(rad) - k + 1);
    /* steps 4 and 5 */
    for (;;)
    {
        tx_round_q(M, mid, q);
        fmpq_sub(t, M, mid);
        fmpq_abs(t, t);
        fmpq_add(t, t, rad);
        if (fmpq_is_zero(t))            /* proto: if R == 0: return fmt(M); not reached (see tx_put_real) */
        {
            fmpq_zero(R);
            tx_put_fmt(b, M, q);
            goto done;
        }
        unit = tx_ceil_k(R, t, k);
        if (fmpq_is_zero(M))
            q2 = tx_floor_log10(R) - k + 1;
        else
            q2 = FLINT_MAX(tx_floor_log10(M) - nk + 1, tx_floor_log10(R) - k + 1);
        if (tx_is_multiple(M, q2))
            break;
        q = q2;
    }
    /* step 6 */
    tx_put_fmt(b, M, q);
    tx_puts(b, " +/- ");
    tx_put_fmt(b, R, unit);
done:
    fmpq_clear(t);
}

/* The interval [M - R, M + R] satisfies the condition: it excludes 0, or (positive) lies in (0, infinity). */
static int
tx_interval_sat(const fmpq_t M, const fmpq_t R, int positive)
{
    fmpq_t lo, hi;
    int ok;

    fmpq_init(lo);
    fmpq_init(hi);
    fmpq_sub(lo, M, R);
    fmpq_add(hi, M, R);
    ok = positive ? fmpq_sgn(lo) > 0 : (fmpq_sgn(lo) > 0 || fmpq_sgn(hi) < 0);
    fmpq_clear(lo);
    fmpq_clear(hi);
    return ok;
}

/* The work of the constrained printer is bounded (review n-review1, D2; docs/api-2.md 4.2, Statement Q; decision
   N-D11).  One level costs about the size of its numbers, S bits (the larger of the bit lengths of the numerators
   and denominators of the midpoint and the radius, at least 64), and the levels are searched one after the other:
   the ball 2^(b-1) + 1/2 +/- 2^(b-1) needs about 0.3 b levels of 0.3 b digits; it takes 0.01 s, 0.09 s and 2.0 s at
   b = 1000, 4000 and 16000 (roughly b^2), and did not end in 170 s at b = 100000, which decision M1-D6 admits.  The
   printer adds S to a counter for every level it forms, over the whole call (the search and the repetitions), and stops with a refusal
   when the counter passes TX_COND_WORK_MAX: no text, the callers return NULL with *len = 0 as for M1-D6.  The bound
   is 2^25.  Measured on the ball above (lanes/n-repair1/printer-times.log): b = 4000 prints as before (0.10 s),
   b = 8000 is refused after 0.37 s (the search and its repetitions need about 2 * 2400 levels of 8000 bits), b =
   100000 is refused after 1.2 s.  Every ball whose passes together need at most 2^25 / S levels prints as
   before. */
#define TX_COND_WORK_MAX ((ulong) 1 << 25)

/* S of the numbers of one pass: the largest bit length among the four integers of mid and rad, at least 64. */
static ulong
tx_cond_size(const fmpq_t mid, const fmpq_t rad)
{
    ulong s = 64, t;

    t = fmpz_bits(fmpq_numref(mid));
    s = FLINT_MAX(s, t);
    t = fmpz_bits(fmpq_denref(mid));
    s = FLINT_MAX(s, t);
    t = fmpz_bits(fmpq_numref(rad));
    s = FLINT_MAX(s, t);
    t = fmpz_bits(fmpq_denref(rad));
    return FLINT_MAX(s, t);
}

/* The constrained printer (conventions 9.5): the least level k >= 2 at which the printed interval satisfies the
   condition; then the text is read back exactly and printed again until it does not change (proto print_real
   with cond, 231-250). The arb satisfies the condition (a precondition of the callers); if it did not, no k
   would exist and the loop would not end, so the unconstrained text (k = 2) is written instead.
   Returns 1 and writes nothing when the work bound TX_COND_WORK_MAX is passed, else 0. */
static int
tx_put_real_cond(tx_buf * b, const arb_t x, slong n, int positive)
{
    int refused = 0;
    ulong work = 0;

    fmpq_t mid, rad, M, R;
    arf_t r;
    char * prev = NULL;
    size_t prevlen = 0;
    tx_buf lb;

    fmpq_init(mid);
    fmpq_init(rad);
    fmpq_init(M);
    fmpq_init(R);
    arf_init(r);
    (void) tx_arf_get_fmpq(mid, arb_midref(x));
    arf_set_mag(r, arb_radref(x));        /* exact (arf.h:1035-1051) */
    (void) tx_arf_get_fmpq(rad, r);
    if (!tx_interval_sat(mid, rad, positive))
    {
        tx_buf_init(&lb);
        tx_real_level(&lb, M, R, mid, rad, n, 2);
        tx_put(b, lb.p, lb.len);
        flint_free(lb.p);
        goto done;
    }
    for (;;)
    {
        slong k = 2;
        ulong size = tx_cond_size(mid, rad);

        for (;;)
        {
            work += size;
            if (work > TX_COND_WORK_MAX)
            {
                refused = 1;
                goto done_refused;
            }
            tx_buf_init(&lb);
            tx_real_level(&lb, M, R, mid, rad, n, k);
            if (tx_interval_sat(M, R, positive))
                break;
            flint_free(lb.p);
            k++;
        }
        if (prev != NULL && prevlen == lb.len && memcmp(prev, lb.p, lb.len) == 0)
        {
            tx_put(b, lb.p, lb.len);
            flint_free(lb.p);
            break;
        }
        flint_free(prev);
        prev = lb.p;
        prevlen = lb.len;
        fmpq_set(mid, M);
        fmpq_set(rad, R);
    }
    flint_free(prev);
    goto done;
done_refused:
    flint_free(prev);
done:
    fmpq_clear(mid);
    fmpq_clear(rad);
    fmpq_clear(M);
    fmpq_clear(R);
    arf_clear(r);
    return refused;
}

/* The unit coset as printed (conventions 9.4): "[c]" for N = 0, else "[c mod N]"; c and N are the normal form. */
static void
tx_put_unit(tx_buf * b, const fmpz_t c, const fmpz_t N)
{
    tx_put(b, "[", 1);
    tx_put_fmpz(b, c);
    if (!fmpz_is_zero(N))
    {
        tx_puts(b, " mod ");
        tx_put_fmpz(b, N);
    }
    tx_put(b, "]", 1);
}

/* The templates of conventions 9.4: "[c mod N]" (c, N the normal form), "(r(x_inf) ; q(r) * U)" and
   "<r(t) ; U>", the real part by the constrained printer of 9.5 (nonzero for the idele, positive for the
   class). NULL with *len = 0 when the real ball is beyond the bound of decision M1-D6, or when the constrained
   printer passes its work bound TX_COND_WORK_MAX (decision N-D11). */
ADF_TX_HIDDEN char * adf_tx_write_unit_form(size_t * len, int form, const arb_t inf, const fmpq_t r,
                                            const fmpz_t c, const fmpz_t N, slong digits);

char *
adf_tx_write_unit_form(size_t * len, int form, const arb_t inf, const fmpq_t r, const fmpz_t c, const fmpz_t N,
                       slong digits)
{
    tx_buf b;

    if (form != ADF_TX_FORM_UCOSET && !tx_arb_printable(inf))
    {
        *len = 0;
        return NULL;
    }
    tx_buf_init(&b);
    if (form == ADF_TX_FORM_UCOSET)
        tx_put_unit(&b, c, N);
    else if (form == ADF_TX_FORM_IDELE)
    {
        tx_put(&b, "(", 1);
        if (tx_put_real_cond(&b, inf, digits, 0))
            goto refused;
        tx_puts(&b, " ; ");
        tx_put_q(&b, r);
        tx_puts(&b, " * ");
        tx_put_unit(&b, c, N);
        tx_put(&b, ")", 1);
    }
    else
    {
        tx_put(&b, "<", 1);
        if (tx_put_real_cond(&b, inf, digits, 1))
            goto refused;
        tx_puts(&b, " ; ");
        tx_put_unit(&b, c, N);
        tx_put(&b, ">", 1);
    }
    return tx_finish(&b, len);
refused:            /* the work bound of the constrained printer (TX_COND_WORK_MAX): as for M1-D6 */
    flint_free(b.p);
    *len = 0;
    return NULL;
}

/* ================================================================================================
   The value form of adf_lball and adf_sball (conventions 9.2 lball_v, sentry, sball_v; lane t-slice2,
   2026-10-04). Written by the lane as the second half of a file src/text_local.c whose first half was a
   verbatim copy of the static machinery above; the orchestrator joined it to this file instead, so that
   the machinery exists once. docs/api-1f.md, section "Slice 1F.5-c".
   ================================================================================================ */

#include <stdio.h>

/* Slice 3.1-d. Reuse the lexical machinery for lifts and union input.
   conventions 8.5: byte and full grammar checks precede all literal/count checks.
   Union semantics follow those checks; LIFT semantics are the adele reader's.
   docs/api-3.md 1 gives the numerical precision limit before reading input. */
int adf_qclass_set_str(adf_qclass_t x, const char *s, size_t len, slong prec,
                      const adf_text_limits_t *lim)
{
    adf_text_limits_t store;
    tx_cur c;
    tx_real r;
    tx_fin f;
    int st, is_union, over = 0;
    size_t count = 0;
    adf_adele_t a;

    if (prec > ADF_REAL_PREC_MAX) return ADF_LIMIT;
    lim = tx_limits(lim, &store);
    st = tx_prep(s, len, lim);
    if (st != ADF_OK) return st;
    c.s = s; c.len = len; c.i = 0;
    if (!tx_start_syntax(&c, ADF_TEXT_QCLASS)) return ADF_PARSE;
    c.i = 0;
    is_union = TX_KW(&c, "union");
    if (is_union) (void) tx_expect(&c, '(');
    do {
        (void) tx_expect(&c, '(');
        (void) tx_real_syntax(&c, &r);
        (void) tx_expect(&c, ';');
        (void) tx_fin_syntax(&c, &f);
        (void) tx_expect(&c, ')');
        if (tx_real_over(s, &r, lim)) over = 1;
        if (is_union && !over) {
            int j;
            for (j = 0; j <= r.has_r; j++) {
                const tx_num *number = j ? &r.r : &r.m;
                size_t ni = number->ie-number->ib, nf = number->fe-number->fb;
                size_t nd = ni+nf, first = nd, k, significant;
                slong e;
                for (k = 0; k < nd; k++) {
                    char digit = k < ni ? s[number->ib+k] : s[number->fb+k-ni];
                    if (digit != '0' && first == nd) first = k;
                }
                if (first == nd) continue; /* tx_dec_value also avoids powers for exact zero. */
                e = tx_dec_exp(s, number);
                if (e > ADF_QCLASS_BITS_MAX || e < -ADF_QCLASS_BITS_MAX) { over = 1; continue; }
                significant = nd-first;
                if (nf > (size_t) ADF_QCLASS_BITS_MAX) { over = 1; continue; }
                e -= (slong) nf;
                /* D3-2 projected decimal sizes, BEFORE powers: 10^k < 2^(4k).
                   Bound the intermediates that tx_dec_value actually forms before cancellation. */
                if (significant > (size_t) ADF_QCLASS_BITS_MAX/4 ||
                    (e > 0 && (size_t) e > (size_t) ADF_QCLASS_BITS_MAX/4-significant) ||
                    e < -ADF_QCLASS_BITS_MAX/4) over = 1;
            }
        }
        count++;
    } while (is_union && tx_expect(&c, ','));
    if (over || (is_union && (lim->max_items < 1 || count > (size_t) lim->max_items)))
        return ADF_LIMIT;
    if (is_union) {
        adf_adele_struct *pieces;
        fmpq_t mid, rad, stored, delta;
        arf_t num, den;
        size_t i;
        if (count > (size_t) WORD_MAX || count > (size_t) -1/sizeof(*pieces)) return ADF_LIMIT;
        pieces = flint_malloc(count * sizeof(*pieces));
        for (i = 0; i < count; i++) adf_adele_init(pieces+i);
        fmpq_init(mid); fmpq_init(rad); fmpq_init(stored); fmpq_init(delta);
        arf_init(num); arf_init(den);
        c.i = 0; (void) TX_KW(&c, "union"); (void) tx_expect(&c, '(');
        st = ADF_OK;
        for (i = 0; i < count; i++) {
            (void) tx_expect(&c, '('); (void) tx_real_syntax(&c, &r);
            (void) tx_expect(&c, ';'); (void) tx_fin_syntax(&c, &f); (void) tx_expect(&c, ')');
            if (i+1 < count) (void) tx_expect(&c, ',');
            if (tx_fin_domain(s, &f)) { st = ADF_DOMAIN; break; }
            tx_dec_fmpq(mid, s, &r.m);
            /* CV-45 and conventions 9.3: test the exact input midpoint, then RN_p.
               refs/src/flint-3.0.1/arf.rst:24-35, :662-681 (correctly rounded division).
               Monotonic rounding fixes 0 and 1 and therefore preserves this range. */
            if (fmpq_sgn(mid) < 0 || fmpq_cmp_ui(mid, 1) > 0) { st = ADF_DOMAIN; break; }
            fmpq_zero(rad);
            if (r.has_r) tx_dec_fmpq(rad, s, &r.r);
            arf_set_fmpz(num, fmpq_numref(mid)); arf_set_fmpz(den, fmpq_denref(mid));
            arf_div(arb_midref(pieces[i].inf), num, den, FLINT_MAX(prec, 2), ARF_RND_NEAR);
            arf_get_fmpq(stored, arb_midref(pieces[i].inf));
            fmpq_sub(delta, mid, stored); fmpq_abs(delta, delta); fmpq_add(rad, rad, delta);
            tx_mag_upper(arb_radref(pieces[i].inf), rad);
            tx_fin_build(&pieces[i].fin, s, &f);
        }
        if (st == ADF_OK) st = adf_qclass_set_pieces(x, pieces, (slong) count, lim->max_items);
        for (i = 0; i < count; i++) adf_adele_clear(pieces+i);
        flint_free(pieces); fmpq_clear(mid); fmpq_clear(rad); fmpq_clear(stored); fmpq_clear(delta);
        arf_clear(num); arf_clear(den);
        return st;
    }
    if (tx_fin_domain(s, &f)) return ADF_DOMAIN;
    adf_adele_init(a);
    tx_arb_from_real(a->inf, s, &r, prec);
    tx_fin_build(&a->fin, s, &f);
    adf_qclass_set_adele(x, a);
    adf_adele_clear(a);
    return ADF_OK;
}

#include <stdlib.h>

/* conventions 9.4, 5.10: order/deduplicate the printed endpoint keys, after the
   ordinary adele printer has enclosed each stored ball. No union reader is added. */
typedef struct {
    char *s;
    size_t n;
    fmpq_t lo, hi;
    fmpz_t H, A;
} tq_printed;

static int tq_print_cmp(const void *vp, const void *vq)
{
    const tq_printed *p = vp, *q = vq;
    int c = fmpq_cmp(p->lo, q->lo);
    if (!c) c = fmpq_cmp(p->hi, q->hi);
    if (!c) c = fmpz_cmp(p->H, q->H);
    if (!c) c = fmpz_cmp(p->A, q->A);
    return c;
}

char *adf_qclass_get_str(size_t *len, const adf_qclass_t x, slong digits)
{
    char *s;
    size_t n;
    tx_buf b;
    tq_printed *p;
    slong i, initialized = 0;
    int failed = 0;
#ifdef ADF_CHECK_INVARIANTS
    if (!adf_qclass_is_canonical(x)) adf_inv_fail(__func__, "x", "adf_qclass");
#endif
    if (x->form == ADF_QCLASS_PIECES) {
        if ((size_t) x->len > (size_t) -1/sizeof(*p)) { *len = 0; return NULL; }
        p = flint_malloc((size_t) x->len*sizeof(*p));
        for (i = 0; i < x->len; i++) {
            tx_cur c; tx_real r; fmpz_t d;
            fmpq_init(p[i].lo); fmpq_init(p[i].hi); fmpz_init(p[i].H); fmpz_init(p[i].A);
            initialized++;
            p[i].s = adf_adele_get_str(&p[i].n, x->piece+i, digits);
            if (!p[i].s) { failed = 1; break; }
            c.s = p[i].s; c.len = p[i].n; c.i = 0;
            (void) tx_expect(&c, '('); (void) tx_real_syntax(&c, &r);
            tx_real_interval(p[i].lo, p[i].hi, p[i].s, &r);
            fmpz_init(d); adf_fball_get_fmpz3(p[i].A, p[i].H, d, &x->piece[i].fin); fmpz_clear(d);
        }
        s = NULL;
        if (!failed) {
            qsort(p, (size_t) x->len, sizeof(*p), tq_print_cmp);
            tx_buf_init(&b); tx_puts(&b, "union(");
            for (i = 0; i < x->len; i++) {
                if (i && !tq_print_cmp(p+i-1, p+i)) continue;
                if (i) tx_puts(&b, ", ");
                tx_put(&b, p[i].s, p[i].n);
            }
            tx_puts(&b, ") + Q"); s = tx_finish(&b, len);
        } else *len = 0;
        for (i = 0; i < initialized; i++) {
            adf_str_free(p[i].s); fmpq_clear(p[i].lo); fmpq_clear(p[i].hi);
            fmpz_clear(p[i].H); fmpz_clear(p[i].A);
        }
        flint_free(p); return s;
    }
    s = adf_adele_get_str(&n, x->piece, digits);
    if (!s) { *len = 0; return NULL; }
    tx_buf_init(&b);
    tx_put(&b, s, n);
    tx_puts(&b, " + Q");
    adf_str_free(s);
    return tx_finish(&b, len);
}
#include <stdlib.h>   /* qsort */

/* The invariant check of the debug build (conventions 4.4, CV-09; src/invariants.h has none for the two
   types, src/lball.c has one of its own for adf_lball). The output of the reader is not checked on a
   status; on ADF_OK it always satisfies the predicate (the components come from the constructors). */
#ifdef ADF_CHECK_INVARIANTS
#define ADF_INV_LBALL_OUT(x)                                                                           \
    do                                                                                                \
    {                                                                                                 \
        if (!adf_lball_is_canonical(x))                                                               \
            flint_abort();                                                                             \
    }                                                                                                 \
    while (0)
#else
#define ADF_INV_LBALL_OUT(x) ((void) 0)
#endif

/* ------------------------------------------------------------------------------------------------
   Comparisons on digit strings. Every limit of stage 4 and the word restriction of stage 5 are decided on
   the digits of the literal, before any number is formed (conventions 8.5 items 4 and 5; decision M1-D7;
   the reference compares the same strings, proto/text_grammar.py _check_limits lines 579-599 and
   _check_unsupported lines 607-617). */

/* 1 if the integer of the digits [b, e) of s (leading zeros allowed, e > b) is above m, m >= 0 a slong.
   The digit string may be as long as the input. */
static int
tl_digits_over_slong(const char * s, size_t b, size_t e, slong m)
{
    char lbuf[24];
    size_t ld = 0, nd, i;
    slong t = m;

    while (b < e && s[b] == '0')
        b++;
    if (b == e)
        return 0;                        /* the value is 0, which no positive m exceeds */
    while (t > 0)
    {
        lbuf[ld++] = (char) ('0' + (int) (t % 10));
        t /= 10;
    }
    nd = e - b;
    if (nd != ld)
        return nd > ld;
    for (i = 0; i < nd; i++)
    {
        char sc = s[b + i];
        char lc = lbuf[ld - 1 - i];

        if (sc != lc)
            return sc > lc;
    }
    return 0;
}

/* -1, 0, 1 as the integer of the digits [b, e) of s compares with the integer of the decimal digits d
   (which has no leading zero). */
static int
tl_digits_cmp_str(const char * s, size_t b, size_t e, const char * d)
{
    size_t i;

    while (b < e && s[b] == '0')
        b++;
    if (e - b != strlen(d))
        return (e - b < strlen(d)) ? -1 : 1;
    for (i = 0; i < e - b; i++)
        if (s[b + i] != d[i])
            return s[b + i] < d[i] ? -1 : 1;
    return 0;
}

/* 1 if the prime literal n is at least 2^64: ADF_UNSUPPORTED at stage 5 (conventions 9.3 line 1155; the
   reference, _check_unsupported: len(v) > 20 or int(v) >= WORD). 2^64 = 18446744073709551616. */
static int
tl_prime_over_word(const char * s, const tx_num * n)
{
    return tl_digits_cmp_str(s, n->ib, n->ie, "18446744073709551616") >= 0;
}

/* The value of a sint literal n, with |value| <= ADF_LBALL_EXP_MAX (2^60), so that it fits a slong: the
   callers have compared the digits with max_prec and with ADF_LBALL_EXP_MAX first. */
static slong
tl_sint_value(const char * s, const tx_num * n)
{
    slong v = 0;
    size_t b;

    for (b = n->ib; b < n->ie; b++)
        v = 10 * v + (s[b] - '0');
    return n->neg ? -v : v;
}

/* The base inside "O(...)" as an unsigned long, or UINTP_MAX when the literal is at least 2^64 (the
   comparison with the prime then fails and the status is ADF_DOMAIN, stage 6; the base has no word
   restriction of its own, conventions 9.3 names it only in "the base inside O(...) equals p"). */
static int
tl_base_get_ulong(const char * s, const tx_num * n, ulong * out)
{
    fmpz_t z;

    if (tl_prime_over_word(s, n))
        return 0;
    fmpz_init(z);
    tx_fmpz_digits(z, s, n->ib, n->ie);
    *out = fmpz_get_ui(z);
    fmpz_clear(z);
    return 1;
}

/* ------------------------------------------------------------------------------------------------
   lcoord = rat ["+" "O" "(" uint ["^" sint] ")"] (conventions 9.2 line 1116; the reference, Parser.lcoord
   lines 359-372: a, then "+" "O" "(" uint, an optional "^" sint, and ")"). The literals are kept as
   offsets into the input; nothing is converted here. */

typedef struct
{
    tx_num a;             /* the centre, a rat literal */
    int has_O;            /* an O-term follows */
    tx_num base;          /* the base inside O(...), a uint literal */
    tx_num ex;            /* the exponent, a sint literal */
    int has_exp;          /* "^" exponent present; else N = 1 */
} tl_lcoord;

static int
tl_lcoord_syntax(tx_cur * c, tl_lcoord * l)
{
    tl_lcoord t;

    memset(&t, 0, sizeof(t));
    if (!tx_scan_rat(c, 1, 1, &t.a))
        return 0;
    if (tx_peek(c, '+'))
    {
        c->i++;
        t.has_O = 1;
        if (!TX_KW(c, "O") || !tx_expect(c, '(') || !tx_scan_rat(c, 0, 0, &t.base))
            return 0;
        if (tx_expect(c, '^'))
        {
            if (!tx_scan_rat(c, 1, 0, &t.ex))
                return 0;
            t.has_exp = 1;
        }
        if (!tx_expect(c, ')'))
            return 0;
    }
    if (l != NULL)
        *l = t;
    return 1;
}

/* One entry of a partial ball: the archimedean place (real or complex) or a prime with its local
   coordinate (sentry of conventions 9.2 line 1127; the reference, Parser.sentry lines 382-394: "inf" and
   ":" and a complex when "(" follows, else a real; or "p" "=" uint ":" lcoord). */
#define TL_INF_REAL 0
#define TL_INF_COMPLEX 1
#define TL_PRIME 2

typedef struct
{
    int kind;
    tx_num p;             /* the prime literal, a uint */
    tx_real re, im;       /* the archimedean parts */
    tl_lcoord lc;
} tl_entry;

static int
tl_sentry_syntax(tx_cur * c, tl_entry * e)
{
    tl_entry t;

    memset(&t, 0, sizeof(t));
    if (TX_KW(c, "inf"))
    {
        if (!tx_expect(c, ':'))
            return 0;
        if (tx_peek(c, '('))
        {
            t.kind = TL_INF_COMPLEX;
            if (!tx_complex_syntax(c, &t.re, &t.im))
                return 0;
        }
        else
        {
            t.kind = TL_INF_REAL;
            if (!tx_real_syntax(c, &t.re))
                return 0;
        }
    }
    else
    {
        t.kind = TL_PRIME;
        if (!TX_KW(c, "p") || !tx_expect(c, '=') || !tx_scan_rat(c, 0, 0, &t.p) || !tx_expect(c, ':'))
            return 0;
        if (!tl_lcoord_syntax(c, &t.lc))
            return 0;
    }
    if (e != NULL)
        *e = t;
    return 1;
}

/* Stage 3 for lball_v = "[" "p" "=" uint ":" lcoord "]" (conventions 9.2 line 1126; the reference,
   _syntax lball, lines 465-471): the entry of a prime place, the closing bracket, and nothing else. The
   reader below calls tl_sentry_syntax_prime directly, so the whole input is checked with tx_at_end. */

/* the "p" "=" uint ":" lcoord of a local entry, without the archimedean alternatives */
static int
tl_sentry_syntax_prime(tx_cur * c, tl_entry * e)
{
    tl_entry t;

    memset(&t, 0, sizeof(t));
    t.kind = TL_PRIME;
    if (!TX_KW(c, "p") || !tx_expect(c, '=') || !tx_scan_rat(c, 0, 0, &t.p) || !tx_expect(c, ':'))
        return 0;
    if (!tl_lcoord_syntax(c, &t.lc))
        return 0;
    if (e != NULL)
        *e = t;
    return 1;
}

/* ------------------------------------------------------------------------------------------------
   The byte range of one entry of a partial ball. sball_v = "{" [sentry {";" sentry}] "}" (conventions 9.2
   line 1128; the reference, _syntax sball, lines 472-481). The ranges are recorded instead of the entries
   themselves: the passes of stages 4 to 6 then walk the input again and nothing of the size of the number
   of entries is held. The number of entries is bounded by the length of the input (each entry needs at
   least five bytes: "p=2:"). */
typedef struct
{
    size_t b, e;
} tl_range;

typedef struct
{
    tl_range * r;
    size_t n, cap;
} tl_list;

static void
tl_list_init(tl_list * l)
{
    l->r = NULL;
    l->n = 0;
    l->cap = 0;
}

static void
tl_list_clear(tl_list * l)
{
    flint_free(l->r);
    l->r = NULL;
    l->n = 0;
    l->cap = 0;
}

static int
tl_list_push(tl_list * l, size_t b, size_t e)
{
    if (l->n == l->cap)
    {
        size_t cap = l->cap ? 2 * l->cap : 8;
        tl_range * r = (tl_range *) flint_realloc(l->r, cap * sizeof(tl_range));

        l->r = r;
        l->cap = cap;
    }
    l->r[l->n].b = b;
    l->r[l->n].e = e;
    l->n++;
    return 1;
}

static int
tl_sball_syntax(tx_cur * c, tl_list * l)
{
    if (!tx_expect(c, '{'))
        return 0;
    /* "{" [sentry {";" sentry}] "}": a ';' must be followed by an entry, so "{p=5: 1;}" is ADF_PARSE */
    if (!tx_peek(c, '}'))
    {
        for (;;)
        {
            size_t b;

            tx_ws(c);
            b = c->i;
            if (!tl_sentry_syntax(c, NULL))
                return 0;
            (void) tl_list_push(l, b, c->i);
            if (!tx_peek(c, ';'))
                break;
            c->i++;
        }
    }
    return tx_expect(c, '}') && tx_at_end(c);
}

/* The entry of the range [b, e) of the input, re-read with a cursor of its own. The text passed stage 3, so
   the return value is 1; 0 is kept as a safety net and would give ADF_PARSE. */
static int
tl_entry_at(const char * s, size_t b, size_t e, tl_entry * out)
{
    tx_cur c;

    c.s = s;
    c.len = e;
    c.i = b;
    return tl_sentry_syntax(&c, out) && tx_at_end(&c);
}

/* ------------------------------------------------------------------------------------------------
   The two limits of the library that are not in conventions 8.4, and the value of N. */

/* 1 if the O-term of the entry has an exponent above lim->max_prec (stage 4, ADF_LIMIT) or above
   ADF_LBALL_EXP_MAX, the bound of lball.h (also ADF_LIMIT; a second limit of the library, decided on the
   digits so that N fits a slong). No exponent means N = 1, which no bound rejects. */
static int
tl_exp_over(const char * s, const tl_entry * e, const adf_text_limits_t * lim)
{
    if (e->kind != TL_PRIME || !e->lc.has_O || !e->lc.has_exp)
        return 0;
    if (tl_digits_over_slong(s, e->lc.ex.ib, e->lc.ex.ie, lim->max_prec))
        return 1;
    return tl_digits_over_slong(s, e->lc.ex.ib, e->lc.ex.ie, ADF_LBALL_EXP_MAX);
}

/* The absolute precision N of the O-term: the exponent as written, 1 when it is absent ("O(p)" means
   N = 1, conventions 9.2 and 9.3). The caller has checked the two bounds of tl_exp_over. */
static slong
tl_exp_value(const char * s, const tl_entry * e)
{
    if (!e->lc.has_exp)
        return 1;
    return tl_sint_value(s, &e->lc.ex);
}

/* Stage 4 of one entry of a partial ball: the decimal exponents of the archimedean parts (max_exp10) and
   the exponent of an O-term (max_prec). */
static int
tl_entry_over(const char * s, const tl_entry * e, const adf_text_limits_t * lim)
{
    switch (e->kind)
    {
        case TL_INF_REAL:
            return tx_real_over(s, &e->re, lim);
        case TL_INF_COMPLEX:
            return tx_real_over(s, &e->re, lim) || tx_real_over(s, &e->im, lim);
        default:
            return tl_exp_over(s, e, lim);
    }
}

/* ------------------------------------------------------------------------------------------------
   The value of one entry. The local components are built with the constructors of lball.h, so that they
   satisfy the predicate of conventions 5.8 and the output of the reader needs no further check. */

/* x = the local coordinate of the entry at the prime p of the place v. The canonical centre of
   conventions 5.8 is formed by adf_lball_set_rat_ball (statement L1 of docs/api-1f.md): the centre is the
   unique element of Z[1/p] in [0, p^N) that lies in the ball, and an exact value is stored as p^v u by
   adf_lball_set_rat. Statuses: ADF_OK; ADF_LIMIT from the constructor (a centre that needs too large a
   power); ADF_DOMAIN only for the archimedean place, which the caller has excluded. */
static int
tl_lball_build(adf_lball_t x, adf_place_t v, const char * s, const tl_entry * e, slong N)
{
    adf_rat_t q;
    int st;

    adf_rat_init(q);
    tx_fmpq_rat(q->q, s, &e->lc.a);
    if (e->lc.has_O)
        st = adf_lball_set_rat_ball(x, v, q, N);
    else
        st = adf_lball_set_rat(x, v, q);
    adf_rat_clear(q);
    return st;
}

/* ------------------------------------------------------------------------------------------------
   adf_lball (lane t-slice2). */

int
adf_lball_set_str(adf_lball_t x, const char * s, size_t len, const adf_text_limits_t * lim)
{
    adf_text_limits_t store;
    tx_cur c;
    tl_entry e;
    adf_lball_t t;
    adf_place_t v;
    ulong p;
    fmpz_t pz;
    int st;

    /* stage 1 (len > max_len, before any byte is read) and stage 2 (the alphabet of conventions 8.2) */
    lim = tx_limits(lim, &store);
    st = tx_prep(s, len, lim);
    if (st != ADF_OK)
        return st;
    /* stage 3: the grammar, and the whole input */
    c.s = s;
    c.len = len;
    c.i = 0;
    if (!(tx_expect(&c, '[') && tl_sentry_syntax_prime(&c, &e) && tx_expect(&c, ']') && tx_at_end(&c)))
        return ADF_PARSE;
    /* stage 4: abs(N) <= max_prec, and abs(N) <= ADF_LBALL_EXP_MAX (a bound of lball.h); both on the
       digits of the literal, before any number is formed */
    if (tl_exp_over(s, &e, lim))
        return ADF_LIMIT;
    /* stage 5: p < 2^64 */
    if (tl_prime_over_word(s, &e.p))
        return ADF_UNSUPPORTED;
    /* stage 6: a denominator of the centre that is 0, the base inside O(...) that is not p, and a prime
       that is not prime (n_is_prime through adf_place_prime). All DOMAIN, so the order is free. */
    if (tx_den_zero(s, &e.lc.a))
        return ADF_DOMAIN;
    fmpz_init(pz);
    tx_fmpz_digits(pz, s, e.p.ib, e.p.ie);
    p = fmpz_get_ui(pz);
    fmpz_clear(pz);
    if (e.lc.has_O)
    {
        ulong base;

        if (!tl_base_get_ulong(s, &e.lc.base, &base) || base != p)
            return ADF_DOMAIN;
    }
    st = adf_place_prime(&v, p);
    if (st != ADF_OK)
        return st;                       /* ADF_DOMAIN: not a prime; *v untouched */
    /* build in a temporary and move it in only on ADF_OK (conventions 4.3) */
    adf_lball_init(t);
    st = tl_lball_build(t, v, s, &e, tl_exp_value(s, &e));
    if (st == ADF_OK)
    {
        adf_lball_swap(x, t);
        ADF_INV_LBALL_OUT(x);
    }
    adf_lball_clear(t);
    return st;
}

/* The local coordinate L of conventions 9.4 (line 1206): "q(p^v u)" when the value is exact, and
   "q(c) + O(p^N)" otherwise, with c the canonical centre of conventions 5.8 (the rational p^v u, which is
   the unique element of Z[1/p] in [0, p^N) that lies in the ball) and N in signed decimal. Returns 0 when
   the centre does not fit, which adf_lball_get_center reports as ADF_LIMIT (|v| bits(p) >
   ADF_LBALL_BITS_MAX), and the caller then refuses the text. */
static int
tl_put_lcoord(tx_buf * b, const adf_lball_t x)
{
    adf_rat_t c;
    int st;

    adf_rat_init(c);
    st = adf_lball_get_center(c, x);
    if (st == ADF_OK)
        tx_put_q(b, c->q);
    adf_rat_clear(c);
    if (st != ADF_OK)
        return 0;
    if (x->exact)
        return 1;
    tx_puts(b, " + O(");
    {
        fmpz_t pz;

        fmpz_init_set_ui(pz, x->p);
        tx_put_fmpz(b, pz);
        fmpz_clear(pz);
    }
    tx_put(b, "^", 1);
    {
        char nbuf[32];

        flint_sprintf(nbuf, "%wd", x->N);
        tx_puts(b, nbuf);
    }
    tx_put(b, ")", 1);
    return 1;
}

char *
adf_lball_get_str(size_t * len, const adf_lball_t x)
{
    tx_buf b;

#ifdef ADF_CHECK_INVARIANTS
    /* conventions 4.4, CV-09: a function that reads a value checks its predicate on entry */
    if (!adf_lball_is_canonical(x))
    {
        fprintf(stderr, "adelefeld: ADF_CHECK_INVARIANTS: %s: argument %s is not a canonical adf_lball\n",
                __func__, "x");
        fflush(stderr);
        flint_abort();
    }
#endif
    tx_buf_init(&b);
    tx_puts(&b, "[p=");
    {
        fmpz_t pz;

        fmpz_init_set_ui(pz, x->p);
        tx_put_fmpz(&b, pz);
        fmpz_clear(pz);
    }
    tx_puts(&b, ": ");
    if (!tl_put_lcoord(&b, x))
    {
        /* the centre of an exact value needs p^v with |v| bits(p) > ADF_LBALL_BITS_MAX: refused, as the
           printers of the value form refuse a real part beyond the bound of decision M1-D6 */
        flint_free(b.p);
        *len = 0;
        return NULL;
    }
    tx_put(&b, "]", 1);
    return tx_finish(&b, len);
}

/* ------------------------------------------------------------------------------------------------
   adf_sball (lane t-slice2). */

/* The keyed local components, sorted into the canonical order of places (conventions 7: the archimedean
   place first, then the primes increasing). The sort is qsort, as in src/sball.c. */
typedef struct
{
    ulong p;
    size_t idx;
} tl_keyed;

static int
tl_keyed_cmp(const void * a, const void * b)
{
    const tl_keyed * x = (const tl_keyed *) a;
    const tl_keyed * y = (const tl_keyed *) b;

    if (x->p < y->p)
        return -1;
    if (x->p > y->p)
        return 1;
    return 0;
}

int
adf_sball_set_str(adf_sball_t x, const char * s, size_t len, slong prec, const adf_text_limits_t * lim)
{
    adf_text_limits_t store;
    tx_cur c;
    tl_list list;
    tl_entry * entries = NULL;
    tl_keyed * keys = NULL;
    adf_lball_struct * loc = NULL;
    acb_t z;
    slong i, n, m = 0, ninf = 0, inf_at = -1;
    int arch = ADF_ARCH_NONE, st = ADF_OK;

    /* The bound of prec of sball.h, decided from prec alone before the text is read (as every function of
       prec in sball.h decides it; conventions 8.5 has no stage for it). */
    if (prec > ADF_REAL_PREC_MAX)
        return ADF_LIMIT;
    lim = tx_limits(lim, &store);
    st = tx_prep(s, len, lim);
    if (st != ADF_OK)
        return st;
    /* stage 3: the grammar of sball_v, and the whole input */
    tl_list_init(&list);
    c.s = s;
    c.len = len;
    c.i = 0;
    if (!tl_sball_syntax(&c, &list))
    {
        tl_list_clear(&list);
        return ADF_PARSE;
    }
    n = (slong) list.n;
    /* stage 4: the number of entries (max_items, the "items" of conventions 8.4), checked on the count
       and before any semantic check of stage 6 (gate finding G8) */
    if (lim->max_items < 0 || (size_t) lim->max_items < list.n)
    {
        tl_list_clear(&list);
        return ADF_LIMIT;
    }
    /* stage 4 for every entry: the decimal exponents and the exponent of every O-term */
    entries = (tl_entry *) flint_malloc((list.n ? list.n : 1) * sizeof(tl_entry));
    for (i = 0; i < n; i++)
    {
        if (!tl_entry_at(s, list.r[i].b, list.r[i].e, &entries[i]))
        {
            tl_list_clear(&list);
            flint_free(entries);
            return ADF_PARSE;
        }
        if (tl_entry_over(s, &entries[i], lim))
        {
            tl_list_clear(&list);
            flint_free(entries);
            return ADF_LIMIT;
        }
        if (entries[i].kind != TL_PRIME)
        {
            ninf++;
            inf_at = i;
        }
        else
            m++;
    }
    /* stage 6: at most one inf entry and no prime twice (conventions 9.3 line 1159), checked before the
       entries themselves, because both are DOMAIN and the status does not depend on the order */
    if (ninf > 1)
    {
        tl_list_clear(&list);
        flint_free(entries);
        return ADF_DOMAIN;
    }
    keys = (tl_keyed *) flint_malloc((m > 0 ? (size_t) m : 1) * sizeof(tl_keyed));
    m = 0;
    for (i = 0; i < n; i++)
    {
        tl_entry * ee = &entries[i];

        if (ee->kind != TL_PRIME)
            continue;
        /* stage 5: the prime is below 2^64 */
        if (tl_prime_over_word(s, &ee->p))
        {
            st = ADF_UNSUPPORTED;
            goto done;
        }
        /* stage 6: a denominator of the centre that is 0, the base inside O(...) that is not the prime,
           and a prime that is not prime. All DOMAIN. */
        if (tx_den_zero(s, &ee->lc.a))
        {
            st = ADF_DOMAIN;
            goto done;
        }
        {
            fmpz_t pz;
            ulong p;

            fmpz_init(pz);
            tx_fmpz_digits(pz, s, ee->p.ib, ee->p.ie);
            p = fmpz_get_ui(pz);
            fmpz_clear(pz);
            if (ee->lc.has_O)
            {
                ulong base;

                if (!tl_base_get_ulong(s, &ee->lc.base, &base) || base != p)
                {
                    st = ADF_DOMAIN;
                    goto done;
                }
            }
            keys[m].p = p;
            keys[m].idx = (size_t) i;
            m++;
        }
    }
    /* the canonical order of the primes, and the refusal of a repetition (conventions 7) */
    qsort(keys, (size_t) m, sizeof(tl_keyed), tl_keyed_cmp);
    for (i = 1; i < m; i++)
        if (keys[i - 1].p == keys[i].p)
        {
            st = ADF_DOMAIN;
            goto done;
        }
    /* the components, in the canonical order */
    if (m > 0)
        loc = (adf_lball_struct *) flint_malloc((size_t) m * sizeof(adf_lball_struct));
    for (i = 0; i < m; i++)
        adf_lball_init(&loc[i]);
    for (i = 0; i < m; i++)
    {
        tl_entry * ee = &entries[keys[i].idx];
        adf_place_t v;
        int s2;

        if (adf_place_prime(&v, keys[i].p) != ADF_OK)
        {
            st = ADF_DOMAIN;
            goto done;
        }
        s2 = tl_lball_build(&loc[i], v, s, ee, tl_exp_value(s, ee));
        if (s2 != ADF_OK)
        {
            st = s2;
            goto done;
        }
    }
    /* the archimedean component: the enclosing ball of conventions 9.5 ("Reading"). There is no sign
       condition on it (9.3), so stage 7 does not occur. */
    acb_init(z);
    if (ninf == 1)
    {
        tl_entry * ee = &entries[inf_at];

        arch = (ee->kind == TL_INF_COMPLEX) ? ADF_ARCH_COMPLEX : ADF_ARCH_REAL;
        tx_arb_from_real(acb_realref(z), s, &ee->re, prec);
        if (arch == ADF_ARCH_COMPLEX)
            tx_arb_from_real(acb_imagref(z), s, &ee->im, prec);
    }
    /* commit: the value of x is replaced only now (conventions 4.3) */
    {
        adf_sball_t t;

        adf_sball_init(t);
        if (arch != ADF_ARCH_COMPLEX)
        {
            /* the constructor of sball.h, which sorts, copies and checks the components */
            st = adf_sball_set_arb_lballs(t, NULL, arch == ADF_ARCH_REAL ? acb_realref(z) : NULL, loc, m);
        }
        else
        {
            /* no function of sball.h makes a COMPLEX value; the struct is filled as a binding does (the
               comment of the struct in sball.h) */
            slong k;

            t->arch = ADF_ARCH_COMPLEX;
            acb_set(t->inf, z);
            t->len = m;
            t->loc = (adf_lball_struct *) flint_malloc((m > 0 ? (size_t) m : 1) * sizeof(adf_lball_struct));
            for (k = 0; k < m; k++)
            {
                adf_lball_init(&t->loc[k]);
                adf_lball_set(&t->loc[k], &loc[k]);
            }
            st = ADF_OK;
        }
        if (st == ADF_OK)
        {
            slong k;

            /* release what x holds (what adf_sball_clear releases) and take t */
            for (k = 0; k < x->len; k++)
                adf_lball_clear(&x->loc[k]);
            flint_free(x->loc);
            x->arch = t->arch;
            acb_swap(x->inf, t->inf);
            x->len = t->len;
            x->loc = t->loc;
            t->arch = ADF_ARCH_NONE;
            acb_zero(t->inf);
            t->len = 0;
            t->loc = NULL;
            /* conventions 4.4: every output written satisfies the predicate of conventions 5.9 */
#ifdef ADF_CHECK_INVARIANTS
            if (!adf_sball_is_canonical(x))
                flint_abort();
#endif
        }
        adf_sball_clear(t);
    }
    acb_clear(z);
done:
    if (loc != NULL)
    {
        slong k;

        for (k = 0; k < m; k++)
            adf_lball_clear(&loc[k]);
        flint_free(loc);
    }
    flint_free(keys);
    flint_free(entries);
    tl_list_clear(&list);
    return st;
}

/* One entry of the printed text (conventions 9.4 line 1207): "inf: r(x)" for the real tag, "inf: z(x)" for
   the complex tag, "p=P: L" at a prime. Returns 0 when a local centre does not fit (tl_put_lcoord), and the
   caller then refuses the text as adf_lball_get_str does. */
static int
tl_put_entry(tx_buf * b, int arch, const acb_t inf, const adf_lball_t l, slong digits)
{
    if (arch == ADF_ARCH_REAL || arch == ADF_ARCH_COMPLEX)
    {
        tx_puts(b, "inf: ");
        if (arch == ADF_ARCH_COMPLEX)
            tx_puts(b, "(");
        tx_put_real(b, acb_realref(inf), digits);
        if (arch == ADF_ARCH_COMPLEX)
        {
            tx_puts(b, ") + (");
            tx_put_real(b, acb_imagref(inf), digits);
            tx_puts(b, ")*i");
        }
    }
    else
    {
        fmpz_t pz;

        tx_puts(b, "p=");
        fmpz_init_set_ui(pz, l->p);
        tx_put_fmpz(b, pz);
        fmpz_clear(pz);
        tx_puts(b, ": ");
        if (!tl_put_lcoord(b, l))
            return 0;
    }
    return 1;
}

char *
adf_sball_get_str(size_t * len, const adf_sball_t x, slong digits)
{
    tx_buf b;
    slong i;

#ifdef ADF_CHECK_INVARIANTS
    if (!adf_sball_is_canonical(x))
    {
        fprintf(stderr, "adelefeld: ADF_CHECK_INVARIANTS: %s: argument %s is not a canonical adf_sball\n",
                __func__, "x");
        fflush(stderr);
        flint_abort();
    }
#endif
    /* decision M1-D6: a real or complex part beyond the bound of the printer is refused before any
       conversion (include/adelefeld/text.h, the rules of every printer) */
    if (x->arch != ADF_ARCH_NONE
        && (!tx_arb_printable(acb_realref(x->inf)) || !tx_arb_printable(acb_imagref(x->inf))))
    {
        *len = 0;
        return NULL;
    }
    tx_buf_init(&b);
    tx_put(&b, "{", 1);
    if (x->arch != ADF_ARCH_NONE)
        (void) tl_put_entry(&b, x->arch, x->inf, NULL, digits);
    for (i = 0; i < x->len; i++)
    {
        if (i > 0 || x->arch != ADF_ARCH_NONE)
            tx_puts(&b, "; ");
        if (!tl_put_entry(&b, ADF_ARCH_NONE, NULL, &x->loc[i], digits))
        {
            /* a local centre that does not fit: no text, as for M1-D6 */
            flint_free(b.p);
            *len = 0;
            return NULL;
        }
    }
    tx_put(&b, "}", 1);
    return tx_finish(&b, len);
}

/* Slice a: char value text (api-3c 2; conventions 8.5,9.4,9.5,11.3).
   Appended here to share full lexical stages and the decimal enclosure kernel.
   proto/text_grammar.py:786-809,888-900 supplies the reference lowering/text. */
#include "adelefeld/char.h"
#include <flint/ulong_extras.h>

int adf_char_set_str(adf_char_t x, const char *s, size_t len, slong prec,
                     const adf_text_limits_t *lim)
{
    adf_text_limits_t store; tx_cur c; tx_num qlit, nlit; tx_real re, im;
    fmpz_t integer; ulong q, n; acb_t ball; int st;
    /* Numerical preflight precedes even reading s or lim. */
    if (prec > ADF_REAL_PREC_MAX) return ADF_LIMIT;
    lim = tx_limits(lim, &store); st = tx_prep(s, len, lim); if (st != ADF_OK) return st;
    c.s = s; c.len = len; c.i = 0;
    if (!(TX_KW(&c, "char") && tx_expect(&c, '(') && TX_KW(&c, "q") && tx_expect(&c, '=') &&
          tx_scan_rat(&c, 0, 0, &qlit) && tx_expect(&c, ',') && TX_KW(&c, "n") && tx_expect(&c, '=') &&
          tx_scan_rat(&c, 0, 0, &nlit) && tx_expect(&c, ',') && TX_KW(&c, "s") && tx_expect(&c, '=') &&
          tx_complex_syntax(&c, &re, &im) && tx_expect(&c, ')') && tx_at_end(&c))) return ADF_PARSE;
    if (tx_real_over(s, &re, lim) || tx_real_over(s, &im, lim)) return ADF_LIMIT;
    if (tl_prime_over_word(s, &qlit)) return ADF_UNSUPPORTED;
    fmpz_init(integer); tx_fmpz_digits(integer, s, qlit.ib, qlit.ie); q = fmpz_get_ui(integer);
    if (q == 0) { fmpz_clear(integer); return ADF_DOMAIN; }
    tx_fmpz_digits(integer, s, nlit.ib, nlit.ie); n = fmpz_fdiv_ui(integer, q); fmpz_clear(integer);
    /* Raw semantics precede the setup budget; no ball conversion or FLINT group before this. */
    if (n_gcd(n, q) != 1) return ADF_DOMAIN;
    if (q > ADF_CHAR_MOD_MAX) return ADF_LIMIT;
    acb_init(ball);
    tx_arb_from_real(acb_realref(ball), s, &re, FLINT_MAX(prec, 2));
    tx_arb_from_real(acb_imagref(ball), s, &im, FLINT_MAX(prec, 2));
    st = adf_char_set_conrey_acb(x, q, n, ball); acb_clear(ball); return st;
}

char *adf_char_get_str(size_t *len, const adf_char_t x, slong digits)
{
    tx_buf b; char integer[32];
#ifdef ADF_CHECK_INVARIANTS
    if (!adf_char_is_canonical(x)) adf_inv_fail(__func__, "x", "adf_char");
#endif
    if (!tx_arb_printable(acb_realref(x->s)) || !tx_arb_printable(acb_imagref(x->s))) {
        *len = 0; return NULL;
    }
    tx_buf_init(&b); tx_puts(&b, "char(q=");
    snprintf(integer, sizeof(integer), "%lu", x->q); tx_puts(&b, integer);
    tx_puts(&b, ", n="); snprintf(integer, sizeof(integer), "%lu", x->n); tx_puts(&b, integer);
    tx_puts(&b, ", s=("); tx_put_real(&b, acb_realref(x->s), digits);
    tx_puts(&b, ") + ("); tx_put_real(&b, acb_imagref(x->s), digits); tx_puts(&b, ")*i)");
    return tx_finish(&b, len);
}

/* ------------------------------------------------------------------------------------------------
   adf_rfun (slice 4d, lane f4-slice2): rfun_v = "rfun" "(" [rterm {"," rterm}] ")" (conventions 9.2,
   lines 1195-1197), template rfun(term(P=[z(c_0), ...], A=z(A), B=z(B), C=z(C)), ...) (9.4, the row of
   adf_rfun), predicate conventions 5.12 (no exact-zero last coefficient, Re(A) > 0 certified). Sources:
   docs/api-4.md section 2 (readers trim only exact trailing zeros, neither combine nor sort terms; NOT_DETERMINED
   only when rounding an exactly positive decimal Re(A) loses its sign); conventions 8.5 (stages), 9.3, 9.5
   (reading; constrained printing "positive" of Re(A)); proto/text_grammar.py Parser.rterm (398-418), the
   rfun branch of _syntax (515-525), _check_limits "items" (597-600), _trim_zero_coeffs (759-769),
   _check_real (772-775), the rfun branch of _build_and_print (878-887), _fmt_complex (747-748).
   Stage 7 uses tx_real_ball_signed with the condition "positive" (the reader of the idele class): the
   enclosure at prec first, then kernel B on the exact end points. */

#include "adelefeld/rfun.h"

#ifdef ADF_CHECK_INVARIANTS
#define ADF_INV_RFUN_TX(x) \
    do { if (!adf_rfun_is_canonical(x)) adf_inv_fail(__func__, #x, "adf_rfun"); } while (0)
#else
#define ADF_INV_RFUN_TX(x) ((void) 0)
#endif

/* The literals of an rfun text: for term k, plen[k] coefficients and then A, B, C, each a complex number
   (two tx_real) in the array z, in the order of the text. The arrays grow with the text. */
typedef struct
{
    tx_real * z;          /* 2 per complex number: real part, imaginary part */
    slong nz, capz;       /* complex numbers stored, allocated */
    slong * plen;
    slong nt, capt;       /* terms stored, allocated */
} tx_rf;

static void
tx_rf_init(tx_rf * r)
{
    memset(r, 0, sizeof(*r));
}

static void
tx_rf_clear(tx_rf * r)
{
    flint_free(r->z);
    flint_free(r->plen);
}

static int
tx_rf_complex(tx_cur * c, tx_rf * r)
{
    if (r->nz == r->capz)
    {
        r->capz = r->capz ? 2 * r->capz : 16;
        r->z = (tx_real *) flint_realloc(r->z, (size_t) r->capz * 2 * sizeof(tx_real));
    }
    if (!tx_complex_syntax(c, r->z + 2 * r->nz, r->z + 2 * r->nz + 1))
        return 0;
    r->nz++;
    return 1;
}

/* rterm (proto Parser.rterm, 398-418), recording the literals. */
static int
tx_rf_term(tx_cur * c, tx_rf * r)
{
    slong n = 0;

    if (r->nt == r->capt)
    {
        r->capt = r->capt ? 2 * r->capt : 4;
        r->plen = (slong *) flint_realloc(r->plen, (size_t) r->capt * sizeof(slong));
    }
    if (!TX_KW(c, "term") || !tx_expect(c, '(') || !TX_KW(c, "P") || !tx_expect(c, '=') || !tx_expect(c, '['))
        return 0;
    if (!tx_peek(c, ']'))
    {
        if (!tx_rf_complex(c, r))
            return 0;
        n++;
        while (tx_expect(c, ','))
        {
            if (!tx_rf_complex(c, r))
                return 0;
            n++;
        }
    }
    if (!(tx_expect(c, ']') && tx_expect(c, ',') && TX_KW(c, "A") && tx_expect(c, '=') && tx_rf_complex(c, r)
          && tx_expect(c, ',') && TX_KW(c, "B") && tx_expect(c, '=') && tx_rf_complex(c, r)
          && tx_expect(c, ',') && TX_KW(c, "C") && tx_expect(c, '=') && tx_rf_complex(c, r)
          && tx_expect(c, ')')))
        return 0;
    r->plen[r->nt++] = n;
    return 1;
}

static int
tx_rf_syntax(tx_cur * c, tx_rf * r)
{
    if (!TX_KW(c, "rfun") || !tx_expect(c, '('))
        return 0;
    if (!tx_peek(c, ')'))
    {
        if (!tx_rf_term(c, r))
            return 0;
        while (tx_expect(c, ','))
            if (!tx_rf_term(c, r))
                return 0;
    }
    return tx_expect(c, ')') && tx_at_end(c);
}

int
adf_rfun_set_str(adf_rfun_t x, const char * s, size_t len, slong prec, const adf_text_limits_t * lim)
{
    adf_text_limits_t store;
    tx_cur c;
    tx_rf r;
    adf_rterm_struct * t = NULL;
    slong k, j, i, base;
    fmpq_t lo, hi;
    int st;

    lim = tx_limits(lim, &store);
    st = tx_prep(s, len, lim);                              /* stages 1 and 2 */
    if (st != ADF_OK)
        return st;
    c.s = s;
    c.len = len;
    c.i = 0;
    tx_rf_init(&r);
    fmpq_init(lo);
    fmpq_init(hi);
    if (!tx_rf_syntax(&c, &r))                              /* stage 3 */
    {
        st = ADF_PARSE;
        goto done;
    }
    /* stage 4: the term list and every coefficient list against max_items (proto _check_limits, the
       "items" nodes), every decimal exponent against max_exp10 */
    if (r.nt > lim->max_items)
        st = ADF_LIMIT;
    for (k = 0; k < r.nt && st == ADF_OK; k++)
        if (r.plen[k] > lim->max_items)
            st = ADF_LIMIT;
    for (i = 0; i < 2 * r.nz && st == ADF_OK; i++)
        if (tx_real_over(s, r.z + i, lim))
            st = ADF_LIMIT;
    if (st != ADF_OK)
        goto done;
    /* stage 6: the exact interval of Re(A) lies in (0, infinity) (proto _check_real "positive") */
    for (k = 0, base = 0; k < r.nt; base += r.plen[k] + 3, k++)
    {
        tx_real_interval(lo, hi, s, r.z + 2 * (base + r.plen[k]));
        if (fmpq_sgn(lo) <= 0)
        {
            st = ADF_DOMAIN;
            goto done;
        }
    }
    /* build, with stage 7 at Re(A) */
    t = r.nt > 0 ? (adf_rterm_struct *) flint_malloc((size_t) r.nt * sizeof(adf_rterm_struct)) : NULL;
    for (k = 0; k < r.nt; k++)
    {
        acb_poly_init(t[k].P);
        acb_init(t[k].A);
        acb_init(t[k].B);
        acb_init(t[k].C);
    }
    for (k = 0, base = 0; k < r.nt && st == ADF_OK; base += r.plen[k] + 3, k++)
    {
        const tx_real * z = r.z + 2 * base;
        acb_ptr abc[3];

        abc[0] = t[k].A;
        abc[1] = t[k].B;
        abc[2] = t[k].C;
        acb_poly_fit_length(t[k].P, r.plen[k]);
        for (j = 0; j < r.plen[k]; j++)
        {
            tx_arb_from_real(acb_realref(t[k].P->coeffs + j), s, z + 2 * j, prec);
            tx_arb_from_real(acb_imagref(t[k].P->coeffs + j), s, z + 2 * j + 1, prec);
        }
        _acb_poly_set_length(t[k].P, r.plen[k]);
        _acb_poly_normalise(t[k].P);        /* exact trailing zeros only (acb_poly.rst:52-54; 5.12) */
        z += 2 * r.plen[k];
        tx_real_interval(lo, hi, s, z);
        st = tx_real_ball_signed(acb_realref(t[k].A), s, z, lo, hi, 1, prec);
        tx_arb_from_real(acb_imagref(t[k].A), s, z + 1, prec);
        for (j = 1; j < 3; j++)
        {
            tx_arb_from_real(acb_realref(abc[j]), s, z + 2 * j, prec);
            tx_arb_from_real(acb_imagref(abc[j]), s, z + 2 * j + 1, prec);
        }
    }
    if (st == ADF_OK)
    {
        adf_rfun_clear(x);
        x->term = t;
        x->len = r.nt;
        t = NULL;
    }
done:
    if (t != NULL)
    {
        for (k = 0; k < r.nt; k++)
        {
            acb_poly_clear(t[k].P);
            acb_clear(t[k].A);
            acb_clear(t[k].B);
            acb_clear(t[k].C);
        }
        flint_free(t);
    }
    tx_rf_clear(&r);
    fmpq_clear(lo);
    fmpq_clear(hi);
    return st;
}

/* z(x) of conventions 9.4: (r(re)) + (r(im))*i; with positive set, re is printed constrained (9.5).
   Returns 1 when the constrained printer refuses (N-D11). */
static int
tx_put_rf_complex(tx_buf * b, const acb_t x, slong digits, int positive)
{
    tx_puts(b, "(");
    if (positive)
    {
        if (tx_put_real_cond(b, acb_realref(x), digits, 1))
            return 1;
    }
    else
        tx_put_real(b, acb_realref(x), digits);
    tx_puts(b, ") + (");
    tx_put_real(b, acb_imagref(x), digits);
    tx_puts(b, ")*i");
    return 0;
}

static int
tx_acb_printable(const acb_t x)
{
    return tx_arb_printable(acb_realref(x)) && tx_arb_printable(acb_imagref(x));
}

char *
adf_rfun_get_str(size_t * len, const adf_rfun_t x, slong digits)
{
    tx_buf b;
    slong k, j;

    ADF_INV_RFUN_TX(x);
    for (k = 0; k < x->len; k++)
    {
        const adf_rterm_struct * t = x->term + k;
        int ok = tx_acb_printable(t->A) && tx_acb_printable(t->B) && tx_acb_printable(t->C);
        for (j = 0; j < acb_poly_length(t->P) && ok; j++)
            ok = tx_acb_printable(t->P->coeffs + j);
        if (!ok)
        {
            *len = 0;
            return NULL;
        }
    }
    tx_buf_init(&b);
    tx_puts(&b, "rfun(");
    for (k = 0; k < x->len; k++)
    {
        const adf_rterm_struct * t = x->term + k;

        if (k > 0)
            tx_puts(&b, ", ");
        tx_puts(&b, "term(P=[");
        for (j = 0; j < acb_poly_length(t->P); j++)
        {
            if (j > 0)
                tx_puts(&b, ", ");
            (void) tx_put_rf_complex(&b, t->P->coeffs + j, digits, 0);
        }
        tx_puts(&b, "], A=");
        if (tx_put_rf_complex(&b, t->A, digits, 1))
        {
            flint_free(b.p);
            *len = 0;
            return NULL;
        }
        tx_puts(&b, ", B=");
        (void) tx_put_rf_complex(&b, t->B, digits, 0);
        tx_puts(&b, ", C=");
        (void) tx_put_rf_complex(&b, t->C, digits, 0);
        tx_puts(&b, ")");
    }
    tx_puts(&b, ")");
    return tx_finish(&b, len);
}
