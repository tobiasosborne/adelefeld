#!/usr/bin/env python3
"""Independent local-zeta value oracle and proposed status simulator.

Reference points: mpmath, 320 decimal digits. Certified point errors: scalar arb
at 2048 bits, with our own complex assembly. Gamma uses an integrated Taylor
polynomial plus proved tails, never an acb/arb Gamma value as its reference.
acb is used only for the real-place implementation simulation, never as a reference.
Proofs, error bounds and scope: docs/design/local-zeta.md Z1-Z9.

expected(place, s_mid, s_rad, prec): place is 'real' or a word prime;
s_mid=(re,im); s_rad=(rx,ry), or a scalar for a square. Coordinates accept
integers, Fraction, or decimal/rational strings. Midpoints must be dyadic.
The returned input radii include arb's outward 30-bit radius rounding.
OK returns value, point_error, derivative_bound, variation_bound and image_bound.
All these bounds except value are outward arb scalars. value is an mp.mpc.
--fixtures writes JSONL with exact rational inputs and outward dyadic bounds.
"""

import argparse
import json
from fractions import Fraction as F
from functools import lru_cache
from math import factorial

import mpmath as mp
from flint import acb, arb, ctx, fmpz

ctx.threads = 1
mp.mp.dps = 320
CERT_BITS = 2048
PREC_MAX = 2097152
RECURRENCE_MAX = 64
T_CUT = 512
T_DEG = 2200
PRIMES = (2, 3, 5, 7, 65537, 2**64 - 59)
JS = (1, 4, 12, 30, 60)


def q(v):
    if isinstance(v, F):
        return v
    if isinstance(v, (int, str)):
        return F(v)
    raise TypeError("use exact int, Fraction, or string inputs")


def dyadic(v):
    v = q(v)
    if v.denominator & (v.denominator - 1):
        raise ValueError("an exact midpoint must be dyadic")
    return v


def aa(v):
    v = q(v)
    return arb(v.numerator) / v.denominator


def aq(v):
    """Exact dyadic represented by an arb midpoint/radius/endpoint."""
    a, b = v.man_exp()
    return F(int(a) * 2**int(b)) if b >= 0 else F(int(a), 2**(-int(b)))


def am(v):
    sign, man, ex, _ = v._mpf_
    return arb((-man if sign else man, ex))


def mm(v):
    v = q(v)
    return mp.mpf(v.numerator) / v.denominator


def rounded_dyadic(v, bits=900):
    return F(int(mp.nint(v * mp.mpf(2)**bits)), 2**bits)


def prime(p):
    """Input validation only: refs/src/flint-3.0.1/fmpz.rst:1529-1534."""
    if not isinstance(p, int) or not 2 <= p < 2**64:
        return False
    return bool(fmpz(p).is_prime())


def cadd(z, w):
    return z[0] + w[0], z[1] + w[1]


def cneg(z):
    return -z[0], -z[1]


def cmul(z, w):
    return z[0]*w[0]-z[1]*w[1], z[0]*w[1]+z[1]*w[0]


def square(t):
    # python-flint's ** uses general arb_pow, whose logarithm rejects negative bases.
    lo, hi = t.abs_lower(), t.abs_upper()
    lo, hi = lo*lo, hi*hi
    center = (lo+hi)/2
    return arb(center.mid(), (((hi-lo)/2).upper()+center.rad()).upper())


def cdiv(z, w):
    den = square(w[0]) + square(w[1])
    return (z[0]*w[0]+z[1]*w[1])/den, (z[1]*w[0]-z[0]*w[1])/den


def cexp(z):
    ex = z[0].exp()
    return ex*z[1].cos(), ex*z[1].sin()


def cexpm1(z):
    # exp(u)cos(v)-1 = expm1(u)cos(v)-2 sin(v/2)^2.
    return z[0].expm1()*z[1].cos()-2*square((z[1]/2).sin()), z[0].exp()*z[1].sin()


