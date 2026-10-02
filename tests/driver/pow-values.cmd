# SPEC 9.3.4 items 2 and 3 (lane f-slice9, 1F.6): rational powers through the root branches, powers of principal
# units. Every expected line is computed by hand or by integer enumeration in the comment above it.
prec 20
# 9^(3/2) at 5: the roots 3 (identifier 3) and -3 (identifier 2) cubed, exactly; 6/4 is reduced to 3/2 first
powrat_at 9 with 5 with 3/2 with 3
powrat_at 9 with 5 with 3/2 with 2
powrat_at 9 with 5 with 6/4 with 2
# 9^(-3/2) on the branch 3: 1/27
powrat_at 9 with 5 with -3/2 with 3
# at 2 the identifier is the sign: the root 3 is -1 (3 mod 4), the root -3 is +1
powrat_at 9 with 2 with 3/2 with -1
powrat_at 9 with 2 with 3/2 with 1
# 8^(2/3) at 5: one cube root, 2 (identifier 2): 4
powrat_at 8 with 5 with 2/3 with 2
# 0^(5/2) = 0 (identifier 0); an integer exponent is the integer power (seed ignored); e = 0 gives 1
powrat_at 0 with 7 with 5/2 with 0
powrat_at 7 with 7 with 3 with 5
powrat_at 2 with 7 with 0 with 1
# a prime of 64 bits: 4^(1/2) on the branch 2
powrat_at 4 with 18446744073709551557 with 1/2 with 2
# a ball: 9 + 5^6 Z_5, n' = 2, e' = 3: E' = 0 + 6 - 0 + 0 = 6; the root of the centre is 3, 3^3 = 27
prec 3
powrat_at (* ; 9 mod 15625) with 5 with 3/2 with 3
prec 10
powrat_at (* ; 9 mod 15625) with 5 with 3/2 with 3
# 6^(1/2) at 5 to 5^10: the x = 1 mod 5 with x^2 = 6 mod 5^10, by enumeration: 6520516
powunit_at 6 with 5 with 1/2
# 6^(2 + 25 Z_5): alpha = v(6 - 1) = 1, R = B + alpha = 3; 6^2 = 36 mod 125
powunit_at 6 with 5 with (* ; 2 mod 25)
# (6 + 25 Z_5)^2: R = A + beta = 2 + 0; 36 = 11 mod 25
powunit_at (* ; 6 mod 25) with 5 with 2
# the exact 1: u = 1, or s = 0
powunit_at 1 with 5 with (* ; 3 mod 5)
powunit_at 7 with 3 with 0
# at 2: (-1)^3 = -1 exactly; (-1)^(Z_2) has the values 1 and -1: the hull 1 + 2 Z_2
powunit_at -1 with 2 with 3
powunit_at -1 with 2 with (* ; 0 mod 1)
# 3^(1/3): 1/3 is odd, so the sign -1 of 3 enters: the cube root of 3 in Z_2, x^3 = 3 mod 2^10: 379
powunit_at 3 with 2 with 1/3
# (3 + 4 Z_2)^(1 + 2 Z_2): u0' = -3, alpha = v(-4) = 2, beta = 0, R = min(2 + 0, 1 + 2, 2 + 1) = 2; centre 3
powunit_at (* ; 3 mod 4) with 2 with (* ; 1 mod 2)
# (3 + 4 Z_2)^(Z_2): both parities, sign -1: the hull 1 + 2 Z_2
powunit_at (* ; 3 mod 4) with 2 with (* ; 0 mod 1)
# (1 + 2 Z_2)^1: both signs: the hull; (1 + 2 Z_2)^2: even, R = 2 + v(2) = 3
powunit_at (* ; 1 mod 2) with 2 with 1
powunit_at (* ; 1 mod 2) with 2 with 2
