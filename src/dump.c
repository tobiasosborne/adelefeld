/* src/dump.c: the dump form, version 1, field Q (docs/conventions.md 10): loaders with context
   bindings, dumpers and inspectors of adf_rat, adf_fball, adf_scaled, adf_adele, adf_cadele
   (include/adelefeld/dump.h), adf_scaled_get_str, and the validation of every dump body that
   adf_modctx_new_from_dump (src/modctx.c) uses. Work package 1.4, lane m1-dump.

   Sources, read before the code was written, and cited at each function:
   - docs/conventions.md 10.1 (grammar, lines 1322-1364), 10.2 (rules, lines 1366-1431: strict
     loader CV-38, real balls as arb_dump_str writes them, validation before any FLINT load
     function CV-52, context bindings G3, descriptors and inspection C2), 8.1 (interface,
     1021-1039), 8.2 (alphabet, 1041-1049), 8.4 (limits, 1059-1073), 8.5 (order of checks,
     1075-1090), 4.3 (outputs after a status, 256-279), 5.1 to 5.5 and 5.14 (the predicates of
     the stored types, 374-544 and 772-790), 9.4 (row "scaled value", 1190).
   - The reference proto/text_grammar.py, dump part (lines 937-1521): _dump_syntax (1014-1096),
     _v_arb (1107-1118), _v_ctx (1153-1169), _v_G (1181-1188), _v_fb (1191-1209), _v_fmpq
     (1212-1214), _ctx_occurrences (1254-1266), _dump_validate (1274-1394), _dump_print
     (1425-1469), dump_contexts (1482-1490), dump_load_check (1493-1507),
     modctx_new_from_dump (1510-1521).
   - FLINT 3.0.1: arf_set_fmpz_2exp and arf_get_fmpz_2exp (refs/src/flint-3.0.1/arf.rst:227-242),
     arf_set_mag (arf.rst:411-413), ARF_PREC_EXACT (arf.rst:91-107), the mag representation
     (/usr/include/flint/mag.h:113-119 MAG_EXPREF, MAG_MAN, MAG_BITS; mag.h:217-221 mag_one, so
     that a mag is MAG_MAN * 2^(MAG_EXP - MAG_BITS) with a mantissa of exactly MAG_BITS bits,
     mag.h:135-140), fmpz_get_str and fmpz_set_str (fmpz.rst:337-344, 427-431), fmpz_sizeinbase
     (fmpz.rst:600), fmpz_CRT_ui (fmpz.rst:1279-1290), fmpz_fdiv_ui (fmpz.rst:845-850).

   This file reads untrusted input. The rules of the lane (lanes/m1-dump/brief.md):
   - One validating pass over (s, len) first, by hand; no NUL terminator is needed, and no
     strlen, strtol, sscanf or atoi touches the input. The stages of 8.5 run in order over the
     whole text; nothing is built before the whole text has passed stage 6.
   - No FLINT load or string function sees raw input (CV-52, M0-D9): a number is copied into a
     buffer of its own after validation before fmpz_set_str reads it; arb_load_str and
     arb_set_str are never called. A real ball is rebuilt exactly from its validated fields.
   - The loader is strict: only canonical text; it never canonicalises.
   - The output is built in a temporary and swapped in on ADF_OK only (4.3).
   - The grammar is not recursive and neither is this reader; every allocation is bounded by the
     length of the input.

   Assumptions about the types of other lanes (listed in lanes/m1-dump/report.md): the fields of
   adf_scaled_struct and of a local adf_fball_struct are read and written directly as
   include/adelefeld/scaled.h and fball.h lay them out; a local value owns res, allocated with
   flint_malloc and freed by adf_fball_clear with flint_free (src/fball.c adf_fball_clear). */

#include <string.h>

#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/fmpq.h>
#include <flint/arf.h>
#include <flint/mag.h>
#include <flint/arb.h>
#include <flint/acb.h>
#include <flint/ulong_extras.h>

#include "adelefeld/dump.h"

#if defined(__GNUC__) || defined(__clang__)
#define DP_HIDDEN __attribute__((visibility("hidden")))
#else
#define DP_HIDDEN
#endif

/* Stage 6 of a body that has no context occurrence and whose predicates this file does not
   check (ucoset, idele, idclass, lball, sball, ffun, rfun, char). Only
   adf_dump_ctx_occurrence sees it, and there the answer is DOMAIN whatever the predicates say:
   occurrence index out of range, or a failed predicate (conventions 10.2). Never returned by a
   public function. */
#define DP_NOSEM (-1)

/* ADF_DUMP_QCLASS_EXP_MAX (adelefeld/dump.h; docs/SPEC.md 15 row M1-D9; conventions 8.4): the bound
   on the binary exponents of the real ball of a piece of a qclass, applied in dp_w_arb. */

/* ================================================================================================
   Tokens (conventions 10.1: exactly one space between tokens; `h` lower-case hexadecimal). */

typedef struct
{
    const char * p;
    size_t n;                          /* >= 1 for every token of a text that passed stage 3 */
} dp_span;

static int
dp_hexdig(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
}

static int
dp_hexnz(char c)
{
    return (c >= '1' && c <= '9') || (c >= 'a' && c <= 'f');
}

static unsigned
dp_hexval(char c)
{
    return (c >= '0' && c <= '9') ? (unsigned) (c - '0') : (unsigned) (c - 'a' + 10);
}

/* h = "0" | ["-"], hnz, {hdig} (conventions 10.1, line 1328; proto RE_HEX, line 940). */
static int
dp_is_h(dp_span t)
{
    size_t j = 0;

    if (t.n == 1 && t.p[0] == '0')
        return 1;
    if (t.p[0] == '-')
        j = 1;
    if (j >= t.n || !dp_hexnz(t.p[j]))
        return 0;
    for (j++; j < t.n; j++)
        if (!dp_hexdig(t.p[j]))
            return 0;
    return 1;
}

static int
dp_neg(dp_span t)
{
    return t.p[0] == '-';
}

static int
dp_zero(dp_span t)
{
    return t.n == 1 && t.p[0] == '0';
}

/* A validated h token as a word: 1 and *v if 0 <= value < 2^64, else 0. */
static int
dp_word(dp_span t, ulong * v)
{
    size_t j;
    ulong r = 0;

    if (dp_neg(t) || t.n > 16)
        return 0;
    for (j = 0; j < t.n; j++)
        r = (r << 4) | dp_hexval(t.p[j]);
    *v = r;
    return 1;
}

/* A validated h token as an fmpz. Up to 15 digits are read here; a longer token is copied into
   a buffer of its own and only then given to fmpz_set_str (fmpz.rst:427-431), so that no raw
   input reaches FLINT (conventions 10.2, CV-52). */
static void
dp_fmpz(fmpz_t z, dp_span t)
{
    size_t j = dp_neg(t) ? 1 : 0;

    if (t.n - j <= 15)
    {
        slong r = 0;
        for (; j < t.n; j++)
            r = r * 16 + (slong) dp_hexval(t.p[j]);
        fmpz_set_si(z, dp_neg(t) ? -r : r);
    }
    else
    {
        char * buf = (char *) flint_malloc(t.n + 1);
        memcpy(buf, t.p, t.n);
        buf[t.n] = '\0';
        if (fmpz_set_str(z, buf, 16) != 0)
            flint_abort();             /* cannot happen: the token passed dp_is_h */
        flint_free(buf);
    }
}

/* |value| > bound, for a validated token and a bound of either sign (a negative bound is
   exceeded by every value, as in the reference, where abs(v) > max_prec). */
static int
dp_abs_over(dp_span t, slong bound)
{
    dp_span u = t;
    ulong v;

    if (bound < 0)
        return 1;
    if (dp_neg(u))
    {
        u.p++;
        u.n--;
    }
    if (!dp_word(u, &v))
        return 1;                      /* >= 2^64 > any slong */
    return v > (ulong) bound;
}

/* value >= 2^64 for a validated token (the word restriction of stage 5). */
static int
dp_over_word(dp_span t)
{
    return !dp_neg(t) && t.n > 16;
}

/* n > max_items; a negative limit is exceeded by every count (proto _dump_validate,
   `len(...) > limits.max_items`). */
static int
dp_over_items(size_t n, const adf_text_limits_t * lim)
{
    return lim->max_items < 0 || n > (size_t) lim->max_items;
}

/* The cursor over the body: the tokens left and the offset of the next one. */
typedef struct
{
    const char * s;
    size_t len;
    size_t pos;
    size_t left;
} dp_cur;

/* The next token. Stage 3 has checked that tokens are separated by exactly one space, that the
   body neither starts nor ends with a space, and has counted the tokens, so a token is never
   empty and the scan never passes len. */
