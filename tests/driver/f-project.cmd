# f-project: the command "project X with PLACES", the partial ball of X over the places (adf_sball_project,
# include/adelefeld/sball.h; SPEC 9.3.1: "project(x, S) then f"; docs/proofs/functions.md Proposition 22).  The
# components are printed in the canonical order (conventions 7: the real place first, then the primes increasing)
# separated by "; ": "real: <ball>", and "<p>: <centre>" for an exact local ball, "<p>: <centre> + O(<p>^<N>)" for a ball.
#
# The expected lines are written from the definitions, and none from the output of the program:
#  - the component of a finite ball (A + H Zhat)/d at a prime p is the ball A/d + p^N Z_p with N = v_p(H) - v_p(d)
#    (api-1f.md L1); a rational is the exact rational at p;
#  - a rational is converted to the real coordinate at the setting prec (SPEC 4.1), and a real ball prints as
#    conventions 9.5 (with digits 6): 2/3 at 64 bits is 0.666667 +/- 3.4e-7 (lanes/f-slice6/expected.py applies
#    proto/text_grammar.py print_real to the value 2/3; the text does not depend on the tiny radius of the ball), and
#    an exact real 0.5 or 0 prints alone.
#!exit 1
digits 6
project 2/3 with 2 5 real
project 1/3 with real
project 0 with 2 real
project -7/4 with 7 2
project 5 with 5 3 2
project 1/5 with 5
# the places may be given in any order and are sorted; the real place is first
project 5 with real 7 2
# 7 + 18 Zhat: at 2, v_2(18) = 1 gives 7 mod 2 = 1: 1 + 2^1 Z_2; at 3, v_3(18) = 2: 7 + 3^2 Z_3; at 5, v_5(18) = 0: the
# whole of Z_5, the ball around 0 of exponent 0.  The real coordinate is the exact 0.5.
project (0.5 ; 7 mod 18) with 2 3 5 real
# 1/5 + 25 Zhat: at 5 the ball 1/5 + 5^2 Z_5 (v_5(25) - v_5(5) = 2, the centre 1/5 = 5^(-1) * 1); at 3 v_3(25) = 0
project (* ; 1/5 mod 25) with 3 5
# a prime of 64 bits, 2^64 - 59
project 1/3 with 18446744073709551557
# a finite ball has no real coordinate: DOMAIN; a complex adele and a kind of the value form that the driver has no
# typed parser for: UNSUPPORTED
project (* ; 5 mod 25) with real
project ((1) + (2)*i ; 5 mod 18) with 2
project [p=5: 3 + O(5^4)] with 5
