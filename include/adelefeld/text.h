/* adelefeld/text.h: the value form (parse and print) for adf_rat, adf_fball, adf_adele,
   adf_cadele; limits; classification of a text.

   Contract: docs/conventions.md 0.4, section 8 (interface 8.1, alphabet 8.2, tokens 8.3, limits
   8.4, order of checks 8.5), section 9 (grammar 9.1, 9.2; semantic constraints 9.3; printing
   templates 9.4; real balls 9.5; round trips 9.6; type of a text 9.7), 4.2 and 12.8 (string
   ownership); docs/SPEC.md 10, item 2 and item 4. Reference implementation: proto/text_grammar.py;
   golden vectors: tests/golden/{rat,fball,adele,cadele,dispatch,realball_read,realball_print}.tsv
   (conventions 11). Implemented in work package 1.4 (docs/PLAN.md section 6).

   Rules common to every parser adf_x_set_str (conventions 8.1, 8.5, 4.3):
   - The input is the len bytes at s. It need not be NUL-terminated; a NUL byte inside it is
     ADF_PARSE (CV-25). s is read during the call only and never retained (conventions 4.2).
     s may be NULL only when len = 0.
   - lim = NULL means the defaults of conventions 8.4 (adf_text_limits_default).
   - The first failing stage of conventions 8.5 determines the status: 1. len > max_len: ADF_LIMIT,
     before any byte is read; 2. a byte outside 8.2: ADF_PARSE; 3. grammar: ADF_PARSE; 4. limits on
     literals and counts: ADF_LIMIT; 5. word restrictions: ADF_UNSUPPORTED; 6. semantic constraints
     of 9.3: ADF_DOMAIN; 7. (types with a real part) a sign condition that fails only for the
     enclosure at prec: ADF_NOT_DETERMINED.
   - On every status other than ADF_OK the output value is untouched (conventions 4.3, 8.5).
   - On ADF_OK the output is canonical (conventions 4.4, last line) and is global: the value form
     does not record the backend (conventions 9.8, A11).
   - The typed parsers do not coerce between types: 7/3 is not an adf_fball text (conventions 9.7).

   Rules common to every printer adf_x_get_str (conventions 8.1, 9.4, 12.8; gate finding G14):
   - Returns a pointer to len + 1 bytes allocated with flint_malloc; *len receives the byte length.
     The len bytes are ASCII without NUL, without leading or trailing whitespace and without a
     newline; s[len] = 0 is not counted. The caller frees the pointer with adf_str_free.
   - The text is the canonical template of conventions 9.4 for the set of the value, whatever its
     backend (local values print through the canonical triple, conventions 5.3).
   - A printer has no status and never returns NULL; it allocates with flint_malloc, as
     fmpz_get_str does (fmpz.h:349). What flint_malloc does on exhaustion is FLINT's
     [source pending: FLINT 3.0.1 documentation of flint_malloc on allocation failure].
   - digits: the number of significant digits of the real-ball printer of conventions 9.5,
     1 <= digits <= 10^6; ADF_DIGITS_DEFAULT = 20 (conventions 8.1). Outside that range the
     behaviour is undefined (precondition). */

#ifndef ADELEFELD_TEXT_H
#define ADELEFELD_TEXT_H

#include "adelefeld/common.h"
#include "adelefeld/status.h"
#include "adelefeld/rat.h"
#include "adelefeld/fball.h"
#include "adelefeld/adele.h"

/* Defaults of conventions 8.4 (CV-27) and 8.1. */
#define ADF_TEXT_MAX_LEN_DEFAULT    ((size_t) 1048576)
#define ADF_TEXT_MAX_EXP10_DEFAULT  100000
#define ADF_TEXT_MAX_PREC_DEFAULT   100000
#define ADF_TEXT_MAX_ITEMS_DEFAULT  1048576
#define ADF_DIGITS_DEFAULT          20
#define ADF_DIGITS_MAX              1000000

