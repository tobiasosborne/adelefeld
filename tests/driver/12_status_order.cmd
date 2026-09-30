# The order of the checks of one command (tools/adf/README.md, "The order of the checks").
# The driver decides in this order, and stops at the first step that fails:
#   1. the line: an operation word of the table and the right number of operands, else PARSE;
#   2. every operand, in order: the syntax of the value form alone, adf_text_classify
#      (conventions 8.5, stages 1 to 3), else PARSE, or LIMIT for a text over max_len;
#   3. every operand, in order: its kind; a kind of 9.7 with no typed parser in this build is
#      UNSUPPORTED, before any value of the line is read;
#   4. every operand, in order: its value, by the typed parser of its kind (conventions 8.5,
#      stages 4 to 7), else LIMIT, DOMAIN or NOT_DETERMINED;
#   5. the operation: the pair of types, the domain of the operation, else DOMAIN, NOT_UNIT,
#      NO_SOLUTION or NOT_UNIQUE;
#   6. the printer: a printer that returns NULL gives LIMIT (M1-D6).
# A line that is not a sentence of the grammar of 9.2 is decided on the syntax of its operands
# first: that is a fact about the line, and no other operand makes it a sentence.  A kind that
# this build does not implement is decided before any value is read: a request on a type that
# version 1 does not implement is not a domain error (conventions 3.1, ADF_UNSUPPORTED).
#!exit 1
# Step 3 before step 4: 1/0 is a sentence of the value form (a rat whose denominator is 0),
# and [p=5: 3] is a sentence (a local ball), whose kind has no typed parser.  The zero
# denominator, which the typed parser would report as DOMAIN, is not read.
add [p=5: 3] with 1/0
# The same with the operands in the other order: the status does not depend on which operand
# has the unimplemented kind.
add 1/0 with [p=5: 3]
# The same: max_exp10 = 100000 is a limit of stage 4, read at step 4, after the kind of every
# operand has been decided.
add [p=5: 3] with (1e100001 ; 0)
# Step 3 for the second end point of the interval of reconstruct: it is a sentence of the
# value form, of a kind with no typed parser.  (Before this repair the two end points went
# straight to adf_rat_set_str and the line was PARSE.)
reconstruct (* ; 1 mod 2) with [p=5: 3] with 1
reconstruct (* ; 1 mod 2) with 0 with [p=5: 3]
# Step 5: both kinds are implemented and both values are read; the pair of types is not one
# that SPEC 4.1 combines, or the second operand is of the wrong kind for the operation.
cap (* ; 3 mod 12) with (* ; 1 mod 2)
reconstruct (* ; 1 mod 2) with (* ; 0 mod 1) with 1
reconstruct (* ; 1 mod 2) with (0 ; 0) with 1
# Step 2 before step 3: @@@ is not a sentence of the grammar of 9.2, so the line is PARSE
# whichever other operand it holds.
add (@@@ ; 0) with [p=5: 3]
add [p=5: 3] with @@@
# Step 4 before step 5: both kinds are implemented, and the value of the first operand is read
# before the value of the second.  The exponent of 1e100001 is over max_exp10 (a limit of
# stage 4), and the zero denominator of the other operand is a constraint of stage 6.
add (1e100001 ; 0) with (1 ; 1/0)
# The other way round: the first value read is the one that fails, so its status is reported
# and the limit of the second operand is not reached.  The finite part of an adele of the
# value form is q(a) or q(a) mod q(N), never a literal with an exponent, so the second
# operand of the next line is a sentence of the grammar with a zero denominator.
add (1 ; 1/0) with (1e100001 ; 0)
add (1 ; 1) with (1 ; 1/0)
