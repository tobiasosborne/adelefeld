# show: parse and print canonically; type: the kind of adf_text_classify (conventions 9.7).
# Expected lines written from conventions 9.3 (canonicalisation on input) and 9.4 (the
# printing templates); every real ball below is dyadic, so the text of 9.5 is exact.
#!exit 1
# 9.3: leading zeros removed, fractions reduced, -0 becomes 0
show 7/3
show 4/2
show -0
show 007
show -0/7
# 9.3: a bare "a mod N" becomes "(* ; a mod N)" (CV-32); the centre is reduced into [0, N)
show 3 mod 12
show (* ; 3 mod 12)
show (* ; 15 mod 12)
show (* ; -1 mod 2)
# 9.3: "mod 0" is dropped
show (* ; 14/6 mod 0)
# 9.4: the templates of adf_rat, adf_fball, adf_adele, adf_cadele
show (0.5 ; 1/2)
show (0 ; 0)
show (1 ; 2 mod 6)
show (-2.5 ; 1 mod 2)
show (0 +/- 0.5 ; 0)
show ((1) + (0)*i ; 2 mod 6)
# 9.2: whitespace between tokens, none inside a token
show ( 1 ; 2 mod 6 )
show (1;2mod6)
# the semantic constraints of 9.3 are found by the typed parser
show 1/0
show (1 ; 1/0)
show (* ; 1/0 mod 1)
# 9.7: the thirteen start symbols, one constant each
type 7/3
type -7
type (* ; 2 mod 6)
type 2 mod 6
type (3.14159 +/- 1e-5 ; 5/3 mod 6)
type ((1) + (0)*i ; 2 mod 6)
type [5 mod 6]
type [1]
type (2.5 +/- 1e-9 ; 3/2 * [5 mod 36])
type <1.25 +/- 1e-30 ; [5 mod 36]>
type [p=5: 3 + O(5^4)]
type {}
type {inf: 1}
type {inf: 1; p=5: 2 + O(5^2)}
type (0.5 ; 0) + Q
type union((0.5 ; 0 mod 1)) + Q
type ffun(D=1, M=1; (0) + (0)*i)
type rfun()
type char(q=1, n=1, s=(0) + (0)*i)
# 9.7: the classifier is syntax only, so it reports the kind of a text with a
# semantic defect
type 1/0
type (1 ; 1/0)
type [2 mod 4]
type (1e100001 ; 0)
# no start symbol derives the text
type hello
type ((* ; 2 mod 6))
