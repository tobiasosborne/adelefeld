/* adelefeld/roots.h: roots of an integer polynomial at a place, with certificates (milestone S, S.2,
   slice 1: the type adf_rootlist and the seed function; slice 2: all roots at a prime).

   Contract: docs/api-s.md sections 1 and 4 (decisions S-D10, S-D13, S-D14, S-D15, S-D16, S-D17, S-D18
   of docs/SPEC.md 15.3); docs/proofs/solvers.md Lemma 3.1 (line 1013), Definition and Proposition 3.2
   (line 1044), Proposition 3.3 (line 1090), Proposition 3.4 (line 1121), Algorithm P and
   Proposition 3.5 (line 1160), Proposition 3.6 (line 1246), Proposition 3.7 (line 1284), 3.11
   (line 1449), Proposition 3.12 (line 1487), Proposition 3.13 (line 1524); docs/conventions.md 2.3,
   3.2, 4.1, 4.3, 7. Implemented in src/roots.c; tests tests/test_roots_seed.c (slice 1) and
   tests/test_roots_padic.c (slice 2); reference proto/solvers_checks.py, functions squarefree_part
   (line 2377), normalise_g (1701), root_cert_ok (1631), newton_step (1639), padic_roots (1649),
   rootlist_padic (1707), seed_root (1721), padic_verify_entries (1754), padic_verify_complete (1776).

   Slice 1 declares the type, its life cycle without set, swap and identical, the seed function, the
   entries verifier and the accessors. Slice 2 adds adf_roots_padic, adf_roots_padic_partial,
   adf_rootlist_get_unresolved and adf_rootlist_verify_complete. Not declared yet: adf_roots_real,
   adf_rootlist_get_arb, set, swap, identical.

   The normalised polynomial (solvers L3.1(3), decision S-D13). For f in Z[X], f not 0, g* is
   f / gcd(f, f') scaled to a primitive integer polynomial with positive leading coefficient; for f
   constant, g* = 1. g* has the roots of f in Z_p and in R, every one of them simple, and
   gcd(g*, g*') = 1 in Q[X] (L3.1(2)). Every certificate of a list refers to g*, never to f.

   The type adf_rootlist holds the roots of one polynomial at one place, with what proves them:

     place     always      the prime p, or the real place (adf_place_t, conventions 7)
     scope     always      ADF_ROOTLIST_PARTITION (0): the balls and unresolved classes cover every
                           root of the input at this place, and complete = 1 exactly when nu = 0;
                           ADF_ROOTLIST_SEED (1): the list of adf_root_padic_from_seed, n = 1,
                           nu = 0, complete = 0, no statement about other roots (S-D16)
     reduced   always      1 iff f has a repeated irreducible factor over Q, that is
                           deg gcd(f, f') > 0 for nonconstant f; 0 for constant f. Multiplication by
                           a constant and removal of content do not set it
     complete  always      1: the list holds every root of the input at this place; 0: it may not
     g         always      the normalised polynomial g* of the input
     n         always      the number of roots in the list, n >= 0
     a, K, s   at a prime  arrays of n: the root certificate (a[i], K[i], s[i]) of root i (solvers
                           D3.2): the ball a[i] + p^K[i] Z_p holds exactly one root of g, in
                           increasing order of a[i]; NULL when n = 0
     nu        always      the number of unresolved classes; 0 at the real place
     ua, ue    at a prime  arrays of nu: the unresolved classes ua[i] + p^ue[i] Z_p (solvers P3.5),
                           in increasing order of ua[i]; NULL when nu = 0. Only
                           adf_roots_padic_partial makes a class
     ball      real place  array of n isolating balls (arb) in increasing order; NULL when n = 0 and
                           at a prime. No function of this slice makes a real ball
     count     real place  the number of distinct real roots of g; 0 at a prime

   Predicate (adf_rootlist_is_canonical, without the input polynomial):
     - scope in {PARTITION, SEED}; reduced and complete in {0, 1}; n >= 0, nu >= 0, count >= 0;
     - g is its own normalised polynomial: not 0, content 1, leading coefficient > 0, and
       gcd(g, g') = 1 (g = 1 if g is constant);
     - every array pointer is NULL exactly when its length is 0 (a, K, s for n at a prime; ua, ue
       for nu; ball for n at the real place); at a prime ball is NULL and count = 0; at the real
       place a, K, s, ua, ue are NULL and nu = 0;
     - PARTITION: complete = 1 exactly when nu = 0; SEED: at a prime, n = 1, nu = 0, complete = 0;
     - at a prime: (R1) for every certificate, 0 <= a[i] < p^K[i] and K[i] > s[i] >= 0; the centres
       strictly increasing; 0 <= ua[i] < p^ue[i], ue[i] >= 0, the ua[i] strictly increasing; the
       balls and classes pairwise disjoint (solvers P3.2(4));
     - at the real place (temporary, until the slice of adf_roots_real): scope PARTITION,
       complete = 1, n = count, the balls finite and hi_i < lo_(i+1) (arb_lt, arb.rst:705).
   Init value: the real place, scope PARTITION, reduced = 0, complete = 1, g = 1, n = nu = count = 0,
   every pointer NULL (the constant 1 has no root).

   The prime. A finite place is made by adf_place_prime (place.h:30 to 36), which certifies the
   primality of p with n_is_prime (ulong_extras.h:335, proved for every word: src/place.c:23 to 28)
   and takes p as a ulong, so 2 <= p < 2^64 (conventions 7, lines 1021 to 1022). The functions of
   this header rely on exactly this: p is a prime, and its size is at most one word because the
   type allows no other. The functions that compute make no primality test of their own. The predicate
   adf_rootlist_is_canonical and the two verifiers are called on values of unknown origin: they repeat
   the test of adf_place_prime and return 0 for a place whose word is not a prime
   (docs/reviews/s2/review.md, finding 1). The seed function and the entries
   verifier need no roots modulo p and no bound on p (decision S-D10: every prime); the search for all
   roots of slice 2 finds the roots modulo p by evaluation at every residue and has the TEMPORARY bound
   ADF_ROOTS_P_EVAL_MAX (below). A place not made by the functions of place.h is outside the contract
   (conventions 7, line 1014). A prime of more than one word cannot be passed through adf_place_t in
   version 1 (HEADER-FINDING of lane s2-slice1: S-D10 speaks of every prime, the place type admits the
   primes below 2^64).

   Limits (decision S-D18). Every exponent (s + 1, max(prec_p, s + 1), 2 k - s, k + s, 2 K) is
   computed with checked slong arithmetic. A precision K with 2 K bits(p) > ADF_ROOTS_BITS_MAX gives
   ADF_LIMIT; 2 K bits(p) is the bound used for the bit length of p^(2 K), the largest power of p the
   seed function forms. The value of the limit is a policy, not a measured budget. The search of
   slice 2 applies the same bound to every precision K = max(prec_p, s + 1) of a root it certifies and
   to the exponent e of every class it opens or leaves unresolved (2 e bits(p) > ADF_ROOTS_BITS_MAX
   gives ADF_LIMIT), and also computes e + 1, w - e, w - 2 e, e + j and the valuations w with checked
   arithmetic. */

