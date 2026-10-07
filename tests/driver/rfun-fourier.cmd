#!exit 1
# Slice 4e (lane f4-slice3): docs/api-4.md 5 (R1, R2, derivative) and 6 (integral, norm2), the commands
# rfun_fourier R, rfun_derivative R, rfun_integral R, rfun_norm2 R. Expected lines derived by hand before the run,
# digits from mpmath at 50 digits and the printer of conventions 9.5 (M rounded to n digits, R = ceil2(|M - mid|
# + rad)). F(exp(-pi x^2)) = exp(-pi y^2) and F(x exp(-pi x^2)) = i y exp(-pi y^2), exactly (P5); A = 4 gives
# the amplitude 4^(-1/2) = 1/2 and A' = 1/4. B = i: B' = i i/1 = -1, C' = i^2/(4 pi) = -0.0795774715|459: at
# digits 5 M = -0.079577, R = 4.715e-7 rounded up to 4.8e-7.
prec 128
digits 5
rfun_fourier rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))
rfun_fourier rfun(term(P=[(0) + (0)*i, (1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))
rfun_fourier rfun(term(P=[(1) + (0)*i], A=(4) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))
rfun_fourier rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (1)*i, C=(0) + (0)*i))
rfun_fourier rfun()
# The sign test of PLAN 4.3 (R1 step 3): exp(-pi (x - 1/3)^2) has B = 2 pi/3 (60 digits, a point) and
# C = -pi/9 (60 digits, +/- 1e-6). The transform has P = 1, A' = 1, B' = i 2.0943951023|93: M = 2.0944,
# R = 4.8976e-6 up to 4.9e-6; C' = C + B^2/(4 pi) = 0 with the radius of C, just above 1e-6: 0 +/- 1.1e-6.
rfun_fourier rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(2.09439510239319549230842892218633525613144626625007054731663) + (0)*i, C=(-0.349065850398865915384738153697722542602431868541702232854367 +/- 1e-6) + (0)*i))
# That printed transform at y = 1/4, digits 3: exp(-pi/16 + i 2.0944/4) = 0.7116341855|2 + 0.4108633503|4 i
# (mpmath); M = 0.712, R = 3.658e-4 plus the ball radius (about 2e-6) up to 0.00037 (fmt: X = -4 is
# positional); M = 0.411, R = 1.3665e-4 plus about 2e-6 up to 0.00014. The imaginary part is positive (the
# opposite kernel gives the conjugate).
digits 3
rfun_eval rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (2.0944 +/- 4.9e-6)*i, C=(0 +/- 1.1e-6) + (0)*i)) with 0.25
digits 5
# Derivative of exp(-pi x^2): P' + (B - 2 pi A x) P = -2 pi x; 2 pi = 6.2831853071|8: M = -6.2832, R = 1.4693e-5
# up to 1.5e-5. The zero function stays zero.
rfun_derivative rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))
rfun_derivative rfun()
# Integrals: exp(-pi x^2) gives 1 exactly, A = 4 gives 1/2 exactly; x^2 exp(-pi x^2) gives h_2 = 1/(2 pi) =
# 0.1591549430|9: M = 0.15915, R = 4.943e-6 up to 5e-6. The zero function gives the exact 0.
rfun_integral rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))
rfun_integral rfun(term(P=[(1) + (0)*i], A=(4) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))
rfun_integral rfun(term(P=[(0) + (0)*i, (0) + (0)*i, (1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))
rfun_integral rfun()
# Norms: |exp(-pi x^2)|^2 = exp(-2 pi x^2) has integral 2^(-1/2) = 0.7071067811|9: M = 0.70711, R = 3.2188e-6
# up to 3.3e-6. The zero function: 0.
rfun_norm2 rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))
rfun_norm2 rfun()
# Statuses: a rational is DOMAIN (the kind), an incomplete text PARSE.
rfun_fourier 1/3
rfun_norm2 rfun(
