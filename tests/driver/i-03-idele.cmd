# Operations on ideles through the driver (lane t-slice1).  Expected lines written by hand from docs/SPEC.md 5
# (componentwise product and inverse; the content r is exact, the unit multiplies as a unit coset, the real part
# is a ball; the idele of a rational q has content |q| and the exact unit sign(q)), conventions 5.7 and 9.4.
# Every real part below is exact (a dyadic number that the arithmetic at 64 bits keeps), so the text of 9.5 is
# exact; the two lines at digits 3 print 1/3 and -2/7 to three digits, where the tiny radius of the ball does not
# change the two printed digits of the radius; checked by lanes/t-slice1/check_driver_cases.py.
#!exit 1
# ---- mul of two ideles
mul (2 ; 3 * [5 mod 36]) with (4 ; 1/2 * [7 mod 12])
mul (-2 ; 1 * [1]) with (4 ; 1 * [1])
mul (-2 ; 1 * [1]) with (-4 ; 1 * [-1 mod 0])
mul (1 ; 5 * [1 mod 0]) with (1 ; 1/5 * [-1])
# ---- mul and div by an exact rational
mul (2 ; 3 * [5 mod 36]) with 3/4
mul 3/4 with (2 ; 3 * [5 mod 36])
mul (2 ; 3 * [5 mod 36]) with -3/4
mul (2 ; 3 * [5 mod 36]) with 0
div (3 ; 1 * [1]) with 3
div (3 ; 1 * [1]) with -3/2
div (3 ; 1 * [1]) with 0
# ---- inv and div
inv (4 ; 3/2 * [5 mod 36])
inv (-0.5 ; 1 * [-1])
div (8 ; 3 * [5 mod 36]) with (2 ; 3/2 * [5 mod 36])
# ---- neg: every coordinate is negated
neg (2 ; 3 * [5 mod 36])
neg (-2 ; 3 * [-1])
neg (2.5 +/- 0.125 ; 1 * [1])
# ---- pow and powtight
pow (2 ; 3 * [5 mod 36]) with 3
pow (2 ; 3 * [5 mod 36]) with -2
pow (-2 ; 3 * [5 mod 36]) with 3
pow (-2 ; 3 * [5 mod 36]) with 2
pow (2 ; 3 * [5 mod 36]) with 0
powtight (2 ; 3 * [2 mod 5]) with 2
# ---- the idele of a rational
idele 3/2
idele -3/2
idele 8
idele 0
digits 3
idele 1/3
idele -2/7
digits 20
# ---- what is not offered or not defined
equal (1 ; 1 * [1]) with (1 ; 1 * [1])
add (1 ; 1 * [1]) with (1 ; 1 * [1])
mul (1 ; 1 * [1]) with <1 ; [1]>
mul (1 ; 1 * [1]) with [1]
mul (1 ; 1 * [1]) with (1 ; 1)
inv 0
inv 3/4
inv (1 ; 1)
pow 3/4 with 2
dump (1 ; 1 * [1])
