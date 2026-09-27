#!/usr/bin/env python3
"""Independent numerical checks of docs/proofs/analysis.md; no production code.

Definitions and formulas are derived in that proof file. External analytic theorems
are explicitly source-pending there. mpmath rounding is not a certified enclosure.
"""
import argparse
import math
import sys
import time
from fractions import Fraction

import mpmath as mp

mp.mp.dps = 55
TOL = mp.mpf('1e-32')
COUNTS = {}
ERRORS = {}


def verify(name, condition, error=0):
    COUNTS[name] = COUNTS.get(name, 0) + 1
    ERRORS[name] = max(ERRORS.get(name, mp.mpf(0)), abs(error))
    if not condition:
        raise AssertionError(f'{name}: assertion {COUNTS[name]}, error={error}')


def close(name, actual, expected, tol=TOL):
    error = abs(actual - expected) / max(1, abs(expected))
    verify(name, error < tol, error)


def phase(x):
    return mp.exp(2j * mp.pi * x)


def finite_transform(values, D, M):
    L = D * M
    return [sum(values[j] * phase(-mp.mpf(j*k)/L) for j in range(L)) / M
            for k in range(L)]


def gaussian_transform(coeffs, A, B, C, y):
    z = B + 2j * mp.pi * y
    H = [mp.mpc(1)]
    result = 0
    for coeff in coeffs:
        result += coeff * sum(value * z**k for k, value in enumerate(H))
        derivative = [(k+1)*H[k+1] for k in range(len(H)-1)] + [0, 0]
        shifted = [0] + [value/(2*mp.pi*A) for value in H]
        H = [a+b for a, b in zip(derivative, shifted)]
    return A**(-mp.mpf('.5')) * mp.exp(C + z*z/(4*mp.pi*A)) * result


def check_finite_fourier():
    name = 'check_finite_fourier'
    for D, M in [(2, 3), (3, 2), (1, 5), (4, 1), (1, 1)]:
        values = [mp.mpc(j * j - 2, 3 * j + 1) for j in range(D * M)]
        result = finite_transform(values, D, M)
        # Independent integral: refine each M-coset into 7 equal-volume pieces.
        for k in range(D * M):
            direct = sum(values[j] * phase(-mp.mpf(j + D * M * r) * k / (D * M))
                         for j in range(D * M) for r in range(7)) / (7 * M)
            close(name, result[k], direct)
        close(name, result[0], sum(values) / M)
        back = finite_transform(result, M, D)
        for j in range(D * M):
            close(name, back[j], values[-j % (D * M)])
        close(name, sum(abs(v)**2 for v in values) / M,
              sum(abs(v)**2 for v in result) / D)
        if D * M > 2:
            wrong_sign = sum(values[j] * phase(mp.mpf(j) / (D * M))
                             for j in range(D * M)) / M
            verify(name, abs(result[1] - wrong_sign) > mp.mpf('.01'))


def check_real_transform():
    name = 'check_real_transform'
    for coeffs, A, B, C in [([1], mp.mpf('1.3'), mp.mpf('1.7'), mp.mpf('-.2')),
                            ([2, -1, 3], mp.mpc('1.2', '.2'), mp.mpc('.4', '.7'), 0)]:
        for y in [mp.mpf('-.7'), 0, mp.mpf('.4')]:
            direct = mp.quad(lambda x: sum(c * x**j for j, c in enumerate(coeffs))
                             * mp.exp(-mp.pi * A*x*x + B*x + C) * phase(x*y),
                             [-mp.inf, -1, 0, 1, mp.inf])
            close(name, gaussian_transform(coeffs, A, B, C, y), direct)
        plus = gaussian_transform(coeffs, A, B, C, mp.mpf('.4'))
        minus = gaussian_transform(coeffs, A, B, C, mp.mpf('-.4'))
        verify(name, abs(plus - minus) > mp.mpf('.01'))
        x = mp.mpf('.3')
        twice = mp.quad(lambda y: gaussian_transform(coeffs,A,B,C,y)*phase(x*y),
                        [-mp.inf,-1,0,1,mp.inf])
        reflected = sum(c*(-x)**j for j,c in enumerate(coeffs))*mp.exp(-mp.pi*A*x*x-B*x+C)
        close(name,twice,reflected)


def valuation(n, p):
    if n == 0:
        return math.inf
    k = 0
    while n % p == 0:
        n //= p
        k += 1
    return k


