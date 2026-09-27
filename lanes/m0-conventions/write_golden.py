#!/usr/bin/env python3
"""Writes tests/golden/*.tsv from the hand-written vectors below (no computation happens here).

Each vector is one line `INPUT ==> EXPECTED`; the split is at the last ` ==> `. INPUT is copied verbatim into
the TSV file, so it uses the escapes of docs/conventions.md 11.1 (`\\xHH`, `\\t`, ...; a leading or trailing
space is written `\\x20`). Lines starting with `#` are copied as comments. The expected outputs were computed
by hand from docs/conventions.md; proto/test_text_grammar.py checks them against proto/text_grammar.py.

Run from the repository root: python3 lanes/m0-conventions/write_golden.py
"""
import os

FILES = {}

FILES["rat"] = r"""
# adf_rat: valid canonical
7/3 ==> 7/3
-7/3 ==> -7/3
0 ==> 0
1 ==> 1
-1 ==> -1
1/2 ==> 1/2
# the exact scalars of SPEC 4.3
12 ==> 12
1/3 ==> 1/3
123456789012345678901234567890 ==> 123456789012345678901234567890
-123456789012345678901234567890/11 ==> -123456789012345678901234567890/11
# valid, non-canonical
4/2 ==> 2
14/6 ==> 7/3
-14/6 ==> -7/3
0/5 ==> 0
-0 ==> 0
-0/7 ==> 0
007 ==> 7
7/003 ==> 7/3
\x20\x207/3\x20\x20 ==> 7/3
\t7/3\r\n ==> 7/3
@gen:|0|100000|7 ==> 7
@gen:|0|1048575|7 ==> 7
# invalid
1/0 ==> !DOMAIN
0/0 ==> !DOMAIN
-5/0 ==> !DOMAIN
6/-2 ==> !PARSE
+7 ==> !PARSE
7 / 3 ==> !PARSE
7/ ==> !PARSE
/3 ==> !PARSE
7/3/2 ==> !PARSE
1.5 ==> !PARSE
1e3 ==> !PARSE
0x10 ==> !PARSE
 ==> !PARSE
\x20\x20\x20 ==> !PARSE
7/3 mod 6 ==> !PARSE
(* ; 7/3) ==> !PARSE
((((7/3)))) ==> !PARSE
- 7 ==> !PARSE
--7 ==> !PARSE
\xe2\x88\x927 ==> !PARSE
\xef\xbc\x97 ==> !PARSE
7\x00 ==> !PARSE
7\x003 ==> !PARSE
\x007 ==> !PARSE
7/3\x7f ==> !PARSE
7\x0b ==> !PARSE
7\x0c ==> !PARSE
@gen:|0|1048576|7 ==> !LIMIT
@gen:|(|10000| ==> !PARSE
"""

FILES["fball"] = r"""
# adf_fball: valid canonical (PLAN 5)
(* ; 2 mod 6) ==> (* ; 2 mod 6)
(* ; 7/3) ==> (* ; 7/3)
(* ; -7/3) ==> (* ; -7/3)
(* ; 0) ==> (* ; 0)
(* ; 0 mod 1) ==> (* ; 0 mod 1)
(* ; 5/3 mod 6) ==> (* ; 5/3 mod 6)
(* ; 0 mod 1/6) ==> (* ; 0 mod 1/6)
(* ; 60 mod 216) ==> (* ; 60 mod 216)
(* ; 1/2 mod 1) ==> (* ; 1/2 mod 1)
(* ; 1/4 mod 1/2) ==> (* ; 1/4 mod 1/2)
(* ; 1/3 mod 2/3) ==> (* ; 1/3 mod 2/3)
(* ; 1/6 mod 1/4) ==> (* ; 1/6 mod 1/4)
# SPEC 4.3 operands and results, bare input form
3 mod 12 ==> (* ; 3 mod 12)
5 mod 18 ==> (* ; 5 mod 18)
2 mod 6 ==> (* ; 2 mod 6)
3 mod 6 ==> (* ; 3 mod 6)
60 mod 216 ==> (* ; 60 mod 216)
5/3 mod 6 ==> (* ; 5/3 mod 6)
1/2 mod 8 ==> (* ; 1/2 mod 8)
2/3 mod 9 ==> (* ; 2/3 mod 9)
0 mod 1/6 ==> (* ; 0 mod 1/6)
# SPEC 4.2, 4.4 and 6
0 mod 2 ==> (* ; 0 mod 2)
0 mod 1 ==> (* ; 0 mod 1)
1 mod 2 ==> (* ; 1 mod 2)
1/2 mod 1 ==> (* ; 1/2 mod 1)
1/2 mod 2 ==> (* ; 1/2 mod 2)
-1 mod 2 ==> (* ; 1 mod 2)
0 mod 1/2 ==> (* ; 0 mod 1/2)
# valid, non-canonical
(* ; 20 mod 6) ==> (* ; 2 mod 6)
(* ; -1 mod 2) ==> (* ; 1 mod 2)
(* ; -7 mod 3) ==> (* ; 2 mod 3)
(* ; 14/6 mod 0) ==> (* ; 7/3)
(* ; 7/3 mod 0) ==> (* ; 7/3)
(* ; 1/2 mod 1/2) ==> (* ; 0 mod 1/2)
(* ; 3/4 mod 1/2) ==> (* ; 1/4 mod 1/2)
(* ; 5 mod 2/3) ==> (* ; 1/3 mod 2/3)
(* ; 1 mod 0/5) ==> (* ; 1)
(* ; 10/4 mod 6/4) ==> (* ; 1 mod 3/2)
(* ; -5/3 mod 6) ==> (* ; 13/3 mod 6)
(* ; 7/6 mod 1/4) ==> (* ; 1/6 mod 1/4)
(* ; 007 mod 012) ==> (* ; 7 mod 12)
(  *  ;  2  mod  6  ) ==> (* ; 2 mod 6)
(*;2mod6) ==> (* ; 2 mod 6)
\t(* ;\n2 mod 6)\r\n ==> (* ; 2 mod 6)
(* ; 123456789012345678901234567890 mod 1000000000000000000000) ==> (* ; 12345678901234567890 mod 1000000000000000000000)
@gen:(* ; |0|1048563|2 mod 6) ==> (* ; 2 mod 6)
# invalid
(* ; 1/0) ==> !DOMAIN
(* ; 1 mod 1/0) ==> !DOMAIN
(* ; 1/0 mod 0) ==> !DOMAIN
(* ; 1 mod -6) ==> !PARSE
1 mod -6 ==> !PARSE
(* ; 1 mod 6 ==> !PARSE
(* ; 1 mod 6)) ==> !PARSE
(* ; ) ==> !PARSE
(*) ==> !PARSE
(* ; 1.5 mod 6) ==> !PARSE
(* ; 1 MOD 6) ==> !PARSE
(* ; 1 modulo 6) ==> !PARSE
(* , 1 mod 6) ==> !PARSE
(1 ; 2 mod 6) ==> !PARSE
7/3 ==> !PARSE
mod 6 ==> !PARSE
2 mod ==> !PARSE
2 mod 6 mod 6 ==> !PARSE
(* ; 2 mod 6)\x00 ==> !PARSE
(* ; 2 mod 6\xc2\xa0) ==> !PARSE
((((* ; 2 mod 6)))) ==> !PARSE
@gen:(* ; |0|1048564|2 mod 6) ==> !LIMIT
"""

