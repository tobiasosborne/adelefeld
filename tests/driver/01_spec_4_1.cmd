# SPEC 4.1: values. Expected lines written from docs/SPEC.md 4.1, not from the program.
#!exit 0
# the table of 4.1: the exact rational is a type of its own
type 7/3
show 7/3
# the table of 4.1: a finite ball of radius 0 is a single rational; "mod 0" is dropped
type (* ; 7/3)
show (* ; 7/3)
show (* ; 7/3 mod 0)
# the table of 4.1: the centre of a finite ball is reduced into [0, N)
show (* ; 15 mod 12)
# the table of 4.1: an adele and a complex adele
show (1 ; 5 mod 18)
show ((1) + (2)*i ; 5 mod 18)
# the table of 4.1: a local ball and a partial ball are kinds of the value form (9.7); the driver
# reads and prints them through the library since lane drv-ball (conventions 9.4)
show [p=5: 3 + O(5^4)]
show {inf: 1; p=5: 2 + O(5^2)}
# 4.1: "It stays exact under arithmetic with other exact values"
mul 7/3 with 3/7
# 4.1: "(i ; 0) squares to (-1 ; 0), which is not the -1 of the ring"
mul ((0) + (1)*i ; 0) with ((0) + (1)*i ; 0)
neg (1 ; 1)
sub ((-1) + (0)*i ; 0) with (-1 ; -1)
# 4.1: the canonical form of a finite ball: the centre is in [0, N) and gcd(A, H, d) = 1.
# The raw form (2 + 2 Zhat)/2 of that paragraph is not observable in the value form (A11);
# the canonical form (0 + 1 Zhat)/1 of it is Zhat, printed (* ; 0 mod 1).
show (* ; 1/2 mod 1)
add (* ; 1/2 mod 1) with (* ; 1/2 mod 1)
