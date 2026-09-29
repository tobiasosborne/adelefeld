/* adelefeld/lball.h: adf_lball, a ball in Q_p at one prime (milestone 1F, work package 1F.3, first slice).

   Contract: docs/conventions.md 0.4, 5.8 (struct, predicate, init), 2.1 (verbs of the set predicates), 2.3 (life
   cycle), 3.1 and 3.2 (statuses), 4.1 (aliasing), 4.3 (outputs after a status), 7 (places), 12 (layout queries);
   docs/SPEC.md 4.1, 9.3.6 (valuation, absolute value, unit part); docs/proofs/functions.md Definition 1 (line 11:
   balls a + p^N Z_p, N any integer) and Proposition 4 (line 92: the decomposition of a point); the statements
   L1 to L8 of docs/api-1f.md, section "Statements to add to functions.md" (the projection of a finite ball to a
   prime; sum, negation, product and inverse of balls; the decomposition of a ball; the predicates). They are
   proved there, stepwise, because docs/proofs/functions.md has no statement for them.

   Meaning of a value (conventions 5.8), p = the prime, v, N slong:
     exact = 1:  the single rational p^v u of Q_p, u a rational that is a unit at p (or the exact 0);
     exact = 0:  the ball p^v u + p^N Z_p = { x in Q_p : v_p(x - p^v u) >= N }; u is an integer prime to p with
                 0 < u < p^(N - v) and v < N (so the ball does not contain 0), or u = 0 = v (the ball p^N Z_p
                 around 0). The centre p^v u is the unique element of Z[1/p] in [0, p^N) that lies in the ball.
   N is the absolute precision: "p^N Z_p" is the error set. A larger N is a smaller ball. Exact 0 and the ball
   O(p^N) around 0 are different values.

   Result rule of the slice: every arithmetic function returns the SMALLEST ball that contains the set of all
   results (sum, product, quotient of one point of each input set), and the exact value when both inputs are exact
   (proved in api-1f.md L2 to L5; tested by enumeration modulo p^(K + 1)). "Tight" in the sense of
   proofs/precision.md, at one prime.

   Limits (conventions 3.1, ADF_LIMIT). The stored form of a ball is an integer u < p^(N - v). A function that
   would need an integer p^k with k bits(p) > ADF_LBALL_BITS_MAX returns ADF_LIMIT before it allocates; so does a
   function whose input has |v| or |N| above ADF_LBALL_EXP_MAX, or whose result would (the sums and differences of
   exponents that the functions form are then far from overflowing slong). is_canonical, init, clear, set, swap,
   identical and the layout queries have no such limit.

   Common rules, unless a comment says otherwise:
   - Aliasing (conventions 4.1): an output may be the same object as any input of the same type; inputs may alias
     each other. Two outputs never alias each other.
   - Two operands of one binary function must be at the same prime; otherwise ADF_DOMAIN (a stated compatibility
     requirement, conventions 3.1) and the outputs are untouched. The set predicates return 0 for two different
     primes (the sets lie in different fields).
   - Inputs satisfy the predicate of conventions 5.8 (adf_lball_is_canonical); otherwise the behaviour is undefined
     (conventions 4.4, CV-09). Every output written satisfies it.
   - A function that returns a status leaves every output untouched on a status other than ADF_OK (CV-06).
   - Cost: M(n) is one multiplication of integers of n bits; "a mod" is a reduction modulo p^k, k = the relative
     precision of the result, with one modular inverse when the unit part is not an integer. */

#ifndef ADELEFELD_LBALL_H
#define ADELEFELD_LBALL_H

#include "adelefeld/common.h"
#include "adelefeld/status.h"
#include "adelefeld/place.h"
#include "adelefeld/rat.h"
#include "adelefeld/fball.h"

/* Bounds of the slice (see "Limits" above). 2^60; 2^26 bits = 8 MiB. */
#define ADF_LBALL_EXP_MAX  ((slong) 1152921504606846976)
#define ADF_LBALL_BITS_MAX ((slong) 67108864)