def norm(z):
    a, b = z[0].abs_upper(), z[1].abs_upper()
    return (a*a+b*b).sqrt().upper()


def add_error(z, e):
    return tuple(arb(t.mid(), (t.rad()+e).upper()) for t in z)


def contains_zero(z):
    return 0 in z[0] and 0 in z[1]


def finite(z):
    return all(t.is_finite() for t in z)


def distance_rect(z):
    return (z[0].abs_lower()**2+z[1].abs_lower()**2).sqrt().lower()


def normalize(mid, rad):
    x, y = map(dyadic, mid)
    if isinstance(rad, (tuple, list)):
        rx, ry = map(q, rad)
    else:
        rx = ry = q(rad)
    if min(rx, ry) < 0:
        raise ValueError("negative radius")
    # Construct at precision sufficient to preserve the exact input midpoints.
    with ctx.workprec(CERT_BITS):
        sx, sy = arb(aa(x), aa(rx)), arb(aa(y), aa(ry))
        assert aq(sx.mid()) == x and aq(sy.mid()) == y
        return (x, y), (aq(sx.rad()), aq(sy.rad()))


def real_pole_possible(mid, rad):
    x, y = mid
    rx, ry = rad
    if not y-ry <= 0 <= y+ry or x-rx > 0:
        return False
    lo, hi = (x-rx)/2, min(F(0), (x+rx)/2)
    ceil_lo = -((-lo.numerator)//lo.denominator)
    return ceil_lo <= hi.numerator//hi.denominator


def finite_certificate(p, mid, rad, prec):
    with ctx.workprec(max(2, prec)+32):
        x, y = mid
        rx, ry = rad
        a = arb(p).log()
        b = -a if x >= 0 else a
        z = aa(x)*b, aa(y)*b
        t0, d0 = cexp(z), cneg(cexpm1(z))
        # At huge uncertain phases expm1's scalar assembly loses correlation.
        # Intersect with 1-exp: both enclose the same exact midpoint denominator.
        direct = 1-t0[0], -t0[1]
        if finite(d0):
            d0 = tuple(d0[i].intersection(direct[i]) for i in (0, 1))
        else:
            d0 = direct
        r = (aa(rx)**2+aa(ry)**2).sqrt().upper()
        e = ((-a*aa(abs(x))).exp()*(a*r).expm1()).upper()
        if not e.is_finite() or not finite(d0):
            return None
        d, t = add_error(d0, e), add_error(t0, e)
        if contains_zero(d):
            return None
        out = cdiv((arb(1), arb(0)), d) if x >= 0 else cdiv(cneg(t), d)
        if not finite(out):
            return None
        # Scalar assembly is a simulator of the prescribed enclosure, not an acb value oracle.
        return d, out


def real_candidate(mid, rad, prec):
    """Only the finite/non-finite predicate of the prospective acb evaluator.

    Neither references nor derivative bounds use this value.
    This is a FLINT status simulation, not independent evidence of Gamma accuracy.
    """
    with ctx.workprec(max(2, prec)+32):
        z = acb(arb(aa(mid[0]), aa(rad[0])), arb(aa(mid[1]), aa(rad[1]))) / 2
        factor = (-z*arb.pi().log()).exp()
        value = z.gamma()*factor
        if not value.is_finite():
            lo = (mid[0]-rad[0])/2
            n = max(0, 1-lo.numerator//lo.denominator)
            if n > RECURRENCE_MAX:
                return 'LIMIT', None
            product = acb(1)
            for j in range(n):
                product *= z+j
            value = (z+n).gamma()/product*factor
        if not value.is_finite():
            return 'NOT_DETERMINED', None
        return 'OK', (value.real, value.imag)


@lru_cache(maxsize=8192)
def point_enclosure(place, x, y):
    """Independent rigorous reference rectangle, at CERT_BITS.

    Gamma's positive-half-plane Mellin integral is integrated after expanding exp(-t).
    Taylor remainder and infinite tail are Z8. Recurrence moves x/2 to [1,2).
    """
    with ctx.workprec(CERT_BITS):
        z = aa(x), aa(y)
        if place != 'real':
            if abs(y) > 2**1024:
                raise ValueError("certified finite oracle range: |Im(s)|<=2^1024")
            a = arb(place).log()
            if x >= 0:
                return cdiv((arb(1), arb(0)), cneg(cexpm1((-a*z[0], -a*z[1]))))
            t = cexp((a*z[0], a*z[1]))
            return cdiv(t, cexpm1((a*z[0], a*z[1])))
        if not (-64 <= x <= 64 and abs(y) <= 64):
            raise ValueError("certified Gamma oracle range: -64<=Re(s)<=64, |Im(s)|<=64")
        zz = z[0]/2, z[1]/2
        n = max(0, 1-(x/2).numerator//(x/2).denominator)
        w = zz[0]+n, zz[1]
        a = aa(x/2+n)
        # For positive x>4 we still allow a>2; the same tail proof works for a<T_CUT.
        coeff = arb(1)
        series = cdiv((coeff, arb(0)), w)
        for j in range(1, T_DEG+1):
            coeff = -coeff*T_CUT/j
            series = cadd(series, cdiv((coeff, arb(0)), (w[0]+j, w[1])))
        tw = cexp((w[0]*arb(T_CUT).log(), w[1]*arb(T_CUT).log()))
        truncated = cmul(tw, series)
        next_coeff = abs(coeff)*T_CUT/(T_DEG+1)
        polynomial_tail = arb(T_CUT)**a * next_coeff/(a+T_DEG+1)
        integral_tail = arb(-T_CUT).exp()*arb(T_CUT)**(a-1)/(1-(a-1)/T_CUT)
        tail = (polynomial_tail+integral_tail).upper()
        g = add_error(truncated, tail)
        if y == 0:
            # The Mellin integral and recurrence are real on a regular real point.
            g = g[0], arb(0)
        product = arb(1), arb(0)
        for j in range(n):
            product = cmul(product, (zz[0]+j, zz[1]))
        g = cdiv(g, product)
        factor = cexp((-zz[0]*arb.pi().log(), -zz[1]*arb.pi().log()))
        return cmul(factor, g)


def value_mp(place, mid):
    y = mid[1]
    phase_bits = max(0, y.numerator.bit_length()-y.denominator.bit_length()+1)
    with mp.workdps(320+(phase_bits+2)//3):
        z = mp.mpc(mm(mid[0]), mm(mid[1]))
        if place == 'real':
            return mp.exp(-z*mp.log(mp.pi)/2)*mp.gamma(z/2)
        a = mp.log(place)
        return -1/mp.expm1(-a*z) if mid[0] >= 0 else mp.exp(a*z)/mp.expm1(a*z)


def certified_reference(place, mid):
    v = value_mp(place, mid)
    box = point_enclosure(place, *mid)
    err = norm((box[0]-am(v.real), box[1]-am(v.imag))).upper()
    allowance = (arb(10)**-200 * max(arb(1), norm((am(v.real), am(v.imag))).upper())).upper()
    assert err <= allowance, ('reference margin', place, mid, err, allowance)
    return v, err, box


def derivative_bound(place, mid, rad, d=None):
    with ctx.workprec(CERT_BITS):
        x, y = mid
        rx, ry = rad
        if place != 'real':
            a = arb(place).log()
            ell = distance_rect(d)
            assert ell > 0
            u = (-a*aa(abs(x))+a*aa(rx)).exp()
            return (a*u/ell**2).upper()
        z = arb(aa(x/2), aa(rx/2)), arb(aa(y/2), aa(ry/2))
        lo, hi = (x-rx)/2, (x+rx)/2
        n = max(0, 1-lo.numerator//lo.denominator)
        a, b = aa(lo+n), hi+n
        ceil_b = -((-b.numerator)//b.denominator)
        m0, m1 = 1/a+factorial(ceil_b-1), 1/a**2+factorial(ceil_b)
        product, reciprocal_sum = arb(1), arb(0)
        for j in range(n):
            delta = distance_rect((z[0]+j, z[1]))
            assert delta > 0
            product *= delta
            reciprocal_sum += 1/delta
        return ((-aa(x-rx)*arb.pi().log()/2).exp()
                *(m1+m0*(arb.pi().log()+reciprocal_sum))/(2*product)).upper()


def expected(place, s_mid, s_rad, prec):
    """Status model and independently certified OK values for normalized dyadic boxes.

    The status model includes the documented acb finite-result check at infinity.
    Scalar exponential assembly may differ by a last radius bit from a C acb call;
    fixtures away from that boundary are definitive. Boundary cases return the
    certificate data so the implementation tests the inequality, not last-bit identity.
    """
    if not isinstance(prec, int):
        raise TypeError("prec must be int")
    if prec > PREC_MAX:
        return dict(status='LIMIT', where=place)
    if place != 'real' and not prime(place):
        return dict(status='DOMAIN', where=None)
    mid, rad = normalize(s_mid, s_rad)
    out = dict(place=place, s_mid=mid, s_rad=rad, prec=prec, where=place)
    if not any(rad) and ((place != 'real' and mid == (0, 0)) or
                       (place == 'real' and mid[1] == 0 and mid[0] <= 0
                        and (mid[0]/2).denominator == 1)):
        return dict(out, status='DOMAIN')
    d = None
    if place == 'real':
        if real_pole_possible(mid, rad):
            return dict(out, status='NOT_DETERMINED')
        status, simulation = real_candidate(mid, rad, prec)
        if status != 'OK':
            return dict(out, status=status)
        out['simulation'] = simulation
    else:
        cert = finite_certificate(place, mid, rad, prec)
        if cert is None:
            return dict(out, status='NOT_DETERMINED')
        d, simulation = cert
        out['certificate'] = d
        out['simulation'] = simulation
    with ctx.workprec(CERT_BITS):
        v, err, box = certified_reference(place, mid)
        b = derivative_bound(place, mid, rad, d)
        r = (aa(rad[0])**2+aa(rad[1])**2).sqrt().upper()
        variation = (r*b).upper()
        if place == 'real' and any(rad):
            # Refine the candidate with an acb MIDPOINT evaluation and the independently
            # proved derivative bound. The certified reference value is never used here.
            lo, hi = (mid[0]-rad[0])/2, (mid[0]+rad[0])/2
            n = max(0, 1-lo.numerator//lo.denominator)
            if n <= RECURRENCE_MAX and hi+n <= 64:
                midpoint_status, midpoint_box = real_candidate(mid, (F(0), F(0)), prec)
                if midpoint_status == 'OK':
                    with ctx.workprec(max(2, prec)+32):
                        refined = add_error(midpoint_box, variation)
                        out['simulation'] = tuple(out['simulation'][i].intersection(refined[i])
                                                  for i in (0, 1))
        return dict(out, status='OK', where=None, value=v, point_error=err, point_box=box,
                    derivative_bound=b, variation_bound=variation,
                    image_bound=(norm((am(v.real), am(v.imag)))+err+variation).upper())


def sample_points(mid, rad):
    x, y = mid
    rx, ry = rad
    return sorted({(x+rx*i, y+ry*j) for i in (-1, 0, 1) for j in (-1, 0, 1)})


def separation_lower(v1, v2):
    with ctx.workprec(CERT_BITS):
        diff = v1[0]-v2[0], v1[1]-v2[1]
        return distance_rect(diff)


def width_witness(place, mid, rad):
    pts = sample_points(mid, rad)
    boxes = [point_enclosure(place, *t) for t in pts]
    return max((separation_lower(boxes[i], boxes[j])
                for i in range(len(boxes)) for j in range(i)), default=arb(0))


def serialize(result, width=None, sampled=None):
    def bound(t):
        # Explicit dyadic endpoints avoid enormous power-of-two denominators and decimal rounding.
        with ctx.workprec(CERT_BITS):
            man, ex = t.upper().man_exp()
            return dict(man=str(man), exp=int(ex))
    row = {k: v for k, v in result.items()
           if k in ('place', 'status', 'where', 'prec')}
    if 's_mid' in result:
        row['s_mid'] = list(map(str, result['s_mid']))
        row['s_rad'] = list(map(str, result['s_rad']))
    if result['status'] == 'OK':
        row['value'] = [mp.nstr(result['value'].real, 230), mp.nstr(result['value'].imag, 230)]
        # Printing value loses at most 1e-229 max(1,abs(value)); account for it explicitly.
        scale = max(arb(1), norm((am(result['value'].real), am(result['value'].imag))).upper())
        row['point_error'] = bound(result['point_error']+scale*arb(10)**-229)
        for k in ('derivative_bound', 'variation_bound', 'image_bound'):
            row[k] = bound(result[k])
        row['width_lower'] = bound(width or arb(0))
        row['width_factor'] = 64
        row['samples'] = []
        for pt, value, error in sampled or []:
            scale = max(arb(1), norm((am(value.real), am(value.imag))).upper())
            row['samples'].append(dict(s=list(map(str, pt)),
                                      value=[mp.nstr(value.real, 230), mp.nstr(value.imag, 230)],
                                      point_error=bound(error+scale*arb(10)**-229)))
    return row


def check_poles():
    count = 0
    with ctx.workprec(CERT_BITS):
        for p in PRIMES:
            for k in range(-3, 4):
                pole = 2*mp.pi*k/mp.log(p)
                assert abs(mp.expm1(-mp.mpc(0, pole)*mp.log(p))) < mp.mpf('1e-230')
                h = mp.mpf('1e-80')
                pt = rounded_dyadic(h), rounded_dyadic(pole)
                v, _, box = certified_reference(p, pt)
                residue = 1/mp.log(p)
                assert abs(h*v-residue) < mp.mpf('1e-70')*max(1, abs(residue))
                exact_residue = 1/arb(p).log()
                difference = cadd(cmul((aa(pt[0]), arb(0)), box), (-exact_residue, arb(0)))
                assert norm(difference) <= arb(10)**-70*max(arb(1), abs(exact_residue).upper())
                count += 1
        for n in range(21):
            h = mp.mpf('1e-80')
            pt = rounded_dyadic(-2*n+h), F(0)
            v, _, box = certified_reference('real', pt)
            residue = 2*(-1)**n*mp.pi**n/mp.factorial(n)
            assert abs(h*v-residue) < mp.mpf('1e-70')*max(1, abs(residue))
            exact_residue = 2*(-1)**n*arb.pi()**n/factorial(n)
            difference = cadd(cmul((aa(pt[0]+2*n), arb(0)), box), (-exact_residue, arb(0)))
            assert norm(difference) <= arb(10)**-70*max(arb(1), abs(exact_residue).upper())
            count += 1
    print(f'poles_and_residues: {count}; residual 1e-230, residue error <1e-70 max(1,|residue|)')
    return count


def cases():
    for p in PRIMES:
        yield p, (F(0), F(0)), (F(0), F(0)), 128, 'DOMAIN', 'exact'
        for k in range(-3, 4):
            y = rounded_dyadic(2*mp.pi*k/mp.log(p))
            for j in JS:
                r = F(1, 10**j)
                yield p, (F(0), y), (r, r), 256, 'NOT_DETERMINED', 'around'
                # Real displacement alone proves pole-freedom. R/d is about sqrt(2)/16.
                x = rounded_dyadic(mm(16*r))
                yield p, (x, y), (r, r), 256, 'OK', 'near'
                yield p, (-x, y), (r, r), 256, 'OK', 'near_negative'
                # Vertical segment crossing a nonzero pole has the same mixed-domain status.
                yield p, (F(0), y), (F(0), r), 256, 'NOT_DETERMINED', 'segment'
    for n in range(21):
        yield 'real', (F(-2*n), F(0)), (F(0), F(0)), 128, 'DOMAIN', 'exact'
        for j in JS:
            r = F(1, 10**j)
            yield 'real', (F(-2*n), F(0)), (r, r), 256, 'NOT_DETERMINED', 'around'
            x = rounded_dyadic(mm(F(-2*n)+16*r))
            yield 'real', (x, F(0)), (r, r), 256, 'OK', 'near'
            y = rounded_dyadic(mm(16*r))
            yield 'real', (F(-2*n), y), (r, r), 256, 'OK', 'near_imaginary'
            yield 'real', (F(-2*n), F(0)), (r, F(0)), 256, 'NOT_DETERMINED', 'segment'
    for p in PRIMES:
        for x, y in ((1, 0), (2, 0), (-1, 0), (1, 1), (-1, 1), (1000, 1), (-1000, 1)):
            yield p, (F(x), F(y)), (F(0), F(0)), 128, 'OK', 'regular'
        # Pole-free vertical box. The disk cover reaches the imaginary-axis pole lattice.
        yield p, (F(1, 1024), F(1)), (F(0), F(10)), 128, 'NOT_DETERMINED', 'wide_free'
        for x in (1, -1):
            yield p, (F(x), F(1)), (F(1, 1024), F(1, 1024)), 128, 'OK', 'regular_ball'
        yield p, (F(0), F(1)), (F(0), F(0)), 128, 'OK', 'imaginary_point'
    for x, y in ((1, 0), (2, 0), (4, 0), (-1, 0), (-3, 0), (1, 1), (-2, 1)):
        yield 'real', (F(x), F(y)), (F(0), F(0)), 128, 'OK', 'regular'
    yield 2, (F(0), F(0)), (F(0), F(0)), PREC_MAX+1, 'LIMIT', 'limit_first'
    for p in (0, 1, 4, 9, 65535):
        yield p, (F(1), F(0)), (F(0), F(0)), 128, 'DOMAIN', 'invalid_prime'
    yield 'real', (F(-130), F(1, 4)), (F(1), F(1, 10)), 256, 'LIMIT', 'recurrence_limit'
    yield 'real', (F(1), F(1)), (F(1, 1024), F(1, 1024)), 128, 'OK', 'regular_ball'
    for place, mid, rad in ((2, (1, 0), (1, 0)), (2, (-1, 0), (1, 0)),
                            ('real', (1, 0), (1, 0)), ('real', (-1, 0), (1, 0)),
                            ('real', (-2, 1), (0, 1))):
        yield place, tuple(map(F, mid)), tuple(map(F, rad)), 128, 'NOT_DETERMINED', 'closed_boundary'
    for x in (2**1000, -2**1000):
        yield 2, (F(x), F(0)), (F(0), F(0)), 128, 'OK', 'huge_real'
    yield 2, (F(1), F(2**1000)), (F(0), F(0)), 128, 'OK', 'huge_imaginary'
    yield 2, (F(0), F(2**1000)), (F(0), F(0)), 128, 'NOT_DETERMINED', 'huge_imaginary'


def check_cases(fixture_path=None):
    counts, statuses, samples, quality = {}, {}, 0, 0
    handle = open(fixture_path, 'w', encoding='ascii') if fixture_path else None
    try:
        for place, mid, rad, prec, wanted, kind in cases():
            result = expected(place, mid, rad, prec)
            assert result['status'] == wanted, (place, mid, rad, prec, wanted, result['status'])
            counts[kind] = counts.get(kind, 0)+1
            statuses[wanted] = statuses.get(wanted, 0)+1
            width, sampled = arb(0), []
            if kind in ('around', 'segment') and place != 'real':
                # Rigorous membership of the actual irrational pole, not its mp approximation.
                with ctx.workprec(CERT_BITS):
                    k = int(mp.nint(mm(result['s_mid'][1])*mp.log(place)/(2*mp.pi)))
                    pole_y = 2*k*arb.pi()/arb(place).log()
                    imag_box = arb(aa(result['s_mid'][1]), aa(result['s_rad'][1]))
                    assert imag_box.contains(pole_y), ('pole membership', place, mid, rad)
            if wanted == 'OK':
                with ctx.workprec(CERT_BITS):
                    for pt in sample_points(result['s_mid'], result['s_rad']):
                        v, err, box = certified_reference(place, pt)
                        gap = norm((am(v.real-result['value'].real), am(v.imag-result['value'].imag)))
                        assert gap <= result['variation_bound']+err+result['point_error'], (
                            'variation', place, mid, rad, pt)
                        samples += 1
                        sampled.append((pt, v, err))
                        assert all(result['simulation'][i].contains(box[i]) for i in (0, 1)), (
                            'simulated sample enclosure', place, mid, rad, pt)
                    if any(result['s_rad']):
                        width = width_witness(place, result['s_mid'], result['s_rad'])
                        assert width > 0, ('missing image-width witness', place, mid, rad)
                        out = result['simulation']
                        diameter = 2*norm((out[0].rad(), out[1].rad()))
                        assert diameter <= 64*width, ('quality target', place, mid, rad, diameter, width)
                        quality += 1
                    if place != 'real':
                        ref = result['point_box']
                        # Actual containment, not only comparison of two approximate midpoints.
                        assert all(result['simulation'][i].contains(ref[i]) for i in (0, 1)), (
                            'finite simulation enclosure', place, mid, rad)
            if handle:
                handle.write(json.dumps(dict(serialize(result, width, sampled), kind=kind), sort_keys=True)+'\n')
    finally:
        if handle:
            handle.close()
    print('case_groups:', json.dumps(counts, sort_keys=True))
    print('statuses:', json.dumps(statuses, sort_keys=True))
    print(f'certified_samples: {samples}; positive image-width witnesses: {quality}; width factor: 64')
    return counts, statuses, samples, quality


def check_precision():
    low, high = 0, 0
    for p in PRIMES:
        y = rounded_dyadic(2*mp.pi/mp.log(p))
        r = F(1, 2**180)
        # Same pole-free rectangle, changing only working precision.
        for prec, wanted in ((16, 'NOT_DETERMINED'), (256, 'OK')):
            ans = expected(p, (F(0), y+16*r), (r, r), prec)
            assert ans['status'] == wanted, ('precision', p, prec, ans['status'])
            low += prec == 16
            high += prec == 256
    print(f'precision_pairs: {low+high}; low-precision refusals: {low}; high-precision OK: {high}')
    return low+high


def check_gamma_integral():
    count = 0
    with ctx.workprec(CERT_BITS):
        for x, y in ((2, 0), (4, 0), (6, 0), (1, 0), (3, 2), (-1, 1), (-40, 1)):
            _, _, box = certified_reference('real', (F(x), F(y)))
            assert finite(box)
            if x == 2 and y == 0:
                assert box[0].contains(1/arb.pi())
            count += 1
    print(f'independent_gamma_integral: {count}; certified reference margin <=1e-200 max(1,|value|)')
    return count


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixtures')
    parser.add_argument('--quick', action='store_true', help='pole/residue and Gamma-integral checks only')
    args = parser.parse_args()
    check_poles()
    check_gamma_integral()
    if not args.quick:
        check_precision()
        check_cases(args.fixtures)


if __name__ == '__main__':
    main()
