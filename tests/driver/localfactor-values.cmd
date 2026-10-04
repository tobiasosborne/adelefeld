#!exit 0
# 1F.9 local zeta factor (lane f-slice14): SPEC 9.3.7 row "local zeta factor", design local-zeta.md Z1, Z2.
# Expected lines written from the formulas before the run (mpmath at 90 digits for the decimals): at digits 5
# the printed radius is the rounding error of the midpoint (conventions 9.5 steps 3-4), not the ball's radius.
# 1/(1 - 2^-2) = 4/3; 1/(1 - 2^(-1-i)); 1/(1 - 3^(1-i)); 7/6; pi^(-1) = 1/pi; pi^(-2); pi^(1/2) Gamma(-1/2) = -2 pi;
# 1/(2 pi); pi^(-(1+i)/2) Gamma((1+i)/2); pi^((3-i)/2) Gamma((-3+i)/2); p/(p-1) at p = 2^64 - 59.
# The finite coordinate of the carrier is ignored: the last line is the first one again.
prec 128
digits 5
local_zeta_factor_at ((2) + (0)*i ; 0) with 2
local_zeta_factor_at ((1) + (1)*i ; 0) with 2
local_zeta_factor_at ((-1) + (1)*i ; 0) with 3
local_zeta_factor_at ((1) + (0)*i ; 0) with 7
local_zeta_factor_at ((2) + (0)*i ; 0) with real
local_zeta_factor_at ((4) + (0)*i ; 0) with real
local_zeta_factor_at ((-1) + (0)*i ; 0) with real
local_zeta_factor_at ((3) + (0)*i ; 0) with real
local_zeta_factor_at ((1) + (1)*i ; 0) with real
local_zeta_factor_at ((-3) + (1)*i ; 0) with real
local_zeta_factor_at ((1) + (0)*i ; 0) with 18446744073709551557
local_zeta_factor_at ((2) + (0)*i ; 5 mod 7) with 2
