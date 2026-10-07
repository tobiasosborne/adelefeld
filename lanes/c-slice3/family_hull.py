"""Certified continuous-family extrema. The finite candidate proof is in api-3d Slice c.

Only exact rational inputs and python-flint interval arithmetic are used. Comparisons
use exact dyadic interval ends. An uncertain inclusion contributes an outer bound,
never an attained witness. No mpmath optimization or grid sampling is used.
"""
from fractions import Fraction as F
import flint
import char_checks as ref


def ball(x):
    x = F(x)
    return flint.arb(flint.fmpq(x.numerator, x.denominator))


def minimum(x, y):
    return (x+y-abs(x-y))/2


def maximum(x, y):
    return (x+y+abs(x-y))/2


def rounded(x, up):
    x = F(x) * 10**60
    return str(-((-x.numerator)//x.denominator) if up else x.numerator//x.denominator)


def certified_hull(params, phases):
    t, tr, a, ar, b, br = params
    L, H = ball(t-tr).log(), ball(t+tr).log()
    pi = flint.arb.pi()
    pi_lo, pi_hi = ref.arb_interval(pi)
    L_lo, L_hi = ref.arb_interval(L)
    H_lo, H_hi = ref.arb_interval(H)
    # This bound covers endpoint angles and the atan2 stationary-angle shift.
    angle_bound = 2*pi_hi + max(abs(b-br), abs(b+br))*max(abs(L_lo), abs(H_hi))
    q = angle_bound/pi_lo
    K = -((-q.numerator)//q.denominator)+3
    extrema = []
    for imaginary in (False, True):
        candidates = []
        def add(value, attained=True):
            lo, hi = ref.arb_interval(value)
            candidates.append((lo, hi, attained))
        for theta in phases:
            phi = 2*pi*ball(theta)
            root = ref.phase_acb(theta) if theta.denominator not in (1, 2, 4) else (
                flint.acb(1), flint.acb(0, 1), flint.acb(-1), flint.acb(0, -1))[int(theta*4)]
            for av in sorted({a-ar, a+ar}):
                A = ball(av)
                for bv in sorted({b-br, b+br}):
                    B = ball(bv)
                    for l in (L, H):
                        power = (flint.acb(A, B)*l).exp()
                        z = power*root
                        add(z.imag if imaginary else z.real)
                    if bv == 0:
                        continue
                    alpha = flint.arb.atan2(-B, A) if imaginary else flint.arb.atan2(A, B)
                    coefficient = (-B if imaginary else B)/(A*A+B*B).sqrt()
                    for k in range(-K, K+1):
                        l = (alpha+k*pi-phi)/B
                        lo, hi = ref.arb_interval(l)
                        if hi < L_lo or lo > H_hi:
                            continue
                        attained = lo >= L_hi and hi <= H_lo
                        add((A*l).exp()*coefficient*(-1 if k % 2 else 1), attained)
                # Interior b extrema have cardinal phase. Split l at zero so the
                # ordering of b0*l and b1*l is fixed; feasibility is two linear inequalities.
                for positive in (False, True):
                    domain_lo = maximum(L, flint.arb(0)) if positive else L
                    domain_hi = H if positive else minimum(H, flint.arb(0))
                    b0, b1 = (b-br, b+br) if positive else (b+br, b-br)
                    for k in range(-K, K+1):
                        phase_multiple = F(k)+F(1, 2) if imaginary else F(k)
                        w_coeff = phase_multiple-2*theta
                        w = pi*ball(w_coeff)
                        lo, hi = domain_lo, domain_hi
                        possible = True
                        # b0*l <= w <= b1*l. Rational coefficient signs are exact.
                        for coefficient_b, lower in ((b0, True), (b1, False)):
                            if coefficient_b == 0:
                                if (lower and w_coeff < 0) or (not lower and w_coeff > 0):
                                    possible = False
                            else:
                                endpoint = w/ball(coefficient_b)
                                if (coefficient_b > 0) == lower:
                                    hi = minimum(hi, endpoint)
                                else:
                                    lo = maximum(lo, endpoint)
                        if not possible:
                            continue
                        lo_lo, lo_hi = ref.arb_interval(lo)
                        hi_lo, hi_hi = ref.arb_interval(hi)
                        if lo_lo > hi_hi:
                            continue
                        attained = lo_hi <= hi_lo
                        sign = -1 if k % 2 else 1
                        add(sign*(A*lo).exp(), attained)
                        add(sign*(A*hi).exp(), attained)
        witnesses = [c for c in candidates if c[2]]
        assert witnesses
        # Outer ends may use uncertain candidates. Inner ends only use attained values.
        low = (min(c[0] for c in candidates), min(c[1] for c in witnesses))
        high = (max(c[0] for c in witnesses), max(c[1] for c in candidates))
        for lo, hi in (low, high):
            assert lo <= hi
            bounds = [rounded(lo, False), rounded(hi, True)]
            assert int(bounds[1])-int(bounds[0]) <= 2, bounds
            extrema.append(bounds)
    return extrema
