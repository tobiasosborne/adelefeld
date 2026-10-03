import random, subprocess, sys
from fractions import Fraction
from math import gcd

def vp(n, p):
    k = 0
    while n % p == 0:
        n //= p; k += 1
    return k

NT = None
def logp(x, p, H):
    """Log(x) mod p^H for integer unit x (not div by p): Log = log(x^(p-1))/(p-1); at 2 log(x^2)/2.
    Series sum_{n=1}^{NT} (-1)^(n+1) t^n / n, t = x^e - 1 exact integer; each term p-integral
    (v(t) >= 1, v(t^n/n) >= n - log_p n; n up to 4H+30 makes the tail > H+3)."""
    e = 2 if p == 2 else p - 1
    G = 3  # guard digits
    mod = p ** (H + G)
    t = pow(x, e, p ** (H + G + 2)) - 1  # t mod p^(H+G+2); t exact mod that modulus suffices
    tot = 0
    for n in range(1, 4 * H + 40):
        # term t^n / n : v_p(n) = s ; compute t^n exactly mod p^(H+G+s+...)
        s = vp(n, p)
        tn = pow(t, n, p ** (H + G + s + 2))
        assert tn % (p ** s) == 0 or True
        q, rem = divmod(tn, p ** s)
        assert rem == 0, (p, x, n)
        m = n // p ** s
        term = q * pow(m, -1, mod) % mod
        tot += term if n % 2 == 1 else -term
        tot %= mod
    # divide by e: at odd p, e=p-1 unit; at 2, e=2
    if p == 2:
        assert tot % 2 == 0
        tot //= 2
        return tot % (2 ** H)
    return tot * pow(e, -1, mod) % (p ** H)

def local_image(rnum, rden, M, cstr, p, H):
    """set of Log(r*u) mod p^H over idele components. M=0 exact c (+-1); else coset c U(M)."""
    m = vp(rnum, p) - vp(rden, p)
    ru = Fraction(rnum, rden) / Fraction(p) ** m
    num, den = ru.numerator, ru.denominator
    k = vp(M, p) if M else 0
    if M == 0:
        c = cstr
        mod = p ** H
        x = (num * c * pow(den, -1, mod)) % mod
        return {logp(x, p, H)}
    S = set()
    mod = p ** H
    if k == 0 or (p == 2 and k == 1):
        units = [u for u in range(mod) if u % p]
    else:
        units = [u for u in range(mod) if u % p**k == cstr % p**k]
    for u in units:
        x = (num * u * pow(den, -1, mod)) % mod
        S.add(logp(x, p, H))
    return S

def smallest_ball(S, p, H):
    """returns (centre, j): smallest ball s0 + p^j Z_p containing S (mod p^H); j=H if singleton"""
    L = sorted(S)
    s0 = L[0]
    j = H
    for s in L:
        d = (s - s0) % p ** H
        if d:
            j = min(j, vp(d, p))
    return s0 % p ** j, j

def run(cases):
    inp = "\n".join(cases) + "\n"
    out = subprocess.run(["lanes/f-review10/w/h"], input=inp, capture_output=True, text=True, timeout=110)
    return out.stdout.strip().split("\n"), out.returncode
