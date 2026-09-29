/* adelefeld/ucoset.h: adf_ucoset, a unit of Zhat to finite precision (a unit coset), with the exact
   units [1] and [-1].

   Contract: docs/SPEC.md 5 ("Unit precision", "Exact units", the table of types); docs/conventions.md
   5.6 (struct, predicate, residue range 1..N, modulus as supplied, normal form), 2.3, 3.2, 4.1, 4.3,
   4.4; decisions M0-D1 (exact units), D2-1 of lane i-slice1's brief (results in normal form,
   constructors keep the modulus as supplied, CV-17). Proofs: docs/proofs/ideles.md Definition 4
   (line 87), Lemma 7 (line 132), Proposition 9 (line 152), Propositions 10 and 11 (lines 195, 213);
   docs/api-2.md 1.3, Statements A, B, C (the exact units, the normal form, the product and inverse
   as computed). Implemented in src/ucoset.c (milestone 2, slice 1); tests tests/test_ucoset.c.

   Meaning: for N >= 1 the value (c, N) is the set c U(N) of the units u of Zhat with u = c modulo
   N Zhat (ideles.md:89). For N = 0 it is the one unit c, c = +1 or -1 (U(0) = {1}, "congruent
   modulo 0" is equality; SPEC 5, M0-D1). It is not the additive ball c + N Zhat (ideles.md P6).

   Predicate (is_canonical, conventions 5.6):
       ( N >= 1 and 1 <= c <= N and gcd(c, N) = 1 )  or  ( N = 0 and c in {1, -1} ).
   Normal form (conventions 5.6; api-2.md Statement B): N' = N/2 if N = 2 mod 4, else N' = N; c' the
   residue of c in 1..N'; an exact unit is its own normal form. Two values are the same set exactly
   when their normal forms are the same pair (api-2.md Statement A.5; ideles.md P9.2). Init: [1].

   Common rules for every function below, unless its comment says otherwise:
   - Aliasing (conventions 4.1): an output may be the same object as any input of this type; inputs
     may alias each other.
   - Inputs satisfy the predicate; otherwise undefined (conventions 4.4, CV-09). With
     -DADF_CHECK_INVARIANTS every function that reads a value of this type checks it on entry and
     aborts, except is_canonical and is_normal (M1-D2); clear, swap, one, minus_one
     and set_fmpz2 read none.
   - Results of operations (mul, inv, normalise) are in normal form (D2-1). Nothing here can fail
     except the constructor from raw data; the operations return void (conventions 2.3). */

#ifndef ADELEFELD_UCOSET_H
#define ADELEFELD_UCOSET_H

#include "adelefeld/common.h"
#include "adelefeld/status.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Layout (conventions 5.6, 12.4), 64-bit: c (fmpz) at 0, N (fmpz) at 8; size 16, alignment 8. */
typedef struct
{
    fmpz_t c;
    fmpz_t N;
} adf_ucoset_struct;

typedef adf_ucoset_struct adf_ucoset_t[1];
typedef adf_ucoset_struct * adf_ucoset_ptr;
typedef const adf_ucoset_struct * adf_ucoset_srcptr;

/* ---- life cycle (conventions 2.3) ---- */

/* adf_ucoset_init(x): x = [1], the exact unit 1, stored (1, 0) (conventions 5.6). Never fails. */
void adf_ucoset_init(adf_ucoset_t x);

/* adf_ucoset_clear(x): releases the memory; afterwards x may only be passed to init. */
void adf_ucoset_clear(adf_ucoset_t x);

/* adf_ucoset_set(y, x): y becomes a copy of x, the stored pair as it is. y may be x. */
void adf_ucoset_set(adf_ucoset_t y, const adf_ucoset_t x);

/* adf_ucoset_swap(x, y): exchanges the contents; O(1), no allocation. */
void adf_ucoset_swap(adf_ucoset_t x, adf_ucoset_t y);

/* adf_ucoset_is_canonical(x): 1 if the predicate above holds, else 0. Never aborts for an
   initialised object, whatever integers its fields hold (decision M1-D2). Cost: a gcd. */
int adf_ucoset_is_canonical(const adf_ucoset_t x);

/* adf_ucoset_is_normal(x): 1 if x is canonical and in normal form (N = 0, or N != 2 mod 4), else 0.
   Never aborts for an initialised object. */
int adf_ucoset_is_normal(const adf_ucoset_t x);

/* adf_ucoset_identical(x, y): 1 if the stored pairs are equal, else 0 (conventions 2.3:
   representation identity). [5 mod 6] and [2 mod 3] are the same set and not identical; use
   adf_ucoset_equal_set for sets. */
int adf_ucoset_identical(const adf_ucoset_t x, const adf_ucoset_t y);

/* ---- constructors and accessors ---- */

