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

   Statuses, both levels: OK; DOMAIN; NOT_DETERMINED; LIMIT (prec, below). The arb-level functions also return DOMAIN for a
   non-finite input ball (a constructor-like invalid input, conventions 3.1). Outputs are untouched on a status.
   Aliasing: y may be x.
   A prec below 2 is taken as 2 (M1-D4). A prec above ADF_REAL_PREC_MAX is ADF_LIMIT at both levels (at the level of the
   partial ball: at the archimedean place only; at a prime `prec` is an absolute p-adic precision, see below): decided from
   prec alone, before every other status (also before the domain checks and the checks on v) and before any
   allocation; the outputs are untouched; at the level of the partial ball `where` = the archimedean place. */

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

/* The largest working precision of the functions of this header and of the functions of adelefeld/sball.h that
   take a prec: 2^21 bits, the value of ADF_ROOTS_REAL_PREC_MAX and ADF_IDELE_PREC_MAX. The same definition stands
   in sball.h (guarded), which this header includes. */
#ifndef ADF_REAL_PREC_MAX
#define ADF_REAL_PREC_MAX 2097152
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
   (docs/proofs/functions.md Proposition 22: the other components of x are not part of y).
   v must be a place of x: the archimedean place (tag REAL) or a prime of x.

   THE ARCHIMEDEAN PLACE (slice 1F.2). y has arch = REAL, len = 0, inf = the real result + 0 i. `prec` is the working
   precision in bits of arb; a prec below 2 is taken as 2 (M1-D4) and a prec above ADF_REAL_PREC_MAX is ADF_LIMIT with
   where = the archimedean place, decided before every other status, also before the check that x has the place
   (only when v is the archimedean place; for a prime see below).

   A PRIME (slices 1F.4 and 1F.7). These functions exist at a prime, through adelefeld/lfunc.h:
       adf_sball_exp_at   : adf_lball_exp on the component of x at v;
       adf_sball_log_at   : adf_lball_log on it (the series; domain 1 + p Z_p);
       adf_sball_Log_at   : adf_lball_Log on it (the Iwasawa logarithm; domain every x != 0).
       adf_sball_sin_at, cos_at, sinh_at, cosh_at: the corresponding local factorial series on p^c Z_p.
   y is the partial ball over v (arch = NONE, inf = 0, len = 1, loc[0] = the result of the lfunc.h function, the
   identical fields). At a prime `prec` is NOT a number of bits: it is the requested ABSOLUTE p-adic precision N of
   lfunc.h (the error set of the result is p^K Z_p, K = N for an exact input, K = min(N, exponent of the image) for a
   ball input; lfunc.h says which). It is passed unchanged: no rounding up of a value below 2, and ADF_REAL_PREC_MAX
   does not apply at a prime. The limits at a prime are those of lfunc.h (|v| or |N| above ADF_LBALL_EXP_MAX, a
   result exponent above it, the working modulus p^W above ADF_LBALL_BITS_MAX bits): ADF_LIMIT with where = v.
   The statuses of lfunc.h arrive unchanged with where = v: ADF_DOMAIN (x does not meet the domain; the exact 0 under
   Log), ADF_NOT_DETERMINED (x meets the domain and its complement; a ball that contains 0 under Log), ADF_LIMIT.
   The other functions (log_abs, sqrt, root) at a prime are ADF_UNSUPPORTED with where = v, a valid request
   that a later slice implements; the check that v is a place of x comes first. A COMPLEX tag concerns the
   archimedean place only and does not affect a prime.

   adf_sball_Log_at at the ARCHIMEDEAN place is the real logarithm, domain t > 0, the same result and statuses as
   adf_sball_log_at (SPEC 9.3.2, "Log at all places": "The real coordinate needs a positive input, or the separately
   named log_abs"). log_abs_at is the function for a negative real input.

   Status, in the order of the checks: ADF_LIMIT (prec above the limit, v archimedean); ADF_DOMAIN, where = v, if v is
   not a place of x; ADF_UNSUPPORTED, where = v, for a prime with a function of the second list above; at the
   archimedean place ADF_UNSUPPORTED, where = the archimedean place, if the tag is COMPLEX; ADF_DOMAIN with where
   untouched for a root of degree 0 at the archimedean place; then the statuses of the function on the component,
   where = v; else ADF_OK, y written (where untouched). y untouched on every status other than OK.
   Aliasing: y may be x. Cost: as the function of arb (archimedean place) or of lfunc.h (a prime). */
int adf_sball_exp_at(adf_sball_t y, adf_place_t * where, const adf_sball_t x, adf_place_t v, slong prec);
int adf_sball_log_at(adf_sball_t y, adf_place_t * where, const adf_sball_t x, adf_place_t v, slong prec);
int adf_sball_Log_at(adf_sball_t y, adf_place_t * where, const adf_sball_t x, adf_place_t v, slong prec);
int adf_sball_log_abs_at(adf_sball_t y, adf_place_t * where, const adf_sball_t x, adf_place_t v, slong prec);
/* sin_at encloses sin(t) for every t in the component at v; only v remains in y (Proposition 22,
   docs/proofs/functions.md:725). At a prime: adf_lball_sin, domain Proposition 6:140, E = M,
   K = min(prec,E) for a ball, K = prec for an exact input; exact zero gives exact zero (F10-F13).
   At real: arb_sin at max(2,prec) bits, refs/src/flint-3.0.1/arb.rst:1101.
   Aliasing y = x allowed. OK leaves where untouched; DOMAIN, NOT_DETERMINED, LIMIT, UNSUPPORTED
   (COMPLEX real component) leave y untouched and set where = v; limits and place checks as above. */
int adf_sball_sin_at(adf_sball_t y, adf_place_t * where, const adf_sball_t x, adf_place_t v, slong prec);
/* cos_at encloses cos(t) for every t at v; only v remains (Proposition 22, functions.md:725).
   At a prime: adf_lball_cos, domain Proposition 6:140; E = M, or 2M-v_p(2) for a centred ball;
   K = min(prec,E) for a ball, prec for exact input. Exact zero gives exact one (F10-F13).
   At real: arb_cos, refs/src/flint-3.0.1/arb.rst:1103, max(2,prec) bits. Aliasing y = x allowed.
   OK, DOMAIN, NOT_DETERMINED, LIMIT, UNSUPPORTED and untouched output/where rules as for sin_at. */
int adf_sball_cos_at(adf_sball_t y, adf_place_t * where, const adf_sball_t x, adf_place_t v, slong prec);
/* sinh_at encloses sinh(t) for every t at v; only v remains (Proposition 22, functions.md:725).
   At a prime: adf_lball_sinh, domain Proposition 6:140; E = M, K = min(prec,E) for a ball,
   prec for exact input. Exact zero gives exact zero (F10-F13). At real: domain R, arb_sinh,
   refs/src/flint-3.0.1/arb.rst:1209, max(2,prec) bits, with arb's radius propagation as exp_at.
   A non-finite result is NOT_DETERMINED. Aliasing y = x allowed. OK, DOMAIN, NOT_DETERMINED,
   LIMIT, UNSUPPORTED and untouched output/where rules as for sin_at, including the real precision limit. */
int adf_sball_sinh_at(adf_sball_t y, adf_place_t * where, const adf_sball_t x, adf_place_t v, slong prec);
/* cosh_at encloses cosh(t) for every t at v; only v remains (Proposition 22, functions.md:725).
   At a prime: adf_lball_cosh, domain Proposition 6:140; E = M, or 2M-v_p(2) for a centred ball;
   K = min(prec,E) for a ball, prec for exact input. Exact zero gives exact one (F10-F13).
   At real: domain R, arb_cosh, refs/src/flint-3.0.1/arb.rst:1211, max(2,prec) bits, with arb's
   radius propagation as exp_at; a non-finite result is NOT_DETERMINED. Aliasing y = x allowed.
   OK, DOMAIN, NOT_DETERMINED, LIMIT, UNSUPPORTED and untouched output/where rules as for sin_at. */
int adf_sball_cosh_at(adf_sball_t y, adf_place_t * where, const adf_sball_t x, adf_place_t v, slong prec);
int adf_sball_sqrt_at(adf_sball_t y, adf_place_t * where, const adf_sball_t x, adf_place_t v, slong prec);
int adf_sball_root_at(adf_sball_t y, adf_place_t * where, const adf_sball_t x, adf_place_t v, ulong n,
                      slong prec);

#ifdef __cplusplus
}
#endif

#endif /* ADELEFELD_RFUNC_H */
