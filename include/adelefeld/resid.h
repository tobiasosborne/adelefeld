/* adelefeld/resid.h: a residue modulo m, and partial rational reconstruction from it (slices 1
   and 2 of milestone S).

   Contract: docs/api-s.md sections 1 and 2 (decisions S-D1 to S-D5, accepted by TJO on
   2026-09-29); docs/proofs/solvers.md Definition 1.1 (line 75), Definition 1.3 (line 117),
   Lemma 1.4 (line 133), Propositions 1.5 to 1.7 (lines 162 to 325); docs/conventions.md 2.3, 3.2,
   4.1, 4.3. Implemented in src/resid.c; tests tests/test_resid.c and tests/test_resid_full.c;
   reference proto/solvers_checks.py, function recon_partial.

   The type adf_resid holds the pair (c, m), meaning the set P(m, c) of the rationals in
   c + m Z_p for every prime p dividing m; nothing is known at the other places. It is not a
   ball of adf_fball and no function of this header takes one. Predicate (is_canonical): m >= 1
   and 0 <= c < m. Init value: (0, 1), every rational.

   The problem (Definition 1.1). Given m >= 1, c, A, B, a solution is a pair of integers (n, d)
   with d > 0, gcd(n, d) = 1, gcd(d, m) = 1, n = c d modulo m, |n| <= A, d <= B. The sign is on
   n, the denominator is positive. Sol(m, c, A, B) is the set of solutions. */

#ifndef ADELEFELD_RESID_H
#define ADELEFELD_RESID_H

#include "adelefeld/common.h"
#include "adelefeld/status.h"
#include "adelefeld/fball.h"
#include "adelefeld/rat.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Layout (docs/api-s.md section 1, 64 bit): 16 bytes, alignment 8; c at offset 0, m at offset 8. */
typedef struct
{
    fmpz_t c;
    fmpz_t m;
} adf_resid_struct;

typedef adf_resid_struct adf_resid_t[1];
typedef adf_resid_struct * adf_resid_ptr;
typedef const adf_resid_struct * adf_resid_srcptr;

/* The certificate pair (R', T', R, T) of Definition 1.3, or none. Layout: 40 bytes, alignment 8;
   Rp at 0, Tp at 8, R at 16, T at 24, kind at 32 (then 4 bytes of padding).
   Predicate: kind is 0 or 1; if kind = 0 all four integers are 0.
   kind = 1: (C1) R = c T and R' = c T' modulo m; (C2) |T R' - T' R| = m; (C3) 0 <= R <= A < R';
   (C4) T is not 0 and T T' <= 0. Init value: kind = 0, all four 0. */
typedef struct
{
    fmpz_t Rp;
    fmpz_t Tp;
    fmpz_t R;
    fmpz_t T;
    int kind;
} adf_recon_cert_struct;

typedef adf_recon_cert_struct adf_recon_cert_t[1];
typedef adf_recon_cert_struct * adf_recon_cert_ptr;
typedef const adf_recon_cert_struct * adf_recon_cert_srcptr;

/* ---- life cycle (conventions 2.3) ---- */

/* adf_resid_init(x): x = (0, 1). Never fails. */
void adf_resid_init(adf_resid_t x);

/* adf_resid_clear(x): releases the memory; afterwards x may only be passed to init. */
void adf_resid_clear(adf_resid_t x);

/* adf_resid_set_fmpz2(x, c, m): x = P(m, c), with c reduced into [0, m) (Definition 1.1: the
   set depends on c modulo m only). Status: ADF_OK; ADF_DOMAIN if m < 1 (invalid raw data,
   conventions 4.4), x untouched. Aliasing: c and m may be the members of x itself. Cost: one
   division. */
int adf_resid_set_fmpz2(adf_resid_t x, const fmpz_t c, const fmpz_t m);

/* adf_resid_set(y, x): y becomes a copy of x (conventions 2.3). y may be x. Cost: two copies. */
void adf_resid_set(adf_resid_t y, const adf_resid_t x);

