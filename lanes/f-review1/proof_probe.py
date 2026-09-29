from fractions import Fraction as Q

p, N, q = 2, 1, Q(1, 3)
a, b, u = q.numerator, q.denominator, 1
lhs, rhs = q-u, Q(b*u-a, b)
print(f"L0 p={p} N={N} q={q} u={u}: t-u={lhs} (b*u-a)/b={rhs} equality={lhs == rhs}")
assert lhs == rhs, "docs/api-1f.md L0 proof has the opposite sign"
