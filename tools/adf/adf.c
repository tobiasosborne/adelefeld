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
   cap, dump, load, roots, realroots and recover; reconstruct takes either one operand (an adele)
   or three (a finite
   ball and an interval given as two exact rationals), which is the one documented extension
   of the two-operand form.  dump writes the dump form of conventions 10.1 of a value, and
   load reads such a text; the driver has no context, so a dump with a context occurrence is
   not read.

   The commands of the solvers of milestone S (roots, realroots and recover) read operands that
   the value form does not have in every position: the polynomial is a list of its integer
   coefficients in decimal, separated by single spaces, the constant term first, and the bounds of
   the reconstruction are exact rationals that must be integers.  They are read apart from the
   value form, by the section "the solver commands" below, and they take the place of the value
   text in the same five steps, with the polynomial read in steps 2 to 4 and the domain of the
   operation (the zero polynomial, an integer that is not a prime of a place, a precision below 1)
   in step 5.  The grammar of the line is unchanged: one operation name, then one to three
   operands separated by " with ".

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
/* The depth limit of the search of adf_roots_padic that the command "roots" runs.  A fixed
   choice of the driver, not a bound of the library: Algorithm P opens its classes at the levels 0
   to depth (include/adelefeld/roots.h, adf_roots_padic_partial), and a larger depth resolves
   more classes.  With a normalised polynomial g*, every root of g* is simple (L3.1 (2)), so a
   large enough depth completes every list. */
#define ADF_DRV_ROOTS_DEPTH ((slong) 64)
/* The search limit of adf_resid_reconstruct that the command "recover" passes: the number of
   rounds of Algorithm R (docs/proofs/solvers.md:266).  A fixed choice of the driver, and not a
   bound: with a limit of 0 the function answers NOT_DETERMINED on every problem in the range
   A < m <= 2 A B with |T| <= B, that is, whenever it cannot decide without a search. */
#define ADF_DRV_RECON_LIMIT ((slong) 1000)

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
    ADF_DRV_ROOTS,
    ADF_DRV_REALROOTS,
    ADF_DRV_RECOVER,
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
    { "roots", ADF_DRV_ROOTS, 3 },
    { "realroots", ADF_DRV_REALROOTS, 1 },
    { "recover", ADF_DRV_RECOVER, 3 },
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

/* ---- the solver commands (milestone S) ---- */

/* The three commands of the solvers read operands that the value form of conventions 9.2 does
   not have in every position, so they have a reading of their own, apart from the value form:

     roots POLY with P with K    the roots of POLY in Z_P, through adf_roots_padic with the depth
                                 ADF_DRV_ROOTS_DEPTH.  The output is the roots in the order of the
                                 list, separated by "; ", each as "A mod P^K" with A the centre in
                                 [0, P^K) and K the precision of the certificate of that root
                                 (roots.h, D3.2), and "none" for a complete list without a root
     realroots POLY             the real roots of POLY, through adf_roots_real at the setting
                                 prec.  The output is the balls in the order of the list, separated
                                 by "; ", each the text of a real ball with the setting digits, and
                                 "none" for a list without a root
     recover C mod M with A with B
                                 the rational n/d of the residue class P(M, C) in the box
                                 |n| <= A, 0 < d <= B, through adf_resid_reconstruct with the limit
                                 ADF_DRV_RECON_LIMIT.  The output is n/d as the driver prints a
                                 rational

   POLY is the list of the integer coefficients in decimal, separated by single spaces, the
   constant term first, at least one coefficient: "-2 0 1" is X^2 - 2.  P, K, A and B are exact
   rationals of the value form, read by adf_rat_set_str, and they must be integers.

   The steps are those of the command at the top of the file, with the polynomial in the place of
   the first operand and the domain of the operation (the zero polynomial, a value of P that is
   not a prime of a place, a precision below 1, a bound that is not an integer) in step 5.  A
   polynomial that is not of that form is ADF_PARSE, the status the driver gives a text it cannot
   read (conventions 8.5, stage 1). */