/* adf_resid_swap(x, y): exchanges the contents of x and y; O(1), no allocation (conventions 2.3).
   x and y must be different objects. */
void adf_resid_swap(adf_resid_t x, adf_resid_t y);

/* adf_resid_identical(x, y): 1 if c and m are equal in x and y, else 0 (conventions 2.3:
   representation identity; the two fields are the whole value, so this is also equality of the
   sets P(m, c) of Proposition 1.10, which depend on c only through c modulo m and on m). Cost:
   two comparisons. */
int adf_resid_identical(const adf_resid_t x, const adf_resid_t y);

/* adf_resid_get_fmpz2(c, m, x): the pair of x. c and m must be different objects; each may
   be a member of x. Cost: two copies. */
void adf_resid_get_fmpz2(fmpz_t c, fmpz_t m, const adf_resid_t x);

/* adf_resid_is_canonical(x): 1 if m >= 1 and 0 <= c < m, else 0; never aborts for an
   initialised object. */
int adf_resid_is_canonical(const adf_resid_t x);

/* adf_recon_cert_init(cert): kind = 0, all four integers 0. adf_recon_cert_clear(cert): releases
   the memory; afterwards cert may only be passed to init. */
void adf_recon_cert_init(adf_recon_cert_t cert);
void adf_recon_cert_clear(adf_recon_cert_t cert);

/* adf_recon_cert_is_canonical(cert): 1 if kind is 0 or 1 and, for kind = 0, all four integers are 0,
   else 0 (the storage invariant of docs/api-s.md section 1; conventions 2.3). For kind = 1 nothing
   else is required: the pair need not satisfy (C1) to (C4) to be a canonical value, it is canonical
   data whose soundness is adf_recon_cert_check. It never aborts for an initialised object. Cost: a
   comparison of kind and, for kind = 0, four tests for zero. */
int adf_recon_cert_is_canonical(const adf_recon_cert_t cert);

/* adf_recon_cert_set(y, x): y becomes a copy of x, kind included (conventions 2.3). y may be x. */
void adf_recon_cert_set(adf_recon_cert_t y, const adf_recon_cert_t x);

/* adf_recon_cert_swap(x, y): exchanges the contents, kind included; O(1), no allocation. x and y must
   be different objects. */
void adf_recon_cert_swap(adf_recon_cert_t x, adf_recon_cert_t y);

/* adf_recon_cert_identical(x, y): 1 if kind and all four integers are equal, else 0 (conventions 2.3:
   representation identity). Two different quadruples may both satisfy (C1) to (C4) for the same
   (m, c, A) and then be different values; identical says nothing about soundness. Cost: five
   comparisons. */
int adf_recon_cert_identical(const adf_recon_cert_t x, const adf_recon_cert_t y);

/* ---- constructors from a rational and from a ball, and membership ---- */

/* adf_resid_set_rat(x, q, m): the P(m, c) of Proposition 1.10 (1) that contains the rational q = n/d:
   the c in [0, m) with c d = n modulo m. It exists exactly when gcd(d, m) = 1, because q is reduced
   (Lemma 1.2 (1) and (3): a reduced q = n/d with n = c d modulo m forces gcd(d, m) = 1; conversely
   then d is invertible modulo m and c = n d^(-1) is unique). For m = 1 the result is P(1, 0) = Q.
   Status: ADF_OK, x written; ADF_DOMAIN if m < 1 or gcd(d, m) > 1 (then q is in no P(m, c), L1.2 (1)),
   x untouched. Aliasing: q is an adf_rat and m an fmpz, neither may alias x. q must be canonical
   (conventions 5.1), which says d > 0 and gcd(n, d) = 1.
   Cost: one gcd and one modular inverse of the sizes of n, d, m. */
int adf_resid_set_rat(adf_resid_t x, const adf_rat_t q, const fmpz_t m);

/* adf_resid_contains_rat(x, q): 1 if the rational q = n/d is in the set P(m, c) of x, that is
   gcd(d, m) = 1 and n = c d modulo m (P1.10 (1) with Lemma 1.2), else 0. It is a predicate and never
   aborts for initialised x and q. q must be canonical. For m = 1 every rational is in P(1, 0).
   Cost: one gcd and one divisibility test. */
