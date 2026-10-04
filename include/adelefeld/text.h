/* adelefeld/text.h: the value form (parse and print) for adf_rat, adf_fball, adf_adele,
   adf_cadele, adf_ucoset, adf_idele and adf_idclass; limits; classification of a text.

   Contract: docs/conventions.md 0.4, section 8 (interface 8.1, alphabet 8.2, tokens 8.3, limits
   8.4, order of checks 8.5), section 9 (grammar 9.1, 9.2; semantic constraints 9.3; printing
   templates 9.4; real balls 9.5; round trips 9.6; type of a text 9.7), 4.2 and 12.8 (string
   ownership); docs/SPEC.md 10, item 2 and item 4. Reference implementation: proto/text_grammar.py;
   golden vectors: tests/golden/{rat,fball,adele,cadele,dispatch,realball_read,realball_print}.tsv
   (conventions 11). Implemented in work package 1.4 (docs/PLAN.md section 6). The unit coset, the
   idele and the idele class (conventions 5.6, 5.7, 9.2 ucoset_v, idele_v, idclass_v; golden vectors
   tests/golden/{ucoset,idele,idclass}.tsv) are added by lane t-slice1 (milestone 2); their
   functions are in src/text_idele.c and are declared at the end of this header. The local ball and
   the partial ball (conventions 5.8, 5.9, 9.2 lball_v, lcoord, sball_v, sentry; golden vectors
   tests/golden/{lball,sball}.tsv) are added by lane t-slice2; their functions are in
   src/text.c (its last section) and are declared in the block "adf_lball, adf_sball" below.

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
   - A printer has no status. It returns NULL, with *len = 0, in two cases only. (1) Decision M1-D6:
     the value has a real or complex part and the midpoint or the radius of one of its real
     balls is not zero and has a binary exponent (ARF_EXP, arf.h; MAG_EXP, mag.h) above
     ADF_PRINT_EXP_MAX in absolute value. The test is made before any conversion. (2) Decision
     N-D11 (review n-review1, D2): the constrained printing of the real part of an idele or a
     class (conventions 9.5) searches its level one by one, and stops without a text when the
     levels formed, weighted by the bit lengths of the numbers, pass a fixed bound (2^25 bit
     levels in src/text.c; about 300 levels at 10^5 bits); the ball 2^99999 + 1/2 +/- 2^99999
     is refused after about 1.2 s, and a ball of 4000 bits prints as before. Every other ball
     that satisfies (1) is printed by the same algorithm as before. The cost of a printer is
     therefore bounded by the sizes of the mantissas, by digits, by ADF_PRINT_EXP_MAX bits and
     by that bound. adf_rat_get_str and adf_fball_get_str never return NULL. adf_str_free(NULL) does
     nothing. Otherwise the printer allocates with flint_malloc, as fmpz_get_str does
     (fmpz.h:349). What flint_malloc does on exhaustion is FLINT's
     [source pending: FLINT 3.0.1 documentation of flint_malloc on allocation failure].
   - Reading a printed text back (conventions 9.6) needs limits that admit it: a value read at the
     limit max_exp10 may print with a decimal exponent one above it (review of milestone 1, text
     R3); the caller who reads printed text sets max_exp10 accordingly.
   - digits: the number of significant digits of the real-ball printer of conventions 9.5,
     1 <= digits <= 10^6; ADF_DIGITS_DEFAULT = 20 (conventions 8.1). Outside that range the
     behaviour is undefined (precondition). */

#ifndef ADELEFELD_TEXT_H
#define ADELEFELD_TEXT_H

#include "adelefeld/common.h"
#include "adelefeld/qclass.h"
#include "adelefeld/status.h"
#include "adelefeld/rat.h"
#include "adelefeld/fball.h"
#include "adelefeld/adele.h"
#include "adelefeld/ucoset.h"
#include "adelefeld/idele.h"
#include "adelefeld/idclass.h"
#include "adelefeld/lball.h"
#include "adelefeld/sball.h"

