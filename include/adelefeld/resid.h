/* adelefeld/resid.h: a residue modulo m, and partial rational reconstruction from it (slice 1).

   Contract: docs/api-s.md sections 1 and 2 (decisions S-D1, S-D2, S-D5, accepted by TJO on
   2026-09-29); docs/proofs/solvers.md Definition 1.1 (line 76), Definition 1.3 (line 121),
   Lemma 1.4 (line 133), Proposition 1.6 (line 210); docs/conventions.md 2.3, 3.2, 4.1, 4.3.
   Implemented in src/resid.c; tests tests/test_resid.c; reference proto/solvers_checks.py,
   function recon_partial.

   The type adf_resid holds the pair (c, m), meaning the set P(m, c) of the rationals in
   c + m Z_p for every prime p dividing m; nothing is known at the other places. It is not a
   ball of adf_fball and no function of this header takes one. Predicate (is_canonical): m >= 1
   and 0 <= c < m. Init value: (0, 1), every rational.

   The problem (Definition 1.1). Given m >= 1, c, A, B, a solution is a pair of integers (n, d)
   with d > 0, gcd(n, d) = 1, gcd(d, m) = 1, n = c d modulo m, |n| <= A, d <= B. The sign is on
   n, the denominator is positive.

   This slice decides the problem only in the range 2 A B < m, where Proposition 1.6 gives the
   complete answer from the certificate pair of the Euclidean algorithm. The range m <= 2 A B is
   not implemented yet: adf_resid_reconstruct returns ADF_UNSUPPORTED there and writes nothing.
   THIS STATUS IS TEMPORARY: it goes away with slice 2 (issue adf-1y1 and its successor), which
   answers that range with OK, NO_SOLUTION, NOT_UNIQUE or NOT_DETERMINED. */

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

/* adf_resid_reconstruct(q, cert, x, A, B, limit): decides Sol(m, c, A, B) with (c, m) = x, in
   the range 2 A B < m (slice 1). The statuses, in the order in which they are decided:

   1. A < 0 or B < 1: ADF_NO_SOLUTION (decision S-D5: the box is empty). cert gets kind = 0.
   2. 2 A B >= m: ADF_UNSUPPORTED (TEMPORARY, see the top of this file); q and cert are
      untouched. 2 A B is computed as an fmpz, never in a word. Note that this includes every
      A >= m, because B >= 1.
   3. Otherwise 0 <= A < m. The certificate pair of Lemma 1.4 is computed by the Euclidean
      algorithm on (m, c mod m), stopping at the first remainder R <= A; cert (if not NULL)
      gets the pair with kind = 1, on every status of this case. Then, by Proposition 1.6:
        |T| > B: ADF_NO_SOLUTION (1.6 (b));
        else gcd(R, T) = 1: ADF_OK and q = sigma R/|T|, sigma the sign of T (1.6 (c));
        else ADF_NO_SOLUTION (1.6 (c), Remark 1: the row is not divided by the gcd, the
        reduced point would not lie in the lattice).
   The answer OK is the unique element of Sol, a reduced fraction with 0 < d <= B, |n| <= A,
   gcd(d, m) = 1 (Lemma 1.2).

   limit is not used in this slice (it bounds the search of the range m <= 2 A B, slice 2); it
   is part of the final signature (docs/api-s.md section 2).
   q is untouched on every status other than ADF_OK (conventions 4.3). cert may be NULL.
   Aliasing: q and cert are outputs of different types; x, A and B may be the same object as
   each other or members of x; no output may alias an input (q is an adf_rat, cert an
   adf_recon_cert). Cost: one Euclidean algorithm on (m, c mod m), with at most one division of
   multiprecision numbers per step; one gcd. */
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
