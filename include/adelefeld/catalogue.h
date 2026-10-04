/* adelefeld/catalogue.h: arithmetic catalogue, WP 1F.9, lane f-slice13.

   Proposed N-D19, before implementation. One header/source pair groups the small arithmetic
   operations left after symbol.h. Alternatives: one header per operation, or declarations in
   existing input headers. Existing value types only; status returned, value through an output,
   optional where after it. Alternative: a new policy/result type. Named operations make the
   conservative and smallest enclosures visible at the call site.

   Common contract: initialised canonical inputs, checked on entry in an INV build (conventions
   4.4). A same-type output may equal its input. Outputs never overlap input subobjects or where.
   On OK the result is canonical. On every other status the value output is untouched. where may
   be NULL and is always untouched: these operations examine the finite part without naming a prime.
   No real precision argument. No new allocation beyond FLINT and existing value constructors.

   Slice A: k is ulong. Alternative: fmpz k, with conversion and a separate limit. The conservative
   operation admits k <= 4096; the smallest operation admits k <= 256, since it costs k binomials,
   each with k factors. Larger k returns LIMIT, before domain checks. Alternatives: unbounded k,
   or a caller budget. Integer inputs have arbitrary bit length. Both operations admit exact
   integer points (radius 0). Nonintegral canonical (A+H Zhat)/d: DOMAIN if gcd(H,d) does not divide A;
   otherwise NOT_DETERMINED, because the input meets Zhat and its complement. This domain rule
   also applies at k=0; the stated domain is Zhat, not a rational extension of the polynomial.
   Formula: docs/proofs/catalogue.md:291-312, Proposition 14. See docs/api-1f9.md Y9-Y10. */
#ifndef ADELEFELD_CATALOGUE_H
#define ADELEFELD_CATALOGUE_H
#include "adelefeld/fball.h"
#include "adelefeld/ucoset.h"
#include "adelefeld/idclass.h"
#ifdef __cplusplus
extern "C" {
#endif

#define ADF_BINOM_K_MAX 4096
#define ADF_BINOM_TIGHT_K_MAX 256

/* binom(x,k): centre binom(a,k), radius N/gcd(N,k!), for integral x=a+N Zhat.
   N=0 or k=0 gives an exact integer. Status: OK, LIMIT, DOMAIN, NOT_DETERMINED. y may be x. */
int adf_fball_binom(adf_fball_t y, adf_place_t *where, const adf_fball_t x, ulong k);

/* Smallest ball: radius gcd(binom(a+Nj,k)-binom(a,k), j=1..k); empty gcd 0.
   Same statuses and alias rule. No factorisation. */
int adf_fball_binom_tight(adf_fball_t y, adf_place_t *where, const adf_fball_t x, ulong k);

/* Slice B, N-D19 continued, before implementation. Exponent x is an adf_fball integral on Zhat,
   with arbitrary-size integer centre e and radius M, including exact integers M=0. Alternatives:
   slong e and ulong M, or separate fmpz parameters. Domain and common failure/where rules above apply.
   Default is strict target precision, matching an operation that promises the input unit modulus.
   Alternative: default to coarsening. Coarsening and unrestricted finest are separately named.
   The target is canon(N), since equal unit cosets must give equal statuses. The coarser modulus
   is canon(gcd(N,c^M-1)); computing it needs one modular power and no factorisation.
   Exact e=0 gives [1]. Exact nonzero e agrees with idpow.h's default enclosure; when e fits slong,
   that implementation is reused. Larger exact exponents use modular powers with explicit inversion.
   Exact bases [1],[-1] are also admitted: [1] is constant; [-1] requires exponent parity. An unknown
   parity gives NOT_DETERMINED in the strict interface and [1 mod 1] in both enclosure interfaces.
   If parity is fixed the result is exact. Alternative: refuse exact bases outside Proposition 12.

   Finest: Proposition 13, docs/proofs/catalogue.md:245-287. g=gcd(e,M), or |e| for M=0, must be
   <=256. Otherwise LIMIT, after the domain check and the constant exact cases above. Alternatives:
   unlimited g factorisation/divisor enumeration, a caller budget, or supplied factorisation.
   N has arbitrary bit length. Reuse idpow.h's tight power of U(1) at g to obtain the outside-prime
   block; repeated gcds remove the primes dividing N. The inside block is gcd(N*g_N,c^M-1), where
   g_N is the part of g supported on N, also obtained by gcds. This avoids factorisation of N.
   No integer c^M is formed. The centre is CRT, not c^e at primes outside N.
   Value cost: tight integer power at g<=256, repeated gcds, modular powers, one CRT.
   Status: strict OK, DOMAIN, NOT_DETERMINED; coarse the same except no precision refusal;
   finest additionally LIMIT. y may equal a. No cross-type or input-subobject output alias. */
#define ADF_PROFPOW_FINE_G_MAX 256
int adf_ucoset_profpow(adf_ucoset_t y, adf_place_t *where,
                       const adf_ucoset_t a, const adf_fball_t x);
int adf_ucoset_profpow_coarse(adf_ucoset_t y, adf_place_t *where,
                              const adf_ucoset_t a, const adf_fball_t x);
int adf_ucoset_profpow_fine(adf_ucoset_t y, adf_place_t *where,
                            const adf_ucoset_t a, const adf_fball_t x);

/* Slice C, N-D19 continued. Reuse the existing adf_fball_haar_volume(adf_rat_t,adf_fball_t)
   declared in fball.h:188 and implemented in src/fball.c:618-625. Alternative: a new fmpq wrapper
   or a status plus where. This function cannot fail on a canonical finite ball and returns void
   (conventions 2.3); no where or limits apply. No duplicate implementation is introduced.
   vol(a+N Zhat)=1/N for rational N>0; volume 0 for a point. Translation and centre do not matter.
   Proposition 10, docs/proofs/catalogue.md:202-212; index proof in docs/api-1f9.md Y13.
   Cross-type output does not overlap any input subobject. Debug entry check applies. */

/* Slice D, N-D19 continued, before implementation. Names fixed by conventions 6.6/CV-53,
   SPEC 9.3.7/M0-D10: exponent u or inverse u, without arithmetic/geometric names.
   Alternatives: a convention argument or those unsourced names. Output j and order n are fmpz;
   alternative: ulong n. j may equal n, but must not overlap x or its subobjects or where.
   n>=1 is required: n<=0 gives DOMAIN. Otherwise OK iff the class unit is exact or
   canon(n)|canon(N), else NOT_DETERMINED. j is the exponent in [0,n), including j=0 for n=1.
   Both canonical moduli are used for certification, but the exponent is returned modulo the
   original n: when n=2m with m odd the unique odd lift is required. The real coordinate is ignored.
   No limits, factorisation or new value type. All failures preserve j and where; all successes
   preserve where. Debug checks the whole class, including its real coordinate.
   Proposition 15, docs/proofs/catalogue.md:318-343. Test idele vector :324-326 and SPEC 9.3.7:
   p at p, 1 elsewhere gives u'=p^(-1) away from p and 1 at p; inverse exponent is p for (p,n)=1,
   and 1 on p-power roots. Sources: refs/src/milne-cft/CFT.txt:9883-9886, 9904-9908 and 1307. */
int adf_idclass_cyclo_exp_u(fmpz_t j, adf_place_t *where, const adf_idclass_t x, const fmpz_t n);
int adf_idclass_cyclo_exp_uinv(fmpz_t j, adf_place_t *where, const adf_idclass_t x, const fmpz_t n);

#ifdef __cplusplus
}
#endif
#endif /* ADELEFELD_CATALOGUE_H */
