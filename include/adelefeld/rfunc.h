/* adelefeld/rfunc.h: the real functions at the archimedean place: thin wrappers around arb, on a real ball and on
   the real component of an adf_sball (milestone 1F, work package 1F.2, first slice).

   Contract: docs/PLAN.md milestone 1F row 1F.2 ("wrapper tests only: projection, guard digits, translation of
   statuses, balls crossing 0 or a branch cut"); docs/SPEC.md 9.3.1 (f_at over a named set of places; "if one place
   fails, the function returns the status with that place and no value"), 9.3.3 (real roots: n odd: one real root;
   n even: a >= 0 and the non-negative root is returned), 9.3.6 (typed values), 10.3; docs/conventions.md 2.2, 3.1
   (the meaning of DOMAIN and NOT_DETERMINED), 3.3, 4.3, 4.4 (a non-finite ball produced from finite inputs is never
   stored: NOT_DETERMINED, CV-08); docs/proofs/functions.md Proposition 14 (line 446: real roots), Proposition 22
   (line 725: typed values over a named place set); docs/api-1f.md statements S5 to S7 (domains, statuses and
   the enclosure of the image by the end points for the monotone functions). The arb functions are called with
   the working precision prec (arb.rst: arb_exp line 1082, arb_log 1050, arb_sin and arb_cos 1101 to 1103,
   arb_sqrt 945, arb_root_ui 979).

   Meaning. Each function is an enclosure of the IMAGE of the whole input ball (SPEC 9.3.1): the output ball
   contains f(t) for every real t in the input ball. It is not claimed to be the smallest ball; arb's own
   propagation is used, and the tests bound the radius against the true width of the image.
   Domain, by the rule of conventions 3.1: DOMAIN is proved only when every point of the input ball lies outside
   the domain; a ball that meets the domain and its complement gives NOT_DETERMINED; a ball inside the domain
   gives OK.
       exp, sin, cos:       domain R; OK. Exception: exp of an argument so large that arb returns an infinite
                            ball is NOT_DETERMINED, by the rule below (measured with FLINT 3.0.1: exp(2^60) is OK,
                            exp(2^1000) is NOT_DETERMINED; exp of a very negative argument is OK, a ball around 0).
       log:                 domain t > 0. All points <= 0 (arb_is_nonpositive): DOMAIN. All points > 0: OK. Else
                            NOT_DETERMINED. (The exact 0 is DOMAIN; a ball [-1, 1] is NOT_DETERMINED.)
       log_abs = log |t|:   domain t != 0. The exact 0: DOMAIN. A ball that contains 0 and is not the exact 0:
                            NOT_DETERMINED. Else OK.
       sqrt = root of degree 2 (SPEC 9.3.3: the non-negative root): domain t >= 0. All points < 0: DOMAIN; all
                            points >= 0: OK (the exact 0 gives the exact 0); else NOT_DETERMINED.
       root of degree n:    n = 0: DOMAIN (invalid degree, no place is at fault). n odd: domain R, OK for every ball;
                            the odd root of a negative ball is minus the root of the negated ball, a ball
                            that contains 0 is enclosed by the images of its outer end points (the root is
                            increasing), the exact 0 gives the exact 0. n even >= 2: as sqrt.
   The wrapper never returns a NaN or an infinite ball with OK: a result that arb_is_finite rejects is
   NOT_DETERMINED, and nothing is written (conventions 4.4, CV-08). The functions call arb only on inputs whose
   domain was checked, because arb returns NaN for the odd root of a negative ball and of 0 (probe of lane
   d-functions, unreviewed; the tests of this slice check it again: tests/test_rfunc.c).

   Statuses, both levels: OK; DOMAIN; NOT_DETERMINED. The arb-level functions also return DOMAIN for a
   non-finite input ball (a constructor-like invalid input, conventions 3.1). Outputs are untouched on a status.
   Aliasing: y may be x.
   A prec below 2 is taken as 2 (M1-D4). No upper bound on prec is checked here: a prec that arb cannot allocate
   is the caller's error, as for adf_adele_mul (adele.h). */

#ifndef ADELEFELD_RFUNC_H
#define ADELEFELD_RFUNC_H

#include <flint/arb.h>

#include "adelefeld/common.h"
#include "adelefeld/status.h"
#include "adelefeld/place.h"
#include "adelefeld/sball.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- real balls ---- */

/* adf_real_exp(y, x, prec) ... adf_real_sqrt(y, x, prec): y = f(x) as above.
   adf_real_root(y, x, n, prec): y = the n-th root of x, n a ulong (SPEC 9.3.3). Cost: one arb call (two, for an
   odd root of a ball containing 0, or an even root of a ball with lower end 0). */
int adf_real_exp(arb_t y, const arb_t x, slong prec);
int adf_real_log(arb_t y, const arb_t x, slong prec);
int adf_real_log_abs(arb_t y, const arb_t x, slong prec);
int adf_real_sin(arb_t y, const arb_t x, slong prec);
int adf_real_cos(arb_t y, const arb_t x, slong prec);
int adf_real_sqrt(arb_t y, const arb_t x, slong prec);
int adf_real_root(arb_t y, const arb_t x, ulong n, slong prec);

/* ---- the same at a named place of a partial ball (SPEC 9.3.1: f_at with one place) ---- */

/* adf_sball_exp_at(y, where, x, v, prec), and log, log_abs, sin, cos, sqrt likewise; adf_sball_root_at(y, where,
   x, v, n, prec): y = the partial ball over the one place v that holds f applied to the component of x at v
   (arch = REAL, len = 0, inf = the real result + 0 i). The other components of x are not part of y
   (docs/proofs/functions.md Proposition 22).
   v must be a place of x and the archimedean place, whose tag is REAL:
   Status: ADF_OK, y written (where untouched);
     ADF_DOMAIN, where = v, if v is not a place of x;
     ADF_UNSUPPORTED, where = v, if v is a prime of x (functions at primes are the milestone 1F.4, not this slice),
       or, where = the archimedean place, if the tag is COMPLEX (complex wrappers are not in this slice);
     ADF_DOMAIN or ADF_NOT_DETERMINED, where = v, with the meaning of the table above (a domain failure at the place
       v; no value is written); ADF_DOMAIN with where untouched for a root of degree 0.
   The checks on v come first, in the order above. y untouched on every status other than OK.
   Aliasing: y may be x. Cost: as the arb-level function. */
int adf_sball_exp_at(adf_sball_t y, adf_place_t * where, const adf_sball_t x, adf_place_t v, slong prec);
int adf_sball_log_at(adf_sball_t y, adf_place_t * where, const adf_sball_t x, adf_place_t v, slong prec);
int adf_sball_log_abs_at(adf_sball_t y, adf_place_t * where, const adf_sball_t x, adf_place_t v, slong prec);
int adf_sball_sin_at(adf_sball_t y, adf_place_t * where, const adf_sball_t x, adf_place_t v, slong prec);
int adf_sball_cos_at(adf_sball_t y, adf_place_t * where, const adf_sball_t x, adf_place_t v, slong prec);
int adf_sball_sqrt_at(adf_sball_t y, adf_place_t * where, const adf_sball_t x, adf_place_t v, slong prec);
int adf_sball_root_at(adf_sball_t y, adf_place_t * where, const adf_sball_t x, adf_place_t v, ulong n,
                      slong prec);

#ifdef __cplusplus
}
#endif

#endif /* ADELEFELD_RFUNC_H */