#ifndef ADELEFELD_ROOTS_H
#define ADELEFELD_ROOTS_H

#include <flint/fmpz_poly.h>
#include <flint/arb.h>

#include "adelefeld/common.h"
#include "adelefeld/status.h"
#include "adelefeld/place.h"
#include "adelefeld/fball.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ADF_ROOTLIST_PARTITION 0
#define ADF_ROOTLIST_SEED 1

/* S-D18: the bound of 2 K bits(p), 2^24 bits (2 MiB for one integer). */
#define ADF_ROOTS_BITS_MAX 16777216

/* S-D10, TEMPORARY: the largest prime for which adf_roots_padic and adf_roots_padic_partial find the
   roots modulo p by evaluation at every residue (solvers P3.7(1)), 2^20. Above it they return
   ADF_UNSUPPORTED. The bound is a property of this slice, not of the library: it goes away with the
   slice that adds the route of solvers P3.7(2) for larger primes. */
#define ADF_ROOTS_P_EVAL_MAX 1048576

/* Layout (64 bit, fixed by this slice and pinned in tests/test_roots_seed.c): 120 bytes, alignment 8.
   place at 0, scope at 8, reduced at 12, complete at 16 (then 4 bytes of padding), g at 24 (an
   fmpz_poly_struct of 24 bytes: coeffs, alloc, length; FLINT 3.0.1 fmpz_types.h), n at 48, a at 56,
   K at 64, s at 72, nu at 80, ua at 88, ue at 96, ball at 104, count at 112. The value owns g and the
   arrays. The arrays come from FLINT's allocator: a and ua from _fmpz_vec_init (fmpz_vec.h), K, s and
   ue from flint_malloc, ball from _arb_vec_init; clear releases them with _fmpz_vec_clear, flint_free
   and _arb_vec_clear with the lengths n and nu. */
