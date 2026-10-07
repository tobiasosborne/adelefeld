#!exit 1
# Slice 4d (lane f4-slice2): docs/api-4.md 5, analysis Proposition 5. Expected lines derived by hand before the
# run (digits of 2 pi/3, pi/9, exp(-pi/4), exp(-pi) from mpmath at 50 digits; the printer of conventions 9.5).
prec 128
# PLAN 4.3: exp(-pi (x - 1/3)^2) = exp(-pi x^2 + (2 pi/3) x - pi/9). 2 pi/3 = 2.0943951023931954923|0843: at 20
# digits M = 2.0943951023931954923 and R = 8.43e-21 rounded up to 8.5e-21; -pi/9 = -0.34906585039886591538|4738,
# R = 4.74e-21 rounded up to 4.8e-21. A and P are exact.
rfun_translate rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) with 1/3
# translation by 0 is the identity on these exact parameters
rfun_translate rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) with 0
# products: pairs in lexicographic order, polynomials multiplied, A, B, C added.
# (x e^(-pi x^2) + 2 e^(-2 pi x^2)) (e^(-pi x^2/2) + e^(-pi x^2/4)): A = 1.5, 1.25, 2.5, 2.25
rfun_mul rfun(term(P=[(0) + (0)*i, (1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i), term(P=[(2) + (0)*i], A=(2) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) with rfun(term(P=[(1) + (0)*i], A=(0.5) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i), term(P=[(1) + (0)*i], A=(0.25) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))
# the other order: A = 1.5, 2.5, 1.25, 2.25
rfun_mul rfun(term(P=[(1) + (0)*i], A=(0.5) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i), term(P=[(1) + (0)*i], A=(0.25) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) with rfun(term(P=[(0) + (0)*i, (1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i), term(P=[(2) + (0)*i], A=(2) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))
rfun_mul rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) with 1/3
rfun_translate rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) with 0.5
rfun_translate rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) with (0 ; 0)
