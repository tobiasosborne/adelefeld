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
   cap, dump, load, roots, realroots, recover, project, exp_at and log_at (with the other functions at places, among
   them powrat_at and powunit_at of lane f-slice9), and, for the unit coset, the idele and
   the idele class (lane t-slice1, milestone 2), inv, pow, powtight, norm, class, idele, hull, hullsimple, unitof,
   valuation and abs (the section "the unit coset, the idele and the idele class" below), and root, the root at all
   places of a rational, an adele or an idele (lane f-slice10; adf_drv_root_all below), which takes two or three
   operands, and exp, sin, sinh, cos and cosh at all places of an adele (adf_drv_series_all below);
   reconstruct takes either one operand (an adele)
   or three (a finite
   ball and an interval given as two exact rationals), which is the one documented extension
   of the two-operand form.  dump writes the dump form of conventions 10.1 of a value, and
   load reads such a text; the driver has no context, so a dump with a context occurrence is
   not read.

   The commands of the functions at places (milestone 1F: project, exp_at and log_at) read a list of
   places as their second operand, which the value form does not have either; see the section "the
   commands at places" below and tools/adf/README.md.

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
   embedding of SPEC 4.1, "The type adf_cadele"), and the pairs of the unit coset, the idele and the class listed
   in the section named above.  Every other pair is ADF_DOMAIN.  A kind of the value form with no typed
   parser in this build (the functions and the character, work packages 1.8 and
   later) is ADF_UNSUPPORTED.  The local ball and the partial ball have typed parsers and printers
   since lane drv-ball (adf_lball_set_str, adf_sball_set_str and their printers); they are values the
   driver holds, `show` prints them, every pair with another type is ADF_DOMAIN and their negation is
   ADF_UNSUPPORTED. Quotient lifts are held and printed in slice 3.1-a; qadd_rat preserves
   their representation. The ordinary pair commands on quotient classes are ADF_DOMAIN.

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

#include <limits.h>
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

/* The arity of an operation that takes two or three operands (root X with N [with SIGN]). */
#define ADF_DRV_ARITY_2_OR_3 (-2)

typedef enum
{
    ADF_DRV_SHOW = 0,
    ADF_DRV_FFUN_SHOW,
    ADF_DRV_FFUN_ADD,
    ADF_DRV_FFUN_FOURIER,
    ADF_DRV_FFUN_MUL,
    ADF_DRV_FFUN_TRANSLATE,
    ADF_DRV_FFUN_DILATE,
    ADF_DRV_FFUN_DILATE_IDELE,
    ADF_DRV_FFUN_REFLECT,
    ADF_DRV_FFUN_CONJ,
    ADF_DRV_TYPE,
    ADF_DRV_QADD_RAT,
    ADF_DRV_QREDUCE,
    ADF_DRV_QEQUAL,
    ADF_DRV_QCONTAINS,
    ADF_DRV_QOVERLAPS,
    ADF_DRV_QNEG,
    ADF_DRV_QADD,
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
    ADF_DRV_PROJECT,
    ADF_DRV_EXP_AT,
    ADF_DRV_LOG_AT,
    ADF_DRV_SIN_AT,
    ADF_DRV_COS_AT,
    ADF_DRV_SINH_AT,
    ADF_DRV_COSH_AT,
    ADF_DRV_ROOT_AT,
    ADF_DRV_ROOTS_AT,
    ADF_DRV_POWRAT_AT,
    ADF_DRV_POWUNIT_AT,
    ADF_DRV_INV,
    ADF_DRV_POW,
    ADF_DRV_POWTIGHT,
    ADF_DRV_NORM,
    ADF_DRV_CLASS,
    ADF_DRV_MKIDELE,
    ADF_DRV_HULL,
    ADF_DRV_HULLSIMPLE,
    ADF_DRV_UNITOF,
    ADF_DRV_VALUATION,
    ADF_DRV_ABS,
    ADF_DRV_ROOT,              /* lane f-slice10 (WP 1F.8): the root at all places */
    ADF_DRV_EXP,               /* lane f-slice10: exp, sin, sinh, cos, cosh at all places */
    ADF_DRV_SIN,
    ADF_DRV_SINH,
    ADF_DRV_COS,
    ADF_DRV_COSH,
    ADF_DRV_IDLOG,
    ADF_DRV_IDLOGABS,
    ADF_DRV_IDLOG_AT,
    ADF_DRV_IDLOGABS_AT,
    ADF_DRV_IDLOG_REFINE,
    ADF_DRV_IDLOGABS_REFINE,
    ADF_DRV_LEGENDRE,
    ADF_DRV_JACOBI,
    ADF_DRV_KRONECKER,
    ADF_DRV_HILBERT_AT,
    ADF_DRV_BINOM,
    ADF_DRV_BINOMTIGHT,
    ADF_DRV_PROFPOW,
    ADF_DRV_PROFPOWCOARSE,
    ADF_DRV_PROFPOWFINE,
    ADF_DRV_HAAR_VOLUME,
    ADF_DRV_CYCLO_EXP_U,
    ADF_DRV_CYCLO_EXP_UINV,
    ADF_DRV_LOCAL_ZETA_AT,
    ADF_DRV_PSI,
    ADF_DRV_PSI_STRICT,
    ADF_DRV_PSI_AT,
    ADF_DRV_PSI_STRICT_AT,
    ADF_DRV_PSI_PHASE,
    ADF_DRV_CHAR,
    ADF_DRV_CHI,
    ADF_DRV_GAUSS,

    ADF_DRV_RFUN,              /* slice 4d (lane f4-slice2): real test functions, adf_drv_rfun below */
    ADF_DRV_RFUN_TRANSLATE,
    ADF_DRV_RFUN_MUL,
    ADF_DRV_RFUN_EVAL,
    ADF_DRV_RFUN_FOURIER,      /* slice 4e (lane f4-slice3): transform, derivative, integral, norm2 */
    ADF_DRV_RFUN_DERIVATIVE,
    ADF_DRV_RFUN_INTEGRAL,
    ADF_DRV_RFUN_NORM2,

    ADF_DRV_CHAR_CONJ,
    ADF_DRV_CHAR_UNIT,
    ADF_DRV_CHAR_UNIT_STRICT,

    ADF_DRV_TENSOR_EVAL,       /* slice 4f (lane f4-slice5): evaluation and additive integrals, adf_drv_tensor */
    ADF_DRV_TENSOR_EVAL_SBALL,
    ADF_DRV_FFUN_EVAL,
    ADF_DRV_FFUN_INTEGRAL,
    ADF_DRV_FFUN_NORM2,
    ADF_DRV_TENSOR_INTEGRAL,
    ADF_DRV_TENSOR_NORM2,
    ADF_DRV_PREC,
    ADF_DRV_DIGITS
} adf_drv_op;

static const struct
{
    const char * name;
    adf_drv_op op;
    int arity;                 /* 0: the operation has two arities (reconstruct, one or three operands);
                                  ADF_DRV_ARITY_2_OR_3: two or three operands (root) */
} adf_drv_ops[] = {
    { "show", ADF_DRV_SHOW, 1 },
    { "ffun", ADF_DRV_FFUN_SHOW, 1 },
    { "ffun_add", ADF_DRV_FFUN_ADD, 2 },
    { "ffun_fourier", ADF_DRV_FFUN_FOURIER, 1 },
    { "ffun_mul", ADF_DRV_FFUN_MUL, 2 },
    { "ffun_translate", ADF_DRV_FFUN_TRANSLATE, 2 },
    { "ffun_dilate", ADF_DRV_FFUN_DILATE, 2 },
    { "ffun_dilate_idele", ADF_DRV_FFUN_DILATE_IDELE, 2 },
    { "ffun_reflect", ADF_DRV_FFUN_REFLECT, 1 },
    { "ffun_conj", ADF_DRV_FFUN_CONJ, 1 },
    { "print", ADF_DRV_SHOW, 1 }, /* Slice 3.1-d spelling, api-3.md:848. */
    { "type", ADF_DRV_TYPE, 1 },
    { "qadd_rat", ADF_DRV_QADD_RAT, 2 },
    { "qreduce", ADF_DRV_QREDUCE, 2 },
    { "qequal", ADF_DRV_QEQUAL, 3 },
    { "qcontains", ADF_DRV_QCONTAINS, 3 },
    { "qoverlaps", ADF_DRV_QOVERLAPS, 3 },
    { "qneg", ADF_DRV_QNEG, 2 },
    { "qadd", ADF_DRV_QADD, 3 },
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
    { "project", ADF_DRV_PROJECT, 2 },
    { "exp_at", ADF_DRV_EXP_AT, 2 },
    { "log_at", ADF_DRV_LOG_AT, 2 },
    { "sin_at", ADF_DRV_SIN_AT, 2 },
    { "cos_at", ADF_DRV_COS_AT, 2 },
    { "sinh_at", ADF_DRV_SINH_AT, 2 },
    { "cosh_at", ADF_DRV_COSH_AT, 2 },
    { "roots_at", ADF_DRV_ROOTS_AT, 3 },
    { "root_at", ADF_DRV_ROOT_AT, 4 },
    { "powrat_at", ADF_DRV_POWRAT_AT, 4 },
    { "powunit_at", ADF_DRV_POWUNIT_AT, 3 },
    { "inv", ADF_DRV_INV, 1 },
    { "pow", ADF_DRV_POW, 2 },
    { "powtight", ADF_DRV_POWTIGHT, 2 },
    { "norm", ADF_DRV_NORM, 1 },
    { "class", ADF_DRV_CLASS, 1 },
    { "idele", ADF_DRV_MKIDELE, 1 },
    { "hull", ADF_DRV_HULL, 1 },
    { "hullsimple", ADF_DRV_HULLSIMPLE, 1 },
    { "unitof", ADF_DRV_UNITOF, 1 },
    { "valuation", ADF_DRV_VALUATION, 2 },
    { "abs", ADF_DRV_ABS, 2 },
    { "root", ADF_DRV_ROOT, ADF_DRV_ARITY_2_OR_3 },
    { "exp", ADF_DRV_EXP, 1 },
    { "sin", ADF_DRV_SIN, 1 },
    { "sinh", ADF_DRV_SINH, 1 },
    { "cos", ADF_DRV_COS, 1 },
    { "cosh", ADF_DRV_COSH, 1 },
    { "Log", ADF_DRV_IDLOG, 1 },
    { "logabs", ADF_DRV_IDLOGABS, 1 },
    { "log_abs", ADF_DRV_IDLOGABS, 1 },
    { "Log_at", ADF_DRV_IDLOG_AT, 2 },
    { "log_abs_at", ADF_DRV_IDLOGABS_AT, 2 },
    { "Log_refine", ADF_DRV_IDLOG_REFINE, 2 },
    { "log_abs_refine", ADF_DRV_IDLOGABS_REFINE, 2 },
    { "logabs_refine", ADF_DRV_IDLOGABS_REFINE, 2 },
    { "prec", ADF_DRV_PREC, 1 },
    { "legendre", ADF_DRV_LEGENDRE, 2 },
    { "jacobi", ADF_DRV_JACOBI, 2 },
    { "kronecker", ADF_DRV_KRONECKER, 2 },
    { "binom", ADF_DRV_BINOM, 2 },
    { "binomtight", ADF_DRV_BINOMTIGHT, 2 },
    { "profpow", ADF_DRV_PROFPOW, 2 },
    { "profpowcoarse", ADF_DRV_PROFPOWCOARSE, 2 },
    { "profpowfine", ADF_DRV_PROFPOWFINE, 2 },
    { "haar_volume", ADF_DRV_HAAR_VOLUME, 1 },
    { "cyclo_exp_u", ADF_DRV_CYCLO_EXP_U, 2 },
    { "cyclo_exp_uinv", ADF_DRV_CYCLO_EXP_UINV, 2 },
    { "hilbert_at", ADF_DRV_HILBERT_AT, 3 },
    { "local_zeta_factor_at", ADF_DRV_LOCAL_ZETA_AT, 2 },
    { "psi", ADF_DRV_PSI, 1 },
    { "psi_strict", ADF_DRV_PSI_STRICT, 1 },
    { "psi_at", ADF_DRV_PSI_AT, 2 },
    { "psi_strict_at", ADF_DRV_PSI_STRICT_AT, 2 },
    { "psi_phase", ADF_DRV_PSI_PHASE, 1 },
    { "char", ADF_DRV_CHAR, 1 },
    { "chi", ADF_DRV_CHI, 2 },
    { "gauss", ADF_DRV_GAUSS, 1 },

    { "rfun", ADF_DRV_RFUN, 1 },
    { "rfun_translate", ADF_DRV_RFUN_TRANSLATE, 2 },
    { "rfun_mul", ADF_DRV_RFUN_MUL, 2 },
    { "rfun_eval", ADF_DRV_RFUN_EVAL, 2 },
    { "rfun_fourier", ADF_DRV_RFUN_FOURIER, 1 },
    { "rfun_derivative", ADF_DRV_RFUN_DERIVATIVE, 1 },
    { "rfun_integral", ADF_DRV_RFUN_INTEGRAL, 1 },
    { "rfun_norm2", ADF_DRV_RFUN_NORM2, 1 },

    { "char_conj", ADF_DRV_CHAR_CONJ, 1 },
    { "char_unit", ADF_DRV_CHAR_UNIT, 2 },
    { "char_unit_strict", ADF_DRV_CHAR_UNIT_STRICT, 2 },

    { "tensor_eval", ADF_DRV_TENSOR_EVAL, 3 },
    { "tensor_eval_sball", ADF_DRV_TENSOR_EVAL_SBALL, 3 },
    { "ffun_eval", ADF_DRV_FFUN_EVAL, 2 },
    { "ffun_integral", ADF_DRV_FFUN_INTEGRAL, 1 },
    { "ffun_norm2", ADF_DRV_FFUN_NORM2, 1 },
    { "tensor_integral", ADF_DRV_TENSOR_INTEGRAL, 2 },
    { "tensor_norm2", ADF_DRV_TENSOR_NORM2, 2 },
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
    ADF_DRV_UCOSET,      /* milestone 2: the unit coset, the idele and the idele class */
    ADF_DRV_IDELE,
    ADF_DRV_IDCLASS,
    ADF_DRV_LBALL,       /* lane drv-ball: the local ball and the partial ball (conventions 5.8, 5.9) */
    ADF_DRV_SBALL,
    ADF_DRV_QCLASS,
    ADF_DRV_FFUN,
    ADF_DRV_RFUN_VALUE, /* dump/load storage; ADF_DRV_RFUN names a distinct operation enum */
    ADF_DRV_CHARACTER,
    ADF_DRV_OTHER        /* a kind of the value form with no typed parser in this build */
} adf_drv_type;

typedef struct
{
    adf_drv_type type;         /* the type the driver works with, from the kind */
    adf_rat_t r;
    adf_fball_t f;
    adf_adele_t a;
    adf_cadele_t c;
    adf_ucoset_t u;
    adf_idele_t i;
    adf_idclass_t k;
    adf_lball_t b;
    adf_sball_t s;
    adf_qclass_t q;
    adf_ffun_t ff;
    adf_rfun_t rf;

    adf_char_t character;
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
    adf_ucoset_init(v->u);
    adf_idele_init(v->i);
    adf_idclass_init(v->k);
    adf_lball_init(v->b);
    adf_sball_init(v->s);
    adf_qclass_init(v->q);
    adf_ffun_init(v->ff);
    adf_rfun_init(v->rf);

    adf_char_init(v->character);
}