#ifdef __cplusplus
extern "C" {
#endif

/* Limits (conventions 8.4). Layout, 64-bit: max_len (size_t) at 0, max_exp10 at 8, max_prec at 16,
   max_items at 24 (slong); size 32, alignment 8.
   max_len:   bytes of the whole input.
   max_exp10: absolute value of a decimal exponent.
   max_prec:  absolute value of N in O(p^N) and of the fields v, N of a dumped local ball.
   max_items: D M of ffun; the number of pieces, terms, places, polynomial coefficients, and the
              block count of every context occurrence (conventions 10.2). */
typedef struct
{
    size_t max_len;
    slong max_exp10;
    slong max_prec;
    slong max_items;
} adf_text_limits_t;

/* adf_text_limits_default(lim): writes the defaults of conventions 8.4 into *lim:
   (1048576, 100000, 100000, 1048576). Status: none. Header-inline and exported, so that a binding
   need not copy the constants (conventions 12.1). */
ADF_INLINE void
adf_text_limits_default(adf_text_limits_t * lim)
{
    lim->max_len = ADF_TEXT_MAX_LEN_DEFAULT;
    lim->max_exp10 = ADF_TEXT_MAX_EXP10_DEFAULT;
    lim->max_prec = ADF_TEXT_MAX_PREC_DEFAULT;
    lim->max_items = ADF_TEXT_MAX_ITEMS_DEFAULT;
}

/* The kind of a text (conventions 9.7; gate finding G14). One constant per start symbol of
   conventions 9.2, in the order of 9.7, with the values 0 to 12. An enum has the size and
   representation of int on the supported platforms (checked by tests/test_abi.c). */
typedef enum
{
    ADF_TEXT_RAT = 0,
    ADF_TEXT_FBALL = 1,
    ADF_TEXT_ADELE = 2,
    ADF_TEXT_CADELE = 3,
    ADF_TEXT_UCOSET = 4,
    ADF_TEXT_IDELE = 5,
    ADF_TEXT_IDCLASS = 6,
    ADF_TEXT_LBALL = 7,
    ADF_TEXT_SBALL = 8,
    ADF_TEXT_QCLASS = 9,
    ADF_TEXT_FFUN = 10,
    ADF_TEXT_RFUN = 11,
    ADF_TEXT_CHAR = 12
} adf_text_kind;

/* adf_text_classify(kind, s, len, lim): decides which start symbol of conventions 9.2 derives the
   text; the thirteen languages are pairwise disjoint (conventions 9.2, 9.7). Syntax only: stages
   1 to 3 of conventions 8.5.
   Status: ADF_OK, *kind written; ADF_LIMIT if len > max_len; ADF_PARSE if no start symbol derives
   the text (a forbidden byte included). *kind is written only on ADF_OK (conventions 12.3).
   Golden vectors: tests/golden/dispatch.tsv. Cost: linear in len. */
int adf_text_classify(adf_text_kind * kind, const char * s, size_t len, const adf_text_limits_t * lim);

/* ---- adf_rat: start symbol rat_v = rat (conventions 9.2); template q(x) (9.4) ---- */

/* adf_rat_set_str(x, s, len, lim). Statuses: ADF_OK, ADF_PARSE, ADF_LIMIT, ADF_DOMAIN (zero
   denominator, 9.3). Canonicalisation on input: leading zeros, reduction, -0 (9.3).
   Golden: tests/golden/rat.tsv. Cost: quasi-linear in len (a gcd of the parsed integers). */
int adf_rat_set_str(adf_rat_t x, const char * s, size_t len, const adf_text_limits_t * lim);

/* adf_rat_get_str(len, x): n or n/d in lowest terms, "-" only before a negative numerator, "0" for
   zero (conventions 9.4). */
char * adf_rat_get_str(size_t * len, const adf_rat_t x);

/* ---- adf_fball: start symbol fball_v (conventions 9.2); template (* ; F) (9.4) ---- */

/* adf_fball_set_str(x, s, len, lim). Accepts "(* ; F)" and the bare "a mod N" (CV-32).
   Statuses: ADF_OK, ADF_PARSE, ADF_LIMIT, ADF_DOMAIN (zero denominator). The result is global and
   canonical (centre reduced into [0, N), "mod 0" dropped; 9.3). Golden: tests/golden/fball.tsv. */
int adf_fball_set_str(adf_fball_t x, const char * s, size_t len, const adf_text_limits_t * lim);

/* adf_fball_get_str(len, x): "(* ; q(a))" if N = 0, else "(* ; q(a) mod q(N))" with a the centre
   in [0, N), from the canonical triple (conventions 9.4; A1, CV-31). */
char * adf_fball_get_str(size_t * len, const adf_fball_t x);

/* ---- adf_adele: start symbol adele_v = "(" real ";" fin ")"; template (r(x_inf) ; F) ---- */

/* adf_adele_set_str(x, s, len, prec, lim). The real ball m +/- r denotes the exact interval
   [m - r, m + r]; the stored arb contains it at prec, and equals it when m is dyadic with at most
   prec bits of odd mantissa and r is dyadic with odd mantissa below 2^30 (conventions 9.5,
   "Reading"; a tested requirement). Statuses: ADF_OK, ADF_PARSE, ADF_LIMIT (a decimal exponent
   above max_exp10), ADF_DOMAIN (zero denominator). adf_adele has no sign condition, so
   ADF_NOT_DETERMINED does not occur. Golden: tests/golden/adele.tsv, realball_read.tsv. */
int adf_adele_set_str(adf_adele_t x, const char * s, size_t len, slong prec,
                      const adf_text_limits_t * lim);

/* adf_adele_get_str(len, x, digits): "(r(x_inf) ; F)", r by the real-ball printer of conventions
   9.5 with n = digits (unconstrained), F as for adf_fball. The printed interval contains the arb
   (9.5, "Properties"). Re-reading the text in C encloses x; it need not be a fixed point (9.6,
   gate finding G4). Golden: tests/golden/adele.tsv, realball_print.tsv. */
char * adf_adele_get_str(size_t * len, const adf_adele_t x, slong digits);

/* ---- adf_cadele: start symbol cadele_v = "(" complex ";" fin ")"; template (z(x_inf) ; F) ---- */

/* adf_cadele_set_str(x, s, len, prec, lim): as adf_adele_set_str, with complex = "(" real ")" "+"
   "(" real ")" "*" "i" (conventions 9.2); each real part read as in 9.5.
   Statuses: ADF_OK, ADF_PARSE, ADF_LIMIT, ADF_DOMAIN. Golden: tests/golden/cadele.tsv. */
int adf_cadele_set_str(adf_cadele_t x, const char * s, size_t len, slong prec,
                       const adf_text_limits_t * lim);

/* adf_cadele_get_str(len, x, digits): "((r(re)) + (r(im))*i ; F)" (conventions 9.4). */
char * adf_cadele_get_str(size_t * len, const adf_cadele_t x, slong digits);

/* Layout queries for the limits struct (conventions 12.4). Header-inline and exported. */
ADF_INLINE size_t adf_sizeof_text_limits(void) { return sizeof(adf_text_limits_t); }
ADF_INLINE size_t adf_alignof_text_limits(void) { return ADF_ALIGNOF(adf_text_limits_t); }

#ifdef __cplusplus
}
#endif

#endif /* ADELEFELD_TEXT_H */
