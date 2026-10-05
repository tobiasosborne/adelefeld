show {r 3 +/- 2; 5: 3 + O(5^2)}
show {inf: 1 +/- 0.5; 3: 1 + O(3^2)}
dump {inf: 1 +/- 0.5; 3: 1 + O(3^2)}
dump {3: 1 + O(3^2)}
dump {}
show {}
dump [3: 1 + O(3^2)]
dump [p=3: 1 + O(3^2)]
dump {inf: 1 +/- 0.5}
load adf1 Q sball n 0
load adf1 Q sball r 1 0 0 0 1 3 b 1 0 2
load adf1 Q sball c 1 0 0 0 1 0 0 0 0 0 0 0 1 3 b 1 0 2
type {}
project {} with real
project {} with 3
project {inf: 1 +/- 0.5; 3: 1 + O(3^2)} with real
project {inf: 1 +/- 0.5; 3: 1 + O(3^2)} with real real
project {inf: 1 +/- 0.5; 3: 1 + O(3^2)} with 3 3
project {inf: 1 +/- 0.5; 3: 1 + O(3^2)} with 5
project {inf: 1 +/- 0.5; 3: 1 + O(3^2)} with 3 real
project {3: 1 + O(3^2)} with real
exp_at {inf: 1 +/- 0.5; 3: 3 + O(3^3)} with real
exp_at {inf: 1 +/- 0.5; 3: 3 + O(3^3)} with 3
log_at {inf: 1 +/- 0.5; 3: 1 + 3 + O(3^3)} with 3
log_at {inf: 1 +/- 0.5; 3: 1 + 3 + O(3^3)} with real
sin_at {inf: 1 +/- 0.5; 3: 3 + O(3^3)} with 3
powunit_at {3: 1 + 3 + O(3^3)} with 3 with [3: 2]
powunit_at {inf: 1; 3: 1 + 3 + O(3^3)} with 3 with {3: 2}
project {inf: (1) + (2)*i} with real
