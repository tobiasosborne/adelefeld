/* tools/adf/adf.c: the command-line driver adf, work package 1.5 (docs/PLAN.md 6, row 1.5:
   "the tables of SPEC.md typed at the prompt").

   The driver is a thin program over the public interface: it parses nothing of the value
   form itself.  Every operand is read by adf_text_classify and by the typed parser of its
   kind (docs/conventions.md 9.7), and printed by the typed printer (conventions 9.4).
   It creates no context, and every value it holds is in the global backend, which the
   value form does not record (SPEC 4.1, conventions 9.8 A11).

   The language.  Input is read line by line from standard input or from the file named
   on the command line.  A line is at most 65536 bytes; a longer line is an error for that
   line (ADF_LIMIT) and the rest of it is skipped.  Empty lines and lines whose first
   non-blank byte is "#" are ignored.  Every other line is one command

       <operation> <operand> [ <separator> <operand> [ <separator> <operand> ] ]

   The separator is the word "with" between spaces; tools/adf/README.md shows why no byte
   outside the alphabet of conventions 8.2 exists and why "with" cannot occur in a value
   text.  The settings are "prec <bits>" and "digits <n>", one per line.  The operations
   are show, type, add, sub, mul, neg, div, equal, contains, overlaps, compare, reconstruct,
   cap, dump and load; reconstruct takes either one operand (an adele) or three (a finite
   ball and an interval given as two exact rationals), which is the one documented extension
   of the two-operand form.  dump writes the dump form of conventions 10.1 of a value, and
   load reads such a text; the driver has no context, so a dump with a context occurrence is
   not read.

   The order of the checks of one command, which tools/adf/README.md states in full:
     1. the line: an operation word of the table and the right number of operands (ADF_PARSE);
     2. every operand, in order: the syntax of the value form alone, adf_text_classify
        (conventions 8.5, stages 1 to 3);
     3. every operand, in order: its kind; a kind of conventions 9.7 with no typed parser in
        this build is ADF_UNSUPPORTED, decided before any value of the line is read;
     4. every operand, in order: its value, by the typed parser of its kind (conventions 8.5,
        stages 4 to 7);
     5. the operation: the pair of types, the domain of the operation, the problem to be
        solved (ADF_DOMAIN, ADF_NOT_UNIT, ADF_NO_SOLUTION, ADF_NOT_UNIQUE);
     6. the printer: a printer that returns NULL is ADF_LIMIT.
   A line that is not a sentence of the grammar of conventions 9.2 is decided on the syntax
   of its operands first, because that is a fact about the line; a kind that this build does
   not implement is decided before any value is read, because a request on a type that
   version 1 does not implement is not a domain error (conventions 3.1, ADF_UNSUPPORTED).

   Mixed operand types are combined only where SPEC 4.1 defines it: an exact rational with
   a finite ball, an adele or a complex adele (the rational is converted only in the
   coordinate where it is inexact, SPEC 4.1), and an adele with a complex adele (the
   embedding of SPEC 4.1, "The type adf_cadele").  Every other pair is ADF_DOMAIN.  A kind
   of the value form with no typed parser in this build (the local ball, the partial ball,
   the idele, the idele class, the quotient class, the functions and the character, work
   packages 1.8 and later) is ADF_UNSUPPORTED.

   Output: one line per command on standard output, either the value text, "true" or
   "false", one of "equal", "different" and "undecided" for compare, a name of a kind for
   type, the dump form for dump, or "error: <STATUS NAME>" with the name of adf_status_str
   (conventions 11.1).  A setting writes no line.  The exit status is 0 if no command failed,
   1 otherwise, 2 for a usage error.  Nothing else is printed unless -v is given.

   The guard on printing is the rule of the library and not one of the driver (decision
   M1-D6, include/adelefeld/text.h:32-38): a printer of a value with a real or complex part
   returns NULL with length 0 when the midpoint or the radius of one of its real balls has a
   binary exponent above ADF_PRINT_EXP_MAX in absolute value, and the driver answers
   ADF_LIMIT for a NULL.  The driver holds no bound of its own; the test tests/test_driver.sh
   checks that.  A prec above ADF_PRINT_EXP_MAX is refused with ADF_LIMIT by decision M1-D1.
   The cap is a choice: a real result of magnitude about 2^k rounded at prec p has a radius
   with a binary exponent of about k - p, so above the cap a result of magnitude about 1 or
   below cannot be printed, and a larger one can. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <adelefeld.h>

/* ---- the limits of the driver (README of tools/adf) ---- */

/* The largest line the driver reads; a longer line is ADF_LIMIT for that line. */
#define ADF_DRV_MAX_LINE ((size_t) 65536)
/* The precision in bits of the real part of an adele or a complex adele, unless the script
   sets it. */
#define ADF_DRV_PREC_DEFAULT ((slong) 64)
/* The largest value a setting of prec may have is ADF_PRINT_EXP_MAX
   (include/adelefeld/text.h:66), and a larger one is ADF_LIMIT (decision M1-D1).  A real
   result of magnitude about 2^k rounded at prec p has a radius with a binary exponent of
   about k - p (measured: 1/3 at prec 100000 has MAG_EXP -100000, at prec 100001 MAG_EXP
   -100001; 2^50000 / 3 at prec 100001 has MAG_EXP -50001 and is printed:
   docs/reviews/m1/surface/closure-checks/prec_reason_closure.c). */
#define ADF_DRV_PREC_MAX ADF_PRINT_EXP_MAX
/* The largest value any setting of the driver can have, ADF_DIGITS_MAX.  The accumulator of
   adf_drv_setting stops here, so a number of any length is read without an overflow. */
