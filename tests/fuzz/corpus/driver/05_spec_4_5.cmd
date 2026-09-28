# SPEC 4.5: what the additive types cannot do. Expected lines written from 4.5.
#!exit 1
# "division by an adf_adele is not defined"
div (* ; 3 mod 12) with (1 ; 1)
div (1 ; 1) with (2 ; 3 mod 5)
# "Every finite ball with positive radius contains non-invertible elements ... So a finite
#  ball can never certify that a divisor is invertible": dividing by a finite ball is
#  therefore not defined either, for any of the four types
div (* ; 3 mod 12) with (* ; 3 mod 12)
div (1 ; 1) with (* ; 1 mod 2)
div ((1) + (2)*i ; 5 mod 18) with (* ; 1 mod 2)
# "One divides by an exact non-zero rational"
div 1/2 with 2
div (* ; 5 mod 18) with 1/3
div (1 ; 1) with 2
div ((1) + (2)*i ; 5 mod 18) with 2
# a divisor that is the exact zero is proved not invertible (conventions 3.1, NOT_UNIT)
div 1/2 with 0
div (* ; 3 mod 12) with 0
div (1 ; 1) with 0
div ((1) + (2)*i ; 5 mod 18) with 0
