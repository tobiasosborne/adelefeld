/* adelefeld/tate.h: the global Tate test vector and the global Tate integral in Re(s) > 1 (slice 5c).
   Contract: docs/api-5.md section 1 (values, scope, statuses, caps D1 and D2), section 3 (the two declarations
   below, with their comment blocks, decision D3), section 4 (the splitting, P15's cutoffs and the quadrature of
   decision D4), section 6 (T1 to T5); docs/SPEC.md 8 and 15.4 N-D24; docs/conventions.md 6.5 (the test vector).
   Proofs: docs/proofs/analysis.md Propositions 11 (:452-495), 12 (:496-563), 13 (:564-626), Lemma 14 (:627-651),
   Proposition 15 (:652-732), Lemma 6 (:213-259). Statements, proofs and decisions where the design is silent:
   docs/api-5b.md "Slice 5c". Code: src/tate.c.

   Common contract (api-5.md section 1). Inputs and outputs are initialized; a canonical chi is a precondition,
   checked under ADF_CHECK_INVARIANTS (INV) after the precision and size preflight. Only the finite character of
   chi is read; chi->s is ignored. Every status other than ADF_OK leaves every output untouched, bit for bit:
   results are built in temporaries and swapped in after the last check. Statuses of the row "Integrals, Poisson
   summation" of conventions 3.2: OK, DOMAIN, NOT_DETERMINED, LIMIT. A setup failure of a character dependency
   (UNSUPPORTED) is returned as NOT_DETERMINED. Order of the checks: prec > ADF_REAL_PREC_MAX (LIMIT); the size
   preflight (LIMIT); INV; the domain. */
#ifndef ADELEFELD_TATE_H
#define ADELEFELD_TATE_H

#include "adelefeld/common.h"
#include "adelefeld/status.h"
#include "adelefeld/char.h"
#include "adelefeld/ffun.h"
#include "adelefeld/rfun.h"
#include <flint/acb.h>

/* D2 (api-5.md:40-46): the total work of one call, charged over every attempt and retry: one unit per phase
   or array entry, per multiply-add of the finite transform, per step of a Lemma 6 series search (prefix
   included), per Taylor coefficient. */
#define ADF_TATE_WORK_MAX ((slong) 1048576)
/* The largest conductor of the transform-based global path: the direct finite transform needs C^2 <= 2^20
   (D1 of milestone 4, api-5.md:45); above it adf_tate_integral returns LIMIT before any setup. */
#define ADF_TATE_TRANSFORM_C_MAX ((ulong) 1024)

#ifdef __cplusplus
extern "C" {
#endif

/* Build precisely f_chi,phi_e above, ignoring chi->s; initialized independent factor outputs.
   OK commits both. LIMIT for precision/C/D1; NOT_DETERMINED for uncertified character phases.
   No mathematical DOMAIN on canonical input. Cost O(C) character phases and factor storage.
   Existing lifecycle, text and dump calls own the resulting factors.
   (api-5.md:152-156.) The vector of conventions 6.5 and analysis P11: f has D = 1, M = C and
   f[j] = chi(j) for j in [0, C), the exact 0 on nonunits, f[0] = 1 for C = 1 (analysis L8: chi(n) = 1 for every
   n when C = 1); each f[j] is the value of adf_char_chi(j) at max(prec, 2): exact for the phases 0, 1/4, 1/2,
   3/4 (psi.h, adf_phase_get_acb), else a ball of that precision. phi is one term P = x^e (e = the parity of chi),
   A = 1, B = C_real = 0, all exact. LIMIT: prec > ADF_REAL_PREC_MAX, then C > ADF_CHAR_MOD_MAX (before any
   character setup). phi and f must be distinct objects; neither may be a member of chi. */
int adf_tate_vector(adf_rfun_t phi, adf_ffun_t f, const adf_char_t chi, slong prec);

/* I_chi over the WHOLE spectral ball, with certified Re(s)>1. DOMAIN if upper(Re s)<=1;
   NOT_DETERMINED if it also contains points to the right of 1 but lower(Re s)<=1.
   Nonfinite s DOMAIN. bits in [0,2^21], else DOMAIN after precision/size checks.
   OK certifies finite output, each coordinate diameter <=2^-bits. Failure preserves z.
   Aliasing z=s allowed, not chi->s. LIMIT for D2; numerical/width failure NOT_DETERMINED.
   Cost the vector/character calls plus O(C^2) transform and O(N sum_panels(J+1)) Taylor terms
   and bound searches. Phase calls include their character setup cost, as char.h specifies.
   (api-5.md:157-164.) I_chi(s) = pi^-z Gamma(z) L(s, chi), z = (s + e)/2 (analysis P11), computed as
   C^-z Lambda(s, chi) with Lambda by the balanced split of analysis P13 step 3: the vector of adf_tate_vector
   with A = 1/C, its two transforms (adf_ffun_fourier, adf_rfun_fourier) giving the dual coefficients
   W_chi conj(chi(n)) n^e, the cutoffs N and R of P15 (Lemma 6, Lemma 14), the composite Taylor rule T3 on the
   panels [2^k, 2^(k+1)] of [1, R] with its geometric remainder, the pole terms 1/(s-1) - 1/s for C = 1.
   Order of the checks: prec > ADF_REAL_PREC_MAX (LIMIT); C > ADF_TATE_TRANSFORM_C_MAX (LIMIT, D2); bits outside
   [0, 2^21] (DOMAIN); INV; s nonfinite (DOMAIN); the half-plane (DOMAIN, NOT_DETERMINED); then the work.
   LIMIT: more than ADF_TATE_WORK_MAX work units in all. The working precision starts at max(prec, 2) and is
   doubled up to ADF_REAL_PREC_MAX while the width 2^-bits is not met; NOT_DETERMINED once a doubling from a
   precision >= 64 does not halve the largest diameter (fixed input radii can hold the width, SPEC 15.4 N-D23),
   or at the cap. Statements and the soundness argument: docs/api-5b.md "Slice 5c". */
int adf_tate_integral(acb_t z, const adf_char_t chi, const acb_t s, slong bits, slong prec);

#ifdef __cplusplus
}
#endif

#endif /* ADELEFELD_TATE_H */
