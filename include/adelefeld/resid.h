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

/* Layout queries (conventions 12.4, CV-40). Header-inline and exported. */
ADF_INLINE size_t adf_sizeof_resid(void) { return sizeof(adf_resid_struct); }
ADF_INLINE size_t adf_alignof_resid(void) { return ADF_ALIGNOF(adf_resid_struct); }
ADF_INLINE size_t adf_sizeof_recon_cert(void) { return sizeof(adf_recon_cert_struct); }
ADF_INLINE size_t adf_alignof_recon_cert(void) { return ADF_ALIGNOF(adf_recon_cert_struct); }

#ifdef __cplusplus
}
#endif

#endif /* ADELEFELD_RESID_H */
