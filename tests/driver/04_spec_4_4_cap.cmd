# SPEC 4.4 item 3: the absolute cap. Expected lines written from 4.4 item 3.
#!exit 1
# "after a tight operation the radius R is replaced by gcd(R, C)": gcd(12, 6) = 6
cap (* ; 3 mod 12) with 6
# gcd of the radii as rational numbers: gcd(12, 1/2) = 1/2, and 3 + (1/2) Zhat = (1/2) Zhat
cap (* ; 3 mod 12) with 1/2
# "The cap never touches an exact value"; the literal gcd(0, C) = C would widen it
cap (* ; 7/3) with 2
# "What is not a policy: forcing every result to a fixed radius.
#  (1 mod 2) * (1/2) is 1/2 mod 1" -- the driver prints that text, which is the canonical
#  triple (1, 2, 2) of the result.  The set is 1/2 + Zhat, since (1/2)(2 Zhat) = Zhat;
#  my first hand computation wrongly took it for (1/2) Zhat, which is the larger set that
#  (* ; 0 mod 1/2) denotes.
mul (* ; 1 mod 2) with 1/2
# "writing it as 1/2 mod 2 excludes 3/2 and is wrong": 3/2 is in the true set ...
contains 3/2 with (* ; 1/2 mod 1)
# ... and is not in the set that the text (* ; 1/2 mod 2) denotes
contains 3/2 with (* ; 1/2 mod 2)
# a cap is a positive rational (policies Definition 13); the status is docs/api-m1.md
# "Choices" item 5
cap (* ; 3 mod 12) with 0
cap (* ; 3 mod 12) with -1
