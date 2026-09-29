from fractions import Fraction as Q
from pathlib import Path

api=Path('docs/api-1f.md').read_text()
assert 't - u = (a - b u)/b' in api
a,b,u=Q(1),Q(3),Q(1)
assert a/b-u == (a-b*u)/b
print('F5 corrected_identity_checks=1 value=-2/3')
N,M,v,w=-5,1,0,0
K=min(N-w,N+M-2*w)
k=K-(v-w)
print(f'L4a zero_centre x=2^-5_Z2 y=1+2_Z2 K={K} claimed_modulus_exponent={k}')
assert k == -5
