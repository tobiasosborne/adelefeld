/* adelefeld/common.h: what every header of adelefeld needs.

   Declarations only. Contract: docs/conventions.md 0.4, sections 1, 2, 4.2, 12. This header
   - includes the FLINT headers of the types that the public structs embed (fmpz, fmpq);
   - refuses, at compile time, a FLINT other than 3.0.x (conventions 12.11, CV-44);
   - defines ADF_INLINE (conventions 12.1) and ADF_ALIGNOF;
   - declares the incomplete context type adf_modctx_struct (conventions 4.6, 5.14, 12.4);
   - declares the functions shared by all string outputs and the run-time version check.

   Language: C11 and C++17, no compiler extension (conventions 12; brief of lane m1-headers).
   Platform: 64-bit only (conventions 12.10, CV-42); checked below. */

#ifndef ADELEFELD_COMMON_H
#define ADELEFELD_COMMON_H

#include <stddef.h>

#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/fmpq.h>

/* FLINT version. conventions 12.11: "A binding must link against a FLINT whose flint_version
   string (flint.h:105) has the same major and minor version". The installed macros are
   __FLINT_VERSION and __FLINT_VERSION_MINOR (/usr/include/flint/flint.h:94-97). */
#if !defined(__FLINT_VERSION) || !defined(__FLINT_VERSION_MINOR)
#error "adelefeld: the FLINT version macros are missing; FLINT 3.0.x is required"
#elif __FLINT_VERSION != 3 || __FLINT_VERSION_MINOR != 0
#error "adelefeld: FLINT 3.0.x is required (conventions 12.11): the public layouts are fixed for it"
#endif

/* 64-bit words (conventions 12.10, CV-42: FLINT_BITS 64, flint.h:193). */
#if FLINT_BITS != 64
#error "adelefeld: version 1 supports 64-bit platforms only (conventions 12.10, CV-42)"
#endif

/* Inline fast paths (conventions 12.1). A function marked ADF_INLINE is defined in the header and
   is also an exported symbol of the library: exactly one source file of the library defines
   ADF_INLINES_C before including the headers, so that the same definitions are compiled with
   external linkage there. This is the FLINT pattern of fmpz.h:15-19, written with the C99/C++
   keyword `inline` instead of the GNU spelling `__inline__`, so that no compiler extension is used.
   Every ADF_INLINE function is a complete public operation that a binding may also reach through
   the exported symbol; no operation exists only in the header. */
#ifdef ADF_INLINES_C
#define ADF_INLINE
#else
#define ADF_INLINE static inline
#endif

/* Alignment of a type: _Alignof in C11, alignof in C++11 and later. */
#ifdef __cplusplus
#define ADF_ALIGNOF(T) alignof(T)
#else
#define ADF_ALIGNOF(T) _Alignof(T)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* The modulus context (conventions 4.6, 5.14, 12.4; gate finding G2). Incomplete: its fields and
   layout are not part of the interface; there is no by-value or array-of-one context type. A
   context exists only after a successful adf_modctx_new_* constructor (adelefeld/modctx.h) and is
   released by adf_modctx_free. Values borrow it through `const adf_modctx_struct *`. */
typedef struct adf_modctx_struct adf_modctx_struct;

/* adf_str_free(s)
   Releases a string returned by any adf_*_get_str or adf_*_dump_str (conventions 4.2, 8.1, 12.8,
   CV-43). It calls flint_free (flint.h:201-204). s = NULL does nothing (as free(NULL)).
   Aliasing: none. Status: none (void). Cost: one flint_free. */
void adf_str_free(char * s);

/* adf_flint_version_compiled()
   Returns the FLINT version string that the library was compiled against, a static NUL-terminated
   string that the caller must not free, "3.0.1" for the build of version 0.1.0 (conventions 12.11,
   CV-44). Status: none. Cost: constant. */
const char * adf_flint_version_compiled(void);

/* adf_version_check()
   Compares the run-time FLINT version string flint_version (flint.h:105) with
   adf_flint_version_compiled(). Returns ADF_OK (0) when their major and minor versions agree, and
   ADF_UNSUPPORTED (8) otherwise (conventions 12.11, CV-44; status values of conventions 3.1).
   Writes nothing. Cost: a comparison of two short strings. */
int adf_version_check(void);

#ifdef __cplusplus
}
#endif

#endif /* ADELEFELD_COMMON_H */
