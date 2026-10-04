/* adelefeld/localfactor.h: local factors at one place (WP 1F.9, last item; lane f-slice14).

   Decision N-D20 (orchestrator, brief of lane f-slice14), from the design docs/design/local-zeta.md (lane
   d-zeta), section 3. Name: docs/conventions.md CV-60, line 953 (adf_local_zeta_factor_at; the design's
   adf_complex_local_zeta_at is not used). The gamma and epsilon factors of CV-60 join this header with
   milestone 3; they are not declared here. Contract: docs/SPEC.md 9.3.7, row "local zeta factor (trivial
   character)", line 703; conventions lines 944-951 (the pole rule) and the status table, line 223;
   catalogue.md Proposition 9; docs/api-1f9.md Y16, Y17. Sources: refs/src/tate-poonen/notes.txt:1733
   (the finite factor (1 - q_v^(-sigma))^(-1)), :1014-1016 (the real factor pi^(-s/2) Gamma(s/2)), :62-64
   (Gamma meromorphic, simple poles at 0, -1, -2, ..., no zeros). */
#ifndef ADELEFELD_LOCALFACTOR_H
#define ADELEFELD_LOCALFACTOR_H

#include <flint/acb.h>

#include "adelefeld/common.h"
#include "adelefeld/status.h"
#include "adelefeld/place.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The largest working precision, as in rfunc.h and sball.h (the same guarded definition). */
#ifndef ADF_REAL_PREC_MAX
#define ADF_REAL_PREC_MAX 2097152
#endif

/* The largest number of factors of the Gamma recurrence fallback at the real place (design Z4, real
   procedure, step 4; decision N-D20 item 4). */
#define ADF_LOCAL_ZETA_SHIFT_MAX 64

/* adf_local_zeta_factor_at(y, where, s, v, prec): the local zeta factor of the trivial character.

   The complex number s is the same spectral variable at every place; it is not a Q_p value.
   At v = p:        y = L_p(s)   = (1 - p^(-s))^(-1) = 1/(1 - exp(-s log p)).
   At v = infinity: y = L_inf(s) = pi^(-s/2) Gamma(s/2) = exp(-(s/2) log pi) Gamma(s/2).
   A finite acb input denotes its whole closed rectangle (separate real and imaginary radii, acb.rst:6-17).
   On OK, y is a finite enclosure of L_v(t) for every t of the rectangle (an enclosure of the image, not the
   smallest one; no relative accuracy is promised).
   Poles (design Z1, Z2): 2 pi i k / log p, k in Z, at p; 0, -2, -4, ... at infinity; all simple.

   prec: working precision in bits at both kinds of place; below 2 it is taken as 2. The internal working
   precision is max(2, prec) + 32; the final enclosure is rounded outward to max(2, prec) bits. Guard bits
   improve the certificate; they promise no fixed accuracy and there is no automatic retry.

   The certificate (design Z4, decision N-D20 item 4; docs/api-1f9.md Y16):
     at p: the midpoint m of s, the stable sign b = -log p for Re(m) >= 0 and b = +log p otherwise,
           exp(b m) and -expm1(b m) at the exact midpoint, the variation radius
           E >= exp(-log p |Re m|) (exp(R log p) - 1), R the Euclidean radius of the rectangle, added to both
           components; a denominator that contains 0 is NOT_DETERMINED;
     at infinity: the exact integer geometry of s/2 against 0, -1, -2, ...; then Gamma on s/2; if that is not
           finite, Gamma(s/2 + n) / ((s/2)(s/2 + 1)...(s/2 + n - 1)) with n <= ADF_LOCAL_ZETA_SHIFT_MAX; for a ball
           of positive radius with n <= 64 and max Re(s/2 + n) <= 64 the candidate is intersected with the
           midpoint value enlarged by R B (B the derivative bound of design Z6).
   No adaptive subdivision. A pole-free but wide ball may be NOT_DETERMINED.

   Statuses, in the order of the checks:
     ADF_LIMIT           prec above ADF_REAL_PREC_MAX, decided first, before every other check; at infinity
                         also: the recurrence fallback needs more than ADF_LOCAL_ZETA_SHIFT_MAX factors
                         (checked only after the pole checks and after the direct Gamma value failed);
     ADF_DOMAIN          a non-finite input (acb_is_finite false, a courtesy to raw acb callers, conventions
                         4.4); an exact pole: the exact 0 at p; an exact non-positive even integer at infinity
                         (imaginary part exactly 0). DOMAIN is never inferred from a computed enclosure;
     ADF_NOT_DETERMINED  a ball of positive radius that meets a pole, a ball whose exclusion of the poles is not
                         certified (including an exact point that is not recognised as a pole, as i 2^1000 at
                         p = 2), and a non-finite result from finite inputs (conventions 4.4, CV-08);
     ADF_OK              y written; where untouched.
   No other status (no UNIT_NOT_CERTIFIED, NEEDS_SPLIT, UNSUPPORTED or branch-cut status) is returned.
   On every status other than OK: y is untouched (its representation, not only its set) and *where = v.
   where may be NULL. No non-finite ball is ever stored.

   v: a place handle made by adf_place_inf or adf_place_prime (place.h); the latter certifies primality
   and returns DOMAIN for 0, 1 and composites without writing its output, so no other handle exists. A
   forged handle violates the precondition (checked under -DADF_CHECK_INVARIANTS, conventions 4.4).
   Every prime below 2^64; no prime-size or argument-magnitude cap other than the two LIMIT cases.
   Aliasing: y may be s. Neither overlaps where. All work is done in temporaries; y is written last.
   Cost at p: one log of a word, one exp and one expm1 of the midpoint, a few mag bounds, one complex
   division; no enumeration of poles, no loop over Im(s). At infinity: endpoint arithmetic, one Gamma and
   one exponential; at most one shifted Gamma with at most 64 factors; the midpoint refinement adds at most
   two Gamma calls. The bit cost grows with prec and with the size of s. */
int adf_local_zeta_factor_at(acb_t y, adf_place_t *where, const acb_t s, adf_place_t v, slong prec);

#ifdef __cplusplus
}
#endif

#endif /* ADELEFELD_LOCALFACTOR_H */