/* adf_drv_poly_syntax(f, s, len): the polynomial operand, step 2.  f is written on ADF_OK and
   may be partly written on ADF_PARSE, so the caller clears it either way.  fmpz_set_str reads a
   null-terminated string (refs/src/flint-3.0.1/fmpz.rst:427 to 431), so every token is copied
   into a buffer of its own; a buffer on the stack serves the tokens that fit, and the rest is
   allocated with flint_malloc, as fmpz_get_str allocates.  The sign is read here and not left to
   fmpz_set_str, whose treatment of a leading "-" is not in refs/:
   [source pending: FLINT 3.0.1, fmpz_set_str and a leading sign].
   The number of coefficients is at most half the length of a line, which the driver bounds by
   ADF_DRV_MAX_LINE, so the index of a coefficient is a slong. */
static int
adf_drv_poly_syntax(fmpz_poly_t f, const char * s, size_t len)
{
    char small[32], * buf;
    fmpz_t t;
    size_t i = 0, start, digits, need;
    slong k = 0;
    int neg;

    if (len == 0)
        return ADF_PARSE;
    fmpz_init(t);
    while (i < len)
    {
        start = i;
        neg = (s[i] == '-');
        if (neg)
            i++;
        if (i >= len || s[i] < '0' || s[i] > '9')
            goto parse;
        while (i < len && s[i] >= '0' && s[i] <= '9')
            i++;
        digits = i - start - (size_t) neg;
        need = digits + 1;
        buf = (need <= sizeof(small)) ? small : flint_malloc(need);
        memcpy(buf, s + start + (size_t) neg, digits);
        buf[digits] = '\0';
        if (fmpz_set_str(t, buf, 10) != 0)
        {
            if (buf != small)
                flint_free(buf);
            goto parse;
        }
        if (buf != small)
            flint_free(buf);
        if (neg)
            fmpz_neg(t, t);
        /* a zero coefficient is not written, so the polynomial keeps the length of its degree and
           the zero polynomial stays the empty one */
        if (!fmpz_is_zero(t))
            fmpz_poly_set_coeff_fmpz(f, k, t);
        k++;
        if (i == len)
            break;
        if (s[i] != ' ')
            goto parse;           /* the separator of two coefficients is a single space */
        i++;
        if (i == len)
            goto parse;           /* and nothing follows the last one */
    }
    fmpz_clear(t);
    return ADF_OK;

parse:
    fmpz_clear(t);
    return ADF_PARSE;
}

/* adf_drv_poly_value(f): the zero polynomial is outside the domain of every function of
   include/adelefeld/roots.h ("Statuses", edit E-C1), so it is ADF_DOMAIN, the status of data
   outside a stated domain (conventions 3.1). */
static int
adf_drv_poly_value(const fmpz_poly_t f)
{
    return fmpz_poly_is_zero(f) ? ADF_DOMAIN : ADF_OK;
}

/* adf_drv_int_value(z, v): the integer of an exact rational, step 5.  P, K, A and B are integers
   (SPEC 9.1 and 9.2), so a value of another type, or a rational whose denominator is not 1, is
   data outside the domain of the operation and ADF_DOMAIN.  z is written only on ADF_OK. */
static int
adf_drv_int_value(fmpz_t z, const adf_drv_value * v)
{
    fmpq_t u;

    if (v->type != ADF_DRV_RAT)
        return ADF_DOMAIN;
    fmpq_init(u);
    adf_rat_get_fmpq(u, v->r);
    if (fmpz_is_one(fmpq_denref(u)))
    {
        fmpz_set(z, fmpq_numref(u));
        fmpq_clear(u);
        return ADF_OK;
    }
    fmpq_clear(u);
    return ADF_DOMAIN;
}

/* adf_drv_place_operand(v, z): the place of the prime z.  A place holds a prime of one word, that
   is below 2^64 (conventions 7, include/adelefeld/place.h:38 to 40), so a value below 2, a
   negative value and a value that does not fit in a word are all outside the domain of the
   command: ADF_DOMAIN.  The primality is certified by adf_place_prime itself, whose status for a
   composite is ADF_DOMAIN (place.h:34 to 37). */
static int
adf_drv_place_operand(adf_place_t * v, const fmpz_t z)
{
    if (fmpz_sgn(z) < 0 || fmpz_cmp_ui(z, 1) <= 0)
        return ADF_DOMAIN;
    if (fmpz_cmp_ui(z, UWORD_MAX) > 0)
        return ADF_DOMAIN;
    return adf_place_prime(v, fmpz_get_ui(z));
}

