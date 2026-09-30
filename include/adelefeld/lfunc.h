/* adelefeld/lfunc.h: the power series exp and log at a prime, and the Iwasawa logarithm Log, on a local ball
   adf_lball (milestone 1F, work package 1F.4, first slice).

   Contract: docs/SPEC.md 9.3.1 (every function is an enclosure of the image of the whole input ball; domain checks
   inspect the whole ball; a requested output precision is an argument), 9.3.2 (domains and radii); docs/conventions.md
   3.1 (line 173: DOMAIN only when every point of the input lies outside the domain; a ball that meets the domain and
   its complement gives NOT_DETERMINED), 4.1 (aliasing), 4.3 (outputs untouched on a status); docs/proofs/functions.md
   Definition 1 (line 11: the series), Lemma 5 (line 117), Proposition 6 (line 140: exact domains), Proposition 7
   (line 165: the count of exp), Proposition 7b (line 198: the count of log), Proposition 8 (line 227: the working
   precision), Lemma 9 (line 265), Proposition 10 (line 299: the radius of exp), Proposition 11 (line 336: the
   radius of log and Log, Log(p) = 0); and the statements F1 to F7 of docs/api-1f4.md, which prove what the code
   adds to these (the domain test by the predicates of lball.h, the exact values, the evaluation of Log without the
   root of unity, the evaluation of exp with one common denominator, the precision of a ball result, the limits).

   Notation. p is the prime of the input x (lball.h), v = v_p, c = 1 for odd p and c = 2 for p = 2. The requested
   precision N is an absolute precision (as the field N of an adf_lball): a result ball has the error set p^K Z_p.

   Meaning of a result. Each function returns a value y that contains f(t) for EVERY point t of the input set x
   (SPEC 9.3.1). The value is:
     - exact, only where the value is a rational proved exactly: exp(0) = 1 for the exact 0; log(1) = 0; log(-1) = 0
       at p = 2 (Proposition 11); Log(p^m) = Log(-p^m) = 0 for the exact +-p^m (F2: for an exact rational x != 0,
       Log(x) = 0 exactly when x = +-p^m). The requested N is then not used.
     - otherwise the ball f(a) + p^K Z_p, a the exact input or the centre of the input ball (lball.h), with
           K = N               for an exact input,
           K = min(N, E)       for a ball input, E the exponent of the image of the ball (F6):
               exp  of a + p^M Z_p inside p^c Z_p:  E = M                       (Proposition 10: an isometry);
               log  of a + p^M Z_p inside 1 + p Z_p: E = M if M >= c, E = 2 if p = 2 and M = 1 (Proposition 11:
                                                     log(1 + 2 Z_2) = 4 Z_2);
               Log  of a + p^M Z_p not containing 0, m = v(a) < M, r = M - m:
                                                     E = r if r >= c, E = 2 if p = 2 and r = 1 (Proposition 11).
       For a ball input with N >= E the result is the image itself, which is a ball (Propositions 10, 11): the
       SMALLEST ball that contains the image. With N < E it is the unique ball of exponent N that contains the image.
       Absolute precision: Log loses m digits for m > 0 and gains -m digits for m < 0 (SPEC 9.3.2), because E = M - m.
       The centre of the result is f(a) reduced to the canonical form of lball.h (api-1f.md L0): f(a) is computed
       modulo p^K with K absolute digits (F4, F5), and never from the input precision alone.

   Domains and statuses (conventions 3.1; Proposition 6; the test is F1):
       exp: domain p^c Z_p.     OK if x lies inside it; DOMAIN if x does not meet it (an exact x != 0 with v(x) < c; a
                                ball whose centre is not 0 and has valuation < c); NOT_DETERMINED if it meets it and its
                                complement (the ball p^M Z_p around 0 with M < c).
       log: domain 1 + p Z_p.   OK inside; DOMAIN if x does not meet it (every exact x with v(x - 1) < 1, the exact 0
                                included; every ball of exponent M >= 1 not inside it; a ball of exponent M <= 0 that
                                does not contain 1); NOT_DETERMINED for a ball of exponent M <= 0 that contains 1.
                                The domain includes -1 + 4 Z_2 at p = 2, unlike FLINT's padic_log (padic.rst:506-507).
       Log: domain all x != 0.  OK for an exact x != 0 and for a ball that does not contain 0; DOMAIN for the exact 0;
                                NOT_DETERMINED for a ball that contains 0 (an uncertain zero: conventions 3.1).
   A status other than OK leaves y untouched (CV-06).

   Limits (conventions 3.1, ADF_LIMIT; the bounds of lball.h). ADF_LIMIT, y untouched, when
     - the input has |v| or |N| above ADF_LBALL_EXP_MAX (lball.h); this is tested first, before the domain, or
     - the result is a ball whose exponent K has |K| > ADF_LBALL_EXP_MAX, or
     - the series must be summed modulo p^W with W bits(p) > ADF_LBALL_BITS_MAX. W = K + D is the working precision of
       Proposition 8, D the valuation of the largest denominator of the sum: for exp D = v_p(L!), L the last degree
       kept, at most about K/(p - 2) for odd p and about K at p = 2 (so W is about 2K at p = 2 and p = 3); for log and
       Log D = floor(log_p T), T the last degree kept. This is the one limit set by an intermediate value, and it is
       stated here because the sum cannot be formed without p^W. No power is formed, and no limit applies, when the
       centre of the result is known without a sum: the exact results above; exp of a ball around 0 or K <= v(a)
       (the centre is 1); log and Log when K <= c or the unit part of a is exactly 1 (the centre is 0).
   The cost is not bounded by these limits. Exp keeps its common-denominator Horner sum, with about
   (p - 1) K / ((p - 1) v(a) - 1) terms. The tagged-word log loop is retained. Other sums with K <= 64 use decreasing term precision
   and word-size unit division (docs/api-1f4.md F8). Larger log sums split the principal unit into factors with increasing
   valuation, then evaluate their shorter series by exact binary splitting (F9). The caller chooses N.

   Aliasing: y may be x. Inputs satisfy adf_lball_is_canonical; with -DADF_CHECK_INVARIANTS each function checks x on
   entry and calls flint_abort (conventions 4.4). Every value written satisfies the predicate. */

