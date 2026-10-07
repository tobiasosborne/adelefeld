/* adelefeld/dump.h: the dump form (version 1, field Q) for adf_rat, adf_fball, adf_scaled,
   adf_adele, adf_cadele: loaders with context bindings, dumpers, inspection of context occurrences.

   Contract: docs/conventions.md 0.4, section 10 (grammar 10.1; rules 10.2: strict loader CV-38,
   real balls as arb_dump_str writes them, validation of the whole text before any FLINT load
   function CV-52, context bindings of gate finding G3, descriptors and inspection of closure
   finding C2), 8.1 (interface), 8.4, 8.5 (limits, order of checks), 4.3, 12.8; docs/SPEC.md 10
   item 2 (M0-D9). Golden vectors: tests/golden/dump.tsv (conventions 11.3 item 5). Reference:
   proto/text_grammar.py. Implemented in work package 1.4 (docs/PLAN.md section 6).

   The ten bodies used here (conventions 10.1): "rat", "fball" (form "g" global: A H d; form "l"
   local: d, context, residues), "scaled" (form "x" exact: num den; form "s": num(s) den(s) u; then
   the context), "adele" (archimedean count 1, one arb, one fball), "cadele" (count 1, one acb, one
   fball), "ucoset" (c N), "idele" (count 1, one arb, num(r) den(r), c N), "idclass" (one arb, c N),
   "lball" (one lb) and "sball" (the tag n, r or c, the count, then that many lb). Every dump starts
   "adf1 Q ". The five bodies of the second group (conventions 5.6 to 5.9) hold no context
   occurrence, so the ctx argument of their one-context loaders is accepted and ignored, like that of
   adf_rat above. An idele, a class and a partial ball need no working precision: the real ball and
   the complex ball are stored in the dump exactly, as arb_dump_str writes them (conventions 10.2).

   Rules common to the loaders (conventions 10.2, 8.5, 4.3):
   - Input (s, len) as for the value form (adelefeld/text.h); lim = NULL means the defaults.
   - Strict: only canonical text is accepted; the fields must satisfy the predicates of conventions
     section 5 (for a local fball the raw predicate L, CV-55). The loader never canonicalises.
   - The whole text is validated before any FLINT load function sees any of it (CV-52, D9); an arb
     field is accepted only if both mantissas are odd or the pair is "0 0", and the radius mantissa
     is positive and below 2^30 (mag.h:117); anything else is ADF_DOMAIN.
   - Statuses: ADF_OK, ADF_PARSE, ADF_LIMIT, ADF_UNSUPPORTED (a version other than 1, a field other
     than Q), ADF_DOMAIN (conventions 3.2 row "Loaders of the dump form"; order of 8.5). On every
     status other than ADF_OK the output value is untouched.
   - Every function that validates a dump validates every body of conventions 10.1, also the
     bodies without a loader here. For the body "qclass" of the form "pieces" the binary exponents
     of the real ball of every piece are limited (decision M1-D9, conventions 8.4): an exponent of
     the midpoint or of the radius above ADF_DUMP_QCLASS_EXP_MAX in absolute value is ADF_LIMIT,
     at stage 4 of conventions 8.5, decided on the digit strings. The form "lift" and the other
     bodies have no such limit. A context occurrence of more than ADF_MODCTX_MAX_BLOCKS blocks is
     ADF_UNSUPPORTED at stage 5 (decision M1-D5, adelefeld/modctx.h).
   - Context bindings (conventions 10.2, G3): there is one context occurrence per local fball and
     per scaled value, in dump traversal order. binds[i] is the caller-owned context for occurrence
     i; it must match that occurrence's modulus and ordered blocks (adf_modctx_matches_desc);
     repeated occurrences may share a pointer. A binding count other than the number of
     occurrences, a NULL binding, or a binding that does not match is ADF_DOMAIN. The loaded value
     borrows the bound contexts; value fields and backend are restored exactly relative to them.
     Loading and then adf_x_identical with the original value requires binding each occurrence to
     its original context pointer.
   - adf_x_load_str(x, s, len, ctx, lim) is the one-context convenience: it stands for binds with
     ctx at every occurrence; for a dump without occurrences any ctx, NULL included, is accepted.

   Rules common to the dumpers (conventions 8.1, 10.2): the text of conventions 10.1 for the stored
   data (raw local data, context K, k, q_1..q_k); allocated with flint_malloc, byte length in *len,
   NUL at s[len], freed with adf_str_free; never fails. Loading the text with the value's own
   contexts gives an identical value, and dumping it again gives the same bytes (CV-38).

   Rules common to the inspectors adf_x_dump_inspect(nctx, descs, s, len, lim) (conventions 10.2,
   closure finding C2, verbatim): "With descs=NULL, ignore the incoming *nctx and write the
   occurrence count only on OK. Otherwise incoming *nctx is capacity in initialized descriptors.
   Validate the whole dump first; insufficient capacity returns ADF_LIMIT with *nctx and every
   descriptor untouched. On OK replace the first required descriptors, releasing their old contents,
   write that count to *nctx, and leave the rest untouched. Other failures also leave all outputs
   untouched. Statuses are loader statuses." No value and no context is constructed. */

