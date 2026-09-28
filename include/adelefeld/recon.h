/* adelefeld/recon.h: rational reconstruction from a full adelic ball.

   Contract: docs/SPEC.md 9.2, first item; docs/proofs/quotient.md Proposition 11 (line 257);
   docs/conventions.md 6.8 (DECISION CV-51, D3: the real interval is closed; statuses), 3.2 row
   "Reconstruction and solvers", 4.3. Implemented in work package 1.6 (docs/PLAN.md section 6).
   Reference: tests/ref/adfref/recon.py, function reconstruct; vectors tests/ref/vectors/recon.jsonl.

   The problem. Given a closed real interval [lo, hi] and a finite ball a + N Zhat, find the
   rationals q with (q ; q) in [lo, hi] x (a + N Zhat). By quotient.md P11 they are
   - for N > 0: a + N k for the integers k with ceil((lo - a)/N) <= k <= floor((hi - a)/N);
     at most one if hi - lo < N, at least one if hi - lo >= N;
   - for N = 0: a, if lo <= a <= hi; none otherwise.
   No lattice reduction and no height bound is used. The finite ball may be local; its set is used.

   Statuses (conventions 6.8): exactly one candidate: ADF_OK, the output holds it; none:
   ADF_NO_SOLUTION; several: ADF_NOT_UNIQUE. adf_adele_reconstruct also returns ADF_LIMIT
   (decision M1-D3; conventions 3.2 allows it for reconstruction, "a size bound of an algorithm",
   conventions 3.1) exactly when the midpoint or the radius of the arb is not zero and its binary
   exponent (arf.h, ARF_EXP; mag.h, MAG_EXP) is above ADF_RECON_EXP_MAX in absolute value. The
   rule is on the exponents, not on the size of the end points: a ball at the bound is
   converted, and its exact end points have about ADF_RECON_EXP_MAX + 2 bits (measured,
   lanes/m1-repair-recon/probe_exp.out). The test is made before any integer of that size is
   built, so the cost of the call is bounded by the sizes of the mantissas, of the finite ball
   and by about ADF_RECON_EXP_MAX bits. On every status other than ADF_OK the output is
   untouched (conventions 4.3). "Uniqueness not certified" (ADF_NOT_DETERMINED) belongs to
   reconstruction from partial data (SPEC 9.2, second item), which is not declared here.
   Aliasing: the output is an adf_rat and aliases no input. Cost: a constant number of divisions,
   floors and ceilings of rationals of the input sizes. */

#ifndef ADELEFELD_RECON_H
#define ADELEFELD_RECON_H

#include "adelefeld/common.h"
#include "adelefeld/status.h"
#include "adelefeld/rat.h"
#include "adelefeld/fball.h"
#include "adelefeld/adele.h"

/* The largest absolute binary exponent of a midpoint or radius that adf_adele_reconstruct
   converts to an exact rational (decision M1-D3): 2^20, that is end points of about 10^6 bits. */
#define ADF_RECON_EXP_MAX 1048576

#ifdef __cplusplus
extern "C" {
#endif

/* adf_fball_reconstruct(q, x, lo, hi): the reconstruction above with the finite ball x and the
   closed interval [lo, hi] given by exact rational end points. lo > hi is the empty interval:
   ADF_NO_SOLUTION. */
int adf_fball_reconstruct(adf_rat_t q, const adf_fball_t x, const adf_rat_t lo, const adf_rat_t hi);

/* adf_adele_reconstruct(q, x): the reconstruction above for the full ball x = I x F, with
   [lo, hi] = [mid - rad, mid + rad] of the arb I, its exact dyadic end points (conventions 6.8),
   and the finite ball F. Cost: as above, with the end points converted exactly to rationals. */
int adf_adele_reconstruct(adf_rat_t q, const adf_adele_t x);

#ifdef __cplusplus
}
#endif

#endif /* ADELEFELD_RECON_H */