static int
dp_next(dp_cur * c, dp_span * t)
{
    size_t b = c->pos;

    if (c->left == 0)
        return 0;
    while (c->pos < c->len && c->s[c->pos] != ' ')
        c->pos++;
    t->p = c->s + b;
    t->n = c->pos - b;
    if (c->pos < c->len)
        c->pos++;
    c->left--;
    return 1;
}

static int
dp_h(dp_cur * c, dp_span * t)
{
    return dp_next(c, t) && dp_is_h(*t);
}

static int
dp_kw_is(dp_span t, const char * w)
{
    size_t n = strlen(w);              /* w is a keyword of this file, not input */
    return t.n == n && memcmp(t.p, w, n) == 0;
}

/* A count (proto Tokens.count, lines 966-970): an h token n with n >= 0 and n * per <= the
   tokens left; otherwise a grammar failure. A count beyond a size_t exceeds the tokens left. */
static int
dp_count(dp_cur * c, size_t per, size_t * out)
{
    dp_span t;
    size_t j, v = 0;

    if (!dp_h(c, &t) || dp_neg(t))
        return 0;
    for (j = 0; j < t.n; j++)
    {
        unsigned d = dp_hexval(t.p[j]);
        if (v > (((size_t) -1) - d) / 16)
            return 0;
        v = v * 16 + d;
    }
    if (v > c->left / per)
        return 0;
    *out = v;
    return 1;
}

/* The token at byte offset *pos of a text that passed stage 3; advances *pos past it and its
   space. Used to walk the blocks and residues again, which stage 3 has read. */
static dp_span
dp_tok_at(const char * s, size_t len, size_t * pos)
{
    dp_span t;
    size_t b = *pos;

    while (*pos < len && s[*pos] != ' ')
        (*pos)++;
    t.p = s + b;
    t.n = *pos - b;
    if (*pos < len)
        (*pos)++;
    return t;
}

/* ================================================================================================
   The parsed pieces (spans into the text; nothing is converted before stage 6). */

typedef struct
{
    dp_span m, e, rm, re;              /* mid mantissa, mid exponent, rad mantissa, rad exponent */
} dp_arb;

typedef struct
{
    dp_span K;
    size_t k;
    size_t q0;                         /* offset of the first block token (k >= 1) */
} dp_ctx;

typedef struct
{
    int local;
    dp_span A, H, d;                   /* g: A H d; l: d only */
    dp_ctx ctx;                        /* l */
    size_t r0;                         /* l: offset of the first residue token (k >= 1) */
} dp_fb;

enum
{
    DP_RAT = 0, DP_FBALL, DP_SCALED, DP_ADELE, DP_CADELE, DP_UCOSET, DP_IDELE, DP_IDCLASS,
    DP_LBALL, DP_SBALL, DP_QCLASS, DP_FFUN, DP_RFUN, DP_CHAR, DP_MODCTX, DP_NKINDS
};

/* The body keywords of conventions 10.1 (proto _dump_syntax, lines 1041-1042). */
static const char * const dp_kind_name[DP_NKINDS] = {
    "rat", "fball", "scaled", "adele", "cadele", "ucoset", "idele", "idclass", "lball", "sball",
    "qclass", "ffun", "rfun", "char", "modctx"};

typedef struct
{
    int kind;
    int form;                          /* scaled: 'x' or 's' */
    dp_span a, b, c;                   /* rat: num den; scaled: num den [u] */
    size_t narch;                      /* adele, cadele: the archimedean count */
    dp_arb arb[2];                     /* adele: arb[0]; cadele: arb[0] real, arb[1] imaginary */
    dp_fb fb;                          /* fball, adele, cadele */
    dp_ctx ctx;                        /* scaled, modctx */
} dp_node;

/* The walk of a body runs once per stage of 8.5 (conventions 8.5, CV-28), so that the first
   failing stage decides whatever the position of the faults. */
enum { DP_SYNTAX = 0, DP_LIMITS, DP_WORDS, DP_SEM, DP_OCC };

typedef struct
{
    int mode;
    const adf_text_limits_t * lim;
    int st;                            /* ADF_OK, or the status of the first failure */
    int arb_exp_limit;                 /* DP_LIMITS: bound the arb exponents (M1-D9) */
    dp_node * node;                    /* filled in every mode */
    size_t nocc;                       /* DP_OCC: the context occurrences met so far */
    adf_ctx_desc_t * descs;            /* DP_OCC: occurrence first + i is copied into descs[i] */
    size_t first, ndescs;
} dp_state;

static int
dp_fail(dp_state * st, int status)
{
    if (st->st == ADF_OK)
        st->st = status;
    return 0;
}

static int
dp_parse_fail(dp_state * st)
{
    return dp_fail(st, ADF_PARSE);
}

/* ================================================================================================
   Stage 6 predicates (conventions 5 and 10.2; proto _v_*). Each returns 1 if the predicate
   holds. */

/* conventions 10.2 (line 1381): an arb field is accepted only if both mantissas are odd or the
   pair is "0 0", and the radius mantissa is positive and below 2^30 (MAG_BITS, mag.h:117)
   (proto _v_arb, lines 1107-1118). Decided on the tokens: the parity of a hexadecimal number is
   that of its last digit. */
static int
dp_v_arb(const dp_arb * a)
{
    ulong r;

    if (dp_zero(a->m))
    {
        if (!dp_zero(a->e))
            return 0;                  /* a special value of arb_dump_str, or an unknown one */
    }
    else if ((dp_hexval(a->m.p[a->m.n - 1]) & 1) == 0)
        return 0;
    if (dp_zero(a->rm))
        return dp_zero(a->re);
    if (!dp_word(a->rm, &r))
        return 0;                      /* negative, or far above 2^30 */
    return (r & 1) == 1 && r < (UWORD(1) << MAG_BITS);
}

/* conventions 5.14 (line 774) and proto _v_ctx (lines 1153-1169): K >= 1; if need_blocks,
   k >= 1; each block a word >= 2, pairwise coprime, product K. Pairwise coprimality is
   checked against the product P of the earlier blocks: gcd(q_i, P) = gcd(q_i, P mod q_i)
   (fmpz_fdiv_ui, fmpz.rst:845), which is 1 exactly when q_i is coprime to each earlier block. */
static int
dp_v_ctx(const dp_cur * c, const dp_ctx * x, int need_blocks)
{
    fmpz_t K, P;
    size_t i, pos = x->q0;
    int ok = 1;

    fmpz_init(K);
    fmpz_init_set_ui(P, 1);
    dp_fmpz(K, x->K);
    if (fmpz_sgn(K) < 1)
        ok = 0;
    if (ok && need_blocks && x->k == 0)
        ok = 0;
    for (i = 0; ok && i < x->k; i++)
    {
        ulong q;
        dp_span t = dp_tok_at(c->s, c->len, &pos);
        if (!dp_word(t, &q) || q < 2 || n_gcd(q, fmpz_fdiv_ui(P, q)) != 1)
            ok = 0;
        else
            fmpz_mul_ui(P, P, q);
    }
    if (ok && x->k > 0 && !fmpz_equal(P, K))
        ok = 0;
    fmpz_clear(K);
    fmpz_clear(P);
    return ok;
}

/* conventions 5.1: fmpq_is_canonical, denominator > 0 and gcd = 1 (proto _v_fmpq). */
static int
dp_v_fmpq(dp_span num, dp_span den)
{
    fmpz_t a, b, g;
    int ok;

    if (dp_neg(den) || dp_zero(den))
        return 0;
    fmpz_init(a);
    fmpz_init(b);
    fmpz_init(g);
    dp_fmpz(a, num);
    dp_fmpz(b, den);
    fmpz_gcd(g, a, b);
    ok = fmpz_is_one(g);
    fmpz_clear(a);
    fmpz_clear(b);
    fmpz_clear(g);
    return ok;
}

/* The fields of a finite ball as integers, and (for qclass) its canonical triple. Predicate G
   of conventions 5.2 (proto _v_G, lines 1181-1188) for "g"; predicate L of 5.3 (raw data, no
   gcd condition; proto _v_fb, lines 1191-1209) for "l". If A, H, d are not NULL they receive
   the canonical global triple of the set (conventions 5.3: (L/g, K/g, d/g), L the CRT lift
   in [0, K), g = gcd(L, K, d)). */
