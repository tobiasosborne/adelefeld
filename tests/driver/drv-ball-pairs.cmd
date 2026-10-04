# A local ball and a partial ball in the commands that combine values (lane drv-ball).  Expected lines
# written from tools/adf/README.md ("The order of the checks") and the report of lane drv-ball, not from the
# program: the two kinds have typed parsers, so step 3 passes; SPEC 4.1 combines no pair of types with them,
# so step 5 answers DOMAIN; the negation and the dump are not implemented for them: UNSUPPORTED.
#!exit 1
add 1/2 with [p=5: 3]
sub 1/2 with [p=5: 3]
mul 1/2 with [p=5: 3]
neg [p=5: 3]
div 1/2 with [p=5: 3]
equal 1/2 with [p=5: 3]
contains 1/2 with [p=5: 3]
overlaps 1/2 with [p=5: 3]
reconstruct [p=5: 3]
cap [p=5: 3] with 2
compare [p=5: 3] with 1/2
show [p=5: 3]
dump [p=5: 3]
add {p=5: 3} with {p=5: 3}
neg {inf: 1}
# step 4 is reached now: the zero denominator and the exponent over the limit are read
add [p=5: 3] with 1/0
add 1/0 with [p=5: 3]
add [p=5: 3] with (1e100001 ; 0)
# the place 4 is not a place: DOMAIN; a local ball is no operand of the commands at places: UNSUPPORTED
exp_at [p=5: 3 + O(5^4)] with 4
exp_at [p=5: 3 + O(5^4)] with 5