static void
adf_drv_value_clear(adf_drv_value * v)
{
    adf_rat_clear(v->r);
    adf_fball_clear(v->f);
    adf_adele_clear(v->a);
    adf_cadele_clear(v->c);
    adf_ucoset_clear(v->u);
    adf_idele_clear(v->i);
    adf_idclass_clear(v->k);
    adf_lball_clear(v->b);
    adf_sball_clear(v->s);
    adf_qclass_clear(v->q);
    adf_ffun_clear(v->ff);
    adf_rfun_clear(v->rf);

    adf_char_clear(v->character);
    v->type = ADF_DRV_OTHER;
}

/* adf_drv_kind_type(kind): the type the driver works with for that kind of conventions 9.7,
   ADF_DRV_OTHER for the kinds with no typed parser in this build (the
   functions and the character, work packages 1.8 and later).  A caller answers ADF_UNSUPPORTED
   for ADF_DRV_OTHER before any value is read. */
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
        case ADF_TEXT_UCOSET:
            return ADF_DRV_UCOSET;
        case ADF_TEXT_IDELE:
            return ADF_DRV_IDELE;
        case ADF_TEXT_IDCLASS:
            return ADF_DRV_IDCLASS;
        case ADF_TEXT_LBALL:
            return ADF_DRV_LBALL;
        case ADF_TEXT_SBALL:
            return ADF_DRV_SBALL;
        case ADF_TEXT_QCLASS:
            return ADF_DRV_QCLASS;
        case ADF_TEXT_FFUN:
            return ADF_DRV_FFUN;

        case ADF_TEXT_CHAR:
            return ADF_DRV_CHARACTER;
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
        case ADF_TEXT_UCOSET:
            return adf_ucoset_set_str(v->u, s, len, NULL);
        case ADF_TEXT_IDELE:
            return adf_idele_set_str(v->i, s, len, prec, NULL);
        case ADF_TEXT_IDCLASS:
            return adf_idclass_set_str(v->k, s, len, prec, NULL);
        case ADF_TEXT_LBALL:
            return adf_lball_set_str(v->b, s, len, NULL);
        case ADF_TEXT_SBALL:
            return adf_sball_set_str(v->s, s, len, prec, NULL);
        case ADF_TEXT_QCLASS:
            return adf_qclass_set_str(v->q, s, len, prec, NULL);
        case ADF_TEXT_FFUN:
            return adf_ffun_set_str(v->ff, s, len, prec, NULL);

        case ADF_TEXT_CHAR:
            return adf_char_set_str(v->character, s, len, prec, NULL);
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
        case ADF_DRV_UCOSET:
            s = adf_ucoset_get_str(&len, v->u);
            break;
        case ADF_DRV_IDELE:
            s = adf_idele_get_str(&len, v->i, digits);
            break;
        case ADF_DRV_IDCLASS:
            s = adf_idclass_get_str(&len, v->k, digits);
            break;
        case ADF_DRV_LBALL:
            s = adf_lball_get_str(&len, v->b);
            break;
        case ADF_DRV_SBALL:
            s = adf_sball_get_str(&len, v->s, digits);
            break;
        case ADF_DRV_QCLASS:
            s = adf_qclass_get_str(&len, v->q, digits);
            break;
        case ADF_DRV_FFUN:
            s = adf_ffun_get_str(&len, v->ff, digits);
            break;
        case ADF_DRV_RFUN_VALUE:
            s = adf_rfun_get_str(&len, v->rf, digits);
            break;
        case ADF_DRV_CHARACTER:
            s = adf_char_get_str(&len, v->character, digits);
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
        case ADF_DRV_UCOSET:
            s = adf_ucoset_dump_str(&len, v->u);
            break;
        case ADF_DRV_IDELE:
            s = adf_idele_dump_str(&len, v->i);
            break;
        case ADF_DRV_IDCLASS:
            s = adf_idclass_dump_str(&len, v->k);
            break;
        case ADF_DRV_LBALL:
            s = adf_lball_dump_str(&len, v->b);
            break;
        case ADF_DRV_SBALL:
            s = adf_sball_dump_str(&len, v->s);
            break;
        case ADF_DRV_QCLASS:
            s = adf_qclass_dump_str(&len, v->q);
            break;
        case ADF_DRV_CHARACTER:
            s = adf_char_dump_str(&len, v->character);
            break;
        case ADF_DRV_FFUN:
            s = adf_ffun_dump_str(&len, v->ff);
            break;
        case ADF_DRV_RFUN_VALUE:
            s = adf_rfun_dump_str(&len, v->rf);
            break;
        default:
            return ADF_UNSUPPORTED;
    }

    (void) fwrite(s, 1, len, out);
    fputc('\n', out);
    adf_str_free(s);
    return ADF_OK;
}

/* The bodies of the grammar of conventions 10.1.  The driver reads and writes the nine it has
   a type for; the others are valid requests on a type that this build does not implement. */
static const char * const adf_drv_dump_bodies[] = {
    "rat", "fball", "scaled", "adele", "cadele", "ucoset", "idele", "idclass", "lball",
    "sball", "qclass", "ffun", "rfun", "char", "modctx"
};

static const size_t adf_drv_dump_body_count =
    sizeof(adf_drv_dump_bodies) / sizeof(adf_drv_dump_bodies[0]);

/* adf_drv_body_slot(name): the adf_drv_type of a body the driver has a value for, or
   ADF_DRV_OTHER for a body of section 10 that it has none. */
