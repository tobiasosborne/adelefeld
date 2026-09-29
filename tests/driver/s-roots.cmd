# s-roots: the roots of an integer polynomial at a prime, through adf_roots_padic
# (include/adelefeld/roots.h, slice 2 of S.2; SPEC 9.1, the table "Polynomial roots at a given
# prime").  The polynomial is written as its integer coefficients in decimal, separated by
# single spaces, the constant term first: "-2 0 1" is X^2 - 2.  The output is the roots in the
# order of the list, separated by "; ", each as "A mod P^K" with A the centre in [0, P^K) and
# K the precision of its certificate, and the word "none" for an empty complete list.
#
# Every expected line of s-roots.out is derived here or in lanes/drv-s/expected_roots.py, which
# computes the centres by the digit rule a + t p^(k-1) with (a + t p^(k-1))^2 = c mod p^k and
# no other.  Nothing is taken from the output of the program.
#!exit 1
# X^2 - 2 in Z_7 at prec_p = 5.  The squares modulo 7 are 0, 1, 2 and 4, and 2 = 3^2 = 4^2 mod 7,
# so there are two roots modulo 7 and both are simple, so both lift to Z_7.  7^5 = 16807.  The
# digit rule from a = 3: (3 + 7 t)^2 = 2 mod 49 gives 7 + 42 t = 0 mod 49, that is t = 1, so 10;
# from 10 to mod 343: (10 + 49 t)^2 = 2 mod 343, and so on, which gives the two centres 4567 and
# 12240.  The check: 4567^2 - 2 = 16807 * 1241 and 12240^2 - 2 = 16807 * 8914, both centres are
# 3 and 4 mod 7, and 4567 + 12240 = 16807.  For each, s = v_7(2 alpha) = 0, so K = max(5, 1) = 5.
roots -2 0 1 with 7 with 5
# X^2 - 2 in Z_5 at prec_p = 5.  The squares modulo 5 are 0, 1 and 4; 2 is not one of them, so
# no root of the polynomial is in Z_5, the list is complete and empty, and the answer is none.
roots -2 0 1 with 5 with 5
# 27 X at 3.  g* is X, since 27 is removed and the leading coefficient is made positive.  The
# only root is 0 with s = v_3(g'(0)) = v_3(1) = 0, so K = max(5, 1) = 5 and the centre is 0.
roots 0 27 with 3 with 5
# (X - 1)(X + 2) = X^2 + X - 2 in Z_7 at prec_p = 5.  The roots are 1 and -2 = 5 mod 7.  For
# both s = v_7(2 alpha + 1) = v_7(3) = v_7(-3) = 0, so K = 5.  The centre of the root 1 is 1; the
# centre of the root -2 is the lift of 5 mod 7 to 7^5, and -2 = 16807 - 2 = 16805 is already in
# [0, 16807) and is the root, so the centre is 16805.
roots -2 1 1 with 7 with 5
# X^2 - 1 in Z_2 at prec_p = 5, so 2^5 = 32.  The two roots are 1 and -1, and they are both
# congruent to 1 modulo 2, so the one root of the polynomial modulo 2 gives two balls in Z_2.
# For both s = v_2(g'(alpha)) = v_2(2 alpha) = 1, so K = max(5, 2) = 5.  The centres are 1 and
# -1 = 31 in [0, 32), in increasing order; the check is 31^2 - 1 = 960 = 30 * 32.
roots -1 0 1 with 2 with 5
# (X - 1)(X + 2)^2 = X^3 + 3 X^2 - 4 in Z_7 at prec_p = 5.  f is not squarefree; the list is the
# list of the roots of g* = (X - 1)(X + 2) (decision S-D13, include/adelefeld/roots.h), and a
# multiplicity is never reported (SPEC 9.1).  So the answer is the same two centres as above.
roots -4 0 3 1 with 7 with 5
# A prime of 64 bits: 18446744073709551557 = 2^64 - 59, which adf_place_prime certifies with
# n_is_prime (include/adelefeld/place.h:34).  (X - 3)(X - 5)(X - 7) = X^3 - 15 X^2 + 71 X - 105
# is squarefree and has the planted integer roots 3, 5 and 7.  g' = 3 X^2 - 30 X + 71 has the
# values -8, -4 and 20 at them, none of which the prime divides, so s = 0 for all three, K is
# max(3, 1) = 3 and the centre is the root itself, the prime being larger than 7.  Above
# ADF_ROOTS_P_EVAL_MAX the roots modulo p are found by the degree of gcd(h, X^p - X), which is
# what this case exercises; p = 7 and p = 3 of the other cases are below it and use the
# evaluation at every residue.
roots -105 71 -15 1 with 18446744073709551557 with 3
# A constant polynomial: g* is 1 (roots.h, the normalised polynomial), which has no root in any
# place, so the complete list is empty and the answer is none.
roots 5 with 7 with 5
# f = 0 is outside the domain of adf_roots_padic (roots.h, "Statuses", edit E-C1), and the driver
# decides it when it reads the polynomial: error: DOMAIN.
roots 0 with 7 with 5
