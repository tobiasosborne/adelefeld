import sys; sys.set_int_max_str_digits(0)
from flint import fmpz_poly
import functools
def lin(a, b): return fmpz_poly([-b, a])
for k, n in ((190, 50), (100, 50), (60, 50), (100, 30)):
    p = functools.reduce(lambda a, b: a * b, [lin(3 * 2 ** k, 2 ** k + i) for i in range(1, n + 1)])
    open('/tmp/x/cl_%d_%d.txt' % (k, n), 'w').write("2 " + " ".join(str(int(c)) for c in p.coeffs()) + "\n")
