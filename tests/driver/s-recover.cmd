# s-recover: the rational of a residue class modulo m in a box, through adf_resid_reconstruct
# (include/adelefeld/resid.h; SPEC 9.2, the second item, "From partial data").  The command is
# "recover C mod M with A with B": the first operand is a finite ball of the value form, whose
# canonical global triple is (C, M, 1), and it is the residue class P(M, C) of
# adf_resid_set_fball_forget; A and B are the bounds of the numerator and of the denominator,
# as exact rationals that must be integers.  The search limit of the driver is 1000 rounds.  A
# solution is a pair of integers (n, d) with d > 0, gcd(n, d) = 1, gcd(d, M) = 1,
# n = C d modulo M, |n| <= A and d <= B, and the answer is n/d in lowest terms.
#
# Every expected line of s-recover.out is derived here or in lanes/drv-s/expected_recover.py,
# which enumerates the solutions of that definition over d in [1, B] and counts them.  The status
# of each line is the status of Algorithm R of docs/proofs/solvers.md:256 to 324, whose steps are
# quoted in the comment of the command they belong to.  Nothing is taken from the output of the
# program.
#!exit 1
# P(19, 7) with A = 3, B = 3.  The enumeration gives exactly one solution, the pair (2, 3):
# d = 1 needs n = 7 modulo 19 with |n| <= 3, which no integer meets; d = 2 needs n = 14 modulo 19,
# that is n = -5, too large; d = 3 needs n = 21 = 2 modulo 19, and n = 2 is in the box, with
# gcd(2, 3) = 1 and gcd(3, 19) = 1.  2 A B = 18 < 19, so at most one solution exists (P1.6 (c)) and
# the answer is it: 2/3.
recover 7 mod 19 with 3 with 3
# P(19, 0) with A = 0, B = 3.  The only pair with |n| <= 0 is (0, 1), and 0 = 0 * 1 modulo 19,
# gcd(0, 1) = 1 and gcd(1, 19) = 1, so 0/1 is a solution.  2 A B = 0 < 19, so it is the only
# one, and the printer writes the exact zero as "0" (conventions 9.4, adf_rat_get_str).
recover 0 mod 19 with 0 with 3
# P(19, 18) = P(19, -1) with A = 1, B = 3.  d = 1 and n = -1 is a solution; 2 A B = 6 < 19 gives
# at most one, so the answer is the pair (-1, 1).  The printer writes a rational whose denominator
# is 1 as the numerator alone (conventions 9.4, adf_rat_get_str, "n or n/d in lowest terms"), so
# the line is -1 and not -1/1.
recover 18 mod 19 with 1 with 3
# P(12, 6) with A = 1, B = 5.  The certificate pair of Lemma 1.4 is (R', T', R, T) = (6, 1, 0, -2),
# and gcd(R, T) = 2 > 1, so the row 0/2 is not reduced and 2 A B = 10 < 12: step 6 of Algorithm R
# gives NO_SOLUTION.  The enumeration agrees: the only candidates have d = 1 or 5 with
# n = 6 d = 6 modulo 12, that is |n| >= 6 > 1.  This is the example of Remark 1 of P1.6.
recover 6 mod 12 with 1 with 5
# P(6, 5) with A = 1, B = 5.  The enumeration gives two solutions, -1/1 and 1/5.  The search of
# step 7 of Algorithm R finds both: with (R', T', R, T) = (5, 1, 1, -1) and sigma = -1, the round
# x = 1 gives y = 0 and the point (-1, 1), and the round x = 4 gives y = 1 (the range of y is
# 0 <= y <= floor((4 * 1 + 1)/5) = 1) and the point (1, 5), so the status is NOT_UNIQUE.
recover 5 mod 6 with 1 with 5
# The same problem with B = 4: the point (1, 5) has d = 5 > 4 and is out of the box, so the
# enumeration gives the single solution -1/1, X = floor(B / |T|) = 4 <= 1000, and step 8 of
# Algorithm R returns OK with it.  The text is -1, as above.
recover 5 mod 6 with 1 with 4
# P(2, 1) with A = 2, B = 1.  A >= m, so step 3 of Algorithm R returns NOT_UNIQUE without a
# search: c mod m / 1 = 1/1 and (c mod m - m) / 1 = -1/1 are two solutions (P1.6 (a)).
recover 1 mod 2 with 2 with 1
# P(2003, 1) with A = 3, B = 1001, the search limit 1000.  2003 is prime, 0 <= A = 3 < m, and
# step 4 of Algorithm R stops at once: (R', T', R, T) = (2003, 0, 1, 1).  |T| = 1 <= B, and
# 2 A B = 6006 >= m = 2003, so step 7 searches with X = floor(B / |T|) = 1001 > 1000 = ell.  In
# the round x the range of y is max(0, ceil((x - 3)/2003)) = 0 to floor((x + 3)/2003) = 0 for
# every x <= 1000, so the only point of the round is (x, x), which is reduced for x = 1 alone.
# The search is cut with one point found, so step 8 returns NOT_DETERMINED: uniqueness is not
# certified.  (The set of solutions is in fact the single pair (1, 1); the status says nothing
# about existence, which is what SPEC 9.2 asks of it.)
recover 1 mod 2003 with 3 with 1001
# P(12, 6) with A = -1, B = 6.  The box is empty, so NO_SOLUTION (decision S-D5, the same status as
# an empty interval for the reconstruction from a full ball).
recover 6 mod 12 with -1 with 6
# P(12, 6) with A = 1, B = 0.  The box is empty again, since B < 1: NO_SOLUTION.
recover 6 mod 12 with 1 with 0
# A planted fraction of 21 digits in the range 2 A B < m, where the answer is decided without a
# search.  A = 10^20 + 7, B = 10^20 + 9, n = -A, d = B, m = 2 A B + 1 = 2 A B + 1, and
# c = n d^-1 modulo m, which is the number below.  gcd(n, d) = 1 (they differ by 16 and d is odd)
# and gcd(d, m) = 1 (d is odd and m is odd), so (n, d) is a solution; 2 A B < m, so it is the only
# one and step 6 of Algorithm R returns it.  The printer writes it in lowest terms with a positive
# denominator: -100000000000000000007/100000000000000000009.
recover 20000000000000000002800000000000000000098 mod 20000000000000000003200000000000000000127 with 100000000000000000007 with 100000000000000000009
# Bounds of 30 digits.  P(1000003, 1000000) with A = 3 and B = 10^30.  1000003 is prime, A = 3 < m,
# and step 4 of Algorithm R runs one step: with (r0, t0, r1, t1) = (1000003, 0, 1000000, 1) and
# q = 1, the pair is (R', T', R, T) = (1000000, 1, 3, -1), so sigma = -1.  |T| = 1 <= B and
# 2 A B = 6 * 10^30 >= m, so step 7 searches with X = floor(B / |T|) = 10^30 > 1000 = ell.  The
# range of y in the round x is max(0, ceil((3 x - 3)/1000000)) = 0 to floor((3 x + 3)/1000000) = 0
# for every x <= 1000, and the point of the round is (n, d) = (-3 x, x), which is reduced for
# x = 1 alone: the pair (-3, 1), and -3 = 1000000 modulo 1000003, so it is a solution.  One point
# in a search that is cut: NOT_DETERMINED, as above.  The two bounds have 30 digits and are exact
# integers of any size, since adf_resid_reconstruct takes fmpz arguments.
recover 1000000 mod 1000003 with 3 with 1000000000000000000000000000000
# An exact ball is a single rational, and it is in no residue class with m > 1: the status of
# adf_resid_set_fball_forget is DOMAIN, which the driver passes on.
recover 6 mod 0 with 1 with 5
# A bound that is not an integer is data outside the domain of the problem, whose bounds are
# integers (SPEC 9.2): ADF_DOMAIN.
recover 6 mod 12 with 1/2 with 5
recover 6 mod 12 with 1 with 5/2
# A first operand that is not a finite ball of the value form is not read: the first operand is
# "6 mod" here, which no start symbol of conventions 9.2 derives, so the syntax check of step 2
# gives ADF_PARSE.  A second or a fourth operand is a line that is not a sentence of the grammar of
# the driver (three operands): ADF_PARSE as well.
recover 6 mod with 1 with 5
recover 6 mod 12 with 1
recover 6 mod 12 with 1 with 5 with 6
# A first operand of another kind: the residue class is read out of a finite ball, and the two
# other kinds with a parser are refused as for cap (tools/adf/README.md, "Types of the operands"):
# an adele is ADF_UNSUPPORTED, a rational ADF_DOMAIN (it has no finite part to take a residue of).
recover (1 ; 2 mod 6) with 1 with 5
