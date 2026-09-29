# s-realroots: the real roots of an integer polynomial, through adf_roots_real
# (include/adelefeld/roots.h, slice 3 of S.2; SPEC 9.1, the table "Real roots").  The polynomial
# is written as its integer coefficients in decimal, separated by single spaces, the constant
# term first.  The output is the balls in the order of the list, separated by "; ", each the
# real-ball text of conventions 9.5 with the setting digits, and the word "none" for an empty
# list.  A ball is printed as the driver prints a real part: the real coordinate of an adele
# printed by adf_adele_get_str (include/adelefeld/text.h:160), the text between the "(" and the
# " ; " (see tools/adf/README.md).
#
# The setting digits 1 is set in the first command of the script, and every expected line below
# is derived from the accuracy contract of the balls and from conventions 9.5 alone, so that no
# line depends on which isolating ball FLINT returns.  The two facts used are
#   (1) the balls of the list have arb_rel_accuracy_bits >= max(prec, 2) = 64 (include/adelefeld/
#       roots.h, decision S-D19), that is rad < |mid| 2^-64 < 1.5e-19 for a root of size about 1:
#       arb_rel_error_bits is the difference between the position of the top bit of the radius and
#       the top bit of the midpoint, plus one (refs/src/flint-3.0.1/arb.rst:500-504), so an
#       accuracy of 64 bits gives a radius below |mid| 2^-64;
#   (2) each ball holds one root of g, so its midpoint is within rad of that root.
#!exit 1
digits 1
# X^2 - 2.  g* = X^2 - 2 has the two real roots -sqrt(2) and sqrt(2), and the list is in
# increasing order.  For the positive root: |mid| is within 1.5e-19 of 1.4142135623..., so
# X(mid) = 0.  Step 3 of conventions 9.5 with n = 1: q = X(mid) - 1 + 1 = 0, and
# q = max(q, X(rad) - 1) = max(0, -21) = 0.  Step 4: M = round(mid, 0) = 1, and
# E = rad + |M - mid| is in (0.41421, 0.41422), so R = ceil2(E) = 0.42.  Step 5 does not move q,
# and the text is "1 +/- 0.42".  The ball of the negative root is its negative, so M = -1, E is in
# the same interval and the text is "-1 +/- 0.42".  The two are printed in the order of the list.
realroots -2 0 1
# X^2 + 1 has no real root, so the list is empty and the answer is none.
realroots 1 0 1
# (X^2 - 2)^2 = X^4 - 4 X^2 + 4.  g* = X^2 - 2, the list holds the distinct real roots and a
# multiplicity is not reported (SPEC 9.1), so the two balls are those of X^2 - 2 above.
realroots 4 0 -4 0 1
# X^4 + 1 is positive for every real x, so the list is empty: none.
realroots 1 0 0 0 1
# f = 0 is outside the domain of adf_roots_real (roots.h, "Statuses"), so the answer is
# error: DOMAIN.
realroots 0
# A polynomial that is not a list of decimal integers separated by single spaces is not a
# sentence of the operand form: error: PARSE.  So is an operand of two spaces between two
# coefficients, and a second operand, of which the command takes none.
realroots 1 2 x
realroots 1  0 1
realroots -2 0 1 with 7
# The setting digits 20, the default of the driver (include/adelefeld/text.h:39,
# ADF_DIGITS_DEFAULT).  The line below is the one line of this case that depends on which
# isolating ball FLINT returns, and it is checked as follows.  The two balls that
# adf_roots_real returns at prec 64 for X^2 - 2 are, exactly (lanes/drv-s/real_probe.c writes
# them, arb_get_interval_fmpz_2exp of the ball and of its midpoint and radius):
#   the ball of -sqrt(2): midpoint -240615969168004511545033772477625056927 2^-127, radius
#                         268435457 2^-155, that is 154 bits of relative accuracy
#   the ball of  sqrt(2): midpoint  240615969168004511545033772477625056927 2^-127, radius
#                         1 2^-127, that is 126 bits
# where the accuracy is the difference between the position of the top bit of the radius and of
# the midpoint, plus one, as arb_rel_error_bits defines it
# (refs/src/flint-3.0.1/arb.rst:500 to 504); both are above the 64 bits that S-D19 requires.
# lanes/drv-s/real_check.py applies the algorithm of conventions 9.5 to those two pairs, in
# exact rational arithmetic and written from the algorithm and not from the program, and gets the
# text of the line: step 3 gives q = X(mid) - 19 = -20, M = round(mid, -20) is
# 14142135623730950488 10^-19, the 20-digit rounding of the root, |M - mid| is 1.68872...e-21 and
# rad is below 2^-126, so R = ceil2(rad + |M - mid|) = 1.7e-21, which fmt writes with an
# exponent because X(R) = -21 < -4.  The script checks three more things: the printed interval
# contains the exact ball, the exact end points of the ball give opposite signs of X^2 - 2 (the
# test of solvers P3.8) and the ball is on one side of 0 with 2 strictly between the squares of
# its end points.  So every property of the line that the specification fixes is derived, and
# the ball is the only thing taken from FLINT.  As the two lines of tests/driver/09_settings.cmd,
# this line depends on FLINT 3.0.1.
digits 20
realroots -2 0 1
# The guard on printing a real ball reaches this command.  At the setting prec 100000 the accuracy
# S-D19 asks for is 100000 bits, and the balls that FLINT returns for X^2 - 2 have then the radius
# 1 2^-131071, whose binary exponent in the form of FLINT (x = m 2^e, 0.5 <= |m| < 1,
# refs/src/flint-3.0.1/arf.rst:14 to 20) is -131070, above ADF_PRINT_EXP_MAX in absolute value.
# A printer of the library that returns NULL is ADF_LIMIT for the driver (decision M1-D6,
# include/adelefeld/text.h:32 to 38), and no byte of the line is written before the error, so the
# answer is one line.  The radius is the one FLINT 3.0.1 gives: the accuracy contract alone asks
# for more than 100000 bits, and a ball of exactly 100001 bits would have an exponent of -100000,
# which the guard admits.  This line depends on FLINT 3.0.1, like the line above; the guard it
# tests is the rule of the library and no bound of the driver.
prec 100000
realroots -2 0 1
