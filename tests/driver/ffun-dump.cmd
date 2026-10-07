#!exit 1
# Conventions 10.1/10.2. Expected dyadic tokens and value texts written before the driver run.
dump ffun(D=1, M=1; (0.5) + (-0.75)*i)
load adf1 Q ffun 1 1 1 -1 0 0 -3 -2 0 0
load adf1 Q ffun 0 1
load adf1 Q ffun 1 2 0 0 0 0 0 0 0 0
load adf1 Q ffun 1 1 0 5 0 0 0 0 0 0
load adf1 Q ffun 100001 1
