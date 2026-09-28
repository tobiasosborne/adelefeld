/* inlines.c: the one file of the library that defines ADF_INLINES_C before including the
   headers, so that every ADF_INLINE function of the public headers is compiled here with
   external linkage and is an exported symbol of the library (conventions 12.1 and DECISION
   CV-40 with the brief of lane m1-headers; the FLINT pattern is fmpz.h:15-19).

   Nothing else belongs in this file. Every other source file includes <adelefeld.h> without the
   macro, and therefore sees the ADF_INLINE functions as static inline fast paths, exactly as a
   user of the headers does. */

#define ADF_INLINES_C
#include <adelefeld.h>
