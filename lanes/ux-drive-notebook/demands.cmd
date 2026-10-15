# Missing record/reason/place fields: an ordinary result and two different DOMAIN causes.
roots_at 2 with 5 with 2
root_at 9 with 4 with 2 with 3
recover 146 mod 1000 with 30 with 15
# One-place commands have no result/status map.
exp_at 1 with real 5
exp_at 1 with 5
# Project output remains driver-specific, while value forms and dumps work.
project 22/7 with real 2 7
show [p=5: 3 + O(5^4)]
show {inf: 1.5; p=2: 1 + O(2^3)}
dump [p=5: 3 + O(5^4)]
dump {inf: 1.5; p=2: 1 + O(2^3)}
dump [5 mod 6]
dump (1 ; 1 * [1])
dump <1 ; [1]>
# No documented driver command for these demands; exploratory names are PARSE.
padic_digits [p=5: 3 + O(5^4)]
places 6/35
version
teichmuller 2 with 5
# Equality of finite enclosures is distinct from point comparison.
equal 280182 mod 390625 with 280182 mod 390625
compare 280182 mod 390625 with 280182 mod 390625
# A certificate is not part of recover's ordinary output.
recover 146 mod 1000 with 40 with 30