typedef struct
{
    adf_place_t place;
    int scope;
    int reduced;
    int complete;
    fmpz_poly_t g;
    slong n;
    fmpz * a;
    slong * K;
    slong * s;
    slong nu;
    fmpz * ua;
    slong * ue;
    arb_ptr ball;
    slong count;
} adf_rootlist_struct;

typedef adf_rootlist_struct adf_rootlist_t[1];
typedef adf_rootlist_struct * adf_rootlist_ptr;
typedef const adf_rootlist_struct * adf_rootlist_srcptr;

/* ---- life cycle (conventions 2.3) ---- */

/* adf_rootlist_init(L): the init value above. Allocates what fmpz_poly_set_ui(g, 1) allocates. */
void adf_rootlist_init(adf_rootlist_t L);

/* adf_rootlist_clear(L): releases g and the arrays; afterwards L may only be passed to init. */
void adf_rootlist_clear(adf_rootlist_t L);

/* adf_rootlist_is_canonical(L): 1 if the predicate above holds, else 0. Never aborts for an
   initialised object whose integer fields hold any values, provided every pointer is NULL or points
   to an array of initialised entries of the length its field states. It never forms a power p^K
   with K above the bit length of the centre it is compared with. Cost: one gcd of g and g'; n + nu
   comparisons with powers of p; (n + nu)^2 valuations for the disjointness. */
int adf_rootlist_is_canonical(const adf_rootlist_t L);

/* ---- the seed function ---- */