#ifndef ADELEFELD_DUMP_H
#define ADELEFELD_DUMP_H

#include "adelefeld/common.h"
#include "adelefeld/status.h"
#include "adelefeld/rat.h"
#include "adelefeld/fball.h"
#include "adelefeld/adele.h"
#include "adelefeld/text.h"
#include "adelefeld/modctx.h"
#include "adelefeld/scaled.h"
#include "adelefeld/ucoset.h"
#include "adelefeld/idele.h"
#include "adelefeld/idclass.h"
#include "adelefeld/lball.h"
#include "adelefeld/sball.h"
#include "adelefeld/char.h"
#include "adelefeld/ffun.h"
#include "adelefeld/rfun.h"

/* The largest absolute binary exponent of the midpoint and of the radius of the real ball of a
   piece of a dumped qclass, form "pieces" (decision M1-D9): 2^20, the bound of ADF_RECON_EXP_MAX
   (decision M1-D3). It is not a field of adf_text_limits_t: a caller cannot change it. */
#define ADF_DUMP_QCLASS_EXP_MAX 1048576

#ifdef __cplusplus
extern "C" {
#endif

/* The binding array is declared `const adf_modctx_struct * const * binds`: the loader reads the
   array and never writes it (conventions 10.2 writes `const adf_modctx_struct ** binds`; the added
   const lets C callers pass an array of const pointers without a cast). nbinds is a size_t
   (conventions 12.10, closure C2). */

/* ---- adf_rat: body "rat num den" (no context occurrence) ---- */
int adf_rat_load_str(adf_rat_t x, const char * s, size_t len, const adf_modctx_struct * ctx,
                     const adf_text_limits_t * lim);
int adf_rat_load_str_binds(adf_rat_t x, const char * s, size_t len,
                           const adf_modctx_struct * const * binds, size_t nbinds,
                           const adf_text_limits_t * lim);
char * adf_rat_dump_str(size_t * len, const adf_rat_t x);
int adf_rat_dump_inspect(size_t * nctx, adf_ctx_desc_t * descs, const char * s, size_t len,
                         const adf_text_limits_t * lim);

/* ---- adf_fball: body "fball g A H d" or "fball l d K k q_1..q_k r_1..r_k" (0 or 1 occurrence) ---- */
int adf_fball_load_str(adf_fball_t x, const char * s, size_t len, const adf_modctx_struct * ctx,
                       const adf_text_limits_t * lim);
int adf_fball_load_str_binds(adf_fball_t x, const char * s, size_t len,
                             const adf_modctx_struct * const * binds, size_t nbinds,
                             const adf_text_limits_t * lim);
char * adf_fball_dump_str(size_t * len, const adf_fball_t x);
int adf_fball_dump_inspect(size_t * nctx, adf_ctx_desc_t * descs, const char * s, size_t len,
                           const adf_text_limits_t * lim);

/* ---- adf_scaled: body "scaled x num den ctx" or "scaled s num den u ctx" (1 occurrence).
   The value form does not record the policy; a scaled value is read from the value form by reading
   an adf_fball and converting it (conventions 9.4), so this is the only text reader of the type. ---- */
int adf_scaled_load_str(adf_scaled_t x, const char * s, size_t len, const adf_modctx_struct * ctx,
                        const adf_text_limits_t * lim);
int adf_scaled_load_str_binds(adf_scaled_t x, const char * s, size_t len,
                              const adf_modctx_struct * const * binds, size_t nbinds,
                              const adf_text_limits_t * lim);
char * adf_scaled_dump_str(size_t * len, const adf_scaled_t x);
int adf_scaled_dump_inspect(size_t * nctx, adf_ctx_desc_t * descs, const char * s, size_t len,
                            const adf_text_limits_t * lim);

/* adf_scaled_get_str(len, x): the value form of the finite ball that x denotes: "(* ; F)" with centre
   s u and radius s K, or "(* ; q(s))" if exact (conventions 9.4 row "scaled value"). */
char * adf_scaled_get_str(size_t * len, const adf_scaled_t x);

/* ---- adf_adele: body "adele 1 <arb> <fb>" (0 or 1 occurrence). For Q the archimedean count must
   be 1; another count is ADF_DOMAIN (conventions 10.1). ---- */
int adf_adele_load_str(adf_adele_t x, const char * s, size_t len, const adf_modctx_struct * ctx,
                       const adf_text_limits_t * lim);
int adf_adele_load_str_binds(adf_adele_t x, const char * s, size_t len,
                             const adf_modctx_struct * const * binds, size_t nbinds,
                             const adf_text_limits_t * lim);
char * adf_adele_dump_str(size_t * len, const adf_adele_t x);
int adf_adele_dump_inspect(size_t * nctx, adf_ctx_desc_t * descs, const char * s, size_t len,
                           const adf_text_limits_t * lim);

/* ---- adf_cadele: body "cadele 1 <acb> <fb>" (0 or 1 occurrence) ---- */
int adf_cadele_load_str(adf_cadele_t x, const char * s, size_t len, const adf_modctx_struct * ctx,
                        const adf_text_limits_t * lim);
int adf_cadele_load_str_binds(adf_cadele_t x, const char * s, size_t len,
                              const adf_modctx_struct * const * binds, size_t nbinds,
                              const adf_text_limits_t * lim);
char * adf_cadele_dump_str(size_t * len, const adf_cadele_t x);
int adf_cadele_dump_inspect(size_t * nctx, adf_ctx_desc_t * descs, const char * s, size_t len,
                            const adf_text_limits_t * lim);

/* ---- adf_ucoset: body "ucoset c N" (no context occurrence). The stored modulus is dumped as
   it is stored (conventions 10.2, "Unit moduli are dumped as stored", CV-17), so the dump of
   [5 mod 6] is "adf1 Q ucoset 5 6" and not the dump of its normal form. The predicate of
   conventions 5.6 is the only condition on the two fields: N >= 1 and 1 <= c <= N and
   gcd(c, N) = 1, or N = 0 and c = +1 or -1 (the exact units). No other status than the ones
   listed above can come from a field: a value of the library that satisfies the predicate gives a
   dump that loads back into an identical value. A text is never able to make the loader abort: the
   value is built in a temporary and exchanged into x on ADF_OK alone (conventions 4.3), and the
   predicate is asked of that temporary, so a text that defeats the check of stage 6 gets
   ADF_DOMAIN with x untouched. ---- */
int adf_ucoset_load_str(adf_ucoset_t x, const char * s, size_t len, const adf_modctx_struct * ctx,
                        const adf_text_limits_t * lim);
int adf_ucoset_load_str_binds(adf_ucoset_t x, const char * s, size_t len,
                              const adf_modctx_struct * const * binds, size_t nbinds,
                              const adf_text_limits_t * lim);
char * adf_ucoset_dump_str(size_t * len, const adf_ucoset_t x);
int adf_ucoset_dump_inspect(size_t * nctx, adf_ctx_desc_t * descs, const char * s, size_t len,
                            const adf_text_limits_t * lim);

/* ---- adf_idele: body "idele 1 <arb> num(r) den(r) c N" (no context occurrence). The
   archimedean count is 1 for Q; another count is ADF_DOMAIN (conventions 10.1, the same rule as
   for an adele). The predicate of conventions 5.7 is checked on the tokens: the real ball is a
   canonical arb that excludes 0 (midpoint of larger absolute value than the radius), the content
   is a canonical fmpq and r > 0, the unit satisfies 5.6. The ball is written and read exactly
   (arb_dump_str, conventions 10.2): the dump holds the ball, not an enclosure of it, and no prec
   enters. The ball excludes 0 exactly when |m| 2^me > rm 2^re; the two powers are compared on the
   digit strings, so an exponent of any length is decided, and a tie of the leading bits of the two
   mantissas in the same binade is the strict inequality it is (the longer mantissa is odd, so it
   has bits below the tie). The value is built in a temporary and exchanged into x on ADF_OK
   alone, so a text that defeats the check of stage 6 gets ADF_DOMAIN with x untouched instead of
   an abort. ---- */
int adf_idele_load_str(adf_idele_t x, const char * s, size_t len, const adf_modctx_struct * ctx,
                       const adf_text_limits_t * lim);
int adf_idele_load_str_binds(adf_idele_t x, const char * s, size_t len,
                             const adf_modctx_struct * const * binds, size_t nbinds,
                             const adf_text_limits_t * lim);
char * adf_idele_dump_str(size_t * len, const adf_idele_t x);
int adf_idele_dump_inspect(size_t * nctx, adf_ctx_desc_t * descs, const char * s, size_t len,
                           const adf_text_limits_t * lim);

/* ---- adf_idclass: body "idclass <arb> c N" (no context occurrence). The type is special to Q and
   its dump has no count (conventions 10.1). The predicate of conventions 5.7 is checked on the
   tokens: the real ball is a canonical arb every point of which is positive (midpoint positive and
   of larger absolute value than the radius), and the unit satisfies 5.6. As for the idele, the
   value is built in a temporary and exchanged into x on ADF_OK alone. ---- */
int adf_idclass_load_str(adf_idclass_t x, const char * s, size_t len, const adf_modctx_struct * ctx,
                         const adf_text_limits_t * lim);
int adf_idclass_load_str_binds(adf_idclass_t x, const char * s, size_t len,
                               const adf_modctx_struct * const * binds, size_t nbinds,
                               const adf_text_limits_t * lim);
char * adf_idclass_dump_str(size_t * len, const adf_idclass_t x);
int adf_idclass_dump_inspect(size_t * nctx, adf_ctx_desc_t * descs, const char * s, size_t len,
                             const adf_text_limits_t * lim);

/* ---- adf_lball: body "lball p (x num(u) den(u) v | b u v N)" (no context occurrence). The
   predicate of conventions 5.8 is checked on the tokens, primality of p included (n_is_prime,
   ulong_extras.h:335): p is a prime, the form is "x" or "b", the rational u of an exact value is
   canonical and p divides neither its numerator nor its denominator, a ball has v < N, an integer
   u prime to p and 0 < u < p^(N - v), and u = 0 goes with v = 0. |v| and |N| are bounded by
   max_prec (conventions 8.4, a limit of stage 4) and by ADF_LBALL_EXP_MAX (adelefeld/lball.h:67,
   the bound that every function of the type and the value form reader (adelefeld/text.h:304, 331)
   apply to those two fields, so that the loader never builds a field that no function of the
   library accepts); p below 2^64 is a word restriction of stage 5 (conventions 8.5 item 5). The
   value is built in a temporary whose predicate is asked before the exchange into x, so a text
   that defeats the check of stage 6 gets ADF_DOMAIN with x untouched instead of an abort. ---- */
int adf_lball_load_str(adf_lball_t x, const char * s, size_t len, const adf_modctx_struct * ctx,
                       const adf_text_limits_t * lim);
int adf_lball_load_str_binds(adf_lball_t x, const char * s, size_t len,
                             const adf_modctx_struct * const * binds, size_t nbinds,
                             const adf_text_limits_t * lim);
char * adf_lball_dump_str(size_t * len, const adf_lball_t x);
int adf_lball_dump_inspect(size_t * nctx, adf_ctx_desc_t * descs, const char * s, size_t len,
                           const adf_text_limits_t * lim);

/* ---- adf_sball: body "sball (n | r <arb> | c <acb>) <count> {<lb>}" (no context occurrence).
   The tag n, r or c is the archimedean tag ADF_ARCH_NONE, ADF_ARCH_REAL or ADF_ARCH_COMPLEX of
   conventions 5.9, and the count is the number of primes; the predicate of 5.9 is checked on the
   tokens: the balls are canonical arbs (so finite, conventions 5.9), each lb satisfies 5.8, and
   the primes strictly increase. The count is bounded by max_items (conventions 8.4, "the number of
   ... places"), the exponents of every lb by max_prec and by ADF_LBALL_EXP_MAX (as for a local
   ball above), and a prime below 2^64 is required of each of them. The value is built in a
   temporary whose predicate is asked before the exchange into x. ---- */
int adf_sball_load_str(adf_sball_t x, const char * s, size_t len, const adf_modctx_struct * ctx,
                       const adf_text_limits_t * lim);
int adf_sball_load_str_binds(adf_sball_t x, const char * s, size_t len,
                             const adf_modctx_struct * const * binds, size_t nbinds,
                             const adf_text_limits_t * lim);
char * adf_sball_dump_str(size_t * len, const adf_sball_t x);
int adf_sball_dump_inspect(size_t * nctx, adf_ctx_desc_t * descs, const char * s, size_t len,
                           const adf_text_limits_t * lim);

/* ---- adf_qclass: "qclass lift arch fb" or "qclass pieces h {arch fb}".
   Sources: docs/api-3.md:283-309; conventions 10.1:1411, 10.2:1427-1480, 5.10:722-752.
   Strict load: no normalization, sorting, or duplicate removal. OK writes x; PARSE, LIMIT,
   UNSUPPORTED, DOMAIN leave it untouched. Stage order: length, bytes/header, entire grammar,
   limits (max_items, M1-D9), word support, domain predicates, then context bindings.
   Validate all bytes before any FLINT load call. PIECES exponents have the fixed M1-D9 bound;
   LIFT exponents are unrestricted. Raw local data and context pointers are preserved.
   binds supplies exactly one matching live context per local occurrence in traversal order;
   load_str repeats ctx at every occurrence. Caller owns contexts. No member aliasing.
   Cost: linear in input plus canonical triples and exact adjacent key comparisons.
   dump_str is lossless: preserve form/order/real bits/backend/raw fields; caller frees its string
   with adf_str_free; *len excludes NUL. No status; cost output size. INV checks its canonical input.
   inspect constructs no class. descs=NULL writes count only on OK. Otherwise *nctx is capacity
   in initialized descriptors; insufficient capacity gives LIMIT with all outputs untouched.
   All other statuses and stage order match load; descriptor cost is linear in recorded contexts. */
int adf_qclass_load_str(adf_qclass_t x, const char *s, size_t len,
                         const adf_modctx_struct *ctx, const adf_text_limits_t *lim);
int adf_qclass_load_str_binds(adf_qclass_t x, const char *s, size_t len,
                               const adf_modctx_struct *const *binds, size_t nbinds,
                               const adf_text_limits_t *lim);
char *adf_qclass_dump_str(size_t *len, const adf_qclass_t x);
int adf_qclass_dump_inspect(size_t *nctx, adf_ctx_desc_t *descs, const char *s, size_t len,
                             const adf_text_limits_t *lim);

/* Slice b, api-3c 2/P3; conventions 10.1:1414, 10.2:1427-1480.
   Strict lossless body "char q n acb"; prefix "adf1 Q". Recompute parity; reject an
   imprimitive pair rather than lower it. Validate all bytes/balls before any FLINT load call.
   Stages: length, alphabet/header, full grammar, limits, q word bound, cheap semantics,
   Cheap principal n=1,q>1 rejection; D1 q<=65536 before group setup, primitive predicate,
   then binding count. The principal shortcut is dirichlet.h:144-147.
   No contexts: ctx ignored; binds requires nbinds=0. No working precision or member aliasing.
   OK/PARSE/LIMIT/UNSUPPORTED/DOMAIN; all failures preserve x. Cost input size+D(q).
   dump_str preserves stored bits, allocates with FLINT; free with adf_str_free, *len excludes NUL.
   No dumper status; cost output size; INV checks canonical input.
   inspect validates the same strict body, writes nctx=0 only on OK, leaves descs untouched.
   With descs=NULL incoming nctx is ignored; otherwise it is capacity. No value is built. */
int adf_char_load_str(adf_char_t x, const char *s, size_t len,
                      const adf_modctx_struct *ctx, const adf_text_limits_t *lim);
int adf_char_load_str_binds(adf_char_t x, const char *s, size_t len,
                            const adf_modctx_struct *const *binds, size_t nbinds,
                            const adf_text_limits_t *lim);
char *adf_char_dump_str(size_t *len, const adf_char_t x);
int adf_char_dump_inspect(size_t *nctx, adf_ctx_desc_t *descs, const char *s, size_t len,
                          const adf_text_limits_t *lim);

/* Slice 4c, api-4.md section 2; conventions 10.1/10.2 and predicates 5.11/5.12.
   Strict lossless bodies: "ffun D M {acb}" (D M values), and
   "rfun len {length(P) {acb} A B C}"; every integer is lower-case hexadecimal.
   Validate length, alphabet/header, full grammar, caller limits, D1 caps, then predicates
   before constructing any value. No normalization, term sorting, precision or FLINT string load.
   D1 (SPEC 15.4 N-D23): D M <= 2^20; terms and total coefficients <= 2^16. Above caps: LIMIT.
   An exact trailing zero coefficient or uncertified Re(A)>0 is DOMAIN.
   M1-D9's qclass-pieces exponent bound does not apply to these bodies.
   Load: OK/PARSE/LIMIT/UNSUPPORTED/DOMAIN, failures leave x untouched;
   cost input size plus clearing old x. The design's cost/identity wording needs these qualifications.
   No context occurrences: ctx is ignored, binds requires nbinds=0 after full validation.
   Dump: allocate exact bytes preserving ball bits, zero terms and term order; *len excludes NUL;
   caller adf_str_free; no status, cost output size. INV checks canonical input/output on entry.
   Inspect: same validation/statuses, write nctx=0 only on OK, descs untouched; cost input size.
   The common initialized-storage and whole-object alias rules of api-4.md section 1 apply. */
int adf_ffun_load_str(adf_ffun_t x, const char *s, size_t n, const adf_modctx_struct *ctx,
                      const adf_text_limits_t *lim);
int adf_rfun_load_str(adf_rfun_t x, const char *s, size_t n, const adf_modctx_struct *ctx,
                      const adf_text_limits_t *lim);
int adf_ffun_load_str_binds(adf_ffun_t x, const char *s, size_t n,
                            const adf_modctx_struct *const *binds, size_t nbinds,
                            const adf_text_limits_t *lim);
int adf_rfun_load_str_binds(adf_rfun_t x, const char *s, size_t n,
                            const adf_modctx_struct *const *binds, size_t nbinds,
                            const adf_text_limits_t *lim);
char *adf_ffun_dump_str(size_t *len, const adf_ffun_t x);
char *adf_rfun_dump_str(size_t *len, const adf_rfun_t x);
int adf_ffun_dump_inspect(size_t *nctx, adf_ctx_desc_t *descs, const char *s, size_t n,
                          const adf_text_limits_t *lim);
int adf_rfun_dump_inspect(size_t *nctx, adf_ctx_desc_t *descs, const char *s, size_t n,
                          const adf_text_limits_t *lim);

/* Layout queries of the descriptor (conventions 10.2, 12.4). Header-inline and exported. */
ADF_INLINE size_t adf_sizeof_ctx_desc(void) { return sizeof(adf_ctx_desc_t); }
ADF_INLINE size_t adf_alignof_ctx_desc(void) { return ADF_ALIGNOF(adf_ctx_desc_t); }

#ifdef __cplusplus
}
#endif

#endif /* ADELEFELD_DUMP_H */