FILES["adele"] = r"""
# adf_adele: valid canonical (PLAN 5 example first)
(3.14159 +/- 1e-5 ; 5/3 mod 6) ==> (3.14159 +/- 1e-5 ; 5/3 mod 6)
(0 ; 0) ==> (0 ; 0)
(0.5 ; 1/2) ==> (0.5 ; 1/2)
(-2.5 +/- 0.25 ; 1 mod 2) ==> (-2.5 +/- 0.25 ; 1 mod 2)
(1 +/- 0.1 ; 0 mod 2) ==> (1 +/- 0.1 ; 0 mod 2)
(0 +/- 1 ; 0) ==> (0 +/- 1 ; 0)
(1e30 ; 0) ==> (1e30 ; 0)
(100000000000000000000 ; 0) ==> (100000000000000000000 ; 0)
(1e21 ; 0) ==> (1e21 ; 0)
(0.0001 ; 0) ==> (0.0001 ; 0)
(1e-5 ; 0) ==> (1e-5 ; 0)
(1.5e-7 +/- 1e-9 ; 0) ==> (1.5e-7 +/- 1e-9 ; 0)
(123.456 +/- 0.01 ; 0) ==> (123.456 +/- 0.01 ; 0)
(1 +/- 1e-30 ; 0) ==> (1 +/- 1e-30 ; 0)
(1e100000 ; 0) ==> (1e100000 ; 0)
(1e-100000 ; 0) ==> (1e-100000 ; 0)
# valid, non-canonical: number formatting
(3.141590 +/- 0.00001 ; 5/3 mod 6) ==> (3.14159 +/- 1e-5 ; 5/3 mod 6)
(3.14159 +/- 1E-5 ; 5/3 mod 6) ==> (3.14159 +/- 1e-5 ; 5/3 mod 6)
(3.14159 +/- 1e-05 ; 5/3 mod 6) ==> (3.14159 +/- 1e-5 ; 5/3 mod 6)
(314159e-5 +/- 1e-5 ; 5/3 mod 6) ==> (3.14159 +/- 1e-5 ; 5/3 mod 6)
(1.0 +/- 0.10 ; 0 mod 2) ==> (1 +/- 0.1 ; 0 mod 2)
(3.14159 +/- 0 ; 1) ==> (3.14159 ; 1)
(-0 ; 0) ==> (0 ; 0)
(-0.0 +/- 0 ; 0) ==> (0 ; 0)
(1e20 ; 0) ==> (100000000000000000000 ; 0)
(1E+30 ; 0) ==> (1e30 ; 0)
(0.00001 ; 0) ==> (1e-5 ; 0)
(1e00000000000000000005 ; 0) ==> (100000 ; 0)
(007.50 ; 20 mod 6) ==> (7.5 ; 2 mod 6)
( 1 +/- 0.1 ; 0 mod 2 ) ==> (1 +/- 0.1 ; 0 mod 2)
(1+/-0.1;0mod2) ==> (1 +/- 0.1 ; 0 mod 2)
# valid, non-canonical: rounding of the real ball (docs/conventions.md 9.5)
(1.23456 +/- 0.01 ; 0) ==> (1.235 +/- 0.011 ; 0)
(1.234 +/- 0.0995 ; 0) ==> (1.23 +/- 0.11 ; 0)
(0.12345 +/- 0.123 ; 0) ==> (0.12 +/- 0.13 ; 0)
(0.9999 +/- 0.001 ; 0) ==> (0.9999 +/- 0.001 ; 0)
(0.99999 +/- 0.001 ; 0) ==> (1 +/- 0.0011 ; 0)
(3.14159265358979323846264338327950288 +/- 1e-35 ; 0) ==> (3.1415926535897932385 +/- 3.8e-20 ; 0)
(0.333333333333333333333333 ; 0) ==> (0.33333333333333333333 +/- 3.4e-21 ; 0)
(12345678901234567890123 ; 0) ==> (1.234567890123456789e22 +/- 130 ; 0)
(1 +/- 0.123 ; 0) ==> (1 +/- 0.13 ; 0)
(1 +/- 0.999999 ; 0) ==> (1 +/- 1 ; 0)
# invalid
(* ; 2 mod 6) ==> !PARSE
(3.14 +/- -1 ; 0) ==> !PARSE
(3.14 +/- ; 0) ==> !PARSE
(3.14 +- 0.1 ; 0) ==> !PARSE
(3.14 \xc2\xb1 0.1 ; 0) ==> !PARSE
(.5 ; 0) ==> !PARSE
(5. ; 0) ==> !PARSE
(inf ; 0) ==> !PARSE
(nan ; 0) ==> !PARSE
([0.9, 1.1] ; 0 mod 2) ==> !PARSE
(1e100001 ; 0) ==> !LIMIT
(1e-100001 ; 0) ==> !LIMIT
(1 +/- 1e100001 ; 0) ==> !LIMIT
(1e99999999999999999999 ; 0) ==> !LIMIT
(1 ; 1/0) ==> !DOMAIN
(1 ; 1 mod 0/0) ==> !DOMAIN
(1e100001 ; 1/0) ==> !LIMIT
(1 ; 2 mod 6 ==> !PARSE
(1 ; 2 mod 6) + Q ==> !PARSE
(1 2 ; 0) ==> !PARSE
(1 +/- 0.5 +/- 0.5 ; 0) ==> !PARSE
(1e5.5 ; 0) ==> !PARSE
(1.5e ; 0) ==> !PARSE
(1 e5 ; 0) ==> !PARSE
(- 1 ; 0) ==> !PARSE
(+1 ; 0) ==> !PARSE
(1 ; 0)\x00 ==> !PARSE
(1 ; 0 ==> !PARSE
"""

