import sys; sys.set_int_max_str_digits(0)
from flint import fmpz_poly
import functools, random
random.seed(9)
X = fmpz_poly([0, 1])
def lin(a, b): return fmpz_poly([-b, a])
def prod(l): return functools.reduce(lambda a, b: a * b, l)
L = []
L.append((2, X ** 2 - 2)); L.append((64, X ** 2 - 2)); L.append((2, fmpz_poly([5])))
L.append((2, fmpz_poly([0, 1]))); L.append((30, prod([lin(1, i) for i in range(1, 12)])))
L.append((2, prod([lin(3 * 2 ** 40, 2 ** 40 + i) for i in range(1, 9)])))          # cluster, scaled count
L.append((2, prod([lin(3 * 2 ** 400, 2 ** 400 + i) for i in range(1, 9)])))          # cluster, big
L.append((300, X ** 9 - 2)); L.append((5000, X ** 8 - 3 * X - 1)); L.append((5000, X ** 6 - 2))
L.append((2, X ** 12 - 2 * fmpz_poly([-1, 2 ** 200]) ** 2))
L.append((64, (X - 1) ** 3 * (X + 2) ** 2 * X * 6))
L.append((2, prod([X ** 2 - 4 ** (50 * k) - 1 for k in range(0, 6)])))
L.append((2, prod([lin(2 ** 30, 2 ** 27 * j + (1 if j % 2 else 0)) for j in range(-4, 5)])))
L.append((100, fmpz_poly([-1, 2 ** 90]) ** 10 - 1))
L.append((2, X ** 8 + 1))                                                        # no real roots
L.append((2, prod([(X * 2 ** 300 - 1) ** 2 + i for i in range(1, 8)])))
L.append((2000, fmpz_poly([random.randint(-2 ** 300, 2 ** 300) for _ in range(16)])))
L.append((2, fmpz_poly([0, 0, 0, 1, 0, 0, 0, 0, 0, 2 ** 5000 + 1])))
L.append((70, prod([lin(2 ** 4, 16 * i + (i % 3) - 1) for i in range(1, 15)])))
open('/tmp/x/vg_in.txt', 'w').write("".join("%d %s\n" % (pr, " ".join(str(int(c)) for c in p.coeffs())) for pr, p in L))
print(len(L))