int adf_resid_contains_rat(const adf_resid_t x, const adf_rat_t q);

/* adf_resid_set_fball_forget(x, b): the passage from the finite ball to the residue, and the only
   one (decision S-D1, the "type" paragraph of P1.10). For b = (A + H Zhat)/d given by its canonical
   global triple (A, H, d) with H >= 1 and gcd(d, H) = 1, the result is x = P(H, c) with c the unique
   integer in [0, H) with c d = A modulo H. Every rational of b is in that P(H, c) (P1.10 (4)) and the
   inclusion is proper for H >= 2; nothing is known at the primes that do not divide H.
   A local b is recombined first: (A, H, d) = adf_fball_get_fmpz3 (fball.h:161), so the result of a
   local b is the residue of the same set, computed through the same triple.
   Status: ADF_OK, x written and canonical; ADF_DOMAIN if the ball is exact (H = 0: a single rational
   is in no P(m, c) with m > 1 and the radius is the only thing that says which m) or if gcd(d, H) > 1
   (then the rationals of b have a denominator prime to no residue modulus, P1.10 (4) needs the
   inverse of d modulo H), x untouched. Aliasing: b is an adf_fball and may not alias x.
   Cost: one gcd and one modular inverse of the sizes of A, H, d (a CRT recombination for a local b). */
int adf_resid_set_fball_forget(adf_resid_t x, const adf_fball_t b);

/* ---- reconstruction ---- */

/* adf_recon_cert_check(cert, x, A): 1 if cert has kind = 1 and satisfies (C1) to (C4) of
   Definition 1.3 for (m, c) = x and A, else 0. It tests the four conditions with four
   multiplications, two reductions and comparisons (docs/proofs/solvers.md:117), and needs no
   other information: it never aborts for an initialised cert, x and A. Kind 0 gives 0. It does
   not test that (C1) to (C4) make the pair the one that the Euclidean algorithm computes (they
   need not: Proposition 1.5 holds for every quadruple that satisfies them). */
int adf_recon_cert_check(const adf_recon_cert_t cert, const adf_resid_t x, const fmpz_t A);

/* adf_resid_reconstruct(q, cert, x, A, B, limit): decides Sol(m, c, A, B) with (c, m) = x.
   Let ell = max(limit, 0), (R', T', R, T) the certificate pair of Lemma 1.4 (the Euclidean
   algorithm on (m, c mod m), stopping at the first remainder R <= A) and X = floor(B / abs(T)).
   The algorithm is Algorithm R of docs/proofs/solvers.md, lines 256 to 324 (Proposition 1.7).
   The statuses, in the order in which they are decided:

   1. A < 0 or B < 1: ADF_NO_SOLUTION (decision S-D5: the box is empty).
   2. A >= m: ADF_NOT_UNIQUE (Proposition 1.6 (a): c mod m / 1 and (c mod m - m) / 1 are two
      solutions).
   3. Otherwise 0 <= A < m. abs(T) > B: ADF_NO_SOLUTION (1.6 (b)).
   4. 2 A B < m: gcd(R, T) = 1 gives ADF_OK with q = sigma R / abs(T), sigma the sign of T;
      gcd(R, T) > 1 gives ADF_NO_SOLUTION (1.6 (c), Remark 1: the row is not divided by the
      gcd, the reduced point would not lie in the lattice). 2 A B is an fmpz, never a word.
   5. Otherwise (A < m <= 2 A B, abs(T) <= B) the search of Algorithm R step 7: the rounds
      x = 1, ..., min(X, ell), each with the integers y in
      [max(0, ceil((x R - A) / R')), floor((x R + A) / R')] found by division (1.5 (3)), while
      d = x abs(T) + y abs(T') <= B; a point (n, d), n = sigma (x R - y R'), is counted if
      gcd(n, d) = 1. As soon as two reduced points are found: ADF_NOT_UNIQUE (also when the
      search would have been cut). Else, if X > ell: ADF_NOT_DETERMINED ("uniqueness not
      certified": the four conditions of Proposition 1.7 (3); a cut search with fewer than two
      reduced points found). Else ADF_OK with the one point found, or ADF_NO_SOLUTION if none.
   OK is the unique element of Sol, a reduced fraction with 0 < d <= B, |n| <= A, gcd(d, m) = 1
   (Lemma 1.2). NOT_UNIQUE, NO_SOLUTION are proved. With limit <= 0 the status is not "2 A B >= m
   gives NOT_DETERMINED": A >= m gives NOT_UNIQUE and abs(T) > B gives NO_SOLUTION (api-s.md
   note 5; m = 2, c = 1, A = 2, B = 1 gives NOT_UNIQUE).

   q is untouched on every status other than ADF_OK (conventions 4.3). cert (may be NULL) is
   written on every status: the pair with kind = 1 if 0 <= A < m and B >= 1, else kind = 0 and
   the four integers 0. The number of rounds is at most min(ell, X) (1.7 (4)); X is an fmpz and
   is compared with ell as such, so B / abs(T) above a word is fine, and a call with a small
   limit costs one Euclidean algorithm and at most ell rounds however large B is. A call with
   a large limit (up to WORD_MAX) runs until two reduced points are found or X rounds are done:
   that may be as long as B.
   Aliasing: x, A and B may be the same object as each other or members of x; no output may
   alias an input (q is an adf_rat, cert an adf_recon_cert). Cost: one Euclidean algorithm on
   (m, c mod m), with at most one division of multiprecision numbers per step; then at most
   min(ell, X) rounds, each with a constant number of multiplications, divisions and at most
   two gcds of integers bounded by max(A B, m). */