/* adf_root_padic_from_seed(L, f, p, a, prec_p): the one root of f near the seed a, if the strong form
   of Hensel's lemma holds for g, the normalised polynomial of f (solvers P3.12; (H2), Conrad
   Theorem 4.1, conrad-hensel:hensel.txt:315 to 319).

   The condition, s and the certificate refer to g (S-D13, S-D16): if g'(a) != 0 and
   v_p(g(a)) > 2 s with s = v_p(g'(a)) (g(a) = 0 allowed), there is exactly one root alpha of g in
   a + p^(s+1) Z_p; it is a root of f, simple for g, and v_p(g'(alpha)) = s (P3.12(1)). L becomes the
   list of one ball a' + p^K Z_p with K = max(prec_p, s + 1), 0 <= a' < p^K, a' = alpha modulo p^K
   (so a' = a modulo p^(s+1)) and the root certificate (a', K, s) for g (P3.12(2)). K is sufficient for
   isolation, not the least isolating precision (solvers 3.11(5)). The list has scope SEED: n = 1,
   nu = 0, complete = 0, place p, g the normalised polynomial, reduced as above (P3.12(5)).
   Computation (P3.12(2), S-D18): if g(a) = 0 or k0 = v_p(g(a)) - s >= K, then a' = a mod p^K, and
   p^k0 is never formed; otherwise (a mod p^k0, k0, s) is a certificate and Newton steps
   (a, k) -> (a - p^k F u mod p^(2k - s), 2 k - s) of P3.3 lift it until k >= K; then it is reduced
   modulo p^K (P3.2(3)).

   Statuses, in the order in which they are decided:
     ADF_DOMAIN (edit E-C1): f = 0, prec_p < 1, or p is the real place; L untouched.
     ADF_LIMIT: 2 prec_p bits(p) > ADF_ROOTS_BITS_MAX, decided before any allocation; L untouched.
     ADF_NOT_DETERMINED: g'(a) = 0, or v_p(g(a)) <= 2 v_p(g'(a)) (the seed does not satisfy the
       strong form for g; P3.12(4)); decided after g, g(a) and g'(a) are formed; L untouched.
     ADF_LIMIT: 2 K bits(p) > ADF_ROOTS_BITS_MAX with K = max(prec_p, s + 1) (only when s + 1 >
       prec_p), or an overflow of slong in an exponent; decided before any power of p is formed;
       L untouched.
     ADF_OK: L written as above.
   ADF_UNSUPPORTED (api-s.md 4) is not returned by this function: it needs no roots modulo p, so no
   bound on p applies. p is any prime of a place (see "The prime" above).

   Aliasing: L is an output of its own type; f and a are inputs and may be fields of L (f = L->g,
   a = L->a + 0): L is written only after the result has been computed. Allocates: g*, g(a) and
   g'(a) exactly, powers of p up to p^(2K), and the arrays of L. Cost: one gcd of f and f', two exact
   evaluations at a, O(log(K / (k0 - s))) Newton steps modulo p^(2K) at most. */
int adf_root_padic_from_seed(adf_rootlist_t L, const fmpz_poly_t f, adf_place_t p, const fmpz_t a,
                             slong prec_p);

/* ---- all roots at a prime (slice 2) ---- */

/* adf_roots_padic_partial(L, f, p, prec_p, depth) (decision S-D15): Algorithm P of solvers 3.5 on g,
   the normalised polynomial of f, through the depth limit depth. Every root of f in Z_p lies in
   exactly one listed ball or unresolved class (P3.5(2)); the balls and classes are pairwise disjoint.

   The list, scope PARTITION, place p, g and reduced as above:
     - n balls a[i] + p^K[i] Z_p, each with the root certificate (a[i], K[i], s[i]) for g (P3.5(1)):
       the ball holds exactly one root of g, a simple one with v_p(g'(alpha)) = s[i], and
       K[i] = max(prec_p, s[i] + 1) (sufficient for isolation, not the least isolating precision:
       solvers 3.11(5)); a[i] = alpha mod p^K[i]; in increasing order of a[i];
     - nu classes ua[i] + p^ue[i] Z_p that Algorithm P left open at the depth limit (step 3,
       "e + 1 > D"; classes are opened at the levels 0 to depth only, so ue[i] = depth + 1); a class
       may hold no root, one or several (P3.5(5)); in increasing order of ua[i];
     - complete = 1 exactly when nu = 0; then the roots of f in Z_p are exactly the n roots of the
       balls (P3.5(3)); the list may be empty.
   Under S-D13 every root of g is simple (L3.1(2)), so a large enough depth completes the list
   (P3.5(6)); the depth v_p(R) + 1 of P3.6 suffices when an identity u g + v g' = R with u, v in Z[X]
   and R != 0 is known.

   The roots modulo p are found by evaluation of the current polynomial of Y and its derivative
   modulo p at every residue 0, ..., p - 1 (solvers P3.7(1)); no routine of FLINT for roots or
   factorisation modulo p is called.

   Statuses, in the order in which they are decided:
     ADF_DOMAIN (edit E-C1): f = 0, p is the real place, prec_p < 1 or depth < 0; L untouched.
     ADF_UNSUPPORTED (edit E-C1; S-D10), TEMPORARY: p > ADF_ROOTS_P_EVAL_MAX; L untouched. This
       status goes away with the slice that adds the route of solvers P3.7(2).
     ADF_LIMIT: 2 prec_p bits(p) > ADF_ROOTS_BITS_MAX, decided before any allocation; L untouched.
     ADF_LIMIT: during the search, an overflow of slong in an exponent, a root with
       2 K bits(p) > ADF_ROOTS_BITS_MAX for K = max(prec_p, s + 1), or a class of exponent e with
       2 e bits(p) > ADF_ROOTS_BITS_MAX; decided before that power of p is formed; L untouched (no
       partial list is returned on LIMIT).
     ADF_OK: L written as above, complete 0 or 1.
   Aliasing: L is an output of its own type; f is an input and may be L->g: L is written only after
   the result has been computed. Allocates: g*, the polynomials of the open classes, powers of p up to
   p^(2 K) for each root, and the arrays of L. Cost (P3.5(7)): one gcd of f and f'; for each class
   opened one Taylor shift and p evaluations of a polynomial and its derivative modulo p; for each root
   O(log(K)) Newton steps modulo p^(2 K) at most. */
