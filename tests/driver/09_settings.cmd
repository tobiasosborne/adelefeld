# The settings prec <bits> and digits <n>. Expected lines written from conventions 9.5.
#!exit 1
# the default of the driver: prec 64 bits, digits = ADF_DIGITS_DEFAULT = 20
show (1.625 ; 1)
# prec: the midpoint of a real ball is stored with prec bits and the rounding error is
# added to the radius (conventions 9.5, "Reading").  1.625 needs 4 bits, so at prec 2
# the midpoint is 1.5 with the error 1/8: the text of 9.5 is then 1.5 +/- 0.13.
# The line below is one of the two lines of this file that depend on FLINT 3.0.1.  9.5 asks
# that the printed interval contain [mid - rad, mid + rad] and that the arb be exact when the
# odd mantissa has at most prec bits; 1.625 at prec 2 is contained in the ball 1.5 +/- 1/8
# and also in the ball 1.25 +/- 3/8, and both texts conform.  The driver gives the first
# because FLINT rounds to nearest.  The test keeps the exact line; see the README of tools/adf.
prec 2
show (1.625 ; 1)
prec 64
show (1.625 ; 1)
# digits: the number of significant digits of the printer of 9.5.  With n = 1 the
# midpoint 1.5 is rounded to the multiple 2 of 10^0 and the radius becomes 0.5.
digits 1
show (1.5 ; 1)
digits 20
show (1.5 ; 1)
# both settings at once: the midpoint 1.5 and the radius 1/8, printed with n = 1
prec 2
digits 1
show (1.625 ; 1)
# The second line that depends on FLINT 3.0.1: with n = 1 the printer of 9.5 rounds the
# midpoint 1.5 to the multiple 2 of 10^0 and the radius 0.125 becomes 0.5, and with the
# radius 0.625 (the rounded midpoint 2 minus the exact 1.375) it becomes 0.63.  9.5 asks for
# an interval that contains the ball, and 2 +/- 0.75 would contain it as well.
prec 64
digits 20
show (1.625 ; 1)
# The range of the settings.  prec is 1 to ADF_PRINT_EXP_MAX = 100000: above that an inexact
# real result of magnitude 1 or more has a radius with a binary exponent below
# -ADF_PRINT_EXP_MAX and no printer can convert it (M1-D6), so the driver answers LIMIT for a
# larger setting, and DOMAIN for a setting below 1.  digits is 1 to ADF_DIGITS_MAX = 1000000
# (include/adelefeld/text.h:39), a domain of the printer, so a value outside it is DOMAIN.
# A setting writes no line.
prec 100000
show (1.625 ; 1)
prec 100001
digits 1000001
prec 64
digits 20
show (1.625 ; 1)
