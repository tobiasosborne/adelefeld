#!exit 1
# 1F.9 local zeta factor (lane f-slice14): the statuses of SPEC 9.3.7 row "local zeta factor" and conventions
# :944-951 through the driver, and the order of the checks of tools/adf/README.md. Written before the run.
# Exact poles (DOMAIN): 0 at p; 0, -2, -40 at real. Balls meeting a pole (NOT_DETERMINED): around 0 at p; the
# segment through the nonzero pole 2 pi i / log 2 = 9.06472028365438761925536589143333362034372293544759116837203
# 30958812019074426102 i (written with 60 decimals, error below 1e-61, radius 1e-60); around -2; [-2, 0] at real.
# The recurrence bound: [-131, -129] + i [0.15, 0.35] needs 67 factors (LIMIT).
# A composite, 1, 2^64 and -2 are no place (DOMAIN); "two" is not a place token (PARSE).
# A rational and a real adele are not the complex carrier (UNSUPPORTED).
# A missing operand and a broken operand are PARSE; a syntax error in the operand wins over a bad place.
prec 128
digits 5
local_zeta_factor_at ((0) + (0)*i ; 0) with 2
local_zeta_factor_at ((0 +/- 0.0001) + (0 +/- 0.0001)*i ; 0) with 2
local_zeta_factor_at ((0) + (9.064720283654387619255365891433333620343722935447591168372033 +/- 1e-60)*i ; 0) with 2
local_zeta_factor_at ((0) + (0)*i ; 0) with real
local_zeta_factor_at ((-2) + (0)*i ; 0) with real
local_zeta_factor_at ((-40) + (0)*i ; 0) with real
local_zeta_factor_at ((-2 +/- 0.0001) + (0)*i ; 0) with real
local_zeta_factor_at ((-1 +/- 1) + (0)*i ; 0) with real
local_zeta_factor_at ((-130 +/- 1) + (0.25 +/- 0.1)*i ; 0) with real
local_zeta_factor_at ((1) + (0)*i ; 0) with 4
local_zeta_factor_at ((1) + (0)*i ; 0) with 1
local_zeta_factor_at ((1) + (0)*i ; 0) with 18446744073709551616
local_zeta_factor_at ((1) + (0)*i ; 0) with -2
local_zeta_factor_at ((1) + (0)*i ; 0) with two
local_zeta_factor_at 1 with 2
local_zeta_factor_at (1 ; 0) with 2
local_zeta_factor_at ((1) + (0)*i ; 0)
local_zeta_factor_at ((1) + (0)*i ; 0 with 2
local_zeta_factor_at ((1) + (0)*i ; 0 with two
local_zeta_factor_at ((-2) + (0)*i ; 0) with 2