static int
dp_v_fb(const dp_cur * c, const dp_fb * f, fmpz_t A, fmpz_t H, fmpz_t d)
{
    fmpz_t a, h, e, g;
    int ok = 1;

    fmpz_init(a);
    fmpz_init(h);
    fmpz_init(e);
    fmpz_init(g);
    if (!f->local)
    {
        dp_fmpz(a, f->A);
        dp_fmpz(h, f->H);
        dp_fmpz(e, f->d);
        if (fmpz_sgn(e) <= 0 || fmpz_sgn(h) < 0)
            ok = 0;
        else if (fmpz_sgn(h) > 0)
        {
            if (fmpz_sgn(a) < 0 || fmpz_cmp(a, h) >= 0)
                ok = 0;
            else
            {
                fmpz_gcd3(g, a, h, e);
                ok = fmpz_is_one(g);
            }
        }
        else
        {
            fmpz_gcd(g, a, e);
            ok = fmpz_is_one(g);
        }
    }
    else
    {
        size_t i, qpos = f->ctx.q0, rpos = f->r0;

        ok = dp_v_ctx(c, &f->ctx, 1);
        if (ok)
        {
            dp_fmpz(e, f->d);
            if (fmpz_sgn(e) < 1)
                ok = 0;
        }
        for (i = 0; ok && i < f->ctx.k; i++)
        {
            ulong q = 0, r = 0;
            dp_span tq = dp_tok_at(c->s, c->len, &qpos);
            dp_span tr = dp_tok_at(c->s, c->len, &rpos);
            (void) dp_word(tq, &q);    /* a word: dp_v_ctx has accepted it */
            if (!dp_word(tr, &r) || r >= q)
                ok = 0;
            else if (A != NULL)
            {
                /* the CRT lift, one block at a time (fmpz_CRT_ui, fmpz.rst:1279-1290: moduli
                   > 1 and coprime, residues reduced) */
                if (i == 0)
                {
                    fmpz_set_ui(a, r);
                    fmpz_set_ui(h, q);
                }
                else
                {
                    fmpz_CRT_ui(a, a, h, r, q, 0);
                    fmpz_mul_ui(h, h, q);
                }
            }
        }
        if (ok && A != NULL)
            fmpz_gcd3(g, a, h, e);
        else
            fmpz_one(g);
    }
    if (ok && A != NULL)
    {
        if (f->local)
        {
            fmpz_divexact(A, a, g);
            fmpz_divexact(H, h, g);
            fmpz_divexact(d, e, g);
        }
        else
        {
            fmpz_set(A, a);
            fmpz_set(H, h);
            fmpz_set(d, e);
        }
    }
    fmpz_clear(a);
    fmpz_clear(h);
    fmpz_clear(e);
    fmpz_clear(g);
    return ok;
}

/* ================================================================================================
   The grammar of conventions 10.1, one function per nonterminal, each run in the mode of the
   state. In DP_SYNTAX a failure is ADF_PARSE; in the later modes the grammar is known to hold. */

static int
dp_w_arb(dp_cur * c, dp_state * st, dp_arb * a)
{
    if (!dp_h(c, &a->m) || !dp_h(c, &a->e) || !dp_h(c, &a->rm) || !dp_h(c, &a->re))
        return dp_parse_fail(st);
    /* The bound of M1-D9 on the exponents of a qclass piece is a limit of stage 4
       (conventions 8.5 item 4), so it is decided here, on the digit strings, before any
       semantic check: dp_abs_over reads a token of any length and calls a value of more than 16
       hexadecimal digits, hence of more than 2^64, over the bound (M1-D7 is the pattern: no
       hidden bound of a machine word).  The other bodies have no such bound. */
    if (st->mode == DP_LIMITS && st->arb_exp_limit
        && (dp_abs_over(a->e, ADF_DUMP_QCLASS_EXP_MAX) || dp_abs_over(a->re, ADF_DUMP_QCLASS_EXP_MAX)))
        return dp_fail(st, ADF_LIMIT);
    return 1;
}

/* Copy a context occurrence into a descriptor, releasing its old block array (closure C2). The
   blocks are words: stage 6 has accepted the occurrence. */
static void
dp_desc_set(adf_ctx_desc_t * d, const dp_cur * c, const dp_ctx * x)
{
    size_t i, pos = x->q0;

    dp_fmpz(d->K, x->K);
    flint_free(d->q);
    d->q = NULL;
    if (x->k > 0)
    {
        d->q = (ulong *) flint_malloc(x->k * sizeof(ulong));
        for (i = 0; i < x->k; i++)
        {
            dp_span t = dp_tok_at(c->s, c->len, &pos);
            ulong v = 0;
            (void) dp_word(t, &v);
            d->q[i] = v;
        }
    }
    d->k = (slong) x->k;
}

/* ctx = h K, h k, then k blocks (conventions 10.1, line 1334). Stage 4: the block count of
   every occurrence (8.5 item 4, gate finding G8). Stage 5: at most ADF_MODCTX_MAX_BLOCKS blocks
   (decision M1-D5), decided from the count of the text alone, before the blocks are read into
   integers, before the predicate of conventions 5.14 and before any allocation in proportion
   to the count.  DP_OCC: the occurrence is counted and, if asked for, copied. Stage 6 is the
   caller's (need_blocks differs). */
static int
dp_w_ctx(dp_cur * c, dp_state * st, dp_ctx * x)
{
    size_t i;
    dp_span t;

    if (!dp_h(c, &x->K) || !dp_count(c, 1, &x->k))
        return dp_parse_fail(st);
    x->q0 = c->pos;
    for (i = 0; i < x->k; i++)
        if (!dp_h(c, &t))
            return dp_parse_fail(st);
    if (st->mode == DP_LIMITS && dp_over_items(x->k, st->lim))
        return dp_fail(st, ADF_LIMIT);
    if (st->mode == DP_WORDS && x->k > (size_t) ADF_MODCTX_MAX_BLOCKS)
        return dp_fail(st, ADF_UNSUPPORTED);
    if (st->mode == DP_OCC)
    {
        size_t o = st->nocc++;
        if (st->descs != NULL && o >= st->first && o - st->first < st->ndescs)
            dp_desc_set(&st->descs[o - st->first], c, x);
    }
    return 1;
}

/* fb = "g" A H d | "l" d ctx res_1 .. res_k (conventions 10.1, lines 1335-1336; proto _d_fb). */
static int
dp_w_fb(dp_cur * c, dp_state * st, dp_fb * f)
{
    dp_span t;
    size_t i;

    if (!dp_next(c, &t))
        return dp_parse_fail(st);
    if (dp_kw_is(t, "g"))
    {
        f->local = 0;
        if (!dp_h(c, &f->A) || !dp_h(c, &f->H) || !dp_h(c, &f->d))
            return dp_parse_fail(st);
        return 1;
    }
    if (!dp_kw_is(t, "l"))
        return dp_parse_fail(st);
    f->local = 1;
    if (!dp_h(c, &f->d))
        return dp_parse_fail(st);
    if (!dp_w_ctx(c, st, &f->ctx))
        return 0;
    if (c->left < f->ctx.k)
        return dp_parse_fail(st);
    f->r0 = c->pos;
    for (i = 0; i < f->ctx.k; i++)
        if (!dp_h(c, &t))
            return dp_parse_fail(st);
    return 1;
}

/* arch = count, then that many real balls (per = 4) or complex balls (per = 8); the first is
   kept (conventions 10.1, lines 1332-1333; proto _d_arch). */
static int
dp_w_arch(dp_cur * c, dp_state * st, size_t per, size_t * n, dp_arb * first)
{
    size_t i;
    dp_arb tmp;

    if (!dp_count(c, per, n))
        return dp_parse_fail(st);
    for (i = 0; i < *n; i++)
    {
        if (!dp_w_arb(c, st, i == 0 ? &first[0] : &tmp))
            return 0;
        if (per == 8 && !dp_w_arb(c, st, i == 0 ? &first[1] : &tmp))
            return 0;
    }
    return 1;
}

/* lb = p ("x" num den v | "b" u v N) (conventions 10.1, lines 1337-1338). Stage 4: |v|, |N| at
   most max_prec (8.4); stage 5: p below 2^64 (8.5 item 5; proto _dump_validate, 1291-1298). */
static int
dp_w_lb(dp_cur * c, dp_state * st)
{
    dp_span p, f, a, b, d;

    if (!dp_h(c, &p) || !dp_next(c, &f) || !(dp_kw_is(f, "x") || dp_kw_is(f, "b"))
        || !dp_h(c, &a) || !dp_h(c, &b) || !dp_h(c, &d))
        return dp_parse_fail(st);
    if (st->mode == DP_LIMITS)
    {
        if (dp_abs_over(d, st->lim->max_prec) || (dp_kw_is(f, "b") && dp_abs_over(b, st->lim->max_prec)))
            return dp_fail(st, ADF_LIMIT);
    }
    if (st->mode == DP_WORDS && dp_over_word(p))
        return dp_fail(st, ADF_UNSUPPORTED);
    return 1;
}

