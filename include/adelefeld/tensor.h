/* adelefeld/tensor.h: evaluation and additive integrals of test functions, design section 6 (slice 4f).
   Contract: docs/api-4.md section 1 (common contract, caps D1, pair calls (phi, f) of D2), section 6 (statement
   E1 steps 1-5, the finite integral sum f[j]/M and squared norm sum |f[j]|^2/M of analysis Proposition 4,
   docs/proofs/analysis.md:150-173), decision D3 (docs/SPEC.md 15.4, N-D23: the partial-place evaluator);
   docs/SPEC.md 7 (evaluation at an adele on a ball that crosses a jump of the finite part: an enclosure of all
   values; the Haar integral); conventions 5.9 (adf_sball), 6.2 (Z_p volume 1, N Zhat volume 1/N).
   Statements, proofs and decisions where the design is silent: docs/api-4c.md "Slice 4f".

   Why the three adf_ffun_* functions are declared here and not in adelefeld/ffun.h: slice 4b appends the
   finite algebra to src/ffun.c and ffun.h in a parallel lane; this slice keeps to its own files so that the
   two lanes do not edit the same file. The orchestrator may move the declarations later; the ABI does not
   depend on the header that declares them.

   Common contract (api-4.md section 1). Inputs and outputs are initialized; canonical inputs are preconditions,
   checked on entry under ADF_CHECK_INVARIANTS (INV) after the size preflight. The output z is an acb_t or arb_t
   of its own; it must not be a member of an input (no member aliasing). Every status other than ADF_OK leaves z
   untouched: results are built in temporaries and swapped in after the last check. Every call first rejects
   prec > ADF_REAL_PREC_MAX with ADF_LIMIT and works at p = max(prec, 2). Statuses of the row "Integrals,
   Poisson summation" of conventions 3.2 (decision D1): OK, DOMAIN, NOT_DETERMINED, LIMIT.
   LIMIT (D1): an ffun with more than ADF_FFUN_ITEMS_MAX = 2^20 entries; more than ADF_TENSOR_WORK_MAX = 2^20
   work units (one unit per index tested at each supplied prime, at least one per index); a finite ball whose
   raw A, H or d has more than ADF_FFUN_BITS_MAX - 128 bits (so that A D, H D, M d D and j d stay within
   ADF_FFUN_BITS_MAX bits), or an lball centre with such a numerator or denominator; the rfun caps D1 of
   adelefeld/rfun.h. NOT_DETERMINED: a nonfinite result or intermediate, or the real bound of E1 step 5 not
   certified (a lower bound of pi Re(A) that is not positive). DOMAIN: a nonfinite real input (raw, not
   canonical), and the COMPLEX archimedean tag of an sball. */
#ifndef ADELEFELD_TENSOR_H
#define ADELEFELD_TENSOR_H

#include "adelefeld/common.h"
#include "adelefeld/status.h"
#include "adelefeld/fball.h"
#include "adelefeld/adele.h"
#include "adelefeld/sball.h"
#include "adelefeld/ffun.h"
#include "adelefeld/rfun.h"
#include <flint/arb.h>
#include <flint/acb.h>

/* D1 work cap of a call (api-4.md section 1: 2^20 charged work units per call). */
#define ADF_TENSOR_WORK_MAX ((slong) 1048576)

#ifdef __cplusplus
extern "C" {
#endif

/* E1 steps 1-3: z = the rectangular hull (acb_union) of f[j] over every j in [0, L), L = D M, whose coset
   j/D + M Zhat meets the finite ball x = (A + H Zhat)/d, and of the exact 0 when x is not inside (1/D) Zhat.
   (A, H, d) is the canonical global triple of x, read by adf_fball_get_fmpz3 (CRT for a local x). The coset
   meets x iff gcd(H D, M d D) divides A D - j d (H = 0: gcd(0, M d D) = M d D); x lies in (1/D) Zhat iff d | D A
   and d | D H. z encloses f(t) for every t in x and every member of f; an empty index set gives the exact 0.
   OK, LIMIT, NOT_DETERMINED. Cost: O(L) exact divisibility tests plus the global triple (a CRT for local x). */
int adf_ffun_eval(acb_t z, const adf_ffun_t f, const adf_fball_t x, slong prec);

/* z = phi(x_inf) f(x_f) enclosed for every point (x_inf, x_f) of the adele x: the product of adf_rfun_eval on
   x->inf and adf_ffun_eval on x->fin (acb_mul encloses every product of points). OK, LIMIT, DOMAIN (a
   nonfinite raw x->inf), NOT_DETERMINED; with several failures LIMIT before DOMAIN before NOT_DETERMINED.
   Cost: the sum of the two component costs. */
int adf_tensor_eval(acb_t z, const adf_rfun_t phi, const adf_ffun_t f, const adf_adele_t x, slong prec);

/* E1 steps 4-5, decision D3: z encloses phi(t_inf) f(t_f) over every adelic completion t of the tuples of the
   partial ball x. Finite part: at each supplied prime p with component a_p + p^e_p Z_p keep j iff
   v_p(j/D - a_p) >= min(e_p, v_p(M)); for an exact local point a_p keep j iff v_p(j/D - a_p) >= v_p(M); missing
   primes impose no restriction; the hull of the kept f[j] and of the exact 0 (always: an absent prime can put a
   completion outside support). Real part: arch REAL, adf_rfun_eval on the real component; arch NONE, the box
   [-R, R] + i [-R, R] with R = sum over terms of exp(gamma + beta^2/(2 alpha)) sum_j upper(|p_j|) T_j,
   alpha = pi lower(Re A), beta = upper(|Re B|), gamma = upper(Re C), T_0 = 1, T_j = (j/(alpha e))^(j/2), which
   bounds |phi| on the whole real axis. z = real enclosure times finite hull. DOMAIN for arch COMPLEX (z
   untouched); OK, LIMIT, NOT_DETERMINED. Cost: O(L * number of supplied primes) valuation tests plus the real
   evaluation or bound. No factorization of unspecified primes. */
int adf_tensor_eval_sball(acb_t z, const adf_rfun_t phi, const adf_ffun_t f, const adf_sball_t x, slong prec);

/* z = integral of f over A_f (Haar, Zhat volume 1) = (1/M) sum_j f[j] (analysis P4:157). OK, LIMIT,
   NOT_DETERMINED. Cost O(L). */
int adf_ffun_integral(acb_t z, const adf_ffun_t f, slong prec);

/* z = integral of |f|^2 = (1/M) sum_j |f[j]|^2 (analysis P4:158), the enclosure intersected with [0, infinity)
   (arb_nonnegative_part). OK, LIMIT, NOT_DETERMINED. Cost O(L). */
int adf_ffun_norm2(arb_t z, const adf_ffun_t f, slong prec);

/* z = (integral of phi) (integral of f): adf_rfun_integral times adf_ffun_integral (Fubini on the product
   measure). OK, LIMIT, NOT_DETERMINED. Cost: the two integrals. */
int adf_tensor_integral(acb_t z, const adf_rfun_t phi, const adf_ffun_t f, slong prec);

/* z = norm2(phi) norm2(f), the product intersected with [0, infinity). OK, LIMIT, NOT_DETERMINED.
   Cost: the two norms (the real norm includes O(len^2) ordered products). */
int adf_tensor_norm2(arb_t z, const adf_rfun_t phi, const adf_ffun_t f, slong prec);

#ifdef __cplusplus
}
#endif

#endif /* ADELEFELD_TENSOR_H */