/* Defaults of conventions 8.4 (CV-27) and 8.1. */
#define ADF_TEXT_MAX_LEN_DEFAULT    ((size_t) 1048576)
#define ADF_TEXT_MAX_EXP10_DEFAULT  100000
#define ADF_TEXT_MAX_PREC_DEFAULT   100000
#define ADF_TEXT_MAX_ITEMS_DEFAULT  1048576
#define ADF_DIGITS_DEFAULT          20
#define ADF_DIGITS_MAX              1000000
/* The largest absolute binary exponent of a midpoint or radius that a printer converts
   (decision M1-D6). The driver adf has used the same bound since M1-D1. */
#define ADF_PRINT_EXP_MAX 100000

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

/* Slice 3.1-a: read (r ; F) + Q as LIFT, enclosing the real interval at prec.
   Union syntax gives ADF_UNSUPPORTED after byte, grammar, exponent and count checks
   (conventions 8.5 stages 1-4), before value semantics. Full union input is slice 3.1-d.
   OK writes x; PARSE, LIMIT, UNSUPPORTED, DOMAIN leave x untouched.
   prec > ADF_REAL_PREC_MAX gives LIMIT first (api-3.md 1). Other real parsing and
   printing limits are those of adf_adele_set_str/get_str. max_items bounds union entries.
   Source: docs/api-3.md 2.5, 7; conventions 9.2, 9.4. */
int adf_qclass_set_str(adf_qclass_t x, const char *s, size_t len, slong prec,
                      const adf_text_limits_t *lim);

/* Allocate the LIFT value form (r ; F) + Q with flint_malloc; *len excludes NUL.
   Free with adf_str_free. NULL and *len=0 when not printable under M1-D6.
   In this slice PIECES also returns NULL and *len=0; its printer is slice 3.1-d.
   The printed lift encloses the stored set; value text is not lossless (conventions 9.6). */
char *adf_qclass_get_str(size_t *len, const adf_qclass_t x, slong digits);

/* ---- adf_cadele: start symbol cadele_v = "(" complex ";" fin ")"; template (z(x_inf) ; F) ---- */

/* adf_cadele_set_str(x, s, len, prec, lim): as adf_adele_set_str, with complex = "(" real ")" "+"
   "(" real ")" "*" "i" (conventions 9.2); each real part read as in 9.5.
   Statuses: ADF_OK, ADF_PARSE, ADF_LIMIT, ADF_DOMAIN. Golden: tests/golden/cadele.tsv. */
int adf_cadele_set_str(adf_cadele_t x, const char * s, size_t len, slong prec,
                       const adf_text_limits_t * lim);

/* adf_cadele_get_str(len, x, digits): "((r(re)) + (r(im))*i ; F)" (conventions 9.4). */
char * adf_cadele_get_str(size_t * len, const adf_cadele_t x, slong digits);

