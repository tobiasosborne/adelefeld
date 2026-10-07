#!exit 1
# SPEC 5, api-3c P1/P2/F2. Radius RU30 successor of 1 prints upward to two digits: 1.1.
char_unit char(q=5, n=2, s=(7) + (3)*i) with [1 mod 1]
char_unit_strict char(q=5, n=2, s=(0) + (0)*i) with [1 mod 1]
char_unit char(q=3, n=2, s=(0) + (0)*i) with [3 mod 4]
char_unit_strict char(q=3, n=2, s=(0) + (0)*i) with [-1]
char_unit char(q=5, n=2, s=(0) + (0)*i) with [2 mod 5]
char_unit_strict char(q=5, n=2, s=(0) + (0)*i) with [2 mod 5]
char_conj char(q=5, n=2, s=(1) + (-2)*i)
char_conj char(q=1, n=1, s=(0) + (3)*i)
char_unit 1 with [1]
char_unit char(q=5, n=2, s=(0) + (0)*i) with 1
char_unit char(q=5, n=2, s=(0) + (0)*i)
prec 2097153
