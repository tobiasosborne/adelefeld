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

/* ---- Slices 3.2-b and 3.2-c (lane q-slice5): docs/api-3.md 3.2:370-424, D3-3:809-817, Q4, Q5;
   statements docs/api-3b.md "Slices 3.2-b and 3.2-c". ---- */

/* z encloses psi of the whole represented class set: union the exact numerical
   coordinate extrema of all stored entries, then form one rectangular enclosure.
   OK, LIMIT, NOT_DETERMINED as above; every constituent must succeed. Statuses
   combine by maximum. Cost O(len) rational phase reductions and cosine evaluations.
   Source: analysis L2 step 4:92-94 (descent to A/Q) and Q4. No member aliasing.
   The status is the maximum over the stored entries of the status that
   adf_adele_psi_tate gives on that entry; prec LIMIT first, then the exact preflight
   of every entry, then the numerical evaluation of every entry. The endpoint excess
   of each coordinate is the bound of adf_adele_psi_tate with W the true width of the
   hull of the UNION (docs/api-3b.md). */
int adf_qclass_psi_tate(acb_t z, const adf_qclass_t x, slong prec);

/* D3-3, taken 2026-10-05: apply the strict finite-radius certificate to EACH STORED entry.
   NOT_DETERMINED if any entry has fractional finite radius; otherwise the same
   enclosure as the default. This certifies those representatives, not a singleton
   phase set. A legal PIECES value always passes this particular certificate.
   The status may change under an exact reduction that preserves the image set: the lift of
   E3, whose finite radius is 1/2, is NOT_DETERMINED, and its exact two-piece reduction has
   integral radii and passes, and both have the image {+1,-1}. That sentence is part of the
   contract of this function, not a remark. Other statuses, outputs, cost, aliasing: as
   qclass_psi_tate. The fractional-radius test follows the exact preflight of all entries
   and precedes all numerical evaluation. */
int adf_qclass_psi_tate_strict(acb_t z, const adf_qclass_t x, slong prec);

/* Exact phase getter on a class: OK writes canonical theta iff every stored entry has a
   singleton image (rad(inf)=0 and integer N) and all those angles agree modulo 1;
   NOT_DETERMINED otherwise (two entries with different singleton phases included).
   LIMIT for D3-2 arithmetic bounds of any entry, first. No floating-point evaluation.
   Source: analysis L2 and Q4 step 7. Cost: a rational reduction per entry.
   No member aliasing. All failures leave theta untouched. */
int adf_qclass_psi_tate_phase(fmpq_t theta, const adf_qclass_t x);

/* Local character on a local ball a+p^e Z_p (or an exact rational).
   For e >= 0 or exact input, one phase fp_p(a); for e < 0, all p^(-e) roots
   times that phase. Default encloses the entire rectangular hull; strict returns
   NOT_DETERMINED for e < 0. OK writes z; failures leave it untouched.
   Source: analysis L2:85-86, Q4. Costs: valuation removal, modular inverse, four
   trig evaluations. Check power bit sizes before forming p^k. No member aliasing.
   LIMIT for prec, D3-2 bit bound, or lball's existing exponent bounds; numerical
   certificate failure gives NOT_DETERMINED. Strict e < 0 is tested after bounds.
   Here e is the stored N of lball.h and a its canonical centre p^v u. fp_p(a) is
   (num(u) den(u)^(-1) mod p^(-v))/p^(-v) for v < 0 and 0 for v >= 0 (Q4 step 8);
   LIMIT if p^(-v) or p^(-e) has more than ADF_QCLASS_BITS_MAX bits, decided from
   k (bits(p)-1)+1 before the power is formed, or if |v|, |e| > ADF_LBALL_EXP_MAX.
   The hull and its excess are those of adf_adele_psi_tate with r = 0 and B = p^(-e). */
int adf_lball_psi_tate(acb_t z, const adf_lball_t x, slong prec);
int adf_lball_psi_tate_strict(acb_t z, const adf_lball_t x, slong prec);

/* Exact phase getter at one prime: OK writes fp_p(a) iff x is exact or e >= 0;
   NOT_DETERMINED for e < 0 after the bounds; LIMIT first, for the exponent bounds and the
   bit bound of p^(-v) and p^(-e) (not the projected distance bounds, as adf_adele_psi_tate_phase).
   No floating-point evaluation. Source: analysis L2 and Q4 step 8. Cost: one modular
   inverse after removing p-powers. All failures leave theta untouched. */
int adf_lball_psi_tate_phase(fmpq_t theta, const adf_lball_t x);

/* psi_v on the projection of x at v. At infinity use E(-I); at a prime use
   a + p^v_p(N) Z_p with N = 0 exact. Both variants permit real uncertainty.
   v is a canonical place handle, an opaque 8-byte value (conventions 7:1028-1037).
   DOMAIN, with where = v, if v is not a place of x. adf_adele has no arch tag and always
   has the archimedean place and every prime (conventions 5.5:569-582, adele.h:119-128), so for
   a canonical v that case is empty for this signature; arch = ADF_ARCH_NONE = 0 belongs to
   adf_sball, whose place set is {infinity} only if arch != 0 (conventions 5.9:697-710), and the
   same sentence covers that type if a later slice adds it. where may be NULL and is untouched
   on OK. No factorization: remove powers of the selected prime from a and N.
   LIMIT precedence and costs as above; no member aliasing. The product over all
   places recovers the adelic character, but arbitrary acb products may be wider.
   On every status other than OK, *where = v (conventions 3.2 row "functions at one
   place", localfactor.h). At infinity strict is the default: the real factor has no
   finite radius (CV-59). At a prime strict is NOT_DETERMINED iff v_p(N) < 0.
   A non-finite real ball is DOMAIN in a normal build, as for adf_adele_psi_tate. */
int adf_adele_psi_tate_at(acb_t z, adf_place_t *where, const adf_adele_t x, adf_place_t v, slong prec);
int adf_adele_psi_tate_strict_at(acb_t z, adf_place_t *where, const adf_adele_t x, adf_place_t v,
                                 slong prec);

#ifdef __cplusplus
}
#endif
#endif /* ADELEFELD_PSI_H */
