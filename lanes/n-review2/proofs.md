# Exact arithmetic in this review

These are own proofs about the algorithm in docs/conventions.md 9.5. They use no external theorem.
X(t) means floor(log10(abs(t))) for nonzero t, as in that contract.

## A. The sign predicate is not monotone in the level

Take mid = 793/512, rad = 741/512, n = 1. The exact lower endpoint is 52/512 = 13/128 > 0.
Both numbers are dyadic. The radius has only 10 significant binary bits, so this is an exact stored ball.

1. At k = 2, nk = n + k - 2 = 1. X(mid) = X(rad) = 0, so q = max(0, -1) = 0.
2. Rounding mid to the nearest integer gives M = 2.
3. E = rad + abs(M - mid) = (741 + 1024 - 793)/512 = 243/128 = 1.8984375.
4. Rounding E upwards to two significant decimal digits gives R = 19/10.
5. q' = max(X(M) - nk + 1, X(R) - k + 1) = max(0, -1) = 0.
   M is a multiple of 10^q'. The level ends with 2 +/- 1.9. Its lower endpoint is 1/10 > 0.
6. At k = 3, nk = 2 and q = max(-1, -2) = -1. M = 3/2.
7. E = (741 + 793 - 768)/512 = 383/256 = 1.49609375.
   Upward rounding to three significant digits gives R = 3/2.
8. q' = max(-1, -2) = -1, and M is a multiple of 10^q'. The level ends with 1.5 +/- 1.5.
   Its lower endpoint is 0. It fails both positivity and exclusion of zero.

Thus k1 = 2 succeeds and k2 = 3 fails. This refutes monotonicity starting at 2.
It does not refute eventual monotonicity after a suitable input-dependent level.
monotonicity.py and printer_probe level check the Python and C implementations independently.
printer_probe counterexample checks a canonical idele and class, including the pre-repair printers.

## B. A necessary lower bound does not require monotonicity

Assume mid > rad > 0, and write delta = mid - rad. Fix a level k that succeeds.

1. Set U = 10^(X(rad) - k + 1).
2. E = rad + abs(M - mid) >= rad. The rounding unit of R is 10^(X(E) - k + 1).
   This is an integer multiple of U, so R is a multiple of U.
3. The final q' is at least X(R) - k + 1. R >= rad, so q' >= X(rad) - k + 1.
   The termination test says M is a multiple of 10^q'. Hence M is a multiple of U.
4. Success says M - R > 0. Since it is an integer multiple of U, M - R >= U.
5. Enclosure says M - R <= delta. Therefore U <= delta.
6. The exponent of U is an integer. Taking decimal orders gives
   X(rad) - k + 1 <= X(delta), or k >= X(rad) - X(delta) + 1.

All levels below max(2, X(rad) - X(delta) + 1) can therefore be skipped.
This proof says nothing about nesting or monotonicity of the later levels.
It uses the exact margin, not just the separate stored exponents of midpoint and radius.
The current printer already forms the exact rational midpoint and radius before the search.
family_scan.c uses this bound and charges S for every skipped level, as the original search would.

## C. Exact work of the reported power-of-two family

Let b >= 2, r = 2^(b-1), mid = r + 1/2, rad = r, n = 1. Put D = X(r).

1. r is an even integer. mid is below the next power of ten: both r and that power are integers.
   Thus X(mid) = X(rad) = D.
2. delta = 1/2. Part B gives k >= D + 2 for every successful level.
3. At k = D + 2, nk = D + 1 and q = 0. The tie-to-even rule rounds r + 1/2 down to even r.
4. E = r + 1/2 is an exact decimal with D + 2 significant digits. Thus R = r + 1/2 exactly.
   q' = 0; M = r passes the multiple test. The lower endpoint is -1/2, so this level fails.
5. At k = D + 3, nk = D + 2 and q = -1. The midpoint is now represented exactly.
   E = r, R = r exactly, and q' = -1. The level succeeds with the original exact interval.
6. The next pass starts with exactly the same rational pair. It repeats the same levels and returns
   the same text. There are exactly two passes, each forming D + 2 levels.
7. The reduced midpoint is (2^b + 1)/2. Its numerator has b + 1 bits. The radius has b bits.
   Hence S = max(64, b + 1) in both passes. Total work is 2 (D + 2) max(64, b + 1).
8. D and S are nondecreasing with b. The total work is therefore nondecreasing.
9. At b = 7462, D = 2245, S = 7463, and work = 2 * 2247 * 7463 = 33538722 <= 33554432.
10. At b = 7463, D = 2246, S = 7464, and work = 2 * 2248 * 7464 = 33558144 > 33554432.

Therefore b = 7463 is the smallest refused member for b >= 2. The b = 1 ball also prints.
The direct calls at b = 7462 and 7463 agree. The exact replay of 2000 inputs agrees as well.
This is the family named in Statement Q. The estimate "from about 5500 bits" is inaccurate.

## D. Conditional rounding proof for C2

The universal libm premise below has not been established from a source on disk. This is a conditional proof.
It is not a proof that the installed log2 meets that premise on every input.

Hypotheses: double has radix 2 and 53 significand bits; integer conversion and division have relative error
at most u = 2^-52; log2 on the converted argument has relative error at most 2^-51. The numerator B is
an exactly represented integer, 1 <= B <= 2^28. The requested BITS_MAX range, B <= 2^26, is smaller.

1. Let L = log2(p), 2 <= p < 2^64. Then L >= 1. Write the converted p as p (1 + d), abs(d) <= u.
2. The integral of 1/t from 1 to 2 is at least 1/2. The integral between 1 and 1 + d has absolute
   value at most abs(d)/(1-abs(d)). Hence abs(log2(1+d)) <= 2u/(1-u) < 2^-50.
3. Let ell be the computed logarithm. By the libm hypothesis,
   abs(ell-L)/L <= 2^-50 + 2^-51 (1 + 2^-50) < 2^-49.
4. Write ell = L (1 + e), abs(e) < 2^-49. Let the division error be h, abs(h) <= 2^-52.
   The computed t is (B/L) (1+h)/(1+e).
5. Consequently abs(t-B/L) < B (2^-52 + 2^-49)/(1-2^-49) < B * 2^-48.
   This is below 2^-20 for B <= 2^28, and below 2^-22 for B <= 2^26.
6. If v = v_p(H), then p^v <= H < 2^B, so v < B/L. Thus t > v - 1.
7. t is positive and below 2^29, so the unsigned conversion is in range and truncates to floor(t).
   The code adds integer 1, not a rounded floating-point 1. Therefore floor(t) + 1 >= v.

arithmetic_checks.py verifies the rational constants in steps 3 and 5.
double_bound.c checks the actual logarithm and quotient against outward Arb intervals on 1191 inputs,
including integers adjacent to powers of two and 1000 generated primes. It does not discharge the universal
libm premise. No numerical counterexample was found in 4764 bound checks.

[source pending: a documented error bound for the installed C libm log2 over these positive inputs]
[source pending: a source under refs/ for the conversion and division rounding model used in Part D]