/* Stage 6 of qclass (proto _dump_validate, lines 1350-1368): a lift is one adele; pieces are
   adeles with the midpoint of the real ball in [0, 1] and a finite part whose canonical triple
   has d = 1, in strictly increasing order of (mid - rad, mid + rad, H, A) (conventions 5.10,
   CV-24, CV-45).  The bound of M1-D9 on the binary exponents of the real ball of a piece is a
   limit of stage 4 and was decided in dp_w_arb before this walk, so the exact sums below are
   formed with exponents of absolute value at most ADF_DUMP_QCLASS_EXP_MAX and fit in memory
   (ARF_PREC_EXACT, arf.rst:91-107). */
typedef struct
{
    arf_t lo, hi;
    fmpz_t H, A;
    int have;
    int unordered;
} dp_qkeys;

static int
dp_q_piece(const dp_cur * c, dp_state * st, size_t narch, const dp_arb * a, const dp_fb * f,
           dp_qkeys * keys)
{
    fmpz_t A, H, d, m, e;
    arf_t mid, rad, lo, hi;
    int ok = 1;

    if (narch != 1 || !dp_v_arb(a))
        return dp_fail(st, ADF_DOMAIN);
    fmpz_init(A);
    fmpz_init(H);
    fmpz_init(d);
    if (!dp_v_fb(c, f, A, H, d))
    {
        fmpz_clear(A);
        fmpz_clear(H);
        fmpz_clear(d);
        return dp_fail(st, ADF_DOMAIN);
    }
    fmpz_init(m);
    fmpz_init(e);
    arf_init(mid);
    arf_init(rad);
    arf_init(lo);
    arf_init(hi);
    dp_fmpz(m, a->m);
    dp_fmpz(e, a->e);
    arf_set_fmpz_2exp(mid, m, e);
    dp_fmpz(m, a->rm);
    dp_fmpz(e, a->re);
    arf_set_fmpz_2exp(rad, m, e);
    if (arf_sgn(mid) < 0 || arf_cmp_si(mid, 1) > 0 || !fmpz_is_one(d))
        ok = dp_fail(st, ADF_DOMAIN);
    if (ok)
    {
        int cmp;
        arf_sub(lo, mid, rad, ARF_PREC_EXACT, ARF_RND_DOWN);
        arf_add(hi, mid, rad, ARF_PREC_EXACT, ARF_RND_DOWN);
        if (keys->have)
        {
            cmp = arf_cmp(keys->lo, lo);
            if (cmp == 0)
                cmp = arf_cmp(keys->hi, hi);
            if (cmp == 0)
                cmp = fmpz_cmp(keys->H, H);
            if (cmp == 0)
                cmp = fmpz_cmp(keys->A, A);
            if (cmp >= 0)
                keys->unordered = 1;
        }
        arf_swap(keys->lo, lo);
        arf_swap(keys->hi, hi);
        fmpz_swap(keys->H, H);
        fmpz_swap(keys->A, A);
        keys->have = 1;
    }
    fmpz_clear(A);
    fmpz_clear(H);
    fmpz_clear(d);
    fmpz_clear(m);
    fmpz_clear(e);
    arf_clear(mid);
    arf_clear(rad);
    arf_clear(lo);
    arf_clear(hi);
    return ok;
}

static int
dp_w_qclass(dp_cur * c, dp_state * st)
{
    dp_span t;
    size_t n = 1, i;
    int pieces;
    dp_qkeys keys;
    int ok = 1;

    if (!dp_next(c, &t) || !(dp_kw_is(t, "lift") || dp_kw_is(t, "pieces")))
        return dp_parse_fail(st);
    pieces = dp_kw_is(t, "pieces");
    if (pieces && !dp_count(c, 8, &n))
        return dp_parse_fail(st);
    if (st->mode == DP_LIMITS && dp_over_items(n, st->lim))
        return dp_fail(st, ADF_LIMIT);
    /* The bound of M1-D9 applies to every piece of the form "pieces"; a "lift" is a single
       adele, the grammar of conventions 10.1 has no piece there, and the cost the row names
       (the range of a piece is formed, so that its end points are exact) is paid by the pieces
       only.  A lift is therefore read with its exponents as they are, as tests/ref/vectors/
       m1-dump/dump_ref.jsonl records; the reading is discussed in the lane report. */
    if (st->mode == DP_LIMITS && pieces)
        st->arb_exp_limit = 1;
    if (st->mode == DP_SEM && pieces && n == 0)
        return dp_fail(st, ADF_DOMAIN);
    arf_init(keys.lo);
    arf_init(keys.hi);
    fmpz_init(keys.H);
    fmpz_init(keys.A);
    keys.have = 0;
    keys.unordered = 0;
    for (i = 0; ok && i < n; i++)
    {
        size_t narch;
        dp_arb a[2];
        dp_fb f;
        if (!dp_w_arch(c, st, 4, &narch, a) || !dp_w_fb(c, st, &f))
            ok = 0;
        else if (st->mode == DP_SEM)
        {
            if (!pieces)
            {
                if (narch != 1 || !dp_v_arb(&a[0]) || !dp_v_fb(c, &f, NULL, NULL, NULL))
                    ok = dp_fail(st, ADF_DOMAIN);
            }
            else
                ok = dp_q_piece(c, st, narch, &a[0], &f, &keys);
        }
    }
    if (ok && st->mode == DP_SEM && keys.unordered)
        ok = dp_fail(st, ADF_DOMAIN);
    arf_clear(keys.lo);
    arf_clear(keys.hi);
    fmpz_clear(keys.H);
    fmpz_clear(keys.A);
    return ok;
}

/* The bodies without a context occurrence and without a typed loader in this file (conventions
   10.1): grammar, stage 4 and stage 5 as in proto _dump_syntax and _dump_validate; stage 6 is
   DP_NOSEM. */
static int
dp_w_other(dp_cur * c, dp_state * st, int kind)
{
    dp_span t, u;
    size_t n, i, j;
    dp_arb a[2], tmp;                  /* a: the first ball of an arch; tmp: parsed, not kept */

    switch (kind)
    {
        case DP_UCOSET:
            if (!dp_h(c, &t) || !dp_h(c, &u))
                return dp_parse_fail(st);
            break;
        case DP_IDELE:
            if (!dp_w_arch(c, st, 4, &n, a))
                return 0;
            for (i = 0; i < 4; i++)
                if (!dp_h(c, &t))
                    return dp_parse_fail(st);
            break;
        case DP_IDCLASS:
            if (!dp_w_arb(c, st, &tmp) || !dp_h(c, &t) || !dp_h(c, &u))
                return dp_parse_fail(st);
            break;
        case DP_LBALL:
            if (!dp_w_lb(c, st))
                return 0;
            break;
        case DP_SBALL:
            if (!dp_next(c, &t))
                return dp_parse_fail(st);
            if (dp_kw_is(t, "r"))
            {
                if (!dp_w_arb(c, st, &tmp))
                    return 0;
            }
            else if (dp_kw_is(t, "c"))
            {
                if (!dp_w_arb(c, st, &tmp) || !dp_w_arb(c, st, &tmp))
                    return 0;
            }
            else if (!dp_kw_is(t, "n"))
                return dp_parse_fail(st);
            if (!dp_count(c, 5, &n))
                return dp_parse_fail(st);
            if (st->mode == DP_LIMITS && dp_over_items(n, st->lim))
                return dp_fail(st, ADF_LIMIT);
            for (i = 0; i < n; i++)
                if (!dp_w_lb(c, st))
                    return 0;
            break;
        case DP_FFUN:
        {
            /* D M, then D M complex balls; D < 0, M < 0 or 8 D M above the tokens left is a
               grammar failure (proto _dump_syntax, lines 1077-1081) */
            fmpz_t D, M;
            size_t dm = 0;
            int bad;
            if (!dp_h(c, &t) || !dp_h(c, &u))
                return dp_parse_fail(st);
            fmpz_init(D);
            fmpz_init(M);
            dp_fmpz(D, t);
            dp_fmpz(M, u);
            bad = fmpz_sgn(D) < 0 || fmpz_sgn(M) < 0;
            if (!bad)
            {
                fmpz_mul(D, D, M);
                fmpz_mul_ui(M, D, 8);
                bad = fmpz_cmp_ui(M, (ulong) c->left) > 0;
                if (!bad)
                    dm = (size_t) fmpz_get_ui(D);
            }
            fmpz_clear(D);
            fmpz_clear(M);
            if (bad)
                return dp_parse_fail(st);
            if (st->mode == DP_LIMITS && dp_over_items(dm, st->lim))
                return dp_fail(st, ADF_LIMIT);
            for (i = 0; i < dm; i++)
                if (!dp_w_arb(c, st, &tmp) || !dp_w_arb(c, st, &tmp))
                    return 0;
            break;
        }
        case DP_RFUN:
            if (!dp_count(c, 25, &n))
                return dp_parse_fail(st);
            if (st->mode == DP_LIMITS && dp_over_items(n, st->lim))
                return dp_fail(st, ADF_LIMIT);
            for (i = 0; i < n; i++)
            {
                size_t L;
                if (!dp_count(c, 8, &L))
                    return dp_parse_fail(st);
                if (st->mode == DP_LIMITS && dp_over_items(L, st->lim))
                    return dp_fail(st, ADF_LIMIT);
                for (j = 0; j < L + 3; j++)
                    if (!dp_w_arb(c, st, &tmp) || !dp_w_arb(c, st, &tmp))
                        return 0;
            }
            break;
        default: /* DP_CHAR: q n s */
            if (!dp_h(c, &t) || !dp_h(c, &u) || !dp_w_arb(c, st, &tmp) || !dp_w_arb(c, st, &tmp))
                return dp_parse_fail(st);
            if (st->mode == DP_WORDS && dp_over_word(t))
                return dp_fail(st, ADF_UNSUPPORTED);
            break;
    }
    if (st->mode == DP_SEM)
        return dp_fail(st, DP_NOSEM);
    return 1;
}