#define ADF_DRV_SET_MAX ((slong) ADF_DIGITS_MAX)
/* The separator between two operands: a blank, the word "with", a blank. */
#define ADF_DRV_SEP " with "
#define ADF_DRV_SEP_LEN ((size_t) 6)

/* ---- the operations ---- */

typedef enum
{
    ADF_DRV_SHOW = 0,
    ADF_DRV_TYPE,
    ADF_DRV_ADD,
    ADF_DRV_SUB,
    ADF_DRV_MUL,
    ADF_DRV_NEG,
    ADF_DRV_DIV,
    ADF_DRV_EQUAL,
    ADF_DRV_CONTAINS,
    ADF_DRV_OVERLAPS,
    ADF_DRV_COMPARE,
    ADF_DRV_RECONSTRUCT,
    ADF_DRV_CAP,
    ADF_DRV_DUMP,
    ADF_DRV_LOAD,
    ADF_DRV_PREC,
    ADF_DRV_DIGITS
} adf_drv_op;

static const struct
{
    const char * name;
    adf_drv_op op;
    int arity;                 /* 0: the operation has two arities (reconstruct) */
} adf_drv_ops[] = {
    { "show", ADF_DRV_SHOW, 1 },
    { "type", ADF_DRV_TYPE, 1 },
    { "add", ADF_DRV_ADD, 2 },
    { "sub", ADF_DRV_SUB, 2 },
    { "mul", ADF_DRV_MUL, 2 },
    { "neg", ADF_DRV_NEG, 1 },
    { "div", ADF_DRV_DIV, 2 },
    { "equal", ADF_DRV_EQUAL, 2 },
    { "contains", ADF_DRV_CONTAINS, 2 },
    { "overlaps", ADF_DRV_OVERLAPS, 2 },
    { "compare", ADF_DRV_COMPARE, 2 },
    { "reconstruct", ADF_DRV_RECONSTRUCT, 0 },
    { "cap", ADF_DRV_CAP, 2 },
    { "dump", ADF_DRV_DUMP, 1 },
    { "load", ADF_DRV_LOAD, 1 },
    { "prec", ADF_DRV_PREC, 1 },
    { "digits", ADF_DRV_DIGITS, 1 }
};

static const size_t adf_drv_op_count = sizeof(adf_drv_ops) / sizeof(adf_drv_ops[0]);

/* The name of a kind of the value form, one per constant of adf_text_kind in the order
   of conventions 9.7.  The names are the start symbols of the grammar of 9.2. */
static const char * const adf_drv_kind_names[] = {
    "rat", "fball", "adele", "cadele", "ucoset", "idele", "idclass",
    "lball", "sball", "qclass", "ffun", "rfun", "char"
};

/* ---- a value ---- */

typedef enum
{
    ADF_DRV_RAT = 0,
    ADF_DRV_FBALL,
    ADF_DRV_ADELE,
    ADF_DRV_CADELE,
    ADF_DRV_OTHER        /* a kind of the value form with no typed parser in this build */
} adf_drv_type;

typedef struct
{
    adf_drv_type type;         /* the type the driver works with, from the kind */
    adf_rat_t r;
    adf_fball_t f;
    adf_adele_t a;
    adf_cadele_t c;
} adf_drv_value;

typedef struct
{
    slong prec;                /* the precision of the real part, in bits */
    slong digits;              /* the significant digits of the real-ball printer */
} adf_drv_state;

static void
adf_drv_value_init(adf_drv_value * v)
{
    v->type = ADF_DRV_OTHER;
    adf_rat_init(v->r);
    adf_fball_init(v->f);
    adf_adele_init(v->a);
    adf_cadele_init(v->c);
}

static void
adf_drv_value_clear(adf_drv_value * v)
{
    adf_rat_clear(v->r);
    adf_fball_clear(v->f);
    adf_adele_clear(v->a);
    adf_cadele_clear(v->c);
    v->type = ADF_DRV_OTHER;
}

/* adf_drv_kind_type(kind): the type the driver works with for that kind of conventions 9.7,
   ADF_DRV_OTHER for the kinds with no typed parser in this build (work packages 1.8 and
   later).  A caller answers ADF_UNSUPPORTED for ADF_DRV_OTHER before any value is read. */
static adf_drv_type
adf_drv_kind_type(adf_text_kind kind)
{
    switch (kind)
    {
        case ADF_TEXT_RAT:
            return ADF_DRV_RAT;
        case ADF_TEXT_FBALL:
            return ADF_DRV_FBALL;
        case ADF_TEXT_ADELE:
            return ADF_DRV_ADELE;
        case ADF_TEXT_CADELE:
            return ADF_DRV_CADELE;
        default:
            return ADF_DRV_OTHER;
    }
}

/* adf_drv_value_read(v, kind, s, len, prec): read one operand whose kind is already known,
   by the typed parser of that kind (conventions 8.1); the parsers do not coerce between
   types.  The kind comes from adf_text_classify (conventions 9.7), which the caller has run
   on every operand before any value is read. */
static int
adf_drv_value_read(adf_drv_value * v, adf_text_kind kind, const char * s, size_t len, slong prec)
{
    v->type = adf_drv_kind_type(kind);

    switch (kind)
    {
        case ADF_TEXT_RAT:
            return adf_rat_set_str(v->r, s, len, NULL);
        case ADF_TEXT_FBALL:
            return adf_fball_set_str(v->f, s, len, NULL);
        case ADF_TEXT_ADELE:
            return adf_adele_set_str(v->a, s, len, prec, NULL);
        case ADF_TEXT_CADELE:
            return adf_cadele_set_str(v->c, s, len, prec, NULL);
        default:
            return ADF_OK;      /* ADF_DRV_OTHER, which the caller has already refused */
    }
}