/* ---- adf_ucoset, adf_idele, adf_idclass (milestone 2, lane t-slice1) ----

   Grammar (conventions 9.2): ucoset_v = "[" int ["mod" uint] "]"; idele_v = "(" real ";" urat "*" ucoset
   ")"; idclass_v = "<" real ";" ucoset ">". Semantic constraints (9.3): a unit coset with N >= 1 needs
   gcd(c, N) = 1, with N = 0 or absent it needs c = 1 or c = -1 (ADF_DOMAIN); an idele needs a real interval
   that excludes 0 and a content r > 0 (ADF_DOMAIN); a class needs a real interval inside (0, infinity)
   (ADF_DOMAIN). Canonicalisation on input (9.3): leading zeros removed, the content reduced, the residue of
   the unit reduced into 1..N; the modulus stays as written (CV-17, so [5 mod 6] is stored (5, 6)), "mod 0"
   is dropped. Printing (9.4): the unit coset in its normal form (5.6), "[c mod N]" with 1 <= c <= N or "[1]"
   and "[-1]"; the idele "(r(x_inf) ; q(r) * U)"; the class "<r(t) ; U>", "q(r)" also for r = 1.

   Statuses of the two readers with a real part, adf_idele_set_str and adf_idclass_set_str: those of
   adf_adele_set_str (ADF_OK, ADF_PARSE, ADF_LIMIT, ADF_DOMAIN; the stages of conventions 8.5 in that order)
   and ADF_NOT_DETERMINED (stage 7): the exact decimal interval satisfies the sign condition and no ball at
   the working precision p = max(prec, 2) does. The reader first forms the enclosing ball of conventions 9.5
   ("Reading"; exact for a dyadic input that fits, so the tightness of 9.5 holds); if that ball does not
   satisfy the condition it forms the ball of the real kernel B of adelefeld/idele.h (api-2.md Statement E)
   from the end points of the exact interval rounded outwards at p bits, which fails only when the end
   points are more than p binades apart (ADF_NOT_DETERMINED). A prec above ADF_IDELE_PREC_MAX gives ADF_LIMIT,
   decided from prec alone before the text is read, as for every function of idele.h (the rule of that
   header; conventions 8.5 has no stage for it). The output is untouched on every status other than ADF_OK.
   adf_ucoset_set_str has no real part and no prec: its statuses are ADF_OK, ADF_PARSE, ADF_LIMIT (len >
   max_len only: an integer of the unit coset has no limit of its own, conventions 8.4), ADF_DOMAIN.

   Printers: the rules of adf_x_get_str above. The real part of an idele or a class is printed by the
   constrained printing of conventions 9.5 (the least k >= 2 for which the printed interval still excludes 0,
   respectively is positive; repeated until the text is a fixed point of printing). The printed interval
   contains the stored ball, so re-reading the text at a sufficient prec gives a ball that contains the
   original one (9.6). The printer returns NULL, *len = 0, under the same condition as adf_adele_get_str
   (decision M1-D6); adf_ucoset_get_str never returns NULL. The value must satisfy its predicate (5.6,
   5.7); otherwise the behaviour is undefined (conventions 4.4). digits: 1 <= digits <= 10^6. */

/* adf_ucoset_set_str(x, s, len, lim): x = the unit coset of the text, residue in 1..N, modulus as written.
   Golden: tests/golden/ucoset.tsv. */
int adf_ucoset_set_str(adf_ucoset_t x, const char * s, size_t len, const adf_text_limits_t * lim);

/* adf_ucoset_get_str(len, x): the normal form of x (conventions 5.6, 9.4): "[c mod N]" or "[1]", "[-1]". */
char * adf_ucoset_get_str(size_t * len, const adf_ucoset_t x);

/* adf_idele_set_str(x, s, len, prec, lim): x = the idele of the text at working precision prec (see above).
   Golden: tests/golden/idele.tsv. */
int adf_idele_set_str(adf_idele_t x, const char * s, size_t len, slong prec, const adf_text_limits_t * lim);

/* adf_idele_get_str(len, x, digits): "(r(x_inf) ; q(r) * U)", U the normal form of the unit of x. */
char * adf_idele_get_str(size_t * len, const adf_idele_t x, slong digits);

/* adf_idclass_set_str(x, s, len, prec, lim): x = the class of the text at working precision prec.
   Golden: tests/golden/idclass.tsv. */
int adf_idclass_set_str(adf_idclass_t x, const char * s, size_t len, slong prec, const adf_text_limits_t * lim);

/* adf_idclass_get_str(len, x, digits): "<r(t) ; U>", U the normal form of the unit of x. */
char * adf_idclass_get_str(size_t * len, const adf_idclass_t x, slong digits);

