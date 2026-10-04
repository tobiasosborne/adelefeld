# drv-ball-values: the local ball and the partial ball as kinds of the value form of the driver
# (conventions 9.2, 9.3 and 9.4 of docs/conventions.md; include/adelefeld/text.h lines 286 to 342, the
# readers and the printers).  Lane drv-ball, step 1.
#
# Every expected line of drv-ball-values.out is written from those sections and from the golden vectors
# tests/golden/lball.tsv and tests/golden/sball.tsv, not from the output of the program.
#!exit 1
# a local ball at 2, at 3, at 5 and at the prime 2^64 - 59 = 18446744073709551557 (conventions 9.3: the
# prime p of a local entry satisfies p < 2^64)
show [p=2: 1 + O(2^3)]
show [p=3: 2/3 + O(3^0)]
show [p=5: 1/125 + O(5^-2)]
show [p=18446744073709551557: 1 + O(18446744073709551557^1)]
# an exact local value (conventions 5.8: an exact local ball is the rational p^v u) and the exact 0 at 2
show [p=5: 1/5]
show [p=2: 0]
# a partial ball with the real place and two primes, and the partial ball with no place (conventions 9.4).
# The real component prints with the radius the ball holds after the reading at prec 64, which is at least
# 1e-9 and at most 1e-9 + 2^-63: conventions 9.5 gives 1.1e-9 (the golden vector tests/golden/sball.tsv
# line 2 gives the expected column as an enclosure, not as bytes; conventions 9.6, gate finding G4: a
# decimal string is an enclosure on input, not a lossless form).
show {inf: 1.5 +/- 1e-9; p=2: 1 + O(2^3); p=5: 3 + O(5^4)}
show {}
# type names the two kinds (conventions 9.7: the classifier names all thirteen kinds)
type [p=5: 3 + O(5^4)]
type {inf: 1}
# canonicalisation on input (conventions 9.3, line 1174, and conventions 7): the centre is reduced into
# [0, p^N), a blank inside a text is the whitespace of conventions 8.2, and the entries are sorted into
# the canonical order of places
show [p=5: 28 + O(5^2)]
show { p = 5 : 28 + O(5^2) ; p=3 : 1/3 }
# the statuses of the two readers (conventions 9.3, the table "Semantic constraints").  A text that is
# no sentence of the grammar is PARSE, which adf_text_classify decides in step 2 of the command.
show [p=5: 3 + O(5^2.5)]
show {p=5: 1;}
# a prime that is not prime, and the same prime twice: DOMAIN (stage 6 of conventions 8.5)
show [p=4: 1]
show {p=4: 1}
show {p=5: 1; p=5: 2}
# a prime p >= 2^64: UNSUPPORTED (stage 5)
show [p=18446744073709551617: 1]
show {p=18446744073709551617: 1}
# abs(N) above ADF_LBALL_EXP_MAX of adelefeld/lball.h, and a decimal exponent above the limit of
# conventions 8.4: LIMIT
show [p=5: 3 + O(5^100001)]
show {inf: 1e100001}
# the session digits: show prints the real component with n = digits.  The exact interval of the text
# and the ball of the arb at prec 64 (its radius is near 2^-63) print alike: proto/text_grammar.py
# print_real at n = 5 gives 1.2346 +/- 3.3e-5 for the radius 1e-9 and for the radii 2^-40, 2^-50, 2^-60,
# 2^-64 and 2^-70.
digits 5
show {inf: 1.23456789 +/- 1e-9}
