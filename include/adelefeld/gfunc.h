/* adelefeld/gfunc.h: functions at ALL places at once (milestone 1F, work package 1F.8; lane f-slice10).

   Contract: docs/SPEC.md 9.3.1 (the form f(x) with no places: "an adele or idele; requires that the whole input is
   certified to lie in the domain, otherwise a status"; "If one place fails, the function returns the status with
   that place and no value"), 9.3.3 lines 636-646 (the root at all places), 15.4 N-D7, N-D8; docs/conventions.md
   2.1 line 122 (the all-places form has no suffix), 2.2 (argument order), 3.1 (statuses: DOMAIN only when every
   point of the input lies outside the domain), 3.2 line 221 (the statuses of this class: OK, DOMAIN with place,
   NOT_DETERMINED, NEEDS_SPLIT, UNSUPPORTED, LIMIT), 3.3 lines 234-247 (the combined status is the maximum; the
   reported place is the first place, in the canonical order of places, with that status; the real place is first,
   conventions 7), 4.1, 4.3; docs/proofs/functions.md Proposition 14 (line 446: real roots), Proposition 16 (line
   538: all places and the rational-root contract), Proposition 22 (line 725: "An unqualified all-place map requires
   every coordinate's domain condition and an adelic output. A stored uncertain zero does not certify an exact
   zero."); the statements G1 to G6 of docs/api-1f8.md, which prove what this header claims.

   Common rules, unless a comment says otherwise:
   - Aliasing: the output may be the same object as the input (y = x); `where` is a distinct object.
   - Every output is untouched on a status other than ADF_OK (conventions 4.3), except the report `where`.
     `where` may be NULL. On ADF_OK `where` is untouched. On a status other than OK `where` is set to the first
     place, in the canonical order (the real place first), whose status is the combined status, WHEN THAT PLACE
     CAN BE NAMED; it is untouched when the status belongs to no place (an invalid degree or sign) or to the finite
     part as a whole, where naming the first prime would need a factorisation (see each function).
   - prec is the working precision, in bits, of the real coordinate; a prec below 2 is taken as 2 (M1-D4). A prec
     above ADF_REAL_PREC_MAX (= ADF_IDELE_PREC_MAX, N-D8) is ADF_LIMIT with where = the real place, decided from
     prec alone before every other status and before any allocation; the outputs are untouched.

   THE BRANCH (SPEC 9.3.3 line 640: "for odd n the one real root, for even n the non-negative one (optionally
   both)"). `sign` is +1 or -1. sign = +1 selects the non-negative root for even n and the only root for odd n;
   sign = -1 selects the non-positive root for even n (for an adele or an idele: the negative of the root in EVERY
   coordinate, so that all coordinates lie on the same branch). For odd n the value -1 names no branch: it is
   DOMAIN, as lroot.h treats a seed that names no branch ("An invalid seed is DOMAIN as an invalid selector, even
   when other branches exist"), except that the exact 0 of the type (the rational 0; the adele whose real ball is
   the exact 0 and whose finite part is exactly 0) has the root 0 for either sign. Any other value of sign is DOMAIN
   for every n >= 2 (an invalid selector; where untouched). Degree 1 is the identity for every input and ignores
   sign, as lroot.h does ("Degree 1 is the identity for every input (including uncertain zero), ignoring seed and
   N"). Degree 0 is DOMAIN, where untouched (an invalid degree, no place is at fault; adf_real_root and lroot.h do
   the same).

   WHAT IS NOT CLAIMED (Proposition 16, functions.md:538-545; SPEC 9.3.3 lines 640-643): the functions return the
   RATIONAL branch; they do not list the adelic roots (1 has continuum many adelic square roots: the signs +1 and -1
   may be chosen independently at every place). All branches over a finite named set of places are listed by
   adf_sball_roots_at (rfunc.h) and adf_lball_roots (lroot.h). */

#ifndef ADELEFELD_GFUNC_H
#define ADELEFELD_GFUNC_H

#include <flint/arb.h>

