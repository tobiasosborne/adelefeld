# lanes/drv-s/expected_recover.py: the solutions of Definition 1.1 (include/adelefeld/resid.h,
# the comment above adf_resid_reconstruct) for the cases of tests/driver/s-recover.cmd, by
# enumeration over d.  A solution is a pair of integers (n, d) with
#
#   d > 0,  gcd(n, d) = 1,  gcd(d, m) = 1,  n = c d mod m,  |n| <= A,  d <= B,
#
# and q = n/d is printed by adf_rat_get_str in lowest terms with d > 0.  Nothing here calls
# the library.

from math import gcd


def sols(c, m, A, B, dcap=None):
    """every (n, d) with d in [1, B] (or [1, dcap] when that is given), |n| <= A,
    n = c d mod m, gcd(n, d) = gcd(d, m) = 1"""
    c = c % m
    out = []
    for d in range(1, (B if dcap is None else min(B, dcap)) + 1):
        if gcd(d, m) != 1:
            continue
        r = (c * d) % m
        # the integers congruent to r modulo m: r - k m for the k with -A <= r - k m <= A
        k = (r - A + m - 1) // m
        while -A <= r - k * m <= A:
            n = r - k * m
            if gcd(abs(n), d) == 1:
                out.append((n, d))
            k += 1
    return sorted(set(out))


def show(c, m, A, B, dcap=None):
    s = sols(c, m, A, B, dcap)
    print("recover %d mod %d with %d with %d: %d solution(s) %s" %
          (c, m, A, B, len(s), ", ".join("%d/%d" % (n, d) for n, d in s)))


show(7, 19, 3, 3)        # one solution, 2 A B = 18 < 19
show(6, 12, 1, 5)        # gcd(R, T) = 2: none
show(5, 6, 1, 5)         # two solutions: -1/1 and 1/5, so NOT_UNIQUE with a large limit
show(5, 6, 1, 4)         # one solution: -1/1
show(1, 2, 1, 1)         # 2 A B = m = 2: -1/1 and 1/1
show(1, 2, 2, 1)         # A >= m: c/1 = 1/1 and (c - m)/1 = -1/1
show(6, 12, -1, 6)       # the empty box, A < 0
show(6, 12, 1, 0)        # the empty box, B < 1
# P(2003, 1) with A = 3, B = 1001, the case of s-recover.cmd: the enumeration gives the single
# solution 1/1 and the status of the function is NOT_DETERMINED, because its search is cut after
# 1000 of the 1001 rounds (Algorithm R, step 8); the search is derived in the command file.
show(1, 2003, 3, 1001)
# P(1000003, 1000000) with A = 3 and B = 10^30, the other case of that file: the enumeration
# cannot run over d up to 10^30, so only the pair of the search is checked, and it is the pair
# (n, d) = (-3, 1) of the round x = 1 of Algorithm R, which is a solution since -3 = 1000000 mod
# 1000003.  The set is not counted here.
show(1000000, 1000003, 3, 30)

# a planted fraction of 21 digits in the range 2 A B < m, so that the answer is unique without
# a search: n = -A, d = B, m = 2 A B + 1, c = n d^-1 mod m.
A = 10 ** 20 + 7
B = 10 ** 20 + 9
m = 2 * A * B + 1
n, d = -A, B
assert gcd(n, d) == 1 and gcd(d, m) == 1
c = (n * pow(d, -1, m)) % m
print("planted: m =", m, "c =", c, "A =", A, "B =", B)
print("2 A B =", 2 * A * B, "< m =", m, ":", 2 * A * B < m)
# the five conditions of Definition 1.1 for the planted pair, one by one; the enumeration is not
# run here, since d = B is of 21 digits and 2 A B < m gives at most one solution anyway
print("d > 0:", d > 0, " gcd(n, d) = 1:", gcd(n, d) == 1, " gcd(d, m) = 1:", gcd(d, m) == 1)
print("|n| <= A:", abs(n) <= A, " d <= B:", d <= B, " n = c d mod m:", (n - c * d) % m == 0)
