/* adelefeld/symbol.h: quadratic residue and Hilbert symbols (WP 1F.9, Slices A and B).

   Proposed decision N-D18, open to reversal. Names: adf_<input>_<symbol>, in symbol.h and symbol.c.
   Alternatives: separate headers for each symbol; adding declarations to the existing input headers.
   Value: int in {-1,0,1} through a pointer, status returned, optional where after the value.
   Alternative: returning the symbol directly, which would confuse 0 with ADF_OK.
   Inputs: fmpz integers, adf_fball, adf_ucoset. Legendre's lower entry is an adf_place_t (odd prime).
   Alternative: arbitrary fmpz primes with a separate primality certification and resource policy.
   Jacobi and Kronecker lower entries are fmpz. No new value type or factorisation is needed.

   Set statement: Definition 1, docs/proofs/catalogue.md:14-27. A finite input denotes the symbols
   of every point in its set. Proposition 2 (:31-39) supplies a sufficient modulus K: p for Legendre,
   b for Jacobi, oddpart(b) for odd b, 8*oddpart(b) for positive even Kronecker b.
   This slice certifies finite inputs only when K divides N, as the brief requests. Alternatives:
   enumerate Proposition 3 (:43-57), or factor b and derive a sharper test. Divisibility is NOT necessary
   mathematically (Proposition 3:55-57). A refusal can therefore concern this algorithm's certificate.

   Common contract (conventions 2.1, 2.2, 3.1-3.3, 4.3-4.4): initialised canonical inputs; debug entry
   checks. Inputs may coincide. The int output and where never overlap each other or any input.
   On OK *value is the exact symbol and where is untouched. On every other status *value is untouched.
   where may be NULL. Legendre failures write where=the supplied place; other failures leave it untouched.
   Limits: every prime a place holds, below 2^64; arbitrary integer bit lengths, no additional size cap.
   Cost: integer reductions, gcds and FLINT's Jacobi/Kronecker routine. No enumeration or factorisation.
   FLINT domains: refs/src/flint-3.0.1/fmpz.rst:1166-1172; ulong_extras.rst:456-462.
   Special conventions at 0, -1 and 2: refs/src/pari-doc/usersch3.tex:9266-9271. */
#ifndef ADELEFELD_SYMBOL_H
#define ADELEFELD_SYMBOL_H
#include "adelefeld/fball.h"
#include "adelefeld/ucoset.h"
#include "adelefeld/place.h"
#include "adelefeld/lball.h"
#include "adelefeld/idele.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Exact integers: OK; Legendre DOMAIN for infinity or 2 (where=that place); Jacobi DOMAIN for
   b <= 0 or even b (where untouched). Kronecker accepts every integer b, including 0 and negatives.
   Definition 1 (:14-20), with the conventions at 0 and negative b pinned there. */
int adf_fmpz_legendre(int *value, adf_place_t *where, const fmpz_t a, adf_place_t p);
int adf_fmpz_jacobi(int *value, adf_place_t *where, const fmpz_t a, const fmpz_t b);
int adf_fmpz_kronecker(int *value, adf_place_t *where, const fmpz_t a, const fmpz_t b);

/* Finite balls: canonical triple (A,H,d), including local backend through get_fmpz3.
   For d=1: H=0 uses the exact integer routine; H>0 uses K|H, else NOT_DETERMINED.
   For d>1: DOMAIN when gcd(H,d) does not divide A (no point lies in Zhat); otherwise
   NOT_DETERMINED (the ball meets Zhat and its complement). See docs/api-1f9.md Y3.
   Lower-entry DOMAIN as above; positive-radius Kronecker with b<=0: NOT_DETERMINED.
   Every failure preserves *value; Legendre where=p, all other where untouched. */
int adf_fball_legendre(int *value, adf_place_t *where, const adf_fball_t a, adf_place_t p);
int adf_fball_jacobi(int *value, adf_place_t *where, const adf_fball_t a, const fmpz_t b);
int adf_fball_kronecker(int *value, adf_place_t *where, const adf_fball_t a, const fmpz_t b);

/* Unit cosets: N=0 uses the exact integer c=+-1, including Kronecker b<=0.
   N>0 uses the stored modulus N and K|N, else NOT_DETERMINED. No additive hull is used.
   Positive lower entries give a sign, never 0. The supplied modulus suffices: removing its lone factor
   of 2 during normalisation does not change K|N, because K is odd or divisible by 8.
   Lower-entry DOMAIN as above; finite Kronecker b<=0: NOT_DETERMINED. Failure reports as above. */