int adf_resid_reconstruct(adf_rat_t q, adf_recon_cert_t cert, const adf_resid_t x, const fmpz_t A,
                          const fmpz_t B, slong limit);

/* adf_resid_reconstruct_first(q, count, cert, x, A, B, limit) (decision S-D4): the same search as
   adf_resid_reconstruct, and on every status where that search has a solution to give, q is the
   FIRST solution in the order of Algorithm R: the points phi(x, y) of Proposition 1.5 (1) with x
   increasing and, for each x, y increasing. For A >= m, which leaves the search at once, the first
   solution is c/1 with c = c mod m (the first pair of Proposition 1.6 (a)).
   The statuses, and what they say:
   1. ADF_OK: q is written and is a solution. *count is 1 if the search was complete and found
      exactly that one solution, 2 if a second solution was found (also when A >= m, where two
      solutions are proved without a search), and 0 if the search was cut by the limit after one
      solution was found, so that the answer is a solution but not the only one.
   2. ADF_NO_SOLUTION: the set is empty, q is untouched, *count = 0.
   3. ADF_NOT_DETERMINED: the search was cut before any solution was found, so nothing is said about
      existence; q is untouched, *count = 0.
   4. ADF_DOMAIN is never returned: an adf_resid has m >= 1, and an empty box (A < 0 or B < 1) is
      ADF_NO_SOLUTION, as for adf_resid_reconstruct (decision S-D5).
   *count and cert are written on every status; cert is written exactly as adf_resid_reconstruct
   writes it. q is untouched on every status other than ADF_OK (conventions 4.3). *count is a report
   argument (conventions 4.3) and must not alias q, cert or any input.
   Aliasing: as adf_resid_reconstruct, and *count is a different type from every other argument.
   Cost: as adf_resid_reconstruct: the same one Euclidean algorithm and at most min(ell, X) rounds.
   A caller who wants only the count of the solutions in the range 2 A B < m should use
   adf_resid_reconstruct, which decides without a search. */
int adf_resid_reconstruct_first(adf_rat_t q, int * count, adf_recon_cert_t cert, const adf_resid_t x,
                                const fmpz_t A, const fmpz_t B, slong limit);