/* adf_drv_prec_operand(out, z): the precision prec_p of the roots at a prime, which is a slong of
   the interface (include/adelefeld/roots.h, adf_roots_padic).  A negative value is below 1, and a
   value below 1 is outside the domain of the function (roots.h, "Statuses"): ADF_DOMAIN.  A
   value that does not fit in a slong is a size bound of the driver, and ADF_LIMIT is the status
   of a size bound (conventions 3.1), the status of a prec setting above ADF_PRINT_EXP_MAX as
   well. */
static int
adf_drv_prec_operand(slong * out, const fmpz_t z)
{
    slong v;

    if (fmpz_sgn(z) < 0)
        return ADF_DOMAIN;
    if (fmpz_cmp_si(z, WORD_MAX) > 0)
        return ADF_LIMIT;
    v = fmpz_get_si(z);
    if (v < 1)
        return ADF_DOMAIN;
    *out = v;
    return ADF_OK;
}

/* adf_drv_put_roots(out, L, p): the line of a list of roots at the prime p.  Every entry is
   "A mod P^K" with the centre and the precision of its certificate, in the order of the list
   (include/adelefeld/roots.h, D3.2), and an empty list is the word none: an empty complete list
   is the answer of the function, not a failure (roots.h, adf_roots_padic).  The list is read
   through its accessors only (decision S-D12), and a list at a prime holds a certificate for each
   of its n entries, so adf_rootlist_get_cert writes for every index in [0, n). */
static int
adf_drv_put_roots(FILE * out, const adf_rootlist_t L, ulong p)
{
    fmpz_t a;
    slong i, n = adf_rootlist_length(L), K, s;
    char * t;

    if (n == 0)
    {
        fputs("none", out);
        fputc('\n', out);
        return ADF_OK;
    }
    if (adf_place_is_archimedean(adf_rootlist_place(L)))
        return ADF_LIMIT;        /* a list at the real place has no centre: a defect of the library */
    fmpz_init(a);
    for (i = 0; i < n; i++)
    {
        if (i > 0)
            fputs("; ", out);
        (void) adf_rootlist_get_cert(a, &K, &s, L, i);
        t = fmpz_get_str(NULL, 10, a);
        fputs(t, out);
        flint_fprintf(out, " mod %wu^%wd", p, K);
        flint_free(t);
    }
    fmpz_clear(a);
    fputc('\n', out);
    return ADF_OK;
}

/* adf_drv_arb_str(x, digits): the text of the real ball x, or NULL when the printer of the
   library refuses it.  The library prints a real ball only as the real part of an adf_adele or of
   an adf_cadele (include/adelefeld/text.h:160 to 175); there is no printer of an arb alone.  So
   the ball is put in the real coordinate of an adele whose finite part is the init value, the
   adele is printed by adf_adele_get_str with n = digits, and the text between the "(" and the
   " ; " is returned: the finite part follows the real part after " ; ", and the real text of
   conventions 9.5 is a decimal or "m +/- r", which holds no " ; ".  A text of another shape is
   copied whole, so that no byte of the printer is lost.  The string is a copy of the driver's own,
   allocated with flint_malloc and released with flint_free.  A NULL is the guard of decision
   M1-D6 (text.h:32 to 38) and the driver answers ADF_LIMIT for it, as for every printer. */
static char *
adf_drv_arb_str(const arb_t x, slong digits)
{
    adf_adele_t a;
    adf_fball_t f;
    char * s, * sep, * t, * u;
    size_t len, k;

    adf_fball_init(f);
    adf_adele_init(a);
    if (adf_adele_set_arb_fball(a, x, f) != ADF_OK)
    {
        /* an infinite real ball, which no list of the library holds; the printer would refuse it
           as well, so the answer is the same NULL */
        adf_fball_clear(f);
        adf_adele_clear(a);
        return NULL;
    }
    adf_fball_clear(f);
    s = adf_adele_get_str(&len, a, digits);
    adf_adele_clear(a);
    if (s == NULL)
        return NULL;
    sep = strstr(s, " ; ");
    if (sep != NULL && sep > s)
    {
        u = s + 1;
        k = (size_t) (sep - s) - 1;
    }
    else
    {
        u = s;
        k = len;
    }
    t = flint_malloc(k + 1);
    memcpy(t, u, k);
    t[k] = '\0';
    adf_str_free(s);
    return t;
}

/* adf_drv_put_reals(out, L, digits): the line of a list of real roots: the balls in the order of
   the list, separated by "; ", each the text of conventions 9.5 with n = digits, and "none" for a
   list without a root.  Every text is formed before any byte is written, so that a printer that
   refuses one of them gives ADF_LIMIT and the line of the error, not half a line of roots. */
