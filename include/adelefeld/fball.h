/* adelefeld/fball.h: adf_fball, the finite ball (A + H Zhat)/d, tight policy.

   Contract: docs/conventions.md 0.4, 5.2 (struct, predicate G, init, stored representative,
   Lemma 5.2 uniqueness), 5.3 (local backend), 2.1 (predicate and comparison names), 3.2
   (statuses), 4.1 (aliasing), 4.3 (outputs after a status), 4.4 (invalid input), 4.6 (implicit
   global fallback); docs/SPEC.md 4.1 to 4.3; docs/proofs/precision.md; docs/proofs/policies.md
   section 1 (Lemma 1: the tight result is the smallest ball containing the result set).

   Meaning of a value (SPEC 4.1): the set (A + H Zhat)/d, centre a = A/d, radius N = H/d. H = 0 is
   the single rational A/d. The radius N is the positive generator of the radius ideal N Z; radii
   are compared by inclusion, never by size, and the precision is read per place (seams R2).

   Backends. Work package 1.2 (docs/PLAN.md section 6) implements the global backend
   (backend = ADF_GLOBAL). A local value (backend = ADF_LOCAL, work package 1.8) is the same set,
   stored as raw residues in a caller-owned context (conventions 5.3, CV-55). Every function of this
   header is defined on sets and accepts inputs of either backend. Its result is global, except
   where conventions 5.3 keeps a result local: negation, sum and the products listed there, when
   all local inputs share one context pointer. Two different context pointers, or a raw result
   that needs other blocks, give a global result (the implicit fallback of conventions 4.6 and 5.3,
   gate finding G1). No function of this header creates or changes a context.

   Common rules for every function below, unless its comment says otherwise:
   - Aliasing (conventions 4.1): an output may be the same object as any input of the same type
     (adf_fball_add(x, x, x) is valid); inputs may alias each other; an output never aliases an
     input of another type.
   - Inputs satisfy predicate G (global) or L (local) of conventions 5.2, 5.3; otherwise the
     behaviour is undefined (conventions 4.4, CV-09).
   - Functions that cannot fail return void (conventions 2.3; 3.2 row 1). A function that returns a
     status leaves every output untouched on a status other than ADF_OK (conventions 4.3, CV-06).
   - Every output written satisfies G or L.
   - Enclosure: if every input set contains the true value, the output set contains the true
     result (docs/PLAN.md section 1, principle 5; policies Theorem 3, line 75).
   - Cost: M(n) denotes one multiplication of integers of n bits; "a gcd" is fmpz_gcd on integers
     of the size of the operands' A, H, d. */

#ifndef ADELEFELD_FBALL_H
#define ADELEFELD_FBALL_H

#include "adelefeld/common.h"
#include "adelefeld/status.h"
#include "adelefeld/place.h"
#include "adelefeld/rat.h"

/* Backend tags (conventions 5.2). */
#define ADF_GLOBAL 0
#define ADF_LOCAL  1

