# The maps between ideles, classes and adeles, and the valuations, through the driver (lane t-slice1).  Expected
# lines written by hand from docs/SPEC.md 5 (the class of an idele is t = |x_inf| / r, u' = sign(x_inf) u; idele to
# adele: the smallest ball r c' + r lcm(N, 2) Zhat with c' odd, the simple ball r c' + r N' Zhat of the normal
# form, and for an exact unit the exact rational r e; the division of an adele by an idele; the norm |x_inf| / r;
# |x_p|_p = p^(-v_p(r))), conventions 5.7 (the formulas of the hulls and of the division are quoted there) and
# docs/proofs/ideles.md P14 to P19; checked by lanes/t-slice1/check_driver_cases.py with exact integers.
#!exit 1
# ---- class: (|x_inf| / r, sign(x_inf) u)
class (2 ; 4 * [5 mod 36])
class (-2 ; 4 * [5 mod 36])
class (3 ; 3 * [1 mod 0])
class (-3 ; 3 * [-1])
class (2 +/- 0.125 ; 1 * [1])
class <1 ; [1]>
class 3/2
# ---- norm
norm (2 ; 4 * [1])
norm (-3 ; 3/2 * [-1])
norm (2 +/- 0.125 ; 1 * [1])
norm 3/2
# ---- hull and hullsimple
hull (2 ; 3 * [5 mod 36])
hull (2 ; 3 * [2 mod 3])
hullsimple (2 ; 3 * [2 mod 3])
hullsimple (2 ; 3 * [5 mod 36])
hull (2 ; 3 * [-1 mod 0])
hullsimple (2 ; 3 * [-1])
hull (-1.5 +/- 0.125 ; 1/2 * [1 mod 1])
hullsimple (-1.5 +/- 0.125 ; 1/2 * [1 mod 1])
hull <1 ; [1]>
hull (1 ; 1)
# ---- unitof: an adele to an idele, when the adele certifies it
unitof (2 ; 3/2)
unitof (-2 ; -3/2)
unitof (2 ; 3 mod 5)
unitof (2 ; 0)
unitof (0 +/- 1 ; 1)
unitof (0 ; 1)
unitof (0 ; 3 mod 5)
unitof 3/2
# ---- the division of an adele by an idele
div (3 ; 6 mod 9) with (2 ; 3 * [5 mod 36])
div (3 ; 6 mod 9) with (2 ; 3 * [1 mod 0])
div (3 ; 6 mod 9) with (-2 ; 3 * [-1])
div 3/2 with (2 ; 1 * [1])
div (* ; 1) with (2 ; 1 * [1])
div (3 ; 6 mod 9) with (2 ; 3 * [5 mod 36]) with 2
# ---- valuation and abs at a place
valuation (2 ; 3/2 * [1]) with 3
valuation (2 ; 3/2 * [1]) with 2
valuation (2 ; 3/2 * [1]) with 5
valuation (2 ; 3/2 * [1]) with real
valuation (2 ; 3/2 * [1]) with 4
valuation (2 ; 3/2 * [1]) with 3 5
valuation 3/2 with 3
abs (2 ; 3/2 * [1]) with 3
abs (2 ; 3/2 * [1]) with 2
abs (2 ; 3/2 * [1]) with 5
abs (-2 +/- 0.125 ; 3/2 * [1]) with real
abs (2 ; 3/2 * [1]) with 1
abs <2 ; [1]> with 3
abs [1] with real