static adf_drv_type
adf_drv_body_slot(const char * name)
{
    /* An explicit pair table since milestone 4 added ADF_DRV_FFUN to the enum without a dump loader:
       the index of a name is no longer its type (orchestrator, merge of lane c-slice2). */
    static const struct { const char * name; adf_drv_type type; } mine[] = {
        { "rat", ADF_DRV_RAT }, { "fball", ADF_DRV_FBALL }, { "adele", ADF_DRV_ADELE },
        { "cadele", ADF_DRV_CADELE }, { "ucoset", ADF_DRV_UCOSET }, { "idele", ADF_DRV_IDELE },
        { "idclass", ADF_DRV_IDCLASS }, { "lball", ADF_DRV_LBALL }, { "sball", ADF_DRV_SBALL },
        { "qclass", ADF_DRV_QCLASS }, { "char", ADF_DRV_CHARACTER },
        { "ffun", ADF_DRV_FFUN }, { "rfun", ADF_DRV_RFUN_VALUE } };
    size_t i;

    for (i = 0; i < sizeof(mine) / sizeof(mine[0]); i++)
        if (strcmp(name, mine[i].name) == 0)
            return mine[i].type;
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
        case ADF_DRV_CADELE:
            status = adf_cadele_dump_inspect(&nctx, NULL, s, len, NULL);
            break;
        case ADF_DRV_UCOSET:
            status = adf_ucoset_dump_inspect(&nctx, NULL, s, len, NULL);
            break;
        case ADF_DRV_IDELE:
            status = adf_idele_dump_inspect(&nctx, NULL, s, len, NULL);
            break;
        case ADF_DRV_IDCLASS:
            status = adf_idclass_dump_inspect(&nctx, NULL, s, len, NULL);
            break;
        case ADF_DRV_LBALL:
            status = adf_lball_dump_inspect(&nctx, NULL, s, len, NULL);
            break;
        case ADF_DRV_QCLASS:
            status = adf_qclass_dump_inspect(&nctx, NULL, s, len, NULL);
            break;
        case ADF_DRV_CHARACTER:
            status = adf_char_dump_inspect(&nctx, NULL, s, len, NULL);
            break;
        case ADF_DRV_FFUN:
            status = adf_ffun_dump_inspect(&nctx, NULL, s, len, NULL);
            break;
        case ADF_DRV_RFUN_VALUE:
            status = adf_rfun_dump_inspect(&nctx, NULL, s, len, NULL);
            break;
        default:
            status = adf_sball_dump_inspect(&nctx, NULL, s, len, NULL);
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
        case ADF_DRV_CADELE:
            return adf_cadele_load_str(v->c, s, len, NULL, NULL);
        case ADF_DRV_UCOSET:
            return adf_ucoset_load_str(v->u, s, len, NULL, NULL);
        case ADF_DRV_IDELE:
            return adf_idele_load_str(v->i, s, len, NULL, NULL);
        case ADF_DRV_IDCLASS:
            return adf_idclass_load_str(v->k, s, len, NULL, NULL);
        case ADF_DRV_LBALL:
            return adf_lball_load_str(v->b, s, len, NULL, NULL);
        case ADF_DRV_QCLASS:
            return adf_qclass_load_str(v->q, s, len, NULL, NULL);
        case ADF_DRV_CHARACTER:
            return adf_char_load_str(v->character, s, len, NULL, NULL);
        case ADF_DRV_FFUN:
            return adf_ffun_load_str(v->ff, s, len, NULL, NULL);
        case ADF_DRV_RFUN_VALUE:
            return adf_rfun_load_str(v->rf, s, len, NULL, NULL);
        default:
            return adf_sball_load_str(v->s, s, len, NULL, NULL);
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

/* Four operands are supported. The fifth slot detects excess operands; no command accepts it. */
typedef struct
{
    const char * word;
    size_t wlen;
    const char * s[5];
    size_t n[5];
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
    while (k < 5 && i < len)
    {
        const char * sep;

        while (i < len && adf_drv_is_blank(line[i]))
            i++;
        if (i >= len)
        {
            k = 5; /* A separator followed only by whitespace leaves a missing operand. */
            break;
        }
        start = i;
        sep = adf_drv_find(line + i, len - i, ADF_DRV_SEP, ADF_DRV_SEP_LEN);
        l->s[k] = line + start;
        if (sep != NULL)
        {
            l->n[k] = (size_t) (sep - (line + start));
            i = (size_t) (sep - line) + ADF_DRV_SEP_LEN;
            if (i == len)
            {
                k = 5; /* A trailing separator is syntax, not an absent optional operand. */
                break;
            }
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
    /* a local ball and a partial ball are values the driver reads and prints, but SPEC 4.1
       combines no pair of types with them: ADF_DOMAIN, as for every other pair the operation
       does not define */
    if (t != ADF_DRV_RAT && t != ADF_DRV_FBALL && t != ADF_DRV_ADELE && t != ADF_DRV_CADELE)
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

/* ---- the commands at places (milestone 1F, lane f-slice6) ---- */

/* These commands read places as their second operand, which the value form does not have:

     project X with PLACES    the partial ball of X over the places, adf_sball_project
     exp_at X with PLACE      exp of X at the one place, adf_sball_exp_at
     log_at X with PLACE      log of X at the one place, adf_sball_log_at
     sin_at, cos_at, sinh_at, cosh_at X with PLACE: the corresponding adf_sball function (1F.7)

   X is an exact rational, a finite ball, an adele or a partial ball of the value form (lane drv-ball).  A
   rational is converted to the
   adele (q ; q) at the setting prec (SPEC 4.1); a finite ball has no real coordinate, so the place "real"
   is ADF_DOMAIN for it; a partial ball is not made into an adele, its component at each place is taken
   (adf_drv_sball_restrict), and a place that is no place of it is ADF_DOMAIN; a complex adele and a complex
   partial ball are ADF_UNSUPPORTED (the complex functions are not in this slice,
   include/adelefeld/sball.h).  PLACES is a list of places separated by single spaces: a prime in decimal
   or the word real.  A token of another shape (a sign other than "-", a leading zero, a letter, two
   spaces in a row) is ADF_PARSE.  A negative number, 0, 1, a composite and a number of more than 64 bits
   are not places: ADF_DOMAIN, as for the solver commands.  A place twice is ADF_DOMAIN (adf_sball_project,
   and adf_sball_set_arb_lballs for a partial ball).
   Each function command takes exactly one place.

   The precision: at the place real, the setting prec is the working precision in bits of arb; at a prime
   it is the requested ABSOLUTE p-adic precision N of adelefeld/lfunc.h (include/adelefeld/rfunc.h, "A
   PRIME").  The line printed is the partial ball: its components in the canonical order (the real place
   first, then the primes increasing), separated by "; ", each

     real: <ball>                  the real-ball text of conventions 9.5 with the setting digits
     <p>: <centre>                 an exact local ball: the rational p^v u, printed as a rational
     <p>: <centre> + O(<p>^<N>)    a local ball: the canonical centre p^v u in [0, p^N), the rational
                                   printed as the driver prints a rational, and N its absolute precision

   This is a text of the driver and not a value form of the library (the value form of a partial ball,
   conventions 9.2, is "{E; E; ...}" with the labels "inf: " and "p=<p>: "; the library printer
   adf_sball_get_str prints that text, and lane drv-ball compares the two on every fixture line:
   lanes/drv-ball/printer-diff.md.  They differ in every line, so this line keeps the text of the
   driver and adf_drv_put_sball is not replaced by the printer of the library).
   The statuses of the library are the statuses of the command; the place reported by the library is not
   printed. */

/* adf_drv_place_token(tok, len, v): one token of a list of places, the syntax and the value together, since
   both are decided on the token alone: ADF_PARSE for a token that is not "real" or an optionally negative
   decimal without a leading zero; ADF_DOMAIN for a value that is no prime of a place; else the place. */
static int
adf_drv_place_token(const char * tok, size_t len, adf_place_t * v)
{
    fmpz_t z;
    char * buf;
    size_t i = 0, digits;
    int neg, status;

    if (len == 4 && memcmp(tok, "real", 4) == 0)
    {
        *v = adf_place_inf();
        return ADF_OK;
    }
    neg = (len > 0 && tok[0] == '-');
    if (neg)
        i = 1;
    digits = len - i;
    if (digits == 0)
        return ADF_PARSE;
    if (tok[i] == '0' && digits > 1)
        return ADF_PARSE;        /* a leading zero is not the decimal of the operand form */
    for (; i < len; i++)
        if (tok[i] < '0' || tok[i] > '9')
            return ADF_PARSE;
    if (neg)
        return ADF_DOMAIN;
    buf = flint_malloc(digits + 1);
    memcpy(buf, tok + (len - digits), digits);
    buf[digits] = '\0';
    fmpz_init(z);
    status = (fmpz_set_str(z, buf, 10) == 0) ? adf_drv_place_operand(v, z) : ADF_PARSE;
    fmpz_clear(z);
    flint_free(buf);
    return status;
}

/* adf_drv_place_list(s, len, places, n): the list of places of the second operand.  Tokens are separated
   by exactly one space; a blank at the end of the operand, or two in a row, is a token of length 0, which
   is ADF_PARSE.  *n receives the number of places read.  The syntax of every token is decided before the
   value of any (the order of the checks): a PARSE anywhere wins over a DOMAIN before it.  The result is
   ADF_OK, or the status of the first token that is no place. */
static int
adf_drv_place_list(const char * s, size_t len, adf_place_t * places, slong * n)
{
    size_t i = 0, start;
    slong k = 0;
    int first_bad = ADF_OK, status;

    while (1)
    {
        adf_place_t v = adf_place_inf();

        start = i;
        while (i < len && s[i] != ' ')
            i++;
        status = adf_drv_place_token(s + start, i - start, &v);
        if (status == ADF_PARSE)
            return ADF_PARSE;
        if (status != ADF_OK && first_bad == ADF_OK)
            first_bad = status;
        places[k++] = v;
        if (i == len)
            break;
        i++;                     /* the single space */
    }
    *n = k;
    return first_bad;
}

/* adf_drv_lball_text(l): the text of a local ball as above, or NULL when the centre is beyond the bound of
   adf_lball_get_center (the printer refuses: ADF_LIMIT, as for every printer).  The string is allocated
   with flint_malloc. */
static char *
adf_drv_lball_text(const adf_lball_struct * l)
{
    adf_rat_t c;
    char * t, * out, tail[64];
    size_t len, tl;

    adf_rat_init(c);
    if (adf_lball_get_center(c, l) != ADF_OK)
    {
        adf_rat_clear(c);
        return NULL;
    }
    t = adf_rat_get_str(&len, c);
    adf_rat_clear(c);
    if (l->exact)
        tail[0] = '\0';
    else
        flint_sprintf(tail, " + O(%wu^%wd)", (ulong) l->p, l->N);
    tl = strlen(tail);
    out = flint_malloc(len + tl + 1);
    memcpy(out, t, len);
    memcpy(out + len, tail, tl + 1);
    adf_str_free(t);
    return out;
}

/* adf_drv_put_sball(out, y, digits): the line of a partial ball.  Every text is formed before a byte is
   written, so that a printer that refuses one gives ADF_LIMIT and the line of the error, not half a line. */
static int
adf_drv_put_sball(FILE * out, const adf_sball_t y, slong digits)
{
    char ** t;
    slong i, m = 0, count = adf_sball_num_places(y);
    int status = ADF_OK;
    arb_t r;

    if (y->arch == ADF_ARCH_COMPLEX)
        return ADF_UNSUPPORTED;
    t = flint_calloc(count + 1, sizeof(char *));
    if (y->arch == ADF_ARCH_REAL)
    {
        char * b;

        arb_init(r);
        (void) adf_sball_get_arb(r, y, adf_place_inf());
        b = adf_drv_arb_str(r, digits);
        arb_clear(r);
        if (b == NULL)
            status = ADF_LIMIT;
        else
        {
            t[m] = flint_malloc(strlen(b) + 7);
            strcpy(t[m], "real: ");
            strcat(t[m], b);
            flint_free(b);
            m++;
        }
    }
    for (i = 0; status == ADF_OK && i < y->len; i++)
    {
        char * b = adf_drv_lball_text(&y->loc[i]), pre[32];

        if (b == NULL)
        {
            status = ADF_LIMIT;
            break;
        }
        flint_sprintf(pre, "%wu: ", (ulong) y->loc[i].p);
        t[m] = flint_malloc(strlen(pre) + strlen(b) + 1);
        strcpy(t[m], pre);
        strcat(t[m], b);
        flint_free(b);
        m++;
    }
    for (i = 0; i < m; i++)
    {
        if (status == ADF_OK)
        {
            if (i > 0)
                fputs("; ", out);
            fputs(t[i], out);
        }
        flint_free(t[i]);
    }
    flint_free(t);
    if (status == ADF_OK)
        fputc('\n', out);
    return status;
}

/* Root degree/seed: use the ordinary typed value parser, then require a word integer.
   At 2 the user sign -1 is the API's residue identifier 3. */
static int
adf_drv_root_word(ulong *word, const char *s, size_t len, int sign_seed, slong prec)
{
    adf_drv_value value;
    adf_text_kind kind;
    fmpz_t z;
    int status;
    adf_drv_value_init(&value); fmpz_init(z);
    status=adf_text_classify(&kind,s,len,NULL);
    if (status==ADF_OK) status=adf_drv_value_read(&value,kind,s,len,prec);
    if (status==ADF_OK) status=adf_drv_int_value(z,&value);
    if (status==ADF_OK)
    {
        if (sign_seed && fmpz_equal_si(z,-1)) *word=3;
        else if (fmpz_sgn(z)<0 || fmpz_cmp_ui(z,UWORD_MAX)>0) status=ADF_DOMAIN;
        else *word=fmpz_get_ui(z);
    }
    fmpz_clear(z); adf_drv_value_clear(&value);
    return status;
}

/* N-D10 component text plus the branch identifier. Form every string before printing. */
static int
adf_drv_root_result(FILE *out, const adf_sball_t x, adf_place_t p, ulong degree,
                    ulong seed, int all, slong N)
{
    adf_lball_t c;
    adf_lball_ptr roots=NULL;
    ulong count=1, *ids=NULL;
    slong len=0;
    char **texts=NULL;
    int status;
    if (adf_place_is_archimedean(p)) return ADF_UNSUPPORTED;
    adf_lball_init(c);
    status=adf_sball_get_lball(c,x,p);
    if (status==ADF_OK && all) status=adf_lball_root_count(&count,c,degree);
    if (status==ADF_OK && count>(ulong)ADF_LROOT_BRANCH_MAX) status=ADF_LIMIT;
    if (status!=ADF_OK) goto done;
    roots=flint_malloc(count*sizeof(adf_lball_struct));
    ids=flint_malloc(count*sizeof(ulong)); texts=flint_calloc(count,sizeof(char *));
    for (ulong i=0;i<count;i++) adf_lball_init(roots+i);
    if (all) status=adf_sball_roots_at(roots,ids,&len,(slong)count,NULL,x,p,degree,N);
    else
    {
        adf_sball_t selected;
        adf_sball_init(selected);
        status=adf_sball_root_seed_at(selected,NULL,x,p,degree,seed,N);
        if (status==ADF_OK) status=adf_sball_get_lball(roots,selected,p);
        adf_sball_clear(selected);
        ids[0]=degree==1 ? 0 : seed; len=1;
    }
    for (slong i=0;status==ADF_OK && i<len;i++)
    {
        texts[i]=adf_drv_lball_text(roots+i);
        if (!texts[i]) status=ADF_LIMIT;
    }
    if (status==ADF_OK)
    {
        for (slong i=0;i<len;i++)
        {
            if (i) fputs("; ",out);
            if (c->p==2 && ids[i])
                flint_fprintf(out,"%wu [%s]: %s",c->p,ids[i]==1 ? "+1" : "-1",texts[i]);
            else flint_fprintf(out,"%wu [%wu]: %s",c->p,ids[i],texts[i]);
        }
        fputc('\n',out);
    }
    for (ulong i=0;i<count;i++) { adf_lball_clear(roots+i); flint_free(texts[i]); }
done:
    flint_free(roots); flint_free(ids); flint_free(texts); adf_lball_clear(c);
    return status;
}

/* Powers at a prime (lane f-slice9, 1F.6; tools/adf/README.md, "Powers at a prime"):

     powrat_at X with PRIME with E/N with SEED     x^(E/N) on the branch SEED, adf_sball_powrat_at
     powunit_at X with PRIME with S                x^s for a principal unit x and s in Z_p, adf_sball_powunit_at

   The exponent E/N of powrat_at is the value text of an exact rational (so it is already reduced, N >= 1); another
   type is ADF_DOMAIN, a numerator beyond a slong or a denominator beyond a word ADF_LIMIT (as the exponent of pow).
   SEED is read as the seed of root_at: a word, -1 accepted for the identifier 3 at 2; it names the root of degree N
   of X (lpow.h). The line is "<p> [<id>]: <value>" as for root_at, the identifier 0 for N = 1 and for the zero.
   S of powunit_at is a value as X is (a rational, a finite ball, an adele), projected to the prime; the line is the
   partial ball, "<p>: <value>", as for exp_at. */

/* adf_drv_ratexp(e, n, s, len, prec): the exponent E/N of powrat_at, read by the typed parser. */
static int
adf_drv_ratexp(slong *e, ulong *n, const char *s, size_t len, slong prec)
{
    adf_drv_value value;
    adf_text_kind kind;
    fmpq_t q;
    int status;
    adf_drv_value_init(&value); fmpq_init(q);
    status=adf_text_classify(&kind,s,len,NULL);
    if (status==ADF_OK) status=adf_drv_value_read(&value,kind,s,len,prec);
    if (status==ADF_OK && value.type!=ADF_DRV_RAT) status=ADF_DOMAIN;
    if (status==ADF_OK)
    {
        adf_rat_get_fmpq(q,value.r);
        if (!fmpz_fits_si(fmpq_numref(q)) || fmpz_cmp_ui(fmpq_denref(q),UWORD_MAX)>0) status=ADF_LIMIT;
        else { *e=fmpz_get_si(fmpq_numref(q)); *n=fmpz_get_ui(fmpq_denref(q)); }
    }
    fmpq_clear(q); adf_drv_value_clear(&value);
    return status;
}

/* adf_drv_value_adele(a, x, has_real, prec): the adele of X or S of the commands at places: a rational is (q ; q) at
   prec, a finite ball has no real coordinate (ADF_DOMAIN when the real place is named), an adele is itself; another
   type is ADF_UNSUPPORTED (README, "The commands at places"). */
static int
adf_drv_value_adele(adf_adele_t a, const adf_drv_value *x, int has_real, slong prec)
{
    arb_t r0;
    int status=ADF_OK;
    if (x->type!=ADF_DRV_RAT && x->type!=ADF_DRV_FBALL && x->type!=ADF_DRV_ADELE) return ADF_UNSUPPORTED;
    if (x->type==ADF_DRV_FBALL)
    {
        if (has_real) return ADF_DOMAIN;
        arb_init(r0);
        status=adf_adele_set_arb_fball(a,r0,x->f);
        arb_clear(r0);
    }
    else if (x->type==ADF_DRV_RAT) adf_adele_set_rat(a,x->r,prec);
    else adf_adele_set(a,x->a);
    return status;
}

/* adf_drv_sball_restrict(y, x, places, n): y = the partial ball of the partial ball x over the n places,
   that is the component of x at each of them (docs/proofs/functions.md Proposition 22 for the projection of
   an adele, read on the partial ball itself: the component of the projection is the component, since the
   projection takes no component away).  The places of x are in the canonical order and distinct, so only a
   place that is no place of x can fail.  Status: ADF_OK, y written; ADF_DOMAIN when a place of the list is
   no place of x, and ADF_DOMAIN when a place of the list occurs twice (adf_sball_set_arb_lballs answers
   DOMAIN for a repeated prime); ADF_LIMIT from adf_sball_set_arb_lballs; ADF_UNSUPPORTED when the
   archimedean tag of x is complex, because the functions at places are the real ones
   (include/adelefeld/sball.h, "A PRIME").  Every place of the list is checked before any component is
   copied, so the status does not depend on the order of the list. */
static int
adf_drv_sball_restrict(adf_sball_t y, const adf_sball_t x, const adf_place_t * places, slong n)
{
    adf_lball_struct * loc;
    arb_t r;
    slong i, m = 0;
    int status = ADF_OK, real = 0;

    if (adf_sball_arch(x) == ADF_ARCH_COMPLEX)
        return ADF_UNSUPPORTED;
    for (i = 0; i < n; i++)
    {
        adf_place_t v = places[i];
        slong j;

        if (!adf_sball_has_place(x, v))
            return ADF_DOMAIN;
        for (j = 0; j < i; j++)
            if (adf_place_equal(places[j], v))
                return ADF_DOMAIN;      /* a place named twice, as adf_sball_project answers it */
    }
    loc = flint_calloc(n > 0 ? n : 1, sizeof(adf_lball_struct));
    arb_init(r);
    for (i = 0; i < n; i++)
    {
        adf_place_t v = places[i];

        if (adf_place_is_archimedean(v))
        {
            status = adf_sball_get_arb(r, x, v);
            real = 1;
            if (status != ADF_OK)
                break;
            continue;
        }
        adf_lball_init(loc + m);
        status = adf_sball_get_lball(loc + m, x, v);
        if (status != ADF_OK)
        {
            adf_lball_clear(loc + m);
            break;
        }
        m++;
    }
    if (status == ADF_OK)
        status = adf_sball_set_arb_lballs(y, NULL, real ? r : NULL, loc, m);
    arb_clear(r);
    for (i = 0; i < m; i++)
        adf_lball_clear(loc + i);
    flint_free(loc);
    return status;
}

/* adf_drv_value_sball(y, x, places, n, has_real, prec): the partial ball of the value x over the n places, which is
   the operand X of the commands at places.  A rational, a finite ball and an adele go through the adele and
   adf_sball_project, as before; a partial ball is restricted to the places (adf_drv_sball_restrict).  A complex
   adele, a unit coset, an idele and a class are ADF_UNSUPPORTED here and their fields must not be read as an adele
   (review n-review1 D1).  Statuses: those of the two, and ADF_DOMAIN for a finite ball with the place real. */
static int
adf_drv_value_sball(adf_sball_t y, const adf_drv_value * x, const adf_place_t * places, slong n, int has_real,
                    slong prec)
{
    adf_adele_t a;
    int status;

    if (x->type == ADF_DRV_SBALL)
        return adf_drv_sball_restrict(y, x->s, places, n);
    if (x->type != ADF_DRV_RAT && x->type != ADF_DRV_FBALL && x->type != ADF_DRV_ADELE)
        return ADF_UNSUPPORTED;
    adf_adele_init(a);
    status = adf_drv_value_adele(a, x, has_real, prec);
    if (status == ADF_OK)
        status = adf_sball_project(y, NULL, a, places, n);
    adf_adele_clear(a);
    return status;
}

/* The line of powrat_at: the component at the prime with the identifier of the branch, formed before printing. */
static int
adf_drv_powrat_result(FILE *out, const adf_sball_t x, adf_place_t p, slong e, ulong n, ulong seed, slong N)
{
    adf_sball_t y;
    adf_lball_t c;
    char *text=NULL;
    ulong id=n==1 ? 0 : seed, prime;
    int status;
    adf_sball_init(y); adf_lball_init(c);
    status=adf_sball_powrat_at(y,NULL,x,p,e,n,seed,N);
    if (status==ADF_OK) status=adf_sball_get_lball(c,y,p);
    if (status==ADF_OK && (text=adf_drv_lball_text(c))==NULL) status=ADF_LIMIT;
    if (status==ADF_OK)
    {
        prime=adf_place_prime_get(p);
        if (prime==2 && id) flint_fprintf(out,"%wu [%s]: %s\n",prime,id==1 ? "+1" : "-1",text);
        else flint_fprintf(out,"%wu [%wu]: %s\n",prime,id,text);
    }
    flint_free(text); adf_sball_clear(y); adf_lball_clear(c);
    return status;
}

/* adf_drv_places(out, op, l, st): project, exp_at and log_at, the steps of the command at the top of the
   file: 2. the syntax of X (adf_text_classify) and of the list of places; 3. the kind of X; 4. its value;
   5. the operation: the places, the type of X against them, the projection, the function; 6. the printer. */
static int
adf_drv_places(FILE * out, adf_drv_op op, const adf_drv_line * l, adf_drv_state * st)
{
    adf_drv_value x;
    adf_text_kind kind;
    adf_place_t * places = NULL;
    adf_sball_t s, y;
    slong n = 0, i;
    ulong degree=0, seed=0, pden=1;
    slong pnum=0;
    adf_drv_value sv;
    adf_text_kind skind = ADF_TEXT_RAT;
    adf_sball_t ss;
    int status, place_status = ADF_OK, has_real = 0;

    adf_drv_value_init(&sv);
    adf_sball_init(ss);
    adf_drv_value_init(&x);
    adf_sball_init(s);
    adf_sball_init(y);

    /* step 2: the syntax of X, then of the places (at most one token to two bytes of the operand) */
    status = adf_text_classify(&kind, l->s[0], l->n[0], NULL);
    if (status != ADF_OK)
        goto done;
    places = flint_malloc((l->n[1] / 2 + 2) * sizeof(adf_place_t));
    place_status = adf_drv_place_list(l->s[1], l->n[1], places, &n);
    if (place_status == ADF_PARSE)
    {
        status = ADF_PARSE;
        goto done;
    }
    if (op != ADF_DRV_PROJECT && n != 1)
    {
        status = ADF_PARSE;      /* each function command takes exactly one place */
        goto done;
    }
    if (op==ADF_DRV_ROOT_AT || op==ADF_DRV_ROOTS_AT)
    {
        status=adf_drv_root_word(&degree,l->s[2],l->n[2],0,st->prec);
        if (status!=ADF_OK) goto done;
        if (op==ADF_DRV_ROOT_AT)
        {
            int sign_seed=place_status==ADF_OK && !adf_place_is_archimedean(places[0]) &&
                          adf_place_prime_get(places[0])==2;
            status=adf_drv_root_word(&seed,l->s[3],l->n[3],sign_seed,st->prec);
            if (status!=ADF_OK) goto done;
        }
    }
    if (op==ADF_DRV_POWRAT_AT)
    {
        int sign_seed=place_status==ADF_OK && !adf_place_is_archimedean(places[0]) &&
                      adf_place_prime_get(places[0])==2;
        status=adf_drv_ratexp(&pnum,&pden,l->s[2],l->n[2],st->prec);
        if (status==ADF_OK) status=adf_drv_root_word(&seed,l->s[3],l->n[3],sign_seed,st->prec);
        if (status!=ADF_OK) goto done;
    }
    if (op==ADF_DRV_POWUNIT_AT)
    {
        /* the syntax of S belongs to step 2, as that of X */
        status=adf_text_classify(&skind,l->s[2],l->n[2],NULL);
        if (status!=ADF_OK) goto done;
    }
    /* step 3 and step 4: the kind and the value of X */
    if (adf_drv_kind_type(kind) == ADF_DRV_OTHER)
    {
        status = ADF_UNSUPPORTED;
        goto done;
    }
    status = adf_drv_value_read(&x, kind, l->s[0], l->n[0], st->prec);
    if (status != ADF_OK)
        goto done;
    if (op==ADF_DRV_POWUNIT_AT)
    {
        if (adf_drv_kind_type(skind)==ADF_DRV_OTHER) { status=ADF_UNSUPPORTED; goto done; }
        status=adf_drv_value_read(&sv,skind,l->s[2],l->n[2],st->prec);
        if (status!=ADF_OK) goto done;
    }
    /* step 5: the places, then the type, then the projection and the function */
    if (place_status != ADF_OK)
    {
        status = place_status;
        goto done;
    }
    /* the domain of these commands is a rational, a finite ball, an adele or a partial ball (README); a complex
       adele, a unit coset, an idele and a class are not, and their fields must not be read as an adele
       (review n-review1 D1) */
    if (x.type != ADF_DRV_RAT && x.type != ADF_DRV_FBALL && x.type != ADF_DRV_ADELE &&
        x.type != ADF_DRV_SBALL)
    {
        status = ADF_UNSUPPORTED;
        goto done;
    }
    for (i = 0; i < n; i++)
        if (adf_place_is_archimedean(places[i]))
            has_real = 1;
    if (x.type == ADF_DRV_FBALL && has_real)
    {
        status = ADF_DOMAIN;     /* a finite ball has no real coordinate */
        goto done;
    }
    status = adf_drv_value_sball(s, &x, places, n, has_real, st->prec);
    if (status != ADF_OK)
        goto done;
    if (op==ADF_DRV_ROOT_AT || op==ADF_DRV_ROOTS_AT)
    {
        status=adf_drv_root_result(out,s,places[0],degree,seed,op==ADF_DRV_ROOTS_AT,st->prec);
        goto done;
    }
    if (op==ADF_DRV_POWRAT_AT)
    {
        status=adf_drv_powrat_result(out,s,places[0],pnum,pden,seed,st->prec);
        goto done;
    }
    if (op==ADF_DRV_POWUNIT_AT)
    {
        status=adf_drv_value_sball(ss,&sv,places,n,has_real,st->prec);
        if (status==ADF_OK) status=adf_sball_powunit_at(y,NULL,s,ss,places[0],st->prec);
        if (status==ADF_OK) status=adf_drv_put_sball(out,y,st->digits);
        goto done;
    }
    if (op == ADF_DRV_PROJECT)
        adf_sball_swap(y, s);
    else if (op == ADF_DRV_EXP_AT)
        status = adf_sball_exp_at(y, NULL, s, places[0], st->prec);
    else if (op == ADF_DRV_SIN_AT)
        status = adf_sball_sin_at(y, NULL, s, places[0], st->prec);
    else if (op == ADF_DRV_COS_AT)
        status = adf_sball_cos_at(y, NULL, s, places[0], st->prec);
    else if (op == ADF_DRV_SINH_AT)
        status = adf_sball_sinh_at(y, NULL, s, places[0], st->prec);
    else if (op == ADF_DRV_COSH_AT)
        status = adf_sball_cosh_at(y, NULL, s, places[0], st->prec);
    else
        status = adf_sball_log_at(y, NULL, s, places[0], st->prec);
    if (status == ADF_OK)
        status = adf_drv_put_sball(out, y, st->digits);

done:
    flint_free(places);
    adf_sball_clear(s);
    adf_sball_clear(y);
    adf_drv_value_clear(&x);
    adf_drv_value_clear(&sv);
    adf_sball_clear(ss);
    return status;
}

/* ---- the unit coset, the idele and the idele class (milestone 2, lane t-slice1) ---- */

/* The kinds ucoset, idele and idclass of the value form have typed parsers and printers, and the driver combines
   them as follows (tools/adf/README.md, "Ideles and classes").  Each command is one line of output.

     show X, type X               as for every kind (dump X is ADF_UNSUPPORTED: no dump form yet)
     mul X with Y                 ucoset * ucoset, idele * idele, class * class; idele * rational and
                                  rational * idele (adf_idele_mul_rat: the rational must not be 0, ADF_NOT_UNIT)
     div X with Y                 the same pairs, x * y^-1 (adf_ucoset_inv, adf_idele_inv, adf_idclass_inv, then the
                                  product; idele / rational is the product with the exact inverse of the rational);
                                  an adele by an idele: adf_adele_div_idele
     neg X                        a unit coset: the product with [-1]; an idele: every coordinate negated, exactly
                                  (-X, r, [-1] u); a class has no negation (the class of -x is the class of x): DOMAIN
     inv X                        ucoset, idele, class, and a rational (ADF_NOT_UNIT for 0)
     pow X with K, powtight X with K
                                  ucoset, idele, class, K an integer that fits a word (adelefeld/idpow.h)
     norm X                       idele and class: the positive real ball |x_inf| / r, resp. t
     class X                      idele to class (adf_idclass_set_idele)
     idele Q                      a rational to its idele at the setting prec (adf_idele_set_rat)
     hull X, hullsimple X         idele to adele: the smallest ball, the simple ball (adelefeld/idmap.h)
     unitof A                     adele to idele when the adele certifies it (adf_idele_set_adele)
     valuation X with P           v_p(r) of an idele, P a prime (ADF_DOMAIN for real, for a non-prime)
     abs X with PLACE             |x_p|_p as a rational at a prime; |x_inf| as a real ball at real
     equal, contains, overlaps    unit cosets only (the sets of adelefeld/ucoset.h); the library has no set
                                  predicate of ideles or classes (docs/api-2.md 3.5, decision i3-1): DOMAIN
     add, sub, cap, compare, reconstruct
                                  DOMAIN with an operand of these kinds

   Every other combination is ADF_DOMAIN: a pair of types that the operation does not define.  The steps are those
   of the command at the top of the file: the kinds of these operands are supported, so step 3 does not refuse
   them, and every domain rule above is in step 5.  The ball of norm and abs at the real place is printed by
   adf_drv_arb_str (the text of conventions 9.5 without a sign condition). */

static int
adf_drv_is_unit_type(adf_drv_type t)
{
    return t == ADF_DRV_UCOSET || t == ADF_DRV_IDELE || t == ADF_DRV_IDCLASS;
}

/* adf_drv_slong_operand(k, v): the exponent of pow and powtight: an exact rational that is an integer.  Another
   type, or a denominator other than 1, is ADF_DOMAIN (data outside the domain of the operation); an integer that
   does not fit a slong is a size bound of the interface, ADF_LIMIT (as adf_drv_prec_operand). */
static int
adf_drv_slong_operand(slong * k, const adf_drv_value * v)
{
    fmpq_t q;
    int status = ADF_OK;

    if (v->type != ADF_DRV_RAT)
        return ADF_DOMAIN;
    fmpq_init(q);
    adf_rat_get_fmpq(q, v->r);
    if (!fmpz_is_one(fmpq_denref(q)))
        status = ADF_DOMAIN;
    else if (!fmpz_fits_si(fmpq_numref(q)))
        status = ADF_LIMIT;
    else
        *k = fmpz_get_si(fmpq_numref(q));
    fmpq_clear(q);
    return status;
}

/* adf_drv_arb_line(out, x, digits): the line of a real ball, or ADF_LIMIT when the printer of the library refuses
   it (decision M1-D6, through adf_drv_arb_str). */
static int
adf_drv_arb_line(FILE * out, const arb_t x, slong digits)
{
    char * s = adf_drv_arb_str(x, digits);

    if (s == NULL)
        return ADF_LIMIT;
    fputs(s, out);
    fputc('\n', out);
    flint_free(s);
    return ADF_OK;
}

/* adf_drv_units_mul(z, x, y, div, prec): z = x * y or x / y for the pairs listed above; ADF_DOMAIN for another
   pair.  The statuses are those of the library (ADF_NOT_DETERMINED of the real kernel, ADF_NOT_UNIT for the
   rational 0); z is written only on ADF_OK. */
static int
adf_drv_units_mul(adf_drv_value * z, const adf_drv_value * x, const adf_drv_value * y, int div, slong prec)
{
    int status;

    if (x->type == ADF_DRV_UCOSET && y->type == ADF_DRV_UCOSET)
    {
        adf_ucoset_t t;

        adf_ucoset_init(t);
        if (div)
            adf_ucoset_inv(t, y->u);
        else
            adf_ucoset_set(t, y->u);
        adf_ucoset_mul(z->u, x->u, t);
        adf_ucoset_clear(t);
        z->type = ADF_DRV_UCOSET;
        return ADF_OK;
    }
    if (x->type == ADF_DRV_IDELE && y->type == ADF_DRV_IDELE)
    {
        adf_idele_t t;

        adf_idele_init(t);
        status = div ? adf_idele_inv(t, y->i, prec) : (adf_idele_set(t, y->i), ADF_OK);
        if (status == ADF_OK)
            status = adf_idele_mul(z->i, x->i, t, prec);
        adf_idele_clear(t);
        z->type = ADF_DRV_IDELE;
        return status;
    }
    if (x->type == ADF_DRV_IDCLASS && y->type == ADF_DRV_IDCLASS)
    {
        adf_idclass_t t;

        adf_idclass_init(t);
        status = div ? adf_idclass_inv(t, y->k, prec) : (adf_idclass_set(t, y->k), ADF_OK);
        if (status == ADF_OK)
            status = adf_idclass_mul(z->k, x->k, t, prec);
        adf_idclass_clear(t);
        z->type = ADF_DRV_IDCLASS;
        return status;
    }
    if ((x->type == ADF_DRV_IDELE && y->type == ADF_DRV_RAT) || (x->type == ADF_DRV_RAT && y->type == ADF_DRV_IDELE))
    {
        const adf_drv_value * i = (x->type == ADF_DRV_IDELE) ? x : y;
        const adf_drv_value * q = (x->type == ADF_DRV_IDELE) ? y : x;
        adf_rat_t t;

        if (div && x->type == ADF_DRV_RAT)
            return ADF_DOMAIN;             /* a rational divided by an idele is not offered */
        adf_rat_init(t);
        status = div ? adf_rat_inv(t, q->r) : (adf_rat_set(t, q->r), ADF_OK);
        if (status == ADF_OK)
            status = adf_idele_mul_rat(z->i, i->i, t, prec);
        adf_rat_clear(t);
        z->type = ADF_DRV_IDELE;
        return status;
    }
    if (div && x->type == ADF_DRV_ADELE && y->type == ADF_DRV_IDELE)
    {
        status = adf_adele_div_idele(z->a, x->a, y->i, prec);
        z->type = ADF_DRV_ADELE;
        return status;
    }
    return ADF_DOMAIN;
}

/* adf_drv_units_neg(z, x): the negation of a unit coset (the product with [-1]) and of an idele (every
   coordinate: the real ball, and the unit times [-1]; the content is positive and stays); a class has none. */
static int
adf_drv_units_neg(adf_drv_value * z, const adf_drv_value * x)
{
    adf_ucoset_t m;

    if (x->type == ADF_DRV_UCOSET)
    {
        adf_ucoset_init(m);
        adf_ucoset_minus_one(m);
        adf_ucoset_mul(z->u, x->u, m);
        adf_ucoset_clear(m);
        z->type = ADF_DRV_UCOSET;
        return ADF_OK;
    }
    if (x->type == ADF_DRV_IDELE)
    {
        arb_t inf;
        fmpq_t r;
        adf_ucoset_t u;
        int status;

        arb_init(inf);
        fmpq_init(r);
        adf_ucoset_init(u);
        adf_ucoset_init(m);
        arb_neg(inf, x->i->inf);
        /* This typed parser supplied a canonical idele, as for inf and u below.
           Copying the field avoids GCC 13's false array-parameter subobject overread. */
        fmpq_set(r, x->i->r);
        adf_ucoset_minus_one(m);
        adf_ucoset_mul(u, &x->i->u, m);
        status = adf_idele_set_parts(z->i, inf, r, u);
        arb_clear(inf);
        fmpq_clear(r);
        adf_ucoset_clear(u);
        adf_ucoset_clear(m);
        z->type = ADF_DRV_IDELE;
        return status;
    }
    return ADF_DOMAIN;
}

/* adf_drv_units_op(out, op, x, y, z, st): every command with an operand of one of the three kinds, and the new
   commands; step 5 and step 6 of the command.  Writes the line on ADF_OK. */
static int
adf_drv_units_op(FILE * out, adf_drv_op op, const adf_drv_value * x, const adf_drv_value * y, adf_drv_value * z,
                 adf_drv_state * st)
{
    int status = ADF_OK;
    slong k = 0;
    const char * text = "";

    switch (op)
    {
        case ADF_DRV_SHOW:
            return adf_drv_value_print(out, x, st->digits);
        case ADF_DRV_DUMP:
            return adf_drv_value_dump(out, x);
        case ADF_DRV_MUL:
        case ADF_DRV_DIV:
            status = adf_drv_units_mul(z, x, y, op == ADF_DRV_DIV, st->prec);
            break;
        case ADF_DRV_NEG:
            status = adf_drv_units_neg(z, x);
            break;
        case ADF_DRV_INV:
            z->type = x->type;
            if (x->type == ADF_DRV_RAT)
                status = adf_rat_inv(z->r, x->r);
            else if (x->type == ADF_DRV_UCOSET)
                adf_ucoset_inv(z->u, x->u);
            else if (x->type == ADF_DRV_IDELE)
                status = adf_idele_inv(z->i, x->i, st->prec);
            else if (x->type == ADF_DRV_IDCLASS)
                status = adf_idclass_inv(z->k, x->k, st->prec);
            else
                status = ADF_DOMAIN;
            break;
        case ADF_DRV_POW:
        case ADF_DRV_POWTIGHT:
        {
            int tight = (op == ADF_DRV_POWTIGHT);

            if (!adf_drv_is_unit_type(x->type))
                return ADF_DOMAIN;
            status = adf_drv_slong_operand(&k, y);
            if (status != ADF_OK)
                return status;
            z->type = x->type;
            if (x->type == ADF_DRV_UCOSET)
            {
                if (tight)
                    adf_ucoset_pow_tight(z->u, x->u, k);
                else
                    adf_ucoset_pow(z->u, x->u, k);
            }
            else if (x->type == ADF_DRV_IDELE)
                status = tight ? adf_idele_pow_tight(z->i, x->i, k, st->prec) : adf_idele_pow(z->i, x->i, k, st->prec);
            else
                status = tight ? adf_idclass_pow_tight(z->k, x->k, k, st->prec)
                               : adf_idclass_pow(z->k, x->k, k, st->prec);
            break;
        }
        case ADF_DRV_NORM:
        {
            arb_t t;

            if (x->type != ADF_DRV_IDELE && x->type != ADF_DRV_IDCLASS)
                return ADF_DOMAIN;
            arb_init(t);
            if (x->type == ADF_DRV_IDELE)
                status = adf_idele_norm(t, x->i, st->prec);
            else
                adf_idclass_norm(t, x->k);
            if (status == ADF_OK)
                status = adf_drv_arb_line(out, t, st->digits);
            arb_clear(t);
            return status;
        }
        case ADF_DRV_CLASS:
            if (x->type != ADF_DRV_IDELE)
                return ADF_DOMAIN;
            status = adf_idclass_set_idele(z->k, x->i, st->prec);
            z->type = ADF_DRV_IDCLASS;
            break;
        case ADF_DRV_MKIDELE:
            if (x->type != ADF_DRV_RAT)
                return ADF_DOMAIN;
            status = adf_idele_set_rat(z->i, x->r, st->prec);
            z->type = ADF_DRV_IDELE;
            break;
        case ADF_DRV_HULL:
        case ADF_DRV_HULLSIMPLE:
            if (x->type != ADF_DRV_IDELE)
                return ADF_DOMAIN;
            if (op == ADF_DRV_HULL)
                adf_adele_set_idele(z->a, x->i);
            else
                adf_adele_set_idele_simple(z->a, x->i);
            z->type = ADF_DRV_ADELE;
            break;
        case ADF_DRV_UNITOF:
            if (x->type != ADF_DRV_ADELE)
                return ADF_DOMAIN;
            status = adf_idele_set_adele(z->i, x->a);
            z->type = ADF_DRV_IDELE;
            break;
        case ADF_DRV_EQUAL:
        case ADF_DRV_CONTAINS:
        case ADF_DRV_OVERLAPS:
        {
            int r;

            if (x->type != ADF_DRV_UCOSET || y->type != ADF_DRV_UCOSET)
                return ADF_DOMAIN;
            if (op == ADF_DRV_EQUAL)
                r = adf_ucoset_equal_set(x->u, y->u);
            else if (op == ADF_DRV_CONTAINS)
                r = adf_ucoset_contains(x->u, y->u);
            else
                r = adf_ucoset_overlaps(x->u, y->u);
            text = r ? "true" : "false";
            fputs(text, out);
            fputc('\n', out);
            return ADF_OK;
        }
        default:            /* add, sub, cap, compare, reconstruct: not defined for these kinds */
            return ADF_DOMAIN;
    }
    if (status == ADF_OK)
        status = adf_drv_value_print(out, z, st->digits);
    return status;
}

/* adf_drv_valabs(out, op, l, st): valuation X with P and abs X with PLACE.  The second operand is a place, as for
   the commands at places (adf_drv_place_token): the steps 2 to 5 are those of adf_drv_places; the type of X is
   ADF_DOMAIN unless it is an idele. */
static int
adf_drv_valabs(FILE * out, adf_drv_op op, const adf_drv_line * l, adf_drv_state * st)
{
    adf_drv_value x;
    adf_text_kind kind;
    adf_place_t pl = adf_place_inf();
    int status, pst;

    adf_drv_value_init(&x);
    status = adf_text_classify(&kind, l->s[0], l->n[0], NULL);
    if (status != ADF_OK)
        goto done;
    pst = adf_drv_place_token(l->s[1], l->n[1], &pl);
    if (pst == ADF_PARSE)
    {
        status = ADF_PARSE;
        goto done;
    }
    if (adf_drv_kind_type(kind) == ADF_DRV_OTHER)
    {
        status = ADF_UNSUPPORTED;
        goto done;
    }
    status = adf_drv_value_read(&x, kind, l->s[0], l->n[0], st->prec);
    if (status != ADF_OK)
        goto done;
    if (pst != ADF_OK)
    {
        status = pst;
        goto done;
    }
    if (x.type != ADF_DRV_IDELE)
    {
        status = ADF_DOMAIN;
        goto done;
    }
    if (op == ADF_DRV_VALUATION)
    {
        slong v = 0;

        status = adf_idele_valuation_at(&v, x.i, pl);
        if (status == ADF_OK)
        {
            flint_fprintf(out, "%wd", v);
            fputc('\n', out);
        }
    }
    else if (adf_place_is_archimedean(pl))
    {
        arb_t a;

        arb_init(a);
        adf_idele_abs_inf(a, x.i);
        status = adf_drv_arb_line(out, a, st->digits);
        arb_clear(a);
    }
    else
    {
        adf_drv_value z;

        adf_drv_value_init(&z);
        status = adf_idele_abs_at(z.r, x.i, pl);
        z.type = ADF_DRV_RAT;
        if (status == ADF_OK)
            status = adf_drv_value_print(out, &z, st->digits);
        adf_drv_value_clear(&z);
    }

done:
    adf_drv_value_clear(&x);
    return status;
}

/* The root at all places (lane f-slice10, WP 1F.8; include/adelefeld/gfunc.h; tools/adf/README.md, "Roots at all
   places"):

     root X with N [with SIGN]

   X is a rational (adf_rat_root), an adele (adf_adele_root at the setting prec) or an idele (adf_idele_root at the
   setting prec); any other type is ADF_DOMAIN (a pair of types the operation does not define). N is the degree, an
   integer from 0 to 2^64 - 1; another value is ADF_DOMAIN (as the degree of roots_at). SIGN is the branch, +1 when
   it is omitted; an integer that fits an int is passed on as it is (the library answers DOMAIN for a value other
   than +1 and -1 when n >= 2), another value is ADF_DOMAIN. The line is the value text of the root, of the type of
   X, or "error: <STATUS>"; the place that the library reports is not printed. The steps are those at the top of
   the file: the three operands are read by the typed parsers in step 4, and everything here is step 5. */
static int
adf_drv_root_all(FILE * out, const adf_drv_value * x, const adf_drv_value * nv, const adf_drv_value * sv,
                 const adf_drv_state * st)
{
    adf_drv_value r;
    fmpz_t z;
    ulong n = 0;
    int sign = 1, status;

    fmpz_init(z);
    adf_drv_value_init(&r);
    status = adf_drv_int_value(z, nv);
    if (status == ADF_OK && (fmpz_sgn(z) < 0 || fmpz_cmp_ui(z, UWORD_MAX) > 0))
        status = ADF_DOMAIN;
    if (status == ADF_OK)
        n = fmpz_get_ui(z);
    if (status == ADF_OK && sv != NULL)
    {
        status = adf_drv_int_value(z, sv);
        if (status == ADF_OK && (fmpz_cmp_si(z, INT_MIN) < 0 || fmpz_cmp_si(z, INT_MAX) > 0))
            status = ADF_DOMAIN;
        if (status == ADF_OK)
            sign = (int) fmpz_get_si(z);
    }
    if (status == ADF_OK)
    {
        r.type = x->type;
        if (x->type == ADF_DRV_RAT)
            status = adf_rat_root(r.r, NULL, x->r, n, sign);
        else if (x->type == ADF_DRV_ADELE)
            status = adf_adele_root(r.a, NULL, x->a, n, sign, st->prec);
        else if (x->type == ADF_DRV_IDELE)
            status = adf_idele_root(r.i, NULL, x->i, n, sign, st->prec);
        else
            status = ADF_DOMAIN;
    }
    if (status == ADF_OK)
        status = adf_drv_value_print(out, &r, st->digits);
    adf_drv_value_clear(&r);
    fmpz_clear(z);
    return status;
}

/* The five factorial series at all places (lane f-slice10, WP 1F.8; include/adelefeld/gfunc.h; tools/adf/README.md,
   "The series at all places"):

     exp X, sin X, sinh X, cos X, cosh X

   X is an adele (adf_adele_exp ... at the setting prec) or a rational q, read as the adele (q ; q) at prec, as the
   commands at places do; any other type is ADF_DOMAIN. The line is the adele, or "error: <STATUS>"; the place that
   the library reports (the real place, or the first prime outside the domain) is not printed. Step 5. */
static int
adf_drv_series_all(FILE * out, adf_drv_op op, const adf_drv_value * x, const adf_drv_state * st)
{
    adf_drv_value r;
    adf_adele_t a;
    int status = ADF_OK;

    adf_drv_value_init(&r);
    adf_adele_init(a);
    if (x->type == ADF_DRV_RAT)
        adf_adele_set_rat(a, x->r, st->prec);
    else if (x->type == ADF_DRV_ADELE)
        adf_adele_set(a, x->a);
    else
        status = ADF_DOMAIN;
    if (status == ADF_OK)
    {
        r.type = ADF_DRV_ADELE;
        if (op == ADF_DRV_EXP)
            status = adf_adele_exp(r.a, NULL, a, st->prec);
        else if (op == ADF_DRV_SIN)
            status = adf_adele_sin(r.a, NULL, a, st->prec);
        else if (op == ADF_DRV_SINH)
            status = adf_adele_sinh(r.a, NULL, a, st->prec);
        else if (op == ADF_DRV_COS)
            status = adf_adele_cos(r.a, NULL, a, st->prec);
        else
            status = adf_adele_cosh(r.a, NULL, a, st->prec);
    }
    if (status == ADF_OK)
        status = adf_drv_value_print(out, &r, st->digits);
    adf_adele_clear(a);
    adf_drv_value_clear(&r);
    return status;
}

/* adf_drv_units_involved(op, x, y, w, nops): 1 for a new command, and for any command with an operand of one of the
   three kinds among its first nops operands. */
static int
adf_drv_units_involved(adf_drv_op op, const adf_drv_value * x, const adf_drv_value * y, const adf_drv_value * w,
                       int nops)
{
    switch (op)
    {
        case ADF_DRV_INV:
        case ADF_DRV_POW:
        case ADF_DRV_POWTIGHT:
        case ADF_DRV_NORM:
        case ADF_DRV_CLASS:
        case ADF_DRV_MKIDELE:
        case ADF_DRV_HULL:
        case ADF_DRV_HULLSIMPLE:
        case ADF_DRV_UNITOF:
            return 1;
        default:
            break;
    }
    return adf_drv_is_unit_type(x->type) || (nops > 1 && adf_drv_is_unit_type(y->type))
           || (nops > 2 && adf_drv_is_unit_type(w->type));
}

/* Idele Log (IL1-IL5). Place syntax uses the existing space-separated grammar.
   Syntax, kind, value, operation, printer run in the driver's documented order. */
static int
adf_drv_idlog(FILE *out, adf_drv_op op, const adf_drv_line *l, const adf_drv_state *st)
{
    adf_drv_value x, y;
    adf_sball_t s;
    adf_text_kind kind;
    adf_place_t v = adf_place_inf();
    slong n = 0;
    adf_place_t *places = NULL;
    int local = op == ADF_DRV_IDLOG_AT || op == ADF_DRV_IDLOGABS_AT;
    int refined = op == ADF_DRV_IDLOG_REFINE || op == ADF_DRV_IDLOGABS_REFINE;
    int status, place_status = ADF_OK;
    adf_drv_value_init(&x); adf_drv_value_init(&y); adf_sball_init(s);
    status = adf_text_classify(&kind, l->s[0], l->n[0], NULL);
    if (status != ADF_OK) goto done;
    if (local || refined)
    {
        if (!(refined && l->n[1] == 4 && memcmp(l->s[1], "none", 4) == 0))
        {
            places = flint_malloc((l->n[1] / 2 + 2) * sizeof(adf_place_t));
            place_status = adf_drv_place_list(l->s[1], l->n[1], places, &n);
        }
        if (place_status == ADF_PARSE || (local && n != 1)) { status = ADF_PARSE; goto done; }
        if (local) v = places[0];
    }
    if (adf_drv_kind_type(kind) == ADF_DRV_OTHER) { status = ADF_UNSUPPORTED; goto done; }
    status = adf_drv_value_read(&x, kind, l->s[0], l->n[0], st->prec);
    if (status != ADF_OK) goto done;
    if (place_status != ADF_OK) { status = place_status; goto done; }
    if (x.type != ADF_DRV_IDELE) { status = ADF_DOMAIN; goto done; }
    if (local)
    {
        status = op == ADF_DRV_IDLOG_AT ? adf_idele_Log_at(s,NULL,x.i,v,st->prec) :
                                       adf_idele_log_abs_at(s,NULL,x.i,v,st->prec);
        if (status == ADF_OK) status = adf_drv_put_sball(out,s,st->digits);
    }
    else
    {
        y.type = ADF_DRV_ADELE;
        if (refined)
            status = op == ADF_DRV_IDLOG_REFINE ?
                adf_idele_Log_refine(y.a,NULL,x.i,places,n,st->prec,st->prec) :
                adf_idele_log_abs_refine(y.a,NULL,x.i,places,n,st->prec,st->prec);
        else
            status = op == ADF_DRV_IDLOG ? adf_idele_Log(y.a,NULL,x.i,st->prec) :
                                        adf_idele_log_abs(y.a,NULL,x.i,st->prec);
        if (status == ADF_OK) status = adf_drv_value_print(out,&y,st->digits);
    }
done:
    flint_free(places); adf_sball_clear(s); adf_drv_value_clear(&x); adf_drv_value_clear(&y);
    return status;
}

/* ---- one command ---- */

/* WP 1F.9: Hilbert at one named place, on pairs of one supported input kind. */
static int
adf_drv_hilbert(FILE *out, const adf_drv_line *l, const adf_drv_state *st)
{
    adf_drv_value x[2];
    adf_lball_struct local[2];
    adf_sball_struct partial[2];
    arb_t ar,br;
    adf_text_kind kind[2];
    adf_place_t v=adf_place_inf();
    int status,z,place_status;
    for(int i=0;i<2;i++)
    {
        adf_drv_value_init(x+i); adf_lball_init(local+i); adf_sball_init(partial+i);
    }
    arb_init(ar); arb_init(br);
    for(int i=0;i<2;i++)
    {
        status=adf_text_classify(kind+i,l->s[i],l->n[i],NULL);
        if(status!=ADF_OK) goto done;
    }
    place_status=adf_drv_place_token(l->s[2],l->n[2],&v);
    if(place_status==ADF_PARSE) { status=place_status; goto done; }
    if(kind[0]!=kind[1]) { status=ADF_UNSUPPORTED; goto done; }
    if(kind[0]!=ADF_TEXT_RAT && kind[0]!=ADF_TEXT_IDELE &&
       kind[0]!=ADF_TEXT_LBALL && kind[0]!=ADF_TEXT_SBALL)
    { status=ADF_UNSUPPORTED; goto done; }
    for(int i=0;i<2;i++)
    {
        if(kind[i]==ADF_TEXT_LBALL)
            status=adf_lball_set_str(local+i,l->s[i],l->n[i],NULL);
        else if(kind[i]==ADF_TEXT_SBALL)
            status=adf_sball_set_str(partial+i,l->s[i],l->n[i],st->prec,NULL);
        else status=adf_drv_value_read(x+i,kind[i],l->s[i],l->n[i],st->prec);
        if(status!=ADF_OK) goto done;
    }
    if(place_status!=ADF_OK) { status=place_status; goto done; }
    if(kind[0]==ADF_TEXT_RAT) status=adf_rat_hilbert_at(&z,NULL,x[0].r,x[1].r,v);
    else if(kind[0]==ADF_TEXT_IDELE) status=adf_idele_hilbert_at(&z,NULL,x[0].i,x[1].i,v);
    else
    {
        if(kind[0]==ADF_TEXT_SBALL)
        {
            if(adf_place_is_archimedean(v))
            {
                status=adf_sball_get_arb(ar,partial,v);
                if(status==ADF_OK) status=adf_sball_get_arb(br,partial+1,v);
                if(status==ADF_OK) status=adf_real_hilbert(&z,NULL,ar,br);
                goto printed;
            }
            for(int i=0;i<2;i++)
            {
                status=adf_sball_get_lball(local+i,partial+i,v);
                if(status!=ADF_OK) goto done;
            }
        }
        if(!adf_place_equal(adf_lball_place(local),v) ||
           !adf_place_equal(adf_lball_place(local+1),v)) { status=ADF_DOMAIN; goto done; }
        status=adf_lball_hilbert(&z,NULL,local,local+1);
    }
printed:
    if(status==ADF_OK) fprintf(out,"%d\n",z);
done:
    arb_clear(ar); arb_clear(br);
    for(int i=0;i<2;i++)
    {
        adf_drv_value_clear(x+i); adf_lball_clear(local+i); adf_sball_clear(partial+i);
    }
    return status;
}

/* WP 1F.9, lane f-slice14: local_zeta_factor_at S with PLACE (localfactor.h; docs/api-1f9.md Y16, Y17). S is
   the text of a complex adele, the carrier of the complex number s: its complex coordinate is s and its finite
   coordinate is read and ignored (design local-zeta.md:394-397, :458-464). The order of the checks: the syntax
   of S, the syntax of the place token (PARSE); the kind of S, which must be a complex adele (UNSUPPORTED);
   the value of S; the place value (DOMAIN); the status of the library. The result is the plain complex ball,
   printed as the complex coordinate of the cadele printer, "(re) + (im)*i", without the finite part. */
static int
adf_drv_local_zeta(FILE *out, const adf_drv_line *l, const adf_drv_state *st)
{
    adf_text_kind kind;
    adf_cadele_t c;
    adf_fball_t zero;
    acb_t s, y;
    adf_place_t v = adf_place_inf();
    int status, place_status;
    char *text = NULL;
    size_t len = 0;
    adf_cadele_init(c); adf_fball_init(zero);
    acb_init(s); acb_init(y);
    status = adf_text_classify(&kind, l->s[0], l->n[0], NULL);
    if (status != ADF_OK) goto done;
    place_status = adf_drv_place_token(l->s[1], l->n[1], &v);
    if (place_status == ADF_PARSE) { status = place_status; goto done; }
    if (kind != ADF_TEXT_CADELE) { status = ADF_UNSUPPORTED; goto done; }
    status = adf_cadele_set_str(c, l->s[0], l->n[0], st->prec, NULL);
    if (status != ADF_OK) goto done;
    if (place_status != ADF_OK) { status = place_status; goto done; }
    adf_cadele_get_complex(s, c);
    status = adf_local_zeta_factor_at(y, NULL, s, v, st->prec);
    if (status != ADF_OK) goto done;
    status = adf_cadele_set_acb_fball(c, y, zero);
    if (status != ADF_OK) goto done;
    text = adf_cadele_get_str(&len, c, st->digits);
    if (text == NULL) { status = ADF_LIMIT; goto done; }
    /* "((re) + (im)*i ; 0)": drop the first byte and the final " ; 0)" */
    if (len < 7 || memcmp(text + len - 5, " ; 0)", 5) != 0) status = ADF_LIMIT;
    else fprintf(out, "%.*s\n", (int) (len - 6), text + 1);
done:
    adf_str_free(text);
    adf_cadele_clear(c); adf_fball_clear(zero);
    acb_clear(s); acb_clear(y);
    return status;
}

/* Slices 3.2-a to 3.2-c: Tate's character, printed like local_zeta_factor_at (docs/api-3.md 7, 3.1-3.3;
   conventions 6.1). psi and psi_strict take an adele, a class (lift text; the union text once slice 3.1-d
   reads it) or a local ball; psi_at and psi_strict_at an adele and a place token ("real" or a prime, as
   adf_drv_place_token); psi_phase prints the exact angle t of E(t) as a rational, or the status. */
static int
adf_drv_psi_print(FILE *out, const acb_t z, const adf_drv_state *st)
{
    adf_cadele_t c; adf_fball_t zero; char *text = NULL; size_t len = 0; int status;
    adf_cadele_init(c); adf_fball_init(zero);
    status = adf_cadele_set_acb_fball(c, z, zero);
    if (status == ADF_OK) {
        text = adf_cadele_get_str(&len, c, st->digits);
        if (text == NULL || len < 7 || memcmp(text+len-5, " ; 0)", 5)) status = ADF_LIMIT;
        else fprintf(out, "%.*s\n", (int) (len-6), text+1);
    }
    adf_str_free(text); adf_cadele_clear(c); adf_fball_clear(zero);
    return status;
}
static int
adf_drv_psi(FILE *out, adf_drv_op op, const adf_drv_line *l, const adf_drv_state *st)
{
    adf_text_kind kind; adf_drv_value x; adf_rat_t r; acb_t z; adf_place_t v = adf_place_inf();
    int status, place_status = ADF_OK, strict = op == ADF_DRV_PSI_STRICT || op == ADF_DRV_PSI_STRICT_AT;
    int at = op == ADF_DRV_PSI_AT || op == ADF_DRV_PSI_STRICT_AT;
    char *text; size_t len;
    adf_drv_value_init(&x); adf_rat_init(r); acb_init(z);
    status = adf_text_classify(&kind, l->s[0], l->n[0], NULL);
    if (status != ADF_OK) goto done;
    if (at) {
        place_status = adf_drv_place_token(l->s[1], l->n[1], &v);
        if (place_status == ADF_PARSE) { status = place_status; goto done; }
        if (kind != ADF_TEXT_ADELE) { status = ADF_UNSUPPORTED; goto done; }
    } else if (kind != ADF_TEXT_ADELE && kind != ADF_TEXT_QCLASS && kind != ADF_TEXT_LBALL &&
               (op != ADF_DRV_PSI_PHASE || kind != ADF_TEXT_FBALL)) { status = ADF_UNSUPPORTED; goto done; }
    status = adf_drv_value_read(&x, kind, l->s[0], l->n[0], st->prec);
    if (status != ADF_OK) goto done;
    if (place_status != ADF_OK) { status = place_status; goto done; }
    if (op == ADF_DRV_PSI_PHASE) {
        status = x.type == ADF_DRV_ADELE ? adf_adele_psi_tate_phase(r->q, x.a) :
                 x.type == ADF_DRV_QCLASS ? adf_qclass_psi_tate_phase(r->q, x.q) :
                 x.type == ADF_DRV_LBALL ? adf_lball_psi_tate_phase(r->q, x.b) :
                 adf_fball_psi_tate_phase(r->q, x.f);
        if (status != ADF_OK) goto done;
        text = adf_rat_get_str(&len, r);
        fprintf(out, "%.*s\n", (int) len, text); adf_str_free(text);
        goto done;
    }
    if (at) status = strict ? adf_adele_psi_tate_strict_at(z, NULL, x.a, v, st->prec) :
                              adf_adele_psi_tate_at(z, NULL, x.a, v, st->prec);
    else if (x.type == ADF_DRV_QCLASS)
        status = strict ? adf_qclass_psi_tate_strict(z, x.q, st->prec) : adf_qclass_psi_tate(z, x.q, st->prec);
    else if (x.type == ADF_DRV_LBALL)
        status = strict ? adf_lball_psi_tate_strict(z, x.b, st->prec) : adf_lball_psi_tate(z, x.b, st->prec);
    else
        status = strict ? adf_adele_psi_tate_strict(z, x.a, st->prec) : adf_adele_psi_tate(z, x.a, st->prec);
    if (status == ADF_OK) status = adf_drv_psi_print(out, z, st);
done:
    adf_drv_value_clear(&x); adf_rat_clear(r); acb_clear(z);
    return status;
}

/* Slice 4d (lane f4-slice2; docs/api-4.md 5, 6 and 9 item 4; include/adelefeld/rfun.h). The first operand is an
   rfun text. "rfun R" prints its canonical text; "rfun_translate R with Q" prints R(x - Q), Q an exact rational;
   "rfun_mul R with S" prints the product (pairs in lexicographic order); "rfun_eval R with X" prints the complex
   ball phi(X) as psi does, X a real ball of the production real of conventions 9.2, read by the adele reader
   as the text "(X ; 0)". Steps as for every command: the syntax of each operand, then its kind (another kind is
   DOMAIN, the pair of types), then the values, then the operation, then the printer (NULL: LIMIT). */
static int
adf_drv_rfun(FILE *out, adf_drv_op op, const adf_drv_line *l, const adf_drv_state *st)
{
    adf_rfun_t r, s, z; adf_rat_t q; adf_adele_t a; acb_t v; adf_text_kind kind;
    char *text = NULL, *real = NULL; size_t len = 0; int status, wrong = 0;
    adf_rfun_init(r); adf_rfun_init(s); adf_rfun_init(z); adf_rat_init(q); adf_adele_init(a); acb_init(v);
    status = adf_text_classify(&kind, l->s[0], l->n[0], NULL);
    if (status != ADF_OK) goto done;
    wrong = kind != ADF_TEXT_RFUN;
    if (op == ADF_DRV_RFUN_EVAL) {
        real = (char *) malloc(l->n[1] + 6);
        if (real == NULL) { status = ADF_LIMIT; goto done; }
        real[0] = '('; memcpy(real + 1, l->s[1], l->n[1]); memcpy(real + 1 + l->n[1], " ; 0)", 5);
        status = adf_text_classify(&kind, real, l->n[1] + 6, NULL);
        if (status != ADF_OK || kind != ADF_TEXT_ADELE) { status = ADF_PARSE; goto done; }
    } else if (op != ADF_DRV_RFUN) {
        status = adf_text_classify(&kind, l->s[1], l->n[1], NULL);
        if (status != ADF_OK) goto done;
        wrong |= kind != (op == ADF_DRV_RFUN_MUL ? ADF_TEXT_RFUN : ADF_TEXT_RAT);
    }
    if (wrong) { status = ADF_DOMAIN; goto done; }
    status = adf_rfun_set_str(r, l->s[0], l->n[0], st->prec, NULL);
    if (status != ADF_OK) goto done;
    if (op == ADF_DRV_RFUN_TRANSLATE) status = adf_rat_set_str(q, l->s[1], l->n[1], NULL);
    else if (op == ADF_DRV_RFUN_MUL) status = adf_rfun_set_str(s, l->s[1], l->n[1], st->prec, NULL);
    else if (op == ADF_DRV_RFUN_EVAL) status = adf_adele_set_str(a, real, l->n[1] + 6, st->prec, NULL);
    if (status != ADF_OK) goto done;
    if (op == ADF_DRV_RFUN_EVAL) {
        status = adf_rfun_eval(v, r, a->inf, st->prec);
        if (status == ADF_OK) status = adf_drv_psi_print(out, v, st);
        goto done;
    }
    if (op == ADF_DRV_RFUN) adf_rfun_swap(z, r);
    else if (op == ADF_DRV_RFUN_TRANSLATE) status = adf_rfun_translate_rat(z, r, q, st->prec);
    else status = adf_rfun_mul(z, r, s, st->prec);
    if (status != ADF_OK) goto done;
    text = adf_rfun_get_str(&len, z, st->digits);
    if (text == NULL) { status = ADF_LIMIT; goto done; }
    fprintf(out, "%.*s\n", (int) len, text);
done:
    adf_str_free(text); free(real);
    adf_rfun_clear(r); adf_rfun_clear(s); adf_rfun_clear(z); adf_rat_clear(q); adf_adele_clear(a); acb_clear(v);
    return status;
}

/* Slice 4e (lane f4-slice3; docs/api-4.md 5, 6 and 9 item 5; docs/api-4b.md "Slice 4e"). One rfun operand.
   "rfun_fourier R" and "rfun_derivative R" print the rfun text of F R and R'; "rfun_integral R" prints the
   complex ball of the integral as psi does; "rfun_norm2 R" prints the real ball of the squared L2 norm as the
   real part of an adele (the text "(X ; 0)" without its parentheses and " ; 0"). Steps: the syntax, the kind
   (another kind is DOMAIN), the value (the statuses of adf_rfun_set_str), the operation, the printer
   (NULL: LIMIT). */
static int
adf_drv_rfun4e(FILE *out, adf_drv_op op, const adf_drv_line *l, const adf_drv_state *st)
{
    adf_rfun_t r, z; acb_t v; arb_t n; adf_adele_t a; adf_fball_t zero; adf_text_kind kind;
    char *text = NULL; size_t len = 0; int status;
    adf_rfun_init(r); adf_rfun_init(z); acb_init(v); arb_init(n); adf_adele_init(a); adf_fball_init(zero);
    status = adf_text_classify(&kind, l->s[0], l->n[0], NULL);
    if (status != ADF_OK) goto done;
    if (kind != ADF_TEXT_RFUN) { status = ADF_DOMAIN; goto done; }
    status = adf_rfun_set_str(r, l->s[0], l->n[0], st->prec, NULL);
    if (status != ADF_OK) goto done;
    if (op == ADF_DRV_RFUN_INTEGRAL) {
        status = adf_rfun_integral(v, r, st->prec);
        if (status == ADF_OK) status = adf_drv_psi_print(out, v, st);
        goto done;
    }
    if (op == ADF_DRV_RFUN_NORM2) {
        status = adf_rfun_norm2(n, r, st->prec);
        if (status == ADF_OK) status = adf_adele_set_arb_fball(a, n, zero);
        if (status != ADF_OK) goto done;
        text = adf_adele_get_str(&len, a, st->digits);
        if (text == NULL || len < 7 || memcmp(text + len - 5, " ; 0)", 5)) { status = ADF_LIMIT; goto done; }
        fprintf(out, "%.*s\n", (int) (len - 6), text + 1);
        goto done;
    }
    status = op == ADF_DRV_RFUN_FOURIER ? adf_rfun_fourier(z, r, st->prec) : adf_rfun_derivative(z, r, st->prec);
    if (status != ADF_OK) goto done;
    text = adf_rfun_get_str(&len, z, st->digits);
    if (text == NULL) { status = ADF_LIMIT; goto done; }
    fprintf(out, "%.*s\n", (int) len, text);
done:
    adf_str_free(text);
    adf_rfun_clear(r); adf_rfun_clear(z); acb_clear(v); arb_clear(n); adf_adele_clear(a); adf_fball_clear(zero);
    return status;
}

/* Slice 4f (lane f4-slice5; docs/api-4.md 6 and 9 item 6; docs/api-4c.md "Slice 4f"; include/adelefeld/tensor.h).
     tensor_eval R with F with X          phi(x_inf) f(x_f) on the adele X, adf_tensor_eval
     tensor_eval_sball R with F with S    the partial-place evaluator on the sball S (E1 steps 4-5, D3)
     ffun_eval F with B                   the hull of F on the finite ball B (an fball text or a rational), E1 1-3
     ffun_integral F, ffun_norm2 F        (1/M) sum f[j], (1/M) sum |f[j]|^2
     tensor_integral R with F, tensor_norm2 R with F   the products of the real and finite integrals and norms
   R is an rfun text and F an ffun text. Complex results are printed as psi prints them; real results (the norms)
   as the real part of an adele, as rfun_norm2. Steps as for every command: the syntax of each operand (in
   order), then its kind (another kind is DOMAIN), then the values (the statuses of the readers), then the
   operation, then the printer (NULL: LIMIT). */
static int
adf_drv_tensor(FILE *out, adf_drv_op op, const adf_drv_line *l, const adf_drv_state *st)
{
    adf_text_kind want[3], kind; adf_rfun_t r; adf_ffun_t f; adf_fball_t b, zero; adf_adele_t a; adf_sball_t s;
    adf_rat_t q; acb_t v; arb_t n; char *text = NULL; size_t len = 0; int i, k = 0, status = ADF_OK, wrong = 0;
    int real = op == ADF_DRV_FFUN_NORM2 || op == ADF_DRV_TENSOR_NORM2;
    int rf = op == ADF_DRV_TENSOR_EVAL || op == ADF_DRV_TENSOR_EVAL_SBALL || op == ADF_DRV_TENSOR_INTEGRAL ||
             op == ADF_DRV_TENSOR_NORM2;
    adf_rfun_init(r); adf_ffun_init(f); adf_fball_init(b); adf_fball_init(zero); adf_adele_init(a);
    adf_sball_init(s); adf_rat_init(q); acb_init(v); arb_init(n);
    if (rf) want[k++] = ADF_TEXT_RFUN;
    want[k++] = ADF_TEXT_FFUN;
    if (op == ADF_DRV_TENSOR_EVAL) want[k++] = ADF_TEXT_ADELE;
    if (op == ADF_DRV_TENSOR_EVAL_SBALL) want[k++] = ADF_TEXT_SBALL;
    if (op == ADF_DRV_FFUN_EVAL) want[k++] = ADF_TEXT_FBALL;
    for (i = 0; i < k; i++) {
        status = adf_text_classify(&kind, l->s[i], l->n[i], NULL);
        if (status != ADF_OK) goto done;
        wrong |= kind != want[i] && !(want[i] == ADF_TEXT_FBALL && kind == ADF_TEXT_RAT);
        if (want[i] == ADF_TEXT_FBALL) want[i] = kind;
    }
    if (wrong) { status = ADF_DOMAIN; goto done; }
    for (i = 0; i < k && status == ADF_OK; i++) {
        if (want[i] == ADF_TEXT_RFUN) status = adf_rfun_set_str(r, l->s[i], l->n[i], st->prec, NULL);
        else if (want[i] == ADF_TEXT_FFUN) status = adf_ffun_set_str(f, l->s[i], l->n[i], st->prec, NULL);
        else if (want[i] == ADF_TEXT_ADELE) status = adf_adele_set_str(a, l->s[i], l->n[i], st->prec, NULL);
        else if (want[i] == ADF_TEXT_SBALL) status = adf_sball_set_str(s, l->s[i], l->n[i], st->prec, NULL);
        else if (want[i] == ADF_TEXT_FBALL) status = adf_fball_set_str(b, l->s[i], l->n[i], NULL);
        else {
            status = adf_rat_set_str(q, l->s[i], l->n[i], NULL);
            if (status == ADF_OK) adf_fball_set_rat(b, q);
        }
    }
    if (status != ADF_OK) goto done;
    if (op == ADF_DRV_TENSOR_EVAL) status = adf_tensor_eval(v, r, f, a, st->prec);
    else if (op == ADF_DRV_TENSOR_EVAL_SBALL) status = adf_tensor_eval_sball(v, r, f, s, st->prec);
    else if (op == ADF_DRV_FFUN_EVAL) status = adf_ffun_eval(v, f, b, st->prec);
    else if (op == ADF_DRV_FFUN_INTEGRAL) status = adf_ffun_integral(v, f, st->prec);
    else if (op == ADF_DRV_TENSOR_INTEGRAL) status = adf_tensor_integral(v, r, f, st->prec);
    else if (op == ADF_DRV_FFUN_NORM2) status = adf_ffun_norm2(n, f, st->prec);
    else status = adf_tensor_norm2(n, r, f, st->prec);
    if (status != ADF_OK) goto done;
    if (!real) { status = adf_drv_psi_print(out, v, st); goto done; }
    adf_adele_clear(a); adf_adele_init(a);
    status = adf_adele_set_arb_fball(a, n, zero);
    if (status != ADF_OK) goto done;
    text = adf_adele_get_str(&len, a, st->digits);
    if (text == NULL || len < 7 || memcmp(text + len - 5, " ; 0)", 5)) { status = ADF_LIMIT; goto done; }
    fprintf(out, "%.*s\n", (int) (len - 6), text + 1);
done:
    adf_str_free(text);
    adf_rfun_clear(r); adf_ffun_clear(f); adf_fball_clear(b); adf_fball_clear(zero); adf_adele_clear(a);
    adf_sball_clear(s); adf_rat_clear(q); acb_clear(v); arb_clear(n);
    return status;
}

/* WP 1F.9: exact scalar result, no value printer; symbol.h and docs/api-1f9.md Y1-Y3. */
static int
adf_drv_symbol(FILE *out, adf_drv_op op, const adf_drv_value *x, const adf_drv_value *y)
{
    adf_place_t p = adf_place_inf();
    int z, st;
    if (y->type != ADF_DRV_RAT) return ADF_UNSUPPORTED;
    if (!fmpz_is_one(fmpq_denref(y->r->q))) return ADF_DOMAIN;
    if (op == ADF_DRV_LEGENDRE)
    {
        const fmpz *b = fmpq_numref(y->r->q);
        if (fmpz_sgn(b) <= 0 || !fmpz_abs_fits_ui(b)) return ADF_DOMAIN;
        st = adf_place_prime(&p,fmpz_get_ui(b));
        if (st != ADF_OK) return st;
    }
    if (x->type == ADF_DRV_RAT)
    {
        if (!fmpz_is_one(fmpq_denref(x->r->q))) return ADF_DOMAIN;
        st = op == ADF_DRV_LEGENDRE ? adf_fmpz_legendre(&z,NULL,fmpq_numref(x->r->q),p) :
             op == ADF_DRV_JACOBI ? adf_fmpz_jacobi(&z,NULL,fmpq_numref(x->r->q),fmpq_numref(y->r->q)) :
             adf_fmpz_kronecker(&z,NULL,fmpq_numref(x->r->q),fmpq_numref(y->r->q));
    }
    else if (x->type == ADF_DRV_FBALL)
        st = op == ADF_DRV_LEGENDRE ? adf_fball_legendre(&z,NULL,x->f,p) :
             op == ADF_DRV_JACOBI ? adf_fball_jacobi(&z,NULL,x->f,fmpq_numref(y->r->q)) :
             adf_fball_kronecker(&z,NULL,x->f,fmpq_numref(y->r->q));
    else if (x->type == ADF_DRV_UCOSET)
        st = op == ADF_DRV_LEGENDRE ? adf_ucoset_legendre(&z,NULL,x->u,p) :
             op == ADF_DRV_JACOBI ? adf_ucoset_jacobi(&z,NULL,x->u,fmpq_numref(y->r->q)) :
             adf_ucoset_kronecker(&z,NULL,x->u,fmpq_numref(y->r->q));
    else return ADF_UNSUPPORTED;
    if (st == ADF_OK) fprintf(out,"%d\n",z);
    return st;
}

/* WP 1F.9, catalogue.h: k is a nonnegative integer, integral finite input or exact integer. */
static int
adf_drv_binomial(FILE *out, adf_drv_op op, const adf_drv_value *x,
                 const adf_drv_value *k, adf_drv_value *z, adf_drv_state *state)
{
    adf_fball_t f;
    int st;
    ulong degree;
    if (k->type!=ADF_DRV_RAT) return ADF_UNSUPPORTED;
    if (!fmpz_is_one(fmpq_denref(k->r->q)) || fmpq_sgn(k->r->q)<0) return ADF_DOMAIN;
    if (!fmpz_abs_fits_ui(fmpq_numref(k->r->q))) return ADF_LIMIT;
    degree=fmpz_get_ui(fmpq_numref(k->r->q));
    if (x->type!=ADF_DRV_RAT && x->type!=ADF_DRV_FBALL) return ADF_UNSUPPORTED;
    adf_fball_init(f);
    if (x->type==ADF_DRV_RAT) adf_fball_set_rat(f,x->r);
    else adf_fball_set(f,x->f);
    st=op==ADF_DRV_BINOM ? adf_fball_binom(z->f,NULL,f,degree) :
                          adf_fball_binom_tight(z->f,NULL,f,degree);
    adf_fball_clear(f);
    z->type=ADF_DRV_FBALL;
    if (st==ADF_OK) st=adf_drv_value_print(out,z,state->digits);
    return st;
}

/* Continuous profinite exponent, independent of the signed-word integer pow command. */
static int
adf_drv_profpow(FILE *out, adf_drv_op op, const adf_drv_value *a,
                const adf_drv_value *x, adf_drv_value *z, adf_drv_state *state)
{
    adf_fball_t exponent;
    int status;
    if (a->type!=ADF_DRV_UCOSET || (x->type!=ADF_DRV_RAT && x->type!=ADF_DRV_FBALL))
        return ADF_UNSUPPORTED;
    adf_fball_init(exponent);
    if (x->type==ADF_DRV_RAT) adf_fball_set_rat(exponent,x->r);
    else adf_fball_set(exponent,x->f);
    if (op==ADF_DRV_PROFPOW) status=adf_ucoset_profpow(z->u,NULL,a->u,exponent);
    else if (op==ADF_DRV_PROFPOWCOARSE) status=adf_ucoset_profpow_coarse(z->u,NULL,a->u,exponent);
    else status=adf_ucoset_profpow_fine(z->u,NULL,a->u,exponent);
    adf_fball_clear(exponent);
    z->type=ADF_DRV_UCOSET;
    if (status==ADF_OK) status=adf_drv_value_print(out,z,state->digits);
    return status;
}

/* Reuse the existing exact Haar-volume accessor, catalogue Proposition 10. */
static int
adf_drv_volume(FILE *out, const adf_drv_value *x, adf_drv_value *z, adf_drv_state *state)
{
    adf_fball_t f;
    if (x->type!=ADF_DRV_RAT && x->type!=ADF_DRV_FBALL) return ADF_UNSUPPORTED;
    adf_fball_init(f);
    if (x->type==ADF_DRV_RAT) adf_fball_set_rat(f,x->r);
    else adf_fball_set(f,x->f);
    adf_fball_haar_volume(z->r,f);
    adf_fball_clear(f);
    z->type=ADF_DRV_RAT;
    return adf_drv_value_print(out,z,state->digits);
}

/* Return an exact exponent modulo the order; two names fixed by CV-53/M0-D10. */
static int
adf_drv_cyclo(FILE *out, adf_drv_op op, const adf_drv_value *x, const adf_drv_value *n)
{
    fmpz_t j;
    int status;
    if (x->type!=ADF_DRV_IDCLASS || n->type!=ADF_DRV_RAT) return ADF_UNSUPPORTED;
    if (!fmpz_is_one(fmpq_denref(n->r->q))) return ADF_DOMAIN;
    fmpz_init(j);
    status=op==ADF_DRV_CYCLO_EXP_U ? adf_idclass_cyclo_exp_u(j,NULL,x->k,fmpq_numref(n->r->q)) :
                                  adf_idclass_cyclo_exp_uinv(j,NULL,x->k,fmpq_numref(n->r->q));
    if (status==ADF_OK) { fmpz_fprint(out,j); fputc('\n',out); }
    fmpz_clear(j);
    return status;
}

/* Slices a and b, api-3c 7: explicit character commands. Classification precedes value semantics. */
static char *adf_drv_char_complex(size_t *len, const acb_t z, slong digits)
{
    adf_cadele_t c; adf_fball_t zero; char *text;
    adf_cadele_init(c); adf_fball_init(zero);
    if (adf_cadele_set_acb_fball(c, z, zero) != ADF_OK) text = NULL;
    else text = adf_cadele_get_str(len, c, digits);
    if (text != NULL) {
        if (*len < 7 || memcmp(text+*len-5, " ; 0)", 5)) { adf_str_free(text); text = NULL; }
        else { *len -= 6; memmove(text, text+1, *len); text[*len] = '\0'; }
    }
    adf_cadele_clear(c); adf_fball_clear(zero); return text;
}
static int adf_drv_char(FILE *out, adf_drv_op op, const adf_drv_line *l, const adf_drv_state *state)
{
    adf_char_t x; adf_rat_t a; adf_ucoset_t u; acb_t tau, W; adf_text_kind kind[2];
    int unit = op == ADF_DRV_CHAR_UNIT || op == ADF_DRV_CHAR_UNIT_STRICT;
    char *text = NULL, *root = NULL; size_t len = 0, rootlen = 0; int status;
    adf_char_init(x); adf_rat_init(a); adf_ucoset_init(u); acb_init(tau); acb_init(W);
    status = adf_text_classify(kind, l->s[0], l->n[0], NULL);
    if (status != ADF_OK) goto done;
    if (op == ADF_DRV_CHI || unit) {
        status = adf_text_classify(kind+1, l->s[1], l->n[1], NULL);
        if (status != ADF_OK) goto done;
    }
    if (kind[0] != ADF_TEXT_CHAR || (op == ADF_DRV_CHI && kind[1] != ADF_TEXT_RAT) ||
        (unit && kind[1] != ADF_TEXT_UCOSET)) {
        status = ADF_UNSUPPORTED; goto done;
    }
    status = adf_char_set_str(x, l->s[0], l->n[0], state->prec, NULL);
    if (status != ADF_OK) goto done;
    if (op == ADF_DRV_CHAR || op == ADF_DRV_CHAR_CONJ) {
        if (op == ADF_DRV_CHAR_CONJ) adf_char_conj(x, x);
        text = adf_char_get_str(&len, x, state->digits);
        if (text == NULL) { status = ADF_LIMIT; goto done; }
        fprintf(out, "%s\n", text);
    } else if (op == ADF_DRV_CHI) {
        status = adf_rat_set_str(a, l->s[1], l->n[1], NULL);
        if (status != ADF_OK) goto done;
        if (!fmpz_is_one(fmpq_denref(a->q))) { status = ADF_DOMAIN; goto done; }
        status = adf_char_chi(tau, x, fmpq_numref(a->q), state->prec);
        if (status == ADF_OK) status = adf_drv_psi_print(out, tau, state);
    } else if (unit) {
        status = adf_ucoset_set_str(u, l->s[1], l->n[1], NULL);
        if (status != ADF_OK) goto done;
        status = op == ADF_DRV_CHAR_UNIT ? adf_char_eval_ucoset(tau, x, u, state->prec) :
                 adf_char_eval_ucoset_strict(tau, x, u, state->prec);
        if (status == ADF_OK) status = adf_drv_psi_print(out, tau, state);
    } else {
        status = adf_char_gauss_sum(tau, x, state->prec);
        if (status != ADF_OK) goto done;
        status = adf_char_root_number(W, x, state->prec);
        if (status != ADF_OK) goto done;
        text = adf_drv_char_complex(&len, tau, state->digits);
        root = adf_drv_char_complex(&rootlen, W, state->digits);
        if (text == NULL || root == NULL) { status = ADF_LIMIT; goto done; }
        fprintf(out, "e=%d tau=%s W=%s\n", adf_char_get_parity(x), text, root);
    }
done:
    adf_str_free(text); adf_str_free(root); adf_char_clear(x); adf_rat_clear(a); adf_ucoset_clear(u);
    acb_clear(tau); acb_clear(W); return status;
}

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
    if (op == ADF_DRV_CHAR || op == ADF_DRV_CHI || op == ADF_DRV_GAUSS || op == ADF_DRV_CHAR_CONJ ||
        op == ADF_DRV_CHAR_UNIT || op == ADF_DRV_CHAR_UNIT_STRICT)
        return adf_drv_char(out, op, l, st);
    if (op == ADF_DRV_HILBERT_AT) return adf_drv_hilbert(out,l,st);
    if (op == ADF_DRV_LOCAL_ZETA_AT) return adf_drv_local_zeta(out, l, st);
    if (op == ADF_DRV_PSI || op == ADF_DRV_PSI_STRICT || op == ADF_DRV_PSI_AT || op == ADF_DRV_PSI_STRICT_AT ||
        op == ADF_DRV_PSI_PHASE) return adf_drv_psi(out, op, l, st);
    if (op == ADF_DRV_RFUN || op == ADF_DRV_RFUN_TRANSLATE || op == ADF_DRV_RFUN_MUL || op == ADF_DRV_RFUN_EVAL)
        return adf_drv_rfun(out, op, l, st);
    if (op == ADF_DRV_RFUN_FOURIER || op == ADF_DRV_RFUN_DERIVATIVE || op == ADF_DRV_RFUN_INTEGRAL ||
        op == ADF_DRV_RFUN_NORM2) return adf_drv_rfun4e(out, op, l, st);
    if (op >= ADF_DRV_TENSOR_EVAL && op <= ADF_DRV_TENSOR_NORM2) return adf_drv_tensor(out, op, l, st);
    if (op == ADF_DRV_ROOTS || op == ADF_DRV_REALROOTS || op == ADF_DRV_RECOVER)
        return adf_drv_solver(out, op, l, st);
    if (op == ADF_DRV_PROJECT || op == ADF_DRV_EXP_AT || op == ADF_DRV_LOG_AT ||
        op == ADF_DRV_SIN_AT || op == ADF_DRV_COS_AT || op == ADF_DRV_SINH_AT || op == ADF_DRV_COSH_AT ||
        op == ADF_DRV_ROOT_AT || op == ADF_DRV_ROOTS_AT || op == ADF_DRV_POWRAT_AT || op == ADF_DRV_POWUNIT_AT)
        return adf_drv_places(out, op, l, st);
    if (op == ADF_DRV_IDLOG || op == ADF_DRV_IDLOGABS ||
        op == ADF_DRV_IDLOG_AT || op == ADF_DRV_IDLOGABS_AT ||
        op == ADF_DRV_IDLOG_REFINE || op == ADF_DRV_IDLOGABS_REFINE)
        return adf_drv_idlog(out, op, l, st);
    if (op == ADF_DRV_VALUATION || op == ADF_DRV_ABS)
        return adf_drv_valabs(out, op, l, st);

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
    /* Slice 4c: rfun dump uses the typed reader and the dump/load value slot.
       The other real-function commands retain their own operation dispatch above. */
    if (op == ADF_DRV_DUMP && kind[0] == ADF_TEXT_RFUN)
    {
        x.type = ADF_DRV_RFUN_VALUE;
        status = adf_rfun_set_str(x.rf, l->s[0], l->n[0], st->prec, NULL);
        if (status == ADF_OK) status = adf_drv_value_dump(out, &x);
        goto done;
    }
    /* step 3: the kind of every operand, in order; a kind with no typed parser in this build
       is ADF_UNSUPPORTED, and no value of the line is read before that is decided */
    for (i = 0; i < nops; i++)
    {
        /* Character arithmetic is deferred (api-3c 2). The generic character branches
           implement show/print and dump; the explicit calls above implement this slice. */
        if (kind[i] == ADF_TEXT_CHAR && op != ADF_DRV_SHOW && op != ADF_DRV_DUMP)
        {
            status = ADF_UNSUPPORTED;
            goto done;
        }
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

    /* Slices 4a/4b: typed finite function commands; docs/api-4.md section 9. */
    if (op>=ADF_DRV_FFUN_SHOW && op<=ADF_DRV_FFUN_CONJ) {
        if (x.type!=ADF_DRV_FFUN ||
            ((op==ADF_DRV_FFUN_ADD || op==ADF_DRV_FFUN_MUL) && y.type!=ADF_DRV_FFUN) ||
            ((op==ADF_DRV_FFUN_TRANSLATE || op==ADF_DRV_FFUN_DILATE) && y.type!=ADF_DRV_RAT) ||
            (op==ADF_DRV_FFUN_DILATE_IDELE && y.type!=ADF_DRV_IDELE)) status=ADF_DOMAIN;
        else if (op==ADF_DRV_FFUN_SHOW) status=adf_drv_value_print(out,&x,st->digits);
        else {
            switch (op) {
                case ADF_DRV_FFUN_ADD: status=adf_ffun_add(z.ff,x.ff,y.ff,st->prec); break;
                case ADF_DRV_FFUN_MUL: status=adf_ffun_mul(z.ff,x.ff,y.ff,st->prec); break;
                case ADF_DRV_FFUN_TRANSLATE: status=adf_ffun_translate_rat(z.ff,x.ff,y.r); break;
                case ADF_DRV_FFUN_DILATE: status=adf_ffun_dilate_rat(z.ff,x.ff,y.r); break;
                case ADF_DRV_FFUN_DILATE_IDELE: status=adf_ffun_dilate_idele(z.ff,x.ff,y.i); break;
                case ADF_DRV_FFUN_REFLECT: status=adf_ffun_reflect(z.ff,x.ff); break;
                case ADF_DRV_FFUN_CONJ: status=adf_ffun_conj(z.ff,x.ff); break;
                default: status=adf_ffun_fourier(z.ff,x.ff,st->prec); break;
            }
            if (status==ADF_OK) { z.type=ADF_DRV_FFUN; status=adf_drv_value_print(out,&z,st->digits); }
        }
        goto done;
    }
    /* Slice 3.1-e, docs/api-3.md 2.3,7: status plus truth, first class inside second.
       The exact query has no precision argument; st->prec affects only input reading. */
    if (op == ADF_DRV_QEQUAL || op == ADF_DRV_QCONTAINS || op == ADF_DRV_QOVERLAPS)
    {
        int truth;
        if (x.type != ADF_DRV_QCLASS || y.type != ADF_DRV_QCLASS || w.type != ADF_DRV_RAT ||
            !fmpz_is_one(fmpq_denref(w.r->q))) status = ADF_DOMAIN;
        else if (!fmpz_fits_si(fmpq_numref(w.r->q))) status = ADF_LIMIT;
        else {
            slong limit = fmpz_get_si(fmpq_numref(w.r->q));
            if (op == ADF_DRV_QEQUAL) status = adf_qclass_equal_set(&truth, x.q, y.q, limit);
            else if (op == ADF_DRV_QCONTAINS) status = adf_qclass_contains(&truth, x.q, y.q, limit);
            else status = adf_qclass_overlaps(&truth, x.q, y.q, limit);
            if (status == ADF_OK) fprintf(out, "%d\n", truth);
        }
        goto done;
    }
    /* docs/api-3.md 7: raw piece-count limit passed to algorithm R; integer conversion
       occurs only after the arbitrary-precision range check. */
    if (op == ADF_DRV_QREDUCE)
    {
        if (x.type != ADF_DRV_QCLASS || y.type != ADF_DRV_RAT ||
            !fmpz_is_one(fmpq_denref(y.r->q))) status = ADF_DOMAIN;
        else if (!fmpz_fits_si(fmpq_numref(y.r->q))) status = ADF_LIMIT;
        else {
            status = adf_qclass_reduce(z.q, x.q, fmpz_get_si(fmpq_numref(y.r->q)), st->prec);
            if (status == ADF_OK) {
                z.type = ADF_DRV_QCLASS;
                status = adf_drv_value_print(out, &z, st->digits);
            }
        }
        goto done;
    }
    /* Slice 3.1-f, docs/api-3.md 2.4 and 7: qneg Q with LIMIT, qadd Q1 with Q2 with LIMIT.
       The limit is read as for qreduce; the union is printed as qreduce prints it. */
    if (op == ADF_DRV_QNEG || op == ADF_DRV_QADD)
    {
        const adf_drv_value * lim = (op == ADF_DRV_QNEG) ? &y : &w;
        if (x.type != ADF_DRV_QCLASS || (op == ADF_DRV_QADD && y.type != ADF_DRV_QCLASS) ||
            lim->type != ADF_DRV_RAT || !fmpz_is_one(fmpq_denref(lim->r->q))) status = ADF_DOMAIN;
        else if (!fmpz_fits_si(fmpq_numref(lim->r->q))) status = ADF_LIMIT;
        else {
            slong limit = fmpz_get_si(fmpq_numref(lim->r->q));
            if (op == ADF_DRV_QNEG) status = adf_qclass_neg(z.q, x.q, limit, st->prec);
            else status = adf_qclass_add(z.q, x.q, y.q, limit, st->prec);
            if (status == ADF_OK) {
                z.type = ADF_DRV_QCLASS;
                status = adf_drv_value_print(out, &z, st->digits);
            }
        }
        goto done;
    }
    /* Quotient identity translation and the common pair domains. */
    if (op == ADF_DRV_QADD_RAT)
    {
        if (x.type != ADF_DRV_QCLASS || y.type != ADF_DRV_RAT) status = ADF_DOMAIN;
        else {
            adf_qclass_add_rat(z.q, x.q, y.r);
            z.type = ADF_DRV_QCLASS;
            status = adf_drv_value_print(out, &z, st->digits);
        }
        goto done;
    }
    if (nops == 2 && (x.type == ADF_DRV_QCLASS || y.type == ADF_DRV_QCLASS) &&
        (op == ADF_DRV_ADD || op == ADF_DRV_SUB || op == ADF_DRV_MUL || op == ADF_DRV_DIV ||
         op == ADF_DRV_EQUAL || op == ADF_DRV_CONTAINS || op == ADF_DRV_OVERLAPS || op == ADF_DRV_COMPARE))
    {
        status = ADF_DOMAIN;
        goto done;
    }

    if (op == ADF_DRV_ROOT)
    {
        status = adf_drv_root_all(out, &x, &y, nops == 3 ? &w : NULL, st);
        goto done;
    }
    if (op == ADF_DRV_BINOM || op == ADF_DRV_BINOMTIGHT)
    {
        status=adf_drv_binomial(out,op,&x,&y,&z,st);
        goto done;
    }
    if (op==ADF_DRV_PROFPOW || op==ADF_DRV_PROFPOWCOARSE || op==ADF_DRV_PROFPOWFINE)
    {
        status=adf_drv_profpow(out,op,&x,&y,&z,st);
        goto done;
    }
    if (op==ADF_DRV_HAAR_VOLUME)
    {
        status=adf_drv_volume(out,&x,&z,st);
        goto done;
    }
    if (op==ADF_DRV_CYCLO_EXP_U || op==ADF_DRV_CYCLO_EXP_UINV)
    {
        status=adf_drv_cyclo(out,op,&x,&y);
        goto done;
    }
    if (op == ADF_DRV_LEGENDRE || op == ADF_DRV_JACOBI || op == ADF_DRV_KRONECKER)
    {
        status = adf_drv_symbol(out,op,&x,&y);
        goto done;
    }
    if (op == ADF_DRV_EXP || op == ADF_DRV_SIN || op == ADF_DRV_SINH || op == ADF_DRV_COS || op == ADF_DRV_COSH)
    {
        status = adf_drv_series_all(out, op, &x, st);
        goto done;
    }
    if (adf_drv_units_involved(op, &x, &y, &w, nops))
    {
        /* the new commands, and every command with an operand of the kinds ucoset, idele, idclass */
        status = adf_drv_units_op(out, op, &x, &y, &z, st);
        goto done;
    }

    switch (op)
    {
        case ADF_DRV_SHOW:
            status = adf_drv_value_print(out, &x, st->digits);
            break;
        case ADF_DRV_NEG:
            /* the negation is defined for the four types of the top of the file and for the three
               kinds of milestone 2, which adf_drv_units_involved has taken; a local ball and a
               partial ball are a request on a combination this driver does not implement */
            if (x.type != ADF_DRV_RAT && x.type != ADF_DRV_FBALL && x.type != ADF_DRV_ADELE &&
                x.type != ADF_DRV_CADELE)
            {
                status = ADF_UNSUPPORTED;
                break;
            }
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
        else if (arity == ADF_DRV_ARITY_2_OR_3)
            status = (l.nops == 2 || l.nops == 3) ? ADF_OK : ADF_PARSE;
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

    /* Slice 3.1-d direct user call: adf print 'union((0.5 ; 7)) + Q'.
       Feed the same line reader as script input, preserving all status/length checks. */
    /* Direct slice-4a/4b calls use the same command parser as scripts. */
    if (argc>=3 && (!strcmp(argv[1],"ffun") || !strcmp(argv[1],"ffun_fourier") ||
                    !strcmp(argv[1],"ffun_add") || !strcmp(argv[1],"ffun_mul") ||
                    !strcmp(argv[1],"ffun_translate") || !strcmp(argv[1],"ffun_dilate") ||
                    !strcmp(argv[1],"ffun_dilate_idele") || !strcmp(argv[1],"ffun_reflect") ||
                    !strcmp(argv[1],"ffun_conj"))) {
        int add=!strcmp(argv[1],"ffun_add") || !strcmp(argv[1],"ffun_mul") ||
                !strcmp(argv[1],"ffun_translate") || !strcmp(argv[1],"ffun_dilate") ||
                !strcmp(argv[1],"ffun_dilate_idele");
        const char *second=NULL;
        if (!add && argc!=3) return adf_usage("one ffun operand required");
        if (add) {
            if (argc==4) second=argv[3];
            else if (argc==5 && !strcmp(argv[3],"with")) second=argv[4];
            else return adf_usage("ffun command requires two operands");
        }
        size_t opn=strlen(argv[1]), n=strlen(argv[2]), m=second ? strlen(second) : 0;
        if (opn+n+m+8>ADF_DRV_MAX_LINE) return adf_usage("command exceeds line limit");
        size_t size=opn+1+n+(second ? 6+m : 0)+1;
        char *line=malloc(size);
        if (line==NULL) return 2;
        memcpy(line,argv[1],opn); line[opn]=' '; memcpy(line+opn+1,argv[2],n);
        size_t pos=opn+1+n;
        if (second) { memcpy(line+pos," with ",6); pos+=6; memcpy(line+pos,second,m); pos+=m; }
        line[pos]='\n'; rc=adf_driver_run_v(line,size,stdout,0); free(line);
        if (fflush(stdout)!=0 || ferror(stdout)) rc=2;
        return rc;
    }
    if (argc == 3 && strcmp(argv[1], "print") == 0) {
        size_t n = strlen(argv[2]);
        char *line = malloc(n+7);
        if (line == NULL) return 2;
        memcpy(line, "print ", 6); memcpy(line+6, argv[2], n); line[n+6] = '\n';
        rc = adf_driver_run_v(line, n+7, stdout, 0); free(line);
        if (fflush(stdout) != 0 || ferror(stdout)) rc = 2;
        return rc;
    }

    /* Direct slice-a calls use the script parser and its existing " with " separator. */
    if ((argc == 3 && (!strcmp(argv[1], "char") || !strcmp(argv[1], "gauss") ||
                      !strcmp(argv[1], "char_conj"))) ||
        ((argc == 4 || (argc == 5 && !strcmp(argv[3], "with"))) &&
         (!strcmp(argv[1], "chi") || !strcmp(argv[1], "char_unit") || !strcmp(argv[1], "char_unit_strict")))) {
        const char *integer = argc == 3 ? NULL : argv[argc-1];
        size_t a = strlen(argv[1]), b = strlen(argv[2]), c = integer ? strlen(integer) : 0;
        char *line = malloc(a+b+c+9);
        if (line == NULL) return 2;
        memcpy(line, argv[1], a); line[a] = ' '; memcpy(line+a+1, argv[2], b);
        len = a+b+1;
        if (integer) { memcpy(line+len, " with ", 6); len += 6; memcpy(line+len, integer, c); len += c; }
        line[len++] = '\n'; rc = adf_driver_run_v(line, len, stdout, 0); free(line);
        if (fflush(stdout) != 0 || ferror(stdout)) rc = 2;
        return rc;
    }

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