/* adf_ucoset_set_fmpz2(x, c, N): x = c U(N), from raw data.
   For N >= 1 and gcd(c, N) = 1: c is any integer (negative allowed) and is stored as its residue in
   1..N (for N = 1 the residue is 1); N is stored as supplied, not normalised (CV-17), so
   [5 mod 6] stays (5, 6). For N = 0: c must be 1 or -1 (the exact unit, M0-D1).
   Status (conventions 3.2, constructors from raw data; 4.4): ADF_OK, x written; ADF_DOMAIN, x
   untouched, if N < 0, or N = 0 and c is not +-1, or N >= 1 and gcd(c, N) != 1 (so c = 0 is refused
   except for N = 1, where gcd(0, 1) = 1 and the result is [1 mod 1], the whole unit group).
   Aliasing: c and N may be the members of x itself. Cost: a gcd and a division. */
int adf_ucoset_set_fmpz2(adf_ucoset_t x, const fmpz_t c, const fmpz_t N);

/* adf_ucoset_one(x): x = [1] = (1, 0). adf_ucoset_minus_one(x): x = [-1] = (-1, 0). The exact units
   (SPEC 5, M0-D1). */
void adf_ucoset_one(adf_ucoset_t x);
void adf_ucoset_minus_one(adf_ucoset_t x);

/* adf_ucoset_get_fmpz2(c, N, x): the stored pair (c, N) of x, copies. c and N must be different
   objects; each may be a member of x. */
void adf_ucoset_get_fmpz2(fmpz_t c, fmpz_t N, const adf_ucoset_t x);

/* adf_ucoset_is_exact(x): 1 if x is an exact unit (N = 0), else 0. */
int adf_ucoset_is_exact(const adf_ucoset_t x);

/* adf_ucoset_normalise(y, x): y = the normal form of x, the same set (api-2.md Statement B; ideles.md
   Lemma 7, line 132). Cost: a division. */
void adf_ucoset_normalise(adf_ucoset_t y, const adf_ucoset_t x);

/* ---- set predicates (SPEC 4.2 names; ideles.md P9, line 152; api-2.md Statement A) ---- */

/* adf_ucoset_equal_set(x, y): 1 if x and y are the same set, else 0: the normal forms are the same
   pair (ideles.md P9.2, line 157; api-2.md A.5). An exact unit equals no coset with N >= 1:
   [1] is not [1 mod 1]. Cost: two divisions. */
int adf_ucoset_equal_set(const adf_ucoset_t x, const adf_ucoset_t y);

/* adf_ucoset_contains(x, y): 1 if the set x is inside the set y ("first inside second", SPEC 4.2),
   else 0. With normal forms (c, N) of x and (c', N') of y:
   N, N' >= 1: N' divides N and c = c' modulo N' (ideles.md P9.1, line 156);
   N = 0, N' >= 1: c = c' modulo N' (api-2.md A.3; conventions 5.6);
   N' = 0: N = 0 and c = c' (a coset with N >= 1 has two elements, A.3). Cost: two divisions. */
int adf_ucoset_contains(const adf_ucoset_t x, const adf_ucoset_t y);

/* adf_ucoset_overlaps(x, y): 1 if the sets meet, else 0: c = c' modulo g = gcd(N, N'), with
   gcd(0, N') = N' and, for g = 0, equality (ideles.md P9.3, line 159; api-2.md A.4). Not
   transitive. Cost: a gcd. */
int adf_ucoset_overlaps(const adf_ucoset_t x, const adf_ucoset_t y);

/* ---- arithmetic (void: nothing can fail) ---- */

/* adf_ucoset_mul(z, x, y): z = the product set {u v : u in x, v in y}, which is the unit coset
   (c c') U(g), g = gcd(N, N') with gcd(0, N') = N' (SPEC 5; ideles.md P10.1, line 199, and P11.1,
   line 217; api-2.md Statements A.1 and C.1), stored in normal form. For N, N' >= 1 it is the
   smallest coset containing the product (P11.2): the finer input's precision is lost because the
   product set is this coset. The product of two exact units is exact. z may be x or y or both.
   Cost: a gcd, a product of the residues and a division. */
void adf_ucoset_mul(adf_ucoset_t z, const adf_ucoset_t x, const adf_ucoset_t y);

/* adf_ucoset_inv(y, x): y = {u^(-1) : u in x} = c* U(N), c* the inverse of c modulo N (1 for
   N = 1) (ideles.md P10.2, line 200; api-2.md C.2), stored in normal form; an exact unit is its own
   inverse (A.2). x * x^-1 is U(N') (N' the normal modulus), which contains 1 and is not the exact
   unit 1 unless N = 0. y may be x. Cost: a modular inverse. */
void adf_ucoset_inv(adf_ucoset_t y, const adf_ucoset_t x);

/* Layout queries (conventions 12.4, CV-40). Header-inline and exported. */
ADF_INLINE size_t adf_sizeof_ucoset(void) { return sizeof(adf_ucoset_struct); }
ADF_INLINE size_t adf_alignof_ucoset(void) { return ADF_ALIGNOF(adf_ucoset_struct); }

#ifdef __cplusplus
}
#endif

#endif /* ADELEFELD_UCOSET_H */
