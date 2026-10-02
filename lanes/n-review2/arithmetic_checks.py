from fractions import Fraction as F

checks = 0
for b, want_d, want_work in ((7462, 2245, 33538722), (7463, 2246, 33558144)):
    d = len(str(1 << (b - 1))) - 1
    work = 2 * (d + 2) * max(64, b + 1)
    assert d == want_d and work == want_work
    checks += 1
    print(f"b={b} D={d} S={b+1} work={work} refused={work > 2**25}")
u = F(1, 2**52)
assert 2*u/(1-u) < F(1, 2**50)
checks += 1
assert F(1, 2**50) + F(1, 2**51) * (1 + F(1, 2**50)) < F(1, 2**49)
checks += 1
assert (F(1, 2**52) + F(1, 2**49)) / (1 - F(1, 2**49)) < F(1, 2**48)
checks += 1
print(f"exact_checks={checks} failures=0")
