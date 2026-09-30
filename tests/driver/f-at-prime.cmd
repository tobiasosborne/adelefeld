# f-at-prime: exp_at and log_at at a prime, through adf_sball_exp_at and adf_sball_log_at (include/adelefeld/
# rfunc.h, "A PRIME"; SPEC 9.3.1, 9.3.2).  At a prime the setting prec is the requested ABSOLUTE precision N of
# adelefeld/lfunc.h, and the line is the partial ball over the one place: "<p>: <centre> + O(<p>^<N>)" with the
# canonical centre in [0, p^N), "<p>: <value>" for an exact result, "real: <ball>" at the real place
# (tools/adf/README.md, "The commands at places").
#
# Every expected line of f-at-prime.out is computed by lanes/f-slice6/expected.py from the series of
# docs/proofs/functions.md Definition 1 with exact Fractions, and none is taken from the output of the program:
#   exp(x) = sum x^k/k!, on x in p^c Z_p (c = 1 odd p, 2 for p = 2), reduced modulo p^N;
#   log(x) = sum (-1)^(k+1) (x-1)^k/k, on x in 1 + p Z_p.
# The checks that can be made by hand are given at each line.
#!exit 1
prec 8
# exp(5) at 5, N = 8, 5^8 = 390625: the centre is 349831 (349831 = 6 modulo 25: exp(5) = 1 + 5 + 25/2 + ... = 6
# modulo 25, and 349831 = 13993 * 25 + 6).  The input is exact, so the exponent is N = 8.
exp_at 5 with 5
# log(6) at 5, N = 8: 6 = 1 + 5, log(6) = 5 - 25/2 + 125/3 - ... = 5 modulo 25, so the valuation is 1 and the
# centre 329930 = 13197 * 25 + 5 is 5 modulo 25.
log_at 6 with 5
# log(-1) = 0 at p = 2 (functions.md Proposition 11): the exact 0, and N is not used.
log_at -1 with 2
# exp(1) at 5: v_5(1) = 0 < 1, 1 is outside 5 Z_5: DOMAIN.
exp_at 1 with 5
# exp(0) is the exact 1 (Definition 1: at zero the constant terms).
exp_at 0 with 5
# exp(4) at 2: v_2(4) = 2 >= c = 2, so 4 is in 4 Z_2.  By hand: 1 + 4 + 8 = 13 modulo 16 (the terms 32/3, 32/3,
# 128/15, ... have valuation >= 5), and the centre 77 is 13 modulo 16.  exp(2): v_2(2) = 1 < 2: DOMAIN (exp needs
# 4 Z_2).
exp_at 4 with 2
exp_at 2 with 2
# log(3) at 2: 3 = 1 + 2, v_2(3 - 1) = 1 >= 1, the value 2 - 2 + 8/3 - 4 + ... has valuation 2: centre 244 = 4 * 61.
log_at 3 with 2
# log(10) at 3: 10 = 1 + 9, log(10) = 9 - 81/2 + 243 - ... , valuation 2.  log(1/2) at 3: 1/2 - 1 = -1/2, a
# 3-adic unit: DOMAIN.  exp(1/5) at 5: v_5(1/5) = -1: DOMAIN.
log_at 10 with 3
log_at 1/2 with 3
exp_at 1/5 with 5
# a rational with a denominator prime to p: exp(20/3) at 5 (v_5(20/3) = 1) and log(26/5) at 5 (26/5 - 1 = 21/5 has
# v_5 = -1: DOMAIN).
exp_at 20/3 with 5
log_at 26/5 with 5
# log(1) = 0 exactly, at 7.
log_at 1 with 7
# a prime of 64 bits, 2^64 - 59 (adf_place_prime certifies it): log(1) = 0 exactly at it.
log_at 1 with 18446744073709551557
# the setting prec is N: at 3, 1 and 20 digits of the same value exp(5) at 5
prec 3
exp_at 5 with 5
prec 1
exp_at 5 with 5
prec 20
exp_at 5 with 5
# a finite ball (* ; 5 mod 25) is 5 + 25 Zhat: at 5 it is 5 + 5^2 Z_5, and exp of it is exp(5) + 5^2 Z_5 (exponent
# min(N, 2) = 2, functions.md Proposition 10: exp is an isometry on 5 Z_5): 6 + O(5^2).  At 3 and at 2 the ball is
# Z_3, Z_2 (v(25) = 0), which meets the domain and its complement: NOT_DETERMINED.  A finite ball has no real place.
prec 8
exp_at (* ; 5 mod 25) with 5
exp_at (* ; 5 mod 25) with 3
exp_at (* ; 5 mod 25) with 2
exp_at (* ; 5 mod 25) with real
# log of (* ; 6 mod 25) = 6 + 5^2 Z_5 at 5: inside 1 + 5 Z_5, the exponent is 2, log(6) = 5 modulo 25: 5 + O(5^2).
log_at (* ; 6 mod 25) with 5
# an adele: the real coordinate is exact 0 (exp = the exact 1) and the finite coordinate is the exact 5
exp_at (0 ; 5) with 5
exp_at (0 ; 5) with real
# the real place: prec is the working precision in bits (64), digits 6.  exp(1) = 2.718281828459045...: with 6 digits
# the ball prints as 2.71828 +/- 1.9e-6 and log(2) = 0.693147180559945... as 0.693147 +/- 1.9e-7 (conventions 9.5,
# proto/text_grammar.py print_real on the true value; the radius of the arb ball, near 2^-62, does not change the
# text: lanes/f-slice6/expected.py runs it with the radii 2^-40 and 2^-70).  log(-1) at the real place: DOMAIN.
prec 64
digits 6
exp_at 1 with real
log_at 2 with real
log_at -1 with real
exp_at (1 ; 5 mod 25) with real