#ifdef __cplusplus
extern "C" {
#endif

/* Layout (conventions 5.8, 12.4, CV-40), 64-bit: p at offset 0 (ulong), u at 8 (fmpq, 16 bytes: num at 8, den at
   16), v at 24 (slong), N at 32 (slong), exact at 40 (int); size 48, alignment 8.

   Predicate (conventions 5.8):
       p is prime and 2 <= p < 2^64 and exact in {0, 1} and fmpq_is_canonical(u) and
       exact = 1:  N = 0 and ( (u = 0 and v = 0) or (p divides neither numerator nor denominator of u) )
       exact = 0:  u is an integer and ( (u = 0 and v = 0) or (v < N and p does not divide u and 0 < u < p^(N - v)) )
   The field p is read through adf_lball_place; the fields are otherwise the contract of a binding that allocates
   the struct inline (conventions 12.4). */
typedef struct
{
    ulong p;
    fmpq_t u;
    slong v;
    slong N;
    int exact;
} adf_lball_struct;

typedef adf_lball_struct adf_lball_t[1];
typedef adf_lball_struct * adf_lball_ptr;
typedef const adf_lball_struct * adf_lball_srcptr;

/* ---- life cycle (conventions 2.3) ---- */

/* adf_lball_init(x): x = the exact 0 at p = 2 (conventions 5.8, CV-19). Never fails. */
void adf_lball_init(adf_lball_t x);

/* adf_lball_clear(x): releases the memory of x. */
void adf_lball_clear(adf_lball_t x);

/* adf_lball_set(y, x): y becomes a copy of x. y may be x. */
void adf_lball_set(adf_lball_t y, const adf_lball_t x);

/* adf_lball_swap(x, y): exchanges the contents; O(1), no allocation. */
void adf_lball_swap(adf_lball_t x, adf_lball_t y);

/* adf_lball_is_canonical(x): 1 if x satisfies the predicate above, else 0. It never aborts for an initialised
   object whose fields hold any values (conventions 2.3). It compares u with p^(N - v) without forming that power
   when the power is larger than u (a comparison of bit lengths), so it allocates at most twice the size of u.
   Cost: a primality test of one word, a gcd-free comparison, one power at most. */
int adf_lball_is_canonical(const adf_lball_t x);

/* adf_lball_identical(x, y): 1 if all fields are equal (representation identity, conventions 2.1, CV-02), else 0.
   For canonical inputs this coincides with adf_lball_equal_set (L8). */
int adf_lball_identical(const adf_lball_t x, const adf_lball_t y);

/* ---- constructors ---- */

/* adf_lball_set_rat(x, v, q): x = the exact rational q of Q_p, p = the prime of the place v; stored as p^w u,
   w = v_p(q), u = q/p^w (a unit at p); 0 is stored with u = 0, w = 0.
   Status: ADF_OK, x written; ADF_DOMAIN if v is the archimedean place, x untouched. No other status.
   Aliasing: q is not an lball; x and q are distinct objects. Cost: two valuations of a numerator and a denominator. */
int adf_lball_set_rat(adf_lball_t x, adf_place_t v, const adf_rat_t q);

/* adf_lball_set_rat_ball(x, v, c, N): x = the ball c + p^N Z_p, p = the prime of v, N any slong ("absolute
   precision N": the error set is p^N Z_p). Stored as the canonical centre p^w u in [0, p^N) with u < p^(N - w)
   an integer prime to p, or, if v_p(c) >= N (c = 0 included), as the ball O(p^N) around 0 (u = 0, v = 0).
   Source of the form: conventions 5.8 ("the stored ball centre"); the reduction is api-1f.md L1, step 1.
   Status: ADF_OK, x written; ADF_DOMAIN if v is the archimedean place; ADF_LIMIT if |N| > ADF_LBALL_EXP_MAX or if
   the reduction needs p^k with k = N - v_p(c) and k bits(p) > ADF_LBALL_BITS_MAX (an integer c >= 0 is stored
   without forming p^k when c < 2^(k (bits(p) - 1))); x untouched on both.
   Cost: a mod. */
int adf_lball_set_rat_ball(adf_lball_t x, adf_place_t v, const adf_rat_t c, slong N);

/* adf_lball_set_fball(x, v, f): x = the projection of the finite ball f = (A + H Zhat)/d to the prime p of v
   (api-1f.md L1): the ball A/d + p^N Z_p with N = v_p(H) - v_p(d) if H > 0, and the exact rational A/d if H = 0
   (f exact). f may be a global or a local value (its canonical triple is used, fball.h adf_fball_get_fmpz3).
   The set of the p-th coordinates of the elements of f is exactly this ball (L1: the projection Zhat -> Z_p is onto).
   Status: ADF_OK; ADF_DOMAIN if v is the archimedean place; ADF_LIMIT as adf_lball_set_rat_ball. x untouched
   otherwise. Cost: two valuations and a mod. */
int adf_lball_set_fball(adf_lball_t x, adf_place_t v, const adf_fball_t f);

/* ---- accessors ---- */

/* adf_lball_place(x): the place of the prime of x (conventions 5.8: "the prime is read through the place"). */
adf_place_t adf_lball_place(const adf_lball_t x);

/* adf_lball_is_exact(x): 1 if x is an exact rational (exact = 1), else 0. */
int adf_lball_is_exact(const adf_lball_t x);

/* adf_lball_contains_zero(x): 1 if 0 lies in the set x: exact 0, or a ball with u = 0. Else 0. */
int adf_lball_contains_zero(const adf_lball_t x);

/* adf_lball_get_prec(N, x): *N = the absolute precision N of a ball.
   Status: ADF_OK, *N written; ADF_DOMAIN, *N untouched, if x is exact (the precision is infinite). */
int adf_lball_get_prec(slong * N, const adf_lball_t x);

/* adf_lball_get_center(c, x): c = the rational p^v u, the value of an exact x, or the canonical centre of a ball
   (0 for a ball around 0).
   Status: ADF_OK; ADF_LIMIT, c untouched, if |v| bits(p) > ADF_LBALL_BITS_MAX (the power p^|v| would be too big).
   Cost: a power and a multiplication. */
int adf_lball_get_center(adf_rat_t c, const adf_lball_t x);

/* ---- set predicates (conventions 2.1, 3.2: int 0 or 1, no status; api-1f.md L8) ---- */

/* adf_lball_equal_set(x, y): 1 if the sets are equal, else 0. Two balls are equal exactly when they have the same
   prime, N and centre; an exact value and a ball are never equal. Cost: a comparison of the fields. */
int adf_lball_equal_set(const adf_lball_t x, const adf_lball_t y);

/* adf_lball_overlaps(x, y): 1 if the sets meet: exact-exact: equal; exact-ball: the point lies in the ball; ball-ball:
   v_p(c - c') >= min(N, N'). Not transitive. No power of p is formed. */
int adf_lball_overlaps(const adf_lball_t x, const adf_lball_t y);

/* adf_lball_contains(x, y): 1 if the set x is inside the set y ("first inside second", fball.h): y exact: x exact
   and equal; y a ball: x exact with v_p(x - c') >= N', or x a ball with N >= N' and v_p(c - c') >= N'. No power of
   p is formed. */
int adf_lball_contains(const adf_lball_t x, const adf_lball_t y);

/* ---- arithmetic. Each result is the smallest ball containing the set of results (L2 to L5); two exact operands
   give the exact result. ---- */

/* adf_lball_neg(y, x): y = -x. Ball: -c + p^N Z_p (L5). Status: ADF_OK; ADF_LIMIT as in set_rat_ball (a mod with
   k = N - v). Cost: a mod. */
int adf_lball_neg(adf_lball_t y, const adf_lball_t x);

/* adf_lball_add(z, x, y): z = x + y. Ball + ball: (c + c') + p^min(N, N') Z_p; exact + ball: (q + c) + p^N Z_p
   (L2). Exact + exact: the exact sum. The centre is reduced modulo p^min(N, N'): an operand whose valuation is
   at least that minimum contributes 0 and its power of p is not formed.
   Status: ADF_OK; ADF_DOMAIN (different primes); ADF_LIMIT (bounds above; two exact operands whose exponents
   differ so much that the exact sum needs a power p^k with k bits(p) > ADF_LBALL_BITS_MAX). Cost: a mod. */
int adf_lball_add(adf_lball_t z, const adf_lball_t x, const adf_lball_t y);

/* adf_lball_sub(z, x, y): z = x - y = x + (-y); tight by L5 and L2. Status: as add. Cost: as add. */
int adf_lball_sub(adf_lball_t z, const adf_lball_t x, const adf_lball_t y);

/* adf_lball_mul(z, x, y): z = x y (L3). With centres c, c' of valuation v, v' (infinity for a centre 0) and
   exponents N, N' (infinity for an exact operand), the result has centre c c' and exponent
   K = min(v + N', v' + N, N + N'); the terms with an infinite part are absent. An exact 0 times anything is the
   exact 0 (the set {0}). Exact times exact is exact. The exponent K is formed from v, v', N, N' only.
   Status: ADF_OK; ADF_DOMAIN (different primes); ADF_LIMIT (bounds above, or a mod with k > the bit bound).
   Cost: one multiplication of the unit parts and a mod. */
int adf_lball_mul(adf_lball_t z, const adf_lball_t x, const adf_lball_t y);

/* adf_lball_inv(y, x): y = 1/x (L4). Exact x != 0: the exact rational 1/x. Ball with v < N (0 not in the ball):
   the ball 1/c + p^(N - 2v) Z_p, the exact set of inverses (relative precision N - v is kept).
   Status: ADF_OK; ADF_NOT_UNIT if x is the exact 0 (proved: not invertible, conventions 3.2), y untouched;
   ADF_UNIT_NOT_CERTIFIED if x is a ball that contains 0 (the set of inverses of the non-zero points is not a
   ball; conventions 3.1: "the enclosure does not prove invertibility", SPEC 4.5), y untouched;
   ADF_LIMIT (bounds, or a mod with k too large). Cost: a modular inverse. */
int adf_lball_inv(adf_lball_t y, const adf_lball_t x);

/* adf_lball_div(z, x, y): z = x / y = x (1/y); tight: the set of quotients equals the set of products of x with the
   set of inverses of y (L4), and the product of two independent balls is tight (L3). Status: as inv, plus
   ADF_DOMAIN for different primes (checked first). Outputs untouched on every status other than ADF_OK. */
int adf_lball_div(adf_lball_t z, const adf_lball_t x, const adf_lball_t y);

/* ---- valuation, absolute value, decomposition (SPEC 9.3.6; L6, L7) ---- */

/* adf_lball_valuation(v, is_inf, x): the valuation v_p of the elements of x. Exact x != 0: v_p(x), *is_inf = 0.
   Exact 0: *is_inf = 1 and *v = 0 (the valuation is infinity). Ball with v < N: every element has valuation v
   (L7), *is_inf = 0, *v = v.
   Status: ADF_OK, both written; ADF_NOT_DETERMINED if x is a ball that contains 0 (its elements have different
   valuations, SPEC 9.3.6), outputs untouched. Cost: constant. */
int adf_lball_valuation(slong * v, int * is_inf, const adf_lball_t x);

/* adf_lball_abs(a, x): a = |x|_p = p^(-v) as an exact rational, the same for all elements of x (L7); the exact 0 gives
   0. Status: ADF_OK; ADF_NOT_DETERMINED if x is a ball that contains 0; ADF_LIMIT if |v| bits(p) >
   ADF_LBALL_BITS_MAX. Outputs untouched on a status other than OK. Cost: a power. */
int adf_lball_abs(adf_rat_t a, const adf_lball_t x);

/* adf_lball_decompose(m, unit, x): x = p^m t for the unit ball or unit t (L6). Exact x != 0: m = v_p(x) and unit = the
   exact unit x/p^m. Ball with v < N: m = v and unit = u + p^(N - v) Z_p, the ball of the unit parts, of
   relative precision N - v >= 1; every element of x is p^m times exactly one element of it, and every element
   of it arises. Proposition 4 (functions.md line 92) then splits each unit into w u' (w a root of unity, u' a
   principal unit); that second split is not in this slice.
   Status: ADF_OK, both written; ADF_DOMAIN if x is the exact 0 (Proposition 4: zero has no such decomposition),
   ADF_NOT_DETERMINED if x is a ball that contains 0; ADF_LIMIT if N - v > ADF_LBALL_EXP_MAX. Outputs untouched
   otherwise. m and unit are distinct; unit may alias x. Cost: constant plus a copy. */
int adf_lball_decompose(slong * m, adf_lball_t unit, const adf_lball_t x);

/* Layout queries (conventions 12.4, CV-40). Header-inline and exported. */
ADF_INLINE size_t adf_sizeof_lball(void) { return sizeof(adf_lball_struct); }
ADF_INLINE size_t adf_alignof_lball(void) { return ADF_ALIGNOF(adf_lball_struct); }

#ifdef __cplusplus
}
#endif

#endif /* ADELEFELD_LBALL_H */
