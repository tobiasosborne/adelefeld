"""f-repair4: the reviewer's oracle (lanes/f-review6/oracle.py, exact integers only) with the exponent rule of
N-D14 for ball inputs: K = min(Nreq, E). For Nreq >= E the reviewer's exact-image check is applied unchanged.
For Nreq < E (R2 step 6): exponent Nreq; for Nreq <= j the zero ball at Nreq (R4 step 5); otherwise valuation j and
centre = the root of the centre unit modulo p^(Nreq - j), by the reviewer's digit lifting (O2), which agrees with
every point of the image modulo p^(E - j) > p^(Nreq - j). COUNT['coarse'] counts the cases where N-D13 and N-D14
differ (an OK ball result with Nreq < E)."""
import sys
sys.path.insert(0, 'lanes/f-review6')
import oracle as _O
from oracle import *
COUNT = {'coarse': 0, 'image': 0}

def check_value(x, n, seed, Nreq, res):
    if x['kind'] != 'ball' or n == 1:
        return _O.check_value(x, n, seed, Nreq, res)
    p = x['p']; c = cfor(p); s = vp(n, p); j = x['m'] // n
    E = x['M'] - s - (n - 1) * j
    if Nreq >= E:
        COUNT['image'] += 1
        return _O.check_value(x, n, seed, Nreq, res)
    COUNT['coarse'] += 1
    ex, v, N, a, b = res
    if ex != 0: return "ball input gave exact"
    if N != Nreq: return "exponent %d, want min(N, E) = %d" % (N, Nreq)
    if Nreq <= j:
        return None if (a, v) == (0, 0) else "N<=j: expected zero ball"
    L = Nreq - j
    rho = lift(p, n, x['Un'], x['Ud'], seed, max(L, c))
    if rho is None: return "oracle: no branch"
    if v != j or b != 1: return "valuation/denominator"
    if a % p == 0 or not (0 < a < p**L): return "noncanonical"
    if a != rho % p**L: return "centre %d != root of centre mod p^%d = %d" % (a, L, rho % p**L)
    return None