#ifndef ADELEFELD_LFUNC_H
#define ADELEFELD_LFUNC_H

#include "adelefeld/common.h"
#include "adelefeld/status.h"
#include "adelefeld/lball.h"

#ifdef __cplusplus
extern "C" {
#endif

/* adf_lball_exp(y, x, N): y encloses exp(t) = sum k>=0 t^k/k! for every t in x (functions.md Definition 1, line
   22), with the absolute precision K of the block above (K = N for an exact x, min(N, M) for a ball of exponent M).
   Domain p^c Z_p (Proposition 6, line 140). Status: ADF_OK; ADF_DOMAIN; ADF_NOT_DETERMINED; ADF_LIMIT; y untouched on
   each status other than OK. The exact 0 gives the exact 1 (Definition 1: "at zero the factorial series take their
   constant terms", Proposition 6). Cost: one sum of Proposition 7's count of terms modulo p^W. */
int adf_lball_exp(adf_lball_t y, const adf_lball_t x, slong N);

/* adf_lball_log(y, x, N): y encloses log(t) = sum k>=1 (-1)^(k+1) (t-1)^k/k for every t in x (Definition 1, line
   27). Domain 1 + p Z_p, for every p including 2 (Proposition 6, step 4). The exact 1, and the exact -1 at p = 2,
   give the exact 0 (Proposition 11: log(-1) = 0 at 2). A ball of exponent 1 at p = 2 has the image 4 Z_2 (Proposition
   11): the result is the ball p^min(N, 2) Z_2 around 0. Status: as adf_lball_exp. On its domain log = Log
   (Proposition 11, step 3), and the value is computed as Log. */
int adf_lball_log(adf_lball_t y, const adf_lball_t x, slong N);

/* adf_lball_Log(y, x, N): the Iwasawa logarithm, Log(p^m w u) = log(u) for the decomposition of Proposition 4
   (functions.md line 92; Proposition 11, line 336: Log(p) = 0, Log(xy) = Log(x) + Log(y), Log = log on 1 + p Z_p).
   y encloses Log(t) for every t in x. Domain: every x != 0. The exact +-p^m give the exact 0 (F2). Status: ADF_OK;
   ADF_DOMAIN (the exact 0); ADF_NOT_DETERMINED (a ball that contains 0); ADF_LIMIT; y untouched on each status other
   than OK. The root of unity w is never formed: F3 (Log(x) = log(a^(p-1))/(p-1) for the unit part a of x at odd p,
   log(+-a) with +-a = 1 modulo 4 at p = 2). Cost: at most one power a^(p-1) modulo p^W, then F8's direct sum
   or F9's balanced factor sums, as described above. */
int adf_lball_Log(adf_lball_t y, const adf_lball_t x, slong N);

#ifdef __cplusplus
}
#endif

#endif /* ADELEFELD_LFUNC_H */