/* adf_drv_value_print(out, v, digits): write the canonical text of v (conventions 9.4).
   Returns ADF_OK, ADF_UNSUPPORTED (no printer for the kind) or ADF_LIMIT (a printer that
   returns NULL).  The bound on the binary exponent of a midpoint or a radius is the rule of
   the library, decision M1-D6 (include/adelefeld/text.h:32-38): a printer of a value with a
   real or complex part returns NULL with length 0 when such an exponent is above
   ADF_PRINT_EXP_MAX in absolute value, and that is the only way a printer of the driver can
   fail.  The driver holds no bound of its own, so the two can never drift apart. */
static int
adf_drv_value_print(FILE * out, const adf_drv_value * v, slong digits)
{
    char * s = NULL;
    size_t len = 0;

    switch (v->type)
    {
        case ADF_DRV_RAT:
            s = adf_rat_get_str(&len, v->r);
            break;
        case ADF_DRV_FBALL:
            s = adf_fball_get_str(&len, v->f);
            break;
        case ADF_DRV_ADELE:
            s = adf_adele_get_str(&len, v->a, digits);
            break;
        case ADF_DRV_CADELE:
            s = adf_cadele_get_str(&len, v->c, digits);
            break;
        default:
            return ADF_UNSUPPORTED;
    }

    if (s == NULL)
        return ADF_LIMIT;
    (void) fwrite(s, 1, len, out);
    fputc('\n', out);
    adf_str_free(s);
    return ADF_OK;
}

/* adf_drv_value_dump(out, v): write the dump form of v (conventions 10.1).  Every value of
   the value form is in the global backend (conventions 9.8, A11), so its dump has the form
   "g" of conventions 10.1 and no context occurrence.  A dumper never fails
   (include/adelefeld/dump.h, "Rules common to the dumpers"). */
static int
adf_drv_value_dump(FILE * out, const adf_drv_value * v)
{
    char * s = NULL;
    size_t len = 0;

    switch (v->type)
    {
        case ADF_DRV_RAT:
            s = adf_rat_dump_str(&len, v->r);
            break;
        case ADF_DRV_FBALL:
            s = adf_fball_dump_str(&len, v->f);
            break;
        case ADF_DRV_ADELE:
            s = adf_adele_dump_str(&len, v->a);
            break;
        case ADF_DRV_CADELE:
            s = adf_cadele_dump_str(&len, v->c);
            break;
        default:
            return ADF_UNSUPPORTED;
    }

    (void) fwrite(s, 1, len, out);
    fputc('\n', out);
    adf_str_free(s);
    return ADF_OK;
}

/* The bodies of the grammar of conventions 10.1.  The driver reads and writes the four it has
   a type for; the others are valid requests on a type that this build does not implement. */
static const char * const adf_drv_dump_bodies[] = {
    "rat", "fball", "scaled", "adele", "cadele", "ucoset", "idele", "idclass", "lball",
    "sball", "qclass", "ffun", "rfun", "char", "modctx"
};

static const size_t adf_drv_dump_body_count =
    sizeof(adf_drv_dump_bodies) / sizeof(adf_drv_dump_bodies[0]);

/* adf_drv_body_slot(name): the adf_drv_type of a body the driver has a value for, or
   ADF_DRV_OTHER for a body of section 10 that it has none.  The four constants of
   adf_drv_type are in the order of the four names, so the index of the name is the type. */
static adf_drv_type
adf_drv_body_slot(const char * name)
{
    static const char * const mine[] = { "rat", "fball", "adele", "cadele" };
    size_t i;

    for (i = 0; i < sizeof(mine) / sizeof(mine[0]); i++)
        if (strcmp(name, mine[i]) == 0)
            return (adf_drv_type) i;
    return ADF_DRV_OTHER;
}

/* adf_drv_dump_body(s, len, body, blen): the body of the dump, that is the fourth token of
   the text, when the text begins with "adf1 Q ".  Returns 0 when it does not, or when the
   body token is empty or runs to the end of the text: the version, the field and the syntax
   are then decided by the loader itself, which reads the whole text anyway. */
static int
adf_drv_dump_body(const char * s, size_t len, const char ** body, size_t * blen)
{
    static const char prefix[] = "adf1 Q ";
    const size_t plen = sizeof(prefix) - 1;
    size_t i;

    if (len <= plen || memcmp(s, prefix, plen) != 0)
        return 0;
    for (i = plen; i < len && s[i] != ' '; i++)
        ;
    if (i == len)
        return 0;
    *body = s + plen;
    *blen = i - plen;
    return 1;
}

/* adf_drv_load(s, len, v): read a dump text (conventions 10) and put the value in v, whose
   type is set.  The driver has no context, so a dump with a context occurrence is
   ADF_UNSUPPORTED (conventions 10.2: one occurrence per local finite ball, per scaled value
   and per piece of a quotient class).  The occurrence count is asked of the inspector of the
   body, which validates the whole text first and constructs neither a value nor a context
   ("Rules common to the inspectors"). */
