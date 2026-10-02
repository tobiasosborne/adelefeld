# f-places-hostile: hostile input of project, exp_at, log_at (tools/adf/README.md, "The commands at places").  The
# order of the checks is that of the driver: 1. the arity of the line (a missing operand is PARSE); 2. the syntax of
# the value operand, then of every token of the list of places (PARSE); 3. the number of places of exp_at and log_at;
# 4. the kind of the value; 5. a token that is no place (DOMAIN); the type of the value against the places; the
# projection (a repeated place is DOMAIN); the function.  Each expected line follows from that order and from the
# statuses of the library, not from the output of the program.
#!exit 1
# a missing operand or a missing separator: PARSE
exp_at 5
log_at 5 with
project
project 5 with
exp_at with 5
# the value operand is not a value: PARSE, also when the places are not places either
exp_at abc with 5
project abc with 4
# a token that is not the word real or a decimal: PARSE (a syntax fault before any value is judged)
exp_at 5 with abc
exp_at 5 with +5
exp_at 5 with 05
exp_at 5 with 5.0
exp_at 5 with Real
exp_at 5 with 5 
exp_at 5 with 5	
project 5 with 5  7
project 5 with 2 x
project 5 with 4 x
# a token that is a decimal but no place: DOMAIN (0, 1, negative, composite, 64 bits and more)
exp_at 5 with 0
exp_at 5 with 1
exp_at 5 with -3
exp_at 5 with -0
exp_at 5 with 4
exp_at 5 with 18446744073709551615
exp_at 5 with 18446744073709551616
exp_at 5 with 123456789012345678901234567890123456789012345678901234567890
project 5 with 2 4
project 5 with 4 2
# exp_at and log_at take exactly one place: PARSE, decided before the DOMAIN of a token
exp_at 5 with 5 5
exp_at 5 with 4 4
log_at 6 with real 5
# a repeated place: DOMAIN
project 5 with 5 5
project 5 with 2 3 2
project 5 with real real
# the function fails at the place: DOMAIN (a unit at 5; a rational with 5 in the denominator)
exp_at 5 with 3
# a value that the driver cannot use, decided before the places are judged: UNSUPPORTED before DOMAIN
exp_at [p=5: 3 + O(5^4)] with 4
exp_at ((1) + (2)*i ; 5 mod 18) with 5
log_at ((1) + (2)*i ; 6 mod 18) with real
# tan is not a command; the name is not one of the table: PARSE
tan_at 5 with 5