int adf_roots_padic_partial(adf_rootlist_t L, const fmpz_poly_t f, adf_place_t p, slong prec_p, slong depth);

/* adf_roots_padic(L, f, p, prec_p, depth): the list of all roots of f in Z_p, as the partial function
   above when its list is complete (nu = 0, complete = 1; the list may be empty: no root in Z_p is an
   answer, not NO_SOLUTION). Statuses: ADF_DOMAIN, ADF_UNSUPPORTED, ADF_LIMIT as the partial function;
   ADF_NOT_DETERMINED: a class is unresolved at this depth (a larger depth may resolve it; api-s.md
   4); L untouched (CV-06). ADF_OK: L written, complete = 1, nu = 0. Aliasing, allocation and cost as
   the partial function. */
int adf_roots_padic(adf_rootlist_t L, const fmpz_poly_t f, adf_place_t p, slong prec_p, slong depth);

/* ---- the entries verifier ---- */

/* adf_rootlist_verify_entries(L, f): at a prime, 1 if f is not 0, g is the normalised polynomial of f
   and reduced is the flag of f, the scope, n, nu, complete and the pointers are consistent (SEED:
   n = 1, nu = 0, complete = 0; PARTITION: complete = 1 exactly when nu = 0), every listed certificate
   (a, K, s) satisfies (R1),
   (R2), (R3) of solvers D3.2 for g, every class satisfies 0 <= ua < p^ue, and the listed balls and
   classes are pairwise disjoint (P3.2(4)); else 0. Then every listed ball holds exactly one root of
   g, so of f, and different balls hold different roots (solvers P3.13(1)). It does NOT verify a flag
   complete, a count n or a coverage (P3.13(1), api-s.md 4), and a SEED list says nothing about other
   roots. A certificate with 2 K bits(p) > ADF_ROOTS_BITS_MAX is not tested and gives 0; no list of
   the library has one.
   At the real place it returns 0 in this slice: this is TEMPORARY, until the slice of adf_roots_real
   brings the exact test of solvers P3.8 (P3.13(4)).
   A predicate; never aborts on a list that satisfies the pointer rule of is_canonical. Cost: one gcd
   of f and f'; for each certificate two evaluations of g modulo p^(K+s) and p^(s+1); (n + nu)^2
   valuations. */
int adf_rootlist_verify_entries(const adf_rootlist_t L, const fmpz_poly_t f);

