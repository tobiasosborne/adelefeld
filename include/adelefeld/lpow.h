/* adelefeld/lpow.h: rational powers and powers of principal units on a local ball at one prime (milestone 1F,
   work package 1F.6; SPEC 9.3.4 items 2 and 3). Integer powers are adf_lball_pow_si (lball.h, statement L12 of
   docs/api-1f.md); the quasi-character (item 4) comes with milestone 3.

   Contract: docs/SPEC.md 9.3.1 (every result encloses the image of the WHOLE input; domain checks inspect the
   whole ball; the requested precision is an argument), 9.3.3 (branches), 9.3.4; docs/conventions.md 3.1 (DOMAIN
   only when no point of the input lies in the domain; a ball meeting the domain and its complement gives
   NOT_DETERMINED), 4.1 (aliasing), 4.3 (outputs untouched on a status); decisions N-D7 (LIMIT), N-D13 and N-D14
   (branches, K = min(N, E)) of docs/SPEC.md 15.4. Proofs: docs/proofs/functions.md Lemma 9 (line 265),
   Proposition 11 (line 336), Proposition 13 (line 410), Proposition 15 (line 463), Proposition 17 (line 577),
   Proposition 18 (line 616); docs/api-1f5.md R1 to R4 (roots), docs/api-1f.md L12 (integer powers); and the
   statements P1 to P8 of docs/api-1f6.md, which prove what this header adds.

   Notation. p is the prime of the input, v = v_p, c = 1 for odd p and c = 2 at p = 2. N is the requested
   ABSOLUTE precision (as in lfunc.h): a ball result has the error set p^K Z_p. "INF" marks an absent term
   (an exact input has no error term). Inputs are canonical adf_lball values (lball.h); with
   -DADF_CHECK_INVARIANTS every input is checked on entry (flint_abort otherwise). Every value written is canonical.
   A status other than ADF_OK leaves the output untouched (CV-06).

   Limits (N-D7, the bounds of lball.h). ADF_LIMIT, output untouched, when an input has |v| or |N| above
   ADF_LBALL_EXP_MAX (tested first, after the prime of a two-input function), when the result has an exponent
   or a valuation beyond it, or when a power p^W with W bits(p) > ADF_LBALL_BITS_MAX would be formed: the working
   powers of lroot.h and lfunc.h (Log, exp) and of lball.h (pow_si, unit_mod), and the stored centre of the result.
   No power is formed where the result is known without it: the exact results below, a centre 1 (K <= c), a
   result p^K Z_p around 0. No cost bound is promised; the working precisions are bounded by K (P3, P6), so the
   requested N bounds the cost of a ball result as well (N-D14). */

#ifndef ADELEFELD_LPOW_H
#define ADELEFELD_LPOW_H

#include "adelefeld/common.h"
#include "adelefeld/status.h"
#include "adelefeld/lball.h"

