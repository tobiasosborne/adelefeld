/* Slice 3.2-a: Tate's additive character. docs/api-3.md 3.1-3.3, Q4;
   docs/proofs/analysis.md Lemma 2:76-94; conventions 6.1:844,876-887 (M0-D11, CV-54, CV-59).
   E(t) = exp(2 pi i t), psi_inf(t) = E(-t), psi_f(a) = E(a).
   Sources: refs/src/tate-poonen/notes.txt:693-700,733-740.
   Initialized canonical inputs and initialized outputs are required. Different types:
   outputs never overlap inputs or their members. All failures leave outputs untouched.
   No contexts are created or changed. Numerical p = max(prec,2); precision LIMIT is first. */
#ifndef ADELEFELD_PSI_H
#define ADELEFELD_PSI_H

#include "adelefeld/qclass.h"
#include "adelefeld/lball.h"
#include "adelefeld/place.h"
#include <flint/acb.h>

#ifndef ADF_REAL_PREC_MAX
#define ADF_REAL_PREC_MAX 2097152
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Exact phase getters: OK writes canonical theta in [0,1) iff the ENTIRE image
   is a singleton; NOT_DETERMINED otherwise. fball: N integer (including zero);
   adele: also rad(inf)=0. LIMIT for D3-2 arithmetic bounds, first.
   No floating-point evaluation. Source: analysis L2 and Q4.
   Cost: canonical finite triple and rational modular reduction. No member aliasing. */
int adf_fball_psi_tate_phase(fmpq_t theta, const adf_fball_t x);
int adf_adele_psi_tate_phase(fmpq_t theta, const adf_adele_t x);

/* Evaluate E(theta), with theta canonical in [0,1), into an acb enclosure.
   OK writes z; LIMIT for prec or phase size; NOT_DETERMINED for certificate failure.
   One sin/cos evaluation, reduced rational argument, no member aliasing.
   Exact cardinal phases are assigned as exact complex integers.
   Source: refs/src/flint-3.0.1/arb.rst:1125-1138.
   Noncanonical or out-of-range theta violates the precondition (checked under INV).
   Uses the section 3.3 endpoint allowance stated below. */
int adf_phase_get_acb(acb_t z, const fmpq_t theta, slong prec);

/* z encloses every psi(xi) for xi in the adele x. Use the rectangular hull H of
   Q4 and its specified numerical error allowance, not an arbitrary enclosing square.
   OK writes z, including for fractional finite radius. LIMIT for precision or
   D3-2 exact-arithmetic bounds; NOT_DETERMINED if the numerical certificate cannot
   be obtained. No output is written on a failure.
   Source: analysis L2:76-91, Q4. Different types: no member aliasing.
   Cost: canonical finite triple, rational modular arithmetic, four real cosine
   evaluations; independent of the numeric value of denominator(N).
   Bounds: ADF_QCLASS_EXP_MAX and ADF_QCLASS_BITS_MAX (qclass.h), including
   projected intermediate sizes before shifts/products. Cosines are evaluated at
   min(p+32, ADF_REAL_PREC_MAX), doubled to the cap until width <= 2^-p.
   Coordinates use Q1 midpoint rounding and RU30 plus one successor for nonzero radius.
   Each endpoint exceeds the true hull by at most
       4*2^-p + 2^-28*(W/2 + 2*2^-p), where W is its true coordinate width.
   A non-finite raw real ball returns DOMAIN in a normal build; it violates the
   canonical precondition and aborts under INV, as in localfactor.h. */
int adf_adele_psi_tate(acb_t z, const adf_adele_t x, slong prec);

/* Same enclosure and cost, but NOT_DETERMINED if denominator(N) > 1, before
   numerical evaluation. Integer N includes zero. Real uncertainty is allowed.
   LIMIT precedence for precision and preflight size checks remains first.
   CV-07 and CV-59 decide this behavior. Other contracts as the default. */
int adf_adele_psi_tate_strict(acb_t z, const adf_adele_t x, slong prec);

#ifdef __cplusplus
}
#endif
#endif /* ADELEFELD_PSI_H */
