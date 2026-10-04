# u-dump-local: dump and load of a local ball and of a partial ball (docs/conventions.md 10, the
# bodies "lball" and "sball" of 10.1 and the rules of 10.2; include/adelefeld/dump.h).  Lane u-dump1.
# Every expected line of u-dump-local.out is written by hand from conventions 10.1 (the order of the
# fields), from the predicates of 5.8 and 5.9, and from the templates of the value form of
# conventions 9.4, not from the output of the program.
#!exit 1
# dump of a local ball: the prime, then the form "x" (num(u) den(u) v) of an exact value or "b"
# (u v N) of a ball (conventions 10.1, lines 1337-1338; predicate of 5.8).  The stored centre is the
# one of 5.8, the unique element of Z[1/p] in [0, p^N).  For an exact value u is a unit at p, so
# the rational 1/5 is stored as p^(-1) * 1: the dump writes u = 1 and v = -1, not u = 1/5.
dump [p=5: 3 + O(5^4)]
dump [p=2: 1 + O(2^3)]
dump [p=5: 1/3]
dump [p=5: 1/5]
dump [p=2: 0]
dump [p=5: 0 + O(5^4)]
# dump of a partial ball: the tag n, r or c (the archimedean tag of 5.9), the ball of the tag, the
# count of the primes, then that many local balls in the canonical order of places (5.9)
dump {}
dump {inf: 1.5}
dump {inf: 1.5; p=2: 1 + O(2^3)}
dump {inf: (1) + (2)*i; p=7: 1 + O(7^2)}
# load: the value form of conventions 9.4 of the value the text denotes
load adf1 Q lball 5 b 3 0 4
load adf1 Q lball 2 b 1 0 3
load adf1 Q lball 5 x 1 3 0
load adf1 Q lball 5 x 1 1 -1
load adf1 Q lball 2 x 0 1 0
load adf1 Q sball n 0
load adf1 Q sball r 3 -1 0 0 1 2 b 1 0 3
load adf1 Q sball c 1 0 0 0 1 1 0 0 1 7 b 1 0 2
# the two directions of one value: the dump of 10.1 of the value of the line above is the text the
# line reads, and the other way round
dump {inf: 1.5; p=2: 1 + O(2^3)}
# strictness (conventions 10.2): a local ball that breaks the predicate of 5.8 is DOMAIN
load adf1 Q lball 4 b 3 0 4
load adf1 Q lball 5 b 1c 0 2
load adf1 Q lball 5 b 5 0 4
load adf1 Q lball 5 b 3 4 4
load adf1 Q lball 5 b 3 0 -1
load adf1 Q lball 5 x 5 1 0
load adf1 Q lball 5 x 0 1 1
# the primes of a partial ball must be strictly increasing (conventions 5.9)
load adf1 Q sball r 1 0 0 0 2 5 b 3 0 4 3 x 1 1 0
load adf1 Q sball r 1 0 0 0 2 5 b 3 0 4 5 b 3 0 4
# a real ball that is not canonical (10.2: the mantissa of an arb is odd, or the pair is 0 0) and a
# non-finite one are DOMAIN
load adf1 Q sball r 0 -1 0 0 0
load adf1 Q sball r 4 0 0 0 0
# |N| above max_prec = 100000 (conventions 8.4) is a limit of stage 4: LIMIT
load adf1 Q lball 5 b 3 0 186a1
# a prime p >= 2^64 is a word restriction of stage 5: UNSUPPORTED
load adf1 Q lball 10000000000000000 b 1 0 1
# a text that is no sentence of the grammar of 10.1 is PARSE
load adf1 Q lball 5 y 3 0 4
load adf1 Q lball 5 b 3 0
load adf1 Q lball 5 b 3 0 4 4
load adf1 Q sball r 1 0 0 0 1 5 b 3 0 4 5 b 3 0 4
load adf1 Q sball m 0
# another version or another field is UNSUPPORTED (conventions 10.2, "Version and field")
load adf2 Q lball 5 b 3 0 4
load adf1 K lball 5 b 3 0 4
