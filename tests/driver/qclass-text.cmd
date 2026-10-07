#!exit 1
# CV-45, conventions 9.3/9.4. Exact dyadic cases; expectations written before running.
print union((0.5 ; 7)) + Q
print union((0.75 ; 1 mod 3), (0.25 ; -1 mod 2), (0.25 ; 1 mod 2)) + Q
print (0.5 ; 1/3 mod 2) + Q
type union((0.5 ; 7)) + Q
print union((1.5 ; 0)) + Q
print union((0.5 ; 1/2)) + Q
print union() + Q