#ifdef __cplusplus
extern "C" {
#endif

/* Layout (conventions 5.2, 12.4, CV-40), 64-bit: A at offset 0, H at 8, d at 16 (fmpz, 8 bytes
   each), backend at 24 (int), mctx at 32, res at 40; size 48, alignment 8.

   Predicate G (backend = ADF_GLOBAL), conventions 5.2:
       mctx == NULL and res == NULL and d > 0 and H >= 0 and
       ((H > 0 and 0 <= A < H and gcd(A, H, d) = 1) or (H = 0 and gcd(A, d) = 1))
   Predicate L (backend = ADF_LOCAL), conventions 5.3:
       mctx != NULL and k >= 1 and H = K = q_1 ... q_k and A = 0 and res != NULL and d >= 1 and
       0 <= res[i] < q_i for all i          (k, q_i, K: the blocks and modulus of mctx)
   The local value (d; res) is the set (A0 + K Zhat)/d, A0 any integer with A0 = res[i] mod q_i
   (policies Definition 16, line 305; Lemma 17, line 312). res is owned by the value; mctx is
   borrowed (conventions 4.2). */
typedef struct
{
    fmpz_t A, H, d;
    int backend;
    const adf_modctx_struct * mctx;
    ulong * res;
} adf_fball_struct;

typedef adf_fball_struct adf_fball_t[1];
typedef adf_fball_struct * adf_fball_ptr;
typedef const adf_fball_struct * adf_fball_srcptr;

/* ---- life cycle (conventions 2.3) ---- */

/* adf_fball_init(x): x = the exact 0, (A, H, d) = (0, 0, 1), global (conventions 5.2, CV-13).
   Never fails. */
void adf_fball_init(adf_fball_t x);

/* adf_fball_clear(x): releases A, H, d and the residue array; never touches the context. */
void adf_fball_clear(adf_fball_t x);

/* adf_fball_set(y, x): y becomes a copy of x: same backend, same context pointer, same fields
   (conventions 2.3). y may be x. Cost: a copy of the integers and of the residue array. */
void adf_fball_set(adf_fball_t y, const adf_fball_t x);

/* adf_fball_swap(x, y): exchanges the contents, context pointers included; O(1), no allocation. */
void adf_fball_swap(adf_fball_t x, adf_fball_t y);

/* adf_fball_is_canonical(x): 1 if x satisfies G (backend ADF_GLOBAL) or L (backend ADF_LOCAL),
   else 0; never aborts, whatever the fields hold (conventions 2.3). For L it reads the blocks of
   mctx. Cost: a gcd for G; k word comparisons for L. */
int adf_fball_is_canonical(const adf_fball_t x);

/* adf_fball_identical(x, y): 1 if x and y have the same backend, the same context pointer and
   equal fields (A, H, d, and res[0..k-1] for a local value), else 0 (conventions 2.1, CV-02).
   This is representation identity, not equality of sets: use adf_fball_equal_set for that.
   Cost: comparison of the fields. */
int adf_fball_identical(const adf_fball_t x, const adf_fball_t y);

/* ---- constructors (global results) ---- */

/* adf_fball_zero(x): x = the exact 0, global. adf_fball_one(x): x = the exact 1, global. */
void adf_fball_zero(adf_fball_t x);
void adf_fball_one(adf_fball_t x);

/* adf_fball_set_si(x, n), adf_fball_set_fmpz(x, n): x = the exact integer n, (n, 0, 1), global. */
void adf_fball_set_si(adf_fball_t x, slong n);
void adf_fball_set_fmpz(adf_fball_t x, const fmpz_t n);

/* adf_fball_set_rat(x, q): x = the exact rational q, (num(q), 0, den(q)), global (SPEC 4.1: a
   ball of radius 0 is a single rational). Cost: a copy. */
void adf_fball_set_rat(adf_fball_t x, const adf_rat_t q);

/* adf_fball_set_fmpz3(x, A, H, d): x = the set (A + H Zhat)/d, stored as its canonical global
   triple (SPEC 4.1, D2; conventions 5.2). The raw triple may be non-canonical. A negative d is
   accepted: (A + H Zhat)/d = (-A + H Zhat)/(-d), since -Zhat = Zhat. The canonical triple is
   computed as in the reference tests/ref/adfref/fball.py function canon: make d > 0; for H = 0
   divide A, d by gcd(A, d); for H > 0 divide A, H, d by g = gcd(A, H, d), then reduce A into
   [0, H) (policies Summary 26, proof of the canonical column, line 564; conventions Lemma 5.2).
   Convention: constructor from raw data, conventions 3.2 (OK, DOMAIN), 4.4.
   Status: ADF_OK, x written; ADF_DOMAIN if d = 0 or H < 0, x untouched.
   Aliasing: A, H, d may alias each other. Cost: two gcds and one division with remainder. */
int adf_fball_set_fmpz3(adf_fball_t x, const fmpz_t A, const fmpz_t H, const fmpz_t d);

/* adf_fball_set_center_radius(x, c, N): x = c + N Zhat, global canonical (N = 0: the point c).
   Status: ADF_OK, x written; ADF_DOMAIN if N < 0, x untouched (conventions 4.4, negative radius).
   Source: tests/ref/adfref/fball.py, Fball.from_center_radius (common denominator of c and N).
   Cost: a gcd and the canonicalisation of adf_fball_set_fmpz3. */
int adf_fball_set_center_radius(adf_fball_t x, const adf_rat_t c, const adf_rat_t N);

/* adf_fball_canonicalise(x): brings a global value whose fields A, H, d were written directly (or
   by an underscore function) into predicate G, without changing its set (conventions 5, general
   rule: "adf_x_canonicalise after an underscore function"). Requires backend = ADF_GLOBAL,
   mctx = NULL, res = NULL. The computation is that of adf_fball_set_fmpz3 applied in place.
   For backend = ADF_LOCAL the input must already satisfy L; the function then returns ADF_OK and
   changes nothing (raw local data is the stored form, conventions 5.3, CV-55).
   Status: ADF_OK, x canonical; ADF_DOMAIN if d = 0 or H < 0, x untouched.
   Cost: as adf_fball_set_fmpz3. */
int adf_fball_canonicalise(adf_fball_t x);

/* ---- accessors (conventions 5.2: "No public function needs a scalar radius beyond the
   constructors and getters of Q") ---- */

/* adf_fball_is_exact(x): 1 if the radius is 0 (H = 0; a single rational), else 0. A local value is
   never exact (policies Definition 16). */
int adf_fball_is_exact(const adf_fball_t x);

/* adf_fball_get_fmpz3(A, H, d, x): (A, H, d) = the canonical global triple of the set of x
   (SPEC 4.1; conventions 5.2). For a local value this is (A0/g, K/g, d/g), A0 the CRT lift in
   [0, K), g = gcd(A0, K, d) (conventions 5.3; policies P24.1, line 457; Lemma 18, line 331).
   A, H, d are distinct fmpz initialised by the caller. Cost: none for global; a CRT recombination
   and a gcd for local. */
void adf_fball_get_fmpz3(fmpz_t A, fmpz_t H, fmpz_t d, const adf_fball_t x);

/* adf_fball_get_center(c, x): c = the stored centre a = A/d of the canonical triple: for N > 0 the
   unique element of a + N Z in [0, N); for N = 0 the point itself (conventions 5.2, "Stored
   representative"). Any other element of a + N Z is an equally valid centre of the same set. */
void adf_fball_get_center(adf_rat_t c, const adf_fball_t x);

/* adf_fball_get_radius(N, x): N = H/d >= 0, in lowest terms; 0 for an exact value. N is the
   positive generator of the radius ideal N Z (conventions 5.2, seams R2). */
void adf_fball_get_radius(adf_rat_t N, const adf_fball_t x);

/* adf_fball_get_den(d, x): d = the denominator d >= 1 of the canonical triple (SPEC 4.1: one
   denominator serves centre and radius). d is an fmpz initialised by the caller. */
void adf_fball_get_den(fmpz_t d, const adf_fball_t x);

/* adf_fball_prec_at(e, x, v): *e = v_p(N), the precision of x at the prime p of the place v
   (SPEC 4.1: "The precision at the prime p is v_p(H/d)"; conventions 5.2, seams R2). The ball
   constrains the coordinate at p exactly to a + p^(v_p(N)) Z_p. e may be negative.
   Status: ADF_OK, *e written; ADF_DOMAIN, *e untouched, if v is the archimedean place, or if x is
   exact (N = 0: the precision is infinite and no slong holds it).
   Cost: a p-adic valuation of H and of d (fmpz_remove). */
int adf_fball_prec_at(slong * e, const adf_fball_t x, adf_place_t v);

/* adf_fball_haar_volume(vol, x): vol = 1/N for N > 0, and 0 for an exact value, with the additive
   Haar measure normalised by vol(Zhat) = 1 (SPEC 9.3.7 row "Haar volume of a finite ball";
   docs/proofs/catalogue.md Proposition 10, line 202, modulo the local Haar scaling marked
   [source pending] there; conventions 6.2). Exact: vol is an exact rational. */
void adf_fball_haar_volume(adf_rat_t vol, const adf_fball_t x);

/* ---- tight arithmetic (SPEC 4.3; conventions 3.2 row 1: void) ---- */

/* adf_fball_add(z, x, y): z = x + y as sets:
       (a + N Zhat) + (b + M Zhat) = (a + b) + gcd(N, M) Zhat.
   The result is the exact set of sums, hence the smallest ball containing them (tight).
   Source: docs/proofs/precision.md Proposition 1 (line 27), with Lemma 2 (line 20).
   Local inputs at one context pointer: the local sum of policies P21.2 (line 386), denominator
   lcm(d, e), exact and tight (conventions 5.3 table). Cost: two gcds and a constant number of
   multiplications of the operand sizes. */
void adf_fball_add(adf_fball_t z, const adf_fball_t x, const adf_fball_t y);

/* adf_fball_sub(z, x, y): z = x - y = x + (-y) as sets; tight (precision.md Proposition 1 with
   negation -(b + M Zhat) = -b + M Zhat, policies Theorem 3 step 2, line 88). Cost: as add. */
void adf_fball_sub(adf_fball_t z, const adf_fball_t x, const adf_fball_t y);

/* adf_fball_neg(y, x): y = -x = -a + N Zhat, the same radius (policies Theorem 3 step 2, line 88;
   policies P21.1 for a local input: (d; -r_i mod q_i), same context). Exact. Cost: linear. */
void adf_fball_neg(adf_fball_t y, const adf_fball_t x);

/* adf_fball_mul(z, x, y): z = the ball a b + G Zhat, G = gcd(a M, b N, N M), which contains every
   product of an element of x with an element of y, and is contained in every ball that does
   (tight for independent inputs). If G = 0 the product set is the single point a b.
   Source: docs/proofs/precision.md Proposition 2 (line 34); policies Lemma 1 (line 43).
   Local inputs at one context pointer: conventions 5.3 table, row "product" (policies P22, line
   407): local when h = 1 or h divides d e, otherwise global; the result is always the tight
   product, never the blockwise enclosure. Cost: a constant number of multiplications and two gcds
   of the operand sizes. */
void adf_fball_mul(adf_fball_t z, const adf_fball_t x, const adf_fball_t y);

/* adf_fball_mul_rat(y, x, q): y = q x = q a + |q| N Zhat for q != 0, and the exact 0 for q = 0.
   Exact as a set (multiplication by q != 0 is a bijection of the finite adeles taking Zhat to
   |q| Zhat; precision.md Proposition 6(2), line 106, and Proposition 2 with radius 0 for the
   scalar, as in the Counterexample, line 122). Examples (SPEC 4.3): 12 * (5 mod 18) = 60 mod 216;
   (1/3) * (5 mod 18) = 5/3 mod 6.
   Local input: conventions 5.3 row "exact scalar" (policies P23, line 443): local if |m| divides d
   for q = m/n, otherwise global. Cost: a constant number of multiplications and one gcd. */
void adf_fball_mul_rat(adf_fball_t y, const adf_fball_t x, const adf_rat_t q);

/* adf_fball_div_rat(y, x, q): y = (1/q) x, as adf_fball_mul_rat with 1/q.
   Convention: conventions 3.2 row "scaling by the inverse of an exact rational" (OK, NOT_UNIT).
   Status: ADF_OK, y written; ADF_NOT_UNIT if q = 0 (proved: 0 is not invertible), y untouched.
   Cost: as mul_rat. */
int adf_fball_div_rat(adf_fball_t y, const adf_fball_t x, const adf_rat_t q);

/* ---- set predicates (SPEC 4.2; conventions 2.1: the verbs equal_set, overlaps, contains;
   3.2: they return int 0 or 1, no status). Radius zero is a single point (precision.md
   Proposition 3, line 61). Source for all three: docs/proofs/precision.md Proposition 3 (line 59).
   Both backends: the predicates compare sets, through the canonical triple (conventions 5.3). ---- */

/* adf_fball_equal_set(x, y): 1 if the sets are equal: N = M and (a - b)/N an integer (N, M > 0);
   two points are equal exactly when they are the same rational; else 0. Cost: a division and an
   integrality test. */
int adf_fball_equal_set(const adf_fball_t x, const adf_fball_t y);

/* adf_fball_overlaps(x, y): 1 if the sets meet: (a - b)/gcd(N, M) an integer (gcd > 0); two points
   meet exactly when equal; else 0. Not transitive (SPEC 4.2). Cost: a gcd and a division. */
int adf_fball_overlaps(const adf_fball_t x, const adf_fball_t y);

/* adf_fball_contains(x, y): 1 if the set x is inside the set y ("first inside second", SPEC 4.2):
   for M > 0, N/M and (a - b)/M integers (N = 0: only the second condition); for M = 0, N = 0 and
   a = b; else 0. Cost: two divisions. */
int adf_fball_contains(const adf_fball_t x, const adf_fball_t y);

/* adf_fball_contains_rat(x, q): 1 if the rational q lies in the set x, else 0: for N > 0 exactly
   when (q - a)/N is an integer (precision.md Lemma 1, consequence, line 18); for N = 0 exactly
   when q = a. Cost: one division. */
int adf_fball_contains_rat(const adf_fball_t x, const adf_rat_t q);

/* adf_fball_compare(x, y): three-valued comparison of two unknown points, one in each ball
   (SPEC 4.2; conventions 2.1; closure finding C1). Returns
       ADF_CMP_EQUAL (0)      if both are exact and equal;
       ADF_CMP_DIFFERENT (1)  if the sets are disjoint (adf_fball_overlaps is 0);
       ADF_CMP_UNDECIDED (2)  otherwise (the sets meet and one of them is not a point).
   Not a status; never any other value. Source: tests/ref/adfref/fball.py function compare.
   Cost: as adf_fball_overlaps. */
int adf_fball_compare(const adf_fball_t x, const adf_fball_t y);

/* Layout queries (conventions 12.4, CV-40). Header-inline and exported. */
ADF_INLINE size_t adf_sizeof_fball(void) { return sizeof(adf_fball_struct); }
ADF_INLINE size_t adf_alignof_fball(void) { return ADF_ALIGNOF(adf_fball_struct); }

#ifdef __cplusplus
}
#endif

#endif /* ADELEFELD_FBALL_H */
