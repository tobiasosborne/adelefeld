#!exit 1
# Slice 4f (lane f4-slice5): docs/api-4.md 6, statement E1, and docs/api-4c.md "Slice 4f". Expected lines derived
# by hand before the run, from E1 and the printer of conventions 9.5 (M rounded to n digits, R = ceil2(|M - mid| +
# rad)); digits from mpmath at 40 digits. F6 = (D, M) = (2, 3) with f = [1..6].
# E1 step 3: Zhat meets {0, 2, 4}: the hull of {1, 3, 5} is [1, 5] = 3 +/- 2 exactly (the radius 2 fits a mag);
# (1/4) Zhat meets every index and leaves the support: [0, 6] = 3 +/- 3; the exact point 1/5 gives exactly 0;
# the point 7 = 14/2 has index 14 mod 6 = 2, the value 3 exactly. 1/2 + Zhat = (1 + 2 Zhat)/2: gcd(H D, M d D) =
# gcd(4, 12) = 4 divides A D - j d = 2 - 2 j iff j is odd: {2, 4, 6}, inside since d = 2 divides D A = 2 and
# D H = 4: [2, 6] = 4 +/- 2. The jump of PLAN 4.4: 10 on 2 Zhat, 20 on 1 + 2 Zhat; Zhat crosses it: 15 +/- 5.
prec 128
digits 5
ffun_eval ffun(D=2, M=3; (1) + (0)*i, (2) + (0)*i, (3) + (0)*i, (4) + (0)*i, (5) + (0)*i, (6) + (0)*i) with (* ; 0 mod 1)
ffun_eval ffun(D=2, M=3; (1) + (0)*i, (2) + (0)*i, (3) + (0)*i, (4) + (0)*i, (5) + (0)*i, (6) + (0)*i) with (* ; 0 mod 1/4)
ffun_eval ffun(D=2, M=3; (1) + (0)*i, (2) + (0)*i, (3) + (0)*i, (4) + (0)*i, (5) + (0)*i, (6) + (0)*i) with 1/5
ffun_eval ffun(D=2, M=3; (1) + (0)*i, (2) + (0)*i, (3) + (0)*i, (4) + (0)*i, (5) + (0)*i, (6) + (0)*i) with 7
ffun_eval ffun(D=2, M=3; (1) + (0)*i, (2) + (0)*i, (3) + (0)*i, (4) + (0)*i, (5) + (0)*i, (6) + (0)*i) with (* ; 1/2 mod 1)
ffun_eval ffun(D=1, M=2; (10) + (0)*i, (20) + (0)*i) with (* ; 0 mod 1)
ffun_eval ffun(D=1, M=2; (10) + (0)*i, (20) + (0)*i) with (* ; 1 mod 2)
# tensor_eval at (0.25 ; 7): exp(-pi/16) 3 = 2.4651748741|0: M = 2.4652, R = ceil2(2.5126e-5 + 2^-120 or so) =
# 2.6e-5. At (0 ; 7): phi(0) = 1 exactly (Horner of [1], exp(0)), times 3: exactly 3.
tensor_eval rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) with ffun(D=2, M=3; (1) + (0)*i, (2) + (0)*i, (3) + (0)*i, (4) + (0)*i, (5) + (0)*i, (6) + (0)*i) with (0.25 ; 7)
tensor_eval rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) with ffun(D=2, M=3; (1) + (0)*i, (2) + (0)*i, (3) + (0)*i, (4) + (0)*i, (5) + (0)*i, (6) + (0)*i) with (0 ; 7)
# tensor_eval_sball (E1 steps 4-5): the exact local point 1/4 at 2 meets no coset (v_2(j/2 - 1/4) = -2 < 0 =
# v_2(3)): exactly 0. At 5 the ball 1 + O(5^1) keeps every index (v_5(j/2 - 1) >= 0 = min(1, v_5(3))), and zero
# is always included: the hull [0, 6] = 3 +/- 3, times phi(0) = 1, whose magnitude arf_get_mag rounds up by one
# mag ulp (2^-29): the radius 3 (1 + 2^-29) prints as ceil2 = 3.1.
tensor_eval_sball rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) with ffun(D=2, M=3; (1) + (0)*i, (2) + (0)*i, (3) + (0)*i, (4) + (0)*i, (5) + (0)*i, (6) + (0)*i) with {inf: 0; p=2: 1/4}
tensor_eval_sball rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) with ffun(D=2, M=3; (1) + (0)*i, (2) + (0)*i, (3) + (0)*i, (4) + (0)*i, (5) + (0)*i, (6) + (0)*i) with {inf: 0; p=5: 1 + O(5^1)}
# arch NONE with the Gaussian: alpha = pi, beta = 0, gamma = 0, R = 1: the box [-1, 1] + i [-1, 1] times the hull
# [0, 1] = 0.5 +/- 0.5 of {0, 1}: each part 0 +/- (1 + mag rounding), printed 0 +/- 1.1 (rule 2). (This line was
# also seen in an exploratory run before the fixture was written.)
tensor_eval_sball rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) with ffun(D=1, M=1; (1) + (0)*i) with {}
# Integrals and norms (analysis P4): (1/3) 21 = 7 exactly; (1/3) 91 = 30.333|33: M = 30.333, R = ceil2(3.33e-4) =
# 0.00034. The Gaussian has integral 1 exactly and norm 2^(-1/2): 7 exactly, and 91/(3 sqrt 2) = 21.448905696|0:
# M = 21.449, R = ceil2(9.43e-5) = 9.5e-5.
ffun_integral ffun(D=2, M=3; (1) + (0)*i, (2) + (0)*i, (3) + (0)*i, (4) + (0)*i, (5) + (0)*i, (6) + (0)*i)
ffun_norm2 ffun(D=2, M=3; (1) + (0)*i, (2) + (0)*i, (3) + (0)*i, (4) + (0)*i, (5) + (0)*i, (6) + (0)*i)
tensor_integral rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) with ffun(D=2, M=3; (1) + (0)*i, (2) + (0)*i, (3) + (0)*i, (4) + (0)*i, (5) + (0)*i, (6) + (0)*i)
tensor_norm2 rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) with ffun(D=2, M=3; (1) + (0)*i, (2) + (0)*i, (3) + (0)*i, (4) + (0)*i, (5) + (0)*i, (6) + (0)*i)
# Statuses: COMPLEX arch is DOMAIN (E1 step 5); a rational in place of R, an adele in place of B: DOMAIN (the
# kind); C = 10^300 overflows exp: NOT_DETERMINED; an incomplete text: PARSE.
tensor_eval_sball rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) with ffun(D=1, M=1; (1) + (0)*i) with {inf: (1) + (1)*i}
tensor_eval 1/3 with ffun(D=2, M=3; (1) + (0)*i, (2) + (0)*i, (3) + (0)*i, (4) + (0)*i, (5) + (0)*i, (6) + (0)*i) with (0 ; 7)
ffun_eval ffun(D=2, M=3; (1) + (0)*i, (2) + (0)*i, (3) + (0)*i, (4) + (0)*i, (5) + (0)*i, (6) + (0)*i) with (1 ; 0)
tensor_eval rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(1e300) + (0)*i)) with ffun(D=2, M=3; (1) + (0)*i, (2) + (0)*i, (3) + (0)*i, (4) + (0)*i, (5) + (0)*i, (6) + (0)*i) with (0 ; 7)
ffun_integral ffun(
