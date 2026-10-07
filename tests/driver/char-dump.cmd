#!exit 1
# Conventions 10.1:1414 and 10.2. Hexadecimal q,n and exact dyadic s, by hand.
dump char(q=5, n=2, s=(0) + (0)*i)
dump char(q=8, n=7, s=(-0.75) + (0.5)*i)
load adf1 Q char 5 2 0 0 0 0 0 0 0 0
load adf1 Q char 4 3 -3 -2 0 0 1 -1 0 0
load adf1 Q char 8 1 0 0 0 0 0 0 0 0
load adf1 Q char 10 9 0 0 0 0 0 0 0 0
load adf1 Q char 0 1 0 0 0 0 0 0 0 0
load adf1 Q char 10001 3 0 0 0 0 0 0 0 0
load adf1 Q char ffffffffffffffff 1 0 0 0 0 0 0 0 0
load adf1 Q char 5 z 0 0 0 0 0 0 0 0
load adf1 Q char 5 2 0 0 0 0 0 0 0
mul char(q=5, n=2, s=(0) + (0)*i) with char(q=5, n=2, s=(0) + (0)*i)
