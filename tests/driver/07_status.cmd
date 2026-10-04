# Every status the library can return through the driver, one command per status.
# Expected lines written from docs/conventions.md 3.1 and the headers.
#
# Eight of the eleven codes are reachable: ADF_OK, ADF_PARSE, ADF_LIMIT, ADF_NOT_UNIT,
# ADF_NO_SOLUTION, ADF_NOT_UNIQUE, ADF_DOMAIN and ADF_UNSUPPORTED.  The other three are
# not, and no command of this file can make them appear:
#   ADF_NOT_DETERMINED       stage 7 of conventions 8.5 needs a sign condition of 9.3, and
#                            the only such types are the idele, the idele class and the A
#                            of a term; adele.h line 144 says of adf_adele_set_str that
#                            "adf_adele has no sign condition, so ADF_NOT_DETERMINED does
#                            not occur";
#   ADF_UNIT_NOT_CERTIFIED   belongs to the inversion of an adele-like value, which SPEC
#                            4.5 does not offer and the driver does not implement;
#   ADF_NEEDS_SPLIT          belongs to the quotient by Q and to the functions at places
#                            (SPEC 6, 9.3), which are work packages 1.9 and later.
# ADF_LIMIT is reached three times: the line limit of the driver (the hostile case h4), the
# decimal exponent over max_exp10, the binary exponent of a real ball over
# ADF_PRINT_EXP_MAX (the guard of tools/adf/README.md, M1-D6), and a setting above the limit
# the driver puts on it (prec over ADF_PRINT_EXP_MAX; a resource limit, conventions 3.1).
#!exit 1
# ADF_OK: the output holds the result
show 7/3
# ADF_PARSE: the text is not a sentence of the grammar (9.2), or the line is not a
# sentence of the grammar of the driver
show @@@
type @@@
frobnicate 1
with 1
# a wrong number of operands is a line that is not a sentence of the grammar
add 1/2
add 1/2 with 1/2 with 1/2
show 1/2 with 1/2
neg 1/2 with 1/2
div 1/2
cap (* ; 1 mod 2)
equal 1/2
# a setting whose argument is not a decimal integer
prec
prec x
digits 1e3
# ADF_LIMIT: a resource limit was reached.  Here: the decimal exponent over max_exp10
# (conventions 8.4), and the binary exponent of a real ball over the limit of the driver
# (README of tools/adf; the guard against the printer finding of lane m1-text).
show (1e100001 ; 0)
show (1e-100001 ; 0)
show (1e100000 ; 0)
# the largest prec the driver accepts is ADF_PRINT_EXP_MAX (100000): an inexact real result
# rounded at prec p has a radius with the binary exponent -p, so above that no inexact real
# result of magnitude 1 or more can be printed
prec 1000001
# ADF_NOT_UNIT: proved: the value is not invertible
div 1/2 with 0
# ADF_NO_SOLUTION: proved: no value satisfies the problem
reconstruct (0 ; 3 mod 12)
# ADF_NOT_UNIQUE: several candidates satisfy the problem
reconstruct (* ; 1 mod 2) with 0 with 10
# ADF_DOMAIN: proved outside the domain
show 1/0
show (1 ; 1/0)
add (* ; 1 mod 2) with (1 ; 1)
cap (* ; 1 mod 2) with 0
# a setting below its range: the data violate the domain of the setting (conventions 3.1)
prec 0
prec -1
digits 0
digits -1
digits 1000001
# ADF_UNSUPPORTED: a valid request that version 1 does not implement
show rfun()
reconstruct ((1) + (2)*i ; 0)
cap (1 ; 1) with 2