int adf_ucoset_legendre(int *value, adf_place_t *where, const adf_ucoset_t a, adf_place_t p);
int adf_ucoset_jacobi(int *value, adf_place_t *where, const adf_ucoset_t a, const fmpz_t b);
int adf_ucoset_kronecker(int *value, adf_place_t *where, const adf_ucoset_t a, const fmpz_t b);

/* Slice B, proposed N-D18 continued. Existing types: local balls, arb real balls, exact rationals
   and ideles at a named place. Alternatives: a rational-only slice, or projecting ideles to additive
   hulls. The latter would change their sets. Value is a sign through the same int pointer.
   Set statement: the Hilbert symbol is +1 iff a x^2+b y^2=z^2 has a nonzero local solution;
   docs/proofs/catalogue.md:63-78, Propositions 5-7:82-165. Its value set is computed over all
   permitted square classes, including the scale cofactor for ideles. A singleton is OK; a pair
   of signs is NOT_DETERMINED. Alternatives: require the sufficient precision bounds for every
   input, or expose both signs in a new result object. No all-places or product-formula function.

   Common scalar-output and alias rules above apply. All failures write where when non-NULL;
   successes leave where untouched. Public canonical value inputs are checked in debug builds.
   Limits: no extra cap, no prime factorisation, no growing prime power is formed. All place primes
   below 2^64 work. The local stored v,N may be any slong: only parity and up to three unit digits
   are read. This slice deliberately has no LBALL exponent LIMIT, unlike local arithmetic.
   Cost: a bounded square-class cross product (at most 16 signs), integer residues/inverses, and
   for rationals/ideles removing the specified prime from numerator and denominator only. */

/* Local balls at their implicit prime. DOMAIN for different primes (where=smaller prime),
   or if either input is exact zero (where=common prime). NOT_DETERMINED for a ball containing
   zero, or two possible signs, at the common prime. DOMAIN dominates a zero-containing ball
   when the other input is exact zero (maximum rule, conventions 3.3).
   Odd p: excluding zero fixes the relevant unit residue. At 2: enumerate odd residues modulo 8
   consistent with the available one, two or three digits; thus a coarse ball is sometimes OK.
   Examples: (1+4 Z_2,2) is NOT_DETERMINED; (1+4 Z_2,1) is OK with +1. */
int adf_lball_hilbert(int *value, adf_place_t *where, const adf_lball_t a, const adf_lball_t b);

/* Real balls: +1 unless both numbers are negative. DOMAIN for a non-finite input (courtesy to
   raw arb users) or either exact zero; NOT_DETERMINED if either finite ball contains zero.
   Otherwise signs are fixed and the result is OK. Failure where=infinity. Exact zero takes
   precedence over a ball that contains zero. No working precision: only signs are read. */
int adf_real_hilbert(int *value, adf_place_t *where, const arb_t a, const arb_t b);

/* Exact rationals at one place: OK for two nonzero rationals, DOMAIN if either is zero.
   where=v on failure. The real formula uses rational signs directly, without arb rounding.
   At p use valuation parity and the unit part modulo p (8 at 2).
   The product formula of catalogue.md:167-180 is tested, not offered as an operation. */
int adf_rat_hilbert_at(int *value, adf_place_t *where, const adf_rat_t a,
                      const adf_rat_t b, adf_place_t v);

/* Ideles: at infinity use their real balls. At p, valuation parity comes from exact positive
   scale r and the unit includes r'=r p^(-v_p(r)) (catalogue.md:142-163). Unknown unit digits
   permit all corresponding square classes. Return OK exactly for a constant sign; otherwise
   NOT_DETERMINED with where=v. Canonical ideles exclude zero, so no DOMAIN is possible.
   At odd p with both valuations even the result is +1 without unit knowledge. At 2 unknown
   units can vary the sign. r=s=3, exact unit 1 gives -1, not the symbol of unit parts alone. */
int adf_idele_hilbert_at(int *value, adf_place_t *where, const adf_idele_t a,
                        const adf_idele_t b, adf_place_t v);
#ifdef __cplusplus
}
#endif
#endif /* ADELEFELD_SYMBOL_H */
