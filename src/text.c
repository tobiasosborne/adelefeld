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