FILES["cadele"] = r"""
# adf_cadele
((1) + (0)*i ; 2 mod 6) ==> ((1) + (0)*i ; 2 mod 6)
((1.5 +/- 1e-9) + (-2 +/- 1e-9)*i ; 5/3 mod 6) ==> ((1.5 +/- 1e-9) + (-2 +/- 1e-9)*i ; 5/3 mod 6)
((0) + (1)*i ; 0) ==> ((0) + (1)*i ; 0)
( ( 1 ) + ( 0 ) * i ; 2 mod 6 ) ==> ((1) + (0)*i ; 2 mod 6)
((1.50 +/- 0.001) + (2e0)*i ; 20 mod 6) ==> ((1.5 +/- 0.001) + (2)*i ; 2 mod 6)
((-0) + (-0)*i ; -0) ==> ((0) + (0)*i ; 0)
# invalid
((1) - (1)*i ; 0) ==> !PARSE
((1) + (1)i ; 0) ==> !PARSE
((1) + (1)*I ; 0) ==> !PARSE
((1) + (1)*j ; 0) ==> !PARSE
(1 + 1*i ; 0) ==> !PARSE
((1) ; 0) ==> !PARSE
((1) + (1)*i ; 1/0) ==> !DOMAIN
((1e100001) + (0)*i ; 0) ==> !LIMIT
(1 ; 0) ==> !PARSE
((1) + (0)*i ; 0 ==> !PARSE
"""

FILES["ucoset"] = r"""
# adf_ucoset: canonical (normal form)
[2 mod 3] ==> [2 mod 3]
[5 mod 36] ==> [5 mod 36]
[1 mod 1] ==> [1 mod 1]
[3 mod 4] ==> [3 mod 4]
[1] ==> [1]
[-1] ==> [-1]
# SPEC 5: 5 mod 6 and 2 mod 3 are the same coset
[5 mod 6] ==> [2 mod 3]
# valid, non-canonical
[0 mod 1] ==> [1 mod 1]
[1 mod 2] ==> [1 mod 1]
[3 mod 2] ==> [1 mod 1]
[1 mod 0] ==> [1]
[-1 mod 0] ==> [-1]
[-1 mod 6] ==> [2 mod 3]
[7 mod 10] ==> [2 mod 5]
[13 mod 12] ==> [1 mod 12]
[-5 mod 12] ==> [7 mod 12]
[5 mod 18] ==> [5 mod 9]
[11 mod 30] ==> [11 mod 15]
[ 5 mod 6 ] ==> [2 mod 3]
[5mod6] ==> [2 mod 3]
[007 mod 010] ==> [2 mod 5]
[1 mod 2000000000000000000000000000002] ==> [1 mod 1000000000000000000000000000001]
# invalid
[2 mod 4] ==> !DOMAIN
[0 mod 6] ==> !DOMAIN
[3 mod 6] ==> !DOMAIN
[2 mod 6] ==> !DOMAIN
[5 mod 0] ==> !DOMAIN
[0] ==> !DOMAIN
[2] ==> !DOMAIN
[-2] ==> !DOMAIN
[5 mod -6] ==> !PARSE
[5 mod 6/1] ==> !PARSE
[1/2 mod 3] ==> !PARSE
[5 mod 6 ==> !PARSE
5 mod 6 ==> !PARSE
[] ==> !PARSE
[p=5: 3] ==> !PARSE
[1 mod 1]\x00 ==> !PARSE
"""

FILES["idele"] = r"""
# adf_idele (PLAN 5 example first)
(2.5 +/- 1e-9 ; 3/2 * [5 mod 36]) ==> (2.5 +/- 1e-9 ; 3/2 * [5 mod 36])
(1 ; 1 * [1]) ==> (1 ; 1 * [1])
(-1 ; 1 * [-1]) ==> (-1 ; 1 * [-1])
(-0.5 +/- 0.25 ; 1/7 * [1 mod 1]) ==> (-0.5 +/- 0.25 ; 1/7 * [1 mod 1])
(-1 +/- 0.99 ; 1 * [-1]) ==> (-1 +/- 0.99 ; 1 * [-1])
# the printed real part must exclude 0 (constrained printing, 9.5)
(1 +/- 0.999999 ; 1 * [1]) ==> (1 +/- 0.999999 ; 1 * [1])
(1.04 +/- 1 ; 1 * [1]) ==> (1.04 +/- 1 ; 1 * [1])
# valid, non-canonical
(2.5 +/- 1e-9 ; 3/2 * [5 mod 6]) ==> (2.5 +/- 1e-9 ; 3/2 * [2 mod 3])
(2.5 +/- 1e-9 ; 6/4 * [41 mod 36]) ==> (2.5 +/- 1e-9 ; 3/2 * [5 mod 36])
(1 ; 1 * [1 mod 0]) ==> (1 ; 1 * [1])
(1;1*[1]) ==> (1 ; 1 * [1])
(2 +/- 0.123 ; 1 * [1]) ==> (2 +/- 0.13 ; 1 * [1])
# invalid
(0 ; 1 * [1]) ==> !DOMAIN
(1 +/- 1 ; 1 * [1]) ==> !DOMAIN
(1 +/- 2 ; 1 * [1]) ==> !DOMAIN
(1 ; 0 * [1]) ==> !DOMAIN
(1 ; 1/0 * [1]) ==> !DOMAIN
(1 ; 1 * [2 mod 4]) ==> !DOMAIN
(1 ; -1 * [1]) ==> !PARSE
(1 ; [1]) ==> !PARSE
(1 ; 1 [1]) ==> !PARSE
(* ; 1 * [1]) ==> !PARSE
(1 ; 1 * [1] ==> !PARSE
(1e100001 ; 1 * [1]) ==> !LIMIT
"""

FILES["idclass"] = r"""
# adf_idclass (PLAN 5 example first)
<1.25 +/- 1e-30 ; [5 mod 36]> ==> <1.25 +/- 1e-30 ; [5 mod 36]>
<1 ; [1]> ==> <1 ; [1]>
<1 ; [1 mod 1]> ==> <1 ; [1 mod 1]>
<1 +/- 0.999999 ; [1]> ==> <1 +/- 0.999999 ; [1]>
# valid, non-canonical
<0.5 +/- 0.25 ; [5 mod 6]> ==> <0.5 +/- 0.25 ; [2 mod 3]>
<2 +/- 0.123 ; [1]> ==> <2 +/- 0.13 ; [1]>
< 1 ; [ 1 mod 0 ] > ==> <1 ; [1]>
# invalid
<0 ; [1]> ==> !DOMAIN
<-1 ; [1]> ==> !DOMAIN
<1 +/- 1 ; [1]> ==> !DOMAIN
<1 ; [2 mod 4]> ==> !DOMAIN
<1 ; 1 * [1]> ==> !PARSE
<1 ; [5 mod 6] ==> !PARSE
<1 ; [1]>> ==> !PARSE
<1e100001 ; [1]> ==> !LIMIT
"""

