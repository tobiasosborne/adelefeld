/* adelefeld/place.h: places of Q as opaque handles.

   Contract: docs/conventions.md 0.4, section 7 (DECISION CV-18, D8; CV-56), 12.5; docs/seams.md
   section 5, R1. A place is an 8-byte struct passed by value. User code creates and reads places
   only through the functions below; the integer inside is not part of the contract, and no
   arithmetic or comparison on it is offered. Version 1: the archimedean place and the primes
   below 2^64. */

#ifndef ADELEFELD_PLACE_H
#define ADELEFELD_PLACE_H

#include "adelefeld/common.h"
#include "adelefeld/status.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Layout fixed for the foreign-function interface (conventions 7, 12.5): one ulong, 8 bytes,
   alignment 8. The field is private. */
typedef struct
{
    ulong opaque;
} adf_place_t;

/* adf_place_inf()
   Returns the archimedean place of Q (conventions 7). Status: none. Cost: constant. */
adf_place_t adf_place_inf(void);

/* adf_place_prime(v, p)
   Sets *v to the place of the prime p. Certifies primality with n_is_prime (ulong_extras.h:335;
   conventions 7, 5.8).
   Status: ADF_OK, *v written; ADF_DOMAIN when p is not prime (0, 1 and composites), *v untouched
   (conventions 3.1, 4.3). A prime at or above 2^64 cannot be passed (the type is ulong; conventions 7).
   Aliasing: v is an output only. Cost: one n_is_prime on a word. */
int adf_place_prime(adf_place_t * v, ulong p);

/* adf_place_is_archimedean(v)
   Returns 1 if v is the archimedean place, else 0 (conventions 7). Cost: constant. */
int adf_place_is_archimedean(adf_place_t v);

/* adf_place_prime_get(v)
   Returns the prime of a finite place; 0 for the archimedean place (conventions 7). Cost: constant. */
ulong adf_place_prime_get(adf_place_t v);

/* adf_place_cmp(v, w)
   Returns -1, 0 or 1 as v comes before, equals or comes after w in the canonical order of places:
   the archimedean place first, then the primes in increasing order (conventions 7). Cost: constant. */
int adf_place_cmp(adf_place_t v, adf_place_t w);

/* adf_place_equal(v, w)
   Returns 1 if v and w are the same place, else 0 (conventions 7). Cost: constant. */
int adf_place_equal(adf_place_t v, adf_place_t w);

/* Layout queries (conventions 12.4, CV-40; brief: adf_alignof_*). Header-inline and exported. */
ADF_INLINE size_t adf_sizeof_place(void) { return sizeof(adf_place_t); }
ADF_INLINE size_t adf_alignof_place(void) { return ADF_ALIGNOF(adf_place_t); }

#ifdef __cplusplus
}
#endif

#endif /* ADELEFELD_PLACE_H */
