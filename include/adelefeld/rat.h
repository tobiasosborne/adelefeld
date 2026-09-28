/* adelefeld/rat.h: adf_rat, the exact global rational.

   Contract: docs/conventions.md 0.4, 5.1 (struct, predicate, init), 2.3 (life cycle), 3.2 (statuses),
   4.1 (aliasing), 4.3 (outputs after a status); docs/SPEC.md 4.1 ("Exact rationals are a separate
   type"). Meaning of a value: the rational q at every place at once, exact. It stays exact under
   arithmetic with other exact values (SPEC 4.1, M3).

   Common rules for every function below, unless its comment says otherwise:
   - Aliasing (conventions 4.1): an output may be the same object as any input of the same type;
     inputs may alias each other.
   - Inputs must satisfy the predicate of conventions 5.1 (fmpq_is_canonical); a non-canonical input
     is a precondition violation (conventions 4.4, CV-09).
   - A function that returns a status leaves every output untouched on a status other than ADF_OK
     (conventions 4.3, CV-06).
   - Every output written with ADF_OK, and every output of a void function, satisfies 5.1.
   Implemented in work package 1.2 (docs/PLAN.md section 6). */

#ifndef ADELEFELD_RAT_H
#define ADELEFELD_RAT_H

#include "adelefeld/common.h"
#include "adelefeld/status.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Layout (conventions 5.1, 12.4): one fmpq, 16 bytes, alignment 8.
   Predicate: fmpq_is_canonical(q) (fmpq.h:117-118): denominator > 0, gcd(num, den) = 1.
   The sign is on the numerator; the exact zero is 0/1. */
typedef struct
{
    fmpq_t q;
} adf_rat_struct;

typedef adf_rat_struct adf_rat_t[1];
typedef adf_rat_struct * adf_rat_ptr;
typedef const adf_rat_struct * adf_rat_srcptr;

/* ---- life cycle (conventions 2.3) ---- */

/* adf_rat_init(x): x = 0, stored 0/1 (conventions 5.1; fmpq.h:28-32). Never fails. */
void adf_rat_init(adf_rat_t x);

/* adf_rat_clear(x): releases the memory of x; afterwards x may only be passed to init. */
void adf_rat_clear(adf_rat_t x);

/* adf_rat_set(y, x): y = x. y may be x. Cost: a copy of two integers. */
void adf_rat_set(adf_rat_t y, const adf_rat_t x);

/* adf_rat_swap(x, y): exchanges the values; O(1), no allocation (conventions 2.3). */
void adf_rat_swap(adf_rat_t x, adf_rat_t y);

/* adf_rat_is_canonical(x): 1 if x satisfies conventions 5.1, else 0; never aborts, for an
   initialised object (decision M1-D2: the two fmpz must be initialised FLINT integers). */
int adf_rat_is_canonical(const adf_rat_t x);

/* adf_rat_identical(x, y): 1 if numerators and denominators are equal, else 0. For this exact type
   representation identity and equality of the rationals coincide (conventions 2.3, 5.1). */
int adf_rat_identical(const adf_rat_t x, const adf_rat_t y);

/* ---- constructors ---- */

/* adf_rat_zero(x): x = 0. adf_rat_one(x): x = 1. */
void adf_rat_zero(adf_rat_t x);
void adf_rat_one(adf_rat_t x);

/* adf_rat_set_si(x, n): x = n. Cost: constant. */
void adf_rat_set_si(adf_rat_t x, slong n);

/* adf_rat_set_fmpz(x, n): x = n. Cost: a copy of n. */
void adf_rat_set_fmpz(adf_rat_t x, const fmpz_t n);

/* adf_rat_set_fmpz2(x, num, den): x = num/den, reduced to lowest terms with a positive denominator.
   Convention: constructor from raw data, conventions 3.2 ("OK, DOMAIN"), 4.4 (zero denominator).
   Status: ADF_OK, x written; ADF_DOMAIN if den = 0, x untouched.
   Aliasing: num and den may alias each other; they are not of the output type.
   Cost: one gcd of the sizes of num and den. */
int adf_rat_set_fmpz2(adf_rat_t x, const fmpz_t num, const fmpz_t den);

/* adf_rat_set_fmpq(x, q): x = the rational q, which may be a non-canonical fmpq (any non-zero
   denominator, any common factor); x is its lowest-terms form with positive denominator.
   Status: ADF_OK, x written; ADF_DOMAIN if the denominator of q is 0, x untouched.
   Convention: conventions 3.2 (constructors from raw data), 4.4. Cost: one gcd. */
int adf_rat_set_fmpq(adf_rat_t x, const fmpq_t q);

/* adf_rat_get_fmpq(q, x): q = x, canonical. q is an fmpq initialised by the caller. */
void adf_rat_get_fmpq(fmpq_t q, const adf_rat_t x);

/* ---- predicates ---- */

/* adf_rat_is_zero(x): 1 if x = 0, else 0. adf_rat_equal(x, y): 1 if x = y, else 0 (exact values:
   point equality is decided; CV-02 excludes _equal only for ball types). */
int adf_rat_is_zero(const adf_rat_t x);
int adf_rat_equal(const adf_rat_t x, const adf_rat_t y);

/* adf_rat_sgn(x): -1, 0 or 1, the sign of x. */
int adf_rat_sgn(const adf_rat_t x);

/* ---- arithmetic: exact (conventions 3.2 row 1: void) ---- */

/* z = x + y, z = x - y, z = x * y, y = -x; exact rationals. Aliasing: any. Cost: fmpq_add,
   fmpq_sub, fmpq_mul, fmpq_neg (a constant number of gcds of the operand sizes). */
void adf_rat_add(adf_rat_t z, const adf_rat_t x, const adf_rat_t y);
void adf_rat_sub(adf_rat_t z, const adf_rat_t x, const adf_rat_t y);
void adf_rat_mul(adf_rat_t z, const adf_rat_t x, const adf_rat_t y);
void adf_rat_neg(adf_rat_t y, const adf_rat_t x);

/* adf_rat_div(z, x, y): z = x / y.
   Convention: conventions 3.2 row "Division of adf_rat by an adf_rat" (OK, NOT_UNIT).
   Status: ADF_OK, z written; ADF_NOT_UNIT if y = 0 (proved: 0 is not invertible), z untouched.
   Aliasing: any. Cost: fmpq_div. */
int adf_rat_div(adf_rat_t z, const adf_rat_t x, const adf_rat_t y);

/* adf_rat_inv(y, x): y = 1/x. Status: ADF_OK; ADF_NOT_UNIT if x = 0, y untouched. Aliasing: any. */
int adf_rat_inv(adf_rat_t y, const adf_rat_t x);

/* Layout queries (conventions 12.4, CV-40). Header-inline and exported. */
ADF_INLINE size_t adf_sizeof_rat(void) { return sizeof(adf_rat_struct); }
ADF_INLINE size_t adf_alignof_rat(void) { return ADF_ALIGNOF(adf_rat_struct); }

#ifdef __cplusplus
}
#endif

#endif /* ADELEFELD_RAT_H */
