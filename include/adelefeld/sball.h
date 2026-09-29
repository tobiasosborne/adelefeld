/* adelefeld/sball.h: adf_sball, a partial ball over a finite set of places (milestone 1F, work package 1F.1,
   second slice of the milestone; the first was adf_lball).

   Contract: docs/conventions.md 5.9 (struct, predicate, init), 5.8 (the local components), 2.1 (verbs of the set
   predicates), 2.2 (argument order: outputs, then reports, then inputs, then prec), 2.3 (life cycle), 3.1 to 3.3
   (statuses; the reported place; the maximum), 4.1 (aliasing), 4.3 (outputs after a status), 7 (places, canonical
   order), 12.4 (layout queries); docs/SPEC.md 9.3.1 (a partial ball over S; "if one place fails, the function
   returns the status with that place and no value"), 4.1; docs/proofs/functions.md Proposition 22 (line 725:
   applying f after projection to a finite named place set S is forming (f_v(x_v)) for v in S; the result is typed
   over S; the other coordinates are not part of it) and Definition 1 (line 11); docs/api-1f.md, section
   "Slice 1F.1-a / 1F.2-a" (statements S1 to S4: the projection of an adele to a set of places, the componentwise
   ring operations, the set predicates). The local components follow adelefeld/lball.h; the real component
   follows arb at the working precision prec (conventions 2.2; refs/src/flint-3.0.1/arb.rst).

   Meaning (conventions 5.9). An adf_sball s is the set of the tuples (x_v), v in the set of places of s, where
   x_inf lies in the real ball (arch = REAL: the real interval of s->inf; arch = COMPLEX: the complex box of s->inf)
   and x_p lies in the local ball loc[i] for each prime p of s. The set of places is {infinity} (if arch is not
   NONE) together with the primes loc[i].p. There is no default set of places: the value is the set of tuples,
   nothing is said of any other place (SPEC 9.3.1).

   Order of places. The canonical order of conventions 7: the archimedean place first, then the primes in
   increasing order. It is the order of adf_sball_get_place (index 0 is infinity when arch is not NONE) and of
   the reports of the statuses (3.3: the reported place is the first place of the maximal status in this order).
   Storage: the primes are strictly increasing in loc (5.9). The brief of the lane said "real place last"; the
   conventions hold and the real place is first.

   Sets of places are passed as an array of adf_place_t and a length n (no adf_places_t struct exists in the
   conventions; the type is named in 7 and 2.2 only). Constructors accept any order and sort; a repetition is
   ADF_DOMAIN reported with the repeated place (conventions 7: "constructors sort and reject repetitions").

   Common rules, unless a comment says otherwise:
   - Aliasing (conventions 4.1): an output may be the same object as an input of the same type; inputs may alias
     each other. An output never aliases a part of an input (&x->loc[i], x->inf).
   - Inputs satisfy the predicate adf_sball_is_canonical; otherwise the behaviour is undefined (conventions 4.4,
     CV-09). Every output written satisfies it.
   - A function that returns a status leaves every output untouched on a status other than ADF_OK, except the
     report `where` (conventions 4.3), which is written on a status other than OK and left untouched on ADF_OK.
     `where` may be NULL.
   - Statuses over several places combine by the maximum; the reported place is the first place in the canonical
     order whose status is that maximum (conventions 3.3).
   - A prec below 2 is taken as 2 (decision M1-D4, adele.h). The local components do not depend on prec.
   - Complex components (arch = COMPLEX) are stored, copied and compared by the functions below; arithmetic on
     them and the functions of adelefeld/rfunc.h return ADF_UNSUPPORTED with the archimedean place in this slice
     (conventions 3.1: a valid request that version 1 does not yet implement). No function of this header makes a
     COMPLEX value; a binding that fills the struct by hand may. */

#ifndef ADELEFELD_SBALL_H
#define ADELEFELD_SBALL_H

#include <flint/arb.h>
#include <flint/acb.h>

#include "adelefeld/common.h"
#include "adelefeld/status.h"
#include "adelefeld/place.h"
#include "adelefeld/rat.h"
#include "adelefeld/fball.h"
#include "adelefeld/lball.h"
#include "adelefeld/adele.h"

#define ADF_ARCH_NONE    0
#define ADF_ARCH_REAL    1
#define ADF_ARCH_COMPLEX 2

