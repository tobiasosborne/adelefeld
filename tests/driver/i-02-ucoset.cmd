# Operations on unit cosets through the driver (lane t-slice1).  Expected lines written by hand from
# docs/SPEC.md 5 (the product c c' U(gcd(N, N')), the inverse, the exact units), conventions 5.6 (normal form,
# the power c^k U(N) and the tight coset chat^k U(M_k)), include/adelefeld/idpow.h (the examples
# [2 mod 5]^2 = [49 mod 120] tight and [1 mod 1]^2 = [1 mod 24] tight); checked by
# lanes/t-slice1/check_driver_cases.py with exact integers.
#!exit 1
# ---- mul: 5 * 7 = 35 = 11 modulo gcd(36, 12) = 12
mul [5 mod 36] with [7 mod 12]
# an exact unit meets a coset: gcd(0, 9) = 9
mul [1 mod 0] with [5 mod 9]
mul [-1] with [-1]
mul [2 mod 3] with [2 mod 3]
# [7 mod 10] is [2 mod 5]; gcd(5, 4) = 1: the whole group
mul [7 mod 10] with [3 mod 4]
# ---- inv: 5 * 29 = 145 = 4 * 36 + 1
inv [5 mod 36]
inv [1 mod 0]
inv [-1]
inv [2 mod 5]
inv [1 mod 1]
# ---- neg
neg [5 mod 36]
neg [1]
neg [1 mod 1]
neg [-1 mod 0]
# ---- div: [5 mod 36] * [7 mod 12]^-1, and 7 * 7 = 49 = 1 modulo 12
div [5 mod 36] with [7 mod 12]
div [2 mod 5] with [2 mod 5]
div [1] with [-1]
# ---- pow and powtight
pow [5 mod 36] with 3
pow [5 mod 36] with -1
pow [5 mod 36] with 0
pow [2 mod 5] with 2
pow [-1] with 3
pow [-1] with 4
powtight [2 mod 5] with 2
powtight [1 mod 1] with 2
powtight [5 mod 6] with 1
# ---- the set predicates of a unit coset (conventions 5.6): equal, contains, overlaps
equal [5 mod 6] with [2 mod 3]
equal [1] with [1 mod 1]
equal [1] with [1 mod 0]
contains [5 mod 36] with [2 mod 3]
contains [2 mod 3] with [5 mod 36]
contains [1] with [1 mod 1]
overlaps [1 mod 4] with [3 mod 4]
overlaps [1 mod 4] with [5 mod 6]
# ---- the pairs that are not defined
add [1] with [1]
sub [5 mod 6] with [1]
mul [1] with 1/2
mul 1/2 with [1]
div [1] with 1/2
neg [1] with [1]
pow [5 mod 36] with 1/2
pow [5 mod 36] with (1 ; 0)
pow [5 mod 36] with 9223372036854775808
pow [5 mod 36] with 9223372036854775807
dump [5 mod 6]
