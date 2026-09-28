# SPEC 4.3: the table of the tight operations. Expected lines written from the table of 4.3.
#!exit 0
# (3 mod 12) + (5 mod 18) = 2 mod 6
add (* ; 3 mod 12) with (* ; 5 mod 18)
# (3 mod 12) * (5 mod 18) = 3 mod 6
mul (* ; 3 mod 12) with (* ; 5 mod 18)
# 12 * (5 mod 18) = 60 mod 216
mul 12 with (* ; 5 mod 18)
# (1/3) * (5 mod 18) = 5/3 mod 6
mul 1/3 with (* ; 5 mod 18)
# (1/2 mod 8) * (2/3 mod 9) = 0 mod 1/6
mul (* ; 1/2 mod 8) with (* ; 2/3 mod 9)