#ifdef __cplusplus
extern "C" {
#endif

/* Layout (conventions 5.9, 12.4, CV-40), 64-bit: arch (int) at 0, inf (acb_struct, 96 bytes) at 8, len (slong) at
   104, loc (pointer) at 112; size 120, alignment 8. The elements of loc are adf_lball_struct of size 48.

   Predicate (conventions 5.9):
       arch in {0, 1, 2} and acb_is_finite(inf) and (arch = 1 implies the imaginary part of inf is exact 0) and
       (arch = 0 implies inf is exact 0) and len >= 0 and (len > 0 implies loc is not NULL) and each loc[i]
       satisfies adf_lball_is_canonical and loc[i].p < loc[i+1].p.
   Ownership: loc is an array of len initialised adf_lball_struct allocated with flint_malloc (NULL when len = 0);
   adf_sball_clear releases each component and the array with flint_free. A binding that fills the struct by hand
   allocates it the same way. */
typedef struct
{
    int arch;
    acb_t inf;
    slong len;
    adf_lball_struct * loc;
} adf_sball_struct;

typedef adf_sball_struct adf_sball_t[1];
typedef adf_sball_struct * adf_sball_ptr;
typedef const adf_sball_struct * adf_sball_srcptr;

/* ---- life cycle (conventions 2.3) ---- */

/* adf_sball_init(x): x = the empty set of places (arch = NONE, len = 0, loc = NULL; conventions 5.9). It is the
   set with one element, the empty tuple. Never fails. */
void adf_sball_init(adf_sball_t x);

/* adf_sball_clear(x): releases the array and the components. */
void adf_sball_clear(adf_sball_t x);

/* adf_sball_set(y, x): y becomes a copy of x. y may be x. Cost: a copy of every component. */
void adf_sball_set(adf_sball_t y, const adf_sball_t x);

/* adf_sball_swap(x, y): exchanges the contents; O(1), no allocation. */
void adf_sball_swap(adf_sball_t x, adf_sball_t y);

/* adf_sball_is_canonical(x): 1 if x satisfies the predicate above, else 0. It never aborts for an initialised
   object whose fields hold any values, provided loc points to len live adf_lball_struct when len > 0 (a negative
   len or a NULL loc with len > 0 gives 0). Cost: len primality tests of one word each. */
int adf_sball_is_canonical(const adf_sball_t x);

/* adf_sball_identical(x, y): 1 if arch, len, acb_equal(inf) (same midpoints and radii) and every component are
   identical (adf_lball_identical), else 0. Representation identity (conventions 2.1, CV-02), not equality of
   sets. */
int adf_sball_identical(const adf_sball_t x, const adf_sball_t y);

/* ---- constructors ---- */

/* adf_sball_set_arb_lballs(y, where, r, loc, n): y = the partial ball with the real component r (if r is not
   NULL; arch = REAL, inf = r + 0 i, the imaginary part exact 0) and the n local components loc[0..n-1] (copied).
   The prime of loc[i] names the place; the array may be in any order and is sorted (statement S1); a component
   is not required to be exact or a ball.
   Status: ADF_OK, y written; ADF_DOMAIN, y untouched, if r is not finite (`where` = the archimedean place), or a
   loc[i] is not canonical (`where` = the place of loc[i].p if that is a prime, untouched if it is not: no place
   can name it; the first such i is reported), or two components have the same prime (`where` = that prime), or
   n < 0 (`where` untouched: no place is at fault). The checks run in this order: r, the components in the order
   given, the repetition. Cost: n copies and a sort.
   Aliasing: r and loc are of other types than y or are not parts of y. */
int adf_sball_set_arb_lballs(adf_sball_t y, adf_place_t * where, const arb_t r, const adf_lball_struct * loc,
                             slong n);

/* adf_sball_project(y, where, x, places, n): y = the projection of the adele x to the n places `places[0..n-1]`
   (the archimedean place at most once, the primes in any order), sorted into the canonical order (statement S1):
   the component at a prime p is adf_lball_set_fball(., p, &x->fin) (the set of the p-th coordinates of the
   finite ball, api-1f.md L1: exactly a ball or the exact rational); the component at the archimedean place is
   the real ball x->inf, copied (arch = REAL). The other coordinates of x are not part of the result
   (docs/proofs/functions.md Proposition 22, line 725). n = 0 gives the empty set of places (places may be NULL).
   Status: ADF_OK, y written; ADF_DOMAIN, y untouched, `where` = the repeated place, if a place occurs twice, or
   untouched if n < 0 (places is checked before any component is made); ADF_LIMIT, y untouched, `where` = the
   first prime, in canonical order, at which adf_lball_set_fball returns ADF_LIMIT. y and x are distinct types;
   no aliasing question arises. Cost: n projections (two valuations and a mod each). */
int adf_sball_project(adf_sball_t y, adf_place_t * where, const adf_adele_t x, const adf_place_t * places,
                      slong n);

/* ---- accessors ---- */

/* adf_sball_arch(x): the tag of the archimedean place, ADF_ARCH_NONE, ADF_ARCH_REAL or ADF_ARCH_COMPLEX. */
int adf_sball_arch(const adf_sball_t x);

/* adf_sball_num_places(x): the number of places: len, plus 1 if the tag is not NONE. */
slong adf_sball_num_places(const adf_sball_t x);

/* adf_sball_get_place(v, x, i): *v = the place number i (0 <= i < num_places) in the canonical order: index 0 is
   the archimedean place when the tag is not NONE, then the primes increasing.
   Status: ADF_OK, *v written; ADF_DOMAIN, *v untouched, if i is out of range. */
int adf_sball_get_place(adf_place_t * v, const adf_sball_t x, slong i);

/* adf_sball_has_place(x, v): 1 if v is one of the places of x, else 0. */
int adf_sball_has_place(const adf_sball_t x, adf_place_t v);

/* adf_sball_get_lball(c, x, v): c = a copy of the local component at the prime of v.
   Status: ADF_OK; ADF_DOMAIN, c untouched, if v is archimedean or v is not a place of x. */
int adf_sball_get_lball(adf_lball_t c, const adf_sball_t x, adf_place_t v);

/* adf_sball_get_arb(r, x, v): r = a copy of the real component (the midpoint and radius of x->inf).
   Status: ADF_OK, r written; ADF_DOMAIN, r untouched, if v is a prime, or the archimedean place is not a place
   of x, or its tag is COMPLEX (the component is not a real ball). */
int adf_sball_get_arb(arb_t r, const adf_sball_t x, adf_place_t v);

/* ---- set predicates (conventions 2.1, 3.2: int 0 or 1, no status) ---- */

/* adf_sball_equal_set(x, y): 1 if the two have the same set of places and the same tag, and at every place the
   sets are equal: adf_lball_equal_set at the primes; at the archimedean place the two real intervals have the same
   end points (arb_contains both ways; a complex component: acb_contains both ways). Tuples over different sets
   of places live in different spaces: 0. */
int adf_sball_equal_set(const adf_sball_t x, const adf_sball_t y);

/* adf_sball_overlaps(x, y): 1 if the sets share a tuple: the same places and tags, and the components overlap at
   every place (a product of sets meets exactly when the factors meet): adf_lball_overlaps, arb_overlaps,
   acb_overlaps. Different places or tags: 0. */
int adf_sball_overlaps(const adf_sball_t x, const adf_sball_t y);

/* adf_sball_contains(x, y): 1 if the set x is inside the set y ("first inside second", fball.h): the same places
   and tags, and at every place the component of x is inside that of y (adf_lball_contains; arb_contains(y, x);
   acb_contains(y, x)). Different places or tags: 0. */
int adf_sball_contains(const adf_sball_t x, const adf_sball_t y);

/* ---- componentwise ring operations over the SAME set of places ---- */

/* adf_sball_neg(y, where, x): y = -x at every place: the local components by adf_lball_neg, the real one by
   arb_neg (exact). Status: ADF_OK; ADF_UNSUPPORTED (a COMPLEX tag; where = the archimedean place); ADF_LIMIT
   (adf_lball_neg; where = the first such prime). Outputs untouched on a status. */
int adf_sball_neg(adf_sball_t y, adf_place_t * where, const adf_sball_t x);

/* adf_sball_add(z, where, x, y, prec): z = x + y at every place: adf_lball_add at the primes (the smallest ball,
   api-1f.md L2), arb_add at prec at the archimedean place. adf_sball_sub, adf_sball_mul: likewise with
   adf_lball_sub, arb_sub and adf_lball_mul, arb_mul.
   The two operands must have the same set of places and the same tag; otherwise ADF_DOMAIN with `where` = the
   first place, in the canonical order, that belongs to one operand and not to the other (or, when the tags
   differ, the archimedean place); the check comes first.
   Status: ADF_OK, z written; ADF_DOMAIN (above); ADF_UNSUPPORTED (a COMPLEX tag, where = the archimedean place);
   ADF_LIMIT (from an adf_lball function, where = the first prime, in canonical order, that fails). The maximum of
   the statuses is taken by the rule of 3.3; the statuses of the places of one call are all ADF_LIMIT or all
   one code, so the first failing place is reported. z is untouched on a status.
   Enclosure: at each place the component of z contains the set of results of the operation on the components
   (arb at the archimedean place; the smallest ball at a prime), so z contains the set of results of the tuples
   (statement S3: the sum and the product of sets of tuples are the products of the sets at each place).
   Cost: one operation of arb at prec, plus one of lball for each prime. */
int adf_sball_add(adf_sball_t z, adf_place_t * where, const adf_sball_t x, const adf_sball_t y, slong prec);
int adf_sball_sub(adf_sball_t z, adf_place_t * where, const adf_sball_t x, const adf_sball_t y, slong prec);
int adf_sball_mul(adf_sball_t z, adf_place_t * where, const adf_sball_t x, const adf_sball_t y, slong prec);

/* Layout queries (conventions 12.4, CV-40). Header-inline and exported. */
ADF_INLINE size_t adf_sizeof_sball(void) { return sizeof(adf_sball_struct); }
ADF_INLINE size_t adf_alignof_sball(void) { return ADF_ALIGNOF(adf_sball_struct); }

#ifdef __cplusplus
}
#endif

#endif /* ADELEFELD_SBALL_H */