FILES["lball"] = r"""
# adf_lball (PLAN 5 example first)
[p=5: 3 + O(5^4)] ==> [p=5: 3 + O(5^4)]
[p=5: 1/3] ==> [p=5: 1/3]
[p=5: -1/3] ==> [p=5: -1/3]
[p=5: 0] ==> [p=5: 0]
[p=5: 0 + O(5^4)] ==> [p=5: 0 + O(5^4)]
[p=2: 1 + O(2^3)] ==> [p=2: 1 + O(2^3)]
[p=5: 1/5 + O(5^2)] ==> [p=5: 1/5 + O(5^2)]
[p=5: 1/125 + O(5^-2)] ==> [p=5: 1/125 + O(5^-2)]
[p=3: 2/3 + O(3^0)] ==> [p=3: 2/3 + O(3^0)]
[p=5: 25/3] ==> [p=5: 25/3]
[p=18446744073709551557: 1 + O(18446744073709551557^1)] ==> [p=18446744073709551557: 1 + O(18446744073709551557^1)]
[p=5: 3 + O(5^100000)] ==> [p=5: 3 + O(5^100000)]
# valid, non-canonical
[p=5: 28 + O(5^2)] ==> [p=5: 3 + O(5^2)]
[p=5: -1 + O(5^2)] ==> [p=5: 24 + O(5^2)]
[p=5: 1/3 + O(5^4)] ==> [p=5: 417 + O(5^4)]
[p=5: 1/10 + O(5)] ==> [p=5: 13/5 + O(5^1)]
[p=5: 1/25 + O(5^-3)] ==> [p=5: 0 + O(5^-3)]
[p=5: 1/25 + O(5^-2)] ==> [p=5: 0 + O(5^-2)]
[p=5: 26/125 + O(5^-2)] ==> [p=5: 1/125 + O(5^-2)]
[p=5: 3 + O(5^0)] ==> [p=5: 0 + O(5^0)]
[p=7: 50 + O(7^2)] ==> [p=7: 1 + O(7^2)]
[p=7: 49 + O(7^2)] ==> [p=7: 0 + O(7^2)]
[p=7: 98/3 + O(7^3)] ==> [p=7: 147 + O(7^3)]
[p=2: 1/3 + O(2^3)] ==> [p=2: 3 + O(2^3)]
[p=2: 5 + O(2)] ==> [p=2: 1 + O(2^1)]
[p=5: 14/2] ==> [p=5: 7]
[ p = 5 : 3 + O( 5 ^ 4 ) ] ==> [p=5: 3 + O(5^4)]
[p=005: 3 + O(005^04)] ==> [p=5: 3 + O(5^4)]
[p=5: 3 + O(5^-0)] ==> [p=5: 0 + O(5^0)]
# invalid
[p=4: 1] ==> !DOMAIN
[p=1: 1] ==> !DOMAIN
[p=0: 1] ==> !DOMAIN
[p=5: 3 + O(7^4)] ==> !DOMAIN
[p=5: 1/0] ==> !DOMAIN
[p=5: 1/0 + O(5^2)] ==> !DOMAIN
[p=4: 1/0] ==> !DOMAIN
[p=5: 3 + O(5^100001)] ==> !LIMIT
[p=5: 3 + O(5^-100001)] ==> !LIMIT
[p=5: 3 + O(5^99999999999999999999999)] ==> !LIMIT
[p=18446744073709551616: 1] ==> !UNSUPPORTED
[p=18446744073709551617: 1] ==> !UNSUPPORTED
[p=18446744073709551617: 1 + O(5^100001)] ==> !LIMIT
[p=5: 3 + O(5^2.5)] ==> !PARSE
[p=5: 3 + O(5^+2)] ==> !PARSE
[p=5: 3 - O(5^2)] ==> !PARSE
[p=5: 3 + O(5^2) ==> !PARSE
[p=5 3] ==> !PARSE
[P=5: 3] ==> !PARSE
[p=-5: 3] ==> !PARSE
[p=5: 1.5] ==> !PARSE
[p=5: O(5^2)] ==> !PARSE
[p=5: 3 + o(5^2)] ==> !PARSE
"""

FILES["sball"] = r"""
# adf_sball
{} ==> {}
{R: 1.5 +/- 1e-9} ==> {R: 1.5 +/- 1e-9}
{C: (1) + (2)*i} ==> {C: (1) + (2)*i}
{p=5: 3 + O(5^4)} ==> {p=5: 3 + O(5^4)}
{p=5: 1/3} ==> {p=5: 1/3}
{R: 1.5 +/- 1e-9; p=2: 1 + O(2^3); p=5: 3 + O(5^4)} ==> {R: 1.5 +/- 1e-9; p=2: 1 + O(2^3); p=5: 3 + O(5^4)}
# valid, non-canonical
{p=5: 3 + O(5^4); R: 1.5 +/- 1e-9; p=2: 1 + O(2^3)} ==> {R: 1.5 +/- 1e-9; p=2: 1 + O(2^3); p=5: 3 + O(5^4)}
{ p=5 : 28 + O(5^2) ; p=3 : 1/3 } ==> {p=3: 1/3; p=5: 3 + O(5^2)}
{p=7: 50 + O(7^2); C: (0) + (-1)*i} ==> {C: (0) + (-1)*i; p=7: 1 + O(7^2)}
{ } ==> {}
# invalid
{R: 1; C: (1) + (0)*i} ==> !DOMAIN
{R: 1; R: 2} ==> !DOMAIN
{p=5: 1; p=5: 2} ==> !DOMAIN
{p=4: 1} ==> !DOMAIN
{p=5: 1;} ==> !PARSE
{;} ==> !PARSE
{p=5: 1, p=7: 1} ==> !PARSE
{R 1} ==> !PARSE
{I: 1} ==> !PARSE
[p=5: 1] ==> !PARSE
{p=18446744073709551617: 1} ==> !UNSUPPORTED
{R: 1e100001} ==> !LIMIT
"""

