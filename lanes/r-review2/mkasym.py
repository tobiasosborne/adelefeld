import sys; sys.set_int_max_str_digits(0)
from flint import fmpz_poly
import functools
def lin(a, b): return fmpz_poly([-b, a])
def prod(l): return functools.reduce(lambda a, b: a * b, l)
k = int(sys.argv[1])
A = prod([lin(2 ** k, i) for i in range(1, 51)])      # roots i 2^-k
B = prod([lin(1, i * 2 ** k) for i in range(1, 51)])  # roots i 2^k
for name, p in (("A", A), ("B", B)):
    open('/tmp/x/asym_%s.txt' % name, 'w').write("2 " + " ".join(str(int(c)) for c in p.coeffs()) + "\n")
    print(name, p.degree(), max(abs(int(c)).bit_length() for c in p.coeffs()))
