# u-dump-units: dump and load of a unit coset, an idele and an idele class (docs/conventions.md 10,
# the bodies "ucoset", "idele" and "idclass" of 10.1 and the rules of 10.2; include/adelefeld/dump.h).
# Lane u-dump1.  Every expected line of u-dump-units.out is written by hand from conventions 10.1
# (the order of the fields), from the predicates of 5.6 and 5.7, and from the templates of the value
# form of conventions 9.4, not from the output of the program.
#!exit 1
# dump: two fields, c and N, the stored pair (conventions 10.1, line 1345).  The reader of the value
# form reduces c into 1..N but keeps the modulus as supplied (conventions 5.6, CV-17), and the
# printer shows the normal form: [5 mod 6] is stored as (5, 6) and printed as [2 mod 3].
dump [5 mod 6]
dump [1 mod 0]
dump [-1 mod 0]
dump [1 mod 1]
dump [3 mod 4]
# the idele: the archimedean count 1, the ball as arb_dump_str writes it (the odd midpoint mantissa
# and its exponent, then the radius mantissa and its exponent), then num(r) den(r), then c and N
dump (1 ; 1 * [1])
dump (-1 ; 1 * [-1])
dump (2 ; 3/2 * [2 mod 3])
dump (2.5 +/- 0.25 ; 3/2 * [5 mod 6])
# the class: no count (the type is special to Q, conventions 10.1), the ball, then c and N
dump <1 ; [1]>
dump <2 ; [2 mod 3]>
dump <1.25 +/- 0.25 ; [5 mod 36]>
# load: the value form of conventions 9.4 of the value the text denotes
load adf1 Q ucoset 5 6
load adf1 Q ucoset 1 0
load adf1 Q ucoset -1 0
load adf1 Q idele 1 1 0 0 0 1 1 1 0
load adf1 Q idele 1 -1 0 0 0 1 1 -1 0
load adf1 Q idclass 1 0 0 0 1 0
load adf1 Q idclass 3 1 0 0 5 6
# the two directions of one value: the dump of 10.1 of the value of the line above is the text the
# line reads, and the other way round
dump (2 ; 3/2 * [2 mod 3])
# strictness (conventions 10.2): a field that breaks the predicate of 5.6 or of 5.7 is DOMAIN
load adf1 Q ucoset 2 4
load adf1 Q ucoset 2 0
load adf1 Q ucoset 6 4
load adf1 Q ucoset 1 -3
load adf1 Q idele 1 0 0 0 0 1 1 1 0
load adf1 Q idele 1 1 0 1 0 1 1 1 0
load adf1 Q idele 1 1 0 0 0 6 4 1 0
load adf1 Q idele 1 1 0 0 0 1 1 2 1
load adf1 Q idele 0 1 1 1 0
load adf1 Q idclass 1 0 1 0 1 0
load adf1 Q idclass -1 0 0 0 1 0
load adf1 Q idclass 1 0 0 0 2 4
# a text that is no sentence of the grammar of 10.1 is PARSE
load adf1 Q ucoset 1 0 0
load adf1 Q ucoset 01 6
load adf1 Q ucoset 1 A
load adf1 Q idele 1 1 0 0 0 3 2 5
# another version or another field is UNSUPPORTED (conventions 10.2, "Version and field")
load adf2 Q ucoset 1 0
load adf1 K ucoset 1 0
