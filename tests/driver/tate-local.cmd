#!exit 1
# Slice 5a: P9/P10. Hand expectations before driver implementation.
# At digits 5: 4/3=1.3333 +/- 3.4e-5; 1; 1/pi=0.31831 +/- 1.2e-7.
# Odd s=1 also gives 1/pi. Exact pole 0 is DOMAIN; a ball around it is ND.
prec 128
digits 5
tate_local ((2) + (0)*i ; 0) with 2 with ((1) + (0)*i ; 0) with char(q=1, n=1, s=(0) + (0)*i)
tate_local ((2) + (0)*i ; 0) with 2 with ((0) + (1)*i ; 0) with char(q=4, n=3, s=(0) + (0)*i)
tate_local ((2) + (0)*i ; 0) with real with ((1) + (0)*i ; 0) with char(q=1, n=1, s=(0) + (0)*i)
tate_local ((1) + (0)*i ; 0) with real with ((0) + (0)*i ; 0) with char(q=4, n=3, s=(0) + (0)*i)
tate_local ((0) + (0)*i ; 0) with 2 with ((1) + (0)*i ; 0) with char(q=1, n=1, s=(0) + (0)*i)
tate_local ((0 +/- 0.001) + (0)*i ; 0) with 2 with ((1) + (0)*i ; 0) with char(q=1, n=1, s=(0) + (0)*i)
tate_local ((-1) + (0)*i ; 0) with real with ((1) + (0)*i ; 0) with char(q=4, n=3, s=(0) + (0)*i)
