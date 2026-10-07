#!exit 1
# Slice 5c (lane t5-slice2): docs/api-5.md 3 and 8 item 3; docs/api-5b.md "Slice 5c"; SPEC 8. Expected lines
# derived by hand before the run (lanes/t5-slice2/derive_fixture.py: conventions 9.5 printing on exact rationals).
# tate_vector: conventions 6.5, f[j] = chi(j) (exact phases: zeta 1; chi_4 = (4,3): 0, 1, 0, -1; (5,2): phases
# 0, 1/4, 3/4, 1/2 at 1, 2, 3, 4, so 0, 1, i, -i, -1); P = x^e (e = 0, 1, 1), A = 1, B = C = 0, all exact.
prec 128
digits 10
tate_vector char(q=1, n=1, s=(0) + (0)*i)
tate_vector char(q=4, n=3, s=(0) + (0)*i)
tate_vector char(q=5, n=2, s=(7) + (1)*i)
# zeta(2) completed: I(2) = pi^-1 Gamma(1) zeta(2) = pi/6 = 0.52359877559829887308; digits 10: q = -10,
# M = 0.5235987756, |M - mid| = 1.70113e-12, R = ceil2(1.70113e-12 + 2.57e-18) = 1.8e-12. The imaginary part is the
# exact 0 plus the added tail and quadrature bound E_n + E_t + both remainders = 2.5667e-18 (N = 4, R = 16, the
# oracle's continuation(zeta, 2, 53), the same cutoffs): 0 +/- ceil2(2.5667e-18) = 2.6e-18.
tate_integral char(q=1, n=1, s=(0) + (0)*i) with ((2) + (0)*i ; 0) with 53
# chi_4 at 2: I = pi^(-3/2) Gamma(3/2) L(2, chi_4) = G/(2 pi) = 0.14578045201540939007 (G Catalan's constant);
# M = 0.145780452 (0.1457804520 rounded at 10^-10), R = ceil2(1.5409e-11) = 1.6e-11; imaginary: the bound
# 2.7102e-18 (N = 8, R = 64) times |4^(-3/2)| = 1/8: ceil2(3.3878e-19) = 3.4e-19.
tate_integral char(q=4, n=3, s=(0) + (0)*i) with ((2) + (0)*i ; 0) with 53
# The domain: Re(s) = 1 (L(1, chi_4) = pi/4 is outside), s = 1/2 + 10 i: DOMAIN; a ball straddling 1:
# NOT_DETERMINED; the width witness of N-D23 (zeta on [1.125, 1.25], end values 3.91 apart): NOT_DETERMINED.
tate_integral char(q=4, n=3, s=(0) + (0)*i) with ((1) + (0)*i ; 0) with 53
tate_integral char(q=1, n=1, s=(0) + (0)*i) with ((1) + (0)*i ; 0) with 20
tate_integral char(q=5, n=2, s=(0) + (0)*i) with ((0.5) + (10)*i ; 0) with 20
tate_integral char(q=1, n=1, s=(0) + (0)*i) with ((1 +/- 0.001) + (0)*i ; 0) with 20
tate_integral char(q=1, n=1, s=(0) + (0)*i) with ((1.1875 +/- 0.0625) + (0)*i ; 0) with 20
# bits outside [0, 2^21] and a non-integral bits: DOMAIN; C = 1031 > 1024: LIMIT (the transform cap of D2), also
# with bits -1 (the size check comes first); an operand of another kind: DOMAIN.
tate_integral char(q=1, n=1, s=(0) + (0)*i) with ((2) + (0)*i ; 0) with -1
tate_integral char(q=1, n=1, s=(0) + (0)*i) with ((2) + (0)*i ; 0) with 2097153
tate_integral char(q=1, n=1, s=(0) + (0)*i) with ((2) + (0)*i ; 0) with 1/2
tate_integral char(q=1031, n=2, s=(0) + (0)*i) with ((2) + (0)*i ; 0) with 20
tate_integral char(q=1031, n=2, s=(0) + (0)*i) with ((2) + (0)*i ; 0) with -1
tate_vector 2
tate_integral char(q=1, n=1, s=(0) + (0)*i) with 2 with 20