/* One body, in the mode of st (proto _dump_syntax, lines 1043-1093, and _dump_validate). */
static int
dp_w_body(dp_cur * c, dp_state * st, int kind)
{
    dp_node * nd = st->node;
    dp_span t;

    nd->kind = kind;
    switch (kind)
    {
        case DP_RAT:
            if (!dp_h(c, &nd->a) || !dp_h(c, &nd->b))
                return dp_parse_fail(st);
            if (st->mode == DP_SEM && !dp_v_fmpq(nd->a, nd->b))
                return dp_fail(st, ADF_DOMAIN);
            return 1;
        case DP_FBALL:
            if (!dp_w_fb(c, st, &nd->fb))
                return 0;
            if (st->mode == DP_SEM && !dp_v_fb(c, &nd->fb, NULL, NULL, NULL))
                return dp_fail(st, ADF_DOMAIN);
            return 1;
        case DP_SCALED:
            if (!dp_next(c, &t) || !(dp_kw_is(t, "x") || dp_kw_is(t, "s")))
                return dp_parse_fail(st);
            nd->form = t.p[0];
            if (!dp_h(c, &nd->a) || !dp_h(c, &nd->b) || (nd->form == 's' && !dp_h(c, &nd->c)))
                return dp_parse_fail(st);
            if (!dp_w_ctx(c, st, &nd->ctx))
                return 0;
            if (st->mode == DP_SEM)
            {
                /* conventions 5.4 (line 477); proto lines 1306-1311 */
                int ok = dp_v_ctx(c, &nd->ctx, 0) && dp_v_fmpq(nd->a, nd->b);
                if (ok && nd->form == 's')
                {
                    fmpz_t u, K;
                    fmpz_init(u);
                    fmpz_init(K);
                    dp_fmpz(u, nd->c);
                    dp_fmpz(K, nd->ctx.K);
                    ok = !dp_neg(nd->a) && !dp_zero(nd->a) && fmpz_sgn(u) >= 0 && fmpz_cmp(u, K) < 0;
                    fmpz_clear(u);
                    fmpz_clear(K);
                }
                if (!ok)
                    return dp_fail(st, ADF_DOMAIN);
            }
            return 1;
        case DP_ADELE:
        case DP_CADELE:
            if (!dp_w_arch(c, st, kind == DP_ADELE ? 4 : 8, &nd->narch, nd->arb) || !dp_w_fb(c, st, &nd->fb))
                return 0;
            if (st->mode == DP_SEM)
            {
                /* conventions 5.5 and 10.1 (the archimedean count is 1 for Q, line 1358) */
                if (nd->narch != 1 || !dp_v_arb(&nd->arb[0])
                    || (kind == DP_CADELE && !dp_v_arb(&nd->arb[1]))
                    || !dp_v_fb(c, &nd->fb, NULL, NULL, NULL))
                    return dp_fail(st, ADF_DOMAIN);
            }
            return 1;
        case DP_QCLASS:
            return dp_w_qclass(c, st);
        case DP_MODCTX:
            if (!dp_w_ctx(c, st, &nd->ctx))
                return 0;
            if (st->mode == DP_SEM && !dp_v_ctx(c, &nd->ctx, 0))
                return dp_fail(st, ADF_DOMAIN);
            return 1;
        default:
            return dp_w_other(c, st, kind);
    }
}

/* ================================================================================================
   The whole text: stages 1 to 6 (conventions 8.5), then the count of context occurrences. */

typedef struct
{
    const char * s;
    size_t len;
    size_t body;                       /* offset of the first token after the body keyword */
    size_t ntok;                       /* tokens from there to the end */
    dp_node node;
    size_t nocc;
} dp_parsed;

/* Stages 1 and 2, and the header of stage 3 (conventions 8.5 items 1 to 3; 10.1 line 1326; 10.2
   "Version and field", lines 1429-1431; proto _dump_syntax, lines 1015-1040). On ADF_OK *body
   is the offset of the body. */
static int
dp_header(const char * s, size_t len, const adf_text_limits_t * lim, size_t * body)
{
    size_t i, pos, v0, f0;

    if (len > lim->max_len)
        return ADF_LIMIT;              /* stage 1, before any byte is read */
    if (s == NULL)
        return ADF_PARSE;              /* only len = 0 is allowed with NULL (text.h) */
    /* Stage 2 (8.2): the alphabet is 0x20 to 0x7e with TAB, LF and CR.  Those three are bytes of
       the alphabet, so they pass here whatever follows; the grammar of a dump (10.1) allows no
       whitespace other than the single spaces between tokens, and the header is judged before
       the body, so a version or a field other than "1" and "Q" is ADF_UNSUPPORTED whatever the
       body holds.  Every other byte, NUL and every byte >= 0x80, is ADF_PARSE here. */
    for (i = 0; i < len; i++)
    {
        unsigned char b = (unsigned char) s[i];

        if ((b < 0x20 && b != 0x09 && b != 0x0a && b != 0x0d) || b > 0x7e)
            return ADF_PARSE;
    }
    if (len < 3 || s[0] != 'a' || s[1] != 'd' || s[2] != 'f')
        return ADF_PARSE;
    pos = v0 = 3;
    while (pos < len && s[pos] >= '0' && s[pos] <= '9')
        pos++;
    if (pos == v0 || (pos < len && s[pos] != ' '))
        return ADF_PARSE;              /* "adf" [0-9]+ ( " " | end ) */
    if (pos - v0 > 1 && s[v0] == '0')
        return ADF_PARSE;              /* a version with a leading zero */
    if (!(pos - v0 == 1 && s[v0] == '1'))
        return ADF_UNSUPPORTED;        /* another version, before the body is read */
    if (pos == len)
        return ADF_PARSE;
    pos++;
    f0 = pos;                          /* field = upper, {nonspace} */
    if (pos >= len || s[pos] < 'A' || s[pos] > 'Z')
        return ADF_PARSE;
    while (pos < len && s[pos] != ' ')
        pos++;
    if (!(pos - f0 == 1 && s[f0] == 'Q'))
        return ADF_UNSUPPORTED;
    if (pos == len)
        return ADF_PARSE;
    *body = pos + 1;
    return ADF_OK;
}

static void
dp_cur_at(dp_cur * c, const dp_parsed * P)
{
    c->s = P->s;
    c->len = P->len;
    c->pos = P->body;
    c->left = P->ntok;
}

/* Validate the whole text (stages 1 to 6). want_kind >= 0 restricts the body to that keyword
   (a typed loader: another body is not a sentence of its start symbol, ADF_PARSE, as a typed
   parser of the value form does not coerce, conventions 9.7; HEADER-FINDING: dump.h does not
   say which status a loader gives for the dump of another type). On ADF_OK
   P->node holds the spans and P->nocc the number of context occurrences. May return DP_NOSEM
   (only when want_kind < 0). */
static int
dp_validate(dp_parsed * P, const char * s, size_t len, const adf_text_limits_t * lim, int want_kind)
{
    adf_text_limits_t dflt;
    dp_state st;
    dp_cur c;
    dp_span t;
    size_t b, i;
    int kind, mode, r;

    if (lim == NULL)
    {
        adf_text_limits_default(&dflt);
        lim = &dflt;
    }
    r = dp_header(s, len, lim, &b);
    if (r != ADF_OK)
        return r;
    /* the body: tokens separated by exactly one space, none empty (proto lines 1037-1039) */
    if (b >= len || s[b] == ' ' || s[len - 1] == ' ')
        return ADF_PARSE;
    P->ntok = 1;
    for (i = b; i < len; i++)
        if (s[i] == ' ')
        {
            if (s[i + 1] == ' ')       /* i + 1 < len: s[len - 1] is not a space */
                return ADF_PARSE;
            P->ntok++;
        }
    P->s = s;
    P->len = len;
    c.s = s;
    c.len = len;
    c.pos = b;
    c.left = P->ntok;
    (void) dp_next(&c, &t);
    for (kind = 0; kind < DP_NKINDS; kind++)
        if (dp_kw_is(t, dp_kind_name[kind]))
            break;
    if (kind == DP_NKINDS || (want_kind >= 0 && kind != want_kind))
        return ADF_PARSE;
    P->body = c.pos;
    P->ntok = c.left;
    for (mode = DP_SYNTAX; mode <= DP_OCC; mode++)
    {
        memset(&st, 0, sizeof(st));     /* st = ADF_OK, nocc = 0, no descriptor to fill */
        st.mode = mode;
        st.lim = lim;
        st.node = &P->node;
        dp_cur_at(&c, P);
        if (!dp_w_body(&c, &st, kind))
            return st.st;
        if (mode == DP_SYNTAX && c.left != 0)
            return ADF_PARSE;          /* tokens after the body (proto line 1094) */
    }
    P->nocc = st.nocc;
    return ADF_OK;
}

