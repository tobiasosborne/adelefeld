# The commands the driver got with the library: compare (adf_fball_compare, the three-valued
# comparison of SPEC 4.2), and dump and load (the dump form of conventions 10, without
# contexts).  The texts of the dump form are the ones of tests/golden/dump.tsv.
#!exit 1
# compare: SPEC 4.2, last sentence: "the comparison of points returns one of 'certainly
# equal' (both exact), 'certainly different' (disjoint), 'undecided'".  The driver prints the
# names of the three values of adf_fball_compare: ADF_CMP_EQUAL, ADF_CMP_DIFFERENT,
# ADF_CMP_UNDECIDED.  As for the other predicates of 4.2, an exact rational is the ball of
# radius 0, so a rational operand is read.
# both operands exact and equal
compare 7/3 with 7/3
# both exact and different: the two points are disjoint
compare 7/3 with 8/3
# the same set, but neither value is a point: the two unknown points may or may not be equal,
# so the comparison is undecided even though equal_set of SPEC 4.2 is true
compare (* ; 1 mod 2) with (* ; 5 mod 2)
# the sets meet ((0 - 1)/gcd(2, 3) = -1 is an integer) and neither is a point: undecided
compare (* ; 0 mod 2) with (* ; 1 mod 3)
# disjoint: (0 - 1)/gcd(2, 4) = -1/2 is not an integer
compare (* ; 0 mod 2) with (* ; 1 mod 4)
# the predicates of SPEC 4.2 are predicates of two finite balls; an adele is not one
compare (1 ; 0) with 1/2
# a kind with no typed parser is UNSUPPORTED, before any value is read
compare (0.5 ; 0) + Q with 1/2
# dump: the dump form of conventions 10.1 of a value the driver read.  Every value of the
# value form is in the global backend (conventions 9.8, A11), so its dump has the form "g"
# and no context occurrence.
dump 7/3
dump (* ; 2 mod 6)
# the real ball of the dump is arb_dump_str: the odd mantissa and the exponent of the
# midpoint, then the 30-bit mantissa and the exponent of the radius (conventions 10.2).
dump (1 ; 0)
dump ((1) + (0.5)*i ; 2 mod 6)
# a kind with no typed parser in this build; the dump form of 10.1 has no body for it here
dump (0.5 ; 0) + Q
# the value is read first: a rat with a zero denominator is DOMAIN
dump 1/0
# load: a dump text read into a value and printed in the value form of conventions 9.4.
load adf1 Q rat 7 3
load adf1 Q fball g 2 6 1
load adf1 Q adele 1 1 -1 0 0 g 0 0 1
load adf1 Q cadele 1 1 0 0 0 1 -1 0 0 g 2 6 1
# the two directions of one value: the dump of 10.1 of the value of the line above is the text
# the line below reads, and the other way round.
dump ((1) + (0.5)*i ; 2 mod 6)
# a dump with a context occurrence needs a context, and the driver has none (conventions
# 10.2: one occurrence per local finite ball)
load adf1 Q fball l 1 6 2 2 3 0 2
# a body of 10.1 that the driver has no type for
load adf1 Q modctx 6 2 2 3
# a local ball: the body lball of 10.1, "b u v N" (lane u-dump1)
load adf1 Q lball 5 b 3 0 4
# "real" is not a body of 10.1, so the text is not a sentence of the grammar of section 10
load adf1 Q real 7 3
# a version other than 1, and a field other than Q (conventions 10.1)
load adf2 Q rat 7 3
load adf1 K rat 7 3
# the raw triple (A, H, d) = (2, 4, 1) breaks the predicate G of conventions 5.2
load adf1 Q rat 2 4
# a leading zero in a hexadecimal integer of 10.1 is a PARSE
load adf1 Q rat 07 3
