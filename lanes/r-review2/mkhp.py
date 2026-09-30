import sys; sys.set_int_max_str_digits(0)
from flint import fmpz_poly
import functools
def lin(a, b): return fmpz_poly([-b, a])
def prod(l): return functools.reduce(lambda a, b: a * b, l)
X = fmpz_poly([0, 1])
for prec in (20000, 100000):
    p = prod([lin(1, i) for i in range(1, 51)]) + 1
    open('/tmp/x/hp_wilk_%d.txt' % prec, 'w').write("%d %s\n" % (prec, " ".join(str(int(c)) for c in p.coeffs())))
    p = prod([X ** 2 - 2 * (i + 1) for i in range(0, 25)])
    open('/tmp/x/hp_sq_%d.txt' % prec, 'w').write("%d %s\n" % (prec, " ".join(str(int(c)) for c in p.coeffs())))
