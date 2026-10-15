"""An independent oracle for the expected lines of the hand-written claims: exp at a prime.

exp(x) = sum over k >= 0 of x^k / k!, summed in exact rationals and reduced modulo p^N. It shares no
code with the library. Used for the expected lines of claims a3 and a5 (exp of 3 and of 12 at 3), so
that those lines were not copied from the driver. Standard library only.

Convergence, proved here: v_p(k!) = sum over j <= k of v_p(j) = sum over i >= 1 of #{j <= k : p^i | j}
= sum of floor(k / p^i) < k (1/p + 1/p^2 + ...) = k/(p - 1); an integer below k/(p - 1) is at most
(k - 1)/(p - 1). So v_p(x^k / k!) >= k v - (k - 1)/(p - 1) with v = v_p(x). The series
converges when v > 1/(p - 1), that is v >= 1 for odd p and v >= 2 for p = 2, which is the domain of
exp_at in tools/adf/README.md ("the series on p^c Z_p, with c=1 for odd p and c=2 at 2"). The lower
bound grows with k, so once it is at least N every later term is 0 modulo p^N.
"""
from fractions import Fraction
import sys


def vp(n, p):
    """v_p of a nonzero integer."""
    v = 0
    while n % p == 0:
        n //= p
        v += 1
    return v


def exp_mod(x, p, n):
    """exp(x) modulo p^n for an integer x in p^c Z_p: the centre in [0, p^n), an integer."""
    if x == 0:
        return 1 % p ** n
    v = vp(x, p)
    c = 2 if p == 2 else 1
    if v < c:
        raise ValueError('exp does not converge at %d for x = %d' % (p, x))
    total = Fraction(0)
    term = Fraction(1)
    k = 0
    while True:
        total += term
        k += 1
        term = term * x / k
        # lower bound on v_p of this term and of every later one
        if k * v - Fraction(k - 1, p - 1) >= n:
            break
    m = p ** n
    num, den = total.numerator, total.denominator
    if den % p == 0:
        raise ValueError('the sum is not p-integral')
    return num * pow(den, -1, m) % m


if __name__ == '__main__':
    # python3 ux/ledger/oracle.py X P N   prints the line exp_at X with P prints at prec N
    x, p, n = (int(a) for a in sys.argv[1:4])
    print('%d: %d + O(%d^%d)' % (p, exp_mod(x, p, n), p, n))
