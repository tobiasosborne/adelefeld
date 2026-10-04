# Every operation with each pair of operand types, the refused pairs included.
# The operands are  R = 1/2, F = (* ; 3 mod 12), A = (1 ; 5 mod 18),
# C = ((1) + (2)*i ; 5 mod 18) and O = rfun() (a kind of the value form with no
# typed parser in this build).  Expected lines written by hand from SPEC 4.1 to 4.3.
#!exit 1
# ---- add: all 16 pairs
add 1/2 with 1/2
add 1/2 with (* ; 3 mod 12)
add 1/2 with (1 ; 5 mod 18)
add 1/2 with ((1) + (2)*i ; 5 mod 18)
add (* ; 3 mod 12) with 1/2
add (* ; 3 mod 12) with (* ; 3 mod 12)
add (* ; 3 mod 12) with (1 ; 5 mod 18)
add (* ; 3 mod 12) with ((1) + (2)*i ; 5 mod 18)
add (1 ; 5 mod 18) with 1/2
add (1 ; 5 mod 18) with (* ; 3 mod 12)
add (1 ; 5 mod 18) with (1 ; 5 mod 18)
add (1 ; 5 mod 18) with ((1) + (2)*i ; 5 mod 18)
add ((1) + (2)*i ; 5 mod 18) with 1/2
add ((1) + (2)*i ; 5 mod 18) with (* ; 3 mod 12)
add ((1) + (2)*i ; 5 mod 18) with (1 ; 5 mod 18)
add ((1) + (2)*i ; 5 mod 18) with ((1) + (2)*i ; 5 mod 18)
# ---- sub: all 16 pairs
sub 1/2 with 1/2
sub 1/2 with (* ; 3 mod 12)
sub 1/2 with (1 ; 5 mod 18)
sub 1/2 with ((1) + (2)*i ; 5 mod 18)
sub (* ; 3 mod 12) with 1/2
sub (* ; 3 mod 12) with (* ; 3 mod 12)
sub (* ; 3 mod 12) with (1 ; 5 mod 18)
sub (* ; 3 mod 12) with ((1) + (2)*i ; 5 mod 18)
sub (1 ; 5 mod 18) with 1/2
sub (1 ; 5 mod 18) with (* ; 3 mod 12)
sub (1 ; 5 mod 18) with (1 ; 5 mod 18)
sub (1 ; 5 mod 18) with ((1) + (2)*i ; 5 mod 18)
sub ((1) + (2)*i ; 5 mod 18) with 1/2
sub ((1) + (2)*i ; 5 mod 18) with (* ; 3 mod 12)
sub ((1) + (2)*i ; 5 mod 18) with (1 ; 5 mod 18)
sub ((1) + (2)*i ; 5 mod 18) with ((1) + (2)*i ; 5 mod 18)
# ---- mul: all 16 pairs
mul 1/2 with 1/2
mul 1/2 with (* ; 3 mod 12)
mul 1/2 with (1 ; 5 mod 18)
mul 1/2 with ((1) + (2)*i ; 5 mod 18)
mul (* ; 3 mod 12) with 1/2
mul (* ; 3 mod 12) with (* ; 3 mod 12)
mul (* ; 3 mod 12) with (1 ; 5 mod 18)
mul (* ; 3 mod 12) with ((1) + (2)*i ; 5 mod 18)
mul (1 ; 5 mod 18) with 1/2
mul (1 ; 5 mod 18) with (* ; 3 mod 12)
mul (1 ; 5 mod 18) with (1 ; 5 mod 18)
mul (1 ; 5 mod 18) with ((1) + (2)*i ; 5 mod 18)
mul ((1) + (2)*i ; 5 mod 18) with 1/2
mul ((1) + (2)*i ; 5 mod 18) with (* ; 3 mod 12)
mul ((1) + (2)*i ; 5 mod 18) with (1 ; 5 mod 18)
mul ((1) + (2)*i ; 5 mod 18) with ((1) + (2)*i ; 5 mod 18)
# ---- neg: one operand
neg 1/2
neg (* ; 3 mod 12)
neg (1 ; 5 mod 18)
neg ((1) + (2)*i ; 5 mod 18)
# ---- div: the second operand is an exact rational; every other type is refused
div 1/2 with 1/2
div (* ; 3 mod 12) with 1/2
div (1 ; 5 mod 18) with 1/2
div ((1) + (2)*i ; 5 mod 18) with 1/2
div 1/2 with (* ; 3 mod 12)
div 1/2 with (1 ; 5 mod 18)
div 1/2 with ((1) + (2)*i ; 5 mod 18)
div (* ; 3 mod 12) with (* ; 3 mod 12)
div (* ; 3 mod 12) with (1 ; 5 mod 18)
div (* ; 3 mod 12) with ((1) + (2)*i ; 5 mod 18)
div (1 ; 5 mod 18) with (* ; 3 mod 12)
div (1 ; 5 mod 18) with (1 ; 5 mod 18)
div (1 ; 5 mod 18) with ((1) + (2)*i ; 5 mod 18)
div ((1) + (2)*i ; 5 mod 18) with (* ; 3 mod 12)
div ((1) + (2)*i ; 5 mod 18) with (1 ; 5 mod 18)
div ((1) + (2)*i ; 5 mod 18) with ((1) + (2)*i ; 5 mod 18)
# ---- equal: all 16 pairs
equal 1/2 with 1/2
equal 1/2 with (* ; 3 mod 12)
equal 1/2 with (1 ; 5 mod 18)
equal 1/2 with ((1) + (2)*i ; 5 mod 18)
equal (* ; 3 mod 12) with 1/2
equal (* ; 3 mod 12) with (* ; 3 mod 12)
equal (* ; 3 mod 12) with (1 ; 5 mod 18)
equal (* ; 3 mod 12) with ((1) + (2)*i ; 5 mod 18)
equal (1 ; 5 mod 18) with 1/2
equal (1 ; 5 mod 18) with (* ; 3 mod 12)
equal (1 ; 5 mod 18) with (1 ; 5 mod 18)
equal (1 ; 5 mod 18) with ((1) + (2)*i ; 5 mod 18)
equal ((1) + (2)*i ; 5 mod 18) with 1/2
equal ((1) + (2)*i ; 5 mod 18) with (* ; 3 mod 12)
equal ((1) + (2)*i ; 5 mod 18) with (1 ; 5 mod 18)
equal ((1) + (2)*i ; 5 mod 18) with ((1) + (2)*i ; 5 mod 18)
# ---- contains: all 16 pairs
contains 1/2 with 1/2
contains 1/2 with (* ; 3 mod 12)
contains 1/2 with (1 ; 5 mod 18)
contains 1/2 with ((1) + (2)*i ; 5 mod 18)
contains (* ; 3 mod 12) with 1/2
contains (* ; 3 mod 12) with (* ; 3 mod 12)
contains (* ; 3 mod 12) with (1 ; 5 mod 18)
contains (* ; 3 mod 12) with ((1) + (2)*i ; 5 mod 18)
contains (1 ; 5 mod 18) with 1/2
contains (1 ; 5 mod 18) with (* ; 3 mod 12)
contains (1 ; 5 mod 18) with (1 ; 5 mod 18)
contains (1 ; 5 mod 18) with ((1) + (2)*i ; 5 mod 18)
contains ((1) + (2)*i ; 5 mod 18) with 1/2
contains ((1) + (2)*i ; 5 mod 18) with (* ; 3 mod 12)
contains ((1) + (2)*i ; 5 mod 18) with (1 ; 5 mod 18)
contains ((1) + (2)*i ; 5 mod 18) with ((1) + (2)*i ; 5 mod 18)
# ---- overlaps: all 16 pairs
overlaps 1/2 with 1/2
overlaps 1/2 with (* ; 3 mod 12)
overlaps 1/2 with (1 ; 5 mod 18)
overlaps 1/2 with ((1) + (2)*i ; 5 mod 18)
overlaps (* ; 3 mod 12) with 1/2
overlaps (* ; 3 mod 12) with (* ; 3 mod 12)
overlaps (* ; 3 mod 12) with (1 ; 5 mod 18)
overlaps (* ; 3 mod 12) with ((1) + (2)*i ; 5 mod 18)
overlaps (1 ; 5 mod 18) with 1/2
overlaps (1 ; 5 mod 18) with (* ; 3 mod 12)
overlaps (1 ; 5 mod 18) with (1 ; 5 mod 18)
overlaps (1 ; 5 mod 18) with ((1) + (2)*i ; 5 mod 18)
overlaps ((1) + (2)*i ; 5 mod 18) with 1/2
overlaps ((1) + (2)*i ; 5 mod 18) with (* ; 3 mod 12)
overlaps ((1) + (2)*i ; 5 mod 18) with (1 ; 5 mod 18)
overlaps ((1) + (2)*i ; 5 mod 18) with ((1) + (2)*i ; 5 mod 18)
# ---- reconstruct: an adele, or a finite ball and an interval given as two rationals
reconstruct (2 ; 2 mod 1)
reconstruct (1 ; 1)
reconstruct (0 ; 3 mod 12)
reconstruct (* ; 1 mod 2) with 1 with 1
reconstruct (* ; 1 mod 2) with 0 with 10
reconstruct (* ; 1 mod 2) with 2 with 0
reconstruct (* ; 1 mod 2)
reconstruct 1/2
reconstruct 1/2 with 0 with 1
reconstruct (* ; 1 mod 2) with 0
reconstruct ((1) + (2)*i ; 0)
# ---- cap: a finite ball and a rational
cap (* ; 3 mod 12) with 6
cap (* ; 3 mod 12) with 1/2
cap (* ; 7/3) with 2
cap (* ; 3 mod 12) with 0
cap (* ; 3 mod 12) with (* ; 1 mod 2)
cap 1/2 with 2
cap (1 ; 1) with 2
cap ((1) + (2)*i ; 1) with 2
# ---- a kind of the value form with no typed parser: UNSUPPORTED, before every other check
add 1/2 with rfun()
sub 1/2 with rfun()
mul 1/2 with rfun()
neg rfun()
div 1/2 with rfun()
equal 1/2 with rfun()
contains 1/2 with rfun()
overlaps 1/2 with rfun()
reconstruct rfun()
cap rfun() with 2
# ---- show and type
show 1/2
show (* ; 3 mod 12)
show (1 ; 5 mod 18)
show ((1) + (2)*i ; 5 mod 18)
show rfun()
type 1/2
type (* ; 3 mod 12)
type (1 ; 5 mod 18)
type ((1) + (2)*i ; 5 mod 18)
type [5 mod 6]
