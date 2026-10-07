#!exit 1
# Slice 3.2-b: docs/api-3.md 3.2:370-389, D3-3:809-817, Q4; conventions 6.1:844,876-887.
# Expected lines written by hand before the run. psi(m ; a) = E(a - m); a class lift has the image of its adele.
# E(1/3) = -1/2 + i sqrt(3)/2 prints as in psi-values.out; E3 = (0 ; 0 mod 1/2) has the image {+1, -1}, the line
# hull [-1,1] x {0}, printed "(0 +/- 1.1) + (0)*i" as in psi-values.out. Its lift fails strict (D3-3).
# The union text of api-3.md 7 waits for slice 3.1-d (the reader answers UNSUPPORTED until then).
prec 128
digits 5
psi (0 ; 1/3) + Q
psi_strict (0 ; 1/3) + Q
psi (0 ; 0 mod 1/2) + Q
psi_strict (0 ; 0 mod 1/2) + Q
psi (0.25 ; 0) + Q
psi_strict (0.5 +/- 0.5 ; 0 mod 2) + Q
psi_phase (0 ; 1/3) + Q
psi_phase (0.25 ; 0) + Q
psi_phase (0 ; 0 mod 1/2) + Q
psi_phase (0 +/- 0.5 ; 0) + Q
psi_phase (* ; 7/3)
psi_phase (* ; 1/4 mod 1/3)
psi_phase (0.5 ; 1/3)
psi 1/3
psi_phase 1/3
psi_phase (0 ; 1/3
