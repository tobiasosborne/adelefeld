# Slice 3.2-a: SPEC 6, conventions 6.1:844,876-887, api-3.md Q4 and 3.3.
# Written before running: E(1/3)=-1/2+i sqrt(3)/2; E(-1/4)=-i.
# sqrt(3)/2=0.86602540378..., so digits 5 rounds to 0.86603 with radius 4.6e-6.
# Q1 RU30's successor makes radius 1 slightly larger than 1; the printer rounds up to 1.1.
prec 128
digits 5
psi (0 ; 1/3)
psi (0 ; -1/3)
psi (0.25 ; 0)
psi (-0.25 ; 0)
psi (0 ; 1/2)
psi (0 ; 0)
psi_strict (0 ; 1/3 mod 6)
psi (0 ; 0 mod 1/2)
psi (0 +/- 0.5 ; 0)
