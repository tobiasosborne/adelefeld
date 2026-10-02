/* Local n-th roots, SPEC 9.3.3; docs/proofs/functions.md:410 (Proposition 13), :463
   (Proposition 15), :538 (Proposition 16); added proofs in docs/api-1f5.md, R1-R8.
   p is the input prime, c=2 at 2 and c=1 otherwise, s=v_p(n), m=v_p(x), j=m/n.
   Inputs are canonical lballs. Every failure leaves ALL outputs untouched.

   For n>=2 a nonexact input containing 0 or with M-m<c+s returns NOT_DETERMINED.
   On the guard, the existence criterion is constant over the entire input ball: n divides m,
   its torsion factor is an n-th power, and v_p(Log(x))>=c+s. Failure means DOMAIN: no input
   point has a root. n=0 is DOMAIN as an invalid degree. Exact 0 has just the exact root 0.
   Degree 1 is the identity for every input (including uncertain zero), ignoring seed and N.

   Branch identifiers are the residue of the root's UNIT modulo p at odd p, in [1,p-1]; at 2
   they are 1 for sign +1 and 3 for sign -1 (unit modulo 4). Identifier 0 denotes the sole
   exact-zero or degree-1 branch. Seeds must equal these identifiers, without modular reduction.
   An invalid seed is DOMAIN as an invalid selector, even when other branches exist.

   The image of a ball input a+p^M Z_p under one branch is the ball b+p^E Z_p (R2), with
       E=M-s-(n-1)j = j+(M-m)-s.
   A ball input returns the ball of exponent K=min(N,E) that contains that image, as lfunc.h
   does (decision N-D14): the image itself when N>=E, and for N<=j the ball p^N Z_p (centre 0).
   An exact input uses K=N. A rational branch is returned exactly, detected by integer
   n-th roots of the numerator and denominator of the unit; other exact-input branches are
   balls of absolute exponent N. Low N may give overlapping enclosures with distinct identifiers.
   No claim that a rational input makes every local root rational is made.

   LIMIT: input |v| or |M| > ADF_LBALL_EXP_MAX (checked first); result exponents K outside that
   bound; a required power p^W with W*bits(p)>ADF_LBALL_BITS_MAX, including the working powers
   of lfunc.h and the stored centre of the result. A power is tested only where it is formed:
   Log and exp of a unit part exactly +-1 need none (lfunc.h), nor does a result centre 1.
   Known exact roots and degree 1 ignore N. No cost bound is promised; the working
   precision is the relative precision max(K-j,c) (R4), so N bounds it for balls as well.
   Computing all branches also bounds their count by ADF_LROOT_BRANCH_MAX below.
   Count and seeded evaluation do not allocate an array proportional to gcd(n,p-1).
   Computation uses exp(Log(unit)/n) and Teichmueller factors, Lemma 9 at functions.md:265.
   Aliasing: each lball output may be x; output objects and scalar outputs are mutually distinct. */
#ifndef ADELEFELD_LROOT_H
#define ADELEFELD_LROOT_H
#include "adelefeld/lball.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Bound of the branch count gcd(n,p-1) that roots lists: its temporary identifier and lball
   arrays have that many entries (R8 lists t0 zeta^i; no polynomial is formed). The value is
   2^26/64-1, as when it bounded a coefficient vector; the driver and bindings may use it. */
#define ADF_LROOT_BRANCH_MAX (ADF_LBALL_BITS_MAX / FLINT_BITS - 1)

/* root_count: number of branches, gcd(n,p-1) at odd p, gcd(n,2) at 2, or 1 for zero/n=1.
   Proposition 13, functions.md:410; R1. No exponent is returned. Identifiers and statuses are
   as above; LIMIT only for inputs or criterion working powers. count and x are distinct. */
int adf_lball_root_count(ulong *count, const adf_lball_t x, ulong n);

/* root_seed: the branch selected by seed, with exponent min(N,E) for ball inputs, N for
   irrational exact-input roots, exact rational roots otherwise. Propositions 13/15/16, functions.md:410,
   :463,:538; R1-R4. DOMAIN, NOT_DETERMINED, LIMIT and aliasing y=x as above. */
int adf_lball_root_seed(adf_lball_t y, const adf_lball_t x, ulong n, ulong seed, slong N);

/* sqrt_seed: root_seed with degree 2, same exponent, identifier, exactness, statuses and y=x
   rules. Proposition 13 step 3, functions.md:435: at 2 the unit must be 1 modulo 8. */
int adf_lball_sqrt_seed(adf_lball_t y, const adf_lball_t x, ulong seed, slong N);

/* roots: list every branch in increasing identifier order in y[0..*len), ids[0..*len).
   y and ids have capacity slots; y slots are initialized lballs. Only the first *len slots
   are written on OK; others stay untouched. R5, R8, Propositions 13/15, functions.md:410,:463.
   Exponents, exact values, DOMAIN and NOT_DETERMINED are as above. LIMIT also if capacity
   cannot hold the count or the count exceeds ADF_LROOT_BRANCH_MAX; negative capacity is DOMAIN.
   These, and a LIMIT of K, of Log/exp or of a Teichmueller power that some branch would
   return, are decided before branches are listed.
   len, ids, y are distinct; x may alias any slot of y, even an unused slot. Transactional on
   every failure, including LIMIT partway through evaluation. Internal temporary arrays use
   flint_malloc/flint_free and are released before return. */
int adf_lball_roots(adf_lball_ptr y, ulong *ids, slong *len, slong capacity,
                   const adf_lball_t x, ulong n, slong N);
#ifdef __cplusplus
}
#endif
#endif
