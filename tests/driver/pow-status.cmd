#!exit 1
# Statuses of powrat_at and powunit_at (lane f-slice9): the reason of each line is in the comment above it.
# 2 has the odd valuation 1: no square root at 2 (DOMAIN); 1 is no square root of 9 modulo 5 (DOMAIN)
powrat_at 2 with 2 with 1/2 with 1
powrat_at 9 with 5 with 3/2 with 1
# 1 + 4 Z_2 is outside the guard (5 is no square): NOT_DETERMINED; a ball around 0: NOT_DETERMINED
powrat_at (* ; 1 mod 4) with 2 with 1/2 with 1
powrat_at (* ; 0 mod 125) with 5 with 1/2 with 0
# 0 to a negative exponent: NOT_UNIT
powrat_at 0 with 5 with -1/2 with 0
# the real place: UNSUPPORTED; 4 is no prime: DOMAIN
powrat_at 9 with real with 1/2 with 1
powrat_at 9 with 4 with 1/2 with 1
# an exponent that is not a rational: DOMAIN; a numerator beyond a word: LIMIT; malformed: PARSE; arity: PARSE
powrat_at 9 with 5 with (* ; 1 mod 2) with 1
powrat_at 9 with 5 with 99999999999999999999/2 with 3
powrat_at 9 with 5 with x with 1
powrat_at 9 with 5 with 3/2
# a seed that is not a word: DOMAIN (as root_at)
powrat_at 9 with 5 with 3/2 with 1/2
# 2 is not 1 mod 5; 1/5 is not in Z_5: DOMAIN
powunit_at 2 with 5 with 1/2
powunit_at 6 with 5 with 1/5
# 5^-1 Z_5 meets Z_5 and its complement; Zhat at 5 is Z_5, which meets 1 + 5 Z_5: NOT_DETERMINED
powunit_at 6 with 5 with (* ; 0 mod 1/5)
powunit_at (* ; 0 mod 1) with 5 with 1/2
# 2 is even: DOMAIN at 2
powunit_at 2 with 2 with 1/3
# the real place: UNSUPPORTED; arity: PARSE
powunit_at 6 with real with 1/2
powunit_at 6 with 5
powunit_at 6 with 5 with 1/2 with 3