/* ---- adf_lball, adf_sball (milestone 1F types in the value form, lane t-slice2) ----

   Grammar (conventions 9.2, lines 1117 and 1127-1128):
       lball_v = "[" "p" "=" uint ":" lcoord "]"
       lcoord  = rat ["+" "O" "(" uint ["^" sint] ")"]
       sball_v = "{" [ sentry {";" sentry} "}"
       sentry  = "inf" ":" (real | complex)  |  "p" "=" uint ":" lcoord
   The label of the archimedean place is "inf" (seams R9, D8); a real entry and a complex entry are told
   apart by the syntax of the ball, and they are the two tags of adf_sball (conventions 5.9).

   Semantic constraints (conventions 9.3, lines 1155-1160): the prime p of a local entry satisfies p < 2^64
   (ADF_UNSUPPORTED) and p is prime (n_is_prime, ADF_DOMAIN); the base inside "O(...)" equals p, else
   ADF_DOMAIN; every "/" of the centre has a denominator that is not 0, else ADF_DOMAIN; a partial ball has
   at most one "inf" entry and no prime twice, else ADF_DOMAIN. abs(N) <= max_prec is a limit of stage 4
   (ADF_LIMIT, checked on the digit string before any number is formed, decision M1-D7).

   Canonicalisation on input (conventions 9.3, line 1174, and 5.8): a local coordinate with an O-term is
   stored as the canonical centre p^v u in [0, p^N) of conventions 5.8, by adf_lball_set_rat_ball (the
   statement L1 of docs/api-1f.md); "O(p)" is stored as N = 1 and printed "O(p^1)"; a centre without an
   O-term is the exact rational of Q_p as adf_lball_set_rat stores it (p^v u with u a unit at p); the
   entries of a partial ball are sorted into the canonical order of places (conventions 7: the
   archimedean place first, then the primes increasing), which is the storage order of 5.9.

   Printing (conventions 9.4, lines 1206-1207): the local coordinate L is "q(p^v u)" when it is exact and
   "q(c) + O(p^N)" otherwise, with c the canonical centre as an integer and N in signed decimal; adf_lball is
   "[p=P: L]" with P in decimal. adf_sball is "{E; E; ...}" in the canonical order of places, E is
   "inf: r(x)" for the real tag, "inf: z(x)" for the complex tag, and "p=P: L" at a prime; a partial ball
   with no place is "{}". r is the printing of a real ball of conventions 9.5 with n = digits (no
   constraint on its sign: the archimedean component of a partial ball has none) and z(x) is
   "(r(re)) + (r(im))*i".

   The real ball of an "inf" entry (conventions 9.5, "Reading"): the entry denotes the exact interval
   [m - r, m + r]; the stored arb contains it at working precision prec and equals it when m is dyadic with
   at most prec bits of odd mantissa and r is dyadic with odd mantissa below 2^30. There is no sign
   condition, so ADF_NOT_DETERMINED (stage 7 of conventions 8.5) cannot occur. A prec above
   ADF_REAL_PREC_MAX (sball.h) is ADF_LIMIT, decided from prec alone before the text is read, as every
   function of prec in sball.h decides it (conventions 8.5 has no stage for it). A prec below 2 is taken as
   2 (decision M1-D4, conventions 8.1).

   Two limits of the library, both ADF_LIMIT, are in addition to those of conventions 8.4: (1) abs(N) above
   ADF_LBALL_EXP_MAX (the bound of lball.h, checked on the digit string after max_prec, so that N fits a
   slong); (2) a canonical centre that would need a power p^k with k bits(p) > ADF_LBALL_BITS_MAX
   (adf_lball_set_rat_ball returns ADF_LIMIT for it). Both are decided before anything is stored.

   On every status other than ADF_OK the output value is untouched (conventions 4.3, 8.5). s is read during
   the call only and never retained; lim = NULL means the defaults of conventions 8.4. The readers write a
   canonical value and every output satisfies its predicate (conventions 5.8, 5.9). Aliasing: there is
   none to state, the value is the only object of its type in the call.

   Printers (the rules of adf_x_get_str above): the caller frees the string with adf_str_free.
   adf_lball_get_str returns NULL with *len = 0 when the exact value needs a power p^|v| with |v| bits(p) >
   ADF_LBALL_BITS_MAX (adf_lball_get_center returns ADF_LIMIT; the centre of a ball is the stored integer
   and is never refused). adf_sball_get_str returns NULL with *len = 0 when a real or complex component of
   the partial ball is not printable (decision M1-D6, the same condition as adf_adele_get_str). digits:
   1 <= digits <= 10^6, ADF_DIGITS_DEFAULT = 20.

   Round trips (conventions 9.6): adf_lball has no real part, so parse(print(v)) = v as a set for every v
   the printer admits, and print(parse(t)) is canonical. For adf_sball without an archimedean place the same
   holds. With an archimedean place, parse(print(v)) is a partial ball whose component contains the
   component of v, and the text of a printed value read back and printed again is a fixed point when the
   real ball is read at a prec that admits the printed text (the value form of a real ball is a decimal
   enclosure, not a lossless form: docs/SPEC.md 10.2, gate finding G4). Golden vectors:
   tests/golden/lball.tsv, tests/golden/sball.tsv. */