static int
adf_drv_put_reals(FILE * out, const adf_rootlist_t L, slong digits)
{
    arb_struct x[1];
    char ** t;
    slong i, n = adf_rootlist_length(L);
    int status = ADF_OK;

    if (n == 0)
    {
        fputs("none", out);
        fputc('\n', out);
        return ADF_OK;
    }
    if (!adf_place_is_archimedean(adf_rootlist_place(L)))
        return ADF_LIMIT;        /* only a list at the real place has balls: a defect of the library */
    t = flint_calloc(n, sizeof(char *));
    arb_init(x);
    for (i = 0; i < n; i++)
    {
        /* a list at the real place holds a ball for each of its n entries (roots.h, P3.10) */
        (void) adf_rootlist_get_arb(x, L, i);
        t[i] = adf_drv_arb_str(x, digits);
        if (t[i] == NULL)
        {
            status = ADF_LIMIT;
            break;
        }
    }
    arb_clear(x);
    for (i = 0; i < n; i++)
    {
        if (status == ADF_OK)
        {
            if (i > 0)
                fputs("; ", out);
            fputs(t[i], out);
        }
        flint_free(t[i]);         /* flint_free(NULL) does nothing */
    }
    flint_free(t);
    if (status == ADF_OK)
        fputc('\n', out);
    return status;
}

/* adf_drv_solve_roots(out, f, vp, vk): the operation of the command "roots".  f is the
   polynomial, vp and vk the values of the two other operands, already read.  The order is that
   of the operands: the zero polynomial, then the integers P and K, then the place and the
   precision, then the function, which decides the rest (roots.h, "Statuses"). */
static int
adf_drv_solve_roots(FILE * out, const fmpz_poly_t f, const adf_drv_value * vp,
                    const adf_drv_value * vk)
{
    adf_rootlist_t L;
    adf_place_t pl;
    fmpz_t p, k;
    slong K;
    int status;

    status = adf_drv_poly_value(f);
    if (status != ADF_OK)
        return status;
    fmpz_init(p);
    fmpz_init(k);
    status = adf_drv_int_value(p, vp);
    if (status == ADF_OK)
        status = adf_drv_int_value(k, vk);
    if (status == ADF_OK)
        status = adf_drv_place_operand(&pl, p);
    if (status == ADF_OK)
        status = adf_drv_prec_operand(&K, k);
    if (status == ADF_OK)
    {
        adf_rootlist_init(L);
        status = adf_roots_padic(L, f, pl, K, ADF_DRV_ROOTS_DEPTH);
        if (status == ADF_OK)
            status = adf_drv_put_roots(out, L, adf_place_prime_get(pl));
        adf_rootlist_clear(L);
    }
    fmpz_clear(p);
    fmpz_clear(k);
    return status;
}

/* adf_drv_solve_realroots(out, f, prec, digits): the operation of the command "realroots": the
   zero polynomial first, then the function at the setting prec, which is its own precision
   argument.  A prec above ADF_ROOTS_REAL_PREC_MAX would be ADF_LIMIT from the function; the
   setting prec is at most ADF_PRINT_EXP_MAX, which is below it, so that status cannot be reached
   from this command. */
static int
adf_drv_solve_realroots(FILE * out, const fmpz_poly_t f, slong prec, slong digits)
{
    adf_rootlist_t L;
    int status;

    status = adf_drv_poly_value(f);
    if (status != ADF_OK)
        return status;
    adf_rootlist_init(L);
    status = adf_roots_real(L, f, prec);
    if (status == ADF_OK)
        status = adf_drv_put_reals(out, L, digits);
    adf_rootlist_clear(L);
    return status;
}

/* adf_drv_solve_recover(out, v0, v1, v2, digits): the operation of the command "recover".  The
   first operand is the finite ball whose residue class is wanted, so it must be a finite ball:
   an adele or a complex adele is ADF_UNSUPPORTED and a rational is ADF_DOMAIN, as for the
   command cap (tools/adf/README.md, "Types of the operands").  Then the two bounds as integers,
   then the passage from the ball to the residue class, adf_resid_set_fball_forget, whose ADF_DOMAIN
   is that of an exact ball (a single rational is in no residue class with m > 1) and of a
   denominator not coprime to the radius, and the reconstruction itself. */