FILES["qclass"] = r"""
# adf_qclass: lift (any adele) and pieces (docs/conventions.md 5.10; PENDING part B)
(0.5 ; 1/3 mod 2) + Q ==> (0.5 ; 1/3 mod 2) + Q
(3.14159 +/- 1e-5 ; 5/3 mod 6) + Q ==> (3.14159 +/- 1e-5 ; 5/3 mod 6) + Q
(1 +/- 0.1 ; 0 mod 2) + Q ==> (1 +/- 0.1 ; 0 mod 2) + Q
union((0.5 ; 0 mod 1)) + Q ==> union((0.5 ; 0 mod 1)) + Q
union((0.5 ; 7)) + Q ==> union((0.5 ; 7)) + Q
union((0.5 +/- 0.5 ; 0 mod 1)) + Q ==> union((0.5 +/- 0.5 ; 0 mod 1)) + Q
union((1 +/- 0.5 ; 0 mod 1)) + Q ==> union((1 +/- 0.5 ; 0 mod 1)) + Q
# SPEC 6: real part [0.9, 1.1] with 0 mod 2 wraps into two pieces
union((0.05 +/- 0.05 ; 1 mod 2), (0.95 +/- 0.05 ; 0 mod 2)) + Q ==> union((0.05 +/- 0.05 ; 1 mod 2), (0.95 +/- 0.05 ; 0 mod 2)) + Q
# valid, non-canonical (order, duplicates, reduction)
union((0.95 +/- 0.05 ; 0 mod 2), (0.05 +/- 0.05 ; -1 mod 2)) + Q ==> union((0.05 +/- 0.05 ; 1 mod 2), (0.95 +/- 0.05 ; 0 mod 2)) + Q
union((0.5 ; 3 mod 1)) + Q ==> union((0.5 ; 0 mod 1)) + Q
union((0.5 ; 0 mod 1), (0.5 ; 0 mod 1)) + Q ==> union((0.5 ; 0 mod 1)) + Q
union((0.25 ; 1 mod 3), (0.25 ; 0 mod 3)) + Q ==> union((0.25 ; 0 mod 3), (0.25 ; 1 mod 3)) + Q
union((0.25 ; 1 mod 6), (0.25 ; 0 mod 3)) + Q ==> union((0.25 ; 0 mod 3), (0.25 ; 1 mod 6)) + Q
union((0.6 +/- 0.2 ; 0 mod 1), (0.5 +/- 0.1 ; 0 mod 1)) + Q ==> union((0.5 +/- 0.1 ; 0 mod 1), (0.6 +/- 0.2 ; 0 mod 1)) + Q
# the order is that of the printed real parts; pieces that print alike are written once (conventions 9.4)
union((0.5 +/- 0.125 ; 0 mod 1), (0.625 +/- 0.25 ; 0 mod 1)) + Q ==> union((0.62 +/- 0.26 ; 0 mod 1), (0.5 +/- 0.13 ; 0 mod 1)) + Q
union((0.5 +/- 0.0999 ; 0 mod 1), (0.5 +/- 0.1 ; 0 mod 1)) + Q ==> union((0.5 +/- 0.1 ; 0 mod 1)) + Q
(0.5;1/3mod2)+Q ==> (0.5 ; 1/3 mod 2) + Q
# invalid
union((1.5 ; 0 mod 1)) + Q ==> !DOMAIN
union((-0.25 ; 0 mod 1)) + Q ==> !DOMAIN
union((0.5 ; 1/2 mod 1)) + Q ==> !DOMAIN
union((0.5 ; 0 mod 1/2)) + Q ==> !DOMAIN
union((0.5 ; 1/2)) + Q ==> !DOMAIN
(0.5 ; 1/0) + Q ==> !DOMAIN
union() + Q ==> !PARSE
union((0.5 ; 0 mod 1)) ==> !PARSE
(0.5 ; 0) + q ==> !PARSE
(0.5 ; 0) - Q ==> !PARSE
(* ; 1 mod 2) + Q ==> !PARSE
(0.5 ; 0) ==> !PARSE
"""

FILES["ffun"] = r"""
# adf_ffun
ffun(D=1, M=1; (0) + (0)*i) ==> ffun(D=1, M=1; (0) + (0)*i)
ffun(D=1, M=2; (1) + (0)*i, (0) + (0)*i) ==> ffun(D=1, M=2; (1) + (0)*i, (0) + (0)*i)
ffun(D=2, M=3; (1) + (0)*i, (2) + (0)*i, (3) + (0)*i, (4) + (0)*i, (5) + (0)*i, (6) + (0)*i) ==> ffun(D=2, M=3; (1) + (0)*i, (2) + (0)*i, (3) + (0)*i, (4) + (0)*i, (5) + (0)*i, (6) + (0)*i)
ffun(D=1, M=1; (0.5 +/- 0.25) + (-1)*i) ==> ffun(D=1, M=1; (0.5 +/- 0.25) + (-1)*i)
# valid, non-canonical
ffun(D=01, M=1; (0.50) + (0)*i) ==> ffun(D=1, M=1; (0.5) + (0)*i)
ffun( D = 1 , M = 1 ; ( 1 ) + ( 0 ) * i ) ==> ffun(D=1, M=1; (1) + (0)*i)
# invalid
ffun(D=0, M=1; (0) + (0)*i) ==> !DOMAIN
ffun(D=1, M=0; (0) + (0)*i) ==> !DOMAIN
ffun(D=1, M=2; (0) + (0)*i) ==> !DOMAIN
ffun(D=1, M=1; (0) + (0)*i, (0) + (0)*i) ==> !DOMAIN
ffun(D=1, M=1; 0) ==> !PARSE
ffun(D=1, M=1) ==> !PARSE
ffun(D=1, M=1;) ==> !PARSE
ffun(D=1048577, M=1; (0) + (0)*i) ==> !LIMIT
ffun(D=1024, M=1025; (0) + (0)*i) ==> !LIMIT
ffun(D=99999999999999999999999, M=1; (0) + (0)*i) ==> !LIMIT
ffun(M=1, D=1; (0) + (0)*i) ==> !PARSE
FFUN(D=1, M=1; (0) + (0)*i) ==> !PARSE
"""

FILES["rfun"] = r"""
# adf_rfun
rfun() ==> rfun()
rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) ==> rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))
rfun(term(P=[(0) + (0)*i, (1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) ==> rfun(term(P=[(0) + (0)*i, (1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))
rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i), term(P=[(2) + (0)*i], A=(0.5 +/- 0.25) + (3)*i, B=(1) + (-1)*i, C=(0) + (0)*i)) ==> rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i), term(P=[(2) + (0)*i], A=(0.5 +/- 0.25) + (3)*i, B=(1) + (-1)*i, C=(0) + (0)*i))
# valid, non-canonical
rfun( term( P = [ (1) + (0)*i ] , A=(1)+(0)*i , B=(0)+(0)*i , C=(0)+(0)*i ) ) ==> rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))
rfun(term(P=[(1) + (0)*i], A=(1 +/- 0.999999) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) ==> rfun(term(P=[(1) + (0)*i], A=(1 +/- 0.999999) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i))
# invalid
rfun(term(P=[(1) + (0)*i], A=(0) + (1)*i, B=(0) + (0)*i, C=(0) + (0)*i)) ==> !DOMAIN
rfun(term(P=[(1) + (0)*i], A=(-1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) ==> !DOMAIN
rfun(term(P=[(1) + (0)*i], A=(0.5 +/- 0.5) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) ==> !DOMAIN
rfun(term(P=[], A=(1) + (0)*i, B=(0) + (0)*i, C=(0) + (0)*i)) ==> !PARSE
rfun(term(A=(1) + (0)*i, P=[(1) + (0)*i], B=(0) + (0)*i, C=(0) + (0)*i)) ==> !PARSE
rfun(term(P=[(1) + (0)*i], A=(1) + (0)*i, B=(0) + (0)*i)) ==> !PARSE
rfun(,) ==> !PARSE
rfun ==> !PARSE
"""

