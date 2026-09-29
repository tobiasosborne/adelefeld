# lanes/drv-s/expected_roots.py: the expected output lines of tests/driver/s-roots.cmd and
# s-hostile.cmd, computed with integer arithmetic only.  No library call and no program output
# is used: every number here comes from a square root modulo a prime power or from an
# enumeration of the solutions of Definition 1.1.

# roots of X^2 - 2 in Z_7 to prec_p = 5: the centres are the solutions of a^2 = 2 mod 7^5,
# one for each root 3 and 4 of X^2 - 2 mod 7, lifted by the digit-by-digit rule
# a_{i+1} = a_i + t 7^i with t in {0,...,6} chosen so that (a_i + t 7^i)^2 = 2 mod 7^(i+1).
def sqrt_mod_2(a, p, K):
    """the two square roots of a modulo p^K, or [] if a is not a square; p odd prime"""
    assert K >= 1
    roots = [r for r in range(p) if (r * r - a) % p == 0]
    if not roots:
        return []
    out = []
    for r0 in roots:
        r = r0
        m = p
        for i in range(1, K):
            m *= p
            for t in range(p):
                c = r + t * (m // p)
                if (c * c - a) % m == 0:
                    r = c
                    break
            else:
                raise AssertionError("no lift")
        out.append(r % (p ** K))
    return sorted(out)

# the roots of X^2 - 2 in Z_p to prec_p = 5 for p = 7 and p = 5
for p in (7, 5):
    rs = sqrt_mod_2(2, p, 5)
    print("X^2 - 2 at p = %d, prec_p = 5:" % p,
          "; ".join("%d mod %d^5" % (a, p) for a in rs) if rs else "none")

# the same check on the square: (3 mod 7^5)^2 = 2 mod 7^5 and the two centres are the only
# ones: the squares modulo 7 are 1, 2 and 4, and 2 has the roots 3 and 4.
a = sqrt_mod_2(2, 7, 5)
print("check a^2 = 2 mod 7^5:", [(x * x - 2) % 7 ** 5 for x in a], "a mod 7:", [x % 7 for x in a])

# the roots of X^2 + X - 2 = (X - 1)(X + 2) in Z_7 to 5 bits: 1 and the lift of -2 = 5 mod 7.
# Same digit-by-digit rule for h = X^2 + X - 2.
def roots_mod(p, K, coeffs):
    """the roots of sum coeffs[k] X^k modulo p^K by the digit rule; coeffs constant first"""
    def ev(x, m):
        v = 0
        for c in reversed(coeffs):
            v = (v * x + c) % m
        return v
    m0 = p
    rs0 = [r for r in range(p) if ev(r, m0) == 0]
    out = []
    for r0 in rs0:
        r = r0
        m = m0
        for i in range(1, K):
            m *= p
            for t in range(p):
                c = r + t * (m // p)
                if ev(c, m) == 0:
                    r = c
                    break
            else:
                raise AssertionError("no lift")
        out.append(r % (p ** K))
    return sorted(out)

print("(X-1)(X+2) at 7, prec 5:", roots_mod(7, 5, [-2, 1, 1]))
# (X-1)(X+2)^2 = X^3 + 3 X^2 - 3 X - 2: the list is the list of g* = (X-1)(X+2) (decision
# S-D13), so the same two roots; the digit rule itself does not lift a multiple root.
print("(X-1)(X+2)^2 at 7, prec 5, through g*:", roots_mod(7, 5, [-2, 1, 1]))
print("X^2 - 3 at 7, prec 5:", roots_mod(7, 5, [-3, 0, 1]))
# 27 X at 3: g* = X, the only root is 0, s = v_3(g'(0)) = v_3(1) = 0, K = max(5, 1) = 5
print("27 X at 3, prec 5: [0], s = 0, K = 5")
# A prime of 64 bits: p = 2^64 - 59 = 18446744073709551557 is prime (n_is_prime, place.h:34).
# The roots of (X-3)(X-5)(X-7) = X^3 - 15 X^2 + 71 X - 105 are the integers 3, 5 and 7, and
# g* = f (squarefree).  For each, s = v_p(g'(alpha)) = 0, since g' = 3 X^2 - 30 X + 71 has the
# values -8, -4, 20 at 3, 5, 7, none divisible by p.  So K = max(prec_p, s + 1) = prec_p = 3
# and the centre is alpha mod p^3 = alpha, since p > 7.
p64 = 18446744073709551557
print("p is prime:", pow(2, p64 - 1, p64) == 1, "p > 7:", p64 > 7)
for alpha, gp in ((3, -8), (5, -4), (7, 20)):
    print("  alpha =", alpha, "g'(alpha) =", gp, "s = 0, K = 3, centre =", alpha % p64 ** 3)
print("7^3 =", 7 ** 3, "> 7: yes")
