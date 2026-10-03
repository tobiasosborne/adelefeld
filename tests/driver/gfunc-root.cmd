#!exit 1
# gfunc-root: the root at all places (lane f-slice10, WP 1F.8; SPEC 9.3.3 lines 636-646; include/adelefeld/gfunc.h;
# docs/api-1f8.md G1 to G4).  root X with N [with SIGN]: X a rational, an adele or an idele.  Every expected line
# is derived by hand in the comment above it.
# ---- PLAN row 1F.8: 1 has the rational square roots 1 and -1; 8 has the cube root 2; 2 has no square root
root 1 with 2
root 1 with 2 with -1
root 1 with 2 with 1
root 8 with 3
root 2 with 2
# -4 with n = 2: no real root (DOMAIN); -8 with n = 3: -2; the branch -1 names nothing for odd n
root -4 with 2
root -8 with 3
root -8 with 3 with -1
# a fraction: 9/4 has the square roots 3/2 and -3/2; 27/8 the cube root 3/2; 8/9 no square root (9 is, 8 is not)
root 9/4 with 2
root 9/4 with 2 with -1
root 27/8 with 3
root 8/9 with 2
# 0 has the root 0 for either sign; n = 1 is the identity (the sign ignored); n = 0 is DOMAIN
root 0 with 5 with -1
root -5/3 with 1 with -1
root 4 with 0
# degrees above the bit length: 2^64 = 18446744073709551616 has the 64th root 2; 2^63 has none of degree 64
root 18446744073709551616 with 64
root 9223372036854775808 with 64
# a degree of 2^64 - 1 (odd): 1 and -1 are their own roots, 3 has none
root -1 with 18446744073709551615
root 3 with 18446744073709551615
# invalid operands: a degree above 2^64 - 1, a negative degree, a fraction as degree, a sign that is not +1 or -1
root 4 with 18446744073709551616
root 4 with -2
root 4 with 1/2
root 4 with 2 with 0
root 4 with 2 with 3000000000
# ---- the adele (exact real parts: 4, 9/4 and 0 are squares of dyadic numbers, arb_sqrt is exact on them)
root (4 ; 9/4) with 2
root (4 ; 9/4) with 2 with -1
root (-4 ; 4) with 2
root (4 ; 2) with 2
root (0 ; 0) with 3 with -1
root (16 ; 81) with 4
# a finite part of positive radius: NOT_DETERMINED (2 + 4 Zhat: no prime of the modulus is examined)
root (4 ; 2 mod 4) with 2
# the real ball meets both signs, the finite part is a square: NOT_DETERMINED (the real place)
root (0 +/- 1 ; 4) with 2
# ---- the idele: an exact unit follows the rational contract; a unit of finite precision is NOT_DETERMINED
root (4 ; 4 * [1]) with 2
root (4 ; 4 * [1]) with 2 with -1
root (4 ; 4 * [-1]) with 2
root (-4 ; 4 * [1]) with 2
root (4 ; 4 * [1 mod 8]) with 2
root (4 ; 4 * [1 mod 8]) with 1
root (-4 ; 4 * [1 mod 8]) with 2
# ---- types the operation does not define: a finite ball, a unit coset: DOMAIN; a wrong number of operands: PARSE
root 4 mod 8 with 2
root [1] with 2
root 4
root 4 with 2 with 1 with 1