static int
adf_drv_solve_recover(FILE * out, const adf_drv_value * v0, const adf_drv_value * v1,
                      const adf_drv_value * v2, slong digits)
{
    adf_resid_t x;
    adf_rat_t q;
    adf_recon_cert_t cert;
    adf_drv_value z;
    fmpz_t A, B;
    int status;

    if (v0->type == ADF_DRV_ADELE || v0->type == ADF_DRV_CADELE)
        return ADF_UNSUPPORTED;
    if (v0->type != ADF_DRV_FBALL)
        return ADF_DOMAIN;
    fmpz_init(A);
    fmpz_init(B);
    status = adf_drv_int_value(A, v1);
    if (status == ADF_OK)
        status = adf_drv_int_value(B, v2);
    if (status != ADF_OK)
    {
        fmpz_clear(A);
        fmpz_clear(B);
        return status;
    }
    adf_resid_init(x);
    status = adf_resid_set_fball_forget(x, v0->f);
    if (status == ADF_OK)
    {
        adf_rat_init(q);
        adf_recon_cert_init(cert);
        status = adf_resid_reconstruct(q, cert, x, A, B, ADF_DRV_RECON_LIMIT);
        if (status == ADF_OK)
        {
            adf_drv_value_init(&z);
            z.type = ADF_DRV_RAT;
            adf_rat_set(z.r, q);
            status = adf_drv_value_print(out, &z, digits);
            adf_drv_value_clear(&z);
        }
        adf_recon_cert_clear(cert);
        adf_rat_clear(q);
    }
    adf_resid_clear(x);
    fmpz_clear(A);
    fmpz_clear(B);
    return status;
}

/* adf_drv_solver(out, op, l, st): the three commands of the solvers, with the steps 2 to 5 of the
   command at the top of this file.  The polynomial of "roots" and "realroots" is the first
   operand and is read by adf_drv_poly_syntax in step 2; the other operands, and all three of
   "recover", are read by adf_text_classify and by the typed parser of their kind as for every
   other command. */
static int
adf_drv_solver(FILE * out, adf_drv_op op, const adf_drv_line * l, adf_drv_state * st)
{
    adf_drv_value v[3];
    adf_text_kind kind[3];
    fmpz_poly_t f;
    int status, i, nops, first, poly_first;

    poly_first = (op != ADF_DRV_RECOVER);
    nops = (op == ADF_DRV_REALROOTS) ? 1 : 3;
    first = poly_first ? 1 : 0;    /* the index of the first operand that is a value text */
    for (i = 0; i < 3; i++)
    {
        adf_drv_value_init(&v[i]);
        kind[i] = ADF_TEXT_RAT;
    }
    fmpz_poly_init(f);
    status = ADF_OK;

    /* step 2: the syntax of every operand, in order */
    if (poly_first)
    {
        status = adf_drv_poly_syntax(f, l->s[0], l->n[0]);
        if (status != ADF_OK)
            goto done;
    }
    for (i = first; i < nops; i++)
    {
        status = adf_text_classify(&kind[i], l->s[i], l->n[i], NULL);
        if (status != ADF_OK)
            goto done;
    }
    /* step 3: the kind of every operand, in order */
    for (i = first; i < nops; i++)
    {
        if (adf_drv_kind_type(kind[i]) == ADF_DRV_OTHER)
        {
            status = ADF_UNSUPPORTED;
            goto done;
        }
    }
    /* step 4: the value of every operand, in order */
    for (i = first; i < nops; i++)
    {
        status = adf_drv_value_read(&v[i], kind[i], l->s[i], l->n[i], st->prec);
        if (status != ADF_OK)
            goto done;
    }
    /* step 5: the operation, and step 6: the printer, inside it */
    if (op == ADF_DRV_ROOTS)
        status = adf_drv_solve_roots(out, f, &v[1], &v[2]);
    else if (op == ADF_DRV_REALROOTS)
        status = adf_drv_solve_realroots(out, f, st->prec, st->digits);
    else
        status = adf_drv_solve_recover(out, &v[0], &v[1], &v[2], st->digits);

done:
    fmpz_poly_clear(f);
    for (i = 0; i < 3; i++)
        adf_drv_value_clear(&v[i]);
    return status;
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
    if (op == ADF_DRV_ROOTS || op == ADF_DRV_REALROOTS || op == ADF_DRV_RECOVER)
        return adf_drv_solver(out, op, l, st);

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
