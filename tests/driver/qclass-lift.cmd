#!exit 1
# docs/api-3.md 7: lift value and exact rational translation.
show (0.5 ; 0) + Q
type (0.5 ; 0) + Q
qadd_rat (0 ; 1/3) + Q with -7/3
qadd_rat (0.5 +/- 0.125 ; 1/3 mod 2) + Q with 1/3
add (0.5 ; 0) + Q with 1
sub (0.5 ; 0) + Q with (0.5 ; 0) + Q
mul 1 with (0.5 ; 0) + Q
div (0.5 ; 0) + Q with 1
equal (0.5 ; 0) + Q with (0.5 ; 0) + Q
contains (0.5 ; 0) + Q with 0
overlaps (0.5 ; 0) + Q with 0
compare (0.5 ; 0) + Q with 0
qadd_rat 0 with 1
qadd_rat (0 ; 0) + Q with (0 ; 0)
qadd_rat [1] with 1
show union((0.5 ; 0 mod 1)) + Q
show (0 ; 1/0) + Q
type rfun()
show rfun()