#ifdef __cplusplus
extern "C" {
#endif

/* adf_lball_powrat(y, x, e, n, seed, N): y encloses t^(e/n) on the branch named by seed, for every t in x
   (SPEC 9.3.4 item 2; functions.md Proposition 17, line 577: "integer powers and the selected n-th-root branches;
   reduce the fraction first and state the branch. A negative exponent excludes zero.").

   The fraction. g = gcd(|e|, n), e' = e/g, n' = n/g (e = 0 gives 0/1). Everything below uses e'/n' only, so
   2/4 and 1/2 are the same request with the same seed. n = 0 is ADF_DOMAIN (an invalid denominator).

   The degree n' = 1: y = x^e' EXACTLY as adf_lball_pow_si(y, x, e') (L12): the same value and the same status,
   seed and N ignored (lroot.h: degree 1 is the identity and ignores seed and N; composing it with L12 gives L12).
   In particular e = 0 gives the exact 1 for every x (L12, a convention), the exact 0 with e' < 0 is ADF_NOT_UNIT
   and a ball that contains 0 with e' < 0 is ADF_UNIT_NOT_CERTIFIED (pow_si, as inv), and an exact x gives the
   exact power. P1.

   The degree n' >= 2. The branch. seed is the identifier of the n'-th root of x in lroot.h (adf_lball_root_seed):
   at odd p the residue modulo p of the unit of the root, in [1, p - 1]; at 2 the value 1 (sign +1) or 3 (sign
   -1); 0 for the exact 0. The value is b^e', b the root of t on that branch: the root FIRST, then the integer
   power. For a reduced fraction the other order gives the same set of values, labelled by the roots of x^e'
   instead (P1); the identifier is that of the root of x, which is why the fraction is reduced first: for
   gcd(e, n) > 1 the branches of the n-th root of x do not give distinct values (with 2/2 at 5, x = 4 has the roots
   2 and 3, whose squares are both 4) and the seed would not name one value.
   The exponent. With m = v(x), j = m/n', M the exponent of a ball x, the image of the ball under the branch is
   exactly the ball b^e' + p^E' Z_p with
       E' = e' j + (M - m) - v_p(n') + v_p(e')                                       (P2: R2 composed with L12)
   (at most one of v_p(n'), v_p(e') is positive). The result has the exponent K = min(N, E') for a ball x (as
   lfunc.h, N-D14): the image itself when N >= E', else the unique ball of exponent N that contains it, which is
   p^N Z_p (centre 0) when N <= e' j. An exact x gives K = N, except that the result is EXACT when the root on
   that branch is a rational (lroot.h: integer root tests of the unit; then b^e' is the exact rational power,
   N is ignored); for a reduced fraction b^e' is rational only if b is (P1).
   Statuses (in this order): ADF_LIMIT (input exponents); ADF_DOMAIN (n = 0); [n' = 1: the statuses of pow_si];
   the statuses of adf_lball_root_seed(x, n', seed): ADF_NOT_DETERMINED for a ball that contains 0 (it meets the
   domain and its complement) and outside the guard M - m >= c + v_p(n') of SPEC 9.3.3; ADF_DOMAIN when no point of
   the guarded ball has an n'-th root (Proposition 13, R1) and for an invalid seed (a seed that names no branch,
   also when other branches exist); then ADF_NOT_UNIT for the exact 0 with e' < 0 (0^e' with e' < 0 is not
   defined; the exact 0 with e' > 0 gives the exact 0); then ADF_LIMIT (the result, or a working power).
   Aliasing: y may be x. Cost: one root (lroot.h) at relative precision max(1, K - e' j - v_p(e')), one pow_si. */
int adf_lball_powrat(adf_lball_t y, const adf_lball_t x, slong e, ulong n, ulong seed, slong N);

/* adf_lball_powunit(y, u, s, N): y encloses u^s for every u in the set u and every s in the set s, both local
   balls at the same prime (SPEC 9.3.4 item 3: "the uncertainty of s and of log u both enter").
   The function (functions.md Proposition 17, line 577): for u in 1 + p Z_p and s in Z_p,
       odd p:  u^s = exp(s log u)                         (the domain of u is 1 + p Z_p, the principal units);
       p = 2:  u^s = w^(s mod 2) exp(s log u'),  u = w u', w = +-1 = u modulo 4, u' in 1 + 4 Z_2
                                                          (the domain of u is 1 + 2 Z_2, all odd units).
   It agrees with every integer power, and u^(s+t) = u^s u^t, (uv)^s = u^s v^s (Proposition 17).
   Domain (conventions 3.1, F1 of api-1f4.md, P4): the pairs (u, s) with u in 1 + p Z_p and s in Z_p. ADF_DOMAIN if
   u does not meet 1 + p Z_p or s does not meet Z_p (an exact s with v(s) < 0; a ball of s whose points have
   negative valuation); else ADF_NOT_DETERMINED if u or s meets its domain and the complement (a ball of
   exponent <= 0 around a point of 1 + p Z_p, a ball p^B Z_p around 0 with B < 0); else the value.

   The value (Proposition 18, line 616, and P5). u0, A: the centre and exponent of u (A = INF for an exact u);
   s0, B: those of s (B = INF for an exact s); ell = log(u0 w0) (w0 the sign of u0 at 2, 1 at odd p),
   alpha = v(ell), beta = v(s0) (INF for s0 = 0: the CENTRE of s, not a point of the ball),
       R = min(A + beta, B + alpha, A + B), the terms with an INF absent;
   the image is exactly exp(s0 ell) + p^R Z_p (times the sign w0^(s0 mod 2) at 2). The result:
     - the exact 1 when an exact factor of s log u is 0: s the exact 0, or u the exact 1 (Proposition 18:
       "irrespective of the other factor"); at 2 also for the exact u = -1 (u' = 1, log u' = 0 exactly) when the
       parity of s is fixed: the exact (-1)^(s mod 2);
     - an exact u and an exact s otherwise: the ball exp(s0 ell) w0^(s0 mod 2) + p^N Z_p (as exp of an exact
       input; no rational value is detected, also not for an integer s);
     - otherwise the ball of exponent K = min(N, R) that contains the image (lfunc.h, N-D14): the image itself
       when N >= R; centre 1 (or -1 at 2) modulo p^K for K <= c; p^K Z_p for K <= 0.
   At p = 2 the sign w and the parity s mod 2 are fixed when A >= 2 (or u exact) and B >= 1 (or s exact)
   (Proposition 18 step 5). Otherwise (P5):
     - w fixed and equal to 1 (u0 = 1 modulo 4, A >= 2): the parity does not enter; the formula above;
     - A = 1 (u = 1 + 2 Z_2, every odd unit) and s0 even with B >= 1 (or s exact and even): the image is exactly
       1 + 2^R Z_2 with R = 2 + min(beta, B) (u' runs over 1 + 4 Z_2, log u' over 4 Z_2);
     - else (A = 1 with an odd or unfixed parity; w = -1 with B = 0): the image contains points of both residues
       1 and 3 modulo 4; the result is its smallest enclosing ball, 1 + 2 Z_2, at exponent K = min(N, 1). It is
       the image exactly when A = 1 and the parity is odd or B = 0, and strictly larger than the image when w = -1
       and B = 0 (P5; returned as a ball, not as ADF_NOT_DETERMINED: the domain is certain, only the image is not
       one coset of 1 + 4 Z_2; lball.h pow_si does the same for a ball around 0).
   Statuses (in this order): ADF_DOMAIN if u and s are at different primes (lball.h); ADF_LIMIT for an input
   exponent; ADF_DOMAIN, ADF_NOT_DETERMINED as above (DOMAIN wins: a pair is in the domain only if both
   coordinates are); then ADF_LIMIT (K outside the bounds, a working power).
   Aliasing: y may be u, s, or both; u and s may be the same object, which is then read as two independent sets
   (the set {a^b : a in u, b in s}). Cost: one Log at
   min(A, N - B) for alpha (only when the exponent ball is not exact and the base is a ball or an exact
   unit other than 1), one Log at max(K - beta, c), one exact product, one exp at K (P6). */
int adf_lball_powunit(adf_lball_t y, const adf_lball_t u, const adf_lball_t s, slong N);

#ifdef __cplusplus
}
#endif

#endif /* ADELEFELD_LPOW_H */
