#!exit 1
# CV-59: fractional finite radius is NOT_DETERMINED for strict; real uncertainty is allowed.
# Wrong kind UNSUPPORTED; malformed grammar PARSE; zero denominator DOMAIN.
prec 128
digits 5
psi_strict (0 ; 0 mod 1/2)
psi_strict (0 +/- 0.5 ; 0 mod 1/3)
psi 1/3
psi (* ; 1/3)
psi ((0) + (0)*i ; 0)
psi (0 ; 1/0)
psi (0 ; 1/3
psi
psi (0 ; 0) with (0 ; 0)
psi_strict (0 +/- 0.5 ; 0 mod 1)
