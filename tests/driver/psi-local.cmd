#!exit 1
# Slice 3.2-c: docs/api-3.md 3.2:391-414, Q4 step 8; conventions 6.1:844 (psi_p = E(fp_p), psi_inf = E(-x)).
# Expected lines written by hand before the run. fp_2(1/6) = 1/2 (1/6 - 1/2 = -1/3 in Z_2), so psi_2 = -1,
# not E(1/6) (fault 3 of api-3.md 5); fp_3(1/6) = 2/3, E(2/3) = -1/2 - i sqrt(3)/2; fp_3(-1/3) = 2/3.
# 1/6 + O(2^0) is one phase 1/2; 1/6 + O(2^-1) is {0, 1/2}: line hull; 0 + O(2^-2) the 4th roots: the square.
# psi_at (m ; a) with p uses a only; with real, E(-m) (real uncertainty allowed by strict). A prime that does
# not divide the denominator gives E(0) = 1 exactly. The places are named as by the other commands: real, p.
prec 128
digits 5
psi [p=2: 1/6]
psi_phase [p=2: 1/6]
psi [p=3: 1/6]
psi_phase [p=3: 1/6]
psi_phase [p=3: -1/3]
psi_strict [p=2: 1/6 + O(2^0)]
psi_strict [p=2: 1/6 + O(2^-1)]
psi [p=2: 1/6 + O(2^-1)]
psi_phase [p=2: 1/6 + O(2^-1)]
psi [p=2: 0 + O(2^-2)]
psi_phase [p=5: 3 + O(5^4)]
psi_phase [p=5: 1/125 + O(5^-2)]
psi_strict [p=5: 1/125 + O(5^-2)]
psi_at (0 ; 1/3) with 3
psi_at (0 ; 1/3) with 5
psi_at (0.25 ; 1/3) with real
psi_at (0.25 ; 1/3) with 3
psi_strict_at (0 ; 1/3 mod 1/2) with 2
psi_strict_at (0 ; 1/3 mod 1/2) with 3
psi_at (0 ; 0 mod 1/2) with 2
psi_strict_at (0 +/- 0.5 ; 0) with real
psi_at (0 ; 1/3) with 18446744073709551557
psi_at (0 ; 1/3) with 4
psi_at (0 ; 1/3) with x
psi_at [p=3: 1/3] with 3
psi_at (0 ; 1/3)