static int
adf_drv_load(const char * s, size_t len, adf_drv_value * v)
{
    const char * body;
    size_t blen, i, nctx = 1;
    int status = ADF_OK;

    v->type = ADF_DRV_OTHER;
    if (!adf_drv_dump_body(s, len, &body, &blen))
    {
        /* no "adf1 Q " prefix: the rat loader reads the text and reports its status, which is
           the status of the text: the version, the field and the syntax do not depend on the
           body (conventions 10.1) */
        v->type = ADF_DRV_RAT;
        return adf_rat_load_str(v->r, s, len, NULL, NULL);
    }
    for (i = 0; i < adf_drv_dump_body_count; i++)
        if (strlen(adf_drv_dump_bodies[i]) == blen
            && memcmp(adf_drv_dump_bodies[i], body, blen) == 0)
            break;
    if (i == adf_drv_dump_body_count)
        return ADF_PARSE;      /* the fourth token is no body of conventions 10.1 */
    v->type = adf_drv_body_slot(adf_drv_dump_bodies[i]);
    if (v->type == ADF_DRV_OTHER)
        return ADF_UNSUPPORTED;   /* a body of section 10 that the driver has no type for */

    /* the inspector validates the whole text and writes the number of context occurrences */
    switch (v->type)
    {
        case ADF_DRV_RAT:
            status = adf_rat_dump_inspect(&nctx, NULL, s, len, NULL);
            break;
        case ADF_DRV_FBALL:
            status = adf_fball_dump_inspect(&nctx, NULL, s, len, NULL);
            break;
        case ADF_DRV_ADELE:
            status = adf_adele_dump_inspect(&nctx, NULL, s, len, NULL);
            break;
        default:
            status = adf_cadele_dump_inspect(&nctx, NULL, s, len, NULL);
            break;
    }
    if (status != ADF_OK)
        return status;
    if (nctx != 0)
        return ADF_UNSUPPORTED; /* the dump needs a context, and the driver has none */

    switch (v->type)
    {
        case ADF_DRV_RAT:
            return adf_rat_load_str(v->r, s, len, NULL, NULL);
        case ADF_DRV_FBALL:
            return adf_fball_load_str(v->f, s, len, NULL, NULL);
        case ADF_DRV_ADELE:
            return adf_adele_load_str(v->a, s, len, NULL, NULL);
        default:
            return adf_cadele_load_str(v->c, s, len, NULL, NULL);
    }
}

/* ---- conversions between the types the driver combines ---- */

/* adf_drv_neg: z = -x.  The negation is exact in every coordinate (policies Theorem 3). */
static void
adf_drv_neg(adf_drv_value * z, const adf_drv_value * x)
{
    z->type = x->type;
    switch (x->type)
    {
        case ADF_DRV_RAT:
            adf_rat_neg(z->r, x->r);
            break;
        case ADF_DRV_FBALL:
            adf_fball_neg(z->f, x->f);
            break;
        case ADF_DRV_ADELE:
            adf_adele_neg(z->a, x->a);
            break;
        case ADF_DRV_CADELE:
            adf_cadele_neg(z->c, x->c);
            break;
        default:
            break;
    }
}

/* z = x + q, q an exact rational.  For a finite ball the exact rational is the ball of
   radius 0 and the sum is the tight sum of precision.md Proposition 1. */
static void
adf_drv_add_rat(adf_drv_value * z, const adf_drv_value * x, const adf_rat_t q, slong prec)
{
    z->type = x->type;
    switch (x->type)
    {
        case ADF_DRV_RAT:
            adf_rat_add(z->r, x->r, q);
            break;
        case ADF_DRV_FBALL:
        {
            adf_fball_t qb;
            adf_fball_init(qb);
            adf_fball_set_rat(qb, q);
            adf_fball_add(z->f, x->f, qb);
            adf_fball_clear(qb);
            break;
        }
        case ADF_DRV_ADELE:
            adf_adele_add_rat(z->a, x->a, q, prec);
            break;
        case ADF_DRV_CADELE:
            adf_cadele_add_rat(z->c, x->c, q, prec);
            break;
        default:
            break;
    }
}

/* adf_drv_rat_sub: z = q - x, the rational on the left.  The sum x + (-q) is exact in
   every coordinate, and so is its negation, so the set is the exact set of the
   differences. */
static void
adf_drv_rat_sub(adf_drv_value * z, const adf_drv_value * x, const adf_rat_t q, slong prec)
{
    adf_rat_t nq;
    adf_drv_value t;

    adf_rat_init(nq);
    adf_rat_neg(nq, q);
    adf_drv_value_init(&t);
    adf_drv_add_rat(&t, x, nq, prec);
    adf_drv_neg(z, &t);
    adf_drv_value_clear(&t);
    adf_rat_clear(nq);
}

/* z = x * q */
static void
adf_drv_mul_rat(adf_drv_value * z, const adf_drv_value * x, const adf_rat_t q, slong prec)
{
    z->type = x->type;
    switch (x->type)
    {
        case ADF_DRV_RAT:
            adf_rat_mul(z->r, x->r, q);
            break;
        case ADF_DRV_FBALL:
            adf_fball_mul_rat(z->f, x->f, q);
            break;
        case ADF_DRV_ADELE:
            adf_adele_mul_rat(z->a, x->a, q, prec);
            break;
        case ADF_DRV_CADELE:
            adf_cadele_mul_rat(z->c, x->c, q, prec);
            break;
        default:
            break;
    }
}

/* ---- the line ---- */

static int
adf_drv_is_blank(char c)
{
    return c == ' ' || c == '\t';
}

/* adf_drv_is_ws(c): the whitespace of the value form (conventions 8.2: the space, TAB, LF
   and CR), which may stand before the first token and after the last one.  The driver splits
   a line at LF, so LF never reaches this predicate. */