/* adf_rootlist_verify_complete(L, f, depth) (decision S-D17; solvers P3.13(2), (3)): at a prime, 1 only
   if adf_rootlist_verify_entries(L, f) = 1, the scope is PARTITION, complete = 1 and nu = 0, and
   Algorithm P, run here again on g with the precision 1 through depth, leaves no unresolved class,
   and every root ball of the rerun meets exactly one listed ball and every listed ball meets exactly
   one root ball of the rerun (P3.2(4)); else 0. Then the roots of f in Z_p are exactly the roots of
   the listed balls, one in each (P3.13(2)). Nothing of the list is trusted beyond what is checked: n
   and complete are compared with the rerun. It returns 0 when depth < 0, when the depth does not
   suffice for the rerun (no claim about L then), when the rerun would return ADF_UNSUPPORTED
   (p > ADF_ROOTS_P_EVAL_MAX, TEMPORARY) or ADF_LIMIT, and for every SEED list (P3.13(3)).
   At the real place it returns 0 in this slice: this is TEMPORARY, until the slice of adf_roots_real
   brings the recount of solvers P3.13(5).
   A predicate; never aborts on a list that satisfies the pointer rule of is_canonical. Cost: that of
   the entries verifier and of the search itself (api-s.md 4, note 3), and (n + m)^2 comparisons of
   balls for m roots found. */
int adf_rootlist_verify_complete(const adf_rootlist_t L, const fmpz_poly_t f, slong depth);

/* ---- accessors ---- */

/* The fields: n, complete, nu, place, scope. Constant cost. */
slong adf_rootlist_length(const adf_rootlist_t L);
int adf_rootlist_is_complete(const adf_rootlist_t L);
slong adf_rootlist_unresolved_length(const adf_rootlist_t L);
adf_place_t adf_rootlist_place(const adf_rootlist_t L);
int adf_rootlist_scope(const adf_rootlist_t L);

/* adf_rootlist_get_poly(g, L): g = a copy of the normalised polynomial of L. g may be L->g itself.
   Cost: O(deg g) coefficient copies. */
void adf_rootlist_get_poly(fmpz_poly_t g, const adf_rootlist_t L);

/* adf_rootlist_get_cert(a, K, s, L, i): the certificate (a, K, s) of root i at a prime. Returns 1 if
   written; 0 if L is at the real place or i is outside [0, n), and then a, K, s are untouched. a may
   be L->a + i. A predicate, not a status. Cost: a copy. */
int adf_rootlist_get_cert(fmpz_t a, slong * K, slong * s, const adf_rootlist_t L, slong i);

/* adf_rootlist_get_unresolved(a, e, L, i): the unresolved class a + p^e Z_p of index i at a prime
   (solvers P3.5). Returns 1 if written; 0 if L is at the real place or i is outside [0, nu), and then
   a and e are untouched. a may be L->ua + i. A predicate, not a status. Cost: a copy. */
int adf_rootlist_get_unresolved(fmpz_t a, slong * e, const adf_rootlist_t L, slong i);

/* adf_rootlist_get_fball(x, L, i) (decision S-D14): at a prime, x = the finite ball a_i + p^(K_i) Zhat,
   the set of the x in Zhat whose component at p lies in the ball a_i + p^(K_i) Z_p of root i. It is
   a CYLINDER: it says nothing about the components at the other primes beyond integrality, and it
   is not a set of roots of f in Zhat. Written as the canonical global triple (a_i, p^(K_i), 1)
   (SPEC 4.1; conventions 5.2). Returns 1 if written; 0 if L is at the real place, i is outside
   [0, n), or K_i bits(p) > ADF_ROOTS_BITS_MAX (no list of the library has that); x untouched on 0.
   Cost: one power of p. */
int adf_rootlist_get_fball(adf_fball_t x, const adf_rootlist_t L, slong i);

/* Layout queries (conventions 12.4, CV-40). Header-inline and exported. */
ADF_INLINE size_t adf_sizeof_rootlist(void) { return sizeof(adf_rootlist_struct); }
ADF_INLINE size_t adf_alignof_rootlist(void) { return ADF_ALIGNOF(adf_rootlist_struct); }

#ifdef __cplusplus
}
#endif

#endif /* ADELEFELD_ROOTS_H */