#include "adelefeld/common.h"
#include "adelefeld/status.h"
#include "adelefeld/place.h"
#include "adelefeld/rat.h"
#include "adelefeld/fball.h"
#include "adelefeld/adele.h"
#include "adelefeld/idele.h"
#include "adelefeld/rfunc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- the root at all places (SPEC 9.3.3 lines 636-646; docs/api-1f8.md G1 to G4) ---- */

/* adf_rat_root(y, where, a, n, sign): y = the rational n-th root of the exact rational a on the branch `sign`.
   Set statement (G1, G2): for a = A/B in lowest terms (B > 0) and n >= 2, a has a rational n-th root exactly when
   |A| and B are n-th powers of integers and (n is odd or a >= 0); then the root is s |A|^(1/n) / B^(1/n) with
   s = sign(a) for odd n and s = sign for even n. Having an n-th root at every place (and also: at every FINITE
   place) is equivalent to having a rational one (Proposition 16, functions.md:538, steps 3-4; G2), so the absence
   of a rational root is a proved DOMAIN. The test is two integer root tests (no factorisation): Proposition 16
   step 4 (functions.md:565-568); an integer q >= 2 with n >= bits(q) has no exact n-th root, so a degree above the
   bit length, and above WORD_MAX, never reaches fmpz_root (docs/api-1f5.md R3 step 4; src/lroot.c:99-106;
   refs/src/flint-3.0.1/fmpz.rst:983-988: fmpz_root takes an slong degree and returns 1 if the root was exact).
   The exact 0 has the single root 0 (Proposition 16, functions.md:543), for either sign.
   Statuses, in the order of the checks:
     n = 0                                   ADF_DOMAIN, where untouched (invalid degree);
     n = 1                                   ADF_OK, y = a (sign ignored);
     sign not in {+1, -1}                    ADF_DOMAIN, where untouched (invalid selector);
     a = 0                                   ADF_OK, y = 0 (either sign);
     n odd and sign = -1                     ADF_DOMAIN, where untouched (invalid selector);
     n even and a < 0                        ADF_DOMAIN, where = the real place (no real root, Proposition 14,
                                             functions.md:446; the real place is the first place in the canonical
                                             order, and it fails);
     |A| or B not an n-th power              ADF_DOMAIN, where untouched: some prime fails (G2), and the first one
                                             cannot be named without a factorisation (with n = 2: 2 and 7 fail
                                             first at 2, 17 = 1 mod 8 first at 3; the function does not say which);
     otherwise                               ADF_OK, y = the root.
   y is untouched on a status. Aliasing: y may be a. No limit: the cost is two integer roots of the sizes of A
   and B (fmpz_root), or none when n >= the bit length. */
int adf_rat_root(adf_rat_t y, adf_place_t * where, const adf_rat_t a, ulong n, int sign);

/* adf_adele_root(y, where, x, n, sign, prec): the n-th root at all places of the adele x = (I ; F) (adele.h: the
   set I x F, the coordinates independent, adele.h:12), when F is exactly a rational q (adf_fball_is_exact,
   fball.h:154; a local value is never exact, fball.h:152-153).
   Set statement (G3): on ADF_OK, y = (J ; rho): rho is the exact rational root of q on the branch `sign`
   (adf_rat_root), and the real ball J contains s t^(1/n) for every t in I, where t^(1/n) is the real root of
   Proposition 14 (the only one for odd n, the non-negative one for even n) and s = sign for even n, s = 1 for odd
   n. J is adf_real_root(I, n, prec) (rfunc.h:79), negated for even n and sign = -1: the enclosure, statuses and
   radius of the real coordinate are those of rfunc.h. So (j, rho)^n = (t, q) for every t in I and some j in J: y
   is a root of every point of x, all its coordinates on the one branch `sign`.
   Statuses: LIMIT (prec, first, where = real); then n = 0: DOMAIN (where untouched); n = 1: OK, y = x, an exact
   copy (no rounding of I); an invalid selector: DOMAIN (where untouched; odd n with sign = -1 is accepted only for
   the exact zero adele (0 ; 0), whose root is (0 ; 0)). Then each coordinate is inspected, and the statuses combine
   by conventions 3.3 (the maximum; the place: the real place if its status is the maximum, else untouched):
     the real coordinate (where = the real place): adf_real_root's statuses on I: even n and every point of I
       negative: DOMAIN; even n and I meets both signs: NOT_DETERMINED; a non-finite result: NOT_DETERMINED;
     the finite coordinate (where untouched): F not exact (a positive radius, or the local backend):
       NOT_DETERMINED, without examining any prime of the modulus (that would need a factorisation of the radius;
       for 2 + 4 Zhat with n = 2 a look at 2 would prove DOMAIN, since every point has v_2 = 1, but no prime is
       examined); q without a rational root (even n and q < 0 included): DOMAIN (G2: no point of A_f with this
       coordinate has an n-th root, so no point of x has one).
   Examples: (-4 ; 4), n = 2: DOMAIN at the real place; (4 ; 2), n = 2: DOMAIN, where untouched; (-4 ; 2), n = 2:
   DOMAIN at the real place; ([-1, 1] ; 2), n = 2: DOMAIN (the maximum), where untouched; ([-1, 1] ; 4), n = 2:
   NOT_DETERMINED at the real place.
   y is untouched on a status. Aliasing: y may be x. Cost: one adf_real_root at prec and adf_rat_root. */
