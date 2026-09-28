# SPEC 4.2: set predicates. Expected lines written from the table and the text of 4.2.
#!exit 0
# row "equal_set": N = M and (a - b)/N is an integer
equal (* ; 0 mod 2) with (* ; 0 mod 2)
equal (* ; 0 mod 2) with (* ; 0 mod 1)
# row "overlaps": (a - b)/gcd(N, M) is an integer
overlaps (* ; 0 mod 2) with (* ; 0 mod 1)
overlaps (* ; 0 mod 1) with (* ; 1 mod 2)
overlaps (* ; 0 mod 2) with (* ; 1 mod 2)
# row "contains" (first inside second): N/M is an integer and (a - b)/M is an integer
contains (* ; 0 mod 1) with (* ; 0 mod 2)
contains (* ; 0 mod 2) with (* ; 0 mod 1)
contains (* ; 1 mod 2) with (* ; 0 mod 2)
# 4.2: "Radius zero is handled as a single point"
equal (* ; 7/3) with 7/3
equal (* ; 7/3) with 8/3
overlaps (* ; 7/3) with 7/3
overlaps (* ; 7/3) with 8/3
contains (* ; 7/3) with (* ; 7/3)
contains 7/3 with (* ; 7/3)
contains 8/3 with (* ; 7/3)
# 4.2: "Overlap is not equality and is not transitive"
equal (* ; 0 mod 2) with (* ; 0 mod 1)