FILES["char"] = r"""
# adf_char (Conrey labels as in FLINT's dirichlet module; stored primitive)
char(q=1, n=1, s=(0) + (0)*i) ==> char(q=1, n=1, s=(0) + (0)*i)
char(q=5, n=2, s=(0) + (0)*i) ==> char(q=5, n=2, s=(0) + (0)*i)
char(q=4, n=3, s=(0) + (14.134725 +/- 1e-6)*i) ==> char(q=4, n=3, s=(0) + (14.134725 +/- 1e-6)*i)
char(q=12, n=11, s=(1) + (0)*i) ==> char(q=12, n=11, s=(1) + (0)*i)
# valid, non-canonical: reduction of n, lowering to the primitive character
char(q=1, n=0, s=(0) + (0)*i) ==> char(q=1, n=1, s=(0) + (0)*i)
char(q=5, n=7, s=(0) + (0)*i) ==> char(q=5, n=2, s=(0) + (0)*i)
char(q=10, n=3, s=(0) + (0)*i) ==> char(q=5, n=3, s=(0) + (0)*i)
char(q=3, n=1, s=(1) + (0)*i) ==> char(q=1, n=1, s=(1) + (0)*i)
char(q=16, n=9, s=(0) + (0)*i) ==> char(q=8, n=5, s=(0) + (0)*i)
char(q=6, n=5, s=(0.5) + (0)*i) ==> char(q=3, n=2, s=(0.5) + (0)*i)
char( q = 5 , n = 2 , s = (0) + (0)*i ) ==> char(q=5, n=2, s=(0) + (0)*i)
# invalid
char(q=0, n=1, s=(0) + (0)*i) ==> !DOMAIN
char(q=6, n=2, s=(0) + (0)*i) ==> !DOMAIN
char(q=6, n=0, s=(0) + (0)*i) ==> !DOMAIN
char(q=18446744073709551616, n=1, s=(0) + (0)*i) ==> !UNSUPPORTED
char(q=5, n=2) ==> !PARSE
char(q=5, n=2, s=0) ==> !PARSE
char(n=2, q=5, s=(0) + (0)*i) ==> !PARSE
char(q=5, n=-2, s=(0) + (0)*i) ==> !PARSE
"""

FILES["dispatch"] = r"""
# adf_text_classify: syntax only (docs/conventions.md 9.7)
7/3 ==> rat
-7 ==> rat
(* ; 2 mod 6) ==> fball
2 mod 6 ==> fball
(3.14159 +/- 1e-5 ; 5/3 mod 6) ==> adele
((1) + (0)*i ; 2 mod 6) ==> cadele
[5 mod 6] ==> ucoset
[1] ==> ucoset
(2.5 +/- 1e-9 ; 3/2 * [5 mod 36]) ==> idele
<1.25 +/- 1e-30 ; [5 mod 36]> ==> idclass
[p=5: 3 + O(5^4)] ==> lball
{} ==> sball
{R: 1} ==> sball
(0.5 ; 0) + Q ==> qclass
union((0.5 ; 0 mod 1)) + Q ==> qclass
ffun(D=1, M=1; (0) + (0)*i) ==> ffun
rfun() ==> rfun
char(q=1, n=1, s=(0) + (0)*i) ==> char
# syntax is enough: semantic errors are found by the typed parser
(1 ; 1/0) ==> adele
[2 mod 4] ==> ucoset
[p=4: 1] ==> lball
(1e100001 ; 0) ==> adele
(0 ; 1 * [1]) ==> idele
<0 ; [1]> ==> idclass
# no type
 ==> !PARSE
hello ==> !PARSE
(* ; 2 mod 6 ==> !PARSE
7\x00 ==> !PARSE
@gen:|0|1048577| ==> !LIMIT
"""

FILES["realball_read"] = r"""
# a real ball in the value form -> the exact interval "lo hi"
3.14159 +/- 1e-5 ==> 157079/50000 3927/1250
0.5 ==> 1/2 1/2
-0.75 ==> -3/4 -3/4
0 ==> 0 0
-0 ==> 0 0
1 +/- 0 ==> 1 1
1 +/- 0.1 ==> 9/10 11/10
1e-5 ==> 1/100000 1/100000
1E+2 ==> 100 100
2.5e-1 +/- 25e-2 ==> 0 1/2
0.1 +/- 0.2 ==> -1/10 3/10
007.50 ==> 15/2 15/2
1.5 +/- 1e-9 ==> 1499999999/1000000000 1500000001/1000000000
1+/-1 ==> 0 2
\x201 +/- 1\x20 ==> 0 2
1e100001 ==> !LIMIT
1 +/- -1 ==> !PARSE
+/- 1 ==> !PARSE
1 +/- ==> !PARSE
.5 ==> !PARSE
5. ==> !PARSE
1.5.2 ==> !PARSE
inf ==> !PARSE
nan ==> !PARSE
1e ==> !PARSE
1e+ ==> !PARSE
--1 ==> !PARSE
1 +/- 1 +/- 1 ==> !PARSE
"""

FILES["realball_print"] = r"""
# "mid rad digits" (dyadic rationals) -> text of docs/conventions.md 9.5 (unconstrained)
0 0 20 ==> 0
1 0 20 ==> 1
-1 0 20 ==> -1
1/2 0 20 ==> 0.5
-3/4 0 20 ==> -0.75
5/2 1/1024 20 ==> 2.5 +/- 0.00098
6004799503160661/18014398509481984 0 20 ==> 0.33333333333333331483 +/- 3.9e-22
1 1 20 ==> 1 +/- 1
0 1/2 20 ==> 0 +/- 0.5
0 3 20 ==> 0 +/- 3
1024 1/1024 3 ==> 1020 +/- 4.1
1023/1024 0 3 ==> 0.999 +/- 2.4e-5
2047/2048 0 3 ==> 1 +/- 0.00049
1180591620717411303424 0 20 ==> 1.1805916207174113034e21 +/- 24
1/1180591620717411303424 0 20 ==> 8.4703294725430033907e-22 +/- 1.7e-42
3/2 1/2 20 ==> 1.5 +/- 0.5
-5/2 1/4 20 ==> -2.5 +/- 0.25
100 1/8 2 ==> 100 +/- 0.13
12345 0 2 ==> 12000 +/- 350
12500 0 2 ==> 12000 +/- 500
13500 0 2 ==> 14000 +/- 500
1 1023/1024 20 ==> 1 +/- 1
10715086071862673209484250490600018105614048117055336074437503883703510511249361224931983788156958581275946729175531468251871452856923140435984577574698574803934567774824230985421074605062371141877954182153046474983581941267398767559165543946077062914571196477686542167660429831652624386837205668069376 0 5 ==> 1.0715e301 +/- 8.7e295
"""

