# The settings prec <bits> and digits <n>. Expected lines written from conventions 9.5.
#!exit 0
# the default of the driver: prec 64 bits, digits = ADF_DIGITS_DEFAULT = 20
show (1.625 ; 1)
# prec: the midpoint of a real ball is stored with prec bits and the rounding error is
# added to the radius (conventions 9.5, "Reading").  1.625 needs 4 bits, so at prec 2
# the midpoint is 1.5 with the error 1/8: the text of 9.5 is then 1.5 +/- 0.13.
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
prec 64
digits 20
show (1.625 ; 1)