int adf_adele_root(adf_adele_t y, adf_place_t * where, const adf_adele_t x, ulong n, int sign, slong prec);

/* adf_idele_root(y, where, x, n, sign, prec): the n-th root at all places of the idele x = (X, r, u) (idele.h:
   the set of (xi, r w), xi in the real ball X, which excludes 0, w in the unit coset u).
   An idele of finite precision (a unit coset of modulus N >= 1) cannot be certified to have an n-th root at all
   places for n >= 2: some unit at a prime outside the modulus is not an n-th power (Proposition 16,
   functions.md:540 and steps 1-2; SPEC 9.3.3 line 637): NOT_DETERMINED, where untouched (the prime of that step
   is not searched).
   An idele whose unit is EXACT, [c] with c = +1 or -1 (ucoset.h:102), has the exact rational q = c r as its finite
   coordinate (idele.h:135: the idele of an exact rational), and follows the rational contract (G4): its finite
   coordinate has an n-th root in A_f exactly when q has a rational n-th root rho (G2), and then y = (J, |rho|,
   [sign(rho)]), with rho on the branch `sign` as in adf_rat_root, J = adf_real_root(X, n, prec), negated for even n
   and sign = -1. On ADF_OK, J contains s xi^(1/n) for every xi in X (s as for adf_adele_root), J excludes 0, the
   content is exactly r^(1/n) and the unit is exact, so y is a canonical idele and a root of every point of x.
   Statuses: LIMIT (prec > ADF_IDELE_PREC_MAX, first, where = real); n = 0: DOMAIN (where untouched); n = 1: OK,
   y = x, an exact copy; an invalid selector: DOMAIN (where untouched; odd n with sign = -1 always, as an idele is
   never 0). Then, combined by conventions 3.3 as for adf_adele_root:
     the real coordinate (where = the real place): even n and X negative: DOMAIN (X excludes 0, so it is wholly
       negative or wholly positive); a result that arb_is_nonzero cannot certify free of 0, or that is not finite:
       NOT_DETERMINED (the idele predicate requires a real ball that excludes 0, idele.h);
     the finite coordinate (where untouched): unit not exact: NOT_DETERMINED; exact unit and q = c r without a
       rational n-th root (c = -1 with even n included): DOMAIN.
   So an idele with a negative real ball and even n is DOMAIN at the real place even when its unit is inexact (the
   maximum of conventions 3.3; every point of the input lies outside the domain).
   y is untouched on a status. Aliasing: y may be x. Cost: one adf_real_root at prec and two integer roots. */
int adf_idele_root(adf_idele_t y, adf_place_t * where, const adf_idele_t x, ulong n, int sign, slong prec);

/* ---- the five factorial series at all places (SPEC 9.3.2 lines 588-596; docs/api-1f8.md G5, G6) ---- */

