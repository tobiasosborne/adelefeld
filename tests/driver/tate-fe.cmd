#!exit 1
# Slice 5e (lane t5-slice5): docs/api-5.md 5 and 8 item 5; docs/api-5s.md "Slice 5e"; SPEC 8; analysis P13
# (Lambda(s, chi) = W_chi Lambda(1 - s, conj(chi)), docs/proofs/analysis.md:574). tate_fe CHI with S with BITS
# prints "A | R | overlap=yes|no | s=S": A = adf_tate_completed(chi, s, BITS), R = W * adf_tate_completed(conj(chi),
# 1 - s, BITS + 4), W = adf_char_root_number(chi). Expected lines derived before the run by
# lanes/t5-slice5/derive_fixture.py from tests/ref/vectors/t5-slice5/fe.jsonl (60-digit mpmath midpoints;
# conventions 9.5 printing) and, for the exact zero imaginary parts, the oracle's bound (proto/tate_checks.py
# continuation: A at BITS, B at BITS + 4).
prec 160
digits 10
# zeta at 1/2: W = 1 exactly and conj(chi) = chi, so A and R are the same number from two calls. The real parts
# print alike (-3.976966226 +/- 5e-10: the printer's rounding dominates); the imaginary parts are the exact 0 plus
# the bound of each call: A at bits 80, 2.0992e-26 -> 2.1e-26 (N = 4, R = 32); B at bits 84, 1.4327e-27 -> 1.5e-27.
tate_fe char(q=1, n=1, s=(0) + (0)*i) with ((0.5) + (0)*i ; 0) with 80
# chi_4 = (4,3) at 3/8 + 3i/4 (odd, W = 1): Lambda = 0.93953433697... - 0.01383492257... i; both radii below 1e-24
# do not show at digits 10, so A and R print alike.
tate_fe char(q=4, n=3, s=(0) + (0)*i) with ((0.375) + (0.75)*i ; 0) with 80
# the raw (8,7) lowers to (4,3) (api-5.md:355-356): the same line.
tate_fe char(q=8, n=7, s=(0) + (0)*i) with ((0.375) + (0.75)*i ; 0) with 80
# non-real characters: (5,2) at 3/8 + 3i/4 (odd), (7,3) at 2 + 3i (odd), the composite even (15,2) at 1/2.
tate_fe char(q=5, n=2, s=(0) + (0)*i) with ((0.375) + (0.75)*i ; 0) with 80
tate_fe char(q=7, n=3, s=(0) + (0)*i) with ((2) + (3)*i ; 0) with 80
tate_fe char(q=15, n=2, s=(0) + (0)*i) with ((0.5) + (0)*i ; 0) with 80
# chi_4 at its trivial zero -1: A = Lambda(-1) = 4G/pi = 1.1662436161 (Gamma(z) has a pole at z = 0, Lambda is
# finite); R = W Lambda(2, chi_4). Imaginary radii: A at bits 80, 3.3347e-26 -> 3.4e-26; B at s = 2, bits 84,
# 1.3152e-27 -> 1.4e-27.
tate_fe char(q=4, n=3, s=(0) + (0)*i) with ((-1) + (0)*i ; 0) with 80
# the poles of C = 1 (statuses, not values): s = 1 and s = 0 are DOMAIN at A (B would be DOMAIN as well, at 0 and
# 1); a ball meeting 1: NOT_DETERMINED at A. The failing call is named before the status.
tate_fe char(q=1, n=1, s=(0) + (0)*i) with ((1) + (0)*i ; 0) with 80
tate_fe char(q=1, n=1, s=(0) + (0)*i) with ((0) + (0)*i ; 0) with 80
tate_fe char(q=1, n=1, s=(0) + (0)*i) with ((1 +/- 0.001) + (0)*i ; 0) with 20
# BITS outside [0, 2^21]: DOMAIN from A (B is not called); BITS not an integer: DOMAIN before any call (no call
# named); C = 1031 > 1024: LIMIT from A; an operand of another kind: DOMAIN before any call.
tate_fe char(q=1, n=1, s=(0) + (0)*i) with ((-1) + (0)*i ; 0) with -1
tate_fe char(q=1, n=1, s=(0) + (0)*i) with ((-1) + (0)*i ; 0) with 2097153
tate_fe char(q=1, n=1, s=(0) + (0)*i) with ((-1) + (0)*i ; 0) with 1/2
tate_fe char(q=1031, n=2, s=(0) + (0)*i) with ((-1) + (0)*i ; 0) with 20
tate_fe char(q=1, n=1, s=(0) + (0)*i) with 2 with 20
