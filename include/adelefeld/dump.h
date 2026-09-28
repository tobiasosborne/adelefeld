/* adelefeld/dump.h: the dump form (version 1, field Q) for adf_rat, adf_fball, adf_scaled,
   adf_adele, adf_cadele: loaders with context bindings, dumpers, inspection of context occurrences.

   Contract: docs/conventions.md 0.4, section 10 (grammar 10.1; rules 10.2: strict loader CV-38,
   real balls as arb_dump_str writes them, validation of the whole text before any FLINT load
   function CV-52, context bindings of gate finding G3, descriptors and inspection of closure
   finding C2), 8.1 (interface), 8.4, 8.5 (limits, order of checks), 4.3, 12.8; docs/SPEC.md 10
   item 2 (M0-D9). Golden vectors: tests/golden/dump.tsv (conventions 11.3 item 5). Reference:
   proto/text_grammar.py. Implemented in work package 1.4 (docs/PLAN.md section 6).

   The five bodies used here (conventions 10.1): "rat", "fball" (form "g" global: A H d; form "l"
   local: d, context, residues), "scaled" (form "x" exact: num den; form "s": num(s) den(s) u; then
   the context), "adele" (archimedean count 1, one arb, one fball), "cadele" (count 1, one acb, one
   fball). Every dump starts "adf1 Q ".

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

/* Layout queries of the descriptor (conventions 10.2, 12.4). Header-inline and exported. */
ADF_INLINE size_t adf_sizeof_ctx_desc(void) { return sizeof(adf_ctx_desc_t); }
ADF_INLINE size_t adf_alignof_ctx_desc(void) { return ADF_ALIGNOF(adf_ctx_desc_t); }

#ifdef __cplusplus
}
#endif

#endif /* ADELEFELD_DUMP_H */
