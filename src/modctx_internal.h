/* src/modctx_internal.h: the two kernels of src/modctx.c that other files of the library use.

   Not installed and not part of the interface: the symbols are hidden in a shared library
   (tests/test_exports.sh fails if a symbol is exported that no public header declares).
   Both functions read the context only and may run in several threads on one context
   (docs/conventions.md 4.5). ctx must have k >= 1 blocks.

   adf_modctx_reduce(ctx, a, res): res[i] = a mod q_i, 0 <= i < k, for any integer a (the
   residue is in [0, q_i)). res has room for k words.
   adf_modctx_recombine(out, ctx, res): the unique integer in [0, K) that is res[i] mod q_i
   for every i (docs/proofs/policies.md Lemma 17.1). */

#ifndef ADELEFELD_MODCTX_INTERNAL_H
#define ADELEFELD_MODCTX_INTERNAL_H

#include "adelefeld/modctx.h"

#if defined(__GNUC__) || defined(__clang__)
#define ADF_INTERNAL __attribute__((visibility("hidden")))
#else
#define ADF_INTERNAL
#endif

ADF_INTERNAL void adf_modctx_reduce(const adf_modctx_struct * ctx, const fmpz_t a, ulong * res);
ADF_INTERNAL void adf_modctx_recombine(fmpz_t out, const adf_modctx_struct * ctx, const ulong * res);

#endif