/* adf_adele_exp(y, where, x, prec), adf_adele_sin, adf_adele_sinh, adf_adele_cos, adf_adele_cosh: f at all places
   of the adele x = (I ; F) (adele.h: the set I x F, the coordinates independent).
   Domain (Proposition 12, docs/proofs/functions.md:375-387; Proposition 6, :140): f converges at a prime p exactly
   on p^c Z_p (c = 2 at 2, c = 1 at odd p), so its domain in A is R x D, D = 4 Z_2 x product over odd p of p Z_p. "D
   contains no finite ball of positive radius, and the only rational in it is 0" (SPEC 9.3.2 lines 593-595): with
   these types the all-places form applies only to an adele whose finite part is EXACTLY 0, with any real ball.
   Set statement (G5): on ADF_OK, y = (J ; k): J contains f(t) for every t in I, and k is the exact constant f(0)
   at every prime: 1 for exp, cos, cosh, 0 for sin, sinh (Proposition 12 step 1, functions.md:394: "At finite
   argument 0 their outputs are respectively 1,0,0,1,1"). The finite part is the exact rational, as at a prime
   (N-D12: "Only the exact 0 gives an exact constant"); SPEC 9.3.1 line 573 ("an exact input does not give an
   exactly representable output") states the general case, of which this is the exception already made at a prime.
   J is the real function at prec bits through adf_sball_exp_at, sin_at, sinh_at, cos_at, cosh_at at the real place
   (rfunc.h: arb_exp, arb_sin, arb_sinh, arb_cos, arb_cosh; the enclosure and radius are those of rfunc.h).
   Statuses, combined by conventions 3.3 (the maximum; `where` the first place, in canonical order, with it):
     LIMIT                prec > ADF_REAL_PREC_MAX, where = the real place, first (N-D8);
     the real coordinate  NOT_DETERMINED when arb returns a non-finite ball (exp, sinh, cosh of a huge argument:
                          exp(2^1000) is NOT_DETERMINED, rfunc.h), where = the real place; otherwise OK (domain R);
     the finite part      exact 0: OK.
                          exact q != 0: DOMAIN (G6: q is outside p^c Z_p at some prime, so every point of x is
                          outside R x D), where = the FIRST prime p in increasing order with v_p(q) < c: 2 when 4
                          does not divide the numerator of q, else the first odd prime that does not divide the
                          numerator. It exists and is found after at most floor(log_3 |num q|) + 1 odd primes (G6).
                          Examples: 1, 1/3, -9/2: 2; 4: 3; 12: 5 (4 | 12 and 3 | 12); 60: 7.
                          not exact (a positive radius, or the local backend, fball.h:152-153): NOT_DETERMINED,
                          where untouched, without examining the primes of the modulus (that would need a
                          factorisation). No look can decide for 0 + 4 Zhat (it contains 0, which is in D, and
                          points outside D, Proposition 12 step 2); a look at 2 would prove DOMAIN for 1 + 4 Zhat
                          (every point is 1 mod 4, outside 4 Z_2) and is not made.
   So (2^1000 ; 1) under exp is DOMAIN at 2 (the maximum, DOMAIN above NOT_DETERMINED); (2^1000 ; 0 + 4 Zhat) is
   NOT_DETERMINED at the real place (both parts NOT_DETERMINED, the real place first); (0 ; 0 + 4 Zhat) is
   NOT_DETERMINED with where untouched.
   y is untouched on a status. Aliasing: y may be x. Cost: one arb function at prec, and for an exact q != 0 at most
   floor(log_3 |num q|) + 2 divisions of num q by a word. */
int adf_adele_exp(adf_adele_t y, adf_place_t * where, const adf_adele_t x, slong prec);
int adf_adele_sin(adf_adele_t y, adf_place_t * where, const adf_adele_t x, slong prec);
int adf_adele_sinh(adf_adele_t y, adf_place_t * where, const adf_adele_t x, slong prec);
int adf_adele_cos(adf_adele_t y, adf_place_t * where, const adf_adele_t x, slong prec);
int adf_adele_cosh(adf_adele_t y, adf_place_t * where, const adf_adele_t x, slong prec);

#ifdef __cplusplus
}
#endif

#endif /* ADELEFELD_GFUNC_H */
