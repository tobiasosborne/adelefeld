/* adelefeld/status.h: status codes and the values of the point comparison.

   Contract: docs/conventions.md 0.4, section 3 (status codes, CV-01, CV-03, CV-04) and section 2.1
   (point comparison, closure finding C1). Statuses are ordered integers, not bit flags; several
   statuses combine by their maximum (conventions 3.3). A status is a plain int return value
   (conventions 12.3); it is never stored in errno or in a global.

   The numeric values are part of the foreign-function interface and are checked against
   conventions 3.1 by tests/test_headers.c. */

#ifndef ADELEFELD_STATUS_H
#define ADELEFELD_STATUS_H

#include "adelefeld/common.h"

/* conventions 3.1, table "The list", in increasing order of precedence. */
#define ADF_OK                 0   /* the output holds the result, which satisfies the contract */
#define ADF_NOT_DETERMINED     1   /* inputs do not determine the quantity, or a result invariant is
                                      not certified; more precision or a sharper method may help */
#define ADF_UNIT_NOT_CERTIFIED 2   /* an invertible input was required; the enclosure does not prove it */
#define ADF_NEEDS_SPLIT        3   /* the result is a union of pieces and one piece was asked for */
#define ADF_NOT_UNIQUE         4   /* several candidates satisfy the problem */
#define ADF_NO_SOLUTION        5   /* proved: no value satisfies the problem */
#define ADF_NOT_UNIT           6   /* proved: the value is not invertible */
#define ADF_DOMAIN             7   /* proved outside the domain; invalid raw data; context mismatch */
#define ADF_UNSUPPORTED        8   /* valid request that version 1 does not implement */
#define ADF_PARSE              9   /* the text is not a sentence of the grammar */
#define ADF_LIMIT             10   /* a resource limit was reached */

/* Number of status codes; the valid codes are 0 to ADF_STATUS_COUNT - 1. */
#define ADF_STATUS_COUNT      11

/* Point comparison (conventions 2.1; SPEC 4.2; closure finding C1). Not a status: a comparison
   function returns one of these three values and nothing else. */
#define ADF_CMP_EQUAL          0   /* both values exact and equal */
#define ADF_CMP_DIFFERENT      1   /* the two sets are disjoint */
#define ADF_CMP_UNDECIDED      2   /* the sets meet and at least one is not a single point */

#ifdef __cplusplus
extern "C" {
#endif

/* adf_status_str(status)
   Returns the name of a status code without the prefix ADF_, as the golden files write it after
   `!` (conventions 11.1): "OK", "NOT_DETERMINED", "UNIT_NOT_CERTIFIED", "NEEDS_SPLIT",
   "NOT_UNIQUE", "NO_SOLUTION", "NOT_UNIT", "DOMAIN", "UNSUPPORTED", "PARSE", "LIMIT". For any other
   int it returns "UNKNOWN". The string is static; the caller must not free it; it is never NULL.
   Convention: conventions 3.1 (names), 11.1 (spelling without ADF_). Aliasing: none. Status: none.
   Cost: constant. Header-inline and exported (conventions 12.1). */
ADF_INLINE const char *
adf_status_str(int status)
{
    switch (status)
    {
        case ADF_OK: return "OK";
        case ADF_NOT_DETERMINED: return "NOT_DETERMINED";
        case ADF_UNIT_NOT_CERTIFIED: return "UNIT_NOT_CERTIFIED";
        case ADF_NEEDS_SPLIT: return "NEEDS_SPLIT";
        case ADF_NOT_UNIQUE: return "NOT_UNIQUE";
        case ADF_NO_SOLUTION: return "NO_SOLUTION";
        case ADF_NOT_UNIT: return "NOT_UNIT";
        case ADF_DOMAIN: return "DOMAIN";
        case ADF_UNSUPPORTED: return "UNSUPPORTED";
        case ADF_PARSE: return "PARSE";
        case ADF_LIMIT: return "LIMIT";
        default: return "UNKNOWN";
    }
}

#ifdef __cplusplus
}
#endif

#endif /* ADELEFELD_STATUS_H */