def fractional_part(q, p):
    q = Fraction(q)
    k = valuation(q.denominator, p)
    if not k:
        return Fraction(0)
    pk = p**k
    return Fraction((q.numerator * pow(q.denominator // pk, -1, pk)) % pk, pk)


def check_characters():
    name = 'check_characters'
    primes = [2, 3, 5, 7, 11, 13]
    for den in range(1, 14):
        for num in range(-17, 18):
            q = Fraction(num, den)
            parts = sum((fractional_part(q, p) for p in primes), Fraction(0))
            verify(name, (q-parts).denominator == 1)
            for p in primes[:4]:
                r = Fraction(5, 12)
                delta = fractional_part(q+r, p)-fractional_part(q, p)-fractional_part(r, p)
                verify(name, delta.denominator == 1)
    for a in [Fraction(-7, 6), Fraction(0), Fraction(2, 5)]:
        for N in [Fraction(0), Fraction(1), Fraction(3), Fraction(1, 2), Fraction(5, 6)]:
            B = N.denominator
            observed = {(a+N*k) % 1 for k in range(B)}
            expected = {(a+Fraction(k, B)) % 1 for k in range(B)}
            verify(name, observed == expected)
    close(name, phase(mp.mpf(1)/3), mp.mpc(-mp.mpf('.5'), mp.sqrt(3)/2))
    verify(name, abs(phase(mp.mpf(2)/3)-1) > 1)  # wrong single-sign rational phase


def check_local_fourier():
    name = 'check_local_fourier'
    for p in [2, 3, 5]:
        for k in [-2, -1, 0, 1, 2]:
            for v in [-k-2, -k-1, -k, -k+1]:
                ell = max(0, -k-v)
                q = p**ell
                direct = mp.mpf(p)**(-k) * sum(phase(mp.mpf(r)/q) for r in range(q))/q
                expected = mp.mpf(p)**(-k) if k+v >= 0 else 0
                close(name, direct, expected)
    # An outside-support frequency cancels on a refinement of every coset.
    for D, M in [(2, 3), (3, 2)]:
        L = D*M
        vals = [mp.mpc(j+1, 2-j) for j in range(L)]
        outside = sum(vals[j]*phase(-mp.mpf(j+L*r)/(2*L))
                      for j in range(L) for r in range(2))/(2*M)
        close(name, outside, 0)
        for k in [-3, 1, 4]:
            basic = sum(vals[j]*phase(-mp.mpf(j*k)/L) for j in range(L))/M
            shifted = sum(vals[j]*phase(-mp.mpf(j*(k+L))/L) for j in range(L))/M
            close(name, basic, shifted)


def poly(coeffs, x):
    return sum(c*x**j for j, c in enumerate(coeffs))


def check_real_closure():
    name = 'check_real_closure'
    A, B, C = mp.mpc('1.2', '.3'), mp.mpc('.7', '.2'), mp.mpf('-.3')
    coeffs = [1, -2, 3]
    f = lambda x: poly(coeffs, x)*mp.exp(-mp.pi*A*x*x+B*x+C)
    for x in [-2, 0, mp.mpf('.7')]:
        for shift in [-1, mp.mpf('.4')]:
            new = poly(coeffs, x-shift)*mp.exp(-mp.pi*A*x*x+(B+2*mp.pi*A*shift)*x
                                             +C-B*shift-mp.pi*A*shift**2)
            close(name, new, f(x-shift))
        for h in [-2, mp.mpf('.5')]:
            close(name, poly(coeffs, h*x)*mp.exp(-mp.pi*A*h*h*x*x+B*h*x+C), f(h*x))
        close(name, poly(coeffs, x)**2*mp.exp(-mp.pi*(2*A)*x*x+2*B*x+2*C), f(x)**2)
    for h in [-2, mp.mpf('.5')]:
        y = mp.mpf('.3')
        direct = mp.quad(lambda x: f(h*x)*phase(x*y), [-mp.inf, 0, mp.inf])
        close(name, direct, gaussian_transform(coeffs, A, B, C, y/h)/abs(h))


def series_bound(j, c, T, beta=0):
    K = math.floor(T)+1
    prefix = mp.mpf(0)
    while True:
        rho = mp.exp(mp.mpf(j)/K-c*(2*K+1)+beta)
        term = K**j*mp.exp(-c*K*K+beta*K)
        if rho <= mp.mpf('.5'):
            return prefix+term/(1-rho)
        prefix += term
        K += 1


def integral_bound(r, b, R):
    rp = max(r, 0)
    R = mp.mpf(R)
    if b >= 2*rp/R:
        return 2*R**r*mp.exp(-b*R)/b
    R0 = 2*rp/b
    peak = min(max(r/b, R), R0)
    return (R0-R)*peak**r*mp.exp(-b*peak)+2*R0**r*mp.exp(-b*R0)/b


def shifted_coeffs(coeffs, a, h):
    return [sum(coeffs[j]*math.comb(j,k)*a**(j-k)*h**k for j in range(k,len(coeffs)))
            for k in range(len(coeffs))]


def lattice_bound(coeffs, A, B, C, a, h, T):
    q = shifted_coeffs(coeffs, a, h)
    alpha = mp.pi*mp.re(A*h*h)
    beta = abs(mp.re(h*(B-2*mp.pi*A*a)))
    Cp = C+B*a-mp.pi*A*a*a
    scale = mp.exp(mp.re(Cp))
    return 2*scale*sum(abs(v)*series_bound(j, alpha, T, beta) for j,v in enumerate(q))


def check_tail_bounds():
    name = 'check_tail_bounds'
    for j in range(6):
        for c in [mp.mpf('.07'), mp.mpf('.5'), mp.mpf('2.3')]:
            for T in [0, mp.mpf('2.4'), 9]:
                # Direct positive partial sum is a lower bound for the true tail.
                observed = sum(n**j*mp.exp(-c*n*n) for n in range(math.floor(T)+1, 150))
                verify(name, observed <= series_bound(j,c,T)*(1+mp.mpf('1e-50')))
    for r in [-5, mp.mpf('-.3'), 0, mp.mpf('3.7'), 12]:
        for b in [mp.mpf('.2'), mp.mpf('2.3')]:
            for R in [1, 7]:
                observed = mp.quad(lambda t: t**r*mp.exp(-b*t), [R, mp.inf])
                verify(name, observed <= integral_bound(r,b,mp.mpf(R))*(1+mp.mpf('1e-50')))
    coeffs = [1, -2, mp.mpc('.5','.3')]
    A, B, C = mp.mpc('1.1','.2'), mp.mpc('.8','-.3'), mp.mpf('-.1')
    for a,h in [(mp.mpf('.7'), mp.mpf('.4')), (mp.mpf('-.3'), mp.mpf('-.8'))]:
        for T in [0, mp.mpf('2.5'), 5]:
            observed = sum(abs(poly(coeffs,a+h*n)*mp.exp(-mp.pi*A*(a+h*n)**2+B*(a+h*n)+C))
                           for n in range(-80,81) if abs(n)>T)
            verify(name, observed <= lattice_bound(coeffs,A,B,C,a,h,T))
    # T=0 is close enough to the bound to require both sides of the lattice.
    slow_a = mp.mpf('.2')
    observed = 2*sum(mp.exp(-mp.pi*slow_a*n*n) for n in range(1, 30))
    verify(name, observed < lattice_bound([1],slow_a,0,0,0,1,0))


def check_poisson():
    name = 'check_poisson'
    coeffs, A, B, C = [1, mp.mpc('.3','.2')], mp.mpf('1.1'), mp.mpf('1.4'), 0
    f = lambda x: poly(coeffs,x)*mp.exp(-mp.pi*A*x*x+B*x+C)
    for D,M in [(2,3), (3,2)]:
        vals = [mp.mpc(j-2,j*j+1) for j in range(D*M)]
        g = finite_transform(vals,D,M)
        left = sum(vals[j]*sum(f(mp.mpf(j)/D+M*n) for n in range(-16,17))
                   for j in range(D*M))
        right = sum(g[n%(D*M)]*gaussian_transform(coeffs,A,B,C,mp.mpf(n)/M)
                    for n in range(-48,49))
        close(name, left, right)
        bound = sum(abs(vals[j])*lattice_bound(coeffs,A,B,C,mp.mpf(j)/D,M,4)
                    for j in range(D*M))
        short = sum(vals[j]*sum(f(mp.mpf(j)/D+M*n) for n in range(-4,5))
                    for j in range(D*M))
        # The residual may round to zero; the analytic tail bound is still checked separately.
        verify(name, abs(left-short) <= bound+mp.mpf('1e-50'))


def character(C, exponent=1):
    if C == 1:
        return lambda n: mp.mpc(1)
    if C == 15:
        c3, c5 = character(3), character(5, exponent)
        return lambda n: c3(n)*c5(n)
    if C == 4:
        table = {1:1, 3:-1}
    elif C == 8:
        table = {1:1, 3:-1, 5:-1, 7:1} if exponent == 0 else {1:1, 3:1, 5:-1, 7:-1}
    elif C == 16:
        table = {((-1)**sign*pow(5,j,16))%16:
                 (-1)**sign*phase(mp.mpf(exponent*j)/4)
                 for sign in range(2) for j in range(4)}
    else:
        order = sum(math.gcd(j,C)==1 for j in range(C))
        generator = next(g for g in range(2,C)
                         if math.gcd(g,C)==1 and len({pow(g,k,C) for k in range(order)})==order)
        table = {pow(generator,k,C):phase(mp.mpf(exponent*k)/order) for k in range(order)}
    return lambda n: mp.mpc(table.get(n % C, 0))


def parity(chi):
    return 0 if abs(chi(-1)-1)<mp.mpf('1e-45') else 1


def gauss(chi,C,sign=1):
    return sum(chi(a)*phase(mp.mpf(sign*a)/C) for a in range(C))


def check_gauss_sums():
    name = 'check_gauss_sums'
    for C,k in [(1,0),(3,1),(4,1),(5,1),(5,2),(7,2),(7,1),(8,0),(8,1)]:
        chi = character(C,k)
        tau = gauss(chi,C)
        units = [u for u in range(C) if math.gcd(u,C)==1]
        for u in units:
            for v in units:
                close(name,chi(u*v),chi(u)*chi(v))
        for d in range(1,C):
            if C % d == 0:
                verify(name,any(abs(chi(u)-1)>mp.mpf('.1') for u in units if (u-1)%d==0))
        for m in range(-C,2*C+1):
            direct = sum(chi(a)*phase(mp.mpf(a*m)/C) for a in range(C))
            close(name,direct,mp.conj(chi(m))*tau)
        close(name,abs(tau)**2,C)
        close(name,tau*gauss(lambda n:mp.conj(chi(n)),C),(-1)**parity(chi)*C)
        if parity(chi):
            verify(name,abs(tau-gauss(chi,C,-1))>1)


def local_mellin(values,p,d,m,alpha,eta,a,s):
    total = mp.mpc(0)
    ratio = alpha*p**(-s)
    if a == 0:
        total += values[0]*ratio**m/(1-ratio)
    for j in range(1,p**(d+m)):
        v = valuation(j,p)
        k = v-d
        if m-k < a:
            continue  # exact unit-character average on this additive coset is zero
        unit = j//p**v
        mass = mp.mpf(p)**(k-m)/(1-mp.mpf(1)/p)
        total += values[j]*alpha**k*eta(unit)*p**(-k*s)*mass
    return total


def local_gamma(p,alpha,eta,a,s):
    if a == 0:
        return (1-alpha*p**(-s))/(1-alpha**(-1)*p**(s-1))
    G = sum(mp.conj(eta(u))*phase(-mp.mpf(u)/p**a)
            for u in range(p**a) if u%p)
    return alpha**a*p**(-a*s)*G


def check_local_integrals():
    name = 'check_local_integrals'
    s = mp.mpc('1.4','.3')
    for p in [2,3,5,7]:
        alpha = phase(mp.mpf(1)/7)
        direct = sum(alpha**k*p**(-k*s) for k in range(180))
        close(name,direct,1/(1-alpha*p**(-s)))
    for C,k in [(3,1),(4,1),(5,1),(7,2),(8,0),(8,1)]:
        chi = character(C,k)
        units = [u for u in range(C) if math.gcd(u,C)==1]
        close(name,sum(chi(u) for u in units)/len(units),0)
        close(name,sum(chi(u)*mp.conj(chi(u)) for u in units)/len(units),1)


def check_local_gamma():
    name = 'check_local_gamma'
    for p,a,eta in [(2,0,lambda n:1),(3,0,lambda n:1),(2,2,character(4)),
                    (2,3,character(8,0)),(2,4,character(16)),
                    (3,1,character(3)),(3,2,character(9)),(3,3,character(27)),
                    (5,1,character(5,1)),(5,2,character(25)),(7,1,character(7,2))]:
        d,m = 1,max(1,a)
        D,M = p**d,p**m
        vals = [mp.mpc((j*j+3*j)%11-4, (2*j+1)%7-2) for j in range(D*M)]
        transformed = finite_transform(vals,D,M)
        alpha = mp.mpf('1.1')*phase(mp.mpf(1)/9)
        for s in [mp.mpc('.43','.27'), mp.mpc('.76','-.4')]:
            direct = local_mellin(transformed,p,m,d,1/alpha,
                                  lambda n:mp.conj(eta(n)),a,1-s)
            original = local_mellin(vals,p,d,m,alpha,eta,a,s)
            close(name,direct,local_gamma(p,alpha,eta,a,s)*original)


def real_mellin(f,e,s):
    def low(u):
        x = u**8
        return 8*u**(8*s-1)*(f(x)+(-1)**e*f(-x))
    high = lambda x: x**(s-1)*(f(x)+(-1)**e*f(-x))
    return mp.quad(low,[0,1])+mp.quad(high,[1,mp.inf])


def check_real_local():
    name = 'check_real_local'
    coeffs,A,B,C = [1,mp.mpf('.4')],mp.mpf('1.2'),mp.mpf('.8'),0
    f = lambda x:poly(coeffs,x)*mp.exp(-mp.pi*A*x*x+B*x)
    fh = lambda y:gaussian_transform(coeffs,A,B,C,y)
    for e in [0,1]:
        for s in [mp.mpc('.43','.2'),mp.mpc('.7','-.3')]:
            g = (1j)**e*mp.pi**(s-mp.mpf('.5'))*mp.gamma((1-s+e)/2)/mp.gamma((s+e)/2)
            close(name,real_mellin(fh,e,1-s),g*real_mellin(f,e,s))
        s=mp.mpc('1.7','.4')
        integral=real_mellin(lambda x:x**e*mp.exp(-mp.pi*x*x),e,s)
        close(name,integral,mp.pi**(-(s+e)/2)*mp.gamma((s+e)/2))
    close(name,real_mellin(lambda x:mp.exp(-mp.pi*x*x),1,mp.mpc('.6','.2')),0)


def l_hurwitz(s,chi,C):
    return sum(chi(a)*mp.zeta(s,mp.mpf(a)/C) for a in range(1,C+1))/C**s


def completed_reference(s,chi,C):
    e=parity(chi)
    return (mp.mpf(C)/mp.pi)**((s+e)/2)*mp.gamma((s+e)/2)*l_hurwitz(s,chi,C)


def check_idele_character():
    name='check_idele_character'
    local3,local5=character(3),character(5)
    chi=lambda n:local3(n)*local5(n)
    for p in [2,7,11,13]:
        u3,u5=pow(p,-1,3),pow(p,-1,5)
        omega=mp.conj(local3(u3)*local5(u5))
        close(name,omega,chi(p))
    close(name,mp.conj(local5(pow(3,-1,5))),local5(3))
    close(name,mp.conj(local3(pow(5,-1,3))),local3(5))
    # Negative rational multiplication flips both the real sign and the unit coordinate.
    for u in [1,2,4,7,8,11,13,14]:
        close(name,mp.conj(chi((-1)*(-u))),mp.conj(chi(u)))


def check_global_integral():
    name='check_global_integral'
    for C,k in [(1,0),(3,1),(5,1),(7,2)]:
        chi=character(C,k)
        e=parity(chi)
        s=mp.mpc('36','.3')
        # Independent real integral and absolute rational lattice sum in its original domain.
        real=2*mp.quad(lambda x:x**(s+e-1)*mp.exp(-mp.pi*x*x),[0,1,2,4,mp.inf])
        series=sum(chi(n)*n**(-s) for n in range(1,33))
        integral=real*series*C**((s+e)/2)
        expected=completed_reference(s,chi,C)
        close(name,integral,expected)
        bound=abs(real*C**((s+e)/2))*mp.mpf(32)**(-35)/35
        verify(name,abs(integral-expected)<=bound+mp.mpf('1e-48')*max(1,abs(expected)))


def theta(chi,C,e,t,N=50):
    delta=1 if C==1 else 0
    return delta+2*sum(chi(n)*n**e*mp.exp(-mp.pi*n*n*t/C) for n in range(1,N+1))


def check_theta():
    name='check_theta'
    for C,k in [(1,0),(3,1),(5,1),(7,2),(8,0),(8,1)]:
        chi=character(C,k)
        e=parity(chi)
        W=gauss(chi,C)/((1j)**e*mp.sqrt(C))
        for t in [mp.mpf('.4'),mp.mpf('1.3'),mp.mpf('2.7')]:
            other=theta(lambda n:mp.conj(chi(n)),C,e,1/t)
            close(name,theta(chi,C,e,t),W*t**(-e-mp.mpf('.5'))*other)


def check_functional_equation():
    name='check_functional_equation'
    points=[mp.mpc('-.7','.3'),mp.mpc('.2','1.1'),mp.mpc('.6','-.8'),mp.mpc('1.4','.7')]
    for C,k in [(1,0),(3,1),(5,1),(5,2),(7,2),(7,1),(8,0),(8,1)]:
        chi=character(C,k)
        W=gauss(chi,C)/((1j)**parity(chi)*mp.sqrt(C))
        for s in points:
            left=completed_reference(s,chi,C)
            right=W*completed_reference(1-s,lambda n:mp.conj(chi(n)),C)
            close(name,left,right)
        if C==5 and k==1:
            s=points[1]
            wrong=W*completed_reference(1-s,chi,C)
            verify(name,abs(completed_reference(s,chi,C)-wrong)>mp.mpf('.01'))
            wrong_sign=gauss(chi,C,-1)/((1j)**parity(chi)*mp.sqrt(C))
            verify(name,abs(W-wrong_sign)>1)


def integral_piece(z,chi,C,N=24,R=420):
    e=parity(chi)
    # Integrate each finite exponential term exactly as an upper incomplete Gamma difference.
    # No zeta or L evaluator is used on this path.
    return sum(chi(n)*n**e*(mp.pi*n*n/C)**(-z)
               *mp.gammainc(z,mp.pi*n*n/C,mp.pi*n*n*R/C) for n in range(1,N+1))


def continued(s,chi,C,N=24,R=420):
    e=parity(chi)
    W=gauss(chi,C)/((1j)**e*mp.sqrt(C))
    z,zp=(s+e)/2,(1-s+e)/2
    result=integral_piece(z,chi,C,N,R)+W*integral_piece(zp,lambda n:mp.conj(chi(n)),C,N,R)
    if C==1:
        result+=1/(s-1)-1/s
    return result


def continuation_error(s,chi,C,N=24,R=420):
    e=parity(chi)
    b=mp.pi/C
    total=0
    for z in [(s+e)/2,(1-s+e)/2]:
        r=mp.re(z)-1
        total+=mp.exp(b)*series_bound(e,b,N)*integral_bound(r,b,mp.mpf(1))
        total+=mp.exp(b)*series_bound(e,b,0)*integral_bound(r,b,mp.mpf(R))
    return total


def check_continuation():
    name='check_continuation'
    points=[mp.mpc('-2.3','.4'),mp.mpc('-.4','1.2'),mp.mpc('.4','.7'),
            mp.mpc('.99999999','.00000001'),mp.mpc('.00000001','-.00000001')]
    for C,k in [(1,0),(3,1),(5,1),(7,2)]:
        chi=character(C,k)
        for s in points:
            result=continued(s,chi,C)
            expected=completed_reference(s,chi,C)
            close(name,result,expected)
            bound=continuation_error(s,chi,C)
            verify(name,bound<mp.mpf('1e-33'))
    # Here the omitted terms are much larger than rounding of the reference.
    # No tolerance is added to the analytic bound.
    for C,k in [(1,0),(5,1),(9,1),(16,1),(25,1),(27,1),(15,1)]:
        chi=character(C,k)
        for s in [mp.mpc('.25','1.2'),mp.mpc('-1.4','.7'),mp.mpc('.5','7')]:
            expected=completed_reference(s,chi,C)
            for N,R in [(2,3),(5,10)]:
                error=abs(continued(s,chi,C,N,R)-expected)
                bound=continuation_error(s,chi,C,N,R)
                verify(name,error<=bound,error/bound)
    # Independent ordinary quadrature of the actual split-integral integrand.
    for C,k,s in [(1,0,mp.mpc('.3','.4')),(5,1,mp.mpc('-.4','.7'))]:
        chi=character(C,k)
        z=(s+parity(chi))/2
        direct=mp.quad(lambda t:sum(chi(n)*n**parity(chi)*mp.exp(-mp.pi*n*n*t/C)
                                  for n in range(1,17))*t**(z-1),[1,2,4,8,20,80,420])
        close(name,direct,integral_piece(z,chi,C,16,420))


def check_quadrature_bound():
    name='check_quadrature_bound'
    for C,k,z in [(1,0,mp.mpc('-.4','.8')),(5,1,mp.mpc('.3','1.2')),
                  (7,2,mp.mpc('2.4','-.6'))]:
        chi=character(C,k)
        e=parity(chi)
        R,N=mp.mpf(4),6
        a=mp.pi/C
        U=[max(1,R**(mp.re(z)-1-ell)) for ell in range(3)]
        M2=sum(n**e*mp.exp(-a*n*n)*(a*a*n**4*U[0]+2*a*n*n*abs(z-1)*U[1]
                                  +abs((z-1)*(z-2))*U[2]) for n in range(1,N+1))
        expected=integral_piece(z,chi,C,N,R)
        errors=[]
        for K in [32,64]:
            h=(R-1)/K
            direct=h*sum(sum(chi(n)*n**e*mp.exp(-a*n*n*(1+(j+mp.mpf('.5'))*h))
                             for n in range(1,N+1))*(1+(j+mp.mpf('.5'))*h)**(z-1)
                         for j in range(K))
            error=abs(direct-expected)
            bound=(R-1)*h*h*M2/24
            verify(name,error<=bound)
            errors.append(error)
        verify(name,errors[1]<errors[0])


def check_poles():
    name='check_poles'
    eps=mp.mpf('1e-35')
    chi=character(1)
    close(name,eps*continued(eps,chi,1),-1,tol=mp.mpf('1e-32'))
    # Form the actual displacement after finite-precision addition near 1.
    s=1+eps
    close(name,(s-1)*continued(s,chi,1),1)
    for p in [2,3,5]:
        for k in [-1,0,1]:
            pole=2j*mp.pi*k/mp.log(p)
            # Extra guard digits resolve the vanishing denominator at the chosen displacement.
            with mp.workdps(90):
                pole=2j*mp.pi*k/mp.log(p)
                s=pole+eps
                residue=(s-pole)/(1-p**(-s))
                close(name,residue,1/mp.log(p))
    for k in range(3):
        s=-2*k+eps
        close(name,(s+2*k)*mp.pi**(-s/2)*mp.gamma(s/2),
              2*(-1)**k*mp.pi**k/math.factorial(k))


def check_continuation_bounds():
    name='check_continuation_bounds'
    for C in [1,5,7,9,16,25,27,15]:
        a=mp.pi/C
        for e in [0,1]:
            for z in [mp.mpf('-.7'),mp.mpf('.4'),mp.mpf('3.2')]:
                N,R=2,mp.mpf(3)
                omitted_n=sum(n**e*(a*n*n)**(-z)*mp.gammainc(z,a*n*n,mp.inf)
                              for n in range(N+1,30))
                omitted_t=sum(n**e*(a*n*n)**(-z)*mp.gammainc(z,a*n*n*R,mp.inf)
                              for n in range(1,30))
                bn=mp.exp(a)*series_bound(e,a,N)*integral_bound(z-1,a,mp.mpf(1))
                bt=mp.exp(a)*series_bound(e,a,0)*integral_bound(z-1,a,R)
                verify(name,omitted_n<=bn)
                verify(name,omitted_t<=bt)
                verify(name,series_bound(e,a,2*N)<series_bound(e,a,N))
                verify(name,integral_bound(z-1,a,2*R)<integral_bound(z-1,a,R))


def ball_series_bound(j, c, T, beta=0):
    """Lemma 6 with outward ball operations; a comparison must be certain."""
    from flint import arb
    K = math.floor(T)+1
    prefix = arb(0)
    while True:
        rho = (arb(j)/K-c*(2*K+1)+beta).exp()
        term = arb(K)**j*(-c*K*K+beta*K).exp()
        if rho <= arb(1)/2:
            return prefix+term/(1-rho)
        prefix += term
        K += 1


def ball_integral_bound(r, b, R):
    from flint import arb
    rp = r if r > 0 else arb(0)
    if b >= 2*rp/R:
        return 2*R**r*(-b*R).exp()/b
    R0 = 2*rp/b
    peak = r/b
    if peak < R:
        peak = R
    elif peak > R0:
        peak = R0
    return (R0-R)*peak**r*(-b*peak).exp()+2*R0**r*(-b*R0).exp()/b


def flint_character(chi, C):
    """Identify the small character table by its values on every residue."""
    from flint import dirichlet_char
    if C == 1:
        return dirichlet_char(1,1)
    for index in range(1,C):
        if math.gcd(index,C) != 1:
            continue
        candidate = dirichlet_char(C,index)
        if all(abs(complex(candidate(n).mid())-complex(chi(n))) < 1e-12 for n in range(C)):
            return candidate
    raise AssertionError(f'no FLINT character of conductor {C} matches the finite table')


def check_certified_continuation():
    """Compare the exact term integrals and the analytic bound to FLINT balls."""
    from flint import acb, arb, ctx
    name = 'check_certified_continuation'
    old_prec = ctx.prec
    ctx.prec = 300
    try:
        pi = arb.pi()
        for C,k in [(1,0),(5,1),(9,1),(16,1),(25,1),(27,1),(15,1)]:
            chi = character(C,k)
            d = flint_character(chi,C)
            e = parity(chi)
            a0 = pi/C
            tau = acb(1) if C == 1 else sum(
                (d(n)*(acb(0,2)*pi*n/C).exp() for n in range(C)), acb(0))
            W = tau/(acb(0,1)**e*arb(C).sqrt())
            for s in [acb('0.25','1.2'),acb('-1.4','0.7'),acb('0.5','7')]:
                z,zp = (s+e)/2,(1-s+e)/2
                reference = (arb(C)/pi)**z*z.gamma()*d.l_function(s)
                for N,R in [(2,3),(5,10)]:
                    value = acb(0)
                    bound = arb(0)
                    for power,char,weight in [(z,d,acb(1)),(zp,flint_character(
                            lambda n: mp.conj(chi(n)),C),W)]:
                        for n in range(1,N+1):
                            if math.gcd(n,C)>1:
                                continue
                            x = a0*n*n
                            term = acb(x)**(-power)*(acb(x).gamma_upper(power)
                                   -acb(x*R).gamma_upper(power))
                            value += weight*char(n)*arb(n)**e*term
                        r = power.real-1
                        bound += a0.exp()*(ball_series_bound(e,a0,N)
                                              *ball_integral_bound(r,a0,arb(1))
                                              +ball_series_bound(e,a0,0)
                                              *ball_integral_bound(r,a0,arb(R)))
                    if C == 1:
                        value += 1/(s-1)-1/s
                    difference = abs(value-reference)
                    verify(name,difference <= bound)
    finally:
        ctx.prec = old_prec


def check_poisson_right_and_idele():
    from flint import acb, arb, ctx
    name = 'check_poisson_right_and_idele'
    old_prec = ctx.prec
    ctx.prec = 256
    try:
        D,M = 2,3
        L = D*M
        values = [1,2,3,4,5,6]
        pi = arb.pi()
        A = arb(1)/5
        g = [sum((acb(values[j])*(acb(0,-2)*pi*j*k/L).exp()
                  for j in range(L)),acb(0))/M for k in range(L)]
        max_g = max(abs(v).upper() for v in g)
        decay = pi/(A*M*M)
        for T in [0,1,2]:
            partial = sum((g[n%L]*A**(-arb(1)/2)*(-decay*n*n).exp()
                           for n in range(-20,21) if abs(n)>T),acb(0))
            omitted = max_g*2*A**(-arb(1)/2)*ball_series_bound(0,decay,20)
            bound = max_g*2*A**(-arb(1)/2)*ball_series_bound(0,decay,T)
            verify(name,abs(partial)+omitted <= bound)
    finally:
        ctx.prec = old_prec

    values = [mp.mpc(j-2,j*j+1) for j in range(L)]
    g = finite_transform(values,D,M)
    xinf,r,u = mp.mpf('-.7'),Fraction(3,2),5
    rin = mp.mpf(r.numerator)/r.denominator
    ui = pow(u,-1,L)
    left = sum(values[(n*u)%L]*mp.exp(-mp.pi*mp.mpf('.8')*(n*xinf/(D*rin))**2)
               for n in range(-80,81))
    right = sum(g[(n*ui)%L]*gaussian_transform([1],mp.mpf('.8'),0,0,n*rin/(M*xinf))
                for n in range(-80,81))
    close(name,left,right/(abs(xinf)/rin))


def check_general_splitting():
    """General Proposition 12 vector with distinct f(0) and F f(0)."""
    name = 'check_general_splitting'
    D,M = 2,3
    L = D*M
    values = [mp.mpc(j+1,2*j-3) for j in range(L)]
    transformed = finite_transform(values,D,M)
    coeffs,A,B,Cg = [1,mp.mpf('.3')],mp.mpf('.8'),mp.mpf('.6'),mp.mpf('.1')
    phi = lambda x: poly(coeffs,x)*mp.exp(-mp.pi*A*x*x+B*x+Cg)
    phihat = lambda x: gaussian_transform(coeffs,A,B,Cg,x)
    units = [u for u in range(L) if math.gcd(u,L)==1]
    c1 = [sum(values[(n*u)%L] for u in units)/len(units) for n in range(L)]
    c2 = [sum(transformed[(n*u)%L] for u in units)/len(units) for n in range(L)]
    a,b = values[0]*phi(0),transformed[0]*phihat(0)
    verify(name,abs(a-b)>mp.mpf('.1'))

    def H(fun,c,den,t):
        return sum(c[n%L]*fun(t*n/den) for n in range(-30,31) if n)

    grid = [1,2,4,8,16,40]

    def split(s):
        return (mp.quad(lambda t:H(phi,c1,D,t)*t**(s-1),grid)
                +mp.quad(lambda t:H(phihat,c2,M,t)*t**(-s),grid)
                +b/(s-1)-a/s)

    def direct(fun,c,den,s):
        real = mp.quad(lambda t:(fun(t)+fun(-t))*t**(s-1),
                       [0,mp.mpf('.5'),1,2,4,8,mp.inf])
        series = (mp.mpf(den)/L)**s*sum(
            c[j%L]*mp.zeta(s,mp.mpf(j)/L) for j in range(1,L+1))
        return real*series

    s = mp.mpc('2.4','.7')
    close(name,split(s),direct(phi,c1,D,s),tol=mp.mpf('1e-27'))
    close(name,split(1-s),direct(phihat,c2,M,s),tol=mp.mpf('1e-27'))
    eps = mp.mpf('1e-14')
    close(name,eps*split(1+eps),b,tol=mp.mpf('1e-12'))
    close(name,eps*split(eps),-a,tol=mp.mpf('1e-12'))


def check_general_adelic_bound():
    """Certified omitted n and t regions with an odd polynomial and conductor 3."""
    from flint import acb, arb, ctx
    name = 'check_general_adelic_bound'
    old_prec = ctx.prec
    ctx.prec = 256
    try:
        pi = arb.pi()
        A,p1,F0 = arb(1)/5,arb(3)/10,arb(3)
        a,c = pi*A,pi*A/2
        K1 = 2*F0*p1
        v = acb('0.6','1.3')
        w = (v+1)/2
        r = v.real

        def coeff(n):
            return arb(0) if n%3==0 else (arb(-1)/2 if n%3==1 else arb(1)/2)

        def term(n,lo):
            x = a*n*n
            return coeff(n)*p1*n*acb(x)**(-w)*acb(x*lo*lo).gamma_upper(w)

        for N in [1,3]:
            partial = sum((term(n,1) for n in range(N+1,26)),acb(0))
            remainder = K1*c.exp()*ball_series_bound(1,c,25)*ball_integral_bound(r,c,arb(1))
            bound = K1*c.exp()*ball_series_bound(1,c,N)*ball_integral_bound(r,c,arb(1))
            verify(name,abs(partial)+remainder <= bound)
        for R in [2,4]:
            partial = sum((term(n,R) for n in range(1,26)),acb(0))
            remainder = K1*c.exp()*ball_series_bound(1,c,25)*ball_integral_bound(r,c,arb(R))
            bound = K1*c.exp()*ball_series_bound(1,c,0)*ball_integral_bound(r,c,arb(R))
            verify(name,abs(partial)+remainder <= bound)
    finally:
        ctx.prec = old_prec


def check_composite_conductor():
    name = 'check_composite_conductor'
    C = 15
    chi3,chi5 = character(3),character(5)
    chi = character(C)
    e = parity(chi)
    s = mp.mpc('.35','1.7')
    G3 = sum(chi3(u)*phase(-mp.mpf(u)/3) for u in range(3))
    G5 = sum(chi5(u)*phase(-mp.mpf(u)/5) for u in range(5))
    product = (1j)**e*chi5(3)*3**(-s)*G3*chi3(5)*5**(-s)*G5
    expected = (1j)**(-e)*gauss(chi,C)*C**(-s)
    close(name,product,expected)
    close(name,abs(gauss(chi,C))**2,C)

CHECKS = [check_characters, check_local_fourier, check_finite_fourier,
          check_real_transform, check_real_closure, check_tail_bounds, check_poisson,
          check_gauss_sums, check_local_integrals, check_local_gamma, check_real_local,
          check_idele_character, check_global_integral, check_theta,
          check_functional_equation, check_continuation, check_quadrature_bound,
          check_continuation_bounds, check_certified_continuation,
          check_poisson_right_and_idele, check_general_splitting,
          check_general_adelic_bound, check_composite_conductor, check_poles]



def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--only', nargs='*', help='check function names')
    args = parser.parse_args()
    failures = 0
    start = time.monotonic()
    for check in CHECKS:
        if args.only and check.__name__ not in args.only:
            continue
        try:
            check()
            print(f'{check.__name__}: count={COUNTS[check.__name__]} failures=0 '
                  f'max_recorded_error={mp.nstr(ERRORS[check.__name__], 5)}', flush=True)
        except Exception as exc:
            failures += 1
            print(f'{check.__name__}: count={COUNTS.get(check.__name__, 0)} '
                  f'failures=1 {type(exc).__name__}: {exc}', flush=True)
    print(f'TOTAL checks={sum(COUNTS.values())} failed_groups={failures} '
          f'seconds={time.monotonic()-start:.3f}', flush=True)
    return bool(failures)


if __name__ == '__main__':
    sys.exit(main())
