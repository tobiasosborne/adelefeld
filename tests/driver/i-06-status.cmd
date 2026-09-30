# The statuses of the readers and of the arithmetic of ideles and classes through the driver (lane t-slice1).
# Expected lines written by hand from conventions 9.3 (the semantic constraints), 8.5 (the order of the checks:
# grammar, limits, then DOMAIN, then the sign condition of the enclosure) and from the rule of SPEC 5 ("Sign
# preservation": a result whose sign is not certified is NOT_DETERMINED, never a ball that contains 0).
# NOT_DETERMINED: the exact interval [1 - r, 1 + r] with r = 0.99999999999 is [1e-11, 2 - 1e-11], whose end
# points are 37 or 38 binades apart, so no ball of 30 bits certifies it; at 64 bits the reader succeeds.  For the
# arithmetic: 1 +/- 0.9999999999999 read at 64 bits has ends 43 or 44 binades apart; the square of the ends is 86 or
# 87 + 2 binades apart, more than 64, so the product and the square are NOT_DETERMINED at 64 bits.
#!exit 1
show (0 ; 1 * [1])
show (1 ; 0 * [1])
show (1 ; 1/0 * [1])
show (1 ; 1 * [2 mod 4])
show (1 +/- 1 ; 1 * [1])
show [2 mod 4]
show [5 mod 0]
show [0]
show [5 mod -6]
show (1 ; -1 * [1])
show (1e100001 ; 1 * [1])
show (1e100001 ; 1 [1])
show [5 mod 6
show (0 +/- 1e100001 ; 0 * [2 mod 4])
show <0 +/- 1 ; [1]>
show <1 ; [1]>>
prec 30
show (1 +/- 0.99999999999 ; 1 * [1])
show <1 +/- 0.99999999999 ; [1]>
show (-1 +/- 0.99999999999 ; 1 * [1])
show (1 +/- 0.99999999999 ; 1 * [2 mod 4])
prec 64
show (1 ; 1 * [1])
mul (1 +/- 0.9999999999999 ; 1 * [1]) with (1 +/- 0.9999999999999 ; 1 * [1])
mul <1 +/- 0.9999999999999 ; [1]> with <1 +/- 0.9999999999999 ; [1]>
pow (1 +/- 0.9999999999999 ; 1 * [1]) with 2
pow <1 +/- 0.9999999999999 ; [1]> with 2
pow (1 +/- 0.9999999999999 ; 1 * [1]) with 0
prec 100001
