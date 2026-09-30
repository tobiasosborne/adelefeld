/* adelefeld/idpow.h: integer powers of unit cosets, ideles and idele classes (milestone 2, slice 3, lane
   i-slice3; docs/PLAN.md 2.1, "power").

   Contract: docs/SPEC.md 5 ("Integer powers of a unit coset": the default enclosure c^k U(N), the smallest coset
   chat^k U(M_k) as a separately named operation, the exponent 0 gives the exact unit 1; "Sign preservation": the
   rule applies to integer powers); docs/conventions.md 5.6 ("Power", decision CV-49 = M0-D6), 5.7 (the result
   sign check G5), 3.2 (row "Unit coset, idele, idele class arithmetic"), 3.3, 4.1, 4.3, 4.4; decisions M0-D1,
   M0-D6, M1-D4, N-D6. Proofs: docs/proofs/ideles.md Lemma 12 (line 242), Proposition 13 (line 261: the table of
   M_k; P13.2 the centre, line 279; P13.3 minimality, line 282; P13.4 the default rule, line 284; P13.6 the
   exponent 0, line 288); docs/api-2.md 3.3, Statements J (the two powers of a unit coset as computed), K (the
   real kernel of a power) and L (the power of an idele value and of a class value). Implemented in
   src/idpow.c; tests tests/test_idpow.c.

   The exponent k is an slong; every value from WORD_MIN to WORD_MAX is admitted (|k| is formed as a ulong).
   For a unit coset x the set of powers is P_k = {u^k : u in x}, one unknown u raised to the power k (P13); for
   an idele value or a class value the power is taken of every point (api-2.md L).

   Common rules for every function below, unless its comment says otherwise:
   - Aliasing (conventions 4.1): the output may be the same object as the input; an output never aliases a part
     of an input (&x->u, x->inf, x->t).
   - Inputs satisfy their predicates (adelefeld/ucoset.h, idele.h, idclass.h); otherwise undefined
     (conventions 4.4, CV-09). With -DADF_CHECK_INVARIANTS every function checks its input on entry and aborts.
   - Results are stored in normal form (D2-1, N-D6): the unit coset of every result is normal.
   - A function that returns a status leaves its output untouched on a status other than ADF_OK (conventions
     4.3). prec is the real working precision; a prec below 2 is taken as 2 (M1-D4); a prec above
     ADF_IDELE_PREC_MAX gives ADF_LIMIT, decided from prec alone before any allocation and before every other
     status (the rule of adelefeld/idele.h). The unit and the content do not depend on prec. */

#ifndef ADELEFELD_IDPOW_H
#define ADELEFELD_IDPOW_H

#include <flint/arb.h>

#include "adelefeld/common.h"
#include "adelefeld/status.h"
#include "adelefeld/ucoset.h"
#include "adelefeld/idele.h"
#include "adelefeld/idclass.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The largest size, in bits, of the content r^k of a power of an idele: ADF_LIMIT when r != 1 and
   |k| (bits(n) + bits(d)) > ADF_IDELE_POW_BITS_MAX for r = n/d, decided before r^k is formed. bits(n^|k|) is at
   most |k| bits(n) (api-2.md L.4). 2^26 bits, 8 MiB for each of the two integers (decision i3-3 of api-2.md
   3.5). */
#define ADF_IDELE_POW_BITS_MAX 67108864

/* ---- unit cosets (void: nothing can fail) ---- */

/* adf_ucoset_pow(y, x, k): y = the default enclosure of P_k (SPEC 5; conventions 5.6; M0-D6):
   - x = c U(N), N >= 1, normal form (c', N') (conventions 5.6): y = (c'^k mod N') U(N'), c'^k the power of the
     inverse of c' modulo N' for k < 0 (1 for N' = 1); it contains P_k (ideles.md P13.4, line 284) and is the
     smallest coset exactly when M_k = N' (always for k = 1 and k = -1);
   - x the exact unit [e], e = +1 or -1: y = [e^k], exact (P_k = {e^k});
   - k = 0: y = [1], the exact unit 1, for every x (P_0 = {1}; P13.6: no coset with N >= 1 equals {1}; M0-D1).
   y is in normal form (api-2.md J.1). y may be x. Cost: a modular inverse (k < 0) and O(log |k|) products
   modulo N'. */
void adf_ucoset_pow(adf_ucoset_t y, const adf_ucoset_t x, slong k);

