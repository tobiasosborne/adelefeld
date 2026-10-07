"""Hunt 1 edge cases: huge B, huge width, LONG_MAX limits, glued family; timed."""
import time
from common import *
cases = []
big = Piece(F(1,2), F(1,2), F(0), F(1, 10**100))
wide = Piece(F(0), F(2)**1000, F(0), F(1))
small = Piece(F(1,2), F(1,2), F(0), F(1))
for W in (10**6, 2**62, 2**63-1):
    for X, Y in ((big, small), (small, big), (wide, small)):
        cases.append('Q 0 %d %s %s' % (W, enc_class(0,[X]), enc_class(0,[Y])))
# glued family: [1/2,1] x (c mod H) vs [0,1/2] x (c+... ) meet only at (1,c)~(0,c-1)
for H in range(2, 9):
    for c in range(H):
        X = Piece(F(3,4), F(1,4), c, H)
        Y = Piece(F(1,4), F(1,4), (c-1) % H, H)
        Z = Piece(F(1,4), F(1,4), (c-2) % H, H)
        cases.append('Q 2 1000000 %s %s' % (enc_class(1,[X]), enc_class(1,[Y])))
        cases.append('Q 2 1000000 %s %s' % (enc_class(1,[X]), enc_class(1,[Z])))
t = time.time()
out, _ = run(cases)
print('elapsed', time.time() - t)
for c, o in zip(cases, out):
    print(o, c[:60])