/* Copy occurrences first .. first + n - 1 of a validated text into d[0 .. n - 1]. */
static void
dp_copy_occurrences(const dp_parsed * P, adf_ctx_desc_t * d, size_t first, size_t n)
{
    dp_state st;
    dp_cur c;
    dp_node nd;

    st.mode = DP_OCC;
    st.lim = NULL;                     /* not read in DP_OCC */
    st.st = ADF_OK;
    st.arb_exp_limit = 0;              /* not read in DP_OCC */
    st.node = &nd;
    st.nocc = 0;
    st.descs = d;
    st.first = first;
    st.ndescs = n;
    dp_cur_at(&c, P);
    (void) dp_w_body(&c, &st, P->node.kind);
}

/* adf_modctx_new_from_dump (src/modctx.c) calls this: validate any dump in the order of 8.5,
   then the occurrence index; on ADF_OK *d holds occurrence `occurrence` (conventions 10.2,
   closure C3: "Validate the whole dump in the order of 8.5 before checking the occurrence index
   or allocating a context"; proto modctx_new_from_dump, lines 1510-1521). d is untouched on
   every other status. Hidden: not part of the interface. */
DP_HIDDEN int adf_dump_ctx_occurrence(adf_ctx_desc_t * d, const char * s, size_t len, size_t occurrence,
                                      const adf_text_limits_t * lim);

DP_HIDDEN int
adf_dump_ctx_occurrence(adf_ctx_desc_t * d, const char * s, size_t len, size_t occurrence,
                        const adf_text_limits_t * lim)
{
    dp_parsed P;
    int r = dp_validate(&P, s, len, lim, -1);

    if (r == DP_NOSEM)
        return ADF_DOMAIN;             /* no occurrence in such a body, valid or not */
    if (r != ADF_OK)
        return r;
    if (occurrence >= P.nocc)
        return ADF_DOMAIN;
    dp_copy_occurrences(&P, d, occurrence, 1);
    return ADF_OK;
}

/* ================================================================================================
   Bindings (conventions 10.2, gate finding G3; proto dump_load_check, lines 1493-1507). */

/* 1 if ctx has the modulus and the ordered blocks of the occurrence x. */
static int
dp_bind_matches(const adf_modctx_struct * ctx, const dp_parsed * P, const dp_ctx * x)
{
    fmpz_t K, Kc;
    size_t i, pos = x->q0;
    int ok;

    fmpz_init(K);
    fmpz_init(Kc);
    dp_fmpz(K, x->K);
    adf_modctx_get_modulus(Kc, ctx);
    ok = fmpz_equal(K, Kc) && adf_modctx_nblocks(ctx) == (slong) x->k;
    for (i = 0; ok && i < x->k; i++)
    {
        ulong q = 0;
        (void) dp_word(dp_tok_at(P->s, P->len, &pos), &q);
        ok = adf_modctx_block(ctx, (slong) i) == q;
    }
    fmpz_clear(K);
    fmpz_clear(Kc);
    return ok;
}

/* The context occurrence of a validated body of the five typed kinds (at most one), or NULL. */
static const dp_ctx *
dp_the_occurrence(const dp_parsed * P)
{
    const dp_node * nd = &P->node;

    if (nd->kind == DP_SCALED)
        return &nd->ctx;
    if ((nd->kind == DP_FBALL || nd->kind == DP_ADELE || nd->kind == DP_CADELE) && nd->fb.local)
        return &nd->fb.ctx;
    return NULL;
}

/* The binding count that stands for the one-context form adf_x_load_str(x, s, len, ctx, lim):
   the array with ctx at every occurrence (conventions 10.2: "a dump without contexts needs no
   binding, and any ctx is accepted"). No caller can pass it: an array of SIZE_MAX pointers does
   not fit in memory. */
#define DP_ONE_CONTEXT ((size_t) -1)

/* A binding count given by a caller of adf_x_load_str_binds. SIZE_MAX is then a count like any
   other, which differs from every number of occurrences (at most one per token), so it is moved
   to SIZE_MAX - 1 and never read as the one-context form. */
static size_t
dp_count_of(size_t nbinds)
{
    return nbinds == DP_ONE_CONTEXT ? nbinds - 1 : nbinds;
}

/* Validate, then check the bindings. */
static int
dp_prepare(dp_parsed * P, int kind, const char * s, size_t len, const adf_modctx_struct * const * binds,
           size_t nbinds, const adf_text_limits_t * lim)
{
    const dp_ctx * occ;
    int r = dp_validate(P, s, len, lim, kind);

    if (r != ADF_OK)
        return r;
    occ = dp_the_occurrence(P);
    if (nbinds == DP_ONE_CONTEXT)
        nbinds = P->nocc;
    if (nbinds != P->nocc)
        return ADF_DOMAIN;
    if (occ != NULL)
    {
        if (binds == NULL || binds[0] == NULL || !dp_bind_matches(binds[0], P, occ))
            return ADF_DOMAIN;
    }
    return ADF_OK;
}

/* ================================================================================================
   Building a value from validated fields (conventions 10.2: "Value fields and backend are
   restored exactly relative to those bindings"). */

/* The radius m 2^e exactly, 1 <= m < 2^30 odd: a mag is MAG_MAN 2^(MAG_EXP - MAG_BITS) with a
   mantissa of exactly MAG_BITS bits (mag.h:113-119, 135-140; mag_one, mag.h:217-221, is
   MAG_ONE_HALF with exponent 1). With b the bit length of m, the mantissa m 2^(30 - b) and the
   exponent e + b give m 2^(30 - b) 2^(e + b - 30) = m 2^e. mag_set_ui_2exp_si is documented as
   an upper bound only (mag.rst:149-151) and mag_set_fmpz_2exp_fmpz is not exact at 30 bits
   (lanes/m1-text/report.md, finding 1), so neither is used. */
static void
dp_mag_set_exact(mag_t r, ulong m, const fmpz_t e)
{
    unsigned b = FLINT_BIT_COUNT(m);

    MAG_MAN(r) = m << (MAG_BITS - b);
    fmpz_add_ui(MAG_EXPREF(r), e, b);
}

static void
dp_set_arb(arb_t x, const dp_arb * a)
{
    fmpz_t m, e;

    fmpz_init(m);
    fmpz_init(e);
    if (dp_zero(a->m))
        arf_zero(arb_midref(x));
    else
    {
        dp_fmpz(m, a->m);
        dp_fmpz(e, a->e);
        arf_set_fmpz_2exp(arb_midref(x), m, e);   /* exact: arf.rst:227-229 */
    }
    if (dp_zero(a->rm))
        mag_zero(arb_radref(x));
    else
    {
        ulong rm = 0;
        (void) dp_word(a->rm, &rm);
        dp_fmpz(e, a->re);
        dp_mag_set_exact(arb_radref(x), rm, e);
    }
    fmpz_clear(m);
    fmpz_clear(e);
}

/* f is a freshly initialised global exact 0. A local ball: A = 0, H = K, d, the residues in an
   array owned by the value (flint_malloc; fball.h layout, conventions 5.3). */
static void
dp_set_fb(adf_fball_struct * f, const dp_parsed * P, const dp_fb * fb, const adf_modctx_struct * ctx)
{
    if (!fb->local)
    {
        dp_fmpz(f->A, fb->A);
        dp_fmpz(f->H, fb->H);
        dp_fmpz(f->d, fb->d);
        return;
    }
    {
        size_t i, pos = fb->r0;
        fmpz_zero(f->A);
        dp_fmpz(f->H, fb->ctx.K);
        dp_fmpz(f->d, fb->d);
        f->res = (ulong *) flint_malloc(fb->ctx.k * sizeof(ulong));
        for (i = 0; i < fb->ctx.k; i++)
        {
            ulong r = 0;
            (void) dp_word(dp_tok_at(P->s, P->len, &pos), &r);
            f->res[i] = r;
        }
        f->mctx = ctx;
        f->backend = ADF_LOCAL;
    }
}

