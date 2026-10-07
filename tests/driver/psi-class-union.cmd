#!exit 1
# The class character on union texts (lane q-slice5, landed after slice 3.1-d read the union form).
# E3's exact reduction union((0 ; 0 mod 1), (0.5 ; 0 mod 1)) + Q: phases 0 and 1/2, the line hull [-1,1] x {0};
# strict passes (D3-3: integral radii); the phase getter is NOT_DETERMINED (two different singletons).
# union((0.25 ; 0), (0.25 ; 0 mod 3)) + Q: both phase 3/4, so the getter prints 3/4 and psi is -i exactly.
prec 128
digits 5
psi union((0 ; 0 mod 1), (0.5 ; 0 mod 1)) + Q
psi_strict union((0 ; 0 mod 1), (0.5 ; 0 mod 1)) + Q
psi_phase union((0 ; 0 mod 1), (0.5 ; 0 mod 1)) + Q
psi_phase union((0.25 ; 0), (0.25 ; 0 mod 3)) + Q
psi union((0.25 ; 0), (0.25 ; 0 mod 3)) + Q