/* adf_lball_set_str(x, s, len, lim): x = the local ball of the text "[p=P: L]" at the prime P.
   Statuses: ADF_OK, x written;
   ADF_PARSE (the grammar; also a forbidden byte and an embedded NUL, stages 2 and 3 of conventions 8.5);
   ADF_LIMIT (len > max_len, stage 1; abs(N) > max_prec, stage 4; abs(N) > ADF_LBALL_EXP_MAX; a centre that
   needs too large a power, from adf_lball_set_rat_ball);
   ADF_UNSUPPORTED (P >= 2^64, stage 5);
   ADF_DOMAIN (P is not prime; the base inside "O(...)" is not P; a denominator of the centre is 0,
   stage 6).
   The output is untouched on every status other than ADF_OK. Cost: one n_is_prime, one primality test of
   the centre. */
int adf_lball_set_str(adf_lball_t x, const char * s, size_t len, const adf_text_limits_t * lim);

/* adf_lball_get_str(len, x): "[p=P: L]" of conventions 9.4, L the canonical centre of 5.8 (the exact value
   q(p^v u) when x is exact). Never NULL except for the centre that does not fit (see above). */
char * adf_lball_get_str(size_t * len, const adf_lball_t x);

/* adf_sball_set_str(x, s, len, prec, lim): x = the partial ball of the text at working precision prec for
   its archimedean component (see above).
   Statuses: ADF_OK, x written;
   ADF_LIMIT (prec > ADF_REAL_PREC_MAX, decided first; len > max_len; more than max_items entries; a decimal
   exponent above max_exp10; abs(N) above max_prec or above ADF_LBALL_EXP_MAX);
   ADF_PARSE (stage 2 and stage 3);
   ADF_UNSUPPORTED (a prime p >= 2^64 in any entry, stage 5);
   ADF_DOMAIN (a prime that is not prime; a base inside "O(...)" different from its p; a denominator of a
   centre that is 0; two "inf" entries; the same prime twice, stage 6).
   The output is untouched on every status other than ADF_OK. Cost: one n_is_prime per entry, one
   allocation of len elements, one sort. */
int adf_sball_set_str(adf_sball_t x, const char * s, size_t len, slong prec, const adf_text_limits_t * lim);

/* adf_sball_get_str(len, x, digits): "{E; E; ...}" in the canonical order of places (conventions 7), E as
   above; "{}" when x has no place. NULL with *len = 0 when a component is not printable (M1-D6). */
char * adf_sball_get_str(size_t * len, const adf_sball_t x, slong digits);

/* Layout queries for the limits struct (conventions 12.4). Header-inline and exported. */
ADF_INLINE size_t adf_sizeof_text_limits(void) { return sizeof(adf_text_limits_t); }
ADF_INLINE size_t adf_alignof_text_limits(void) { return ADF_ALIGNOF(adf_text_limits_t); }

/* The same for the kind written by adf_text_classify (conventions 9.7, 12.4): an enum, 4 bytes on the
   platform of conventions 12.10; a binding allocates it inline and compares these two numbers. */
ADF_INLINE size_t adf_sizeof_text_kind(void) { return sizeof(adf_text_kind); }
ADF_INLINE size_t adf_alignof_text_kind(void) { return ADF_ALIGNOF(adf_text_kind); }

#ifdef __cplusplus
}
#endif

#endif /* ADELEFELD_TEXT_H */