/* ================================================================================================
   Writing (conventions 10.1; proto _dump_print, lines 1425-1469). */

typedef struct
{
    char * p;
    size_t n, cap;
} dp_sb;

static void
dp_sb_init(dp_sb * b)
{
    b->cap = 64;
    b->n = 0;
    b->p = (char *) flint_malloc(b->cap);
}

static void
dp_sb_room(dp_sb * b, size_t extra)
{
    if (b->n + extra + 1 > b->cap)
    {
        while (b->n + extra + 1 > b->cap)
            b->cap *= 2;
        b->p = (char *) flint_realloc(b->p, b->cap);
    }
}

static void
dp_sb_lit(dp_sb * b, const char * w)
{
    size_t n = strlen(w);              /* w is a literal of this file */
    dp_sb_room(b, n);
    memcpy(b->p + b->n, w, n);
    b->n += n;
}

/* " " then the lower-case hexadecimal spelling of x (fmpz_get_str into a buffer of
   fmpz_sizeinbase + 2 bytes, fmpz.rst:337-344, 600). */
static void
dp_sb_fmpz(dp_sb * b, const fmpz_t x)
{
    /* written: the space, the digits, a sign, the NUL of fmpz_get_str; dp_sb_room adds the byte of
       the NUL, so the space and the sign are the 2 */
    size_t room = fmpz_sizeinbase(x, 16) + 2;

    dp_sb_room(b, room);
    b->p[b->n++] = ' ';
    fmpz_get_str(b->p + b->n, 16, x);
    b->n += strlen(b->p + b->n);       /* our own output, NUL-terminated by fmpz_get_str */
}

static void
dp_sb_ui(dp_sb * b, ulong v)
{
    char tmp[20];
    size_t n = 0, i;

    do
    {
        unsigned d = (unsigned) (v & 15);
        tmp[n++] = (char) (d < 10 ? '0' + d : 'a' + (d - 10));
        v >>= 4;
    } while (v != 0);
    dp_sb_room(b, n + 1);
    b->p[b->n++] = ' ';
    for (i = 0; i < n; i++)
        b->p[b->n++] = tmp[n - 1 - i];
}

static char *
dp_sb_finish(dp_sb * b, size_t * len)
{
    /* dp_sb_room keeps n + 1 <= cap after every append; a violation would already be a heap
       error, so it is not survivable (as the length check of adf_modctx_dump_str) */
    if (b->n + 1 > b->cap)
        flint_abort();
    b->p[b->n] = '\0';
    *len = b->n;
    return b->p;
}

/* The four fields of a real ball as arb_dump_str writes them (conventions 10.2, [probed]):
   mantissa and exponent of the midpoint, odd mantissa or "0 0"; the same for the radius
   (arf_get_fmpz_2exp, arf.rst:236-242; arf_set_mag, arf.rst:411-413, exact). */
static void
dp_sb_arb(dp_sb * b, const arb_t x)
{
    fmpz_t m, e;
    arf_t r;

    fmpz_init(m);
    fmpz_init(e);
    arf_init(r);
    arf_get_fmpz_2exp(m, e, arb_midref(x));
    dp_sb_fmpz(b, m);
    dp_sb_fmpz(b, e);
    arf_set_mag(r, arb_radref(x));
    arf_get_fmpz_2exp(m, e, r);
    dp_sb_fmpz(b, m);
    dp_sb_fmpz(b, e);
    fmpz_clear(m);
    fmpz_clear(e);
    arf_clear(r);
}

/* ctx = K k q_1 .. q_k of a context (public read access, modctx.h). */
static void
dp_sb_ctx(dp_sb * b, const adf_modctx_struct * ctx)
{
    fmpz_t K;
    slong i, k = adf_modctx_nblocks(ctx);

    fmpz_init(K);
    adf_modctx_get_modulus(K, ctx);
    dp_sb_fmpz(b, K);
    dp_sb_ui(b, (ulong) k);
    for (i = 0; i < k; i++)
        dp_sb_ui(b, adf_modctx_block(ctx, i));
    fmpz_clear(K);
}

/* fb: "g A H d" or "l d K k q.. r.." (the raw local data, conventions 5.3, 10.2). */
static void
dp_sb_fb(dp_sb * b, const adf_fball_struct * f)
{
    if (f->backend != ADF_LOCAL)
    {
        dp_sb_lit(b, " g");
        dp_sb_fmpz(b, f->A);
        dp_sb_fmpz(b, f->H);
        dp_sb_fmpz(b, f->d);
    }
    else
    {
        slong i, k = adf_modctx_nblocks(f->mctx);
        dp_sb_lit(b, " l");
        dp_sb_fmpz(b, f->d);
        dp_sb_ctx(b, f->mctx);
        for (i = 0; i < k; i++)
            dp_sb_ui(b, f->res[i]);
    }
}

/* ================================================================================================
   The public functions (include/adelefeld/dump.h). */

/* ---- inspection (conventions 10.2, closure C2, verbatim in dump.h) ---- */

static int
dp_inspect(int kind, size_t * nctx, adf_ctx_desc_t * descs, const char * s, size_t len,
           const adf_text_limits_t * lim)
{
    dp_parsed P;
    int r;

    if (nctx == NULL)
        return ADF_DOMAIN;             /* HEADER-FINDING: dump.h does not say; nothing is written */
    r = dp_validate(&P, s, len, lim, kind);
    if (r != ADF_OK)
        return r;
    if (descs != NULL)
    {
        if (P.nocc > *nctx)
            return ADF_LIMIT;          /* *nctx and every descriptor untouched */
        dp_copy_occurrences(&P, descs, 0, P.nocc);
    }
    *nctx = P.nocc;
    return ADF_OK;
}

int
adf_rat_dump_inspect(size_t * nctx, adf_ctx_desc_t * descs, const char * s, size_t len,
                     const adf_text_limits_t * lim)
{
    return dp_inspect(DP_RAT, nctx, descs, s, len, lim);
}

int
adf_fball_dump_inspect(size_t * nctx, adf_ctx_desc_t * descs, const char * s, size_t len,
                       const adf_text_limits_t * lim)
{
    return dp_inspect(DP_FBALL, nctx, descs, s, len, lim);
}

int
adf_scaled_dump_inspect(size_t * nctx, adf_ctx_desc_t * descs, const char * s, size_t len,
                        const adf_text_limits_t * lim)
{
    return dp_inspect(DP_SCALED, nctx, descs, s, len, lim);
}

int
adf_adele_dump_inspect(size_t * nctx, adf_ctx_desc_t * descs, const char * s, size_t len,
                       const adf_text_limits_t * lim)
{
    return dp_inspect(DP_ADELE, nctx, descs, s, len, lim);
}

int
adf_cadele_dump_inspect(size_t * nctx, adf_ctx_desc_t * descs, const char * s, size_t len,
                        const adf_text_limits_t * lim)
{
    return dp_inspect(DP_CADELE, nctx, descs, s, len, lim);
}

/* ---- adf_rat: body "rat num den" (conventions 10.1, 5.1) ---- */

static int
dp_load_rat(adf_rat_t x, const char * s, size_t len, const adf_modctx_struct * const * binds, size_t nbinds,
            const adf_text_limits_t * lim)
{
    dp_parsed P;
    adf_rat_t t;
    int r = dp_prepare(&P, DP_RAT, s, len, binds, nbinds, lim);

    if (r != ADF_OK)
        return r;
    adf_rat_init(t);
    dp_fmpz(fmpq_numref(t->q), P.node.a);
    dp_fmpz(fmpq_denref(t->q), P.node.b);
    adf_rat_swap(x, t);
    adf_rat_clear(t);
    return ADF_OK;
}

int
adf_rat_load_str(adf_rat_t x, const char * s, size_t len, const adf_modctx_struct * ctx,
                 const adf_text_limits_t * lim)
{
    (void) ctx;                        /* no occurrence: the one-context form is no binding */
    return dp_load_rat(x, s, len, NULL, 0, lim);
}

int
adf_rat_load_str_binds(adf_rat_t x, const char * s, size_t len, const adf_modctx_struct * const * binds,
                       size_t nbinds, const adf_text_limits_t * lim)
{
    return dp_load_rat(x, s, len, binds, dp_count_of(nbinds), lim);
}

char *
adf_rat_dump_str(size_t * len, const adf_rat_t x)
{
    dp_sb b;

    dp_sb_init(&b);
    dp_sb_lit(&b, "adf1 Q rat");
    dp_sb_fmpz(&b, fmpq_numref(x->q));
    dp_sb_fmpz(&b, fmpq_denref(x->q));
    return dp_sb_finish(&b, len);
}

