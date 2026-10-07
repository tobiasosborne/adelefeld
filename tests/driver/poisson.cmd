#!exit 1
# Slice 4g (lane f4-slice6): docs/api-4.md 7, statement P1, and docs/api-4c.md "Slice 4g". Expected lines derived
# by hand before the run, from P1, Lemma 6 (docs/proofs/analysis.md:213-259) and the printer of conventions 9.5
# (q = max(X(mid) - n + 1, X(rad) - 1), M = round(mid, q), R = ceil2(rad + |M - mid|)); digits of theta and of the
# tails from mpmath at 60 digits (the oracle's lattice_tail, tests/ref/vectors/f4-slice6/poisson.jsonl, record 0).
# G = exp(-pi x^2), F = 1_Zhat (D = M = 1, f = [1]): left = right = theta = sum_n exp(-pi n^2) = 1.0864348112133080.
# The transform of G is G exactly (A' = 1/1, B' = 0, C' = 0, root 1) and g = [1], so the two lines agree digit for
# digit. The tail E(N) = B(0, 1, N) is the only radius of the imaginary part (the sums are real and exact there).
prec 144
digits 10
# bits 20: goal 2^-23 = 1.19e-7; E(1) = 6.97e-6 > goal, E(2) = 1.0511e-12 <= goal: N = 2 (the search 0, 1, 2).
# mid = 1 + 2 exp(-pi) + 2 exp(-4 pi) = 1.0864348112122569; q = max(-9, -13) = -9: M = 1.086434811,
# R = ceil2(2.1226e-10 + 1.05e-12) = 2.2e-10; imaginary 0 +/- ceil2(1.0511e-12) = 1.1e-12.
poisson rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) with ffun(D=1, M=1; (1) + (0)*i) with 20
# bits 80 (the design's example, prec 144): goal 2^-83 = 1.03e-25; E(2) > goal, E(4) = 1.5546e-34 <= goal: N = 4.
# M = 1.086434811, R = ceil2(2.1331e-10) = 2.2e-10; imaginary 0 +/- ceil2(1.5546e-34) = 1.6e-34.
poisson rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) with ffun(D=1, M=1; (1) + (0)*i) with 80
# bits 0: goal 1/8; E(0) = 0.0864348 <= 1/8: N = 0, the term n = 0 alone: 1 +/- 0.0864348 on both coordinates;
# q = max(-9, X(0.0864) - 1 = -3) = -3: M = 1, R = ceil2(0.0864348) = 0.087.
poisson rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) with ffun(D=1, M=1; (1) + (0)*i) with 0
digits 30
# bits 53: N = 4 (E(2) = 1.05e-12 > 2^-56 = 1.39e-17 >= E(4)). q = max(-29, -35) = -29:
# M = 1.08643481121330801457531612151, |M - mid| = 2.2346e-31 - 1.55e-34, R = ceil2(2.2346e-31) = 2.3e-31.
poisson rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) with ffun(D=1, M=1; (1) + (0)*i) with 53
digits 5
prec 20
# prec 20 cannot give 2^-30 (one rounding of 1.08 at 20 bits is 2^-20): the retry at 40 bits does (rounding about
# 2^-38, tail E(2) = 1.05e-12, N = 2 since 2^-33 = 1.16e-10 >= E(2)). digits 5: q = -4, M = 1.0864,
# R = ceil2(3.4811e-5 + about 1e-11) = 3.5e-5; imaginary 1.1e-12 as above.
poisson rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) with ffun(D=1, M=1; (1) + (0)*i) with 30
prec 144
# The witness of api-4.md section 10: c in [1, 2], the left value interval has width theta = 1.086 > 2^-10 at every
# precision: NOT_DETERMINED (after one doubling 144 -> 288 that does not halve the diameter).
poisson rfun(term(P=[(1.5 +/- 0.5) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) with ffun(D=1, M=1; (1) + (0)*i) with 10
# bits outside [0, 2^21]: DOMAIN; a rational that is not an integer: DOMAIN; another kind of operand: DOMAIN.
poisson rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) with ffun(D=1, M=1; (1) + (0)*i) with -1
poisson rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) with ffun(D=1, M=1; (1) + (0)*i) with 2097153
poisson rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) with ffun(D=1, M=1; (1) + (0)*i) with 1/2
poisson ffun(D=1, M=1; (1) + (0)*i) with rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) with 20
# Re(A) = 10^-8 (design section 8): the Lemma 6 prefix needs about log(2)/(2 pi 10^-8) = 1.1e7 > 2^20 ratio
# iterations: LIMIT (D1).
poisson rfun(term(P=[(1) + (0)*i], A=(0.00000001) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) with ffun(D=1, M=1; (1) + (0)*i) with 20
