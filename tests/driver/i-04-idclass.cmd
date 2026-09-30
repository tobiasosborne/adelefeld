# Operations on idele classes through the driver (lane t-slice1).  Expected lines written by hand from docs/SPEC.md 5
# (the class group of Q is R_{>0} x Zhat^x: the product and the inverse are componentwise on the pair (t, u'), the
# real part t is a positive ball) and conventions 5.7; the real parts are exact dyadic numbers; checked by
# lanes/t-slice1/check_driver_cases.py.  A class has no negation: the class of -x is the class of x (the class of
# the idele -1 of Q^x is trivial), and -u' is not the negative of a class but another class, so neg is DOMAIN.
#!exit 1
mul <2 ; [5 mod 36]> with <4 ; [7 mod 12]>
mul <2 ; [1]> with <0.5 ; [-1]>
inv <4 ; [5 mod 36]>
inv <0.5 ; [-1]>
div <8 ; [5 mod 36]> with <2 ; [5 mod 36]>
pow <2 ; [5 mod 36]> with 2
pow <2 ; [5 mod 36]> with -1
pow <2 ; [5 mod 36]> with 0
pow <2 ; [5 mod 36]> with 3
powtight <2 ; [2 mod 5]> with 2
norm <2 ; [1]>
norm <0.5 +/- 0.25 ; [1]>
neg <1 ; [1]>
mul <1 ; [1]> with 2
mul <1 ; [1]> with [1]
equal <1 ; [1]> with <1 ; [1]>
add <1 ; [1]> with <1 ; [1]>
pow <2 ; [5 mod 36]> with
show <0 ; [1]>
show <-1 ; [1]>
show <1 +/- 1 ; [1]>
show <1 ; [2 mod 4]>
show <1e100001 ; [1]>
show <1 ; 1 * [1]>
dump <1 ; [1]>
