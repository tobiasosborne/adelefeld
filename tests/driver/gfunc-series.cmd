#!exit 1
# gfunc-series: exp, sin, sinh, cos, cosh at all places (lane f-slice10, WP 1F.8; SPEC 9.3.2 lines 588-596;
# include/adelefeld/gfunc.h; docs/api-1f8.md G5, G6).  X a rational or an adele.  Every expected line is derived by
# hand in the comment above it.
# ---- the finite part exactly 0: the exact constants 1, 0, 0, 1, 1 (Proposition 12 step 1); the real part exact at 0
exp (0 ; 0)
sin (0 ; 0)
sinh (0 ; 0)
cos (0 ; 0)
cosh (0 ; 0)
exp 0
# The five inexact lines below: the text of the printer (digits 10, radius rounded up) was taken from the program's
# output, and each was checked by hand to contain the true value: |1.648721271 - e^(1/2)| = 2.9999e-10 <= 3e-10, and
# likewise 7.9e-12, 3.19e-11, 3.56e-10, 1.85e-10 for the four values below.
# e^(1/2) = 1.64872127070012814684865...
digits 10
exp (0.5 ; 0)
# sin(1) = 0.8414709848078965..., cos(1) = 0.5403023058681397..., sinh(1) = 1.1752011936438014...,
# cosh(1) = 1.5430806348152437...
sin (1 ; 0)
cos (1 ; 0)
sinh (1 ; 0)
cosh (1 ; 0)
# ---- an exact finite part q != 0: DOMAIN (the driver prints the status; the library names the first prime with
# v_p(q) < c: 2 for 1, 1/3, -9/2; 3 for 4; 5 for 12)
exp 1
exp (0 ; 4)
sin (0 ; 12)
cos (0 ; 1/3)
cosh (0 ; -9/2)
# ---- a finite part of positive radius: NOT_DETERMINED (0 + 4 Zhat cannot be decided; 3 + 9 Zhat is not examined)
exp (0 ; 0 mod 4)
sinh (1 ; 3 mod 9)
# ---- a real part that arb cannot evaluate: exp(10^300) is NOT_DETERMINED (an infinite ball, rfunc.h); with q = 1
# the DOMAIN of the finite part is the maximum (DOMAIN above NOT_DETERMINED)
exp (1e300 ; 0)
exp (1e300 ; 1)
# ---- types the operation does not define: a finite ball, an idele: DOMAIN; a second operand: PARSE
exp 0 mod 4
exp (1 ; 1 * [1])
exp 0 with 1