static int
adf_drv_is_ws(char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

/* The operands of one line: the word of the operation, and up to four operands.  A fifth
   operand is not looked for; a line with four of them is refused by the arity check, so
   the last operand may hold the separator and no harm is done. */
typedef struct
{
    const char * word;
    size_t wlen;
    const char * s[4];
    size_t n[4];
    int nops;
} adf_drv_line;

/* adf_drv_find(s, len, sep, seplen): the first place where sep occurs in s, or NULL. */
static const char *
adf_drv_find(const char * s, size_t len, const char * sep, size_t seplen)
{
    size_t i;

    if (len < seplen)
        return NULL;
    for (i = 0; i + seplen <= len; i++)
        if (memcmp(s + i, sep, seplen) == 0)
            return s + i;
    return NULL;
}

static void
adf_drv_split(const char * line, size_t len, adf_drv_line * l)
{
    size_t i = 0, start;
    int k = 0;

    while (i < len && adf_drv_is_blank(line[i]))
        i++;
    start = i;
    while (i < len && !adf_drv_is_blank(line[i]))
        i++;
    l->word = line + start;
    l->wlen = i - start;

    l->nops = 0;
    while (k < 4 && i < len)
    {
        const char * sep;

        while (i < len && adf_drv_is_blank(line[i]))
            i++;
        if (i >= len)
            break;
        start = i;
        sep = adf_drv_find(line + i, len - i, ADF_DRV_SEP, ADF_DRV_SEP_LEN);
        l->s[k] = line + start;
        if (sep != NULL)
        {
            l->n[k] = (size_t) (sep - (line + start));
            i = (size_t) (sep - line) + ADF_DRV_SEP_LEN;
        }
        else
        {
            l->n[k] = len - start;
            i = len;
        }
        k++;
    }
    l->nops = k;
}

/* adf_drv_ignorable(line, len): 1 for an empty line and for a line whose first non-blank
   byte is "#".  Only the space and the tab are blanks of the driver: every other byte
   outside the alphabet of conventions 8.2 is passed on to the parsers, which reject it. */
static int
adf_drv_ignorable(const char * line, size_t len)
{
    size_t i = 0;

    while (i < len && adf_drv_is_blank(line[i]))
        i++;
    if (i == len)
        return 1;
    return line[i] == '#';
}

/* adf_drv_setting(s, len, hi, over, out): read the argument of a setting: whitespace, then
   an optional "-", then decimal digits, then nothing, and the value in [1, hi].  The
   whitespace around the number is that of conventions 8.2, so a setting may be written with
   blanks, tabs or a CR around it and a CRLF script runs every setting line; every other byte
   is ADF_PARSE.  A value above hi is the status over, which the caller names: ADF_LIMIT for
   a size bound of an algorithm (prec, README of tools/adf), ADF_DOMAIN when the data
   violate a stated domain (digits, include/adelefeld/text.h:39).  ADF_PARSE when the
   argument is not that shape. */
static int
adf_drv_setting(const char * s, size_t len, slong hi, int over, slong * out)
{
    size_t i = 0;
    slong v = 0;
    int neg = 0;

    while (i < len && adf_drv_is_ws(s[i]))
        i++;
    while (len > i && adf_drv_is_ws(s[len - 1]))
        len--;
    if (i >= len)
        return ADF_PARSE;
    if (s[i] == '-')
    {
        neg = 1;
        i++;
    }
    if (i == len)
        return ADF_PARSE;
    for (; i < len; i++)
    {
        if (s[i] < '0' || s[i] > '9')
            return ADF_PARSE;
        if (v <= ADF_DRV_SET_MAX)
            v = v * 10 + (s[i] - '0');
    }
    /* a signed value: any negative one is below the range [1, hi] */
    if (neg || v < 1)
        return ADF_DOMAIN;
    if (v > hi)
        return over;
    *out = v;
    return ADF_OK;
}

/* ---- the operations ---- */

static int
adf_drv_lookup_op(const char * name, size_t len, adf_drv_op * op)
{
    size_t i;

    for (i = 0; i < adf_drv_op_count; i++)
        if (strlen(adf_drv_ops[i].name) == len
            && memcmp(adf_drv_ops[i].name, name, len) == 0)
        {
            *op = adf_drv_ops[i].op;
            return 1;
        }
    return 0;
}

/* adf_drv_is_ball_like(t): 1 for a type that a set predicate of SPEC 4.2 accepts.  A
   finite ball is one, and so is an exact rational, which is read as the ball of radius 0
   ("Radius zero is handled as a single point"). */
static int
adf_drv_is_ball_like(adf_drv_type t)
{
    return t == ADF_DRV_FBALL || t == ADF_DRV_RAT;
}

/* adf_drv_arith(op, x, y, z, prec): add, sub and mul.  Returns the status; the output is
   untouched on a status other than ADF_OK (conventions 4.3). */
static int
adf_drv_arith(adf_drv_op op, const adf_drv_value * x, const adf_drv_value * y,
              adf_drv_value * z, slong prec)
{
    adf_rat_t nq;
    adf_drv_type t;

    adf_rat_init(nq);

    /* the type of the result: the pair SPEC 4.1 defines */
    if (x->type == y->type)
        t = x->type;
    else if (x->type == ADF_DRV_RAT)
        t = y->type;                   /* the rational meets the inexact value */
    else if (y->type == ADF_DRV_RAT)
        t = x->type;
    else if ((x->type == ADF_DRV_ADELE && y->type == ADF_DRV_CADELE)
             || (x->type == ADF_DRV_CADELE && y->type == ADF_DRV_ADELE))
        t = ADF_DRV_CADELE;            /* the adeles are in C x A_f (SPEC 4.1) */
    else
    {
        adf_rat_clear(nq);
        return ADF_DOMAIN;
    }

    if (x->type == ADF_DRV_RAT)
    {
        /* the exact rational on the left: q - y is the negative of y + (-q) */
        if (op == ADF_DRV_MUL)
            adf_drv_mul_rat(z, y, x->r, prec);
        else if (op == ADF_DRV_ADD)
            adf_drv_add_rat(z, y, x->r, prec);
        else
            adf_drv_rat_sub(z, y, x->r, prec);
        z->type = t;
        adf_rat_clear(nq);
        return ADF_OK;
    }

    if (y->type == ADF_DRV_RAT)
    {
        if (op == ADF_DRV_MUL)
            adf_drv_mul_rat(z, x, y->r, prec);
        else if (op == ADF_DRV_ADD)
            adf_drv_add_rat(z, x, y->r, prec);
        else
        {
            adf_rat_neg(nq, y->r);
            adf_drv_add_rat(z, x, nq, prec);
        }
        z->type = t;
        adf_rat_clear(nq);
        return ADF_OK;
    }

    /* two values of the same type, or an adele and a complex adele: the adele is
       embedded in C x A_f (SPEC 4.1, "The type adf_cadele") */
    if (t == ADF_DRV_CADELE)
    {
        adf_cadele_t cx, cy;

        adf_cadele_init(cx);
        adf_cadele_init(cy);
        if (x->type == ADF_DRV_ADELE)
            adf_cadele_set_adele(cx, x->a);
        else
            adf_cadele_set(cx, x->c);
        if (y->type == ADF_DRV_ADELE)
            adf_cadele_set_adele(cy, y->a);
        else
            adf_cadele_set(cy, y->c);
        switch (op)
        {
            case ADF_DRV_ADD:
                adf_cadele_add(z->c, cx, cy, prec);
                break;
            case ADF_DRV_SUB:
                adf_cadele_sub(z->c, cx, cy, prec);
                break;
            default:
                adf_cadele_mul(z->c, cx, cy, prec);
                break;
        }
        adf_cadele_clear(cx);
        adf_cadele_clear(cy);
    }
    else
    {
        switch (op)
        {
            case ADF_DRV_ADD:
                if (t == ADF_DRV_FBALL)
                    adf_fball_add(z->f, x->f, y->f);
                else
                    adf_adele_add(z->a, x->a, y->a, prec);
                break;
            case ADF_DRV_SUB:
                if (t == ADF_DRV_FBALL)
                    adf_fball_sub(z->f, x->f, y->f);
                else
                    adf_adele_sub(z->a, x->a, y->a, prec);
                break;
            default:
                if (t == ADF_DRV_FBALL)
                    adf_fball_mul(z->f, x->f, y->f);
                else
                    adf_adele_mul(z->a, x->a, y->a, prec);
                break;
        }
    }
    z->type = t;
    adf_rat_clear(nq);
    return ADF_OK;
}

/* adf_drv_div(x, y, z, prec): z = x / y, where y is an exact rational.  Division by an
   adele is not defined (SPEC 4.5), and a finite ball never certifies a divisor, so every
   other second operand is ADF_DOMAIN.  The divisor 0 gives ADF_NOT_UNIT
   (conventions 3.1). */
static int
adf_drv_div(const adf_drv_value * x, const adf_drv_value * y, adf_drv_value * z, slong prec)
{
    if (y->type != ADF_DRV_RAT)
        return ADF_DOMAIN;

    z->type = x->type;
    switch (x->type)
    {
        case ADF_DRV_RAT:
            return adf_rat_div(z->r, x->r, y->r);
        case ADF_DRV_FBALL:
            return adf_fball_div_rat(z->f, x->f, y->r);
        case ADF_DRV_ADELE:
            return adf_adele_div_rat(z->a, x->a, y->r, prec);
        case ADF_DRV_CADELE:
            return adf_cadele_div_rat(z->c, x->c, y->r, prec);
        default:
            return ADF_UNSUPPORTED;
    }
}

/* adf_drv_cap(x, y, z): z = the set of x with its radius replaced by gcd(R, C), C the
   rational y (SPEC 4.4 item 3, policies Definition 13).  The cap is a property of a
   finite ball, so an adele or a complex adele is ADF_UNSUPPORTED and a rational, which
   has no radius of its own to cap, is ADF_DOMAIN. */
static int
adf_drv_cap(const adf_drv_value * x, const adf_drv_value * y, adf_drv_value * z)
{
    if (y->type != ADF_DRV_RAT)
        return ADF_DOMAIN;
    if (x->type == ADF_DRV_ADELE || x->type == ADF_DRV_CADELE)
        return ADF_UNSUPPORTED;
    if (x->type != ADF_DRV_FBALL)
        return ADF_DOMAIN;
    z->type = ADF_DRV_FBALL;
    return adf_fball_cap(z->f, x->f, y->r);
}

/* adf_drv_pred(op, x, y, text): the three set predicates of SPEC 4.2 and the three-valued
   comparison of the same section, written as one of "equal", "different", "undecided".  The
   words of SPEC 4.2 are "certainly equal" (both exact), "certainly different" (the sets are
   disjoint) and "undecided"; the three values are those of adf_fball_compare. */
static int
adf_drv_pred(adf_drv_op op, const adf_drv_value * x, const adf_drv_value * y,
             const char ** text)
{
    adf_fball_t a, b;
    int r;

    if (!adf_drv_is_ball_like(x->type) || !adf_drv_is_ball_like(y->type))
        return ADF_DOMAIN;

    adf_fball_init(a);
    adf_fball_init(b);
    if (x->type == ADF_DRV_FBALL)
        adf_fball_set(a, x->f);
    else
        adf_fball_set_rat(a, x->r);
    if (y->type == ADF_DRV_FBALL)
        adf_fball_set(b, y->f);
    else
        adf_fball_set_rat(b, y->r);

    if (op == ADF_DRV_COMPARE)
    {
        switch (adf_fball_compare(a, b))
        {
            case ADF_CMP_EQUAL:
                *text = "equal";
                break;
            case ADF_CMP_DIFFERENT:
                *text = "different";
                break;
            default:
                *text = "undecided";
                break;
        }
        r = 1;
    }
    else
    {
        if (op == ADF_DRV_EQUAL)
            r = adf_fball_equal_set(a, b);
        else if (op == ADF_DRV_OVERLAPS)
            r = adf_fball_overlaps(a, b);
        else
            r = adf_fball_contains(a, b);
        *text = r ? "true" : "false";
    }
    adf_fball_clear(a);
    adf_fball_clear(b);
    return ADF_OK;
}

/* adf_drv_arity(op): the number of operands the operation takes, 0 for the operations
   that have two arities (reconstruct takes one or three operands). */
static int
adf_drv_arity(adf_drv_op op)
{
    size_t i;

    for (i = 0; i < adf_drv_op_count; i++)
        if (adf_drv_ops[i].op == op)
            return adf_drv_ops[i].arity;
    return -1;
}

/* ---- one command ---- */

/* adf_drv_command(out, op, l, st): run one command and write its line.  Returns ADF_OK
   when a line was written (or a setting was made), else the status to report.  The steps
   are those of the comment at the top of the file and of the README of tools/adf. */
static int
adf_drv_command(FILE * out, adf_drv_op op, const adf_drv_line * l, adf_drv_state * st)
{
    adf_drv_value x, y, w, z;
    adf_text_kind kind[3];
    const char * text;
    int i, status, nops;

    if (op == ADF_DRV_PREC || op == ADF_DRV_DIGITS)
        return (op == ADF_DRV_PREC)
                   ? adf_drv_setting(l->s[0], l->n[0], ADF_DRV_PREC_MAX, ADF_LIMIT, &st->prec)
                   : adf_drv_setting(l->s[0], l->n[0], ADF_DIGITS_MAX, ADF_DOMAIN, &st->digits);

    adf_drv_value_init(&x);
    adf_drv_value_init(&y);
    adf_drv_value_init(&w);
    adf_drv_value_init(&z);
    text = "";
    status = ADF_OK;
    nops = l->nops;           /* 1, 2 or 3: the arity of the operation is checked before */

    if (op == ADF_DRV_TYPE)
    {
        /* "type" asks the classifier alone, which checks the syntax only (conventions 9.7),
           and names all thirteen kinds */
        status = adf_text_classify(&kind[0], l->s[0], l->n[0], NULL);
        if (status == ADF_OK)
        {
            text = adf_drv_kind_names[kind[0]];
            fputs(text, out);
            fputc('\n', out);
        }
        goto done;
    }
    if (op == ADF_DRV_LOAD)
    {
        /* the dump form of conventions 10, not the value form: the loaders read it, and the
           value is printed in the value form */
        status = adf_drv_load(l->s[0], l->n[0], &x);
        if (status == ADF_OK)
            status = adf_drv_value_print(out, &x, st->digits);
        goto done;
    }

    /* step 2: the syntax of every operand, in order (conventions 8.5, stages 1 to 3) */
    for (i = 0; i < nops; i++)
    {
        status = adf_text_classify(&kind[i], l->s[i], l->n[i], NULL);
        if (status != ADF_OK)
            goto done;
    }
    /* step 3: the kind of every operand, in order; a kind with no typed parser in this build
       is ADF_UNSUPPORTED, and no value of the line is read before that is decided */
    for (i = 0; i < nops; i++)
    {
        if (adf_drv_kind_type(kind[i]) == ADF_DRV_OTHER)
        {
            status = ADF_UNSUPPORTED;
            goto done;
        }
    }
    /* step 4: the value of every operand, in order, by the typed parser of its kind */
    for (i = 0; i < nops; i++)
    {
        adf_drv_value * v = (i == 0) ? &x : ((i == 1) ? &y : &w);

        status = adf_drv_value_read(v, kind[i], l->s[i], l->n[i], st->prec);
        if (status != ADF_OK)
            goto done;
    }

    switch (op)
    {
        case ADF_DRV_SHOW:
            status = adf_drv_value_print(out, &x, st->digits);
            break;
        case ADF_DRV_NEG:
            adf_drv_neg(&z, &x);
            z.type = x.type;
            status = adf_drv_value_print(out, &z, st->digits);
            break;
        case ADF_DRV_DUMP:
            status = adf_drv_value_dump(out, &x);
            break;
        case ADF_DRV_ADD:
        case ADF_DRV_SUB:
        case ADF_DRV_MUL:
            status = adf_drv_arith(op, &x, &y, &z, st->prec);
            if (status == ADF_OK)
                status = adf_drv_value_print(out, &z, st->digits);
            break;
        case ADF_DRV_DIV:
            status = adf_drv_div(&x, &y, &z, st->prec);
            if (status == ADF_OK)
                status = adf_drv_value_print(out, &z, st->digits);
            break;
        case ADF_DRV_CAP:
            status = adf_drv_cap(&x, &y, &z);
            if (status == ADF_OK)
                status = adf_drv_value_print(out, &z, st->digits);
            break;
        case ADF_DRV_EQUAL:
        case ADF_DRV_CONTAINS:
        case ADF_DRV_OVERLAPS:
        case ADF_DRV_COMPARE:
            status = adf_drv_pred(op, &x, &y, &text);
            if (status == ADF_OK)
            {
                fputs(text, out);
                fputc('\n', out);
            }
            break;
        case ADF_DRV_RECONSTRUCT:
            if (nops == 1)
            {
                /* the full ball: the interval is the one of the real coordinate.  A
                   complex adele is a request version 1 does not implement (the library
                   declares reconstruction for adf_adele and adf_fball only); a finite
                   ball or a rational without an interval is not a well-formed problem
                   for this command. */
                if (x.type == ADF_DRV_ADELE)
                    status = adf_adele_reconstruct(z.r, x.a);
                else if (x.type == ADF_DRV_CADELE)
                    status = ADF_UNSUPPORTED;
                else
                    status = ADF_DOMAIN;
            }
            else
            {
                /* a finite ball and the closed interval [lo, hi] of two exact rationals */
                if (x.type != ADF_DRV_FBALL || y.type != ADF_DRV_RAT || w.type != ADF_DRV_RAT)
                {
                    status = ADF_DOMAIN;
                    break;
                }
                status = adf_fball_reconstruct(z.r, x.f, y.r, w.r);
            }
            if (status == ADF_OK)
            {
                z.type = ADF_DRV_RAT;
                status = adf_drv_value_print(out, &z, st->digits);
            }
            break;
        default:
            status = ADF_PARSE;
            break;
    }

done:
    adf_drv_value_clear(&x);
    adf_drv_value_clear(&y);
    adf_drv_value_clear(&w);
    adf_drv_value_clear(&z);
    return status;
}

/* ---- the script ---- */

/* adf_emit_status(out, status): the line of a command that did not succeed. */
static void
adf_emit_status(FILE * out, int status)
{
    fputs("error: ", out);
    fputs(adf_status_str(status), out);
    fputc('\n', out);
}

static int
adf_driver_line(FILE * out, const char * line, size_t len, adf_drv_state * st, slong lineno,
                int verbose)
{
    adf_drv_line l;
    adf_drv_op op;
    int status, arity;

    adf_drv_split(line, len, &l);
    if (l.wlen == 0 || !adf_drv_lookup_op(l.word, l.wlen, &op))
    {
        status = ADF_PARSE;
    }
    else
    {
        arity = adf_drv_arity(op);
        if (arity == 0)
            status = (l.nops == 1 || l.nops == 3) ? ADF_OK : ADF_PARSE;
        else
            status = (l.nops == arity) ? ADF_OK : ADF_PARSE;
        if (status == ADF_OK)
            status = adf_drv_command(out, op, &l, st);
    }

    if (status != ADF_OK)
        adf_emit_status(out, status);
    if (verbose)
        fprintf(stderr, "adf: line %ld: %.*s: %s\n", (long) lineno, (int) l.wlen, l.word,
                adf_status_str(status));
    return status;
}

int
adf_driver_run_v(const char * text, size_t len, FILE * out, int verbose)
{
    adf_drv_state st;
    size_t pos = 0;
    slong lineno = 0;
    int failed = 0;

    st.prec = ADF_DRV_PREC_DEFAULT;
    st.digits = ADF_DIGITS_DEFAULT;

    while (pos < len)
    {
        size_t start = pos, n;
        int status;

        while (pos < len && text[pos] != '\n')
            pos++;
        n = pos - start;
        if (pos < len)
            pos++;
        lineno++;

        if (n > ADF_DRV_MAX_LINE)
        {
            /* the line is one byte too long or more: the rest of it is skipped */
            status = ADF_LIMIT;
            adf_emit_status(out, ADF_LIMIT);
            failed = 1;
            if (verbose)
                fprintf(stderr, "adf: line %ld: %lu bytes: LIMIT\n", (long) lineno,
                        (unsigned long) n);
        }
        else if (!adf_drv_ignorable(text + start, n))
        {
            status = adf_driver_line(out, text + start, n, &st, lineno, verbose);
            if (status != ADF_OK)
                failed = 1;
        }
        if (ferror(out))
            return 2;
    }
    if (ferror(out))
        return 2;
    return failed;
}

int
adf_driver_run(const char * text, size_t len, FILE * out)
{
    return adf_driver_run_v(text, len, out, 0);
}

#ifndef ADF_DRIVER_NO_MAIN

/* ---- the program ---- */

static int
adf_usage(const char * msg)
{
    fprintf(stderr, "adf: %s\n", msg);
    fprintf(stderr, "usage: adf [-v] [file]\n");
    fprintf(stderr, "  reads a script of commands from the file or from standard input\n");
    return 2;
}

int
main(int argc, char ** argv)
{
    const char * path = NULL;
    FILE * in = stdin;
    char * buf = NULL;
    size_t cap = 0, len = 0;
    int verbose = 0, rc, i;

    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-v") == 0)
            verbose = 1;
        else if (argv[i][0] == '-' && argv[i][1] != '\0')
            return adf_usage("unknown option");
        else if (path != NULL)
            return adf_usage("at most one file may be named");
        else
            path = argv[i];
    }

    if (path != NULL)
    {
        in = fopen(path, "rb");
        if (in == NULL)
        {
            fprintf(stderr, "adf: cannot open %s\n", path);
            return 2;
        }
    }

    /* the whole script is read first: the run of a script is a function of its bytes */
    for (;;)
    {
        size_t got;

        if (len == cap)
        {
            char * nb;
            size_t ncap = (cap == 0) ? 65536 : 2 * cap;

            nb = (char *) realloc(buf, ncap);
            if (nb == NULL)
            {
                fprintf(stderr, "adf: out of memory after %lu bytes\n", (unsigned long) len);
                free(buf);
                if (in != stdin)
                    fclose(in);
                return 2;
            }
            buf = nb;
            cap = ncap;
        }
        got = fread(buf + len, 1, cap - len, in);
        len += got;
        if (got == 0)
            break;
    }
    if (ferror(in))
    {
        fprintf(stderr, "adf: %s: read error\n", (path != NULL) ? path : "standard input");
        free(buf);
        if (in != stdin)
            fclose(in);
        return 2;
    }
    if (in != stdin)
        fclose(in);

    rc = adf_driver_run_v((buf != NULL) ? buf : "", len, stdout, verbose);
    free(buf);
    if (fflush(stdout) != 0 || ferror(stdout))
        rc = 2;
    return rc;
}

#endif /* ADF_DRIVER_NO_MAIN */