/* ---- adf_fball: body "fball fb" (conventions 10.1, 5.2, 5.3) ---- */

static int
dp_load_fball(adf_fball_t x, const char * s, size_t len, const adf_modctx_struct * const * binds,
              size_t nbinds, const adf_text_limits_t * lim)
{
    dp_parsed P;
    adf_fball_t t;
    int r = dp_prepare(&P, DP_FBALL, s, len, binds, nbinds, lim);

    if (r != ADF_OK)
        return r;
    adf_fball_init(t);
    dp_set_fb(t, &P, &P.node.fb, P.node.fb.local ? binds[0] : NULL);
    adf_fball_swap(x, t);
    adf_fball_clear(t);
    return ADF_OK;
}

int
adf_fball_load_str(adf_fball_t x, const char * s, size_t len, const adf_modctx_struct * ctx,
                   const adf_text_limits_t * lim)
{
    return dp_load_fball(x, s, len, &ctx, DP_ONE_CONTEXT, lim);
}

int
adf_fball_load_str_binds(adf_fball_t x, const char * s, size_t len, const adf_modctx_struct * const * binds,
                         size_t nbinds, const adf_text_limits_t * lim)
{
    return dp_load_fball(x, s, len, binds, dp_count_of(nbinds), lim);
}

char *
adf_fball_dump_str(size_t * len, const adf_fball_t x)
{
    dp_sb b;

    dp_sb_init(&b);
    dp_sb_lit(&b, "adf1 Q fball");
    dp_sb_fb(&b, x);
    return dp_sb_finish(&b, len);
}

/* ---- adf_scaled: body "scaled x num den ctx" | "scaled s num den u ctx" (conventions 10.1,
   5.4). The fields of adf_scaled_struct are written directly (scaled.h layout); the output's
   old s and u are released after the swap. ---- */

static int
dp_load_scaled(adf_scaled_t x, const char * s, size_t len, const adf_modctx_struct * const * binds,
               size_t nbinds, const adf_text_limits_t * lim)
{
    dp_parsed P;
    fmpq_t q;
    fmpz_t u;
    int r = dp_prepare(&P, DP_SCALED, s, len, binds, nbinds, lim);

    if (r != ADF_OK)
        return r;
    fmpq_init(q);
    fmpz_init(u);
    dp_fmpz(fmpq_numref(q), P.node.a);
    dp_fmpz(fmpq_denref(q), P.node.b);
    if (P.node.form == 's')
        dp_fmpz(u, P.node.c);
    fmpq_swap(x->s, q);
    fmpz_swap(x->u, u);
    x->mctx = binds[0];
    x->exact = P.node.form == 'x';
    fmpq_clear(q);
    fmpz_clear(u);
    return ADF_OK;
}

int
adf_scaled_load_str(adf_scaled_t x, const char * s, size_t len, const adf_modctx_struct * ctx,
                    const adf_text_limits_t * lim)
{
    return dp_load_scaled(x, s, len, &ctx, DP_ONE_CONTEXT, lim);
}

int
adf_scaled_load_str_binds(adf_scaled_t x, const char * s, size_t len, const adf_modctx_struct * const * binds,
                          size_t nbinds, const adf_text_limits_t * lim)
{
    return dp_load_scaled(x, s, len, binds, dp_count_of(nbinds), lim);
}

char *
adf_scaled_dump_str(size_t * len, const adf_scaled_t x)
{
    dp_sb b;

    dp_sb_init(&b);
    dp_sb_lit(&b, x->exact ? "adf1 Q scaled x" : "adf1 Q scaled s");
    dp_sb_fmpz(&b, fmpq_numref(x->s));
    dp_sb_fmpz(&b, fmpq_denref(x->s));
    if (!x->exact)
        dp_sb_fmpz(&b, x->u);
    dp_sb_ctx(&b, x->mctx);
    return dp_sb_finish(&b, len);
}

/* adf_scaled_get_str: the value form of the finite ball that x denotes (conventions 9.4 row
   "scaled value", line 1190; 5.4 "Meaning", line 479): "(* ; q(s))" if exact, else the ball
   s u + s K Zhat, printed by adf_fball_get_str. The ball is built with adf_fball_set_rat and
   adf_fball_set_center_radius (fball.h), not with adf_scaled_get_fball, which is another lane's. */
char *
adf_scaled_get_str(size_t * len, const adf_scaled_t x)
{
    adf_fball_t f;
    adf_rat_t c, N;
    fmpz_t K;
    char * t;

    adf_fball_init(f);
    adf_rat_init(c);
    adf_rat_init(N);
    if (x->exact)
    {
        fmpq_set(c->q, x->s);
        adf_fball_set_rat(f, c);
    }
    else
    {
        fmpz_init(K);
        adf_modctx_get_modulus(K, x->mctx);
        fmpq_mul_fmpz(c->q, x->s, x->u);
        fmpq_mul_fmpz(N->q, x->s, K);
        if (adf_fball_set_center_radius(f, c, N) != ADF_OK)
            flint_abort();             /* cannot happen: N = s K > 0 by the predicate of 5.4 */
        fmpz_clear(K);
    }
    t = adf_fball_get_str(len, f);
    adf_rat_clear(c);
    adf_rat_clear(N);
    adf_fball_clear(f);
    return t;
}

/* ---- adf_adele: body "adele 1 arb fb"; adf_cadele: "cadele 1 arb arb fb" (conventions 10.1,
   5.5). ---- */

static int
dp_load_adele(adf_adele_t x, const char * s, size_t len, const adf_modctx_struct * const * binds,
              size_t nbinds, const adf_text_limits_t * lim)
{
    dp_parsed P;
    adf_adele_t t;
    int r = dp_prepare(&P, DP_ADELE, s, len, binds, nbinds, lim);

    if (r != ADF_OK)
        return r;
    adf_adele_init(t);
    dp_set_arb(t->inf, &P.node.arb[0]);
    dp_set_fb(&t->fin, &P, &P.node.fb, P.node.fb.local ? binds[0] : NULL);
    adf_adele_swap(x, t);
    adf_adele_clear(t);
    return ADF_OK;
}

int
adf_adele_load_str(adf_adele_t x, const char * s, size_t len, const adf_modctx_struct * ctx,
                   const adf_text_limits_t * lim)
{
    return dp_load_adele(x, s, len, &ctx, DP_ONE_CONTEXT, lim);
}

int
adf_adele_load_str_binds(adf_adele_t x, const char * s, size_t len, const adf_modctx_struct * const * binds,
                         size_t nbinds, const adf_text_limits_t * lim)
{
    return dp_load_adele(x, s, len, binds, dp_count_of(nbinds), lim);
}

char *
adf_adele_dump_str(size_t * len, const adf_adele_t x)
{
    dp_sb b;

    dp_sb_init(&b);
    dp_sb_lit(&b, "adf1 Q adele 1");
    dp_sb_arb(&b, x->inf);
    dp_sb_fb(&b, &x->fin);
    return dp_sb_finish(&b, len);
}

static int
dp_load_cadele(adf_cadele_t x, const char * s, size_t len, const adf_modctx_struct * const * binds,
               size_t nbinds, const adf_text_limits_t * lim)
{
    dp_parsed P;
    adf_cadele_t t;
    int r = dp_prepare(&P, DP_CADELE, s, len, binds, nbinds, lim);

    if (r != ADF_OK)
        return r;
    adf_cadele_init(t);
    dp_set_arb(acb_realref(t->inf), &P.node.arb[0]);
    dp_set_arb(acb_imagref(t->inf), &P.node.arb[1]);
    dp_set_fb(&t->fin, &P, &P.node.fb, P.node.fb.local ? binds[0] : NULL);
    adf_cadele_swap(x, t);
    adf_cadele_clear(t);
    return ADF_OK;
}

int
adf_cadele_load_str(adf_cadele_t x, const char * s, size_t len, const adf_modctx_struct * ctx,
                    const adf_text_limits_t * lim)
{
    return dp_load_cadele(x, s, len, &ctx, DP_ONE_CONTEXT, lim);
}

int
adf_cadele_load_str_binds(adf_cadele_t x, const char * s, size_t len, const adf_modctx_struct * const * binds,
                          size_t nbinds, const adf_text_limits_t * lim)
{
    return dp_load_cadele(x, s, len, binds, dp_count_of(nbinds), lim);
}

char *
adf_cadele_dump_str(size_t * len, const adf_cadele_t x)
{
    dp_sb b;

    dp_sb_init(&b);
    dp_sb_lit(&b, "adf1 Q cadele 1");
    dp_sb_arb(&b, acb_realref(x->inf));
    dp_sb_arb(&b, acb_imagref(x->inf));
    dp_sb_fb(&b, &x->fin);
    return dp_sb_finish(&b, len);
}
