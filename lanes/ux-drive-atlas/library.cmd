prec 8
project 1/5 with 5
digits_at [p=5: 1/5] with 5
period [p=5: 1/5]
exp_at 1 with 2
show {inf: 3; p=2: 3; p=3: -3; p=5: -3}
mul {inf: 3; p=2: 3; p=3: -3; p=5: -3} with {inf: 3; p=2: 3; p=3: -3; p=5: -3}
mul [p=5: 1 + O(5^2)] with [p=5: 1 + O(5^2)]
map [p=5: 1 + O(5^2)] with square
support (* ; 0 mod 3027)
project (* ; 0 mod 3027) with 2 3 5
project 1/5 with real
dump (0.2 ; 1/5)
dump {inf: 0.2}
cost roots_at 9 with 5 with 2
limits roots_at