FILES["dump"] = r"""
# dump form, version 1 (docs/conventions.md 10); a valid dump is its own expected output
adf1 rat 7 3 ==> adf1 rat 7 3
adf1 rat -7 3 ==> adf1 rat -7 3
adf1 rat 0 1 ==> adf1 rat 0 1
adf1 rat 1f 1 ==> adf1 rat 1f 1
adf1 rat 2 4 ==> !DOMAIN
adf1 rat 7 0 ==> !DOMAIN
adf1 rat 7 -3 ==> !DOMAIN
adf1 rat 07 3 ==> !PARSE
adf1 rat 7 3\x20 ==> !PARSE
adf1  rat 7 3 ==> !PARSE
\x20adf1 rat 7 3 ==> !PARSE
adf1 rat 7 3\n ==> !PARSE
adf1 rat 7 ==> !PARSE
adf1 rat 7 3 1 ==> !PARSE
adf1 rat A 3 ==> !PARSE
adf1 rat -0 1 ==> !PARSE
adf1 rat 0x7 3 ==> !PARSE
adf1\trat 7 3 ==> !PARSE
adf2 rat 7 3 ==> !UNSUPPORTED
adf0 rat 7 3 ==> !UNSUPPORTED
adf2 anything at all ==> !UNSUPPORTED
adf01 rat 7 3 ==> !PARSE
ADF1 rat 7 3 ==> !PARSE
adf1 real 7 3 ==> !PARSE
@gen:adf1 rat 1|0|1048576| 1 ==> !LIMIT
# finite balls, global and local
adf1 fball g 2 6 1 ==> adf1 fball g 2 6 1
adf1 fball g 5 12 3 ==> adf1 fball g 5 12 3
adf1 fball g 0 0 1 ==> adf1 fball g 0 0 1
adf1 fball g 7 0 3 ==> adf1 fball g 7 0 3
adf1 fball g 8 6 1 ==> !DOMAIN
adf1 fball g 2 6 2 ==> !DOMAIN
adf1 fball g 0 0 2 ==> !DOMAIN
adf1 fball g -1 6 1 ==> !DOMAIN
adf1 fball g 1 -6 1 ==> !DOMAIN
adf1 fball g 1 6 0 ==> !DOMAIN
adf1 fball x 1 6 1 ==> !PARSE
adf1 fball l 1 6 2 2 3 0 2 ==> adf1 fball l 1 6 2 2 3 0 2
adf1 fball l 1 6 2 3 2 2 0 ==> adf1 fball l 1 6 2 3 2 2 0
adf1 fball l 1 6 2 2 3 2 0 ==> !DOMAIN
adf1 fball l 1 6 2 2 4 0 2 ==> !DOMAIN
adf1 fball l 1 8 2 2 4 0 2 ==> !DOMAIN
adf1 fball l 2 6 2 2 3 0 0 ==> !DOMAIN
adf1 fball l 1 1 0 ==> !DOMAIN
adf1 fball l 1 6 2 2 3 0 ==> !PARSE
adf1 fball l 1 6 2 2 3 0 2 1 ==> !PARSE
# contexts
adf1 modctx 6 2 2 3 ==> adf1 modctx 6 2 2 3
adf1 modctx 6 2 3 2 ==> adf1 modctx 6 2 3 2
adf1 modctx 1 0 ==> adf1 modctx 1 0
adf1 modctx 6 1 6 ==> adf1 modctx 6 1 6
adf1 modctx 10000000000000000 0 ==> adf1 modctx 10000000000000000 0
adf1 modctx ffffffffffffffff 1 ffffffffffffffff ==> adf1 modctx ffffffffffffffff 1 ffffffffffffffff
adf1 modctx 0 0 ==> !DOMAIN
adf1 modctx 6 2 2 2 ==> !DOMAIN
adf1 modctx 7 2 2 3 ==> !DOMAIN
adf1 modctx 1 1 1 ==> !DOMAIN
adf1 modctx 10000000000000000 1 10000000000000000 ==> !DOMAIN
adf1 modctx 6 2 2 3 5 ==> !PARSE
adf1 modctx 6 2 2 ==> !PARSE
# adeles: real balls as arb_dump_str writes them
adf1 adele 1 -1 0 0 g 0 0 1 ==> adf1 adele 1 -1 0 0 g 0 0 1
adf1 adele c90fcf80dc337 -32 a7c5ac5 -2c g 5 12 3 ==> adf1 adele c90fcf80dc337 -32 a7c5ac5 -2c g 5 12 3
adf1 adele 0 0 0 0 g 0 0 1 ==> adf1 adele 0 0 0 0 g 0 0 1
adf1 adele -3 -2 0 0 g 1 2 1 ==> adf1 adele -3 -2 0 0 g 1 2 1
adf1 adele 1 0 3fffffff 0 g 0 0 1 ==> adf1 adele 1 0 3fffffff 0 g 0 0 1
adf1 adele 1 7fffffffffffffffffff 1 7fffffffffffffffffff g 0 0 1 ==> adf1 adele 1 7fffffffffffffffffff 1 7fffffffffffffffffff g 0 0 1
adf1 adele 1 0 0 0 l 1 6 2 2 3 0 2 ==> adf1 adele 1 0 0 0 l 1 6 2 2 3 0 2
adf1 adele 2 0 0 0 g 0 0 1 ==> !DOMAIN
adf1 adele 0 -1 0 0 g 0 0 1 ==> !DOMAIN
adf1 adele 0 -3 0 -1 g 0 0 1 ==> !DOMAIN
adf1 adele 0 0 0 -1 g 0 0 1 ==> !DOMAIN
adf1 adele 0 5 0 0 g 0 0 1 ==> !DOMAIN
adf1 adele 1 0 -1 0 g 0 0 1 ==> !DOMAIN
adf1 adele 1 0 40000000 0 g 0 0 1 ==> !DOMAIN
adf1 adele 1 0 40000001 0 g 0 0 1 ==> !DOMAIN
adf1 adele 1 0 0 1 g 0 0 1 ==> !DOMAIN
adf1 adele 1 0 0 0 5 g 0 0 1 ==> !PARSE
adf1 adele 1 0 0 g 0 0 1 ==> !PARSE
adf1 adele A 0 0 0 g 0 0 1 ==> !PARSE
adf1 adele 1 0 0 0 g 0 0 2 ==> !DOMAIN
adf1 cadele 1 0 0 0 1 -1 0 0 g 2 6 1 ==> adf1 cadele 1 0 0 0 1 -1 0 0 g 2 6 1
adf1 cadele 1 0 0 0 0 -1 0 0 g 2 6 1 ==> !DOMAIN
# unit cosets, ideles, classes
adf1 ucoset 5 6 ==> adf1 ucoset 5 6
adf1 ucoset 1 0 ==> adf1 ucoset 1 0
adf1 ucoset -1 0 ==> adf1 ucoset -1 0
adf1 ucoset 1 1 ==> adf1 ucoset 1 1
adf1 ucoset 0 1 ==> !DOMAIN
adf1 ucoset b 6 ==> !DOMAIN
adf1 ucoset 2 4 ==> !DOMAIN
adf1 ucoset 2 0 ==> !DOMAIN
adf1 ucoset -1 6 ==> !DOMAIN
adf1 ucoset 1 -1 ==> !DOMAIN
adf1 idele 5 -1 1 -1e 3 2 5 24 ==> adf1 idele 5 -1 1 -1e 3 2 5 24
adf1 idele 1 0 1 -1 1 1 1 0 ==> adf1 idele 1 0 1 -1 1 1 1 0
adf1 idele 0 0 0 0 1 1 1 0 ==> !DOMAIN
adf1 idele 1 0 1 0 1 1 1 0 ==> !DOMAIN
adf1 idele 1 0 0 0 0 1 1 0 ==> !DOMAIN
adf1 idele 1 0 0 0 -1 1 1 0 ==> !DOMAIN
adf1 idele 1 0 0 0 2 2 1 0 ==> !DOMAIN
adf1 idclass 5 -2 0 0 5 24 ==> adf1 idclass 5 -2 0 0 5 24
adf1 idclass -1 0 0 0 1 0 ==> !DOMAIN
adf1 idclass 1 0 1 0 1 0 ==> !DOMAIN
# local balls
adf1 lball 5 b 3 0 4 ==> adf1 lball 5 b 3 0 4
adf1 lball 5 x 1 3 0 ==> adf1 lball 5 x 1 3 0
adf1 lball 5 x 1 3 1 ==> adf1 lball 5 x 1 3 1
adf1 lball 5 x 0 1 0 ==> adf1 lball 5 x 0 1 0
adf1 lball 5 b 0 0 4 ==> adf1 lball 5 b 0 0 4
adf1 lball 5 b 1 -1 2 ==> adf1 lball 5 b 1 -1 2
adf1 lball 5 b 1c 0 2 ==> !DOMAIN
adf1 lball 5 b 5 0 4 ==> !DOMAIN
adf1 lball 5 b 3 4 4 ==> !DOMAIN
adf1 lball 5 b 3 0 -1 ==> !DOMAIN
adf1 lball 4 b 3 0 4 ==> !DOMAIN
adf1 lball 5 x 5 1 0 ==> !DOMAIN
adf1 lball 5 x 0 1 1 ==> !DOMAIN
adf1 lball 5 b 3 0 186a1 ==> !LIMIT
adf1 lball 10000000000000000 b 1 0 1 ==> !UNSUPPORTED
adf1 lball 5 y 3 0 4 ==> !PARSE
# partial balls
adf1 sball n 0 ==> adf1 sball n 0
adf1 sball r 3 -1 0 0 2 3 x 1 1 0 5 b 3 0 4 ==> adf1 sball r 3 -1 0 0 2 3 x 1 1 0 5 b 3 0 4
adf1 sball c 1 0 0 0 0 0 0 0 0 ==> adf1 sball c 1 0 0 0 0 0 0 0 0
adf1 sball r 3 -1 0 0 2 5 b 3 0 4 3 x 1 1 0 ==> !DOMAIN
adf1 sball r 3 -1 0 0 2 5 b 3 0 4 5 b 3 0 4 ==> !DOMAIN
adf1 sball r 1 0 0 0 1 5 b 3 0 4 5 b 3 0 4 ==> !PARSE
# quotient by Q
adf1 qclass lift 1 -1 0 0 g 1 3 3 ==> adf1 qclass lift 1 -1 0 0 g 1 3 3
adf1 qclass pieces 2 1 -4 1 -4 g 1 2 1 f -4 1 -4 g 0 2 1 ==> adf1 qclass pieces 2 1 -4 1 -4 g 1 2 1 f -4 1 -4 g 0 2 1
adf1 qclass pieces 2 f -4 1 -4 g 0 2 1 1 -4 1 -4 g 1 2 1 ==> !DOMAIN
adf1 qclass pieces 1 3 -1 0 0 g 0 1 1 ==> !DOMAIN
adf1 qclass pieces 1 1 -1 0 0 g 1 2 2 ==> !DOMAIN
adf1 qclass pieces 0 ==> !DOMAIN
# test functions and characters
adf1 ffun 1 1 0 0 0 0 0 0 0 0 ==> adf1 ffun 1 1 0 0 0 0 0 0 0 0
adf1 ffun 1 2 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ==> adf1 ffun 1 2 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
adf1 ffun 1 2 1 0 0 0 0 0 0 0 ==> !PARSE
adf1 ffun 0 1 ==> !DOMAIN
adf1 rfun 0 ==> adf1 rfun 0
adf1 rfun 1 1 1 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ==> adf1 rfun 1 1 1 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
adf1 rfun 1 1 1 0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ==> !DOMAIN
adf1 rfun 1 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ==> !DOMAIN
adf1 char 1 1 0 0 0 0 0 0 0 0 ==> adf1 char 1 1 0 0 0 0 0 0 0 0
adf1 char 5 2 0 0 0 0 0 0 0 0 ==> adf1 char 5 2 0 0 0 0 0 0 0 0
adf1 char 4 3 0 0 0 0 0 0 0 0 ==> adf1 char 4 3 0 0 0 0 0 0 0 0
adf1 char a 3 0 0 0 0 0 0 0 0 ==> !DOMAIN
adf1 char 6 5 0 0 0 0 0 0 0 0 ==> !DOMAIN
adf1 char 5 0 0 0 0 0 0 0 0 0 ==> !DOMAIN
adf1 char 5 5 0 0 0 0 0 0 0 0 ==> !DOMAIN
adf1 char 10000000000000000 1 0 0 0 0 0 0 0 0 ==> !UNSUPPORTED
# scaled values
adf1 scaled x 0 1 6 2 2 3 ==> adf1 scaled x 0 1 6 2 2 3
adf1 scaled s 1 2 5 6 2 2 3 ==> adf1 scaled s 1 2 5 6 2 2 3
adf1 scaled x 1 2 1 0 ==> adf1 scaled x 1 2 1 0
adf1 scaled s 1 2 6 6 2 2 3 ==> !DOMAIN
adf1 scaled s 0 1 0 6 1 6 ==> !DOMAIN
adf1 scaled s -1 2 1 6 1 6 ==> !DOMAIN
adf1 scaled x 1 2 5 6 1 0 ==> !PARSE
"""


def main():
    out_dir = os.path.join("tests", "golden")
    os.makedirs(out_dir, exist_ok=True)
    total = 0
    for name, block in FILES.items():
        lines = []
        for raw in block.strip("\n").split("\n"):
            if raw.startswith("#") or raw == "":
                lines.append(raw)
                continue
            i = raw.rindex(" ==> ")
            inp, exp = raw[:i], raw[i + 5:]
            if "\t" in inp or "\t" in exp:
                raise SystemExit("literal TAB in vector: " + raw)
            lines.append(inp + "\t" + exp)
            total += 1
        with open(os.path.join(out_dir, name + ".tsv"), "w", encoding="ascii", newline="\n") as f:
            f.write("\n".join(lines) + "\n")
    print("wrote", len(FILES), "files,", total, "vectors")


if __name__ == "__main__":
    main()
