show {inf: 1 +/- 0.5; p=3: 1 + O(3^2); p=5: 7}
show {inf: (1)+(2)*i; p=3: 1 + O(3^2)}
project {inf: 1 +/- 0.5; p=3: 1 + O(3^2); p=5: 7} with real
project {inf: 1 +/- 0.5; p=3: 1 + O(3^2); p=5: 7} with real real
project {inf: 1 +/- 0.5; p=3: 1 + O(3^2); p=5: 7} with 3 3
project {inf: 1 +/- 0.5; p=3: 1 + O(3^2); p=5: 7} with 7
project {inf: 1 +/- 0.5; p=3: 1 + O(3^2); p=5: 7} with 5 3 real
project {inf: 1 +/- 0.5; p=3: 1 + O(3^2); p=5: 7} with
project {p=3: 1 + O(3^2); p=5: 7} with real
project {} with real
project {} with 3
project {} with
project {inf: (1)+(2)*i; p=3: 1 + O(3^2)} with real
project {inf: (1)+(2)*i; p=3: 1 + O(3^2)} with 3
project {inf: (1)+(2)*i; p=3: 1 + O(3^2)} with 3 real
exp_at {inf: 1 +/- 0.5; p=3: 3 + O(3^3)} with real
exp_at {inf: 1 +/- 0.5; p=3: 3 + O(3^3)} with 3
exp_at {inf: (1)+(2)*i; p=3: 3 + O(3^3)} with 3
exp_at {inf: (1)+(2)*i; p=3: 3 + O(3^3)} with real
exp_at {} with real
log_at {inf: 1 +/- 0.5; p=3: 1 + 3 + O(3^3)} with 3
log_at {inf: 1 +/- 0.5; p=3: 1 + 3 + O(3^3)} with real
sin_at {inf: 1 +/- 0.5; p=3: 3 + O(3^3)} with 3
powunit_at {p=3: 1 + 3 + O(3^3)} with 3 with [p=3: 2]
powunit_at {p=3: 1 + 3 + O(3^3)} with 3 with {p=3: 2}
powunit_at {p=3: 1 + 3 + O(3^3)} with 3 with {p=5: 2}
powunit_at {p=3: 1 + 3 + O(3^3)} with 3 with {}
powrat_at {p=3: 4 + O(3^5)} with 3 with 1/2 with 1
root_at {p=3: 4 + O(3^5)} with 3
prec 2
show {inf: 1.234567890123 +/- 0.001; p=3: 1 + O(3^2)}
digits 3
show {inf: 1.234567890123 +/- 0.001; p=3: 1 + O(3^2)}
prec 100000
show {inf: 1.234567890123 +/- 0.001; p=3: 1 + O(3^2)}
exp_at {inf: 1.234567890123 +/- 0.001; p=3: 3 + O(3^3)} with real
digits 1000000
show {inf: 1.234567890123 +/- 0.001; p=3: 1 + O(3^2)}
prec 3
exp_at {inf: 1.234567890123 +/- 0.001; p=3: 3 + O(3^3)} with real
prec 3
project {inf: 1.234567890123 +/- 0.001; p=3: 3 + O(3^3)} with real 3