/* adf_ucoset_pow_tight(y, x, k): y = chat^k U(M_k), the smallest coset that contains P_k (ideles.md P13.2 and
   P13.3, lines 279 and 282; SPEC 5, M0-D6: a separately named operation). For x = c U(N) with normal form
   (c', N') and k != 0 (api-2.md Statement J.2):
       M_k = A B,  A = N' s,  s = the product of p^(v_p(k)) over the primes p dividing N',
       B = 2^(2 + v_2(k)) if N' is odd and k is even (else 1), times p^(1 + v_p(k)) for every odd prime p that
           does not divide N' and for which p - 1 divides k,
   and the residue of y is the one that is c'^k modulo A and 1 modulo B (chat^k of P13.2 for the choice chat = c'
   modulo A, chat = 1 modulo B). M_k is normal (v_2(M_k) != 1, P13.1), so y is in normal form. For k = 1 and
   k = -1, M_k = N' and y equals adf_ucoset_pow(x, k) (P13.4): [5 mod 6]^1 is [2 mod 3], not a coset modulo 6
   (lane i-slice1, finding 1, against docs/PLAN.md 2.1 "M_k = N"). Examples: [1 mod 1]^2 = [1 mod 24];
   [2 mod 5]^2 = [49 mod 120] (adf_ucoset_pow gives [4 mod 5]). Exact units and k = 0: as adf_ucoset_pow.
   Only |k| is factored (n_factor, refs/src/flint-3.0.1/ulong_extras.rst:1203), N' is not; the primes p with
   p - 1 dividing k are found among d + 1, d a divisor of |k|, by n_is_prime (certified below 2^64,
   ulong_extras.rst:833-840). y may be x.
   Cost: the factorisation of one word, tau(|k|) primality tests of a word (tau the number of divisors) and
   powers modulo M_k, which has at most bits(N') + bits(|k|) + 2 + 64 (tau(|k|) - 1) bits (api-2.md J.4). */
void adf_ucoset_pow_tight(adf_ucoset_t y, const adf_ucoset_t x, slong k);

/* ---- ideles ---- */

/* adf_idele_pow(z, x, k, prec): z = (Z, r^k, adf_ucoset_pow(u, k)) for x = (X, r, u): the k-th power of every
   point of x lies in z (api-2.md L.1). The content r^k is exact (fmpq_pow_si, fmpq.rst:480); the unit is the
   default enclosure of adf_ucoset_pow; Z is kernel B (adelefeld/idele.h, api-2.md E5) on the end points of
   Statement K: lo <= |xi|^|k| <= hi by binary powering of the ends l, h of |X| (E1) with rounding down,
   respectively up, at p = max(prec, 2) bits after every product, then for k < 0 the ends RD_p(1/hi),
   RU_p(1/lo); the sign is sign(X) for odd k and +1 for even k.
   k = 0: z is the exact idele 1 (inf = 1 exact, r = 1, u = [1]) for every x: xi^0 = 1, r^0 = 1, w^0 = 1.
   If X is exact and |m|^|k| (m the midpoint) has at most p bits, Z is exact (K.4).
   Status: ADF_OK, z written; ADF_NOT_DETERMINED (B1: e(hi) - e(lo) > p) if the real part cannot be certified
   free of 0 at p bits, z untouched; ADF_LIMIT, z untouched, if prec > ADF_IDELE_PREC_MAX, or if r != 1 and
   |k| (bits(n) + bits(d)) > ADF_IDELE_POW_BITS_MAX for r = n/d (both decided before any allocation, before
   r^k is formed and before every other status). Never ADF_NOT_UNIT.
   z may be x. Cost: at most 2 bits(|k|) products at p bits for each end, the kernel, r^k and the unit power. */
int adf_idele_pow(adf_idele_t z, const adf_idele_t x, slong k, slong prec);

/* adf_idele_pow_tight(z, x, k, prec): as adf_idele_pow, with the unit adf_ucoset_pow_tight(u, k), the smallest
   coset that contains the k-th powers of the unit of x (P13.3). The real part, the content, the statuses and
   the limits are those of adf_idele_pow. z may be x. */
int adf_idele_pow_tight(adf_idele_t z, const adf_idele_t x, slong k, slong prec);

/* ---- idele classes ---- */

/* adf_idclass_pow(z, x, k, prec): z = (Z, adf_ucoset_pow(u, k)) for x = (T, u): the k-th power of every class
   of x lies in z (A^x/Q^x = R_{>0} x Zhat^x as groups, ideles.md P15.1, line 370; api-2.md L.3). Z is kernel B
   on the ends of Statement K for the positive ball T, with the sign +1. k = 0: z = <1 ; [1]>, exact.
   Status: ADF_OK, z written; ADF_NOT_DETERMINED (B1), z untouched; ADF_LIMIT if prec > ADF_IDELE_PREC_MAX,
   z untouched. z may be x. Cost: as adf_idele_pow without the content. */
int adf_idclass_pow(adf_idclass_t z, const adf_idclass_t x, slong k, slong prec);

/* adf_idclass_pow_tight(z, x, k, prec): as adf_idclass_pow, with the unit adf_ucoset_pow_tight(u, k). */
int adf_idclass_pow_tight(adf_idclass_t z, const adf_idclass_t x, slong k, slong prec);

#ifdef __cplusplus
}
#endif

#endif /* ADELEFELD_IDPOW_H */
