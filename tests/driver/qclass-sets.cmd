#!exit 1
# Q2, P3:71 and D3-1. Expected answers derived before implementing the commands.
# The first pair meets only through (1,0) = (0,-1); K=2, E=3, L=3, budget=42.
qoverlaps (0.75 +/- 0.25 ; 0 mod 3) + Q with (0.25 +/- 0.25 ; 2 mod 3) + Q with 42
qoverlaps (0.75 +/- 0.25 ; 0 mod 3) + Q with (0.25 +/- 0.25 ; 1 mod 3) + Q with 42
qequal (0.75 +/- 0.25 ; 0 mod 3) + Q with (0.25 +/- 0.25 ; 2 mod 3) + Q with 42
qcontains (0.75 +/- 0.25 ; 0 mod 3) + Q with (0.25 +/- 0.25 ; 2 mod 3) + Q with 42
qoverlaps (0.75 +/- 0.25 ; 0 mod 3) + Q with (0.25 +/- 0.25 ; 2 mod 3) + Q with 41
# Exact diagonal translation: both are the zero class; budget 6.
qequal (1 ; 1) + Q with (0 ; 0) + Q with 6
qequal (1 ; 1) + Q with (0 ; 0) + Q with 5
qcontains (0.5 ; 0) + Q with (0.5 +/- 0.25 ; 0 mod 3) + Q with 1000
qcontains (0.5 +/- 0.25 ; 0 mod 3) + Q with (0.5 ; 0) + Q with 1000
qoverlaps (0.5 ; 0) + Q with (0.5 +/- 0.25 ; 1 mod 3) + Q with 1000
qoverlaps (0.25 +/- 0.25 ; 0 mod 3) + Q with (0.75 +/- 0.25 ; 0 mod 3) + Q with 1000
qcontains (0.5 ; 0 mod 1) + Q with (0 ; 0 mod 1/2) + Q with 15
qequal (0 ; 0 mod 1/2) + Q with (0.5 ; 0 mod 1) + Q with 15
qequal (0.5 ; 0 mod 3) + Q with (0.5 ; 0 mod 3) + Q with 18
qequal (0.5 ; 0 mod 3) + Q with (0.5 ; 0 mod 3) + Q with 17
qcontains (0 ; 0) + Q with (0 ; 0) + Q with 0
qoverlaps (0 ; 0) + Q with (0 ; 0) + Q with -1
qequal (0 ; 0) + Q with (0 ; 0) + Q with 1000000000000000000000000000000
qequal (0 ; 0) + Q with (0 ; 0) + Q with 1/2
qcontains 0 with (0 ; 0) + Q with 100
qoverlaps (0 ; 0) + Q with 0 with 100
qequal (0 ; 0) + Q with (0 ; 0) + Q