/* adf_resid_verify_result(x, A, B, limit, status, q, cert) (solvers P1.11, solvers.md:462 to 494; the
   order of the arguments is the one of docs/api-s.md section 2, which puts the two integer parameters
   before the two values): 1 if the claim "adf_resid_reconstruct returned `status` for (x, A, B, limit),
   and on ADF_OK the solution q" is TRUE, else 0. It is a predicate: 1 only if the claim is true, and
   0 also where the claim may be true but this function cannot decide it. It never aborts for
   initialised x, q and cert.
   The tests, in the order of P1.11:
   1. m < 1 (x is not a residue; the type excludes it, so no call of the library returns DOMAIN here):
      the claim is ADF_DOMAIN. A < 0 or B < 1: the claim is ADF_NO_SOLUTION, the only one, since the box
      is empty (decision S-D5).
   2. ADF_NOT_UNIQUE: 1 if the set has at least two elements. A >= m proves it without any search
      (Proposition 1.6 (a)). For A < m the complete enumeration of Proposition 1.5 (4) is run, with the
      pair computed here as the Euclidean algorithm of Lemma 1.4 computes it; it stops at the second
      solution, so the cost is that of the search itself. The certificate is not read: the claim of
      Proposition 1.11 (1) is about the set, and a second call with B lowered is NOT a certificate
      (P1.11 (3): m = 2, c = 1, A = B = 1, where both solutions have denominator 1 and B = 0 empties the
      box). The enumeration is not run when |T| > B or 2 A B < m (then at most one solution exists), and
      the claim is not decided when floor(B/|T|) is above a word, since the rounds could not all be
      visited.
   3. Every other status needs a certificate pair: adf_recon_cert_check first, which also refuses
      A >= m (the pair needs A < m) and a kind = 0 certificate. Then, with (R', T', R, T) of the
      certificate, T nonzero and ell = max(limit, 0):
      ADF_NO_SOLUTION: 1 if |T| > B (1.6 (b)), or if 2 A B < m and gcd(R, T) > 1 (1.6 (c)), or if the
        complete enumeration finds no reduced point (1.6 (d));
      ADF_OK: 1 if q = n/d is a solution of Definition 1.1 (d > 0, d <= B, |n| <= A, gcd(n, d) = 1,
        gcd(d, m) = 1, n = c d modulo m) and either 2 A B < m and q = (sigma R, |T|) (1.6 (c)), or the
        complete enumeration finds exactly the reduced point q (1.6 (d));
      ADF_NOT_DETERMINED: 1 if the four conditions of Proposition 1.7 (3) hold, that is A < m <= 2 A B,
        |T| <= B, floor(B/|T|) > ell, and the cut search of the rounds x <= ell finds fewer than two
        reduced points; that search is run, so the fourth condition is not assumed.
   4. Any other status (ADF_DOMAIN, ADF_LIMIT, a code outside the class) gives 0.
   q is read only for ADF_OK, since the other statuses leave it untouched (conventions 4.3), so a claim
   of NO_SOLUTION or NOT_DETERMINED says nothing about q. On NOT_UNIQUE the claim of P1.11 (1) is a list
   of two pairs; this interface carries one rational, so the list is replaced by the complete
   enumeration, the second way named in note 4 of docs/api-s.md section 2.
   Cost: the four multiplications of the certificate check, and, for the three cases that need it, one
   Euclidean algorithm and at most floor(B/|T|) rounds. */
int adf_resid_verify_result(const adf_resid_t x, const fmpz_t A, const fmpz_t B, slong limit, int status,
                            const adf_rat_t q, const adf_recon_cert_t cert);

/* Layout queries (conventions 12.4, CV-40). Header-inline and exported. */
ADF_INLINE size_t adf_sizeof_resid(void) { return sizeof(adf_resid_struct); }
ADF_INLINE size_t adf_alignof_resid(void) { return ADF_ALIGNOF(adf_resid_struct); }
ADF_INLINE size_t adf_sizeof_recon_cert(void) { return sizeof(adf_recon_cert_struct); }
ADF_INLINE size_t adf_alignof_recon_cert(void) { return ADF_ALIGNOF(adf_recon_cert_struct); }

#ifdef __cplusplus
}
#endif

#endif /* ADELEFELD_RESID_H */
