# show and type of the unit coset, the idele and the idele class (lane t-slice1).  Expected lines written by hand
# from conventions 5.6 (residue 1..N, normal form N = 2 mod 4 -> N/2), 9.3 (canonicalisation on input) and 9.4
# (templates), with real parts that are dyadic, so that the text of 9.5 is exact (the radius 0.125 is printed
# as 0.13: two significant digits, rounded up); checked by lanes/t-slice1/check_driver_cases.py.
#!exit 1
# the unit coset
show [5 mod 6]
show [2 mod 3]
show [1 mod 0]
show [-1 mod 0]
show [-1]
show [0 mod 1]
show [-7 mod 10]
show [ 007 mod 010 ]
show [1 mod 2000000000000000000000000000002]
# the idele: PLAN 5 with dyadic reals; the unit in normal form, the content reduced
show (2.5 +/- 0.125 ; 3/2 * [5 mod 6])
show (2 +/- 0.125 ; 6/4 * [41 mod 36])
show (-0.5 +/- 0.25 ; 1/7 * [1 mod 1])
show (1 ; 1 * [1 mod 0])
show (-1 ; 1 * [-1])
show ( 8 ;12/8* [ 3 mod 4 ] )
# the idele class
show <1.25 +/- 0.25 ; [5 mod 36]>
show <0.5 +/- 0.25 ; [5 mod 6]>
show <1 ; [1 mod 0]>
# the kinds
type [5 mod 6]
type (1 ; 1 * [1])
type <1 ; [1]>
type (1 ; [1])
